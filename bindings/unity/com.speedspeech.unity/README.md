# SpeedSpeech for Unity

Real-time speech synthesis for characters. It streams through a normal `AudioSource`, so 3D
spatialization, mixer groups and effects all apply. It also reports word and viseme events
for subtitles and lip sync.

## Install

**From a release (recommended).** Download from the
[latest release](https://github.com/brandon2500/SpeedSpeech/releases/latest). The packages
already contain the native plugins for Windows, macOS, Linux and Android. Then do one of:

- `com.speedspeech.unity-<version>.tgz`: in Unity, open **Window → Package Manager → + →
  Add package from tarball…** and pick the file. Keep the file inside your project (for
  example in a `Packages/` subfolder) so teammates resolve the same path.
- `SpeedSpeech-Unity-<version>.zip`: unzip it into your project's `Packages/` folder. That
  makes an embedded package you can edit.

Without a release you can download the `com.speedspeech.unity` artifact from any CI run
on the Actions tab and use **Add package from disk…** on its `package.json`.

**From source.** Build the native library for your machine and copy it into the package:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --config Release --target speedspeech
```

```bash
python3 tools/package_unity.py --host build
```

Then add the package from disk as above. Installing from a git URL gives you the scripts
plus the iOS/WebGL source plugin, but not the desktop binaries, because those are built by
CI and are not committed.

| Platform | How the native code gets there |
|---|---|
| Windows / macOS / Linux / Android | prebuilt library in `Plugins/<platform>` |
| iOS, tvOS, WebGL | `Plugins/Source/speedspeech.c`, compiled by IL2CPP |

## Use

1. Add an **AudioSource** and a **SpeedSpeech Source** component (*Add Component → Audio*)
   to a character. Set the AudioSource's Spatial Blend to 3D if you want positional speech.
2. Pick a preset, or untick *Use Preset* and design a custom voice.
3. Talk:

```csharp
using SpeedSpeech;

var speaker = GetComponent<SpeedSpeechSource>();
int id = speaker.Speak("Do you have the key?");
speaker.Speak("Then we are doomed.", SpeechVoice.FromPreset(VoicePreset.Giant));
speaker.SpeakPhonemes("HH AH0 L OW1 | W ER1 L D");
speaker.Stop();                                   // short fade-out, clears the queue

speaker.onWord.AddListener(w => subtitles.Highlight(w));
speaker.onSpeechFinished.AddListener(id => NextLine());
speaker.VisemeChanged += (viseme, seconds) => { /* drive your rig */ };
```

Heavy lines can be compiled off the main thread:

```csharp
int rate = speaker.SampleRate;
var utterance = await Task.Run(() => SpeechUtterance.Create(longText, voice, rate));
speaker.Speak(utterance);
```

### Lip sync

Add **SpeedSpeech Lip Sync** and assign the face's `SkinnedMeshRenderer`. Then either:

- fill **Viseme Shapes** with your blend shape names, in the order
  `sil PP FF TH DD kk CH SS nn RR aa E ih oh ou`, or
- set a single **Mouth Open Shape** for simple characters.

`Weights` and `MouthOpen` expose the smoothed values if your rig uses bones, sprites or
shaders.

### Baking

- `SpeedSpeechSynth.CreateClip(text, voice)` returns an `AudioClip`. This is the option for
  WebGL, which has no `OnAudioFilterRead`.
- `SpeechUtterance.GetEvents()` returns the full word/viseme timeline without playing
  anything.
- **Window → Audio → SpeedSpeech** previews voices and bakes lines to `.wav` assets.

## Notes

- Events are delivered in `Update`, so they lag the audio by about one audio buffer.
- One `SpeedSpeechSource` speaks one line at a time. Lines you queue play back to back.
- This binding was tested under Mono against the real native library using stand-in
  UnityEngine types. It has not yet been run inside the Unity editor, so please report any
  problems.
