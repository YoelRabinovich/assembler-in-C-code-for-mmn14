#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

/*!
 * SymType:
 *  Whether a symbol refers to a line of code (instruction) or data
 */
typedef enum SymType {
    TYPE_UNK = 0,   /* .entry or .extern */
    TYPE_CODE = 1,  /* .data */
    TYPE_DATA = 2   /* .string */
} SymType;
char* sym_type_str(SymType sym_type); /* convert to str */

/*!
 * SymLoc:
 *  Whether a symbol is declared in the current file or an external file
 */
typedef enum SymLoc {
    LOC_UNK = 0,        /* .entry/.extern */
    LOC_ENTRY = 1,      /* .data */
    LOC_EXTERNAL = 2    /* .string */
} SymLoc;
char* sym_loc_str(SymLoc loc); /* convert to str */

/*!
 * Symbol:
 * Symbols will be stored as a dynamic linked list
 */
typedef struct Symbol {
    char* label;
    int address;
    SymType type;
    SymLoc loc;
    struct Symbol* next;
} Symbol;

/*!
 * Adds a new error to the tail of the list
 * Returns 1 if success, 0 if error (if the symbol already exists)
 */
int add_symbol(char* label, SymType type, SymLoc loc);

/*!
 * Lookup a symbol in the table
 * Returns NULL if not found
 */
Symbol* lookup_symbol(char* label);

/*!
 * shifts the addresses of data symbols by the number of words in the code section (IC)
 * so that the data section will come immediately after the code section
 */
void shift_data_addresses();

/*!
 * Updates the 'entry' attribute of symbols in the talble which were declared by
 * an '.entry' directive in the source code
 */
void update_entry_symbol(char* label);

/*!
 * Write entry symbols to file:
 */
void export_entry_symbols(char* file_path);

/*!
 * Free memory of symbol table
 */
void free_symbol_table();

#endif
