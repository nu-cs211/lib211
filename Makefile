CFLAGS  = -std=c11 -pedantic -Wall
CFLAGS += -g -fpic -Iinclude
CFLAGS += -fsanitize=address,undefined
LDFLAGS =

DESTDIR ?= $(TOV_PUB)

SLIB = build/lib211.so
ALIB = build/lib211.a
SRC = eprintf.c read_line.c test_rt.c
OBJ = $(SRC:%.c=build/%.o)

lib: $(SLIB) $(ALIB)

test: $(SLIB)
	make -C test

test-install:
	make -C test test-install

install-static: $(ALIB)
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

install: $(SLIB) $(ALIB)
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

$(SLIB): $(OBJ)
	cc -shared -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(ALIB): $(OBJ)
	ar -crs $@ $^

build/%.o: src/%.c
	@mkdir -p "build/$$(dirname $@)"
	cc -c -o $@ $< $(CFLAGS)

clean:
	rm -Rf build
	make -C test clean

.PHONY: clean test test-install install install-static
