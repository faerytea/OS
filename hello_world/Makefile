all : hello_world

%.o : %.c
	gcc -c $? -o $@ 

hello_world : hello_world.o
	gcc -s $? -o $@

clean : clean_obj
	rm -f hello_world

clean_obj :
	rm -f *.o
