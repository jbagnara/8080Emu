SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
OBJ_DIR := obj
TST_DIR := test

OBJECTS := $(addprefix $(OBJ_DIR)/,disassemble.o display.o emulator.o)
EMU_OBJ := $(OBJ_DIR)/main.o
DIS_OBJ := $(OBJ_DIR)/disassembler.o
TEST_OBJ := $(OBJ_DIR)/testop.o

CC := gcc
CFLAGS := -g -MMD -MP
CLIBS := -lSDL2 -pthread

.PHONY: all clean test

emulator: $(BIN_DIR)/emulator

all: emulator disassembler

disassembler: $(BIN_DIR)/disassembler

test: $(BIN_DIR)/test runtest


$(BIN_DIR)/emulator: $(EMU_OBJ) $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(CLIBS) -o $@

$(BIN_DIR)/disassembler: $(DIS_OBJ) $(OBJECTS) | $(BIN_DIR)	
	$(CC) $(CFLAGS) $^ $(CLIBS) -o $@

$(BIN_DIR)/test: $(TEST_OBJ) $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(CLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -I$(INC_DIR) $(CFLAGS) -c $<  -o $@

$(OBJ_DIR)/%.o: $(TST_DIR)/%.c | $(OBJ_DIR)
	$(CC) -I$(INC_DIR) $(CFLAGS) -c $<  -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

runtest:
	./bin/test

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)
	@rm out.txt

