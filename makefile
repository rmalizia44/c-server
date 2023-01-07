all:
	gcc -o bin/server src/*.c -luv -Wall -Werror -ggdb