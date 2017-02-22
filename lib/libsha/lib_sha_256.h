/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:  Taken from https://github.com/B-Con/crypto-algorithms/blob/master/sha256.h
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*             Modified a little for PeoplePower needs by GSG ar 19-Feb-2017
*********************************************************************/

#ifndef PRESTO_LIB_SHA256_H
#define PRESTO_LIB_SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/****************************** MACROS ******************************/
#define LIB_SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef uint8_t     LIB_SHA_BYTE;             // 8-bit byte
typedef uint32_t    LIB_SHA_WORD;             // 32-bit word, change to "long" for 16-bit machines



//Return 1 if OK
//c_sum supposed to be hex in string presentation with two-bytes per byte (i.e 64 symbols for 32 bytes)
int lib_sha_file_compare(const char* c_sum, FILE* binay_opened_file);
int lib_sha_string_compare(LIB_SHA_BYTE* c_sum, const char* str);

#endif   // PRESTO_LIB_SHA256_H