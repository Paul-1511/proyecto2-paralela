#!/usr/bin/env bash
# Uso: bash scripts/medir_paralelo.sh archivo.bin "frase" limite procesos bloques|ciclica [repeticiones=10]
set -euo pipefail
if (( $# < 5 || $# > 6 )); then
  echo 'Uso: medir_paralelo.sh archivo.bin "frase" limite procesos bloques|ciclica [repeticiones=10]' >&2
  exit 1
fi
ARCHIVO=$1; CLAVE=$2; LIMITE=$3; PROCESOS=$4; MODO=$5; REPS=${6:-10}
[[ -f "$ARCHIVO" && -r "$ARCHIVO" ]] || { echo "Archivo no legible: $ARCHIVO" >&2; exit 1; }
[[ -n "$CLAVE" ]] || { echo 'La frase no puede estar vacia' >&2; exit 1; }
[[ "$PROCESOS" =~ ^[1-9][0-9]*$ && "$REPS" =~ ^[1-9][0-9]*$ ]] || { echo 'Procesos y repeticiones deben ser enteros positivos' >&2; exit 1; }
[[ "$MODO" == bloques || "$MODO" == ciclica ]] || { echo 'Modo invalido' >&2; exit 1; }
[[ -x ./bin/bruteforce_mpi ]] || { echo 'Ejecuta make mpi primero' >&2; exit 1; }
command -v mpirun >/dev/null || { echo 'mpirun no encontrado' >&2; exit 1; }
mkdir -p resultados
CSV=${CSV_RESULTADOS:-resultados/paralelo.csv}
if [[ ! -s "$CSV" ]]; then
  echo 'fecha;corrida;archivo;limite;llave;iteraciones;tiempo_s;procesos;distribucion;encontrada' > "$CSV"
fi
# Solo se promedian las repeticiones de ESTA invocacion; nunca se mezclan otras llaves.
TEMP=$(mktemp)
trap 'rm -f "$TEMP"' EXIT
for ((i=1;i<=REPS;i++)); do
  set +e
  SALIDA=$(mpirun -np "$PROCESOS" ./bin/bruteforce_mpi "$ARCHIVO" "$CLAVE" "$LIMITE" "$MODO" 2>&1)
  ESTADO=$?
  set -e
  if [[ $ESTADO -ne 0 && $ESTADO -ne 2 ]]; then
    printf 'Error en corrida %d (codigo %d):\n%s\n' "$i" "$ESTADO" "$SALIDA" >&2
    exit "$ESTADO"
  fi
  LINEA=$(printf '%s\n' "$SALIDA" | grep '^CSV;' | tail -n 1) || { printf '%s\n' "$SALIDA" >&2; exit 1; }
  IFS=';' read -r pref archivo limite llave iteraciones tiempo procesos modo encontrada <<< "$LINEA"
  [[ "$pref" == CSV && "$archivo" == "$ARCHIVO" && "$procesos" == "$PROCESOS" && "$modo" == "$MODO" && "$tiempo" =~ ^[0-9]+([.][0-9]+)?$ && "$encontrada" =~ ^[01]$ ]] || { echo "CSV inesperado: $LINEA" >&2; exit 1; }
  [[ "$ESTADO" -eq 0 && "$encontrada" == 1 || "$ESTADO" -eq 2 && "$encontrada" == 0 ]] || { echo "Estado y resultado inconsistentes: $LINEA" >&2; exit 1; }
  printf '%s;%s;%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$i" "${LINEA#CSV;}" >> "$CSV"
  printf '%s\n' "$tiempo" >> "$TEMP"
  printf 'Corrida %d: %s s, llave %s, encontrada=%s\n' "$i" "$tiempo" "$llave" "$encontrada"
done
awk -v p="$PROCESOS" -v m="$MODO" -v a="$ARCHIVO" '{s+=$1; ss+=$1*$1; n++} END {if(n) {v=(n>1?(ss-s*s/n)/(n-1):0); if(v<0)v=0; printf "Esta serie: archivo=%s, procesos=%s, modo=%s, n=%d, promedio=%.6f s, desviacion=%.6f s\n",a,p,m,n,s/n,sqrt(v)}}' "$TEMP"
