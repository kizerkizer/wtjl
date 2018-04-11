default:
	gcc -static -std=gnu99 -g interpreter.c -E > ./out/preprocessed.c
	gcc -static -std=gnu99 -g interpreter.c -o wtjl
