wodo: main.o
	gcc -Wall -Wextra -pedantic -ggdb -o wodo main.o

main.o: main.c
	gcc -Wall -Wextra -pedantic -ggdb -c main.c -o main.o

clean:
	rm -rf *.o wodo
