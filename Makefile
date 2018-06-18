CC=gcc
LDFLAGS=-lm

CFLAGS += $(shell pkg-config --cflags json-c libcurl)
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

OBJ=tkuftop.o json.o http.o

all: tkuftop

tkuftop: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS)  -o tkuftop

clean:
	-rm tkuftop *.o
