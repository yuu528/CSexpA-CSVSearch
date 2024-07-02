CC=gcc
C++=g++
LD=g++
CFLAGS=-c -Wall -pedantic-errors -Ofast
LDFLAGS=-pthread
OBJECTS=./src/main.o ./src/includes/socketutil.o
EXECUTABLE=csvsearch.out

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(C++) $(CFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f ${EXECUTABLE} ${EXECUTABLE}.exe ${OBJECTS}

