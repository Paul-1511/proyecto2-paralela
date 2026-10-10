/* des_compat.c -- Implementacion con OpenSSL (-lcrypto). */
#include "des_compat.h"
#include <openssl/des.h>

void des_setparity_compat(unsigned char llave8[8]) {
    DES_set_odd_parity((DES_cblock *)llave8);
}

int ecb_crypt_compat(const unsigned char llave8[8], unsigned char *buffer,
                     unsigned int longitud, int modo) {
    DES_key_schedule ks;
    if (longitud % 8 != 0) return -1;
    DES_set_key_unchecked((const_DES_cblock *)llave8, &ks);
    for (unsigned int i = 0; i < longitud; i += 8) {
        DES_ecb_encrypt((const_DES_cblock *)(buffer + i), (DES_cblock *)(buffer + i),
                        &ks, modo == MODO_CIFRAR ? DES_ENCRYPT : DES_DECRYPT);
    }
    return 0;
}
