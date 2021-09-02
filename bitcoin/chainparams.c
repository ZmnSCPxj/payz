#include"chainparams.h"
#include<ccan/compiler/compiler.h>

static const struct chainparams cp = {
	false, NULL
};

const struct chainparams *chainparams_for_network(const char *network UNUSED)
{
	return &cp;
}
