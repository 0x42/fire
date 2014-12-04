# =============================================================================
# 0x42
# =============================================================================

UNITY_ROOT = ../unity
CC = gcc

TARGET = test.out

SRC_FILES_UNITY = \
$(UNITY_ROOT)/src/unity.c \
$(UNITY_ROOT)/extras/fixture/src/unity_fixture.c

SRC1 = \
    ../netfifo/test_runners/Test_fifo_Runners.c \
    ../netfifo/test_runners/all_tests.c \
    ../netfifo/testfifonet.c \
    ../../src/nettcp/bo_fifo.c \
    ../../src/nettcp/bo_net_get.c \
    ../../src/nettcp/bo_net.c \
    ../../src/tools/ocfg.c \
    ../../src/tools/oht.c \
    ../../src/tools/dbgout.c \
    ../../src/nettcp/bo_net_fifo_server.c \
    ../../src/log/bologging.c \
    ../../src/tools/ocrc.c

SRC3 = \
    ../bolog/test_runners/Test_log_Runners.c \
    ../bolog/test_runners/all_tests.c \
    ../bolog/logtest.c \
    ../../src/log/bologging.c 

SRC4 = \
    ../llist/test_runners/Test_llist_Runners.c \
    ../llist/test_runners/all_tests.c \
    ../llist/testllist.c \
    ../../src/tools/listsock.c

SRC5 = \
    ../master/test_runners/Test_master_Runners.c \
    ../master/test_runners/all_tests.c \
    ../master/testmaster.c \
    ../../src/log/bologging.c \
    ../../src/nettcp/bo_net.c \
    ../../src/tools/ocrc.c \
    ../../src/tools/oht.c \
    ../../src/nettcp/bo_net_master_core.c \
    ../../src/tools/dbgout.c \
    ../../src/nettcp/bo_cycle_arr.c \
    ../../src/nettcp/bo_net_master_core_log.c

SRC6 = \
    ../cycarr/test_runners/Test_cycarr_Runners.c \
    ../cycarr/test_runners/all_tests.c \
    ../cycarr/testcycarr.c             \
    ../../src/nettcp/bo_cycle_arr.c    

INC_DIRS = -Isrc -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src \
-Isrc/nettcp -Isrc/tools


fifo:
	$(CC)  $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC1) -lpthread -o  $(TARGET)
	./$(TARGET)

llist:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC4) -o $(TARGET)
	./$(TARGET)

log:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) -lpthread $(SRC3) -o $(TARGET)
	./$(TARGET)

master:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) -lpthread $(SRC5)  -o $(TARGET)
	./$(TARGET)

cycarr:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC6) -o $(TARGET)
	./$(TARGET)

clean:
	rm $(TARGET)