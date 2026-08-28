CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c17 -O2 -I./config -I./include
LDFLAGS = 

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    EXEC = cweb_server.exe
else
    LIBS = -lpthread -lm
    EXEC = cweb_server
endif

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(EXEC)

.PHONY: all clean
