all: bin/myshell

bin/myshell: obj/main.o obj/shell.o obj/execute.o
	mkdir -p bin
	gcc obj/main.o obj/shell.o obj/execute.o -o bin/myshell -lreadline
obj/main.o: src/main.c
	mkdir -p obj
	gcc -Wall -Iinclude -c src/main.c -o obj/main.o

obj/shell.o: src/shell.c
	mkdir -p obj
	gcc -Wall -Iinclude -c src/shell.c -o obj/shell.o

obj/execute.o: src/execute.c
	mkdir -p obj
	gcc -Wall -Iinclude -c src/execute.c -o obj/execute.o

clean:
	rm -rf obj bin

.PHONY: all clean

