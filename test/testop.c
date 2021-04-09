#include "emulator.h"

void reset();
void check();
void checkMem(uint16_t, uint8_t);

uint8_t e_sign, e_zero, e_aux, e_parity, e_carry;
uint8_t e_A, e_B, e_D, e_H, e_F, e_C, e_E, e_L;
uint16_t e_PC, e_SP;
uint8_t e_INTE;

state8080 state;
int failed;

uint16_t test_case = 0x00;

int main(){

	state.memBuff = malloc(0x10000);

	//0x01
	test_case = 0x01;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x11;
	state.memBuff[2] = 0xcc;
	e_C = 0x11;
	e_B = 0xcc;
	e_PC = 3;

	execute(&state);
	check();


	//0x02
	test_case = 0x02;
	reset();
	state.memBuff[0] = test_case;
	state.A = 0xAC;
	e_A = state.A;
	e_B = state.A;
	e_PC = 1;	

	execute(&state);
	check();

	
	//0x03
	test_case = 0x03;
	reset();
	state.memBuff[0] = test_case;
	state.B = 0x11;
	state.C = 0xcc;
	e_B = 0x11;
	e_C = 0xcd;
	e_PC = 1;
	
	execute(&state);
	check();

	//0x04
	test_case = 0x04;
	reset();
	state.memBuff[0] = test_case;
	state.B = 0xcc;
	e_B = 0xcd;
	e_sign = 0x1;
	e_parity = 0;
	e_aux = 0;
	e_PC = 1;

	execute(&state);
	check();


	//0x05
	test_case = 0x05;
	reset();
	state.memBuff[0] = test_case;
	state.B = 0xcc;
	e_B = 0xcb;
	e_sign = 0x1;
	e_parity = 0;
	e_aux = 0;
	e_PC = 1;
	
	execute(&state);
	check();


	//0x06
	test_case = 0x06;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x44;
	e_B = 0x44;
	e_PC = 2;
	
	execute(&state);
	check();


	//0x07
	test_case = 0x07;
	reset();
	state.memBuff[0] = test_case;
	state.A = 0xcc;
	e_A = 0x99;
	e_carry = 1;
	e_PC = 1;

	execute(&state);
	check();
	

	//0x09
	test_case = 0x09;
	reset();
	state.memBuff[0] = test_case;
	state.B = 0x11;
	state.C = 0x22;
	state.H = 0x33;
	state.L = 0x44;
	e_B = state.B;
	e_C = state.C;
	e_H = 0x44;
	e_L = 0x66;
	e_carry = 0;
	e_PC = 1;

	execute(&state);
	check();
	

	//0x0D
	test_case = 0x0D;
	reset();
	state.memBuff[0] = test_case;
	state.C = 0xcc;
	e_C = 0xcb;
	e_sign = 0x1;
	e_parity = 0;
	e_aux = 0;
	e_PC = 1;

	execute(&state);
	check();


	//0x0E
	test_case = 0x0E;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0xf4;
	e_C = 0xf4;
	e_PC = 2;

	execute(&state);
	check();

	
	//0x0F
	test_case = 0x0F;
	reset();
	state.memBuff[0] = test_case;
	state.A = 0xcc;
	e_A = 0x66;
	e_carry = 0;
	e_PC = 1;
	
	execute(&state);
	check();


	//0x11
	test_case = 0x11;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x21;
	state.memBuff[2] = 0x43;
	e_D = 0x43;
	e_E = 0x21;
	e_PC = 3;

	execute(&state);
	check();


	//0x13
	test_case = 0x13;
	reset();
	state.memBuff[0] = test_case;
	state.D = 0xff;
	state.E = 0xff;
	e_D = 0x00;
	e_E = 0x00;
	e_PC = 1;

	execute(&state);
	check();


	//0x19
	test_case = 0x19;
	reset();
	state.memBuff[0] = test_case;
	state.D = 0x00;
	state.E = 0xf0;
	state.H = 0x00;
	state.L = 0xf1;
	e_D = state.D;
	e_E = state.E;
	e_H = 0x01;
	e_L = 0x00;
	e_PC = 1;

	execute(&state);
	check();	


	//0x1A
	test_case = 0x1A;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[0x1234] = 0x55;
	state.D = 0x12;
	state.E = 0x34;
	e_A = 0x55;
	e_D = state.D;
	e_E = state.E;
	e_PC = 1;

	execute(&state);
	check();


	//0x21
	test_case = 0x21;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x55;
	state.memBuff[2] = 0x44;
	e_H = 0x44;
	e_L = 0x55;
	e_PC = 3;

	execute(&state);
	check();


	//0x23
	test_case = 0x23;
	reset();
	state.memBuff[0] = test_case;
	state.H = 0x00;
	state.L = 0xff;
	e_H = 0x01;
	e_L = 0x00;
	e_PC = 1;

	execute(&state);
	check();


	//0x26
	test_case = 0x26;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0xf9;
	e_H = 0xf9;
	e_PC = 2;

	execute(&state);
	check();


	//0x29
	test_case = 0x29;
	reset();
	state.memBuff[0] = test_case;
	state.H = 0xaa;
	state.L = 0xaa;
	e_H = 0x55;
	e_L = 0x55;
	e_carry = 1;
	e_PC = 1;

	execute(&state);
	check();
	

	//0x31
	test_case = 0x31;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x10;
	state.memBuff[2] = 0xf0;
	e_SP = 0xf010;
	e_PC = 3;

	execute(&state);
	check();


	//0x32
	test_case = 0x32;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x22;
	state.memBuff[2] = 0x56;
	state.A = 0xc3;
	e_A = state.A;
	e_PC = 3;

	execute(&state);
	checkMem(0x5622, 0xc3);
	check();


	//0x36
	test_case = 0x36;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0xcf;
	state.H = 0x10;
	state.L = 0xff;
	e_H = state.H;
	e_L = state.L;
	e_PC = 2;

	execute(&state);
	checkMem(0x10ff, 0xcf);
	check();


	//0x3A
	test_case = 0x3A;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x50;
	state.memBuff[2] = 0x04;
	state.memBuff[0x0450] = 0x90;
	e_A = 0x90;
	e_PC = 3;

	execute(&state);
	check();


	//0x3E
	test_case = 0x3E;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[1] = 0x40;
	e_A = 0x40;
	e_PC = 2;

	execute(&state);
	check();


	//0x56
	test_case = 0x56;
	reset();
	state.memBuff[0] = test_case;
	state.memBuff[0x5678] = 0x06;
	state.H = 0x56;
	state.L = 0x78;
	e_D = 0x06;
	e_H = state.H;
	e_L = state.L;
	e_PC = 1;

	execute(&state);
	check();


	//0x5E
	

}

void reset(){
	failed = 0;

	state.f.S = 0x0;
	state.f.Z = 0x0;
	state.f.A = 0x0;
	state.f.P = 0x0;
	state.f.C = 0x0;

	state.A = 0x0;
	state.B = 0x0;
	state.D = 0x0;
	state.H = 0x0;
	state.F = 0x0;
	state.C = 0x0;
	state.E = 0x0;
	state.L = 0x0;
	
	for(int i = 0; i < 0x10000; i++)
		state.memBuff[i] = 0x0;

	state.SP = 0x0;
	state.PC = 0x0;
	state.INTE = 0;

	e_A = 0x0;
	e_B = 0x0;
	e_D = 0x0;
	e_H = 0x0;
	e_F = 0x0;
	e_C = 0x0;
	e_E = 0x0;
	e_L = 0x0;
	e_SP = 0x00;
	e_PC = 0x00;
	e_INTE = 0x0;
	e_sign = 0x0;
	e_zero = 0x0;
	e_aux = 0x0;
	e_parity = 0x0;
	e_carry = 0x0;
}

void check(){


	printf("\033[0;31m");

	if(e_sign != state.f.S){
		printf("sign flag is 0x%.2x, expected 0x%.2x in case 0x%.2x\n", state.f.S, e_sign, test_case);
		failed = 1;
	}

	if(e_zero != state.f.Z){
		printf("zero flag is 0x%.2x, expected 0x%.2x in case 0x%.2x\n", state.f.Z, e_zero, test_case);
		failed = 1;
	}

	if(e_aux != state.f.A){
		printf("incorrect aux flag in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_parity != state.f.P){
		printf("incorrect parity flag in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_carry != state.f.C){
		printf("incorrect carry flag in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_A != state.A){
		printf("A reg is 0x%.2x, expected 0x%.2x in case 0x%.2x\n", state.A, e_A, test_case);
		failed = 1;
	}

	if(e_B != state.B){
		printf("incorrect B reg in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_D != state.D){
		printf("incorrect D reg in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_H != state.H){
		printf("incorrect H reg in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_F != state.F){
		printf("incorrect F reg in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_SP != state.SP){
		printf("incorrect SP in case 0x%.4x\n", test_case);
		failed = 1;
	}

	if(e_PC != state.PC){
		printf("A is 0x%.2x, expected 0x%.2x in case 0x%.2x\n", state.PC, e_PC, test_case);
		failed = 1;
	}

	if(e_INTE != state.INTE){
		printf("incorrect INTE ff in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(failed == 0){
		printf("\033[0;32m");
		printf("0x%.2x case passed\n", test_case);
	}

	printf("\033[0m");

}

void checkMem(uint16_t addr, uint8_t e){

	printf("\033[0;31m");

	uint8_t val = state.memBuff[addr];
	if(val != e){
		printf("incorrect value 0x%.2x at addr 0x%.4x, \
				expected 0x%.2x in case 0x%.2x\n", val, addr, e, test_case);
		failed = 1;
	}
	printf("\033[0m");
}
