PROJECT_NAME=build/fire
CFLAGS=-g -Wall
INC_TEST = -Itest/unity/src
INC_SRC = -Isrc/log -Isrc/
SRC_LOG = src/log/logging.c src/log/tools.c
UNITY = test/unity/src/unity.c
SRC_TEST = test/maintest.c test/logtest.c

all: main.o logging.o tools.o
	gcc $(CFLAGS) -o $(PROJECT_NAME) main.o logging.o tools.o 
logging.o:
	gcc $(CFLAGS) -c src/log/logging.h src/log/logging.c
tools.o:
	gcc $(CFLAGS) -c src/tools/dbgout.h src/tools/dbgout.c
main.o:
	gcc $(CFLAGS) -c src/main.c

runtest:
	gcc $(INC_TEST) $(INC_SRC) $(SRC_LOG) $(UNITY) $(SRC_TEST) -o test1   
	./test1
clean:
	rm *.o
	rm $(PROJECT_NAME)