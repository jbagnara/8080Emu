disassembler:	disassembler.o
	cc -o disassembler disassembler.o

disassembler.o:
	cc -c disassembler.c

clean:
	rm disassembler.o
