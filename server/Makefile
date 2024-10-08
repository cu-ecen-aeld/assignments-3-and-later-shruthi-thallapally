ifdef CROSS_COMPILE
ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
CC = $(CROSS_COMPILE)gcc
else
CC = gcc
endif
endif

#Target executable aesdsocket utility
TARGET=aesdsocket

OBJS = $(SRC:.c=.o)
SRC  = aesdsocket.c
CFLAGS = -Werror -Wall -Wunused -Wunused-variable -Wextra

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@
clean: 
	rm -f $(OBJS) $(TARGET)
