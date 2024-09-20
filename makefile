# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -Wall -g

# Include directories (including the path to stb_image.h)
INCLUDES = -I./ -I/usr/include -I/usr/include/GL -I/usr/include/stb

# Library directories
LIBDIRS = -L/usr/lib

# Libraries
LIBS = -lglfw3

# Source files
CPPSRCS = main.cpp
CSRCS = stb.cpp glad.c

# Output executable
OUT = app

# Object files
CPPOBJS = $(CPPSRCS:.cpp=.o)
COBJS = $(CSRCS:.c=.o)
OBJS = $(CPPOBJS) $(COBJS)

# Default target
all: $(OUT)

# Compile the project
$(OUT): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LIBDIRS) $(LIBS) -o $(OUT)

# Compile .cpp source files into object files
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile .c source files into object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean object files and executable
clean:
	rm -f $(OBJS) $(OUT)

# Run the program
run: $(OUT)
	./$(OUT)

.PHONY: all clean run
