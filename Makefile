CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
TARGET = your_program
SRCS = main.c tgc.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c tgc.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

rebuild: clean all

.PHONY: all clean rebuild
