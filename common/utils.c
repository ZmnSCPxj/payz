#include"utils.h"
#include<assert.h>
#include<ccan/str/hex/hex.h>
#include<ccan/short_types/short_types.h>
#include<ccan/utf8/utf8.h>
#include<errno.h>
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

void *tal_dup_talarr_(const tal_t *ctx, const tal_t *src TAKES, const char *label)
{
	if (!src) {
		/* Correctly handle TAKES on a NULL `src`.  */
		(void) taken(src);
		return NULL;
	}
	return tal_dup_(ctx, src, 1, tal_bytelen(src), 0, label);
}

void tal_arr_remove_(void *p, size_t elemsize, size_t n)
{
    // p is a pointer-to-pointer for tal_resize.
    char *objp = *(char **)p;
    size_t len = tal_bytelen(objp);
    assert(len % elemsize == 0);
    assert((n + 1) * elemsize <= len);
    memmove(objp + elemsize * n, objp + elemsize * (n+1),
	    len - (elemsize * (n+1)));
    tal_resize((char **)p, len - elemsize);
}


/* Check for valid UTF-8 */
bool utf8_check(const void *vbuf, size_t buflen)
{
	const u8 *buf = vbuf;
	struct utf8_state utf8_state = UTF8_STATE_INIT;
	bool need_more = false;

	for (size_t i = 0; i < buflen; i++) {
		if (!utf8_decode(&utf8_state, buf[i])) {
			need_more = true;
			continue;
		}
		need_more = false;
		if (errno != 0)
			return false;
	}
	return !need_more;
}
