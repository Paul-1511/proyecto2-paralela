#!/usr/bin/env bash
# medir_secuencial.sh -- Corre bruteforce_seq N veces y reporta promedio y desviacion.
# Uso: scripts/medir_secuencial.sh archivo.bin "palabra clave" limite [repeticiones=10]
# Agrega cada corrida a resultados/secuencial.csv (compatible con la bitacora de Persona 3).
set -euo pipefail
if [ $# -lt 3 ]; then
  echo "Uso: $0 archivo.bin \"palabra clave\" limite [repeticiones=10]" >&2; exit 1
fi
ARCHIVO="$1"; CLAVE="$2"; LIMITE="$3"; REPS="${4:-10}"
BIN="./bin/bruteforce_seq"
[ -x "$BIN" ] || { echo "Falta compilar: make" >&2; exit 1; }
mkdir -p resultados
CSV="resultados/secuencial.csv"
[ -f "$CSV" ] || echo "fecha;corrida;archivo;limite;llave;iteraciones;tiempo_s" > "$CSV"

TIEMPOS=()
for i in $(seq 1 "$REPS"); do
  LINEA=$("$BIN" "$ARCHIVO" "$CLAVE" "$LIMITE" | grep '^CSV;' | sed 's/^CSV;//')
  echo "$(date -Iseconds);$i;$LINEA" >> "$CSV"
  TIEMPOS+=("$(echo "$LINEA" | awk -F';' '{print $5}')")
  echo "  corrida $i: $(echo "$LINEA" | awk -F';' '{print $5" s (llave "$3")"}')"
done
printf '%s\n' "${TIEMPOS[@]}" | awk '{s+=$1; ss+=$1*$1; n++} END {m=s/n; sd=(n>1)?sqrt((ss-n*m*m)/(n-1)):0; printf "Promedio: %.6f s  Desv. estandar: %.6f s  (n=%d)\n", m, sd, n}'
