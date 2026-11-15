# SpeedSpeech for Unreal Engine

A runtime plugin that adds a **SpeedSpeech** audio component. It synthesizes speech in
real time and plays it from the component's location, using the normal audio pipeline:
attenuation, spatialization, submixes and concurrency. It also broadcasts word and viseme
events for subtitles and lip sync.

The C core is included as a single source file (`Source/ThirdParty/SpeedSpeechCore`) and
is compiled by UnrealBuildTool. There are no prebuilt binaries, so the plugin builds for
every platform Unreal targets.

## Install

1. Copy this `SpeedSpeech` folder into your project's `Plugins/` folder.
2. Regenerate project files, build, and enable **SpeedSpeech** under *Edit → Plugins →
   Audio* if it isn't already enabled.

The plugin is written against UE 5 (`USynthComponent` from the AudioMixer module). C++
projects should add `"SpeedSpeech"` to their module's dependencies.

## Blueprint

- Add a **SpeedSpeech** component to an actor and choose a **Preset**, or untick *Use
  Preset* and edit **Custom Voice**.
- **Speak** (Text) returns an utterance id. Related nodes: **Speak With Voice**,
  **Speak Phonemes**, **Stop Speaking**, **Is Speaking**.
- Events: **On Speech Started**, **On Speech Finished**, **On Word** (id, word, index),
  **On Viseme** (viseme, duration) and **On Speech Event** (everything).
- Library functions: **Make Preset Voice**, **Text To Phonemes**, **Get Speech Timeline**
  (bake lip sync or subtitles ahead of time), **Synthesize To Sound Wave** (one-shot
  procedural wave) and **Get Viseme Name**.

For lip sync, map `OnViseme` to morph targets or a pose asset. **Get Viseme Name** returns
`sil, PP, FF, TH, DD, kk, CH, SS, nn, RR, aa, E, ih, oh, ou`, so you can name morph targets
to match.

## C++

```cpp
#include "SpeedSpeechComponent.h"

USpeedSpeechComponent* Voice = CreateDefaultSubobject<USpeedSpeechComponent>(TEXT("Voice"));
Voice->SetupAttachment(GetRootComponent());
Voice->Preset = ESpeedSpeechPreset::Robot;

Voice->Speak(TEXT("Intruder detected. Initiating lockdown."));
Voice->OnWord.AddDynamic(this, &AMyActor::HandleWord);
```

Text is compiled on the game thread, and the audio render thread only renders. If you
need the raw C API, it is available in-module through `speedspeech.h`.

## Notes

- This plugin was compiled and exercised against stand-in engine headers with the real
  core, but it has not yet been built inside a real Unreal Engine project. Please report
  any compile errors along with your engine version.
- `Synthesize To Sound Wave` returns a procedural wave, which plays once. Render it again
  to replay.
