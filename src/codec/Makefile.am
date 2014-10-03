AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS)
AM_LDFLAGS = $(LIBOSMOCORE_LIBS)

noinst_LIBRARIES = libgmr1-codec.a

libgmr1_codec_a_SOURCES = \
	ambe.c codec.c frame.c math.c tables.c tone.c
