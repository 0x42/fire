PROJECT_NAME=build/fire

all: clean main.o
	gcc -o $(PROJECT_NAME) main.o

main.o:
	gcc -c src/main.c

clean:
	rm *.o
	rm $(PROJECT_NAME)
