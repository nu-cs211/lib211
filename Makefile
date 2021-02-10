# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = $(DEBUGFLAG) $(OPTFLAG) -fpic -std=c11 -pedantic -Wall
SANFLAGS    = -fsanitize=address,undefined

DEBUGFLAG   = -g
OPTFLAG     = -O2

PUB211     ?= /usr/local
DESTDIR    ?= $(PUB211)
MANDIR     ?= $(DESTDIR)/man
LIBDIR     ?= $(DESTDIR)/lib
INCLUDEDIR ?= $(DESTDIR)/include

DEPFLAGS    = -MT $@ -MMD -MP -MF $@.d

COMPILE.c   = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c
OUTPUT_OPT  = -o $@
MKOUTDIR    = mkdir -p "$$(dirname "$@")"

PREPROC.sh  = scripts/preprocess.sh
READLINE.sh = scripts/read_line.sh
MAN.in      = $(shell find man -name '*.in')
MAN.out     = $(MAN.in:%.in=%)
INCLUDE.in  = $(shell find include -name '*.in')
INCLUDE.out = $(INCLUDE.in:%.in=%)

ALIB_SAN    = build/lib211.a
ALIB_UNSAN  = build/lib211-unsan.a
SOLIB_SAN   = build/lib211.so
SOLIB_UNSAN = build/lib211-unsan.so
LIBS        = $(ALIB_UNSAN) $(SOLIB_SAN) $(SOLIB_UNSAN)

SRCS        = build/read_line.c \
              $(wildcard src/*.c)
OBJS_SAN    = $(SRCS:%.c=build/%.o)
OBJS_UNSAN  = $(SRCS:%.c=build/%-unsan.o)
OBJS        = $(OBJS_SAN) $(OBJS_UNSAN)

all: lib man

lib: $(LIBS)

man: $(MAN.out)

test: $(LIBS)
	make -C test

test-install:
	make -C test test-install PREFIX=$(DESTDIR)

install: all
	$(SUDO) install -dm 755 $(LIBDIR)
	$(SUDO) install -m 755 $(LIBS) $(LIBDIR)
	$(SUDO) install -dm 755 $(INCLUDEDIR)
	$(SUDO) install -m 644 include/* $(INCLUDEDIR)
	$(SUDO) install -dm 755 $(MANDIR)
	tar --exclude='*.in' -c man | \
	    $(SUDO) tar --strip-components=1 -xC $(MANDIR)
	$(SUDO) chmod -R a+rX $(MANDIR)

clean:
	git clean -fX

$(SOLIB_SAN): $(OBJS_SAN)
	cc -o $@ $^ -shared $(SANFLAGS)

$(SOLIB_UNSAN): $(OBJS_UNSAN)
	cc -o $@ $^ -shared

$(ALIB_SAN): $(OBJS_SAN)
	ar -crs $@ $^

$(ALIB_UNSAN): $(OBJS_UNSAN)
	ar -crs $@ $^

build/src/alloc_rt.o build/src/alloc_rt-unsan.o: DEBUGFLAG =
build/src/alloc_rt.o build/src/alloc_rt-unsan.o: OPTFLAG = -O0

build/%.o: %.c
build/%.o: %.c build/%.o.d $(INCLUDE.out)
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $< $(SANFLAGS)

build/%-unsan.o: %.c
build/%-unsan.o: %.c build/%-unsan.o.d $(INCLUDE.out)
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $<

%: %.in .version
	$(PREPROC.sh) $<

%.h: %.h.in .version
	$(PREPROC.sh) $<

build/read_line.c: src/read_line.inc $(READLINE.sh)
	$(READLINE.sh) $< > $@

DEPFILES := $(SRCS:%.c=build/%.o.d) \
            $(SRCS:%.c=build/%-unsan.o.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))

.PHONY: all lib man test test-install install clean
