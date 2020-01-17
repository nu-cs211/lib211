CFLAGS  = -std=c11 -pedantic -Wall
CFLAGS += -g -fpic -Iinclude
CFLAGS += -fsanitize=address,undefined
LDFLAGS =

DESTDIR ?= $(TOV_PUB)
LIBDIR  ?= $(DESTDIR)/lib

LIB = build/lib211.so
SRC = eprintf.c read_line.c test_rt.c
OBJ = $(SRC:%.c=build/%.o)

lib: $(LIB)

test: $(LIB)
	make -C test

test-install:
	make -C test test-install

install: $(LIB)
	install -m 755 $(LIB) $(LIBDIR)

$(LIB): $(OBJ)
	cc -shared -o $@ $^ $(CFLAGS) $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p "build/$$(dirname $@)"
	cc -c -o $@ $< $(CFLAGS)

clean:
	rm -Rf build
	make -C test clean

.PHONY: clean test
