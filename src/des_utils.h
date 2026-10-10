/*
 * des_utils.h -- Rutinas DES del proyecto (equivalentes a encrypt/decrypt/tryKey
 * del codigo base). Se renombraron para no chocar con encrypt() de <unistd.h>.
 *
 *   encrypt(key, ciph, len)  ->  cifrar_des(llave, buffer, longitud)
 *   decrypt(key, ciph, len)  ->  descifrar_des(llave, buffer, longitud)
 *   tryKey(key, ciph, len)   ->  probar_llave(llave, cifrado, longitud, palabra_clave, temporal)
 */
#ifndef DES_UTILS_H
#define DES_UTILS_H
#include <stdint.h>
#include <stddef.h>

#define LLAVE_BITS 56
#define LLAVE_MAX  (1ULL << LLAVE_BITS)

/* Expande una llave de 56 bits a 8 bytes DES (7 bits utiles por byte + paridad). */
void derivar_llave_des(uint64_t llave56, unsigned char salida8[8]);

/* Cifra/descifra 'longitud' bytes (multiplo de 8) en sitio. Retorna 0 si ok. */
int cifrar_des(uint64_t llave56, unsigned char *buffer, size_t longitud);
int descifrar_des(uint64_t llave56, unsigned char *buffer, size_t longitud);

/* Prueba una llave: copia 'cifrado' a 'temporal' (tamano longitud+1), descifra y
 * busca 'palabra_clave' con strstr. Retorna 1 si la encuentra, 0 si no. */
int probar_llave(uint64_t llave56, const unsigned char *cifrado, size_t longitud,
                 const char *palabra_clave, unsigned char *temporal);

/* Lee un archivo completo a memoria (malloc). Retorna NULL si falla. */
unsigned char *leer_archivo(const char *ruta, size_t *longitud);

/* Interpreta "12345" o "2^24" como limite superior (<= 2^56). Retorna 0 si ok. */
int parsear_limite(const char *texto, uint64_t *limite);
#endif
