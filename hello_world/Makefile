all : hello

%.o : %.c
	gcc -c $* -o $@

hello_world : hello_world.o
	gcc -s $* -o $@

clean : clean_obj
	rm hello

clean_obj :
	rm *.o
