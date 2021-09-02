#ifndef PAYZ_BITCOIN_CHAINPARAMS_H
#define PAYZ_BITCOIN_CHAINPARAMS_H
#include<ccan/short_types/short_types.h>
#include<stddef.h>
#include<stdbool.h>

/*
Dummy stub, to get plugins/libplugin.c and
common/amount.c compiling.
*/
struct chainparams {
	/* amount.c refers to these.  */
	bool is_elements;
	const u8 *fee_asset_tag;
};

const struct chainparams *chainparams_for_network(const char *network);

#endif /* PAYZ_BITCOIN_CHAINPARAMS_H */
