#ifndef PAYZ_COMMON_PSEUDORAND_H
#define PAYZ_COMMON_PSEUDORAND_H
#include"config.h"
#include<stdint.h>

/* Shim to get common/channel_id.c and common/route.c
 * compiling.
 * The only thing that needs the pseudorand functions is
 * temporary_channel_id, which a pay algorithm has no
 * need of.
 */
#define pseudorand(x) ((uint64_t) 0)

#endif /* PAYZ_COMMON_PSEUDORAND_H */
