# Informe - Parte Persona 1: DES y version secuencial
(Pegar en el informe final con formato UVG: caratula, indice, introduccion, cuerpo, citas, conclusiones, apendice, >= 3 referencias.)

## 1. Antecedentes numericos y conceptuales (1-2 paginas)
- [ ] Que es DES: cifrado de bloque simetrico, bloque de 64 bits, llave de 64 bits (56 utiles + 8 de paridad), 16 rondas Feistel, subllaves de 48 bits, 8 cajas S (6 -> 4 bits). _(citar FIPS 46-3 / fuente confiable)_
- [ ] Pasos para cifrar: IP -> 16 rondas (E, XOR Ki, cajas S, P, XOR, intercambio) -> IP^-1.
- [ ] Pasos para descifrar: mismo algoritmo con subllaves en orden inverso K16..K1.
- [ ] Modo ECB y relleno con '\0' (por que se usa aqui y sus limitaciones: bloques iguales -> cifrados iguales).
- [ ] Fuerza bruta: espacio de 2^56 = 72,057,594,037,927,936 llaves. Criterio de exito: `strstr` de la palabra clave.
- [ ] Numeros propios (reemplazar con SUS mediciones): velocidad secuencial = ____ llaves/s -> tiempo para 2^56 = ____ anios.
      (En una prueba de referencia con texto.bin salieron ~130,000 llaves/s, es decir ~17,500 anios para 2^56; midan en su maquina.)

## 2. Diagrama de flujo de DES
- [ ] Usar `docs/diagramas/des_flujo.mmd` (renderizar en https://mermaid.live o con `mmdc`) y limpiarlo/dibujarlo a su gusto. Cabe en 1 pagina adicional.

## 3. Explicacion de rutinas (con diagramas)
- [ ] `decrypt/encrypt` -> `descifrar_des/cifrar_des` + `derivar_llave_des` + `ecb_crypt` (ver `rutinas.mmd`)
- [ ] `tryKey` -> `probar_llave`
- [ ] `memcpy`: copia `n` bytes de origen a destino (no maneja solapamiento; por que se copia antes de descifrar)
- [ ] `strstr`: busca subcadena terminada en '\0'; por eso el `\0` al final del buffer. Costo O(n*m) en el peor caso.

## 4. Retos encontrados (aporte de Persona 1)
- `rpc/des_crypt.h` ya no existe en glibc moderna -> se reemplazo con OpenSSL (`src/des_compat.*`).
- `encrypt()` choca con el prototipo de `<unistd.h>` -> funciones renombradas.
- Derivacion de llave del codigo base: la mascara `(0xFE << i*8)` usa `int` de 32 bits; para i >= 3 hay desbordamiento/signo y para i >= 4 comportamiento indefinido. En x86-64 produce una llave DES distinta a la "esperada" (ver `make DERIVACION=base` y `derivar_llave_des`). Consecuencia: los textos cifrados entre grupos solo se descifran si ambos usan la misma derivacion.
- Relleno con '\0' obligatorio para que `strstr` termine; si el texto es multiplo de 8 se agrega un bloque extra.
- Probar llaves en orden y el espacio 2^56 es inviable: se mide con limite reducido (p. ej. 2^24).

## 5. Anexo 1 - Catalogo de funciones (Persona 1)
| Funcion | Entradas | Salidas | Descripcion |
|---|---|---|---|
| `derivar_llave_des` | `llave56` (uint64_t) | `salida8` (unsigned char[8]) | Expande 56 bits a 8 bytes DES y ajusta paridad. |
| `cifrar_des` | `llave56`, `buffer` (unsigned char*), `longitud` (size_t, mult. de 8) | `buffer` cifrado en sitio; int 0 ok / -1 error | DES-ECB en modo cifrar. Equivale a `encrypt`. |
| `descifrar_des` | igual | `buffer` descifrado en sitio; int | DES-ECB en modo descifrar. Equivale a `decrypt`. |
| `probar_llave` | `llave56`, `cifrado`, `longitud`, `palabra_clave` (const char*), `temporal` (longitud+1) | int 1/0 | Copia, descifra y busca la palabra clave. Equivale a `tryKey`. |
| `leer_archivo` | `ruta` (const char*) | `longitud` (size_t*); buffer malloc o NULL | Lee un archivo completo a memoria. |
| `parsear_limite` | `texto` (const char*) | `limite` (uint64_t*); int 0 ok | Interpreta `N` o `2^N` (max 2^56). |
| `ahora_seg` (bruteforce_seq.c) | - | double | Reloj monotonico en segundos. |
| `main` (bruteforce_seq.c) | argv: archivo, palabra clave, limite | codigo 0/1/2 + linea CSV | Busqueda secuencial de la llave. |
| `main` (cifrar.c) | argv: txt, bin, llave | archivo .bin | Cifra un texto (relleno '\0'). |

## 6. Anexo 2 - Bitacora secuencial
- [ ] Correr `scripts/medir_secuencial.sh data/texto.bin "sudo make me a sandwich" 2^24 10` (>= 10 corridas) y pegar tabla + promedio.
- [ ] Entregar `resultados/secuencial.csv` a Persona 3.
