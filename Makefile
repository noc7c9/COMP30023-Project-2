#
# COMP30023 Computer Systems Project 2
# Ibrahim Athir Saleem (isaleem) (682989)
#
# Makefile
#

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -lpthread
PORT = 4480

OBJ = main.o server.o sstp.o
EXE = server

VALGRIND_OPTS = -v --leak-check=full

## Top level target is executable.
$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ)

## Clean: Remove object files and core dump files.
clean:
	rm -f $(OBJ)

## Clobber: Performs Clean and removes executable file.
clobber: clean
	rm -f $(EXE)

## Run
run: $(EXE)
	./$(EXE) $(PORT)

## Test
# test: $(EXE)
# 	./$(EXE) -f testdata/input.txt -a first -m 1000 -q 7 | diff - testdata/output.txt

## Valgrind
# valgrind: $(EXE)
# 	valgrind $(VALGRIND_OPTS) --log-file=valgrind.1.log ./$(EXE)

## Dependencies
main.o: server.h sstp.h
server.o: server.h
sstp.o: sstp.h
