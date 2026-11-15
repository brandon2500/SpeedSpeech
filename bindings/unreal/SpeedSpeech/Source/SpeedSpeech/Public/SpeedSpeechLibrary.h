// SpeedSpeech - Blueprint helpers.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpeedSpeechTypes.h"
#include "SpeedSpeechLibrary.generated.h"

class USoundWave;

UCLASS()
class SPEEDSPEECH_API USpeedSpeechLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The values of a built-in voice preset (tweak them, then use SpeakWithVoice). */
	UFUNCTION(BlueprintPure, Category = "SpeedSpeech")
	static FSpeedSpeechVoice MakePresetVoice(ESpeedSpeechPreset Preset);

	/** ARPAbet transcription of text, words separated by " | ". */
	UFUNCTION(BlueprintPure, Category = "SpeedSpeech")
	static FString TextToPhonemes(const FString& Text);

	/**
	 * Render text into a procedural sound wave (plays once; render again to replay).
	 * Handy for UI sounds, or when a SpeedSpeech component is not wanted.
	 */
	UFUNCTION(BlueprintCallable, Category = "SpeedSpeech", meta = (AdvancedDisplay = "SampleRate"))
	static USoundWave* SynthesizeToSoundWave(const FString& Text, const FSpeedSpeechVoice& Voice, int32 SampleRate = 22050);

	/**
	 * The event timeline a line will produce (words, phonemes, visemes) without playing
	 * it: bake lip sync or subtitles ahead of time.
	 */
	UFUNCTION(BlueprintCallable, Category = "SpeedSpeech")
	static TArray<FSpeedSpeechEvent> GetSpeechTimeline(const FString& Text, const FSpeedSpeechVoice& Voice, float& OutDuration);

	/** Short name of a viseme ("sil", "PP", "aa", ...), e.g. to build morph target names. */
	UFUNCTION(BlueprintPure, Category = "SpeedSpeech")
	static FName GetVisemeName(ESpeedSpeechViseme Viseme);
};
