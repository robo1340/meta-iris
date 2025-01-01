#pragma once

//#ifndef _RS_UTIL_H
//#define _RS_UTIL_H
//#else

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LEN(x) (sizeof(x) / sizeof(x[0]))

void vector_print(uint32_t *a, uint32_t size);

// allocate new matrix
uint32_t **matrix_new(uint32_t rows, uint32_t cols);

// free allocated matrix
void matrix_free(uint32_t **a, uint32_t rows, uint32_t cols);

// returns pointer to new copy of matrix `a`
uint32_t **matrix_copy(uint32_t **a, uint32_t rows, uint32_t cols);

void fatal(const char *who, const char *msg);

void swap(uint32_t *a, uint32_t *b);

uint32_t *vector_new(uint32_t len);

uint32_t *vector_copy(uint32_t *a, uint32_t len);

void vector_free(uint32_t *a);

// a = [0,1,2]
// b = [4,5,6]
// stack(a,b) = [0,1,2,4,5,6]
// out_size = len(a) + len(b)
uint32_t *stack(uint32_t *a, uint32_t alen, uint32_t *b, uint32_t blen, uint32_t *out_size);

// returns subvector in a, starting from start and ending in end (not including)
uint32_t *submatrix(uint32_t *a, uint32_t start, uint32_t end);

uint32_t pow2(uint32_t q);

uint32_t *vector_sum(uint32_t *a, uint32_t a_len, uint32_t *b, uint32_t b_len, uint32_t *new_len);

uint8_t *vector_to_str(uint32_t *a, uint32_t a_len);

uint32_t *str_to_vector(uint8_t *s, uint32_t s_len);

uint32_t sum(uint32_t *v, uint32_t v_len);

// returns new string = substring[start,end]
uint8_t *substring(uint8_t *s, uint32_t start, uint32_t end, uint32_t *new_size);

// same as strlen
uint32_t strlength(uint8_t *s);

uint32_t count(uint32_t *v, uint32_t len, uint32_t item);

//#endif