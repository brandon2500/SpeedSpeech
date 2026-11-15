/*
 * Public API: voices, utterances, streaming engines and one-shot helpers.
 */
#include "ss_internal.h"
#include <math.h>
#include <string.h>

#define ENG_EVQ 256

struct ss_engine {
    int sample_rate;
    ss_synth syn;
    ss_utterance *head, *tail;
    int next_id;
    /* playback of head */
    int started;
    int pos;
    ss_cursor cur;
    int ev_index;
    int frame_left;
    /* stop fade */
    int fading;
    float fade_gain, fade_step;
    /* event ring */
    ss_event evq[ENG_EVQ];
    int evr, evw;
};

/* ------------------------------------------------------------------------ */
/* Voices                                                                   */
/* ------------------------------------------------------------------------ */

void ss_voice_preset(ss_voice *v, int preset)
{
    if (!v) return;
    memset(v, 0, sizeof(*v));
    v->pitch_hz = 110.0f;
    v->pitch_range = 1.0f;
    v->rate = 1.0f;
    v->formant_shift = 1.0f;
    v->breathiness = 0.05f;
    v->roughness = 0.02f;
    v->volume = 1.0f;
    v->seed = 0x5EEDu;
    switch (preset) {
    case SS_PRESET_FEMALE:
        v->pitch_hz = 205.0f; v->pitch_range = 1.15f; v->rate = 1.05f;
        v->formant_shift = 1.17f; v->breathiness = 0.15f; v->roughness = 0.0f;
        break;
    case SS_PRESET_CHILD:
        v->pitch_hz = 290.0f; v->pitch_range = 1.3f; v->rate = 1.08f;
        v->formant_shift = 1.35f; v->breathiness = 0.1f; v->roughness = 0.0f;
        break;
    case SS_PRESET_ROBOT:
        v->pitch_hz = 95.0f; v->pitch_range = 0.0f; v->rate = 0.92f;
        v->breathiness = 0.0f; v->roughness = 0.0f; v->buzz = 1.0f;
        break;
    case SS_PRESET_GIANT:
        v->pitch_hz = 62.0f; v->pitch_range = 0.7f; v->rate = 0.78f;
        v->formant_shift = 0.8f; v->breathiness = 0.1f; v->roughness = 0.25f;
        break;
    case SS_PRESET_WHISPER:
        v->pitch_hz = 110.0f; v->pitch_range = 0.8f; v->rate = 0.95f;
        v->formant_shift = 1.05f; v->whisper = 1.0f; v->volume = 1.1f;
        break;
    case SS_PRESET_GOBLIN:
        v->pitch_hz = 175.0f; v->pitch_range = 1.6f; v->rate = 1.25f;
        v->formant_shift = 1.25f; v->breathiness = 0.2f; v->roughness = 0.4f;
        break;
    case SS_PRESET_ELDER:
        v->pitch_hz = 100.0f; v->pitch_range = 0.8f; v->rate = 0.82f;
        v->formant_shift = 1.02f; v->breathiness = 0.3f; v->roughness = 0.35f;
        v->vibrato_hz = 5.5f; v->vibrato_depth = 0.25f;
        break;
    default:
        break;
    }
}

static float eng_clamp(float x, float lo, float hi, float dflt)
{
    if (!(x == x)) return dflt; /* NaN */
    return x < lo ? lo : x > hi ? hi : x;
}

static void eng_sanitize(ss_voice *v)
{
    v->pitch_hz = eng_clamp(v->pitch_hz, 30.0f, 1000.0f, 110.0f);
    v->pitch_range = eng_clamp(v->pitch_range, 0.0f, 4.0f, 1.0f);
    v->rate = eng_clamp(v->rate, 0.2f, 5.0f, 1.0f);
    v->formant_shift = eng_clamp(v->formant_shift, 0.5f, 2.0f, 1.0f);
    v->breathiness = eng_clamp(v->breathiness, 0.0f, 1.0f, 0.0f);
    v->roughness = eng_clamp(v->roughness, 0.0f, 1.0f, 0.0f);
    v->whisper = eng_clamp(v->whisper, 0.0f, 1.0f, 0.0f);
    v->buzz = eng_clamp(v->buzz, 0.0f, 1.0f, 0.0f);
    v->volume = eng_clamp(v->volume, 0.0f, 8.0f, 1.0f);
    v->vibrato_hz = eng_clamp(v->vibrato_hz, 0.0f, 30.0f, 0.0f);
    v->vibrato_depth = eng_clamp(v->vibrato_depth, 0.0f, 12.0f, 0.0f);
}

/* ------------------------------------------------------------------------ */
/* Utterances                                                               */
/* ------------------------------------------------------------------------ */

ss_utterance *ss_utterance_create(const char *text, const ss_voice *voice, int sample_rate, int flags)
{
    ss_utterance *u;
    ss_tokens t;
    int need;
    if (sample_rate < 4000 || sample_rate > 384000) return NULL;
    u = (ss_utterance *)SS_MALLOC(sizeof(ss_utterance));
    if (!u) return NULL;
    memset(u, 0, sizeof(*u));
    u->sample_rate = sample_rate;
    if (voice) u->voice = *voice;
    else ss_voice_preset(&u->voice, SS_PRESET_DEFAULT);
    eng_sanitize(&u->voice);

    ss_tokens_init(&t);
    ss_text_process(text ? text : "", flags, &t);
    if (t.failed || ss_build_utterance(u, &t) != 0) {
        ss_tokens_free(&t);
        ss_utterance_destroy(u);
        return NULL;
    }
    need = ss_format_phonemes(&t, NULL, 0);
    u->phonemes = (char *)SS_MALLOC((size_t)need + 1);
    if (u->phonemes) {
        ss_format_phonemes(&t, u->phonemes, need + 1);
        u->phonemes_len = need;
    }
    ss_tokens_free(&t);
    return u;
}

void ss_utterance_destroy(ss_utterance *u)
{
    if (!u) return;
    SS_FREE(u->seg);
    SS_FREE(u->pitch);
    SS_FREE(u->ev);
    SS_FREE(u->phonemes);
    SS_FREE(u);
}

int ss_utterance_length(const ss_utterance *u) { return u ? u->length : 0; }
int ss_utterance_event_count(const ss_utterance *u) { return u ? u->nev : 0; }

int ss_utterance_get_event(const ss_utterance *u, int index, ss_event *out)
{
    if (!u || !out || index < 0 || index >= u->nev) return 0;
    *out = u->ev[index];
    return 1;
}

int ss_utterance_phonemes(const ss_utterance *u, char *out, int cap)
{
    int i;
    if (!u) {
        if (out && cap > 0) out[0] = 0;
        return 0;
    }
    if (out && cap > 0) {
        for (i = 0; i < u->phonemes_len && i < cap - 1; i++) out[i] = u->phonemes[i];
        out[i] = 0;
    }
    return u->phonemes_len;
}

/* ------------------------------------------------------------------------ */
/* Engine                                                                   */
/* ------------------------------------------------------------------------ */

ss_engine *ss_engine_create(int sample_rate)
{
    ss_engine *e;
    if (sample_rate < 4000 || sample_rate > 384000) return NULL;
    e = (ss_engine *)SS_MALLOC(sizeof(ss_engine));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->sample_rate = sample_rate;
    e->next_id = 1;
    ss_synth_init(&e->syn, sample_rate);
    return e;
}

static void eng_drop_all(ss_engine *e)
{
    ss_utterance *u = e->head;
    while (u) {
        ss_utterance *n = u->next;
        ss_utterance_destroy(u);
        u = n;
    }
    e->head = e->tail = NULL;
    e->started = 0;
}

void ss_engine_destroy(ss_engine *e)
{
    if (!e) return;
    eng_drop_all(e);
    SS_FREE(e);
}

int ss_engine_sample_rate(const ss_engine *e) { return e ? e->sample_rate : 0; }

int ss_engine_queue(ss_engine *e, ss_utterance *u)
{
    if (!u) return 0;
    if (!e) {
        ss_utterance_destroy(u);
        return 0;
    }
    u->id = e->next_id++;
    if (e->next_id <= 0) e->next_id = 1;
    u->next = NULL;
    if (e->tail) e->tail->next = u;
    else e->head = u;
    e->tail = u;
    return u->id;
}

int ss_engine_speak(ss_engine *e, const char *text, const ss_voice *voice)
{
    ss_utterance *u;
    if (!e) return 0;
    u = ss_utterance_create(text, voice, e->sample_rate, SS_INPUT_TEXT);
    if (!u) return 0;
    return ss_engine_queue(e, u);
}

void ss_engine_stop(ss_engine *e)
{
    if (!e || !e->head) return;
    if (!e->started) {
        eng_drop_all(e);
        return;
    }
    if (!e->fading) {
        e->fading = 1;
        e->fade_gain = 1.0f;
        e->fade_step = 1.0f / (0.008f * (float)e->sample_rate);
    }
}

int ss_engine_is_speaking(const ss_engine *e) { return e && e->head != NULL; }

static void eng_push_event(ss_engine *e, const ss_event *ev)
{
    int nw = (e->evw + 1) % ENG_EVQ;
    if (nw == e->evr) e->evr = (e->evr + 1) % ENG_EVQ; /* full: drop oldest */
    e->evq[e->evw] = *ev;
    e->evw = nw;
}

int ss_engine_poll_event(ss_engine *e, ss_event *out)
{
    if (!e || e->evr == e->evw) return 0;
    if (out) *out = e->evq[e->evr];
    e->evr = (e->evr + 1) % ENG_EVQ;
    return 1;
}

static int eng_event_sample(const ss_utterance *u, const ss_event *ev)
{
    return (int)(ev->time * (float)u->sample_rate + 0.5f);
}

/* Emit the head utterance's events that fall before sample `until`. */
static void eng_emit_events(ss_engine *e, int until, int block_base)
{
    ss_utterance *u = e->head;
    while (e->ev_index < u->nev) {
        ss_event ev = u->ev[e->ev_index];
        int at = eng_event_sample(u, &ev);
        if (at >= until) break;
        ev.utterance_id = u->id;
        ev.block_offset = block_base + (at > e->pos ? at - e->pos : 0);
        eng_push_event(e, &ev);
        e->ev_index++;
    }
}

static void eng_finish_head(ss_engine *e)
{
    ss_utterance *u = e->head;
    e->head = u->next;
    if (!e->head) e->tail = NULL;
    ss_utterance_destroy(u);
    e->started = 0;
}

int ss_engine_render(ss_engine *e, float *out, int frames)
{
    int written = 0, active = 0;
    if (!e || !out || frames <= 0) return 0;
    while (written < frames) {
        ss_utterance *u = e->head;
        int n, i;
        if (!u) {
            memset(out + written, 0, sizeof(float) * (size_t)(frames - written));
            break;
        }
        if (!e->started) {
            ss_synth_set_voice(&e->syn, &u->voice);
            e->started = 1;
            e->pos = 0;
            e->cur.seg = e->cur.pitch = 0;
            e->ev_index = 0;
            e->frame_left = 0;
        }
        if (e->frame_left <= 0) {
            ss_frame f;
            ss_eval_frame(u, e->pos + e->syn.frame_len, &e->cur, &f);
            ss_synth_frame(&e->syn, &f);
            e->frame_left = e->syn.frame_len;
        }
        n = e->frame_left;
        if (n > frames - written) n = frames - written;
        if (n > u->length - e->pos) n = u->length - e->pos;
        if (n > 0) {
            eng_emit_events(e, e->pos + n, written);
            ss_synth_run(&e->syn, out + written, n);
            if (e->fading) {
                for (i = 0; i < n; i++) {
                    out[written + i] *= e->fade_gain;
                    e->fade_gain -= e->fade_step;
                    if (e->fade_gain < 0.0f) e->fade_gain = 0.0f;
                }
            }
            e->pos += n;
            written += n;
            active += n;
            e->frame_left -= n;
        }
        if (e->fading && e->fade_gain <= 0.0f) {
            ss_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SS_EVENT_UTTERANCE_END;
            ev.utterance_id = u->id;
            ev.block_offset = written;
            ev.time = (float)e->pos / (float)u->sample_rate;
            ev.word_index = ev.phoneme = ev.viseme = -1;
            eng_push_event(e, &ev);
            eng_drop_all(e);
            ss_synth_reset(&e->syn);
            e->fading = 0;
            continue;
        }
        if (e->pos >= u->length) {
            eng_emit_events(e, 0x7fffffff, written);
            eng_finish_head(e);
        }
    }
    return active;
}

int ss_engine_render_interleaved(ss_engine *e, float *out, int frames, int channels)
{
    float tmp[256];
    int done = 0, active = 0;
    if (!e || !out || frames <= 0 || channels <= 0) return 0;
    if (channels == 1) return ss_engine_render(e, out, frames);
    while (done < frames) {
        int n = frames - done, i, c;
        if (n > 256) n = 256;
        active += ss_engine_render(e, tmp, n);
        for (i = 0; i < n; i++) {
            float *o = out + (size_t)(done + i) * (size_t)channels;
            for (c = 0; c < channels; c++) o[c] = tmp[i];
        }
        done += n;
    }
    return active;
}

/* ------------------------------------------------------------------------ */
/* One-shot helpers                                                         */
/* ------------------------------------------------------------------------ */

int ss_synthesize(const char *text, const ss_voice *voice, int sample_rate, float **out_samples)
{
    ss_engine *e;
    ss_utterance *u;
    float *buf;
    int len;
    if (!out_samples) return -1;
    *out_samples = NULL;
    u = ss_utterance_create(text, voice, sample_rate, SS_INPUT_TEXT);
    if (!u) return -1;
    len = u->length;
    e = ss_engine_create(sample_rate);
    buf = (float *)SS_MALLOC(sizeof(float) * (size_t)(len > 0 ? len : 1));
    if (!e || !buf) {
        ss_utterance_destroy(u);
        ss_engine_destroy(e);
        SS_FREE(buf);
        return -1;
    }
    ss_engine_queue(e, u);
    ss_engine_render(e, buf, len);
    ss_engine_destroy(e);
    *out_samples = buf;
    return len;
}

void ss_free(void *p) { SS_FREE(p); }

int ss_text_to_phonemes(const char *text, char *out, int cap)
{
    ss_tokens t;
    int n;
    ss_tokens_init(&t);
    ss_text_process(text ? text : "", SS_INPUT_TEXT, &t);
    n = ss_format_phonemes(&t, out, cap);
    ss_tokens_free(&t);
    return n;
}

const char *ss_version_string(void)
{
    return "0.1.0";
}

#undef ENG_EVQ
