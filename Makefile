CXX = gcc
CXX_FLAGS = -Wall -Wextra -pedantic -ggdb

ifeq ($(BUILD), 1)
	CXX_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -fstack-protector-strong -Wall -Wextra -Wpedantic -fvisibility=hidden
endif

wodo: main.o
	$(CXX) $(CXX_FLAGS) -o wodo main.o

main.o: main.c
	$(CXX) $(CXX_FLAGS) -c main.c -o main.o

clean:
	rm -rf *.o wodo
