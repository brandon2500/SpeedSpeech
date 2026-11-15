// SpeedSpeech for Unity - native interop.
// Mirrors core/include/speedspeech.h. Struct layouts are part of the ABI.
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace SpeedSpeech
{
    /// <summary>Built-in character voices.</summary>
    public enum VoicePreset
    {
        Default = 0, Male, Female, Child, Robot, Giant, Whisper, Goblin, Elder
    }

    /// <summary>Kinds of timeline events produced while speaking.</summary>
    public enum SpeechEventType
    {
        UtteranceStart = 0, Word, Phoneme, Viseme, UtteranceEnd
    }

    /// <summary>The 15 standard mouth shapes used by most lip-sync rigs.</summary>
    public enum Viseme
    {
        sil = 0, PP, FF, TH, DD, kk, CH, SS, nn, RR, aa, E, ih, oh, ou
    }

    /// <summary>
    /// Every knob that defines a character's voice. Blittable: passed straight to native code.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct SpeechVoice
    {
        [UnityEngine.Tooltip("Base pitch in Hz. ~110 adult male, ~200 adult female, ~280 child.")]
        [UnityEngine.Range(30f, 600f)] public float pitchHz;
        [UnityEngine.Tooltip("Intonation amount. 0 = monotone, 1 = normal.")]
        [UnityEngine.Range(0f, 3f)] public float pitchRange;
        [UnityEngine.Tooltip("Speaking rate multiplier.")]
        [UnityEngine.Range(0.3f, 3f)] public float rate;
        [UnityEngine.Tooltip("Vocal tract size. 1 = adult male, 1.17 female, 1.35 child, < 1 giant.")]
        [UnityEngine.Range(0.6f, 1.8f)] public float formantShift;
        [UnityEngine.Range(0f, 1f)] public float breathiness;
        [UnityEngine.Tooltip("Pitch/amplitude jitter: gravelly, old or monstrous voices.")]
        [UnityEngine.Range(0f, 1f)] public float roughness;
        [UnityEngine.Range(0f, 1f)] public float whisper;
        [UnityEngine.Tooltip("0 = natural, 1 = harsh and robotic.")]
        [UnityEngine.Range(0f, 1f)] public float buzz;
        [UnityEngine.Range(0f, 4f)] public float volume;
        [UnityEngine.Range(0f, 12f)] public float vibratoHz;
        [UnityEngine.Tooltip("Vibrato depth in semitones.")]
        [UnityEngine.Range(0f, 2f)] public float vibratoDepth;
        [UnityEngine.Tooltip("Noise seed. Output is deterministic for a given seed.")]
        public uint seed;

        /// <summary>The neutral default voice (same values as SS_PRESET_DEFAULT).</summary>
        public static SpeechVoice Default => new SpeechVoice
        {
            pitchHz = 110f, pitchRange = 1f, rate = 1f, formantShift = 1f,
            breathiness = 0.05f, roughness = 0.02f, volume = 1f, seed = 0x5EED
        };

        /// <summary>Get one of the built-in presets.</summary>
        public static SpeechVoice FromPreset(VoicePreset preset)
        {
            SpeechVoice v;
            SpeedSpeechNative.ss_voice_preset(out v, (int)preset);
            return v;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEvent
    {
        public int type;
        public int utteranceId;
        public int blockOffset;
        public float time;
        public float duration;
        public int phoneme;
        public int viseme;
        public int wordIndex;
        public int textOffset;
        public int textLength;
    }

    /// <summary>A speech timeline event (word, phoneme, viseme, start or end).</summary>
    public struct SpeechEvent
    {
        public SpeechEventType type;
        /// <summary>Id returned by Speak().</summary>
        public int utteranceId;
        /// <summary>Seconds since the start of the utterance.</summary>
        public float time;
        /// <summary>Length of the word / phoneme / viseme in seconds.</summary>
        public float duration;
        /// <summary>ARPAbet phoneme (Phoneme events), otherwise null.</summary>
        public string phoneme;
        public Viseme viseme;
        /// <summary>Index of the word in the utterance, -1 if none.</summary>
        public int wordIndex;
        /// <summary>The word as written in the input text (Word events), otherwise null.</summary>
        public string word;

        public override string ToString()
        {
            switch (type)
            {
                case SpeechEventType.Word: return $"{time:0.000}s word \"{word}\"";
                case SpeechEventType.Phoneme: return $"{time:0.000}s phoneme {phoneme}";
                case SpeechEventType.Viseme: return $"{time:0.000}s viseme {viseme}";
                default: return $"{time:0.000}s {type}";
            }
        }

        internal static SpeechEvent FromNative(in NativeEvent e, byte[] utf8Text)
        {
            var ev = new SpeechEvent
            {
                type = (SpeechEventType)e.type,
                utteranceId = e.utteranceId,
                time = e.time,
                duration = e.duration,
                viseme = e.viseme >= 0 ? (Viseme)e.viseme : Viseme.sil,
                wordIndex = e.wordIndex,
            };
            if (ev.type == SpeechEventType.Phoneme)
                ev.phoneme = SpeedSpeechNative.PhonemeName(e.phoneme);
            if (ev.type == SpeechEventType.Word && utf8Text != null && e.textOffset >= 0 &&
                e.textLength > 0 && e.textOffset + e.textLength <= utf8Text.Length)
                ev.word = Encoding.UTF8.GetString(utf8Text, e.textOffset, e.textLength);
            return ev;
        }
    }

    /// <summary>Raw P/Invoke declarations. Prefer the higher level API.</summary>
    public static class SpeedSpeechNative
    {
#if (UNITY_IOS || UNITY_WEBGL) && !UNITY_EDITOR
        // Compiled from Plugins/Source/speedspeech.c by IL2CPP.
        public const string Library = "__Internal";
#else
        public const string Library = "speedspeech";
#endif
        public const int SS_INPUT_TEXT = 0;
        public const int SS_INPUT_PHONEMES = 1;

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ss_voice_preset(out SpeechVoice voice, int preset);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ss_utterance_create(byte[] utf8Text, ref SpeechVoice voice, int sampleRate, int flags);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ss_utterance_destroy(IntPtr utterance);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_utterance_length(IntPtr utterance);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_utterance_event_count(IntPtr utterance);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ss_utterance_get_event(IntPtr utterance, int index, out NativeEvent ev);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_utterance_phonemes(IntPtr utterance, byte[] buffer, int capacity);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ss_engine_create(int sampleRate);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ss_engine_destroy(IntPtr engine);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_engine_queue(IntPtr engine, IntPtr utterance);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ss_engine_stop(IntPtr engine);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_engine_is_speaking(IntPtr engine);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_engine_render(IntPtr engine, float[] output, int frames);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ss_engine_poll_event(IntPtr engine, out NativeEvent ev);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_text_to_phonemes(byte[] utf8Text, byte[] buffer, int capacity);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ss_phoneme_count();

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ss_phoneme_name(int phoneme);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ss_version_string();

        // ---- helpers ----

        static string[] s_phonemeNames;

        internal static byte[] ToUtf8(string text)
        {
            text = text ?? string.Empty;
            var bytes = new byte[Encoding.UTF8.GetByteCount(text) + 1];
            Encoding.UTF8.GetBytes(text, 0, text.Length, bytes, 0);
            return bytes; // null terminated
        }

        internal static string FromUtf8(byte[] buffer, int length)
        {
            if (length <= 0) return string.Empty;
            return Encoding.UTF8.GetString(buffer, 0, Math.Min(length, buffer.Length - 1));
        }

        /// <summary>ARPAbet name of a phoneme id (cached, safe on the audio thread).</summary>
        public static string PhonemeName(int id)
        {
            var names = s_phonemeNames;
            if (names == null)
            {
                int n = ss_phoneme_count();
                names = new string[n];
                for (int i = 0; i < n; i++) names[i] = Marshal.PtrToStringAnsi(ss_phoneme_name(i));
                s_phonemeNames = names;
            }
            return id >= 0 && id < names.Length ? names[id] : null;
        }

        /// <summary>Version of the loaded native library.</summary>
        public static string Version => Marshal.PtrToStringAnsi(ss_version_string());
    }
}
