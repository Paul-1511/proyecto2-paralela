# Proyecto 2 - MPI (CC3069): DES y version secuencial

## Estructura
    src/des_compat.[ch]     reemplazo de rpc/des_crypt.h (OpenSSL)
    src/des_utils.[ch]      derivar_llave_des, cifrar_des, descifrar_des, probar_llave, utilidades E/S
    src/bruteforce_seq.c    busqueda secuencial (version secuencial del proyecto)
    src/cifrar.c            cifra un .txt -> .bin con una llave
    scripts/medir_secuencial.sh   N corridas + promedio + CSV
    data/                   texto_plano.txt, texto.bin, texto_base.bin
    compat/rpc/des_crypt.h  shim para compilar el bruteforce.c base tal cual
    base/                   aqui va el bruteforce.c de la catedra
    tests/                  test_shim_base.c
    docs/                   informe_persona1.md, diagramas/*.mmd, ESTILO_DOCUMENTACION.md
    resultados/             secuencial.csv (lo genera el script)

## Requisitos
    sudo apt install build-essential libssl-dev

## Uso
    make                                   # compila (llave "corregida")
    make clean && make DERIVACION=base     # emula la derivacion del bruteforce.c original
    ./bin/cifrar data/texto_plano.txt data/texto.bin 8675309
    ./bin/bruteforce_seq data/texto.bin "sudo make me a sandwich" 2^24
    scripts/medir_secuencial.sh data/texto.bin "sudo make me a sandwich" 2^24 10

## Parte Persona 2: busqueda paralela MPI

**Entornos:** Ubuntu/Debian, WSL2 con Ubuntu/Debian, otras distribuciones Linux y macOS. El codigo usa C y MPI estandar; en Windows nativo se recomienda WSL2. `make` sigue compilando los programas secuenciales sin requerir MPI.

Dependencias en Ubuntu/Debian y WSL2:

```bash
sudo apt update
sudo apt install build-essential libssl-dev openmpi-bin libopenmpi-dev
```

En Fedora: `sudo dnf install gcc make openssl-devel openmpi openmpi-devel` (activar OpenMPI en PATH segun distribucion). En macOS con Homebrew: `brew install open-mpi openssl@3`; si OpenSSL no se encuentra automaticamente, indicar las rutas de Homebrew en `CFLAGS` y `LDLIBS` al ejecutar make.

```bash
make mpi
mpirun -np 4 ./bin/bruteforce_mpi data/texto.bin 'sudo make me a sandwich' 2^24 bloques
mpirun -np 4 ./bin/bruteforce_mpi data/texto.bin 'sudo make me a sandwich' 2^24 ciclica
scripts/medir_paralelo.sh data/texto.bin 'sudo make me a sandwich' 2^24 4 ciclica 10
```

El argumento `limite` es **exclusivo** (busca `0..limite-1`), opcional con valor predeterminado `2^24`; para el espacio completo usar `2^56` (puede tardar un tiempo prohibitivo). El ultimo argumento es `bloques` (predeterminado) o `ciclica`. Codigos de salida: 0 encontrada, 2 no encontrada, 1 error. La salida `CSV;...` sirve para bitacora automatizada. **No mezclar** binarios compilados con `DERIVACION=base` y cifrados con derivacion corregida.

Para compilar con OpenSSL instalado en un prefijo no estandar:

```bash
make mpi CFLAGS='-O2 -Wall -Wextra -Wno-deprecated-declarations -Isrc -I/RUTA/include' LDLIBS='-L/RUTA/lib -lcrypto'
```

Detalles del protocolo y diagrama: `docs/informe_persona2.md` y `docs/diagramas/mpi-flujo.mmd`. Las pruebas de rendimiento requieren una maquina con OpenMPI y deben registrarse antes de entregar el informe.

### Medicion corregida y pruebas defensivas (Persona 2)

```bash
make && make mpi
bash scripts/medir_paralelo.sh data/texto.bin 'sudo make me a sandwich' 2^24 4 ciclica 10
bash scripts/probar_mpi_defensivo.sh
```

El resumen del script de medicion corresponde **solo a la serie actual**, sin mezclar archivos, llaves ni series historicas. Los datos crudos se siguen anexando a `resultados/paralelo.csv`; para escribir en otro CSV, usar `CSV_RESULTADOS=resultados/otro.csv bash scripts/medir_paralelo.sh ...`. El script defensivo utiliza `timeout` (GNU coreutils): disponible normalmente en Linux y WSL; en macOS se puede instalar `coreutils` y adaptar `timeout` por `gtimeout`. Requiere `mpirun` y un binario MPI compilado. Estas pruebas se deben ejecutar en el equipo del estudiante para verificar comunicacion real.

## rendimiento y pruebas experimentales

(OpenMPI 5.0.11, L = 2^24, 10 reps por serie).
Llaves propias sobre `texto_plano.txt` (frase "sudo make me a sandwich"):
`facil.bin` (8388609), `media.bin` (10485760), `dificil.bin` (3687301).
Externos: `texto6.bin`, `texto7.bin` (llave 3412587), `textoequipo10.bin`
(llave 27124); `texto6.bin` no esta en [0, 2^24). Defensivas: 16/16 OK.

```bash
for P in 2 4 6; do
  for M in bloques ciclica; do
    bash scripts/medir_paralelo.sh data/facil.bin "sudo make me a sandwich" 2^24 $P $M 10
  done
done
# repetir con media.bin, dificil.bin, texto7.bin ("se le olvida como manejar") y textoequipo10.bin ("calabaza")
```

Resultados cíclica (S = Tseq/Tpar, E = S/P): fácil S(4)=3.677/E=91.9%,
media S(4)=3.633/E=90.8%, difícil S(4)=3.679/E=92.0%,
texto7 S(4)=3.649/E=91.2%, equipo10 S(4)=3.768/E=94.2%.
Velocidad secuencial ~185 000 llaves/s. Bloques es inconsistente por
posición de la llave (fácil ~0.002 s, speedup aparente ~15 000-30 000x;
difícil sin speedup hasta P=6); cíclica escala estable
(E ≈ 0.95/0.91/0.85 con P=2/4/6). Tablas y análisis en `proyecto2.pdf`
(sección 8 y anexos); gráficas en `resultados/grafica_*_vs_P.png`.
