#include <stdio.h>
#include <stdlib.h>

#include "symbol_table.h"
#include "file_utils.h"
#include "machine_coder.h"

/*********************************** Variables ***********************************/

/* The instructions counter holding the address where the next instruction/operand word will go */
static unsigned int IC = MEM_START_ADDRESS;

/* The machine code (instructions/operands) to be generated will accumulate here */
static union Code* code_image;

/* To keep track of the types of each word entered into the code output array (i.e. instruction or operand) */
static WordType* word_types;

/* The data words (string/int) to be generated will accumulate here */
static int* data_image;

/* The data counter pointing to where in the data image the next data word will go */
static unsigned int DC = 0;

/* Needed to write extern file */
extern int i_symbol_ref;
extern  SymbolInfo* symbol_references;


/*********************************** Functions ***********************************/

/*!
* Initialize code image array:
 * returns 1 if success, 0 if failure
*/
int init_code_image(size_t n) {
    IC = MEM_START_ADDRESS;
    code_image = (Code*)malloc(sizeof(Code) * n);
    return code_image != NULL;
}

/*!
* Initialize data image array:
 * returns 1 if success, 0 if failure
*/
int init_data_image(size_t n) {
    DC = 0;
    data_image = (int*)malloc(sizeof(int) * n);
    return data_image != NULL;
}

/*!
* Initialize word_types array:
 * returns 1 if success, 0 if failure
*/
int init_word_types(size_t n) {
    word_types = (WordType*)malloc(sizeof(WordType) * n);
    return word_types != NULL;
}

/* Frees memory allocated for code image, data image and word_types array */
void free_mc_memory() {
    free(code_image);
    free(data_image);
    free(word_types);
}

/* Symbol table needs to know the current IC when adding new symbol */
unsigned int get_IC() {
    return IC;
}

/* Symbol table needs to know the current DC when adding new symbol */
unsigned int get_DC() {
    return DC;
}

/* Add an instruction word to the code image */
void add_instruction(int opcode, AddrMode addrMode_1, int reg_1, AddrMode addrMod_2, int reg_2, int funct) {
    union Code word;
    Instruction instruction;
    int index;
    index = IC - MEM_START_ADDRESS;
    instruction.opcode = opcode;
    instruction.arg_1_mode = addrMode_1;
    instruction.reg_1 = reg_1;
    instruction.arg_2_mode = addrMod_2;
    instruction.reg_2 = reg_2;
    instruction.funct = funct;
    instruction.linker_info = Linker_A;
    word.instruction = instruction;
    code_image[index] = word;
    word_types[index] = INSTRUCTION;
    IC++;
}

/* Edit an operand word in the code image whose address and linker info was missing */
void edit_operand(int ic, char*label, AddrMode mode) {
    int index;
    Operand* operand;
    Symbol* symbol;

    index = ic - MEM_START_ADDRESS;
    symbol = lookup_symbol(label);
    if (symbol == NULL) {
        return;
    }

    if (word_types[index] == OPERAND) {
        operand = &(code_image[index].operand);
        if (mode == DIRECT) {
            operand->value = symbol->address;
            if (symbol->loc == LOC_EXTERNAL) {
                operand->linker_info = Linker_E;
            }
            else {
                operand->linker_info = Linker_R;
            }
        }
        else if (mode == RELATIVE) {
            operand->value = symbol->address - ic + 1;
            operand->linker_info = Linker_A;
        }
    }
}

/* Add an operand word to the code image */
void add_operand(int value, LinkerInfo linker_info) {
    union Code word;
    Operand operand;
    int index;
    index = IC - MEM_START_ADDRESS;
    operand.value = value;
    operand.linker_info = linker_info;
    word.operand = operand;
    code_image[index] = word;
    word_types[index] = OPERAND;
    IC++;
}

/* Add a data word to the data image */
void add_data(int data) {
    data_image[DC++] = data;
}

/* converts a number to 2's complement */
int twos_comp(int val) {
    if (val < 0) {
        return (1 << 24) + val;
    }
    return val;
}

/* encodes an instruction word */
int encode_instruction(Instruction instruction) {
    return
        (instruction.opcode << 18) + \
        (instruction.arg_1_mode << 16) + \
        (instruction.reg_1 << 13) + \
        (instruction.arg_2_mode << 11) + \
        (instruction.reg_2 << 8) + \
        (instruction.funct << 3) + \
        (instruction.linker_info);
}

/* encodes an operand word (2s complement) */
int encode_operand(Operand operand) {
    return twos_comp(operand.value << 3) + (operand.linker_info);
}

/* Generate the machine code to .ob file */
void write_object_file(char* file_path) {
    int i;
    int address;
    WordType wordType;
    FILE *fp;
    fp = fopen(file_path, "w");

    address = MEM_START_ADDRESS;
    if (fp) {
        /* Header */
        fprintf(fp, "%7i %-6i\n", IC - MEM_START_ADDRESS, DC);

        /* Code section: */
        for (i = 0; i < IC - MEM_START_ADDRESS; i++, address++) {
            write_address(fp, address);
            wordType = word_types[i];
            switch (wordType) {
                case INSTRUCTION:
                    write_val(fp, encode_instruction(code_image[i].instruction));
                    break;
                case OPERAND:
                    write_val(fp, encode_operand(code_image[i].operand));
                    break;
                case DATA: {/* not relevant here */}
            }
        }

        /* Data section: */
        for (i = 0; i < DC; i++, address++) {
            write_address(fp, address);
            write_val(fp, twos_comp(data_image[i]));
        }
        fclose(fp);
    } else {
        n_errors++;
    }
}

/* Generate the .ext file */
void write_ext_file(char* file_path) {
    int i;
    int address;
    char* label = NULL;
    Symbol* symbol;
    FILE *fp;
    fp = fopen(file_path, "w");
    if (fp) {
        for (i = 0; i < i_symbol_ref; i++) {
            address = symbol_references[i].IC;
            label = symbol_references[i].label;
            symbol = lookup_symbol(label);
            if (symbol != NULL && symbol->loc == LOC_EXTERNAL) {
                fprintf(fp, "%s ", label);
                write_address(fp, address);
                fprintf(fp, "\n");
            }

        }
        fclose(fp);
    }
    else {
        n_errors++;
    }
}
