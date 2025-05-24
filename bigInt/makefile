# Compiler and linker
CC = gcc
CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lgmp

TARGET = bigInt
SRCS = bigInt.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	rm -f $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

prof:
	$(CC) $(CFLAGS) -pg -o $(TARGET) $(SRCS) $(LDFLAGS)
	./$(TARGET)
	gprof $(TARGET) gmon.out > profile.txt
	rm gmon.out