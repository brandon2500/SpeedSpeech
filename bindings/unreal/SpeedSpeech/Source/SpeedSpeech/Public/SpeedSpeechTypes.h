// SpeedSpeech - Blueprint types.
#pragma once

#include "CoreMinimal.h"
#include "SpeedSpeechTypes.generated.h"

struct ss_voice;

/** Built-in character voices. */
UENUM(BlueprintType)
enum class ESpeedSpeechPreset : uint8
{
	Default = 0,
	Male,
	Female,
	Child,
	Robot,
	Giant,
	Whisper,
	Goblin,
	Elder,
};

/** The 15 standard mouth shapes used by most lip-sync rigs. */
UENUM(BlueprintType)
enum class ESpeedSpeechViseme : uint8
{
	Sil = 0 UMETA(DisplayName = "sil (silence)"),
	PP      UMETA(DisplayName = "PP (p b m)"),
	FF      UMETA(DisplayName = "FF (f v)"),
	TH      UMETA(DisplayName = "TH (th)"),
	DD      UMETA(DisplayName = "DD (t d)"),
	KK      UMETA(DisplayName = "kk (k g)"),
	CH      UMETA(DisplayName = "CH (ch j sh)"),
	SS      UMETA(DisplayName = "SS (s z)"),
	NN      UMETA(DisplayName = "nn (n l)"),
	RR      UMETA(DisplayName = "RR (r)"),
	AA      UMETA(DisplayName = "aa (a)"),
	E       UMETA(DisplayName = "E (e)"),
	IH      UMETA(DisplayName = "ih (i)"),
	OH      UMETA(DisplayName = "oh (o)"),
	OU      UMETA(DisplayName = "ou (u)"),
};

UENUM(BlueprintType)
enum class ESpeedSpeechEventType : uint8
{
	UtteranceStart = 0,
	Word,
	Phoneme,
	Viseme,
	UtteranceEnd,
};

/** Every knob that defines a character's voice. */
USTRUCT(BlueprintType)
struct SPEEDSPEECH_API FSpeedSpeechVoice
{
	GENERATED_BODY()

	/** Base pitch in Hz. ~110 adult male, ~200 adult female, ~280 child. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "30", ClampMax = "1000", UIMin = "30", UIMax = "600"))
	float PitchHz = 110.f;

	/** Intonation amount. 0 = monotone, 1 = normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "4", UIMax = "3"))
	float PitchRange = 1.f;

	/** Speaking-rate multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0.2", ClampMax = "5", UIMin = "0.3", UIMax = "3"))
	float Rate = 1.f;

	/** Vocal tract size. 1 = adult male, ~1.17 female, ~1.35 child, < 1 giant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0.5", ClampMax = "2", UIMin = "0.6", UIMax = "1.8"))
	float FormantShift = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "1"))
	float Breathiness = 0.05f;

	/** Pitch/amplitude jitter: gravelly, old or monstrous voices. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "1"))
	float Whisper = 0.f;

	/** 0 = natural, 1 = harsh and robotic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "1"))
	float Buzz = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "8", UIMax = "4"))
	float Volume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "30", UIMax = "12"))
	float VibratoHz = 0.f;

	/** Vibrato depth in semitones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ClampMin = "0", ClampMax = "12", UIMax = "2"))
	float VibratoDepth = 0.f;

	/** Noise seed: output is deterministic for a given seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	int32 Seed = 0x5EED;

	/** Values of a built-in preset. */
	static FSpeedSpeechVoice FromPreset(ESpeedSpeechPreset Preset);

	/** Convert to the C struct. */
	void ToNative(ss_voice& Out) const;
};

/** A speech timeline event. */
USTRUCT(BlueprintType)
struct SPEEDSPEECH_API FSpeedSpeechEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	ESpeedSpeechEventType Type = ESpeedSpeechEventType::UtteranceStart;

	/** Id returned by Speak. */
	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	int32 UtteranceId = 0;

	/** Seconds since the start of the utterance. */
	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	float Time = 0.f;

	/** Length of the word / phoneme / viseme in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	float Duration = 0.f;

	/** ARPAbet phoneme (Phoneme events). */
	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	FName Phoneme;

	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	ESpeedSpeechViseme Viseme = ESpeedSpeechViseme::Sil;

	/** Index of the word in the utterance, -1 if none. */
	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	int32 WordIndex = -1;

	/** The word as written in the input text (Word events). */
	UPROPERTY(BlueprintReadOnly, Category = "SpeedSpeech")
	FString Word;
};
