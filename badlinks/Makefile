all : run

clean : clean-dir
	rm -rf hw2-gen.sh

clean-dir : 
	rm -rf ./hw2-example

hw2-gen.sh :
	wget "http://neerc.ifmo.ru/~os/static/hw2-gen.sh"

hw2-example : hw2-gen.sh
	chmod a+x "hw2-gen.sh"
	./hw2-gen.sh

run : hw2-example
	chmod a+x "badlinks.sh"
	./badlinks.sh hw2-example

