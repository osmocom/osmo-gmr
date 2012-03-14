SUBDIRS = codec l1 rtfwk sdr

AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMODSP_CFLAGS)

bin_PROGRAMS = gmr1_rx gmr1_rx_live gmr1_rach_gen gmr1_gen_mat gmr1_ambe_decode

gmr1_rx_SOURCES = gmr1_rx.c gsmtap.c
gmr1_rx_LDADD = $(top_builddir)/src/l1/libgmr1-l1.a \
		$(top_builddir)/src/sdr/libgmr1-sdr.a \
		$(LIBOSMOCORE_LIBS) $(LIBOSMODSP_LIBS) $(FFTW3F_LIBS)

gmr1_rx_live_SOURCES = gmr1_rx_live.c gsmtap.c
gmr1_rx_live_LDADD = $(top_builddir)/src/rtfwk/libgmr1-rtfwk.a \
		     $(top_builddir)/src/l1/libgmr1-l1.a \
		     $(top_builddir)/src/sdr/libgmr1-sdr.a \
		     $(LIBOSMOCORE_LIBS) $(LIBOSMODSP_LIBS) $(FFTW3F_LIBS)

gmr1_rach_gen_SOURCES = gmr1_rach_gen.c
gmr1_rach_gen_LDADD = $(top_builddir)/src/l1/libgmr1-l1.a \
		      $(top_builddir)/src/sdr/libgmr1-sdr.a \
		      $(LIBOSMOCORE_LIBS) $(LIBOSMODSP_LIBS) $(FFTW3F_LIBS)

gmr1_gen_mat_SOURCES = gmr1_gen_mat.c
gmr1_gen_mat_LDADD = $(top_builddir)/src/l1/libgmr1-l1.a \
		     $(LIBOSMOCORE_LIBS)

gmr1_ambe_decode_SOURCES = gmr1_ambe_decode.c
gmr1_ambe_decode_LDADD = $(top_builddir)/src/codec/libgmr1-codec.a \
			 $(LIBOSMOCORE_LIBS) -lm
