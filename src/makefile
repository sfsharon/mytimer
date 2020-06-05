CFLAGS=-g -ansi -pedantic -Wall -Werror
LDFLAGS=-pthread 
# LDFLAGS=-lrt
CC=gcc
OBJECTS=mytimer.o main_mytimer.o 
TARGET=myTimerApp

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm ./$(TARGET) *.o