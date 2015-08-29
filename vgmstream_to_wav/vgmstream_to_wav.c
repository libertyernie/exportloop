#include <stdio.h>
#include <fcntl.h>

#include "../vgmstream/src/vgmstream.h"
#include "../vgmstream/src/util.h"

int usage(int retval, char* name) {
	fprintf(stderr, "vgmstream_to_wav 1.0\n"
		"https://github.com/libertyernie/vgmstream_to_wav\n"
		"\n"
		"Permission to use, copy, modify, and distribute this software for any "
		"purpose with or without fee is hereby granted, provided that the above "
		"copyright notice and this permission notice appear in all copies.\n"
		"\n"
		"THE SOFTWARE IS PROVIDED ""AS IS"" AND THE AUTHOR DISCLAIMS ALL WARRANTIES "
		"WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF "
		"MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR "
		"ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES "
		"WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN "
		"ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF "
		"OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n"
		"\n"
		"Usage:\n"
		"vgmstream_to_wav filename\n"
		"\n"
		"File will be written to standard output.\n"
		"\n");
	return retval;
}

// make a header for PCM .wav, with smpl chunk included for looping information. 0x70.
void make_looping_wav_header(uint8_t * buf, int32_t loop_start, int32_t sample_count, int32_t sample_rate, int channels) {
	size_t bytecount;

	bytecount = sample_count*channels*sizeof(sample);

	/* RIFF header */
	memcpy(buf + 0, "RIFF", 4);
	/* size of RIFF */
	put_32bitLE(buf + 4, (int32_t)(bytecount + 0x2c - 8 + 0x44));

	/* WAVE header */
	memcpy(buf + 8, "WAVE", 4);

	/* WAVE fmt chunk */
	memcpy(buf + 0xc, "fmt ", 4);
	/* size of WAVE fmt chunk */
	put_32bitLE(buf + 0x10, 0x10);

	/* compression code 1=PCM */
	put_16bitLE(buf + 0x14, 1);

	/* channel count */
	put_16bitLE(buf + 0x16, channels);

	/* sample rate */
	put_32bitLE(buf + 0x18, sample_rate);

	/* bytes per second */
	put_32bitLE(buf + 0x1c, sample_rate*channels*sizeof(sample));

	/* block align */
	put_16bitLE(buf + 0x20, (int16_t)(channels*sizeof(sample)));

	/* significant bits per sample */
	put_16bitLE(buf + 0x22, sizeof(sample) * 8);

	/* WAVE smpl chunk */
	memcpy(buf + 0x24, "smpl", 4);
	/* size of WAVE smpl chunk */
	put_32bitLE(buf + 0x28, 0x3c);

	/* manufacturer */
	put_32bitLE(buf + 0x2c, 0);

	/* product */
	put_32bitLE(buf + 0x30, 0);

	/* sample period */
	put_32bitLE(buf + 0x34, (int32_t)(1 / (sample_rate * 1000000000)));

	/* MIDI note */
	put_32bitLE(buf + 0x38, 0);

	/* MIDI pitch fraction */
	put_32bitLE(buf + 0x3c, 0);

	/* SMPTE format */
	put_32bitLE(buf + 0x40, 0);

	/* SMPTE offset */
	put_32bitLE(buf + 0x44, 0);

	/* Number of sample loops */
	put_32bitLE(buf + 0x48, 1);

	/* Number of optional fields after sample loops */
	put_32bitLE(buf + 0x4c, 0);

	/* Unique identifier for loop */
	put_32bitLE(buf + 0x50, 1);

	/* Loop type */
	put_32bitLE(buf + 0x54, 0);

	/* Loop start */
	put_32bitLE(buf + 0x58, loop_start);

	/* Loop end */
	put_32bitLE(buf + 0x5c, sample_count);

	/* Loop fractional area */
	put_32bitLE(buf + 0x60, 0);

	/* Loop play count */
	put_32bitLE(buf + 0x64, 0);

	/* WAVE data chunk */
	memcpy(buf + 0x68, "data", 4);
	/* size of WAVE data chunk */
	put_32bitLE(buf + 0x6c, (int32_t)bytecount);
}

int main(int argc, char** argv) {
	if (argc != 2) {
		return usage(1, argv[0]);
	}

	VGMSTREAM* vgmstream = init_vgmstream(argv[1]);
	fprintf(stderr, "[%s]\n", argv[1]);
	if (vgmstream == NULL) {
		fprintf(stderr, "Could not open file [%s]\n", argv[1]);
		fflush(stderr);
		return 0;
	} else {
		sample* samples = (sample*)malloc(sizeof(sample) * vgmstream->channels * vgmstream->num_samples);
		render_vgmstream(samples, vgmstream->num_samples, vgmstream);

		swap_samples_le(samples, vgmstream->channels * vgmstream->num_samples);

		if (vgmstream->loop_flag) {
			uint8_t header[0x70];
			make_looping_wav_header(&header, vgmstream->loop_start_sample, vgmstream->num_samples, vgmstream->sample_rate, vgmstream->channels);
			_setmode(fileno(stdout), O_BINARY);
			fwrite(header, 0x70, 1, stdout);
			fwrite(samples, sizeof(sample), vgmstream->channels * vgmstream->num_samples, stdout);
			fflush(stdout);
		}

		free(samples);

		close_vgmstream(vgmstream);
		return 1;
	}
}
