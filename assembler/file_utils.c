#include <stdlib.h>
#include <string.h>

#include "file_utils.h"

/* return filename with extension */
char* create_file_name(char* base, char* extension) {
    char* filename = (char *)malloc(strlen(base) + strlen(extension) + 1);
    if (filename) {
        strcpy(filename, base);
        strcat(filename, extension);
    }
    return filename;
}

/* writes a number to file (in padded hex format) */
void write_val(FILE *fp, int val) {
    fprintf(fp, "%06x\n", val);
}

/* writes an address to file (in padded hex format) */
void write_address(FILE *fp, int val) {
    fprintf(fp, "%07d ", val);
}
