# Weather Station
#   make       build the program
#   make run   build, then launch it
#   make clean delete build files

CC      = clang
CFLAGS  = -std=c11 -Wall -Wextra -g $(shell pkg-config --cflags gtk+-3.0)
LDLIBS  = $(shell pkg-config --libs gtk+-3.0) -lm

TARGET  = station-meteo
SOURCES = main.c meteo.c interface.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDLIBS)

%.o: %.c meteo.h interface.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET) tests/test-meteo

tests/test-meteo: tests/test_meteo.c meteo.c meteo.h
	$(CC) -std=c11 -Wall -Wextra -g -I. tests/test_meteo.c meteo.c -o $@ -lm

test-simulation: tests/test-meteo
	./tests/test-meteo simulation

test-export: tests/test-meteo
	./tests/test-meteo export

.PHONY: all run clean test-simulation test-export
