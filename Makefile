CXX = clang
CXX_FLAGS = -Wall -Wextra -pedantic -ggdb

ifeq ($(BUILD), 1)
	CXX_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -Wall -Wextra -Wpedantic -fvisibility=hidden
endif

wodo: main.o conf.o
	$(CXX) $(CXX_FLAGS) -o wodo $^

main.o: main.c
	$(CXX) $(CXX_FLAGS) -c main.c -o main.o

conf.o: conf.c conf.h
	$(CXX) $(CXX_FLAGS) -c conf.c -o conf.o

clean:
	rm -rf *.o wodo
