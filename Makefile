PROJECT_NAME=build/fire
CFLAGS=-g -Wall

all: main.o logging.o tools.o
	gcc $(CFLAGS) -o $(PROJECT_NAME) main.o logging.o tools.o 

logging.o:
	gcc $(CFLAGS) -c src/log/logging.h src/log/logging.c

tools.o:
	gcc $(CFLAGS) -c src/log/tools.h src/log/tools.c
main.o:
	gcc $(CFLAGS) -c src/main.c

clean:
	rm *.o
	rm $(PROJECT_NAME)
