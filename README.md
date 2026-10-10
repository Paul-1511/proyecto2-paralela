# Proyecto 2 - MPI (CC3069) - Parte Persona 1: DES y version secuencial

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

## Codigo base de la clase (bruteforce.c)
Copiar el archivo de la catedra a `base/bruteforce.c` y correr `make base`. El shim `compat/rpc/des_crypt.h`
reemplaza `<rpc/des_crypt.h>` sin editar el codigo (requiere `mpicc`/OpenMPI). `make test` verifica que
`data/texto_base.bin` es el cifrado que produce el codigo base. Con este binario se toman las "mediciones sobre codigo base".

## Datos para el foro de Canvas
- Palabra clave: `sudo make me a sandwich`
- Archivo: `data/texto.bin` (derivacion corregida) o `data/texto_base.bin` (derivacion del codigo base). Publiquen el que
  corresponda a la derivacion que use el bruteforce.c de la clase (ver nota en docs/informe_persona1.md, seccion 4).
- Llave (NO publicar): 8675309

## Para Persona 2 y 3
Reutilicen `probar_llave` en el loop paralelo y mantengan el formato CSV de `bruteforce_seq`
(`archivo;limite;llave;iteraciones;tiempo_s`) para que la bitacora de Persona 3 sea uniforme.
Todos deben compilar con la MISMA opcion `DERIVACION` para que los resultados sean comparables.
