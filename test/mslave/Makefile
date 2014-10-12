#
# test oht ocfg ofnv1a ocrc Makefile
#

UNITY_ROOT = ../unity
CC = gcc

TARGET1 = all_tests_oht.out
TARGET2 = all_tests_ocfg.out
TARGET3 = all_tests_ofnv1a.out
TARGET4 = all_tests_ocrc.out
TARGET5 = all_tests_ort.out

SRC_FILES_UNITY = \
$(UNITY_ROOT)/src/unity.c \
$(UNITY_ROOT)/extras/fixture/src/unity_fixture.c

SRC_FILES1 = \
	../../src/tools/oht.c \
	../oht/Test_oht.c \
	../oht/test_runners/Test_oht_Runner.c \
	../oht/test_runners/all_tests.c

SRC_FILES2 = \
	../../src/tools/oht.c \
	../../src/tools/ocfg.c \
	../ocfg/Test_ocfg.c \
	../ocfg/test_runners/Test_ocfg_Runner.c \
	../ocfg/test_runners/all_tests.c

SRC_FILES3 = \
	../../src/tools/ofnv1a.c \
	../ofnv1a/Test_ofnv1a.c \
	../ofnv1a/test_runners/Test_ofnv1a_Runner.c \
	../ofnv1a/test_runners/all_tests.c

SRC_FILES4 = \
	../../src/tools/ocrc.c \
	../ocrc/Test_ocrc.c \
	../ocrc/test_runners/Test_ocrc_Runner.c \
	../ocrc/test_runners/all_tests.c

SRC_FILES5 = \
	../../src/tools/oht.c \
	../../src/tools/ort.c \
	../ort/Test_ort.c \
	../ort/test_runners/Test_ort_Runner.c \
	../ort/test_runners/all_tests.c

INC_DIRS = -Isrc -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src -I../../src/tools

CLEANUP = rm -f build/*.o ; rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

all:	clean default

default:
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC_FILES1) -o $(TARGET1)
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC_FILES2) -o $(TARGET2)
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC_FILES3) -o $(TARGET3)
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC_FILES4) -o $(TARGET4)
	$(CC) $(INC_DIRS) $(SRC_FILES_UNITY) $(SRC_FILES5) -o $(TARGET5)
	./$(TARGET1)
	./$(TARGET2)
	./$(TARGET3)
	./$(TARGET4)
	./$(TARGET5)

clean:
	$(CLEANUP)

