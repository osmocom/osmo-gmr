INCLUDES = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS)
AM_LDFLAGS = $(LIBOSMOCORE_LIBS)

noinst_LIBRARIES = libgmr1-l1.a

libgmr1_l1_a_SOURCES = \
	conv.c crc.c interleave.c punct.c scramb.c \
	bcch.c ccch.c facch3.c
