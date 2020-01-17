CFLAGS  = -g -Wall -Werror -pedantic -std=c11 -fpic -I$(shell pwd)/include
CFLAGS += -fsanitize=address,undefined
LDFLAGS =

SRC = eprintf.c read_line.c test_rt.c
OBJ = $(SRC:%.c=build/%.o)

build/lib211.so: $(OBJ)
	cc -shared -o $@ $^ $(CFLAGS) $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p "build/$$(dirname $@)"
	cc -c -o $@ $< $(CFLAGS)
