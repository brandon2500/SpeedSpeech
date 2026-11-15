/*
 * speedspeech - command line front end.
 *
 *   speedspeech [options] "text to say"
 *
 * Writes a 16-bit mono WAV file. Run with --help for options.
 */
#include "speedspeech.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const preset_names[SS_PRESET_COUNT] = {
    "default", "male", "female", "child", "robot", "giant", "whisper", "goblin", "elder"
};

static const char *event_name(int type)
{
    switch (type) {
    case SS_EVENT_UTTERANCE_START: return "start";
    case SS_EVENT_WORD: return "word";
    case SS_EVENT_PHONEME: return "phoneme";
    case SS_EVENT_VISEME: return "viseme";
    case SS_EVENT_UTTERANCE_END: return "end";
    default: return "?";
    }
}

static void usage(void)
{
    int i;
    printf("usage: speedspeech [options] \"text\"\n"
           "  -o FILE        output WAV file (default: speech.wav)\n"
           "  -v PRESET      voice preset:");
    for (i = 0; i < SS_PRESET_COUNT; i++) printf(" %s", preset_names[i]);
    printf("\n"
           "  -r RATE        sample rate in Hz (default 22050)\n"
           "  -p HZ          base pitch\n"
           "  -s SPEED       speaking rate multiplier\n"
           "  -f SHIFT       formant shift (vocal tract size)\n"
           "  --range X      intonation amount\n"
           "  --breath X     breathiness 0..1\n"
           "  --rough X      roughness 0..1\n"
           "  --whisper X    whisper 0..1\n"
           "  --buzz X       buzziness 0..1\n"
           "  --arpabet      input is ARPAbet phonemes\n"
           "  --phonemes     print the phoneme transcription and exit\n"
           "  --events       print the event timeline\n"
           "  -i FILE        read text from FILE ('-' = stdin)\n");
}

static void put16(FILE *f, unsigned v) { fputc((int)(v & 255), f); fputc((int)((v >> 8) & 255), f); }
static void put32(FILE *f, unsigned v) { put16(f, v & 0xFFFF); put16(f, v >> 16); }

static int write_wav(const char *path, const float *s, int n, int rate)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) return -1;
    fwrite("RIFF", 1, 4, f);
    put32(f, (unsigned)(36 + n * 2));
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 1);
    put16(f, 1);
    put32(f, (unsigned)rate);
    put32(f, (unsigned)rate * 2);
    put16(f, 2);
    put16(f, 16);
    fwrite("data", 1, 4, f);
    put32(f, (unsigned)(n * 2));
    for (i = 0; i < n; i++) {
        float x = s[i];
        int v;
        if (x > 1.0f) x = 1.0f;
        if (x < -1.0f) x = -1.0f;
        v = (int)(x * 32767.0f);
        put16(f, (unsigned)(v & 0xFFFF));
    }
    fclose(f);
    return 0;
}

static char *read_file(const char *path)
{
    FILE *f = strcmp(path, "-") == 0 ? stdin : fopen(path, "rb");
    char *buf = NULL;
    size_t len = 0, cap = 0;
    int c;
    if (!f) return NULL;
    while ((c = fgetc(f)) != EOF) {
        if (len + 1 >= cap) {
            char *nb;
            cap = cap ? cap * 2 : 4096;
            nb = (char *)realloc(buf, cap);
            if (!nb) { free(buf); if (f != stdin) fclose(f); return NULL; }
            buf = nb;
        }
        buf[len++] = (char)c;
    }
    if (f != stdin) fclose(f);
    if (!buf) buf = (char *)calloc(1, 1);
    else buf[len] = 0;
    return buf;
}

int main(int argc, char **argv)
{
    const char *out = "speech.wav";
    const char *text = NULL;
    char *owned = NULL;
    int rate = 22050, flags = SS_INPUT_TEXT, show_phonemes = 0, show_events = 0, i;
    ss_voice v;
    ss_utterance *u;
    ss_engine *e;
    float *buf;
    int len;

    ss_voice_preset(&v, SS_PRESET_DEFAULT);
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *next = i + 1 < argc ? argv[i + 1] : NULL;
#define NEED_ARG() do { if (!next) { fprintf(stderr, "missing value for %s\n", a); return 2; } i++; } while (0)
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        else if (!strcmp(a, "-o")) { NEED_ARG(); out = next; }
        else if (!strcmp(a, "-v")) {
            int p, found = -1;
            NEED_ARG();
            for (p = 0; p < SS_PRESET_COUNT; p++) if (!strcmp(next, preset_names[p])) found = p;
            if (found < 0) { fprintf(stderr, "unknown preset '%s'\n", next); return 2; }
            ss_voice_preset(&v, found);
        }
        else if (!strcmp(a, "-r")) { NEED_ARG(); rate = atoi(next); }
        else if (!strcmp(a, "-p")) { NEED_ARG(); v.pitch_hz = (float)atof(next); }
        else if (!strcmp(a, "-s")) { NEED_ARG(); v.rate = (float)atof(next); }
        else if (!strcmp(a, "-f")) { NEED_ARG(); v.formant_shift = (float)atof(next); }
        else if (!strcmp(a, "--range")) { NEED_ARG(); v.pitch_range = (float)atof(next); }
        else if (!strcmp(a, "--breath")) { NEED_ARG(); v.breathiness = (float)atof(next); }
        else if (!strcmp(a, "--rough")) { NEED_ARG(); v.roughness = (float)atof(next); }
        else if (!strcmp(a, "--whisper")) { NEED_ARG(); v.whisper = (float)atof(next); }
        else if (!strcmp(a, "--buzz")) { NEED_ARG(); v.buzz = (float)atof(next); }
        else if (!strcmp(a, "--arpabet")) flags = SS_INPUT_PHONEMES;
        else if (!strcmp(a, "--phonemes")) show_phonemes = 1;
        else if (!strcmp(a, "--events")) show_events = 1;
        else if (!strcmp(a, "-i")) {
            NEED_ARG();
            owned = read_file(next);
            if (!owned) { fprintf(stderr, "cannot read %s\n", next); return 1; }
            text = owned;
        }
        else if (a[0] == '-' && a[1]) { fprintf(stderr, "unknown option %s\n", a); usage(); return 2; }
        else text = a;
#undef NEED_ARG
    }
    if (!text) { usage(); return 2; }

    u = ss_utterance_create(text, &v, rate, flags);
    if (!u) { fprintf(stderr, "synthesis failed (bad sample rate?)\n"); free(owned); return 1; }
    if (show_phonemes) {
        int n = ss_utterance_phonemes(u, NULL, 0);
        char *p = (char *)malloc((size_t)n + 1);
        if (p) { ss_utterance_phonemes(u, p, n + 1); printf("%s\n", p); free(p); }
        ss_utterance_destroy(u);
        free(owned);
        return 0;
    }
    if (show_events) {
        int n = ss_utterance_event_count(u), k;
        for (k = 0; k < n; k++) {
            ss_event ev;
            ss_utterance_get_event(u, k, &ev);
            printf("%8.3f %-8s", ev.time, event_name(ev.type));
            if (ev.type == SS_EVENT_WORD)
                printf(" \"%.*s\"", ev.text_length, text + ev.text_offset);
            else if (ev.type == SS_EVENT_PHONEME)
                printf(" %s", ss_phoneme_name(ev.phoneme));
            else if (ev.type == SS_EVENT_VISEME)
                printf(" %s", ss_viseme_name(ev.viseme));
            printf("  (%.3fs)\n", ev.duration);
        }
    }

    len = ss_utterance_length(u);
    buf = (float *)malloc(sizeof(float) * (size_t)(len > 0 ? len : 1));
    e = ss_engine_create(rate);
    if (!buf || !e) { fprintf(stderr, "out of memory\n"); return 1; }
    ss_engine_queue(e, u);
    ss_engine_render(e, buf, len);
    ss_engine_destroy(e);
    if (write_wav(out, buf, len, rate) != 0) {
        fprintf(stderr, "cannot write %s\n", out);
        free(buf);
        free(owned);
        return 1;
    }
    fprintf(stderr, "wrote %s (%.2f s)\n", out, (double)len / rate);
    free(buf);
    free(owned);
    return 0;
}
