// SpeedSpeech for Unity - drive blend shapes from speech visemes.
using System;
using UnityEngine;

namespace SpeedSpeech
{
    /// <summary>
    /// Animates a face from a <see cref="SpeedSpeechSource"/>.
    ///
    /// Two modes:
    ///  - Viseme rig: map each of the 15 visemes to a blend shape (names or empty).
    ///  - Simple rig: leave the viseme list empty and set <see cref="mouthOpenShape"/>;
    ///    a single "mouth open" shape is driven by how open each viseme is.
    ///
    /// <see cref="Weights"/> exposes the smoothed 0..1 weights for custom rigs (bones,
    /// sprites, shaders) even when no renderer is assigned.
    /// </summary>
    [AddComponentMenu("Audio/SpeedSpeech Lip Sync")]
    public class SpeedSpeechLipSync : MonoBehaviour
    {
        public SpeedSpeechSource source;
        public SkinnedMeshRenderer face;

        [Tooltip("Blend shape name per viseme, in order: sil PP FF TH DD kk CH SS nn RR aa E ih oh ou. Leave empty to skip.")]
        public string[] visemeShapes = new string[(int)Viseme.ou + 1];

        [Tooltip("Single blend shape for simple rigs (used for visemes without their own shape).")]
        public string mouthOpenShape = "";

        [Tooltip("Blend shape weight at full strength (Unity blend shapes usually go to 100).")]
        public float maxWeight = 100f;

        [Tooltip("Seconds to blend into a new mouth shape.")]
        [Range(0.005f, 0.3f)] public float attack = 0.05f;
        [Tooltip("Seconds to blend out of a mouth shape.")]
        [Range(0.005f, 0.5f)] public float release = 0.09f;

        /// <summary>How open the mouth is for each viseme (simple rig mode).</summary>
        public static readonly float[] Openness =
        {
            0.0f, 0.0f, 0.15f, 0.25f, 0.3f, 0.35f, 0.3f, 0.2f, 0.25f, 0.35f, 1.0f, 0.7f, 0.45f, 0.8f, 0.5f
        };

        const int Count = (int)Viseme.ou + 1;
        readonly float[] _weights = new float[Count];
        int[] _shapeIndex;
        int _openIndex = -1;
        Viseme _target = Viseme.sil;

        /// <summary>Smoothed weight (0..1) for each viseme.</summary>
        public float[] Weights => _weights;

        /// <summary>Smoothed mouth openness (0..1).</summary>
        public float MouthOpen { get; private set; }

        void Reset()
        {
            source = GetComponentInParent<SpeedSpeechSource>();
            face = GetComponentInChildren<SkinnedMeshRenderer>();
        }

        void OnEnable()
        {
            if (source == null) source = GetComponentInParent<SpeedSpeechSource>();
            if (source != null) source.VisemeChanged += OnViseme;
            ResolveShapes();
        }

        void OnDisable()
        {
            if (source != null) source.VisemeChanged -= OnViseme;
        }

        /// <summary>Call after changing the renderer or shape names at runtime.</summary>
        public void ResolveShapes()
        {
            _shapeIndex = new int[Count];
            Mesh mesh = face != null ? face.sharedMesh : null;
            for (int i = 0; i < Count; i++)
            {
                string name = visemeShapes != null && i < visemeShapes.Length ? visemeShapes[i] : null;
                _shapeIndex[i] = mesh != null && !string.IsNullOrEmpty(name) ? mesh.GetBlendShapeIndex(name) : -1;
            }
            _openIndex = mesh != null && !string.IsNullOrEmpty(mouthOpenShape) ? mesh.GetBlendShapeIndex(mouthOpenShape) : -1;
        }

        void OnViseme(Viseme v, float duration) => _target = v;

        void LateUpdate()
        {
            if (source != null && !source.IsSpeaking) _target = Viseme.sil;
            float dt = Time.deltaTime;
            float open = 0f;
            for (int i = 0; i < Count; i++)
            {
                float goal = i == (int)_target ? 1f : 0f;
                float tau = goal > _weights[i] ? attack : release;
                _weights[i] += (goal - _weights[i]) * (1f - Mathf.Exp(-dt / Mathf.Max(1e-4f, tau)));
                open += _weights[i] * Openness[i];
            }
            MouthOpen = Mathf.Clamp01(open);

            if (face == null) return;
            float simple = 0f;
            for (int i = 0; i < Count; i++)
            {
                if (_shapeIndex != null && _shapeIndex[i] >= 0) face.SetBlendShapeWeight(_shapeIndex[i], _weights[i] * maxWeight);
                else simple += _weights[i] * Openness[i];
            }
            if (_openIndex >= 0) face.SetBlendShapeWeight(_openIndex, Mathf.Clamp01(simple) * maxWeight);
        }
    }
}
