#ifndef _JSON_UTIL_H
#define _JSON_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

///>read a string from a file containing a string in the form "<contents>"
bool read_json_str(char * path, char * dst, size_t dst_len);

///>write a string to a file in the form "<contents>"
bool write_json_str(char * path, char * to_write);

void * json_load(char * json_str, size_t json_str_len, int * num_toks);

char * jsmn_json_lookup(void * tokens, int num_toks, char * json_str, char * input_key);

///>read a value from a json object given its key
char * json_lookup(char * json_str, size_t json_str_len, char * input_key);

#endif 




