CC     ?= gcc
CFLAGS  = -Wall -Wextra -Wpedantic --std=c11 -Ofast -Os -lX11
SRC     = meowmeow.c
OBJ     = $(SRC:.c=.o)
OUTPUT  = meowmeow

default:
	$(CC) $(CFLAGS) -c $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJ)

clean:
	$(RM) $(OUTPUT) $(OBJ)
