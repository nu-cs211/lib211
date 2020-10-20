# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = $(DEBUGFLAG) -O2 -fpic -std=c11 -pedantic-errors -Wall
LDFLAGS     = -shared
SANFLAGS    = -fsanitize=address,undefined
DEBUGFLAG   = -g

TOV_PUB    ?= /usr/local
DESTDIR    ?= $(TOV_PUB)
MANDIR     ?= $(DESTDIR)/man
LIBDIR     ?= $(DESTDIR)/lib
INCLUDEDIR ?= $(DESTDIR)/include

DEPFLAGS    = -MT $@ -MMD -MP -MF build/$*.d

COMPILE.c   = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c
OUTPUT_OPT  = -o $@
MKOUTDIR    = mkdir -p "$$(dirname "$@")"

ALIB_SAN    = build/lib211.a
ALIB_UNSAN  = build/lib211-unsan.a
SOLIB_SAN   = build/lib211.so
SOLIB_UNSAN = build/lib211-unsan.so
LIBS        = $(ALIB_SAN) $(ALIB_UNSAN) $(SOLIB_SAN) $(SOLIB_UNSAN)

SRCS        = $(wildcard src/*.c)
OBJS_SAN    = $(SRCS:%.c=build/%.o)
OBJS_UNSAN  = $(SRCS:%.c=build/%-unsan.o)
OBJS        = $(OBJS_SAN) $(OBJS_UNSAN)

all: $(LIBS)

test: $(LIBS)
	make -C test

test-install:
	make -C test test-install PREFIX=$(DESTDIR)

install: $(LIBS)
	$(SUDO) install -d -m 755 $(INCLUDEDIR) $(LIBDIR) $(MANDIR)/man3
	$(SUDO) install -m 755 $^         $(LIBDIR)
	$(SUDO) install -m 644 include/*  $(INCLUDEDIR)
	$(SUDO) install -m 644 man/man3/* $(MANDIR)/man3

$(SOLIB_SAN): $(OBJS_SAN)
	cc -o $@ $^ $(LDFLAGS) $(SANFLAGS)

$(SOLIB_UNSAN): $(OBJS_UNSAN)
	cc -o $@ $^ $(LDFLAGS)

$(ALIB_SAN): $(OBJS_SAN)
	ar -crs $@ $^

$(ALIB_UNSAN): $(OBJS_UNSAN)
	ar -crs $@ $^

build/src/alloc_rt.o: DEBUGFLAG =
build/src/alloc_rt-unsan.o: DEBUGFLAG =

build/%.o: %.c
build/%.o: %.c build/%.d
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $< $(SANFLAGS)

build/%-unsan.o: %.c
build/%-unsan.o: %.c build/%.d
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $<

DEPFILES := $(SRCS:%.c=build/%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))

clean:
	$(RM) -R build $(DEPDIR)
	make -C test clean

.PHONY: clean test test-install install
