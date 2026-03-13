PREFIX ?= /usr/local/bin
CXX = clang
CXX_FLAGS = -Wall -Wextra -pedantic
OPENSSL_FLAGS = `pkg-config --libs --cflags openssl`
BUILD_FLAGS =
DEBUG_FLAGS = -ggdb
OUTPUT_FOLDER = ./bin

ifeq ($(BUILD), 1)
	BUILD_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -fvisibility=hidden
	DEBUG_FLAGS = 
else
	CXX_FLAGS += -DDEV_MODE="1"
endif

.PHONY: directories

all: directories $(OUTPUT_FOLDER)/wodo

$(OUTPUT_FOLDER)/wodo: wodo.o parser.o io.o date.o visualizer.o json.o database.o utils.o crypt.o argparser.o actions.o
	$(CXX) $(CXX_FLAGS) $(OPENSSL_FLAGS) $(BUILD_FLAGS) -lm -o $(OUTPUT_FOLDER)/wodo $^

wodo.o: wodo.c parser.h parser.c ./clibs/arr.h io.h io.c systemtypes.h json.h json.c argparser.h argparser.c actions.c actions.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c wodo.c -o wodo.o

parser.o: parser.c parser.h systemtypes.h date.h date.c
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c parser.c -o parser.o

io.o: io.c io.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c io.c -o io.o

date.o: date.c date.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c date.c -o date.o

visualizer.o: visualizer.c visualizer.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c visualizer.c -o visualizer.o

json.o: json.c json.h visualizer.c visualizer.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c json.c -o json.o

utils.o: utils.c utils.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c utils.c -o utils.o

database.o: database.c database.h utils.c utils.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c database.c -o database.o

crypt.o: crypt.c crypt.h
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c crypt.c -o crypt.o

argparser.o: argparser.c argparser.h clibs/arr.h utils.h utils.c
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c argparser.c -o argparser.o

actions.o: actions.c actions.h clibs/arr.h utils.h utils.c
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) -c actions.c -o actions.o

directories: $(OUTPUT_FOLDER)

$(OUTPUT_FOLDER):
	mkdir -p $(OUTPUT_FOLDER)

USER_HOME := $(shell echo $$HOME)
PWD := $(shell pwd)

install:
	# Create directories if they don't exist
	mkdir -p $(USER_HOME)/.config/nvim/ftdetect
	mkdir -p $(USER_HOME)/.config/nvim/syntax
	mkdir -p $(USER_HOME)/.config/nvim/lua/config

	ln -sf $(PWD)/bin/wodo $(PREFIX)/wodo
	
	# Symlink Neovim files
	ln -sf $(PWD)/nvim/ftdetect/wodo.lua $(USER_HOME)/.config/nvim/ftdetect/wodo.lua
	ln -sf $(PWD)/nvim/syntax/wodo.vim $(USER_HOME)/.config/nvim/syntax/wodo.vim
	ln -sf $(PWD)/nvim/lua/config/wodo.lua $(USER_HOME)/.config/nvim/lua/config/wodo.lua

uninstall:
	rm -f $(PREFIX)/wodo
	rm -f $(USER_HOME)/.config/nvim/ftdetect/wodo.lua
	rm -f $(USER_HOME)/.config/nvim/syntax/wodo.vim
	rm -f $(USER_HOME)/.config/nvim/lua/config/wodo.lua

clean:
	rm -rf *.o clibs/*.o $(OUTPUT_FOLDER)
