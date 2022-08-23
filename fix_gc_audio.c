#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "speex/speex_resampler.h"

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
	u32 Marker; // 'RIFF'
	u32 Size; // file size - 8
	u32 Type; // 'WAVE'

	u32 FmtChunk; // 'fmt '
	u32 FmtSize; // 16
	u16 Format; // 1
	u16 Channels; // 2
	u32 SampleRate; // 48000
	u32 ByteRate; // 48000 * 4
	u16 FrameSize;  // 4
	u16 BitDepth; // 16

	u32 DataChunk; // 'data'
	u32 DataSize; // length of samples
} __attribute__((packed)) WavHeader_t;

static_assert(sizeof(WavHeader_t) == 44, "WavHeader_t is the wrong size");
static_assert(sizeof(size_t) > 4, "size_t is too small");

int main(int argc, char* argv[]) {
	if (argc < 2) {
		return -1;
	}

	int err = 0;
	SpeexResamplerState* resampler = speex_resampler_init(2, 32000, 48000, SPEEX_RESAMPLER_QUALITY_MAX, &err);
	if (err != RESAMPLER_ERR_SUCCESS) {
		fprintf(stderr, "ERROR: Could not create resampler, reason: %s\n", speex_resampler_strerror(err));
		return -1;
	}

	FILE* output = fopen("out.wav", "wb");
	if (!output) {
		fprintf(stderr, "ERROR: Could not create output file.\n");
		speex_resampler_destroy(resampler);
		return -1;
	}
	static const WavHeader_t dummy = {0};
	fwrite(&dummy, sizeof(WavHeader_t), 1, output);
	for (int i = 1; i < argc; i++) {
		FILE* f = fopen(argv[i], "rb");
		if (!f) {
			fprintf(stderr, "WARNING: Could not open file %s. Skipping this file...\n", argv[i]);
			continue;
		}

		fseek(f, 0, SEEK_END);
		size_t size = ftello(f);
		if (size < sizeof(WavHeader_t)) {
			fprintf(stderr, "WARNING: File is smaller than a wav header (is this a wav file?). Skipping this file...\n");
			fclose(f);
			continue;
		}

		u8* wavBuf = malloc(size);
		if (!wavBuf) {
			fprintf(stderr, "WARNING: Could not allocate memory for wav buffer. Skipping this file...\n");
			fclose(f);
			continue;
		}
		fseek(f, 0, SEEK_SET);
		fread(wavBuf, sizeof(u8), size, f);
		fclose(f);

		const WavHeader_t* header = (const WavHeader_t*)wavBuf;
		u32 inputDivisor;

		switch (header->SampleRate) {
			// "normal" rates (wii or very old dolphin)
			case 32000: inputDivisor = 30375; break;
			case 48000: inputDivisor = 20250; break;
			// bad rates (these are several hertz off)
			case 32029: inputDivisor = 30352; break;
			case 48043: inputDivisor = 20233; break;
			// close but still off (less than a hertz)
			case 32028: inputDivisor = 30348; break;
			case 48042: inputDivisor = 20232; break;
			// uhhh what?
			default:
				fprintf(stderr, "WARNING: Could not identify sample rate (Is this a Dolphin wav?) Skipping this file...\n");
				free(wavBuf);
				continue;
		}

		speex_resampler_set_rate_frac(resampler, 20250, inputDivisor, 972000000 / inputDivisor, 48000);

		static s16 outSampleBuf[48000] = {0};
		u32 index;
		for (index = sizeof(WavHeader_t); (index + 3200) < size; index += 3200) {
			u32 inLen = 800;
			u32 outLen = sizeof(outSampleBuf) / 4;
			speex_resampler_process_interleaved_int(resampler, (const s16*)&wavBuf[index], &inLen, outSampleBuf, &outLen);
			if (inLen != 800) {
				fprintf(stderr, "WARNING: Resampler did not eat chunk of samples\n");
			}
			fwrite(outSampleBuf, 4, outLen, output);
		}

		u32 inLen = (size - index) / 4;
		if (inLen) {
			u32 outLen = sizeof(outSampleBuf) / 4;
			speex_resampler_process_interleaved_int(resampler, (const s16*)&wavBuf[index], &inLen, outSampleBuf, &outLen);
			if (inLen != ((size - index) / 4)) {
				fprintf(stderr, "WARNING: Resampler did not eat final chunk of samples\n");
			}
			fwrite(outSampleBuf, 4, outLen, output);
		}

		free(wavBuf);
	}

	speex_resampler_destroy(resampler);

	fseek(output, 0, SEEK_END);
	u64 size = ftello(output);
	fseek(output, 0, SEEK_SET);
	WavHeader_t header = {
		.Marker = __builtin_bswap32('RIFF'),
		.Size = size - 8,
		.Type = __builtin_bswap32('WAVE'),

		.FmtChunk = __builtin_bswap32('fmt '),
		.FmtSize = 16,
		.Format = 1,
		.Channels = 2,
		.SampleRate = 48000,
		.ByteRate = 48000 * 4,
		.FrameSize = 4,
		.BitDepth = 16,

		.DataChunk = __builtin_bswap32('data'),
		.DataSize = size - sizeof(WavHeader_t),
	};
	fwrite(&header, sizeof(WavHeader_t), 1, output);
	fclose(output);

	return 0;
}
