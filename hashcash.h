/*
 * COMP30023 Computer Systems Project 2
 * Ibrahim Athir Saleem (isaleem) (682989)
 *
 * This module provides functions to verify and find the hashcash proof-of-work
 * solutions.
 *
 */

#pragma once

/*
 * Verifies that the given solution is indeed valid for the given seed and
 * target.
 * Returns 1 if it is valid and 0 otherwise.
 */
int hashcash_verify(BYTE *target, BYTE *seed, uint64_t solution);

/*
 * Converts the given difficulty value into the target (a uint256).
 */
void hashcash_calc_target(BYTE *target, uint32_t difficulty);
