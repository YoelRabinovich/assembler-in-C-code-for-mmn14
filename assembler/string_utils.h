#ifndef STRING_UTILS_H
#define STRING_UTILS_H

/* Trims leading and trailing whitespace from str*/
char * trim(char* str);

/* Counts number of times character c occurs in str*/
int count_char(char* str, char c);

/* Returns whether the str is alphabetic*/
int is_alpha(char* str);

/* Returns whether the str is alphanumeric*/
int is_alnum(char* str);

/* Returns whether the str is a valid string representation of an integer*/
int is_integer(char* str, int start_idx);

/* Converts a (pos/neg) integer string (possibly prefixed by '#' to an int value */
int get_int_value(char * str, int start_idx);

/* Returns whether the str is printable */
int is_printable(char* str);

/* Replace a section of a string with the specified replacement */
char* str_replace(char* str, char* replacement, int start_idx, int end_idx);

/* Copy a str. (remember to free when done) */
char* str_cpy(char* str);

/* Return substring. (remember to free when done) */
char* get_substr(char* str, int start_idx, int end_idx);

#endif
