#include<bitcoin/chainparams.h>
#include<ccan/tal/tal.h>
#include<plugins/payz/main.h>

int payz_main(int argc, char **argv,
	      const char *pay_command,
	      const char *keysend_commnad)
{
	char *owner = tal(NULL, char);

	/* TODO.  */

	tal_free(owner);
	return 0;
}

#if 0
int main(int argc, char **argv)
{
	return payz_main(argc, argv,
			 "pay",
			 "keysend");
}
#endif
