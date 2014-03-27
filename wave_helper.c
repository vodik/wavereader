#include "wave_helper.h"

#include <stdlib.h>
#include <err.h>
#include <pulse/simple.h>
#include "wave.h"

/* DEBUGGING {{{ */
void dump_toc(const wave_t *wave)
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

void dump_wave_fmt(struct wave_fmt *fmt)
{
    _cleanup_fclose_ FILE *tty = fopen("/dev/tty", "w");

    fprintf(tty, "format: %d\n", fmt->format);
    fprintf(tty, "channels: %d\n", fmt->channels);
    fprintf(tty, "sample_rate: %d\n", fmt->sample_rate);
    fprintf(tty, "bytes_per_second: %d\n", fmt->bytes_per_second);
    fprintf(tty, "block_align: %d\n", fmt->block_align);
    fprintf(tty, "bits_per_sample: %d\n", fmt->bits_per_sample);
}
/* }}} */

void populate_pa_sample_spec(pa_sample_spec *ss, const struct wave_fmt *fmt)
{
    pa_sample_format_t format;

    switch (fmt->bits_per_sample) {
    case 8:
        format = PA_SAMPLE_U8;
        break;
    default:
        errx(EXIT_FAILURE, "unsupported bits_per_sample");
    }

    *ss = (pa_sample_spec){
        .format   = format,
        .rate     = fmt->sample_rate,
        .channels = fmt->channels
    };
}
