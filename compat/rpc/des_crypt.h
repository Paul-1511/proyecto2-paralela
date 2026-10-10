/*
 * compat/rpc/des_crypt.h -- Shim de <rpc/des_crypt.h> para glibc moderna.
 * Permite compilar el bruteforce.c base SIN modificarlo:
 *     mpicc -Icompat base/bruteforce.c -o bin/bruteforce_base -lcrypto
 * Implementa des_setparity() y ecb_crypt() sobre OpenSSL. Los macros DES_ENCRYPT /
 * DES_DECRYPT son los de OpenSSL (1 = cifrar, 0 = descifrar); como el codigo base
 * solo usa los nombres, el comportamiento es el esperado.
 */
#ifndef RPC_DES_CRYPT_SHIM_H
#define RPC_DES_CRYPT_SHIM_H
#include <openssl/des.h>

#ifndef DES_HW
#define DES_HW 2
#endif
#ifndef DES_SW
#define DES_SW 4
#endif
#define DESERR_NONE 0
#define DESERR_BADPARAM 3

static inline void des_setparity(char *llave8) {
    DES_set_odd_parity((DES_cblock *)llave8);
}

/* modo: DES_ENCRYPT o DES_DECRYPT (se ignoran los bits DES_HW/DES_SW). */
static inline int ecb_crypt(char *llave8, char *datos, unsigned longitud, unsigned modo) {
    DES_key_schedule ks;
    if (longitud % 8 != 0) return DESERR_BADPARAM;
    DES_set_key_unchecked((const_DES_cblock *)llave8, &ks);
    for (unsigned i = 0; i < longitud; i += 8)
        DES_ecb_encrypt((const_DES_cblock *)(datos + i), (DES_cblock *)(datos + i), &ks,
                        (modo & 1) ? DES_ENCRYPT : DES_DECRYPT);
    return DESERR_NONE;
}
#endif
