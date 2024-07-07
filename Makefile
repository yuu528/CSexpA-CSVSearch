# CC=gcc
# C++=g++
# LD=g++
CC=~/buildroot-2024.02.3/output/host/bin/aarch64-linux-gcc
C++=~/buildroot-2024.02.3/output/host/bin/aarch64-linux-g++
LD=~/buildroot-2024.02.3/output/host/bin/aarch64-linux-g++
CFLAGS=-c -Wall -pedantic-errors -Ofast -flto -fno-stack-protector
# CFLAGS=-c -Wall -pedantic-errors -O0 -g
LDFLAGS=-pthread
OBJECTS=./src/csvsearch.o ./src/includes/csvloader.o ./src/includes/extras.o ./src/includes/session.o ./src/includes/socketutil.o
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

