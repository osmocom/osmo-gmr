#!/usr/bin/env python

import math
import sys


class LogParser(object):

	def __init__(self, log, center_freq, sample_rate):
		self.log = log
		self.center_freq = center_freq
		self.sample_rate = sample_rate
		self.pkts = []

	def parse(self):
		pkt = {}
		for l in self.log.readlines():
			l = l.strip()
			if l[0] == '*':
				if pkt:
					self.pkts.append(pkt)
				pkt = {}

			elif l.startswith('('):
				d = dict([tuple(x.split(' . ')) for x in l[2:-2].split(') (')])
				pkt['sb_mask'] = int(d['sb_mask'])
				pkt['freq'] = float(d['freq'])

			elif l.startswith('0'):
				pkt['data'] = pkt.get('data', '') + l[6:].replace(' ', '').decode('hex')

	def freq_angular2absolute(self, freq):
		return self.center_freq + self.sample_rate * (freq / (2 * math.pi))

	def rach_latitude(self, data):
		n = (ord(data[12]) & 0x3f) << 12 | \
			(ord(data[13]) & 0xff) <<  4 | \
			(ord(data[14]) & 0xf0) >>  4

		if ord(data[12]) & 0x40:
			n = - ((n ^ 0x3ffff) + 1)

		return n / 2912.7

	def rach_longitude(self, data):
		n = (ord(data[14]) & 0x07) << 16 | \
			(ord(data[15]) & 0xff) <<  8 | \
			(ord(data[16]) & 0xff) <<  0

		if ord(data[14]) & 0x08:
			n = - ((n ^ 0x7ffff) + 1)

		return n / 2912.70555

	def gen_kml(self):
		kml = []

		kml.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
		kml.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\">")
		kml.append("<Document>")

		for pkt in self.pkts:
			kml.append(" <Placemark>")
			kml.append("  <name>SB Mask %02x</name>" % pkt['sb_mask'])
			kml.append("  <description>Freq %f</description>" % self.freq_angular2absolute(pkt['freq']))
			kml.append("  <Point>")
			kml.append("   <coordinates>%f,%f,0</coordinates>" % (self.rach_longitude(pkt['data']), self.rach_latitude(pkt['data'])))
			kml.append("  </Point>")
			kml.append(" </Placemark>")

		kml.append("</Document>")
		kml.append("</kml>")

		return '\n'.join(kml)



def main(argv0, log_filename, center_freq, sample_rate):
	# Parse arguments
	log = file(log_filename, 'r')
	center_freq = float(center_freq)
	sample_rate = int(sample_rate)

	#
	lp = LogParser(log, center_freq, sample_rate)
	lp.parse()
	print lp.gen_kml()

	# Done
	log.close()


if __name__ == '__main__':
	main(*sys.argv)
