# Estilo de documentacion del equipo (rubrica: "disene su propio estilo")

Todo archivo .c/.h abre con un bloque `/* ... */` con: proposito, uso, entradas, salidas, codigos de retorno.
Cada funcion publica lleva encima un comentario de 3 partes:

    /* Proposito : una linea.
     * Entradas  : nombre (tipo) - descripcion.
     * Salidas   : nombre (tipo) - descripcion / valor de retorno. */

Convenciones: nombres en espanol, `snake_case`, verbos para funciones (`probar_llave`), sustantivos para
variables (`llave_hallada`). Constantes en MAYUSCULAS. Comentarios `/* */` explican el "por que", no el "que".
Persona 2 y 3: usen el mismo formato para que el catalogo de funciones del informe salga directo de los comentarios.
