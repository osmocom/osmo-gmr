SUBDIRS = l1 sdr

INCLUDES = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOSDR_CFLAGS)
AM_LDFLAGS = $(LIBOSMOCORE_LIBS) $(LIBOSMOSDR_LIBS)

bin_PROGRAMS = gmr1_rx

gmr1_rx_SOURCES = gmr1_rx.c gsmtap.c
gmr1_rx_LDADD =	$(top_builddir)/src/l1/libgmr1-l1.a \
		$(top_builddir)/src/sdr/libgmr1-sdr.a \
		$(FFTW3F_LIBS)
