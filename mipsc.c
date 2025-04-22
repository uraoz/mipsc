#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr,"引数が正しくない\n");
		return 1;
	}

	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");
	printf("	li $a0, %d\n",atoi(argv[1]));
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}