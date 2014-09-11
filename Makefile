TARGET   = build/fire
TARGET_T = build/test1 

#CC = arm-elf_gcc
CC = gcc

INC_DIR      = -Isrc/log -Isrc/tools -I/usr/local/arm-elf/include
INC_DIR_TEST = -Itest/unity/src

CFLAGS = -g -Wall -c

SRC    = src/main.c
SRC1   = src/tools/dbgout.c
SRC2   = src/log/logging.c

SRC_T  = test/maintest.c
SRC_T1 = test/unity/src/unity.c
SRC_T2 = test/logtest.c 

OBJ    = build/main.o
OBJ1   = build/dbgout.o
OBJ2   = build/logging.o

OBJ_T  = build/maintest.o 
OBJ_T1 = build/unity.o
OBJ_T2 = build/logtest.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ) -o $(TARGET)

check:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T1) -o $(OBJ_T1)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T2) -o $(OBJ_T2)
	$(CC) $(INC_DIR_TEST) $(INC_DIR) $(CFLAGS) $(SRC_T)  -o $(OBJ_T)
	$(CC) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ_T1) $(OBJ_T2) $(OBJ_T) -o $(TARGET_T)
	$(TARGET_T)

clean:
	rm build/*.o
	rm $(TARGET)
