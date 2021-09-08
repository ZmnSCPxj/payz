#include<bitcoin/chainparams.h>
#include<ccan/tal/tal.h>
#include<ccan/tal/str/str.h>
#include<common/utils.h>
#include<plugins/libplugin.h>
#include<plugins/payz/main.h>
#include<plugins/payz/top.h>

int payz_main(int argc, char **argv,
	      const char *pay_command,
	      const char *keysend_command)
{
	const char *disablempp_option;

	setup_locale();
	setup_payz_top(pay_command, keysend_command);

	if (streq(pay_command, "pay") &&
	    streq(keysend_command, "keysend")) {
		disablempp_option = "disable-mpp";
	} else {
		disablempp_option = tal_fmt(payz_top, "%s-disable-mpp",
					    pay_command);
	}

	plugin_main(argv, &payz_top_init, PLUGIN_STATIC, true,
		    NULL,
		    payz_top->commands, tal_count(payz_top->commands),
		    payz_top->notifications, tal_count(payz_top->notifications),
		    payz_top->hooks, tal_count(payz_top->hooks),
		    payz_top->notif_topics, tal_count(payz_top->notif_topics),
		    plugin_option(disablempp_option, "flag",
				  "Disable multi-part payments.",
				  flag_option, &payz_top->disablempp),
		    NULL);

	shutdown_payz_top();

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
