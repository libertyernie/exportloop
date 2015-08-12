#include <stdio.h>
#include <limits.h>

#include "../src/vgmstream.h"
#include "../src/util.h"

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

int exportvgmstream(export_types export_type, int vbgui, const char* infile) {
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

		uint8_t* wavbuffer = (uint8_t*)malloc(0x2C);
		char* outfile = (char*)malloc(strlen(infile_noext) + 20);

		if (export_type & ZERO_TO_LOOP) {
			if (vbgui) printf("vbgui: saving\n");

			sprintf(outfile, "%s (beginning).wav", infile_noext);
			printf("Output: %s\n", outfile);
			fflush(stdout);

			FILE* f = fopen(outfile, "wb");
			make_wav_header(wavbuffer,
				vgmstream->loop_start_sample,
				vgmstream->sample_rate,
				vgmstream->channels);
			fwrite(wavbuffer, 0x2C, 1, f);
			fwrite(samples,
				sizeof(sample),
				vgmstream->loop_start_sample * vgmstream->channels,
				f);
			fclose(f);
		}
		if (export_type & LOOP_TO_END) {
			if (vbgui) printf("vbgui: saving\n");

			sprintf(outfile, "%s (loop).wav", infile_noext);
			printf("Output: %s\n", outfile);
			fflush(stdout);

			FILE* f = fopen(outfile, "wb");
			make_wav_header(wavbuffer,
				vgmstream->loop_end_sample - vgmstream->loop_start_sample,
				vgmstream->sample_rate,
				vgmstream->channels);
			fwrite(wavbuffer, 0x2C, 1, f);
			fwrite(samples + vgmstream->loop_start_sample * vgmstream->channels,
				sizeof(sample),
				(vgmstream->loop_end_sample - vgmstream->loop_start_sample) * vgmstream->channels,
				f);
			fclose(f);
		}
		if (export_type & ZERO_TO_END) {
			if (vbgui) printf("vbgui: saving\n");

			sprintf(outfile, "%s.wav", infile_noext);
			printf("Output: %s\n", outfile);
			fflush(stdout);

			FILE* f = fopen(outfile, "wb");
			make_wav_header(wavbuffer,
				vgmstream->num_samples,
				vgmstream->sample_rate,
				vgmstream->channels);
			fwrite(wavbuffer, 0x2C, 1, f);
			fwrite(samples,
				sizeof(sample),
				vgmstream->num_samples * vgmstream->channels,
				f);
			fclose(f);
		}

		free(samples);
		free(outfile);
		free(wavbuffer);
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

			int result = exportvgmstream(export_type, vbgui, infile);
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

			int result = exportvgmstream(export_type, vbgui, buffer);
			if (!result) errors++;
		} while (1);
	}

	return errors > 0
		? 1
		: 0;
}
