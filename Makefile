PROJECT_NAME=build/fire
CFLAGS=-g -Wall

all: main.o logging.o
	gcc $(CFLAGS) -o $(PROJECT_NAME) main.o logging.o

logging.o:
	gcc $(CFLAGS) -c src/log/logging.h src/log/logging.c

main.o:
	gcc $(CFLAGS) -c src/main.c

clean:
	rm *.o
	rm $(PROJECT_NAME)
