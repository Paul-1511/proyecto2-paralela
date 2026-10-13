/* Busqueda DES distribuida. Protocolo: cada trabajador reporta una sola vez
 * (HALLADA/AGOTADA) al coordinador; este difunde PARAR al hallar solucion.
 * Recepciones no bloqueantes y comprobacion periodica con MPI_Test.
 */
#include <mpi.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "des_utils.h"

enum { TAG_REPORTE = 10, TAG_PARAR = 11 };
enum { AGOTADA = 0, HALLADA = 1 };
#define LOTE 256U

static void ayuda(const char *exe) {
    fprintf(stderr, "Uso: %s archivo.bin \"palabra clave\" [limite=2^24] [bloques|ciclica]\n", exe);
}

/* Rangos semiabiertos, calculados sin multiplicaciones que desborden. */
static void rango(uint64_t limite, int n, int id, uint64_t *inicio, uint64_t *fin) {
    uint64_t q = limite / (uint64_t)n, r = limite % (uint64_t)n;
    *inicio = q * (uint64_t)id + (uint64_t)(id < (int)r ? id : (int)r);
    *fin = *inicio + q + (uint64_t)((uint64_t)id < r);
}

static void mpi_ok(int codigo, MPI_Comm comm, const char *operacion) {
    if (codigo != MPI_SUCCESS) {
        fprintf(stderr, "Error MPI en %s\n", operacion);
        MPI_Abort(comm, 1);
    }
}

int main(int argc, char **argv) {
    MPI_Comm comm = MPI_COMM_WORLD;
    int n, id;
    mpi_ok(MPI_Init(&argc, &argv), comm, "MPI_Init");
    mpi_ok(MPI_Comm_size(comm, &n), comm, "MPI_Comm_size");
    mpi_ok(MPI_Comm_rank(comm, &id), comm, "MPI_Comm_rank");
    mpi_ok(MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN), comm, "MPI_Comm_set_errhandler");

    int valido = 1, ciclica = 0;
    uint64_t limite = 1ULL << 24;
    if (argc < 3 || argc > 5 || !argv[1][0] || !argv[2][0]) valido = 0;
    if (valido && argc >= 4 && parsear_limite(argv[3], &limite)) valido = 0;
    if (valido && argc == 5) {
        if (!strcmp(argv[4], "ciclica")) ciclica = 1;
        else if (strcmp(argv[4], "bloques")) valido = 0;
    }
    if (!valido) {
        if (id == 0) ayuda(argv[0]);
        MPI_Finalize();
        return 1;
    }

    /* Solo el coordinador abre el archivo; envia bytes binarios sin strlen. */
    size_t longitud = 0;
    unsigned char *cifrado = NULL;
    if (id == 0) {
        cifrado = leer_archivo(argv[1], &longitud);
        if (!cifrado || !longitud || longitud % 8 || longitud > INT_MAX || longitud == SIZE_MAX)
            valido = 0;
        if (!valido) fprintf(stderr, "Error: archivo inexistente, vacio, demasiado grande o no multiplo de 8 bytes\n");
    }
    mpi_ok(MPI_Bcast(&valido, 1, MPI_INT, 0, comm), comm, "Bcast validacion");
    if (!valido) {
        free(cifrado);
        MPI_Finalize();
        return 1;
    }
    unsigned long long tam = (unsigned long long)longitud;
    mpi_ok(MPI_Bcast(&tam, 1, MPI_UNSIGNED_LONG_LONG, 0, comm), comm, "Bcast longitud");
    longitud = (size_t)tam;
    if (id != 0) cifrado = malloc(longitud + 1);
    int memoria = cifrado != NULL;
    int todos = 0;
    mpi_ok(MPI_Allreduce(&memoria, &todos, 1, MPI_INT, MPI_MIN, comm), comm, "Allreduce memoria");
    if (!todos) {
        if (id == 0) fprintf(stderr, "Error: memoria insuficiente\n");
        free(cifrado);
        MPI_Finalize();
        return 1;
    }
    mpi_ok(MPI_Bcast(cifrado, (int)longitud, MPI_BYTE, 0, comm), comm, "Bcast cifrado");
    unsigned char *temporal = malloc(longitud + 1);
    memoria = temporal != NULL;
    mpi_ok(MPI_Allreduce(&memoria, &todos, 1, MPI_INT, MPI_MIN, comm), comm, "Allreduce temporal");
    if (!todos) {
        if (id == 0) fprintf(stderr, "Error: memoria insuficiente\n");
        free(temporal); free(cifrado);
        MPI_Finalize();
        return 1;
    }

    uint64_t inicio = 0, fin = limite;
    if (!ciclica) rango(limite, n, id, &inicio, &fin);
    else inicio = (uint64_t)id;
    uint64_t k = inicio, intentos = 0, solucion = 0;
    int encontrada = 0, detener = 0, reportado = 0;
    /* Mensaje: [tipo, llave]; el proceso 0 escucha reportes de cualquier nodo. */
    unsigned long long reporte[2] = {0, 0}, entrada[2] = {0, 0};
    int orden_parar = 0;
    MPI_Request peticion = MPI_REQUEST_NULL;
    if (id == 0 && n > 1)
        mpi_ok(MPI_Irecv(entrada, 2, MPI_UNSIGNED_LONG_LONG, MPI_ANY_SOURCE,
                         TAG_REPORTE, comm, &peticion), comm, "Irecv reportes");
    if (id != 0)
        mpi_ok(MPI_Irecv(&orden_parar, 1, MPI_INT, 0, TAG_PARAR,
                         comm, &peticion), comm, "Irecv parada");

    mpi_ok(MPI_Barrier(comm), comm, "Barrier inicio");
    double t0 = MPI_Wtime();
    int terminados = 0;
    /* El coordinador continua recibiendo informes hasta saber que todos terminaron.
     * Tras encontrar llave, avisa a los demas con envios no bloqueantes. */
    MPI_Request *avisos = NULL;
    int avisados = 0;
    if (id == 0 && n > 1) {
        avisos = calloc((size_t)(n - 1), sizeof(*avisos));
        if (!avisos) MPI_Abort(comm, 1);
    }
    while (1) {
        if (id == 0 && n > 1 && terminados < n - 1) {
            int listo = 0;
            MPI_Status estado;
            mpi_ok(MPI_Test(&peticion, &listo, &estado), comm, "Test reportes");
            if (listo) {
                ++terminados;
                if (entrada[0] == HALLADA && !encontrada) {
                    encontrada = 1;
                    solucion = (uint64_t)entrada[1];
                    detener = 1;
                }
                if (terminados < n - 1)
                    mpi_ok(MPI_Irecv(entrada, 2, MPI_UNSIGNED_LONG_LONG,
                            MPI_ANY_SOURCE, TAG_REPORTE, comm, &peticion), comm, "Irecv siguiente");
            }
        } else if (id != 0 && !detener) {
            int listo = 0;
            mpi_ok(MPI_Test(&peticion, &listo, MPI_STATUS_IGNORE), comm, "Test parada");
            if (listo) detener = 1;
        }
        if (id == 0 && detener && !avisados) {
            for (int p = 1; p < n; ++p)
                mpi_ok(MPI_Isend(&orden_parar, 1, MPI_INT, p, TAG_PARAR,
                                 comm, &avisos[p - 1]), comm, "Isend parada");
            avisados = 1;
        }
        if (!detener && !reportado && k < (ciclica ? limite : fin)) {
            for (unsigned j = 0; j < LOTE && k < (ciclica ? limite : fin); ++j) {
                ++intentos;
                if (probar_llave(k, cifrado, longitud, argv[2], temporal)) {
                    encontrada = 1; solucion = k; detener = 1;
                    break;
                }
                if (ciclica) {
                    if (limite - k <= (uint64_t)n) k = limite;
                    else k += (uint64_t)n;
                } else ++k;
            }
        }
        if (id != 0 && !reportado && (detener || k >= (ciclica ? limite : fin))) {
            reporte[0] = encontrada ? HALLADA : AGOTADA;
            reporte[1] = (unsigned long long)solucion;
            /* Un solo reporte por trabajador. Se utiliza Isend para no bloquear
             * mientras el coordinador difunde la orden de detener. */
            MPI_Request envio;
            mpi_ok(MPI_Isend(reporte, 2, MPI_UNSIGNED_LONG_LONG, 0, TAG_REPORTE,
                             comm, &envio), comm, "Isend reporte");
            mpi_ok(MPI_Wait(&envio, MPI_STATUS_IGNORE), comm, "Wait reporte");
            reportado = 1;
        }
        if (id != 0 && reportado) {
            /* Espera la orden final aun cuando haya agotado su rango. */
            if (peticion != MPI_REQUEST_NULL) mpi_ok(MPI_Wait(&peticion, MPI_STATUS_IGNORE), comm, "Wait parada");
            break;
        }
        if (id == 0 && (detener || k >= (ciclica ? limite : fin))) {
            /* Si el coordinador agoto su rango, espera mensajes pendientes.
             * Si otro proceso encontro llave, sigue drenando reportes. */
            if (!detener && terminados < n - 1) {
                MPI_Status estado;
                mpi_ok(MPI_Wait(&peticion, &estado), comm, "Wait reporte pendiente");
                ++terminados;
                if (entrada[0] == HALLADA) {
                    encontrada = 1; solucion = (uint64_t)entrada[1]; detener = 1;
                }
                if (terminados < n - 1)
                    mpi_ok(MPI_Irecv(entrada, 2, MPI_UNSIGNED_LONG_LONG,
                            MPI_ANY_SOURCE, TAG_REPORTE, comm, &peticion), comm, "Irecv pendiente");
            }
            if (terminados == n - 1) break;
        }
    }
    /* Si nadie encontro llave, aun es necesario enviar PARAR a los trabajadores. */
    if (id == 0 && n > 1) {
        if (!avisados) {
            for (int p = 1; p < n; ++p)
                mpi_ok(MPI_Isend(&orden_parar, 1, MPI_INT, p, TAG_PARAR,
                                 comm, &avisos[p - 1]), comm, "Isend fin");
        }
        mpi_ok(MPI_Waitall(n - 1, avisos, MPI_STATUSES_IGNORE), comm, "Waitall avisos");
    }
    double local = MPI_Wtime() - t0, total = 0;
    unsigned long long local_intentos = (unsigned long long)intentos, global_intentos = 0;
    mpi_ok(MPI_Reduce(&local, &total, 1, MPI_DOUBLE, MPI_MAX, 0, comm), comm, "Reduce tiempo");
    mpi_ok(MPI_Reduce(&local_intentos, &global_intentos, 1, MPI_UNSIGNED_LONG_LONG,
                      MPI_SUM, 0, comm), comm, "Reduce intentos");
    /* Si un trabajador encontro llave, el coordinador la conoce por el reporte. */
    if (id == 0) {
        printf("Archivo: %s\nPalabra clave: \"%s\"\nDistribucion: %s\nProcesos: %d\nLimite: %" PRIu64 "\n",
               argv[1], argv[2], ciclica ? "ciclica" : "bloques", n, limite);
        if (encontrada) {
            descifrar_des(solucion, cifrado, longitud);
            printf("Llave encontrada: %" PRIu64 "\nTexto descifrado:\n%s\n", solucion, (char *)cifrado);
        } else printf("Llave NO encontrada en [0, %" PRIu64 ")\n", limite);
        printf("Iteraciones: %llu  Tiempo: %.6f s\n", global_intentos, total);
        printf("CSV;%s;%llu;%llu;%llu;%.6f;%d;%s;%d\n", argv[1],
               (unsigned long long)limite, (unsigned long long)(encontrada ? solucion : 0),
               global_intentos, total, n, ciclica ? "ciclica" : "bloques", encontrada);
    }
    mpi_ok(MPI_Bcast(&encontrada, 1, MPI_INT, 0, comm), comm, "Bcast resultado");
    free(avisos); free(temporal); free(cifrado);
    mpi_ok(MPI_Finalize(), comm, "MPI_Finalize");
    return encontrada ? 0 : 2;
}
