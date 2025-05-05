CXX = clang
CXX_FLAGS = -Wall -Wextra -pedantic -ggdb 
OPENSSL_FLAGS = `pkg-config --libs --cflags openssl`

ifeq ($(BUILD), 1)
	CXX_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -Wall -Wextra -Wpedantic -fvisibility=hidden
endif

wodo: main.o conf.o database.o crypt.o utils.o
	$(CXX) $(CXX_FLAGS) $(OPENSSL_FLAGS) -o wodo $^

test: conf.o test.o
	$(CXX) $(CXX_FLAGS) -o test $^

main.o: main.c utils.c utils.h crypt.c crypt.h database.h disk_database.c
	$(CXX) $(CXX_FLAGS) -c main.c -o main.o

conf.o: conf.c conf.h
	$(CXX) $(CXX_FLAGS) -c conf.c -o conf.o

database.o: disk_database.c database.h utils.c utils.h
	$(CXX) $(CXX_FLAGS) -c disk_database.c -o database.o

utils.o: utils.c utils.h
	$(CXX) $(CXX_FLAGS) -c utils.c -o utils.o

crypt.o: crypt.c crypt.h
	$(CXX) $(CXX_FLAGS) -c crypt.c -o crypt.o

test.o: test-conf.c conf.h
	$(CXX) $(CXX_FLAGS) -c test-conf.c -o test.o

clean:
	rm -rf *.o wodo test
