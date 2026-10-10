CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -Wno-deprecated-declarations -Isrc
LDLIBS  = -lcrypto
# make DERIVACION=base  -> emula la derivacion de llave del bruteforce.c original
ifeq ($(DERIVACION),base)
CFLAGS += -DDERIVACION_BASE
endif
COMUN   = src/des_utils.c src/des_compat.c

all: bin/bruteforce_seq bin/cifrar

bin/%: src/%.c $(COMUN) | bin
	$(CC) $(CFLAGS) $< $(COMUN) -o $@ $(LDLIBS)

bin:
	mkdir -p bin

clean:
	rm -rf bin

# Compila el bruteforce.c base de la clase (colocarlo en base/bruteforce.c) con el shim de rpc/des_crypt.h
base: base/bruteforce.c | bin
	mpicc -O2 -Icompat base/bruteforce.c -o bin/bruteforce_base $(LDLIBS)

# Verifica que texto_base.bin coincide con la derivacion de llave del codigo base
test: | bin
	$(CC) -w -Icompat tests/test_shim_base.c -o bin/test_shim_base $(LDLIBS) && ./bin/test_shim_base

.PHONY: all clean base test
