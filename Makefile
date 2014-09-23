TARGET    = build/fire
TARGET_T  = build/test1 
TARGET_T1 = build/test2 


#CC = arm-elf-gcc
CC = gcc

INC_DIR      = -Isrc/log -Isrc/tools -I/usr/local/arm-elf/include
INC_DIR_TEST = -Itest/unity/src

CFLAGS  = -g -Wall -c

LDFLAGS = 
#LDFLAGS = -Wl, -elf2flt

SRC    = src/main.c
SRC1   = src/tools/dbgout.c
SRC2   = src/log/logging.c
SRC3   = src/log/robolog.c
SRC4   = src/tools/linkedlist.c
SRC5   = src/tools/bmempool.c

SRC_T  = test/maintest.c
SRC_T1 = test/unity/src/unity.c
SRC_T2 = test/logtest.c 
SRC_T3 = test/robologtest.c
SRC_T4 = test/llist_test.c

SRC_TT = test/mainmem.c
SRC_TT1 = test/mempool_test.c

OBJ    = build/main.o
OBJ1   = build/dbgout.o
OBJ2   = build/logging.o
OBJ3   = build/robolog.o
OBJ4   = build/linkedlist.o
OBJ5   = build/bmempool.o	

OBJ_T  = build/maintest.o 
OBJ_T1 = build/unity.o
OBJ_T2 = build/logtest.o
OBJ_T3 = build/robologtest.o
OBJ_T4 = build/llist_test.o

OBJ_TT = build/mainmem.o
OBJ_TT1 = build/mempool_test.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3) -o $(OBJ3)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ) -o $(TARGET)

check:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3) -o $(OBJ3)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC4) -o $(OBJ4)

	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T1) -o $(OBJ_T1)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T2) -o $(OBJ_T2)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T3) -o $(OBJ_T3)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T4) -o $(OBJ_T4)
	$(CC) $(INC_DIR_TEST) $(INC_DIR) $(CFLAGS) $(SRC_T)  -o $(OBJ_T)
	$(CC) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ_T1) $(OBJ_T2) $(OBJ_T3) $(OBJ_T4) $(OBJ_T) -o $(TARGET_T)
	$(TARGET_T)

check2:
	echo "MEMORY - TEST "
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_T1) -o $(OBJ_T1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC5) -o $(OBJ5)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_TT1) -o $(OBJ_TT1)
	$(CC) $(INC_DIR) $(INC_DIR_TEST) $(CFLAGS) $(SRC_TT)  -o $(OBJ_TT)
	$(CC) $(LDFLAGS) $(OBJ_T1) $(OBJ5) $(OBJ_TT1) $(OBJ_TT) -o $(TARGET_T1)
# перем окр в прилож испол др набор функций для работы с паммятью
# выявляет двойной free() на один указ и однобайтовое перепол буфера
# находит ошибки переполнения буфера в случае вызова free на эту
# область памяти; программа mtrace позволяет прочитать файл трассировки
# чтобы найти место где программа упала 
#$ MALLOC_CHECK_=2 gdb <имя программы> -> run -> where
	MALLOC_CHECK_=2 $(TARGET_T1) 
	echo "MEM TEST 2"
	MALLOC_TRACE=trace.log $(TARGET_T1)
	mtrace $(TARGET_T1) trace.log
	
clean:
	rm build/*.o
	rm $(TARGET)
	rm *.err
