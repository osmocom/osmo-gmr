#include <stdint.h>
#include <stdio.h>

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/gmr1/gsmtap.h>
#include <osmocom/gmr1/l1/bcch.h>


int main(int argc, char *argv[])
{
	struct gsmtap_inst *gti;
	sbit_t bits_e[424];
	uint8_t l2[24];
	int i, crc, conv;

	for (i=0; i<424; i++) {
		bits_e[i] = argv[1][i] == '1' ? -127 : 127;
	}

	gti = gsmtap_source_init("127.0.0.1", GSMTAP_UDP_PORT, 0);
	gsmtap_source_add_sink(gti);

	crc = gmr1_bcch_decode(l2, bits_e, &conv);

	fprintf(stderr, "conv: %d\n", conv);
	fprintf(stderr, "crc: %d\n", crc);

	if (!crc)
		gsmtap_sendmsg(gti, gmr1_gsmtap_makemsg(GSMTAP_GMR1_BCCH, l2, 24));

	return 0;
}
