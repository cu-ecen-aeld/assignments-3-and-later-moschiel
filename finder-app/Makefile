# CC: Define the compiler to use. 
# $(CROSS_COMPILE) allows you to specify a prefix for cross-compilation in the command line (e.g., aarch64-none-linux-gnu-).
# By default, $(CROSS_COMPILE) is empty, so it will use the native gcc.

# Command example using CROSS_COMPILE
# make CROSS_COMPILE=aarch64-none-linux-gnu-

CC = $(CROSS_COMPILE)gcc

# Compiler flags: These are additional options passed to the compiler for warnings and debugging.
# -Wall: Enables common warnings to catch potential issues in the code.
# -g: Includes debugging information in the generated binary (useful for debugging with tools like gdb).
CFLAGS = -Wall -g

# Name of the final output application (the executable).
TARGET = writer

# The object file needed to build the application.
# Here it is hardcoded because this is a simple project with only one source file.
OBJS = writer.o

# The source file for the application.
# This is the file that will be compiled into the object file.
SRCS = writer.c

# Default target: This is the first target executed when you run `make`.
# The 'all' target depends on $(TARGET), so running make will ensure the application is built
# If $(TARGET) is already up-to-date (no changes to $(OBJS)), make skips rebuilding it.
all: $(TARGET)
	@echo "Make complete."

# Rule to create the executable (writer).
# This depends on the object file (writer.o). 
# If writer.o changes or doesn't exist, it will be rebuilt.
$(TARGET): $(OBJS)
	# Use the compiler "$(CC)" to link the object file (OBJS) into the final executable (TARGET).
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile the source file (writer.c) into the object file (writer.o).
# If writer.c changes or writer.o doesn't exist, this rule is executed.
$(OBJS): $(SRCS)
	# Use the compiler "$(CC)" to compile writer.c into writer.o.
	$(CC) $(CFLAGS) -c $(SRCS) -o $(OBJS)

# The "clean" target is a utility rule.
# It removes all generated files to ensure a fresh build.
# It doesn't depend on any other rules and is only executed when explicitly called with "make clean".
clean:
	# Remove the executable (writer) and the object file (writer.o).
	rm -f $(TARGET) $(OBJS)
