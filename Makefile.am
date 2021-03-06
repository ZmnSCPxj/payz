SUBDIRS = ccan external/basicsecure
noinst_LTLIBRARIES = libpayz.la

bin_PROGRAMS = payz

payz_SOURCES = \
	main.c

# Sources from lightningd, or stubs of those source files.
COMMON_SOURCES = \
	bitcoin/chainparams.c \
	bitcoin/chainparams.h \
	bitcoin/preimage.c \
	bitcoin/preimage.h \
	bitcoin/privkey.c \
	bitcoin/privkey.h \
	bitcoin/psbt.c \
	bitcoin/psbt.h \
	bitcoin/pubkey.h \
	bitcoin/shadouble.c \
	bitcoin/shadouble.h \
	bitcoin/signature.h \
	bitcoin/short_channel_id.c \
	bitcoin/short_channel_id.h \
	bitcoin/tx.c \
	bitcoin/tx.h \
	common/amount.c \
	common/amount.h \
	common/base32.c \
	common/base32.h \
	common/bigsize.c \
	common/bigsize.h \
	common/channel_id.c \
	common/channel_id.h \
	common/configdir.h \
	common/daemon.c \
	common/daemon.h \
	common/dijkstra.c \
	common/dijkstra.h \
	common/errcode.h \
	common/features.c \
	common/features.h \
	common/fp16.c \
	common/fp16.h \
	common/gossip_constants.h \
	common/gossip_rcvd_filter.c \
	common/gossip_rcvd_filter.h \
	common/gossip_store.c \
	common/gossip_store.h \
	common/gossmap.c \
	common/gossmap.h \
	common/json.c \
	common/json.h \
	common/json_command.h \
	common/json_helpers.c \
	common/json_helpers.h \
	common/json_stream.c \
	common/json_stream.h \
	common/json_tok.c \
	common/json_tok.h \
	common/jsonrpc_errors.h \
	common/lease_rates.h \
	common/memleak.h \
	common/node_id.c \
	common/node_id.h \
	common/overflows.h \
	common/param.c \
	common/param.h \
	common/pseudorand.c \
	common/pseudorand.h \
	common/random_select.h \
	common/route.c \
	common/route.h \
	common/status_levels.c \
	common/status_levels.h \
	common/type_to_string.c \
	common/type_to_string.h \
	common/utils.c \
	common/utils.h \
	common/wireaddr.c \
	common/wireaddr.h \
	gossipd/gossip_store_wiregen.c \
	gossipd/gossip_store_wiregen.h \
	external/gheap/gheap.h \
	external/jsmn/jsmn.c \
	external/jsmn/jsmn.h \
	plugins/libplugin.c \
	plugins/libplugin.h \
	sodium/randombytes.h \
	wally_psbt.h \
	wire/peer_wire.h \
	wire/tlvstream.c \
	wire/tlvstream.h \
	wire/wire.c \
	wire/wire.h

EXTRA_DIST = \
	external/gheap/LICENSE \
	external/gheap/README

# Sources for actual payz, which we hope will eventually
# make their way to lightningd.
PAY_SOURCES = \
	plugins/payz/ecs/ec.c \
	plugins/payz/ecs/ec.h \
	plugins/payz/ecs/ecs.c \
	plugins/payz/ecs/ecs.h \
	plugins/payz/ecs/ecsys.c \
	plugins/payz/ecs/ecsys.h \
	plugins/payz/json_equal.c \
	plugins/payz/json_equal.h \
	plugins/payz/main.c \
	plugins/payz/main.h \
	plugins/payz/parsing.c \
	plugins/payz/parsing.h \
	plugins/payz/payecs_code.c \
	plugins/payz/payecs_code.h \
	plugins/payz/payecs_data.c \
	plugins/payz/payecs_data.h \
	plugins/payz/setsystems.c \
	plugins/payz/setsystems.h \
	plugins/payz/systems/defaulter.c \
	plugins/payz/systems/defaulter.h \
	plugins/payz/systems/invoice_amount.c \
	plugins/payz/systems/invoice_amount.h \
	plugins/payz/systems/nonce.c \
	plugins/payz/systems/nonce.h \
	plugins/payz/systems/parse_invoice.c \
	plugins/payz/systems/parse_invoice.h \
	plugins/payz/tester/command.c \
	plugins/payz/tester/command.h \
	plugins/payz/tester/loop.c \
	plugins/payz/tester/loop.h \
	plugins/payz/tester/rpc.c \
	plugins/payz/tester/rpc.h \
	plugins/payz/tester/spawn.c \
	plugins/payz/tester/spawn.h \
	plugins/payz/tester/tester.c \
	plugins/payz/tester/tester.h \
	plugins/payz/top.c \
	plugins/payz/top.h

libpayz_la_SOURCES = \
	$(COMMON_SOURCES) \
	$(PAY_SOURCES)

libpayz_la_LIBADD = \
	ccan/libccan.la \
	external/basicsecure/libbasicsecure.la

ACLOCAL_AMFLAGS = -I m4

# We need access to both $(top_srcdir)/ccan (to get 
# the ccan header-only modules) as well as the
# pwd ccan (to get the ccan config.h file).
MY_INCLUDEDIRS = \
	-I $(srcdir)/ccan \
	-I ccan \
	-I $(srcdir)/external/basicsecure \
	-I $(srcdir)/external/gheap

AM_CFLAGS = -Wall -Werror $(MY_INCLUDEDIRS)
LDADD = libpayz.la

AM_DEFAULT_SOURCE_EXT = .c
TESTS = \
	plugins/payz/tests/test_advance_fail \
	plugins/payz/tests/test_advance_systrace \
	plugins/payz/tests/test_getdefaultsystems \
	plugins/payz/tests/test_setcomponents \
	plugins/payz/tests/test_simple \
	plugins/payz/tests/test_system_defaulter \
	plugins/payz/tests/test_system_invoice_amount \
	plugins/payz/tests/test_system_nonce
check_PROGRAMS = $(TESTS)

if USE_VALGRIND
LOG_COMPILER = valgrind
AM_LOG_FLAGS = --leak-check=full --error-exitcode=1
AM_CFLAGS += -DUSE_VALGRIND
endif
