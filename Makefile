CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mipsc: $(OBJS)
	$(CC) -o mipsc $(OBJS) $(LDFLAGS)

$(OBJS): mipsc.h

test: mipsc
	./test.sh

clean:
	rm -f mipsc *.o *~ tmp*

.PHONY: test clean