/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * Please see the corresponding header file for documentation on the module.
 *
 */

#include <inttypes.h>

#include "uint256.h"
#include "sha256.h"

#include "hashcash.h"


/***** Helper function prototypes
 */

void uint256_init_with_value(BYTE *uint256, uint64_t value);
void concat(BYTE *dst, BYTE *seed, uint64_t nonce);
void sha256twice(BYTE *hash, BYTE *data, int len);
void difficulty_to_target(BYTE *target, uint32_t difficulty);


/***** Public functions
 */

int hashcash_verify(uint32_t difficulty, BYTE *seed, uint64_t solution) {
    BYTE concatenated[40];
    BYTE target[32];
    BYTE hash[32];

    // calculate target
    difficulty_to_target(target, difficulty);

    // concat
    concat(concatenated, seed, solution);
    sha256twice(hash, concatenated, 40);

    // compare
    return -1 == sha256_compare(hash, target);
}


/***** Helper functions
 */

/*
 * Initializes the given uint256 values with the given initial value.
 */
void uint256_init_with_value(BYTE *uint256, uint64_t value) {
    if (uint256 == NULL) {
        return;
    }

    for (int i = 31; i >= 0; i--) {
        uint256[i] = value & 0xff;
        value = value >> 8;
    }
}

/*
 * Helper function to concatenate the given seed and nonce value,
 * and copy the result into the destination array.
 * Assumes dst is a BYTE array with enough space for the concatenation.
 */
void concat(BYTE *dst, BYTE *seed, uint64_t nonce) {
    int i;

    // copy over the seed
    for (i = 0; i < 32; i++) {
        dst[i] = seed[i];
    }

    // copy over the nonce value
    dst += 32;
    for (i = 7; i >= 0; i--) {
        dst[i] = nonce & 0xff;
        nonce = nonce >> 8;
    }
}

/*
 * Hashes the given value using sha256 twice.
 */
void sha256twice(BYTE *hash, BYTE *data, int len) {
    SHA256_CTX ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);

    sha256_init(&ctx);
    sha256_update(&ctx, hash, 32);
    sha256_final(&ctx, hash);
}

/*
 * Converts the given difficulty value into the target value. (32-byte array)
 */
void difficulty_to_target(BYTE *target, uint32_t difficulty) {
    BYTE alpha[32];
    BYTE beta[32];
    BYTE two[32];

    uint256_init_with_value(beta, difficulty & 0xffffff);

    uint256_init_with_value(two, 2);
    uint256_exp(alpha, two, 8 * ((difficulty >> 24) - 3));

    uint256_mul(target, beta, alpha);
}
