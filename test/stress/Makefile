TARGET = stress

CC	     = arm-elf-gcc
LDFLAGS	     = -Wl, -elf2flt

INC_DIR      = -I../../src/log -I../../src/tools -I/usr/local/arm-elf/include
CFLAGS	     = -g -Wall -c

SRC    = stress.c
SRC1   = ../../src/tools/dbgout.c
SRC2   = ../../src/log/logging.c
SRC3   = ../../src/log/robolog.c

OBJ    = stress.o
OBJ1   = dbgout.o
OBJ2   = logging.o
OBJ3   = robolog.o

default:
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC1) -o $(OBJ1)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC2) -o $(OBJ2)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC3) -o $(OBJ3)
	$(CC) $(INC_DIR) $(CFLAGS) $(SRC)  -o $(OBJ)

	$(CC) $(LDFLAGS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ) -o $(TARGET)
	
clean:
	rm *.o
	rm $(TARGET)