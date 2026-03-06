CXX = clang
CXX_FLAGS = -Wall -Wextra -pedantic -lm
OPENSSL_FLAGS = `pkg-config --libs --cflags openssl`
BUILD_FLAGS =
DEBUG_FLAGS = -ggdb
OUTPUT_FOLDER = ./bin

ifeq ($(BUILD), 1)
	BUILD_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -fvisibility=hidden
	DEBUG_FLAGS = 
endif

.PHONY: directories

all: directories $(OUTPUT_FOLDER)/wodo

$(OUTPUT_FOLDER)/wodo: wodo.o parser.o io.o
	$(CXX) $(CXX_FLAGS) $(OPENSSL_FLAGS) $(BUILD_FLAGS) -o $(OUTPUT_FOLDER)/wodo $^

wodo.o: wodo.c parser.h parser.c ./clibs/arr.h io.h io.c
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c wodo.c -o wodo.o

parser.o: parser.c parser.h ./clibs/arr.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c parser.c -o parser.o

io.o: io.c io.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c io.c -o io.o

directories: $(OUTPUT_FOLDER)

$(OUTPUT_FOLDER):
	mkdir -p $(OUTPUT_FOLDER)

clean:
	rm -rf *.o clibs/*.o $(OUTPUT_FOLDER)
