SRC = ./src/pqp.cpp
CC = g++
CFLAGS = -O0 -march=native -Wall
OUT = ./bin/PQP.elf
INPUT_TXT = ./input/pqp_golden.input
OUTPUT_TXT = ./output/output.txt


all: compile run


$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

compile: $(OUT)

run: $(OUT)
	$(OUT) $(INPUT_TXT) $(OUTPUT_TXT)

clean:
	rm -f $(OUT)
	rm -f $(OUTPUT_TXT)

gdb:
	$(CC) -Wall -g $(SRC) -o $(OUT)
	gdb --args $(OUT) $(INPUT_TXT) $(OUTPUT_TXT)

.PHONY:
	all run clean
