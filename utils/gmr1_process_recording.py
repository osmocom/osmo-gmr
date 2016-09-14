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

from collections import namedtuple

import datetime
import os
import sys
import re


EXEC_GMR1_DEMOD = 'gmr1_rx_live'
EXEC_GMR1_SPLIT = 'gmr1_rx_sdr.py'



CHAN_BW = 31.25e3

Recording = namedtuple('Recording', 'center samplerate timestamp')

def parse_filename(fn):
	m = re.match('^.*-f([0-9\.e+-]*)-s([0-9\.e+-]*)-t([0-9]{14})\.cfile$', fn)
	if not m:
		return m

	return Recording(
		float(m.group(1)),
		float(m.group(2)),
		datetime.datetime.strptime(m.group(3), '%Y%m%d%H%M%S'),
	)

def arfcn_to_freq(arfcn, band='L'):
	base = {
		'L': 1525e6,
		'S': 2170e6 + 15.625e3,
	}
	return base[band] + 31.25e3 * arfcn

def arfcn_fifo(arfcn):
	return "/tmp/arfcn_%d.cfile" % arfcn


def main(argv0, capture_fn):

	# Parse filename
	p = parse_filename(capture_fn)

	# Derive upper and lower band
	ll = p.center - p.samplerate / 2 + CHAN_BW
	ul = p.center + p.samplerate / 2 - CHAN_BW

	# Select band
	band = 'S' if ul > 2e9 else 'L'
	n_arfcns = {
		'L': 1087,
		'S': 960,
	}

	# List all visible arfcns
	visible_arfcns = [x for x in range(0,n_arfcns[band]+1) if ll <= arfcn_to_freq(x, band) <= ul]

	# Create all FIFOs
	#for arfcn in visible_arfcns:
	#	if os.path.exists(arfcn_fifo(arfcn)):
	#		print "Output FIFO already exists, aborting !"
	#		return -1

	#for arfcn in visible_arfcns:
	#	os.mkfifo(arfcn_fifo(arfcn))

	#
	exec_split = [
		EXEC_GMR1_SPLIT,
		'-s', '%f' % p.samplerate,
		'-f', '%f' % p.center,
		'-B', band,
		'--args', 'file=%s,freq=%f,rate=%f,throttle=false,repeat=false' % (capture_fn, p.center, p.samplerate),
	]

	for arfcn in visible_arfcns:
		exec_split.extend(['-a', '%d' % arfcn])

	print ' '.join(exec_split)

	#
	exec_rx = [
		EXEC_GMR1_DEMOD, '4',
	]

	for arfcn in visible_arfcns:
		exec_rx.append('%d:%s' % (arfcn, arfcn_fifo(arfcn)))

	print ' '.join(exec_rx)

	# Cleanup all FIFOs
	#for arfcn in visible_arfcns:
	#	os.unlink(arfcn_fifo(arfcn))


if __name__ == '__main__':
	main(*sys.argv)
