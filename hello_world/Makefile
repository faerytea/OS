all : hello

%.o : %.c
	gcc -c $* -o $@

hello : hello_world.o
	gcc -s $* -o $@

clean : clean_obj
	rm hello

clean_obj :
	rm *.o
