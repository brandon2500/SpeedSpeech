# SpeedSpeech

A tiny, dependency-free English speech synthesizer written from scratch for games. It
runs in **Unity** and **Unreal Engine**, or anywhere you can compile C.

```
"Welcome to the dungeon, brave warrior!"
  -> W EH1 L K AH0 M | T UW0 | DH AH0 | D AH1 N JH AH0 N , B R EY1 V | W AO1 R IY0 ER0 !
  -> 3.1 s of audio, rendered in under 40 ms
```

- **Small**: ~4,400 lines of C99, including the dictionary and rule tables, and about 160 KB compiled. There are no voice data files, no
  models and no dependencies beyond libc/libm.
- **Fast**: about 85× realtime at 48 kHz on one desktop core, so each talking character costs
  roughly 1% of a CPU. Compiling a sentence takes about 0.05 ms.
- **Built for game audio threads**: rendering is pull-based and doesn't allocate. Text is
  compiled on your game thread, so the audio thread only renders. Output is deterministic
  (the same seed gives the same samples) and works at any sample rate from 8 kHz to 384 kHz.
- **Lip sync and subtitles**: it reports word, phoneme and viseme events (the standard
  15-viseme set) with timings. You get them live while speaking, or ahead of time for
  baking.
- **Character voices**: 9 presets (male, female, child, robot, giant, whisper, goblin,
  elder) plus 11 continuous parameters: pitch, intonation, rate, vocal-tract size,
  breathiness, roughness, whisper, buzz, vibrato and more.
- **Game text just works**: it reads numbers (`1,234`, `3.5`, `-7`, `21st`), money (`$5`),
  percentages, clock times (`10:30`), acronyms (`NPC`, `XP`), abbreviations (`Dr.`),
  typographic punctuation and accented names. You can also inline ARPAbet with
  `{AA1 R K EY0 N}` for made-up names.

SpeedSpeech is a classic *formant* synthesizer in the tradition of Klatt's work. It
sounds clearly synthetic, a bit like 80s/90s text-to-speech, which suits robots, retro
games, prototyping and placeholder dialogue. It is not a neural voice.

## How intelligible is it?

To check intelligibility, I ran SpeedSpeech output through an off-the-shelf speech
recognizer (the Vosk small English model, which was trained on human speech). On a
60-sentence game-dialogue corpus:

| | Word error rate (lower is better) |
|---|---|
| SpeedSpeech (default voice) | **~30%** |
| eSpeak-NG 1.51 (default voice), same corpus | 30.4% |

On a closed-set rhyme test, where the recognizer picks between minimal pairs like
*bat/pat/cat/hat*, it identifies **83%** of words correctly. Human listeners usually do
better than a small recognizer does on synthetic speech.

## Repository layout

```
core/include/speedspeech.h        public C API (the only header you need)
core/src/                         the synthesizer (C99, also valid C++)
tools/speedspeech_cli.c           command-line tool: text -> WAV
bindings/unity/com.speedspeech.unity/   Unity package (UPM)
bindings/unreal/SpeedSpeech/            Unreal Engine plugin
tests/                            unit tests (+ C++ single-translation-unit check)
```

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --parallel
```

```bash
ctest --test-dir build --output-on-failure
```

This produces `libspeedspeech_static`, the shared library `speedspeech` (the Unity
native plugin), the `speedspeech-cli` tool and the tests.

### Command line

```bash
./build/speedspeech-cli -v goblin -o goblin.wav "Shiny! Give me the shiny, or I bite your toes."
```

```bash
./build/speedspeech-cli --phonemes "The NPC gives you 50% more XP."
```

```bash
./build/speedspeech-cli --events -o /dev/null "Hello there."
```

Run `speedspeech-cli --help` for all the voice options.

## Using the C API

```c
#include "speedspeech.h"

ss_engine *engine = ss_engine_create(48000);         /* one per talking character */

ss_voice voice;
ss_voice_preset(&voice, SS_PRESET_FEMALE);
voice.rate = 1.1f;

/* game thread: compile (thread-safe, ~0.05 ms) */
ss_utterance *line = ss_utterance_create("Shields at 15 percent!", &voice, 48000, SS_INPUT_TEXT);

/* audio thread: hand it over (O(1)) and render */
ss_engine_queue(engine, line);                       /* engine now owns it */

void audio_callback(float *out, int frames) {
    ss_engine_render(engine, out, frames);           /* mono; silence when idle */
    ss_event ev;
    while (ss_engine_poll_event(engine, &ev)) {
        /* ev.type: SS_EVENT_WORD / PHONEME / VISEME / UTTERANCE_START / UTTERANCE_END
           ev.block_offset: frame inside this block where it happened */
    }
}
```

One-shot helpers are also available: `ss_synthesize()` (text to a sample buffer),
`ss_text_to_phonemes()`, and `ss_utterance_get_event()`, which returns the full timeline
ahead of time.

**Threading:** `ss_utterance_*` functions are pure and safe on any thread. An `ss_engine`
is single-threaded: call it from the audio thread, or put a lock around it.
`ss_engine_queue` and `ss_engine_stop` are O(1).

**Memory:** define `SS_MALLOC`, `SS_REALLOC` and `SS_FREE` when compiling to use your own
allocator. The Unreal plugin routes them to `FMemory`.

**Single file:** `python3 tools/amalgamate.py` writes `speedspeech.c` + `speedspeech.h` (one
translation unit, valid as C or C++) into the engine bindings. You can also drop that file
into any other engine.

## Unity

See [`bindings/unity/com.speedspeech.unity/README.md`](bindings/unity/com.speedspeech.unity/README.md).

```csharp
var speaker = GetComponent<SpeedSpeechSource>();            // needs an AudioSource
speaker.onWord.AddListener(word => subtitle.Highlight(word));
speaker.Speak("Do you have the key?");                      // streamed and 3D-spatialized
AudioClip clip = SpeedSpeechSynth.CreateClip("Baked line.", SpeechVoice.FromPreset(VoicePreset.Robot));
```

## Unreal Engine

See [`bindings/unreal/SpeedSpeech/README.md`](bindings/unreal/SpeedSpeech/README.md). Add a
**SpeedSpeech** component to any actor, then call **Speak** from Blueprint or C++. It
plays through the normal audio pipeline (attenuation, submixes, concurrency) and
broadcasts `OnWord`, `OnViseme`, `OnSpeechStarted` and `OnSpeechFinished`.

## How it works

1. **Text normalization**: numbers, currency, times, ordinals, abbreviations, acronyms,
   symbols and UTF-8 punctuation are expanded to words. Accented Latin letters are
   transliterated.
2. **Pronunciation**: a ~620-word exception dictionary covers function words, irregular
   spellings, numbers and some game vocabulary. Plurals, possessives, `-ing` and `-ed` forms
   of dictionary words are derived from the base word. Everything else goes through
   context-sensitive letter-to-sound rules based on the public-domain NRL rule set
   (Elovitz et al., 1976). Heuristics then assign lexical stress.
3. **Prosody**: Klatt-style duration rules handle stress, phrase-final lengthening,
   shortening before voiceless consonants and consonant clusters. Intonation uses a
   declination line with pitch accents on stressed content words, a falling end for
   statements and wh-questions, and a rising end for yes/no questions.
4. **Acoustics**: each phoneme becomes timed segments, with stops split into closure,
   burst and aspiration. Formant transitions follow Holmes-style dominance/locus rules,
   including the velar pinch. American English flapping is applied ("water", "better").
5. **Synthesis**: a cascade/parallel formant synthesizer in the style of Klatt (1980) does
   the rendering. It uses a polynomial glottal pulse with jitter, shimmer and flutter, a
   nasal pole/zero pair, six cascade formants, and a 4-pole + 2-pole frication branch with
   radiation. Levels are normalized so loudness doesn't depend on sample rate or pitch.

## Limitations

- English only, with an American accent. The letter-to-sound rules guess wrong on some
  words. Add them to `core/src/ss_lexicon.c` (then run `tools/sort_lexicon.py`) or use
  inline `{ARPAbet}`.
- There is no SSML. Prosody is controlled through punctuation and voice parameters.
- The Unity and Unreal bindings were compiled and run against the real native core with
  stand-in engine APIs (Mono for Unity; stub headers for Unreal). They have not yet been
  exercised inside the actual editors. Bug reports are welcome.
- Unity WebGL has no `OnAudioFilterRead`, so use `SpeedSpeechSynth.CreateClip` there.

## License

MIT. See [LICENSE](LICENSE).
