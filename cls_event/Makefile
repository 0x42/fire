TARGET		     = build/cls_event
#CC = arm-elf-gcc
CC = gcc
LDFLAGS		     = 
#LDFLAGS	     = -Wl, -elf2flt

CFLAGS		     = -Wall -c -g

INC_DIR		     = -I../src/log -I../src/tools -I../src/nettcp

SRC    = main.c
SRC1   = ../src/log/bologging.c
SRC2   = ../src/nettcp/bo_net.c
SRC3   = ../src/tools/dbgout.c

OBJ    = build/main.o
OBJ1   = build/bologging.o
OBJ2   = build/bo_net.o
OBJ3   = build/dbgout.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1)  -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2)  -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3)  -o $(OBJ3)

	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)

	$(CC) $(LDFLAGS) -g $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ) -o $(TARGET)

memtrace:
	MALLOC_TRACE=trace.log $(TARGET)
	mtrace $(TARGET) trace.log
	
clean:
	rm -R build
	mkdir build
	