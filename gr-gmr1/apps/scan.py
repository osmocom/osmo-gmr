#!/usr/bin/env python



from gnuradio import blocks
from gnuradio import gmr1
from gnuradio import gr
from gnuradio import fosphor
from gnuradio import filter
from gnuradio.filter import pfb

from gnuradio.wxgui import stdgui2


class scan_top_block(stdgui2.std_top_block):

	def __init__(self, frame, panel, vbox, argv):
		stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

		self.frame = frame
		self.panel = panel

		self._chan_width = 31.25e3
		self._src_rate = 4e6
		self._nchans = 11

		self._build_graph()


	def _build_source(self):
		self._src_file = blocks.file_source(
			gr.sizeof_gr_complex,
			"/tmp/thuraya-f1.701276e+09-s4.000000e+06-t20140823204350.cfile",
			True
		)

		self._src_slow = blocks.throttle(
			gr.sizeof_gr_complex,
			self._src_rate
		)

		self._src_swp1 = blocks.complex_to_float()
		self._src_swp2 = blocks.float_to_complex()

		self.connect( (self._src_file, 0), (self._src_slow, 0) )
		self.connect( (self._src_slow, 0), (self._src_swp1, 0) )
		self.connect( (self._src_swp1, 0), (self._src_swp2, 1) )
		self.connect( (self._src_swp1, 1), (self._src_swp2, 0) )

		return (self._src_swp2, 0)

	def _build_conditionner(self):
		# Compute params
		self._if_rate = self._chan_width * (self._nchans + 1)
		self._decim   = int(self._src_rate / self._if_rate)

		# First freq-xlating FIR filter
		taps = filter.firdes.low_pass(
			1.0,
			self._src_rate,
			(self._src_rate / (2 * self._decim)) - self._chan_width / 3.0,
			self._chan_width /  1.5
		)

		self._cond_filter = filter.freq_xlating_fir_filter_ccf(
			self._decim,
			taps,
			-905.5e3,
			self._src_rate,
		)

		if (self._src_rate / self._decim) == self._if_rate:
			return (self._cond_filter, 0), (self._cond_filter, 0)

		# Resampler
		taps = filter.firdes.low_pass(
			1.0,
			16 * self._src_rate / self._decim,
			self._chan_width * (self._nchans + 1) / 2.0 - (self._chan_width / 4.0),
			self._chan_width / 2.0
		)

		self._cond_resamp = filter.pfb_arb_resampler_ccf(
			self._decim * self._chan_width * (self._nchans + 1) / self._src_rate,
			taps,
			16
		)

		self.connect( (self._cond_filter, 0), (self._cond_resamp, 0) )

		return (self._cond_filter, 0), (self._cond_resamp, 0)

	def _build_channelization(self):
		# PFB
		taps = filter.firdes.low_pass(
			1.0,
			self._if_rate,
			(self._chan_width / 2.0) - 1.0e3,
			2e3
		)

		self._chan_pfb = pfb.channelizer_ccf(
			self._nchans + 1,
			taps,
			2.0
		)

		# Resamplers
		self._chan_resamp = [
			filter.fractional_resampler_cc(0, (self._chan_width * 2) / 93.6e3)
			for i in range(self._nchans)
		]

		for i in range(self._nchans):
			self.connect( (self._chan_pfb, i), (self._chan_resamp[i] , 0) )

		# NULL sink for the last one
		null = blocks.null_sink(gr.sizeof_gr_complex)
		self.connect( (self._chan_pfb, self._nchans), (null, 0) )

		# Return
		return (self._chan_pfb, 0), [(x,0) for x in self._chan_resamp]

	def _build_decoder(self):
		ins = []
		outs = []

		for i in range(-2, 3):
			detect = gmr1.rach_detect(4, 80, i * 0.0335 * 4, "packet_len")
			demod  = gmr1.rach_demod(4, 40, "packet_len")

			self.connect( (detect, 0), (demod, 0) )

			ins.append( (detect, 0) )
			outs.append( demod )

		return ins, outs

	def _build_graph(self):
		# Source
		src_out = self._build_source()

		# Conditionning
		cond_in, cond_out = self._build_conditionner()

		# Channelization
		chan_in, chan_outs = self._build_channelization()

		# Decoders
		dec_ins = []
		dec_outs = []

		for i in range(self._nchans):
			i, o = self._build_decoder()
			dec_ins.append(i)
			dec_outs.extend(o)

		# Sink
		self._gsmtap = gmr1.gsmtap_sink("127.0.0.1", 4729)

		# Connect it all
		self.connect( src_out, cond_in )
		self.connect( cond_out, chan_in )

		for idx in range(self._nchans):
			for i in dec_ins[idx]:
				self.connect( chan_outs[idx], i )

		for o in dec_outs:
			self.msg_connect(o, "pdus", self._gsmtap, "pdus")



def main ():
	app = stdgui2.stdapp(scan_top_block, "osmocom GMR-1 C-band scanner", nstatus=1)
	app.MainLoop()

if __name__ == '__main__':
	main ()
