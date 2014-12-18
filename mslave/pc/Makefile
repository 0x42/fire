#
# mslave PC Makefile
#


CC = gcc

#CFLAGS = -O2 -Wall -g -fPIC
#CFLAGS = -Wall -g -fPIC -ansi -pedantic
CFLAGS = -Wall -g -fPIC
LDFLAGS = 
INC_DIRS = -I/usr/include -I/usr/local/include -I../ -I../../src/log \
	-I../../src/lib -I../../src/tools -I../../src/nettcp -I../slave

SRC1 = ../../src/log/bologging.c
SRC2 = ../../src/lib/serial.c
SRC3 = ../../src/tools/dbgout.c
SRC4 = ../../src/tools/oht.c
SRC5 = ../../src/tools/ocfg.c
SRC6 = ../../src/tools/ocrc.c
SRC7 = ../../src/tools/ort.c
SRC8 = ../../src/tools/ocs.c
SRC9 = ../../src/tools/ofnv1a.c
SRC10 = ../../src/nettcp/bo_net.c
SRC11 = ../../src/nettcp/bo_net_get.c
SRC12 = ../../src/nettcp/bo_net_master_core.c
SRC13 = ../../src/nettcp/bo_cycle_arr.c
SRC14 = ../../src/nettcp/bo_net_master_core_log.c
SRC15 = ../../src/nettcp/bo_net_fifo_server.c
SRC_SL1 = ../slave/slave.c
SRC_SL2 = ../slave/ch1_threads.c
SRC_SL3 = ../slave/ch2_threads.c
SRC_SL4 = ../slave/a_threads.c

SRC_T1 = ../mslave.c

OBJ1 = build/bologging.o
OBJ2 = build/serial.o
OBJ3 = build/dbgout.o
OBJ4 = build/oht.o
OBJ5 = build/ocfg.o
OBJ6 = build/ocrc.o
OBJ7 = build/ort.o
OBJ8 = build/ocs.o
OBJ9 = build/ofnv1a.o
OBJ10 = build/bo_net.o
OBJ11 = build/bo_net_get.o
OBJ12 = build/bo_net_master_core.o
OBJ13 = build/bo_cycle_arr.o
OBJ14 = build/bo_net_master_core_log.o
OBJ15 = build/bo_net_fifo_server.o
OBJ_SL1 = build/slave.o
OBJ_SL2 = build/ch1_threads.o
OBJ_SL3 = build/ch2_threads.o
OBJ_SL4 = build/a_threads.o

OBJ_T1 = build/mslave.o

T1 = build/mslave


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
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC10) -o $(OBJ10)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC11) -o $(OBJ11)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC12) -o $(OBJ12)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC13) -o $(OBJ13)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC14) -o $(OBJ14)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC15) -o $(OBJ15)

	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_SL1) -o $(OBJ_SL1)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_SL2) -o $(OBJ_SL2)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_SL3) -o $(OBJ_SL3)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_SL4) -o $(OBJ_SL4)
	$(CC) $(INC_DIRS) $(CFLAGS) -c $(SRC_T1) -o $(OBJ_T1)

	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) \
		$(OBJ6) $(OBJ7) $(OBJ8) $(OBJ9) $(OBJ10) $(OBJ11) $(OBJ12) \
		$(OBJ13) $(OBJ14) $(OBJ15) \
		$(OBJ_SL1) $(OBJ_SL2) $(OBJ_SL3) $(OBJ_SL4) \
		$(OBJ_T1) -o $(T1) -lpthread

clean:
	rm -f $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(OBJ7) $(OBJ8) \
		$(OBJ9) $(OBJ10) $(OBJ11) $(OBJ12) $(OBJ13) $(OBJ14) $(OBJ15)
	rm -f $(OBJ_SL1) $(OBJ_SL2) $(OBJ_SL3) $(OBJ_SL4) $(OBJ_T1) $(T1)

