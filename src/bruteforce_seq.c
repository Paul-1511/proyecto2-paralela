/*
 * bruteforce_seq.c -- Busqueda secuencial de la llave DES por fuerza bruta.
 * Uso: ./bin/bruteforce_seq archivo.bin "palabra clave" [limite=2^24]
 *   limite: numero (p. ej. 1000000) o potencia de 2 ("2^24"), maximo 2^56.
 * Salida: resumen legible + linea "CSV;archivo;limite;llave;iteraciones;tiempo_s"
 * Codigo de salida: 0 llave encontrada, 2 no encontrada, 1 error de uso/E-S.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "des_utils.h"

static double ahora_seg(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s archivo.bin \"palabra clave\" [limite=2^24]\n", argv[0]);
        return 1;
    }
    const char *ruta = argv[1], *clave = argv[2];
    uint64_t limite = 1ULL << 24;
    if (strlen(clave) == 0) { fprintf(stderr, "Error: palabra clave vacia\n"); return 1; }
    if (argc == 4 && parsear_limite(argv[3], &limite) != 0) {
        fprintf(stderr, "Error: limite invalido '%s' (use N o 2^N, N<=56)\n", argv[3]);
        return 1;
    }

    size_t longitud = 0;
    unsigned char *cifrado = leer_archivo(ruta, &longitud);
    if (!cifrado) return 1;
    if (longitud % 8 != 0) {
        fprintf(stderr, "Error: tamano %zu no es multiplo de 8 (no es un cifrado DES-ECB valido)\n", longitud);
        free(cifrado); return 1;
    }
    unsigned char *temporal = malloc(longitud + 1);
    if (!temporal) { free(cifrado); return 1; }

    uint64_t llave_hallada = 0, iteraciones = 0;
    int encontrada = 0;
    double t0 = ahora_seg();
    for (uint64_t k = 0; k < limite; ++k) {
        iteraciones++;
        if (probar_llave(k, cifrado, longitud, clave, temporal)) {
            llave_hallada = k; encontrada = 1; break;
        }
    }
    double t1 = ahora_seg();

    printf("Archivo: %s\nPalabra clave: \"%s\"\nLimite: %llu\n", ruta, clave, (unsigned long long)limite);
    if (encontrada) {
        printf("Llave encontrada: %llu\nTexto descifrado:\n%s\n", (unsigned long long)llave_hallada, temporal);
    } else {
        printf("Llave NO encontrada en [0, %llu)\n", (unsigned long long)limite);
    }
    printf("Iteraciones: %llu  Tiempo: %.6f s\n", (unsigned long long)iteraciones, t1 - t0);
    printf("CSV;%s;%llu;%llu;%llu;%.6f\n", ruta, (unsigned long long)limite,
           (unsigned long long)(encontrada ? llave_hallada : 0),
           (unsigned long long)iteraciones, t1 - t0);
    free(temporal); free(cifrado);
    return encontrada ? 0 : 2;
}
