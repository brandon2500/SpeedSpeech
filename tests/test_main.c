/*
 * SpeedSpeech unit tests. No framework: each check prints on failure and the
 * process exits non-zero if anything failed.
 */
#include "speedspeech.h"
#include "ss_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond) do { \
    g_checks++; \
    if (!(cond)) { g_failures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_MSG(cond, ...) do { \
    g_checks++; \
    if (!(cond)) { g_failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

/* ---- helpers ---- */

static char g_buf[4096];

static const char *phon(const char *text)
{
    ss_text_to_phonemes(text, g_buf, (int)sizeof(g_buf));
    return g_buf;
}

static void check_phonemes_contain(const char *text, const char *expected)
{
    const char *got = phon(text);
    CHECK_MSG(strstr(got, expected) != NULL, "\"%s\" -> \"%s\", expected to contain \"%s\"", text, got, expected);
}

static void check_phonemes_equal(const char *text, const char *expected)
{
    const char *got = phon(text);
    CHECK_MSG(strcmp(got, expected) == 0, "\"%s\" -> \"%s\", expected \"%s\"", text, got, expected);
}

typedef struct stats {
    double rms, peak;
    int bad; /* NaN / inf count */
} stats;

static stats measure(const float *x, int n)
{
    stats s;
    double acc = 0.0;
    int i;
    memset(&s, 0, sizeof(s));
    for (i = 0; i < n; i++) {
        double v = x[i];
        if (!(v == v) || v > 1e30 || v < -1e30) { s.bad++; continue; }
        acc += v * v;
        if (fabs(v) > s.peak) s.peak = fabs(v);
    }
    s.rms = n > 0 ? sqrt(acc / n) : 0.0;
    return s;
}

/* ---- tests ---- */

static void test_lexicon(void)
{
    int i, n = ss_lexicon_size();
    CHECK(n > 300);
    for (i = 1; i < n; i++) {
        CHECK_MSG(strcmp(ss_lexicon_word(i - 1), ss_lexicon_word(i)) < 0,
                  "lexicon not sorted at \"%s\" / \"%s\" (run tools/sort_lexicon.py)",
                  ss_lexicon_word(i - 1), ss_lexicon_word(i));
    }
    for (i = 0; i < n; i++) {
        const char *w = ss_lexicon_word(i);
        CHECK_MSG(ss_lexicon_lookup(w, (int)strlen(w)) != NULL, "lookup failed for \"%s\"", w);
    }
    CHECK(ss_lexicon_lookup("zzzzqx", 6) == NULL);
    CHECK(ss_lexicon_lookup("th", 2) == NULL); /* prefix of "than"/"the" */
}

static void test_lexicon_phonemes_valid(void)
{
    /* every phoneme in every entry must parse */
    int i, n = ss_lexicon_size();
    for (i = 0; i < n; i++) {
        const char *w = ss_lexicon_word(i);
        const char *p = ss_lexicon_lookup(w, (int)strlen(w));
        int tokens = 1, parsed;
        const char *c;
        ss_tokens t;
        for (c = p; *c; c++) if (*c == ' ') tokens++;
        ss_tokens_init(&t);
        parsed = ss_parse_arpabet(p, (int)strlen(p), &t, 0, 0);
        CHECK_MSG(parsed == tokens, "entry \"%s\" has an unknown phoneme in \"%s\"", w, p);
        ss_tokens_free(&t);
    }
}

static void test_text_frontend(void)
{
    check_phonemes_equal("Hello world.", "HH AH0 L OW1 | W ER1 L D .");
    check_phonemes_equal("", "");
    check_phonemes_contain("make", "M EY1 K");
    check_phonemes_contain("nice", "N AY1 S");
    check_phonemes_contain("the apple", "DH IY0");          /* "the" before a vowel */
    check_phonemes_contain("the dog", "DH AH0");
    check_phonemes_contain("42", "F AO1 R T IY0 T UW1");
    check_phonemes_contain("1,234", "W AH1 N TH AW1 Z AH0 N D T UW1 HH AH1 N D R AH0 D");
    check_phonemes_contain("3rd", "TH ER1 D");
    check_phonemes_contain("21st", "T W EH1 N T IY0 F ER1 S T");
    check_phonemes_contain("$5", "F AY1 V D AA1 L ER0 Z");
    check_phonemes_contain("$1", "D AA1 L ER0");
    check_phonemes_contain("50%", "P ER0 S EH1 N T");
    check_phonemes_contain("-7", "M AY1 N AH0 S");
    check_phonemes_contain("3.14", "TH R IY1 P OY1 N T W AH1 N F AO1 R");
    check_phonemes_contain("10:30", "T EH1 N TH ER1 T IY0");
    check_phonemes_contain("10:05", "OW1 F AY1 V");
    check_phonemes_contain("007", "Z IH1 R OW0 Z IH1 R OW0 S EH1 V AH0 N");
    check_phonemes_contain("NPC", "EH2 N P IY2 S IY1");
    check_phonemes_contain("dragons", "D R AE1 G AH0 N Z");
    check_phonemes_contain("dragon's", "D R AE1 G AH0 N Z");
    check_phonemes_contain("swords", "S AO1 R D Z");
    check_phonemes_contain("attacking", "AH0 T AE1 K IH0 NG");
    check_phonemes_contain("don\xE2\x80\x99t", "D OW1 N T"); /* typographic apostrophe */
    check_phonemes_contain("Dr. Smith", "D AA1 K T ER0");
    check_phonemes_contain("salt & pepper", "AH0 N D");
    check_phonemes_contain("Hello {W ER1 L D}", "W ER1 L D");
    check_phonemes_contain("Really?", "?");
    check_phonemes_contain("Wait... what", "...");
    check_phonemes_contain("one, two", ",");
}

static void test_utterance_events(void)
{
    const char *text = "Hello brave hero, the dragon awaits!";
    ss_utterance *u = ss_utterance_create(text, NULL, 22050, SS_INPUT_TEXT);
    int n, i, words = 0, phonemes = 0;
    float last = -1.0f;
    ss_event ev;
    CHECK(u != NULL);
    if (!u) return;
    CHECK(ss_utterance_length(u) > 22050); /* more than a second */
    n = ss_utterance_event_count(u);
    CHECK(n > 10);
    CHECK(ss_utterance_get_event(u, 0, &ev) && ev.type == SS_EVENT_UTTERANCE_START);
    CHECK(ss_utterance_get_event(u, n - 1, &ev) && ev.type == SS_EVENT_UTTERANCE_END);
    CHECK(!ss_utterance_get_event(u, n, &ev));
    CHECK(!ss_utterance_get_event(u, -1, &ev));
    for (i = 0; i < n; i++) {
        ss_utterance_get_event(u, i, &ev);
        CHECK_MSG(ev.time >= last, "events out of order at %d", i);
        last = ev.time;
        CHECK(ev.duration >= 0.0f);
        if (ev.type == SS_EVENT_WORD) {
            CHECK(ev.text_offset >= 0 && ev.text_offset + ev.text_length <= (int)strlen(text));
            if (words == 0) CHECK(strncmp(text + ev.text_offset, "Hello", 5) == 0 && ev.text_length == 5);
            if (words == 4) CHECK(strncmp(text + ev.text_offset, "dragon", 6) == 0);
            CHECK(ev.word_index == words);
            words++;
        }
        if (ev.type == SS_EVENT_PHONEME) {
            CHECK(ev.phoneme > 0 && ev.phoneme < ss_phoneme_count());
            CHECK(ev.viseme >= 0 && ev.viseme < SS_VISEME_COUNT);
            phonemes++;
        }
        if (ev.type == SS_EVENT_VISEME) CHECK(ev.viseme >= 0 && ev.viseme < SS_VISEME_COUNT);
    }
    CHECK(words == 6);
    CHECK(phonemes > 20);
    CHECK(last <= (float)ss_utterance_length(u) / 22050.0f + 1e-4f);

    {
        char small[8];
        int need = ss_utterance_phonemes(u, NULL, 0);
        CHECK(need > 10);
        CHECK(ss_utterance_phonemes(u, small, (int)sizeof(small)) == need);
        CHECK(strlen(small) == sizeof(small) - 1);
    }
    ss_utterance_destroy(u);
}

static float *render_all(const char *text, const ss_voice *v, int rate, int *len)
{
    float *buf = NULL;
    *len = ss_synthesize(text, v, rate, &buf);
    return buf;
}

static void test_render_basic(void)
{
    int n1, n2, i, same = 1;
    float *a = render_all("The quick brown fox jumps over the lazy dog.", NULL, 22050, &n1);
    float *b = render_all("The quick brown fox jumps over the lazy dog.", NULL, 22050, &n2);
    stats s;
    CHECK(a && b && n1 > 0 && n1 == n2);
    if (!a || !b) { ss_free(a); ss_free(b); return; }
    s = measure(a, n1);
    CHECK(s.bad == 0);
    CHECK(s.peak <= 1.0);
    CHECK_MSG(s.rms > 0.02 && s.rms < 0.5, "rms %f", s.rms);
    for (i = 0; i < n1; i++) if (a[i] != b[i]) { same = 0; break; }
    CHECK_MSG(same, "rendering is not deterministic");
    ss_free(a);
    ss_free(b);
}

static void test_streaming_matches_oneshot(void)
{
    const char *text = "Streaming must match one-shot rendering exactly.";
    int n, pos = 0, i, k = 0, same = 1;
    float *ref = render_all(text, NULL, 16000, &n);
    float *got;
    ss_engine *e = ss_engine_create(16000);
    static const int blocks[] = {1, 7, 64, 333, 512, 1024, 3};
    CHECK(ref && e);
    if (!ref || !e) { ss_free(ref); ss_engine_destroy(e); return; }
    got = (float *)malloc(sizeof(float) * (size_t)n);
    ss_engine_speak(e, text, NULL);
    while (pos < n) {
        int b = blocks[k++ % 7];
        if (b > n - pos) b = n - pos;
        ss_engine_render(e, got + pos, b);
        pos += b;
    }
    for (i = 0; i < n; i++) if (got[i] != ref[i]) { same = 0; break; }
    CHECK_MSG(same, "streamed output differs at sample %d", i);
    CHECK(!ss_engine_is_speaking(e));
    free(got);
    ss_free(ref);
    ss_engine_destroy(e);
}

static void test_sample_rates(void)
{
    static const int rates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000};
    const char *text = "Sample rate independence, please.";
    double ref_rms = 0.0;
    int i;
    for (i = 0; i < (int)(sizeof(rates) / sizeof(rates[0])); i++) {
        int n;
        float *x = render_all(text, NULL, rates[i], &n);
        stats s;
        CHECK_MSG(x != NULL && n > 0, "rate %d failed", rates[i]);
        if (!x) continue;
        s = measure(x, n);
        CHECK_MSG(s.bad == 0, "rate %d produced NaN/inf", rates[i]);
        CHECK_MSG(s.peak <= 1.0, "rate %d peak %f", rates[i], s.peak);
        if (rates[i] == 22050) ref_rms = s.rms;
        /* duration scales with the rate */
        CHECK_MSG(fabs((double)n / rates[i] - 2.0) < 1.5, "rate %d duration %f", rates[i], (double)n / rates[i]);
        if (ref_rms > 0.0 && rates[i] >= 16000) {
            CHECK_MSG(s.rms > ref_rms * 0.6 && s.rms < ref_rms * 1.6,
                      "rate %d rms %f vs %f", rates[i], s.rms, ref_rms);
        }
        ss_free(x);
    }
    CHECK(ss_engine_create(100) == NULL);
    CHECK(ss_utterance_create("hi", NULL, 1000000, 0) == NULL);
}

static void test_presets(void)
{
    int p;
    for (p = 0; p < SS_PRESET_COUNT; p++) {
        ss_voice v;
        int n;
        float *x;
        stats s;
        ss_voice_preset(&v, p);
        x = render_all("Every voice should be clean and safe.", &v, 22050, &n);
        CHECK_MSG(x != NULL, "preset %d failed", p);
        if (!x) continue;
        s = measure(x, n);
        CHECK_MSG(s.bad == 0 && s.peak <= 1.0 && s.rms > 0.005, "preset %d: bad=%d peak=%f rms=%f",
                  p, s.bad, s.peak, s.rms);
        ss_free(x);
    }
}

static void test_extreme_voices(void)
{
    ss_voice v;
    int n;
    float *x;
    stats s;
    ss_voice_preset(&v, SS_PRESET_DEFAULT);
    v.pitch_hz = 1e9f;
    v.rate = -3.0f;
    v.formant_shift = 0.0f;
    v.volume = (float)HUGE_VAL;
    v.vibrato_hz = 1000.0f;
    v.vibrato_depth = 100.0f;
    v.pitch_range = 0.0f / 1.0f;
    x = render_all("Out of range parameters are clamped.", &v, 22050, &n);
    CHECK(x != NULL);
    if (x) {
        s = measure(x, n);
        CHECK(s.bad == 0 && s.peak <= 1.0);
        ss_free(x);
    }
}

static void test_engine_queue_and_stop(void)
{
    ss_engine *e = ss_engine_create(22050);
    float buf[512];
    ss_event ev;
    int id1, id2, ends = 0, starts = 0, i, guard = 0;
    CHECK(e != NULL);
    if (!e) return;
    CHECK(!ss_engine_is_speaking(e));
    CHECK(ss_engine_render(e, buf, 512) == 0);
    for (i = 0; i < 512; i++) if (buf[i] != 0.0f) break;
    CHECK(i == 512);

    id1 = ss_engine_speak(e, "First line.", NULL);
    id2 = ss_engine_speak(e, "Second line.", NULL);
    CHECK(id1 > 0 && id2 > id1);
    CHECK(ss_engine_is_speaking(e));
    while (ss_engine_is_speaking(e) && guard++ < 10000) {
        ss_engine_render(e, buf, 512);
        while (ss_engine_poll_event(e, &ev)) {
            CHECK(ev.block_offset >= 0 && ev.block_offset <= 512);
            if (ev.type == SS_EVENT_UTTERANCE_START) { CHECK(ev.utterance_id == (starts == 0 ? id1 : id2)); starts++; }
            if (ev.type == SS_EVENT_UTTERANCE_END) ends++;
        }
    }
    CHECK(starts == 2 && ends == 2);

    /* stop: fades out quickly, reports the end, then silence */
    ss_engine_speak(e, "This sentence will be interrupted before it can finish.", NULL);
    ss_engine_speak(e, "And this one never plays.", NULL);
    ss_engine_render(e, buf, 512);
    while (ss_engine_poll_event(e, &ev)) {}
    ss_engine_stop(e);
    ends = 0;
    for (i = 0; i < 4; i++) {
        ss_engine_render(e, buf, 512);
        while (ss_engine_poll_event(e, &ev)) if (ev.type == SS_EVENT_UTTERANCE_END) ends++;
    }
    CHECK(!ss_engine_is_speaking(e));
    CHECK(ends == 1);
    ss_engine_render(e, buf, 512);
    for (i = 0; i < 512; i++) if (buf[i] != 0.0f) break;
    CHECK(i == 512);

    /* stop before anything rendered drops the queue immediately */
    ss_engine_speak(e, "Never heard.", NULL);
    ss_engine_stop(e);
    CHECK(!ss_engine_is_speaking(e));

    CHECK(ss_engine_queue(e, NULL) == 0);
    CHECK(ss_engine_queue(NULL, ss_utterance_create("x", NULL, 22050, 0)) == 0); /* freed, no leak */
    ss_engine_destroy(e);
    ss_engine_destroy(NULL);
}

static void test_interleaved(void)
{
    ss_engine *a = ss_engine_create(22050), *b = ss_engine_create(22050);
    float mono[1000], stereo[2000];
    int i, ok = 1;
    ss_engine_speak(a, "Stereo test.", NULL);
    ss_engine_speak(b, "Stereo test.", NULL);
    ss_engine_render(a, mono, 1000);
    ss_engine_render_interleaved(b, stereo, 1000, 2);
    for (i = 0; i < 1000; i++) if (stereo[2 * i] != mono[i] || stereo[2 * i + 1] != mono[i]) ok = 0;
    CHECK(ok);
    ss_engine_destroy(a);
    ss_engine_destroy(b);
}

static void test_phoneme_input(void)
{
    ss_utterance *u = ss_utterance_create("HH AH0 L OW1 | W ER1 L D .", NULL, 22050, SS_INPUT_PHONEMES);
    char out[256];
    CHECK(u != NULL);
    if (!u) return;
    ss_utterance_phonemes(u, out, (int)sizeof(out));
    CHECK_MSG(strcmp(out, "HH AH0 L OW1 | W ER1 L D .") == 0, "got \"%s\"", out);
    ss_utterance_destroy(u);
}

static void test_garbage_input(void)
{
    unsigned int seed = 12345;
    int it;
    char buf[200];
    /* empty and NULL text are valid and silent */
    {
        ss_utterance *u = ss_utterance_create(NULL, NULL, 22050, 0);
        CHECK(u != NULL && ss_utterance_length(u) > 0);
        ss_utterance_destroy(u);
        u = ss_utterance_create("   \n\t", NULL, 22050, 0);
        CHECK(u != NULL);
        ss_utterance_destroy(u);
    }
    for (it = 0; it < 300; it++) {
        int len = (int)(seed % 150), i, n;
        float *x;
        for (i = 0; i < len; i++) {
            seed = seed * 1103515245u + 12345u;
            buf[i] = (char)((seed >> 16) & 0xFF);
            if (buf[i] == 0) buf[i] = 'a';
        }
        buf[len] = 0;
        seed = seed * 1103515245u + 12345u;
        x = NULL;
        n = ss_synthesize(buf, NULL, 8000, &x);
        CHECK(n > 0 && x != NULL);
        if (x) {
            stats s = measure(x, n);
            CHECK(s.bad == 0 && s.peak <= 1.0);
        }
        ss_free(x);
        ss_text_to_phonemes(buf, g_buf, 16);
    }
    /* very long word and very long number */
    {
        char longw[600];
        float *x = NULL;
        memset(longw, 'a', sizeof(longw) - 1);
        longw[sizeof(longw) - 1] = 0;
        CHECK(ss_synthesize(longw, NULL, 8000, &x) > 0);
        ss_free(x);
        x = NULL;
        CHECK(ss_synthesize("123456789012345678901234567890", NULL, 8000, &x) > 0);
        ss_free(x);
    }
}

static void test_introspection(void)
{
    int i;
    CHECK(ss_phoneme_count() == SS_PH_COUNT);
    for (i = 0; i < ss_phoneme_count(); i++) {
        CHECK(ss_phoneme_name(i) != NULL);
        CHECK(ss_phoneme_from_name(ss_phoneme_name(i), (int)strlen(ss_phoneme_name(i))) == i);
        CHECK(ss_phoneme_viseme(i) >= 0 && ss_phoneme_viseme(i) < SS_VISEME_COUNT);
    }
    CHECK(ss_phoneme_name(-1) == NULL && ss_phoneme_name(SS_PH_COUNT) == NULL);
    for (i = 0; i < SS_VISEME_COUNT; i++) CHECK(ss_viseme_name(i) != NULL);
    CHECK(ss_viseme_name(SS_VISEME_COUNT) == NULL);
    CHECK(strcmp(ss_version_string(), "0.1.0") == 0);
}

int main(void)
{
    test_lexicon();
    test_lexicon_phonemes_valid();
    test_text_frontend();
    test_utterance_events();
    test_render_basic();
    test_streaming_matches_oneshot();
    test_sample_rates();
    test_presets();
    test_extreme_voices();
    test_engine_queue_and_stop();
    test_interleaved();
    test_phoneme_input();
    test_garbage_input();
    test_introspection();
    printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
