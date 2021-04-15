#include <stdio.h>

#include "emulator.h"

int main(int argc, char *argv[]){
	FILE * Rom = fopen(argv[1], "r");

	startEmulation(Rom);
}

