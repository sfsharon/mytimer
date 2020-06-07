CFLAGS= -ansi -pedantic -Wall -Werror
LDFLAGS=-pthread 
CC=g++
OBJECTS=mytimer_OO.o main_mytimer.o 
TARGET=myTimerApp_OO.out

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm ./$(TARGET) *.o