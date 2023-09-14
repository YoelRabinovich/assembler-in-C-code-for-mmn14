#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "assembler.h"

/*
* In the first pass over the parsed input, all label declarations are entered into the symbol table,
* and whatever parts of the instructions that don't involve label references (whether using direct or
* relative addressing) are encoded, with placeholders being created for the rest.
* The missing pieces from the previous step will be handled in a 'second pass', when the symbol
* table is complete.
*/
 void first_pass();

/*
* Now that all the symbols have been entered into the table, we can resolve all the addresses of the
 * labels that were referenced (either 'directly' or 'relatively') and fill in the information in the placeholders
 * that were entered into the code image in the first pass accordingly.
 * Also we can update some extra info in the symbol table regarding 'entry' and 'external' symbols.
*/
void second_pass();

/*
* Used in the first pass: Generates instruction words and operand words (for the lines with an 'op'
* in the source code) to go into the code image. Some info will be missing due to label references whose
* addresses have not yet been entered into the symbol table. This will be filled in in the second pass.
*/
void handle_op(ParsedLine *parsed_line);


/*
 * Used in first pass: Adds an operand word for each (non-register) arg
 * For IMMEDIATE operands, we can add the full information.
 * For DIRECT and RELATIVE operands, it will be just a placeholder,
 * and we will store some other information on the side to be used in the
 * 'second pass' to fill in the missing details:
 */
void handle_operand(char *arg, AddrMode addr_mod);

/* Detect AddrMode of input arg */
AddrMode get_addr_mode(char* str);


/*
 * For first pass - enter numerical and string data into data image and symbol table
 * and also extern symbols into the symbol table:
 */
void handle_directive(ParsedLine *parsed_line);

#endif