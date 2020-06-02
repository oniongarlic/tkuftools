CC=gcc
LDFLAGS=-lm

# Debug
CFLAGS=-g -O0 -Werror

# Release
# CFLAGS=-O2

# OS X
#CFLAGS+=-pipe -Wall

#CFLAGS+=-pipe -Wall -D_GNU_SOURCE

CFLAGS += -Wstrict-prototypes
CFLAGS += -Wunreachable-code
CFLAGS += -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wcast-align -Wcast-qual

# libcurl and json-c
ifeq ($(shell pkg-config --exists json-c libcurl && echo 0),0)
CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)
else
$(error json-c and libcurl is required)
endif

# libmosquitto MQTT Support
ifeq ($(shell pkg-config --exists libmosquitto && echo 0),0)
CFLAGS += $(shell pkg-config --cflags libmosquitto)
LDFLAGS += $(shell pkg-config --libs libmosquitto)
else
$(warning assuming libmosquitto exists in the system)
LDFLAGS+=-lmosquitto
endif

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
