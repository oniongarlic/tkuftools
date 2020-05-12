CC=gcc
LDFLAGS=-lm

# Debug
CFLAGS=-g -O0 -Werror

# Release
# CFLAGS=-O2

# OS X
#CFLAGS+=-pipe -Wall

#CFLAGS+=-pipe -Wall -D_GNU_SOURCE

CFLAGS += $(shell pkg-config --cflags json-c libcurl libmosquitto)
LDFLAGS += $(shell pkg-config --libs json-c libcurl libmosquitto) 

# MQTT Support
LDFLAGS+=-lmosquitto

COBJ=json.o http.o racks.o

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
