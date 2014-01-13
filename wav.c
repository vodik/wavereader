#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#define _unused_           __attribute__((unsused))
#define _packed_           __attribute__((packed))
#define _cleanup_(x)       __attribute__((cleanup(x)))
#define _cleanup_fclose_   _cleanup_(fclosep)

static inline void fclosep(FILE **fp) { if (*fp) fclose(*fp); }

static inline bool strneq(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n) == 0; }
static inline bool ck_magic(const char *s1, const char *s2) { return strneq(s1, s2, 4); }

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

/* DEBUGGING {{{ */
static void dump_toc(const wave_t *wave)
{
    const struct wave_toc *entry;
    _cleanup_fclose_ FILE *tty = fopen("/dev/tty", "w");

    fprintf(tty, ">> DUMPING TOC\n");
    for (entry = wave->toc; entry; entry = entry->next)
        fprintf(tty, "   -> \"%c%c%c%c\" chunk found\n",
               entry->id[0],
               entry->id[1],
               entry->id[2],
               entry->id[3]);
}
/* }}} */

static int find_header(const wave_t *wave, const char *id, struct wave_header *hdr)
{
    struct wave_toc *entry;

    for (entry = wave->toc; entry && !ck_magic(entry->id, id); entry = entry->next);
    if (!entry)
        return -1;

    fseek(wave->fp, entry->offset, SEEK_SET);
    return fread(hdr, sizeof(*hdr), 1, wave->fp);
}

static ssize_t read_chunk(const wave_t *wave, const char *id, void *dest, size_t len)
{
    struct wave_header hdr;

    if (find_header(wave, id, &hdr) < 0)
        return -1;
    return fread(dest, 1, len, wave->fp);
}

static int read_riff_header(FILE *fp)
{
    struct riff_header hdr;

    fread(&hdr, sizeof(hdr), 1, fp);
    if (!ck_magic(hdr.id, "RIFF") || !ck_magic(hdr.format, "WAVE"))
        return -EINVAL;
    return 0;
}

static void build_toc(wave_t *wave)
{
    struct wave_toc *entry;
    struct wave_header hdr;

    while (1) {
        off_t offset = ftell(wave->fp);

        if (!fread(&hdr, sizeof(hdr), 1, wave->fp))
            break;

        if (!wave->toc)
            entry = wave->toc = malloc(sizeof(struct wave_toc));
        else
            entry = entry->next = malloc(sizeof(struct wave_toc));

        *entry = (struct wave_toc){
            .length = hdr.length,
            .offset = offset
        };
        memcpy(entry->id, hdr.id, sizeof(entry->id));

        fseek(wave->fp, hdr.length, SEEK_CUR);
    }
}

static int wave_open(wave_t *wave, const char *filename)
{
    FILE *fp = fopen(filename, "r+");

    if (read_riff_header(fp) < 0)
        err(1, "not a valid header");

    *wave = (wave_t){ .fp = fp, };

    build_toc(wave);
    dump_toc(wave);
    return read_chunk(wave, "fmt ", &wave->fmt, sizeof(wave->fmt));
}

static ssize_t load_pcm_data(const wave_t *wave)
{
    struct wave_header hdr;

    ssize_t nbytes_r = find_header(wave, "data", &hdr);
    if (nbytes_r < 0)
        return -1;

    return hdr.length;
}

int main(int argc, char *argv[])
{
    wave_t wave;

    if (argc != 2) {
        fprintf(stderr, "usage: wav [file]\n");
        return 1;
    }

    if (wave_open(&wave, argv[1]) < 0)
        err(1, "failed to open wave\n");

    if (wave.fmt.format != WAVE_FORMAT_PCM)
        errx(1, "only PCM waves are supported\n");

    _cleanup_fclose_ FILE *tty = fopen("/dev/tty", "w");
    fprintf(tty, "format: %d\n", wave.fmt.format);
    fprintf(tty, "channels: %d\n", wave.fmt.channels);
    fprintf(tty, "sample_rate: %d\n", wave.fmt.sample_rate);
    fprintf(tty, "bytes_per_second: %d\n", wave.fmt.bytes_per_second);
    fprintf(tty, "block_align: %d\n", wave.fmt.block_align);
    fprintf(tty, "bits_per_sample: %d\n", wave.fmt.bits_per_sample);

    ssize_t len = load_pcm_data(&wave);
    for (ssize_t i = 0; i < len; i += BUFSIZ) {
        size_t chunk_size = (i + BUFSIZ > len) ? (len - i) : BUFSIZ;
        char buf[BUFSIZ];

        fread(buf, 1, chunk_size, wave.fp);
        for (size_t j = 0; j < chunk_size; ++j)
            putchar(buf[j]);
    }

    return 0;
}
