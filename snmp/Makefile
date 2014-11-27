TARGET		     = build/moxa_snmp
#CC = arm-elf-gcc
CC = gcc
LDFLAGS		     = -lpthread
#LDFLAGS	     = -Wl, -elf2flt, -lpthread
CFLAGS		     = -Wall -c -g

INC_DIR		     = -I../src/log -I../src/tools -I../src/nettcp -I../src/snmp

SRC    = main.c
SRC1   = ../src/tools/dbgout.c
SRC2   = ../src/log/bologging.c
SRC3   = ../src/snmp/bo_snmp_mng.c

OBJ    = build/main.o
OBJ1   = build/dbgout.o
OBJ2   = build/bologging.o
OBJ3   = build/bo_snmp_mng.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3) -o $(OBJ3)

	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)
	$(CC) $(LDFLAGS) -g $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ) -o $(TARGET)

memtrace:
	MALLOC_TRACE=trace.log $(TARGET)
	mtrace $(TARGET) trace.log
	
clean:
	rm -R build
	mkdir build