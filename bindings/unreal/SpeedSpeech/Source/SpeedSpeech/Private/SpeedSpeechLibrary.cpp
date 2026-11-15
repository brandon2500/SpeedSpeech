#include "SpeedSpeechLibrary.h"
#include "Sound/SoundWaveProcedural.h"
#include "speedspeech.h"

namespace SpeedSpeechPrivate
{
	static TArray<uint8> Utf8Bytes(const FString& Text)
	{
		FTCHARToUTF8 Converter(*Text);
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());
		Bytes.Add(0);
		return Bytes;
	}
}

FSpeedSpeechVoice USpeedSpeechLibrary::MakePresetVoice(ESpeedSpeechPreset Preset)
{
	return FSpeedSpeechVoice::FromPreset(Preset);
}

FString USpeedSpeechLibrary::TextToPhonemes(const FString& Text)
{
	const TArray<uint8> Utf8 = SpeedSpeechPrivate::Utf8Bytes(Text);
	const char* Str = reinterpret_cast<const char*>(Utf8.GetData());
	const int Needed = ss_text_to_phonemes(Str, nullptr, 0);
	TArray<ANSICHAR> Buffer;
	Buffer.SetNumZeroed(Needed + 1);
	ss_text_to_phonemes(Str, Buffer.GetData(), Buffer.Num());
	return FString(ANSI_TO_TCHAR(Buffer.GetData()));
}

USoundWave* USpeedSpeechLibrary::SynthesizeToSoundWave(const FString& Text, const FSpeedSpeechVoice& Voice, int32 SampleRate)
{
	SampleRate = FMath::Clamp(SampleRate, 8000, 96000);
	const TArray<uint8> Utf8 = SpeedSpeechPrivate::Utf8Bytes(Text);
	ss_voice NativeVoice;
	Voice.ToNative(NativeVoice);

	float* Samples = nullptr;
	const int Count = ss_synthesize(reinterpret_cast<const char*>(Utf8.GetData()), &NativeVoice, SampleRate, &Samples);
	if (Count <= 0 || !Samples)
	{
		ss_free(Samples);
		return nullptr;
	}

	TArray<int16> Pcm;
	Pcm.SetNumUninitialized(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		Pcm[i] = static_cast<int16>(FMath::Clamp(Samples[i], -1.f, 1.f) * 32767.f);
	}
	ss_free(Samples);

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = static_cast<float>(Count) / static_cast<float>(SampleRate);
	Wave->SoundGroup = SOUNDGROUP_Voice;
	Wave->bLooping = false;
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
	return Wave;
}

TArray<FSpeedSpeechEvent> USpeedSpeechLibrary::GetSpeechTimeline(const FString& Text, const FSpeedSpeechVoice& Voice, float& OutDuration)
{
	TArray<FSpeedSpeechEvent> Events;
	OutDuration = 0.f;
	const TArray<uint8> Utf8 = SpeedSpeechPrivate::Utf8Bytes(Text);
	ss_voice NativeVoice;
	Voice.ToNative(NativeVoice);

	const int SampleRate = 22050; // the timeline does not depend on the rate
	ss_utterance* Utterance = ss_utterance_create(reinterpret_cast<const char*>(Utf8.GetData()), &NativeVoice, SampleRate, SS_INPUT_TEXT);
	if (!Utterance)
	{
		return Events;
	}
	OutDuration = static_cast<float>(ss_utterance_length(Utterance)) / SampleRate;

	const int Count = ss_utterance_event_count(Utterance);
	Events.Reserve(Count);
	for (int i = 0; i < Count; ++i)
	{
		ss_event Native;
		if (!ss_utterance_get_event(Utterance, i, &Native))
		{
			continue;
		}
		FSpeedSpeechEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = static_cast<ESpeedSpeechEventType>(Native.type);
		Event.Time = Native.time;
		Event.Duration = Native.duration;
		Event.WordIndex = Native.word_index;
		Event.Viseme = static_cast<ESpeedSpeechViseme>(FMath::Clamp(Native.viseme, 0, SS_VISEME_COUNT - 1));
		if (Native.type == SS_EVENT_PHONEME)
		{
			if (const char* Name = ss_phoneme_name(Native.phoneme))
			{
				Event.Phoneme = FName(Name);
			}
		}
		if (Native.type == SS_EVENT_WORD && Native.text_offset >= 0 && Native.text_offset + Native.text_length < Utf8.Num())
		{
			const FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Utf8.GetData() + Native.text_offset), Native.text_length);
			Event.Word = FString(Converter.Length(), Converter.Get());
		}
	}
	ss_utterance_destroy(Utterance);
	return Events;
}

FName USpeedSpeechLibrary::GetVisemeName(ESpeedSpeechViseme Viseme)
{
	const char* Name = ss_viseme_name(static_cast<int>(Viseme));
	return Name ? FName(Name) : NAME_None;
}
