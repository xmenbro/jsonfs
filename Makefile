# Compiler and flags
CC = gcc
CFLAGS = -Wall -D_FILE_OFFSET_BITS=64
LDFLAGS = `pkg-config fuse3 jansson --cflags --libs`

# Target
TARGET = fs.out

# Source files
SRCS = fs.c funcs.c main.c
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = fs.h funcs.h

# Default target
all: $(TARGET)

# Obj to exec
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# Clean files
clean: 
	rm -f $(OBJS) $(TARGET)

# Clean and rebuild
rebuild: clean all

# Install
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the program (default)"
	@echo "  clean    - Remove object files and executable"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  install  - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall- Remove from /usr/local/bin (requires sudo)"
	@echo "  help     - Show this help message"

.PHONY: all clean rebuild install uninstall help
