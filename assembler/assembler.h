#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <string.h>

#include "symbol_table.h"

/*********************************** Constants ***********************************/

/* The instruction image will be generated to start at this address */
#define MEM_START_ADDRESS 100

/* Number of input lines to allocate for. Each time the amount allocated
 * is exceeded, another 'batch' is dynamically reallocated */
#define INPUT_BATCH_SIZE 1024

/* Amount to allocate for a line (even though the legal max is only 80) */
#define LINE_LEN 2056

/* Maximum length of an input line */
#define MAX_LINE_LEN 80

/* num of registers */
#define N_REGISTERS 8

/* num of directives */
#define N_DIRECTIVES 4

/* num of ops */
#define N_OPS 16



/*********************************** Structures ***********************************/
/* Data structures and their relevant function prototypes */
/*
 * LinkerInfo:
 * 3 flag bits (A-R-E) at the end of an instruction/operand's encoding (needed by the linking/loading stage)
 *  - A indicates that the encoding is 'absolute' and will not change
 *  - R indicates that the encoding is an internal address which is 'relocatable' i.e. will change in the linking/loading stage
 *  - E indicates that the encoding is an external address which will be determined in the linking/loading stage
 */
typedef enum LinkerInfo {
    Linker_UNK = 0, /* (000) */
    Linker_E = 1,   /* (001) */
    Linker_R = 2,   /* (010) */
    Linker_A = 4   /* (100) */
} LinkerInfo;

char* linker_info_str(LinkerInfo linker_info); /* enum to str */

/*
 * DirectiveArgType:
 *  Argument types that used by the various directives
 */
typedef enum DirectiveArgType {
    LABEL = 0,  /* .entry or .extern */
    INT = 1,    /* .data */
    STRING = 2  /* .string */
} DirectiveArgType;

char* arg_type_str(DirectiveArgType arg_type); /* enum to str */

/*
 * AddrMode:
 *  Operand (addressing) modes:
    - IMMEDIATE: an actual (pos/neg) integer value prefixed with '#' e.g. prn #-48
    - DIRECT: a label, to be translated into the address in the code image where it is declared e.g. "inc X", or "jmp LOOP"
    - RELATIVE: a label, prefixed by '&', denoting the (pos/neg) distance (in 'words') of the current instruction from the address where the specified label is declared e.g. "jmp &LOOP"
    - REGISTER: the name of a register ('r0'-'r7')
 */
typedef enum AddrMode {
    IMMEDIATE = 0,
    DIRECT = 1,
    RELATIVE = 2,
    REGISTER = 3
} AddrMode;

char* addr_mode_str(AddrMode arg_type); /* enum to str */

/*
 * ParsedLine:
 * Each input line is parsed, checked for syntax, and restructured into the following structure
 * before entering the 2-pass assembler stages:
 */

typedef struct ParsedLine {
    int line_num;
    char* label; /* optional */
    char* op; /* relevant iff the input line is one of the 16 assembler 'operations' */
    char* directive;  /* relevant iff the input line is one of the 4 assembler 'directives' */
    int n_args; /* the number of (comma-separated) args specified */
    char** args;  /* the (comma-separated) argument(s) which followed the op/directive on the input line */
} ParsedLine;

/*!
 * SymbolInfo:
 * This will store information concerning label references during the
 * first pass that will then be used in the second pass to fill the missing pieces.
 * Also used to write the .ext file */
typedef struct SymbolInfo {
    int line_num;
    unsigned int IC;
    char* label;
    AddrMode addrMode;
} SymbolInfo;


/*
 * Instruction:
 * Contains the information for a code instruction word to be encoded into hex and generated as output
 */
typedef struct Instruction {
    int opcode;
    AddrMode arg_1_mode;
    int reg_1;
    AddrMode arg_2_mode;
    int reg_2;
    int funct;
    LinkerInfo linker_info;
} Instruction;

/*
 * Operand:
 * Contains the information for a code operand word to be encoded into hex and generated as output
 */
typedef struct Operand {
    int value;  /* either an address or a literal numeric value */
    LinkerInfo linker_info;
} Operand;

/*
 * WordType:
 * To keep track of the types of each word entered into the code output array
 */
typedef enum WordType {
    INSTRUCTION = 0,
    OPERAND = 1,
    DATA = 2
} WordType;

/*
 * Code:
 * Represents a line of code that will be generated - can be either an instruction/operand
 */
typedef union Code {
    Instruction instruction;
    Operand operand;
} Code;


/* Register Struct: Name and id of the registers */
typedef struct Register {
    char* name;
    int id;
} Register;

/* Finds register info by name
 * Returns -1 if invalid */
int get_register(char* reg_name);


/*
 * Directive:
 * Specification info for the 4 assembler directives ('.entry', '.extern', '.data', '.string')
 */
typedef struct Directive {
    char* name;  /* Can be: '.entry', '.extern', '.data', '.string' */
    int n_args;
    DirectiveArgType arg_type;
} Directive;

/* Fetches directive info by name
 * Returns NULL if invalid */
Directive* get_directive(char* directive_name);

/*
 * Op:
 * Specification info for the 16 instruction operations:
 * These are: mov, cmp, add, sub, lea, clr, not, inc, dec, jmp, bne, jsr, red, prn, rst, stop
 * Each op takes 0-2 args (operands), each of which can be used only with certain 'addressing modes'
 * (as indicated by a 1 in the relevant position of its bit array).
 */
typedef struct Op {
    char* name;  /* One of: '.entry', '.extern', '.data', '.string' */
    int opcode; /* Not necessarily unique - see funct below */
    int funct;  /* Needed to distinguish ops having the same opcode */
    int n_args;
    int arg_1_modes[4]; /* a bit array indicating, which address modes can be used by the 'src' operand of this op */
    int arg_2_modes[4]; /* a bit array indicating, which address modes can be used by the 'dest' operand of this op */
} Op;

/* Finds op info by name
 * Returns NULL if invalid */
Op* get_op(char* op);

/*********************************** Function Prototypes ***********************************/

/* Add a new parsed line (allocating memory if needed) */
int add_parsed_line(ParsedLine* parsed_line);

/* Free parsed_lines memory after each input file is done */
void free_parsed_lines();

/* init symbol_references array */
int init_symbol_refs();

/* init symbol_references array */
void free_symbol_refs();

/* Free all memory after each input file is done */
void free_memory();

/* to_string function for LinkerInfo enum */
char* linker_info_str(LinkerInfo linker_info);

/* to_string function for DirectiveArgType enum */
char* arg_type_str(DirectiveArgType arg_type);

/* to_string function for AddrMode enum */
char* addr_mode_str(AddrMode mode);

/* If no errors, the output files are generated */
void create_output_files(char *output_path);

/* reset the various counters before processing each file */
void reset_counters();


/*********************************** External variables ***********************************/
/* Initialized in main file assembler.c, and are also used in other files */

/* Keep track of errors */
extern int n_errors;

/* Keep track of source file line_num (including blank lines) to indicate the line number in case of errors  */
extern int line_num;

/* Keep track of valid parsed lines (this is the length of the parsed_lines array below) */
extern int n_lines;

/* * Processed input lines will be stored in an array of structured data */
extern ParsedLine** parsed_lines;

/* SymbolInfos for instances of symbol references are stored in pass 1 for use in pass 2:*/
extern SymbolInfo* symbol_references;
extern int i_symbol_ref;

#endif
