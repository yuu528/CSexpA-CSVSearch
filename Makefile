# CC=gcc
# LD=g++
CC=~/buildroot-2024.02.3/output/host/bin/aarch64-none-linux-gnu-gcc
LD=~/buildroot-2024.02.3/output/host/bin/aarch64-none-linux-gnu-gcc
CFLAGS=-c -Wall -pedantic-errors -Ofast -flto -fno-stack-protector
# CFLAGS=-c -Wall -pedantic-errors -Og -g
LDFLAGS=-pthread
OBJECTS=./src/csvsearch.o ./src/includes/csvloader.o ./src/includes/extras.o ./src/includes/session.o ./src/includes/socketutil.o
EXECUTABLE=csvsearch.out
GDB=~/buildroot-2024.02.3/output/host/bin/aarch64-linux-gdb
GDBINIT=~/buildroot-2024.02.3/output/staging/usr/share/buildroot/gdbinit

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f ${EXECUTABLE} ${EXECUTABLE}.exe ${OBJECTS} core

debug:
	$(GDB) -ix $(GDBINIT) $(EXECUTABLE)
