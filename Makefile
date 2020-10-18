# For building lib211.

CPPFLAGS    = -Iinclude
CFLAGS      = $(DEBUGFLAG) -O2 -fpic -std=c11 -pedantic-errors -Wall
LDFLAGS     = -shared
SANFLAGS    = -fsanitize=address,undefined
DEBUGFLAG   = -g

TOV_PUB    ?= /usr/local
DESTDIR    ?= $(TOV_PUB)
OBJDIR     ?= build

DEPFLAGS    = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

COMPILE.c   = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c
OUTPUT_OPT  = -o $@
MKOUTDIR    = mkdir -p "$$(dirname "$@")"

ALIB_SAN    = $(OBJDIR)/lib211.a
ALIB_UNSAN  = $(OBJDIR)/lib211-unsan.a
SOLIB_SAN   = $(OBJDIR)/lib211.so
SOLIB_UNSAN = $(OBJDIR)/lib211-unsan.so
LIBS        = $(ALIB_SAN) $(ALIB_UNSAN) $(SOLIB_SAN) $(SOLIB_UNSAN)

SRCS        = $(wildcard src/*.c)
OBJS_SAN    = $(SRCS:%.c=$(OBJDIR)/%.o)
OBJS_UNSAN  = $(SRCS:%.c=$(OBJDIR)/%-unsan.o)
OBJS        = $(OBJS_SAN) $(OBJS_UNSAN)

all: $(LIBS)

test: $(LIBS)
	make -C test

test-install:
	make -C test test-install PREFIX=$(DESTDIR)

install: $(LIBS)
	$(SUDO) install -m 755 $^ $(DESTDIR)/lib
	$(SUDO) install -m 644 include/* $(DESTDIR)/include

$(SOLIB_SAN): $(OBJS_SAN)
	cc -o $@ $^ $(LDFLAGS) $(SANFLAGS)

$(SOLIB_UNSAN): $(OBJS_UNSAN)
	cc -o $@ $^ $(LDFLAGS)

$(ALIB_SAN): $(OBJS_SAN)
	ar -crs $@ $^

$(ALIB_UNSAN): $(OBJS_UNSAN)
	ar -crs $@ $^

$(OBJDIR)/src/alloc_rt.o: DEBUGFLAG =
$(OBJDIR)/src/alloc_rt-unsan.o: DEBUGFLAG =

$(OBJDIR)/%.o: %.c
$(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $< $(SANFLAGS)

$(OBJDIR)/%-unsan.o: %.c
$(OBJDIR)/%-unsan.o: %.c $(OBJDIR)/%.d
	@$(MKOUTDIR)
	$(COMPILE.c) $(OUTPUT_OPT) $<

DEPFILES := $(SRCS:%.c=$(OBJDIR)/%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))

clean:
	$(RM) -R $(OBJDIR) $(DEPDIR)
	make -C test clean

.PHONY: clean test test-install install install-static
