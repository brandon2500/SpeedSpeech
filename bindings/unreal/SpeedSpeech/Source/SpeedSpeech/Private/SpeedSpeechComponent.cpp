#include "SpeedSpeechComponent.h"
#include "speedspeech.h"

namespace SpeedSpeechPrivate
{
	/** Null-terminated UTF-8 copy of a string. */
	static TArray<uint8> ToUtf8(const FString& Text)
	{
		FTCHARToUTF8 Converter(*Text);
		TArray<uint8> Bytes;
		Bytes.Reserve(Converter.Length() + 1);
		Bytes.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());
		Bytes.Add(0);
		return Bytes;
	}
}

USpeedSpeechComponent::USpeedSpeechComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	NumChannels = 1;
}

bool USpeedSpeechComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	FScopeLock Lock(&EngineLock);
	if (Engine && EngineSampleRate == SampleRate)
	{
		return true;
	}
	if (Engine)
	{
		ss_engine_destroy(Engine);
	}
	Engine = ss_engine_create(SampleRate);
	EngineSampleRate = SampleRate;
	TicketByNativeId.Reset();
	bAudioSpeaking = false;
	return Engine != nullptr;
}

int32 USpeedSpeechComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	FScopeLock Lock(&EngineLock);
	if (!Engine)
	{
		FMemory::Memzero(OutAudio, NumSamples * sizeof(float));
		return NumSamples;
	}

	// NumChannels is 1, so NumSamples is the number of frames.
	ss_engine_render(Engine, OutAudio, NumSamples);

	ss_event Native;
	while (ss_engine_poll_event(Engine, &Native))
	{
		FRawEvent Raw;
		Raw.Type = Native.type;
		const int32* Ticket = TicketByNativeId.Find(Native.utterance_id);
		Raw.Ticket = Ticket ? *Ticket : 0;
		Raw.Phoneme = Native.phoneme;
		Raw.Viseme = Native.viseme;
		Raw.WordIndex = Native.word_index;
		Raw.TextOffset = Native.text_offset;
		Raw.TextLength = Native.text_length;
		Raw.Time = Native.time;
		Raw.Duration = Native.duration;
		EventQueue.Enqueue(Raw);
		if (Native.type == SS_EVENT_UTTERANCE_END)
		{
			TicketByNativeId.Remove(Native.utterance_id);
		}
	}

	const bool bSpeaking = ss_engine_is_speaking(Engine) != 0;
	if (!bSpeaking)
	{
		TicketByNativeId.Reset(); // utterances dropped by a stop never report an end
	}
	bAudioSpeaking = bSpeaking;
	return NumSamples;
}

void USpeedSpeechComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!SpeakOnBeginPlay.IsEmpty())
	{
		Speak(SpeakOnBeginPlay);
	}
}

void USpeedSpeechComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Read the idle state first: once the audio thread reports idle, every event
	// it produced is already in the queue.
	const bool bIdle = !IsSpeaking();

	FRawEvent Raw;
	while (EventQueue.Dequeue(Raw))
	{
		FSpeedSpeechEvent Event;
		Event.Type = static_cast<ESpeedSpeechEventType>(Raw.Type);
		Event.UtteranceId = Raw.Ticket;
		Event.Time = Raw.Time;
		Event.Duration = Raw.Duration;
		Event.WordIndex = Raw.WordIndex;
		Event.Viseme = static_cast<ESpeedSpeechViseme>(FMath::Clamp(Raw.Viseme, 0, SS_VISEME_COUNT - 1));
		if (Event.Type == ESpeedSpeechEventType::Phoneme)
		{
			if (const char* Name = ss_phoneme_name(Raw.Phoneme))
			{
				Event.Phoneme = FName(Name);
			}
		}
		if (Event.Type == ESpeedSpeechEventType::Word)
		{
			const TArray<uint8>* Text = TextByTicket.Find(Raw.Ticket);
			if (Text && Raw.TextOffset >= 0 && Raw.TextLength > 0 && Raw.TextOffset + Raw.TextLength < Text->Num())
			{
				const FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Text->GetData() + Raw.TextOffset), Raw.TextLength);
				Event.Word = FString(Converter.Length(), Converter.Get());
			}
		}

		switch (Event.Type)
		{
		case ESpeedSpeechEventType::UtteranceStart:
			OnSpeechStarted.Broadcast(Event.UtteranceId);
			break;
		case ESpeedSpeechEventType::UtteranceEnd:
			CurrentViseme = ESpeedSpeechViseme::Sil;
			TextByTicket.Remove(Event.UtteranceId);
			OnSpeechFinished.Broadcast(Event.UtteranceId);
			break;
		case ESpeedSpeechEventType::Word:
			OnWord.Broadcast(Event.UtteranceId, Event.Word, Event.WordIndex);
			break;
		case ESpeedSpeechEventType::Viseme:
			CurrentViseme = Event.Viseme;
			OnViseme.Broadcast(Event.Viseme, Event.Duration);
			break;
		default:
			break;
		}
		OnSpeechEvent.Broadcast(Event);
	}

	if (bIdle)
	{
		TextByTicket.Reset();
		CurrentViseme = ESpeedSpeechViseme::Sil;
	}
}

void USpeedSpeechComponent::BeginDestroy()
{
	DestroyEngine();
	Super::BeginDestroy();
}

void USpeedSpeechComponent::DestroyEngine()
{
	FScopeLock Lock(&EngineLock);
	if (Engine)
	{
		ss_engine_destroy(Engine);
		Engine = nullptr;
	}
	TicketByNativeId.Reset();
	bAudioSpeaking = false;
}

FSpeedSpeechVoice USpeedSpeechComponent::GetEffectiveVoice() const
{
	return bUsePreset ? FSpeedSpeechVoice::FromPreset(Preset) : CustomVoice;
}

int32 USpeedSpeechComponent::Speak(const FString& Text)
{
	return QueueUtterance(Text, GetEffectiveVoice(), SS_INPUT_TEXT);
}

int32 USpeedSpeechComponent::SpeakWithVoice(const FString& Text, const FSpeedSpeechVoice& Voice)
{
	return QueueUtterance(Text, Voice, SS_INPUT_TEXT);
}

int32 USpeedSpeechComponent::SpeakPhonemes(const FString& Arpabet)
{
	return QueueUtterance(Arpabet, GetEffectiveVoice(), SS_INPUT_PHONEMES);
}

int32 USpeedSpeechComponent::QueueUtterance(const FString& Text, const FSpeedSpeechVoice& Voice, int32 Flags)
{
	if (!IsPlaying())
	{
		Start(); // creates the synth, which calls Init() with the device sample rate
	}
	const int32 Rate = EngineSampleRate > 0 ? EngineSampleRate.load() : 48000;

	TArray<uint8> Utf8 = SpeedSpeechPrivate::ToUtf8(Text);
	ss_voice NativeVoice;
	Voice.ToNative(NativeVoice);

	// Compiling is the expensive part: do it here, on the game thread.
	ss_utterance* Utterance = ss_utterance_create(reinterpret_cast<const char*>(Utf8.GetData()), &NativeVoice, Rate, Flags);
	if (!Utterance)
	{
		return 0;
	}

	const int32 Ticket = NextTicket++;
	TextByTicket.Add(Ticket, MoveTemp(Utf8));
	++PendingCommands;
	SynthCommand([this, Utterance, Ticket]()
	{
		{
			FScopeLock Lock(&EngineLock);
			if (Engine)
			{
				const int Id = ss_engine_queue(Engine, Utterance);
				if (Id > 0)
				{
					TicketByNativeId.Add(Id, Ticket);
					bAudioSpeaking = true;
				}
			}
			else
			{
				ss_utterance_destroy(Utterance);
			}
		}
		--PendingCommands;
	});
	return Ticket;
}

void USpeedSpeechComponent::StopSpeaking()
{
	TextByTicket.Reset();
	++PendingCommands;
	SynthCommand([this]()
	{
		{
			FScopeLock Lock(&EngineLock);
			if (Engine)
			{
				ss_engine_stop(Engine);
			}
		}
		--PendingCommands;
	});
}

bool USpeedSpeechComponent::IsSpeaking() const
{
	return bAudioSpeaking || PendingCommands > 0;
}
