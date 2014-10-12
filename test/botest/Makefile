# =============================================================================
# 0x42
# test nettcp/(bo_net.c, bo_net_fifo_server.c)
# =============================================================================

UNITY_ROOT = ../unity
CC = gcc

TARGET1 = test_netfifo.out

SRC_FILES_UNITY = \
$(UNITY_ROOT)/src/unity.c \
$(UNITY_ROOT)/extras/fixture/src/unity_fixture.c

SRC1 = \
    ../netfifo/test_runners/Test_fifo_Runners.c \
    ../netfifo/test_runners/all_tests.c \
    ../netfifo/testfifonet.c \
    ../../src/nettcp/bo_fifo.c \
    ../../src/tools/ocrc.c

INC_DIRS = -Isrc -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src \
-Isrc/nettcp -Isrc/tools


default:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC1) -o $(TARGET1)
	./$(TARGET1)
	
clean:
	rm $(TARGET1)