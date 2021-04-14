#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include "emulator.h"
#include "disassemble.h"
#include "display.h"

static const uint16_t FRAME_BUFFER 		= 0x2400;
static const uint16_t MAX_ROM_SIZE 		= 0x2000;
static const long CPU_CLOCK_SPEED		= 2000000;	//2MHz
static const long CYCLE_SPD_NSEC		= 1000000000 / CPU_CLOCK_SPEED;
static const int REFRESH_RATE			= 600;		//60fps
static const long DISPLAY_SPD_NSEC		= 1000000000 / REFRESH_RATE;

typedef struct flags{
	uint8_t S:1;	//sign flag
	uint8_t Z:1;	//zero flag
	uint8_t A:1;	//auxiliary carry flag
	uint8_t P:1;	//parity flag
	uint8_t C:1;	//carry flag
	uint8_t I:1;	//interrupt flag
	uint8_t x:2;	//extra bits
} flags;

typedef struct taito{
	uint8_t offset;
	uint16_t shift_reg;
	uint8_t watchdog;
} taito;

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
	uint8_t i_bus;
	uint8_t i_ready;
	struct taito peripherals;
} state8080;

int startEmulation(FILE* rom);
int parity(uint32_t string, int size);
int execute(state8080* state);

#endif
