#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "parser.h"
#include "string_utils.h"
#include "passes.h"

/* Constructor' for ParsedLine struct */
ParsedLine* construct_parsed_line(char* label, char* op, char* directive, int n_args, char** args) {
    int i;
    ParsedLine* parsed_line;
    parsed_line = (ParsedLine*)malloc(sizeof(ParsedLine));
    parsed_line->line_num = line_num;
    parsed_line->label = label; /*str_cpy(label);*/
    parsed_line->op = str_cpy(op);
    parsed_line->directive = str_cpy(directive);
    parsed_line->args = (char**)malloc(sizeof(char*) *  n_args);
    for (i = 0; i < n_args; i++) {
        parsed_line->args[i] = str_cpy(args[i]);
    }
    parsed_line->n_args = n_args;
    free(args);
    return parsed_line;
}

/* Free memory allocated to a ParsedLine */
void free_parsed_line(ParsedLine* parsed_line) {
    int i;
    if (parsed_line->label != NULL) {
        free(parsed_line->label);
    }
    if (parsed_line->op != NULL) {
        free(parsed_line->op);
    }
    if (parsed_line->directive != NULL) {
        free(parsed_line->directive);
    }
    for (i = 0; i < parsed_line->n_args; i++) {
        free(parsed_line->args[i]);
    }
    if (parsed_line->args != NULL) {
        free(parsed_line->args);
    }
    free(parsed_line);
}

/*
* Calculates the number of instruction/operand words that will be required to encode this line
 * (for the purpose of efficiently allocating the correct size array for the
 * code image without assuming anything about the size of the input)
*/
int get_num_code_words(ParsedLine* line) {
    int n_code_words;
    int i;
    n_code_words = 0;
    if (line->op != NULL) {
        n_code_words++; /* 1 instruction word */
        for (i = 0; i < line->n_args; i++) {
            if (get_register(line->args[i]) == -1) { /* only non-register operands generate words */
                n_code_words++;
            }
        }
        return n_code_words;
    }
    else { /* .entry and .extern directives don't generate words in the machine code output */
        return 0;
    }
}

/*
* Calculates the number of data words that will be required to encode this line
 * (for the purpose of efficiently allocating the correct size array for the
 * data image without assuming anything about the size of the input)
*/
int get_num_data_words(ParsedLine* line) {
    if (line->directive != NULL && strcmp(line->directive, ".data") == 0) {
        return line->n_args; /* 1 word for each integer */
    }
    else if (line->directive != NULL && strcmp(line->directive, ".string") == 0) {
        return strlen(line->args[0]) -2 + 1; /* the number of chars, minus the quotes, plus the terminating '\0' */
    }
    else { /* .entry and .extern directives don't generate words in the machine code output */
        return 0;
    }
}

/*
* Calculates the number of symbol declarations in this line (to help when
 * initializing the symbol table)
*/
int get_num_symbols(ParsedLine* line) {
    int n_line_symbols;
    int is_label_declaration;

    n_line_symbols = 0;

    /* A symbol can come from either a lable preceding an op or a .data / .string directive,
     * or as the arg of a .extern directive.
     * Labels preceding an .extern or an .entry directive do not count (and are ignored) */
    is_label_declaration =
            (line->label != NULL &&
                (line->op != NULL ||
                (line->directive != NULL && (strcmp(line->directive, ".data") == 0 || strcmp(line->directive, ".string") == 0))));

    is_label_declaration |= (line->directive != NULL && strcmp(line->directive, ".extern") == 0);

    if (is_label_declaration) {
        n_line_symbols++;
    }
    /* Give warning for redundant label declaration */
    if (line->label != NULL &&
            (line->directive != NULL && (strcmp(line->directive, ".entry") == 0 || strcmp(line->directive, ".extern") == 0))) {
        printf("Warning in line %i. Ignoring redundant label \'%s\' in directive \'%s\' ...\n", line_num, line->label, line->directive);
    }

    return n_line_symbols;
}

/*
* Calculates the number of symbol references in this line (to help when
 * initializing the symbol_references array)
*/
int get_num_symbol_refs(ParsedLine* line) {
    int n_symbol_refs;
    int i;
    n_symbol_refs = 0;
    if (line->op != NULL) {
        for (i = 0; i < line->n_args; i++) {
            AddrMode mode = get_addr_mode(line->args[i]);
            if (mode == DIRECT || mode == RELATIVE) { /* these are label references */
                n_symbol_refs++;
            }
        }
    }
    return n_symbol_refs;
}

/*
 * Checks label validity (alpha-numeric, doesn't exceed max length, not reserved words etc.)
 * Returns 1 if valid, otherwise 0
 */
int _validate_label(char* label) {
    if (!isalpha(label[0]) || !is_alnum(label)) {
        printf("Error in line %i: Invalid label: \'%s\' (labels must start with a letter and contain only letters and numbers)\n", line_num, label);
        n_errors++;
        return 0;
    }
    if (strlen(label) > MAX_LABEL_LEN) {
        printf("Error in line %i: Label exceeds max length (31): \'%s\'\n", line_num, label);
        n_errors++;
        return 0;
    }
    if (get_register(label) != -1) {
        printf("Error in line %i: Invalid label: \'%s\' (register names are reserved)\n", line_num, label);
        n_errors++;
        return 0;
    }
    if (get_op(label) != NULL) {
        printf("Error in line %i: Invalid label: \'%s\' (op names are reserved)\n", line_num, label);
        n_errors++;
        return 0;
    }
    if (get_directive(label) != NULL) {
        printf("Error in line %i: Invalid label: \'%s\' (directive names are reserved)\n", line_num, label);
        n_errors++;
        return 0;
    }
    return 1;
}

/*
 * Checks validity of a (pos/neg) integer arg
 * Returns 1 if valid, otherwise 0
 */
int _validate_int(char* arg, int start_idx) {
    if (!is_integer(arg, start_idx)) {
        printf("Error in line %i: Invalid integer value: \'%s\'\n", line_num, arg);
        n_errors++;
        return 0;
    }
    return 1;
}

/*
 * Checks validity of a string arg (for .string directive)
 * Returns 1 if valid, otherwise 0
 */
int _validate_string(char* arg) {
    if (strlen(arg) < 2 || arg[0] != '"' || arg[strlen(arg)-1] != '"') {
        printf("Error in line %i: String literal missing quotes: %s\n", line_num, arg);
        n_errors++;
        return 0;
    }
    if (count_char(arg, '"') > 2) { /* we don't allow this, nor do we support escape characters to allow this */
        printf("Error in line %i: Quotes found inside string literal: \'%s\'\n", line_num, arg);
        n_errors++;
        return 0;
    }
    if (!is_printable(arg)) {
        printf("Error in line %i: Invalid string literal \'%s\'. (must contain only printable chars)\n", line_num, arg);
        n_errors++;
        return 0;
    }
    return 1;
}

/*
 * Checks validity of a directive and checks that its args are according to the specification
 * Returns 1 if valid, otherwise 0
 */
int _validate_directive(char* directive_name, char** args, int n_args ,int line_num) {
    int i;
    Directive* directive = get_directive(directive_name);
    if (directive == NULL) {
        printf("Error in line %i. Unrecognized directive: \'%s\'\n", line_num, directive_name);
        n_errors++;
        return 0;
    }

    if (n_args == 0 || n_args > directive->n_args) {
        printf("Error in line %i. Incorrect number of args for \'%s\' directive. Expected %i but got %i\n",
                line_num, directive_name, directive->n_args, n_args);
        n_errors++;
        return 0;
    }

    for (i = 0; i < n_args; i++) {
        char* arg = args[i];
        if ((directive->arg_type == LABEL && !_validate_label(arg)) ||
                (directive->arg_type == INT && !_validate_int(arg, 0)) ||
                (directive->arg_type == STRING && !_validate_string(arg))) {
            return 0;
        }
    }
    return 1;
}

/*
* Checks validity of an operand (addressing mode and value)
 * Returns 1 if valid, otherwise 0
*/
int _validate_operand(char* op_name, char* operand, char* operand_name, int* valid_addr_modes) {
    char* substr;
    AddrMode mode = get_addr_mode(operand);
    if (!valid_addr_modes[mode]) {
        printf("Error in line: %i. %s operand \'%s\' of \'%s\'. Invalid addr mode: %s'\n",
                line_num, operand_name, operand, op_name, addr_mode_str(mode));
        n_errors++;
        return 0;
    }
    if ((mode == IMMEDIATE && !_validate_int(operand, 1)) ||
    (mode == DIRECT && !_validate_label(operand))) {
        return 0;
    }
    if (mode == RELATIVE) {
        substr = get_substr(operand, 1, strlen(operand));
        if (!_validate_label(substr)) {
            free(substr);
            return 0;
        }
        free(substr);
    }
    return 1;
}

/*
 * Checks validity of an op and checks that its args (both the number of args and their addr modes)
 * are according to specification:
 */
int _validate_op(char* op_name, char** args, int n_args) {
    Op* op = get_op(op_name);
    if (op == NULL) {
        printf("Error in line %i. Unrecognized op: \'%s\'\n", line_num, op_name);
        n_errors++;
        return 0;
    }

    if (n_args != op->n_args) { /* checking the number of args */
        printf("Error in line %i. Incorrect number of args for \'%s\'. Expected %i but got %i\n",
               line_num, op_name, op->n_args, n_args);
        n_errors++;
        return 0;
    }

    if (n_args >= 1) { /* check the last arg with arg_2_modes */
        if (!_validate_operand(op_name, args[n_args-1], "Dest",  op->arg_2_modes)) {
            return 0;
        }
        if (n_args == 2) { /* also check the first arg with arg_1_modes */
            if (!_validate_operand(op_name, args[0], "Source", op->arg_1_modes)) {
                return 0;
            }
        }
    }
    return 1;
}

/*
 * Check there are not multiple consecutive commas or 'dangling' commas at the start or end:
 */
int check_comma_formatting(char* arg_input_str) {
    int i;
    int n_commas;

    /* Check that there are no 'dangling' commas at the start of end */
    if ((arg_input_str[0] == ',' || arg_input_str[strlen(arg_input_str) - 1] == ',')) {
        return 0;
    }

    /* Check that there are not multiple 'consecutive' commas */
    n_commas = 0;
    for (i = 0; i < strlen(arg_input_str); i++) {
        if (arg_input_str[i] == ' ') { /* ignore spaces */
            continue;
        }
        if (arg_input_str[i] == ',') { /* a comma */
            n_commas++;
            if (n_commas > 1) {
                return 0;
            }
        }
        else { /* a arg character so reset the commas count */
            n_commas = 0;
        }
    }
    return 1;
}

/*
 * This is the main input parsing function which parses and checks the syntax of
 * each input line, and restructures it for the subsequent assembler stages:
 */
ParsedLine* parse_line(int line_num, char* line) {
    char* label = NULL;
    char* op = NULL;
    char* directive = NULL;
    int n_args;
    int n_commas;
    int bad_commas;
    char** args;
    char* token;
    char* next_arg;
    char* arg_input;
    int token_len;

    args = (char**)malloc(sizeof(char*) * MAX_LINE_LEN); /* can't have more args than the max length of line! */

    /* Strip newline character and trim leading and trailing whitespaces */
    line[strcspn(line, "\n")] = 0;
    line = trim(line);

    /* Skip empty lines and comments */
    if (strlen(line) == 0 || line[0] == ';') {
        return NULL;
    }

    if (strlen(line) > MAX_LINE_LEN) {
        printf("Error in line: %i. Line exceeds max length of %i chars\n", line_num, MAX_LINE_LEN);
        n_errors++;
    }

    /* Get first token of line */
    token = strtok(line, " \t");

    /* See if it's a label */
    token_len = strlen(token);
    if (token[token_len - 1] == ':') {
        label = get_substr(token, 0, token_len - 1);

        /* Validate the label */
        if (!_validate_label(label)) {
            return NULL;
        }

        /* Move on to next token of line */
        token = trim(strtok(NULL, " \t"));
    }

    /* A label by itself (with no op or directive) is an error: */
    if (token == NULL) {
        printf("Error in line %i. No op or directive given\n", line_num);
        n_errors++;
        return NULL;
    }

    /* Get args: */
    n_args = 0;
    bad_commas = 0;
    arg_input = trim(strtok(NULL, "")); /* the remaining part of the line are the args */
    n_commas = 0;
    if (arg_input != NULL) {
        /* If we're expecting a string arg or we got a string arg, don't split by commas and spaces
         * (the string itself might contain these chars, which is valid for a string): */
        if (strcmp(token, ".string") == 0 || (arg_input[0]=='"' && arg_input[strlen(arg_input)-1]=='"')) {
            args[n_args++] = arg_input;
        }
        else { /* otherwise, read in comma separated list of args one by one (if any) */
            n_commas = count_char(arg_input, ',');
            bad_commas = !check_comma_formatting(arg_input);
            next_arg = trim(strtok(arg_input, ", \t"));
            while (next_arg != NULL) {
                args[n_args++] = next_arg;
                next_arg = trim(strtok(NULL, ", \t"));
            }
        }
    }

    /* Check the type of the command (directive/op) and validate accordingly: */
    if (token[0] == '.') { /* directive */
        if (!_validate_directive(token, args, n_args, line_num)) {
            return NULL;
        }
        directive = token;
    }
    else { /* op */
        if (!_validate_op(token, args, n_args)) {
            return NULL;
        }
        op = token;
    }

    bad_commas |= (n_args == 0 && n_commas != 0) || (n_args > 0 && n_commas != n_args - 1);
    if (bad_commas) {
        printf("Error in line %i. Bad comma formatting (a SINGLE comma is required BETWEEN each argument)\n", line_num);
        n_errors++;
    }
    return construct_parsed_line(label, op, directive, n_args, args);
}
