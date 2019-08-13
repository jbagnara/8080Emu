#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "disassemble.h"

int main(int argc, char *argv[]){
	FILE * instrFile = fopen(argv[1], "r");
	if(instrFile==NULL){
		printf("instruction file not found\n");
		exit(1);
	}
	
	fseek(instrFile, 0L, SEEK_END);
	int size = ftell(instrFile);
	fseek(instrFile, 0L, SEEK_SET);

	unsigned char* buffer = malloc(size*sizeof(unsigned char));
	fread(buffer, size, sizeof(unsigned char), instrFile);
	fclose(instrFile);

	int pc = 0;

	int out = open("out.txt", O_CREAT | O_RDWR, 0664);
	int tmpout = dup(1);
	dup2(out, 1);
	while(pc < size)
		pc+=disassemble(buffer, pc);
	dup2(1, tmpout);

	close(out);

	return 0;
}


/*memBuff is pointer to buffer containing assembly instructions
pc is program counter. Returns byte size of instruction*/
