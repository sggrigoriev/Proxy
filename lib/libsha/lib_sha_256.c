/*********************************************************************
* Filename:   sha256.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:  Taken from https://github.com/B-Con/crypto-algorithms/blob/master/sha256.c
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA-256 hashing algorithm.
              SHA-256 is one of the three algorithms in the SHA2
              specification. The others, SHA-384 and SHA-512, are not
              offered in this implementation.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
*             Modified a little for PeoplePower needs by GSG ar 19-Feb-2017
*********************************************************************/

/*************************** HEADER FILES ***************************/
#include <stdlib.h>
#include <memory.h>

#include "lib_sha_256.h"

/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))
//
//Local types definition
typedef struct {
    LIB_SHA_BYTE data[64];
    LIB_SHA_WORD datalen;
    unsigned long long bitlen;
    LIB_SHA_WORD state[8];
} SHA256_CTX;


/**************************** VARIABLES *****************************/
static const LIB_SHA_WORD k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
///////////////////////////////////////////////////////////////////////////////////////
//Static functions definition
static void sha256_transform(SHA256_CTX *ctx, const LIB_SHA_BYTE data[]);
static void sha256_init(SHA256_CTX *ctx);
static void sha256_update(SHA256_CTX *ctx, const LIB_SHA_BYTE data[], size_t len);
static void sha256_final(SHA256_CTX *ctx, LIB_SHA_BYTE hash[]);
//
static void hex_string_2_hex_bytes(LIB_SHA_BYTE dest[], const char* src);

/*********************** FUNCTION DEFINITIONS ***********************/

//Calculates the check sum for the file and compares it with c_sum
//Return 1 if OK
//0 if not compared
//c_sum supposed to be hex in string presentation with two-bytes per byte (i.e 64 symbols for 32 bytes)
int lib_sha_file_compare(const char* c_sum, FILE* binary_opened_file) {
    const long blk_size = 4096;
    LIB_SHA_BYTE rd_buf[blk_size];

    LIB_SHA_BYTE buf[LIB_SHA256_BLOCK_SIZE];
    LIB_SHA_BYTE cmp_buf[LIB_SHA256_BLOCK_SIZE];
    SHA256_CTX ctx;

//Check the hash lenght
    if(!c_sum || (strlen(c_sum) != LIB_SHA256_BLOCK_SIZE * 2)) return 0;
// Convert char presentation to the byte one
    hex_string_2_hex_bytes(cmp_buf, c_sum);
//Calculate file length
    if(fseek(binary_opened_file, 0, SEEK_END)) return 0; // seek to end of file
    long size = ftell(binary_opened_file); // get current file pointer
    if(size < 0) return 0;
    if(fseek(binary_opened_file, 0, SEEK_SET)) return 0;

//Calculate blockks size and reminder
    ldiv_t sz = ldiv(size, blk_size);

//Run the check sum clculation
    sha256_init(&ctx);
    while(sz.quot--) {
        fread(rd_buf, sizeof(LIB_SHA_BYTE), blk_size, binary_opened_file);
        sha256_update(&ctx, rd_buf, blk_size);
    }
    if(sz.rem) {
        fread(rd_buf, sizeof(LIB_SHA_BYTE), (size_t)sz.rem, binary_opened_file);
        sha256_update(&ctx, rd_buf, (size_t)sz.rem);
    }
    sha256_final(&ctx, buf);

//Compare with hash sent and report the result
    return (memcmp(cmp_buf, buf, LIB_SHA256_BLOCK_SIZE) == 0);
}

int lib_sha_string_compare(LIB_SHA_BYTE* c_sum, const char* str) {
    LIB_SHA_BYTE buf[LIB_SHA256_BLOCK_SIZE];
    SHA256_CTX ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, (LIB_SHA_BYTE*)str, strlen(str));
    sha256_final(&ctx, buf);

    return (memcmp(c_sum, buf, LIB_SHA256_BLOCK_SIZE) == 0);
}
static void sha256_transform(SHA256_CTX *ctx, const LIB_SHA_BYTE data[])
{
    LIB_SHA_WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx)
{
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const LIB_SHA_BYTE data[], size_t len)
{
    LIB_SHA_WORD i;

    for (i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, LIB_SHA_BYTE hash[])
{
    LIB_SHA_WORD i;

    i = ctx->datalen;

    // Pad whatever data is left in the buffer.
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56)
            ctx->data[i++] = 0x00;
    }
    else {
        ctx->data[i++] = 0x80;
        while (i < 64)
            ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    // Append to the padding the total message's length in bits and transform.
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;
    ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;
    sha256_transform(ctx, ctx->data);

    // Since this implementation uses little endian byte ordering and SHA uses big endian,
    // reverse all the bytes when copying the final state to the output hash.
    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
    }
}
//
static void hex_string_2_hex_bytes(LIB_SHA_BYTE dest[], const char* src) {
    unsigned int i;
    for(i = 0; i < 32; i++) {
        sscanf(src+i*2, "%2hhx", &dest[i]);
    }
}