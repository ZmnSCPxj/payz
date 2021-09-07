SUBDIRS = ccan
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
	bitcoin/short_channel_id.c \
	bitcoin/short_channel_id.h \
	bitcoin/tx.h \
	common/amount.c \
	common/amount.h \
	common/base32.c \
	common/base32.h \
	common/channel_id.c \
	common/channel_id.h \
	common/configdir.h \
	common/daemon.c \
	common/daemon.h \
	common/errcode.h \
	common/features.c \
	common/features.h \
	common/gossip_constants.h \
	common/json.c \
	common/json.h \
	common/json_command.h \
	common/json_stream.c \
	common/json_stream.h \
	common/jsonrpc_errors.h \
	common/memleak.h \
	common/node_id.c \
	common/node_id.h \
	common/overflows.h \
	common/peer_wire.h \
	common/pseudorand.h \
	common/status_levels.c \
	common/status_levels.h \
	common/type_to_string.c \
	common/type_to_string.h \
	common/utils.c \
	common/utils.h \
	external/jsmn/jsmn.c \
	external/jsmn/jsmn.h \
	wally_psbt.h \
	wire/wire.c \
	wire/wire.h

# Sources for actual payz, which we hope will eventually
# make their way to lightningd.
PAY_SOURCES = \
	plugins/payz/ecs/ec.c \
	plugins/payz/ecs/ec.h \
	plugins/payz/ecs/ecsys.c \
	plugins/payz/ecs/ecsys.h \
	plugins/payz/main.c \
	plugins/payz/main.h

libpayz_la_SOURCES = \
	$(COMMON_SOURCES) \
	$(PAY_SOURCES)

ACLOCAL_AMFLAGS = -I m4

# We need access to both $(top_srcdir)/ccan (to get 
# the ccan header-only modules) as well as the
# pwd ccan (to get the ccan config.h file).
MY_INCLUDEDIRS = \
	-I $(srcdir)/ccan \
	-I ccan

AM_CFLAGS = -Wall -Werror $(MY_INCLUDEDIRS)
LDADD = libpayz.la ccan/libccan.la

if USE_VALGRIND
LOG_COMPILER = valgrind
AM_LOG_FLAGS = --leak-check=full --error-exitcode=1
AM_CFLAGS += -DUSE_VALGRIND
endif
