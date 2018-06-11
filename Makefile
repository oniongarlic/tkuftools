CC=gcc

CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

all: tkuftop

tkuftop: tkuftop.o
	$(CC) tkuftop.o $(LDFLAGS)  -o tkuftop

clean:
	-rm tkuftop tkuftop.o
