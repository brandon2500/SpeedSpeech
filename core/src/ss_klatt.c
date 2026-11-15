/*
 * Cascade/parallel formant synthesizer in the style of Klatt (1980).
 *
 *   glottal pulse --+--> nasal pole/zero -> F6..F1 (cascade) -------------+
 *   aspiration -----+                                                    +--> radiation -> out
 *   frication -----------> two noise resonators + flat bypass ----------+
 *
 * Everything runs at the output sample rate; levels are normalised so that
 * loudness and timbre do not depend on it.
 */
#include "ss_internal.h"
#include <math.h>
#include <string.h>

#define KL_REF_RATE 16000.0f
#define KL_VOICE_LEVEL 6.3f  /* balance of voicing ...           */
#define KL_FRIC_LEVEL 0.28f  /* ... against frication             */
#define KL_ASP_LEVEL 0.25f   /* ... and aspiration               */

/* ---- resonators ---- */

static void kl_reson_set(ss_reson *r, float f, float bw, float fs)
{
    float T = 1.0f / fs;
    float c = -expf(-2.0f * SS_PI * bw * T);
    float b = 2.0f * expf(-SS_PI * bw * T) * cosf(2.0f * SS_PI * f * T);
    r->a = 1.0f - b - c;
    r->b = b;
    r->c = c;
}

static float kl_reson_run(ss_reson *r, float x)
{
    float y = r->a * x + r->b * r->y1 + r->c * r->y2;
    r->y2 = r->y1;
    r->y1 = y;
    return y;
}

/* Antiresonator: inverse of a resonator; y1/y2 hold past *inputs*. */
static void kl_anti_set(ss_reson *r, float f, float bw, float fs)
{
    kl_reson_set(r, f, bw, fs);
    r->a = 1.0f / r->a;
    r->b = -r->b * r->a;
    r->c = -r->c * r->a;
}

static float kl_anti_run(ss_reson *r, float x)
{
    float y = r->a * x + r->b * r->y1 + r->c * r->y2;
    r->y2 = r->y1;
    r->y1 = x;
    return y;
}

/* Gain that makes a resonator's peak response 1. */
static float kl_peak_norm(const ss_reson *r, float f, float fs)
{
    float w = 2.0f * SS_PI * f / fs;
    float re = 1.0f - r->b * cosf(w) - r->c * cosf(2.0f * w);
    float im = r->b * sinf(w) + r->c * sinf(2.0f * w);
    float mag = sqrtf(re * re + im * im);
    float peak = mag > 1e-9f ? fabsf(r->a) / mag : 1.0f;
    return peak > 1e-9f ? 1.0f / peak : 1.0f;
}

/* ---- random numbers ---- */

static float kl_rand(ss_synth *s)
{
    unsigned int x = s->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->rng = x;
    return (float)(x >> 8) * (2.0f / 16777216.0f) - 1.0f;
}

/* ---- setup ---- */

void ss_synth_reset(ss_synth *s)
{
    int i;
    s->np.y1 = s->np.y2 = s->nz.y1 = s->nz.y2 = 0.0f;
    for (i = 0; i < 6; i++) s->cas[i].y1 = s->cas[i].y2 = 0.0f;
    s->fr1.y1 = s->fr1.y2 = s->fr1b.y1 = s->fr1b.y2 = s->fr2.y1 = s->fr2.y2 = 0.0f;
    s->rad_prev = s->dc_x = s->dc_y = 0.0f;
    s->tilt_state = s->noise_lp = 0.0f;
    s->phase = 0.0f;
}

void ss_synth_init(ss_synth *s, int sample_rate)
{
    ss_voice v;
    memset(s, 0, sizeof(*s));
    s->fs = (float)sample_rate;
    s->frame_len = sample_rate / 500; /* 2 ms parameter frames */
    if (s->frame_len < 8) s->frame_len = 8;
    s->gain = 1.0f;
    ss_voice_preset(&v, SS_PRESET_DEFAULT);
    ss_synth_set_voice(s, &v);
    ss_synth_reset(s);
}

void ss_synth_set_voice(ss_synth *s, const ss_voice *v)
{
    float fc;
    s->voice = *v;
    s->rng = v->seed ? v->seed : 0x9E3779B9u;
    /* open quotient: natural ~0.6, buzzy voices have short, sharp pulses */
    s->oq = 0.62f - 0.42f * v->buzz;
    /* spectral tilt: a one-pole lowpass on the source */
    fc = 5000.0f + 9000.0f * v->buzz;
    s->tilt_coef = expf(-2.0f * SS_PI * fc / s->fs);
    s->gain = 0.5f * v->volume;
    /* sharper pulses excite the tract harder: compensate */
    s->voice_norm = KL_VOICE_LEVEL * s->oq / 0.62f;
}

static float kl_minf(float a, float b) { return a < b ? a : b; }

/* Apply voice transformations and update filter coefficients for frame `f`. */
void ss_synth_frame(ss_synth *s, const ss_frame *f)
{
    const ss_voice *v = &s->voice;
    float fs = s->fs, nyq = 0.46f * s->fs;
    float shift = v->formant_shift;
    float bshift = 1.0f + (shift - 1.0f) * 0.5f;
    float fshift = sqrtf(shift);
    static const float hi_f[3] = {3300.0f, 3850.0f, 4900.0f};
    static const float hi_b[3] = {250.0f, 300.0f, 1000.0f};
    int i;
    ss_frame n = *f;

    s->cur = s->next;
    for (i = 0; i < 3; i++) {
        n.form[SS_FP_F1 + i] = kl_minf(n.form[SS_FP_F1 + i] * shift, nyq);
        n.form[SS_FP_B1 + i] *= bshift;
    }
    n.form[SS_FP_NZ] *= shift;
    n.amp[SS_AP_FR1F] = kl_minf(n.amp[SS_AP_FR1F] * fshift, nyq);
    n.amp[SS_AP_FR2F] = kl_minf(n.amp[SS_AP_FR2F] * fshift, nyq);
    s->next = n;

    /* cascade */
    kl_reson_set(&s->np, kl_minf(270.0f * shift, nyq), 100.0f, fs);
    kl_anti_set(&s->nz, kl_minf(n.form[SS_FP_NZ], nyq), 100.0f, fs);
    for (i = 0; i < 3; i++)
        kl_reson_set(&s->cas[i], n.form[SS_FP_F1 + i], n.form[SS_FP_B1 + i], fs);
    for (i = 0; i < 3; i++) {
        float hf = hi_f[i] * shift;
        s->cas_on[3 + i] = hf < nyq;
        if (s->cas_on[3 + i]) kl_reson_set(&s->cas[3 + i], hf, hi_b[i] * bshift, fs);
    }
    s->cas_on[0] = s->cas_on[1] = s->cas_on[2] = 1;

    /* frication */
    kl_reson_set(&s->fr1, n.amp[SS_AP_FR1F], n.amp[SS_AP_FR1B], fs);
    s->fr1b.a = s->fr1.a; /* same coefficients, own state */
    s->fr1b.b = s->fr1.b;
    s->fr1b.c = s->fr1.c;
    s->fr1_norm = kl_peak_norm(&s->fr1, n.amp[SS_AP_FR1F], fs);
    s->fr1_norm *= s->fr1_norm;
    kl_reson_set(&s->fr2, n.amp[SS_AP_FR2F], n.amp[SS_AP_FR2B], fs);
    s->fr2_norm = kl_peak_norm(&s->fr2, n.amp[SS_AP_FR2F], fs);

    /* pitch modulation for this frame: Klatt-style flutter plus optional vibrato */
    {
        float tt = (float)s->time;
        float fl = sinf(2.0f * SS_PI * 12.7f * tt) + sinf(2.0f * SS_PI * 7.1f * tt) +
                   sinf(2.0f * SS_PI * 4.7f * tt);
        s->pitch_mod = 1.0f + 0.004f * fl;
        if (v->vibrato_hz > 0.0f && v->vibrato_depth > 0.0f)
            s->pitch_mod *= powf(2.0f, v->vibrato_depth / 12.0f * sinf(2.0f * SS_PI * v->vibrato_hz * tt));
        s->time += (double)s->frame_len / (double)fs;
        if (s->time > 1000.0) s->time -= 1000.0;
    }
    s->frame_pos = 0;
}

/* One glottal sample. Returns the flow and sets *open to whether the glottis
 * is open (used to gate breath noise). */
static float kl_glottal(ss_synth *s, float f0, int *open)
{
    float x;
    s->phase += s->period_f0 / s->fs;
    if (s->phase >= 1.0f) {
        const ss_voice *v = &s->voice;
        s->phase -= 1.0f;
        if (s->phase >= 1.0f) s->phase = 0.0f;
        /* latch pitch for this period, with jitter / shimmer */
        s->period_f0 = f0 * (1.0f + v->roughness * 0.05f * kl_rand(s));
        s->shimmer = 1.0f + v->roughness * 0.35f * kl_rand(s);
        /* the pulse derivative grows with f0: keep loudness steady under intonation */
        s->pulse_gain = s->shimmer * s->voice_norm * powf(110.0f / s->period_f0, 0.9f);
    }
    if (s->period_f0 <= 0.0f) {
        s->period_f0 = f0;
        s->pulse_gain = s->voice_norm * powf(110.0f / f0, 0.9f);
    }
    if (s->phase < s->oq) {
        x = s->phase / s->oq;
        *open = 1;
        return (x * x - x * x * x) * 6.75f; /* peak 1 at x = 2/3 */
    }
    *open = 0;
    return 0.0f;
}

void ss_synth_run(ss_synth *s, float *out, int n)
{
    const ss_voice *v = &s->voice;
    const float fs = s->fs;
    const float rad_gain = fs / KL_REF_RATE;
    const float noise_gain = sqrtf(fs / KL_REF_RATE);
    const float noise_lp = expf(-2.0f * SS_PI * 1000.0f / fs);
    const float dc_r = 1.0f - 2.0f * SS_PI * 30.0f / fs;
    const float whisper = v->whisper;
    const float breath = v->breathiness;
    const float flen = (float)s->frame_len;
    int i;

    for (i = 0; i < n; i++) {
        /* linear interpolation of amplitudes across the frame */
        float w = (float)(s->frame_pos + 1) / flen;
        float av, ah, af, a1, a2, byp, f0;
        float src, noise, glot, casc, par, y;
        int open;
        if (w > 1.0f) w = 1.0f;
        av = s->cur.amp[SS_AP_AV] + (s->next.amp[SS_AP_AV] - s->cur.amp[SS_AP_AV]) * w;
        ah = s->cur.amp[SS_AP_AH] + (s->next.amp[SS_AP_AH] - s->cur.amp[SS_AP_AH]) * w;
        af = s->cur.amp[SS_AP_AF] + (s->next.amp[SS_AP_AF] - s->cur.amp[SS_AP_AF]) * w;
        a1 = s->next.amp[SS_AP_FR1A];
        a2 = s->next.amp[SS_AP_FR2A];
        byp = s->next.amp[SS_AP_BYP];
        f0 = s->cur.f0 + (s->next.f0 - s->cur.f0) * w;
        s->frame_pos++;

        f0 *= s->pitch_mod;
        if (f0 < 30.0f) f0 = 30.0f;
        if (f0 > 0.25f * fs) f0 = 0.25f * fs;

        /* noise: lowpassed white noise, flat after radiation */
        noise = kl_rand(s) * noise_gain;
        s->noise_lp = noise + (s->noise_lp - noise) * noise_lp;
        noise = s->noise_lp * 2.5f;

        /* glottal source */
        glot = kl_glottal(s, f0, &open);
        s->tilt_state = glot + (s->tilt_state - glot) * s->tilt_coef;
        glot = s->tilt_state * s->pulse_gain;

        src = glot * av * (1.0f - whisper);
        /* breathiness: noise during the open phase; whisper: noise always */
        src += noise * av * (breath * 0.15f * (float)open + whisper * 0.25f);
        /* aspiration and noise in voiced frication are pitch-synchronous */
        if (av > 0.01f && !open && whisper < 0.5f) noise *= 0.6f;
        src += noise * ah * KL_ASP_LEVEL;

        /* cascade branch */
        casc = kl_reson_run(&s->np, src);
        casc = kl_anti_run(&s->nz, casc);
        if (s->cas_on[5]) casc = kl_reson_run(&s->cas[5], casc);
        if (s->cas_on[4]) casc = kl_reson_run(&s->cas[4], casc);
        if (s->cas_on[3]) casc = kl_reson_run(&s->cas[3], casc);
        casc = kl_reson_run(&s->cas[2], casc);
        casc = kl_reson_run(&s->cas[1], casc);
        casc = kl_reson_run(&s->cas[0], casc);

        /* parallel (frication) branch */
        par = 0.0f;
        if (af > 1e-5f) {
            float fn = noise * af * KL_FRIC_LEVEL;
            par = kl_reson_run(&s->fr1b, kl_reson_run(&s->fr1, fn)) * s->fr1_norm * a1 +
                  kl_reson_run(&s->fr2, fn) * s->fr2_norm * a2 + fn * byp;
        } else {
            kl_reson_run(&s->fr1b, kl_reson_run(&s->fr1, 0.0f));
            kl_reson_run(&s->fr2, 0.0f);
        }

        /* radiation: first difference, scaled to be rate independent */
        y = casc + par;
        {
            float d = (y - s->rad_prev) * rad_gain;
            s->rad_prev = y;
            y = d;
        }
        /* DC blocker */
        {
            float hp = y - s->dc_x + dc_r * s->dc_y;
            s->dc_x = y;
            s->dc_y = hp;
            y = hp;
        }
        y *= s->gain;
        /* gentle soft clip above 0.8 */
        if (y > 0.8f) y = 0.8f + 0.2f * tanhf((y - 0.8f) * 5.0f);
        else if (y < -0.8f) y = -0.8f - 0.2f * tanhf((-y - 0.8f) * 5.0f);
        out[i] = y;
    }
}

#undef KL_REF_RATE
#undef KL_VOICE_LEVEL
#undef KL_FRIC_LEVEL
#undef KL_ASP_LEVEL
