/* Prueba: reproduce encrypt/decrypt del codigo base (con el shim) y compara con data/texto_base.bin */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rpc/des_crypt.h>

static void encrypt_base(long key, char *ciph, int len) {
    long k = 0;
    for (int i = 0; i < 8; ++i) { key <<= 1; k += (key & (0xFE << i * 8)); }
    des_setparity((char *)&k);
    ecb_crypt((char *)&k, ciph, len, DES_ENCRYPT);
}
static void decrypt_base(long key, char *ciph, int len) {
    long k = 0;
    for (int i = 0; i < 8; ++i) { key <<= 1; k += (key & (0xFE << i * 8)); }
    des_setparity((char *)&k);
    ecb_crypt((char *)&k, ciph, len, DES_DECRYPT);
}
int main(void) {
    FILE *f = fopen("data/texto_plano.txt", "rb"); if (!f) return 1;
    char plano[768] = {0}; size_t n = fread(plano, 1, 767, f); fclose(f);
    char esperado[768]; f = fopen("data/texto_base.bin", "rb"); if (!f) return 1;
    if (fread(esperado, 1, 768, f) != 768) return 1; fclose(f);
    char buf[768]; memcpy(buf, plano, 768);
    encrypt_base(8675309L, buf, 768);
    int igual = memcmp(buf, esperado, 768) == 0;
    decrypt_base(8675309L, buf, 768);
    int ida_vuelta = strstr(buf, "sudo make me a sandwich") != NULL;
    printf("texto_base.bin coincide con codigo base: %s | ida y vuelta: %s (n=%zu)\n",
           igual ? "SI" : "NO", ida_vuelta ? "SI" : "NO", n);
    return (igual && ida_vuelta) ? 0 : 1;
}
