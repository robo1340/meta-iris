/**
 * @file print_util.h
 * @author Haines Todd
 * @date April 28, 2021
 * @brief a collection of helpful printer functions
 */

#ifndef print_util_H_
#define print_util_H_

#include <stdio.h>
#include <stdint.h>
//#include <stddef.h>

/** @brief print an array of bytes in hex
*/
void printArrHex(const uint8_t *ptr, int len);

void printArrHex_no_new_line(const uint8_t *ptr, int len);

/** @brief print an array of uint32_t
*/
void print_uint32_arr(const uint32_t *ptr, size_t len);

/** @brief print an array of bytes to a string buffer as hex values and null terminate the buffer
*/
int snprintArrHex(char * str_buf, int str_buf_len, const uint8_t * ptr, int len);

#endif
