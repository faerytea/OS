all : hello_world hello_static hello_shared

%.o : %.c
	gcc -c $? -o $@

hello_world : hello_world.o
	gcc -s $? -o $@

clean : clean_obj clean_lib clean_shared
	rm -f hello_world hello_shared hello_static

clean_obj :
	rm -f *.o

clean_lib :
	rm -f *.a

clean_shared :
	rm -f *.so

lib%.a : %.o
	ar rcs $@ $<

lib%.so : %.o
	gcc -shared $< -o $@

hello_static : hello_static.o libhello.a

hello_shared : hello_shared.o libhello.so
	gcc $< -o $@ -L. -lhello

run : hello_shared
	LD_LIBRARY_PATH=. ./hello_shared
