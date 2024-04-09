//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <czmq.h>

#include "print_util.h"

#define RADIO_CONFIG_DIR "/bridge/conf/si4463/active/"

zhashx_t * load_radio_configs(char * config_dir_path, zhashx_t * current_config);
zhashx_t * parse_radio_config(char * path, zhashx_t * current_config);

int hexstr_to_arr(char * hex_str, uint8_t * out_buf, size_t buf_len){
	int j;
	char * hex = hex_str;
	char * next;
	
	for (j=0; *hex!='\0';){
		
		if ((*hex==' ')||(*hex==',')){ 
			hex++;
			continue;
		}
		if ((*hex=='\0')||(*hex=='\r')||(*hex=='\n')){
			break;
		}
		
		//expect next characters in the string to be of the form "0x04," or "0x04"
		out_buf[j] = strtoul(hex, &next, 0); //let strtoul figure out that this is a hex string
		hex = next;
		j++;
		if (j>buf_len){
			printf("Error in convert_hex_string_to_array() buffer overflow\n");
			return -1;
		}
	}
	return j;
}

zhashx_t * load_radio_configs(char * config_dir_path, zhashx_t * current_config){
	zhashx_t * to_return = current_config;
	if (current_config == NULL){
		to_return = zhashx_new();
		if (to_return == NULL){return NULL;}
		zhashx_set_key_destructor(to_return, (zhashx_destructor_fn*) zstr_free);
		zhashx_set_key_duplicator(to_return, (zhashx_duplicator_fn*) strdup);
		zhashx_set_destructor(to_return, (zhashx_destructor_fn*)zchunk_destroy);
	}

	zdir_t * config_dir = zdir_new(config_dir_path, NULL);
	if (config_dir == NULL){return NULL;}
	
	printf("INFO load_radio_configs(\"%s\") %u children found\n", zdir_path(config_dir), zdir_count(config_dir));
	
	zlist_t * child_files = zdir_list(config_dir);
	zfile_t * child = zlist_first(child_files);
	while (child != NULL){
		if (zfile_is_directory(child)){
			to_return = load_radio_configs((char*)zfile_filename(child,NULL), to_return);
		} else if (zfile_is_readable(child)){
			to_return = parse_radio_config((char*)zfile_filename(child,NULL), to_return);
		}
		//if (to_return == NULL){return NULL;}
		child = zlist_next(child_files);
	}
	
	zdir_destroy(&config_dir);
	return to_return;
}

zhashx_t * parse_radio_config(char * path, zhashx_t * current_config){
	printf("INFO parse_radio_config(\"%s\")\n", path);
	static char line_buf[512];
	char * line = line_buf;
	FILE * fp;
	size_t len = 0;
	int rc;
	char property[64];
	char value[256];
	uint8_t buf[32];
	zchunk_t * new_prop;
	
	zhashx_t * to_return = current_config;
	if (current_config == NULL){
		to_return = zhashx_new();
		if (to_return == NULL){return NULL;}
		zhashx_set_key_destructor(to_return, (zhashx_destructor_fn*) zstr_free);
		zhashx_set_key_duplicator(to_return, (zhashx_duplicator_fn*) strdup);
		zhashx_set_destructor(to_return, (zhashx_destructor_fn*)zchunk_destroy);
	}

	#define RADIO_CONFIG_REGEX "^\\s*#define\\s+(RF_[^ ]*)\\s+(0x[^\\t\\r\\n]*)"
	zrex_t *rex = zrex_new (RADIO_CONFIG_REGEX);
	if (!zrex_valid (rex)) {return NULL;}
	//#define RF_GLOBAL_XO_TUNE_2 0x11, 0x00, 0x02, 0x00, 0x52, 0x00

	fp = fopen(path, "r");
	if (fp == NULL) {
		zrex_destroy (&rex);
		return NULL;
	}

	while (getline(&line, &len, fp) != -1){
		if (!zrex_matches (rex, line)) {continue;}
		strncpy(property, zrex_hit(rex,1), sizeof(property));
		strncpy(value, zrex_hit(rex,2), sizeof(value));
		
		//printf("\"%s\":\"%s\"\n", property, value);
		
		rc = hexstr_to_arr(value, buf, sizeof(buf));
		if (rc < 0){return NULL;}
		else if (rc > 0) {
			new_prop = zchunk_new(buf, rc);
			if (new_prop == NULL) {return NULL;}
			if (zhashx_lookup(to_return, property) != NULL){
				printf("WARNING: overwriting property \"%s\"\n",property);
			} else {
				zhashx_update(to_return, property, new_prop);
			}
		}
		
		zrex_destroy (&rex);
		rex = zrex_new (RADIO_CONFIG_REGEX);
		
	}
	fclose(fp);
	zrex_destroy (&rex);
	return to_return;
}

int main(void) {
	
	//zhashx_t * config = parse_radio_config("./test_radio_config.h", NULL);
	//assert(config != NULL);
	
	zhashx_t * config = load_radio_configs(RADIO_CONFIG_DIR, NULL);
	assert(config != NULL);
	
	zchunk_t * c = zhashx_first(config);
	while (c != NULL){
		printArrHex(zchunk_data(c), zchunk_size(c));
		c = zhashx_next(config);
	}
	
}
