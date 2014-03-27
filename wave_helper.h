#pragma once

#include <pulse/simple.h>
#include "wave.h"

/* DEBUGGING {{{ */
void dump_toc(const wave_t *wave);
void dump_wave_fmt(struct wave_fmt *fmt);
/* }}} */

int populate_pa_sample_spec(pa_sample_spec *ss, const struct wave_fmt *fmt);
