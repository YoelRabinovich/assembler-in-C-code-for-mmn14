#ifndef PARSER_H
#define PARSER_H

#include "assembler.h"

#define MAX_LABEL_LEN 31

/* ParsedLine 'constructor' */
ParsedLine* construct_parsed_line(char* label, char* op, char* directive, int n_args, char** args);

/* Free memory allocated to a ParsedLine */
 void free_parsed_line(ParsedLine* parsed_line);

/*
* Calculates the number of instruction/operand words that will be required to encode this line
 * (for the purpose of efficiently allocating the correct size array for the
 * code image without assuming anything about the size of the input)
*/
 int get_num_code_words(ParsedLine* line);

/*
* Calculates the number of data words that will be required to encode this line
 * (for the purpose of efficiently allocating the correct size array for the
 * data image without assuming anything about the size of the input)
*/
int get_num_data_words(ParsedLine* line);

/*
* Calculates the number of symbol declarations in the current line (to help when
 * initializing the symbol table)
*/
int get_num_symbols(ParsedLine* line);

/*
* Calculates the number of symbol references in the current line (to help when
 * initializing the symbol_references array)
*/
int get_num_symbol_refs(ParsedLine* line);

/*
 * Parses, checks syntax is according to specification, and restructures each input line
 */
ParsedLine* parse_line(int line_num, char* line);

#endif
