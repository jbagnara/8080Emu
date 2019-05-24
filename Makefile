makefile: emulator disassembler

emulator: emulator.o
	cc -o emulator emulator.o
	rm emulator.o

emulator.o:
	cc -c -g emulator.c

disassembler:	disassembler.o
	cc -o disassembler disassembler.o
	rm disassembler.o

disassembler.o:
	cc -c -g disassembler.c

clean:
	@rm -f disassembler
	@rm -f emulator
	@rm -f out.txt
	@rm -f *.o
