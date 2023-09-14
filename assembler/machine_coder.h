#ifndef MACHINE_CODER_H
#define MACHINE_CODER_H

#include "assembler.h"

/*********************************** Function Prototypes ***********************************/

/* Initialize code image array:
 * returns 1 if success, 0 if failure */
int init_code_image(size_t n);

/* Initialize data image array:
 * returns 1 if success, 0 if failure */
int init_data_image(size_t n);

/* Initialize word_types array:
 * returns 1 if success, 0 if failure */
int init_word_types(size_t n);

/* Frees memory allocated for code image, data image and word_types array */
void free_mc_memory();

/* Symbol table needs to know the current IC when adding new symbol */
unsigned int get_IC();

/* Symbol table needs to know the current DC when adding new symbol */
unsigned int get_DC();

/* Add an instruction word to the code stack */
void add_instruction(int opcode, AddrMode addrMode_1, int reg_1, AddrMode addrMod_2, int reg_2, int funct);

/* Add an operand word to the code stack */
void add_operand(int value, LinkerInfo linker_info);

/* Edit an operand word in the code image */
void edit_operand(int ic, char*label, AddrMode mode);

/* Add a data word to the data stack */
void add_data(int data);

/* Generate the machine code */
void write_object_file(char* file_path);

/* Generate the .ext file */
void write_ext_file(char* file_path);

#endif
