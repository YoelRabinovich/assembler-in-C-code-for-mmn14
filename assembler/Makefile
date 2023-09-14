assembler:	assembler.o	symbol_table.o	parser.o	machine_coder.o	string_utils.o	file_utils.o	passes.o
	gcc	-g	assembler.o	passes.o	symbol_table.o	parser.o	machine_coder.o	string_utils.o	file_utils.o	-pedantic	-Wall	-o	assembler
assembler.o:	assembler.c	assembler.h	parser.h	machine_coder.h	passes.h symbol_table.h	file_utils.h
	gcc	-c	assembler.c	-ansi	-pedantic	-Wall	-o	assembler.o
passes.o:	passes.c passes.h	assembler.h	string_utils.h	machine_coder.h	symbol_table.h	parser.h
	gcc	-c	passes.c -ansi	-pedantic	-Wall	-o	passes.o
symbol_table.o:	symbol_table.c	symbol_table.h	machine_coder.h	file_utils.h
	gcc	-c	symbol_table.c	-ansi	-pedantic	-Wall	-o	symbol_table.o
parser.o:	parser.c	parser.h	assembler.h	string_utils.h	passes.h
	gcc	-c	parser.c	-ansi	-pedantic	-Wall	-o	parser.o
machine_coder.o:	machine_coder.c	machine_coder.h	assembler.h	file_utils.h	symbol_table.h
	gcc	-c	machine_coder.c	-ansi	-pedantic	-Wall	-o	machine_coder.o
string_utils.o:	string_utils.c	string_utils.h
	gcc	-c	string_utils.c	-ansi	-pedantic	-Wall	-o	string_utils.o
file_utils.o:	file_utils.c	file_utils.h
	gcc	-c	file_utils.c	-ansi	-pedantic	-Wall	-o	file_utils.o
