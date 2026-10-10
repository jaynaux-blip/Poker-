#pragma once

#include "ShortStack/Audio/Voice.h"

#include <cstdint>
#include <vector>

namespace ss
{
namespace audio
{
/**
 * The card room in stereo (next_segment_spec §4.4, §4.9), in room centimetres: the tables' chips and calls, the walla
 * that follows the night's mood, the rail, the desk and the cage, the PA, positioned around the listener with distance,
 * head shadow, reverb, muffling and ducking. Not thread-safe: one per stream, every call on one thread. Coefficients
 * per syllable and per event, never per sample (the fix for CardRoomAmbience.h:62-63).
 *
 * Bodies: audio-synth (CardRoom.cpp). Phase A, day 1: a stub that compiles and renders silence.
 */
class CardRoom
{
public:
	enum class Spot : int
	{
		Table,
		FeatureTable,
		Doorway,
		Bar,
		Rail,
		Desk,
		Cage,
		Terrace,
	};
	struct Emitter
	{
		Spot Kind = Spot::Table;
		int Id = 0;
		float X = 0.0f;
		float Y = 0.0f;
		bool Open = true;
		int Players = 6;
	};
	SHORTSTACKCORE_API explicit CardRoom(uint32_t Seed);
	SHORTSTACKCORE_API void SetEmitters(const std::vector<Emitter>& Emitters);
	SHORTSTACKCORE_API void SetListener(float X, float Y, float Yaw);
	/** dB, eased over 1.5 s. */
	SHORTSTACKCORE_API void SetMix(float Bed, float Talk, float Chips, float Calls, float Rail, float Crowd);
	SHORTSTACKCORE_API void SetMuffle(float CutoffHz, float Db);
	SHORTSTACKCORE_API void SetPace(float EventClock);
	SHORTSTACKCORE_API void Duck(float Db, float Seconds);
	SHORTSTACKCORE_API void TableHand(int Id, float Seconds, bool AllIn, bool Called, float PotBb);
	SHORTSTACKCORE_API void Say(const SpeechPlan& Plan, const Timbre& T, Spot Where, int Id, float Effort, bool Murmur);
	SHORTSTACKCORE_API void Crowd(Vocal V, int Voices, Spot Where, int Id, float Level);
	/** Fills Frames stereo frames, interleaved left and right. */
	SHORTSTACKCORE_API void RenderStereo(float* Out, int Frames);
	struct Meter
	{
		float BedDb = -120.0f;
		float TalkDb = -120.0f;
		float ChipsDb = -120.0f;
		float CallsDb = -120.0f;
		float PaDb = -120.0f;
		float RailDb = -120.0f;
		int Voiced = 0;
	};
	SHORTSTACKCORE_API Meter Measure() const;

private:
	uint32_t State = 1;
};
} // namespace audio
} // namespace ss
