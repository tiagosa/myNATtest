CFLAGS = -Wall -g
CC = gcc
LIBS = -lnice
INCLUDES = -I/usr/local/include -I/usr/local/include/nice -L/usr/local/lib -Wl,-R/usr/local/lib
CFLAGS	+=  $(shell pkg-config --cflags --libs glib-2.0)

myice:
	${CC} ${CFLAGS} ${INCLUDES} ${LIBS} myice.c -o $@
icetest:
	g++ ${CFLAGS} ${INCLUDES} ${LIBS} icetest.cpp -o $@

client:
	${CC} ${CFLAGS} ${INCLUDES} ${LIBS} client.c -o $@

clean:
	rm -f client myice a.out *.bin
