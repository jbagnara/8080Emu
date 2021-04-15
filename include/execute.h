#ifndef _EXECUTE_H
#define _EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "emulator.h"
#include "disassemble.h"

int parity(uint32_t string, int size);
int execute(state8080* state);

#endif
