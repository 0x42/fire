# =============================================================================
# 0x42
# test nettcp/(bo_net.c, bo_net_fifo_server.c)
# =============================================================================

UNITY_ROOT = ../unity
CC = gcc

TARGET1 = test_netfifo.out
TARGET2 = test_netroute.out

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
    ../../src/log/bologging.c \
    ../../src/tools/ocrc.c

SRC2 = \
    ../netroute/test_runners/Test_route_Runners.c \
    ../netroute/test_runners/all_tests.c \
    ../netroute/testroutenet.c \
    ../../src/tools/ocrc.c \
    ../../src/tools/dbgout.c

INC_DIRS = -Isrc -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src \
-Isrc/nettcp -Isrc/tools


fifo:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC1) -o $(TARGET1)
	./$(TARGET1)

route:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC2) -o $(TARGET2)
	./$(TARGET2)
	
clean:
	rm $(TARGET1)