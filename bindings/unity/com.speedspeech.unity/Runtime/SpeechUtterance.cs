// SpeedSpeech for Unity - compiled utterances and one-shot helpers.
using System;
using System.Collections.Generic;
using UnityEngine;

namespace SpeedSpeech
{
    /// <summary>
    /// Text compiled for a voice and sample rate. Creating one is thread-safe, so
    /// heavy lines can be prepared on a worker thread and spoken later.
    /// Pass it to <see cref="SpeedSpeechSource.Speak(SpeechUtterance)"/> (which takes
    /// ownership) or Dispose() it.
    /// </summary>
    public sealed class SpeechUtterance : IDisposable
    {
        IntPtr _handle;

        /// <summary>The input text.</summary>
        public string Text { get; }
        public int SampleRate { get; }
        public SpeechVoice Voice { get; }
        internal byte[] Utf8 { get; }

        SpeechUtterance(IntPtr handle, string text, byte[] utf8, SpeechVoice voice, int sampleRate)
        {
            _handle = handle;
            Text = text;
            Utf8 = utf8;
            Voice = voice;
            SampleRate = sampleRate;
        }

        /// <summary>Compile text (or ARPAbet when <paramref name="phonemeInput"/> is true).</summary>
        public static SpeechUtterance Create(string text, SpeechVoice voice, int sampleRate, bool phonemeInput = false)
        {
            var utf8 = SpeedSpeechNative.ToUtf8(text);
            var handle = SpeedSpeechNative.ss_utterance_create(utf8, ref voice, sampleRate,
                phonemeInput ? SpeedSpeechNative.SS_INPUT_PHONEMES : SpeedSpeechNative.SS_INPUT_TEXT);
            if (handle == IntPtr.Zero)
                throw new ArgumentException("SpeedSpeech: could not create utterance (invalid sample rate?)");
            return new SpeechUtterance(handle, text ?? string.Empty, utf8, voice, sampleRate);
        }

        public bool IsValid => _handle != IntPtr.Zero;

        /// <summary>Length in samples at <see cref="SampleRate"/>.</summary>
        public int LengthSamples => IsValid ? SpeedSpeechNative.ss_utterance_length(_handle) : 0;

        public float LengthSeconds => (float)LengthSamples / SampleRate;

        /// <summary>ARPAbet transcription, words separated by " | ".</summary>
        public string Phonemes
        {
            get
            {
                if (!IsValid) return string.Empty;
                int n = SpeedSpeechNative.ss_utterance_phonemes(_handle, null, 0);
                var buf = new byte[n + 1];
                SpeedSpeechNative.ss_utterance_phonemes(_handle, buf, buf.Length);
                return SpeedSpeechNative.FromUtf8(buf, n);
            }
        }

        /// <summary>The full event timeline, e.g. to bake lip sync or subtitles.</summary>
        public List<SpeechEvent> GetEvents()
        {
            var list = new List<SpeechEvent>();
            if (!IsValid) return list;
            int n = SpeedSpeechNative.ss_utterance_event_count(_handle);
            for (int i = 0; i < n; i++)
            {
                if (SpeedSpeechNative.ss_utterance_get_event(_handle, i, out var e) != 0)
                    list.Add(SpeechEvent.FromNative(e, Utf8));
            }
            return list;
        }

        /// <summary>Hand the native handle over (the receiver frees it).</summary>
        internal IntPtr Release()
        {
            var h = _handle;
            _handle = IntPtr.Zero;
            GC.SuppressFinalize(this);
            return h;
        }

        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                SpeedSpeechNative.ss_utterance_destroy(_handle);
                _handle = IntPtr.Zero;
            }
            GC.SuppressFinalize(this);
        }

        ~SpeechUtterance()
        {
            if (_handle != IntPtr.Zero) SpeedSpeechNative.ss_utterance_destroy(_handle);
        }
    }

    /// <summary>One-shot synthesis helpers.</summary>
    public static class SpeedSpeechSynth
    {
        /// <summary>Render text to mono float samples.</summary>
        public static float[] Synthesize(string text, SpeechVoice voice, int sampleRate = 22050)
        {
            return Render(SpeechUtterance.Create(text, voice, sampleRate));
        }

        /// <summary>Render a compiled utterance to mono float samples. Consumes the utterance.</summary>
        public static float[] Render(SpeechUtterance utterance)
        {
            int len = utterance.LengthSamples;
            var samples = new float[len];
            IntPtr engine = SpeedSpeechNative.ss_engine_create(utterance.SampleRate);
            if (engine == IntPtr.Zero)
            {
                utterance.Dispose();
                return samples;
            }
            try
            {
                SpeedSpeechNative.ss_engine_queue(engine, utterance.Release());
                SpeedSpeechNative.ss_engine_render(engine, samples, len);
            }
            finally
            {
                SpeedSpeechNative.ss_engine_destroy(engine);
            }
            return samples;
        }

        /// <summary>
        /// Bake text into an AudioClip. Works on every platform, including WebGL where
        /// streaming playback (OnAudioFilterRead) is unavailable. Main thread only.
        /// </summary>
        public static AudioClip CreateClip(string text, SpeechVoice voice, int sampleRate = 22050, string clipName = "SpeedSpeech")
        {
            var samples = Synthesize(text, voice, sampleRate);
            var clip = AudioClip.Create(clipName, Math.Max(1, samples.Length), 1, sampleRate, false);
            if (samples.Length > 0) clip.SetData(samples, 0);
            return clip;
        }

        /// <summary>Text to ARPAbet transcription.</summary>
        public static string ToPhonemes(string text)
        {
            var utf8 = SpeedSpeechNative.ToUtf8(text);
            int n = SpeedSpeechNative.ss_text_to_phonemes(utf8, null, 0);
            var buf = new byte[n + 1];
            SpeedSpeechNative.ss_text_to_phonemes(utf8, buf, buf.Length);
            return SpeedSpeechNative.FromUtf8(buf, n);
        }
    }
}
