CC=gcc
LDFLAGS=-lm

# Debug
# CFLAGS=-g -O0 -pipe -Wall -Werror

# Release
CFLAGS=-O2 -pipe -Wall -Werror

CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

COBJ=json.o http.o

TKUFTOP=tkuftop.o $(COBJ)
TKUFSTOP=tkufstop.o $(COBJ)

DESTDIR=/

all: tkuftop tkufstop

tkuftop: $(TKUFTOP)
	$(CC) $(TKUFTOP) $(LDFLAGS) -o tkuftop

tkufstop: $(TKUFSTOP)
	$(CC) $(TKUFSTOP) $(LDFLAGS)  -o tkufstop

clean:
	-rm tkuftop tkufstop *.o

install:
	install -m 755 tkuftop tkufstop $(DESTDIR)/usr/bin
