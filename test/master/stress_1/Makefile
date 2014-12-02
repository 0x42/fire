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
    ../../../src/nettcp/bo_cycle_arr.c \
    ../../../src/nettcp/bo_net_master_core_log.c \
    ../../../src/nettcp/bo_net_master_core.c \
    main.c

default:
	$(CC)  $(INC_DIRS) $(SRC1) -lpthread -o $(TARGET)
