SUBDIRS = ccan

bin_PROGRAMS = payz

payz_SOURCES = \
	main.c

ACLOCAL_AMFLAGS = -I m4

# We need access to both $(top_srcdir)/ccan (to get 
# the ccan header-only modules) as well as the
# pwd ccan (to get the ccan config.h file).
MY_INCLUDEDIRS = \
	-I $(srcdir)/ccan \
	-I ccan

AM_CFLAGS = -Wall -Werror $(MY_INCLUDEDIRS)
LDADD = ccan/libccan.la

if USE_VALGRIND
LOG_COMPILER = valgrind
AM_LOG_FLAGS = --leak-check=full --error-exitcode=1
AM_CFLAGS += -DUSE_VALGRIND
endif
