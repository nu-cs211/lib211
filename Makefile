# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = -g -O2 -fpic -std=c11 -pedantic-errors -Wall $(SANFLAGS)
LDFLAGS     = -shared $(SANFLAGS)
SANFLAGS    = -fsanitize=address,undefined

DESTDIR    ?= $(TOV_PUB)
DESTDIR    ?= /usr/local
OBJDIR     ?= build

DEPFLAGS    = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

COMPILE.c   = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c
OUTPUT_OPT  = -o $@
MKOUTDIR    = mkdir -p "$$(dirname "$@")"

LIB = $(OBJDIR)/lib211.so
SRC = $(wildcard src/*.c)
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

lib: $(LIB)

test: $(LIB)
	make -C test

test-install:
	make -C test test-install

install: $(LIB)
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

$(LIB): $(OBJ)
	cc -o $@ $^ $(LDFLAGS)

$(OBJDIR)/lib211.a: $(OBJ)
	ar -crs $@ $^

$(OBJDIR)/%.o: %.c
$(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $<

DEPFILES := $(SRC:%.c=$(OBJDIR)/%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))

clean:
	$(RM) -R $(OBJDIR) $(DEPDIR)
	make -C test clean

.PHONY: clean test test-install install install-static
