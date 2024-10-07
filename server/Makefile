CC = $(CROSS_COMPILE)gcc

CFLAGS = -Werror -Wall

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c

clean:
	rm -f aesdsocket
