/*
 * Text front end: normalisation, tokenisation and pronunciation.
 *
 * Input is UTF-8. Letters outside ASCII are skipped, except typographic
 * apostrophes, dashes and ellipses, which are understood.
 */
#include "ss_internal.h"
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
        if (txt_isalpha(c)) {
            char wbuf[128];
            int wl = 0, s0 = i, w;
            const char *ab;
            while (i < n) {
                if (txt_isalpha(s[i])) {
                    if (wl < 120) wbuf[wl++] = (char)s[i];
                    i++;
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
            if (ab) {
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
