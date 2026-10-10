/*
 * cifrar.c -- Cifra un .txt con DES-ECB usando una llave de hasta 56 bits.
 * Uso: ./bin/cifrar entrada.txt salida.bin LLAVE
 * Rellena con '\0' hasta multiplo de 8 (minimo un '\0') para que strstr termine bien.
 * Advierte si el texto supera 350 palabras (limite de Canvas).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "des_utils.h"

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "Uso: %s entrada.txt salida.bin LLAVE\n", argv[0]); return 1; }
    uint64_t llave;
    if (parsear_limite(argv[3], &llave) != 0) { fprintf(stderr, "Error: llave invalida (1..2^56)\n"); return 1; }

    size_t n = 0;
    unsigned char *txt = leer_archivo(argv[1], &n);
    if (!txt) return 1;

    size_t palabras = 0; int dentro = 0;
    for (size_t i = 0; i < n; ++i) {
        if (isspace(txt[i])) dentro = 0; else if (!dentro) { dentro = 1; palabras++; }
    }
    if (palabras > 350) fprintf(stderr, "Aviso: %zu palabras (maximo 350 para Canvas)\n", palabras);

    size_t relleno = 8 - (n % 8);           /* 1..8 bytes '\0' */
    size_t total = n + relleno;
    unsigned char *buf = calloc(total, 1);
    if (!buf) { free(txt); return 1; }
    memcpy(buf, txt, n);

    if (cifrar_des(llave, buf, total) != 0) { fprintf(stderr, "Error al cifrar\n"); return 1; }
    FILE *f = fopen(argv[2], "wb");
    if (!f || fwrite(buf, 1, total, f) != total) { fprintf(stderr, "Error al escribir '%s'\n", argv[2]); return 1; }
    fclose(f);
    printf("OK: %zu palabras, %zu bytes de texto, %zu bytes cifrados -> %s\n", palabras, n, total, argv[2]);
    free(buf); free(txt);
    return 0;
}
