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
#include "ss_internal.h"
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
