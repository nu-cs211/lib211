# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = -g -fpic -std=c11 -pedantic-errors -Wall
LDFLAGS     =
SANFLAGS    = -fsanitize=address,undefined

DESTDIR    ?= $(TOV_PUB)
OBJDIR     ?= build

LIB = $(OBJDIR)/lib211.so
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=$(OBJDIR)/%.o)

lib: $(LIB)

test: $(LIB)
	make -C test

test-install:
	make -C test test-install

install: $(LIB)
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

$(LIB): $(OBJ)
	cc -shared -o $@ $^ $(CFLAGS) $(SANFLAGS)
	-strip -S $@

$(OBJDIR)/lib211.a: $(OBJ) | $(OBJDIR)
	ar -crs $@ $^

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	cc -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

$(OBJDIR):
	mkdir -p $@

# $(OBJDIR)/%.o: src/%.c
# 	@mkdir -p "$(OBJDIR)/$$(dirname $@)"
# 	cc -c -o $@ $< $(CFLAGS)

clean:
	$(RM) -R $(OBJDIR)
	make -C test clean

.PHONY: clean test test-install install install-static
