#
# emulators PC Makefile
#


CC = gcc

#CFLAGS = -O2 -Wall -g -fPIC
#CFLAGS = -Wall -g -fPIC -ansi -pedantic
CFLAGS = -Wall -g -fPIC -D PC_TARGET
LDFLAGS =
INC_DIRS = -I/usr/include -I/usr/local/include -I../ -I../../src/log \
	-I../../src/lib -I../../src/tools -I../uso -I../pr

SRC1 = ../../src/log/bologging.c
SRC2 = ../../src/lib/serial.c
SRC3 = ../../src/tools/dbgout.c
SRC4 = ../../src/tools/oht.c
SRC5 = ../../src/tools/ocfg.c
SRC6 = ../../src/tools/ocrc.c
SRC7 = ../../src/tools/ort.c
SRC8 = ../../src/tools/ocs.c
SRC9 = ../../src/tools/ofnv1a.c
SRC_ACT1 = ../uso/uso.c
SRC_ACT2 = ../uso/uso_threads.c
SRC_ACT3 = ../pr/pr.c
SRC_ACT4 = ../pr/pr_threads.c

SRC_T1 = ../euso.c
SRC_T2 = ../epr.c

OBJ1 = build/bologging.o
OBJ2 = build/serial.o
OBJ3 = build/dbgout.o
OBJ4 = build/oht.o
OBJ5 = build/ocfg.o
OBJ6 = build/ocrc.o
OBJ7 = build/ort.o
OBJ8 = build/ocs.o
OBJ9 = build/ofnv1a.o
OBJ_ACT1 = build/uso/uso.o
OBJ_ACT2 = build/uso/uso_threads.o
OBJ_ACT3 = build/pr/pr.o
OBJ_ACT4 = build/pr/pr_threads.o

OBJ_T1 = build/uso/euso.o
OBJ_T2 = build/pr/epr.o

T1 = build/uso/euso
T2 = build/pr/epr


all:	clean default

default:
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC3) -o $(OBJ3)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC4) -o $(OBJ4)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC5) -o $(OBJ5)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC6) -o $(OBJ6)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC7) -o $(OBJ7)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC8) -o $(OBJ8)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC9) -o $(OBJ9)

	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_ACT1) -o $(OBJ_ACT1)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_ACT2) -o $(OBJ_ACT2)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_T1) -o $(OBJ_T1)

	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_ACT3) -o $(OBJ_ACT3)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_ACT4) -o $(OBJ_ACT4)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_T2) -o $(OBJ_T2)

	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) \
		$(OBJ6) $(OBJ7) $(OBJ8) $(OBJ9) \
		$(OBJ_ACT1) $(OBJ_ACT2) $(OBJ_T1) -o $(T1) -lpthread

	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) \
		$(OBJ6) $(OBJ7) $(OBJ8) \
		$(OBJ_ACT3) $(OBJ_ACT4) $(OBJ_T2) -o $(T2) -lpthread

clean:
	rm -f $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(OBJ7) \
		$(OBJ8) $(OBJ9)
	rm -f $(OBJ_ACT1) $(OBJ_ACT2) $(OBJ_T1) $(T1)
	rm -f $(OBJ_ACT3) $(OBJ_ACT4) $(OBJ_T2) $(T2)

