// SpeedSpeech for Unity - bake lines to WAV assets in the editor.
using System.IO;
using UnityEditor;
using UnityEngine;

namespace SpeedSpeech.Editor
{
    /// <summary>Window > Audio > SpeedSpeech: preview voices and bake lines to .wav files.</summary>
    public class SpeedSpeechBakeWindow : EditorWindow
    {
        string _text = "Welcome, traveler. The dragon awaits.";
        VoicePreset _preset = VoicePreset.Default;
        bool _custom;
        SpeechVoice _voice = SpeechVoice.Default;
        int _sampleRate = 22050;
        string _phonemes = "";
        AudioClip _preview;
        Vector2 _scroll;

        [MenuItem("Window/Audio/SpeedSpeech")]
        static void Open() => GetWindow<SpeedSpeechBakeWindow>("SpeedSpeech");

        void OnGUI()
        {
            _scroll = EditorGUILayout.BeginScrollView(_scroll);
            EditorGUILayout.LabelField("Text");
            _text = EditorGUILayout.TextArea(_text, GUILayout.MinHeight(60));
            _preset = (VoicePreset)EditorGUILayout.EnumPopup("Preset", _preset);
            _custom = EditorGUILayout.Toggle("Customize", _custom);
            if (_custom)
            {
                EditorGUI.indentLevel++;
                _voice.pitchHz = EditorGUILayout.Slider("Pitch (Hz)", _voice.pitchHz, 30, 600);
                _voice.pitchRange = EditorGUILayout.Slider("Intonation", _voice.pitchRange, 0, 3);
                _voice.rate = EditorGUILayout.Slider("Rate", _voice.rate, 0.3f, 3);
                _voice.formantShift = EditorGUILayout.Slider("Formant shift", _voice.formantShift, 0.6f, 1.8f);
                _voice.breathiness = EditorGUILayout.Slider("Breathiness", _voice.breathiness, 0, 1);
                _voice.roughness = EditorGUILayout.Slider("Roughness", _voice.roughness, 0, 1);
                _voice.whisper = EditorGUILayout.Slider("Whisper", _voice.whisper, 0, 1);
                _voice.buzz = EditorGUILayout.Slider("Buzz", _voice.buzz, 0, 1);
                _voice.vibratoHz = EditorGUILayout.Slider("Vibrato (Hz)", _voice.vibratoHz, 0, 12);
                _voice.vibratoDepth = EditorGUILayout.Slider("Vibrato depth", _voice.vibratoDepth, 0, 2);
                _voice.volume = EditorGUILayout.Slider("Volume", _voice.volume, 0, 4);
                if (GUILayout.Button("Load preset values")) _voice = SpeechVoice.FromPreset(_preset);
                EditorGUI.indentLevel--;
            }
            _sampleRate = EditorGUILayout.IntPopup("Sample rate", _sampleRate,
                new[] { "16000", "22050", "44100", "48000" }, new[] { 16000, 22050, 44100, 48000 });

            EditorGUILayout.Space();
            using (new EditorGUILayout.HorizontalScope())
            {
                if (GUILayout.Button("Preview")) Preview();
                if (GUILayout.Button("Bake to WAV...")) Bake();
            }
            if (!string.IsNullOrEmpty(_phonemes))
                EditorGUILayout.HelpBox(_phonemes, MessageType.None);
            EditorGUILayout.EndScrollView();
        }

        SpeechVoice CurrentVoice => _custom ? _voice : SpeechVoice.FromPreset(_preset);

        void Preview()
        {
            _phonemes = SpeedSpeechSynth.ToPhonemes(_text);
            if (_preview != null) DestroyImmediate(_preview);
            _preview = SpeedSpeechSynth.CreateClip(_text, CurrentVoice, _sampleRate, "SpeedSpeechPreview");
            // Editor-only preview through a hidden AudioSource
            var go = EditorUtility.CreateGameObjectWithHideFlags("SpeedSpeechPreview", HideFlags.HideAndDontSave, typeof(AudioSource));
            var src = go.GetComponent<AudioSource>();
            src.clip = _preview;
            src.Play();
            double end = EditorApplication.timeSinceStartup + _preview.length + 0.5;
            void Cleanup()
            {
                if (EditorApplication.timeSinceStartup < end) return;
                EditorApplication.update -= Cleanup;
                if (go != null) DestroyImmediate(go);
            }
            EditorApplication.update += Cleanup;
        }

        void Bake()
        {
            string path = EditorUtility.SaveFilePanelInProject("Bake speech", "line", "wav", "Save the spoken line as a WAV asset");
            if (string.IsNullOrEmpty(path)) return;
            float[] samples = SpeedSpeechSynth.Synthesize(_text, CurrentVoice, _sampleRate);
            File.WriteAllBytes(path, Wav.Encode(samples, _sampleRate));
            AssetDatabase.ImportAsset(path);
        }
    }

    /// <summary>Minimal 16-bit PCM WAV writer.</summary>
    public static class Wav
    {
        public static byte[] Encode(float[] samples, int sampleRate)
        {
            using (var ms = new MemoryStream())
            using (var w = new BinaryWriter(ms))
            {
                int bytes = samples.Length * 2;
                w.Write(new[] { 'R', 'I', 'F', 'F' });
                w.Write(36 + bytes);
                w.Write(new[] { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
                w.Write(16);
                w.Write((short)1);
                w.Write((short)1);
                w.Write(sampleRate);
                w.Write(sampleRate * 2);
                w.Write((short)2);
                w.Write((short)16);
                w.Write(new[] { 'd', 'a', 't', 'a' });
                w.Write(bytes);
                foreach (float s in samples)
                    w.Write((short)(Mathf.Clamp(s, -1f, 1f) * 32767f));
                w.Flush();
                return ms.ToArray();
            }
        }
    }
}
