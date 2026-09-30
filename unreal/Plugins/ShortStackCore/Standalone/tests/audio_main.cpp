// Renders every synthesized sound and checks it is audible, finite and within range.
// Usage: audio_test [out-dir]   (writes 16-bit WAVs when a directory is given)
#include "ShortStack/Audio/Synth.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace audio_test
{
int Failures = 0;
std::string OutDir;

void WriteWav(const std::string& Name, const std::vector<float>& S)
{
	if (OutDir.empty())
	{
		return;
	}
	FILE* F = std::fopen((OutDir + "/" + Name + ".wav").c_str(), "wb");
	if (!F)
	{
		return;
	}
	const uint32_t Rate = ss::audio::SampleRate;
	const uint32_t DataBytes = static_cast<uint32_t>(S.size() * 2);
	auto U32 = [&](uint32_t V) { std::fwrite(&V, 4, 1, F); };
	auto U16 = [&](uint16_t V) { std::fwrite(&V, 2, 1, F); };
	std::fwrite("RIFF", 1, 4, F);
	U32(36 + DataBytes);
	std::fwrite("WAVEfmt ", 1, 8, F);
	U32(16);
	U16(1);
	U16(1);
	U32(Rate);
	U32(Rate * 2);
	U16(2);
	U16(16);
	std::fwrite("data", 1, 4, F);
	U32(DataBytes);
	for (const float V : S)
	{
		const float C = V < -1.0f ? -1.0f : V > 1.0f ? 1.0f : V;
		const int16_t I = static_cast<int16_t>(C * 32767.0f);
		std::fwrite(&I, 2, 1, F);
	}
	std::fclose(F);
}

void Check(const std::string& Name, const std::vector<float>& S, float MinPeak)
{
	float Peak = 0.0f;
	bool Finite = true;
	for (const float V : S)
	{
		Finite = Finite && std::isfinite(V);
		Peak = std::fabs(V) > Peak ? std::fabs(V) : Peak;
	}
	const bool Ok = Finite && Peak >= MinPeak && Peak <= 1.0f && !S.empty();
	std::printf("  %-10s %6.2f s  peak %.3f %s\n", Name.c_str(), static_cast<double>(S.size()) / ss::audio::SampleRate, static_cast<double>(Peak), Ok ? "" : "FAILED");
	Failures += Ok ? 0 : 1;
	WriteWav(Name, S);
}
} // namespace audio_test

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		audio_test::OutDir = Argv[1];
	}
	for (int I = 0; I <= static_cast<int>(ss::SoundId::Cash); ++I)
	{
		const ss::SoundId Id = static_cast<ss::SoundId>(I);
		audio_test::Check(ss::SoundName(Id), ss::audio::Render(Id, 1.0f, 7u + static_cast<uint32_t>(I)), 0.01f);
	}
	audio_test::Check("buzz", ss::audio::Render(ss::audio::Effect::Buzz, 1u), 0.05f);
	audio_test::Check("canopen", ss::audio::Render(ss::audio::Effect::CanOpen, 2u), 0.02f);
	audio_test::Check("heartbeat", ss::audio::Render(ss::audio::Effect::Thump, 3u), 0.05f);
	audio_test::Check("thunder", ss::audio::RenderThunder(1.0f, 4u), 0.02f);
	ss::audio::Ambience Amb(5u);
	std::vector<float> Room(static_cast<size_t>(ss::audio::SampleRate) * 6);
	for (size_t I = 0; I < Room.size(); I += 480)
	{
		Amb.Render(Room.data() + I, 480);
	}
	audio_test::Check("ambience", Room, 0.02f);
	if (audio_test::Failures == 0)
	{
		std::printf("audio tests: all passed\n");
	}
	return audio_test::Failures == 0 ? 0 : 1;
}
