/* des_utils.c -- Implementacion de las rutinas DES y utilidades de E/S. */
#include "des_utils.h"
#include "des_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void derivar_llave_des(uint64_t llave56, unsigned char salida8[8]) {
    /* Byte i recibe los bits [7i, 7i+6] de la llave en sus 7 bits altos. */
    uint64_t k = 0, aux = llave56;
    for (int i = 0; i < 8; ++i) {
        aux <<= 1;
#ifdef DERIVACION_BASE
        /* Emula el codigo base (bruteforce.c) en x86-64: la mascara (0xFE << i*8)
         * se calcula con int de 32 bits, el conteo de desplazamiento se toma mod 32
         * y el resultado se extiende con signo a long. Es comportamiento indefinido
         * en C, pero es lo que produce gcc en x86-64. Solo para interoperar. */
        uint64_t mascara = (uint64_t)(int64_t)(int32_t)(0xFEu << ((i * 8) % 32));
        k += (aux & mascara);
#else
        /* Derivacion corregida: mascara de 64 bits sin signo. */
        k |= (aux & (0xFEULL << (i * 8)));
#endif
    }
    for (int i = 0; i < 8; ++i) salida8[i] = (unsigned char)((k >> (8 * i)) & 0xFF);
    des_setparity_compat(salida8);
}

int cifrar_des(uint64_t llave56, unsigned char *buffer, size_t longitud) {
    unsigned char llave8[8];
    derivar_llave_des(llave56, llave8);
    return ecb_crypt_compat(llave8, buffer, (unsigned int)longitud, MODO_CIFRAR);
}

int descifrar_des(uint64_t llave56, unsigned char *buffer, size_t longitud) {
    unsigned char llave8[8];
    derivar_llave_des(llave56, llave8);
    return ecb_crypt_compat(llave8, buffer, (unsigned int)longitud, MODO_DESCIFRAR);
}

int probar_llave(uint64_t llave56, const unsigned char *cifrado, size_t longitud,
                 const char *palabra_clave, unsigned char *temporal) {
    memcpy(temporal, cifrado, longitud);   /* no destruir el cifrado original */
    temporal[longitud] = '\0';             /* terminador para strstr */
    descifrar_des(llave56, temporal, longitud);
    return strstr((const char *)temporal, palabra_clave) != NULL;
}

unsigned char *leer_archivo(const char *ruta, size_t *longitud) {
    FILE *f = fopen(ruta, "rb");
    if (!f) { fprintf(stderr, "Error: no se pudo abrir '%s': %s\n", ruta, strerror(errno)); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long tam = ftell(f);
    rewind(f);
    if (tam <= 0) { fprintf(stderr, "Error: '%s' esta vacio\n", ruta); fclose(f); return NULL; }
    unsigned char *datos = malloc((size_t)tam + 1);
    if (!datos) { fclose(f); return NULL; }
    if (fread(datos, 1, (size_t)tam, f) != (size_t)tam) {
        fprintf(stderr, "Error: lectura incompleta de '%s'\n", ruta);
        free(datos); fclose(f); return NULL;
    }
    datos[tam] = '\0';
    fclose(f);
    *longitud = (size_t)tam;
    return datos;
}

int parsear_limite(const char *texto, uint64_t *limite) {
    char *fin = NULL;
    if (strncmp(texto, "2^", 2) == 0) {
        long e = strtol(texto + 2, &fin, 10);
        if (*fin != '\0' || e < 1 || e > LLAVE_BITS) return -1;
        *limite = 1ULL << e;
        return 0;
    }
    errno = 0;
    unsigned long long v = strtoull(texto, &fin, 10);
    if (errno != 0 || *fin != '\0' || v == 0 || v > LLAVE_MAX) return -1;
    *limite = v;
    return 0;
}
