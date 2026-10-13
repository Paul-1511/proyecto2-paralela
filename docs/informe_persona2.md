# Persona 2 - Paralelizacion y optimizacion con OpenMPI

## Arquitectura

El proceso 0 coordina y tambien busca llaves. Cada proceso mantiene una copia local del texto cifrado, recibida mediante `MPI_Bcast`. Se utiliza el mismo `probar_llave` y la misma derivacion DES que en la version secuencial. El limite de busqueda es **exclusivo**: `[0, limite)`.

### Distribucion original: bloques

Con `L` llaves y `P` procesos, `q=L/P` y `r=L%P`. El proceso `rank` recibe `q + (rank < r)` llaves, empezando en `q*rank + min(rank,r)`. Asi se cubren exactamente todas las llaves, incluso si `L` no es divisible por `P` o hay mas procesos que llaves.

### Distribucion optimizada: ciclica

El proceso `rank` comprueba `rank, rank+P, rank+2P, ...` mientras sea menor que `L`. La asignacion intercala llaves entre procesos y evita que un proceso empiece justo en un limite artificial de bloque. No garantiza mayor speedup en todos los casos; es indispensable comparar tiempos para distintas llaves.

## Flujo de comunicacion

1. Todos llaman a `MPI_Init`, `MPI_Comm_size` y `MPI_Comm_rank`.
2. El proceso 0 valida y lee el archivo binario. `MPI_Bcast` distribuye estado, longitud y bytes. `MPI_Allreduce` verifica que todos pudieron reservar memoria.
3. `MPI_Barrier` establece un punto de inicio de medicion; cada proceso usa `MPI_Wtime`.
4. El coordinador publica `MPI_Irecv` para reportes de cualquier trabajador. Cada trabajador publica `MPI_Irecv` para la orden de parada del coordinador.
5. Cada proceso comprueba lotes de 256 llaves; entre lotes usa `MPI_Test` para recibir novedades sin bloquear la busqueda.
6. Al hallar una llave o agotar su rango, cada trabajador envia un reporte (`HALLADA` con llave, o `AGOTADA`) mediante `MPI_Isend` y `MPI_Wait`. Solo envia un reporte.
7. El coordinador, al hallar una llave local o recibir `HALLADA`, difunde una orden de parada con `MPI_Isend`. Sigue atendiendo todos los reportes antes de finalizar; si nadie halla llave, tambien envia parada tras recibir todos los reportes.
8. Los trabajadores completan su recepcion de parada con `MPI_Wait` si aun estaba pendiente. El coordinador completa sus envios con `MPI_Waitall`.
9. `MPI_Reduce` suma intentos y obtiene el tiempo maximo entre procesos. El coordinador imprime el resultado y `MPI_Bcast` unifica el codigo de salida. Todos llaman `MPI_Finalize`.

**Importante:** no se asume que `MPI_Send` o `MPI_Isend` entregan inmediatamente mensajes; el protocolo utiliza operaciones no bloqueantes para evitar dependencias circulares durante la parada. `MPI_Send` es bloqueante y se describe en el catalogo porque lo pide la guia, aunque la implementacion usa `MPI_Isend` para los reportes y paradas.

## Catalogo de funciones MPI

| Funcion | Entradas | Salidas / efecto | Proposito |
|---|---|---|---|
| `MPI_Init` | `argc`, `argv` | Inicializa MPI | Debe llamarse antes de operaciones MPI. |
| `MPI_Comm_size` | comunicador, puntero a entero | numero de procesos | Conocer `P`. |
| `MPI_Comm_rank` | comunicador, puntero a entero | identificador `rank` | Identificar coordinador/trabajador. |
| `MPI_Irecv` | buffer, cantidad, tipo, origen, etiqueta, comunicador, solicitud | `MPI_Request` | Publicar recepcion no bloqueante. |
| `MPI_Send` | buffer, cantidad, tipo, destino, etiqueta, comunicador | codigo de error | Envio bloqueante: puede esperar hasta que MPI pueda completar el envio; no implica que el receptor ya proceso los datos. |
| `MPI_Isend` | mismos argumentos mas solicitud | `MPI_Request` | Iniciar envio no bloqueante. |
| `MPI_Test` | solicitud, indicador, estado | indica finalizacion | Comprobar recepcion sin detener la busqueda. |
| `MPI_Wait` | solicitud, estado | solicitud completada | Esperar envio o recepcion pendiente. |
| `MPI_Waitall` | cantidad, solicitudes, estados | todas completadas | Confirmar envios de parada. |
| `MPI_Bcast` | buffer, cantidad, tipo, raiz, comunicador | datos replicados | Distribuir entrada y resultado. |
| `MPI_Allreduce` | entrada, salida, cantidad, tipo, operador, comunicador | validacion global | Evitar que un proceso siga si otro fallo al reservar memoria. |
| `MPI_Reduce` | entrada, salida, cantidad, tipo, operador, raiz, comunicador | tiempo maximo / suma de intentos | Recolectar metricas. |
| `MPI_Wtime` | ninguna | segundos (double) | Medicion del tramo paralelo. |
| `MPI_Finalize` | ninguna | finaliza MPI | Cerrar el entorno MPI. |

## Programacion defensiva

Se validan cantidad de argumentos, palabra clave no vacia, limite dentro de `1..2^56`, modo de distribucion, lectura del archivo, longitud multiplo de 8, longitud compatible con contador MPI (`INT_MAX`) y reserva de memoria en todos los procesos. Se usan `uint64_t` para llaves y rangos. Se comprueban errores MPI y se utiliza `MPI_Abort` ante fallas irrecuperables de comunicacion. La entrada binaria no se mide con `strlen`.

## Bitacora y comparacion (pendiente de ejecucion real en OpenMPI)

El script `scripts/medir_paralelo.sh` genera `resultados/paralelo.csv`. Ejecutar al menos 10 repeticiones por texto y configuracion, usando los mismos textos y llaves para las dos distribuciones. Variar procesos (por ejemplo 2, 4, 6 y 8) y medir tiempo secuencial equivalente.

- Speedup: `S(P) = T_secuencial / T_paralelo(P)`.
- Eficiencia: `E(P) = S(P) / P`.
- Comparar llaves faciles, medianas y dificiles, incluyendo limites de bloques y casos sin solucion.
- Documentar procesador, memoria, sistema operativo, OpenMPI, OpenSSL, compilador y condiciones de ejecucion.
- **No hay mediciones MPI verificadas en este entorno.** Completar esta seccion con resultados reales; no inventar cifras.

## Observaciones y limites

- El cifrado es DES-ECB, sin autenticacion: encontrar una frase puede producir falsos positivos; usar frase suficientemente distintiva y validar el resultado.
- El texto de salida se muestra como cadena C, de acuerdo con el programa secuencial; el archivo cifrado debe proceder del cifrador del proyecto.
- La derivacion `DERIVACION=base` depende de comportamiento historico no portable del codigo de la catedra. Para intercambio entre Linux, WSL y macOS usar por defecto la derivacion corregida, identica en secuencial, cifrador y MPI.
- Los archivos del grupo y las llaves usadas en experimentos deben coincidir con la misma derivacion.

## Resultados experimentales aportados por el equipo (12 de octubre de 2026)

Se realizaron 10 repeticiones por configuracion para el archivo original (limite 2^24) y para tres archivos de 130 palabras cifrados con llaves 131072, 524288 y 917504 (limite 2^20). Los siguientes valores son promedios de tiempos en segundos comunicados por el integrante que ejecuto las pruebas en WSL. El entorno de elaboracion de este documento no dispone de OpenMPI para repetirlos de manera independiente.

| Llave | Secuencial | Bloques P=2 | Bloques P=4 | Bloques P=6 | Ciclica P=2 | Ciclica P=4 | Ciclica P=6 |
|---|---:|---:|---:|---:|---:|---:|---:|
| 131072 | 0.902551 | 1.092629 | 1.161915 | 1.271410 | 0.685458 | 0.335792 | 0.186246 |
| 524288 | 3.458601 | 0.001762 | 0.002848 | 1.394744 | 1.815569 | 0.937304 | 0.692590 |
| 917504 | 6.193471 | 2.693856 | 0.913540 | 0.351700 | 3.392926 | 1.791101 | 1.216907 |

Para la llave 524288, el modo por bloques con P=2 comienza a buscar justo en esa llave en el proceso 1: el speedup extremadamente alto es aparente, producto de la posicion de la solucion, no de una aceleracion proporcional del hardware. En los tres casos, el tiempo ciclico disminuye al aumentar P, pero no es siempre menor que el tiempo por bloques. Las conclusiones sobre consistencia requieren comparar varias llaves, no solo repetir una llave.

### Validacion defensiva pendiente de ejecutar en OpenMPI

El script `scripts/probar_mpi_defensivo.sh` genera un texto de prueba temporal y comprueba que el codigo MPI devuelve el resultado esperado con 1, 2 y 4 procesos para ambas distribuciones, con llave encontrada y no encontrada. Comprueba tambien archivo inexistente, palabra vacia, modo y limite invalidos. Cada invocacion tiene un tiempo maximo de 30 segundos para detectar bloqueos. Ejecutar con `bash scripts/probar_mpi_defensivo.sh` en Linux o WSL; en macOS instalar GNU coreutils para disponer de `timeout` (o adaptar el script a `gtimeout`). Conservar la salida de estas pruebas como evidencia; no se afirma aqui que hayan pasado.

### Correccion de la bitacora

`medir_paralelo.sh` ahora calcula promedio y desviacion sobre las repeticiones de su propia invocacion, en lugar de agrupar todos los registros historicos con el mismo numero de procesos y modo. El archivo `resultados/paralelo.csv` sigue siendo acumulativo, y los registros anteriores no se borran.
