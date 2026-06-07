SRC = ./src/pqp.cpp
CC = g++
CFLAGS = -O3 -march=native -Wall
OUT = ./bin/PQP.elf
INPUT_TXT = ./input/pqp_golden.input
OUTPUT_TXT = ./output/output.txt


all: compile run


compile: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: compile
	$(OUT) $(INPUT_TXT) $(OUTPUT_TXT)

clean:
	rm $(OUT)
	rm $(OUTPUT_TXT)

gdb:
	$(CC) -Wall -g $(SRC) -o $(OUT)
	gdb --args $(OUT) $(INPUT_TXT) $(OUTPUT_TXT)

.PHONY:
	all run clean
