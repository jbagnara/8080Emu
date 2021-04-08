#include "emulator.h"

void reset();
void check();

uint8_t e_sign, e_zero, e_aux, e_parity, e_carry;
uint8_t e_A, e_B, e_D, e_H, e_F, e_C, e_E, e_L;
uint16_t e_PC, e_SP;
uint8_t e_INTE;

state8080 state;

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
	
}

void reset(){
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

	int failed = 0;

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
		printf("incorrect A reg in case 0x%.2x\n", test_case);
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
		printf("incorrect SP in case 0x%.2x\n", test_case);
		failed = 1;
	}

	if(e_PC != state.PC){
		printf("incorrect PC in case 0x%.2x\n", test_case);
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
