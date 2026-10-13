#!/usr/bin/env bash
# Uso: scripts/medir_base.sh [repeticiones=10] [np ...]  (por defecto np = 1 2 4)
REPS=${1:-10}; shift || true
NPS=${@:-1 2 4}
CSV=resultados/base.csv
mkdir -p resultados
[ -f "$CSV" ] || echo "fecha;np;corrida;tiempo_s" > "$CSV"
for np in $NPS; do
  T=()
  for i in $(seq 1 "$REPS"); do
    s=$(date +%s%N)
    mpirun --oversubscribe -np "$np" ./bin/bruteforce_base_fix > /dev/null
    e=$(date +%s%N)
    t=$(awk -v a="$s" -v b="$e" 'BEGIN{printf "%.6f",(b-a)/1e9}')
    echo "$(date -Iseconds);$np;$i;$t" >> "$CSV"
    T+=("$t")
  done
  printf '%s\n' "${T[@]}" | awk -v np="$np" '{s+=$1;ss+=$1*$1;n++} END{m=s/n;sd=(n>1)?sqrt((ss-n*m*m)/(n-1)):0;printf "np=%d  promedio=%.4f s  desv=%.4f s  (n=%d)\n",np,m,sd,n}'
done
