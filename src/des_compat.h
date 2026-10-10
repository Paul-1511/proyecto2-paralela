/*
 * des_compat.h -- Reemplazo de <rpc/des_crypt.h> (ya no existe en glibc moderna)
 * Estilo de documentacion del equipo: ver docs/ESTILO_DOCUMENTACION.md
 *
 * Expone el mismo comportamiento que des_setparity() y ecb_crypt() del
 * codigo base, apoyandose en OpenSSL (libcrypto).
 */
#ifndef DES_COMPAT_H
#define DES_COMPAT_H

#define MODO_CIFRAR    1
#define MODO_DESCIFRAR 0

/* Ajusta paridad impar en cada uno de los 8 bytes de la llave. */
void des_setparity_compat(unsigned char llave8[8]);

/* DES en modo ECB sobre buffer (longitud multiplo de 8). Opera en sitio.
 * Retorna 0 si todo bien, -1 si la longitud no es multiplo de 8. */
int ecb_crypt_compat(const unsigned char llave8[8], unsigned char *buffer,
                     unsigned int longitud, int modo);
#endif
