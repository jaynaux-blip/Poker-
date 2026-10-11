#pragma once

#include "ShortStack/Audio/Voice.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ss
{
namespace audio
{
/**
 * The card room in stereo (next_segment_spec §4.4, §4.9), in room centimetres: the tables' chips and cards, the walla
 * that follows the night's mood, the rail, the bar, the desk and the cage, the slot floor through the doorway and the
 * PA, positioned around the listener with distance (1/r from 1 m, air and bodies darkening it), head shadow (behind:
 * -3 dB and a 6 kHz low-pass), a reverb of RT60 0.7 s, muffling and ducking. 48 kHz, interleaved stereo.
 *
 * Not thread-safe: one per stream, every call on one thread (the game thread, or a render thread that owns it).
 * Coefficients are computed per syllable and per event, never per sample (the fix for CardRoomAmbience.h:62-63).
 * What lives under 4 kHz (the room's talkers and their murmur, the rail's vocals, the walla, the air, the slot floor and
 * the reverb) runs at 8 kHz, clear lines and the PA at 16 kHz, chips, cards and claps at 48 kHz; the slow buses are
 * lifted to 48 kHz by polyphase filters. Any render size gives the same samples. Every random draw comes from the seed
 * and the calls made, so the same calls give the same stream (N5). About 0.04 ms of a 60 fps frame on average; state
 * about 62 KB with 6 talkers.
 *
 * Layers (each a SetMix level): Bed = the air handling, the slot floor, the bar, the desk and the cage; Talk = the
 * walla, the room's own talkers, murmured lines and vocals away from the rail; Chips = the tables' chips and cards;
 * Calls = clear lines (dealers); Rail = vocals and claps at the rail or the bar, and lines said there; the PA has its
 * own level and ducks the room by itself.
 *
 * Bodies: audio-synth (CardRoom.cpp).
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
		Pa, // no place: the ceiling's speakers (Say only)
	};
	/**
	 * A place in the room. Tables: Id is the table's id, Players how many sit there; give the hero's own table
	 * Players = 0, so its Foley stays with the table actor (its place still serves Say and Crowd). Terrace: Open means
	 * the doors are open (the wind comes in).
	 */
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
	SHORTSTACKCORE_API ~CardRoom();
	SHORTSTACKCORE_API CardRoom(CardRoom&& Other) noexcept;
	SHORTSTACKCORE_API CardRoom& operator=(CardRoom&& Other) noexcept;
	CardRoom(const CardRoom&) = delete;
	CardRoom& operator=(const CardRoom&) = delete;

	/** Cheap when the list hasn't changed (the host may call it every frame). The 4 nearest open tables are voiced. */
	SHORTSTACKCORE_API void SetEmitters(const std::vector<Emitter>& Emitters);
	/** The listener in room centimetres; Yaw in degrees, Unreal's (0 faces +X, 90 faces +Y). */
	SHORTSTACKCORE_API void SetListener(float X, float Y, float Yaw);
	/** Levels in dB, eased 1.5 s (room::MixFor's Mix). Crowd: how full the room is, 0..1 (how many talk at once). */
	SHORTSTACKCORE_API void SetMix(float Bed, float Talk, float Chips, float Calls, float Rail, float Crowd);
	/** The whole room through a low-pass at CutoffHz (20000: open) and Db (Focus, tunnel hearing, the terrace's glass). */
	SHORTSTACKCORE_API void SetMuffle(float CutoffHz, float Db);
	/** The room's events run on this clock (Focus's dilation, a fast-forward), clamped to 0.45 .. 1.5. Speech never. */
	SHORTSTACKCORE_API void SetPace(float EventClock);
	/**
	 * Ducks the room for Seconds: Talk by Db, the bed and chips by half; 60 ms in, 500 ms out. Like Say and Crowd it lands
	 * on the next 5 ms block the room renders (at most 5 ms after the samples already handed out). For a line the host
	 * plays itself (a one-shot at your table), held by the stream's queued depth so the two line up.
	 */
	SHORTSTACKCORE_API void Duck(float Db, float Seconds);
	/**
	 * An engine hand at a table, played out over the next Seconds: the shuffle, the pitch, the action, the push. A table
	 * with Players 0 (yours) plays nothing here: its table actor has the Foley.
	 */
	SHORTSTACKCORE_API void TableHand(int Id, float Seconds, bool AllIn, bool Called, float PotBb);
	/**
	 * A line spoken in the room from the next 5 ms block, at a spot (a table's Id, or Spot::Pa). Murmur: talk (Talk layer);
	 * otherwise a clear call (Calls layer); at the rail, the Rail layer. A line from a table within 3 m of the listener
	 * ducks the room (bed -3, talk -6, chips -2 dB; 60 ms in, 500 ms out) but not itself; the PA ducks the room's talk
	 * itself. With the voice budget at 0 (the fallback) a murmured line is only a soft breath under its caption.
	 */
	SHORTSTACKCORE_API void Say(const SpeechPlan& Plan, const Timbre& T, Spot Where, int Id, float Effort, bool Murmur);
	/**
	 * A crowd's sound from a spot: Voices people (the rail's bank voices up to 8 of them); Level 0..1, linear. At the rail
	 * or the bar it's the Rail layer, at a table the Talk layer; Spot::Pa means the whole room, all around the listener.
	 */
	SHORTSTACKCORE_API void Crowd(Vocal V, int Voices, Spot Where, int Id, float Level);
	/** The room's own talkers (6 by default, at most 12). 0 is the fallback: walla noise and non-word vocals only. */
	SHORTSTACKCORE_API void SetVoiceBudget(int Talkers);
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
		float OutDb = -120.0f; // the whole stream
		int Talkers = 0;       // formant voices speaking (the room's talkers, lines, the rail's bank)
		int Tables = 0;        // tables voiced
		int Bytes = 0;         // the room's memory, heap included
	};
	/** Each layer's level as heard (RMS dBFS over about the last half second, before muffling) and the voices in use. */
	SHORTSTACKCORE_API Meter Measure() const;

private:
	struct Impl;
	std::unique_ptr<Impl> P;
};
} // namespace audio
} // namespace ss
