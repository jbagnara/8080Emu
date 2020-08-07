#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include "disassemble.h"

typedef struct flags{
	uint8_t S:1;	//sign flag
	uint8_t Z:1;	//zero flag
	uint8_t A:1;	//auxiliary carry flag
	uint8_t P:1;	//parity flag
	uint8_t C:1;	//carry flag
	uint8_t x:3;	//extra bits
} flags;

typedef struct state8080{
	uint8_t A;	//register A (accumulator) pairs F (flags)
	uint8_t B;	//register B pairs C
	uint8_t D;	//register D pairs E
	uint8_t H;	//register H pairs L
	uint8_t F;
	uint8_t C;
	uint8_t E;
	uint8_t L;
	uint8_t* memBuff;
	uint16_t SP;	//stack pointer
	uint16_t PC;	//function pointer
	struct flags f;
} state8080;

SDL_Renderer *renderer;
uint8_t* memBuff;
int emulate(state8080*);
int parity(uint32_t, int);
void* drawScreen(void* p);

int fps = 60;
int cpuClock = 3125000;

int main(int argc, char *argv[]){

	state8080* state = calloc(1, sizeof(state8080));
	state->memBuff = malloc(0x10000);
	memBuff = state->memBuff;

	FILE * instrFile = fopen(argv[1], "r");
	if(instrFile==NULL){
		printf("instruction file not found\n");
		exit(1);
	}
	
	fseek(instrFile, 0L, SEEK_END);
	int size = ftell(instrFile);
	fseek(instrFile, 0L, SEEK_SET);

	fread(state->memBuff, size, sizeof(unsigned char), instrFile);
	fclose(instrFile);

	SDL_Window *win;
	SDL_CreateWindowAndRenderer(500, 500, 0, &win, &renderer);
	SDL_Event events;

	pthread_t id;
	pthread_create(&id, NULL, drawScreen, NULL );

	while(1){
		//SDL_Texture* buffer = SDL_CreateTexture(render, 
		if(events.type==SDL_QUIT)
			break;
		emulate(state);
        usleep(50); //TODO change this
	}

	SDL_Quit();
	free(state->memBuff);
	free(state);

	return 0;
}

void* drawScreen(void* p){
	uint8_t* fb = memBuff+0x2400;
	
	while(1){
		SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 224);
		SDL_SetRenderTarget(renderer, texture);
      	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      	SDL_RenderClear(renderer);
      	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);

		for(int x=0; x<32; x++){
			for(int bit=0; bit<8; bit++){
				for(int y=0; y<224; y++){
					if(fb[x+32*y] & (1 << bit)){
						SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
						SDL_RenderDrawPoint(renderer, x*8+bit, y);
					} else {
						//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
						//SDL_RenderDrawPoint(renderer, x*8+bit, y);
					}
				}
			}
		}
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopyEx(renderer, texture, NULL, NULL, 270, NULL, SDL_FLIP_NONE);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);
        SDL_Delay(1/fps); //TODO change this
	}
}

/*counts number of 1 bits in string and returns 1 for even total, 0 for odd total*/
int parity(uint32_t string, int size){
	int total = 0;
	for(int x=0; x<size; x++){
		if(((string >> x) & 0x0001) == 0x0001)
			total+=1;
	}
	return (total+1)%2;	
}
/*memBuff is pointer to buffer containing assembly instructions
pc is program counter. Returns byte size of instruction*/
int emulate(state8080* state){
	unsigned char* instr = state->memBuff+state->PC;	//current instruction
	//disassemble(state->memBuff, state->PC);
	
	printf("0x%02x:	 a: %02x szapc: %d%d%d%d%d\
	  bc: %02x%02x\
	  de: %02x%02x\
	  hl: %02x%02x\
	  pc: %02x\
	  sp: %02x\n", *instr,
	  state->A, state->f.S, state->f.Z, state->f.A,
	  state->f.P, state->f.C, state->B, state->C, state->D, 
	  state->E, state->H, state->L, state->PC, state->SP);

	switch(*instr){
		case 0x00: break;
		case 0x01:	//LXI, Loads 16 bit address into register B
			state->B = state->memBuff[state->PC+2];
			state->C = state->memBuff[state->PC+1];
			state->PC+=2;
			break;
		case 0x02:	//STAX, stores contents of accumulator in B
			state->B = state->A;	
			break;

		case 0x03:	//INX, Increments value in pair B,C by 1
			{
				uint16_t BC = (state->B << 8) | state->C;
				uint16_t res = BC+=1;
				state->B = (res >> 8) & 0xFF;
				state->C = res & 0xFF;
			}
			break;
		case 0x04:	//INR, Increments value in register B by 1
			{
				uint8_t res = state->B+1;
				state->f.S = res >> 7;
				state->f.Z = (res == 0x00);
				state->f.A = (((state->B << 4) >> 4) + 1) > 0x0F;
				state->f.P = parity((uint32_t) res, 8);
				state->B = res;
			}
			break;
			
		case 0x05:	//DCR, Decrements value in B by 1
			{
				uint8_t res = (uint8_t)state->B-1;
				state->f.S = res >> 7;
				state->f.Z = (res == 0x00);
				state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
				state->f.P = parity((uint32_t) res, 8);
				state->B = res;
			}
			break;

		case 0x06:	//MVI, Loads 8 bit decimal into register B
			state->B = state->memBuff[state->PC+1];
			state->PC+=1;
			break;

		//case 0x07: printf("RLC"); break;	//Rotate accumulator left
		//case 0x08: printf("NOP"); break;
		case 0x09:{	//DAD B 
			uint32_t hl = state-> H << 8 | state->L;
			uint32_t bc = state-> B << 8 | state->C;
			uint32_t res = hl + bc;
			state->H = (res >> 8) & 0xFF;
			state->L = res & 0xFF;
			state->f.C = (res & 0xFFFF0000) > 1;
			break;
		}		
		//case 0x0A: printf("LDAX B"); break;	
		//case 0x0B: printf("DCX B"); break;	//Decrement register pair B C by 1
		//case 0x0C: printf("INR C"); break; 
		case 0x0D: {	//DCR C
			uint8_t res = (uint8_t)state->C-1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);
			state->C = res;
			break;
		}

		case 0x0E:	//MVI, Loads 8 bit decimal into register C
			state->C = state->memBuff[state->PC+1];
			state->PC+=1;
			break;

		case 0x0F:{	// RRC (Rotate accumulator right)
			state->A = state->A << 7 | state->A >> 1;	//TODO make sure that's right
		}
		//case 0x10: printf("NOP"); break;

		case 0x11:	//LXI, Loads 16 bit address into register pair DE
			state->D = state->memBuff[state->PC+2];
			state->E = state->memBuff[state->PC+1];
			state->PC+=2;
			break;


		//case 0x12: printf("STAX D"); break;
		case 0x13:{ //INX D increment register pair D by 1
			uint16_t res = (state->D << 8 | state->E) + 0x0001;
			state->D = res >> 8;
			state->E = res & 0xFF;
			}
			break;
		//case 0x14: printf("INR D"); break;
		//case 0x15: printf("DCR D"); break;
		//case 0x16: printf("MVI D, #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0x17: printf("RAL"); break;	//Rotate accumulator left through carry
		//case 0x18: printf("NOP"); break;
		case 0x19:{ //DAD D	
			uint32_t hl = state-> H << 8 | state->L;
			uint32_t de = state-> D << 8 | state->E;
			uint32_t res = hl + de;
			state->H = (res >> 8) & 0xFF;
			state->L = res & 0xFF;
			state->f.C = (res & 0xFFFF0000) > 1;
			break;
		}

		case 0x1A:{	//LDAX D, Loads address pointed by register pair DE into accumulator A
			uint16_t DE =  state->D << 8 | state->E;
			state->A = state->memBuff[DE];
			break;
		}
			

		//case 0x1B: printf("DCX D"); break;
		//case 0x1C: printf("INR E"); break;
		//case 0x1D: printf("DCR E"); break;
		//case 0x1E: printf("MVI E, #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0x1F: printf("RAR"); break;

		//case 0x20: printf("NOP"); break;
		case 0x21:	//LXI, Loads 16 bit address into register pair HL
			state->H = *(state->memBuff+state->PC+2);
			state->L = *(state->memBuff+state->PC+1);
			state->PC+=2;
			break;

		//case 0x22: printf("SHLD #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;			//value of L is stored at location 16 bit address points to, value of H is stored in next highest address

		case 0x23:{	//INX H Increment register pair H by 1
			uint16_t res = ((state->H << 8) | (state-> L)) + 1;
			state->H = res >> 8;
			state->L = res & 0xFF;
			break;
		}

		//case 0x24: printf("INR H"); break;
		//case 0x25: printf("DCR H"); break;
		case 0x26:{	//MVI H, D8 
			state->H = state->memBuff[state->PC+1];
			state->PC+=1;
			break;
		}
		//case 0x27: printf("DAA"); break;
		//case 0x28: printf("NOP"); break;
		case 0x29:{	//DAD H
			uint32_t hl = state-> H << 8 | state->L;
		 	hl = hl << 1;
			state->H = hl >> 8;
			state->L = hl & 0xFF;
			state->f.C = (hl & 0xFFFF0000) > 0;
			break;
		}
		//case 0x2A: printf("LHLD #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0x2B: printf("DCX H"); break;
		//case 0x2C: printf("INR L"); break;
		//case 0x2D: printf("DCR L"); break;
		//case 0x2E: printf("MVI L, #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0x2F: printf("CMA"); break;
		
		//case 0x30: printf("NOP"); break;
		case 0x31:	//LXI,	Loads 16 bit address into SP
			state->SP = state->memBuff[state->PC+2] << 8 |
					state->memBuff[state->PC+1];
			state->PC+=2;
			break;

		//case 0x32: printf("STA #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0x33: printf("INX SP"); break;
		//case 0x34: printf("INR M"); break;
		//case 0x35: printf("DCR M"); break; 
		case 0x36:{	//MVI M, d8
			uint16_t HL = state->H << 8 | state->L;	
			state->memBuff[HL] = state->memBuff[state->PC+1];
			state->PC+=1;
			break;
		}

		//case 0x37: printf("STC"); break;
		//case 0x38: printf("NOP"); break;
		//case 0x39: printf("DAD SP"); break;
		//case 0x3A: printf("LDA #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0x3B: printf("DCX SP"); break;
		//case 0x3C: printf("INR A"); break;
		//case 0x3D: printf("DCR A"); break;
		//case 0x3E: printf("MVI A, #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0x3F: printf("CMC"); break;

		//case 0x40: printf("MOV B, B"); break;
		//case 0x41: printf("MOV B, C"); break;
		//case 0x42: printf("MOV B, D"); break;
		//case 0x43: printf("MOV B, E"); break;
		//case 0x44: printf("MOV B, H"); break;
		//case 0x45: printf("MOV B, L"); break;
		//case 0x46: printf("MOV B, M"); break;
		//case 0x47: printf("MOV B, A"); break;
		//case 0x48: printf("MOV C, B"); break;
		//case 0x49: printf("MOV C, C"); break;
		//case 0x4A: printf("MOV] | state->memBuff[state->SP+1]; C, D"); break;
		//case 0x4B: printf("MOV C, E"); break;
		//case 0x4C: printf("MOV C, H"); break;
		//case 0x4D: printf("MOV C, L"); break;
		//case 0x4E: printf("MOV C, M"); break;
		//case 0x4F: printf("MOV C, A"); break;

		//case 0x50: printf("MOV D, B"); break;
		//case 0x51: printf("MOV D, C"); break;
		//case 0x52: printf("MOV D, D"); break;
		//case 0x53: printf("MOV D, E"); break;
		//case 0x54: printf("MOV D, H"); break;
		//case 0x55: printf("MOV D, L"); break;
		case 0x56:{ //Mov M -> D
			uint16_t M = state->H << 8 | state->L;
			state->D = state->memBuff[M];
			break;
		}
		//case 0x57: printf("MOV D, A"); break;
		//case 0x58: printf("MOV E, B"); break;
		//case 0x59: printf("MOV E, C"); break;
		//case 0x5A: printf("MOV E, D"); break;
		//case 0x5B: printf("MOV E, E"); break;
		//case 0x5C: printf("MOV E, H"); break;
		//case 0x5D: printf("MOV E, L"); break;
		case 0x5E:{ //Mov M -> E
			uint16_t M = state->H << 8 | state->L;
			state->E = state->memBuff[M];
			break;
		}
		//case 0x5F: printf("MOV E, A"); break;

		//case 0x60: printf("MOV H, B"); break;
		//case 0x61: printf("MOV H, C"); break;
		//case 0x62: printf("MOV H, D"); break;
		//case 0x63: printf("MOV H, E"); break;
		//case 0x64: printf("MOV H, H"); break;
		//case 0x65: printf("MOV H, L"); break;
		case 0x66:{ // Mov M -> H 
			uint16_t M = state->H << 8 | state->L;
			state->H = state->memBuff[M];
			break;
		}
		//case 0x67: printf("MOV H, A"); break;
		//case 0x68: printf("MOV L, B"); break;
		//case 0x69: printf("MOV L, C"); break;
		//case 0x6A: printf("MOV L, D"); break;
		//case 0x6B: printf("MOV L, E"); break;
		//case 0x6C: printf("MOV L, H"); break;
		//case 0x6D: printf("MOV L, L"); break;
		//case 0x6E: printf("MOV L, M"); break;
		case 0x6F:{	//MOV A -> L
			state->L = state->A;
			break;	
		}

		//case 0x70: printf("MOV M, B"); break;
		//case 0x71: printf("MOV M, C"); break;
		//case 0x72: printf("MOV M, D"); break;
		//case 0x73: printf("MOV M, E"); break;
		//case 0x74: printf("MOV M, H"); break;
		//case 0x75: printf("MOV M, L"); break;
		//case 0x76: printf("HLT"); break;
		case 0x77:{ //Move A -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->A;
			break;
		}
			
		//case 0x78: printf("MOV A, B"); break;
		//case 0x79: printf("MOV A, C"); break;
		case 0x7A:{ // Mov D -> A
			state->A = state->D;
			break;
		}
		//case 0x7B: printf("MOV A, E"); break;
		case 0x7C: //Move H -> A
			state->A = state->H;
			break;
		//case 0x7D: printf("MOV A, L"); break;
		case 0x7E:{ // Mov M -> A
			uint16_t M = state->H << 8 | state->L;
			state->A = state->memBuff[M];
			break;
		}
		//case 0x7F: printf("MOV A, A"); break;

		//case 0x80: printf("ADD B"); break;
		//case 0x81: printf("ADD C"); break;
		//case 0x82: printf("ADD D"); break;
		//case 0x83: printf("ADD E"); break;
		//case 0x84: printf("ADD H"); break;
		//case 0x85: printf("ADD L"); break;
		//case 0x86: printf("ADD M"); break;
		//case 0x87: printf("ADD A"); break;
		//case 0x88: printf("ADC B"); break;	//add with carry
		//case 0x89: printf("ADC C"); break;
		//case 0x8A: printf("ADC D"); break;
		//case 0x8B: printf("ADC E"); break;
		//case 0x8C: printf("ADC H"); break;
		//case 0x8D: printf("ADC L"); break;
		//case 0x8E: printf("ADC M"); break;
		//case 0x8F: printf("ADC A"); break;

		//case 0x90: printf("SUB B"); break;
		//case 0x91: printf("SUB C"); break;
		//case 0x92: printf("SUB D"); break;
		//case 0x93: printf("SUB E"); break;
		//case 0x94: printf("SUB H"); break;
		//case 0x95: printf("SUB L"); break;
		//case 0x96: printf("SUB M"); break;
		//case 0x97: printf("SUB A"); break;
		//case 0x98: printf("SBB B"); break;	//subtract with borrow
		//case 0x99: printf("SBB C"); break;
		//case 0x9A: printf("SBB D"); break;
		//case 0x9B: printf("SBB E"); break;
		//case 0x9C: printf("SBB H"); break;
		//case 0x9D: printf("SBB L"); break;
		//case 0x9E: printf("SBB M"); break;
		//case 0x9F: printf("SBB A"); break;

		//case 0xA0: printf("ANA B"); break;	//AND B with REGM
		//case 0xA1: printf("ANA C"); break;
		//case 0xA2: printf("ANA D"); break;
		//case 0xA3: printf("ANA E"); break;
		//case 0xA4: printf("ANA H"); break;
		//case 0xA5: printf("ANA L"); break;
		//case 0xA6: printf("ANA M"); break;
		//case 0xA7: printf("ANA A"); break;
		//case 0xA8: printf("XRA B"); break;	//XOR with REGM
		//case 0xA9: printf("XRA C"); break;
		//case 0xAA: printf("XRA D"); break;
		//case 0xAB: printf("XRA E"); break;
		//case 0xAC: printf("XRA H"); break;
		//case 0xAD: printf("XRA L"); break;
		//case 0xAE: printf("XRA M"); break;
		//case 0xAF: printf("XRA A"); break;

		//case 0xB0: printf("ORA B"); break;
		//case 0xB1: printf("ORA C"); break;
		//case 0xB2: printf("ORA D"); break;
		//case 0xB3: printf("ORA E"); break;
		//case 0xB4: printf("ORA H"); break;
		//case 0xB5: printf("ORA L"); break;
		//case 0xB6: printf("ORA M"); break;
		//case 0xB7: printf("ORA A"); break;
		//case 0xB8: printf("CMP B"); break;
		//case 0xB9: printf("CMP C"); break;
		//case 0xBA: printf("CMP D"); break;
		//case 0xBB: printf("CMP E"); break;
		//case 0xBC: printf("CMP H"); break;
		//case 0xBD: printf("CMP L"); break;
		//case 0xBE: printf("CMP M"); break;
		//case 0xBF: printf("CMP A"); break;

		//case 0xC0: printf("RNZ"); break;
		case 0xC1:{ //POP B
			state->B = state->memBuff[state->SP+1];
			state->C = state->memBuff[state->SP];
			state->SP+=2;
			break;	
		}

		case 0xC2: //JNZ a16, If not Zero flag, jump to 16 bit address
			if(!state->f.Z){
				state->PC = state->memBuff[state->PC+2] << 8 |
						state->memBuff[state->PC+1];
				state->PC-=1;
			} else{
				state->PC+=2;
			} 
			break;
				
				

		case 0xC3:	//JMP ADDR, jump to 16 bit address
			state->PC = state->memBuff[state->PC+2] << 8 |
					state->memBuff[state->PC+1];
			state->PC-=1;
			break;
		//case 0xC4: printf("CNZ #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xC5:{	//PUSH B
			state->memBuff[state->SP-1] = state->B;
			state->memBuff[state->SP-2] = state->C;
			state->SP-=2;
			break;	
		}

		case 0xC6:{ //ADI (add immediate to accumulator)
			uint16_t imd = state->memBuff[state->PC+1];
			state->A+=imd;
			uint8_t res = state->A & imd;
			state->f.C = ((uint16_t)state->A + imd) >> 8;
			state->f.S = res >> 7;
			state->f.Z = res == 0;
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);	
			break;
		}
		//case 0xC7: printf("RST 0"); break;
		//case 0xC8: printf("RZ"); break;
		case 0xC9: //RET return from subroutine
			state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
			state->SP+=2;
			break;

		//case 0xCA: printf("JZ #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xCB: printf("JMP #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xCC: printf("CZ #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xCD: {	//CALL ADDR, jump to 16 bit address and push address to stack
			uint16_t ret = state->PC+2;
			state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
			state->memBuff[state->SP-2] = ret & 0xFF;
			state->SP-=2;
			state->PC = state->memBuff[state->PC+2] << 8 | 
					state->memBuff[state->PC+1];
			state->PC-=1;
			break;
		}


		//case 0xCE: printf("ACI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xCF: printf("RST 1"); break;

		//case 0xD0: printf("RNC"); break;
		case 0xD1: { //Pop D
			state->D = state->memBuff[state->SP+1];
			state->E = state->memBuff[state->SP];
			state->SP+=2;

			break;
		}
		//case 0xD2: printf("JNC #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xD3:{	//OUT D8
			uint8_t device = state->memBuff[state->PC+1];
			state->PC+=1;

			//SOMETHING
			break;
		}
		//case 0xD4: printf("CNC #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xD5:{	//PUSH D to stack
			state->memBuff[state->SP-1] = state->D;
			state->memBuff[state->SP-2] = state->E;
			state->SP-=2;
			break;
		}
		//case 0xD6: printf("SUI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xD7: printf("RST 2"); break;
		//case 0xD8: printf("RC"); break;
		//case 0xD9: printf("RET"); break;
		//case 0xDA: printf("JC #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xDB: printf("IN #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xDC: printf("CC #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xDD: printf("CALL #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xDE: printf("SBI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xDF: printf("RST 3"); break;

		//case 0xE0: printf("RPO"); break;
		case 0xE1:{ //Pop H
			state->H = state->memBuff[state->SP+1];
			state->L = state->memBuff[state->SP];
			state->SP+=2;
			break;		
		}

		//case 0xE2: printf("JPO #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xE3: printf("XTHL"); break;
		//case 0xE4: printf("CPO #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xE5:{	//Push H
			state->memBuff[state->SP-1] = state->H;
			state->memBuff[state->SP-2] = state->L;
			state->SP-=2;
			break;
		}
		case 0xE6:{ //ANI (and immediate with accumulator)
			uint8_t imd = memBuff[state->SP+1];
			uint8_t res = state->A & imd;
			state->f.C = 0;
			state->f.S = res >> 7;
			state->f.Z = res == 0;
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);
			state->A = res;
			state->PC+=1;
			break;
		}
		//case 0xE7: printf("RST 4"); break;
		//case 0xE8: printf("RPE"); break;
		//case 0xE9: printf("PCHL"); break;
		//case 0xEA: printf("JPE #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xEB:{	//XCHG
			uint8_t tmpD, tmpE;
			tmpD = state->D;
			tmpE = state->E;
			state->D = state->H;
			state->E = state->L;
			state->H = tmpD;
			state->L = tmpE;
			break;
		}
		//case 0xEC: printf("CPE #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xED: printf("CALL #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xEE: printf("XRI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xEF: printf("RST 5"); break;

		//case 0xF0: printf("RP"); break;
		case 0xF1:{ // Pop PSW
			uint8_t flags = memBuff[state->SP];
			state->A = memBuff[state->SP+1];
			state->f.S = flags >> 4 & 1;
			state->f.Z = flags >> 3 & 1;
			state->f.A = flags >> 2 & 1;
			state->f.P = flags >> 1 & 1;
			state->f.C = flags & 1;

			state->SP+=2;
		}
		//case 0xF2: printf("JP #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xF3: printf("DI"); break;
		//case 0xF4: printf("CP #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xF5:{ // Push PSW
			uint8_t flags = (state->f.S << 4) |
				(state->f.Z << 3) |
				(state->f.A << 2) |
				(state->f.P << 1) |
				state->f.C;
			state->memBuff[state->SP-1] = state->A;
			state->memBuff[state->SP-2] = flags;
			state->SP-=2;
			break;
		}
		//case 0xF6: printf("ORI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xF7: printf("RST 6"); break;
		//case 0xF8: printf("RM"); break;
		//case 0xF9: printf("SPHL"); break;
		//case 0xFA: printf("JM #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xFB: printf("EI"); break;
		//case 0xFC: printf("CM #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xFD: printf("CALL #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xFE:{	//CPI d8	compare d8 with A
			//uint8_t cmp = state->memBuff[state->PC+2] << 8 |
			//		state->memBuff[state->PC+1];
			uint8_t cmp = state->memBuff[state->PC+1];
			cmp = (uint8_t) cmp * (-1);
			
			uint8_t res = state->A + cmp;
			state->f.C = res < state->A;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);	

			if(res >> 7){	//flop carry for subtraction
				state->f.C = !state->f.C;
				state->f.A = !state->f.A;
			}

			state->PC+=1;
			break;
			}
			
		//case 0xFF: printf("RST 7"); break;

		default: 
			printf("***UNKNOWN INSTRUCTION 0x%02x AT LINE 0x%04x***\n", *instr, state->PC);
			SDL_Quit();
			exit(1);
			
	}
	state->PC+=1;
	//printf("\n");
	return 1;
}	
