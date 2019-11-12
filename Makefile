CC=gcc
LDFLAGS=-lm

# Debug
CFLAGS=-g -O0 -Werror

# Release
# CFLAGS=-O2

CFLAGS+=-pipe -Wall -D_GNU_SOURCE

CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

CFLAGS += -Wstrict-prototypes
CFLAGS += -Wunreachable-code
CFLAGS += -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wcast-align -Wcast-qual

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
