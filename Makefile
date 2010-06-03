CC=gcc
CFLAGS=-I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include \
-I${HOME}/opt/pidgin/include/libpurple
CPPFLAGS=-g -O2 -Wall -fPIC

all: xssidle.so

xssidle.so: xssidle.o
	${CC} -shared -o xssidle.so xssidle.o -lnsl -lresolv \
	-lXss

xssidle.o: xssidle.c

install: xssidle.so
	cp xssidle.so ~/.purple/plugins

clean:
	rm -f *.o *.so
