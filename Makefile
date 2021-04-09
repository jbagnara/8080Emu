makefile: emulator

all: emulator disassembler

emulator: emulator.o disassemble.o display.o
	cc -o emulator main.c emulator.o disassemble.o display.o  -lSDL2 -pthread
	@rm *.o
	@rm -f *.h.gch

disassembler:	disassembler.o
	cc -o disassembler disassembler.o disassemble.h disassemble.c
	@rm disassembler.o

test: emulator.o disassemble.o display.o
	cc -c -g -I"." test/testop.c
	cc -o test/test testop.o emulator.o disassemble.o display.o -lSDL2 -pthread
	@rm *.o
	@rm -f *.h.gch
	./test/test
	@rm ./test/test

emulator.o:
	cc -c -g emulator.c

disassembler.o:
	cc -c -g disassembler.c

disassemble.o:
	cc -c -g disassemble.h disassemble.c

display.o:
	cc -c -g display.c display.h -lSDL2

clean:
	@rm -f disassembler
	@rm -f emulator
	@rm -f out.txt
	@rm -f *.o
	@rm -f *.h.gch
