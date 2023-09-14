#include <string.h>

#include "passes.h"
#include "string_utils.h"
#include "machine_coder.h"
#include "symbol_table.h"
#include "parser.h"

/*!
 * In the first pass over the parsed input, all label declarations are entered into the symbol table,
 * and whatever parts of the instructions that don't involve label references (whether using direct or
 * relative addressing) are encoded, with placeholders being created for the rest.
   The missing pieces from the previous step will be handled in a 'second pass', when the symbol
   table is complete.
 */
 void first_pass() {
     int i_line;
     n_errors = 0;
     line_num = 0;

     for (i_line = 0; i_line < n_lines; i_line++) {
         ParsedLine* parsed_line = parsed_lines[i_line];
         line_num = parsed_line->line_num;
         if (parsed_line->op != NULL) { /* a code instruction word */
             handle_op(parsed_line);
         }
         else if (parsed_line->directive != NULL) { /* an assembler directive */
             handle_directive(parsed_line);
         }
     }

     /* Also update data addresses in symbol table by shifting them by the number of words in the code section,
      * so that the data section will start immediately after the code section in memory: */
     shift_data_addresses();
}

/*!
* Now that all the symbols have been entered into the table, we can resolve all the addresses of the
 * labels that were referenced (either 'directly' or 'relatively') and fill in the information in the placeholders
 * that were entered into the code image in the first pass accordingly.
 * Also we can update some extra info in the symbol table regarding 'entry' and 'external' symbols.
*/
void second_pass() {
    int i;
    int i_line;
    line_num = 0;
    n_errors = 0;

    /*! Update symbols from 'entry' directives with the 'entry' attribute in the table */
    for (i_line = 0; i_line < n_lines; i_line++) {
        ParsedLine* parsed_line = parsed_lines[i_line];
        line_num = parsed_line->line_num;
        if (parsed_line->directive != NULL && strcmp(parsed_line->directive, ".entry") == 0) {
            update_entry_symbol(parsed_line->args[0]);
        }
    }

    /*! Finally fill in addresses and linker info (A-R-E) for label operands that were referenced
     * using direct and relative address modes, and whose addresses are now in the symbol table */
    for (i = 0; i < i_symbol_ref; i++) {
        SymbolInfo symbol_info = symbol_references[i];
        line_num = symbol_info.line_num;
        edit_operand(symbol_info.IC, symbol_info.label, symbol_info.addrMode);
    }
}

/*!
 * Used in the first pass: Generates instruction words and operand words (for the lines with an 'op'
 * in the source code) to go into the code image. Some info will be missing due to label references whose
 * addresses have not yet been entered into the symbol table. This will be filled in in the second pass.
 */
void handle_op(ParsedLine *parsed_line) {
    int i_arg;
    int reg_1 = 0;
    int reg_2 = 0;
    char* arg_1 = NULL;
    char* arg_2 = NULL;
    Op* op;
    AddrMode addr_mod_1 = IMMEDIATE;
    AddrMode addr_mod_2 = IMMEDIATE;

    /* Enter label (if there is one) into symbol table before adding new code */
    if (parsed_line->label != NULL) {
        add_symbol(parsed_line->label, TYPE_CODE, LOC_UNK);
    }

    /* Add instruction word to code image: */
    op = get_op(parsed_line->op);
    if (op == NULL) { /* can't happen since we are already passed pre-processing stage but in any case ... */
        return;
    }
    /* Determine addr mode and register fields for each operand */
    for (i_arg = 0; i_arg < parsed_line->n_args; i_arg++) {
        char* arg = parsed_line->args[i_arg];
        AddrMode mode = get_addr_mode(arg);
        if (i_arg == 0 && parsed_line->n_args == 2) { /* so this is the source arg */
            arg_1 = str_cpy(arg);
            addr_mod_1 = mode;
            if (mode == REGISTER) {
                reg_1 = get_register(arg);
            }
        }
        else { /* dest arg */
            arg_2 = str_cpy(arg);
            addr_mod_2 = mode;
            if (mode == REGISTER) {
                reg_2 = get_register(arg);
            }
        }
    }
    add_instruction(op->opcode, addr_mod_1, reg_1, addr_mod_2, reg_2, op->funct);

    /* Add an operand word for each (non-register) arg: */
    if (arg_1 != NULL) {
        handle_operand(arg_1, addr_mod_1);
    }
    if (arg_2 != NULL) {
        handle_operand(arg_2, addr_mod_2);
    }
}

/*!
 * Used in first pass: Adds an operand word for each (non-register) arg
 * For IMMEDIATE operands, we can add the full information.
 * For DIRECT and RELATIVE operands, it will be just a placeholder,
 * and we will store some other information on the side to be used in the
 * 'second pass' to fill in the missing details:
 */
void handle_operand(char *arg, AddrMode addr_mod) {
    switch (addr_mod) {
        case IMMEDIATE: /* first remove the '#' prefix */
            add_operand(get_int_value(arg, 1), Linker_A);
            return;
        case RELATIVE: /* first remove the '&' prefix */
            arg = get_substr(arg, 1, strlen(arg));
        case DIRECT: {
            SymbolInfo symbolInfo;
            symbolInfo.line_num = line_num;
            symbolInfo.IC = get_IC();
            symbolInfo.label = arg;
            symbolInfo.addrMode = addr_mod;
            symbol_references[i_symbol_ref++] = symbolInfo;
            add_operand(0, Linker_UNK);
        case REGISTER:
            {/* these are encoded inside the instruction word and do not generate operand words */}
        }
    }
}

/* Detect AddrMode of input arg */
AddrMode get_addr_mode(char* str) {
    if (str[0] == '#') {
        return IMMEDIATE;
    }
    if (str[0] == '&') {
        return RELATIVE;
    }
    if (get_register(str) != -1) {
        return REGISTER;
    }
    return DIRECT;
}

/*!
 * For first pass - enter numerical and string data into data image and symbol table
 * and also extern symbols into the symbol table:
 */
void handle_directive(ParsedLine *parsed_line) {
    int i_arg;
    int i;

    if (strcmp(parsed_line->directive, ".data") == 0 || strcmp(parsed_line->directive, ".string") == 0) {
        /* Enter data symbol (if there is was a label in the src code) into symbol table,
         * and then add the new integer/string data: */
        if (parsed_line->label != NULL) {
            add_symbol(parsed_line->label, TYPE_DATA, LOC_UNK);
        }
        if (strcmp(parsed_line->directive, ".data") == 0) {
            for (i_arg = 0; i_arg < parsed_line->n_args; i_arg++) {
                add_data(get_int_value(parsed_line->args[i_arg], 0));
            }
        }
        else { /* string data: need to convert it to a seq of ascii values (excluding the quotes) */
            for (i = 1; i < strlen(parsed_line->args[0]) -1; i++) {
                add_data((int)parsed_line->args[0][i]);
            }
            add_data(0); /* terminating 0 */
        }
    }
    else if (strcmp(parsed_line->directive, ".extern") == 0) {
        add_symbol(parsed_line->args[0], TYPE_UNK, LOC_EXTERNAL);
    }
}
