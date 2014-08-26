PROJECT_NAME=build/fire
CFLAGS=-g

all: clean main.o
	gcc $(CFLAGS) -o $(PROJECT_NAME) main.o

main.o:
	gcc $(CFLAGS) -c src/main.c

clean:
	rm *.o
	rm $(PROJECT_NAME)
