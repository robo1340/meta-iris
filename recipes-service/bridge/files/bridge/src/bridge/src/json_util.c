
#include "json_util.h"

#include "jsmn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

bool read_json_str(char * path, char * dst, size_t dst_len){
	char buf[256];
	FILE * f = fopen(path,"r");
	if (f==NULL) {return false;}
	int num_toks, rc;
	if (f == NULL){return false;}
	if (fgets(buf, sizeof(buf), f) == NULL){
		fclose(f);
		return false;
	} else {fclose(f);}
	
	jsmn_parser p;
	jsmntok_t tokens[1]; 
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	num_toks = jsmn_parse(&p, buf, sizeof(buf), tokens, sizeof(tokens)/sizeof(jsmntok_t));
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){printf("bad token, JSON string is corrupted\n");}
		else if (num_toks == JSMN_ERROR_NOMEM){printf("not enough tokens, JSON string is too large\n");}
		else if (num_toks == JSMN_ERROR_PART){printf("JSON string is too short, expecting more JSON data\n");}
	    return false;
	} else { //start parsing the tokens
		if (tokens[0].type != JSMN_STRING ) {return false;}
		rc = snprintf(dst, dst_len,  "%.*s",tokens[0].end-tokens[0].start, buf+tokens[0].start);
		if ((rc<0)||(rc>(int)dst_len)){return false;}
		
	}
	return true;
}

bool write_json_str(char * path, char * to_write){
	char buf[256]; //arbitrarily limit the maximum input string length to 200
	char path_to_file[200]; //path without the file name
	strncpy (path_to_file, path, sizeof(path_to_file));
	int i;
	int len = strnlen(path_to_file, sizeof(path_to_file)); 
	assert(path_to_file[len] == '\0');
	
	for (i=len; i>0; i--){
		if (path_to_file[i] == '/') { //beginning of file name encountered
			path_to_file[i] = '\0';	//null terminated the string
			break;
		} 
	}
	
	//char *strtok(char *str, const char *delims);
	//need to strip the filename off the end of the path
	snprintf(buf, sizeof(buf), "mkdir -p %s", path_to_file);
	system(buf); //create the directory if it doesn't exist

	FILE * f = fopen(path,"w+");
	if (f==NULL) {return false;}
	
	fputc('\"',f);
	fputs(to_write, f);
	fputc('\"',f);
	fputc('\n',f);
	fclose(f);
	return true;
}

char * jsmn_json_lookup(void * tokens, int num_toks, char * json_str, char * input_key){
	if (tokens==NULL) {return NULL;}
	#define toks ((jsmntok_t*)tokens)
	static char val[32];
	char key[32];
	int i,j;
	
	for(i=1,j=2; j<num_toks; i+=2, j+=2){ //iterate over the key-val pairs in json_str
		snprintf(key, sizeof(key),  "%.*s",toks[i].end-toks[i].start, json_str+toks[i].start);
		snprintf(val, sizeof(val),"%.*s",toks[j].end-toks[j].start, json_str+toks[j].start);
		//printf("%s %s\n", key, val);
		if (strncmp(key, input_key, sizeof(key)) == 0){
			return val;
		}
	}
	return NULL;
}

void * json_load(char * json_str, size_t json_str_len, int * num_toks){
	jsmn_parser p;
	static jsmntok_t tokens[100];
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	*num_toks = jsmn_parse(&p, json_str, json_str_len, tokens, sizeof(tokens)/sizeof(jsmntok_t));
	
	if (*num_toks < 0){
		if (*num_toks == JSMN_ERROR_INVAL){printf("ERROR: bad token, JSON string is corrupted\n");}
		else if (*num_toks == JSMN_ERROR_NOMEM){printf("ERROR: not enough tokens, JSON string is too large\n");}
		else if (*num_toks == JSMN_ERROR_PART){printf("ERROR: JSON string is too short, expecting more JSON data\n");}
		return NULL;
	} else {
		return (void*)tokens;
	}	
}

char * json_lookup(char * json_str, size_t json_str_len, char * input_key){
	jsmn_parser p;
	jsmntok_t tokens[10]; 
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, json_str, json_str_len, tokens, sizeof(tokens)/sizeof(jsmntok_t));
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){printf("bad token, JSON string is corrupted\n");}
		else if (num_toks == JSMN_ERROR_NOMEM){printf("not enough tokens, JSON string is too large\n");}
		else if (num_toks == JSMN_ERROR_PART){printf("JSON string is too short, expecting more JSON data\n");}
	    return NULL;
	} 
	else if (tokens[0].type != JSMN_OBJECT ) {return NULL;}
	else if (num_toks < 2) {
		printf("not enough tokens found in json object!"); 
		return NULL;
	}
	else {
		return jsmn_json_lookup(tokens, num_toks, json_str, input_key);
	}
}
