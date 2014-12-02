TARGET		     = build/master
# ========== for Moxa ====================
#CC = arm-elf-gcc
#LDFLAGS	     = -Wl, -elf2flt -lpthread
# ========== for PC   ====================
CC = gcc
LDFLAGS		     = -lpthread
# ========================================
CFLAGS		     = -Wall -c -g

INC_DIR		     = -I../src/log -I../src/tools -I../src/nettcp

SRC    = main.c
SRC1   = ../src/log/bologging.c
SRC2   = ../src/tools/dbgout.c
SRC3   = ../src/nettcp/bo_net_master.c
SRC4   = ../src/tools/oht.c
SRC5   = ../src/tools/ocfg.c
SRC6   = ../src/nettcp/bo_net.c
SRC7   = ../src/tools/listsock.c
SRC8   = ../src/nettcp/bo_net_master_core.c
SRC9   = ../src/tools/ocrc.c
SRC10  = ../src/tools/ort.c
SRC11  = ../src/nettcp/bo_cycle_arr.c
SRC12  = ../src/nettcp/bo_net_master_core_log.c

OBJ    = build/main.o
OBJ1   = build/bologging.o
OBJ2   = build/dbgout.o
OBJ3   = build/bo_net_master.o
OBJ4   = build/oht.o
OBJ5   = build/ocfg.o
OBJ6   = build/bo_net.o
OBJ7   = build/listsock.o
OBJ8   = build/bo_net_master_core.o
OBJ9   = build/ocrc.o
OBJ10  = build/ort.o
OBJ11  = build/bo_cycle_arr.o
OBJ12  = build/bo_net_master_core_log.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1)  -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2)  -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3)  -o $(OBJ3)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC4)  -o $(OBJ4)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC5)  -o $(OBJ5)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC6)  -o $(OBJ6)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC7)  -o $(OBJ7)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC8)  -o $(OBJ8)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC9)  -o $(OBJ9)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC10) -o $(OBJ10)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC11) -o $(OBJ11)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC12) -o $(OBJ12)
	
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)
	$(CC) $(LDFLAGS) -g   $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) \
	      $(OBJ5) $(OBJ6) $(OBJ7) $(OBJ8) $(OBJ9) $(OBJ10) \
	      $(OBJ11) $(OBJ12) $(OBJ) -o $(TARGET)

#memtrace:
#	MALLOC_TRACE=trace.log $(TARGET)
#	mtrace $(TARGET) trace.log
	
clean:
	rm -R build
	mkdir build
	