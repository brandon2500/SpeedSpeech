/*
 * Phoneme inventory: acoustic targets, durations and coarticulation data.
 *
 * Formant targets are adult-male values in the spirit of Klatt (1980),
 * "Software for a cascade/parallel formant synthesizer", and Peterson &
 * Barney (1952). Other voices are produced by scaling at render time.
 */
#include "ss_internal.h"
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
