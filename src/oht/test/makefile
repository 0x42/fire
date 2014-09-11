#
# test oht Makefile
#

UNITY_ROOT=unity
C_COMPILER=gcc
TARGET_BASE1=all_tests
TARGET_EXTENSION=.out
TARGET1 = $(TARGET_BASE1)$(TARGET_EXTENSION)
SRC_FILES1=\
$(UNITY_ROOT)/src/unity.c \
$(UNITY_ROOT)/extras/fixture/src/unity_fixture.c \
	../src/oht.c \
	test/Test_oht.c \
	test/test_runners/Test_oht_Runner.c \
	test/test_runners/all_tests.c
INC_DIRS=-Isrc -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src -I../src
SYMBOLS=

CLEANUP = rm -f build/*.o ; rm -f $(TARGET1)

all:	clean default

default:
	$(C_COMPILER) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES1) -o $(TARGET1)
	./$(TARGET1)

clean:
	$(CLEANUP)

