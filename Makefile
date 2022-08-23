CC := gcc

CFLAGS := -Wall -Wextra -Wpedantic \
	-Wno-multichar -Wno-sign-compare \
	-DFLOATING_POINT -DUSE_SSE -D_FILE_OFFSET_BITS=64 \
	-O3 -fvisibility=internal

LDFLAGS := -s

ifeq ($(CC),clang)
	CFLAGS += -Wno-four-char-constants -Weverything
	LDFLAGS += -target x86_64-w64-windows-gnu
else ifeq ($(CC),gcc)
	CFLAGS += -flto
else
$(error Unsupported compiler, please use gcc or clang)
endif

SRCS := fix_gc_audio.c speex/speex_resampler.c

all: fix_gc_audio

fix_gc_audio: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o fix_gc_audio.exe $(LDFLAGS)
