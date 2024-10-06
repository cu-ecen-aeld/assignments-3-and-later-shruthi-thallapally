# Variables

CFLAGS := -Wall -Werror
TARGET := aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)

# Default target: Build the aesdsocket application
.PHONY: all
all: $(TARGET)

# Compile the aesdsocket application
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200112L -o $(TARGET) $(OBJ)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200112L -c $< -o $@

# Clean target: remove the binary and object files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

