SUBDIRS = l1 sdr

INCLUDES = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOSDR_CFLAGS)
AM_LDFLAGS = $(LIBOSMOCORE_LIBS) $(LIBOSMOSDR_LIBS)

bin_PROGRAMS = gmr1_rx gmr1_gen_mat

gmr1_rx_SOURCES = gmr1_rx.c gsmtap.c
gmr1_rx_LDADD =	$(top_builddir)/src/l1/libgmr1-l1.a \
		$(top_builddir)/src/sdr/libgmr1-sdr.a \
		$(FFTW3F_LIBS)

gmr1_gen_mat_SOURCES = gmr1_gen_mat.c
gmr1_gen_mat_LDADD = $(top_builddir)/src/l1/libgmr1-l1.a
