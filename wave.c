#include "wave.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#include "util.h"
#include "wave_helper.h"

static inline bool ck_magic(const char *s1, const char *s2) { return strneq(s1, s2, 4); }

static int find_header(const wave_t *wave, const char *id, struct wave_header *hdr)
{
    struct wave_toc *entry;

    for (entry = wave->toc; entry && !ck_magic(entry->id, id); entry = entry->next);
    if (!entry)
        return -EINVAL;

    fseek(wave->fp, entry->offset, SEEK_SET);
    return fread(hdr, sizeof(*hdr), 1, wave->fp);
}

static ssize_t read_chunk(const wave_t *wave, const char *id, void *dest, size_t len)
{
    struct wave_header hdr;

    if (find_header(wave, id, &hdr) < 0)
        return -EINVAL;
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

int wave_open(wave_t *wave, const char *filename)
{
    FILE *fp = fopen(filename, "r+");

    if (read_riff_header(fp) < 0)
        err(1, "not a valid header");

    *wave = (wave_t){ .fp = fp, };

    build_toc(wave);
    dump_toc(wave);
    return read_chunk(wave, "fmt ", &wave->fmt, sizeof(wave->fmt));
}

ssize_t find_pcm_data(const wave_t *wave)
{
    struct wave_header hdr;

    ssize_t nbytes_r = find_header(wave, "data", &hdr);
    if (nbytes_r < 0)
        return -EINVAL;

    return hdr.length;
}
