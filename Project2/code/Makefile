CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/application.c
	$(CC) $(CFLAGS) -o application $^

.PHONY: clean
clean:
	rm -rf application