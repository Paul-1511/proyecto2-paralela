#!/usr/bin/env bash
# Casos de regresion para errores de entrada, sin solucion y comunicacion MPI.
set -euo pipefail
cd "$(dirname "$0")/.."
[[ -x bin/bruteforce_mpi && -x bin/cifrar ]] || { echo 'Ejecuta make && make mpi' >&2; exit 1; }
command -v timeout >/dev/null || { echo 'Se requiere timeout (coreutils)' >&2; exit 1; }
TEMP=$(mktemp -d)
trap 'rm -rf "$TEMP"' EXIT
printf 'MARCADOR_MPI_PRUEBA_2026 texto de verificacion\n' > "$TEMP/plano.txt"
./bin/cifrar "$TEMP/plano.txt" "$TEMP/cifrado.bin" 1234 >/dev/null
ok=0
probar() {
  local nombre=$1 esperado=$2 patron=$3
  shift 3
  local salida estado
  set +e
  salida=$(timeout 30s "$@" 2>&1)
  estado=$?
  set -e
  if [[ "$estado" -ne "$esperado" ]] || ! grep -q -- "$patron" <<< "$salida"; then
    printf 'FALLO %s: codigo=%d, esperado=%d\n%s\n' "$nombre" "$estado" "$esperado" "$salida" >&2
    exit 1
  fi
  echo "OK: $nombre"
  ok=$((ok+1))
}
for n in 1 2 4; do
  for modo in bloques ciclica; do
    probar "encontrada n=$n modo=$modo" 0 'Llave encontrada: 1234' mpirun -np "$n" ./bin/bruteforce_mpi "$TEMP/cifrado.bin" MARCADOR_MPI_PRUEBA_2026 2048 "$modo"
    probar "no encontrada n=$n modo=$modo" 2 'Llave NO encontrada' mpirun -np "$n" ./bin/bruteforce_mpi "$TEMP/cifrado.bin" MARCADOR_MPI_PRUEBA_2026 100 "$modo"
  done
done
probar 'archivo inexistente' 1 'Error:' mpirun -np 2 ./bin/bruteforce_mpi "$TEMP/noexiste.bin" MARCADOR_MPI_PRUEBA_2026 2048 bloques
probar 'palabra vacia' 1 'Uso:' mpirun -np 2 ./bin/bruteforce_mpi "$TEMP/cifrado.bin" '' 2048 bloques
probar 'modo invalido' 1 'Uso:' mpirun -np 2 ./bin/bruteforce_mpi "$TEMP/cifrado.bin" MARCADOR_MPI_PRUEBA_2026 2048 otro
probar 'limite invalido' 1 'Uso:' mpirun -np 2 ./bin/bruteforce_mpi "$TEMP/cifrado.bin" MARCADOR_MPI_PRUEBA_2026 0 bloques
printf 'Pruebas defensivas completadas: %d\n' "$ok"
