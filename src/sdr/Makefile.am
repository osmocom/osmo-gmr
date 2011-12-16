INCLUDES = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOSDR_CFLAGS) $(FFTW3F_CFLAGS)
AM_LDFLAGS = $(LIBOSMOCORE_LIBS) $(LIBOSMOSDR_LIBS) $(FFTW3F_LIBS)

noinst_LIBRARIES = libgmr1-sdr.a

libgmr1_sdr_a_SOURCES = dkab.c fcch.c nb.c pi4cxpsk.c
