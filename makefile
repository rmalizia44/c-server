all:
	gcc -o bin/server src/*.c demo/*.c -Isrc -luv -Wall -Werror -ggdb