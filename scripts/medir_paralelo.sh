#!/usr/bin/env bash
# Uso: scripts/medir_paralelo.sh archivo.bin "frase" limite procesos distribucion [repeticiones]
set -euo pipefail
if [ "$#" -lt 5 ] || [ "$#" -gt 6 ]; then
  echo 'Uso: medir_paralelo.sh archivo.bin "frase" limite procesos bloques|ciclica [repeticiones=10]' >&2
  exit 1
fi
ARCHIVO=$1; CLAVE=$2; LIMITE=$3; PROCESOS=$4; MODO=$5; REPS=${6:-10}
case "$PROCESOS:$REPS:$MODO" in * ) ;; esac
[[ "$PROCESOS" =~ ^[1-9][0-9]*$ && "$REPS" =~ ^[1-9][0-9]*$ ]] || { echo 'Procesos y repeticiones deben ser enteros positivos' >&2; exit 1; }
[[ "$MODO" == bloques || "$MODO" == ciclica ]] || { echo 'Modo invalido' >&2; exit 1; }
[[ -x ./bin/bruteforce_mpi ]] || { echo 'Ejecuta make mpi primero' >&2; exit 1; }
mkdir -p resultados
CSV=resultados/paralelo.csv
[[ -f "$CSV" ]] || echo 'fecha;corrida;archivo;limite;llave;iteraciones;tiempo_s;procesos;distribucion;encontrada' > "$CSV"
for ((i=1;i<=REPS;i++)); do
  # 2 = llave no encontrada; es un resultado valido para pruebas de rangos.
  set +e
  SALIDA=$(mpirun -np "$PROCESOS" ./bin/bruteforce_mpi "$ARCHIVO" "$CLAVE" "$LIMITE" "$MODO" 2>&1)
  ESTADO=$?
  set -e
  if [[ $ESTADO -ne 0 && $ESTADO -ne 2 ]]; then printf '%s\n' "$SALIDA" >&2; exit "$ESTADO"; fi
  LINEA=$(printf '%s\n' "$SALIDA" | grep '^CSV;' | tail -n 1) || { printf '%s\n' "$SALIDA" >&2; exit 1; }
  printf '%s;%s;%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$i" "${LINEA#CSV;}" >> "$CSV"
  printf 'Corrida %s: %s s, llave %s\n' "$i" "$(echo "$LINEA" | cut -d';' -f6)" "$(echo "$LINEA" | cut -d';' -f4)"
done
awk -F';' -v p="$PROCESOS" -v m="$MODO" 'NR>1 && $8==p && $9==m {s+=$7; ss+=$7*$7; n++} END {if(n) printf "Acumulado para %s procesos (%s): n=%d, promedio=%.6f s, desviacion=%.6f s\n",p,m,n,s/n,(n>1?sqrt((ss-s*s/n)/(n-1)):0)}' "$CSV"
