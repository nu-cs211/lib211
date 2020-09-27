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

LIB_SAN     = $(OBJDIR)/lib211.so
LIB_UNSAN   = $(OBJDIR)/lib211-unsan.so
LIBS        = $(LIB_SAN) $(LIB_UNSAN)

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
	install -m 755 $^ $(DESTDIR)/lib
	install -m 644 include/* $(DESTDIR)/include

$(LIB_SAN): $(OBJS_SAN)
	cc -o $@ $^ $(LDFLAGS) $(SANFLAGS)

$(LIB_UNSAN): $(OBJS_UNSAN)
	cc -o $@ $^ $(LDFLAGS)

$(OBJDIR)/lib211.a: $(OBJS_SAN)
	ar -crs $@ $^

$(OBJDIR)/lib211-unsan.a: $(OBJS_UNSAN)
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
