TARGET		     = build/moxa_snmp
# ========== for Moxa ====================
#CC = arm-elf-gcc
#LDFLAGS	     = -Wl, -elf2flt -lpthread
# ========== for PC   ====================
CC = gcc
LDFLAGS		     = -lpthread
# ========================================
CFLAGS		     = -Wall -c -g 

# Выводит отлад инфу в терминал
CFLAGS		     += -D __PRINT__ 
# =========================================
INC_DIR		     = -I../src/log -I../src/tools -I../src/nettcp -I../src/snmp -I../src/netudp

SRC    = main.c
SRC1   = ../src/tools/dbgout.c
SRC2   = ../src/log/bologging.c
SRC3   = ../src/snmp/bo_snmp_mng.c
SRC4   = ../src/snmp/bo_asn.c
SRC5   = ../src/snmp/bo_snmp.c
SRC6   = ../src/netudp/bo_udp.c
SRC7   = ../src/snmp/bo_parse.c

OBJ    = build/main.o
OBJ1   = build/dbgout.o
OBJ2   = build/bologging.o
OBJ3   = build/bo_snmp_mng.o
OBJ4   = build/bo_asn.o
OBJ5   = build/bo_snmp.o
OBJ6   = build/bo_udp.o
OBJ7   = build/bo_parse.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3) -o $(OBJ3)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC4) -o $(OBJ4)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC5) -o $(OBJ5)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC6) -o $(OBJ6)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC7) -o $(OBJ7)

	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)
	$(CC) $(LDFLAGS) -g $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) \
	$(OBJ5) $(OBJ6) $(OBJ7) $(OBJ) -o $(TARGET)

memtrace:
	MALLOC_TRACE=trace.log $(TARGET)
	mtrace $(TARGET) trace.log
	
clean:
	rm -R build
	mkdir build