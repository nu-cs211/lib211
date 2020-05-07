PREFIX  ?= /usr/local
LIBDIR   = ../build
CPPFLAGS = -I../include
CFLAGS   = -g -Wall -pedantic-errors -std=c11 $(SANFLAGS)
LDFLAGS  = -L$(LIBDIR) -l211 $(SANFLAGS)
SANFLAGS = -fsanitize=address,undefined
LIBENV   = LD_LIBRARY_PATH=$(LIBDIR)

TESTS    = use_lib211_test check_string_test
EXES     = $(TESTS:%=build/%)
SYS_EXES = $(TESTS:%=build/%.system)

test-install build/%.system: LIBDIR = $(PREFIX)/lib

test: $(EXES)
	for test in $(TESTS); do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    $(LIBENV) build/$$test 2>&1 || echo "Error exit: $$?" >&2; \
	    echo; \
	done

test-install: $(SYS_EXES)
	for test in $(TESTS); do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    $(LIBENV) build/$$test 2>&1 || echo "Error exit: $$?" >&2; \
	    echo; \
	done

build/%: build/%.o
	cc -o $@ $^ $(LDFLAGS)

build/%.system: build/%.o
	cc -o $@ $^ $(LDFLAGS)

build/%.o: %.c | build
	cc -c -o $@ $^ $(CPPFLAGS) $(CFLAGS)

build:
	mkdir -p build

clean:
	$(RM) -R build

.PHONY: clean test
