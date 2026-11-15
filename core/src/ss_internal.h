/*
 * SpeedSpeech internal definitions. Not part of the public API.
 *
 * Every source file in core/src must compile as C99 *and* as C++ (the Unreal
 * plugin builds them as C++), and all files must be able to share a single
 * translation unit (amalgamation): file-local statics carry a per-file prefix.
 */
#ifndef SS_INTERNAL_H
#define SS_INTERNAL_H

#include "speedspeech.h"
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
