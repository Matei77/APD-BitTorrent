
CC = $(CC)
FLAGS = $(FLAGS)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

build: tema3

tema3: main.o peer.o tracker.o
	$(CC) $^ -o $@ $(FLAGS)

main.o: main.cpp constants.h
	$(CC) -c $< $(FLAGS)

peer.o: peer.cpp peer.h constants.h
	$(CC) -c $< $(FLAGS)

tracker.o: tracker.cpp tracker.h constants.h
	$(CC) -c $< $(FLAGS)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

clean:
	rm -rf tema3 main.o peer.o tracker.o

