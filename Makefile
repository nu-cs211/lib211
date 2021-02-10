# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = $(DEBUGFLAG) -O0 -fpic -std=c11 -pedantic -Wall $(SANFLAG)
LDFLAGS     = $(SANFLAG)

DEBUGFLAG   = -g
RAWFLAG     = -DLIB211_RAW_ALLOC
SANFLAG     = -fsanitize=address,undefined

RAWSUF      = -raw_alloc
UNSANSUF    = -unsan

PUB211     ?= /usr/local
DESTDIR    ?= $(PUB211)
MANDIR     ?= $(DESTDIR)/man
LIBDIR     ?= $(DESTDIR)/lib
INCLUDEDIR ?= $(DESTDIR)/include
OUTDIR     ?= build

DEPFLAGS    = -MT $@ -MMD -MP -MF $@.d

COMPILE.c   = $(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS)
LINK.shared = $(CC) -shared -o $@ $^ $(LDFLAGS)
LINK.static = $(AR) -crs $@ $^
MKOUTDIR    = mkdir -p "$$(dirname "$@")"

PREPROC.sh  = scripts/preprocess.sh
MAN.in      = $(shell find man -name '*.in')
MAN.out     = $(MAN.in:%.in=%)
INCLUDE.in  = $(shell find include -name '*.in')
INCLUDE.out = $(INCLUDE.in:%.in=%)

LIBSTEM     = $(OUTDIR)/lib211
ALIB_UNSAN  = $(LIBSTEM)$(UNSANSUF).a
SOLIB_SAN   = $(LIBSTEM).so
SOLIB_UNSAN = $(LIBSTEM)$(UNSANSUF).so
LIBS        = $(ALIB_UNSAN) $(SOLIB_SAN) $(SOLIB_UNSAN)

SRCS        = $(wildcard src/*.c)
OBJS_SAN    = $(OUTDIR)/src/read_line$(RAWSUF).o \
              $(SRCS:%.c=$(OUTDIR)/%.o)
OBJS_UNSAN  = $(OBJS_SAN:%.o=%$(UNSANSUF).o)
ALL_OBJS    = $(OBJS_SAN) $(OBJS_UNSAN)

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

$(OUTDIR)/src/alloc_rt%.o:              DEBUGFLAG =
%$(RAWSUF).o %$(RAWSUF)$(UNSANSUF).o:   CPPFLAGS += $(RAWFLAG)
$(SOLIB_UNSAN) $(OBJS_UNSAN):           SANFLAG =

$(ALIB_UNSAN): $(OBJS_UNSAN)
	$(LINK.static)

$(SOLIB_SAN): $(OBJS_SAN)
	$(LINK.shared)

$(SOLIB_UNSAN): $(OBJS_UNSAN)
	$(LINK.shared)

$(OUTDIR)/%.o: %.c
$(OUTDIR)/%.o: %.c $(OUTDIR)/%.o.d $(INCLUDE.out)
	@$(MKOUTDIR)
	$(COMPILE.c)

$(OUTDIR)/%$(UNSANSUF).o: %.c
$(OUTDIR)/%$(UNSANSUF).o: %.c $(OUTDIR)/%$(UNSANSUF).o.d $(INCLUDE.out)
	@$(MKOUTDIR)
	$(COMPILE.c)

$(OUTDIR)/%$(RAWSUF).o: %.c
$(OUTDIR)/%$(RAWSUF).o: %.c $(OUTDIR)/%$(RAWSUF).o.d $(INCLUDE.out)
	@$(MKOUTDIR)
	$(COMPILE.c)

$(OUTDIR)/%$(RAWSUF)$(UNSANSUF).o: %.c
$(OUTDIR)/%$(RAWSUF)$(UNSANSUF).o: %.c $(OUTDIR)/%$(RAWSUF)$(UNSANSUF).o.d $(INCLUDE.out)
	@$(MKOUTDIR)
	$(COMPILE.c)

%: %.in .version
	$(PREPROC.sh) $<

%.h: %.h.in .version
	$(PREPROC.sh) $<

DEPFILES := $(ALL_OBJS:%=%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))

.PHONY: all lib man test test-install install clean
