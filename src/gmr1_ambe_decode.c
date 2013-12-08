/* GMR-1 Codec decoder tool */

/* (C) 2013 by Sylvain Munaut <tnt@246tNt.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include <osmocom/gmr1/codec/codec.h>


static const uint8_t wav_hdr[] = {
	/* WAV header */
	'R', 'I', 'F', 'F',		/* ChunkID   */
	0x00, 0x00, 0x00, 0x00,		/* ChunkSize */
	'W', 'A', 'V', 'E',		/* Format    */

	/* Sub chunk: format */
	'f', 'm', 't', ' ',		/* Subchunk1ID         */
	0x10, 0x00, 0x00, 0x00,		/* Subchunk1Size       */
	0x01, 0x00,			/* AudioFormat: PCM    */
	0x01, 0x00,			/* NumChannels: Mono   */
	0x40, 0x1f, 0x00, 0x00,		/* SampleRate: 8000 Hz */
	0x80, 0x3e, 0x00, 0x00,		/* ByteRate: 16k/s     */
	0x02, 0x00,			/* BlockAlign: 2 bytes */
	0x10, 0x00,			/* BitsPerSample: 16   */

	/* Sub chunk: data */
	'd', 'a', 't', 'a',		/* Subchunk2ID   */
	0x00, 0x00, 0x00, 0x00,		/* Subchunk2Size */
};

static uint32_t
le32(uint32_t v)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return v;
#else
	return ((v & 0x000000ff) << 24) |
	       ((v & 0x0000ff00) <<  8) |
	       ((v & 0x00ff0000) >>  8) |
	       ((v & 0xff000000) >> 24);
#endif
}

static uint16_t
le16(uint16_t v)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return v;
#else
	return ((v & 0x00ff) << 8) |
	       ((v & 0xff00) >> 8);
#endif
}

int main(int argc, char *argv[])
{
	struct gmr1_codec *codec = NULL;
	FILE *fin, *fout;
	int is_wave = 0, l, rv;

        /* Arguments */
	if (argc > 3) {
		fprintf(stderr, "Usage: %s [in_file [out_file]]\n", argv[0]);
		return -1;
	}

	if ((argc < 2) || !strcmp(argv[1], "-"))
		fin = stdin;
	else {
		fin = fopen(argv[1], "rb");
		if (!fin) {
			fprintf(stderr, "[!] Unable to open input file\n");
			return -1;
		}
	}

	if ((argc < 3) || !strcmp(argv[2], "-"))
		fout = stdout;
	else {
		fout = fopen(argv[2], "wb");
		if (!fout) {
			fprintf(stderr, "[!] Unable to open output file\n");
			return -1;
		}

		l = strlen(argv[2]);

		if ((l > 4) && (!strcmp(".wav", &argv[2][l-4])))
			is_wave = 1;
	}

	/* Write inital wave header */
	if (is_wave) {
		rv = fwrite(wav_hdr, sizeof(wav_hdr), 1, fout);
		if (rv != 1) {
			fprintf(stderr, "[!] Failed to write WAV header\n");
			goto exit;
		}
	}

	/* Init decoder */
	codec = gmr1_codec_alloc();
	if (!codec)
		goto exit;

        /* Process all frames */
	l = 0;

	while (!feof(fin))
	{
		uint8_t frame[10];
		int16_t audio[160];
		int rv, i;

		/* Read input frame */
		rv = fread(frame, 1, 10, fin);
		if (rv != 10)
			break;

		/* Decompress */
		rv = gmr1_codec_decode_frame(codec, audio, 160, frame, 0);
		if (rv) {
			fprintf(stderr, "[!] codec error\n");
			break;
		}

		/* Write audio output */
		for (i=0; i<160; i++)
			audio[i] = le16(audio[i]);

		rv = fwrite(audio, 2, 160, fout);
		if (rv != 160) {
			fprintf(stderr, "[!] short write\n");
			break;
		}

		/* Keep track of number of samples */
		l += 160;
	}

	/* Release decoder */
	gmr1_codec_release(codec);

	/* Fix wave header */
	if (is_wave)
	{
		uint32_t v;

		/* Fixup Subchunk2Size */
		v = le32(l * 2);

		rv = fseek(fout, 40, SEEK_SET);
		if (rv < 0)
			goto exit;

		rv = fwrite(&v, 4, 1, fout);
		if (rv < 0)
			goto exit;

		/* Fixup ChunkSize */
		v = le32(l * 2 + 36);

		rv = fseek(fout, 4, SEEK_SET);
		if (rv < 0)
			goto exit;

		rv = fwrite(&v, 4, 1, fout);
		if (rv < 0)
			goto exit;
	}

exit:
	/* Close in/out */
	fclose(fout);
	fclose(fin);

	/* All done ! */
	return 0;
}
