all : hello

hello.o : hello.c
	gcc -c hello_world.c -o hello.o

hello : hello.o
	gcc -s hello.o -o hello.o

clean : clean_obj
	rm hello

clean_obj :
	rm hello.o
