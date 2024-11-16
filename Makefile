GCC = gcc
# GCC = g++
SOURCES = $(wildcard  *.c)
OBJECTS = $(SOURCES:.c=.o)
CFLAGS = -g -Wall

ifeq ($(OS),Windows_NT)
LIBS = -ljpeg -lm -largtable2
EXECUTABLE = point.exe

else ifeq ($(shell uname),Darwin)
LIBS = $(shell pkg-config --libs --static libjpeg argtable2)
EXECUTABLE = point
CFLAGS += $(shell pkg-config --cflags libjpeg argtable2)

else
LIBS = -ljpeg -lm -largtable2
EXECUTABLE = point
endif


all: $(SOURCES) $(EXECUTABLE)



$(EXECUTABLE): $(OBJECTS) 
	$(GCC) $(OBJECTS) -o $@ $(LIBS)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

