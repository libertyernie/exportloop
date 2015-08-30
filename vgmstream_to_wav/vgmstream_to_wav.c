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
			uint8_t header[0x2c];
			make_wav_header(&header, vgmstream->num_samples, vgmstream->sample_rate, vgmstream->channels);

			uint32_t size = get_32bitLE(header + 4);
			size += 0x44;
			put_32bitLE(header + 4, size);

			uint8_t smpl[0x44];
			make_smpl_chunk(&smpl, vgmstream->loop_start_sample, vgmstream->loop_end_sample);

			_setmode(fileno(stdout), O_BINARY);
			fwrite(header, 0x2c, 1, stdout);
			fwrite(samples, sizeof(sample), vgmstream->channels * vgmstream->num_samples, stdout);
			fwrite(smpl, 0x44, 1, stdout);
			fflush(stdout);
		}

		free(samples);

		close_vgmstream(vgmstream);
		return 1;
	}
}
