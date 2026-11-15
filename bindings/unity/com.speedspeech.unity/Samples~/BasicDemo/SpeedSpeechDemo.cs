// Attach to a GameObject that has a SpeedSpeechSource. Press keys to talk.
using SpeedSpeech;
using UnityEngine;

public class SpeedSpeechDemo : MonoBehaviour
{
    public SpeedSpeechSource speaker;

    static readonly string[] Lines =
    {
        "Welcome, traveler! The village is under attack.",
        "Do you have the key to the north gate?",
        "You found 250 gold coins and 3 health potions.",
        "Warning. Shields at 15 percent.",
    };

    void Start()
    {
        if (speaker == null) speaker = GetComponent<SpeedSpeechSource>();
        speaker.onWord.AddListener(w => Debug.Log($"word: {w}"));
        speaker.onSpeechFinished.AddListener(id => Debug.Log($"finished line {id}"));
    }

    void Update()
    {
        for (int i = 0; i < Lines.Length; i++)
        {
            if (Input.GetKeyDown(KeyCode.Alpha1 + i))
            {
                speaker.Stop();
                speaker.Speak(Lines[i]);
            }
        }
        if (Input.GetKeyDown(KeyCode.Alpha5))
        {
            speaker.Stop();
            speaker.Speak("I am a robot. Beep boop.", SpeechVoice.FromPreset(VoicePreset.Robot));
        }
        if (Input.GetKeyDown(KeyCode.Space)) speaker.Stop();
    }

    void OnGUI()
    {
        GUILayout.Label("1-4: say a line   5: robot   Space: stop");
        GUILayout.Label($"Speaking: {speaker.IsSpeaking}   Mouth: {speaker.CurrentViseme}");
    }
}
