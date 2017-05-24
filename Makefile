#
# COMP30023 Computer Systems Project 2
# Ibrahim Athir Saleem (isaleem) (682989)
#
# Makefile
#

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -lpthread
PORT = 4480

OBJ = main.o server.o sstp-socket-wrapper.o sstp.o log.o sha256.o
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
test: $(EXE)
	# make sure the server is running
	pytest -xv

## Valgrind
valgrind: $(EXE)
	# run `make test`
	valgrind $(VALGRIND_OPTS) --log-file=valgrind.log ./$(EXE) $(PORT)

## Dependencies
main.o: server.o sstp-socket-wrapper.o log.o sha256.o uint256.h
server.o: server.h
sstp.o: sstp.h
sstp-socket-wrapper.o: sstp-socket-wrapper.h sstp.o
log.o: log.h server.o
sha256.o: sha256.h
