/*********************************************************************
* Filename:   sha256.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Performs known-answer tests on the corresponding SHA1
	          implementation. These tests do not encompass the full
	          range of available test vectors, however, if the tests
	          pass it is very, very likely that the code is correct
	          and was compiled properly. This code also serves as
	          example usage of the functions.
*             Modified a little for PeoplePower needs by GSG ar 19-Feb-2017
*   NB! file check works with text presentation, so original test#3 will not work.
*********************************************************************/

/*************************** HEADER FILES ***************************/
#include <stdio.h>
#include <memory.h>
#include "lib_sha_256.h"

/*********************** FUNCTION DEFINITIONS ***********************/
int sha256_test()
{
    const char* text1 = "abc";
    const char* text2 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const char* text3 = "aaaaaaaaaa";
    LIB_SHA_BYTE hash1[LIB_SHA256_BLOCK_SIZE] = {0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
                                     0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
    LIB_SHA_BYTE hash2[LIB_SHA256_BLOCK_SIZE] = {0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
                                     0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1};
    LIB_SHA_BYTE hash3[LIB_SHA256_BLOCK_SIZE] = {0xcd,0xc7,0x6e,0x5c,0x99,0x14,0xfb,0x92,0x81,0xa1,0xc7,0xe2,0x84,0xd7,0x3e,0x67,
                                     0xf1,0x80,0x9a,0x48,0xa4,0x97,0x20,0x0e,0x04,0x6d,0x39,0xcc,0xc7,0x11,0x2c,0xd0};
    int idx;
    int pass = 1;

    pass = pass && lib_sha_string_compare(hash1, text1);
    pass = pass && lib_sha_string_compare(hash2, text2);


    FILE* f = fopen("./sha_test", "w+");
    for(idx = 0; idx < 100000; idx++)
        fwrite(text3, strlen(text3), sizeof(char), f);
    fclose(f);
/*
    FILE* f = fopen("./test.zip", "rb");
*/
    f = fopen("./sha_test", "r");

    pass = pass && lib_sha_file_compare((const char*)hash3, f);

    fclose(f);

    return(pass);
}

int main()
{
    printf("SHA-256 tests: %s\n", sha256_test() ? "SUCCEEDED" : "FAILED");

    return(0);
}