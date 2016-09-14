#!/usr/bin/env python

#
# (C) 2011-2019 by Sylvain Munaut <tnt@246tNt.com>
# All Rights Reserved
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Call XInitThreads as the _very_ first thing.
# After some Qt import, it's too late
import ctypes
import sys
if sys.platform.startswith('linux'):
	try:
		x11 = ctypes.cdll.LoadLibrary('libX11.so')
		x11.XInitThreads()
	except:
		print "Warning: failed to XInitThreads()"

# Standard lib imports
import argparse
import math

# Try to import UI
try:
	from PyQt4 import Qt
	from distutils.version import StrictVersion
	import sip
	from gnuradio import fosphor
	QT_AVAILABLE = True

except ImportError:
	QT_AVAILABLE = False

# GNURadio
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.filter import pfb
from gnuradio.fft import window

import osmosdr


# ----------------------------------------------------------------------------
# Utils
# ----------------------------------------------------------------------------

def indent(txt, n=1):
	return '\n'.join(['\t'*n + l for l in txt.splitlines()])


# ----------------------------------------------------------------------------
# Channel description
# ----------------------------------------------------------------------------

class Channel(object):

	BASE_BANDWIDTH = 31.25e3
	BASE_SYMRATE   = 23.4e3

	def __init__(self, arfcn, width=1, uplink=False, band='L'):
		if width not in (1,2,3,5):
			raise ValueError("Invalid channel width")
		if band not in ('L', 'S'):
			raise ValueError("Invalid frequency band")

		if isinstance(arfcn, basestring):
			if arfcn[0] == 'U':
				uplink = True
				arfcn  = arfcn[1:]

			if 'x' in arfcn:
				width = int(arfcn.split('x')[1])
				arfcn = arfcn.split('x')[0]

			arfcn = int(arfcn)

		self._arfcn  = arfcn
		self._width  = width
		self._uplink = uplink
		self.band    = band		# Use setter

	def __repr__(self):
		pfx = 'U' if self._uplink else ''
		sfx = ('x%d' % self._width) if (self._width > 1) else ''
		return '%s%d%s' % (pfx, self._arfcn, sfx)

	@property
	def band(self):
		return self._band

	@band.setter
	def band(self, band):
		if band not in ('L', 'S'):
			raise ValueError("Invalid frequency band")
		self._band = band

		if self._band == 'L':
			self._base_ul = 1626.5e6
			self._base_dl = 1525e6

		elif self._band == 'S':
			self._base_ul = 1980e6 + 15.625e3
			self._base_dl = 2170e6 + 15.625e3

	@property
	def arfcn(self):
		return self._arfcn

	@property
	def arfcns(self):
		return range(self.arfcn - (self.width-1)//2, self.arfcn + (self.width+2)//2)

	@property
	def width(self):
		return self._width

	@property
	def uplink(self):
		return self._uplink

	@property
	def frequency(self):
		base_freq = self._base_ul if self._uplink else self._base_dl
		return base_freq + Channel.BASE_BANDWIDTH * (self._arfcn + 0.5 * ((self._width ^ 1) & 1))

	@property
	def bandwidth(self):
		return Channel.BASE_BANDWIDTH * self._width

	@property
	def symbol_rate(self):
		return Channel.BASE_SYMRATE * self._width

	@property
	def subchannels(self):
		return [
			Channel(sa, 1)
				for sa in range(
					self.arfcn - (self.width-1) // 2,
					self.arfcn + (self.width+2) // 2
				)
		]

	@classmethod
	def align_freq(kls, freq):
		bases = [
			1525e6,				# DL L-band
			1626.5e6,			# UL L-band
			1980e6 + 15.625e3,	# UL S-band
			2170e6 + 15.625e3	# DL S-band
		]

		base_freq = min([(abs(b-freq), b) for b in bases])[1]
		chan = round((freq - base_freq) / kls.BASE_BANDWIDTH)
		return base_freq + chan * kls.BASE_BANDWIDTH


# ----------------------------------------------------------------------------
# Arguments parsing
# ----------------------------------------------------------------------------

class PerChannelArgType(object):

	def __init__(self, type_func):
		self._type_func = type_func

	def __call__(self, val):
		if isinstance(val, basestring) and ('/' in val):
			val = val.split('/')
			return (int(val[0]), self._type_func(val[1]))
		else:
			return (None, self._type_func(val))


def gain_type(val):
	if ':' not in val:
		return (None, float(val))
	else:
		val = val.split(':')
		return (val[0], float(val[1]))


class AttrDictWithFallback(dict):

	def __init__(self, *args, **kwargs):
		self.__dict__['_fallback'] = kwargs.pop('_fallback', {})
		super(AttrDictWithFallback, self).__init__(*args, **kwargs)

	def __getattr__(self, key, *args):
		return self[key] if (key in self) else self._fallback.get(key, *args)

	def __setattr__(self, key, value):
		self[key] = value

	def __delattr__(self, key):
		del self[key]



def args_parse_raw():
	# Create parser
	parser = argparse.ArgumentParser()

	# Global options
	gp = parser.add_argument_group('Global options')

	gp.add_argument(
		"--args",
		dest="args",
		metavar="ARGS",
		default="",
		type=str,
		help="Arguments to the osmosdr source"
	)

	gp.add_argument(
		"-s", "--samp-rate",
		dest="samp_rate",
		metavar="SAMP_RATE",
		type=eng_notation.str_to_num,
		help="Set samp_rate",
		required=True
	)

	gp.add_argument(
		"-a", "--arfcn",
		dest="arfcns",
		metavar="ARFCN",
		type=Channel,
		action="append",
		help="Add an ARFCN to listen to",
		required=True
	)

	gp.add_argument(
		"-B", "--band",
		dest="band",
		metavar="BAND",
		default="L",
		type=str,
		choices=("L", "S"),
		help="Select operating band (L-band / S-band)",
		required=True
	)

	gp.add_argument(
		"-t", "--time",
		dest="time",
		metavar="SEC",
		type=float,
		help="Set the time to record",
	)

	gp.add_argument(
		"-q", "--qt",
		dest="qt",
		action='store_true',
		help="Enable Qt UI",
	)

	gp.add_argument(
		"-o", "--output",
		dest="output",
		metavar="TEMPLATE",
		default="/tmp/arfcn_%s.cfile",
		type=str,
		help="Output filename template",
	)

	gp.add_argument(
		"-p", "--pfb",
		dest="pfb",
		action="store_true",
		help="Use PFB topology instead of independent DDCs",
	)

	# Per input channel options
	pcp = parser.add_argument_group('Per input channel options',
		"Use the 'n/' prefix to specify to which input channel to apply. " +
		"Non prefixed value will apply to all channels")

	pcp.add_argument(
		"-f", "--center-freq",
		dest="center_freq",
		metavar="FREQ",
		type=PerChannelArgType(eng_notation.str_to_num),
		default=[],
		action="append",
		help="Set center_freq",
		required=True
	)

	pcp.add_argument(
		"-g", "--gain",
		dest="gain",
		metavar="GAIN",
		type=PerChannelArgType(gain_type),
		default=[],
		action="append",
		help="Set gain to the osmosdr source"
	)

	pcp.add_argument(
		"--corr",
		dest="corr",
		metavar="PPM",
		type=PerChannelArgType(float),
		default=[],
		action="append",
		help="Set correction factor in PPM"
	)

	pcp.add_argument(
		"-b", "--bw",
		dest="bw",
		metavar="BW_HZ",
		type=PerChannelArgType(eng_notation.str_to_num),
		default=[],
		action="append",
		help="Select the filter bandwidth"
	)

	return parser.parse_args()


def args_parse():
	# Grab raw arguments
	raw = args_parse_raw()

	# Post process
	ga  = ['args', 'samp_rate', 'arfcns', 'band', 'time', 'qt', 'output', 'pfb']
	pca = ['center_freq', 'gain', 'corr', 'bw']

		# Global: Just copy
	gad = AttrDictWithFallback()
	for k in ga:
		gad[k] = getattr(raw, k)

		# Per-Channel: Group in dict
	pcad = { None: AttrDictWithFallback() }
	for k in pca:
		if k == 'gain':
			for ci, v in (getattr(raw, k) or []):
				pcad.setdefault(ci, AttrDictWithFallback(_fallback=pcad[None])).setdefault(k,{})[v[0]] = v[1]
		else:
			for ci, v in (getattr(raw, k) or []):
				pcad.setdefault(ci, AttrDictWithFallback(_fallback=pcad[None]))[k] = v

		# Gain: Transform to dict and handle fallback mix
	if 'gain' in pcad[None]:
		fgs = pcad[None]['gain']
		for k,v in pcad.iteritems():
			if k is None:
				continue
			if 'gain' not in v:
				continue
			for l,w in fgs.iteritems():
				if l not in v['gain']:
					v['gain'][l] = w

		# Qt check
	if gad.qt and not QT_AVAILABLE:
		print "Qt UI not available"
		gad.qt = False

	# Return value
	gad.channel = pcad
	return gad


# ----------------------------------------------------------------------------
# PFB Channelizer mode
# ----------------------------------------------------------------------------

class PFBBase(gr.hier_block2):

	def __init__(self, center_freq, samp_rate, chan_width, chan_align_fn, need_Nx=False, sps=4):
		# Pre-compute params
			# Grid alignement
		mid_center_freq = chan_align_fn(center_freq)

		if abs(mid_center_freq - center_freq) > 200:
			self.rotation = 2.0 * math.pi * (self.center_freq - new_center_freq) / samp_rate
		else:
			self.rotation = 0

			# Save pfb alignement data
		self.pfb_center_freq = mid_center_freq
		self.pfb_chan_width = chan_width

			# Channel count (must be even !)
		self.n_chans = (int(math.ceil(samp_rate / chan_width)) + 1) & ~1

			# Resampling
		self.resamp = (self.n_chans * chan_width) / samp_rate

		if abs(self.resamp - 1.0) < 1e-5:
			self.resamp = 1.0
			mid_samp_rate = samp_rate
		else:
			mid_samp_rate = (math.ceil(self.samp_rate / chan_width) * chan_width)

			# PFB taps
		if need_Nx:
			# Need multiple width channels, so we need a filter supporting perfect reconstruction !
			self.taps = firdes.low_pass_2(
				1.0,
				self.n_chans,
				0.5,
				0.2,
				80,
				firdes.WIN_BLACKMAN_HARRIS
			)
		else:
			# Use a looser filter to reduce CPU
			self.taps = firdes.low_pass(
				1.0,
				mid_samp_rate,
				chan_width * 0.50,
				chan_width * 0.25,
			)

		# Super
		gr.hier_block2.__init__(self,
			"OutputBranch",
			gr.io_signature(1,1,gr.sizeof_gr_complex),
			gr.io_signature(self.n_chans,self.n_chans,gr.sizeof_gr_complex)
		)
		prev = self

		# Pre-rotation
		if self.rotation:
			self.rotator = blocks.rotator_cc(self.rotation)
			self.connect((prev, 0), (self.rotator, 0))
			prev = self.rotator

		# Pre-resampling
		if self.resamp != 1:
			self.resamp = pfb.arb_resampler_ccf(
				self.resamp,
				taps = None,
				flt_size = 32
			)
			self.connect( (prev, 0), (self.resamp, 0) )
			prev = self.resamp

		# Channelizer
		self.channelizer = pfb.channelizer_ccf(
			self.n_chans,
			self.taps,
			2,
			100
		)
		self.connect( (prev, 0), (self.channelizer, 0) )

		# Link all outputs
		for i in range(self.n_chans):
			self.connect( (self.channelizer, i), (self, i) )

	def describe(self):
		return '\n'.join([
			"Channelize pre-rotation : %s" % (("%f rad/sample" % self.rotation) if (self.rotation != 0) else "None"),
			"Channelize pre-resample : %s" % (("%f" % self.resamp) if (self.resamp != 1) else "None"),
			"Channelize # channels   : %d" % self.n_chans,
			"Channelize taps         : %d" % len(self.taps),
		])

	def freq2index(self, freq):
		idx = int(round((freq - self.pfb_center_freq) / self.pfb_chan_width))
		if (idx >= (self.n_chans / 2)) or (idx <= -(self.n_chans / 2)):
			return None
		elif idx < 0:
			idx += self.n_chans
		return idx


class PFBOutputParameters(object):

	OVERSAMPLE = 2	# Each channel rate is in fact oversamples by 2x internally

	def __init__(self, width, chan_width, sym_rate, sps):
		# Save params
		self.width = width

		# Synthesizer (always need even # of channels for 2x oversampling)
		if width > 1:
			self.width_synth = ((width + 1) & ~1)
			self.taps_synth = firdes.low_pass_2(
				1.0,
				self.width_synth,
				0.5,
				0.2,
				80,
				firdes.WIN_BLACKMAN_HARRIS
			)
			self.rotation = - math.pi * ((self.width-1) / (2.0 * self.width_synth))
			chan_rate = chan_width * self.width_synth / self.width

		else:
			self.width_synth = None
			self.taps_synth = None
			self.rotation = 0
			chan_rate = chan_width

		# Resampler
		self.resamp = (sym_rate * sps) / (chan_rate * self.OVERSAMPLE)
		self.taps_resamp = firdes.root_raised_cosine(
			32.0,
			32.0 * chan_rate * self.OVERSAMPLE,
			sym_rate,
			0.35,
			int(11.0 * 32 * chan_rate * self.OVERSAMPLE / sym_rate)
		)

	def describe(self):
		return '\n'.join([
			"Width       : %d" % self.width,
			"Synthesizer : %s" % (("%d chans, %d taps" % (self.width_synth, len(self.taps_synth))) if self.width > 1 else "None"),
			"Resampling  : %f [%d taps, 32 filters]" % (self.resamp, len(self.taps_resamp)),
			"Min Delay   : %f" % (self.min_delay(),),
		])

	def min_delay(self):
		if self.width > 1:
			return (
				(len(self.taps_synth)  / (2.0 * self.width_synth)) +
				(len(self.taps_resamp) / (2.0 * 32.0 * self.width_synth))
			)
		else:
			return (
				len(self.taps_resamp) / (2.0 * 32.0)
			)

	def adjust_delay(self, delay):
		self.delay = delay - self.min_delay()


class PFBOutputBranch(gr.hier_block2):

	def __init__(self, params, filename):
		# Super
		gr.hier_block2.__init__(self,
			"PFBOutputBranch",
			gr.io_signature(params.width,params.width,gr.sizeof_gr_complex),
			gr.io_signature(0,0,0)
		)
		prev = self

		# Synthesizer
		if params.width > 1:
			self.synth = filter.pfb_synthesizer_ccf(
				params.width_synth,
				params.taps_synth,
				True	# 2x oversample
			)
			for i in range(params.width):
				self.connect( (prev, i), (self.synth, i) )
			prev = self.synth

		# Delay
		if params.delay:
			self.delay = blocks.delay(
				gr.sizeof_gr_complex,
				int(round(params.delay * params.width_synth)),
			)
			self.connect( (prev, 0), (self.delay, 0) )
			prev = self.delay

		# Post synth rotation
		if params.rotation != 0:
			self.rotator = blocks.rotator_cc(params.rotation)
			self.connect( (prev, 0), (self.rotator, 0) )
			prev = self.rotator

		# PFB Arb Resampler
		if params.resamp != 1:
			self.resamp = pfb.arb_resampler_ccf(
				params.resamp, params.taps_resamp,
				flt_size = 32
			)
			self.connect( (prev, 0), (self.resamp, 0) )
			prev = self.resamp

		# Output file
		self.sink = blocks.file_sink(gr.sizeof_gr_complex, filename, False)
		self.connect( (prev, 0), (self.sink, 0) )


# ----------------------------------------------------------------------------
# Direct mode
# ----------------------------------------------------------------------------

class DirectOutputParameters(object):

	def __init__(self, samp_rate, sym_rate, sps):
		# Save input rate
		self.samp_rate = samp_rate
		self.sym_rate = sym_rate
		self.sps = sps

		# Select the decimation and resampling ratio
		self._select_decim()

		# Generate the taps
		self._generate_taps()

		# Default is no delay
		self.delay = 0

	def describe(self):
		return '\n'.join([
			"Decimation 1: %d [%d taps]" % (self.decim1, len(self.taps1)),
			"Decimation 2: %d [%d taps]" % (self.decim2, len(self.taps2)),
			"Resampling rate: %f [%d taps, 32 filters]" % (self.resamp, len(self.taps_resamp)),
			"Min Delay : %f\n" % (self.min_delay(),),
		])

	def min_delay(self):
		return (
			(len(self.taps1) / 2.0) +
			(len(self.taps2) / 2.0) * self.decim1 +
			(len(self.taps_resamp) / (2.0 * 32.0)) * (self.decim1 * self.decim2)
		)

	def adjust_delay(self, delay):
		self.delay = delay - self.min_delay()

	def _factor(self, decim):
		d_ideal = int(round(math.sqrt(decim)))
		for i in range(d_ideal, 1, -1):
			if (decim % i) == 0:
				return [ decim // i, i ]
		return [ decim ]

	def _score(self, factors):
		# If single factor, prefer larger
		if len(factors) == 1:
			return factors[0]

		# If two factor, balance larger first decim and 'squareness'
		return (factors[0] * factors[0] * factors[1]) / (1 + (1.0 * factors[0] / factors[1]))

	def _select_decim(self):
		# Handle the 'exact' case
		if (self.samp_rate % (self.sym_rate * self.sps)) == 0:
			decim  = int(self.samp_rate / (self.sym_rate * self.sps))
			factors = self._factor(decim)
			return (factors + [1, 1])[0:3]

		# Min an max total decim
		decim_max = int(math.floor(self.samp_rate / (2 * self.sym_rate)))
		decim_min = int(math.ceil (self.samp_rate / (3 * self.sym_rate)))

		# Factors
		factors = [self._factor(i) for i in range(decim_min, decim_max+1)]

		# Rank them and select best
		factors_best = sorted(factors, key=lambda x: -self._score(x))[0]
		factors_best = (factors_best + [1])[0:2]

		# Resampling factor
		decim = factors_best[0] * factors_best[1]
		resamp = (1.0 * self.sym_rate * self.sps * decim) / self.samp_rate

		# If decim2 is <= 4, merge with resampler
		if factors_best[1] <= 4:
			resamp /= factors_best[1]
			factors_best[1] = 1

		# Store result
		self.decim1 = factors_best[0]
		self.decim2 = factors_best[1]
		self.resamp = resamp

	def _generate_taps(self):
		# Filter taps
		need_rrc = True

			# PFB Arb Resampler
		if self.resamp != 1:
			if need_rrc:
				self.taps_resamp = firdes.root_raised_cosine(
					32.0,
					32.0 * self.samp_rate / (self.decim1 * self.decim2),
					self.sym_rate,
					0.35,
					int(11.0 * 32 * self.samp_rate / (self.decim1 * self.decim2 * self.sym_rate))
				)
				need_rrc = False
			else:
				self.taps_resamp = firdes.low_pass(
					32.0,
					32.0 * self.samp_rate / (self.decim1 * self.decim2),
					self.sym_rate * 1.4 / 2,
					self.sym_rate * 0.1
				)
		else:
			self.taps_resamp = []

			# Decim 2
		if self.decim2 != 1:
			if need_rrc:
				self.taps2 = firdes.root_raised_cosine(
					1.0,
					self.samp_rate / self.decim1,
					self.sym_rate,
					0.35,
					int(11.0 * self.samp_rate / (self.decim1 * self.sym_rate))
				)
				need_rrc = False
			else:
				self.taps2 = firdes.low_pass(
					1.0,
					1.0,
					0.45 / self.decim2,
					0.10 / self.decim2
				)
		else:
			self.taps2 = []

			# Decim 1
		if need_rrc:
			self.taps1 = firdes.root_raised_cosine(
				1.0,
				self.samp_rate,
				self.sym_rate,
				0.35,
				int(11.0 * self.samp_rate / self.sym_rate)
			)
			need_rrc = False
		else:
			self.taps1 = firdes.low_pass(
				1.0,
				1.0,
				0.3 / self.decim1,
				0.3 / self.decim1
			)


class DirectOutputBranch(gr.hier_block2):

	def __init__(self, params, freq, filename):
		# Super
		gr.hier_block2.__init__(self,
			"DirectOutputBranch",
			gr.io_signature(1,1,gr.sizeof_gr_complex),
			gr.io_signature(0,0,0)
		)

		prev = self

		# Delay
		if params.delay:
			self.delay = blocks.delay(
				gr.sizeof_gr_complex,
				int(round(params.delay)),
			)
			self.connect( (prev, 0), (self.delay, 0) )
			prev = self.delay

		# Freq xlating filter
		if params.decim1 > 1:
			self.filt1 = filter.freq_xlating_fir_filter_ccc(
				params.decim1, params.taps1,
				freq, params.samp_rate
			)

			self.connect( (prev, 0), (self.filt1, 0) )
			prev = self.filt1

		# Decimating FIR filter
		if params.decim2 > 1:
			self.filt2 = filter.fir_filter_ccc(
				params.decim2, params.taps2
			)

			self.connect( (prev, 0), (self.filt2, 0) )
			prev = self.filt2

		# PFB Arb Resampler
		if params.resamp != 1:
			self.resamp = pfb.arb_resampler_ccf(
				params.resamp, params.taps_resamp,
				flt_size = 32
			)
			self.connect( (prev, 0), (self.resamp, 0) )
			prev = self.resamp

		# Output file
		self.sink = blocks.file_sink(gr.sizeof_gr_complex, filename, False)
		self.connect( (prev, 0), (self.sink, 0) )


# ----------------------------------------------------------------------------
# Top-Level flowgraph
# ----------------------------------------------------------------------------

class top_block(gr.top_block):

	def __init__(self, config):
		# Super init
		gr.top_block.__init__(self, "GMR-1 L-band RX Top Block")

		# Save config
		self.config = config

		# Setup source
		self._setup_source()

		# Setup GUI base
		if self.config.qt:
			self._setup_qt()

		# ARFCNs sorting & source assignement
		self._arfcn_prepare()

		# Setup Channelizer or Direct topology
		if self.config.pfb:
			self._setup_pfb()
		else:
			self._setup_direct()

	def _setup_qt_channel(self, chan):
		fblk = fosphor.qt_sink_c()
		fblk.set_fft_window(window.WIN_BLACKMAN_hARRIS)
		fblk.set_frequency_range(self.source_freq[chan], self.source_rate)
		fblk_win = sip.wrapinstance(fblk.pyqwidget(), Qt.QWidget)
		self.top_layout.addWidget(fblk_win)
		self.connect( self.source_ep[chan], (fblk, 0) )

	def _setup_qt(self):
		# Qt window setup
		self.widget = Qt.QWidget()
		self.widget.setWindowTitle("GMR-1 L-band RX Top Block")
		self.widget.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))

		self.top_scroll_layout = Qt.QVBoxLayout()
		self.widget.setLayout(self.top_scroll_layout)
		self.top_scroll = Qt.QScrollArea()
		self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
		self.top_scroll_layout.addWidget(self.top_scroll)
		self.top_scroll.setWidgetResizable(True)
		self.top_widget = Qt.QWidget()
		self.top_scroll.setWidget(self.top_widget)
		self.top_layout = Qt.QVBoxLayout(self.top_widget)
		self.top_grid_layout = Qt.QGridLayout()
		self.top_layout.addLayout(self.top_grid_layout)

		# Setup GUI for each channels
		for i in range(self.source_chans):
			self._setup_qt_channel(i)

	def _setup_source_channel(self, chan):
		# Source params
		cp = self.config.channel.get(chan, self.config.channel[None])

		self.source.set_center_freq(cp.center_freq, chan)
		self.source_freq[chan] = self.source.get_center_freq(chan)

		corr = cp.get('corr', None)
		if corr is not None:
			self.source.set_freq_corr(corr, chan)

		bw = cp.get('bw', None)
		if bw is not None:
			self.source.set_bandwidth(bw, chan)

		gain = cp.get('gain', [])
		if gain:
			if None in gain:
				self.source.set_gain(gain.pop(None), chan)
			for gs,gv in gain.iteritems():
				self.source.set_gain(gv, gs, chan)

		# Time limit or direct
		if self.config.time:
			hb = blocks.head(gr.sizeof_gr_complex, int(1.0 * self.source_rate * self.config.time))
			self.connect( (self.source, chan), (hb, 0) )
			self.source_ep[chan] = (hb, 0)
		else:
			self.source_ep[chan] = (self.source, chan)

	def _setup_source(self):
		# Source instance
		self.source = osmosdr.source(args=self.config.args)

		self.source.set_sample_rate(self.config.samp_rate)
		self.source_rate = self.source.get_sample_rate()

		self.source.set_min_output_buffer(int(self.source_rate * 0.01 * gr.sizeof_gr_complex))

		# Tag debug
		if True:
			td = blocks.tag_debug(gr.sizeof_gr_complex, "Source")
			self.connect( (self.source, 0), (td, 0) )

		# Configure channels
		self.source_chans = self.source.get_num_channels()
		self.source_ep = {}
		self.source_freq = {}

		for i in range(self.source_chans):
			self._setup_source_channel(i)

	def _arfcn_prepare(self):
		# Set the band
		for a in self.config.arfcns:
			a.band = self.config.band

		# Accumulate all channels width we need to support
		self.needed_widths = dict([
			(x.width, (x.bandwidth, x.symbol_rate))
				for x in self.config.arfcns
		])

		# Assign ARFCNs to sources
		self.source_arfcns = {}

		for arfcn in self.config.arfcns:
			bs = sorted(range(len(self.source_freq)), key=lambda i:abs(arfcn.frequency - self.source_freq[i]))[0]
			self.source_arfcns.setdefault(bs, []).append(arfcn)

	def _setup_direct(self, sps=4):
		# Compute params
		oparams = {}
		for k in sorted(self.needed_widths.keys()):
			oparams[k] = DirectOutputParameters(
				self.source_rate,
				self.needed_widths[k][1],
				sps
			)
			print "Params for width %dx:" % k
			print indent(oparams[k].describe())
			print ""

		# Adjust the delays to match
		delay = max([x.min_delay() for x in oparams.values()])
		for x in oparams.values():
			x.adjust_delay(delay)

		# Generate all the output branches
		for source_chan, arfcns in self.source_arfcns.iteritems():
			for arfcn in arfcns:
				# Compute frequency offset
				f = arfcn.frequency
				df = f - self.source_freq[source_chan]
				if abs(df) >= (self.source_rate / 2):
					print "ARFCN %s (%sHz) is outside the range\n" % (
						arfcn,
						eng_notation.num_to_str(f)
					)
					continue

				# Debug print
				print "ARFCN %s (abs: %sHz, rel: %sHz)" % (
					arfcn,
					eng_notation.num_to_str(f),
					eng_notation.num_to_str(df)
				)

				# Generate branch and connect it
				b = DirectOutputBranch(
					oparams[arfcn.width],
					df,
					self.config.output % ( arfcn, )
				)

				self.connect( (self.source, source_chan), (b, 0) )

	def _setup_pfb(self, sps=4):
		# Do we need more the 1x channels ?
		need_Nx = self.needed_widths.keys() != [1]

		# Create the base channelization block for each source channel
		self.pfb_base = {}
		for source_chan, freq in sorted(self.source_freq.iteritems()):
			self.pfb_base[source_chan] = PFBBase(
				freq,
				self.source_rate,
				Channel.BASE_BANDWIDTH,
				Channel.align_freq,
				need_Nx
			)

			print "Channelization of source port %d:" % source_chan
			print indent(self.pfb_base[source_chan].describe())
			print ""

			self.connect( (self.source, source_chan), (self.pfb_base[source_chan], 0) )

		# Compute the output branch params for each width
		oparams = {}
		for k in sorted(self.needed_widths.keys()):
			oparams[k] = PFBOutputParameters(
				k,
				self.needed_widths[k][0],
				self.needed_widths[k][1],
				4
			)

			print "Output params for width %dx:" % k
			print indent(oparams[k].describe())
			print ""

		# Adjust the delays to match
		delay = max([x.min_delay() for x in oparams.values()])
		for x in oparams.values():
			x.adjust_delay(delay)

		# Generate all the output branches
		for source_chan, arfcns in self.source_arfcns.iteritems():
			# Need to save used indexes to NULL sink the unused ones
			used_indexes = set()

			# Scan all arfcn
			for arfcn in arfcns:
				# Map this arfcn to a channel list from the PFB base
				pcl = [
					self.pfb_base[source_chan].freq2index(sc.frequency)
						for sc in arfcn.subchannels
				]

				if None in pcl:
					print "ARFCN %s (out-of-range)" % (arfcn,)
					continue

				# Collect indexes
				used_indexes.update(pcl)

				# Debug print
				print "ARFCN %s (abs: %sHz, pfb chans: %r)" % (
					arfcn,
					eng_notation.num_to_str(arfcn.frequency),
					pcl	# FIXME
				)

				# Generate branch and connect it
				b = PFBOutputBranch(
					oparams[arfcn.width],
					self.config.output % ( arfcn, )
				)

				for i, pc in enumerate(pcl):
					self.connect( (self.pfb_base[source_chan], pc), (b, i) )

			# Plug unused channels
			term = blocks.null_sink(gr.sizeof_gr_complex)
			i = 0
			for index in range(self.pfb_base[source_chan].n_chans):
				if index not in used_indexes:
					self.connect( (self.pfb_base[source_chan], index), (term, i) )
					i += 1

	def show(self):
		self.widget.show()


# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

def main():
	# Arguments
	args = args_parse()

	# Qt setup ?
	if args.qt:
		# Qt config
		if(StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0")):
			Qt.QApplication.setGraphicsSystem(gr.prefs().get_string('qtgui','style','raster'))

		# Create app
		qapp = Qt.QApplication(sys.argv)

	# Create top-block
	tb = top_block(config=args)

	# Qt run ...
	if args.qt:
		# Ensure proper shutdown
		def quitting():
			tb.stop()
			tb.wait()

		qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)

		# Run the flow graph & app
		tb.start()
		tb.show()

		# App run
		qapp.exec_()

	# ... or Console run
	else:
		tb.start()
		tb.wait()

	# Force gargage collection, to clean up Qt widgets
	tb = None

	return 0


if __name__ == '__main__':
	sys.exit(main())
