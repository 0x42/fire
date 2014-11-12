# 0x42

TARGET = stress

CC = gcc

INC_DIRS = -Isrc -Isrc/nettcp -Isrc/tools

SRC1 = \
    ../../../src/nettcp/bo_net.c \
    ../../../src/log/bologging.c \
    ../../../src/tools/dbgout.c  \
    ../../../src/tools/ocrc.c    \
    ../../../src/tools/oht.c     \
    ../../../src/nettcp/bo_net_master_core.c \
    main.c

default:
	$(CC) -lpthread $(INC_DIRS) $(SRC1) -o $(TARGET)
