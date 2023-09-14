#include <stdlib.h>

#include "parser.h"
#include "assembler.h"
#include "machine_coder.h"
#include "passes.h"
#include "file_utils.h"


/*********************************** Global variables ***********************************/

/* Keep track of errors */
int n_errors = 0;

char line[LINE_LEN];

/* Keep track of source file line_num (including blank lines) to indicate the line number in case of errors  */
int line_num = 0;

/* Keep track of valid parsed lines (this is the length of the parsed_lines array below) */
int n_lines = 0;

/* * Processed input lines will be stored in an array of structured data */
ParsedLine** parsed_lines;

/* Keep track of number of symbols during the pre-processing stage */
int n_symbols = 0;

/* During the pre-processing stage, keep track of number of words
 * (instruction/operand) that will be generated so we can efficiently
 * allocate the correct size array for the code/data images without
 * assuming anything about the size of the input */
int n_code_words = 0;

/* During the pre-processing stage, keep track of number of data words
 * that will be generated so we can efficiently allocate the correct size array
 * for the code/data images without assuming anything about the size of the input */
int n_data_words = 0;

/* SymbolInfos for instances of symbol references are stored in pass 1 for use in pass 2 */
SymbolInfo* symbol_references;
int i_symbol_ref = 0;
int n_symbol_refs = 0;

/* Initializing the 8 predefined registers */
Register registers[] = {
    {"r0", 0},
    {"r1", 1},
    {"r2", 2},
    {"r3", 3},
    {"r4", 4},
    {"r5", 5},
    {"r6", 6},
    {"r7", 7}
};

/* Construct specification info for the 4 assembler directives
('.entry', '.extern', '.data', '.string') */
Directive directives[4] = {
    {".string", 1, STRING},  /* e.g. .string "abcd" which is converted to .string 'a', 'b', 'c', 'd', '\0' */
    {".data", 999999, INT}, /*  e.g. .data 6, -9, 87...*/
    {".entry", 1, LABEL}, /* e.g. .entry MAIN */
    {".extern", 1, LABEL}  /* e.g. .extern MAX}*/
};

/*
 * Construct the specification info for the 16 instruction operations:
 * Each op takes 0-2 args (operands), each of which can be used only with certain addressing modes
 * (as indicated by a 1 in the relevant position of the corresponding bit array).
 * Format: {<op name>, <opcode>, <funct>, <num of args>, {src arg addr modes>, <dest arg addr modes>}
 */
Op ops[] = {
    /* mov: e.g. mov X, r1 / mov X, Y / mov #10, r1 */
    {"mov", 0, 0, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    /* "cmp": e.g. cmp X, r1 / cmp #10, X / cmp X, #10  */
    {"cmp", 1, 0, 2, {1, 1, 0, 1}, {1, 1, 0, 1}},
    /* "add" e.g. add X, r1 */
    {"add", 2, 1, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    /* "sub" e.g. sub #5, r1 */
    {"sub", 2, 2, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    /* "lea" e.g. lea X, r1 / lea X, Y */
    {"lea", 4, 0, 2, {0, 1, 0, 0}, {0, 1, 0, 1}},
    /* "clr" e.g. clr r1 / clr X */
    {"clr", 5, 1, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    /* "not" e.g. not r1 / not X */
    {"not", 5, 2, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    /* "inc" e.g. inc r1 / inc X */
    {"inc", 5, 3, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    /* "dec" e.g. dec r1 / dec Y */
    {"dec", 5, 4, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    /* "jmp" e.g. jmp LOOP / jmp &LOOP */
    {"jmp", 9, 1, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    /*"bne" e.g. bne LOOP / bne &LOOP */
    {"bne", 9, 2, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    /* "jsr" e.g. jsr LOOP / jsr &LOOP */
    {"jsr", 9, 3, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    /* "red" e.g. red X, / red reg2 */
    {"red", 12, 0, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    /* "prn" e.g. prn #10 / prn X /  prn reg2 */
    {"prn", 13, 0, 1, {0, 0, 0, 0}, {1, 1, 0, 1}},
    /* "rts" - no args  */
    {"rts", 14, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}},
    /* "stop" - no args  */
    {"stop", 15, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}}
};


/*********************************** Main ***********************************/ 

/*
* General outline:
*
* First we do a preliminary pre-processing stage where all syntax is parsed
* and checked for validity according to the assembly language specifications.
* (If syntax errors are caught at this stage, we don't proceed any further).
*
* The result of this stage is a structured version of the input so that
* the next stages (the '2-passes' of encoding and generating the
* machine code output) only need to pass over this validated and structured input,
* rather than re-read the raw input files again.
*
* Furthermore, during the pre-processing stage, we can record
* how many symbols/instructions/operands are encountered, so that we are able allocate
* the correct amount of memory for the various data structures without having to assume
* any maximum input size.
*
* We then proceed to the core part of the assembler i.e. the '2 passes' of encoding and
* generating the machine code output, as described below:
*/
int main(int argc, char * argv[]) {
    FILE* fp;
    int i_inputs;
    char * input_path;
    int rc = 0;
    ParsedLine *parsed_line;

    if (argc < 2) {
        printf("No input files specified.\nUsage: assembler <file1> [<file2> <file3> ...]\n");
        return 1;
    }

    /* Process each .as file given in the cmd line input */
    for (i_inputs = 1; i_inputs < argc; i_inputs++) {
        reset_counters();

        input_path = create_file_name(argv[i_inputs], ".as");
        fp = fopen(input_path, "r");
        if (fp == NULL) {
            fprintf(stderr, "Error: Unable to open '%s'\n", input_path);
            continue;
        }

        printf("\n>>> \'%s\'\n\n", input_path);

        /* Pre-processing stage: Parse, validate and restructure input file line by line: */
        while (fgets(line, sizeof(line), fp) != NULL) {
            line_num++;
            parsed_line = parse_line(line_num, line);
            if (parsed_line != NULL && add_parsed_line(parsed_line)) {
                /* Keep track of how many entries we will have to allocate for the symbol table: */
                n_symbols += get_num_symbols(parsed_line);

                /* Keep track of how many words we will have to allocate for the code image: */
                n_code_words += get_num_code_words(parsed_line);

                /* Keep track of how many words we will have to allocate for the data image: */
                n_data_words += get_num_data_words(parsed_line);

                /* Keep track of how many symbol references we need to allocate for: */
                n_symbol_refs += get_num_symbol_refs(parsed_line);
            }
        }
        fclose(fp);
        free(input_path);

        if (n_errors) { /* no point in carrying on to next stage */
            printf("*** Syntax checker found %i errors. Skipping file. ***\n", n_errors);
            free_parsed_lines();
            rc |= n_errors;
            continue;
        }

        /* Allocate memory for the assembler stages: */
        if (!init_code_image(n_code_words) ||
            !init_data_image(n_data_words) ||
            !init_word_types(n_code_words + n_data_words) ||
            !init_symbol_refs()) {

            printf("*** Memory allocation error. Skipping file. ***.\n");
            n_errors++;
            rc |= n_errors;
            continue;
        }

        /* Do the 'first pass' on the validated and structured input to build symbol table and
         * to start encoding machine code output */
        first_pass();
        if (n_errors) { /* no point in carrying on to next stage */
            printf("*** %i errors found in first pass. Skipping file. ***\n", n_errors);
            free_memory();
            rc |= n_errors;
            continue;
        }

        /* Do the 'second pass' to fill missing info from the completed symbol table */
        second_pass();

        /* If no errors, generate output files */
        if (!n_errors) {
            create_output_files(argv[i_inputs]);
        }
        else {
            printf("*** %i errors found in second pass. Skipping file. ***\n", n_errors);
            rc |= n_errors;
        }
        free_memory();
    }
    return rc > 0;
}

/*********************************** Struct functions and variables ***********************************/

/* Finds register info by name
 * Returns -1 if invalid */
int get_register(char* reg_name) {
    int i;
    for (i = 0; i < N_REGISTERS; i++) {
        if (strcmp(reg_name, registers[i].name) == 0) {
            return registers[i].id;
        }
    }
    return -1;
}

/* Fetches directive info by name
 * Returns NULL if invalid */
Directive* get_directive(char* directive_name) {
    int i;
    for (i = 0; i < N_DIRECTIVES; i++) {
        if (strcmp(directives[i].name, directive_name) == 0) {
            return &directives[i];
        }
    }
    return NULL;
}

/* Finds op info by name
 * Returns NULL if invalid */
Op* get_op(char* op) {
    int i;
    for (i = 0; i < N_OPS; i++) {
        if (strcmp(ops[i].name, op) == 0) {
            return &ops[i];
        }
    }
    return NULL;
}


/*********************************** Help functions ***********************************/

/* Add a new parsed line (allocating memory if needed) */
int add_parsed_line(ParsedLine* parsed_line) {
    if (n_lines == 0) { /* need to allocate initial memory */
        parsed_lines = (ParsedLine**)malloc(sizeof(ParsedLine*) * INPUT_BATCH_SIZE);
        if (parsed_lines == NULL) {
            n_errors++;
            printf("Failed to allocate memory for parsing input lines\n");
            return 0;
        }
    }
    else if (n_lines % INPUT_BATCH_SIZE == 0) { /* need to resize the array */
        ParsedLine** tmp = (ParsedLine**)realloc(parsed_lines, sizeof(ParsedLine*) * (n_lines / INPUT_BATCH_SIZE) + 1);
        if (tmp == NULL) {
            n_errors++;
            printf("Failed to reallocate memory for parsing input lines\n");
            return 0;
        }
        parsed_lines = tmp;
    }
    parsed_lines[n_lines++] = parsed_line;
    return 1;
}

/* Free parsed_lines memory after each input file is done */
void free_parsed_lines() {
    int i_line;
    if (n_lines > 0) {
        for (i_line = 0; i_line < n_lines; i_line++) {
            free_parsed_line(parsed_lines[i_line]);
        }
        n_lines = 0;
        free(parsed_lines);
    }
}

/* init symbol_references array */
int init_symbol_refs() { /* free memory after each input file is done */
    i_symbol_ref = 0;
    symbol_references = (SymbolInfo*)malloc(sizeof(SymbolInfo) * n_symbol_refs);
    return symbol_references != NULL;
}

/* init symbol_references array */
void free_symbol_refs() { /* free memory after each input file is done */
    int i;
    for (i = 0; i < n_symbol_refs; i++) {
        free(symbol_references[i].label);
    }
    free(symbol_references);
    i_symbol_ref = 0;
    n_symbol_refs = 0;
}

/* Free all memory after each input file is done */
void free_memory() {
    free_parsed_lines();
    free_mc_memory();
    free_symbol_table();
    free_symbol_refs();
}

/* to_string function for LinkerInfo enum */
char* linker_info_str(LinkerInfo linker_info) {
    switch (linker_info) {
        case Linker_UNK: return "UNK";
        case Linker_E:   return "E";
        case Linker_R: return "R";
        case Linker_A: return "A";
        default: return "Unknown LinkerInfo";
    }
}

/* to_string function for DirectiveArgType enum */
char* arg_type_str(DirectiveArgType arg_type) {
    switch (arg_type) {
        case LABEL: return "LABEL";
        case INT:   return "INT";
        case STRING: return "STRING";
        default: return "Unknown DirectiveArgType";
    }
}

/* to_string function for AddrMode enum */
char* addr_mode_str(AddrMode mode) {
    switch (mode) {
        case IMMEDIATE: return "IMMEDIATE";
        case DIRECT:   return "DIRECT";
        case RELATIVE: return "RELATIVE";
        case REGISTER: return "REGISTER";
        default: return "Unknown Addrmode";
    }
}

/* If no errors, the output files are generated */
void create_output_files(char *output_path) {
    char* path;
    path = create_file_name(output_path, ".ob");
    write_object_file(path);
    printf("  - Successfully created %s\n", path);
    free(path);

    path = create_file_name(output_path, ".ext");
    write_ext_file(path);
    printf("  - Successfully created %s\n", path);
    free(path);

    path = create_file_name(output_path, ".ent");
    export_entry_symbols(path);
    printf("  - Successfully created %s\n", path);
    free(path);
}

/* reset the various counters before processing each file */
void reset_counters() {
    n_errors = 0;
    line_num = 0;
    n_lines = 0;
    n_symbols = 0;
    n_code_words = 0;
    n_data_words = 0;
    i_symbol_ref = 0;
}