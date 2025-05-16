CC = gcc
CFLAGS = -Wall -Wextra -pthread

SRC = echo_server.c
OBJ = $(SRC:.c=.o)
TARGET = echo_server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(TARGET)
