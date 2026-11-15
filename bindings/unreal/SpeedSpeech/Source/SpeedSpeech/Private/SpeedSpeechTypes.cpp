#include "SpeedSpeechTypes.h"
#include "speedspeech.h"

FSpeedSpeechVoice FSpeedSpeechVoice::FromPreset(ESpeedSpeechPreset Preset)
{
	ss_voice V;
	ss_voice_preset(&V, static_cast<int>(Preset));
	FSpeedSpeechVoice Out;
	Out.PitchHz = V.pitch_hz;
	Out.PitchRange = V.pitch_range;
	Out.Rate = V.rate;
	Out.FormantShift = V.formant_shift;
	Out.Breathiness = V.breathiness;
	Out.Roughness = V.roughness;
	Out.Whisper = V.whisper;
	Out.Buzz = V.buzz;
	Out.Volume = V.volume;
	Out.VibratoHz = V.vibrato_hz;
	Out.VibratoDepth = V.vibrato_depth;
	Out.Seed = static_cast<int32>(V.seed);
	return Out;
}

void FSpeedSpeechVoice::ToNative(ss_voice& Out) const
{
	Out.pitch_hz = PitchHz;
	Out.pitch_range = PitchRange;
	Out.rate = Rate;
	Out.formant_shift = FormantShift;
	Out.breathiness = Breathiness;
	Out.roughness = Roughness;
	Out.whisper = Whisper;
	Out.buzz = Buzz;
	Out.volume = Volume;
	Out.vibrato_hz = VibratoHz;
	Out.vibrato_depth = VibratoDepth;
	Out.seed = static_cast<unsigned int>(Seed);
}
