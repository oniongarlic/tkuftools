CC=gcc
LDFLAGS=-lm

CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

all: tkuftop

tkuftop: tkuftop.o json.o
	$(CC) tkuftop.o json.o $(LDFLAGS)  -o tkuftop

clean:
	-rm tkuftop *.o
