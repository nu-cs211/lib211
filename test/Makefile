LIBDIR   = ../build
CPPFLAGS = -I../include
CFLAGS   = -g -Wall -pedantic -std=c11
SANFLAGS = -fsanitize=address,undefined
LDFLAGS  = -L$(LIBDIR) -l211

TESTS    = use_lib211_test check_string_test
EXES     = $(TESTS:%=build/%)

LIBENV   = LD_LIBRARY_PATH=$(LIBDIR)

test: $(EXES)
	for test in $(TESTS); do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    $(LIBENV) build/$$test 2>&1 || echo "Error exit: $$?" >&2; \
	    echo; \
	done

test-install: $(EXES)
	for test in $(TESTS); do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    build/$$test 2>&1; \
	done

build/%: build/%.o
	cc -o $@ $^ $(SANFLAGS) $(LDFLAGS)

build/%.o: %.c | build
	cc -c -o $@ $^ $(SANFLAGS) $(CPPFLAGS) $(CFLAGS)

build:
	mkdir -p build

clean:
	$(RM) -R build

.PHONY: clean test
