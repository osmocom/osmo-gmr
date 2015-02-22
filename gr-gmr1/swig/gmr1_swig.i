/* -*- c++ -*- */

#define GR_GMR1_API

%include "gnuradio.i"                   // the common stuff

//load generated python docstrings
%include "gmr1_swig_doc.i"


%{
#include "gnuradio/gmr1/burst_to_tagged_stream.h"
#include "gnuradio/gmr1/gsmtap_sink.h"
#include "gnuradio/gmr1/rach_demod.h"
#include "gnuradio/gmr1/rach_detect_fft.h"
#include "gnuradio/gmr1/rach_file_sink.h"
%}

%include "gnuradio/gmr1/burst_to_tagged_stream.h"
GR_SWIG_BLOCK_MAGIC2(gmr1, burst_to_tagged_stream);

%include "gnuradio/gmr1/gsmtap_sink.h"
GR_SWIG_BLOCK_MAGIC2(gmr1, gsmtap_sink);

%include "gnuradio/gmr1/rach_demod.h"
GR_SWIG_BLOCK_MAGIC2(gmr1, rach_demod);

%include "gnuradio/gmr1/rach_detect_fft.h"
GR_SWIG_BLOCK_MAGIC2(gmr1, rach_detect_fft);

%include "gnuradio/gmr1/rach_file_sink.h"
GR_SWIG_BLOCK_MAGIC2(gmr1, rach_file_sink);
