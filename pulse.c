#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "wave.h"
#include "wave_helper.h"

static inline void pa_simple_freep(pa_simple **s) { if (*s) pa_simple_free(*s); }
#define _cleanup_pa_simple_ _cleanup_(pa_simple_freep)

static void _noreturn_ _printf_(3,4) pa_err(int eval, int error, const char *fmt, ...)
{
    fprintf(stderr, "%s: ", program_invocation_short_name);

    if (fmt) {
        va_list ap;

        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, ": ");
    }

    fprintf(stderr, "%s\n", pa_strerror(error));
    exit(eval);
}

int main(int argc, char *argv[])
{
    _cleanup_pa_simple_ pa_simple *s = NULL;
    pa_sample_spec ss;
    wave_t wave;
    int ret = 1;
    int error;

    if (argc != 2) {
        fprintf(stderr, "usage: wav [file]\n");
        return 1;
    }

    if (wave_open(&wave, argv[1]) < 0)
        err(1, "failed to open wave\n");

    if (wave.fmt.format != WAVE_FORMAT_PCM)
        errx(1, "only PCM waves are supported\n");

    dump_wave_fmt(&wave.fmt);
    populate_pa_sample_spec(&ss, &wave.fmt);

    s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
    if (!s)
        pa_err(EXIT_FAILURE, error, "pa_simple_new failed");

    ssize_t len = load_pcm_data(&wave);
    for (ssize_t i = 0; i < len; i += BUFSIZ) {
        size_t chunk_size = (i + BUFSIZ > len) ? (len - i) : BUFSIZ;
        char buf[BUFSIZ];

        pa_usec_t latency = pa_simple_get_latency(s, &error);
        if (latency == (pa_usec_t)-1)
            pa_err(EXIT_FAILURE, error, "pa_simple_get_latency failed");
        fprintf(stderr, " %0.0f usec latency  \r", (float)latency);

        fread(buf, 1, chunk_size, wave.fp);
        if (pa_simple_write(s, buf, chunk_size, &error) < 0)
            pa_err(EXIT_FAILURE, error, "pa_simple_write failed");
    }

    if (pa_simple_drain(s, &error) < 0)
        pa_err(EXIT_FAILURE, error, "pa_simple_drain failed");

    return ret;
}
