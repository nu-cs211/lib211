CFLAGS  = -g -Wall -Werror -pedantic -std=c11 -I../include
LDFLAGS = -L../build -l211

TESTS = use_lib211_test

test: $(TESTS)
	for test in $^; do \
	    printf '\n*** Running %s: ***\n' $$test; \
	    ./$$test; \
	done

%: %.o
	cc -o $@ $^ $(CFLAGS) $(LDFLAGS)
