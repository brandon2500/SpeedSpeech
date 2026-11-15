// SpeedSpeech for Unity - a talking AudioSource.
using System;
using System.Collections.Concurrent;
using UnityEngine;
using UnityEngine.Events;

namespace SpeedSpeech
{
    [Serializable] public class SpeechWordEvent : UnityEvent<string> { }
    [Serializable] public class SpeechIdEvent : UnityEvent<int> { }

    /// <summary>
    /// Streams synthesized speech through this GameObject's AudioSource, so it is
    /// spatialized, mixed and routed like any other sound (3D settings, mixer groups,
    /// effects all apply).
    ///
    /// Text is compiled on the calling thread; the audio thread only renders.
    /// Events (words, visemes, start/end) are delivered on the main thread in Update.
    ///
    /// Not available on WebGL (no OnAudioFilterRead there): use
    /// <see cref="SpeedSpeechSynth.CreateClip"/> instead.
    /// </summary>
    [RequireComponent(typeof(AudioSource))]
    [AddComponentMenu("Audio/SpeedSpeech Source")]
    public class SpeedSpeechSource : MonoBehaviour
    {
        [Tooltip("Preset applied when 'Use Preset' is ticked.")]
        public VoicePreset preset = VoicePreset.Default;
        [Tooltip("Use the preset instead of the custom voice below.")]
        public bool usePreset = true;
        public SpeechVoice customVoice = SpeechVoice.Default;

        [Tooltip("Optional line spoken when the component starts.")]
        [TextArea] public string speakOnStart = "";

        [Header("Events (main thread)")]
        public SpeechIdEvent onSpeechStarted = new SpeechIdEvent();
        public SpeechIdEvent onSpeechFinished = new SpeechIdEvent();
        public SpeechWordEvent onWord = new SpeechWordEvent();

        /// <summary>Every event, including phonemes and visemes.</summary>
        public event Action<SpeechEvent> EventReceived;
        /// <summary>Mouth shape changes: (viseme, duration in seconds).</summary>
        public event Action<Viseme, float> VisemeChanged;

        /// <summary>The voice used by Speak(string).</summary>
        public SpeechVoice Voice
        {
            get => usePreset ? SpeechVoice.FromPreset(preset) : customVoice;
            set { customVoice = value; usePreset = false; }
        }

        /// <summary>True while something is playing or queued.</summary>
        public bool IsSpeaking => _speaking || !_pending.IsEmpty;

        /// <summary>Current mouth shape.</summary>
        public Viseme CurrentViseme { get; private set; }

        /// <summary>Output sample rate the engine runs at.</summary>
        public int SampleRate => _sampleRate;

        // ---- state shared with the audio thread ----
        readonly object _lock = new object();
        IntPtr _engine;
        int _sampleRate;
        float[] _mono = new float[1024];
        volatile bool _speaking;
        volatile bool _stopRequested;
        readonly ConcurrentQueue<Pending> _pending = new ConcurrentQueue<Pending>();
        readonly ConcurrentQueue<SpeechEvent> _events = new ConcurrentQueue<SpeechEvent>();
        // utterance id -> UTF-8 text, for resolving words (audio thread writes, both read)
        readonly ConcurrentDictionary<int, byte[]> _texts = new ConcurrentDictionary<int, byte[]>();
        // ids are handed out on the main thread so Speak() can return them immediately
        int _nextTicket = 1;
        readonly ConcurrentDictionary<int, int> _ticketByNativeId = new ConcurrentDictionary<int, int>();

        struct Pending
        {
            public IntPtr utterance;
            public byte[] utf8;
            public int ticket;
        }

        AudioSource _source;
        static AudioClip s_carrier;

        // ------------------------------------------------------------------
        // Public API
        // ------------------------------------------------------------------

        /// <summary>Speak text with this component's voice. Returns an utterance id.</summary>
        public int Speak(string text) => Speak(text, Voice);

        /// <summary>Speak text with a specific voice. Returns an utterance id.</summary>
        public int Speak(string text, SpeechVoice voice)
        {
            EnsureEngine();
            return Speak(SpeechUtterance.Create(text, voice, _sampleRate));
        }

        /// <summary>Speak ARPAbet phonemes, e.g. "HH AH0 L OW1 | W ER1 L D".</summary>
        public int SpeakPhonemes(string arpabet) => Speak(SpeechUtterance.Create(arpabet, Voice, EnsureEngine(), true));

        /// <summary>
        /// Queue a pre-compiled utterance (e.g. built on a worker thread). Takes ownership.
        /// It must have been created at <see cref="SampleRate"/>.
        /// </summary>
        public int Speak(SpeechUtterance utterance)
        {
            if (utterance == null || !utterance.IsValid) return 0;
            EnsureEngine();
            if (utterance.SampleRate != _sampleRate)
                Debug.LogWarning($"SpeedSpeech: utterance sample rate {utterance.SampleRate} != output rate {_sampleRate}; pitch and speed will be off.");
            int ticket = _nextTicket++;
            _pending.Enqueue(new Pending { utterance = utterance.Release(), utf8 = utterance.Utf8, ticket = ticket });
            if (_source != null && !_source.isPlaying && isActiveAndEnabled) _source.Play();
            return ticket;
        }

        /// <summary>Stop talking (short fade-out) and drop everything queued.</summary>
        public void Stop()
        {
            while (_pending.TryDequeue(out var p)) SpeedSpeechNative.ss_utterance_destroy(p.utterance);
            _stopRequested = true;
        }

        // ------------------------------------------------------------------
        // Unity lifecycle
        // ------------------------------------------------------------------

        void Awake()
        {
            _source = GetComponent<AudioSource>();
            EnsureEngine();
        }

        void OnEnable()
        {
            AudioSettings.OnAudioConfigurationChanged += OnAudioConfigurationChanged;
            if (_source == null) _source = GetComponent<AudioSource>();
            // The source plays a constant 1.0 "carrier" that we multiply by speech in
            // OnAudioFilterRead: the result keeps the source's 3D panning and attenuation.
            _source.clip = Carrier();
            _source.loop = true;
            if (!_source.isPlaying) _source.Play();
        }

        void Start()
        {
            if (!string.IsNullOrEmpty(speakOnStart)) Speak(speakOnStart);
        }

        void OnDisable()
        {
            AudioSettings.OnAudioConfigurationChanged -= OnAudioConfigurationChanged;
        }

        void OnDestroy()
        {
            lock (_lock)
            {
                if (_engine != IntPtr.Zero) SpeedSpeechNative.ss_engine_destroy(_engine);
                _engine = IntPtr.Zero;
            }
            while (_pending.TryDequeue(out var p)) SpeedSpeechNative.ss_utterance_destroy(p.utterance);
        }

        void OnAudioConfigurationChanged(bool deviceWasChanged)
        {
            int rate = AudioSettings.outputSampleRate;
            if (rate == _sampleRate) return;
            lock (_lock)
            {
                if (_engine != IntPtr.Zero) SpeedSpeechNative.ss_engine_destroy(_engine);
                _engine = SpeedSpeechNative.ss_engine_create(rate);
                _sampleRate = rate;
                _speaking = false;
            }
            if (_source != null && isActiveAndEnabled && !_source.isPlaying) _source.Play();
        }

        void Update()
        {
            while (_events.TryDequeue(out var e))
            {
                switch (e.type)
                {
                    case SpeechEventType.UtteranceStart: onSpeechStarted.Invoke(e.utteranceId); break;
                    case SpeechEventType.UtteranceEnd:
                        onSpeechFinished.Invoke(e.utteranceId);
                        CurrentViseme = Viseme.sil;
                        break;
                    case SpeechEventType.Word: onWord.Invoke(e.word); break;
                    case SpeechEventType.Viseme:
                        CurrentViseme = e.viseme;
                        VisemeChanged?.Invoke(e.viseme, e.duration);
                        break;
                }
                EventReceived?.Invoke(e);
            }
        }

        // ------------------------------------------------------------------
        // Audio thread
        // ------------------------------------------------------------------

        void OnAudioFilterRead(float[] data, int channels)
        {
            int frames = data.Length / channels;
            lock (_lock)
            {
                if (_engine == IntPtr.Zero)
                {
                    Array.Clear(data, 0, data.Length);
                    return;
                }
                if (_stopRequested)
                {
                    _stopRequested = false;
                    SpeedSpeechNative.ss_engine_stop(_engine);
                }
                while (_pending.TryDequeue(out var p))
                {
                    int id = SpeedSpeechNative.ss_engine_queue(_engine, p.utterance);
                    if (id > 0)
                    {
                        _texts[id] = p.utf8;
                        _ticketByNativeId[id] = p.ticket;
                    }
                }
                if (_mono.Length < frames) _mono = new float[frames];
                SpeedSpeechNative.ss_engine_render(_engine, _mono, frames);

                while (SpeedSpeechNative.ss_engine_poll_event(_engine, out var ne) != 0)
                {
                    _texts.TryGetValue(ne.utteranceId, out var utf8);
                    var ev = SpeechEvent.FromNative(ne, utf8);
                    if (_ticketByNativeId.TryGetValue(ne.utteranceId, out var ticket)) ev.utteranceId = ticket;
                    if (ev.type == SpeechEventType.UtteranceEnd)
                    {
                        _texts.TryRemove(ne.utteranceId, out _);
                        _ticketByNativeId.TryRemove(ne.utteranceId, out _);
                    }
                    _events.Enqueue(ev);
                }
                _speaking = SpeedSpeechNative.ss_engine_is_speaking(_engine) != 0;
                if (!_speaking && !_texts.IsEmpty)
                {
                    // lines dropped by Stop() never report an end: forget them all
                    _texts.Clear();
                    _ticketByNativeId.Clear();
                }
            }

            // data holds the carrier (1.0) after the source's gain: multiply in speech
            for (int i = 0, k = 0; i < frames; i++)
            {
                float s = _mono[i];
                for (int c = 0; c < channels; c++, k++) data[k] *= s;
            }
        }

        // ------------------------------------------------------------------

        int EnsureEngine()
        {
            if (_engine != IntPtr.Zero) return _sampleRate;
            lock (_lock)
            {
                if (_engine == IntPtr.Zero)
                {
                    _sampleRate = AudioSettings.outputSampleRate;
                    if (_sampleRate <= 0) _sampleRate = 48000;
                    _engine = SpeedSpeechNative.ss_engine_create(_sampleRate);
                }
            }
            return _sampleRate;
        }

        static AudioClip Carrier()
        {
            if (s_carrier != null) return s_carrier;
            int rate = AudioSettings.outputSampleRate > 0 ? AudioSettings.outputSampleRate : 48000;
            var ones = new float[rate / 10];
            for (int i = 0; i < ones.Length; i++) ones[i] = 1f;
            s_carrier = AudioClip.Create("SpeedSpeechCarrier", ones.Length, 1, rate, false);
            s_carrier.SetData(ones, 0);
            s_carrier.hideFlags = HideFlags.DontSave;
            return s_carrier;
        }
    }
}
