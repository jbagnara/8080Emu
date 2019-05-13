#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int disassemble(unsigned char*, int);

int main(int argc, char **argv[]){
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

	int out = open("out.txt", O_WRONLY);
	int tmpout = dup(1);
	dup2(out, 1);
	while(pc < size)
		pc+=disassemble(buffer, pc);
	dup2(1, tmpout);

	return 0;
}


/*memBuff is pointer to buffer containing assembly instructions
pc is program counter. Returns byte size of instruction*/
int disassemble(unsigned char* memBuff, int pc){
	unsigned char* instr = memBuff+pc;	//current instruction
	int opbytes = 1;	//size of current instruction
	printf("%04d ", pc);
	switch(*instr){
		case 0x00: printf("NOP"); break;
		case 0x01: printf("LXI B, #$%02x%02x", memBuff+pc+2, memBuff+pc+1); opbytes = 3; break; 	//Loads 16 bit address into register B
		case 0x02: printf("STAX B"); break;	//stores contents of accumulator in B
		case 0x03: printf("INX B"); break;	//Increments value in B by 1
		case 0x04: printf("INR B"); break;	//Increment low order byte
		case 0x05: printf("DCR B"); break;	//Decrements value in B by 1
		case 0x06: printf("MVI B, #$%02x", memBuff+pc+1); opbytes = 2; break;	//Load 16 bit value into address B
		case 0x07: printf("RLC"); break;	//Rotate accumulator left
		case 0x08: printf("NOP"); break;
		case 0x09: printf("DAD B"); break;	//Double increment value in B
		case 0x0A: printf("LDAX B"); break;	
		case 0x0B: printf("DCX B"); break;	//Decrement register pair B C by 1
		case 0x0C: printf("INR C"); break; 
		case 0x0D: printf("DCR C"); break;
		case 0x0E: printf("MVI C, #$%02x", memBuff+pc+1); opbytes = 2; break;
		case 0x0F: printf("RRC"); break;	//Rotate accumulator right
		case 0x10: printf("NOP"); break;
		case 0x11: printf("LXI D, #$%02x%02x", memBuff+pc+2, memBuff+pc+1); opbytes = 3; break;
		case 0x12: printf("STAX D"); break;
		case 0x13: printf("INX D"); break;
		case 0x14: printf("INR D"); break;
		case 0x15: printf("DCR D"); break;
		case 0x16: printf("MVI D, #$%02x", memBuff+pc+1); break;
		case 0x17: printf("RAL"); break;	//Rotate accumulator left through carry
		case 0x18: printf("NOP"); break;
		case 0x19: printf("DAD D"); break;
		case 0x1A: printf("LDAX D"); break;
		case 0x1B: printf("DCX D"); break;
		case 0x1C: printf("INR E"); break;
		case 0x1D: printf("DCR E"); break;
		case 0x1E: printf("MVI E, %$%02x", memBuff+pc+1); break;
		case 0x1F: printf("RAR"); break;

		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
		
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F:

		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:

		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x6F:

		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F:

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:

		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:

		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAE:
		case 0xAF:

		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF:

		case 0xC0:
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:

		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDD:
		case 0xDE:
		case 0xDF:

		case 0xE0:
		case 0xE1:
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 0xEF:

		case 0xF0:
		case 0xF1:
		case 0xF2:
		case 0xF3:
		case 0xF4:
		case 0xF5:
		case 0xF6:
		case 0xF7:
		case 0xF8:
		case 0xF9:
		case 0xFA:
		case 0xFB:
		case 0xFC:
		case 0xFD:
		case 0xFE:
		case 0xFF:

		default: printf("***UNKNOWN INSTRUCTION***"); break;
	}
	printf("\n");
	return opbytes;
}	
