/*
 * Prosody and timeline construction.
 *
 *  tokens -> durations (Klatt-style rules) -> acoustic segments
 *         -> formant transitions (Holmes-style locus/dominance rules)
 *         -> pitch contour (declination, accents, phrase-final tunes)
 *         -> event timeline (words, phonemes, visemes)
 */
#include "ss_internal.h"
#include <math.h>
#include <string.h>

#define PRO_OFF_DB (-99.0f)
#define PRO_VIS_INHERIT 255

static float pro_db(float db) { return db <= PRO_OFF_DB ? 0.0f : powf(10.0f, db / 20.0f); }

typedef struct pro_ctx {
    ss_utterance *u;
    const ss_tokens *t;
    float fs;
    float rate;
    float *dur;       /* per token, ms */
    int *seg_first;   /* per token: first segment index */
    int *seg_count;
} pro_ctx;

static int pro_ms(const pro_ctx *c, float ms)
{
    int n = (int)(ms * c->fs * 0.001f + 0.5f);
    return n < 1 ? 1 : n;
}

/* ------------------------------------------------------------------------ */
/* Durations                                                                */
/* ------------------------------------------------------------------------ */

static float pro_pause_ms(int kind)
{
    switch (kind) {
    case SS_PAUSE_SHORT: return 190.0f;
    case SS_PAUSE_PERIOD: return 380.0f;
    case SS_PAUSE_QUESTION: return 380.0f;
    case SS_PAUSE_EXCLAIM: return 340.0f;
    case SS_PAUSE_LONG: return 520.0f;
    default: return 60.0f;
    }
}

static void pro_durations(pro_ctx *c)
{
    const ss_tokens *t = c->t;
    int i;
    /* phrase-final region: from the last vowel of each phrase to its end */
    int *final_from = (int *)SS_MALLOC(sizeof(int) * (size_t)(t->ntok + 1));
    if (!final_from) {
        for (i = 0; i < t->ntok; i++) c->dur[i] = (float)ss_phtab[t->tok[i].ph].inh_ms / c->rate;
        return;
    }
    {
        int ps = 0;
        while (ps < t->ntok) {
            int pe = ps, lv = -1, k;
            while (pe < t->ntok && t->tok[pe].ph != SS_PH_PAU) pe++;
            for (k = ps; k < pe; k++) if (ss_ph_is_vowel(t->tok[k].ph)) lv = k;
            for (k = ps; k < pe; k++) final_from[k] = lv < 0 ? pe : lv;
            if (pe < t->ntok) final_from[pe] = pe + 1;
            ps = pe + 1;
        }
    }

    for (i = 0; i < t->ntok; i++) {
        const ss_phtok *k = &t->tok[i];
        const ss_phinfo *p = &ss_phtab[k->ph];
        float inh = (float)p->inh_ms, mn = (float)p->min_ms, pct = 1.0f;
        int final_region = i >= final_from[i];
        int same_word_next = i + 1 < t->ntok && t->tok[i + 1].word == k->word && k->word >= 0;
        int nph = i + 1 < t->ntok ? t->tok[i + 1].ph : (int)SS_PH_PAU;
        int pph = i > 0 ? t->tok[i - 1].ph : (int)SS_PH_PAU;

        if (k->ph == SS_PH_PAU) {
            c->dur[i] = pro_pause_ms(k->stress) / c->rate;
            continue;
        }
        if (ss_ph_is_vowel(k->ph)) {
            if (k->flags & SS_TF_FUNCTION) pct *= 0.55f;
            else if (k->stress == 0) pct *= k->ph == SS_PH_AX ? 0.6f : 0.55f;
            else if (k->stress == 2) pct *= 0.8f;
            if (final_region) pct *= 1.45f;
            else if (same_word_next || !(k->flags & SS_TF_FUNCTION)) pct *= 0.88f;
            if (nph != SS_PH_PAU && !ss_ph_is_vowel(nph)) {
                int ncl = ss_ph_class(nph);
                if (!ss_ph_is_voiced(nph) && (ncl == SS_CL_STOP || ncl == SS_CL_AFFRIC)) pct *= 0.62f;
                else if (!ss_ph_is_voiced(nph) && ncl == SS_CL_FRIC) pct *= 0.85f;
                else if (ss_ph_is_voiced(nph) && ncl != SS_CL_NASAL && ncl != SS_CL_LIQUID &&
                         ncl != SS_CL_GLIDE) pct *= 1.15f;
            }
        } else {
            int nstress = 1, j;
            if (final_region) pct *= 1.3f;
            if ((pph != SS_PH_PAU && !ss_ph_is_vowel(pph)) || (nph != SS_PH_PAU && !ss_ph_is_vowel(nph)))
                pct *= 0.8f;
            for (j = i + 1; j < t->ntok && t->tok[j].word == k->word; j++) {
                if (ss_ph_is_vowel(t->tok[j].ph)) { nstress = t->tok[j].stress; break; }
            }
            if (nstress == 0 && !(k->flags & SS_TF_WORD_START)) pct *= 0.8f;
            if (k->flags & SS_TF_FUNCTION) pct *= 0.8f;
        }
        c->dur[i] = (mn + (inh - mn) * pct) / c->rate;
    }
    SS_FREE(final_from);
}

/* ------------------------------------------------------------------------ */
/* Segments                                                                 */
/* ------------------------------------------------------------------------ */

static ss_seg *pro_new_seg(pro_ctx *c, int len)
{
    ss_utterance *u = c->u;
    ss_seg *s = &u->seg[u->nseg++];
    memset(s, 0, sizeof(*s));
    s->start = u->length;
    s->len = len < 1 ? 1 : len;
    s->pa = s->na = -1;
    u->length += s->len;
    return s;
}

static void pro_set_formants(ss_seg *s, const ss_phinfo *p)
{
    int k;
    for (k = 0; k < 3; k++) {
        s->f0[SS_FP_F1 + k] = p->f[k];
        s->f1[SS_FP_F1 + k] = p->fe[k];
        s->f0[SS_FP_B1 + k] = p->b[k];
        s->f1[SS_FP_B1 + k] = p->b[k];
    }
    s->f0[SS_FP_NZ] = s->f1[SS_FP_NZ] = (p->cls == SS_CL_NASAL) ? 450.0f : 270.0f;
    s->anchor = 1;
    s->rank = p->rank;
    s->int_ms = p->int_ms;
    s->ext_ms = p->ext_ms;
    s->wdom = p->wdom;
}

static void pro_set_fric(ss_seg *s, int shape, float level)
{
    const ss_fric_shape *f = &ss_fric_shapes[shape];
    s->amp[SS_AP_AF] = level;
    s->amp[SS_AP_FR1F] = f->f1;
    s->amp[SS_AP_FR1B] = f->b1;
    s->amp[SS_AP_FR1A] = f->g1;
    s->amp[SS_AP_FR2F] = f->f2;
    s->amp[SS_AP_FR2B] = f->b2;
    s->amp[SS_AP_FR2A] = f->g2;
    s->amp[SS_AP_BYP] = f->bypass;
}

/* Next vowel's F2 (for velar loci and bursts), or a neutral value. */
static float pro_next_vowel_f2(const ss_tokens *t, int i)
{
    int j;
    for (j = i + 1; j < t->ntok && j <= i + 3; j++) {
        int ph = t->tok[j].ph;
        if (ph == SS_PH_PAU) break;
        if (ss_ph_is_vowel(ph)) return ss_phtab[ph].f[1];
    }
    for (j = i - 1; j >= 0 && j >= i - 2; j--) {
        int ph = t->tok[j].ph;
        if (ph == SS_PH_PAU) break;
        if (ss_ph_is_vowel(ph)) return ss_phtab[ph].fe[1];
    }
    return 1600.0f;
}

static float pro_clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

static void pro_segments(pro_ctx *c)
{
    const ss_tokens *t = c->t;
    ss_utterance *u = c->u;
    int i;

    /* lead-in silence lets the filters settle */
    {
        ss_seg *s = pro_new_seg(c, pro_ms(c, 12.0f));
        s->amp_in = 0;
    }

    for (i = 0; i < t->ntok; i++) {
        const ss_phtok *k = &t->tok[i];
        const ss_phinfo *p = &ss_phtab[k->ph];
        int total = pro_ms(c, c->dur[i]);
        int prev_ph = i > 0 ? t->tok[i - 1].ph : (int)SS_PH_PAU;
        int next_ph = i + 1 < t->ntok ? t->tok[i + 1].ph : (int)SS_PH_PAU;
        int prev_voiced = ss_ph_is_voiced(prev_ph);
        int next_sonorant = ss_ph_is_sonorant(next_ph);
        ss_seg *s;
        c->seg_first[i] = u->nseg;

        switch (p->cls) {
        case SS_CL_PAUSE:
            s = pro_new_seg(c, total);
            s->amp_in = pro_ms(c, 15.0f);
            break;

        case SS_CL_VOWEL:
        case SS_CL_DIPH:
        case SS_CL_LIQUID:
        case SS_CL_GLIDE:
        case SS_CL_NASAL: {
            s = pro_new_seg(c, total);
            pro_set_formants(s, p);
            s->amp[SS_AP_AV] = pro_db(p->av);
            if (prev_ph == SS_PH_PAU) s->amp_in = pro_ms(c, 18.0f);
            else if (prev_ph == SS_PH_HH || (ss_ph_class(prev_ph) == SS_CL_STOP && !prev_voiced))
                s->amp_in = pro_ms(c, 6.0f);
            else s->amp_in = pro_ms(c, 10.0f);
            break;
        }

        case SS_CL_FRIC:
            s = pro_new_seg(c, total);
            pro_set_formants(s, p);
            s->amp[SS_AP_AV] = pro_db(p->av);
            pro_set_fric(s, p->fric, pro_db(p->af));
            s->amp_in = pro_ms(c, (p->flags & SS_PF_VOICED) ? 10.0f : 16.0f);
            break;

        case SS_CL_ASP:
            s = pro_new_seg(c, total);
            s->amp[SS_AP_AH] = pro_db(p->ah);
            s->amp_in = pro_ms(c, 10.0f);
            break;

        case SS_CL_STOP: {
            int voiced = (p->flags & SS_PF_VOICED) != 0;
            int flap = 0;
            /* American flap: t/d between a vowel (or r) and an unstressed vowel */
            if ((k->ph == SS_PH_T || k->ph == SS_PH_D) && i > 0 && i + 1 < t->ntok &&
                (ss_ph_is_vowel(prev_ph) || prev_ph == SS_PH_R) && ss_ph_is_vowel(next_ph) &&
                t->tok[i + 1].stress == 0 && t->tok[i + 1].word == k->word) {
                flap = 1;
            }
            if (flap) {
                s = pro_new_seg(c, pro_ms(c, 26.0f / c->rate));
                pro_set_formants(s, &ss_phtab[SS_PH_D]);
                s->amp[SS_AP_AV] = pro_db(-14.0f);
                s->amp_in = pro_ms(c, 6.0f);
                break;
            }
            {
                float burst_ms = k->ph == SS_PH_K || k->ph == SS_PH_G ? 12.0f :
                                 k->ph == SS_PH_T || k->ph == SS_PH_D ? 9.0f : 6.0f;
                int full_release = next_sonorant;
                int burst = pro_ms(c, burst_ms);
                int closure = total - burst;
                float vf2 = pro_next_vowel_f2(t, i);
                float level = pro_db(p->af);
                int s_cluster = prev_ph == SS_PH_S;
                if (closure < pro_ms(c, 15.0f)) closure = pro_ms(c, 15.0f);

                /* closure */
                s = pro_new_seg(c, closure);
                pro_set_formants(s, p);
                if (p->flags & SS_PF_VELAR) {
                    float f2 = pro_clampf(vf2 + 150.0f, 1350.0f, 2350.0f);
                    s->f0[SS_FP_F2] = s->f1[SS_FP_F2] = f2;
                    s->f0[SS_FP_F3] = s->f1[SS_FP_F3] = f2 + 350.0f; /* velar pinch */
                }
                s->amp[SS_AP_AV] = (voiced && prev_voiced) ? pro_db(p->av) : 0.0f;
                s->amp_in = pro_ms(c, 8.0f);

                /* burst */
                s = pro_new_seg(c, burst);
                pro_set_fric(s, p->fric, full_release ? level : level * 0.5f);
                if (p->fric == SS_FR_BURST_K)
                    s->amp[SS_AP_FR1F] = pro_clampf(vf2 * 1.15f, 1500.0f, 3100.0f);
                s->amp[SS_AP_AV] = voiced ? pro_db(-22.0f) : 0.0f;
                s->amp_in = pro_ms(c, 0.5f);

                /* aspiration */
                if (!voiced && !s_cluster && (full_release || next_ph == SS_PH_PAU)) {
                    float asp_ms;
                    int nstress = i + 1 < t->ntok ? t->tok[i + 1].stress : 0;
                    if (!full_release) asp_ms = 20.0f;
                    else if (ss_ph_is_vowel(next_ph) && nstress == 1) asp_ms = 55.0f;
                    else asp_ms = 35.0f;
                    asp_ms /= (c->rate < 1.0f ? 1.0f : c->rate);
                    s = pro_new_seg(c, pro_ms(c, asp_ms));
                    s->amp[SS_AP_AH] = pro_db(-4.0f);
                    pro_set_fric(s, p->fric, level * 0.18f);
                    if (p->fric == SS_FR_BURST_K)
                        s->amp[SS_AP_FR1F] = pro_clampf(vf2 * 1.15f, 1500.0f, 3100.0f);
                    s->amp_in = pro_ms(c, 2.0f);
                }
            }
            break;
        }

        case SS_CL_AFFRIC: {
            int voiced = (p->flags & SS_PF_VOICED) != 0;
            int closure = (int)((float)total * 0.35f);
            int fric = total - closure;
            if (closure < pro_ms(c, 15.0f)) closure = pro_ms(c, 15.0f);
            s = pro_new_seg(c, closure);
            pro_set_formants(s, p);
            s->amp[SS_AP_AV] = (voiced && prev_voiced) ? pro_db(-24.0f) : 0.0f;
            s->amp_in = pro_ms(c, 8.0f);
            s = pro_new_seg(c, fric);
            pro_set_formants(s, p);
            s->rank = 5; /* the frication part blends like a fricative */
            s->amp[SS_AP_AV] = pro_db(p->av);
            pro_set_fric(s, p->fric, pro_db(p->af));
            s->amp_in = pro_ms(c, 2.0f);
            break;
        }
        default:
            s = pro_new_seg(c, total);
            break;
        }
        c->seg_count[i] = u->nseg - c->seg_first[i];
    }

    /* tail silence lets the resonators ring out */
    {
        ss_seg *s = pro_new_seg(c, pro_ms(c, 60.0f));
        s->amp_in = pro_ms(c, 15.0f);
    }
}

/* Segments without frication borrow the noise shape of the nearest fricative
 * so that filter frequencies never sweep audibly. */
static void pro_fill_fric(ss_utterance *u)
{
    int i, j;
    for (i = 0; i < u->nseg; i++) {
        ss_seg *s = &u->seg[i];
        const ss_seg *src = NULL;
        if (s->amp[SS_AP_AF] > 0.0f) continue;
        for (j = i + 1; j < u->nseg && j <= i + 4; j++) {
            if (u->seg[j].amp[SS_AP_AF] > 0.0f) { src = &u->seg[j]; break; }
        }
        if (!src) {
            for (j = i - 1; j >= 0 && j >= i - 4; j--) {
                if (u->seg[j].amp[SS_AP_AF] > 0.0f) { src = &u->seg[j]; break; }
            }
        }
        if (src) {
            s->amp[SS_AP_FR1F] = src->amp[SS_AP_FR1F];
            s->amp[SS_AP_FR1B] = src->amp[SS_AP_FR1B];
            s->amp[SS_AP_FR2F] = src->amp[SS_AP_FR2F];
            s->amp[SS_AP_FR2B] = src->amp[SS_AP_FR2B];
        } else {
            s->amp[SS_AP_FR1F] = 2500.0f;
            s->amp[SS_AP_FR1B] = 1000.0f;
            s->amp[SS_AP_FR2F] = 5000.0f;
            s->amp[SS_AP_FR2B] = 2000.0f;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Formant transitions                                                      */
/* ------------------------------------------------------------------------ */

static void pro_transitions(pro_ctx *c)
{
    ss_utterance *u = c->u;
    int i, last = -1;
    /* neighbour anchors for transparent segments */
    for (i = 0; i < u->nseg; i++) {
        if (u->seg[i].anchor) last = i;
        u->seg[i].pa = last;
    }
    last = -1;
    for (i = u->nseg - 1; i >= 0; i--) {
        if (u->seg[i].anchor) last = i;
        u->seg[i].na = last;
    }

    last = -1;
    for (i = 0; i < u->nseg; i++) {
        ss_seg *a, *b;
        int j, pause_between = 0, p;
        float da_ms, db_ms;
        int da, db, tb, a_in_portion, avail;
        if (!u->seg[i].anchor) continue;
        if (last < 0) { last = i; continue; }
        a = &u->seg[last];
        b = &u->seg[i];
        for (j = last + 1; j < i; j++) {
            /* a transparent segment with no sound at all is a pause */
            if (u->seg[j].amp[SS_AP_AH] <= 0.0f && u->seg[j].amp[SS_AP_AF] <= 0.0f &&
                u->seg[j].amp[SS_AP_AV] <= 0.0f && u->seg[j].len > (int)(0.1f * c->fs))
                pause_between = 1;
        }
        last = i;
        if (pause_between) continue;

        if (a->rank > b->rank) {
            da_ms = a->int_ms;
            db_ms = a->ext_ms;
            for (p = 0; p < SS_FP_COUNT; p++)
                b->in_v[p] = b->f0[p] + (a->f1[p] - b->f0[p]) * a->wdom;
        } else if (b->rank > a->rank) {
            da_ms = b->ext_ms;
            db_ms = b->int_ms;
            for (p = 0; p < SS_FP_COUNT; p++)
                b->in_v[p] = a->f1[p] + (b->f0[p] - a->f1[p]) * b->wdom;
        } else {
            da_ms = db_ms = 0.5f * (float)(a->int_ms + b->int_ms);
            if (da_ms < 20.0f) da_ms = db_ms = 20.0f;
            for (p = 0; p < SS_FP_COUNT; p++)
                b->in_v[p] = 0.5f * (a->f1[p] + b->f0[p]);
        }
        /* faster talking means faster transitions, but not linearly */
        if (c->rate > 1.0f) { da_ms /= sqrtf(c->rate); db_ms /= sqrtf(c->rate); }

        tb = a->start + a->len;
        da = pro_ms(c, da_ms);
        db = pro_ms(c, db_ms);
        a_in_portion = a->has_in ? (a->in_tb + a->in_d) - a->start : 0;
        if (a_in_portion < 0) a_in_portion = 0;
        avail = a->len - a_in_portion;
        if (da > (int)(0.6f * (float)a->len)) da = (int)(0.6f * (float)a->len);
        if (da > avail) da = avail;
        if (da < 1) da = 1;
        if (db > (int)(0.6f * (float)(b->start + b->len - tb))) db = (int)(0.6f * (float)(b->start + b->len - tb));
        if (db < 1) db = 1;

        a->has_out = 1;
        a->out_d = da;
        memcpy(a->out_v, b->in_v, sizeof(a->out_v));
        b->has_in = 1;
        b->in_tb = tb;
        b->in_d = db;
    }
}

/* ------------------------------------------------------------------------ */
/* Pitch                                                                    */
/* ------------------------------------------------------------------------ */

static void pro_add_pitch(ss_utterance *u, int t, float hz)
{
    if (hz < 30.0f) hz = 30.0f;
    if (u->npitch > 0 && u->pitch[u->npitch - 1].t >= t) {
        /* keep strictly increasing times */
        t = u->pitch[u->npitch - 1].t + 1;
    }
    u->pitch[u->npitch].t = t;
    u->pitch[u->npitch].hz = hz;
    u->npitch++;
}

static int pro_tok_start(const pro_ctx *c, int i) { return c->u->seg[c->seg_first[i]].start; }
static int pro_tok_end(const pro_ctx *c, int i)
{
    const ss_seg *s = &c->u->seg[c->seg_first[i] + c->seg_count[i] - 1];
    return s->start + s->len;
}

static void pro_pitch(pro_ctx *c)
{
    const ss_tokens *t = c->t;
    ss_utterance *u = c->u;
    float base = u->voice.pitch_hz;
    float range = u->voice.pitch_range;
    int ps = 0;
    pro_add_pitch(u, 0, base * (1.0f + 0.1f * range));

    while (ps < t->ntok) {
        int pe = ps, kind = SS_PAUSE_PERIOD, k, accent_idx = 0;
        int last_vowel = -1, last_accent = -1, t0, t1;
        int wh = 0;
        while (pe < t->ntok && t->tok[pe].ph != SS_PH_PAU) pe++;
        if (pe < t->ntok) kind = t->tok[pe].stress;
        if (pe == ps) { ps = pe + 1; continue; }
        wh = (t->tok[ps].flags & SS_TF_WH) != 0;
        if (kind == SS_PAUSE_QUESTION && wh) kind = SS_PAUSE_PERIOD; /* wh-questions fall */

        for (k = ps; k < pe; k++) {
            if (!ss_ph_is_vowel(t->tok[k].ph)) continue;
            last_vowel = k;
            if (t->tok[k].stress == 1 && !(t->tok[k].flags & SS_TF_FUNCTION)) {
                last_accent = k;
            }
        }
        if (last_vowel < 0) { ps = pe + 1; continue; }
        if (last_accent < 0) last_accent = last_vowel;
        t0 = pro_tok_start(c, ps);
        t1 = pro_tok_end(c, pe - 1);
        if (t1 <= t0) t1 = t0 + 1;

        for (k = ps; k < pe; k++) {
            int vs, ve, len;
            float x0, x1, d0, d1, acc = 0.0f;
            if (!ss_ph_is_vowel(t->tok[k].ph)) continue;
            vs = pro_tok_start(c, k);
            ve = pro_tok_end(c, k);
            len = ve - vs;
            x0 = (float)(vs - t0) / (float)(t1 - t0);
            x1 = (float)(ve - t0) / (float)(t1 - t0);
            /* declination line from +12% to -8% around the base pitch */
            d0 = base * (1.0f + range * (0.12f - 0.20f * x0));
            d1 = base * (1.0f + range * (0.12f - 0.20f * x1));
            if (t->tok[k].stress == 1 && !(t->tok[k].flags & SS_TF_FUNCTION)) {
                float strength = kind == SS_PAUSE_EXCLAIM ? 0.34f : 0.22f;
                acc = range * strength * base * powf(0.82f, (float)accent_idx);
                accent_idx++;
            } else if (t->tok[k].stress == 2) {
                acc = range * 0.06f * base;
            }

            if (k == last_accent && (kind == SS_PAUSE_QUESTION)) {
                /* yes/no question: low accent, then a rise to the end */
                pro_add_pitch(u, vs, d0 - range * 0.04f * base);
                if (k == last_vowel) {
                    pro_add_pitch(u, vs + len / 3, d0 - range * 0.02f * base);
                    pro_add_pitch(u, ve, base * (1.0f + 0.5f * range));
                } else {
                    pro_add_pitch(u, ve, d1);
                }
            } else if (k > last_accent && kind == SS_PAUSE_QUESTION) {
                float x = (float)(ve - pro_tok_start(c, last_accent)) /
                          (float)(t1 - pro_tok_start(c, last_accent) + 1);
                pro_add_pitch(u, vs + len / 2, base * (1.0f + 0.5f * range * x));
                if (k == last_vowel) pro_add_pitch(u, ve, base * (1.0f + 0.55f * range));
            } else if (k == last_vowel) {
                /* phrase-final tune */
                float end;
                if (kind == SS_PAUSE_SHORT) end = base * (1.0f + 0.04f * range);
                else if (kind == SS_PAUSE_LONG) end = d1;
                else end = base * (1.0f - 0.24f * range);
                if (acc > 0.0f) {
                    pro_add_pitch(u, vs, d0 + acc * 0.6f);
                    pro_add_pitch(u, vs + len / 4, d0 + acc);
                } else {
                    pro_add_pitch(u, vs, d0);
                }
                pro_add_pitch(u, ve, end);
            } else if (acc > 0.0f) {
                pro_add_pitch(u, vs, d0 + acc * 0.5f);
                pro_add_pitch(u, vs + len * 2 / 5, d0 + acc);
                pro_add_pitch(u, ve, d1 + acc * 0.45f);
            } else {
                pro_add_pitch(u, vs + len / 2, 0.5f * (d0 + d1) - range * 0.03f * base);
            }
        }
        ps = pe + 1;
    }
    pro_add_pitch(u, u->length, u->npitch ? u->pitch[u->npitch - 1].hz : base);
}

/* ------------------------------------------------------------------------ */
/* Events                                                                   */
/* ------------------------------------------------------------------------ */

static ss_event *pro_add_event(ss_utterance *u, int type, int sample)
{
    ss_event *e = &u->ev[u->nev++];
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->time = (float)sample / (float)u->sample_rate;
    e->word_index = -1;
    e->phoneme = -1;
    e->viseme = -1;
    return e;
}

static int pro_viseme_of(const ss_tokens *t, int i)
{
    int v = ss_phtab[t->tok[i].ph].viseme;
    if (v == PRO_VIS_INHERIT) {
        int j;
        for (j = i + 1; j < t->ntok; j++) {
            v = ss_phtab[t->tok[j].ph].viseme;
            if (v != PRO_VIS_INHERIT) return v == SS_VISEME_SIL ? SS_VISEME_AA : v;
        }
        return SS_VISEME_AA;
    }
    return v;
}

static void pro_events(pro_ctx *c)
{
    const ss_tokens *t = c->t;
    ss_utterance *u = c->u;
    int i, last_word = -1, last_vis = -1;
    ss_event *vis_ev = NULL;
    pro_add_event(u, SS_EVENT_UTTERANCE_START, 0)->duration = (float)u->length / (float)u->sample_rate;
    for (i = 0; i < t->ntok; i++) {
        const ss_phtok *k = &t->tok[i];
        int st = pro_tok_start(c, i), en = pro_tok_end(c, i), v;
        ss_event *e;
        if (k->word >= 0 && k->word != last_word) {
            int j = i, wend;
            while (j + 1 < t->ntok && t->tok[j + 1].word == k->word) j++;
            wend = pro_tok_end(c, j);
            e = pro_add_event(u, SS_EVENT_WORD, st);
            e->word_index = k->word;
            e->text_offset = t->word[k->word].text_offset;
            e->text_length = t->word[k->word].text_length;
            e->duration = (float)(wend - st) / (float)u->sample_rate;
            last_word = k->word;
        }
        if (k->ph != SS_PH_PAU) {
            e = pro_add_event(u, SS_EVENT_PHONEME, st);
            e->phoneme = k->ph;
            e->viseme = pro_viseme_of(t, i);
            e->word_index = k->word;
            e->duration = (float)(en - st) / (float)u->sample_rate;
        }
        v = k->ph == SS_PH_PAU ? SS_VISEME_SIL : pro_viseme_of(t, i);
        if (v != last_vis) {
            if (vis_ev) vis_ev->duration = (float)st / (float)u->sample_rate - vis_ev->time;
            vis_ev = pro_add_event(u, SS_EVENT_VISEME, st);
            vis_ev->viseme = v;
            vis_ev->word_index = k->word;
            last_vis = v;
        }
    }
    if (t->ntok > 0) {
        int en = pro_tok_end(c, t->ntok - 1);
        if (vis_ev && last_vis != SS_VISEME_SIL) {
            vis_ev->duration = (float)en / (float)u->sample_rate - vis_ev->time;
            vis_ev = pro_add_event(u, SS_EVENT_VISEME, en);
            vis_ev->viseme = SS_VISEME_SIL;
        }
        if (vis_ev) vis_ev->duration = (float)u->length / (float)u->sample_rate - vis_ev->time;
    }
    pro_add_event(u, SS_EVENT_UTTERANCE_END, u->length);
}

/* ------------------------------------------------------------------------ */

int ss_build_utterance(ss_utterance *u, const ss_tokens *t)
{
    pro_ctx c;
    int nvowel = 0, i, ok = 1;
    memset(&c, 0, sizeof(c));
    c.u = u;
    c.t = t;
    c.fs = (float)u->sample_rate;
    c.rate = u->voice.rate;
    for (i = 0; i < t->ntok; i++) if (ss_ph_is_vowel(t->tok[i].ph)) nvowel++;

    u->seg = (ss_seg *)SS_MALLOC(sizeof(ss_seg) * (size_t)(t->ntok * 3 + 4));
    u->pitch = (ss_pitchpt *)SS_MALLOC(sizeof(ss_pitchpt) * (size_t)(nvowel * 3 + t->ntok + 4));
    u->ev = (ss_event *)SS_MALLOC(sizeof(ss_event) * (size_t)(t->ntok * 3 + 4));
    c.dur = (float *)SS_MALLOC(sizeof(float) * (size_t)(t->ntok + 1));
    c.seg_first = (int *)SS_MALLOC(sizeof(int) * (size_t)(t->ntok + 1));
    c.seg_count = (int *)SS_MALLOC(sizeof(int) * (size_t)(t->ntok + 1));
    if (!u->seg || !u->pitch || !u->ev || !c.dur || !c.seg_first || !c.seg_count) {
        ok = 0;
    } else {
        u->nseg = u->npitch = u->nev = 0;
        u->length = 0;
        pro_durations(&c);
        pro_segments(&c);
        pro_fill_fric(u);
        pro_transitions(&c);
        pro_pitch(&c);
        pro_events(&c);
    }
    SS_FREE(c.dur);
    SS_FREE(c.seg_first);
    SS_FREE(c.seg_count);
    return ok ? 0 : -1;
}

/* ------------------------------------------------------------------------ */
/* Frame evaluation                                                         */
/* ------------------------------------------------------------------------ */

static const float pro_default_form[SS_FP_COUNT] = {500, 1500, 2500, 80, 90, 150, 270};

static void pro_anchor_value(const ss_seg *s, int t, float *v)
{
    int p;
    float pos = s->len > 1 ? (float)(t - s->start) / (float)s->len : 0.0f;
    /* diphthongs: hold, glide, hold */
    float g = (pos - 0.2f) / 0.65f;
    g = g < 0.0f ? 0.0f : g > 1.0f ? 1.0f : g;
    g = g * g * (3.0f - 2.0f * g);
    for (p = 0; p < SS_FP_COUNT; p++) v[p] = s->f0[p] + (s->f1[p] - s->f0[p]) * g;
}

void ss_eval_frame(const ss_utterance *u, int t, ss_cursor *c, ss_frame *out)
{
    int s = c->seg, p;
    const ss_seg *S;
    if (u->nseg == 0) {
        memset(out, 0, sizeof(*out));
        memcpy(out->form, pro_default_form, sizeof(out->form));
        out->f0 = u->voice.pitch_hz;
        return;
    }
    if (t < 0) t = 0;
    if (s < 0 || s >= u->nseg) s = 0;
    while (s > 0 && u->seg[s].start > t) s--;
    while (s + 1 < u->nseg && u->seg[s + 1].start <= t) s++;
    c->seg = s;
    S = &u->seg[s];

    /* amplitudes */
    {
        int x = t - S->start;
        if (S->amp_in > 0 && x < S->amp_in && s > 0) {
            const ss_seg *P = &u->seg[s - 1];
            float w = (float)x / (float)S->amp_in;
            for (p = 0; p < SS_AP_COUNT; p++) out->amp[p] = P->amp[p] + (S->amp[p] - P->amp[p]) * w;
        } else {
            memcpy(out->amp, S->amp, sizeof(out->amp));
        }
    }

    /* formants */
    if (S->anchor) {
        pro_anchor_value(S, t, out->form);
        if (S->has_in) {
            int d = t - S->in_tb;
            if (d < S->in_d) {
                float w = (float)d / (float)S->in_d, e;
                if (w < 0.0f) w = 0.0f;
                e = 1.0f - (1.0f - w) * (1.0f - w);
                for (p = 0; p < SS_FP_COUNT; p++) out->form[p] = S->in_v[p] + (out->form[p] - S->in_v[p]) * e;
            }
        }
        if (S->has_out) {
            int d0 = S->start + S->len - S->out_d;
            if (t > d0) {
                float w = (float)(t - d0) / (float)S->out_d, e;
                if (w > 1.0f) w = 1.0f;
                e = w * w;
                for (p = 0; p < SS_FP_COUNT; p++) out->form[p] += (S->out_v[p] - out->form[p]) * e;
            }
        }
    } else if (S->na >= 0) {
        const ss_seg *N = &u->seg[S->na];
        if (N->has_in && N->in_tb <= t) {
            float w = (float)(t - N->in_tb) / (float)N->in_d, e;
            if (w > 1.0f) w = 1.0f;
            e = 1.0f - (1.0f - w) * (1.0f - w);
            for (p = 0; p < SS_FP_COUNT; p++) out->form[p] = N->in_v[p] + (N->f0[p] - N->in_v[p]) * e;
        } else {
            memcpy(out->form, N->f0, sizeof(out->form));
        }
    } else if (S->pa >= 0) {
        memcpy(out->form, u->seg[S->pa].f1, sizeof(out->form));
    } else {
        memcpy(out->form, pro_default_form, sizeof(out->form));
    }

    /* pitch */
    {
        int k = c->pitch;
        if (u->npitch == 0) {
            out->f0 = u->voice.pitch_hz;
        } else {
            if (k < 0 || k >= u->npitch) k = 0;
            while (k > 0 && u->pitch[k].t > t) k--;
            while (k + 1 < u->npitch && u->pitch[k + 1].t <= t) k++;
            c->pitch = k;
            if (k + 1 >= u->npitch || t <= u->pitch[k].t) {
                out->f0 = u->pitch[k].hz;
            } else {
                const ss_pitchpt *a = &u->pitch[k], *b = &u->pitch[k + 1];
                float w = (float)(t - a->t) / (float)(b->t - a->t);
                out->f0 = a->hz + (b->hz - a->hz) * w;
            }
        }
    }
}

#undef PRO_OFF_DB
#undef PRO_VIS_INHERIT
