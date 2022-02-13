GCC = g++
CFLAGS = -g -Wall
SRC = commands
OBJ = working_dir/bin
SOURCES = $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(OBJ)/%, $(SOURCES))


all: bin comm cp_ls_cat cp_test np

bin:
	mkdir -p $(OBJ)

comm: $(OBJECTS)
	# $(GCC) $^ -o $@

$(OBJ)/%: $(SRC)/%.cpp
	$(GCC) $< -o $@

cp_ls_cat:
	cp /bin/ls /bin/cat $(OBJ)

cp_test:
	cp test.html working_dir

np: npshell.cpp
	$(GCC) npshell.cpp -o working_dir/npshell $(CFLAGS)

clean:
	rm -rf npshell working_dir bin
