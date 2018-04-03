LDFLAGS = -L/Users/jtanderson/.homebrew/opt/ncurses/lib -lncurses
CFLAGS = -std=c++11 -Wall -O3 -I/Users/jtanderson/.homebrew/opt/ncurses/include -g

all: main

main: main.o
	g++ $(CFLAGS) main.o -o main $(LDFLAGS)

main.o: main.cpp
	g++ -c $(CFLAGS) main.cpp -o main.o

clean:
	rm *.o main
