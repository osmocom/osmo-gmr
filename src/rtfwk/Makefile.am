AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMODSP_CFLAGS)
AM_LDFLAGS = $(LIBOSMOCORE_LIBS) $(LIBOSMODSP_LIBS) $(FFTW3F_LIBS)

noinst_HEADERS = \
	common.h ringbuf.h sampbuf.h sa_file.h \
	sa_bcch_ccch.h sa_fcch.h sa_tch3.h sa_tch9.h
noinst_LIBRARIES = libgmr1-rtfwk.a

libgmr1_rtfwk_a_SOURCES = \
	common.c ringbuf.c sampbuf.c sa_file.c \
	sa_bcch_ccch.c sa_fcch.c sa_tch3.c sa_tch9.c
