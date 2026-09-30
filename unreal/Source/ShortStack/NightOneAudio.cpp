#include "NightOneAudio.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace NightOneAudioDetail
{
USoundWaveProcedural* MakeWave(UObject* Outer, float Seconds)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
	Wave->SetSampleRate(ss::audio::SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = Seconds;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	return Wave;
}

void Queue(USoundWaveProcedural* Wave, const float* Samples, int32 Count, float Gain)
{
	TArray<int16> Pcm;
	Pcm.SetNumUninitialized(Count);
	for (int32 I = 0; I < Count; ++I)
	{
		Pcm[I] = static_cast<int16>(FMath::Clamp(Samples[I] * Gain, -1.0f, 1.0f) * 32767.0f);
	}
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * static_cast<int32>(sizeof(int16)));
}
} // namespace NightOneAudioDetail

UNightOneAudio::UNightOneAudio()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UNightOneAudio::StartAmbience()
{
	if (AmbienceComponent)
	{
		return;
	}
	AmbienceGen = MakeUnique<ss::audio::Ambience>(7u);
	AmbienceWave = NightOneAudioDetail::MakeWave(this, 10000.0f);
	AmbienceComponent = UGameplayStatics::SpawnSound2D(this, AmbienceWave.Get(), AmbienceLevel(), 1.0f, 0.0f, nullptr, false, false);
}

void UNightOneAudio::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Keep about a quarter second of ambience queued ahead of the audio renderer.
	if (AmbienceWave && AmbienceGen)
	{
		const int32 Target = ss::audio::SampleRate / 4 * static_cast<int32>(sizeof(int16));
		int32 Guard = 0;
		while (AmbienceWave->GetAvailableAudioByteCount() < Target && Guard++ < 16)
		{
			float Block[1200];
			AmbienceGen->Render(Block, 1200);
			NightOneAudioDetail::Queue(AmbienceWave, Block, 1200, 1.0f);
		}
	}
	HeartClock += DeltaTime;
	if (bHeart && HeartClock >= NextBeat)
	{
		NextBeat = HeartClock + 0.68;
		PlayEffect(ss::audio::Effect::Thump);
		bBeat = true;
	}
	// A procedural wave keeps its voice open after the queue runs dry, so stop each one-shot when it is done.
	for (int32 I = OneShots.Num() - 1; I >= 0; --I)
	{
		if (HeartClock >= OneShotEnds[I])
		{
			if (IsValid(OneShots[I]))
			{
				OneShots[I]->Stop();
			}
			OneShots.RemoveAt(I);
			OneShotEnds.RemoveAt(I);
		}
	}
	for (int32 I = Thunders.Num() - 1; I >= 0; --I)
	{
		if (HeartClock >= Thunders[I].At)
		{
			if (!bMuted)
			{
				PlayPcm(ss::audio::RenderThunder(Thunders[I].Strength, Seed++), 1.0f, true);
			}
			Thunders.RemoveAt(I);
		}
	}
}

void UNightOneAudio::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AmbienceComponent)
	{
		AmbienceComponent->Stop();
	}
	AmbienceComponent = nullptr;
	AmbienceWave = nullptr;
	for (UAudioComponent* Shot : OneShots)
	{
		if (IsValid(Shot))
		{
			Shot->Stop();
		}
	}
	OneShots.Reset();
	OneShotEnds.Reset();
	Super::EndPlay(EndPlayReason);
}

void UNightOneAudio::PlayPcm(const std::vector<float>& Samples, float Volume, bool bAmbience)
{
	if (bMuted || Samples.empty())
	{
		return;
	}
	const float Seconds = static_cast<float>(Samples.size()) / static_cast<float>(ss::audio::SampleRate);
	USoundWaveProcedural* Wave = NightOneAudioDetail::MakeWave(this, Seconds);
	NightOneAudioDetail::Queue(Wave, Samples.data(), static_cast<int32>(Samples.size()), 1.0f);
	const float Level = Volume * MasterVolume * (bAmbience ? AmbienceVolume : EffectsVolume);
	if (UAudioComponent* Shot = UGameplayStatics::SpawnSound2D(this, Wave, Level, 1.0f, 0.0f, nullptr, false, true))
	{
		OneShots.Add(Shot);
		OneShotEnds.Add(HeartClock + Seconds + 0.2);
	}
}

void UNightOneAudio::Play(ss::SoundId Id, float Volume)
{
	if (!bMuted)
	{
		PlayPcm(ss::audio::Render(Id, Volume, Seed++), 1.0f);
	}
}

void UNightOneAudio::PlayEffect(ss::audio::Effect Id)
{
	if (!bMuted)
	{
		PlayPcm(ss::audio::Render(Id, Seed++), 1.0f);
	}
}

void UNightOneAudio::Thunder(float Delay, float Strength)
{
	Thunders.Add({HeartClock + Delay, Strength});
}

void UNightOneAudio::SetHeartbeat(bool bOn)
{
	if (bOn && !bHeart)
	{
		NextBeat = HeartClock + 0.1;
	}
	bHeart = bOn;
}

void UNightOneAudio::SetMuted(bool bInMuted)
{
	bMuted = bInMuted;
	if (AmbienceComponent)
	{
		AmbienceComponent->SetVolumeMultiplier(AmbienceLevel());
	}
}

void UNightOneAudio::SetMix(float Master, float Effects, float Ambience)
{
	MasterVolume = FMath::Clamp(Master, 0.0f, 4.0f);
	EffectsVolume = FMath::Clamp(Effects, 0.0f, 1.0f);
	AmbienceVolume = FMath::Clamp(Ambience, 0.0f, 1.0f);
	if (AmbienceComponent)
	{
		AmbienceComponent->SetVolumeMultiplier(AmbienceLevel());
	}
}

bool UNightOneAudio::ConsumeBeat()
{
	const bool bWas = bBeat;
	bBeat = false;
	return bWas;
}
