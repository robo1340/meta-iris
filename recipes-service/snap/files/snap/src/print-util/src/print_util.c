/**
 * @file print_util.c
 * @author Haines Todd
 * @date 4/28/2021
 * @brief a collection of helpful printer functions
 */

#include "print_util.h"

void printArrHex(const uint8_t *ptr, int len){
	int i;
	printf("[");
	for (i=0;i<len;i++){
		printf("0x%02x",*ptr); ptr++;
		if (i<(len-1)) {printf(",");}
	}
	printf("]\n");
}

void printArrHex_no_new_line(const uint8_t *ptr, int len){
	int i;
	printf("[");
	for (i=0;i<len;i++){
		printf("0x%02x",*ptr); ptr++;
		if (i<(len-1)) {printf(",");}
	}
	printf("]");
}

void print_uint32_arr(const uint32_t *ptr, size_t len){
	uint32_t i;
	printf("[");
	for (i=0; i<len;i++){
		printf("%4u",ptr[i]);
		if (i<(len-1)) {printf(",");}
	}
	printf("]\n");
}

int snprintArrHex(char * str_buf, int str_buf_len, const uint8_t * ptr, int len){
	int i;
	int to_return=0;
	for (i=0; i<len; i++){
		if ((str_buf_len-to_return) <= 1){
			return -1;
		}
		
		snprintf(str_buf, (str_buf_len-to_return), "%02x", *ptr); ptr++;
		to_return += 2;
		str_buf += 2;
	}
	*str_buf = '\0';
	return to_return;
}
