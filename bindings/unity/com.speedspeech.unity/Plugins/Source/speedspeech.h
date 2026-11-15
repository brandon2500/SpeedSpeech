/* GENERATED FILE - do not edit. Produced by tools/amalgamate.py from core/.
 * SpeedSpeech (MIT License) - https://github.com/brandon2500/SpeedSpeech */
/*
 * SpeedSpeech - a tiny, dependency-free formant speech synthesizer for games.
 *
 * Public C API. This header is the only file engine bindings need.
 *
 * Threading model
 * ---------------
 *  - ss_utterance_create() and the other ss_utterance_* functions are pure:
 *    they touch no global state and may be called from any thread, e.g. the
 *    game thread, while an engine renders on the audio thread.
 *  - An ss_engine is NOT internally synchronized. Call all ss_engine_*
 *    functions for one engine from a single thread (usually the audio
 *    thread), or protect it with a lock. ss_engine_queue() is O(1) so a lock
 *    held around it is held only very briefly.
 *  - ss_engine_render() never allocates, except to free utterances it has
 *    finished playing.
 *
 * Memory
 * ------
 *  Define SS_MALLOC / SS_FREE when compiling the library to route all
 *  allocations through your own allocator.
 *
 * License: MIT (see LICENSE).
 */
#ifndef SPEEDSPEECH_H
#define SPEEDSPEECH_H

#ifdef __cplusplus
extern "C" {
#endif

#define SS_VERSION_MAJOR 0
#define SS_VERSION_MINOR 1
#define SS_VERSION_PATCH 0

/* Symbol visibility: build the shared library with SS_BUILD_SHARED defined. */
#if defined(SS_BUILD_SHARED)
#  if defined(_WIN32)
#    define SS_API __declspec(dllexport)
#  else
#    define SS_API __attribute__((visibility("default")))
#  endif
#elif defined(SS_USE_SHARED) && defined(_WIN32)
#  define SS_API __declspec(dllimport)
#else
#  define SS_API
#endif

/* ------------------------------------------------------------------------ */
/* Voices                                                                   */
/* ------------------------------------------------------------------------ */

/*
 * All the knobs that make a character sound like themselves.
 * Layout is part of the ABI (it is marshalled by the Unity binding):
 * only append new fields at the end.
 */
typedef struct ss_voice {
    float pitch_hz;      /* base fundamental frequency, ~70..400. 110 = adult male */
    float pitch_range;   /* intonation amount. 0 = monotone, 1 = normal, 2 = very expressive */
    float rate;          /* speaking-rate multiplier. 1 = normal, 2 = twice as fast */
    float formant_shift; /* vocal-tract size. 1 = adult male, ~1.17 female, ~1.35 child, <1 = giant */
    float breathiness;   /* 0..1: aspiration noise mixed into the voice */
    float roughness;     /* 0..1: pitch/amplitude jitter (gravelly, old, monster) */
    float whisper;       /* 0..1: 1 = no vocal fold vibration at all */
    float buzz;          /* 0..1: 0 = natural glottal pulse, 1 = harsh, buzzy, robotic */
    float volume;        /* linear output gain, 1 = default */
    float vibrato_hz;    /* vibrato rate (0 = off) */
    float vibrato_depth; /* vibrato depth in semitones */
    unsigned int seed;   /* noise seed; output is fully deterministic for a given seed */
} ss_voice;

typedef enum ss_preset {
    SS_PRESET_DEFAULT = 0, /* neutral adult male */
    SS_PRESET_MALE,
    SS_PRESET_FEMALE,
    SS_PRESET_CHILD,
    SS_PRESET_ROBOT,
    SS_PRESET_GIANT,
    SS_PRESET_WHISPER,
    SS_PRESET_GOBLIN,
    SS_PRESET_ELDER,
    SS_PRESET_COUNT
} ss_preset;

/* Fill *voice with a preset. Unknown presets give SS_PRESET_DEFAULT. */
SS_API void ss_voice_preset(ss_voice *voice, int preset);

/* ------------------------------------------------------------------------ */
/* Events (lip sync, subtitles, gameplay hooks)                             */
/* ------------------------------------------------------------------------ */

typedef enum ss_event_type {
    SS_EVENT_UTTERANCE_START = 0,
    SS_EVENT_WORD,           /* a word starts: text_offset/text_length index the input text */
    SS_EVENT_PHONEME,        /* a phoneme starts: phoneme is an ss_phoneme id */
    SS_EVENT_VISEME,         /* mouth shape changes: viseme is an ss_viseme id */
    SS_EVENT_UTTERANCE_END
} ss_event_type;

/* Mouth shapes. Matches the common 15-viseme set used by most lip-sync rigs. */
typedef enum ss_viseme {
    SS_VISEME_SIL = 0, /* silence / closed relaxed */
    SS_VISEME_PP,      /* p b m */
    SS_VISEME_FF,      /* f v */
    SS_VISEME_TH,      /* th dh */
    SS_VISEME_DD,      /* t d */
    SS_VISEME_KK,      /* k g ng */
    SS_VISEME_CH,      /* ch jh sh zh */
    SS_VISEME_SS,      /* s z */
    SS_VISEME_NN,      /* n l */
    SS_VISEME_RR,      /* r er */
    SS_VISEME_AA,      /* aa ae ah ay aw */
    SS_VISEME_E,       /* eh ey */
    SS_VISEME_IH,      /* iy ih y */
    SS_VISEME_OH,      /* ao ow oy */
    SS_VISEME_OU,      /* uw uh w */
    SS_VISEME_COUNT
} ss_viseme;

/* Layout is part of the ABI: only append fields. */
typedef struct ss_event {
    int   type;         /* ss_event_type */
    int   utterance_id; /* id returned by ss_engine_queue() (0 for ss_utterance_get_event) */
    int   block_offset; /* engine events: frame index inside the render block it fired in */
    float time;         /* seconds since the start of the utterance */
    float duration;     /* seconds: length of the word / phoneme / viseme */
    int   phoneme;      /* SS_EVENT_PHONEME: phoneme id */
    int   viseme;       /* SS_EVENT_VISEME / SS_EVENT_PHONEME: viseme id */
    int   word_index;   /* index of the word in the utterance, -1 if none */
    int   text_offset;  /* SS_EVENT_WORD: byte offset of the word in the input text */
    int   text_length;  /* SS_EVENT_WORD: byte length of the word in the input text */
} ss_event;

/* ------------------------------------------------------------------------ */
/* Utterances: text compiled into a playable form (thread-safe, pure)       */
/* ------------------------------------------------------------------------ */

typedef struct ss_utterance ss_utterance;

/* Input flags for ss_utterance_create(). */
#define SS_INPUT_TEXT     0 /* plain English text; {AA1 R P AH0 B EH T} braces embed phonemes */
#define SS_INPUT_PHONEMES 1 /* the whole input is ARPAbet phonemes, e.g. "HH AH0 L OW1" */

/*
 * Compile text into an utterance for a given voice and output sample rate.
 * Returns NULL on allocation failure or invalid arguments. Empty text gives a
 * valid, silent utterance.
 */
SS_API ss_utterance *ss_utterance_create(const char *text, const ss_voice *voice,
                                         int sample_rate, int flags);
SS_API void ss_utterance_destroy(ss_utterance *utt);

/* Length in samples (frames) at the utterance's sample rate. */
SS_API int ss_utterance_length(const ss_utterance *utt);

/* Pre-computed event timeline: useful for baking lip-sync or subtitles offline. */
SS_API int ss_utterance_event_count(const ss_utterance *utt);
SS_API int ss_utterance_get_event(const ss_utterance *utt, int index, ss_event *out);

/*
 * Write the utterance's phonemes as an ARPAbet string with stress digits,
 * words separated by " | ". Returns the full length needed (excluding the
 * terminator), like snprintf. Pass cap = 0 to query the size.
 */
SS_API int ss_utterance_phonemes(const ss_utterance *utt, char *out, int cap);

/* ------------------------------------------------------------------------ */
/* Engines: streaming playback, one per talking character                   */
/* ------------------------------------------------------------------------ */

typedef struct ss_engine ss_engine;

SS_API ss_engine *ss_engine_create(int sample_rate);
SS_API void ss_engine_destroy(ss_engine *engine);
SS_API int  ss_engine_sample_rate(const ss_engine *engine);

/*
 * Queue an utterance for playback after anything already queued. The engine
 * takes ownership and frees it when done. Returns a positive utterance id, or
 * 0 on failure (the utterance is still freed). The utterance must have been
 * created with the engine's sample rate, otherwise it plays at the wrong
 * speed (it is still accepted).
 */
SS_API int ss_engine_queue(ss_engine *engine, ss_utterance *utt);

/* Convenience: ss_utterance_create() + ss_engine_queue(). voice may be NULL. */
SS_API int ss_engine_speak(ss_engine *engine, const char *text, const ss_voice *voice);

/* Stop talking: fades out over a few milliseconds, then drops the queue. */
SS_API void ss_engine_stop(ss_engine *engine);

/* 1 while anything is playing or queued. */
SS_API int ss_engine_is_speaking(const ss_engine *engine);

/*
 * Render `frames` mono float samples into `out` (silence when idle).
 * Returns the number of frames that contained speech.
 */
SS_API int ss_engine_render(ss_engine *engine, float *out, int frames);

/*
 * Render interleaved multi-channel output, the same signal on every channel.
 * Handy for audio callbacks that hand you an interleaved buffer.
 */
SS_API int ss_engine_render_interleaved(ss_engine *engine, float *out, int frames, int channels);

/*
 * Pop the next event produced by ss_engine_render(). Returns 1 and fills *out,
 * or 0 when the queue is empty. Events older than the queue capacity (256)
 * are dropped if you never poll.
 */
SS_API int ss_engine_poll_event(ss_engine *engine, ss_event *out);

/* ------------------------------------------------------------------------ */
/* One-shot helpers                                                         */
/* ------------------------------------------------------------------------ */

/*
 * Synthesize text to a newly allocated mono float buffer. Free the buffer
 * with ss_free(). Returns the number of samples, or -1 on failure.
 */
SS_API int ss_synthesize(const char *text, const ss_voice *voice, int sample_rate,
                         float **out_samples);

SS_API void ss_free(void *ptr);

/* Text -> ARPAbet, same format as ss_utterance_phonemes(). */
SS_API int ss_text_to_phonemes(const char *text, char *out, int cap);

/* ------------------------------------------------------------------------ */
/* Introspection                                                            */
/* ------------------------------------------------------------------------ */

SS_API int ss_phoneme_count(void);
SS_API const char *ss_phoneme_name(int phoneme);   /* "AA", "B", ... NULL if out of range */
SS_API int ss_phoneme_viseme(int phoneme);
SS_API const char *ss_viseme_name(int viseme);     /* "sil", "PP", ... */
SS_API const char *ss_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* SPEEDSPEECH_H */
