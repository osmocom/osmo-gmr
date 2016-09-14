#!/usr/bin/env python

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


import argparse
import math
from distutils.version import StrictVersion

try:
	from PyQt4 import Qt
	import sip
	from gnuradio import fosphor
	UI = True

except ImportError:
	UI = False

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.filter import pfb
from gnuradio.fft import window

import osmosdr


SYM_RATE = 23.4e3
CHAN_WIDTH = 31.25e3


class PFBSplitter(gr.hier_block2):

	def __init__(self, n_chans):
		# Super
		gr.hier_block2.__init__(self,
			"OutputBranch",
			gr.io_signature(1,1,gr.sizeof_gr_complex),
			gr.io_signature(0,0,0)
		)


class PFBOutputBranchParameters(object):

	def __init__(self, samp_rate, chan_rate, sps):
		self.decim1 = 1
		self.decim2 = 1
		self.resamp = (chan_rate * sps) / samp_rate

		self.taps_pfb = firdes.root_raised_cosine(
			32.0,
			32.0 * samp_rate,
			chan_rate,
			0.35,
			int(11.0 * 32 * samp_rate / chan_rate)
		)


class DirectOutputBranchParameters(object):

	def __init__(self, samp_rate, chan_rate, sps):
		# Save input rate
		self.samp_rate = samp_rate

		# Select the decimation and resampling ratio
		self.decim1, self.decim2, self.resamp = self._select_decim(samp_rate, chan_rate, sps)

		# Filter taps
		need_rrc = True

			# PFB
		if self.resamp != 1:
			if need_rrc:
				self.taps_pfb = firdes.root_raised_cosine(
					32.0,
					32.0 * samp_rate / (self.decim1 * self.decim2),
					chan_rate,
					0.35,
					int(11.0 * 32 * samp_rate / (self.decim1 * self.decim2 * chan_rate))
				)
				need_rrc = False
			else:
				self.taps_pfb = firdes.low_pass(
					32.0,
					32.0 * samp_rate / (self.decim1 * self.decim2),
					chan_rate * 1.4 / 2,
					chan_rate * 0.1
				)
		else:
			self.taps_pfb = []

			# Decim 2
		if self.decim2 != 1:
			if need_rrc:
				self.taps2 = firdes.root_raised_cosine(
					1.0,
					samp_rate / self.decim1,
					chan_rate,
					0.35,
					int(11.0 * samp_rate / (self.decim1 * chan_rate))
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
				samp_rate,
				chan_rate,
				0.35,
				int(11.0 * samp_rate / chan_rate)
			)
			need_rrc = False
		else:
			self.taps1 = firdes.low_pass(
				1.0,
				1.0,
				0.3 / self.decim1,
				0.3 / self.decim1
			)

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

	def _select_decim(self, samp_rate, chan_rate, sps):
		"""Returns [decim_1, decim_2, resamp]"""

		# Handle the 'exact' case
		if (samp_rate % (chan_rate * sps)) == 0:
			decim  = int(samp_rate / (chan_rate * sps))
			factors = self._factor(decim)
			return (factors + [1, 1])[0:3]

		# Min an max total decim
		decim_max = int(math.floor(samp_rate / (2 * chan_rate)))
		decim_min = int(math.ceil (samp_rate / (3 * chan_rate)))

		# Factors
		factors = [self._factor(i) for i in range(decim_min, decim_max+1)]

		# Rank them and select best
		factors_best = sorted(factors, key=lambda x: -self._score(x))[0]
		factors_best = (factors_best + [1])[0:2]

		# Resampling factor
		decim = factors_best[0] * factors_best[1]
		resamp = (1.0 * chan_rate * sps * decim) / samp_rate

		# If decim2 is <= 4, merge with resampler
		if factors_best[1] <= 4:
			resamp /= factors_best[1]
			factors_best[1] = 1

		return factors_best[0], factors_best[1], resamp


class OutputBranch(gr.hier_block2):

	def __init__(self, params, freq, filename):
		# Super
		gr.hier_block2.__init__(self,
			"OutputBranch",
			gr.io_signature(1,1,gr.sizeof_gr_complex),
			gr.io_signature(0,0,0)
		)

		prev = self

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

		# PFB
		if params.resamp != 1:
			self.resamp = pfb.arb_resampler_ccf(
				params.resamp, params.taps_pfb,
				flt_size = 32
			)
			self.connect( (prev, 0), (self.resamp, 0) )
			prev = self.resamp

		# Output file
		self.sink = blocks.file_sink(gr.sizeof_gr_complex, filename, False)
		self.connect( (prev, 0), (self.sink, 0) )


class top_block(gr.top_block):

	def __init__(self, samp_rate=None, center_freq=None, args="", gain=20.0, corr=0.0, arfcns=[], time=None, bw=None, ui=False):
		# Super-init
		gr.top_block.__init__(self, "GMR-1 L-band RX Top Block")

		# Setup all the GUI
		if ui:
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

		# Source
		self.source = osmosdr.source(args=args)
		self.source.set_sample_rate(samp_rate)
		self.source.set_center_freq(center_freq, 0)
		self.source.set_gain(gain, 0)
		self.source.set_freq_corr(corr, 0)

		if bw:
			self.source.set_bandwidth(bw)

		self.samp_rate = samp_rate = self.source.get_sample_rate()
		self.center_freq = center_freq

		# fosphor
		if ui:
			self.fosphor = fosphor.qt_sink_c()
			self.fosphor.set_fft_window(window.WIN_BLACKMAN_hARRIS)
			self.fosphor.set_frequency_range(center_freq, samp_rate)
			self._fosphor_win = sip.wrapinstance(self.fosphor.pyqwidget(), Qt.QWidget)
			self.top_layout.addWidget(self._fosphor_win)

			self.connect( (self.source, 0), (self.fosphor, 0) )

		# Timelimit
		if time:
			self.true_source = self.source

			self.source = blocks.head(gr.sizeof_gr_complex, int(1.0 * time * samp_rate))
			self.connect( (self.true_source, 0), (self.source, 0) )

		# Outputs
		if len(arfcns) > 10:
			self._init_direct(arfcns)
		else:
			self._init_pfb(arfcns)

	def _init_direct(self, arfcns):
		# Find config for all branches
		oparams = DirectOutputBranchParameters(self.samp_rate, SYM_RATE, 4)
		self.branches = {}

		print "Decimation 1: %d [%d taps]" % (oparams.decim1, len(oparams.taps1))
		print "Decimation 2: %d [%d taps]" % (oparams.decim2, len(oparams.taps2))
		print "Resampling rate: %f [%d taps, 32 filters]" % (oparams.resamp, len(oparams.taps_pfb))

		for arfcn in arfcns:
			f = self._arfcn_to_freq(arfcn)
			df = f - self.center_freq
			if abs(df) > (self.samp_rate / 2):
				print "ARFCN %d (%sHz) is outside the range\n" % (
					arfcn,
					eng_notation.num_to_str(f)
				)
				continue

			print "ARFCN %d (abs: %sHz, rel: %sHz)" % (
				arfcn,
				eng_notation.num_to_str(f),
				eng_notation.num_to_str(df)
			)

			b = OutputBranch(oparams, df, "/tmp/arfcn_%d.cfile" % arfcn)
			self.branches[arfcn] = b

			self.connect( (self.source, 0), (b, 0) )

	def _init_pfb(self, arfcns):
		# Parameters
		arfcn_center = self._freq_to_arfcn(self.center_freq)
		new_center_freq = self._arfcn_to_freq(arfcn_center)

		if abs(new_center_freq - self.center_freq) > 200:
			# FIXME: Check this
			rotation = 2 * math.pi * (self.center_freq - new_center_freq) / self.samp_rate
		else:
			rotation = 0

		n_chans = (int(math.ceil(self.samp_rate / CHAN_WIDTH)) + 1) & ~1
		resamp = (n_chans * CHAN_WIDTH) / self.samp_rate

		if abs(resamp - 1.0) < 1e3:
			resamp = 1.0
			new_samp_rate = self.samp_rate
		else:
			new_samp_rate = (math.ceil(self.samp_rate / CHAN_WIDTH) * CHAN_WIDTH)

		print "Rotation     : %f rad/sample" % rotation
		print "Resampling   : %f" % resamp
		print "PFB Channels : %d" % n_chans

		# Source
		prev = self.source

		# Pre-rotation
		if rotation:
			self.rotator = blocks.rotator_cc(rotation)
			self.connect((prev, 0), (self.rotator, 0))
			prev = self.rotator

		# Pre-resampling
		if resamp != 1:
			self.resamp = pfb.arb_resampler_ccf(
				resamp,
				taps = None,
				flt_size = 32
			)
			self.connect( (prev, 0), (self.resamp, 0) )
			prev = self.resamp


		# Channelizer
		taps = firdes.root_raised_cosine(
			1.0,
			new_samp_rate,
			CHAN_WIDTH,
			0.35,
			int(11.0 * new_samp_rate / CHAN_WIDTH)
		)

		self.channelizer = pfb.channelizer_ccf(
			n_chans,
			taps,
			2,
			100
		)

		self.connect( (prev, 0), (self.channelizer, 0) )
		prev = self.channelizer

		# Output branches
		oparams = PFBOutputBranchParameters(CHAN_WIDTH * 2, SYM_RATE, 4)
		self.branches = {}
		not_used = set(range(n_chans))

		for arfcn in arfcns:
			# Get output index from PFB
			idx = arfcn - arfcn_center

			if (idx >= (n_chans / 2)) or (idx <= -(n_chans / 2)):
				print "ARFCN %d (out-of-range)" % (arfcn,)
				continue
			elif idx < 0:
				idx += n_chans

			print "ARFCN %d (idx=%d)" % (arfcn, idx)

			not_used.remove(idx)

			# Create output branch
			b = OutputBranch(oparams, None, "/tmp/arfcn_%d.cfile" % arfcn)
			self.branches[arfcn] = b

			# Connect
			self.connect( (prev, idx), (b, 0) )

		# Fill missing ones with NULL sinks
		term = blocks.null_sink(gr.sizeof_gr_complex)

		for i, idx in enumerate(not_used):
			self.connect( (prev, idx), (term, i) )

	def _arfcn_to_freq(self, arfcn):
		# FIXME: for multi range channels, the involved arfcns
		# f = lambda c,n: range(c - (n-1)//2, c + (n+2)//2)
		return 1525e6 + CHAN_WIDTH * arfcn

	def _freq_to_arfcn(self, freq):
		return int(round((freq - 1525e6) / CHAN_WIDTH))

	def show(self):
		self.widget.show()


if __name__ == '__main__':
	# Parse options
	parser = argparse.ArgumentParser()

	parser.add_argument(
		"-s", "--samp-rate",
		dest="samp_rate",
		metavar="SAMP_RATE",
		type=eng_notation.str_to_num,
		help="Set samp_rate",
		required=True
	)
	parser.add_argument(
		"-f", "--center-freq",
		dest="center_freq",
		metavar="FREQ",
		type=eng_notation.str_to_num,
		help="Set center_freq",
		required=True
	)
	parser.add_argument(
		"--args",
		dest="args",
		metavar="ARGS",
		default="",
		type=str,
		help="Arguments to the osmosdr source"
	)
	parser.add_argument(
		"-g", "--gain",
		dest="gain",
		metavar="GAIN",
		default=20.0,
		type=float,
		help="Set gain to the osmosdr source"
	)
	parser.add_argument(
		"--corr",
		dest="corr",
		metavar="PPM",
		default=0.0,
		type=float,
		help="Set correction factor in PPM"
	)
	parser.add_argument(
		"-b", "--bw",
		dest="bw",
		metavar="BW_HZ",
		type=eng_notation.str_to_num,
		help="Select the filter bandwidth"
	)
	parser.add_argument(
		"-a", "--arfcn",
		dest="arfcns",
		metavar="ARFCN",
		type=int,
		action="append",
		help="Add an ARFCN to listen to",
		required=True
	)
	parser.add_argument(
		"-t", "--time",
		dest="time",
		metavar="SEC",
		type=float,
		help="Set the time to record",
	)
	parser.add_argument(
		"--ui",
		dest="ui",
		action='store_true',
		help="Enable fosphor UI",
	)
	args = parser.parse_args()

	# Check if UI is enabled
	if not args.ui:
		UI = False
	elif not UI:
		print "UI not available"

	# Qt setup
	if UI:
		# Qt config
		if(StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0")):
			Qt.QApplication.setGraphicsSystem(gr.prefs().get_string('qtgui','style','raster'))

		# Create app
		qapp = Qt.QApplication(sys.argv)

	# Create top-block
	tb = top_block(
		samp_rate = args.samp_rate,
		center_freq = args.center_freq,
		args = args.args,
		gain = args.gain,
		corr = args.corr,
		bw = args.bw,
		arfcns = args.arfcns,
		time = args.time,
		ui = UI,
	)

	# Qt Run
	if UI:
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

	# Console Run
	else:
		tb.start()
		tb.wait()

	# Force gargage collection, to clean up Qt widgets
	tb = None
