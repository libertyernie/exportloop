#include <stdio.h>
#include <limits.h>

#include "../vgmstream/src/vgmstream.h"
#include "../vgmstream/src/util.h"

char* remove_extension_and_path(const char* input) {
	const char* ptr;
	
	// Loop through characters backwards to find extension
	const char* end = input + strlen(input);
	for (ptr = end; ptr >= input; ptr--) {
		if (*ptr == '/' || *ptr == '\\') {
			// No extension
			break;
		} else if (*ptr == '.') {
			// Extension found - remove
			end = ptr;
			break;
		}
	}

	// Loop through characters backwards to find slash/backslash
	const char* start = input;
	for (ptr = end; ptr >= input; ptr--) {
		if (*ptr == '/' || *ptr == '\\') {
			// Found directory separator
			start = ptr + 1;
			break;
		}
	}

	int len = end - start;
	char* buffer = (char*)malloc(len + 1);
	strncpy(buffer, start, len);
	buffer[len] = '\0';

	return buffer;
}

int usage(int retval, char* name) {
	fprintf(stderr, "exportloop 1.1.2\n"
		"https://github.com/libertyernie/vgmstream\n"
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
		"exportloop [options] {files ...}\n"
		"\n"
		"Options:\n"
		"  -vbgui   send extra data to stdout so Export Loop VB can follow the progress\n"
		"  -0L      export a .WAV file going from 0 to the loop start\n"
		"  -LE      export a .WAV file going from the loop start to the end\n"
		"  -0E      export a .WAV file going from 0 to the end\n"
		"           (will be assumed if neither -0L nor -LE are given)\n"
		"  -multi   export multiple .WAV files for streams with more than two channels\n"
		"\n"
		"If no filenames are given, but there is at least one argument, filenames will be read from standard input.\n"
		"\n");
	return retval;
}

typedef enum {
	ZERO_TO_LOOP = 1,
	LOOP_TO_END = 2,
	ZERO_TO_END = 4
} export_types;

void write_to_file(const sample* samples, const char* outfile_no_ext, const char* suffix, int32_t sample_count, int32_t sample_rate, int32_t channels, int multi) {
	char* filename_buffer = (char*)malloc(strlen(outfile_no_ext) + strlen(suffix) + 8);

	if (multi) {
		sample* buffer = (sample*)malloc(sizeof(sample) * 2 * sample_count);
		for (int startChannel = 0; startChannel < channels; startChannel += 2) {
			if (startChannel < 10000) sprintf(filename_buffer, "%s%s (%d)", outfile_no_ext, suffix, (startChannel / 2) + 1);

			int32_t local_channels = (startChannel + 1 == channels) ? 1 : 2;

			const sample* from = samples;
			sample* to = buffer;
			for (int32_t i = 0; i < sample_count; i++) {
				for (int32_t j = 0; j < channels; j++) {
					if (j >= startChannel && j < startChannel + local_channels) {
						*(to++) = *(from++);
					} else {
						from++;
					}
				}
			}

			write_to_file(buffer, filename_buffer, "", sample_count, sample_rate, local_channels, 0);
		}
	} else {
		sprintf(filename_buffer, "%s%s.wav", outfile_no_ext, suffix);

		printf("Output: %s\n", filename_buffer);
		fflush(stdout);

		uint8_t wavbuffer[0x2C];

		FILE* f = fopen(filename_buffer, "wb");
		make_wav_header(wavbuffer,
			sample_count,
			sample_rate,
			channels);
		fwrite(wavbuffer, 0x2C, 1, f);
		fwrite(samples,
			sizeof(sample),
			sample_count * channels,
			f);
		fclose(f);
	}

	free(filename_buffer);
}

int exportvgmstream(export_types export_type, int vbgui, int multi, const char* infile) {
	VGMSTREAM* vgmstream = init_vgmstream(infile);
	if (vgmstream == NULL) {
		fprintf(stderr, "Could not open file [%s]\n", infile);
		fflush(stderr);
		printf("vbgui: skipped\n");
		return 0;
	} else {
		char* infile_noext = remove_extension_and_path(infile);

		if (vbgui) printf("vbgui: rendering\n");

		printf("Rendering: %s\n", infile_noext);
		printf("Channels: %d\n", vgmstream->channels);
		fflush(stdout);
		sample* samples = (sample*)malloc(sizeof(sample) * vgmstream->channels * vgmstream->num_samples);
		render_vgmstream(samples, vgmstream->num_samples, vgmstream);

		swap_samples_le(samples, vgmstream->channels * vgmstream->num_samples);

		char* outfile = (char*)malloc(strlen(infile_noext) + 20);

		if (export_type & ZERO_TO_LOOP) {
			if (vbgui) printf("vbgui: saving\n");

			write_to_file(
				samples,
				infile_noext,
				" (beginning)",
				vgmstream->loop_start_sample,
				vgmstream->sample_rate,
				vgmstream->channels,
				multi);
		}
		if (export_type & LOOP_TO_END) {
			if (vbgui) printf("vbgui: saving\n");

			write_to_file(
				samples + vgmstream->loop_start_sample * vgmstream->channels,
				infile_noext,
				" (loop)",
				vgmstream->loop_end_sample - vgmstream->loop_start_sample,
				vgmstream->sample_rate,
				vgmstream->channels,
				multi);
		}
		if (export_type & ZERO_TO_END) {
			if (vbgui) printf("vbgui: saving\n");

			write_to_file(
				samples,
				infile_noext,
				"",
				vgmstream->num_samples,
				vgmstream->sample_rate,
				vgmstream->channels,
				multi);
		}

		free(samples);
		free(outfile);
		free(infile_noext);

		if (vbgui) printf("vbgui: finished %s\n", infile);

		close_vgmstream(vgmstream);
		return 1;
	}
}

int main(int argc, char** argv) {
	if (argc == 1) {
		return usage(1, argv[0]);
	}

	export_types export_type = 0;
	int vbgui = 0;
	int multi = 0;

	int i;
	for (i = 1; i < argc; i++) {
		char* arg = argv[i];
		if (arg == NULL) {
			// End of parameters
			break;
		} else if (strcasecmp("-0L", arg) == 0) {
			export_type |= ZERO_TO_LOOP;
		} else if (strcasecmp("-LE", arg) == 0) {
			export_type |= LOOP_TO_END;
		} else if (strcasecmp("-0E", arg) == 0) {
			export_type |= ZERO_TO_END;
		} else if (strcasecmp("-multi", arg) == 0) {
			multi = 1;
		} else if (strcasecmp("-vbgui", arg) == 0) {
			vbgui = 1;
		} else if (strcasecmp("--help", arg) == 0 || strcasecmp("/?", arg) == 0) {
			return usage(0, argv[0]);
		} else {
			// End of loop parameters
			break;
		}
	}

	if (export_type == 0) {
		export_type = ZERO_TO_END;
	}

	int filename_count = argc - i;
	char** filenames = argv + i;

	int errors = 0;

	if (filename_count > 0) {
		int j;
		for (j = 0; j < filename_count; j++) {
			char* infile = filenames[j];

			int result = exportvgmstream(export_type, vbgui, multi, infile);
			if (!result) errors++;
		}
	} else {
		char buffer[1024];
		do {
			fgets(buffer, 1024, stdin);
			char* ptr;
			for (ptr = buffer; *ptr != '\0'; ptr++) {
				if (*ptr == '\0') break;
				if (*ptr == '\r' || *ptr == '\n') {
					*ptr = '\0';
					break;
				}
			}
			if (strlen(buffer) == 0) break;

			int result = exportvgmstream(export_type, vbgui, multi, buffer);
			if (!result) errors++;
		} while (1);
	}

	return errors > 0
		? 1
		: 0;
}
