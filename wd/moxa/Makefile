#
# cron watchdog MOXA Makefile
#


CC = arm-elf-gcc
CFLAGS = -O2 -fomit-frame-pointer -fno-builtin -fpic -msingle-pic-base

LDFLAGS =  -Wl,-elf2flt="-r -z -s 32768"
INC_DIRS = -I/usr/local/arm-elf/include -I../ -I../../src/tools

SRC1 = ../../src/tools/oht.c
SRC2 = ../../src/tools/ocfg.c

SRC_T1 = ../owd.c

OBJ1 = build/oht.o
OBJ2 = build/ocfg.o

OBJ_T1 = build/owd.o

T1 = build/wd


all:	clean default

default:
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC2) -o $(OBJ2)

	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_T1) -o $(OBJ_T1)

	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ1) $(OBJ2) \
		$(OBJ_T1) -o $(T1)

clean:
	rm -f $(OBJ1) $(OBJ2)
	rm -f $(OBJ_T1) $(T1)

