GXX = g++

WARNINGS = -Wall -Wno-sign-compare

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

SRCS = $(SRC_DIR)/shell.cc $(SRC_DIR)/fs.cc $(SRC_DIR)/disk.cc $(SRC_DIR)/graphic_interface.cpp
OBJS = $(OBJ_DIR)/shell.o $(OBJ_DIR)/fs.o $(OBJ_DIR)/disk.o $(OBJ_DIR)/graphic_interface.o

$(BIN_DIR)/simplefs: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(GXX) $(OBJS) -o $(BIN_DIR)/simplefs $(SFML_LIBS)

$(OBJ_DIR)/shell.o: $(SRC_DIR)/shell.cc
	@mkdir -p $(OBJ_DIR)
	$(GXX) $(WARNINGS) -c $< -o $@ -g

$(OBJ_DIR)/fs.o: $(SRC_DIR)/fs.cc $(SRC_DIR)/fs.h
	@mkdir -p $(OBJ_DIR)
	$(GXX) $(WARNINGS) -c $< -o $@ -g

$(OBJ_DIR)/disk.o: $(SRC_DIR)/disk.cc $(SRC_DIR)/disk.h
	@mkdir -p $(OBJ_DIR)
	$(GXX) $(WARNINGS) -c $< -o $@ -g

$(OBJ_DIR)/graphic_interface.o: $(SRC_DIR)/graphic_interface.cpp
	@mkdir -p $(OBJ_DIR)
	$(GXX) $(WARNINGS) -c $< -o $@ -g $(SFML_LIBS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)