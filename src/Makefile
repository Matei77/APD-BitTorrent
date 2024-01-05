
CC = mpic++
FLAGS = -Wall -g

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

build: tema3

tema3: main.o peer.o tracker.o
	$(CC) $^ -o $@ $(FLAGS)

main.o: main.cpp utils.h
	$(CC) -c $< $(FLAGS)

peer.o: peer.cpp peer.h utils.h
	$(CC) -c $< $(FLAGS)

tracker.o: tracker.cpp tracker.h utils.h
	$(CC) -c $< $(FLAGS)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# run:
# 	mpirun -np 4 tema3

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

clean:
	rm -rf tema3 main.o peer.o tracker.o

