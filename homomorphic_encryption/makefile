# Variables
CC = gcc
CFLAGS = -Wall -Wextra -O3
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = client

# Rules
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ljson-c -lgmp 
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(EXEC)

re: fclean all

.PHONY: all clean fclean re