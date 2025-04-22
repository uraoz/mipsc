CFLAGS=-std=c11 -g -static

mipsc: mipsc.c

test: mipsc
	./test.sh

clean:
	rm -f mipsc *.o *~ tmp*

.PHONY: test clean