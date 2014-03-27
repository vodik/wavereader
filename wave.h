#pragma once

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "util.h"

/* WAV internals {{{ */
enum wave_format_code {
    WAVE_FORMAT_PCM = 0x0001,
    WAVE_FORMAT_IEEE_FLOAT = 0x0003,
    WAVE_FORMAT_ALAW = 0x0006,
    WAVE_FORMAT_MULAW = 0x0007,
    WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

struct riff_header {
    char id[4];
    int32_t length;
    char format[4];
} _packed_;

struct wave_header {
    char id[4];
    int32_t length;
} _packed_;

struct wave_fmt {
    int16_t format;
    int16_t channels;
    int32_t sample_rate;
    int32_t bytes_per_second;
    int16_t block_align;
    int16_t bits_per_sample;
} _packed_;

struct wave_fact {
    int32_t sample_length;
} _packed_;
/* }}} */

struct wave_toc {
    char id[4];
    int32_t length;
    off_t offset;
    struct wave_toc *next;
};

typedef struct wave {
    FILE *fp;
    struct wave_toc *toc;
    struct wave_fmt fmt;
} wave_t;

int wave_open(wave_t *wave, const char *filename);
ssize_t load_pcm_data(const wave_t *wave);
