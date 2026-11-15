/* GENERATED FILE - do not edit. Produced by tools/amalgamate.py from core/.
 * SpeedSpeech (MIT License) - https://github.com/brandon2500/SpeedSpeech */
#include "speedspeech.h"

/* ---- ss_internal.h ---- */
/*
 * SpeedSpeech internal definitions. Not part of the public API.
 *
 * Every source file in core/src must compile as C99 *and* as C++ (the Unreal
 * plugin builds them as C++), and all files must be able to share a single
 * translation unit (amalgamation): file-local statics carry a per-file prefix.
 */
#ifndef SS_INTERNAL_H
#define SS_INTERNAL_H

#include <stddef.h>

#ifndef SS_MALLOC
#  include <stdlib.h>
#  define SS_MALLOC(sz) malloc(sz)
#  define SS_REALLOC(p, sz) realloc((p), (sz))
#  define SS_FREE(p) free(p)
#endif
#ifndef SS_REALLOC
#  error "Define SS_REALLOC together with SS_MALLOC / SS_FREE"
#endif

#define SS_PI 3.14159265358979323846f

/* ------------------------------------------------------------------------ */
/* Phonemes                                                                 */
/* ------------------------------------------------------------------------ */

enum {
    SS_PH_PAU = 0,
    /* vowels */
    SS_PH_AA, SS_PH_AE, SS_PH_AH, SS_PH_AO, SS_PH_AW, SS_PH_AX, SS_PH_AY,
    SS_PH_EH, SS_PH_ER, SS_PH_EY, SS_PH_IH, SS_PH_IY, SS_PH_OW, SS_PH_OY,
    SS_PH_UH, SS_PH_UW,
    /* consonants */
    SS_PH_B, SS_PH_CH, SS_PH_D, SS_PH_DH, SS_PH_F, SS_PH_G, SS_PH_HH,
    SS_PH_JH, SS_PH_K, SS_PH_L, SS_PH_M, SS_PH_N, SS_PH_NG, SS_PH_P,
    SS_PH_R, SS_PH_S, SS_PH_SH, SS_PH_T, SS_PH_TH, SS_PH_V, SS_PH_W,
    SS_PH_Y, SS_PH_Z, SS_PH_ZH,
    SS_PH_COUNT
};

enum {
    SS_CL_PAUSE = 0,
    SS_CL_VOWEL,
    SS_CL_DIPH,
    SS_CL_STOP,
    SS_CL_AFFRIC,
    SS_CL_FRIC,
    SS_CL_ASP,
    SS_CL_NASAL,
    SS_CL_LIQUID,
    SS_CL_GLIDE
};

#define SS_PF_VOICED 1u
#define SS_PF_VELAR  2u
#define SS_PF_LABIAL 4u

/* Noise spectrum shapes used for frication and bursts. */
enum {
    SS_FR_NONE = 0,
    SS_FR_S,
    SS_FR_SH,
    SS_FR_F,
    SS_FR_TH,
    SS_FR_BURST_P,
    SS_FR_BURST_T,
    SS_FR_BURST_K,
    SS_FR_COUNT
};

typedef struct ss_fric_shape {
    float f1, b1, g1; /* first noise resonance: Hz, bandwidth Hz, linear gain */
    float f2, b2, g2; /* second noise resonance */
    float bypass;     /* unfiltered (flat) noise gain */
} ss_fric_shape;

typedef struct ss_phinfo {
    const char *name;
    unsigned char cls;
    unsigned char flags;
    unsigned char viseme;
    unsigned char fric;
    short inh_ms, min_ms;   /* inherent / minimum duration (Klatt-style) */
    float f[3], b[3];       /* formant targets (start) */
    float fe[3];            /* formant targets (end, for diphthongs) */
    float av, af, ah;       /* voicing / frication / aspiration level, dB re vowel (<= -99 = off) */
    unsigned char rank;     /* dominance in formant transitions */
    unsigned char int_ms;   /* transition length inside this phoneme */
    unsigned char ext_ms;   /* transition length it imposes on neighbours */
    float wdom;             /* locus weight at the boundary when dominant */
} ss_phinfo;

extern const ss_phinfo ss_phtab[SS_PH_COUNT];
extern const ss_fric_shape ss_fric_shapes[SS_FR_COUNT];

int ss_phoneme_from_name(const char *name, int len); /* -1 if unknown */
int ss_ph_is_vowel(int ph);
int ss_ph_is_voiced(int ph);
int ss_ph_is_sonorant(int ph);
int ss_ph_class(int ph);

/* ------------------------------------------------------------------------ */
/* Front end: text -> phoneme tokens                                        */
/* ------------------------------------------------------------------------ */

/* Pause kinds (stored in ss_phtok.stress for SS_PH_PAU tokens). */
enum {
    SS_PAUSE_NONE = 0, /* word boundary without a pause */
    SS_PAUSE_SHORT,    /* , ; : and dashes */
    SS_PAUSE_PERIOD,
    SS_PAUSE_QUESTION,
    SS_PAUSE_EXCLAIM,
    SS_PAUSE_LONG      /* ellipsis */
};

#define SS_TF_WORD_START 1u
#define SS_TF_FUNCTION   2u   /* function word: no pitch accent */
#define SS_TF_WH         4u   /* first word of the sentence is a wh-word */

typedef struct ss_phtok {
    unsigned char ph;
    signed char stress;  /* vowels: 0,1,2. pauses: SS_PAUSE_* */
    unsigned char flags;
    unsigned char pad;
    int word;            /* index into word table, -1 for pauses */
} ss_phtok;

typedef struct ss_word {
    int text_offset, text_length;
} ss_word;

typedef struct ss_tokens {
    ss_phtok *tok;
    int ntok, captok;
    ss_word *word;
    int nword, capword;
    int failed; /* allocation failure */
} ss_tokens;

void ss_tokens_init(ss_tokens *t);
void ss_tokens_free(ss_tokens *t);
void ss_tokens_push(ss_tokens *t, int ph, int stress, unsigned flags, int word);
int  ss_tokens_add_word(ss_tokens *t, int offset, int length);

/* Convert input into tokens. */
void ss_text_process(const char *text, int flags, ss_tokens *out);

/* Parse an ARPAbet string ("HH AH0 L OW1"), appending phoneme tokens for one
 * word. Returns the number of phonemes appended. */
int ss_parse_arpabet(const char *s, int len, ss_tokens *out, int word, unsigned flags);

/* Format tokens as "HH AH0 L OW1 | W ER1 L D ." (snprintf semantics). */
int ss_format_phonemes(const ss_tokens *t, char *out, int cap);

/* Dictionary lookup: lowercase ASCII word. Returns ARPAbet string or NULL. */
const char *ss_lexicon_lookup(const char *word, int len);
int ss_lexicon_size(void);
const char *ss_lexicon_word(int i);

/* Letter-to-sound rules: uppercase ASCII word -> ARPAbet string (no stress).
 * Returns length written (excluding terminator), truncating at cap. */
int ss_rules_translate(const char *upper_word, int len, char *out, int cap);

/* ------------------------------------------------------------------------ */
/* Utterance timeline                                                       */
/* ------------------------------------------------------------------------ */

/* Formant-group parameters: interpolated with Holmes-style locus rules. */
enum { SS_FP_F1 = 0, SS_FP_F2, SS_FP_F3, SS_FP_B1, SS_FP_B2, SS_FP_B3, SS_FP_NZ, SS_FP_COUNT };
/* Amplitude-group parameters: short linear ramps at segment starts. */
enum {
    SS_AP_AV = 0, SS_AP_AH, SS_AP_AF,
    SS_AP_FR1F, SS_AP_FR1B, SS_AP_FR1A,
    SS_AP_FR2F, SS_AP_FR2B, SS_AP_FR2A,
    SS_AP_BYP,
    SS_AP_COUNT
};

typedef struct ss_seg {
    int start, len;               /* samples */
    float f0[SS_FP_COUNT];        /* formant targets at segment start */
    float f1[SS_FP_COUNT];        /* formant targets at segment end */
    float amp[SS_AP_COUNT];       /* linear amplitudes, Hz for filter params */
    int amp_in;                   /* samples of amplitude ramp at the start */
    unsigned char anchor;         /* defines formant targets? */
    int pa, na;                   /* previous / next anchor segment (-1 = none) */
    unsigned char rank, int_ms, ext_ms;
    float wdom;
    /* resolved transitions (anchors only), filled by the builder */
    int has_in, has_out;
    int in_tb, in_d;              /* boundary time and length of in-transition */
    int out_d;                    /* length of out-transition before segment end */
    float in_v[SS_FP_COUNT];      /* boundary value at in boundary */
    float out_v[SS_FP_COUNT];     /* boundary value at out boundary */
} ss_seg;

typedef struct ss_pitchpt {
    int t;
    float hz;
} ss_pitchpt;

struct ss_utterance {
    int sample_rate;
    ss_voice voice;
    int length;
    ss_seg *seg;
    int nseg;
    ss_pitchpt *pitch;
    int npitch;
    ss_event *ev;
    int nev;
    char *phonemes;    /* debug string */
    int phonemes_len;
    /* engine queue bookkeeping */
    int id;
    struct ss_utterance *next;
};

/* Build the timeline from tokens. Returns 0 on success. */
int ss_build_utterance(ss_utterance *u, const ss_tokens *t);

/* Frame evaluation cursor. */
typedef struct ss_cursor {
    int seg;
    int pitch;
} ss_cursor;

/* Parameter frame fed to the synthesizer. */
typedef struct ss_frame {
    float form[SS_FP_COUNT];
    float amp[SS_AP_COUNT];
    float f0;
} ss_frame;

void ss_eval_frame(const ss_utterance *u, int t, ss_cursor *c, ss_frame *out);

/* ------------------------------------------------------------------------ */
/* Synthesizer                                                              */
/* ------------------------------------------------------------------------ */

typedef struct ss_reson {
    float a, b, c;
    float y1, y2;
} ss_reson;

typedef struct ss_synth {
    float fs;
    ss_voice voice;
    /* glottal source */
    float phase;       /* 0..1 within the current period */
    float period_f0;   /* f0 latched for the current period */
    float shimmer;     /* amplitude factor for the current period */
    float oq;          /* open quotient */
    float voice_norm;  /* voicing level compensation */
    float pulse_gain;  /* per-period level (pitch compensation x shimmer) */
    float pitch_mod;   /* flutter/vibrato factor, updated once per frame */
    float tilt_state, tilt_coef;
    double time;       /* seconds, for flutter / vibrato */
    unsigned int rng;
    float noise_lp;
    /* filters */
    ss_reson np, nz;          /* nasal pole / zero */
    ss_reson cas[6];          /* F1..F6 */
    int cas_on[6];
    ss_reson fr1, fr1b, fr2;  /* frication resonators (fr1 is 4-pole: fr1 -> fr1b) */
    float fr1_norm, fr2_norm;
    float rad_prev;           /* radiation (first difference) */
    float dc_x, dc_y;         /* DC blocker */
    /* parameter interpolation */
    ss_frame cur, next;
    int frame_len, frame_pos;
    float gain;
} ss_synth;

void ss_synth_init(ss_synth *s, int sample_rate);
void ss_synth_set_voice(ss_synth *s, const ss_voice *v);
/* Load a new parameter frame: coefficients for `next`, ramping amplitudes. */
void ss_synth_frame(ss_synth *s, const ss_frame *f);
/* Render n samples (n <= frame_len) with the current frame. */
void ss_synth_run(ss_synth *s, float *out, int n);
/* Reset filter memories (e.g. after a hard stop). */
void ss_synth_reset(ss_synth *s);

#endif /* SS_INTERNAL_H */

/* ---- ss_phonemes.c ---- */
/*
 * Phoneme inventory: acoustic targets, durations and coarticulation data.
 *
 * Formant targets are adult-male values in the spirit of Klatt (1980),
 * "Software for a cascade/parallel formant synthesizer", and Peterson &
 * Barney (1952). Other voices are produced by scaling at render time.
 */
#include <string.h>

#define OFF (-100.0f)
#define VIS_INHERIT 255

/*  name   class          flags                 viseme             fric
 *  inh min   F1   F2   F3     B1  B2  B3     F1e  F2e  F3e
 *  AV    AF    AH    rank int ext wdom */
const ss_phinfo ss_phtab[SS_PH_COUNT] = {
    {"PAU", SS_CL_PAUSE, 0, SS_VISEME_SIL, SS_FR_NONE,
     0, 0, {500, 1500, 2500}, {80, 90, 150}, {500, 1500, 2500},
     OFF, OFF, OFF, 0, 0, 0, 0.5f},

    /* ---- vowels ---- */
    {"AA", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_AA, SS_FR_NONE,
     240, 100, {700, 1220, 2600}, {130, 70, 160}, {700, 1220, 2600},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"AE", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_AA, SS_FR_NONE,
     230, 80, {680, 1680, 2410}, {70, 130, 300}, {680, 1680, 2410},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"AH", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_AA, SS_FR_NONE,
     140, 60, {620, 1220, 2550}, {80, 50, 140}, {620, 1220, 2550},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"AO", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_OH, SS_FR_NONE,
     240, 100, {600, 990, 2570}, {90, 100, 80}, {600, 990, 2570},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"AW", SS_CL_DIPH, SS_PF_VOICED, SS_VISEME_AA, SS_FR_NONE,
     260, 100, {640, 1230, 2550}, {80, 70, 140}, {420, 940, 2350},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"AX", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_E, SS_FR_NONE,
     120, 60, {550, 1260, 2470}, {80, 50, 140}, {550, 1260, 2470},
     -1, OFF, OFF, 2, 40, 40, 0.5f},
    {"AY", SS_CL_DIPH, SS_PF_VOICED, SS_VISEME_AA, SS_FR_NONE,
     250, 150, {660, 1200, 2550}, {100, 70, 200}, {400, 1880, 2500},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"EH", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_E, SS_FR_NONE,
     150, 70, {530, 1680, 2500}, {60, 90, 200}, {530, 1680, 2500},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"ER", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_RR, SS_FR_NONE,
     180, 80, {470, 1270, 1540}, {100, 60, 110}, {470, 1270, 1540},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"EY", SS_CL_DIPH, SS_PF_VOICED, SS_VISEME_E, SS_FR_NONE,
     190, 100, {480, 1720, 2520}, {70, 100, 200}, {330, 2200, 2600},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"IH", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_IH, SS_FR_NONE,
     135, 40, {400, 1800, 2570}, {50, 100, 140}, {400, 1800, 2570},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"IY", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_IH, SS_FR_NONE,
     155, 55, {310, 2200, 2960}, {45, 200, 400}, {290, 2250, 3000},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"OW", SS_CL_DIPH, SS_PF_VOICED, SS_VISEME_OH, SS_FR_NONE,
     220, 80, {540, 1100, 2300}, {80, 70, 70}, {450, 900, 2300},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"OY", SS_CL_DIPH, SS_PF_VOICED, SS_VISEME_OH, SS_FR_NONE,
     280, 150, {550, 960, 2400}, {80, 50, 130}, {360, 1820, 2450},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"UH", SS_CL_VOWEL, SS_PF_VOICED, SS_VISEME_OU, SS_FR_NONE,
     160, 60, {440, 1020, 2240}, {80, 100, 80}, {440, 1020, 2240},
     0, OFF, OFF, 2, 40, 40, 0.5f},
    {"UW", SS_CL_DIPH, SS_PF_VOICED, SS_VISEME_OU, SS_FR_NONE,
     210, 70, {350, 1250, 2200}, {65, 110, 140}, {320, 900, 2200},
     0, OFF, OFF, 2, 40, 40, 0.5f},

    /* ---- consonants ---- */
    {"B", SS_CL_STOP, SS_PF_VOICED | SS_PF_LABIAL, SS_VISEME_PP, SS_FR_BURST_P,
     85, 60, {200, 1100, 2150}, {60, 110, 130}, {200, 1100, 2150},
     -24, -12, OFF, 6, 10, 50, 0.75f},
    {"CH", SS_CL_AFFRIC, 0, SS_VISEME_CH, SS_FR_SH,
     120, 80, {350, 1800, 2820}, {200, 90, 300}, {350, 1800, 2820},
     OFF, 4, OFF, 6, 10, 50, 0.7f},
    {"D", SS_CL_STOP, SS_PF_VOICED, SS_VISEME_DD, SS_FR_BURST_T,
     75, 50, {200, 1700, 2600}, {60, 100, 170}, {200, 1700, 2600},
     -24, -8, OFF, 6, 10, 50, 0.75f},
    {"DH", SS_CL_FRIC, SS_PF_VOICED, SS_VISEME_TH, SS_FR_TH,
     50, 30, {270, 1290, 2540}, {60, 80, 170}, {270, 1290, 2540},
     -8, -22, OFF, 5, 15, 40, 0.6f},
    {"F", SS_CL_FRIC, SS_PF_LABIAL, SS_VISEME_FF, SS_FR_F,
     100, 80, {340, 1100, 2080}, {200, 120, 150}, {340, 1100, 2080},
     OFF, -10, OFF, 5, 15, 40, 0.5f},
    {"G", SS_CL_STOP, SS_PF_VOICED | SS_PF_VELAR, SS_VISEME_KK, SS_FR_BURST_K,
     80, 60, {200, 1990, 2850}, {60, 150, 280}, {200, 1990, 2850},
     -24, -10, OFF, 6, 10, 50, 0.75f},
    {"HH", SS_CL_ASP, 0, VIS_INHERIT, SS_FR_NONE,
     80, 20, {500, 1500, 2500}, {300, 150, 250}, {500, 1500, 2500},
     OFF, OFF, -2, 1, 0, 0, 0.5f},
    {"JH", SS_CL_AFFRIC, SS_PF_VOICED, SS_VISEME_CH, SS_FR_SH,
     110, 70, {260, 1800, 2820}, {60, 80, 270}, {260, 1800, 2820},
     -10, 0, OFF, 6, 10, 50, 0.7f},
    {"K", SS_CL_STOP, SS_PF_VELAR, SS_VISEME_KK, SS_FR_BURST_K,
     80, 60, {300, 1990, 2850}, {250, 160, 330}, {300, 1990, 2850},
     OFF, -2, OFF, 6, 10, 50, 0.75f},
    {"L", SS_CL_LIQUID, SS_PF_VOICED, SS_VISEME_NN, SS_FR_NONE,
     80, 40, {310, 1050, 2880}, {50, 100, 280}, {310, 1050, 2880},
     -3, OFF, OFF, 3, 30, 60, 0.65f},
    {"M", SS_CL_NASAL, SS_PF_VOICED | SS_PF_LABIAL, SS_VISEME_PP, SS_FR_NONE,
     70, 60, {480, 1270, 2130}, {40, 200, 200}, {480, 1270, 2130},
     -4, OFF, OFF, 4, 15, 45, 0.6f},
    {"N", SS_CL_NASAL, SS_PF_VOICED, SS_VISEME_NN, SS_FR_NONE,
     60, 50, {480, 1340, 2470}, {40, 300, 300}, {480, 1340, 2470},
     -4, OFF, OFF, 4, 15, 45, 0.6f},
    {"NG", SS_CL_NASAL, SS_PF_VOICED | SS_PF_VELAR, SS_VISEME_KK, SS_FR_NONE,
     95, 60, {480, 2150, 2450}, {160, 150, 100}, {480, 2150, 2450},
     -4, OFF, OFF, 4, 15, 45, 0.6f},
    {"P", SS_CL_STOP, SS_PF_LABIAL, SS_VISEME_PP, SS_FR_BURST_P,
     90, 50, {400, 1100, 2150}, {300, 150, 220}, {400, 1100, 2150},
     OFF, -4, OFF, 6, 10, 50, 0.75f},
    {"R", SS_CL_LIQUID, SS_PF_VOICED, SS_VISEME_RR, SS_FR_NONE,
     80, 30, {310, 1060, 1380}, {70, 100, 120}, {310, 1060, 1380},
     -3, OFF, OFF, 3, 40, 60, 0.7f},
    {"S", SS_CL_FRIC, 0, SS_VISEME_SS, SS_FR_S,
     105, 60, {320, 1390, 2530}, {200, 80, 200}, {320, 1390, 2530},
     OFF, 0, OFF, 5, 15, 40, 0.6f},
    {"SH", SS_CL_FRIC, 0, SS_VISEME_CH, SS_FR_SH,
     105, 80, {300, 1840, 2750}, {200, 100, 300}, {300, 1840, 2750},
     OFF, 0, OFF, 5, 15, 40, 0.6f},
    {"T", SS_CL_STOP, 0, SS_VISEME_DD, SS_FR_BURST_T,
     75, 50, {400, 1600, 2600}, {300, 120, 250}, {400, 1600, 2600},
     OFF, 0, OFF, 6, 10, 50, 0.75f},
    {"TH", SS_CL_FRIC, 0, SS_VISEME_TH, SS_FR_TH,
     90, 60, {320, 1290, 2540}, {200, 90, 200}, {320, 1290, 2540},
     OFF, -10, OFF, 5, 15, 40, 0.5f},
    {"V", SS_CL_FRIC, SS_PF_VOICED | SS_PF_LABIAL, SS_VISEME_FF, SS_FR_F,
     60, 40, {220, 1100, 2080}, {60, 90, 120}, {220, 1100, 2080},
     -8, -20, OFF, 5, 15, 40, 0.5f},
    {"W", SS_CL_GLIDE, SS_PF_VOICED | SS_PF_LABIAL, SS_VISEME_OU, SS_FR_NONE,
     80, 60, {290, 610, 2150}, {50, 80, 60}, {290, 610, 2150},
     -2, OFF, OFF, 3, 40, 60, 0.7f},
    {"Y", SS_CL_GLIDE, SS_PF_VOICED, SS_VISEME_IH, SS_FR_NONE,
     80, 40, {260, 2070, 3020}, {40, 250, 500}, {260, 2070, 3020},
     -2, OFF, OFF, 3, 40, 60, 0.7f},
    {"Z", SS_CL_FRIC, SS_PF_VOICED, SS_VISEME_SS, SS_FR_S,
     75, 40, {240, 1390, 2530}, {70, 60, 180}, {240, 1390, 2530},
     -8, -8, OFF, 5, 15, 40, 0.6f},
    {"ZH", SS_CL_FRIC, SS_PF_VOICED, SS_VISEME_CH, SS_FR_SH,
     70, 40, {300, 1840, 2750}, {220, 100, 300}, {300, 1840, 2750},
     -8, -8, OFF, 5, 15, 40, 0.6f},
};

/* Noise colourings: two resonances plus a flat component. */
const ss_fric_shape ss_fric_shapes[SS_FR_COUNT] = {
    /* NONE    */ {2000, 1000, 0.0f, 4000, 1000, 0.0f, 0.0f},
    /* S       */ {5200, 1400, 1.0f, 7800, 2500, 0.45f, 0.0f},
    /* SH      */ {2600, 650, 1.0f, 4200, 1500, 0.25f, 0.0f},
    /* F       */ {1500, 2500, 0.15f, 7000, 4000, 0.45f, 0.35f},
    /* TH      */ {1800, 2500, 0.15f, 6200, 3500, 0.5f, 0.25f},
    /* BURST_P */ {900, 900, 0.7f, 2400, 2500, 0.3f, 0.3f},
    /* BURST_T */ {4100, 1500, 1.0f, 6500, 3000, 0.5f, 0.15f},
    /* BURST_K */ {2200, 700, 1.0f, 4000, 2500, 0.3f, 0.1f},
};

int ss_phoneme_from_name(const char *name, int len)
{
    int i;
    char up[4];
    if (len <= 0 || len > 3) return -1;
    for (i = 0; i < len; i++) {
        char ch = name[i];
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        up[i] = ch;
    }
    up[len] = 0;
    for (i = 0; i < SS_PH_COUNT; i++) {
        if (strcmp(ss_phtab[i].name, up) == 0) return i;
    }
    /* common aliases from other ARPAbet dialects */
    if (strcmp(up, "IX") == 0) return SS_PH_IH;
    if (strcmp(up, "AXR") == 0) return SS_PH_ER;
    if (strcmp(up, "DX") == 0) return SS_PH_D;
    if (strcmp(up, "SIL") == 0 || strcmp(up, "_") == 0) return SS_PH_PAU;
    return -1;
}

int ss_ph_is_vowel(int ph)
{
    if (ph < 0 || ph >= SS_PH_COUNT) return 0;
    return ss_phtab[ph].cls == SS_CL_VOWEL || ss_phtab[ph].cls == SS_CL_DIPH;
}

int ss_ph_is_voiced(int ph)
{
    if (ph < 0 || ph >= SS_PH_COUNT) return 0;
    return (ss_phtab[ph].flags & SS_PF_VOICED) != 0;
}

int ss_ph_class(int ph)
{
    if (ph < 0 || ph >= SS_PH_COUNT) return SS_CL_PAUSE;
    return ss_phtab[ph].cls;
}

int ss_ph_is_sonorant(int ph)
{
    int c;
    if (ph < 0 || ph >= SS_PH_COUNT) return 0;
    c = ss_phtab[ph].cls;
    return c == SS_CL_VOWEL || c == SS_CL_DIPH || c == SS_CL_NASAL ||
           c == SS_CL_LIQUID || c == SS_CL_GLIDE;
}

/* ---- public introspection ---- */

static const char *const ss_viseme_names[SS_VISEME_COUNT] = {
    "sil", "PP", "FF", "TH", "DD", "kk", "CH", "SS", "nn", "RR", "aa", "E", "ih", "oh", "ou"
};

int ss_phoneme_count(void) { return SS_PH_COUNT; }

const char *ss_phoneme_name(int phoneme)
{
    if (phoneme < 0 || phoneme >= SS_PH_COUNT) return NULL;
    return ss_phtab[phoneme].name;
}

int ss_phoneme_viseme(int phoneme)
{
    if (phoneme < 0 || phoneme >= SS_PH_COUNT) return SS_VISEME_SIL;
    if (ss_phtab[phoneme].viseme == VIS_INHERIT) return SS_VISEME_AA;
    return ss_phtab[phoneme].viseme;
}

const char *ss_viseme_name(int viseme)
{
    if (viseme < 0 || viseme >= SS_VISEME_COUNT) return NULL;
    return ss_viseme_names[viseme];
}

#undef OFF
#undef VIS_INHERIT

/* ---- ss_lexicon.c ---- */
/*
 * Exception dictionary: words whose spelling the letter-to-sound rules get
 * wrong, very common function words, number words, and some game vocabulary.
 *
 * Format: lowercase word, ARPAbet with stress digits (1 primary, 2 secondary,
 * 0 unstressed). "AH0" is read as schwa. A word with no primary stress (1)
 * is treated as a function word and receives no pitch accent.
 *
 * KEEP THIS TABLE SORTED (strcmp order): lookups use binary search, and the
 * test suite verifies the order.
 */
#include <string.h>

typedef struct ss_lex_entry {
    const char *word;
    const char *pron;
} ss_lex_entry;

static const ss_lex_entry ss_lex_table[] = {
    /* BEGIN LEXICON */
    {"a", "AH0"},
    {"about", "AH0 B AW1 T"},
    {"above", "AH0 B AH1 V"},
    {"achievement", "AH0 CH IY1 V M AH0 N T"},
    {"across", "AH0 K R AO1 S"},
    {"after", "AE1 F T ER0"},
    {"again", "AH0 G EH1 N"},
    {"against", "AH0 G EH1 N S T"},
    {"ago", "AH0 G OW1"},
    {"ahead", "AH0 HH EH1 D"},
    {"alien", "EY1 L IY0 AH0 N"},
    {"alive", "AH0 L AY1 V"},
    {"all", "AO1 L"},
    {"allow", "AH0 L AW1"},
    {"almost", "AO1 L M OW2 S T"},
    {"alone", "AH0 L OW1 N"},
    {"along", "AH0 L AO1 NG"},
    {"already", "AO0 L R EH1 D IY0"},
    {"also", "AO1 L S OW0"},
    {"always", "AO1 L W EY2 Z"},
    {"am", "AE0 M"},
    {"ammo", "AE1 M OW0"},
    {"among", "AH0 M AH1 NG"},
    {"an", "AH0 N"},
    {"and", "AH0 N D"},
    {"another", "AH0 N AH1 DH ER0"},
    {"answer", "AE1 N S ER0"},
    {"any", "EH1 N IY0"},
    {"anyone", "EH1 N IY0 W AH2 N"},
    {"anything", "EH1 N IY0 TH IH2 NG"},
    {"are", "AA0 R"},
    {"area", "EH1 R IY0 AH0"},
    {"aren't", "AA1 R AH0 N T"},
    {"armor", "AA1 R M ER0"},
    {"around", "ER0 AW1 N D"},
    {"arrow", "AE1 R OW0"},
    {"as", "AE0 Z"},
    {"ask", "AE1 S K"},
    {"at", "AE0 T"},
    {"attack", "AH0 T AE1 K"},
    {"away", "AH0 W EY1"},
    {"axe", "AE1 K S"},
    {"be", "B IY0"},
    {"bear", "B EH1 R"},
    {"beautiful", "B Y UW1 T AH0 F AH0 L"},
    {"because", "B IH0 K AH1 Z"},
    {"been", "B IH0 N"},
    {"before", "B IH0 F AO1 R"},
    {"begin", "B IH0 G IH1 N"},
    {"behind", "B IH0 HH AY1 N D"},
    {"being", "B IY1 IH0 NG"},
    {"believe", "B IH0 L IY1 V"},
    {"below", "B IH0 L OW1"},
    {"between", "B IH0 T W IY1 N"},
    {"beyond", "B IH0 Y AA1 N D"},
    {"billion", "B IH1 L Y AH0 N"},
    {"blood", "B L AH1 D"},
    {"board", "B AO1 R D"},
    {"body", "B AA1 D IY0"},
    {"bonus", "B OW1 N AH0 S"},
    {"boss", "B AO1 S"},
    {"both", "B OW1 TH"},
    {"bought", "B AO1 T"},
    {"bread", "B R EH1 D"},
    {"break", "B R EY1 K"},
    {"bridge", "B R IH1 JH"},
    {"brother", "B R AH1 DH ER0"},
    {"brought", "B R AO1 T"},
    {"brown", "B R AW1 N"},
    {"build", "B IH1 L D"},
    {"built", "B IH1 L T"},
    {"busy", "B IH1 Z IY0"},
    {"but", "B AH0 T"},
    {"buy", "B AY1"},
    {"by", "B AY0"},
    {"bye", "B AY1"},
    {"cafe", "K AE0 F EY1"},
    {"calm", "K AA1 M"},
    {"can", "K AE0 N"},
    {"can't", "K AE1 N T"},
    {"cannot", "K AE1 N AA0 T"},
    {"captain", "K AE1 P T AH0 N"},
    {"castle", "K AE1 S AH0 L"},
    {"caught", "K AO1 T"},
    {"cave", "K EY1 V"},
    {"cent", "S EH1 N T"},
    {"cents", "S EH1 N T S"},
    {"change", "CH EY1 N JH"},
    {"character", "K EH1 R IH0 K T ER0"},
    {"checkpoint", "CH EH1 K P OY2 N T"},
    {"children", "CH IH1 L D R AH0 N"},
    {"choose", "CH UW1 Z"},
    {"city", "S IH1 T IY0"},
    {"climb", "K L AY1 M"},
    {"clown", "K L AW1 N"},
    {"coin", "K OY1 N"},
    {"coins", "K OY1 N Z"},
    {"color", "K AH1 L ER0"},
    {"come", "K AH1 M"},
    {"coming", "K AH1 M IH0 NG"},
    {"commander", "K AH0 M AE1 N D ER0"},
    {"complete", "K AH0 M P L IY1 T"},
    {"computer", "K AH0 M P Y UW1 T ER0"},
    {"continue", "K AH0 N T IH1 N Y UW0"},
    {"cost", "K AO1 S T"},
    {"cough", "K AO1 F"},
    {"could", "K UH0 D"},
    {"couldn't", "K UH1 D AH0 N T"},
    {"country", "K AH1 N T R IY0"},
    {"courage", "K ER1 IH0 JH"},
    {"cow", "K AW1"},
    {"create", "K R IY0 EY1 T"},
    {"creature", "K R IY1 CH ER0"},
    {"crowd", "K R AW1 D"},
    {"crown", "K R AW1 N"},
    {"crystal", "K R IH1 S T AH0 L"},
    {"damage", "D AE1 M IH0 JH"},
    {"danger", "D EY1 N JH ER0"},
    {"dangerous", "D EY1 N JH ER0 AH0 S"},
    {"dark", "D AA1 R K"},
    {"data", "D EY1 T AH0"},
    {"dead", "D EH1 D"},
    {"death", "D EH1 TH"},
    {"defeat", "D IH0 F IY1 T"},
    {"defend", "D IH0 F EH1 N D"},
    {"demon", "D IY1 M AH0 N"},
    {"destroy", "D IH0 S T R OY1"},
    {"did", "D IH0 D"},
    {"didn't", "D IH1 D AH0 N T"},
    {"different", "D IH1 F ER0 AH0 N T"},
    {"do", "D UW0"},
    {"doctor", "D AA1 K T ER0"},
    {"does", "D AH0 Z"},
    {"doesn't", "D AH1 Z AH0 N T"},
    {"doing", "D UW1 IH0 NG"},
    {"dollar", "D AA1 L ER0"},
    {"dollars", "D AA1 L ER0 Z"},
    {"don't", "D OW1 N T"},
    {"done", "D AH1 N"},
    {"door", "D AO1 R"},
    {"down", "D AW1 N"},
    {"dragon", "D R AE1 G AH0 N"},
    {"drown", "D R AW1 N"},
    {"dungeon", "D AH1 N JH AH0 N"},
    {"during", "D UH1 R IH0 NG"},
    {"each", "IY1 CH"},
    {"early", "ER1 L IY0"},
    {"earth", "ER1 TH"},
    {"easy", "IY1 Z IY0"},
    {"eight", "EY1 T"},
    {"eighteen", "EY0 T IY1 N"},
    {"eighteenth", "EY0 T IY1 N TH"},
    {"eighth", "EY1 T TH"},
    {"eightieth", "EY1 T IY0 AH0 TH"},
    {"eighty", "EY1 T IY0"},
    {"either", "IY1 DH ER0"},
    {"eleven", "IH0 L EH1 V AH0 N"},
    {"eleventh", "IH0 L EH1 V AH0 N TH"},
    {"elf", "EH1 L F"},
    {"else", "EH1 L S"},
    {"empire", "EH1 M P AY0 ER0"},
    {"end", "EH1 N D"},
    {"enemies", "EH1 N AH0 M IY0 Z"},
    {"enemy", "EH1 N AH0 M IY0"},
    {"energy", "EH1 N ER0 JH IY0"},
    {"engine", "EH1 N JH AH0 N"},
    {"english", "IH1 NG G L IH0 SH"},
    {"enough", "IH0 N AH1 F"},
    {"enter", "EH1 N T ER0"},
    {"equals", "IY1 K W AH0 L Z"},
    {"every", "EH1 V R IY0"},
    {"everyone", "EH1 V R IY0 W AH2 N"},
    {"everything", "EH1 V R IY0 TH IH2 NG"},
    {"evil", "IY1 V AH0 L"},
    {"experience", "IH0 K S P IH1 R IY0 AH0 N S"},
    {"eye", "AY1"},
    {"eyes", "AY1 Z"},
    {"father", "F AA1 DH ER0"},
    {"feather", "F EH1 DH ER0"},
    {"few", "F Y UW1"},
    {"fifteen", "F IH0 F T IY1 N"},
    {"fifteenth", "F IH0 F T IY1 N TH"},
    {"fifth", "F IH1 F TH"},
    {"fiftieth", "F IH1 F T IY0 AH0 TH"},
    {"fifty", "F IH1 F T IY0"},
    {"fight", "F AY1 T"},
    {"find", "F AY1 N D"},
    {"fire", "F AY1 ER0"},
    {"first", "F ER1 S T"},
    {"five", "F AY1 V"},
    {"flower", "F L AW1 ER0"},
    {"for", "F AO0 R"},
    {"forest", "F AO1 R AH0 S T"},
    {"fortieth", "F AO1 R T IY0 AH0 TH"},
    {"forty", "F AO1 R T IY0"},
    {"four", "F AO1 R"},
    {"fourteen", "F AO0 R T IY1 N"},
    {"fourteenth", "F AO0 R T IY1 N TH"},
    {"fourth", "F AO1 R TH"},
    {"friend", "F R EH1 N D"},
    {"friends", "F R EH1 N D Z"},
    {"from", "F R AH0 M"},
    {"front", "F R AH1 N T"},
    {"frown", "F R AW1 N"},
    {"game", "G EY1 M"},
    {"general", "JH EH1 N ER0 AH0 L"},
    {"ghost", "G OW1 S T"},
    {"give", "G IH1 V"},
    {"glove", "G L AH1 V"},
    {"go", "G OW1"},
    {"goblin", "G AA1 B L IH0 N"},
    {"goes", "G OW1 Z"},
    {"gold", "G OW1 L D"},
    {"gone", "G AO1 N"},
    {"good", "G UH1 D"},
    {"goodbye", "G UH2 D B AY1"},
    {"great", "G R EY1 T"},
    {"group", "G R UW1 P"},
    {"guard", "G AA1 R D"},
    {"guide", "G AY1 D"},
    {"guy", "G AY1"},
    {"had", "HH AE0 D"},
    {"half", "HH AE1 F"},
    {"has", "HH AE0 Z"},
    {"hasn't", "HH AE1 Z AH0 N T"},
    {"have", "HH AE0 V"},
    {"haven't", "HH AE1 V AH0 N T"},
    {"he", "HH IY0"},
    {"he's", "HH IY0 Z"},
    {"head", "HH EH1 D"},
    {"health", "HH EH1 L TH"},
    {"heard", "HH ER1 D"},
    {"heart", "HH AA1 R T"},
    {"heavy", "HH EH1 V IY0"},
    {"hello", "HH AH0 L OW1"},
    {"her", "HH ER0"},
    {"here", "HH IY1 R"},
    {"here's", "HH IY1 R Z"},
    {"hero", "HH IY1 R OW0"},
    {"heroes", "HH IY1 R OW0 Z"},
    {"hey", "HH EY1"},
    {"hi", "HH AY1"},
    {"him", "HH IH0 M"},
    {"his", "HH IH0 Z"},
    {"home", "HH OW1 M"},
    {"honest", "AA1 N AH0 S T"},
    {"host", "HH OW1 S T"},
    {"hour", "AW1 ER0"},
    {"house", "HH AW1 S"},
    {"how", "HH AW1"},
    {"however", "HH AW0 EH1 V ER0"},
    {"howl", "HH AW1 L"},
    {"hundred", "HH AH1 N D R AH0 D"},
    {"hundredth", "HH AH1 N D R AH0 D TH"},
    {"i", "AY0"},
    {"i'd", "AY0 D"},
    {"i'll", "AY0 L"},
    {"i'm", "AY0 M"},
    {"i've", "AY0 V"},
    {"idea", "AY0 D IY1 AH0"},
    {"if", "IH0 F"},
    {"important", "IH0 M P AO1 R T AH0 N T"},
    {"in", "IH0 N"},
    {"into", "IH1 N T UW0"},
    {"inventory", "IH1 N V AH0 N T AO2 R IY0"},
    {"is", "IH0 Z"},
    {"island", "AY1 L AH0 N D"},
    {"isn't", "IH1 Z AH0 N T"},
    {"it", "IH0 T"},
    {"it's", "IH0 T S"},
    {"item", "AY1 T AH0 M"},
    {"items", "AY1 T AH0 M Z"},
    {"its", "IH0 T S"},
    {"journey", "JH ER1 N IY0"},
    {"jump", "JH AH1 M P"},
    {"just", "JH AH1 S T"},
    {"key", "K IY1"},
    {"kill", "K IH1 L"},
    {"king", "K IH1 NG"},
    {"kingdom", "K IH1 NG D AH0 M"},
    {"knife", "N AY1 F"},
    {"knight", "N AY1 T"},
    {"know", "N OW1"},
    {"knowledge", "N AA1 L IH0 JH"},
    {"known", "N OW1 N"},
    {"language", "L AE1 NG G W AH0 JH"},
    {"laser", "L EY1 Z ER0"},
    {"laugh", "L AE1 F"},
    {"layer", "L EY1 ER0"},
    {"learn", "L ER1 N"},
    {"leather", "L EH1 DH ER0"},
    {"leave", "L IY1 V"},
    {"legend", "L EH1 JH AH0 N D"},
    {"let's", "L EH1 T S"},
    {"level", "L EH1 V AH0 L"},
    {"life", "L AY1 F"},
    {"light", "L AY1 T"},
    {"listen", "L IH1 S AH0 N"},
    {"little", "L IH1 T AH0 L"},
    {"live", "L IH1 V"},
    {"lived", "L IH1 V D"},
    {"loading", "L OW1 D IH0 NG"},
    {"long", "L AO1 NG"},
    {"look", "L UH1 K"},
    {"lord", "L AO1 R D"},
    {"lose", "L UW1 Z"},
    {"lost", "L AO1 S T"},
    {"love", "L AH1 V"},
    {"machine", "M AH0 SH IY1 N"},
    {"magic", "M AE1 JH IH0 K"},
    {"make", "M EY1 K"},
    {"mana", "M AA1 N AH0"},
    {"many", "M EH1 N IY0"},
    {"map", "M AE1 P"},
    {"master", "M AE1 S T ER0"},
    {"may", "M EY0"},
    {"me", "M IY0"},
    {"menu", "M EH1 N Y UW0"},
    {"merchant", "M ER1 CH AH0 N T"},
    {"might", "M AY0 T"},
    {"million", "M IH1 L Y AH0 N"},
    {"minus", "M AY1 N AH0 S"},
    {"minute", "M IH1 N AH0 T"},
    {"minutes", "M IH1 N AH0 T S"},
    {"mission", "M IH1 SH AH0 N"},
    {"missus", "M IH1 S IH0 Z"},
    {"mister", "M IH1 S T ER0"},
    {"money", "M AH1 N IY0"},
    {"monster", "M AA1 N S T ER0"},
    {"month", "M AH1 N TH"},
    {"more", "M AO1 R"},
    {"morning", "M AO1 R N IH0 NG"},
    {"most", "M OW1 S T"},
    {"mother", "M AH1 DH ER0"},
    {"mountain", "M AW1 N T AH0 N"},
    {"move", "M UW1 V"},
    {"much", "M AH1 CH"},
    {"multiplayer", "M AH2 L T IY0 P L EY1 ER0"},
    {"must", "M AH0 S T"},
    {"my", "M AY0"},
    {"myself", "M AY0 S EH1 L F"},
    {"night", "N AY1 T"},
    {"nine", "N AY1 N"},
    {"nineteen", "N AY0 N T IY1 N"},
    {"nineteenth", "N AY0 N T IY1 N TH"},
    {"ninetieth", "N AY1 N T IY0 AH0 TH"},
    {"ninety", "N AY1 N T IY0"},
    {"ninth", "N AY1 N TH"},
    {"no", "N OW1"},
    {"nobody", "N OW1 B AA2 D IY0"},
    {"none", "N AH1 N"},
    {"nothing", "N AH1 TH IH0 NG"},
    {"now", "N AW1"},
    {"number", "N AH1 M B ER0"},
    {"o'clock", "AH0 K L AA1 K"},
    {"objective", "AH0 B JH EH1 K T IH0 V"},
    {"ocean", "OW1 SH AH0 N"},
    {"of", "AH0 V"},
    {"off", "AO1 F"},
    {"often", "AO1 F AH0 N"},
    {"oh", "OW1"},
    {"ok", "OW2 K EY1"},
    {"okay", "OW2 K EY1"},
    {"old", "OW1 L D"},
    {"on", "AA0 N"},
    {"once", "W AH1 N S"},
    {"one", "W AH1 N"},
    {"online", "AO1 N L AY2 N"},
    {"only", "OW1 N L IY0"},
    {"open", "OW1 P AH0 N"},
    {"options", "AA1 P SH AH0 N Z"},
    {"or", "AO0 R"},
    {"orc", "AO1 R K"},
    {"other", "AH1 DH ER0"},
    {"our", "AW0 ER0"},
    {"out", "AW1 T"},
    {"over", "OW1 V ER0"},
    {"owl", "AW1 L"},
    {"own", "OW1 N"},
    {"pause", "P AO1 Z"},
    {"paused", "P AO1 Z D"},
    {"people", "P IY1 P AH0 L"},
    {"percent", "P ER0 S EH1 N T"},
    {"person", "P ER1 S AH0 N"},
    {"pilot", "P AY1 L AH0 T"},
    {"place", "P L EY1 S"},
    {"planet", "P L AE1 N AH0 T"},
    {"play", "P L EY1"},
    {"player", "P L EY1 ER0"},
    {"please", "P L IY1 Z"},
    {"plus", "P L AH1 S"},
    {"point", "P OY1 N T"},
    {"police", "P AH0 L IY1 S"},
    {"post", "P OW1 S T"},
    {"potion", "P OW1 SH AH0 N"},
    {"power", "P AW1 ER0"},
    {"prepare", "P R IY0 P EH1 R"},
    {"pretty", "P R IH1 T IY0"},
    {"princess", "P R IH1 N S EH0 S"},
    {"prison", "P R IH1 Z AH0 N"},
    {"prove", "P R UW1 V"},
    {"pull", "P UH1 L"},
    {"push", "P UH1 SH"},
    {"put", "P UH1 T"},
    {"quest", "K W EH1 S T"},
    {"question", "K W EH1 S CH AH0 N"},
    {"quiet", "K W AY1 AH0 T"},
    {"radio", "R EY1 D IY0 OW0"},
    {"read", "R IY1 D"},
    {"ready", "R EH1 D IY0"},
    {"real", "R IY1 L"},
    {"really", "R IH1 L IY0"},
    {"reload", "R IY0 L OW1 D"},
    {"resume", "R IH0 Z UW1 M"},
    {"reward", "R IH0 W AO1 R D"},
    {"right", "R AY1 T"},
    {"river", "R IH1 V ER0"},
    {"robot", "R OW1 B AA2 T"},
    {"rough", "R AH1 F"},
    {"run", "R AH1 N"},
    {"said", "S EH1 D"},
    {"saint", "S EY1 N T"},
    {"save", "S EY1 V"},
    {"says", "S EH1 Z"},
    {"science", "S AY1 AH0 N S"},
    {"second", "S EH1 K AH0 N D"},
    {"secret", "S IY1 K R AH0 T"},
    {"see", "S IY1"},
    {"settings", "S EH1 T IH0 NG Z"},
    {"seven", "S EH1 V AH0 N"},
    {"seventeen", "S EH2 V AH0 N T IY1 N"},
    {"seventeenth", "S EH2 V AH0 N T IY1 N TH"},
    {"seventh", "S EH1 V AH0 N TH"},
    {"seventieth", "S EH1 V AH0 N T IY0 AH0 TH"},
    {"seventy", "S EH1 V AH0 N T IY0"},
    {"shall", "SH AE0 L"},
    {"she", "SH IY0"},
    {"she's", "SH IY0 Z"},
    {"shield", "SH IY1 L D"},
    {"ship", "SH IH1 P"},
    {"shoot", "SH UW1 T"},
    {"should", "SH UH0 D"},
    {"shouldn't", "SH UH1 D AH0 N T"},
    {"shower", "SH AW1 ER0"},
    {"sign", "S AY1 N"},
    {"sister", "S IH1 S T ER0"},
    {"six", "S IH1 K S"},
    {"sixteen", "S IH0 K S T IY1 N"},
    {"sixteenth", "S IH0 K S T IY1 N TH"},
    {"sixth", "S IH1 K S TH"},
    {"sixtieth", "S IH1 K S T IY0 AH0 TH"},
    {"sixty", "S IH1 K S T IY0"},
    {"skill", "S K IH1 L"},
    {"so", "S OW0"},
    {"soldier", "S OW1 L JH ER0"},
    {"some", "S AH0 M"},
    {"someone", "S AH1 M W AH2 N"},
    {"something", "S AH1 M TH IH0 NG"},
    {"son", "S AH1 N"},
    {"sorry", "S AA1 R IY0"},
    {"soul", "S OW1 L"},
    {"sound", "S AW1 N D"},
    {"space", "S P EY1 S"},
    {"speak", "S P IY1 K"},
    {"speech", "S P IY1 CH"},
    {"speedspeech", "S P IY1 D S P IY2 CH"},
    {"spell", "S P EH1 L"},
    {"station", "S T EY1 SH AH0 N"},
    {"steak", "S T EY1 K"},
    {"still", "S T IH1 L"},
    {"stop", "S T AA1 P"},
    {"story", "S T AO1 R IY0"},
    {"street", "S T R IY1 T"},
    {"strength", "S T R EH1 NG K TH"},
    {"strong", "S T R AO1 NG"},
    {"sugar", "SH UH1 G ER0"},
    {"sure", "SH UH1 R"},
    {"sword", "S AO1 R D"},
    {"synthesizer", "S IH1 N TH AH0 S AY2 Z ER0"},
    {"system", "S IH1 S T AH0 M"},
    {"table", "T EY1 B AH0 L"},
    {"take", "T EY1 K"},
    {"talk", "T AO1 K"},
    {"target", "T AA1 R G AH0 T"},
    {"tell", "T EH1 L"},
    {"ten", "T EH1 N"},
    {"tenth", "T EH1 N TH"},
    {"than", "DH AE0 N"},
    {"thank", "TH AE1 NG K"},
    {"thanks", "TH AE1 NG K S"},
    {"that", "DH AE0 T"},
    {"that's", "DH AE1 T S"},
    {"the", "DH AH0"},
    {"their", "DH EH0 R"},
    {"them", "DH EH0 M"},
    {"then", "DH EH1 N"},
    {"there", "DH EH0 R"},
    {"there's", "DH EH1 R Z"},
    {"these", "DH IY1 Z"},
    {"they", "DH EY0"},
    {"they'll", "DH EY0 L"},
    {"they're", "DH EH0 R"},
    {"thing", "TH IH1 NG"},
    {"think", "TH IH1 NG K"},
    {"third", "TH ER1 D"},
    {"thirteen", "TH ER0 T IY1 N"},
    {"thirteenth", "TH ER0 T IY1 N TH"},
    {"thirtieth", "TH ER1 T IY0 AH0 TH"},
    {"thirty", "TH ER1 T IY0"},
    {"this", "DH IH1 S"},
    {"those", "DH OW1 Z"},
    {"though", "DH OW1"},
    {"thought", "TH AO1 T"},
    {"thousand", "TH AW1 Z AH0 N D"},
    {"thousandth", "TH AW1 Z AH0 N D TH"},
    {"three", "TH R IY1"},
    {"through", "TH R UW1"},
    {"time", "T AY1 M"},
    {"to", "T UW0"},
    {"today", "T AH0 D EY1"},
    {"together", "T AH0 G EH1 DH ER0"},
    {"tomorrow", "T AH0 M AA1 R OW0"},
    {"ton", "T AH1 N"},
    {"tonight", "T AH0 N AY1 T"},
    {"too", "T UW1"},
    {"tough", "T AH1 F"},
    {"towel", "T AW1 AH0 L"},
    {"tower", "T AW1 ER0"},
    {"town", "T AW1 N"},
    {"travel", "T R AE1 V AH0 L"},
    {"treasure", "T R EH1 ZH ER0"},
    {"trillion", "T R IH1 L Y AH0 N"},
    {"truth", "T R UW1 TH"},
    {"twelfth", "T W EH1 L F TH"},
    {"twelve", "T W EH1 L V"},
    {"twentieth", "T W EH1 N T IY0 AH0 TH"},
    {"twenty", "T W EH1 N T IY0"},
    {"two", "T UW1"},
    {"under", "AH1 N D ER0"},
    {"unity", "Y UW1 N AH0 T IY0"},
    {"unlocked", "AH0 N L AA1 K T"},
    {"unreal", "AH0 N R IY1 L"},
    {"until", "AH0 N T IH1 L"},
    {"up", "AH1 P"},
    {"upgrade", "AH1 P G R EY2 D"},
    {"upon", "AH0 P AA1 N"},
    {"us", "AH0 S"},
    {"use", "Y UW1 Z"},
    {"versus", "V ER1 S AH0 S"},
    {"very", "V EH1 R IY0"},
    {"victory", "V IH1 K T ER0 IY0"},
    {"video", "V IH1 D IY0 OW0"},
    {"village", "V IH1 L AH0 JH"},
    {"voice", "V OY1 S"},
    {"vowel", "V AW1 AH0 L"},
    {"wait", "W EY1 T"},
    {"walk", "W AO1 K"},
    {"want", "W AA1 N T"},
    {"war", "W AO1 R"},
    {"warning", "W AO1 R N IH0 NG"},
    {"warrior", "W AO1 R IY0 ER0"},
    {"was", "W AH0 Z"},
    {"wasn't", "W AA1 Z AH0 N T"},
    {"watch", "W AA1 CH"},
    {"water", "W AO1 T ER0"},
    {"we", "W IY0"},
    {"we'll", "W IY0 L"},
    {"we're", "W IH0 R"},
    {"weapon", "W EH1 P AH0 N"},
    {"wear", "W EH1 R"},
    {"weather", "W EH1 DH ER0"},
    {"welcome", "W EH1 L K AH0 M"},
    {"well", "W EH1 L"},
    {"were", "W ER0"},
    {"weren't", "W ER1 AH0 N T"},
    {"what", "W AH1 T"},
    {"what's", "W AH1 T S"},
    {"when", "W EH1 N"},
    {"where", "W EH1 R"},
    {"which", "W IH1 CH"},
    {"while", "W AY1 L"},
    {"who", "HH UW1"},
    {"whole", "HH OW1 L"},
    {"whose", "HH UW1 Z"},
    {"why", "W AY1"},
    {"will", "W IH0 L"},
    {"wind", "W IH1 N D"},
    {"window", "W IH1 N D OW0"},
    {"windows", "W IH1 N D OW0 Z"},
    {"with", "W IH0 DH"},
    {"without", "W IH0 TH AW1 T"},
    {"wizard", "W IH1 Z ER0 D"},
    {"wolf", "W UH1 L F"},
    {"woman", "W UH1 M AH0 N"},
    {"women", "W IH1 M AH0 N"},
    {"won", "W AH1 N"},
    {"won't", "W OW1 N T"},
    {"wonder", "W AH1 N D ER0"},
    {"word", "W ER1 D"},
    {"work", "W ER1 K"},
    {"world", "W ER1 L D"},
    {"would", "W UH0 D"},
    {"wouldn't", "W UH1 D AH0 N T"},
    {"write", "R AY1 T"},
    {"wrong", "R AO1 NG"},
    {"yeah", "Y AE1"},
    {"year", "Y IH1 R"},
    {"years", "Y IH1 R Z"},
    {"yes", "Y EH1 S"},
    {"yet", "Y EH1 T"},
    {"you", "Y UW0"},
    {"you'll", "Y UW0 L"},
    {"you're", "Y UH0 R"},
    {"you've", "Y UW0 V"},
    {"young", "Y AH1 NG"},
    {"your", "Y AO0 R"},
    {"yourself", "Y AO0 R S EH1 L F"},
    {"zero", "Z IH1 R OW0"},
    {"zombie", "Z AA1 M B IY0"},
    /* END LEXICON */
};

#define SS_LEX_COUNT ((int)(sizeof(ss_lex_table) / sizeof(ss_lex_table[0])))

static int lex_cmp(const char *word, int len, const char *entry)
{
    int i;
    for (i = 0; i < len; i++) {
        unsigned char a = (unsigned char)word[i];
        unsigned char b = (unsigned char)entry[i];
        if (b == 0) return 1; /* entry is a prefix of word */
        if (a != b) return a < b ? -1 : 1;
    }
    return entry[len] == 0 ? 0 : -1;
}

const char *ss_lexicon_lookup(const char *word, int len)
{
    int lo = 0, hi = SS_LEX_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = lex_cmp(word, len, ss_lex_table[mid].word);
        if (c == 0) return ss_lex_table[mid].pron;
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }
    return NULL;
}

int ss_lexicon_size(void) { return SS_LEX_COUNT; }

const char *ss_lexicon_word(int i)
{
    if (i < 0 || i >= SS_LEX_COUNT) return NULL;
    return ss_lex_table[i].word;
}

#undef SS_LEX_COUNT

/* ---- ss_rules.c ---- */
/*
 * Letter-to-sound rules for English.
 *
 * The rule format and much of the rule content follow the classic NRL
 * report: H. S. Elovitz, R. Johnson, A. McHugh, J. E. Shore, "Automatic
 * Translation of English Text to Phonetics by Means of Letter-to-Sound
 * Rules", NRL Report 7948, Naval Research Laboratory, 1976 (a US government
 * work in the public domain). Rules here are rewritten for ARPAbet output.
 *
 * Each rule is  LEFT [MATCH] RIGHT = PHONEMES.  The first rule (in order)
 * whose match string and both contexts fit wins. Context symbols:
 *
 *   ' '  word boundary          '#'  one or more vowels
 *   '.'  a voiced consonant     '%'  a suffix: E, ER, ES, ED, ING, ELY
 *   '&'  a sibilant             '@'  a consonant after which long U is "oo"
 *   '^'  exactly one consonant  '+'  a front vowel: E, I, Y
 *   ':'  zero or more consonants  '*' one or more consonants
 *   anything else: that literal character
 */
#include <string.h>

typedef struct ss_rule {
    const char *left, *match, *right, *out;
} ss_rule;

static const ss_rule rul_a[] = {
    {"", "A", " ", "AX"},
    {" ", "ARE", " ", "AA R"},
    {" ", "AR", "O", "AX R"},
    {"", "AR", "#", "EH R"},
    {"^", "AS", "#", "EY S"},
    {"", "A", "WA", "AX"},
    {"", "AW", "", "AO"},
    {" :", "ANY", "", "EH N IY"},
    {"", "A", "^+#", "EY"},
    {"#:", "ALLY", "", "AX L IY"},
    {" ", "AL", "#", "AX L"},
    {"", "AGAIN", "", "AX G EH N"},
    {"#:", "AG", "E", "IH JH"},
    {"", "A", "^+:#", "AE"},
    {" :", "A", "^+ ", "EY"},
    {"", "A", "^%", "EY"},
    {" ", "ARR", "", "AX R"},
    {"", "ARR", "", "AE R"},
    {" :", "AR", " ", "AA R"},
    {"", "AR", " ", "ER"},
    {"", "AR", "", "AA R"},
    {"", "AIR", "", "EH R"},
    {"", "AI", "", "EY"},
    {"", "AY", "", "EY"},
    {"", "AU", "", "AO"},
    {"#:", "AL", " ", "AX L"},
    {"#:", "ALS", " ", "AX L Z"},
    {"", "ALK", "", "AO K"},
    {"", "AL", "^", "AO L"},
    {" :", "ABLE", "", "EY B AX L"},
    {"", "ABLE", "", "AX B AX L"},
    {"", "ANG", "+", "EY N JH"},
    {"", "A", "", "AE"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_b[] = {
    {" ", "BE", "^#", "B IH"},
    {"", "BEING", "", "B IY IH NG"},
    {" ", "BOTH", " ", "B OW TH"},
    {" ", "BUS", "#", "B IH Z"},
    {"", "BUIL", "", "B IH L"},
    {"", "BB", "", "B"},
    {"", "B", "", "B"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_c[] = {
    {" ", "CH", "^", "K"},
    {"^E", "CH", "", "K"},
    {"", "CH", "", "CH"},
    {" S", "CI", "#", "S AY"},
    {"", "CI", "A", "SH"},
    {"", "CI", "O", "SH"},
    {"", "CI", "EN", "SH"},
    {"", "C", "+", "S"},
    {"", "CK", "", "K"},
    {"", "COM", "%", "K AH M"},
    {"", "CC", "+", "K S"},
    {"", "CC", "", "K"},
    {"", "C", "", "K"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_d[] = {
    {"#:", "DED", " ", "D IH D"},
    {".E", "D", " ", "D"},
    {"#:^E", "D", " ", "T"},
    {" ", "DE", "^#", "D IH"},
    {" ", "DO", " ", "D UW"},
    {" ", "DOES", "", "D AH Z"},
    {" ", "DOING", "", "D UW IH NG"},
    {" ", "DOW", "", "D AW"},
    {"", "DU", "A", "JH UW"},
    {"", "DD", "", "D"},
    {"", "DGE", "", "JH"},
    {"", "D", "", "D"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_e[] = {
    {"#:", "E", " ", ""},
    {"':^", "E", " ", ""},
    {" :", "E", " ", "IY"},
    {"#", "ED", " ", "D"},
    {"#:", "E", "D ", ""},
    {"", "EV", "ER", "EH V"},
    {"", "E", "^%", "IY"},
    {"", "ERI", "#", "IY R IY"},
    {"", "ERI", "", "EH R IH"},
    {"#:", "ER", "#", "ER"},
    {"", "ER", "#", "EH R"},
    {"", "ER", "", "ER"},
    {" ", "EVEN", "", "IY V EH N"},
    {"#:", "E", "W", ""},
    {"@", "EW", "", "UW"},
    {"", "EW", "", "Y UW"},
    {"", "E", "O", "IY"},
    {"#:&", "ES", " ", "IH Z"},
    {"#:", "E", "S ", ""},
    {"#:", "ELY", " ", "L IY"},
    {"#:", "EMENT", "", "M EH N T"},
    {"", "EFUL", "", "F UH L"},
    {"", "EE", "", "IY"},
    {"", "EARN", "", "ER N"},
    {" ", "EAR", "^", "ER"},
    {"", "EAD", "", "EH D"},
    {"#:", "EA", " ", "IY AX"},
    {"", "EA", "SU", "EH"},
    {"", "EA", "", "IY"},
    {"", "EIGH", "", "EY"},
    {"", "EI", "", "IY"},
    {" ", "EYE", "", "AY"},
    {"", "EY", "", "IY"},
    {"", "EU", "", "Y UW"},
    {"", "E", "", "EH"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_f[] = {
    {"", "FUL", "", "F UH L"},
    {"", "FF", "", "F"},
    {"", "F", "", "F"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_g[] = {
    {"", "GIV", "", "G IH V"},
    {" ", "G", "I^", "G"},
    {"", "GE", "T", "G EH"},
    {"SU", "GGES", "", "G JH EH S"},
    {"", "GG", "", "G"},
    {" B#", "G", "", "G"},
    {"", "G", "+", "JH"},
    {"", "GREAT", "", "G R EY T"},
    {"#", "GH", "", ""},
    {"", "G", "", "G"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_h[] = {
    {" ", "HAV", "", "HH AE V"},
    {" ", "HERE", "", "HH IY R"},
    {" ", "HOUR", "", "AW ER"},
    {"", "HOW", "", "HH AW"},
    {"", "H", "#", "HH"},
    {"", "H", "", ""},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_i[] = {
    {" ", "IN", "", "IH N"},
    {" ", "I", " ", "AY"},
    {"", "IN", "D", "AY N"},
    {"", "IER", "", "IY ER"},
    {"#:R", "IED", "", "IY D"},
    {"", "IED", " ", "AY D"},
    {"", "IEN", "", "IY EH N"},
    {"", "IE", "T", "AY EH"},
    {" :", "I", "%", "AY"},
    {"", "I", "%", "IY"},
    {"", "IE", "", "IY"},
    {"", "I", "^+:#", "IH"},
    {"", "IR", "#", "AY R"},
    {"", "IZ", "%", "AY Z"},
    {"", "IS", "%", "AY Z"},
    {"", "I", "D%", "AY"},
    {"+^", "I", "^+", "IH"},
    {"", "I", "T%", "AY"},
    {"#:^", "I", "^+", "IH"},
    {"", "I", "^E ", "AY"},
    {"", "I", "^ES ", "AY"},
    {"", "I", "^ED ", "AY"},
    {"", "IR", "", "ER"},
    {"", "IGH", "", "AY"},
    {"", "ILD", "", "AY L D"},
    {"", "IGN", " ", "AY N"},
    {"", "IGN", "^", "AY N"},
    {"", "IGN", "%", "AY N"},
    {"", "IQUE", "", "IY K"},
    {"", "I", "", "IH"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_j[] = {
    {"", "J", "", "JH"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_k[] = {
    {" ", "K", "N", ""},
    {"", "K", "", "K"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_l[] = {
    {"", "LO", "C#", "L OW"},
    {"L", "L", "", ""},
    {"#:^", "L", "%", "AX L"},
    {"", "LEAD", "", "L IY D"},
    {"", "L", "", "L"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_m[] = {
    {"", "MOV", "", "M UW V"},
    {"", "MM", "", "M"},
    {"", "M", "", "M"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_n[] = {
    {"E", "NG", "+", "N JH"},
    {"", "NG", "R", "NG G"},
    {"", "NG", "#", "NG G"},
    {"", "NGL", "%", "NG G AX L"},
    {"", "NG", "", "NG"},
    {"", "NK", "", "NG K"},
    {" ", "NOW", " ", "N AW"},
    {"", "NN", "", "N"},
    {"", "N", "", "N"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_o[] = {
    {"", "OF", " ", "AX V"},
    {"", "OROUGH", "", "ER OW"},
    {"#:", "OR", " ", "ER"},
    {"#:", "ORS", " ", "ER Z"},
    {"", "OR", "", "AO R"},
    {" ", "ONE", "", "W AH N"},
    {"", "OW", "", "OW"},
    {" ", "OVER", "", "OW V ER"},
    {"", "OV", "", "AH V"},
    {"", "O", "^%", "OW"},
    {"", "O", "^EN", "OW"},
    {"", "O", "^I#", "OW"},
    {"", "OL", "D", "OW L"},
    {"", "OUGHT", "", "AO T"},
    {"", "OUGH", "", "AH F"},
    {" ", "OU", "", "AW"},
    {"H", "OU", "S#", "AW"},
    {"", "OUS", "", "AX S"},
    {"", "OUR", "", "AO R"},
    {"", "OULD", "", "UH D"},
    {"^", "OU", "^L", "AH"},
    {"", "OUP", "", "UW P"},
    {"", "OU", "", "AW"},
    {"", "OY", "", "OY"},
    {"", "OING", "", "OW IH NG"},
    {"", "OI", "", "OY"},
    {"", "OOR", "", "AO R"},
    {"", "OOK", "", "UH K"},
    {"", "OOD", "", "UH D"},
    {"", "OO", "", "UW"},
    {"", "O", "E", "OW"},
    {"", "O", " ", "OW"},
    {"", "OA", "", "OW"},
    {" ", "ONLY", "", "OW N L IY"},
    {" ", "ONCE", "", "W AH N S"},
    {"", "ON'T", "", "OW N T"},
    {"C", "O", "N", "AA"},
    {"", "O", "NG", "AO"},
    {" :^", "O", "N", "AH"},
    {"I", "ON", "", "AX N"},
    {"#:", "ON", " ", "AX N"},
    {"#^", "ON", "", "AX N"},
    {"", "O", "ST ", "OW"},
    {"", "OF", "^", "AO F"},
    {"", "OTHER", "", "AH DH ER"},
    {"", "OSS", " ", "AO S"},
    {"#:^", "OM", "", "AH M"},
    {"", "O", "", "AA"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_p[] = {
    {"", "PH", "", "F"},
    {"", "PEOP", "", "P IY P"},
    {"", "POW", "", "P AW"},
    {"", "PUT", " ", "P UH T"},
    {"", "PP", "", "P"},
    {"", "P", "", "P"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_q[] = {
    {"", "QUAR", "", "K W AO R"},
    {"", "QU", "", "K W"},
    {"", "Q", "", "K"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_r[] = {
    {" ", "RE", "^#", "R IY"},
    {"", "RR", "", "R"},
    {"", "R", "", "R"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_s[] = {
    {"", "SH", "", "SH"},
    {"#", "SION", "", "ZH AX N"},
    {"", "SOME", "", "S AH M"},
    {"#", "SUR", "#", "ZH ER"},
    {"", "SUR", "#", "SH ER"},
    {"#", "SU", "#", "ZH UW"},
    {"#", "SSU", "#", "SH UW"},
    {"#", "SED", " ", "Z D"},
    {"#", "S", "#", "Z"},
    {"", "SAID", "", "S EH D"},
    {"^", "SION", "", "SH AX N"},
    {"", "S", "S", ""},
    {".", "S", " ", "Z"},
    {"#:.E", "S", " ", "Z"},
    {"#:^##", "S", " ", "Z"},
    {"#:^#", "S", " ", "S"},
    {"U", "S", " ", "S"},
    {" :#", "S", " ", "Z"},
    {" ", "SCH", "", "S K"},
    {"", "S", "C+", ""},
    {"#", "SM", "", "Z M"},
    {"#", "SN", "'", "Z AX N"},
    {"", "S", "", "S"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_t[] = {
    {" ", "THE", " ", "DH AX"},
    {"", "TO", " ", "T UW"},
    {"", "THAT", " ", "DH AE T"},
    {" ", "THIS", " ", "DH IH S"},
    {" ", "THEY", "", "DH EY"},
    {" ", "THERE", "", "DH EH R"},
    {"", "THER", "", "DH ER"},
    {"", "THEIR", "", "DH EH R"},
    {" ", "THAN", " ", "DH AE N"},
    {" ", "THEM", " ", "DH EH M"},
    {"", "THESE", " ", "DH IY Z"},
    {" ", "THEN", "", "DH EH N"},
    {"", "THROUGH", "", "TH R UW"},
    {"", "THOSE", "", "DH OW Z"},
    {"", "THOUGH", " ", "DH OW"},
    {" ", "THUS", "", "DH AH S"},
    {"", "TH", "", "TH"},
    {"#:", "TED", " ", "T IH D"},
    {"S", "TI", "#N", "CH"},
    {"", "TI", "O", "SH"},
    {"", "TI", "A", "SH"},
    {"", "TIEN", "", "SH AX N"},
    {"", "TUR", "#", "CH ER"},
    {"", "TU", "A", "CH UW"},
    {" ", "TWO", "", "T UW"},
    {"", "TCH", "", "CH"},
    {"", "TT", "", "T"},
    {"", "T", "", "T"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_u[] = {
    {" ", "UN", "I", "Y UW N"},
    {" ", "UN", "", "AH N"},
    {" ", "UPON", "", "AX P AO N"},
    {"@", "UR", "#", "UH R"},
    {"", "UR", "#", "Y UH R"},
    {"", "UR", "", "ER"},
    {"", "U", "^ ", "AH"},
    {"", "U", "^^", "AH"},
    {"", "UY", "", "AY"},
    {" G", "U", "#", ""},
    {"G", "U", "%", ""},
    {"G", "U", "#", "W"},
    {"#N", "U", "", "Y UW"},
    {"@", "U", "", "UW"},
    {"", "U", "", "Y UW"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_v[] = {
    {"", "VIEW", "", "V Y UW"},
    {"", "V", "", "V"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_w[] = {
    {" ", "WERE", "", "W ER"},
    {"", "WA", "S", "W AA"},
    {"", "WA", "T", "W AA"},
    {"", "WHERE", "", "W EH R"},
    {"", "WHAT", "", "W AA T"},
    {"", "WHOL", "", "HH OW L"},
    {"", "WHO", "", "HH UW"},
    {"", "WH", "", "W"},
    {"", "WAR", "", "W AO R"},
    {"", "WOR", "^", "W ER"},
    {"", "WR", "", "R"},
    {"", "W", "", "W"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_x[] = {
    {" ", "X", "", "Z"},
    {"", "X", "", "K S"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_y[] = {
    {"", "YOUNG", "", "Y AH NG"},
    {" ", "YOU", "", "Y UW"},
    {" ", "YES", "", "Y EH S"},
    {" ", "Y", "", "Y"},
    {"#:^", "Y", " ", "IY"},
    {"#:^", "Y", "I", "IY"},
    {" :", "Y", " ", "AY"},
    {" :", "Y", "#", "AY"},
    {" :", "Y", "^+:#", "IH"},
    {" :", "Y", "^#", "AY"},
    {"#", "Y", "", "Y"},
    {"", "Y", "", "IH"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule rul_z[] = {
    {"", "ZZ", "", "Z"},
    {"", "Z", "", "Z"},
    {NULL, NULL, NULL, NULL}
};

static const ss_rule *const rul_by_letter[26] = {
    rul_a, rul_b, rul_c, rul_d, rul_e, rul_f, rul_g, rul_h, rul_i,
    rul_j, rul_k, rul_l, rul_m, rul_n, rul_o, rul_p, rul_q, rul_r,
    rul_s, rul_t, rul_u, rul_v, rul_w, rul_x, rul_y, rul_z
};

/* ---- character classes ---- */

static int rul_isletter(char c) { return c >= 'A' && c <= 'Z'; }
static int rul_isvowel(char c) { return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U'; }
static int rul_iscons(char c) { return rul_isletter(c) && !rul_isvowel(c); }
static int rul_isvoiced(char c)
{
    return c == 'B' || c == 'D' || c == 'V' || c == 'G' || c == 'J' || c == 'L' ||
           c == 'M' || c == 'N' || c == 'R' || c == 'W' || c == 'Z';
}
static int rul_isfront(char c) { return c == 'E' || c == 'I' || c == 'Y'; }

/* `w` is the padded word (" WORD "), n its length. Positions out of range
 * read as a word boundary. */
static char rul_at(const char *w, int n, int i) { return (i < 0 || i >= n) ? ' ' : w[i]; }

/* Match left context `ctx` ending just before position `pos` (scanning
 * right-to-left). */
static int rul_left(const char *ctx, const char *w, int n, int pos)
{
    int ci = (int)strlen(ctx) - 1;
    int i = pos - 1;
    for (; ci >= 0; ci--) {
        char p = ctx[ci];
        char c = rul_at(w, n, i);
        switch (p) {
        case ' ':
            if (rul_isletter(c)) return 0;
            i--;
            break;
        case '#':
            if (!rul_isvowel(c)) return 0;
            while (rul_isvowel(rul_at(w, n, i))) i--;
            break;
        case ':':
            while (rul_iscons(rul_at(w, n, i))) i--;
            break;
        case '*':
            if (!rul_iscons(c)) return 0;
            while (rul_iscons(rul_at(w, n, i))) i--;
            break;
        case '^':
            if (!rul_iscons(c)) return 0;
            i--;
            break;
        case '.':
            if (!rul_isvoiced(c)) return 0;
            i--;
            break;
        case '+':
            if (!rul_isfront(c)) return 0;
            i--;
            break;
        case '&':
            if (c == 'H') {
                char d = rul_at(w, n, i - 1);
                if (d == 'C' || d == 'S') { i -= 2; break; }
                return 0;
            }
            if (c == 'S' || c == 'C' || c == 'G' || c == 'Z' || c == 'X' || c == 'J') { i--; break; }
            return 0;
        case '@':
            if (c == 'H') {
                char d = rul_at(w, n, i - 1);
                if (d == 'T' || d == 'C' || d == 'S') { i -= 2; break; }
                return 0;
            }
            if (c == 'T' || c == 'S' || c == 'R' || c == 'D' || c == 'L' || c == 'Z' ||
                c == 'N' || c == 'J') { i--; break; }
            return 0;
        default:
            if (c != p) return 0;
            i--;
            break;
        }
    }
    return 1;
}

/* Match right context `ctx` starting at position `pos`. */
static int rul_right(const char *ctx, const char *w, int n, int pos)
{
    int i = pos;
    const char *p;
    for (p = ctx; *p; p++) {
        char c = rul_at(w, n, i);
        switch (*p) {
        case ' ':
            if (rul_isletter(c)) return 0;
            i++;
            break;
        case '#':
            if (!rul_isvowel(c)) return 0;
            while (rul_isvowel(rul_at(w, n, i))) i++;
            break;
        case ':':
            while (rul_iscons(rul_at(w, n, i))) i++;
            break;
        case '*':
            if (!rul_iscons(c)) return 0;
            while (rul_iscons(rul_at(w, n, i))) i++;
            break;
        case '^':
            if (!rul_iscons(c)) return 0;
            i++;
            break;
        case '.':
            if (!rul_isvoiced(c)) return 0;
            i++;
            break;
        case '+':
            if (!rul_isfront(c)) return 0;
            i++;
            break;
        case '&':
            if ((c == 'C' || c == 'S') && rul_at(w, n, i + 1) == 'H') { i += 2; break; }
            if (c == 'S' || c == 'C' || c == 'G' || c == 'Z' || c == 'X' || c == 'J') { i++; break; }
            return 0;
        case '@':
            if ((c == 'T' || c == 'C' || c == 'S') && rul_at(w, n, i + 1) == 'H') { i += 2; break; }
            if (c == 'T' || c == 'S' || c == 'R' || c == 'D' || c == 'L' || c == 'Z' ||
                c == 'N' || c == 'J') { i++; break; }
            return 0;
        case '%':
            /* suffix: E, ER, ES, ED, ELY, ING */
            if (c == 'E') {
                char d = rul_at(w, n, i + 1);
                if (d == 'L' && rul_at(w, n, i + 2) == 'Y') { i += 3; break; }
                if (d == 'R' || d == 'S' || d == 'D') { i += 2; break; }
                i++;
                break;
            }
            if (c == 'I' && rul_at(w, n, i + 1) == 'N' && rul_at(w, n, i + 2) == 'G') { i += 3; break; }
            return 0;
        default:
            if (c != *p) return 0;
            i++;
            break;
        }
    }
    return 1;
}

static int rul_append(char *out, int cap, int len, const char *s)
{
    if (!*s) return len;
    if (len > 0) {
        if (len < cap - 1) out[len] = ' ';
        len++;
    }
    while (*s) {
        if (len < cap - 1) out[len] = *s;
        len++;
        s++;
    }
    return len;
}

int ss_rules_translate(const char *upper_word, int len, char *out, int cap)
{
    /* pad with a boundary on each side so contexts can see the edges */
    char w[130];
    int n, pos, olen = 0;
    if (cap > 0) out[0] = 0;
    if (len <= 0) return 0;
    if (len > 126) len = 126;
    w[0] = ' ';
    memcpy(w + 1, upper_word, (size_t)len);
    w[len + 1] = ' ';
    n = len + 2;
    w[n] = 0;

    pos = 1;
    while (pos < n - 1) {
        char c = w[pos];
        const ss_rule *r;
        int matched = 0;
        if (!rul_isletter(c)) { pos++; continue; } /* apostrophes etc. */
        for (r = rul_by_letter[c - 'A']; r->match; r++) {
            int mlen = (int)strlen(r->match);
            if (pos + mlen > n) continue;
            if (memcmp(w + pos, r->match, (size_t)mlen) != 0) continue;
            if (!rul_left(r->left, w, n, pos)) continue;
            if (!rul_right(r->right, w, n, pos + mlen)) continue;
            olen = rul_append(out, cap, olen, r->out);
            pos += mlen;
            matched = 1;
            break;
        }
        if (!matched) pos++; /* every letter has a catch-all, but be safe */
    }
    if (cap > 0) out[olen < cap ? olen : cap - 1] = 0;
    return olen;
}

/* ---- ss_text.c ---- */
/*
 * Text front end: normalisation, tokenisation and pronunciation.
 *
 * Input is UTF-8. Letters outside ASCII are skipped, except typographic
 * apostrophes, dashes and ellipses, which are understood.
 */
#include <string.h>

/* ------------------------------------------------------------------------ */
/* Token buffer                                                             */
/* ------------------------------------------------------------------------ */

void ss_tokens_init(ss_tokens *t)
{
    memset(t, 0, sizeof(*t));
}

void ss_tokens_free(ss_tokens *t)
{
    SS_FREE(t->tok);
    SS_FREE(t->word);
    memset(t, 0, sizeof(*t));
}

void ss_tokens_push(ss_tokens *t, int ph, int stress, unsigned flags, int word)
{
    ss_phtok *k;
    if (t->failed) return;
    if (t->ntok == t->captok) {
        int nc = t->captok ? t->captok * 2 : 64;
        ss_phtok *nt = (ss_phtok *)SS_REALLOC(t->tok, (size_t)nc * sizeof(ss_phtok));
        if (!nt) { t->failed = 1; return; }
        t->tok = nt;
        t->captok = nc;
    }
    k = &t->tok[t->ntok++];
    k->ph = (unsigned char)ph;
    k->stress = (signed char)stress;
    k->flags = (unsigned char)flags;
    k->pad = 0;
    k->word = word;
}

int ss_tokens_add_word(ss_tokens *t, int offset, int length)
{
    if (t->failed) return -1;
    if (t->nword == t->capword) {
        int nc = t->capword ? t->capword * 2 : 16;
        ss_word *nw = (ss_word *)SS_REALLOC(t->word, (size_t)nc * sizeof(ss_word));
        if (!nw) { t->failed = 1; return -1; }
        t->word = nw;
        t->capword = nc;
    }
    t->word[t->nword].text_offset = offset;
    t->word[t->nword].text_length = length;
    return t->nword++;
}

/* ------------------------------------------------------------------------ */
/* ARPAbet parsing and stress                                               */
/* ------------------------------------------------------------------------ */

static int txt_isalpha(unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int txt_isdigit(unsigned char c) { return c >= '0' && c <= '9'; }
static char txt_lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static char txt_upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }

int ss_parse_arpabet(const char *s, int len, ss_tokens *out, int word, unsigned flags)
{
    int i = 0, count = 0;
    unsigned wf = flags;
    while (i < len) {
        int st, ph, stress = -1, nlen;
        while (i < len && !txt_isalpha((unsigned char)s[i])) i++;
        st = i;
        while (i < len && txt_isalpha((unsigned char)s[i])) i++;
        nlen = i - st;
        if (nlen == 0) break;
        if (i < len && txt_isdigit((unsigned char)s[i])) {
            stress = s[i] - '0';
            if (stress > 2) stress = 2;
            i++;
        }
        ph = ss_phoneme_from_name(s + st, nlen);
        if (ph < 0 || ph == SS_PH_PAU) continue;
        if (ph == SS_PH_AH && stress == 0) ph = SS_PH_AX;
        if (!ss_ph_is_vowel(ph)) stress = 0;
        ss_tokens_push(out, ph, stress, wf, word);
        wf &= ~SS_TF_WORD_START;
        count++;
    }
    return count;
}

static int txt_ends_with(const char *up, int len, const char *suf)
{
    int sl = (int)strlen(suf);
    return len >= sl && memcmp(up + len - sl, suf, (size_t)sl) == 0;
}

static int txt_starts_with(const char *up, int len, const char *pre)
{
    int pl = (int)strlen(pre);
    return len > pl && memcmp(up, pre, (size_t)pl) == 0;
}

/*
 * Assign lexical stress to tokens [from, to) that came without stress marks
 * (stress == -1). `up` is the uppercase spelling, or NULL.
 */
static void txt_assign_stress(ss_tokens *t, int from, int to, const char *up, int ulen)
{
    int vidx[64];
    int nv = 0, i, target = -1;
    for (i = from; i < to; i++) {
        if (ss_ph_is_vowel(t->tok[i].ph)) {
            if (t->tok[i].stress >= 0) return; /* already marked */
            if (nv < 64) vidx[nv++] = i;
        }
    }
    if (nv == 0) return;
    if (nv == 1) {
        target = 0;
    } else {
        if (up) {
            if (txt_ends_with(up, ulen, "ITY") || txt_ends_with(up, ulen, "ETY") ||
                txt_ends_with(up, ulen, "OGY") || txt_ends_with(up, ulen, "APHY") ||
                txt_ends_with(up, ulen, "ICAL") || txt_ends_with(up, ulen, "IAN") ||
                txt_ends_with(up, ulen, "IUM") || txt_ends_with(up, ulen, "IAL")) {
                target = nv >= 3 ? nv - 3 : 0;
            } else if (txt_ends_with(up, ulen, "TION") || txt_ends_with(up, ulen, "SION") ||
                       txt_ends_with(up, ulen, "CIAN") || txt_ends_with(up, ulen, "CIOUS") ||
                       txt_ends_with(up, ulen, "TIOUS") || txt_ends_with(up, ulen, "CIAL") ||
                       txt_ends_with(up, ulen, "TIAL") || txt_ends_with(up, ulen, "IC") ||
                       txt_ends_with(up, ulen, "ICS") || txt_ends_with(up, ulen, "TIONS") ||
                       txt_ends_with(up, ulen, "SIONS")) {
                target = nv - 2;
            } else if (txt_ends_with(up, ulen, "EE") || txt_ends_with(up, ulen, "EER") ||
                       txt_ends_with(up, ulen, "ESE") || txt_ends_with(up, ulen, "IQUE") ||
                       txt_ends_with(up, ulen, "OON") || txt_ends_with(up, ulen, "ETTE")) {
                target = nv - 1;
            }
        }
        if (target < 0) {
            int first = 0;
            if (up && (txt_starts_with(up, ulen, "BE") || txt_starts_with(up, ulen, "DE") ||
                       txt_starts_with(up, ulen, "RE") || txt_starts_with(up, ulen, "UN") ||
                       txt_starts_with(up, ulen, "DIS") || txt_starts_with(up, ulen, "MIS") ||
                       txt_starts_with(up, ulen, "EX") || txt_starts_with(up, ulen, "A"))) {
                int ph = t->tok[vidx[0]].ph;
                if (ph == SS_PH_IH || ph == SS_PH_IY || ph == SS_PH_AH || ph == SS_PH_AX ||
                    ph == SS_PH_EH)
                    first = 1;
            }
            for (i = first; i < nv; i++) {
                if (t->tok[vidx[i]].ph != SS_PH_AX) { target = i; break; }
            }
            if (target < 0) target = first < nv ? first : 0;
        }
    }
    if (target < 0) target = 0;
    if (target >= nv) target = nv - 1;
    for (i = 0; i < nv; i++) {
        ss_phtok *k = &t->tok[vidx[i]];
        k->stress = (signed char)(i == target ? 1 : 0);
        if (k->stress == 0 && k->ph == SS_PH_AH) k->ph = SS_PH_AX;
    }
}

static void txt_finish_word(ss_tokens *t, int from)
{
    int i, primary = 0;
    for (i = from; i < t->ntok; i++) {
        if (t->tok[i].stress < 0) t->tok[i].stress = 0;
        if (ss_ph_is_vowel(t->tok[i].ph) && t->tok[i].stress == 1) primary = 1;
    }
    if (!primary) {
        for (i = from; i < t->ntok; i++) t->tok[i].flags |= SS_TF_FUNCTION;
    }
}

/* ------------------------------------------------------------------------ */
/* Words                                                                    */
/* ------------------------------------------------------------------------ */

static const char *const txt_letter_names[26] = {
    "EY1", "B IY1", "S IY1", "D IY1", "IY1", "EH1 F", "JH IY1", "EY1 CH", "AY1",
    "JH EY1", "K EY1", "EH1 L", "EH1 M", "EH1 N", "OW1", "P IY1", "K Y UW1",
    "AA1 R", "EH1 S", "T IY1", "Y UW1", "V IY1", "D AH1 B AH0 L Y UW0", "EH1 K S",
    "W AY1", "Z IY1"
};

typedef struct txt_state {
    ss_tokens *t;
    unsigned sentence_flags;
    int words_in_sentence;
    int the_tok;       /* index of the schwa in a preceding "the", or -1 */
} txt_state;

static const char *const txt_wh_words[] = {
    "what", "where", "when", "why", "who", "how", "which", "whose", "whom", NULL
};

static int txt_is_wh(const char *lw, int len)
{
    int i;
    for (i = 0; txt_wh_words[i]; i++) {
        if ((int)strlen(txt_wh_words[i]) == len && memcmp(txt_wh_words[i], lw, (size_t)len) == 0)
            return 1;
    }
    return 0;
}

static void txt_begin_word(txt_state *st, const char *lw, int len)
{
    if (st->words_in_sentence == 0) {
        st->sentence_flags = (lw && txt_is_wh(lw, len)) ? SS_TF_WH : 0;
    }
    st->words_in_sentence++;
}

/* After a word has been pushed starting at token `from`: fix up "the". */
static void txt_after_word(txt_state *st, int from, const char *lw, int len)
{
    ss_tokens *t = st->t;
    if (st->the_tok >= 0 && from < t->ntok && ss_ph_is_vowel(t->tok[from].ph)) {
        t->tok[st->the_tok].ph = SS_PH_IY;
    }
    st->the_tok = -1;
    if (lw && len == 3 && memcmp(lw, "the", 3) == 0 && t->ntok > from &&
        t->tok[t->ntok - 1].ph == SS_PH_AX) {
        st->the_tok = t->ntok - 1;
    }
}

static void txt_spell(txt_state *st, const char *w, int len, int word)
{
    ss_tokens *t = st->t;
    int i, from = t->ntok;
    unsigned fl = SS_TF_WORD_START | st->sentence_flags;
    for (i = 0; i < len; i++) {
        char c = txt_upper(w[i]);
        int k0 = t->ntok, k;
        if (c < 'A' || c > 'Z') continue;
        ss_parse_arpabet(txt_letter_names[c - 'A'], (int)strlen(txt_letter_names[c - 'A']),
                         t, word, fl);
        fl &= ~SS_TF_WORD_START;
        /* letters before the last one get secondary stress */
        if (i < len - 1) {
            for (k = k0; k < t->ntok; k++)
                if (t->tok[k].stress == 1) t->tok[k].stress = 2;
        }
    }
    txt_finish_word(t, from);
}

/* Pronounce one lowercase-able word. */
static void txt_word(txt_state *st, const char *w, int len, int word)
{
    ss_tokens *t = st->t;
    char lw[128], up[128];
    char buf[512];
    const char *pron;
    int i, from = t->ntok, n;
    unsigned fl;
    if (len <= 0) return;
    if (len > 120) len = 120;
    for (i = 0; i < len; i++) {
        lw[i] = txt_lower(w[i]);
        up[i] = txt_upper(w[i]);
    }
    lw[len] = up[len] = 0;
    txt_begin_word(st, lw, len);
    fl = SS_TF_WORD_START | st->sentence_flags;

    /* Acronyms: an all-caps word without vowels (NPC, HP, XP) is spelled. */
    if (len >= 2) {
        int caps = 1, vowels = 0;
        for (i = 0; i < len; i++) {
            if (w[i] >= 'a' && w[i] <= 'z') caps = 0;
            if (up[i] == 'A' || up[i] == 'E' || up[i] == 'I' || up[i] == 'O' || up[i] == 'U')
                vowels = 1;
        }
        if (caps && !vowels && !ss_lexicon_lookup(lw, len)) {
            txt_spell(st, w, len, word);
            txt_after_word(st, from, NULL, 0);
            return;
        }
    }

    pron = ss_lexicon_lookup(lw, len);
    if (pron) {
        ss_parse_arpabet(pron, (int)strlen(pron), t, word, fl);
        txt_finish_word(t, from);
        txt_after_word(st, from, lw, len);
        return;
    }

    /* Possessives and simple plurals / -ing of dictionary words. */
    {
        int stem = 0, suffix = 0; /* 1 = s/z, 2 = ing, 3 = ed */
        if (len > 2 && lw[len - 2] == '\'' && lw[len - 1] == 's') { stem = len - 2; suffix = 1; }
        else if (len > 3 && lw[len - 1] == 's' && lw[len - 2] != 's' && lw[len - 2] != '\'') { stem = len - 1; suffix = 1; }
        else if (len > 5 && memcmp(lw + len - 3, "ing", 3) == 0) { stem = len - 3; suffix = 2; }
        else if (len > 4 && memcmp(lw + len - 2, "ed", 2) == 0) { stem = len - 2; suffix = 3; }
        if (stem > 0) {
            const char *sp = ss_lexicon_lookup(lw, stem);
            if (!sp && suffix == 2) {
                /* "making" -> "make", "running" -> "run" */
                char tmp[128];
                memcpy(tmp, lw, (size_t)stem);
                tmp[stem] = 'e';
                sp = ss_lexicon_lookup(tmp, stem + 1);
                if (!sp && stem >= 2 && lw[stem - 1] == lw[stem - 2]) sp = ss_lexicon_lookup(lw, stem - 1);
            }
            if (!sp && suffix == 3) {
                sp = ss_lexicon_lookup(lw, stem + 1); /* "moved" -> "move" */
            }
            if (sp) {
                int last;
                ss_parse_arpabet(sp, (int)strlen(sp), t, word, fl);
                last = t->ntok > from ? t->tok[t->ntok - 1].ph : (int)SS_PH_PAU;
                if (suffix == 1) {
                    if (last == SS_PH_S || last == SS_PH_Z || last == SS_PH_SH || last == SS_PH_ZH ||
                        last == SS_PH_CH || last == SS_PH_JH) {
                        ss_tokens_push(t, SS_PH_IH, 0, 0, word);
                        ss_tokens_push(t, SS_PH_Z, 0, 0, word);
                    } else if (ss_ph_is_voiced(last)) {
                        ss_tokens_push(t, SS_PH_Z, 0, 0, word);
                    } else {
                        ss_tokens_push(t, SS_PH_S, 0, 0, word);
                    }
                } else if (suffix == 2) {
                    ss_tokens_push(t, SS_PH_IH, 0, 0, word);
                    ss_tokens_push(t, SS_PH_NG, 0, 0, word);
                } else {
                    if (last == SS_PH_T || last == SS_PH_D) {
                        ss_tokens_push(t, SS_PH_IH, 0, 0, word);
                        ss_tokens_push(t, SS_PH_D, 0, 0, word);
                    } else if (ss_ph_is_voiced(last)) {
                        ss_tokens_push(t, SS_PH_D, 0, 0, word);
                    } else {
                        ss_tokens_push(t, SS_PH_T, 0, 0, word);
                    }
                }
                txt_finish_word(t, from);
                txt_after_word(st, from, lw, len);
                return;
            }
        }
    }

    /* Letter-to-sound rules. */
    n = ss_rules_translate(up, len, buf, (int)sizeof(buf));
    (void)n;
    ss_parse_arpabet(buf, (int)strlen(buf), t, word, fl);
    if (t->ntok == from) {
        /* nothing pronounceable: spell it */
        txt_spell(st, w, len, word);
        txt_after_word(st, from, NULL, 0);
        return;
    }
    txt_assign_stress(t, from, t->ntok, up, len);
    txt_finish_word(t, from);
    txt_after_word(st, from, lw, len);
}

/* Pronounce a word from the lexicon by name (used for expansions). */
static void txt_say(txt_state *st, const char *lw, int word)
{
    txt_word(st, lw, (int)strlen(lw), word);
}

/* ------------------------------------------------------------------------ */
/* Numbers                                                                  */
/* ------------------------------------------------------------------------ */

static const char *const txt_ones[20] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
    "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
    "seventeen", "eighteen", "nineteen"
};
static const char *const txt_tens[10] = {
    "", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"
};
static const char *const txt_ord_ones[20] = {
    "zeroth", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth",
    "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth",
    "sixteenth", "seventeenth", "eighteenth", "nineteenth"
};
static const char *const txt_ord_tens[10] = {
    "", "", "twentieth", "thirtieth", "fortieth", "fiftieth", "sixtieth", "seventieth",
    "eightieth", "ninetieth"
};

/* Say 0..999. `ordinal` makes the last word an ordinal. */
static void txt_say_999(txt_state *st, int n, int word, int ordinal)
{
    int h = n / 100, r = n % 100;
    if (h) {
        txt_say(st, txt_ones[h], word);
        txt_say(st, (ordinal && r == 0) ? "hundredth" : "hundred", word);
    }
    if (r || !h) {
        if (r < 20) {
            if (r || !h) txt_say(st, ordinal ? txt_ord_ones[r] : txt_ones[r], word);
        } else {
            int tn = r / 10, o = r % 10;
            if (o == 0) {
                txt_say(st, ordinal ? txt_ord_tens[tn] : txt_tens[tn], word);
            } else {
                txt_say(st, txt_tens[tn], word);
                txt_say(st, ordinal ? txt_ord_ones[o] : txt_ones[o], word);
            }
        }
    }
}

static void txt_say_int(txt_state *st, unsigned long long n, int word, int ordinal)
{
    static const char *const scale[5] = {"", "thousand", "million", "billion", "trillion"};
    static const char *const oscale[5] = {"", "thousandth", "millionth", "billionth", "trillionth"};
    int groups[5], ng = 0, i;
    if (n == 0) { txt_say(st, ordinal ? "zeroth" : "zero", word); return; }
    while (n && ng < 5) { groups[ng++] = (int)(n % 1000); n /= 1000; }
    for (i = ng - 1; i >= 0; i--) {
        int lastgroup = 1, j;
        if (!groups[i]) continue;
        for (j = i - 1; j >= 0; j--) if (groups[j]) lastgroup = 0;
        if (i > 0) {
            txt_say_999(st, groups[i], word, 0);
            txt_say(st, (ordinal && lastgroup) ? oscale[i] : scale[i], word);
        } else {
            txt_say_999(st, groups[i], word, ordinal);
        }
    }
}

static void txt_say_digits(txt_state *st, const char *s, int len, int word)
{
    int i;
    for (i = 0; i < len; i++) {
        if (txt_isdigit((unsigned char)s[i])) txt_say(st, txt_ones[s[i] - '0'], word);
    }
}

/* ------------------------------------------------------------------------ */
/* Abbreviations and symbols                                                */
/* ------------------------------------------------------------------------ */

typedef struct txt_abbrev {
    const char *abbr;
    const char *full;
} txt_abbrev;

static const txt_abbrev txt_abbrevs[] = {
    {"mr", "mister"}, {"mrs", "missus"}, {"ms", "miz"}, {"dr", "doctor"},
    {"st", "saint"}, {"vs", "versus"}, {"etc", "et cetera"}, {"lt", "lieutenant"},
    {"sgt", "sergeant"}, {"capt", "captain"}, {"col", "colonel"}, {"gen", "general"},
    {"prof", "professor"}, {"jr", "junior"}, {"sr", "senior"}, {"no", "number"},
    {NULL, NULL}
};

static const char *txt_find_abbrev(const char *w, int len)
{
    int i, j;
    for (i = 0; txt_abbrevs[i].abbr; i++) {
        const char *a = txt_abbrevs[i].abbr;
        if ((int)strlen(a) != len) continue;
        for (j = 0; j < len; j++) if (txt_lower(w[j]) != a[j]) break;
        if (j == len) return txt_abbrevs[i].full;
    }
    return NULL;
}

/* ------------------------------------------------------------------------ */
/* Scanner                                                                  */
/* ------------------------------------------------------------------------ */

static void txt_pause(txt_state *st, int kind)
{
    ss_tokens *t = st->t;
    st->the_tok = -1;
    if (kind >= SS_PAUSE_PERIOD) st->words_in_sentence = 0;
    if (t->ntok == 0) return; /* no leading pauses */
    if (t->tok[t->ntok - 1].ph == SS_PH_PAU) {
        ss_phtok *p = &t->tok[t->ntok - 1];
        /* merge: keep the strongest; a question mark wins over a period */
        if (kind == SS_PAUSE_QUESTION || (kind > p->stress && p->stress != SS_PAUSE_QUESTION))
            p->stress = (signed char)kind;
        return;
    }
    ss_tokens_push(t, SS_PH_PAU, kind, 0, -1);
}

/* Is byte i the start of a typographic apostrophe (U+2019 / U+2018)? */
static int txt_is_curly_apos(const unsigned char *s, int n, int i)
{
    return i + 2 < n && s[i] == 0xE2 && s[i + 1] == 0x80 && (s[i + 2] == 0x99 || s[i + 2] == 0x98);
}


/* ASCII spelling of U+00C0..U+00FF (two chars per code point, '\0' padded;
 * "??" = not a letter) and U+0100..U+017F (one char each). */
static const char txt_latin1[64 * 2 + 1] =
    "A\0A\0A\0A\0A\0A\0AEC\0E\0E\0E\0E\0I\0I\0I\0I\0"
    "D\0N\0O\0O\0O\0O\0O\0??O\0U\0U\0U\0U\0Y\0THss"
    "a\0a\0a\0a\0a\0a\0aec\0e\0e\0e\0e\0i\0i\0i\0i\0"
    "d\0n\0o\0o\0o\0o\0o\0??o\0u\0u\0u\0u\0y\0thy\0";
static const char txt_latin_ext_a[128 + 1] =
    "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlLlLl"
    "NnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";

/* If a non-ASCII letter starts at s[i], return its byte length and write its
 * ASCII spelling (0-2 chars) to out, its length to outlen. Returns 0 for anything that is
 * not a letter (punctuation, symbols, invalid UTF-8). Letters from scripts
 * we cannot pronounce are consumed with an empty spelling. */
static int txt_utf8_letter(const unsigned char *s, int n, int i, char *out, int *outlen)
{
    unsigned cp;
    int len;
    unsigned char c = s[i];
    *outlen = 0;
    if (c < 0xC0) return 0;
    if (c < 0xE0) { len = 2; cp = c & 0x1Fu; }
    else if (c < 0xF0) { len = 3; cp = c & 0x0Fu; }
    else if (c < 0xF8) { len = 4; cp = c & 0x07u; }
    else return 0;
    if (i + len > n) return 0;
    {
        int k;
        for (k = 1; k < len; k++) {
            if ((s[i + k] & 0xC0) != 0x80) return 0;
            cp = (cp << 6) | (s[i + k] & 0x3Fu);
        }
    }
    if (cp >= 0xC0 && cp <= 0xFF) {
        const char *m = txt_latin1 + (cp - 0xC0) * 2;
        if (m[0] == '?') return 0; /* multiplication / division sign */
        out[(*outlen)++] = m[0];
        if (m[1]) out[(*outlen)++] = m[1];
        return len;
    }
    if (cp >= 0x100 && cp <= 0x17F) {
        out[(*outlen)++] = txt_latin_ext_a[cp - 0x100];
        return len;
    }
    /* general punctuation, symbols, CJK punctuation, fullwidth forms, emoji */
    if (cp < 0x370 || (cp >= 0x2000 && cp <= 0x2BFF) || (cp >= 0x3000 && cp <= 0x303F) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF00 && cp <= 0xFF20) || cp >= 0x1F000)
        return 0;
    return len; /* a letter we cannot pronounce */
}

static void txt_say_words(txt_state *st, const char *s, int word)
{
    /* space-separated words, all mapped to the same word event */
    while (*s) {
        const char *e = s;
        while (*e && *e != ' ') e++;
        if (e > s) txt_word(st, s, (int)(e - s), word);
        s = *e ? e + 1 : e;
    }
}

static void txt_phoneme_mode(txt_state *st, const char *text, int n)
{
    int i = 0;
    while (i < n) {
        int s0, from, w;
        char c = text[i];
        if (c == ',' || c == ';' || c == ':') { txt_pause(st, SS_PAUSE_SHORT); i++; continue; }
        if (c == '.') { txt_pause(st, SS_PAUSE_PERIOD); i++; continue; }
        if (c == '?') { txt_pause(st, SS_PAUSE_QUESTION); i++; continue; }
        if (c == '!') { txt_pause(st, SS_PAUSE_EXCLAIM); i++; continue; }
        if (c == '|' || c == ' ' || c == '\t' || c == '\n' || c == '\r') { i++; continue; }
        s0 = i;
        while (i < n && text[i] != '|' && text[i] != ',' && text[i] != '.' && text[i] != '?' &&
               text[i] != '!' && text[i] != ';' && text[i] != ':')
            i++;
        w = ss_tokens_add_word(st->t, s0, i - s0);
        from = st->t->ntok;
        txt_begin_word(st, NULL, 0);
        ss_parse_arpabet(text + s0, i - s0, st->t, w, SS_TF_WORD_START | st->sentence_flags);
        txt_assign_stress(st->t, from, st->t->ntok, NULL, 0);
        txt_finish_word(st->t, from);
    }
}

void ss_text_process(const char *text, int flags, ss_tokens *out)
{
    const unsigned char *s = (const unsigned char *)text;
    txt_state st;
    int n, i = 0;
    int currency = 0; /* pending "$" */
    char tl[4];
    int tll = 0;
    st.t = out;
    st.sentence_flags = 0;
    st.words_in_sentence = 0;
    st.the_tok = -1;
    if (!text) return;
    n = (int)strlen(text);

    if (flags & SS_INPUT_PHONEMES) {
        txt_phoneme_mode(&st, text, n);
        return;
    }

    while (i < n && !out->failed) {
        unsigned char c = s[i];

        /* embedded phonemes: {HH AH0 L OW1} */
        if (c == '{') {
            int e = i + 1, w, from;
            while (e < n && s[e] != '}') e++;
            w = ss_tokens_add_word(out, i, (e < n ? e + 1 : e) - i);
            from = out->ntok;
            txt_begin_word(&st, NULL, 0);
            ss_parse_arpabet(text + i + 1, e - i - 1, out, w, SS_TF_WORD_START | st.sentence_flags);
            txt_assign_stress(out, from, out->ntok, NULL, 0);
            txt_finish_word(out, from);
            st.the_tok = -1;
            i = e < n ? e + 1 : e;
            continue;
        }

        /* words */
        if (txt_isalpha(c) || (c >= 0xC0 && txt_utf8_letter(s, n, i, tl, &tll) > 0)) {
            char wbuf[128];
            int wl = 0, s0 = i, w, ul;
            const char *ab;
            while (i < n) {
                if (txt_isalpha(s[i])) {
                    if (wl < 120) wbuf[wl++] = (char)s[i];
                    i++;
                } else if (s[i] >= 0xC0 && (ul = txt_utf8_letter(s, n, i, tl, &tll)) > 0) {
                    int k;
                    for (k = 0; k < tll; k++) if (wl < 120) wbuf[wl++] = tl[k];
                    i += ul;
                } else if (s[i] == '\'' && i + 1 < n && txt_isalpha(s[i + 1])) {
                    if (wl < 120) wbuf[wl++] = '\'';
                    i++;
                } else if (txt_is_curly_apos(s, n, i) && i + 3 < n && txt_isalpha(s[i + 3])) {
                    if (wl < 120) wbuf[wl++] = '\'';
                    i += 3;
                } else {
                    break;
                }
            }
            w = ss_tokens_add_word(out, s0, i - s0);
            ab = (i < n && s[i] == '.') ? txt_find_abbrev(wbuf, wl) : NULL;
            if (wl == 0) {
                /* letters from a script we cannot pronounce */
            } else if (ab) {
                txt_say_words(&st, ab, w);
                i++; /* swallow the abbreviation's period */
            } else {
                txt_word(&st, wbuf, wl, w);
            }
            continue;
        }

        /* numbers */
        if (txt_isdigit(c) || (c == '.' && i + 1 < n && txt_isdigit(s[i + 1]))) {
            int s0 = i, w, ip_len = 0;
            char digits[32];
            int nd = 0, negative = 0, ordinal = 0;
            unsigned long long val = 0;
            /* negative sign directly before, at start or after space */
            if (s0 > 0 && s[s0 - 1] == '-' && (s0 == 1 || s[s0 - 2] == ' ' || s[s0 - 2] == '(')) negative = 1;
            while (i < n) {
                if (txt_isdigit(s[i])) {
                    if (nd < 30) digits[nd++] = (char)s[i];
                    i++;
                } else if (s[i] == ',' && nd > 0 && i + 3 < n && txt_isdigit(s[i + 1]) &&
                           txt_isdigit(s[i + 2]) && txt_isdigit(s[i + 3]) &&
                           (i + 4 >= n || !txt_isdigit(s[i + 4]))) {
                    i++; /* thousands separator */
                } else {
                    break;
                }
            }
            ip_len = nd;
            w = ss_tokens_add_word(out, negative ? s0 - 1 : s0, 0);
            if (negative) txt_say(&st, "minus", w);
            /* ordinal suffix */
            if (i + 1 < n && ((s[i] == 's' && s[i + 1] == 't') || (s[i] == 'n' && s[i + 1] == 'd') ||
                              (s[i] == 'r' && s[i + 1] == 'd') || (s[i] == 't' && s[i + 1] == 'h')) &&
                (i + 2 >= n || !txt_isalpha(s[i + 2]))) {
                ordinal = 1;
            }
            if (ip_len == 0) {
                /* ".5" */
            } else if ((ip_len > 1 && digits[0] == '0') || ip_len > 15) {
                txt_say_digits(&st, digits, ip_len, w);
            } else {
                int k;
                for (k = 0; k < ip_len; k++) val = val * 10 + (unsigned long long)(digits[k] - '0');
                /* clock time 10:30 */
                if (i + 2 < n && s[i] == ':' && txt_isdigit(s[i + 1]) && txt_isdigit(s[i + 2]) &&
                    (i + 3 >= n || !txt_isdigit(s[i + 3])) && val <= 24) {
                    int mm = (s[i + 1] - '0') * 10 + (s[i + 2] - '0');
                    txt_say_int(&st, val, w, 0);
                    if (mm == 0) txt_say(&st, "o'clock", w);
                    else {
                        if (mm < 10) txt_say(&st, "oh", w);
                        txt_say_int(&st, (unsigned long long)mm, w, 0);
                    }
                    i += 3;
                } else {
                    txt_say_int(&st, val, w, ordinal);
                }
            }
            if (ordinal) i += 2;
            /* decimals */
            if (i + 1 < n && s[i] == '.' && txt_isdigit(s[i + 1])) {
                int d0 = ++i;
                while (i < n && txt_isdigit(s[i])) i++;
                if (ip_len == 0) txt_say(&st, "zero", w);
                txt_say(&st, "point", w);
                txt_say_digits(&st, text + d0, i - d0, w);
            }
            if (i < n && s[i] == '%') {
                txt_say(&st, "percent", w);
                i++;
            }
            if (currency) {
                txt_say(&st, (val == 1 && ip_len > 0) ? "dollar" : "dollars", w);
                currency = 0;
            }
            if (w >= 0) out->word[w].text_length = i - out->word[w].text_offset;
            continue;
        }

        /* punctuation */
        if (c == ',' || c == ';' || c == ':' || c == '(' || c == ')') { txt_pause(&st, SS_PAUSE_SHORT); i++; continue; }
        if (c == '.') {
            if (i + 2 < n && s[i + 1] == '.' && s[i + 2] == '.') { txt_pause(&st, SS_PAUSE_LONG); i += 3; continue; }
            txt_pause(&st, SS_PAUSE_PERIOD);
            i++;
            continue;
        }
        if (c == '?') { txt_pause(&st, SS_PAUSE_QUESTION); i++; continue; }
        if (c == '!') { txt_pause(&st, SS_PAUSE_EXCLAIM); i++; continue; }
        if (c == '-') {
            /* spaced dash or double dash acts as a comma; joined hyphens don't */
            if ((i + 1 < n && s[i + 1] == '-') || (i > 0 && s[i - 1] == ' ' && i + 1 < n && s[i + 1] == ' '))
                txt_pause(&st, SS_PAUSE_SHORT);
            i++;
            continue;
        }
        if (c == '$') { currency = 1; i++; continue; }
        if (c == '&' || c == '+' || c == '=' || c == '@' || c == '%') {
            const char *word = c == '&' ? "and" : c == '+' ? "plus" : c == '=' ? "equals" :
                               c == '@' ? "at" : "percent";
            int w = ss_tokens_add_word(out, i, 1);
            txt_say(&st, word, w);
            i++;
            continue;
        }
        /* UTF-8 punctuation: em/en dash, ellipsis */
        if (c == 0xE2 && i + 2 < n && s[i + 1] == 0x80) {
            unsigned char d = s[i + 2];
            if (d == 0x93 || d == 0x94) txt_pause(&st, SS_PAUSE_SHORT);
            else if (d == 0xA6) txt_pause(&st, SS_PAUSE_LONG);
            i += 3;
            continue;
        }
        i++; /* whitespace and anything else */
    }
}

/* ------------------------------------------------------------------------ */
/* Formatting                                                               */
/* ------------------------------------------------------------------------ */

static int txt_put(char *out, int cap, int len, const char *str)
{
    while (*str) {
        if (len < cap - 1) out[len] = *str;
        len++;
        str++;
    }
    return len;
}

int ss_format_phonemes(const ss_tokens *t, char *out, int cap)
{
    int i, len = 0, last_word = -2;
    for (i = 0; i < t->ntok; i++) {
        const ss_phtok *k = &t->tok[i];
        if (k->ph == SS_PH_PAU) {
            static const char *const marks[] = {"", ",", ".", "?", "!", "..."};
            int kind = k->stress >= 0 && k->stress <= SS_PAUSE_LONG ? k->stress : 0;
            if (len > 0) len = txt_put(out, cap, len, " ");
            len = txt_put(out, cap, len, marks[kind]);
            last_word = -1;
            continue;
        }
        if (len > 0) {
            if (k->word != last_word && last_word >= 0) len = txt_put(out, cap, len, " | ");
            else len = txt_put(out, cap, len, " ");
        }
        last_word = k->word;
        if (k->ph == SS_PH_AX) {
            len = txt_put(out, cap, len, "AH0");
        } else {
            len = txt_put(out, cap, len, ss_phtab[k->ph].name);
            if (ss_ph_is_vowel(k->ph)) {
                char d[2];
                d[0] = (char)('0' + (k->stress < 0 ? 0 : k->stress));
                d[1] = 0;
                len = txt_put(out, cap, len, d);
            }
        }
    }
    if (cap > 0) out[len < cap ? len : cap - 1] = 0;
    return len;
}

/* ---- ss_prosody.c ---- */
/*
 * Prosody and timeline construction.
 *
 *  tokens -> durations (Klatt-style rules) -> acoustic segments
 *         -> formant transitions (Holmes-style locus/dominance rules)
 *         -> pitch contour (declination, accents, phrase-final tunes)
 *         -> event timeline (words, phonemes, visemes)
 */
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

/* ---- ss_klatt.c ---- */
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

/* ---- ss_engine.c ---- */
/*
 * Public API: voices, utterances, streaming engines and one-shot helpers.
 */
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
