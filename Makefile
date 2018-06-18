CC=gcc
LDFLAGS=-lm

CFLAGS=-g -O0 -pipe

CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

COBJ=json.o http.o

TKUFTOP=tkuftop.o $(COBJ)
TKUFSTOP=tkufstop.o $(COBJ)

all: tkuftop tkufstop

tkuftop: $(TKUFTOP)
	$(CC) $(TKUFTOP) $(LDFLAGS) -o tkuftop

tkufstop: $(TKUFSTOP)
	$(CC) $(TKUFSTOP) $(LDFLAGS)  -o tkufstop

clean:
	-rm tkuftop *.o
