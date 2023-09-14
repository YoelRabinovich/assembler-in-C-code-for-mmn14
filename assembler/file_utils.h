#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>

/* return filename */
char* create_file_name(char* base, char* extension);

/* writes a number to file (in padded hex format) */
void write_val(FILE *fp, int val);

/* writes an address to file (in padded hex format) */
void write_address(FILE *fp, int val);

#endif
