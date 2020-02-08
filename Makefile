CFLAGS  = -std=c11 -pedantic -Wall
CFLAGS += -g -fpic -Iinclude
CFLAGS += -fsanitize=address,undefined
LDFLAGS =

DESTDIR ?= $(TOV_PUB)

LIB = build/lib211.so
SRC = eprintf.c read_line.c test_rt.c
OBJ = $(SRC:%.c=build/%.o)

lib: $(LIB)

test: $(LIB)
	make -C test

test-install:
	make -C test test-install

install: $(LIB)
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

$(LIB): $(OBJ)
	cc -shared -o $@ $^ $(CFLAGS) $(LDFLAGS)
	-strip --strip-debug --strip-unneeded $@

build/lib211.a: $(OBJ)
	ar -crs $@ $^

build/%.o: src/%.c
	@mkdir -p "build/$$(dirname $@)"
	cc -c -o $@ $< $(CFLAGS)

clean:
	rm -Rf build
	make -C test clean

.PHONY: clean test test-install install install-static
