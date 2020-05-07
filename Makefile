# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = $(DEBUGFLAG) -O2 -fpic -std=c11 -pedantic-errors -Wall $(SANFLAGS)
LDFLAGS     = -shared $(SANFLAGS)
SANFLAGS    = -fsanitize=address,undefined
DEBUGFLAG   = -g

TOV_PUB    ?= /usr/local
DESTDIR    ?= $(TOV_PUB)
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
	make -C test test-install PREFIX=$(DESTDIR)

install: $(LIB)
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

$(LIB): $(OBJ)
	cc -o $@ $^ $(LDFLAGS)

$(OBJDIR)/lib211.a: $(OBJ)
	ar -crs $@ $^

$(OBJDIR)/src/alloc_rt.o: DEBUGFLAG =

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
