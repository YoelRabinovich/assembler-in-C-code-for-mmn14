#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "machine_coder.h"
#include "file_utils.h"
#include "symbol_table.h"

/* Symbols will be stored as a dynamic linked list */
struct Symbol* symbol_table = NULL;
struct Symbol* tail = NULL; /* to insert at the end without traversal */

/* enum to_string converter */
char* sym_type_str(SymType sym_type) {
    switch (sym_type) {
        case TYPE_UNK: return "N/A";
        case TYPE_CODE:   return "CODE";
        case TYPE_DATA: return "DATA";
        default: return "Unknown SymType";
    }
}

/* enum to_string converter */
char* sym_loc_str(SymLoc sym_loc) {
    switch (sym_loc) {
        case LOC_UNK: return "N/A";
        case LOC_ENTRY:   return "ENTRY";
        case LOC_EXTERNAL: return "EXTERNAL";
        default: return "Unknown SymLoc";
    }
}

/* Internal function used by lookup_symbol and add_symbol */
Symbol* _get_symbol(char* label) {
    Symbol* symbol;
    symbol = symbol_table;
    while (symbol != NULL) {
        if (strcmp(symbol->label, label) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }
    return NULL;
}

/* Adds a new symbol to the tail of the list */
int add_symbol(char* label, SymType type, SymLoc loc) {
    struct Symbol* new_symbol;

    /* First check this symbol doesn't already exist */
    if (_get_symbol(label) != NULL) {
        n_errors++;
        printf("Error in line %i: Symbol \'%s\' already exists\n", line_num, label);
        return 0;
    }

    new_symbol = (Symbol*)malloc(sizeof(Symbol));
    new_symbol->label = label;
    if (loc == LOC_EXTERNAL) {
        new_symbol->address = 0;
    }
    else if (type == TYPE_CODE) {
        new_symbol->address = get_IC();
    }
    else { /* data */
        new_symbol->address = get_DC();
    }

    new_symbol->type = type;
    new_symbol->loc = loc;
    new_symbol->next = NULL;

    if (symbol_table == NULL) {
        symbol_table = new_symbol;
        tail = new_symbol;
    }
    else {
        tail->next = new_symbol;
        tail = new_symbol;
    }
    return 1;
}

/* Lookup a symbol in the table.
 * Returns NULL if not found. */
Symbol* lookup_symbol(char* label) {
    Symbol* symbol;
    symbol = _get_symbol(label);
    if (symbol == NULL) {
        printf("Error in line %i: Unrecognized symbol \'%s\'\n", line_num, label);
        n_errors++;
    }
    return symbol;
}

/* shifts the addresses of data symbols by the number of words in the code section (=IC)
 * so that the data section will come immediately after the code section */
void shift_data_addresses() {
    Symbol* symbol;
    symbol = symbol_table;
    while (symbol != NULL) {
        if (symbol->type == (int)DATA) {
            symbol->address += get_IC();
        }
        symbol = symbol->next;
    }
}

/* Updates the 'entry' attribute of symbols in the table which were declared by
 * an '.entry' directive in the source code */
void update_entry_symbol(char* label) {
    Symbol *symbol;
    symbol = lookup_symbol(label);
    if (symbol != NULL) {
        symbol->loc = LOC_ENTRY;
    }
}

/* Write Symbol table to .ent file */
void export_entry_symbols(char* file_path) {
    FILE *fp;
    Symbol* symbol;
    symbol = symbol_table;
    fp = fopen(file_path, "w");
    if (fp) {
        while (symbol != NULL) {
            if (symbol->loc == LOC_ENTRY) {
                fprintf(fp, "%s ", symbol->label);
                write_address(fp, symbol->address);
                fprintf(fp, "\n");
            }
            symbol = symbol->next;
        }
        fclose(fp);
    }
    else {
        n_errors++;
    }
}

/* Free symbol table memory */
void free_symbol_table() {
    Symbol* tmp;
    while (symbol_table != NULL) {
        tmp = symbol_table;
        symbol_table = symbol_table->next;
        free(tmp);
    }
}
