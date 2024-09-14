#references : ECEN5813

# Variables
CC := gcc
CFLAGS := -Wall -Werror
TARGET := writer
SRC := writer.c
OBJ := $(SRC:.c=.o)

# Check if CROSS_COMPILE is set
ifeq ($(CROSS_COMPILE),)
    CC := gcc
else
    CC := $(CROSS_COMPILE)gcc
endif

# Default target: Build the writer application
.PHONY: all
all: $(TARGET)

# Compile the writer application
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target: remove the binary and object files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)
