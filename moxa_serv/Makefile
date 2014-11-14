TARGET		     = build/moxa_serv
CC = arm-elf-gcc
#CC = gcc
#LDFLAGS		     = -lpthread
LDFLAGS	     = -Wl, -elf2flt, -lpthread
CFLAGS		     = -Wall -c -g

INC_DIR		     = -I../src/log -I../src/tools -I../src/nettcp

SRC    = main.c
SRC1   = ../src/tools/dbgout.c
SRC2   = ../src/log/bologging.c
SRC3   = ../src/nettcp/bo_net.c
SRC4   = ../src/nettcp/bo_net_fifo_server.c
SRC5   = ../src/nettcp/bo_fifo.c
SRC6   = ../src/nettcp/bo_net_get.c
SRC7   = ../src/tools/ocfg.c
SRC8   = ../src/tools/oht.c

OBJ    = build/main.o
OBJ1   = build/dbgout.o
OBJ2   = build/bologging.o
OBJ3   = build/bo_net.o
OBJ4   = build/bo_net_fifo_server.o
OBJ5   = build/bo_fifo.o
OBJ6   = build/bo_net_get.o
OBJ7   = build/ocfg.o
OBJ8   = build/oht.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3) -o $(OBJ3)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC4) -o $(OBJ4)	
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC5) -o $(OBJ5)	
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC6) -o $(OBJ6)	
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC7) -o $(OBJ7)	
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC8) -o $(OBJ8)	

	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)
	$(CC) $(LDFLAGS) -g $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) \
	      $(OBJ6) $(OBJ7) $(OBJ8)  $(OBJ) -o $(TARGET)

memtrace:
	MALLOC_TRACE=trace.log $(TARGET)
	mtrace $(TARGET) trace.log
	
clean:
	rm -R build
	mkdir build