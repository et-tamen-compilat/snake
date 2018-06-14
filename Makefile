CC = gcc -g

test: test.c core.c types.h
	$(CC) test.c core.c -o test

all: snake main

snake: snake.c core.c types.h led-matrix-c.h
	$(CC) snake.c core.c librgbmatrix.a -o snake -lrt -lm -lpthread -lstdc++

main: main.c types.h
main2: main2.c types.h
