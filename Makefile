PREFIX ?= /usr/local/bin
CXX = clang
CXX_FLAGS = -Wall -Wextra -pedantic
OPENSSL_FLAGS = `pkg-config --libs --cflags openssl`
BUILD_FLAGS =
DEBUG_FLAGS = -ggdb
OUTPUT_FOLDER = ./bin

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

ifeq ($(BUILD), 1)
	BUILD_FLAGS = -O3 -march=native -flto -fPIE -pie -fno-semantic-interposition -fvisibility=hidden
	DEBUG_FLAGS = 
else
	CXX_FLAGS += -DDEV_MODE="1"
endif

.PHONY: directories

all: directories $(OUTPUT_FOLDER)/wodo

$(OUTPUT_FOLDER)/wodo: $(OBJS)
	$(CXX) $(CXX_FLAGS) $(OPENSSL_FLAGS) $(BUILD_FLAGS) -lm -o $(OUTPUT_FOLDER)/wodo $^

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
	rm -rf $(OBJS) $(OUTPUT_FOLDER)
