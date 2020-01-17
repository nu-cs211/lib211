CFLAGS  = -g -Wall -pedantic -std=c11 -I../include
CFLAGS += -fsanitize=address,undefined
LDFLAGS = -L../build -l211

TESTS = build/use_lib211_test

test: $(TESTS)
	for test in $^; do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    LD_LIBRARY_PATH=../build ./$$test; \
	done

test-install: $(TESTS)
	for test in $^; do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    ./$$test; \
	done

build/%.o: %.c
	@mkdir -p "$$(dirname $@)"
	cc -c -o $@ $^ $(CFLAGS)

%: %.o
	cc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -Rf build

.PHONY: clean test
