/*
 *
 * This file is based on  of pyA20.
 * spi_lib.c python SPI extension.
 *
 * Copyright (c) 2014 Stefan Mavrodiev @ OLIMEX LTD, <support@olimex.com>
 *
 * pyA20 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * Adpated by Philippe Van Hecke <lemouchon@gmail.com>
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "turbofec/turbo.h"
#include "conv_wrapper.h"

static uint8_t ubits[UNCODED_BIT_LEN+4]; //uncoded bit buffer
static uint8_t cbits[CODED_BIT_LEN_PAD]; //coded bit buffer


/* GSM XCCH */
const struct lte_conv_code gsm_conv_xcch = {
	.n = 2,
	.k = 5,
	.len = 224,
	.gen = { 023, 033 },
};

/* GPRS CS2 */
const struct lte_conv_code gsm_conv_cs2 = {
	.n = 2,
	.k = 5,
	.len = 290,
	.gen = { 023, 033 },
};

/* GPRS CS3 */
const struct lte_conv_code gsm_conv_cs3 = {
	.n = 2,
	.k = 5,
	.len = 334,
	.gen = { 023, 033 },
};

/* GSM RACH */
const struct lte_conv_code gsm_conv_rach = {
	.n = 2,
	.k = 5,
	.len = 14,
	.gen = { 023, 033 },
};

/* GSM SCH */
const struct lte_conv_code gsm_conv_sch = {
	.n = 2,
	.k = 5,
	.len = 35,
	.gen = { 023, 033 },
};

/* GSM TCH-FR */
const struct lte_conv_code gsm_conv_tch_fr = {
	.n = 2,
	.k = 5,
	.len = 185,
	.gen = { 023, 033 },
};

static int tch_hr_puncture[] = {
	  1,   4,   7,  10,  13,  16,  19,  22,  25,  28,  31,  34,
	 37,  40,  43,  46,  49,  52,  55,  58,  61,  64,  67,  70,
	 73,  76,  79,  82,  85,  88,  91,  94,  97, 100, 103, 106,
	109, 112, 115, 118, 121, 124, 127, 130, 133, 136, 139, 142,
	145, 148, 151, 154, 157, 160, 163, 166, 169, 172, 175, 178,
	181, 184, 187, 190, 193, 196, 199, 202, 205, 208, 211, 214,
	217, 220, 223, 226, 229, 232, 235, 238, 241, 244, 247, 250,
	253, 256, 259, 262, 265, 268, 271, 274, 277, 280, 283, 295,
	298, 301, 304, 307, 310, 313,  -1,
};

/* GSM TCH-HR */
const struct lte_conv_code gsm_conv_tch_hr = {
	.n = 3,
	.k = 7,
	.len = 98,
	.gen = { 0133, 0145, 0175 },
	.punc = tch_hr_puncture,
};

static int tch_afs_12_2_puncture[] = {
	321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 363,
	365, 369, 373, 377, 379, 381, 385, 389, 393, 395, 397, 401,
	405, 409, 411, 413, 417, 421, 425, 427, 429, 433, 437, 441,
	443, 445, 449, 453, 457, 459, 461, 465, 469, 473, 475, 477,
	481, 485, 489, 491, 493, 495, 497, 499, 501, 503, 505, 507,
	-1,
};

/* GSM TCH-AFS12.2 */
const struct lte_conv_code gsm_conv_tch_afs_12_2 = {
	.n = 2,
	.k = 5,
	.len = 250,
	.rgen = 023,
	.gen = { 020, 033 },
	.punc = tch_afs_12_2_puncture,
};

static int tch_afs_10_2_puncture[] = {
	  1,   4,   7,  10,  16,  19,  22,  28,  31,  34,  40,  43,
	 46,  52,  55,  58,  64,  67,  70,  76,  79,  82,  88,  91,
	 94, 100, 103, 106, 112, 115, 118, 124, 127, 130, 136, 139,
	142, 148, 151, 154, 160, 163, 166, 172, 175, 178, 184, 187,
	190, 196, 199, 202, 208, 211, 214, 220, 223, 226, 232, 235,
	238, 244, 247, 250, 256, 259, 262, 268, 271, 274, 280, 283,
	286, 292, 295, 298, 304, 307, 310, 316, 319, 322, 325, 328,
	331, 334, 337, 340, 343, 346, 349, 352, 355, 358, 361, 364,
	367, 370, 373, 376, 379, 382, 385, 388, 391, 394, 397, 400,
	403, 406, 409, 412, 415, 418, 421, 424, 427, 430, 433, 436,
	439, 442, 445, 448, 451, 454, 457, 460, 463, 466, 469, 472,
	475, 478, 481, 484, 487, 490, 493, 496, 499, 502, 505, 508,
	511, 514, 517, 520, 523, 526, 529, 532, 535, 538, 541, 544,
	547, 550, 553, 556, 559, 562, 565, 568, 571, 574, 577, 580,
	583, 586, 589, 592, 595, 598, 601, 604, 607, 609, 610, 613,
	616, 619, 621, 622, 625, 627, 628, 631, 633, 634, 636, 637,
	639, 640,  -1,
};

/* GSM TCH-AFS10.2 */
const struct lte_conv_code gsm_conv_tch_afs_10_2 = {
	.n = 3,
	.k = 5,
	.len = 210,
	.rgen = 037,
	.gen = { 033, 025, 020 },
	.punc = tch_afs_10_2_puncture,
};

static int tch_afs_7_95_puncture[] = {
	  1,   2,   4,   5,   8,  22,  70, 118, 166, 214, 262, 310,
	317, 319, 325, 332, 334, 341, 343, 349, 356, 358, 365, 367,
	373, 380, 382, 385, 389, 391, 397, 404, 406, 409, 413, 415,
	421, 428, 430, 433, 437, 439, 445, 452, 454, 457, 461, 463,
	469, 476, 478, 481, 485, 487, 490, 493, 500, 502, 503, 505,
	506, 508, 509, 511, 512,  -1,
};

/* GSM TCH-AFS7.95 */
const struct lte_conv_code gsm_conv_tch_afs_7_95 = {
	.n = 3,
	.k = 7,
	.len = 165,
	.rgen = 033,
	.gen = { 0100, 0145, 0175 },
	.punc = tch_afs_7_95_puncture,
};

static int tch_afs_7_4_puncture[] = {
	  0, 355, 361, 367, 373, 379, 385, 391, 397, 403, 409, 415,
	421, 427, 433, 439, 445, 451, 457, 460, 463, 466, 468, 469,
	471, 472,  -1,
};

/* GSM TCH-AFS7.4 */
const struct lte_conv_code gsm_conv_tch_afs_7_4 = {
	.n = 3,
	.k = 5,
	.len = 154,
	.rgen = 037,
	.gen = { 033, 025, 020 },
	.punc = tch_afs_7_4_puncture,
};

static int tch_afs_6_7_puncture[] = {
	  1,   3,   7,  11,  15,  27,  39,  55,  67,  79,  95, 107,
	119, 135, 147, 159, 175, 187, 199, 215, 227, 239, 255, 267,
	279, 287, 291, 295, 299, 303, 307, 311, 315, 319, 323, 327,
	331, 335, 339, 343, 347, 351, 355, 359, 363, 367, 369, 371,
	375, 377, 379, 383, 385, 387, 391, 393, 395, 399, 401, 403,
	407, 409, 411, 415, 417, 419, 423, 425, 427, 431, 433, 435,
	439, 441, 443, 447, 449, 451, 455, 457, 459, 463, 465, 467,
	471, 473, 475, 479, 481, 483, 487, 489, 491, 495, 497, 499,
	503, 505, 507, 511, 513, 515, 519, 521, 523, 527, 529, 531,
	535, 537, 539, 543, 545, 547, 549, 551, 553, 555, 557, 559,
	561, 563, 565, 567, 569, 571, 573, 575,
	-1,
};

/* GSM TCH-AFS6.7 */
const struct lte_conv_code gsm_conv_tch_afs_6_7 = {
	.n = 4,
	.k = 5,
	.len = 140,
	.rgen = 037,
	.gen = { 033, 025, 020, 020 },
	.punc = tch_afs_6_7_puncture,
};

static int tch_afs_5_9_puncture[] = {
	  0,   1,   3,   5,   7,  11,  15,  31,  47,  63,  79,  95,
	111, 127, 143, 159, 175, 191, 207, 223, 239, 255, 271, 287,
	303, 319, 327, 331, 335, 343, 347, 351, 359, 363, 367, 375,
	379, 383, 391, 395, 399, 407, 411, 415, 423, 427, 431, 439,
	443, 447, 455, 459, 463, 467, 471, 475, 479, 483, 487, 491,
	495, 499, 503, 507, 509, 511, 512, 513, 515, 516, 517, 519,
	-1,
};

/* GSM TCH-AFS5.9 */
const struct lte_conv_code gsm_conv_tch_afs_5_9 = {
	.n = 4,
	.k = 7,
	.len = 124,
	.rgen = 0175,
	.gen = { 0133, 0145, 0100, 0100 },
	.punc = tch_afs_5_9_puncture,
};

static int tch_ahs_7_95_puncture[] = {
	  1,   3,   5,   7,  11,  15,  19,  23,  27,  31,  35,  43,
	 47,  51,  55,  59,  63,  67,  71,  79,  83,  87,  91,  95,
	 99, 103, 107, 115, 119, 123, 127, 131, 135, 139, 143, 151,
	155, 159, 163, 167, 171, 175, 177, 179, 183, 185, 187, 191,
	193, 195, 197, 199, 203, 205, 207, 211, 213, 215, 219, 221,
	223, 227, 229, 231, 233, 235, 239, 241, 243, 247, 249, 251,
	255, 257, 259, 261, 263, 265,
	-1,
};

/* GSM TCH-AHS7.95 */
const struct lte_conv_code gsm_conv_tch_ahs_7_95 = {
	.n = 2,
	.k = 5,
	.len = 129,
	.rgen = 023,
	.gen = { 020, 033 },
	.punc = tch_ahs_7_95_puncture,
};

static int tch_ahs_7_4_puncture[] = {
	  1,   3,   7,  11,  19,  23,  27,  35,  39,  43,  51,  55,
	 59,  67,  71,  75,  83,  87,  91,  99, 103, 107, 115, 119,
	123, 131, 135, 139, 143, 147, 151, 155, 159, 163, 167, 171,
	175, 179, 183, 187, 191, 195, 199, 203, 207, 211, 215, 219,
	221, 223, 227, 229, 231, 235, 237, 239, 243, 245, 247, 251,
	253, 255, 257, 259,  -1,
};

/* GSM TCH-AHS7.4 */
const struct lte_conv_code gsm_conv_tch_ahs_7_4 = {
	.n = 2,
	.k = 5,
	.len = 126,
	.rgen = 023,
	.gen = { 020, 033 },
	.punc = tch_ahs_7_4_puncture,
};

static int tch_ahs_6_7_puncture[] = {
	  1,   3,   9,  19,  29,  39,  49,  59,  69,  79,  89,  99,
	109, 119, 129, 139, 149, 159, 167, 169, 177, 179, 187, 189,
	197, 199, 203, 207, 209, 213, 217, 219, 223, 227, 229, 231,
	233, 235, 237, 239,  -1,
};

/* GSM TCH-AHS6.7 */
const struct lte_conv_code gsm_conv_tch_ahs_6_7 = {
	.n = 2,
	.k = 5,
	.len = 116,
	.rgen = 023,
	.gen = { 020, 033 },
	.punc = tch_ahs_6_7_puncture,
};

static int tch_ahs_5_9_puncture[] = {
	  1,  15,  71, 127, 139, 151, 163, 175, 187, 195, 203, 211,
	215, 219, 221, 223,  -1,
};

/* GSM TCH-AHS5.9 */
const struct lte_conv_code gsm_conv_tch_ahs_5_9 = {
	.n = 2,
	.k = 5,
	.len = 108,
	.rgen = 023,
	.gen = { 020, 033 },
	.punc = tch_ahs_5_9_puncture,
};

static int tch_ahs_5_15_puncture[] = {
	  0,   1,   3,   4,   6,   9,  12,  15,  18,  21,  27,  33,
	 39,  45,  51,  54, 57,  63,  69,  75,  81,  87,  90,  93,
	 99, 105, 111, 117, 123, 126, 129, 135, 141, 147, 153, 159,
	162, 165, 168, 171, 174, 177, 180, 183, 186, 189, 192, 195,
	198, 201, 204, 207, 210, 213, 216, 219, 222, 225, 228, 231,
	234, 237, 240, 243, 244, 246, 249, 252, 255, 256, 258, 261,
	264, 267, 268, 270, 273, 276, 279, 280, 282, 285, 288, 289,
	291, 294, 295, 297, 298, 300, 301,  -1,
};

/* GSM TCH-AHS5.15 */
const struct lte_conv_code gsm_conv_tch_ahs_5_15 = {
	.n = 3,
	.k = 5,
	.len = 97,
	.rgen = 037,
	.gen = { 033, 025, 020 },
	.punc = tch_ahs_5_15_puncture,
};

static int tch_ahs_4_75_puncture[] = {
	  1,   2,   4,   5,   7,   8,  10,  13,  16,  22,  28,  34,
	 40,  46,  52,  58, 64,  70,  76,  82,  88,  94, 100, 106,
	112, 118, 124, 130, 136, 142, 148, 151, 154, 160, 163, 166,
	172, 175, 178, 184, 187, 190, 196, 199, 202, 208, 211, 214,
	220, 223, 226, 232, 235, 238, 241, 244, 247, 250, 253, 256,
	259, 262, 265, 268, 271, 274, 275, 277, 278, 280, 281, 283,
	284,  -1,
};

/* GSM TCH-AHS4.75 */
const struct lte_conv_code gsm_conv_tch_ahs_4_75 = {
	.n = 3,
	.k = 7,
	.len = 89,
	.rgen = 0133,
	.gen = { 0100, 0145, 0175 },
	.punc = tch_ahs_4_75_puncture,
};

/* WiMax FCH */
const struct lte_conv_code wimax_conv_fch = {
	.n = 2,
	.k = 7,
	.len = 48,
	.gen = { 0171, 0133 },
	.term = CONV_TERM_TAIL_BITING,
};

/* LTE PBCH */
const struct lte_conv_code lte_conv_pbch = {
	.n = 3,
	.k = 7,
	.len = 512,
	.gen = { 0133, 0171, 0165 },
	.term = CONV_TERM_TAIL_BITING,
};


const conv_encoder_attributes_t types[] = {
	{
		.name = "GSM xCCH",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_xcch,
		.in_len  = 224,
		.out_len = 456,
	},
	{
		.name = "GPRS CS2",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_cs2,
		.in_len  = 290,
		.out_len = 588,
	},
	{
		.name = "GPRS CS3",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_cs3,
		.in_len  = 334,
		.out_len = 676,
	},
	{
		.name = "GSM RACH",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_rach,
		.in_len  = 14,
		.out_len = 36,
	},
	{
		.name = "GSM SCH",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_sch,
		.in_len  = 35,
		.out_len = 78,
	},
	{
		.name = "GSM TCH/FR",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_tch_fr,
		.in_len  = 185,
		.out_len = 378,
	},
	{
		.name = "GSM TCH/AFS 12.2",
		.spec = "(N=2, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_afs_12_2,
		.in_len  = 250,
		.out_len = 448,
	},
	{
		.name = "GSM TCH/AFS 10.2",
		.spec = "(N=3, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_afs_10_2,
		.in_len  = 210,
		.out_len = 448,
	},
	{
		.name = "GSM TCH/AFS 7.95",
		.spec = "(N=3, K=7, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_afs_7_95,
		.in_len  = 165,
		.out_len = 448,
	},
	{
		.name = "GSM TCH/AFS 7.4",
		.spec = "(N=3, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_afs_7_4,
		.in_len  = 154,
		.out_len = 448,
	},
	{
		.name = "GSM TCH/AFS 6.7",
		.spec = "(N=4, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_afs_6_7,
		.in_len  = 140,
		.out_len = 448,
	},
	{
		.name = "GSM TCH/AFS 5.9",
		.spec = "(N=4, K=7, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_afs_5_9,
		.in_len  = 124,
		.out_len = 448,
	},
	{
		.name = "GSM TCH/AHS 7.95",
		.spec = "(N=2, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_ahs_7_95,
		.in_len  = 129,
		.out_len = 188,
	},
	{
		.name = "GSM TCH/AHS 7.4",
		.spec = "(N=2, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_ahs_7_4,
		.in_len  = 126,
		.out_len = 196,
	},
	{
		.name = "GSM TCH/AHS 6.7",
		.spec = "(N=2, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_ahs_6_7,
		.in_len  = 116,
		.out_len = 200,
	},
	{
		.name = "GSM TCH/AHS 5.9",
		.spec = "(N=2, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_ahs_5_9,
		.in_len  = 108,
		.out_len = 208,
	},
	{
		.name = "GSM TCH/AHS 5.15",
		.spec = "(N=3, K=5, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_ahs_5_15,
		.in_len  = 97,
		.out_len = 212,
	},
	{
		.name = "GSM TCH/AHS 4.75",
		.spec = "(N=3, K=7, recursive, flushed, punctured)",
		.code = &gsm_conv_tch_ahs_4_75,
		.in_len  = 89,
		.out_len = 212,
	},
	{
		.name = "WiMax FCH",
		.spec = "(N=2, K=7, non-recursive, tail-biting, non-punctured)",
		.code = &wimax_conv_fch,
		.in_len  = 48,
		.out_len = 96,
	},
	{
		.name = "LTE PBCH",
		.spec = "(N=3, K=7, non-recursive, tail-biting, non-punctured)",
		.code = &lte_conv_pbch,
		.in_len  = 512,
		.out_len = 1536,
	},
};

/*
struct __attribute__((__packed__)) encoder_DEFINITION {
	uint8_t type;
	conv_encoder_attributes_t attr;
	uint8_t * uncoded_bytes;
	uint32_t uncoded_bytes_len;
	uint8_t * coded_bytes;
	uint32_t coded_bytes_len;
};
*/

encoder_t * encoder(uint8_t type){
	if (type >= sizeof(types)/sizeof(conv_encoder_attributes_t)){
		printf("ERROR: encode type at index %u does not exist\n", type);
		return NULL;
	}
	
	encoder_t * to_return = (encoder_t*)malloc(sizeof(encoder_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->type = type;
	memcpy(&to_return->attr, &types[type], sizeof(conv_encoder_attributes_t));
	
	to_return->uncoded_bytes_len = to_return->attr.in_len/8  + ((to_return->attr.in_len%8) ? 1 : 0);
	to_return->coded_bytes_len   = to_return->attr.out_len/8 + ((to_return->attr.out_len%8) ? 1 : 0);
	
	to_return->uncoded_bits_len_padded = to_return->attr.in_len  + ((to_return->attr.in_len%8 != 0) ? (8-to_return->attr.in_len%8):0);
	to_return->coded_bits_len_padded = to_return->attr.out_len  + ((to_return->attr.out_len%8 != 0) ? (8-to_return->attr.out_len%8):0);
	
	//printf("%d->%d : %u->%u\n", to_return->attr.in_len, to_return->attr.out_len, to_return->uncoded_bits_len_padded, to_return->coded_bits_len_padded);
	
	to_return->uncoded_bytes = malloc(to_return->uncoded_bytes_len);
	to_return->coded_bytes = malloc(to_return->coded_bytes_len);
	if ((to_return->uncoded_bytes == NULL) || (to_return->coded_bytes == NULL)) {return NULL;}
	
	memset(to_return->uncoded_bytes, 0, to_return->uncoded_bytes_len);
	memset(to_return->coded_bytes, 0, to_return->coded_bytes_len);
	
	return to_return;
}

int encoder_bits_in_padded(encoder_t * encoder){
	return encoder->uncoded_bits_len_padded;
}

int encoder_bits_out_padded(encoder_t * encoder){
	return encoder->coded_bits_len_padded;
}

extern uint8_t ubits[]; //uncoded bit buffer
extern uint8_t cbits[]; //coded bit buffer

uint8_t * encoder_encode(encoder_t * encoder, uint8_t * input, uint32_t input_len){
	int rc;
	
	if (input_len != encoder->uncoded_bytes_len){
		printf("WARNING: encoder_encode(%u) expected input length of %u\n", encoder->type, encoder->uncoded_bytes_len);
		return NULL;
	}	
	
	rc = bytes2bits(input, input_len, ubits, encoder_bits_in_padded(encoder)); //bit-lify the input byte array and store in ubits
	if (rc != encoder_bits_in_padded(encoder)) {
		printf("bytes2bits failed rc=%d, expected %d\n", rc, encoder_bits_in_padded(encoder));
		return NULL;
	}

	rc = lte_conv_encode(encoder->attr.code, ubits, cbits);
	if (rc != encoder->attr.out_len) {
		printf("ERROR encoder_encode() failed encoding length check %d\n", rc);
		return NULL;
	}
	
	rc = bits2bytes(cbits, encoder->attr.out_len, encoder->coded_bytes, encoder->coded_bytes_len); //debit-lify the input byte array and store in output
	if (rc != (int)encoder->coded_bytes_len) {
		printf("bits2bytes failed rc=%d, expected %d\n", rc, encoder->coded_bytes_len);
		return NULL;
	}
	return encoder->coded_bytes;	
}

uint8_t * encoder_decode(encoder_t * encoder, uint8_t * input, uint32_t input_len){
	int rc;
	if (input_len != encoder->coded_bytes_len){
		printf("WARNING: encoder_decode(%u) expected input length of %u\n", encoder->type, encoder->coded_bytes_len);
		return NULL;
	}	
	
	rc = bytes2bits_ranged(input, input_len, (int8_t*)cbits, encoder_bits_out_padded(encoder));
	if (rc != encoder_bits_out_padded(encoder)) {
		printf("bytes2bits_ranged failed rc=%d, expected %d\n", rc, encoder_bits_out_padded(encoder));
		return NULL;
	}
	
	lte_conv_decode(encoder->attr.code, (int8_t*)cbits, ubits);


	rc = bits2bytes(ubits, encoder_bits_in_padded(encoder), encoder->uncoded_bytes, encoder->uncoded_bytes_len);
	if (rc != (int)encoder->uncoded_bytes_len) {
		printf("bits2bytes failed rc=%d, expected %d\n", rc, encoder->uncoded_bytes_len);
		return NULL;
	}

	return encoder->uncoded_bytes;
}

void encoder_destroy(encoder_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying encoder_t at 0x%p\n", (void*)*to_destroy);
#endif
	if ((*to_destroy)->uncoded_bytes != NULL){
		free((*to_destroy)->uncoded_bytes);
	}
	if ((*to_destroy)->coded_bytes != NULL){
		free((*to_destroy)->coded_bytes);
	}
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}