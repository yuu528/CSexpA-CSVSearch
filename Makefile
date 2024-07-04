CC=gcc
C++=g++
LD=g++
# CFLAGS=-c -Wall -pedantic-errors -Ofast
CFLAGS=-c -Wall -pedantic-errors -O0 -g
LDFLAGS=-pthread
OBJECTS=./src/csvsearch.o ./src/includes/socketutil.o ./src/includes/csvloader.o ./src/includes/stringutil.o
EXECUTABLE=csvsearch.out

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(C++) $(CFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f ${EXECUTABLE} ${EXECUTABLE}.exe ${OBJECTS} core

