CXX = clang
CXX_FLAGS = -Wall -Wextra -pedantic
OPENSSL_FLAGS = `pkg-config --libs --cflags openssl`
BUILD_FLAGS = -ggdb
OUTPUT_FOLDER = ./bin

ifeq ($(BUILD), 1)
	BUILD_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -fvisibility=hidden
endif

.PHONY: directories

all: directories $(OUTPUT_FOLDER)/wodo

$(OUTPUT_FOLDER)/wodo: main.o conf.o database.o crypt.o utils.o
	$(CXX) $(CXX_FLAGS) $(OPENSSL_FLAGS) $(BUILD_FLAGS) -o $(OUTPUT_FOLDER)/wodo $^

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

directories: $(OUTPUT_FOLDER)

$(OUTPUT_FOLDER):
	mkdir -p $(OUTPUT_FOLDER)

clean:
	rm -rf *.o $(OUTPUT_FOLDER)
