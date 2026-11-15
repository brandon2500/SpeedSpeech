// Compiles the whole core as C++ in a single translation unit, exactly the
// way the Unreal plugin does, and runs a smoke test.
#include "../core/src/ss_engine.c"
#include "../core/src/ss_klatt.c"
#include "../core/src/ss_lexicon.c"
#include "../core/src/ss_phonemes.c"
#include "../core/src/ss_prosody.c"
#include "../core/src/ss_rules.c"
#include "../core/src/ss_text.c"

#include <cstdio>

int main()
{
    float *samples = nullptr;
    int n = ss_synthesize("Compiled as C plus plus.", nullptr, 16000, &samples);
    if (n <= 0 || !samples) {
        std::printf("FAIL: synthesis returned %d\n", n);
        return 1;
    }
    double energy = 0.0;
    for (int i = 0; i < n; i++) energy += samples[i] * samples[i];
    ss_free(samples);
    if (energy <= 0.0) {
        std::printf("FAIL: silent output\n");
        return 1;
    }
    std::printf("ok: %d samples\n", n);
    return 0;
}
