# Makefile for the customAllocator program
CC = g++
CFLAGS = -g -Wall -std=c++11 -Werror -pedantic-errors -DNDEBUG -pthread
CCLINK = $(CC)
OBJS = main.o customAllocator.o
RM = rm -f
TARGET = main

all: $(TARGET)
# Creating the  executable
$(TARGET): $(OBJS)
	$(CCLINK) -o $(TARGET) $(OBJS)
# Creating the object files
customAllocator.o: customAllocator.cpp customAllocator.h
	$(CC) $(CFLAGS) -c customAllocator.cpp
main.o: main.cpp customAllocator.h
	$(CC) $(CFLAGS) -c main.cpp
# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ core.*

