SUBDIRS = ccan
noinst_LTLIBRARIES = libpayz.la

bin_PROGRAMS = payz

payz_SOURCES = \
	main.c

# Sources from lightningd, or stubs of those source files.
COMMON_SOURCES = \
	bitcoin/chainparams.h \
	common/type_to_string.c \
	common/type_to_string.h \
	common/utils.c \
	common/utils.h \
	wire/wire.c \
	wire/wire.h

# Sources for actual payz, which we hope will eventually
# make their way to lightningd.
PAY_SOURCES = \
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
