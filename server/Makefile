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
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target: remove the binary and object files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

