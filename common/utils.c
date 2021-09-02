#include"utils.h"
#include<ccan/str/hex/hex.h>
#include<locale.h>
#include<stdlib.h>

const struct chainparams *chainparams;

char *tal_hexstr(const tal_t *ctx, const void *data, size_t len)
{
	char *str = tal_arr(ctx, char, hex_str_size(len));
	hex_encode(data, len, str, hex_str_size(len));
	return str;
}

void setup_locale(void)
{
	setlocale(LC_ALL, "C");
	putenv("LC_ALL=C"); /* For exec{l,lp,v,vp}(...) */
}

const tal_t *tmpctx;

/* Initial creation of tmpctx. */
void setup_tmpctx(void)
{
	tmpctx = tal_arr_label(NULL, char, 0, "tmpctx");
}

/* Free any children of tmpctx. */
void clean_tmpctx(void)
{
	const tal_t *p;

	/* Don't actually free tmpctx: we hand pointers to it around. */
	while ((p = tal_first(tmpctx)) != NULL)
		tal_free(p);
}
