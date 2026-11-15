// SpeedSpeech - a talking scene component.
#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "Containers/Queue.h"
#include "HAL/CriticalSection.h"
#include "SpeedSpeechTypes.h"
#include <atomic>
#include "SpeedSpeechComponent.generated.h"

struct ss_engine;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpeedSpeechUtteranceDelegate, int32, UtteranceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpeedSpeechWordDelegate, int32, UtteranceId, const FString&, Word, int32, WordIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpeedSpeechVisemeDelegate, ESpeedSpeechViseme, Viseme, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpeedSpeechEventDelegate, const FSpeedSpeechEvent&, Event);

/**
 * Synthesizes speech in real time and plays it from this component's location,
 * with the usual attenuation, spatialization, submix sends and concurrency settings.
 *
 * Text is compiled on the game thread; the audio render thread only renders.
 * Events (words, visemes, start/end) are broadcast on the game thread.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent, DisplayName = "SpeedSpeech"))
class SPEEDSPEECH_API USpeedSpeechComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	USpeedSpeechComponent(const FObjectInitializer& ObjectInitializer);

	/** Built-in voice used when bUsePreset is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedSpeech")
	ESpeedSpeechPreset Preset = ESpeedSpeechPreset::Default;

	/** Use the preset instead of the custom voice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedSpeech")
	bool bUsePreset = true;

	/** Custom voice used when bUsePreset is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedSpeech", meta = (EditCondition = "!bUsePreset"))
	FSpeedSpeechVoice CustomVoice;

	/** Optional line spoken on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedSpeech", meta = (MultiLine = true))
	FString SpeakOnBeginPlay;

	UPROPERTY(BlueprintAssignable, Category = "SpeedSpeech|Events")
	FSpeedSpeechUtteranceDelegate OnSpeechStarted;

	UPROPERTY(BlueprintAssignable, Category = "SpeedSpeech|Events")
	FSpeedSpeechUtteranceDelegate OnSpeechFinished;

	/** A word starts: great for subtitles and dialogue highlighting. */
	UPROPERTY(BlueprintAssignable, Category = "SpeedSpeech|Events")
	FSpeedSpeechWordDelegate OnWord;

	/** The mouth shape changes: drive morph targets / pose assets for lip sync. */
	UPROPERTY(BlueprintAssignable, Category = "SpeedSpeech|Events")
	FSpeedSpeechVisemeDelegate OnViseme;

	/** Every event, including phonemes. */
	UPROPERTY(BlueprintAssignable, Category = "SpeedSpeech|Events")
	FSpeedSpeechEventDelegate OnSpeechEvent;

	/** Speak text with this component's voice. Returns an utterance id (0 on failure). */
	UFUNCTION(BlueprintCallable, Category = "SpeedSpeech")
	int32 Speak(const FString& Text);

	/** Speak text with a specific voice. Returns an utterance id (0 on failure). */
	UFUNCTION(BlueprintCallable, Category = "SpeedSpeech")
	int32 SpeakWithVoice(const FString& Text, const FSpeedSpeechVoice& Voice);

	/** Speak ARPAbet phonemes, e.g. "HH AH0 L OW1 | W ER1 L D". */
	UFUNCTION(BlueprintCallable, Category = "SpeedSpeech")
	int32 SpeakPhonemes(const FString& Arpabet);

	/** Fade out the current line and drop everything queued. */
	UFUNCTION(BlueprintCallable, Category = "SpeedSpeech")
	void StopSpeaking();

	/** True while something is playing or queued. */
	UFUNCTION(BlueprintPure, Category = "SpeedSpeech")
	bool IsSpeaking() const;

	UFUNCTION(BlueprintPure, Category = "SpeedSpeech")
	ESpeedSpeechViseme GetCurrentViseme() const { return CurrentViseme; }

	/** The voice Speak() uses (preset or custom). */
	UFUNCTION(BlueprintPure, Category = "SpeedSpeech")
	FSpeedSpeechVoice GetEffectiveVoice() const;

protected:
	//~ USynthComponent
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

	//~ UActorComponent
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginDestroy() override;

private:
	/** Plain-data copy of a native event, safe to pass between threads. */
	struct FRawEvent
	{
		int32 Type = 0;
		int32 Ticket = 0;
		int32 Phoneme = -1;
		int32 Viseme = 0;
		int32 WordIndex = -1;
		int32 TextOffset = 0;
		int32 TextLength = 0;
		float Time = 0.f;
		float Duration = 0.f;
	};

	int32 QueueUtterance(const FString& Text, const FSpeedSpeechVoice& Voice, int32 Flags);
	void DestroyEngine();

	// Shared with the audio render thread.
	FCriticalSection EngineLock;
	ss_engine* Engine = nullptr;
	std::atomic<int32> EngineSampleRate{0};
	std::atomic<bool> bAudioSpeaking{false};
	std::atomic<int32> PendingCommands{0};
	TQueue<FRawEvent, EQueueMode::Spsc> EventQueue;
	TMap<int32, int32> TicketByNativeId; // audio thread only

	// Game thread only.
	TMap<int32, TArray<uint8>> TextByTicket;
	int32 NextTicket = 1;
	ESpeedSpeechViseme CurrentViseme = ESpeedSpeechViseme::Sil;
};
