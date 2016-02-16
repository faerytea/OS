all : cat

cat : cat.o libredir.a
	gcc $< -o $@ -L. -lredir

%.o : %.c
	gcc -c $? -o $@

lib%.a : %.o
	ar rcs $@ $<

lib%.so : %.o
	gcc -shared $< -o $@

clean : clean_obj clean_lib clean_shared
	rm -f cat

clean_obj :
	rm -f *.o

clean_lib :
	rm -f *.a

clean_shared :
	rm -f *.so
