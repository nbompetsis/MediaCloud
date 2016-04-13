OBJS 	= HayStackServer.o lists.o hash.o
SOURCE	= HayStackServer.c lists.c hash.c
OUT  	= HSServer
CC	= gcc
LIB = -pthread
#FLAGS   = -g -O2 -Wall -ansi -pedantic -c 
FLAGS = -c

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LIB)

HayStackServer.o: HayStackServer.c
	$(CC) $(FLAGS) $^ $(LIB)

lists.o: lists.c
	$(CC) $(FLAGS) $^

hash.o: hash.c
	$(CC) $(FLAGS) $^

clean:
	rm -f $(OBJS) $(OUT)
