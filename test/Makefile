#
# test ocfg Makefile
#

UNITY_ROOT = unity
CC = gcc
TARGET1 = all_tests.out
SRC_FILES1 = \
$(UNITY_ROOT)/src/unity.c \
$(UNITY_ROOT)/extras/fixture/src/unity_fixture.c \
	../src/tools/oht.c \
	../src/tools/ocfg.c \
	ocfg/Test_ocfg.c \
	ocfg/test_runners/Test_ocfg_Runner.c \
	ocfg/test_runners/all_tests.c
INC_DIRS = -Isrc -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src -I../src/tools

CLEANUP = rm -f build/*.o ; rm -f $(TARGET1)

all:	clean default

default:
	$(CC) $(INC_DIRS) $(SRC_FILES1) -o $(TARGET1)
	./$(TARGET1)

clean:
	$(CLEANUP)

