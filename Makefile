CC=gcc
CFLAGS=-I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include \
$(shell pkg-config --cflags finch)
CPPFLAGS=-g -O2 -Wall -fPIC
OBJS=xssidle.o betterhistory.o

all: $(patsubst %.o,%.so,${OBJS})

xssidle.so: xssidle.o
	${CC} -shared -o $@ $< -lnsl -lresolv \
	-lXss

betterhistory.so: betterhistory.o
	${CC} -shared -o $@ $< -lnsl -lresolv

install: all
	cp *.so ~/.purple/plugins

clean:
	rm -f *.o *.so
