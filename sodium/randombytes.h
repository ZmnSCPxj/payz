#ifndef PAYZ_SODIUM_RANDOMBYTES_H
#define PAYZ_SODIUM_RANDOMBYTES_H
#include"config.h"
#include<basicsecure.h>

/* Shim to get common/wireaddr.c compiling.
 * common/wireaddr.c does not actually use any functions or
 * definitions from this file.
 * Shim to use basicsecure, which is a much lighter library
 * than sodium.
 */

#define randombytes_buf(p, size) BASICSECURE_RAND(p, size)

#endif /* PAYZ_SODIUM_RANDOMBYTES_H */
