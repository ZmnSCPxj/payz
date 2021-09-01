SUBDIRS = ccan

bin_PROGRAMS = payz

payz_SOURCES = \
	main.c

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall -Werror
LDADD = ccan/libccan.la

if USE_VALGRIND
LOG_COMPILER = valgrind
AM_LOG_FLAGS = --leak-check=full --error-exitcode=1
AM_CFLAGS += -DUSE_VALGRIND
endif
