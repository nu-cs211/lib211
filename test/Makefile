PREFIX  ?= /usr/local
LIBDIR   = ../build
CPPFLAGS = -I../include
CFLAGS   = -g -Wall -pedantic-errors -std=c11 $(SANFLAGS)
# SANFLAGS = -fsanitize=address,undefined
SANFLAGS =
LDFLAGS  = -L$(LIBDIR) -ldl -l211-unsan $(SANFLAGS)
LIBENV   = LD_LIBRARY_PATH=$(LIBDIR)

TESTS    = use_lib211_test \
           check_string_test \
           alloc_limit_test \
           check_command_test
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

build/alloc_limit_test.o build/alloc_record.o: alloc_record.h

build/alloc_limit_test build/alloc_limit_test.system: build/alloc_record.o

build/% build/%.system: build/%.o
	cc -o $@ $^ $(LDFLAGS)

build/%.o: %.c | build
	cc -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

build:
	mkdir -p build

clean:
	$(RM) -R build

.PHONY: clean test
