CC = gcc
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/lsv1.4.0
SRC = $(SRC_DIR)/lsv1.4.0.c
OBJ = $(OBJ_DIR)/lsv1.4.0.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

$(OBJ): $(SRC)
	$(CC) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
