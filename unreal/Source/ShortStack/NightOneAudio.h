#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "ShortStack/Audio/Synth.h"

#include "NightOneAudio.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/**
 * Plays the synthesized audio from ShortStackCore: one-shot sounds are
 * rendered to PCM and queued on procedural sound waves; the room ambience is
 * generated block by block and streamed. Also keeps the all-in heartbeat.
 */
UCLASS(ClassGroup = (ShortStack), meta = (BlueprintSpawnableComponent))
class SHORTSTACK_API UNightOneAudio : public UActorComponent
{
	GENERATED_BODY()

public:
	UNightOneAudio();

	void StartAmbience();
	/** Streams a different room tone (Source fills N mono samples at ss::audio::SampleRate). */
	void StartAmbience(TFunction<void(float*, int32)> Source, float Level = 1.0f);
	void Play(ss::SoundId Id, float Volume = 1.0f);
	/** A sound from a place in the world, falling off with distance. */
	void PlayAt(ss::SoundId Id, const FVector& At, float Volume = 1.0f);
	void PlayEffect(ss::audio::Effect Id, float Volume = 1.0f);
	void Thunder(float Delay, float Strength);
	void SetHeartbeat(bool bOn);
	void SetMuted(bool bInMuted);
	/** Master, effects and ambience (rain, thunder) levels, 0..1. */
	void SetMix(float Master, float Effects, float Ambience);
	bool IsMuted() const { return bMuted; }
	/** True for the frame a heartbeat thumps (drives the visual pulse). */
	bool ConsumeBeat();

	UPROPERTY(EditAnywhere, Category = "Short Stack", meta = (ClampMin = "0", ClampMax = "4"))
	float MasterVolume = 1.0f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void PlayPcm(const std::vector<float>& Samples, float Volume, bool bAmbience = false, const FVector* At = nullptr);
	float AmbienceLevel() const { return bMuted ? 0.0f : MasterVolume * AmbienceVolume; }

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> AmbienceWave;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AmbienceComponent;

	/** One-shot sounds still playing; each is stopped (and destroyed) once its samples have played. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> OneShots;
	TArray<double> OneShotEnds;

	TUniquePtr<ss::audio::Ambience> AmbienceGen;
	TFunction<void(float*, int32)> AmbienceSource;
	float AmbienceGain = 1.0f;
	UPROPERTY(Transient)
	TObjectPtr<class USoundAttenuation> Attenuation;
	float EffectsVolume = 1.0f;
	float AmbienceVolume = 1.0f;
	uint32 Seed = 1;
	bool bMuted = false;
	bool bHeart = false;
	bool bBeat = false;
	double HeartClock = 0.0;
	double NextBeat = 0.0;

	struct FPendingThunder
	{
		double At = 0.0;
		float Strength = 1.0f;
	};
	TArray<FPendingThunder> Thunders;
};
