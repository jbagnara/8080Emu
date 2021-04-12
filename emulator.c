#include "emulator.h"

int total_cycles = 0;

/*
	Instantiates emulator registers and memory buffers
	- rom: file pointer to 8080 rom instruction file
*/
int startEmulation(FILE* rom){
	uint8_t buff[sizeof(state8080)];
	state8080 state;

	uint8_t memBuff[0x10000];	//Allocate memory for rom instructions and RAM
	state.memBuff = memBuff;

	if(rom == NULL){
		printf("instruction file not found\n");
		return -1;
	}

	fseek(rom, 0L, SEEK_END);
	int size = ftell(rom);
	fseek(rom, 0L, SEEK_SET);

	/*if(size > MAX_ROM_SIZE){
		printf("instruction file too large; should not exceed 8192 bytes");
		return -1;
	}*/

	fread(state.memBuff, size, sizeof(uint8_t), rom);	//Read instructions to addresses 0x0 - 0x1FFF

	createWindow();
	pthread_t id;
	pthread_create(&id, NULL, drawScreen, memBuff+FRAME_BUFFER);

	while(1){
		if(pollEvent())	//Quit event reached, exit game loop
			break;
		
		int cycles = total_cycles;

		struct timespec ts;
		timespec_get(&ts, TIME_UTC);	
		long start = ts.tv_nsec;

		if(execute(&state) == -1) {
			closeWindow();
			break;
		}
		int instr_cycles = total_cycles - cycles;


		long current, timedif;
		do {
			timespec_get(&ts, TIME_UTC);
			current = ts.tv_nsec;
			timedif = current - start;
			if(timedif < 0)
				timedif += start;
		} while(timedif < CYCLE_SPD_NSEC * instr_cycles);

	}

	return 0;
}

/*
	Returns parity of a bit string
	- string: bit string to calculate parity for
	- size: number of bits to iterate across
*/
int parity(uint32_t string, int size){
	int total = 0;
	for(int x=0; x<size; x++){
		if(((string >> x) & 0x1) == 0x1)
			total+=1;
	}
	return (total+1)%2;	
}

/*
	Reads in and executes next instruction from memory buffer using op case
	- state: state of emulator
*/
int execute(state8080* state){

	if(state->PC == 0x13a){
		for(int i=0; i<50; i++){
			printf("0x%.4x\n", state->memBuff[state->SP+i]);
		}
	}

	if(state->f.I == 0x1){
		//ISR
		if(i_d == 0x1){
			state->f.I = 0x0;
			i_d = 0;
			printf("Interrupt %.2x\n", i_d);
			//printf("memBuff:\n");

			//for(int i = 0; i < 0xffff; i++)
			//	printf("%.4x: %.2x\n", i, state->memBuff[i]);

			uint16_t ret = state->PC;
			state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
			state->memBuff[state->SP-2] = ret & 0xFF;
			state->SP -= 2;
			state->PC = d_bus * 8;

			total_cycles += 11;
			return 0;
		}
	}
	i_d = 0;

	unsigned char* instr = state->memBuff+state->PC;	//current instruction
	uint8_t* memBuff = state->memBuff;
	//disassemble(state->memBuff, state->PC);
	
	int instr_cycles = 0;

	
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
		case 0x01: {	//LXI, Loads 16 bit address into register B
			state->B = state->memBuff[state->PC+2];
			state->C = state->memBuff[state->PC+1];
			state->PC+=2;
			instr_cycles = 10;
			break;
		}

		case 0x02: {	//STAX, stores contents of accumulator in B
			uint16_t BC = (state->B << 8) | state->C;
			state->memBuff[BC] = state->A;
			instr_cycles = 7;
			break;
		}

		case 0x03: {	//INX, Increments value in pair B,C by 1
				uint16_t BC = (state->B << 8) | state->C;
				uint16_t res = BC + 1;
				state->B = (res >> 8) & 0xFF;
				state->C = res & 0xFF;
				instr_cycles = 5;
				break;
		}

		case 0x04: {	//INR, Increments value in register B by 1
				uint8_t res = state->B+1;
				state->f.S = res >> 7;
				state->f.Z = (res == 0x00);
				state->f.A = ((state->B & 0x08) == 0x08) && ((res & 0x08) == 0x00);
				state->f.P = parity((uint32_t) res, 8);
				state->B = res;
				instr_cycles = 5;
				break;
		}
			
		case 0x05: {	//DCR, Decrements value in B by 1
				uint8_t res = (uint8_t)state->B-1;
				state->f.S = res >> 7;
				state->f.Z = (res == 0x00);
				state->f.A = ((state->B & 0x08) == 0x08) && ((res & 0x08) == 0x00);
				state->f.P = parity((uint32_t) res, 8);
				state->B = res;
				instr_cycles = 5;
				break;
		}

		case 0x06:	//MVI, Loads 8 bit decimal into register B
			state->B = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;

		case 0x07: {	//RLC, Rotate accumulator left
			state->f.C = (state->A >> 7) & 0x1;
			uint8_t res = (state->A << 1) | state->f.C;
			state->A = res;
			instr_cycles = 4;
			break;
		}

		case 0x08: break;

		case 0x09:{	//DAD B 
			uint16_t hl = state-> H << 8 | state->L;
			uint16_t bc = state-> B << 8 | state->C;
			uint16_t res = hl + bc;
			state->f.C = hl > res;
			state->H = (res >> 8) & 0xFF;
			state->L = res & 0xFF;
			instr_cycles = 10;
			break;
		}		

		case 0x0A: { //LDAX B
			uint16_t BC =  state->B << 8 | state->C;
			state->A = state->memBuff[BC];
			instr_cycles = 7;
			break;	
		}

		case 0x0B: { //DCX B
			uint16_t BC = state->B << 8 | state->C;
			uint16_t res = BC - 1;
			state->B = (res >> 8) & 0xFF;
			state->C = res & 0xFF;
			instr_cycles = 5;
			break;
		}

		case 0x0C: {	//INR C
			uint8_t res = (uint8_t)state->C+1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->C & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->C = res;
			instr_cycles = 5;
			break;
		}

		case 0x0D: {	//DCR C
			uint8_t res = (uint8_t)state->C-1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->C & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->C = res;
			instr_cycles = 5;
			break;
		}

		case 0x0E:	//MVI, Loads 8 bit decimal into register C
			state->C = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;

		case 0x0F:{	// RRC (Rotate accumulator right)
			state->f.C = (state->A) & 0x1;
			uint8_t res = (state->A >> 1) | (state->f.C << 7);
			state->A = res;
			instr_cycles = 4;
			break;
		}
		case 0x10: break;

		case 0x11:	//LXI, Loads 16 bit address into register pair DE
			state->D = state->memBuff[state->PC+2];
			state->E = state->memBuff[state->PC+1];
			state->PC+=2;
			instr_cycles = 10;
			break;


		case 0x12: { //STAX D
			uint16_t DE = (state->D << 8) | state->E;
			state->memBuff[DE] = state->A;
			instr_cycles = 7;
			break;
		}

		case 0x13: { //INX D increment register pair D by 1
			uint16_t res = (state->D << 8 | state->E) + 1;
			state->D = res >> 8;
			state->E = res & 0xFF;
			instr_cycles = 5;
			break;
		}
		case 0x14: { //INR D
			uint8_t res = state->D+1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->D & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->D = res;
			instr_cycles = 5;
			break;
			
		}
		case 0x15: { //DCR B
			uint8_t res = (uint8_t)state->B-1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->B & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->B = res;
			instr_cycles = 5;
			break;
			
		}
		case 0x16: { //MVI D, d8
			state->D = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;	
		}
		case 0x17: { //RAL
			uint8_t res = (state->A << 1) | state->f.C;
			state->f.C = state->A >> 7;
			state->A = res;
			instr_cycles = 4;
			break;
		}

		case 0x18: break;

		case 0x19:{ //DAD D	
			uint32_t hl = state-> H << 8 | state->L;
			uint32_t de = state-> D << 8 | state->E;
			uint32_t res = hl + de;
			state->f.C = hl > res;
			state->H = (res >> 8) & 0xFF;
			state->L = res & 0xFF;
			instr_cycles = 10;
			break;
		}

		case 0x1A:{	//LDAX D, Loads address pointed by register pair DE into accumulator A
			uint16_t DE =  state->D << 8 | state->E;
			state->A = state->memBuff[DE];
			instr_cycles = 7;
			break;
		}
			

		//case 0x1B: printf("DCX D"); break;
		case 0x1C: { //INR E
			uint8_t res = state->E+1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->B & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->E = res;
			instr_cycles = 5;
			break;
		}
		case 0x1D: { //DCR E
			uint8_t res = (uint8_t)state->E-1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->E & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->E = res;
			instr_cycles = 5;
			break;
		}

		case 0x1E: { //MVI E, d8
			state->E = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;	
		}
		case 0x1F: { //RAR!
			uint8_t res = (state->A >> 1) | state->f.C << 7;
			state->f.C = state->A & 0x1;
			state->A = res;
			instr_cycles = 4;
			break;
		}

		case 0x20: break;
		case 0x21:	//LXI, Loads 16 bit address into register pair HL
			state->H = *(state->memBuff+state->PC+2);
			state->L = *(state->memBuff+state->PC+1);
			state->PC+=2;
			instr_cycles = 10;
			break;

		case 0x22: {
			uint16_t addr = (state->memBuff[state->PC+2] << 8) |
							(state->memBuff[state->PC+1]);
			state->memBuff[addr] = state->L;
			state->memBuff[addr+1] = state->H;
			instr_cycles = 16;
			break;
		}

		case 0x23:{	//INX H Increment register pair H by 1
			uint16_t res = ((state->H << 8) | (state-> L)) + 1;
			state->H = res >> 8;
			state->L = res & 0xFF;
			instr_cycles = 5;
			break;
		}

		case 0x24: { //INR H
			uint8_t res = state->H+1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->H & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->B = res;
			instr_cycles = 5;
			break;
		}

		//case 0x25: printf("DCR H"); break;
		case 0x26:{	//MVI H, D8 
			state->H = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;
		}
		case 0x27: { //DAA
			if(state->A & 0xF > 0x9 || state->f.A == 1){
				uint8_t res1 = state->A + 0x6;
				state->f.A = (res1 & 0xF) < (state->A & 0xF);
				state->A = res1;

				if((state->A >> 8) > 0x9){
					uint8_t res2 = (((state->A >> 8) + 0x6) << 8) | state->A;
					state->f.C = res2 < state->A;
					state->A = res2;
				}
			}
			state->f.S = state->A >> 7;
			state->f.Z = state->A == 0x00;
			state->f.P = parity(state->A, 8);
			
			instr_cycles = 4;
			break;
		}
		case 0x28: break;

		case 0x29: { //DAD H
			uint16_t hl = state-> H << 8 | state->L;
		 	uint16_t res = hl << 1;
			state->f.C = hl > res;
			state->H = res >> 8;
			state->L = res & 0xFF;
			instr_cycles = 10;
			break;
		}

		case 0x2A: { //LHLD a16
			uint16_t addr = (state->memBuff[state->PC+2] << 8) |
							(state->memBuff[state->PC+1]);
			state->L = state->memBuff[addr];

			state->PC += 2;
			instr_cycles = 16;
			break;	
		}

		case 0x2B: { //DCX H
			uint16_t HL = state->H << 8 | state->L;
			uint16_t res = HL - 1;
			state->H = (res >> 8) & 0xFF;
			state->L = res & 0xFF;
			instr_cycles = 5;
			break;
		}
		//case 0x2C: printf("INR L"); break;
		//case 0x2D: printf("DCR L"); break;
		case 0x2E: { //MVI L d8
			state->L = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;
		}
		//case 0x2F: printf("CMA"); break;
		
		case 0x30: break;
		case 0x31:	//LXI,	Loads 16 bit address into SP
			state->SP = state->memBuff[state->PC+2] << 8 |
					state->memBuff[state->PC+1];
			state->PC+=2;
			instr_cycles = 10;
			break;

		case 0x32:{ //STA 16d (store accumulator in memory at address of immediate)
			uint16_t addr = memBuff[state->PC+2] << 8 |
					state->memBuff[state->PC+1];
			memBuff[addr] = state->A;
			state->PC+=2;
			instr_cycles = 5;
			break;
		}
		//case 0x33: printf("INX SP"); break;
		case 0x34: { //INR M
			uint16_t HL = state->H << 8 | state->L;			
			uint8_t M = state->memBuff[HL];
			uint8_t res = M+1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((M & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->memBuff[HL] = res;
			instr_cycles = 5;
			break;
		}

		case 0x35:{ //printf("DCR M"); break; 
			uint16_t HL = state->H << 8 | state->L;
			uint8_t res = state->memBuff[HL] - 1;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = res & 0xF > state->memBuff[HL] & 0xF;
			state->f.P = parity(res, 8);
			state->memBuff[HL] = res;
			instr_cycles = 10;
			break;
		}
		case 0x36:{	//MVI M, d8
			uint16_t HL = state->H << 8 | state->L;	
			state->memBuff[HL] = state->memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 10;
			break;
		}

		case 0x37: { //STC
			state->f.C = 1;
			instr_cycles = 4;
			break;
		}
		case 0x38: break;

		case 0x39: { //DAD SP
			uint16_t HL = state-> H << 8 | state->L;
			uint16_t res = HL + state->SP;
			state->f.C = HL > res;
			state->H = (res >> 8) & 0xFF;
			state->L = res & 0xFF;
			instr_cycles = 10;
			break;
			
		}
		case 0x3A: { //LDA (2oad value at address of 16 bit immediate into accumulator
			uint16_t addr = memBuff[state->PC+2] << 8 |
					state->memBuff[state->PC+1];
			state->A = state->memBuff[addr];
			state->PC+=2;
			instr_cycles = 13;
			break;
		}
		case 0x3B: { //DCX SP
			uint16_t res = state->SP - 1;
			state->SP = res;
			instr_cycles = 5;
			break;
		}

		case 0x3C: { //INR A
				uint8_t res = state->A+1;
				state->f.S = res >> 7;
				state->f.Z = (res == 0x00);
				state->f.A = ((state->A & 0x08) == 0x08) && ((res & 0x08) == 0x00);
				state->f.P = parity((uint32_t) res, 8);
				state->B = res;
				instr_cycles = 5;
				break;
		}
		case 0x3D: { //DCR A
			uint8_t res = (uint8_t)state->A-1;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = ((state->A & 0x08) == 0x08) && ((res & 0x08) == 0x00);
			state->f.P = parity((uint32_t) res, 8);
			state->A = res;
			instr_cycles = 5;
			break;
			
		}
		case 0x3E: { //MVI A, 8d
			state->A = memBuff[state->PC+1];
			state->PC+=1;
			instr_cycles = 7;
			break;
		}
		//case 0x3F: printf("CMC"); break;

		case 0x40: { //MOV B -> B
			state->B = state->B;
			instr_cycles = 5;
			break;
		}
		case 0x41: { //MOV C -> B
			state->B = state->C;
			instr_cycles = 5;
			break;	
		}
		case 0x42: { //MOV D -> B
			state->B = state->D;
			instr_cycles = 5;
			break;	
		}

		case 0x43: { //MOV E -> B
			state->B = state->E;
			instr_cycles = 5;
			break;	
		}

		case 0x44: { //MOV H -> B
			state->B = state->H;
			instr_cycles = 5;
			break;	
		}

		case 0x45: { //MOV L -> B
			state->B = state->L;
			instr_cycles = 5;
			break;
		}
		case 0x46: { //MOV M -> B
			uint16_t M = state->H << 8 | state->L;
			state->B = state->memBuff[M];
			instr_cycles = 5;
			break;
		}
		//case 0x47: printf("MOV B, A"); break;
		case 0x48: { //MOV B -> C
			state->C = state->B;
			instr_cycles = 5;
			break;
		}
		case 0x49: { //MOV C -> C
			state->C = state->C;
			instr_cycles = 5;
			break;
		}
		case 0x4A: { //MOV D -> C
			state->C = state->D;
			instr_cycles = 5;
			break;
		}
		//case 0x4B: printf("MOV C, E"); break;
		case 0x4C: { //MOV H -> C
			state->C = state->H;
			instr_cycles = 5;
			break;
		}
		case 0x4D: { //MOV L -> C
			state->C = state->L;
			instr_cycles = 5;
			break;
		}
		case 0x4E: { //MOV M -> C
			uint16_t M = state->H << 8 | state->L;
			state->C = state->memBuff[M];
			instr_cycles = 5;
			break;
		}
		case 0x4F: { //MOV A -> C	
			state->C = state->A;
			instr_cycles = 5;
			break;
		}

		case 0x50: { //MOV B -> D
			state->D = state->B;
			instr_cycles = 5;
			break;
		}

		case 0x51: { //MOV C -> D
			state->D = state->C;
			instr_cycles = 5;
			break;
		}

		case 0x52: { //MOV D -> D
			state->D = state->D;
			instr_cycles = 5;
			break;
		}

		case 0x53: { //MOV D -> E
			state->D = state->E;
			instr_cycles = 5;
			break;
		}

		case 0x54: { //MOV D -> H
			state->D = state->H;
			instr_cycles = 5;
			break;
		}

		case 0x55: { //MOV D -> L
			state->D = state->L;
			instr_cycles = 5;
			break;
		}

		case 0x56:{ //Mov M -> D
			uint16_t M = state->H << 8 | state->L;
			state->D = state->memBuff[M];
			instr_cycles = 7;
			break;
		}
		case 0x57: { //Mov A-> D
			state->D = state->A;
			instr_cycles = 5;
			break;
		}
		case 0x58: { //Mov B -> E
			state->E = state->B;
			instr_cycles = 5;
			break;
		}
		case 0x59: { //Mov C -> E
			state->E = state->C;
			instr_cycles = 5;
			break;
		}

		case 0x5A: { //Mov D -> E
			state->E = state->D;
			instr_cycles = 5;
			break;
		}

		case 0x5B: { //Mov E -> E
			state->E = state->E;
			instr_cycles = 5;
			break;
		}

		case 0x5C: { //Mov H -> E
			state->E = state->H;
			instr_cycles = 5;
			break;
		}

		case 0x5D: { //Mov L -> E
			state->E = state->B;
			instr_cycles = 5;
			break;
		}

		case 0x5E:{ //Mov M -> E
			uint16_t HL = state->H << 8 | state->L;
			state->E = state->memBuff[HL];
			instr_cycles = 5;
			break;
		}
		case 0x5F: { //MOV A -> E
			state->E = state->A;
			instr_cycles = 5;
			break;
		}

		case 0x60: { //MOV B -> H
			state->H = state->B;
			instr_cycles = 5;
			break;
		}
		//case 0x61: printf("MOV H, C"); break;
		//case 0x62: printf("MOV H, D"); break;
		//case 0x63: printf("MOV H, E"); break;
		//case 0x64: printf("MOV H, H"); break;
		//case 0x65: printf("MOV H, L"); break;
		case 0x66:{ // Mov M -> H 
			uint16_t M = state->H << 8 | state->L;
			state->H = state->memBuff[M];
			instr_cycles = 7;
			break;
		}
		case 0x67: { // Mov A -> H;
			state->H = state->A;
			instr_cycles = 5;
			break;
		}
		case 0x68: { //mov B -> L
			state->L = state->B;
			instr_cycles = 5;
			break;
		}
		//case 0x69: printf("MOV L, C"); break;
		//case 0x6A: printf("MOV L, D"); break;
		//case 0x6B: printf("MOV L, E"); break;
		//case 0x6C: printf("MOV L, H"); break;
		case 0x6D: { //MOV L -> L
			state->L = state->L;
			instr_cycles = 5;
			break;	
		}
		//case 0x6E: printf("MOV L, M"); break;
		case 0x6F: {	//MOV A -> L
			state->L = state->A;
			instr_cycles = 5;
			break;	
		}

		case 0x70: { //MOV B -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->B;
			instr_cycles = 7;
			break;
		}

		case 0x71: { //MOV C -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->C;
			instr_cycles = 7;
			break;
		}

		case 0x72: { //MOV D -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->D;
			instr_cycles = 7;
			break;
		}

		case 0x73: { //MOV E -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->E;
			instr_cycles = 7;
			break;
		}

		case 0x74: { //MOV H -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->H;
			instr_cycles = 7;
			break;
		}

		case 0x75: { //MOV L -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->L;
			instr_cycles = 7;
			break;
		}

		//case 0x76: //HLT
		case 0x77: { //Move A -> M
			uint16_t M = state->H << 8 | state->L;
			state->memBuff[M] = state->A;
			instr_cycles = 7;
			break;
		}
			
		case 0x78: { //Mov B -> A
			state->A = state->B;
			instr_cycles = 5;
			break;
		}
		case 0x79: { //Mov C -> A
			state->A = state->C;
			instr_cycles = 5;
			break;
		}
		case 0x7A:{ // Mov D -> A
			state->A = state->D;
			instr_cycles = 5;
			break;
		}
		case 0x7B: //Mov E -> A
			state->A = state->E;
			instr_cycles = 5;
			break;
		case 0x7C: //Move H -> A
			state->A = state->H;
			instr_cycles = 5;
			break;
		case 0x7D: { //MOV L -> A
			state->A = state-> L;
			instr_cycles = 5;
			break;
		}
		case 0x7E:{ // Mov M -> A
			uint16_t M = state->H << 8 | state->L;
			state->A = state->memBuff[M];
			instr_cycles = 5;
			break;
		}
		case 0x7F: { //Mov A -> A
			state->A = state->A;
			instr_cycles = 5;
			break;
		}

		case 0x80: { //ADD B
			uint8_t res = state->A + state->B;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 4;
			break;
		}
		//case 0x81: printf("ADD C"); break;
		case 0x82: { //ADD D
			uint8_t res = state->A + state->D;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 4;
			break;
		}
		case 0x83: { //ADD E
			uint8_t res = state->A + state->E;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 4;
			break;
		}
		//case 0x84: printf("ADD H"); break;
		//case 0x85: printf("ADD L"); break;
		case 0x86: { //ADD M
			uint16_t HL = (state->H << 8) | state->L;
			uint8_t res = state->A + memBuff[HL];
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}

		//case 0x87: printf("ADD A"); break;
		case 0x88: { //ADC B
			uint8_t res = state->A + state->B + state->f.C;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}
		case 0x89: { //ADC C
			uint8_t res = state->A + state->C + state->f.C;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}
		case 0x8A: { //ADC D
			uint8_t res = state->A + state->D + state->f.C;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}
		case 0x8B: { //ADC E
			uint8_t res = state->A + state->E + state->f.C;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}

		case 0x8C: { //ADC H
			uint8_t res = state->A + state->H + state->f.C;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}

		case 0x8D: { //ADC L
			uint8_t res = state->A + state->L + state->f.C;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}

		case 0x8E: { //ADC M
			uint16_t HL = state->H << 8 | state->L;
			uint8_t M = state->memBuff[HL];	
			uint8_t res = state->A + state->f.C + M;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;
			
			instr_cycles = 7;
			break;
		}
		case 0x8F: { //ADC A
			uint8_t res = state->f.C + (state->A << 1);
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res < state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}

		case 0x90: { //SUB B
			uint8_t res = state->A - state->B;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) > (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res > state->A;
			state->A = res;

			instr_cycles = 4;
			break;
		}
		//case 0x91: printf("SUB C"); break;
		//case 0x92: printf("SUB D"); break;
		//case 0x93: printf("SUB E"); break;
		//case 0x94: printf("SUB H"); break;
		//case 0x95: printf("SUB L"); break;
		case 0x96: { //SUB M
			uint16_t HL = (state->H << 8) | state->L;
			uint8_t res = state->A - memBuff[HL];
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) > (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res > state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}
		//case 0x97: printf("SUB A"); break;
		//case 0x98: printf("SBB B"); break;	//subtract with borrow
		//case 0x99: printf("SBB C"); break;
		//case 0x9A: printf("SBB D"); break;
		//case 0x9B: printf("SBB E"); break;
		//case 0x9C: printf("SBB H"); break;
		//case 0x9D: printf("SBB L"); break;
		case 0x9E: { //SBB M
			uint16_t HL = (state->H << 8) | state->L;
			uint8_t res = state->A - (memBuff[HL] + state->f.C);
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) > (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->f.C = res > state->A;
			state->A = res;

			instr_cycles = 7;
			break;
		}
		//case 0x9F: printf("SBB A"); break;

		case 0xA0: { //ANA B
			uint8_t res = state->A & state->B;
			state->f.C = 0x0;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = 0x0;
			state->f.P = parity(res, 8);
			state->A = res;

			instr_cycles = 4;
			break;
		}
		//case 0xA1: printf("ANA C"); break;
		//case 0xA2: printf("ANA D"); break;
		//case 0xA3: printf("ANA E"); break;
		case 0xA4: { //ANA H
			uint8_t res = state->A & state->H;
			state->f.C = 0x0;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = 0x0;
			state->f.P = parity(res, 8);
			state->A = res;

			instr_cycles = 4;
			break;
		}
		case 0xA5: { //ANA L
			uint8_t res = state->A & state->L;
			state->f.C = 0x0;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = 0x0;
			state->f.P = parity(res, 8);
			state->A = res;

			instr_cycles = 4;
			break;
		}
		//case 0xA6: printf("ANA M"); break;
		case 0xA7: { //ANA A
			state->f.C = 0x0;
			state->f.S = state->A >> 7;
			state->f.Z = state->A == 0x0;
			state->f.A = 0x0;
			state->f.P = parity(state->A, 8);
			instr_cycles = 4;
			break;
		}
		case 0xA8: { //XRA B
			uint8_t res = state->A ^ state->B;
			state->f.C = res < state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			instr_cycles = 4;
			break;
		}
		//case 0xA9: printf("XRA C"); break;
		//case 0xAA: printf("XRA D"); break;
		case 0xAB: { //XRA E
			uint8_t res = state->A ^ state->E;
			state->f.C = res < state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			instr_cycles = 4;
			break;
		}
		//case 0xAC: printf("XRA H"); break;
		//case 0xAD: printf("XRA L"); break;
		//case 0xAE: printf("XRA M"); break;
		case 0xAF: { //XRA A, exclusive or A with A (zero out)
			state->A = 0x00;
			state->f.C = 0x0;
			state->f.S = 0x0;
			state->f.Z = 0x1;
			state->f.A = 0x0;
			state->f.P = 0x1;
			instr_cycles = 4;
			break;
		}

		case 0xB0: { //ORA B
			uint8_t res = state->A | state->B;
			state->f.C = res < state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->A = res;
			instr_cycles = 4;	
			break;
		}
		//case 0xB1: printf("ORA C"); break;
		//case 0xB2: printf("ORA D"); break;
		//case 0xB3: printf("ORA E"); break;
		case 0xB4: { //ORA H
			uint8_t res = state->A | state->H;
			state->f.C = res < state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			state->A = res;
			instr_cycles = 4;	
			break;
		}
		//case 0xB5: printf("ORA L"); break;
		case 0xB6: { //ORA M
			uint16_t HL = state->H << 8 | state->L;
			uint8_t M = state->memBuff[HL];
			uint8_t res = state->A | M;
			state->f.C = res < state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x0;
			state->f.A = (res & 0xF) < (state->A & 0xF);
			state->f.P = parity(res, 8);
			instr_cycles = 4;	
			break;
		}
		//case 0xB7: printf("ORA A"); break;
		//case 0xB8: printf("CMP B"); break;
		//case 0xB9: printf("CMP C"); break;
		case 0xBA: { //CMP D
			uint8_t res = state->A - state->D;
			state->f.C = res > state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x00;
			state->f.A = (res & 0xF) > (state->A & 0xF);
			state->f.P = parity(res, 8);
			instr_cycles = 4;
			break;
		}

		case 0xBB: { //CMP E
			uint8_t res = state->A - state->E;
			state->f.C = res > state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x00;
			state->f.A = (res & 0xF) > (state->A & 0xF);
			state->f.P = parity(res, 8);
			instr_cycles = 4;
			break;
		}
		case 0xBC: { //CMP H
			uint8_t res = state->A - state->H;
			state->f.C = res > state->A;
			state->f.S = res >> 7;
			state->f.Z = res == 0x00;
			state->f.A = (res & 0xF) > (state->A & 0xF);
			state->f.P = parity(res, 8);
			instr_cycles = 4;
			break;
		}
		//case 0xBD: printf("CMP L"); break;
		//case 0xBE: printf("CMP M"); break;
		//case 0xBF: printf("CMP A"); break;

		case 0xC0: { //RNZ
			if(!state->f.Z){
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;
			} else {
				instr_cycles = 5;
			}
			break;
		}
		case 0xC1:{ //POP B
			state->B = state->memBuff[state->SP+1];
			state->C = state->memBuff[state->SP];
			state->SP+=2;
			instr_cycles = 10;
			break;	
		}

		case 0xC2: //JNZ a16, If not Zero flag, jump to 16 bit address
			if(state->f.Z == 0){
				state->PC = state->memBuff[state->PC+2] << 8 |
						state->memBuff[state->PC+1];
				state->PC-=1;
			} else{
				state->PC+=2;
			} 
			instr_cycles = 10;
			break;			

		case 0xC3:	//JMP ADDR, jump to 16 bit address
			state->PC = state->memBuff[state->PC+2] << 8 |
					state->memBuff[state->PC+1];
			state->PC-=1;
			instr_cycles = 10;
			break;
		case 0xC4: { //CNZ a16
			if(!state->f.Z){
				uint16_t ret = state->PC+3;
				state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
				state->memBuff[state->SP-2] = ret & 0xFF;
				state->SP -= 2;
				state->PC = state->memBuff[state->PC+2] << 8 | 
						state->memBuff[state->PC+1];

				state->PC -= 1;
				instr_cycles = 17;
			} else {
				instr_cycles = 11;
				state->PC += 2;
			}
			break;
		}

		case 0xC5:{	//PUSH B
			state->memBuff[state->SP-1] = state->B;
			state->memBuff[state->SP-2] = state->C;
			state->SP-=2;
			instr_cycles = 11;
			break;	
		}

		case 0xC6:{ //ADI (add immediate to accumulator)
			uint8_t imd = state->memBuff[state->PC+1];
			uint8_t res = state->A + imd;
			state->A = res;
			state->f.C = imd > res;
			state->f.S = res >> 7;
			state->f.Z = res == 0;
			state->f.A = (imd & 0xF) > (res & 0xF);
			state->f.P = parity((uint32_t) res, 8);	
			state->PC+=1;
			instr_cycles = 7;
			break;
		}

		//case 0xC7: printf("RST 0"); break;
		case 0xC8: { //RZ
			if(state->f.Z){
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->SP+=2;
				state->PC -= 1;
				instr_cycles = 11;
			} else {
				instr_cycles = 5;
			}
			break;
		}
		case 0xC9: //RET return from subroutine
			state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
			state->PC -= 1;
			state->SP+=2;
			instr_cycles = 10;
			break;

		case 0xCA: { //JZ a16
			if(state->f.Z == 1) {
				uint16_t addr = (state->memBuff[state->PC+2] << 8) | 
								(state->memBuff[state->PC+1]);
				
				state->PC = addr;
				state->PC -= 1;
			} else {
				state->PC += 2;
			}
			instr_cycles = 10;	
			break;
		} 
		//case 0xCB: printf("JMP #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xCC: { //CZ a16
			if(state->f.Z == 0) {
				uint16_t ret = state->PC+3;
				state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
				state->memBuff[state->SP-2] = ret & 0xFF;
				state->SP -= 2;
				state->PC = state->memBuff[state->PC+2] << 8 | 
						state->memBuff[state->PC+1];

				state->PC -= 1;
				instr_cycles = 17;
			} else {
				instr_cycles = 11;
			}
		}

			break;	
		case 0xCD: {	//CALL ADDR, jump to 16 bit address and push address to stack
			uint16_t ret = state->PC+3;
			state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
			state->memBuff[state->SP-2] = ret & 0xFF;
			state->SP -= 2;
			state->PC = state->memBuff[state->PC+2] << 8 | 
					state->memBuff[state->PC+1];

			if(state->PC == 0x08ff)
				printf("woah\n");

			state->PC -= 1;
			instr_cycles = 17;
			break;
		}


		//case 0xCE: printf("ACI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xCF: printf("RST 1"); break;

		case 0xD0: { //RNC
			if(!state->f.C){
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;
			} else {
				instr_cycles = 5;
			}
			break;	
		}

		case 0xD1: { //Pop D
			state->D = state->memBuff[state->SP+1];
			state->E = state->memBuff[state->SP];
			state->SP+=2;
			instr_cycles = 10;
			break;
		}
		case 0xD2: { // JNC a16
			uint16_t addr = (state->memBuff[state->PC + 2] << 8) |
							(state->memBuff[state->PC + 1]);
			if(state->f.C == 0){
				state->PC = addr;
				state->PC -= 1;
			}
			instr_cycles = 10;
			break;
		}
		case 0xD3:{	//OUT D8
			uint8_t device = state->memBuff[state->PC+1];
			state->PC+=1;

			//TODO SOMETHING
			instr_cycles = 10;
			break;
		}
		//case 0xD4: printf("CNC #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xD5:{	//PUSH D to stack
			state->memBuff[state->SP-1] = state->D;
			state->memBuff[state->SP-2] = state->E;
			state->SP-=2;
			instr_cycles = 11;
			break;
		}
		case 0xD6: { //SUI
			uint8_t imd = state->memBuff[state->PC+1];
			uint8_t res = state->A - imd;
			state->A = res;
			state->f.C = imd < res;
			state->f.S = res >> 7;
			state->f.Z = res == 0;
			state->f.A = (imd & 0xF) < (res & 0xF);
			state->f.P = parity((uint32_t) res, 8);	
			state->PC+=1;
			instr_cycles = 7;
			break;
			
		}
		//case 0xD7: printf("RST 2"); break;
		case 0xD8: { //RC
			if(state->f.C){
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;
			} else {
				instr_cycles = 5;
			}
			break;	
		}
		case 0xD9: { //RET
			state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
			state->PC -= 1;
			state->SP+=2;
			instr_cycles = 10;
			break;	
		}
		case 0xDA: { //JC a16	
			if(state->f.C == 1) {
				uint16_t addr = (state->memBuff[state->PC+2] << 8) | 
								(state->memBuff[state->PC+1]);
				
				state->PC = addr;
				state->PC -= 1;
			} else {
				state->PC += 2;
			}
			instr_cycles = 10;	
			break;
		}
		case 0xDB: { //IN d8
			uint8_t device = state->memBuff[state->PC+1];
			state->PC+=1;

			//TODO
			break;
		}
		//case 0xDC: printf("CC #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xDD: printf("CALL #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		//case 0xDE: printf("SBI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xDF: printf("RST 3"); break;

		case 0xE0: { //RPO
			if(state->f.S == 1) {
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;	
			} else {
				instr_cycles = 5;	
			}
			break;	
		}
		case 0xE1:{ //Pop H
			state->H = state->memBuff[state->SP+1];
			state->L = state->memBuff[state->SP];
			state->SP+=2;
			instr_cycles = 10;
			break;		
		}

		case 0xE2: { //JPO
			uint16_t addr = (state->memBuff[state->PC + 2] << 8) |
							(state->memBuff[state->PC + 1]);
			if(state->f.P == 0){
				state->PC = addr;
				state->PC -= 1;
			}
			instr_cycles = 10;
			break;
		}

		case 0xE3: { //XTHL
			uint8_t t_H, t_L;
			t_H = state->H;
			t_L = state->L;
			state->L = state->memBuff[state->SP];
			state->H = state->memBuff[state->SP+1];
			state->memBuff[state->SP] = t_L;
			state->memBuff[state->SP+1] = t_H;
			instr_cycles = 18;
			break;
		}
		//case 0xE4: printf("CPO #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xE5:{	//Push H
			state->memBuff[state->SP-1] = state->H;
			state->memBuff[state->SP-2] = state->L;
			state->SP-=2;
			instr_cycles = 11;
			break;
		}
		case 0xE6:{ //ANI (and immediate with accumulator)
			uint8_t imd = memBuff[state->PC+1];
			uint8_t res = state->A & imd;
			state->f.C = 0;
			state->f.S = res >> 7;
			state->f.Z = res == 0;
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);
			state->A = res;
			state->PC+=1;
			instr_cycles = 7;
			break;
		}
		//case 0xE7: printf("RST 4"); break;
		case 0xE8: { //RPE
			if(state->f.P){
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;
			} else {
				instr_cycles = 5;
			}
			break;	
		}

		case 0xE9: { //PCHL
			state->PC = state->H << 8 | state->L;
			state->PC -= 1;
			instr_cycles = 5;
			break;
		}
		//case 0xEA: printf("JPE #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xEB:{	//XCHG
			uint8_t tmpD, tmpE;
			tmpD = state->D;
			tmpE = state->E;
			state->D = state->H;
			state->E = state->L;
			state->H = tmpD;
			state->L = tmpE;
			instr_cycles = 5;
			break;
		}
		//case 0xEC: printf("CPE #$%02x%02x", *(memBuff+pc+2), *(memBuff+pc+1)); opbytes = 3; break;
		case 0xED: { //CALL a16
			uint16_t ret = state->PC+3;
			state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
			state->memBuff[state->SP-2] = ret & 0xFF;
			state->SP -= 2;
			state->PC = state->memBuff[state->PC+2] << 8 | 
					state->memBuff[state->PC+1];

			if(state->PC == 0x08ff)
				printf("woah\n");

			state->PC -= 1;
			instr_cycles = 17;
			break;
		}
		//case 0xEE: printf("XRI #$%02x", *(memBuff+pc+1)); opbytes = 2; break;
		//case 0xEF: //RST 5

		case 0xF0: { //RP
			if(state->f.S == 0) {
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;	
			} else {
				instr_cycles = 5;	
			}
			break;
		}

		case 0xF1: { // Pop PSW
			uint8_t flags = memBuff[state->SP];
			state->A = memBuff[state->SP+1];
			state->f.S = flags >> 4 & 1;
			state->f.Z = flags >> 3 & 1;
			state->f.A = flags >> 2 & 1;
			state->f.P = flags >> 1 & 1;
			state->f.C = flags & 1;
			state->SP+=2;
			instr_cycles = 10;
			break;
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
			instr_cycles = 11;
			break;
		}
		case 0xF6: { //ORI d8
			uint8_t imd = memBuff[state->PC+1];
			uint8_t res = state->A | imd;
			state->f.C = 0;
			state->f.S = res >> 7;
			state->f.Z = res == 0;
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);
			state->A = res;
			state->PC+=1;
			instr_cycles = 7;
			break;	
		}
		//case 0xF7: printf("RST 6"); break;
		case 0xF8: { //RM
			if(state->f.S){
				state->PC = state->memBuff[state->SP] | state->memBuff[state->SP+1] << 8;
				state->PC -= 1;
				state->SP+=2;
				instr_cycles = 11;
			} else {
				instr_cycles = 5;
			}
			break;	
		}
		//case 0xF9: printf("SPHL"); break;
		case 0xFA: { //JM a16
			if(state->f.S){
				state->PC = state->memBuff[state->PC+2] << 8 |
						state->memBuff[state->PC+1];
				state->PC-=1;
			} else{
				state->PC+=2;
			} 
			instr_cycles = 10;
			break;			
		}
		case 0xFB: { //EI, enable interrupts in INTE
			state->f.I = 0x1;
			instr_cycles = 4;
			break;
		}

		case 0xFC: { //CM
			if(state->f.S == 1) {
				uint16_t ret = state->PC+3;
				state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
				state->memBuff[state->SP-2] = ret & 0xFF;
				state->SP -= 2;
				state->PC = state->memBuff[state->PC+2] << 8 | 
						state->memBuff[state->PC+1];

				state->PC -= 1;
				instr_cycles = 17;
			} else {
				state->PC += 2;
				instr_cycles = 11;
			}

			break;	
		}
		case 0xFD: { //CALL a16
			uint16_t ret = state->PC+3;
			state->memBuff[state->SP-1] = ret >> 8 & 0xFF;
			state->memBuff[state->SP-2] = ret & 0xFF;
			state->SP -= 2;
			state->PC = state->memBuff[state->PC+2] << 8 | 
					state->memBuff[state->PC+1];

			if(state->PC == 0x08ff)
				printf("woah\n");

			state->PC -= 1;
			instr_cycles = 17;
			break;
		}
		case 0xFE:{	//CPI d8	compare d8 with A
			uint8_t cmp = state->memBuff[state->PC+1];
			
			uint8_t res = state->A - cmp;
			state->f.C = res > state->A;
			state->f.S = res >> 7;
			state->f.Z = (res == 0x00);
			state->f.A = (((res << 4) >> 4) + 1) > 0x0F;
			state->f.P = parity((uint32_t) res, 8);	

			state->PC+=1;
			instr_cycles = 7;
			break;
		}
			
		//case 0xFF: printf("RST 7"); break;

		default: 
			printf("***UNKNOWN INSTRUCTION 0x%02x AT LINE 0x%04x***\n", *instr, state->PC);
			return -1;
	}
	state->PC+=1;
	total_cycles += instr_cycles;
	return 0;
}	
