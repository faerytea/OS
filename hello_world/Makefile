all : hello_world

%.o : %.c
	gcc -c $* -o $@

hello_world : hello_world.o
	gcc -s $* -o $@

clean : clean_obj
	rm hello_world

clean_obj :
	rm *.o
