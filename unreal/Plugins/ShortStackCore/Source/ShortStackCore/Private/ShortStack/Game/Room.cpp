#include "ShortStack/Game/Room.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// The room's one voice (next_segment_spec §4). core-room.
//
// Everything the director decides is a hash of the night's seed and ids (the tick, a table, a name), never a draw from
// a stream, so a resumed night decides exactly what the uninterrupted one did; what it must remember (what's been said,
// who spoke when, the talkers' running urge to talk) is in Write's text. Time is engine time: the tick at a nominal 45 s
// a hand. Talk runs on integrate-and-fire: each hand a talker's urge grows by their rate (lines an hour at Live pace by
// Sociability, times the Table talk setting), and they speak when it passes a hashed threshold, so rates hold exactly
// over a night whatever the caps delay.

namespace ss
{
namespace room
{
namespace room_detail
{
constexpr int HandSeconds = 45; // a hand's nominal engine time (N5)
constexpr int TableGap = 10;    // the table: one line per 10 s ...
constexpr int TableMinute = 4;  // ... and four a minute
constexpr int SpeakerGap = 45;  // each speaker 45 s apart ...
constexpr int SmallGap = 60;    // ... 60 for small talk
constexpr int NeedleTicks = 4;  // one needle per needler per 4 hands
constexpr int PaGap = 30;       // the routine PA: one line per 30 s
constexpr int UrgeLine = 1000;  // a line's worth of urge (milli-lines)
constexpr int MaxShown = 84;    // a caption's two lines (the copy lint's table-talk limit)

enum Milestone : int
{
	SaidStart,
	SaidDesk,
	SaidTen,
	SaidLastCall,
	SaidRegClosed,
	SaidAway,
	SaidH4H,
	SaidBubble,
	SaidTwoTables,
	SaidFinal,
	SaidIntros,
	SaidHeadsUp,
	SaidChampion,
	SaidCourtesy,
	SaidOnBubble, // after the rest: a night kept before it reads with it unsaid
};

uint32_t Fnv(const std::string& S, uint32_t H = 2166136261u)
{
	for (const char Ch : S)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
	}
	// A final avalanche, so nearby keys spread.
	H ^= H >> 15;
	H *= 0x2c1b3c6du;
	H ^= H >> 12;
	H *= 0x297a2d39u;
	H ^= H >> 15;
	return H;
}

float Q(double X)
{
	return static_cast<float>(std::round(X * 1000.0) / 1000.0);
}

float Jitter(const std::string& Name, const char* What, float Spread)
{
	const uint32_t H = Fnv(std::string(What) + ":" + Name);
	return (static_cast<float>(H % 2001u) / 1000.0f - 1.0f) * Spread;
}

float SocialOf(int Style)
{
	switch (static_cast<Archetype>(Style))
	{
	case Archetype::Fish: return 0.6f;
	case Archetype::Station: return 0.55f;
	case Archetype::Nit: return 0.25f;
	case Archetype::Tag: return 0.35f;
	case Archetype::Lag: return 0.6f;
	case Archetype::Maniac: return 0.7f;
	case Archetype::Reg: return 0.3f;
	case Archetype::Crusher: return 0.2f;
	}
	return 0.35f;
}

float AggroOf(int Style)
{
	switch (static_cast<Archetype>(Style))
	{
	case Archetype::Fish: return 0.35f;
	case Archetype::Station: return 0.25f;
	case Archetype::Nit: return 0.15f;
	case Archetype::Tag: return 0.45f;
	case Archetype::Lag: return 0.75f;
	case Archetype::Maniac: return 0.9f;
	case Archetype::Reg: return 0.55f;
	case Archetype::Crusher: return 0.7f;
	}
	return 0.5f;
}

struct CastFacts
{
	const char* Name;
	float Aggro;
	world::Identity Is;
	int Age;
};

const CastFacts* CastFor(const std::string& Name)
{
	static const CastFacts All[] = {
		{"Sal", 0.2f, world::Identity::LiveRegular, 71},
		{"Big Lou", 0.45f, world::Identity::Recreational, 54},
		{"Twitch", 0.85f, world::Identity::Recreational, 26},
		{"Mei", 0.55f, world::Identity::DisciplinedPro, 34},
		{"Mrs. Park", 0.15f, world::Identity::LiveRegular, 68},
		{"Rick", 0.75f, world::Identity::WealthyAmateur, 47},
		{"Dre", 0.4f, world::Identity::Recreational, 31},
		{"gh0stfold", 0.7f, world::Identity::OnlineGrinder, 24},
	};
	for (const CastFacts& C : All)
	{
		if (Name == C.Name)
		{
			return &C;
		}
	}
	return nullptr;
}

/** How someone the room knows only by a face label feels about the player (VoicesFor; no world to ask). */
void FromLabel(const std::string& Label, life::LiveVoice& V)
{
	if (Label == "Rival")
	{
		V.Rivalry = 0.55f;
		V.Familiarity = 0.4f;
	}
	else if (Label == "Holds a grudge")
	{
		V.Rivalry = 0.4f;
		V.Familiarity = 0.3f;
	}
	else if (Label == "Respects you")
	{
		V.Respect = 0.55f;
		V.Familiarity = 0.3f;
	}
	else if (Label == "Trusts you")
	{
		V.Respect = 0.2f;
		V.Familiarity = 0.35f;
	}
	else if (Label == "Knows you well")
	{
		V.Familiarity = 0.6f;
	}
	else if (Label == "Knows you")
	{
		V.Familiarity = 0.3f;
	}
	else if (Label == "Has met you")
	{
		V.Familiarity = 0.15f;
	}
}

/** Lines an hour at Live pace (§4.5), by Sociability, inside each band. */
double LinesPerHour(float Social)
{
	const double S = std::clamp(static_cast<double>(Social), 0.0, 1.0);
	if (S < 0.25)
	{
		return 1.0 + 1.0 * S / 0.25;
	}
	if (S < 0.5)
	{
		return 3.0 + 2.0 * (S - 0.25) / 0.25;
	}
	if (S < 0.75)
	{
		return 6.0 + 3.0 * (S - 0.5) / 0.25;
	}
	return 10.0 + 4.0 * std::min(1.0, (S - 0.75) / 0.25);
}

size_t Characters(const std::string& Utf8)
{
	size_t N = 0;
	for (const char Ch : Utf8)
	{
		N += (static_cast<unsigned char>(Ch) & 0xC0u) != 0x80u ? 1u : 0u;
	}
	return N;
}

double SpeechSeconds(const std::string& Text, double Cps)
{
	return static_cast<double>(Characters(Text)) / Cps + 0.4;
}

/**
 * An announcement in caption-sized parts: whole sentences, as many as fit in PaPart characters (a caption box's two
 * rows at 150% at 16:9 and at 2550x1220). A sentence ends at ". ", "! " or "? " before a capital or a digit, never
 * after an initial ("Maria G. ").
 */
constexpr size_t PaPart = 96;

std::vector<std::string> SplitAnnouncement(const std::string& Text)
{
	std::vector<std::string> Sentences;
	size_t Start = 0;
	for (size_t I = 0; I + 2 < Text.size(); ++I)
	{
		const char Ch = Text[I];
		const char Next = Text[I + 2];
		if ((Ch != '.' && Ch != '!' && Ch != '?') || Text[I + 1] != ' ' || !((Next >= 'A' && Next <= 'Z') || (Next >= '0' && Next <= '9')))
		{
			continue;
		}
		const bool Initial = Ch == '.' && I >= 1 && Text[I - 1] >= 'A' && Text[I - 1] <= 'Z' && (I == 1 || Text[I - 2] == ' ' || Text[I - 2] == '.');
		if (Initial)
		{
			continue;
		}
		Sentences.push_back(Text.substr(Start, I + 1 - Start));
		Start = I + 2;
	}
	Sentences.push_back(Text.substr(Start));
	std::vector<std::string> Parts;
	for (const std::string& S : Sentences)
	{
		if (!Parts.empty() && Characters(Parts.back()) + 1 + Characters(S) <= PaPart)
		{
			Parts.back() += " " + S;
		}
		else
		{
			Parts.push_back(S);
		}
	}
	return Parts;
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return S;
}

/** "Forty-something", "A hundred-something", "Fifteen thousand", as someone at the rail rounds a stack. */
std::string Rough(Chips Shown)
{
	const long long K = static_cast<long long>(Shown / 1000);
	if (K < 10)
	{
		return "Under ten thousand";
	}
	if (K < 20)
	{
		return talk::Capitalized(talk::NumberWords(K)) + " thousand";
	}
	if (K < 100)
	{
		return talk::Capitalized(talk::NumberWords(K / 10 * 10)) + "-something";
	}
	if (K < 200)
	{
		return "A hundred-something";
	}
	if (K < 1000)
	{
		return talk::Capitalized(talk::NumberWords(K / 100)) + " hundred-something";
	}
	return "Over a million";
}

// ---- Write/Read: "room1|key=value|..." with %-escaped values; maps as k:v,k:v.

std::string Esc(const std::string& S)
{
	static const char* Hex = "0123456789ABCDEF";
	std::string Out;
	for (const char Ch : S)
	{
		const unsigned char U = static_cast<unsigned char>(Ch);
		if (U < 0x20 || Ch == '%' || Ch == '|' || Ch == '=' || Ch == ',' || Ch == ':' || Ch == '~')
		{
			Out += '%';
			Out += Hex[U >> 4];
			Out += Hex[U & 15];
		}
		else
		{
			Out += Ch;
		}
	}
	return Out;
}

std::string Unesc(const std::string& S)
{
	std::string Out;
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (S[I] == '%' && I + 2 < S.size())
		{
			const std::string Hex = S.substr(I + 1, 2);
			Out += static_cast<char>(std::strtol(Hex.c_str(), nullptr, 16));
			I += 2;
		}
		else
		{
			Out += S[I];
		}
	}
	return Out;
}

std::vector<std::string> Split(const std::string& S, char By)
{
	std::vector<std::string> Out;
	if (S.empty())
	{
		return Out;
	}
	size_t A = 0;
	while (true)
	{
		const size_t B = S.find(By, A);
		Out.push_back(S.substr(A, B == std::string::npos ? std::string::npos : B - A));
		if (B == std::string::npos)
		{
			break;
		}
		A = B + 1;
	}
	return Out;
}

std::string MapText(const std::map<std::string, int>& M)
{
	std::string Out;
	for (const auto& Kv : M)
	{
		Out += (Out.empty() ? "" : ",") + Esc(Kv.first) + ":" + std::to_string(Kv.second);
	}
	return Out;
}

std::map<std::string, int> MapFrom(const std::string& S)
{
	std::map<std::string, int> Out;
	for (const std::string& Item : Split(S, ','))
	{
		const size_t C = Item.rfind(':');
		if (C != std::string::npos)
		{
			Out[Unesc(Item.substr(0, C))] = std::atoi(Item.substr(C + 1).c_str());
		}
	}
	return Out;
}

bool IsHeroName(const std::string& S)
{
	return S == HeroId;
}

/** Levels to the next break (1: it comes after this one); 99 with no breaks. */
int LevelsToBreak(const Tournament& T)
{
	const int Every = std::max(1, T.Spec.BreakEvery);
	return T.Spec.BreakEvery > 0 ? (T.LevelIndex / Every + 1) * Every - T.LevelIndex : 99;
}

/**
 * The dealer's answer to a table's question (Moment::Ask's Topic: "average", "left" or "break"): the call, with what it
 * says added to Vars (the room's).
 */
std::string AskAnswer(const Tournament& T, const std::string& Topic, std::map<std::string, std::string>& Vars)
{
	if (Topic == "average")
	{
		Vars["Average"] = talk::Count(static_cast<long long>(std::lround(T.AverageStack() / 1000.0)) * 1000);
		return "average";
	}
	if (Topic == "left")
	{
		return T.InTheMoney() ? "left.itm" : "left";
	}
	const int ToBreak = LevelsToBreak(T);
	Vars["LevelsCap"] = talk::Capitalized(talk::NumberWords(ToBreak));
	return ToBreak <= 1 ? "break.next" : "break.levels";
}
} // namespace room_detail

using namespace room_detail;

std::vector<std::string> Announcement(const std::string& Text)
{
	return SplitAnnouncement(Text);
}

// ------------------------------------------------------------------ moods and the mix

const char* MoodName(Mood M)
{
	switch (M)
	{
	case Mood::Arrival: return "Arrival";
	case Mood::Early: return "Early";
	case Mood::Running: return "Running";
	case Mood::Break: return "Break";
	case Mood::MoneyClose: return "MoneyClose";
	case Mood::HandForHand: return "HandForHand";
	case Mood::Burst: return "Burst";
	case Mood::LastTables: return "LastTables";
	case Mood::FinalTable: return "FinalTable";
	case Mood::HeadsUp: return "HeadsUp";
	case Mood::Champion: return "Champion";
	case Mood::After: return "After";
	}
	return "";
}

namespace room_detail
{
Mood MoodWith(const Tournament& T, bool OnBreak, bool Dealt, int LateLevels)
{
	if (T.bFinished || T.Tables.empty())
	{
		return Mood::After;
	}
	if (!Dealt)
	{
		return Mood::Arrival;
	}
	if (OnBreak)
	{
		return Mood::Break;
	}
	if (T.Remaining <= 2)
	{
		return Mood::HeadsUp;
	}
	if (T.Tables.size() == 1)
	{
		return Mood::FinalTable;
	}
	if (T.HandForHand())
	{
		return Mood::HandForHand;
	}
	if (T.Tables.size() <= 2)
	{
		return Mood::LastTables;
	}
	if (!T.InTheMoney() && T.Remaining <= T.PaidPlaces() + 8)
	{
		return Mood::MoneyClose;
	}
	return T.LevelIndex < LateLevels ? Mood::Early : Mood::Running;
}
} // namespace room_detail

Mood MoodOf(const Tournament& T, bool OnBreak, bool Dealt)
{
	const live::Occurrence O = live::FindOccurrence(T.Spec.Id);
	return room_detail::MoodWith(T, OnBreak, Dealt, O.Valid() ? O.T->LateRegLevels : 3);
}

Mix MixFor(Mood M, float OpenShare)
{
	// §4.3: bed, talk, chips, calls, rail (dB; -60 is off).
	Mix X;
	switch (M)
	{
	case Mood::Arrival: X = {0.0f, 2.0f, -6.0f, 0.0f, -60.0f, 1.0f}; break;
	case Mood::Early: X = {0.0f, 0.0f, 0.0f, 0.0f, -60.0f, 1.0f}; break;
	case Mood::Running: X = {0.0f, -1.0f, 0.0f, 0.0f, -60.0f, 1.0f}; break;
	case Mood::Break: X = {1.0f, 4.0f, -20.0f, -60.0f, -60.0f, 1.0f}; break;
	case Mood::MoneyClose: X = {0.0f, -4.0f, 2.0f, 3.0f, -6.0f, 1.0f}; break;
	case Mood::HandForHand: X = {-2.0f, -10.0f, 3.0f, 4.0f, 0.0f, 1.0f}; break;
	case Mood::Burst: X = {0.0f, 6.0f, 0.0f, 0.0f, 0.0f, 1.0f}; break;
	case Mood::LastTables: X = {-2.0f, -6.0f, -2.0f, 0.0f, 0.0f, 1.0f}; break;
	case Mood::FinalTable: X = {-6.0f, -12.0f, -6.0f, -8.0f, 4.0f, 1.0f}; break;
	case Mood::HeadsUp: X = {-8.0f, -14.0f, -8.0f, -8.0f, 6.0f, 1.0f}; break;
	case Mood::Champion: X = {-8.0f, 6.0f, -12.0f, -60.0f, 10.0f, 1.0f}; break;
	case Mood::After: X = {-8.0f, -10.0f, -12.0f, -12.0f, -60.0f, 1.0f}; break;
	}
	// The crowd: talk follows the number of talkers, and talkers raise their voices with the room (Lombard).
	const double Share = std::clamp(static_cast<double>(OpenShare), 0.02, 1.0);
	X.Crowd = static_cast<float>(std::clamp(static_cast<double>(OpenShare), 0.0, 1.0));
	const double Crowd = 10.0 * std::log10(Share);
	const double Talk = static_cast<double>(X.Talk) + Crowd;
	const double Lombard = 0.5 * (Talk - (-1.0)); // relative to a full room, Running
	X.Talk = static_cast<float>(std::max(-40.0, Talk + Lombard));
	return X;
}

// ------------------------------------------------------------------ voices

life::LiveVoice VoiceForStranger(const std::string& Name, int Style)
{
	life::LiveVoice V;
	V.Name = Name;
	V.Style = Style;
	V.Social = Q(std::clamp(SocialOf(Style) + Jitter(Name, "social", 0.1f), 0.0f, 1.0f));
	V.Aggro = Q(std::clamp(AggroOf(Style) + Jitter(Name, "aggro", 0.08f), 0.0f, 1.0f));
	V.Grit = Q(std::clamp(0.5f + Jitter(Name, "grit", 0.15f), 0.0f, 1.0f));
	V.Mood = static_cast<int>(world::Momentum::Normal);
	V.Is = static_cast<int>(world::Identity::Recreational);
	V.Age = 25 + static_cast<int>(Fnv("age:" + Name) % 40u);
	return V;
}

std::vector<life::LiveVoice> VoicesFor(const life::LiveEntry& E)
{
	std::vector<life::LiveVoice> Out;
	std::map<std::string, std::string> Labels;
	for (const life::LiveFace& F : E.Faces)
	{
		Labels[F.Name] = F.Label;
	}
	for (const std::pair<std::string, int>& R : E.Roster)
	{
		life::LiveVoice V = VoiceForStranger(R.first, R.second);
		if (const live::RoomLocal* L = live::FindRoomLocal(R.first))
		{
			V.Is = static_cast<int>(world::Identity::LiveRegular);
			V.Age = L->Age;
		}
		if (const CastFacts* C = CastFor(R.first))
		{
			V.Social = Q(talk::CastChatter(R.first));
			V.Aggro = Q(C->Aggro);
			V.Is = static_cast<int>(C->Is);
			V.Age = C->Age;
		}
		const auto It = Labels.find(R.first);
		if (It != Labels.end())
		{
			FromLabel(It->second, V);
		}
		Out.push_back(V);
	}
	return Out;
}

life::LiveVoice VoiceOf(const world::World& W, int Npc, const std::string& Hero)
{
	life::LiveVoice V;
	const world::Npc* N = W.Get(Npc);
	if (!N || N->Name.empty() || N->Name == Hero)
	{
		return V;
	}
	const int Day = net::DayOf(W.Clock());
	V.Name = N->Name;
	V.Style = static_cast<int>(live::PlayStyle(*N));
	for (const live::CastMember& Cm : live::EmbercrestCast())
	{
		if (N->Name == Cm.Name)
		{
			V.Style = static_cast<int>(Cm.Type);
		}
	}
	V.Social = Q(talk::IsCast(N->Name) ? talk::CastChatter(N->Name) : N->TraitOf(world::Trait::Sociability));
	V.Aggro = Q(N->TraitOf(world::Trait::Aggro));
	V.Grit = Q(N->TraitOf(world::Trait::Grit));
	V.Mood = static_cast<int>(N->Mood);
	V.Is = static_cast<int>(N->Is);
	V.Age = N->Age(Day);
	V.Pro = N->Pro;
	V.Staked = N->Backer != -1;
	if (const world::Bond* B = W.BondWith(Npc))
	{
		V.Rivalry = Q(B->Rivalry);
		V.Familiarity = Q(B->Familiarity);
		V.Respect = Q(B->Respect);
	}
	// What they can claim: their own results of the last ten days (the world's record of them), newest first.
	std::vector<talk::Result> Story;
	for (const world::Finish& F : N->Recent)
	{
		if (F.Day > Day || F.Day < Day - 10 || Story.size() >= 3)
		{
			continue;
		}
		talk::Result R;
		R.Day = F.Day;
		R.Event = F.Event;
		R.Place = F.Place;
		R.Entries = F.Entries;
		R.Prize = F.Prize;
		R.Online = F.Where == static_cast<int>(world::Venue::Online);
		Story.push_back(R);
	}
	V.Story = talk::StoryText(Story);
	// Who they know (never who backs whom: that's private).
	for (const world::Tie& T : N->Ties)
	{
		if (T.Kind == world::TieKind::Backer || T.Kind == world::TieKind::Backed || V.Ties.size() >= 6)
		{
			continue;
		}
		const world::Npc* O = W.Get(T.Other);
		if (O && !O->Name.empty())
		{
			V.Ties.push_back({O->Name, static_cast<int>(T.Kind)});
		}
	}
	return V;
}

// ------------------------------------------------------------------ a fresh start's books (update 2 §3.3 F9)

namespace room_detail
{
/** When one of the room's events is over: its schedule's hours, as the world plays it (WorldCalendar: start + hours). */
double FreshEnds(const live::Occurrence& O)
{
	return O.Start + O.T->Hours * 60.0;
}

/** The room's events from Day From to Day To that end after Opened, in the order they end (start order on a tie). */
std::vector<live::Occurrence> FreshRoom(double Opened, int From, int To)
{
	std::vector<live::Occurrence> Out;
	for (int D = From; D <= To; ++D)
	{
		for (const live::Occurrence& O : live::Occurrences(D))
		{
			if (FreshEnds(O) > Opened)
			{
				Out.push_back(O);
			}
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const live::Occurrence& A, const live::Occurrence& B) { return FreshEnds(A) < FreshEnds(B); });
	return Out;
}

/** How a first-month line names one of the room's events: its short name, but "Embercrest Sunday" ("the first Sunday in
 *  the books" would be a day). */
std::string FreshEvent(const live::Occurrence& O)
{
	const std::string Name = O.T->Name;
	return std::string(O.T->Key) == "sunday" ? Name.substr(0, Name.find(" $")) : std::string(O.T->Short);
}

/** The world's news of a family's first ("Embercrest Nightly", "Embercrest Noon Deepstack": WorldSim's BooksFamily). */
std::string FreshFamily(const live::Occurrence& O)
{
	const std::string Name = std::string(O.T->Name).substr(0, std::string(O.T->Name).rfind(" $"));
	return Name.find("Embercrest") == std::string::npos ? "Embercrest " + Name : Name;
}

/** How the table says one of the world's people in a first-month line: by name when the room knows them by it (or it's a
 *  whole name), else the first name in their handle; "" when there's only a handle (handles are never said). */
std::string FreshName(const std::string& Name)
{
	if (Name.empty())
	{
		return std::string();
	}
	if (talk::KnownByName(Name) || Name.find(' ') != std::string::npos)
	{
		return Name;
	}
	return talk::FirstNameIn(Name);
}

bool FreshEvening(const live::Occurrence& O)
{
	return O.T->StartMinute >= 17 * 60;
}

/** When one of the room's events still to come is, from Now: "on right now", "tonight", "tomorrow", "Saturday night". */
std::string FreshWhen(const live::Occurrence& O, double Now)
{
	const int Day = net::DayOf(Now);
	if (O.Start <= Now)
	{
		return "on right now";
	}
	if (O.Day <= Day)
	{
		return FreshEvening(O) ? "tonight" : "later today";
	}
	if (O.Day == Day + 1)
	{
		return FreshEvening(O) ? "tomorrow night" : "tomorrow";
	}
	return std::string(net::WeekdayName(O.Day, true)) + (FreshEvening(O) ? " night" : "");
}

/** When one of the room's events now over was played, from Now: "yesterday", "last night", "Saturday night", "opening week". */
std::string FreshThen(const live::Occurrence& O, double Now, double Opened)
{
	const int Ago = net::DayOf(Now) - O.Day;
	if (Ago <= 0)
	{
		return FreshEvening(O) ? "tonight" : "today";
	}
	if (Ago == 1)
	{
		return FreshEvening(O) ? "last night" : "yesterday";
	}
	if (Ago < 7)
	{
		return std::string(net::WeekdayName(O.Day, true)) + (FreshEvening(O) ? " night" : "");
	}
	if (O.Day - net::DayOf(Opened) < 7)
	{
		return "opening week";
	}
	return Ago < 14 ? "last week" : "a while back";
}

/** One thing a regular could honestly say about a fresh world's books right now (FreshTopics). */
struct FreshTalk
{
	std::string Key;   // what it spends, once a night ("next:bigstack")
	std::string Topic; // the pool ("fresh.next"); "...self" or "...you" when it's about the speaker or the player
	std::map<std::string, std::string> Vars;
	std::string Who;   // whom it's about (the world's name for them)
	bool Hero = false; // it's about the player
};

/**
 * What's true of a fresh world's books at Now, the night of Tonight (whose winner nobody knows while the room still talks),
 * from what B can show: each topic only while it's true (U5). Decided: a first the books show, or over and not known to
 * be called off (its winner then unknown, so a first after it is never claimed). Empty in an established world and after
 * the books' first FreshBooksDays.
 */
std::vector<FreshTalk> FreshTopics(const Books& B, const live::Occurrence& Tonight, double Now)
{
	std::vector<FreshTalk> Out;
	const int Fresh = net::DayOf(Now) - net::DayOf(B.Opened);
	if (!B.Blank || Now < B.Opened || Fresh < 0 || Fresh >= FreshBooksDays)
	{
		return Out;
	}
	const auto Cancelled = [&B](const std::string& Id) { return std::find(B.Cancelled.begin(), B.Cancelled.end(), Id) != B.Cancelled.end(); };
	const auto FirstOf = [&B](const std::string& Id) -> const Books::First* {
		for (const Books::First& F : B.Firsts)
		{
			if (F.Id == Id)
			{
				return &F;
			}
		}
		return nullptr;
	};
	const auto Decided = [&](const live::Occurrence& O) {
		if (Tonight.Valid() && O.Id == Tonight.Id)
		{
			return false;
		}
		return FirstOf(O.Id) != nullptr || (FreshEnds(O) <= Now && !Cancelled(O.Id));
	};
	const std::vector<live::Occurrence> Room = FreshRoom(B.Opened, net::DayOf(B.Opened) - 1, net::DayOf(Now) + 6);
	std::set<std::string> Won; // the families with a winner in the books by now
	for (const live::Occurrence& O : Room)
	{
		if (Decided(O))
		{
			Won.insert(O.T->Key);
		}
	}
	const std::string TonightKey = Tonight.Valid() ? std::string(Tonight.T->Key) : std::string();
	// Nothing the room has played since the books opened has a winner yet: the board by the desk is blank.
	if (Won.empty())
	{
		Out.push_back({"wall", "fresh.wall", {}, std::string(), false});
	}
	// Tonight's is the first of its kind in the books.
	if (Tonight.Valid() && FreshEnds(Tonight) > B.Opened && !Won.count(TonightKey))
	{
		Out.push_back({"tonight", "fresh.tonight", {{"FreshEvent", FreshEvent(Tonight)}}, std::string(), false});
	}
	// The first of each of the room's other events still to come: its next one, being dealt now or within the week.
	std::set<std::string> Looked;
	for (const live::Occurrence& O : Room)
	{
		const std::string Key = O.T->Key;
		if (Key == TonightKey || FreshEnds(O) <= Now || Cancelled(O.Id) || Looked.count(Key))
		{
			continue;
		}
		Looked.insert(Key); // the family's next one decides it
		if (!Won.count(Key))
		{
			Out.push_back({"next:" + Key, Key == "nightly" ? "fresh.next.nightly" : "fresh.next", {{"FreshEvent", FreshEvent(O)}, {"FreshWhen", FreshWhen(O, Now)}},
				std::string(), false});
		}
	}
	// The firsts the books can show, oldest first: the first name on the board, then each family's. A first is claimed only
	// when every event that ended before it (of its family; for the board, of any) is one the room knows was called off.
	const auto Clear = [&](const live::Occurrence& Of, bool Family) {
		for (const live::Occurrence& O : Room)
		{
			if (FreshEnds(O) > FreshEnds(Of))
			{
				break;
			}
			if (O.Id != Of.Id && (!Family || std::string(O.T->Key) == std::string(Of.T->Key)) && !Cancelled(O.Id))
			{
				return false;
			}
		}
		return true;
	};
	const auto Claim = [&](const std::string& Key, const char* Topic, const live::Occurrence& O, const Books::First& F) {
		const std::string Then = FreshThen(O, Now, B.Opened);
		FreshTalk T{Key, Topic, {{"FreshEvent", FreshEvent(O)}, {"FreshThen", Then}, {"FreshThenCap", talk::Capitalized(Then)}}, F.Hero ? std::string() : F.Winner, F.Hero};
		if (!F.Hero)
		{
			T.Vars["FreshWho"] = FreshName(F.Winner);
		}
		Out.push_back(T);
	};
	const auto Sayable = [](const Books::First& F) { return F.Hero || !FreshName(F.Winner).empty(); };
	std::string BoardKey;
	for (const Books::First& F : B.Firsts)
	{
		const live::Occurrence O = live::FindOccurrence(F.Id);
		if (!O.Valid() || (Tonight.Valid() && O.Id == Tonight.Id) || FreshEnds(O) <= B.Opened)
		{
			continue;
		}
		if (Clear(O, false) && Sayable(F))
		{
			Claim("wallfirst", "fresh.wallfirst", O, F);
			BoardKey = O.T->Key;
		}
		break; // the board's first is the oldest first the books show, whoever it was
	}
	for (const Books::First& F : B.Firsts)
	{
		const live::Occurrence O = live::FindOccurrence(F.Id);
		if (O.Valid() && !(Tonight.Valid() && O.Id == Tonight.Id) && FreshEnds(O) > B.Opened && std::string(O.T->Key) != BoardKey && Clear(O, true) && Sayable(F))
		{
			Claim("first:" + std::string(O.T->Key), "fresh.first", O, F);
		}
	}
	// More than one win up the hill already (at least: there may be more since the world was read). Never "nobody's won
	// twice": the world doesn't keep the wins of the strangers it doesn't follow, so the room can't know it.
	if (B.HeroWins >= 2)
	{
		Out.push_back({"twice:you", "fresh.twice", {}, std::string(), true});
	}
	if (B.MostWins >= 2 && !FreshName(B.Most).empty())
	{
		Out.push_back({"twice", "fresh.twice", {{"FreshWho", FreshName(B.Most)}}, B.Most, false});
	}
	// The books' first week.
	if (Fresh < 7)
	{
		Out.push_back({"books", "fresh.books", {}, std::string(), false});
	}
	return Out;
}
} // namespace room_detail

Books BooksOf(const world::World& W, const std::vector<life::LiveEntry>& Entries)
{
	Books B;
	if (!W.Ready() || W.Began() != world::Start::Blank)
	{
		return B;
	}
	B.Blank = true;
	B.Opened = W.StartedAt();
	B.Known = W.Clock();
	// The room's wins in the books (a fresh world's records start at nothing, so every win it keeps is one of them).
	B.HeroWins = W.HeroRoom().Wins;
	for (const world::Npc& N : W.People())
	{
		if (N.Room.Wins > B.MostWins)
		{
			B.MostWins = N.Room.Wins;
			B.Most = N.Name;
		}
	}
	const auto EntryOf = [&Entries](const std::string& Id) -> const life::LiveEntry* {
		for (const life::LiveEntry& E : Entries)
		{
			if (E.Id == Id)
			{
				return &E;
			}
		}
		return nullptr;
	};
	// Each family's first decided event, in the order they ended. One the world can't tell about yet (not over by its
	// clock, or the player's own, still being played) holds its family: nothing after it is a first anyone can claim.
	std::set<std::string> Held;
	for (const live::Occurrence& O : FreshRoom(B.Opened, net::DayOf(B.Opened) - 1, net::DayOf(B.Known) + 1))
	{
		const std::string Key = O.T->Key;
		if (W.CancellationOf(O.Id))
		{
			B.Cancelled.push_back(O.Id);
			continue;
		}
		if (Held.count(Key))
		{
			continue;
		}
		Held.insert(Key);
		Books::First F;
		F.Id = O.Id;
		bool Decided = false;
		const life::LiveEntry* E = EntryOf(O.Id);
		if (const net::EventResult* R = W.ResultOf(O.Id))
		{
			Decided = !R->FinalTable.empty();
			for (const net::Placing& P : R->FinalTable)
			{
				if (P.Place == 1)
				{
					const world::Npc* N = P.Player >= 0 ? W.Get(P.Player) : nullptr;
					F.Hero = P.Player == -1;
					F.Winner = F.Hero ? std::string() : N ? N->Name : P.Name;
				}
			}
		}
		else if (E && E->State == life::LiveEntry::Finished)
		{
			// The player's own night, settled in the room before the world hears of it.
			Decided = true;
			F.Hero = E->Place == 1;
		}
		else if (FreshEnds(O) <= B.Known && !(E && E->State == life::LiveEntry::Registered))
		{
			// Over by the world's clock and no result kept (it keeps a few days'): its first, as the world's own news has it.
			Decided = true;
			const std::string Family = FreshFamily(O);
			for (const world::WorldEvent& Ev : W.Events())
			{
				if (Ev.Kind == world::EventKind::FirstWinner && Ev.What == Family && std::fabs(Ev.At - FreshEnds(O)) < 1.0)
				{
					const world::Npc* N = W.Get(Ev.Npc);
					F.Hero = Ev.Npc == -1;
					F.Winner = F.Hero || !N ? std::string() : N->Name;
				}
			}
		}
		if (Decided)
		{
			B.Firsts.push_back(F);
		}
	}
	return B;
}

Books BooksOfText(const std::string& WorldText, const std::vector<life::LiveEntry>& Entries, double Now)
{
	const std::string& Text = WorldText;
	std::vector<std::string> Fields;
	const auto ReadLine = [&Text, &Fields](world::World& W, size_t From, size_t End) {
		Fields.clear();
		for (size_t Cell = From, K = From; K <= End; ++K)
		{
			if (K == End || Text[K] == '\t')
			{
				Fields.emplace_back(Text, Cell, K - Cell);
				Cell = K + 1;
			}
		}
		W.Read(Fields);
	};
	// The head first: how the world began, and when its books opened.
	const std::string HeadTag = "world\thead\t";
	size_t Head = Text.compare(0, HeadTag.size(), HeadTag) == 0 ? 0 : Text.find("\n" + HeadTag);
	if (Head == std::string::npos)
	{
		return Books();
	}
	Head += Head == 0 ? 0 : 1;
	world::World Peek;
	ReadLine(Peek, Head, std::min(Text.find('\n', Head), Text.size()));
	if (!Peek.Ready() || Peek.Began() != world::Start::Blank || net::DayOf(Now) - net::DayOf(Peek.StartedAt()) >= FreshBooksDays)
	{
		return Books();
	}
	// Then what the books read: the people (their names and their records up the hill), results, cancellations, the news.
	world::World W;
	static const char* const Needed[] = {"head", "npc", "ghost", "room", "res", "cancelled", "log"};
	for (size_t From = 0; From < Text.size();)
	{
		const size_t End = std::min(Text.find('\n', From), Text.size());
		const size_t Kind = From + 6; // after "world\t"
		const size_t Tab = Kind < End ? Text.find('\t', Kind) : std::string::npos;
		if (Text.compare(From, 6, "world\t") == 0 && Tab != std::string::npos && Tab < End)
		{
			for (const char* N : Needed)
			{
				if (Text.compare(Kind, Tab - Kind, N) == 0)
				{
					ReadLine(W, From, End);
					break;
				}
			}
		}
		From = End + 1;
	}
	return BooksOf(W, Entries);
}

// ------------------------------------------------------------------ the director

Director::Director(const life::LiveEntry& E, const live::Occurrence& O, const std::string& Seed, const std::string& HeroFirstName)
{
	HeroFullName = HeroFirstName;
	EntryId = E.Id;
	SeedText = Seed;
	SeedHash = Fnv(Seed);
	HeroFirst = HeroFirstName;
	Occ = O;
	if (O.Valid())
	{
		EventShort = O.T->Short;
		EventName = talk::SpokenEvent(O.T->Name) == O.T->Short ? std::string(O.T->Name) : talk::SpokenEvent(O.T->Name);
		const size_t Price = EventName.find(" $");
		if (Price != std::string::npos)
		{
			EventName = EventName.substr(0, Price);
		}
		TemplateKey = O.T->Key;
	}
	else
	{
		EventShort = "tournament";
		EventName = "tournament";
	}
	const std::vector<life::LiveVoice> Field = E.Voices.empty() ? VoicesFor(E) : E.Voices;
	for (const life::LiveVoice& V : Field)
	{
		Voices[V.Name] = V;
	}
	for (const life::LiveFace& F : E.Faces)
	{
		FaceLabel[F.Name] = F.Label;
		FaceLine[F.Name] = F.Line;
	}
	for (const std::pair<std::string, int>& R : E.Roster)
	{
		RosterNames.insert(R.first);
	}
	for (const std::pair<std::string, std::string>& B : E.Badges)
	{
		// The field's rows by name; the player's own (badge::HeroFieldPack, under the save's name) is the one that isn't
		// in the roster, like their patch rows.
		(RosterNames.count(B.first) ? BadgePack[B.first] : HeroBadges) = B.second;
	}
	for (const std::pair<std::string, std::string>& P : E.Patches)
	{
		(RosterNames.count(P.first) ? FieldPatches : HeroPatches).push_back(P);
	}
}

bool Director::Flagship() const
{
	return TemplateKey == "sunday" || TemplateKey == "bigstack";
}

bool Director::Sunday() const
{
	return Occ.Valid() && net::Weekday(Occ.Day) == 6;
}

uint32_t Director::Hash(const std::string& What) const
{
	return Fnv(What, SeedHash);
}

double Director::Unit(const std::string& What) const
{
	return static_cast<double>(Hash(What)) / 4294967296.0;
}

double Director::TickWorld(const Tournament& T) const
{
	return Occ.Start + (T.ClockMinutes() - T.Spec.StartClock);
}

int Director::HeroTable(const Tournament& T) const
{
	return T.Hero().Busted ? -1 : T.Hero().TableId;
}

int Director::SeatedTable(const Tournament& T) const
{
	return T.Hero().Busted && HeroOutTick != T.Tick ? -1 : T.Hero().TableId;
}

Director::Seen Director::Find(const Tournament& T, const std::string& IdOrName) const
{
	Seen S;
	int Idx = T.PlayerIndex(IdOrName);
	if (Idx < 0)
	{
		for (size_t I = 0; I < T.Players.size(); ++I)
		{
			if (T.Players[I].Name == IdOrName)
			{
				Idx = static_cast<int>(I);
				break;
			}
		}
	}
	if (Idx < 0)
	{
		return S;
	}
	const TPlayer& P = T.Players[static_cast<size_t>(Idx)];
	S.Id = P.Id;
	S.Name = P.Name;
	S.Table = P.TableId;
	S.Seat = P.Seat;
	S.Hero = P.IsHero;
	S.Caption = P.IsHero ? HeroFirst : talk::CaptionName(P.Id, P.Name, P.Seat);
	return S;
}

std::vector<Director::Seen> Director::AtTable(const Tournament& T, int TableId) const
{
	std::vector<Seen> Seats;
	const auto It = T.Tables.find(TableId);
	if (It == T.Tables.end())
	{
		return Seats;
	}
	for (const int Idx : It->second.Seats)
	{
		if (Idx < 0)
		{
			continue;
		}
		const TPlayer& P = T.Players[static_cast<size_t>(Idx)];
		Seen S;
		S.Id = P.Id;
		S.Name = P.Name;
		S.Table = P.TableId;
		S.Seat = P.Seat;
		S.Hero = P.IsHero;
		S.Caption = P.IsHero ? HeroFirst : talk::CaptionName(P.Id, P.Name, P.Seat);
		Seats.push_back(S);
	}
	return Seats;
}

const life::LiveVoice& Director::VoiceFor(const Tournament& T, const Seen& P) const
{
	const auto It = Voices.find(P.Name);
	if (It != Voices.end())
	{
		return It->second;
	}
	const auto Made = Strangers.find(P.Name);
	if (Made != Strangers.end())
	{
		return Made->second;
	}
	int Style = static_cast<int>(Archetype::Tag);
	const int Idx = T.PlayerIndex(P.Id);
	if (Idx >= 0 && T.Players[static_cast<size_t>(Idx)].HasProfile)
	{
		Style = static_cast<int>(T.Players[static_cast<size_t>(Idx)].Prof.Type);
	}
	life::LiveVoice V = VoiceForStranger(P.Name, Style);
	const auto Face = FaceLabel.find(P.Name);
	if (Face != FaceLabel.end())
	{
		FromLabel(Face->second, V);
	}
	// The same every time it's made (a hash of the name and how they play), so keeping it changes nothing.
	return Strangers.emplace(P.Name, std::move(V)).first->second;
}

std::string Director::Spoken(const Seen& P) const
{
	// What the floor reads from a registration: the player's name as they gave it; the room's people by name; someone
	// known only by a handle, the first name in it, or nothing.
	if (P.Hero)
	{
		return HeroFullName.empty() ? HeroFirst : HeroFullName;
	}
	if (talk::KnownByName(P.Name))
	{
		return P.Name;
	}
	return talk::FirstNameIn(P.Name);
}

std::map<std::string, std::string> Director::RoomVars(const Tournament& T) const
{
	std::map<std::string, std::string> V;
	V["Short"] = EventShort;
	V["Event"] = EventName;
	V["Players"] = std::to_string(T.Spec.Entrants);
	V["Pool"] = talk::Dollars(T.PrizePoolCents);
	V["Paid"] = std::to_string(T.PaidPlaces());
	V["PlacesAre"] = T.PaidPlaces() == 1 ? "place is" : "places are"; // "{Paid} {PlacesAre} paid": "1 place is paid", "14 places are paid"
	V["MinCash"] = T.Payouts.empty() ? std::string() : talk::Dollars((T.Payouts.back() + 50) / 100 * 100); // the floor rounds to the dollar
	V["Left"] = std::to_string(T.Remaining);
	V["Hero"] = HeroFirst;
	return V;
}

void Director::Sync(const Tournament& T)
{
	const int Now = T.Tick * HandSeconds;
	if (T.Tick != LastTick || Now > Engine)
	{
		Engine = std::max(Engine, Now);
		TableClock = std::max(TableClock, Engine);
		LastTick = T.Tick;
	}
}

void Director::UpdateMood(const Tournament& T, double World)
{
	Mood M = room_detail::MoodWith(T, OnBreak, HandsSeen > 0 || T.Tick > 0, Occ.Valid() ? Occ.T->LateRegLevels : 3);
	if (T.bFinished)
	{
		M = ChampionUntil < 0.0 || World < ChampionUntil ? Mood::Champion : Mood::After;
	}
	else if (BurstTick >= 0 && T.Tick <= BurstTick && !OnBreak && M != Mood::FinalTable && M != Mood::HeadsUp)
	{
		M = Mood::Burst;
	}
	else if (T.InTheMoney() && !Has(SaidBubble) && Current == Mood::HandForHand && !OnBreak)
	{
		M = Mood::HandForHand; // the hush holds until the floor calls the bubble (its event comes after the hand's news)
	}
	Current = M;
}

void Director::Snapshot(const Tournament& T)
{
	int Table = HeroTable(T);
	if (Table < 0 && T.Tables.size() == 1)
	{
		Table = T.Tables.begin()->first; // on the rail at the final table: the feature table
	}
	Stacks.clear();
	StacksTable = Table;
	for (const Seen& P : AtTable(T, Table))
	{
		const int Idx = T.PlayerIndex(P.Id);
		Stacks[P.Name] = Idx >= 0 ? T.Players[static_cast<size_t>(Idx)].Stack : 0;
	}
}

// ------------------------------------------------------------------ cues

void Director::Pa(const Tournament& T, const std::string& Name, std::map<std::string, std::string> Vars, int Priority, bool Chime, const std::string& Speaker,
	bool Routine)
{
	// Milestones and the chains (priority 2 and 3) are never dropped; the routine PA waits for its slot (one line per
	// 30 s of engine time) or isn't said. The break's own clock (five minutes left) runs while no hand is dealt, so it
	// isn't routine: it would never find a slot after the break's chain.
	if (Routine && Priority <= 1 && Engine < PaFree)
	{
		return;
	}
	const talk::Said S = talk::Floor(Name, Vars, Hash("pa:" + Name + ":" + std::to_string(T.Tick)));
	if (S.Text.empty())
	{
		return;
	}
	if (Routine)
	{
		PaFree = Engine + PaGap;
	}
	Announce(S, Priority, Chime, Speaker);
}

void Director::PaTogether(const Tournament& T, const FloorCalls& Calls, int Priority)
{
	// As the floor says it in one breath (how far from the money, then hand for hand): one chime and one "Players,". The
	// key names each call's template, so nothing keyed to one of them (a recorded line) is played for the whole. Each
	// call's line is the one Pa would pick for it, and the routine PA waits after it as after Pa's.
	static const std::string Players = "Players, ";
	talk::Said All;
	for (const auto& Call : Calls)
	{
		const talk::Said S = talk::Floor(Call.first, Call.second, Hash("pa:" + Call.first + ":" + std::to_string(T.Tick)));
		if (S.Text.empty())
		{
			continue;
		}
		std::string Text = S.Text;
		if (!All.Text.empty() && Text.size() > Players.size() && Text.compare(0, Players.size(), Players) == 0)
		{
			Text = talk::Capitalized(Text.substr(Players.size()));
		}
		All.Text += (All.Text.empty() ? "" : " ") + Text;
		All.Key += (All.Key.empty() ? "" : "+") + S.Key;
	}
	if (All.Text.empty())
	{
		return;
	}
	PaFree = Engine + PaGap;
	Announce(All, Priority, true, "Floor");
}

void Director::Announce(const talk::Said& S, int Priority, bool Chime, const std::string& Speaker)
{
	// One announcement, a caption-sized part at a time: the chime, then the floor reads straight through.
	const std::vector<std::string> Parts = SplitAnnouncement(S.Text);
	for (size_t I = 0; I < Parts.size(); ++I)
	{
		Cue C;
		C.What = Kind::Pa;
		C.Where = From::Pa;
		C.Speaker = Speaker;
		C.Key = S.Key;
		C.Text = Parts[I];
		C.Part = static_cast<int>(I);
		C.Priority = Priority;
		C.Chime = Chime && I == 0;
		C.Effort = 0.7f;
		C.Delay = PaCursor;
		C.Tick = LastTick;
		PaCursor += (C.Chime ? 1.1 : 0.0) + SpeechSeconds(Parts[I], 14.0) + (I + 1 < Parts.size() ? 0.2 : 0.6);
		Out.push_back(C);
	}
}

void Director::Call(const Tournament& T, const std::string& Name, std::map<std::string, std::string> Vars, int TableId, float Effort, const std::string& Speaker,
	bool AtHero)
{
	const int Here = SeatedTable(T);
	AtHero = AtHero || TableId == Here;
	const bool DeeDeals = Sunday() && (AtHero || T.Tables.size() == 1);
	talk::Context C;
	C.Vars = std::move(Vars);
	if (DeeDeals)
	{
		C.Vars["Dealer"] = "Dee";
	}
	C.Salt = Hash("call:" + Name + ":" + std::to_string(T.Tick) + ":" + std::to_string(TableId));
	const talk::Said S = talk::Dealer(Name, C);
	if (S.Text.empty())
	{
		return;
	}
	Cue Q2;
	Q2.What = Kind::Call;
	Q2.Where = AtHero ? From::HeroTable : T.Tables.size() == 1 ? From::FeatureTable : From::Table;
	Q2.TableId = TableId;
	Q2.Speaker = DeeDeals ? std::string("Dee") : Speaker;
	Q2.Key = S.Key;
	Q2.Text = S.Text;
	Q2.Effort = Effort;
	Q2.Tick = LastTick;
	if (AtHero)
	{
		Q2.Delay = TalkCursor;
		TalkCursor += SpeechSeconds(S.Text, 15.0) + 0.5;
	}
	else
	{
		// Other tables' dealers call when their own hands end: spread over the shuffle, never in step.
		Q2.Delay = static_cast<double>(Hash("calldelay:" + std::to_string(TableId) + ":" + std::to_string(T.Tick)) % 40u) / 10.0;
	}
	Out.push_back(Q2);
}

bool Director::Say(const Tournament& T, const Seen& P, talk::Moment M, std::map<std::string, std::string> Vars, bool Small, bool ToHero, const life::LiveVoice* To,
	bool Reaction)
{
	if (P.Name.empty() || P.Hero)
	{
		return false;
	}
	// The table: one line per 10 s and four a minute, inside this hand's 45 s; each speaker 45 s apart (60 for small talk).
	int At = TableClock;
	if (!TableLines.empty())
	{
		At = std::max(At, TableLines.back() + TableGap);
	}
	if (At >= Engine + HandSeconds)
	{
		return false;
	}
	int InMinute = 0;
	for (const int L : TableLines)
	{
		InMinute += L > At - 60 ? 1 : 0;
	}
	if (InMinute >= TableMinute)
	{
		return false;
	}
	const auto Spoke = SpokeAt.find(P.Name);
	if (Spoke != SpokeAt.end() && At - Spoke->second < SpeakerGap)
	{
		return false;
	}
	const auto Smalled = SmallAt.find(P.Name);
	if (Small && Smalled != SmallAt.end() && At - Smalled->second < SmallGap)
	{
		return false;
	}
	const life::LiveVoice& Self = VoiceFor(T, P);
	talk::Context C;
	C.Self = &Self;
	C.To = To;
	C.Vars = std::move(Vars);
	C.Vars["Venue"] = "embercrest";
	C.Vars["Hero"] = HeroFirst;
	C.Vars["Day"] = std::to_string(Occ.Day);
	if (Sunday() && (P.Table == SeatedTable(T) || T.Tables.size() == 1))
	{
		C.Vars["Dealer"] = "Dee";
	}
	C.Salt = Hash("say:" + P.Name + ":" + talk::MomentName(M) + ":" + std::to_string(T.Tick));
	C.Told = &Told;
	const talk::Said S = talk::Line(M, C);
	if (S.Text.empty() || Characters(S.Text) > static_cast<size_t>(MaxShown))
	{
		return false;
	}
	TableLines.push_back(At);
	while (TableLines.size() > static_cast<size_t>(TableMinute))
	{
		TableLines.erase(TableLines.begin());
	}
	SpokeAt[P.Name] = At;
	if (Small)
	{
		SmallAt[P.Name] = At;
	}
	TableClock = At + TableGap;
	talk::Remember(Told, S);
	Cue Line;
	Line.What = Kind::Line;
	const int Here = SeatedTable(T);
	Line.Where = P.Table == Here ? From::HeroTable : From::FeatureTable;
	Line.TableId = P.Table;
	Line.Seat = P.Seat;
	Line.Who = P.Id;
	Line.Speaker = P.Caption;
	Line.Key = S.Key;
	Line.Text = S.Text;
	Line.Topic = S.Topic;
	Line.ToHero = ToHero;
	Line.Tick = LastTick;
	Line.Delay = TalkCursor + (Reaction ? 0.0 : 0.6);
	TalkCursor = Line.Delay + SpeechSeconds(S.Text, 15.0) + 0.7;
	Out.push_back(Line);
	return true;
}

void Director::Vocal(audio::Vocal V, From Where, int TableId, int Count, const std::string& Sound, double Delay)
{
	Cue C;
	C.What = Kind::Vocal;
	C.Where = Where;
	C.TableId = TableId;
	C.Noise = V;
	C.Voices = std::max(1, Count);
	C.Sound = Sound;
	C.Delay = Delay;
	C.Tick = LastTick;
	C.Effort = 0.8f;
	Out.push_back(C);
}

void Director::RailSay(const Tournament& T, int Moment, const std::string& Text, const std::string& Key)
{
	// The backers holding the player's action who are out of tonight's field (stake::RailFor, from the host): one of
	// them says it, in their own voice (stake::RailLine; never a chip count or a card), and nobody twice in one breath.
	static const stake::Book NoBook;
	for (size_t I = 0; I < Rail.size(); ++I)
	{
		const RailBacker& B = Rail[(I + Hash("rail:" + std::to_string(T.Tick)) % Rail.size()) % Rail.size()];
		if (RailSpoke.count(B.Name))
		{
			continue;
		}
		std::string Line = Text;
		// A backer's pool for a moment ("Well played. Now breathe." on every double-up): what they've said from it tonight
		// is in Told (across checkpoints) under Pool, and the last of it under Last.
		const std::string Pool = "rail:" + B.Name + ":" + std::to_string(Moment) + ":";
		const std::string Last = "raillast:" + B.Name + ":" + std::to_string(Moment) + ":";
		auto KeyOf = [this](const std::string& L) { return std::to_string(Hash("said:" + L) % 1000000u); };
		bool Dealt = false;
		if (Line.empty())
		{
			// The pool's lines in turn from the night's pick, for one not said tonight. Said through, the pool is dealt
			// fresh, never with the line said last coming up first.
			const uint32_t Salt = Hash("railline:" + B.Name + ":" + std::to_string(T.Tick) + ":" + std::to_string(Moment));
			const auto LastIt = Told.lower_bound(Last);
			const std::string LastKey = LastIt != Told.end() && LastIt->rfind(Last, 0) == 0 ? LastIt->substr(Last.size()) : std::string();
			std::string First;
			std::string NotLast;
			for (uint32_t Try = 0; Try < 8u; ++Try)
			{
				const std::string Drawn = stake::RailLine(NoBook, EntryId, B.Name, Moment, Salt + Try);
				if (Drawn.empty())
				{
					break;
				}
				First = First.empty() ? Drawn : First;
				if (!Told.count(Pool + KeyOf(Drawn)))
				{
					Line = Drawn;
					break;
				}
				NotLast = NotLast.empty() && KeyOf(Drawn) != LastKey ? Drawn : NotLast;
			}
			Dealt = Line.empty();
			Line = !Line.empty() ? Line : !NotLast.empty() ? NotLast : First;
		}
		if (Line.empty() || Characters(Line) > static_cast<size_t>(MaxShown))
		{
			continue;
		}
		if (Text.empty())
		{
			auto Erase = [this](const std::string& Prefix) {
				auto It = Told.lower_bound(Prefix);
				while (It != Told.end() && It->rfind(Prefix, 0) == 0)
				{
					It = Told.erase(It);
				}
			};
			if (Dealt)
			{
				Erase(Pool);
			}
			Erase(Last);
			Told.insert(Pool + KeyOf(Line));
			Told.insert(Last + KeyOf(Line));
		}
		RailSpoke.insert(B.Name);
		Cue C;
		C.What = Moment == static_cast<int>(talk::Moment::Break) ? Kind::Phone : Kind::Line;
		C.Where = From::Rail;
		C.Who = B.Name;
		C.Speaker = B.Name;
		C.Key = !Key.empty() ? Key : Moment == static_cast<int>(talk::Moment::Break) ? "stake.break" : "stake.rail";
		C.Text = Line;
		C.ToHero = true;
		C.Tick = LastTick;
		C.Delay = TalkCursor + 0.4;
		TalkCursor = C.Delay + SpeechSeconds(Line, 15.0) + 0.5;
		Out.push_back(C);
		return;
	}
}

std::vector<Cue> Director::Take()
{
	std::vector<Cue> Got;
	Got.swap(Out);
	std::stable_sort(Got.begin(), Got.end(), [](const Cue& A, const Cue& B) { return A.Delay < B.Delay; });
	PaCursor = 0.0;
	TalkCursor = 0.0;
	ReactionEnd = 0.0;
	return Got;
}

bool Director::Shouter(const Tournament& T, std::string& Who, std::string& WhoId) const
{
	for (const RailBacker& B : Rail)
	{
		if (!RailSpoke.count(B.Name))
		{
			Who = B.Name;
			WhoId = B.Name;
			return true;
		}
	}
	for (const TPlayer& P : T.Players)
	{
		const auto Known = Voices.find(P.Name);
		if (P.Busted && Known != Voices.end() && (Known->second.Familiarity >= 0.3f || Known->second.Respect >= 0.3f) && Unit("stayed:" + P.Name) <= 0.6 &&
			!RailSpoke.count(P.Id))
		{
			Who = talk::CaptionName(P.Id, P.Name, P.Seat);
			WhoId = P.Id;
			return true;
		}
	}
	return false;
}

void Director::Shout(const Tournament& T, const std::string& Topic, int TableId, double Delay)
{
	// The player's name from the rail: their backer, or someone out tonight who knows them.
	std::string Who;
	std::string WhoId;
	if (!Shouter(T, Who, WhoId))
	{
		return;
	}
	talk::Context C;
	C.Vars["Topic"] = Topic;
	C.Vars["Name"] = HeroFirst;
	C.Salt = Hash("comeon:" + Topic + ":" + std::to_string(T.Tick));
	C.Told = &Told;
	const talk::Said S = talk::Line(talk::Moment::Rail, C);
	if (S.Text.empty())
	{
		return;
	}
	talk::Remember(Told, S);
	RailSpoke.insert(WhoId);
	Cue Cry;
	Cry.What = Kind::Line;
	Cry.Where = From::Rail;
	Cry.TableId = TableId;
	Cry.Who = WhoId;
	Cry.Speaker = Who;
	Cry.Key = S.Key;
	Cry.Text = S.Text;
	Cry.ToHero = true;
	Cry.Effort = 0.95f;
	Cry.Tick = LastTick;
	Cry.Delay = std::max(Delay, TalkCursor);
	TalkCursor = Cry.Delay + SpeechSeconds(S.Text, 15.0) + 0.3;
	Out.push_back(Cry);
}

void Director::AllInCalled(const Tournament& T, int TableId, bool HeroIn)
{
	Sync(T);
	RailSpoke.clear();
	const int Here = SeatedTable(T);
	int Table = TableId != 0 ? TableId : Here;
	if (Table <= 0 && T.Tables.size() == 1)
	{
		Table = T.Tables.begin()->first;
	}
	if (Table <= 0)
	{
		return;
	}
	// Somebody's watching: the rail at the final table, or near the money the crowd at the nearby tables.
	const bool Watchers = Table != Here || T.Tables.size() == 1 || Current == Mood::FinalTable || Current == Mood::HeadsUp;
	if (!Watchers && Current != Mood::MoneyClose && Current != Mood::HandForHand)
	{
		return;
	}
	LeanTick = T.Tick;
	LeanTable = Table;
	Vocal(audio::Vocal::Ooh, Watchers ? From::Rail : From::Table, Table, Watchers ? 12 : 6, "[The rail leans in: ooh.]", 0.0);
	if (HeroIn && Watchers)
	{
		Shout(T, "allin", Table, 0.3);
	}
}

void Director::SetRail(const std::vector<RailBacker>& Backers)
{
	Rail = Backers;
	if (Rail.size() > 2)
	{
		Rail.resize(2);
	}
}

bool Director::Prompt(const Tournament& T, talk::Moment M, int Seat)
{
	if (M == talk::Moment::Rail)
	{
		const size_t Before = Out.size();
		RailSpoke.clear();
		RailSay(T, static_cast<int>(talk::Moment::Rail));
		return Out.size() > Before;
	}
	const int Here = SeatedTable(T);
	if (Here < 0)
	{
		return false;
	}
	// A question to the dealer is asked about what the night would ask it (Chatter: the average or the count near the money,
	// the break when it's close), else the average; the dealer answers it, as at the table.
	std::string Topic;
	if (M == talk::Moment::Ask && (Current == Mood::MoneyClose || Current == Mood::LastTables))
	{
		Topic = Unit("asktopic:" + std::to_string(T.Tick)) < 0.5 ? "average" : "left";
	}
	else if (M == talk::Moment::Ask)
	{
		Topic = LevelsToBreak(T) <= 2 ? "break" : "average";
	}
	for (const Seen& P : AtTable(T, Here))
	{
		if (P.Hero || (Seat >= 0 && P.Seat != Seat))
		{
			continue;
		}
		const life::LiveVoice& V = VoiceFor(T, P);
		talk::Context C;
		C.Self = &V;
		C.Vars["Venue"] = "embercrest";
		C.Vars["Hero"] = HeroFirst;
		C.Vars["Day"] = std::to_string(Occ.Day);
		if (!Topic.empty())
		{
			C.Vars["Topic"] = Topic;
		}
		if (!HeroPatches.empty() && M == talk::Moment::Noticed)
		{
			C.Vars["Topic"] = HeroPatches.front().second;
			if (const sponsor::Brand* B = sponsor::FindBrand(HeroPatches.front().second))
			{
				C.Vars["Brand"] = B->Short;
			}
		}
		C.Salt = Hash("prompt:" + P.Name + ":" + talk::MomentName(M) + ":" + std::to_string(T.Tick));
		C.Told = &Told;
		const talk::Said S = talk::Line(M, C);
		if (S.Text.empty())
		{
			continue;
		}
		Cue Line;
		Line.What = Kind::Line;
		Line.Where = From::HeroTable;
		Line.TableId = Here;
		Line.Seat = P.Seat;
		Line.Who = P.Id;
		Line.Speaker = P.Caption;
		Line.Key = S.Key;
		Line.Text = S.Text;
		Line.Topic = S.Topic;
		Line.ToHero = M == talk::Moment::Needle || M == talk::Moment::Tank || M == talk::Moment::ShowedBluff || M == talk::Moment::Noticed;
		Line.Tick = LastTick;
		Out.push_back(Line);
		if (M == talk::Moment::Ask)
		{
			// The dealer's answer once the question's been asked (spaced as Say spaces a line from the next).
			TalkCursor = std::max(TalkCursor, Line.Delay + SpeechSeconds(S.Text, 15.0) + 0.7);
			std::map<std::string, std::string> D = RoomVars(T);
			const std::string Answer = AskAnswer(T, Topic, D);
			Call(T, Answer, D, Here, 0.6f);
		}
		return true;
	}
	return false;
}

// ------------------------------------------------------------------ the engine's news

void Director::Bust(const Tournament& T, const TEvent& E)
{
	const Seen P = Find(T, E.Id);
	const int Here = SeatedTable(T);
	if (E.IsHero)
	{
		return; // the backer's word comes after the floor's (Events)
	}
	if (E.TableId == Here)
	{
		// At the player's table: the dealer first ("Seat open.", a hand up; or the cage for a cash), then their goodbye
		// (unless they've just said their piece).
		std::map<std::string, std::string> D;
		if (E.PrizeCents > 0)
		{
			D["Place"] = talk::Ordinal(E.Place);
			D["Amount"] = talk::Dollars(E.PrizeCents);
			Call(T, "cage", D, E.TableId, 0.6f);
		}
		else
		{
			Call(T, "seatopen", D, E.TableId, 0.6f);
		}
		std::map<std::string, std::string> Vars;
		if (E.PrizeCents > 0)
		{
			Vars["Topic"] = "paid";
		}
		Say(T, P, talk::Moment::Goodbye, Vars, false, false, nullptr, true);
	}
	else if (T.Tables.size() > 1 && T.Tables.count(E.TableId))
	{
		// Elsewhere: that table's dealer, heard nearby (and across the room at hand for hand).
		std::map<std::string, std::string> D;
		D["Table"] = std::to_string(E.TableId);
		D["TableWord"] = talk::NumberWords(E.TableId);
		Call(T, "table.seatopen", D, E.TableId, Current == Mood::HandForHand || T.HandForHand() ? 0.9f : 0.6f);
	}
	if (T.Tables.size() == 1)
	{
		Vocal(audio::Vocal::Applause, From::Rail, E.TableId, 12, "[Polite applause.]", 0.8);
	}
}

void Director::Events(const Tournament& T, const std::vector<TEvent>& E, double World)
{
	Sync(T);
	WorldNow = World;
	RailSpoke.clear();
	// The floor speaks once the hand's own reactions have landed (the champion after the rail's ooh, not over it).
	PaCursor = std::max(PaCursor, ReactionEnd);
	bool FinalNow = false;
	for (const TEvent& Ev : E)
	{
		if (Ev.Type == TEventType::Bust && Ev.IsHero)
		{
			HeroOutTick = T.Tick;
		}
		FinalNow = FinalNow || Ev.Type == TEventType::FinalTable;
	}
	if (OnBreak && !E.empty())
	{
		bool Starting = false;
		for (const TEvent& Ev : E)
		{
			Starting = Starting || Ev.Type == TEventType::Break;
		}
		if (!Starting)
		{
			BreakOver(T);
		}
	}
	bool BreakNow = false;
	std::string BubbleName;
	for (const TEvent& Ev : E)
	{
		BreakNow = BreakNow || Ev.Type == TEventType::Break;
		if (Ev.Type == TEventType::Bubble)
		{
			BubbleName = Ev.Name;
		}
	}
	const int Paid = T.PaidPlaces();
	const int Here = HeroTable(T);
	for (const TEvent& Ev : E)
	{
		switch (Ev.Type)
		{
		case TEventType::Bust:
		{
			Bust(T, Ev);
			// The floor calls busts close to the money and in it (BackRoomLive.cpp's gate, without its dice): not the
			// player's (the cage does that), not the bubble's (its own line), not at the player's own table.
			const bool TheBubble = !BubbleName.empty() && Ev.Name == BubbleName;
			if (!Ev.IsHero && !TheBubble && Ev.Place > 1 && Ev.Place <= Paid + 4 && Ev.TableId != T.Hero().TableId)
			{
				const Seen P = Find(T, Ev.Id);
				std::map<std::string, std::string> V;
				V["Table"] = std::to_string(Ev.TableId);
				V["Seat"] = std::to_string(live::SeatLabel(P.Seat));
				V["Place"] = talk::Ordinal(Ev.Place);
				if (Ev.PrizeCents > 0)
				{
					V["Amount"] = talk::Dollars((Ev.PrizeCents + 50) / 100 * 100); // the floor rounds to the dollar
					Pa(T, "bust.paid", V, 1, true);
				}
				else
				{
					Pa(T, "bust", V, 1, true);
				}
			}
			break;
		}
		case TEventType::Level:
			if (Ev.LevelNumber > LevelSaid)
			{
				LevelSaid = Ev.LevelNumber;
				if (!BreakNow)
				{
					std::map<std::string, std::string> V;
					V["Sb"] = talk::Count(Ev.Blinds.Sb);
					V["Bb"] = talk::Count(Ev.Blinds.Bb);
					if (Ev.Blinds.Ante > 0)
					{
						V["Ante"] = talk::Count(Ev.Blinds.Ante);
					}
					Pa(T, Ev.Blinds.Ante > 0 ? "level.ante" : "level", V, 1, true);
					if (Here >= 0 && Unit("blinds:" + std::to_string(Ev.LevelNumber)) < 0.5)
					{
						TalkCursor = std::max(TalkCursor, PaCursor);
						Call(T, "blinds", {}, Here, 0.6f);
					}
					// A backer at the rail reads your stack every other level, the way the rail can see it.
					if (!Rail.empty() && Here >= 0 && LevelSaid - RailRead >= 2)
					{
						RailRead = LevelSaid;
						const Chips Mine = T.Hero().Stack;
						const chipset::Read Look = chipset::ReadStack(Mine, chipset::InPlay(chipset::Room::Embercrest, T.Levels, T.LevelIndex, T.Spec.BreakEvery), false);
						const std::string Words = Rough(Look.Shown > 0 ? Look.Shown : Mine);
						talk::Context C;
						C.Vars["Topic"] = static_cast<double>(Mine) < 0.75 * T.AverageStack() ? "read.short" : "read";
						C.Vars["Rough"] = Words;
						C.Vars["RoughLower"] = Lower(Words);
						C.Salt = Hash("railread:" + std::to_string(LevelSaid));
						C.Told = &Told;
						const talk::Said S = talk::Line(talk::Moment::Rail, C);
						const size_t Before = Out.size();
						RailSay(T, static_cast<int>(talk::Moment::Rail), S.Text, S.Key);
						if (Out.size() > Before)
						{
							talk::Remember(Told, S);
						}
					}
				}
			}
			break;
		case TEventType::TableBroken:
			if (Ev.TableId != Here && (T.Tables.size() <= 4 || Unit("breaking:" + std::to_string(Ev.TableId)) < 0.5))
			{
				std::map<std::string, std::string> V;
				V["Table"] = std::to_string(Ev.TableId);
				Pa(T, "table.breaking", V, 1, false);
			}
			break;
		case TEventType::Moved:
			// At the final table the floor's call carries everyone to the feature table; nobody's told twice.
			if (Ev.IsHero)
			{
				Greet = true;
				if (FinalNow)
				{
					break;
				}
				std::map<std::string, std::string> V;
				V["From"] = std::to_string(Ev.From);
				V["Table"] = std::to_string(Ev.To);
				V["Seat"] = std::to_string(live::SeatLabel(T.Hero().Seat));
				if (HeroUp)
				{
					Pa(T, "moved", V, 2, false); // up from the table when it broke: the floor finds them
				}
				else
				{
					Call(T, "move", V, Ev.From, 0.6f, "Dealer", true); // seated: their dealer tells them
				}
			}
			else if (Here >= 0 && Ev.To == Here && !FinalNow && Has(SaidStart))
			{
				// Someone moved in to the player's table: a face says hello, or the dealer seats them.
				const Seen P = Find(T, Ev.Id);
				const auto Line = FaceLine.find(P.Name);
				if (Line != FaceLine.end() && !Line->second.empty() && Characters(Line->second) <= static_cast<size_t>(MaxShown))
				{
					std::map<std::string, std::string> V;
					V["Greeting"] = Line->second;
					Say(T, P, talk::Moment::SitDown, V, false, true, nullptr, true);
				}
				else if (!P.Id.empty() && Unit("welcome:" + P.Name + ":" + std::to_string(T.Tick)) < 0.6)
				{
					std::map<std::string, std::string> V;
					V["Seat"] = std::to_string(live::SeatLabel(P.Seat));
					Call(T, "welcome", V, Here, 0.6f);
				}
			}
			break;
		case TEventType::HandForHand:
			if (!Has(SaidH4H))
			{
				Mark(SaidH4H);
				// One announcement: how far the money is when the field got here in one hand (on the bubble, at one away),
				// then hand for hand; one chime and one "Players," however many calls it carries.
				// At a lone table there's no other table for its dealer to wait on: the rule without the decks.
				FloorCalls Calls;
				Away(T, Calls);
				Calls.push_back({T.Tables.size() == 1 ? "h4h.lone" : "h4h", RoomVars(T)});
				PaTogether(T, Calls, 3);
				if (Here >= 0)
				{
					TalkCursor = std::max(TalkCursor, PaCursor);
					Call(T, "h4h", {}, Here, 0.6f);
					RailSay(T, static_cast<int>(talk::Moment::Bubble));
				}
			}
			break;
		case TEventType::Bubble:
			if (!Has(SaidBubble))
			{
				Mark(SaidBubble);
				Mark(SaidH4H);
				Mark(SaidAway);
				Mark(SaidOnBubble); // defensive: Away marks it in the money, but Milestones skips Away on a burst that ends the night (one paid)
				BurstTick = T.Tick;
				const Seen B = Find(T, Ev.Name);
				const std::string Name = Spoken(B);
				std::map<std::string, std::string> V = RoomVars(T);
				if (!Name.empty())
				{
					V["Name"] = Name;
					Pa(T, "bubble", V, 3, true);
				}
				else
				{
					V["Table"] = std::to_string(B.Table);
					V["Seat"] = std::to_string(live::SeatLabel(B.Seat));
					Pa(T, "bubble.seat", V, 3, true);
				}
				// The room answers the call, after it (as at the final table): a fixed delay came before it when the floor had
				// already spoken that hand (a paid bust called first).
				Vocal(audio::Vocal::Applause, From::Pa, 0, 40, "[Applause all around.]", PaCursor);
				if (Here >= 0)
				{
					// One or two at the table, the sociable ones, once the floor has said it (and the bubble's name only
					// when the floor said it).
					TalkCursor = std::max(TalkCursor, PaCursor - 0.6);
					int Lines = 0;
					for (const Seen& P : AtTable(T, Here))
					{
						const life::LiveVoice& V2 = VoiceFor(T, P);
						if (Lines >= 2 || P.Hero || V2.Social < 0.5f || Unit("bubbletalk:" + P.Name) > 0.6)
						{
							continue;
						}
						std::map<std::string, std::string> Vars;
						if (!Name.empty())
						{
							Vars["Bubble"] = B.Caption;
						}
						Lines += Say(T, P, talk::Moment::Bubble, Vars, false, false, nullptr, true) ? 1 : 0;
					}
					RailSay(T, static_cast<int>(talk::Moment::InTheMoney));
				}
			}
			break;
		case TEventType::FinalTable:
			if (!Has(SaidFinal))
			{
				Mark(SaidFinal);
				Mark(SaidTwoTables);
				FinalTableCalls(T);
				Vocal(audio::Vocal::Applause, From::Rail, T.Tables.empty() ? 0 : T.Tables.begin()->first, 30, "[Applause from the rail.]", PaCursor);
				if (Here >= 0)
				{
					// The table and the rail answer the floor, after it (and after the introductions).
					TalkCursor = std::max(TalkCursor, PaCursor);
					RailSay(T, static_cast<int>(talk::Moment::FinalTable));
					for (const Seen& P : AtTable(T, Here))
					{
						if (!P.Hero && Unit("finaltalk:" + P.Name) < 0.25 && Say(T, P, talk::Moment::FinalTable, {}, false, false, nullptr, true))
						{
							break;
						}
					}
				}
			}
			break;
		case TEventType::Finished:
			if (!Has(SaidChampion))
			{
				Mark(SaidChampion);
				ChampionUntil = World + 1.5;
				const Seen W = Find(T, Ev.Id);
				const std::string Name = Spoken(W);
				std::map<std::string, std::string> V = RoomVars(T);
				// On Sundays it's Dee on the mic: she's in the box (§1.9 #17).
				const std::string Voice = Sunday() ? "Dee" : "Floor";
				if (!Name.empty())
				{
					V["Name"] = Name;
					Pa(T, W.Hero && Sunday() ? "champion.dee.hero" : "champion", V, 3, true, Voice);
				}
				else
				{
					V["Table"] = std::to_string(W.Table);
					V["Seat"] = std::to_string(live::SeatLabel(W.Seat));
					Pa(T, "champion.seat", V, 3, true, Voice);
				}
				Vocal(audio::Vocal::Cheer, From::Rail, W.Table, 20, "[The rail cheers.]", PaCursor - 0.6);
				Vocal(audio::Vocal::Applause, From::Pa, 0, 40, "[Applause.]", PaCursor);
				if (W.Hero)
				{
					TalkCursor = std::max(TalkCursor, PaCursor);
					RailSay(T, static_cast<int>(talk::Moment::Win));
				}
			}
			break;
		case TEventType::Break:
		{
			OnBreak = true;
			BreakCounting = false;
			BreakLevel = Ev.LevelNumber;
			BreakTick = T.Tick;
			if (Ev.LevelNumber > BreakSaid)
			{
				BreakSaid = Ev.LevelNumber;
				std::map<std::string, std::string> V = RoomVars(T);
				V["Minutes"] = std::to_string(static_cast<int>(std::lround(T.Spec.BreakMinutes)));
				Pa(T, "break", V, 3, true);
				const double AfterBreakCall = PaCursor;
				// When we come back: the level the break leads into, and the chips leaving play (the colour-up).
				const Level& Next = T.CurrentLevel();
				V["Sb"] = talk::Count(Next.Sb);
				V["Bb"] = talk::Count(Next.Bb);
				if (Next.Ante > 0)
				{
					V["Ante"] = talk::Count(Next.Ante);
				}
				Pa(T, Next.Ante > 0 ? "break.next.ante" : "break.next", V, 3, false);
				if (live::IsEmbercrest(T.Spec.Id))
				{
					const int Before = std::max(0, T.LevelIndex - 1);
					const std::vector<chipset::Denom> Was = chipset::InPlay(chipset::Room::Embercrest, T.Levels, Before, T.Spec.BreakEvery);
					const std::vector<chipset::Denom> Now = chipset::InPlay(chipset::Room::Embercrest, T.Levels, T.LevelIndex, T.Spec.BreakEvery);
					std::vector<std::string> Leaving;
					for (const chipset::Denom& D : Was)
					{
						const bool Stays = std::any_of(Now.begin(), Now.end(), [&D](const chipset::Denom& X) { return X.Value == D.Value; });
						if (!Stays)
						{
							Leaving.push_back(D.Value == 100 ? "black hundreds" : D.Value == 500 ? "purple five-hundreds" : D.Value == 1000 ? "yellow thousands" : talk::Count(D.Value) + "s");
						}
					}
					if (!Leaving.empty())
					{
						V["Denoms"] = Leaving.size() == 1 ? Leaving[0] : Leaving[0] + " and the " + Leaving[1];
						Pa(T, "colorup", V, 3, false);
					}
				}
				if (Here >= 0)
				{
					TalkCursor = std::max(TalkCursor, AfterBreakCall);
					Call(T, "break", {}, Here, 0.6f);
					// The table gets up: a bad beat still on someone's mind, or what people say standing up.
					int Said2 = 0;
					for (const Seen& P : AtTable(T, Here))
					{
						if (P.Hero || Said2 >= 2)
						{
							continue;
						}
						const auto Beat = std::find_if(Beats.begin(), Beats.end(), [&P](const std::pair<std::string, std::string>& B) { return B.first == P.Name; });
						if (Beat != Beats.end())
						{
							const std::vector<std::string> F = Split(Beat->second, '|');
							if (F.size() >= 3)
							{
								std::map<std::string, std::string> Vars;
								Vars["Topic"] = "beat";
								Vars["Mine"] = talk::Capitalized(F[0]);
								Vars["MineLower"] = F[0];
								Vars["Theirs"] = talk::Capitalized(F[1]);
								Vars["TheirsLower"] = F[1];
								Vars["Street"] = F[2];
								Said2 += Say(T, P, talk::Moment::Break, Vars, false, false, nullptr, true) ? 1 : 0;
							}
							Beats.erase(Beat);
							continue;
						}
						if (Unit("breaktalk:" + P.Name + ":" + std::to_string(BreakLevel)) < 0.3)
						{
							Said2 += Say(T, P, talk::Moment::Break, {}, false, false, nullptr, true) ? 1 : 0;
						}
					}
					RailSay(T, static_cast<int>(talk::Moment::Break));
				}
			}
			break;
		}
		default:
			break;
		}
	}
	Milestones(T, World);
	// The player's out: their backer at the rail says their piece once the floor has said its own (the champion, the
	// level), not over it.
	const bool HeroOut = std::any_of(E.begin(), E.end(), [](const TEvent& Ev) { return Ev.Type == TEventType::Bust && Ev.IsHero; });
	if (HeroOut)
	{
		TalkCursor = std::max(TalkCursor, PaCursor);
		RailSay(T, static_cast<int>(talk::Moment::Lose));
	}
	UpdateMood(T, World);
	Snapshot(T);
}

void Director::FinalTableCalls(const Tournament& T)
{
	Pa(T, "final", RoomVars(T), 3, true);
	if (T.Tables.empty())
	{
		return;
	}
	const std::vector<Seen> Seats = AtTable(T, T.Tables.begin()->first);
	// One honorific per seat (the badges' Room phrase, the player's Team Embercrest, a sponsor's patch).
	auto HonorOf = [this](const Seen& P, int& Rank) -> std::string {
		Rank = 99;
		if (P.Hero)
		{
			for (const std::pair<std::string, std::string>& Pt : HeroPatches)
			{
				if (Pt.second == "embercrest")
				{
					Rank = 20;
					return "Team Embercrest's own";
				}
			}
		}
		const auto Pack = BadgePack.find(P.Name);
		const std::string Packed = P.Hero ? HeroBadges : Pack != BadgePack.end() ? Pack->second : std::string();
		if (!Packed.empty())
		{
			const badge::Set Set = badge::Unpack(Packed);
			const std::string H = badge::Honorific(Set, badge::Room);
			if (!H.empty())
			{
				const badge::Badge* Top = badge::Top(Set, badge::Room, badge::FrameNone);
				Rank = Top ? static_cast<int>(Top->K) : 12;
				return H;
			}
		}
		for (const std::pair<std::string, std::string>& Pt : P.Hero ? HeroPatches : FieldPatches)
		{
			if (Pt.first != P.Name && !P.Hero)
			{
				continue;
			}
			if (Pt.second == "riverline")
			{
				Rank = 21;
				return "Team RiverLine's";
			}
			if (const sponsor::Brand* B = sponsor::FindBrand(Pt.second))
			{
				Rank = 30;
				return "representing " + std::string(B->Id == "luckypenny" ? "the " : "") + B->Short;
			}
		}
		return std::string();
	};
	auto FormOf = [](const std::string& H) -> std::string {
		const auto Ends = [&H](const std::string& Tail) { return H.size() >= Tail.size() && H.compare(H.size() - Tail.size(), Tail.size(), Tail) == 0; };
		if (Ends("'s") || Ends(" own"))
		{
			return "own";
		}
		if (H.rfind("a ", 0) == 0 || H.rfind("an ", 0) == 0 || H.rfind("representing ", 0) == 0)
		{
			return "article";
		}
		return "title";
	};
	if (Flagship())
	{
		// Seat by seat, with the counts on the plates and one honorific each, 2.5 s apart (§4.7, §11 Q20).
		Pa(T, "intro.open", RoomVars(T), 3, false);
		for (const Seen& P : Seats)
		{
			const int Idx = T.PlayerIndex(P.Id);
			std::map<std::string, std::string> V;
			V["SeatWord"] = talk::NumberWords(live::SeatLabel(P.Seat));
			V["Chips"] = talk::Count(Idx >= 0 ? T.Players[static_cast<size_t>(Idx)].Stack : 0);
			const std::string Name = Spoken(P);
			int Rank = 0;
			const std::string Honor = HonorOf(P, Rank);
			if (Name.empty())
			{
				Pa(T, "intro.seat.nameless", V, 3, false);
			}
			else
			{
				V["Name"] = Name;
				if (Honor.empty())
				{
					Pa(T, "intro.seat", V, 3, false);
				}
				else
				{
					V["Honor"] = Honor;
					Pa(T, "intro.seat." + FormOf(Honor), V, 3, false);
				}
			}
			PaCursor = std::max(PaCursor, Out.empty() ? 0.0 : Out.back().Delay + 2.5);
		}
		Mark(SaidIntros);
		return;
	}
	// Other events: at most one honorific sentence, the room's best.
	int Best = 99;
	const Seen* Who = nullptr;
	std::string Honor;
	for (const Seen& P : Seats)
	{
		int Rank = 0;
		const std::string H = HonorOf(P, Rank);
		if (!H.empty() && !Spoken(P).empty() && Rank < Best)
		{
			Best = Rank;
			Who = &P;
			Honor = H;
		}
	}
	if (Who)
	{
		std::map<std::string, std::string> V;
		V["Name"] = Spoken(*Who);
		V["Honor"] = Honor;
		Pa(T, "final.honor." + FormOf(Honor), V, 3, false);
	}
	Mark(SaidIntros);
}

void Director::Away(const Tournament& T, FloorCalls& Calls)
{
	// Three away from the money (or two, when the field jumps past three in one hand): said once, before hand for hand
	// when they come together (as a floor says it: how far, then the rule). Then on the bubble, once, at one away: the
	// field walked down to it, or jumped there with the rest in one call. A jump straight to one away is said once, as a
	// floor would: on the bubble, what's paid, then the rule (no "one away from the money" and then "on the bubble"). The
	// burst is its own call.
	const int Left = T.Remaining - T.PaidPlaces();
	if (!Has(SaidAway))
	{
		if (T.InTheMoney())
		{
			Mark(SaidAway);
		}
		else if (Left <= 3 && Has(SaidStart))
		{
			Mark(SaidAway);
			std::map<std::string, std::string> V = RoomVars(T);
			if (Left == 1 && !Has(SaidOnBubble))
			{
				Mark(SaidOnBubble);
				Calls.push_back({"away.bubble", V});
			}
			else
			{
				V["Away"] = talk::NumberWords(Left);
				Calls.push_back({"away", V});
			}
		}
	}
	if (!Has(SaidOnBubble))
	{
		if (T.InTheMoney())
		{
			Mark(SaidOnBubble);
		}
		else if (Left == 1 && Has(SaidStart))
		{
			Mark(SaidOnBubble);
			Calls.push_back({"onbubble", RoomVars(T)});
		}
	}
}

void Director::Milestones(const Tournament& T, double World)
{
	(void)World;
	if (T.bFinished)
	{
		return;
	}
	FloorCalls Calls;
	Away(T, Calls);
	if (!Calls.empty())
	{
		PaTogether(T, Calls, 2);
	}
	if (!Has(SaidTwoTables) && T.Tables.size() <= 2)
	{
		Mark(SaidTwoTables);
		if (T.Tables.size() == 2 && Has(SaidStart))
		{
			Pa(T, "two.tables", RoomVars(T), 2, true);
		}
	}
	if (!Has(SaidHeadsUp) && T.Remaining == 2)
	{
		Mark(SaidHeadsUp);
		std::vector<Seen> Two;
		for (const auto& It : T.Tables)
		{
			for (const Seen& P : AtTable(T, It.first))
			{
				Two.push_back(P);
			}
		}
		if (Two.size() == 2)
		{
			std::map<std::string, std::string> V = RoomVars(T);
			const std::string A = Spoken(Two[0]);
			const std::string B = Spoken(Two[1]);
			V["A"] = A.empty() ? "Seat " + std::to_string(live::SeatLabel(Two[0].Seat)) : A;
			V["B"] = B.empty() ? "Seat " + std::to_string(live::SeatLabel(Two[1].Seat)) : B;
			V["AChips"] = talk::Count(T.Players[static_cast<size_t>(T.PlayerIndex(Two[0].Id))].Stack);
			V["BChips"] = talk::Count(T.Players[static_cast<size_t>(T.PlayerIndex(Two[1].Id))].Stack);
			V["First"] = talk::Dollars((T.PrizeFor(1) + 50) / 100 * 100);
			Pa(T, "headsup", V, 3, true);
		}
	}
	// The courtesy call (P2): once on a Friday-to-Sunday night, an hour and a half in, when the room's quiet enough.
	if (!Has(SaidCourtesy))
	{
		const int Wd = Occ.Valid() ? net::Weekday(Occ.Day) : 0;
		if (Wd < 4 || Current == Mood::MoneyClose || Current == Mood::HandForHand || T.InTheMoney() || Unit("courtesy") >= 0.5)
		{
			Mark(SaidCourtesy);
		}
		else if (Has(SaidStart) && TickWorld(T) >= Occ.Start + 90.0 && Engine >= PaFree && T.Tables.size() > 2)
		{
			std::vector<int> Open;
			for (const auto& It : T.Tables)
			{
				if (It.first != HeroTable(T))
				{
					Open.push_back(It.first);
				}
			}
			if (!Open.empty())
			{
				Mark(SaidCourtesy);
				std::map<std::string, std::string> V;
				V["Table"] = std::to_string(Open[Hash("courtesy:table") % Open.size()]);
				Pa(T, "courtesy", V, 0, false);
			}
		}
	}
}

void Director::Rounds(const Tournament& T, const std::vector<TableRound>& R)
{
	Sync(T);
	if (OnBreak)
	{
		BreakOver(T);
	}
	// The player's table, the hand that put them out included (Hand has seen it; nobody there shouts its own number).
	const int Here = SeatedTable(T);
	const bool Close = Current == Mood::MoneyClose || Current == Mood::HandForHand || T.HandForHand() ||
		(!T.InTheMoney() && T.Remaining <= T.PaidPlaces() + 8);
	const bool H4H = T.HandForHand() || Current == Mood::HandForHand;
	bool AnyAllIn = false;
	for (const TableRound& Round : R)
	{
		AnyAllIn = AnyAllIn || Round.AllIn;
		const bool HeroThere = std::find(Round.Out.begin(), Round.Out.end(), std::string(HeroId)) != Round.Out.end();
		if (Round.TableId == Here || HeroThere || T.Tables.size() <= 1)
		{
			continue; // the player's own table (and the feature table) go through Hand
		}
		std::map<std::string, std::string> V;
		V["Table"] = std::to_string(Round.TableId);
		V["TableWord"] = talk::NumberWords(Round.TableId);
		if (Round.Called && Close)
		{
			// All in and a call, shouted from that table; at hand for hand the floor relays it.
			Call(T, "table.called", V, Round.TableId, 0.9f);
			Vocal(audio::Vocal::Ooh, From::Table, Round.TableId, 6, "[The rail leans in.]", 0.6);
			if (H4H)
			{
				if (Has(SaidH4H))
				{
					Pa(T, "h4h.call", V, 2, false);
				}
			}
		}
		else if (Round.AllIn)
		{
			Call(T, "table.allin", V, Round.TableId, 0.6f);
		}
	}
	// Hand for hand: the floor sends the room on after any hand with an all-in, otherwise every third hand. A lone table
	// has no other tables to wait for, so nothing to call complete (the hands still count).
	if (H4H && !T.InTheMoney() && Has(SaidH4H))
	{
		++H4HHands;
		if (T.Tables.size() > 1 && (AnyAllIn || H4HHands % 3 == 0))
		{
			Pa(T, "h4h.done", {}, 1, false);
		}
	}
	UpdateMood(T, WorldNow);
}

// ------------------------------------------------------------------ the hand at the player's table

void Director::Showdown(const Tournament& T, const HandNote& H, const std::vector<Seen>& Table, int TableId)
{
	(void)Table;
	const int Here = SeatedTable(T);
	const bool Watched = TableId == Here || T.Tables.size() == 1;
	auto Named = [&T, this](const std::string& Who) { return Find(T, Who); };
	auto StackOf = [&T](const Seen& P) -> Chips {
		const int Idx = T.PlayerIndex(P.Id);
		return Idx >= 0 ? T.Players[static_cast<size_t>(Idx)].Stack : 0;
	};
	std::vector<Seen> Winners;
	for (const std::string& W : H.Winners)
	{
		const Seen P = Named(W);
		if (!P.Id.empty())
		{
			Winners.push_back(P);
		}
	}
	const auto IsWinner = [&Winners](const Seen& P) {
		return std::any_of(Winners.begin(), Winners.end(), [&P](const Seen& W) { return W.Id == P.Id; });
	};
	const bool Feature = TableId != Here;
	const bool Watchers = Feature || T.Tables.size() == 1 || Current == Mood::FinalTable || Current == Mood::HeadsUp;
	// The double-ups (against the stacks before this hand).
	for (const Seen& W : Winners)
	{
		const auto Before = Stacks.find(W.Name);
		if (StacksTable != TableId || Before == Stacks.end() || Before->second <= 0 || StackOf(W) < 2 * Before->second)
		{
			continue;
		}
		if (W.Hero)
		{
			RailSay(T, static_cast<int>(talk::Moment::DoubleUp));
			continue;
		}
		const life::LiveVoice& V = VoiceFor(T, W);
		if (Unit("double:" + W.Name + ":" + std::to_string(T.Tick)) < 0.5 + 0.4 * V.Social)
		{
			Say(T, W, talk::Moment::DoubleUp, {}, false, false, nullptr, true);
		}
		// A friend out of the event and still in the room shouts for them.
		for (const std::pair<std::string, int>& Tie : V.Ties)
		{
			if (Tie.second != static_cast<int>(world::TieKind::Friend))
			{
				continue;
			}
			const Seen F = Named(Tie.first);
			const int Idx = F.Id.empty() ? -1 : T.PlayerIndex(F.Id);
			if (Idx < 0 || !T.Players[static_cast<size_t>(Idx)].Busted || Unit("stayed:" + F.Name) > 0.6 || RailSpoke.count(F.Id))
			{
				continue;
			}
			talk::Context C;
			C.Vars["Name"] = W.Caption;
			C.Salt = Hash("railshout:" + F.Name + ":" + std::to_string(T.Tick));
			C.Told = &Told;
			const talk::Said S = talk::Line(talk::Moment::Rail, C);
			if (!S.Text.empty())
			{
				talk::Remember(Told, S);
				RailSpoke.insert(F.Id);
				Cue Cheer;
				Cheer.What = Kind::Line;
				Cheer.Where = From::Rail;
				Cheer.Who = F.Id;
				Cheer.Speaker = F.Caption;
				Cheer.Key = S.Key;
				Cheer.Text = S.Text;
				Cheer.Effort = 0.95f;
				Cheer.Tick = LastTick;
				Cheer.Delay = TalkCursor;
				TalkCursor += SpeechSeconds(S.Text, 15.0) + 0.3;
				Out.push_back(Cheer);
				Vocal(audio::Vocal::Cheer, From::Rail, TableId, 6, "[A cheer from the rail.]", Cheer.Delay + 0.6);
			}
			break;
		}
	}
	if (H.Shown.size() < 2 || Winners.empty())
	{
		// No showdown: now and then the winner says something (the eight in their own words).
		if (Winners.size() == 1 && !Winners[0].Hero)
		{
			const life::LiveVoice& V = VoiceFor(T, Winners[0]);
			if (Unit("win:" + Winners[0].Name + ":" + std::to_string(T.Tick)) < 0.06 + 0.12 * V.Social)
			{
				Say(T, Winners[0], talk::Moment::Win, {}, false, false, nullptr, true);
			}
		}
		return;
	}
	// The hands tabled, the winners and the rest.
	struct Tabled
	{
		Seen Who;
		std::vector<Card> Hole;
		bool Won = false;
	};
	std::vector<Tabled> Shown;
	for (const std::pair<std::string, std::vector<Card>>& S : H.Shown)
	{
		Tabled X;
		X.Who = Named(S.first);
		X.Hole = S.second;
		if (X.Who.Id.empty() || X.Hole.size() < 2)
		{
			continue;
		}
		X.Won = IsWinner(X.Who);
		Shown.push_back(X);
	}
	if (Shown.size() < 2)
	{
		return;
	}
	const Tabled* Winner = nullptr;
	for (const Tabled& X : Shown)
	{
		if (X.Won && !Winner)
		{
			Winner = &X;
		}
	}
	if (!Winner)
	{
		return;
	}
	if (H.AllIn && H.Called && Watched && H.BoardAtAllIn.size() < 5 && H.Board.size() == 5)
	{
		// Equity when the money went in (the hero's table and the feature table only: someone can see the cards there).
		EquityLog.push_back(TableId);
		std::vector<std::vector<Card>> Hands;
		for (const Tabled& X : Shown)
		{
			Hands.push_back(X.Hole);
		}
		Rng R(SeedText + ":equity:" + std::to_string(T.Tick) + ":" + std::to_string(TableId));
		const std::vector<double> Eq = EquityKnownHands(Hands, H.BoardAtAllIn, R, 2000);
		size_t LoserAt = Shown.size();
		for (size_t I = 0; I < Shown.size(); ++I)
		{
			if (!Shown[I].Won && (LoserAt == Shown.size() || Eq[I] > Eq[LoserAt]))
			{
				LoserAt = I;
			}
		}
		// The rail leans in as the cards turn (AllInCalled already did, mid-hand, when the host called it: the tick then
		// was this one, or the one before FinishTick moved it on).
		const bool Leaned = LeanTable == TableId && (LeanTick == T.Tick || LeanTick + 1 == T.Tick);
		if (!Leaned && (Watchers || Current == Mood::MoneyClose || Current == Mood::HandForHand))
		{
			Vocal(audio::Vocal::Ooh, Watchers ? From::Rail : From::Table, TableId, Watchers ? 12 : 6, "[The rail leans in: ooh.]", 0.0);
		}
		// The player all in at the final table and it held for them: their backer at the rail, or someone out tonight
		// who knows them, calls their name. ("Hold, Jesse! Hold!" belongs to while the cards turn: AllInCalled. Said now
		// it would come after the river.)
		const bool HeroIn = std::any_of(Shown.begin(), Shown.end(), [](const Tabled& X) { return X.Who.Hero; });
		const bool HeroWon = Winners.size() == 1 && Winners[0].Hero;
		if (HeroIn && HeroWon && Watchers)
		{
			Shout(T, "", TableId, 1.6);
		}
		if (LoserAt < Shown.size())
		{
			const Tabled& Loser = Shown[LoserAt];
			const double LoserEq = Eq[LoserAt];
			// The street where the winner got there for good.
			std::string Street = "river";
			for (const size_t Cards : {static_cast<size_t>(3), static_cast<size_t>(4), static_cast<size_t>(5)})
			{
				if (Cards <= H.BoardAtAllIn.size() && Cards < 5)
				{
					continue;
				}
				bool Ahead = true;
				for (size_t Later = Cards; Later <= 5; ++Later)
				{
					const std::vector<Card> Board(H.Board.begin(), H.Board.begin() + static_cast<std::ptrdiff_t>(Later));
					Ahead = Ahead && EvaluateHand(Winner->Hole, Board) > EvaluateHand(Loser.Hole, Board);
				}
				if (Ahead)
				{
					Street = Cards == 3 ? "flop" : Cards == 4 ? "turn" : "river";
					break;
				}
			}
			const std::string Mine = talk::SpokenHand(Loser.Hole);
			const std::string Theirs = talk::SpokenHand(Winner->Hole);
			const int LoserMade = CategoryOf(EvaluateHand(Loser.Hole, H.Board));
			if (LoserEq >= 0.75)
			{
				std::map<std::string, std::string> V;
				V["Mine"] = talk::Capitalized(Mine);
				V["MineLower"] = Mine;
				V["Theirs"] = talk::Capitalized(Theirs);
				V["TheirsLower"] = Theirs;
				V["Street"] = Street;
				if (!Winner->Who.Hero && Unit("suckout:" + std::to_string(T.Tick)) < 0.7)
				{
					Say(T, Winner->Who, talk::Moment::Suckout, {}, false, Loser.Who.Hero, nullptr, true);
				}
				if (!Loser.Who.Hero && Unit("badbeat:" + std::to_string(T.Tick)) < 0.85)
				{
					Say(T, Loser.Who, talk::Moment::BadBeat, V, false, false, nullptr, true);
				}
				// Still on their mind at the break, if they're still in it.
				const int Li = T.PlayerIndex(Loser.Who.Id);
				if (!Loser.Who.Hero && Li >= 0 && !T.Players[static_cast<size_t>(Li)].Busted && Beats.size() < 3)
				{
					Beats.push_back({Loser.Who.Name, Mine + "|" + Theirs + "|" + Street});
				}
			}
			else if (LoserEq >= 0.4 && LoserMade >= static_cast<int>(Category::TwoPair) && !Loser.Who.Hero && Unit("cooler:" + std::to_string(T.Tick)) < 0.6)
			{
				Say(T, Loser.Who, talk::Moment::Cooler, {}, false, false, nullptr, true);
			}
			// The rail: the favourite caught on the turn or the river gets the sharp "Ohh!" and the groan; when the
			// favourite holds, a murmur and light applause.
			if (LoserEq > 0.5 && (Watchers || Current == Mood::MoneyClose || Current == Mood::HandForHand))
			{
				Vocal(audio::Vocal::Gasp, Watchers ? From::Rail : From::Table, TableId, Watchers ? 12 : 6, "[A sharp \"Ohh!\" from the rail.]", 1.2);
				Vocal(audio::Vocal::Groan, Watchers ? From::Rail : From::Table, TableId, Watchers ? 12 : 6, "[The rail groans.]", 1.9);
			}
			else if (LoserEq <= 0.5 && Watchers)
			{
				Vocal(audio::Vocal::Clap, From::Rail, TableId, 8, "[A murmur, then light applause.]", 1.2);
			}
		}
		return;
	}
	// No all-in: a loser's courtesy (a reg's habit, or respect for whoever took it), and the eight's own reactions.
	for (const Tabled& X : Shown)
	{
		if (X.Won || X.Who.Hero)
		{
			continue;
		}
		const life::LiveVoice& V = VoiceFor(T, X.Who);
		if (talk::IsCast(X.Who.Name))
		{
			if (Unit("lose:" + X.Who.Name + ":" + std::to_string(T.Tick)) < 0.3 + 0.6 * V.Social)
			{
				Say(T, X.Who, talk::Moment::Lose, {}, false, false, nullptr, true);
			}
			continue;
		}
		const bool Reg = V.Style == static_cast<int>(Archetype::Reg) || V.Style == static_cast<int>(Archetype::Tag) || V.Style == static_cast<int>(Archetype::Crusher);
		const bool Courteous = Winner->Who.Hero ? (V.Respect >= 0.3f || Reg) : Reg;
		if (!Courteous || Unit("nice:" + X.Who.Name + ":" + std::to_string(T.Tick)) >= 0.35 + 0.3 * V.Social)
		{
			continue;
		}
		std::map<std::string, std::string> Vars;
		const Seen Aggressor = H.Aggressor.empty() ? Seen() : Named(H.Aggressor);
		if (!Aggressor.Id.empty() && Aggressor.Id == X.Who.Id)
		{
			Vars["Topic"] = CategoryOf(EvaluateHand(X.Hole, H.Board.size() >= 3 ? H.Board : std::vector<Card>())) >= static_cast<int>(Category::Pair) ? "called.made"
																																		   : "called.air";
		}
		Say(T, X.Who, talk::Moment::NiceHand, Vars, false, Winner->Who.Hero, nullptr, true);
		break;
	}
	if (talk::IsCast(Winner->Who.Name))
	{
		const life::LiveVoice& V = VoiceFor(T, Winner->Who);
		if (Unit("castwin:" + std::to_string(T.Tick)) < 0.35 + 0.5 * V.Social)
		{
			Say(T, Winner->Who, talk::Moment::Win, {}, false, false, nullptr, true);
		}
	}
}

void Director::Greetings(const Tournament& T)
{
	const int Here = HeroTable(T);
	if (!Greet || Here < 0 || !Has(SaidStart))
	{
		return;
	}
	Greet = false;
	GreetTable = Here;
	// After the floor (the welcome, a move): nobody talks over the PA's cards-in-the-air.
	TalkCursor = std::max(TalkCursor, PaCursor);
	int Faces = 0;
	// At the final table nobody says "Evening, all.": only the faces who know the player say hello.
	bool Generic = T.Tables.size() <= 1;
	for (const Seen& P : AtTable(T, Here))
	{
		if (P.Hero)
		{
			continue;
		}
		const auto Line = FaceLine.find(P.Name);
		if (Line != FaceLine.end() && !Line->second.empty() && Faces < 2 && Characters(Line->second) <= static_cast<size_t>(MaxShown))
		{
			std::map<std::string, std::string> V;
			V["Greeting"] = Line->second;
			Faces += Say(T, P, talk::Moment::SitDown, V, false, true, nullptr, true) ? 1 : 0;
		}
		else if (!Generic && Unit("hello:" + P.Name + ":" + std::to_string(T.Tick)) < 0.35)
		{
			Generic = Say(T, P, talk::Moment::SitDown, {}, false, false, nullptr, true);
		}
	}
}

void Director::Chatter(const Tournament& T, const HandNote& H, const std::vector<Seen>& Table)
{
	// Hand for hand is reactions only, from the hand that took the room there: Hand runs before the mood turns (its busts
	// are in the engine already), so the engine's word counts as well as the mood's (as Rounds reads it).
	if (OnBreak || !Has(SaidStart) || Current == Mood::HandForHand || T.HandForHand() || Current == Mood::Champion || Current == Mood::After || Current == Mood::Burst ||
		Current == Mood::HeadsUp)
	{
		return;
	}
	const std::set<std::string> Out2(H.Busted.begin(), H.Busted.end());
	std::vector<Seen> Here;
	for (const Seen& P : Table)
	{
		if (!P.Hero && !Out2.count(P.Name) && !Out2.count(P.Id))
		{
			Here.push_back(P);
		}
	}
	if (Here.empty())
	{
		return;
	}
	// Each talker's urge grows by their rate: lines an hour at Live pace (80 hands) by Sociability, times the setting;
	// people sitting with a friend talk twice as much.
	std::set<std::string> Names;
	for (const Seen& P : Here)
	{
		Names.insert(P.Name);
	}
	std::map<std::string, const life::LiveVoice*> VoiceOfSeat;
	for (const Seen& P : Here)
	{
		const life::LiveVoice& V = VoiceFor(T, P);
		VoiceOfSeat[P.Name] = &V;
		double Rate = LinesPerHour(V.Social) / 80.0 * static_cast<double>(std::clamp(TalkScale, 0.0f, 2.0f));
		for (const std::pair<std::string, int>& Tie : V.Ties)
		{
			if (Names.count(Tie.first) && (Tie.second == static_cast<int>(world::TieKind::Friend) || Tie.second == static_cast<int>(world::TieKind::HomeGame) ||
											  Tie.second == static_cast<int>(world::TieKind::TravelPartner)))
			{
				Rate *= 2.0;
				break;
			}
		}
		int& Urge = Acc[P.Name];
		Urge = std::min(3 * UrgeLine, Urge + static_cast<int>(std::lround(Rate * UrgeLine)));
	}
	// Who's ready, in an order the seed picks.
	std::vector<Seen> Ready;
	for (const Seen& P : Here)
	{
		const int Need = static_cast<int>(UrgeLine * (0.6 + 0.8 * Unit("need:" + P.Name + ":" + std::to_string(T.Tick))));
		if (Acc[P.Name] >= Need)
		{
			Ready.push_back(P);
		}
	}
	std::stable_sort(Ready.begin(), Ready.end(), [this, &T](const Seen& A, const Seen& B) {
		return Hash("order:" + A.Name + ":" + std::to_string(T.Tick)) < Hash("order:" + B.Name + ":" + std::to_string(T.Tick));
	});
	bool Thread = false;
	bool Asked = false;
	const std::string Tick = std::to_string(T.Tick);
	// A fresh world's first month (update 2 §3.3 F9): what the room's regulars could say about its new books right now
	// (worked out when someone first gets to the topics; never in an established world).
	std::vector<FreshTalk> BookTalk;
	bool BooksRead = false;
	for (const Seen& P : Ready)
	{
		if (Thread && Asked)
		{
			break;
		}
		const life::LiveVoice& V = *VoiceOfSeat[P.Name];
		const auto Spend = [this, &P]() { Acc[P.Name] -= UrgeLine; };
		// Noticed: someone who knows the player sees the patch (once a night a brand), said to them in their seat.
		if (!HeroPatches.empty() && !HeroUp)
		{
			const auto Face = FaceLabel.find(P.Name);
			const bool Knows = V.Familiarity >= 0.12f || (Face != FaceLabel.end() && Face->second != "Embercrest regular");
			if (Knows)
			{
				const std::string BrandId = HeroPatches[Hash("noticed:" + P.Name) % HeroPatches.size()].second;
				if (!Told.count("noticed:" + BrandId))
				{
					std::map<std::string, std::string> Vars;
					Vars["Topic"] = BrandId;
					if (const sponsor::Brand* B = sponsor::FindBrand(BrandId))
					{
						Vars["Brand"] = B->Short;
						Vars["Category"] = B->Cat == sponsor::Category::Local ? "local" : B->Cat == sponsor::Category::Energy ? "energy" : B->Cat == sponsor::Category::Site ? "site" : "other";
					}
					if (Say(T, P, talk::Moment::Noticed, Vars, true, true))
					{
						Spend();
						continue;
					}
				}
			}
		}
		// Asking the dealer: the average and the count near the money, the break when it's close (a table asks at most
		// once in six hands: the dealer's just said it).
		if (!Asked && T.Tick - AskTick >= 6)
		{
			const int ToBreak = LevelsToBreak(T);
			std::string Ask;
			if ((Current == Mood::MoneyClose || Current == Mood::LastTables) && Unit("ask:" + P.Name + ":" + Tick) < 0.35)
			{
				Ask = Unit("asktopic:" + Tick) < 0.5 ? "average" : "left";
			}
			else if (ToBreak <= 2 && Unit("askbreak:" + P.Name + ":" + Tick) < 0.12)
			{
				Ask = "break";
			}
			if (!Ask.empty())
			{
				std::map<std::string, std::string> Vars;
				Vars["Topic"] = Ask;
				if (Say(T, P, talk::Moment::Ask, Vars, true, false))
				{
					Spend();
					Asked = true;
					AskTick = T.Tick;
					std::map<std::string, std::string> D = RoomVars(T);
					const std::string Answer = AskAnswer(T, Ask, D);
					Call(T, Answer, D, HeroTable(T), 0.6f);
					continue;
				}
			}
		}
		if (Thread)
		{
			continue;
		}
		// A thread: something true to say, and a neighbour (a friend first) who answers.
		std::string Topic;
		std::map<std::string, std::string> Vars;
		const life::LiveVoice* To = nullptr;
		Seen Partner;
		if (talk::IsCast(P.Name) && Unit("castchat:" + P.Name + ":" + Tick) < 0.5)
		{
			Topic = "";
		}
		else
		{
			const std::vector<talk::Result> Story = talk::StoryOf(V.Story);
			std::vector<std::string> Options;
			for (size_t I = 0; I < Story.size(); ++I)
			{
				if (!Told.count("story:" + P.Name + ":" + std::to_string(Story[I].Day) + ":" + Story[I].Event))
				{
					Options.push_back("story:" + std::to_string(I));
				}
			}
			for (const std::pair<std::string, int>& Tie : V.Ties)
			{
				const bool Friendly = Tie.second == static_cast<int>(world::TieKind::Friend) || Tie.second == static_cast<int>(world::TieKind::HomeGame) ||
					Tie.second == static_cast<int>(world::TieKind::TravelPartner);
				const bool Rival = Tie.second == static_cast<int>(world::TieKind::Rival);
				if ((Friendly || Rival) && Names.count(Tie.first) && !Told.count(P.Name + ":small." + (Friendly ? "friend" : "rival")))
				{
					Options.push_back(std::string(Friendly ? "friend:" : "rival:") + Tie.first);
				}
			}
			for (const char* Self : {"heater", "down", "online", "regular", "makeup"})
			{
				if (!Told.count(P.Name + ":small." + Self))
				{
					Options.push_back(Self);
				}
			}
			for (const char* Common : {"trip", "food", "room", "night"})
			{
				if ((std::string(Common) != "trip" || T.LevelIndex < 3) && (std::string(Common) != "night" || T.LevelIndex < 8))
				{
					Options.push_back(Common);
				}
			}
			// The room's own (its locals, the eight, the live regulars) on the new books: each topic once a night.
			if (Fresh.Blank && (talk::KnownByName(P.Name) || V.Is == static_cast<int>(world::Identity::LiveRegular)))
			{
				if (!BooksRead)
				{
					BookTalk = FreshTopics(Fresh, Occ, TickWorld(T));
					BooksRead = true;
				}
				for (size_t I = 0; I < BookTalk.size(); ++I)
				{
					if (!Told.count("fresh:" + BookTalk[I].Key))
					{
						Options.push_back("fresh:" + std::to_string(I));
					}
				}
			}
			// The first option that's true of them, starting from one the seed picks among the personal ones.
			std::stable_sort(Options.begin(), Options.end(), [this, &P, &Tick](const std::string& A, const std::string& B) {
				const auto Weight = [](const std::string& O) { return O.rfind("story:", 0) == 0 ? 0 : O.find(':') != std::string::npos ? 1 : O == "trip" || O == "food" || O == "room" || O == "night" ? 3 : 2; };
				const int Wa = Weight(A);
				const int Wb = Weight(B);
				if (Wa != Wb)
				{
					return Wa < Wb;
				}
				return Hash("opt:" + P.Name + ":" + A + ":" + Tick) < Hash("opt:" + P.Name + ":" + B + ":" + Tick);
			});
			for (const std::string& O : Options)
			{
				// Personal topics come up some of the time; the common ones fill in.
				const bool Personal = O.rfind("story:", 0) == 0 || O.find(':') != std::string::npos || O == "heater" || O == "down" || O == "online" ||
					O == "regular" || O == "makeup";
				if (Personal && Unit("want:" + P.Name + ":" + O + ":" + Tick) > 0.55)
				{
					continue;
				}
				std::map<std::string, std::string> Try;
				std::string Tp = O;
				Seen Other;
				const life::LiveVoice* ToVoice = nullptr;
				if (O.rfind("story:", 0) == 0)
				{
					const int Which = std::atoi(O.substr(6).c_str());
					Tp = Story[static_cast<size_t>(Which)].Prize > 0 ? "week" : "bubble";
					Try["Record"] = std::to_string(Which);
				}
				else if (O.rfind("friend:", 0) == 0 || O.rfind("rival:", 0) == 0)
				{
					Tp = O.substr(0, O.find(':'));
					for (const Seen& S : Here)
					{
						if (S.Name == O.substr(O.find(':') + 1))
						{
							Other = S;
						}
					}
					if (Other.Name.empty())
					{
						continue;
					}
					ToVoice = VoiceOfSeat[Other.Name];
					Try["Name"] = Other.Caption;
				}
				else if (O.rfind("fresh:", 0) == 0)
				{
					// About the speaker themselves ("...self") or the player, to them ("...you").
					const FreshTalk& F = BookTalk[static_cast<size_t>(std::atoi(O.c_str() + 6))];
					Tp = F.Topic + (F.Hero ? ".you" : !F.Who.empty() && F.Who == P.Name ? ".self" : "");
					Try = F.Vars;
					Try["Books"] = "fresh";
					Try["FreshKey"] = F.Key;
				}
				Try["Topic"] = Tp;
				// Check it's honest and fits before spending the line.
				talk::Context Probe;
				Probe.Self = &V;
				Probe.To = ToVoice;
				Probe.Vars = Try;
				Probe.Vars["Day"] = std::to_string(Occ.Day);
				Probe.Vars["Hero"] = HeroFirst;
				Probe.Told = &Told;
				if (talk::Line(talk::Moment::Small, Probe).Text.empty())
				{
					continue;
				}
				Topic = Tp;
				Vars = Try;
				To = ToVoice;
				Partner = Other;
				break;
			}
			if (Topic.empty())
			{
				continue;
			}
		}
		if (!Topic.empty())
		{
			Vars["Topic"] = Topic;
		}
		// (A first-month line about the player is said to them, and nobody answers for them.)
		const bool ToYou = Topic.rfind("fresh.", 0) == 0 && Topic.size() > 4 && Topic.compare(Topic.size() - 4, 4, ".you") == 0;
		if (!Say(T, P, talk::Moment::Small, Vars, true, ToYou, To))
		{
			continue;
		}
		Spend();
		Thread = true;
		if (Topic.empty() || ToYou)
		{
			continue; // the eight's own chat stands alone
		}
		// The answer: the one addressed, or a neighbour with something of an urge to talk.
		std::vector<Seen> Answerers;
		if (!Partner.Name.empty())
		{
			Answerers.push_back(Partner);
		}
		else
		{
			for (const Seen& Q2 : Here)
			{
				const int D = std::abs(Q2.Seat - P.Seat);
				if (Q2.Name != P.Name && (D == 1 || D == T.TableSize - 1))
				{
					Answerers.push_back(Q2);
				}
			}
			std::stable_sort(Answerers.begin(), Answerers.end(), [this, &Tick](const Seen& A, const Seen& B) {
				return Hash("answer:" + A.Name + ":" + Tick) < Hash("answer:" + B.Name + ":" + Tick);
			});
		}
		for (const Seen& A : Answerers)
		{
			if (Partner.Name.empty() && Acc[A.Name] < UrgeLine / 2)
			{
				continue;
			}
			std::map<std::string, std::string> Reply;
			Reply["Topic"] = Topic;
			if (Say(T, A, talk::Moment::Answer, Reply, true, false, &V))
			{
				Acc[A.Name] -= UrgeLine;
				break;
			}
		}
	}
}

void Director::Hand(const Tournament& T, const HandNote& H)
{
	Sync(T);
	RailSpoke.clear();
	if (OnBreak)
	{
		BreakOver(T);
	}
	++HandsSeen;
	const size_t Before = Out.size();
	// When the hand's own reactions have landed, the floor may speak (Events waits for it).
	const auto Landed = [this, Before]() {
		for (size_t I = Before; I < Out.size(); ++I)
		{
			if (Out[I].What != Kind::Pa)
			{
				ReactionEnd = std::max(ReactionEnd, std::min(3.0, Out[I].Delay + 1.0));
			}
		}
	};
	// The player's table, the hand that put them out included (the engine has them busted already).
	const bool HeroOut = std::any_of(H.Busted.begin(), H.Busted.end(), [this](const std::string& B) { return IsHeroName(B) || B == HeroFirst; });
	if (HeroOut)
	{
		HeroOutTick = T.Tick;
	}
	const int Here = SeatedTable(T);
	int TableId = H.TableId != 0 ? H.TableId : Here;
	if (TableId <= 0 && T.Tables.size() == 1)
	{
		TableId = T.Tables.begin()->first;
	}
	if (TableId <= 0)
	{
		UpdateMood(T, WorldNow);
		return;
	}
	if (Greet)
	{
		Greetings(T);
	}
	const std::vector<Seen> Table = AtTable(T, TableId);
	Showdown(T, H, Table, TableId);
	LeanTable = -1; // a mid-hand lean is this hand's only
	Landed();       // the showdown's reactions; small talk after it doesn't hold up the floor
	if (TableId == Here)
	{
		const std::string Tick = std::to_string(T.Tick);
		const std::set<std::string> Gone(H.Busted.begin(), H.Busted.end());
		if (HeroOut)
		{
			// Never a needle at a bust: the felted get "Nice playing." from someone decent.
			for (const Seen& P : AtTable(T, T.Hero().TableId))
			{
				const life::LiveVoice& V = VoiceFor(T, P);
				if (!P.Hero && (V.Respect >= 0.2f || V.Social >= 0.5f) && Unit("felted:" + P.Name + ":" + Tick) < 0.6)
				{
					std::map<std::string, std::string> Vars;
					Vars["Topic"] = "felted";
					if (Say(T, P, talk::Moment::Goodbye, Vars, false, true, nullptr, true))
					{
						break;
					}
				}
			}
		}
		else
		{
			// Needles: after the player folds to their bet, shows a bluff, or tanks (once per needler per 4 hands); none at a
			// player up from their seat (the dealer mucked their hand for them, and they aren't there to hear it).
			const Seen Aggressor = H.Aggressor.empty() ? Seen() : Find(T, H.Aggressor);
			auto CanNeedle = [this, &T, &Gone](const Seen& P) {
				if (P.Id.empty() || P.Hero || Gone.count(P.Name) || Gone.count(P.Id))
				{
					return false;
				}
				const auto Last = NeedleAt.find(P.Name);
				return Last == NeedleAt.end() || T.Tick - Last->second >= NeedleTicks;
			};
			bool Needled = false;
			if (!HeroUp && H.HeroFoldedToRaise && CanNeedle(Aggressor) && Aggressor.Table == Here)
			{
				const life::LiveVoice& V = VoiceFor(T, Aggressor);
				if (Unit("needle:" + Tick) < 0.35 + 0.4 * V.Aggro && Say(T, Aggressor, talk::Moment::Needle, {}, false, true))
				{
					NeedleAt[Aggressor.Name] = T.Tick;
					Needled = true;
				}
			}
			if (!Needled && !HeroUp && H.HeroShowedBluff)
			{
				for (const Seen& P : Table)
				{
					if (CanNeedle(P) && Say(T, P, talk::Moment::ShowedBluff, {}, false, true))
					{
						NeedleAt[P.Name] = T.Tick;
						Needled = true;
						break;
					}
				}
			}
			if (!Needled && !HeroUp && H.HeroThink > 25.0)
			{
				for (const Seen& P : Table)
				{
					if (P.Hero || Gone.count(P.Name) || Unit("tank:" + P.Name + ":" + Tick) > 0.5)
					{
						continue;
					}
					const life::LiveVoice& V = VoiceFor(T, P);
					if (V.Social >= 0.6f && V.Aggro < 0.6f)
					{
						if (Say(T, P, talk::Moment::Tank, {}, false, true))
						{
							break;
						}
					}
					else if (CanNeedle(P))
					{
						std::map<std::string, std::string> Vars;
						Vars["Topic"] = "tank";
						if (Say(T, P, talk::Moment::Needle, Vars, false, true))
						{
							NeedleAt[P.Name] = T.Tick;
							break;
						}
					}
				}
			}
			Chatter(T, H, Table);
		}
	}
	UpdateMood(T, WorldNow);
	Snapshot(T);
}

// ------------------------------------------------------------------ the clock

void Director::BreakOver(const Tournament& T)
{
	if (!OnBreak)
	{
		return;
	}
	OnBreak = false;
	BreakCounting = false;
	if (OverSaid < BreakLevel)
	{
		OverSaid = BreakLevel;
		Pa(T, "break.over", RoomVars(T), 2, true);
	}
	UpdateMood(T, WorldNow);
}

void Director::Clock(const Tournament& T, double World, double BreakLeft)
{
	WorldNow = World;
	Sync(T);
	if (!Occ.Valid())
	{
		UpdateMood(T, World);
		return;
	}
	// Before cards: the desk and the ten-minute call, for someone already in the room.
	if (T.Tick == 0 && HandsSeen == 0 && !Has(SaidStart))
	{
		if (!Has(SaidDesk) && World >= Occ.DeskOpens)
		{
			Mark(SaidDesk);
			if (World < Occ.Start - 10.0)
			{
				std::map<std::string, std::string> V = RoomVars(T);
				V["Stack"] = talk::Count(Occ.T->StartingStack);
				V["Minutes"] = std::to_string(static_cast<int>(std::lround(Occ.T->LevelMinutes)));
				V["Time"] = net::TimeLabel(Occ.Start);
				Pa(T, "desk.open", V, 1, true);
			}
		}
		if (!Has(SaidTen) && World >= Occ.Start - 10.0)
		{
			Mark(SaidTen);
			if (World < Occ.Start)
			{
				Pa(T, "ten.minutes", RoomVars(T), 2, true);
			}
		}
	}
	// Cards in the air (or, for someone seated late, where registration stands).
	if (!Has(SaidStart) && (T.Tick > 0 || World >= Occ.Start - 1.0))
	{
		Mark(SaidStart);
		Mark(SaidDesk);
		Mark(SaidTen);
		std::map<std::string, std::string> V = RoomVars(T);
		const double Tw = TickWorld(T);
		if (T.Tick == 0)
		{
			Pa(T, "start", V, 3, true);
		}
		else if (Tw < Occ.LateRegEnds)
		{
			V["Time"] = net::TimeLabel(Occ.LateRegEnds);
			Pa(T, "start.late", V, 2, true);
		}
		else
		{
			Pa(T, "start.closed", V, 2, true);
			Mark(SaidLastCall);
			Mark(SaidRegClosed);
		}
		LevelSaid = std::max(LevelSaid, T.LevelIndex + 1);
		Greet = true;
	}
	if (Greet && Has(SaidStart) && HeroTable(T) >= 0)
	{
		Greetings(T);
	}
	// Registration, from the tournament's own clock (the same tick at any pace).
	const double Tw = TickWorld(T);
	if (Has(SaidStart) && !Has(SaidLastCall) && Tw >= live::LastCallAt(Occ.LateRegEnds))
	{
		Mark(SaidLastCall);
		if (Tw < Occ.LateRegEnds)
		{
			std::map<std::string, std::string> V = RoomVars(T);
			V["Time"] = net::TimeLabel(Occ.LateRegEnds);
			Pa(T, "last.call", V, 3, true);
		}
	}
	if (Has(SaidStart) && !Has(SaidRegClosed) && Tw >= Occ.LateRegEnds)
	{
		Mark(SaidRegClosed);
		Mark(SaidLastCall);
		Pa(T, "reg.closed", RoomVars(T), 3, true);
	}
	// The break's clock.
	if (OnBreak)
	{
		if (BreakLeft > 0.0)
		{
			BreakCounting = true;
			const double Share = T.Spec.BreakMinutes > 0.0 ? std::min(1.0, 5.0 / T.Spec.BreakMinutes) : 0.35;
			if (FiveSaid < BreakLevel && BreakLeft <= BreakSeconds * Share)
			{
				FiveSaid = BreakLevel;
				Pa(T, "five.minutes", {}, 1, false, "Floor", false);
			}
		}
		else if (BreakCounting)
		{
			BreakOver(T);
		}
	}
	UpdateMood(T, World);
}

// ------------------------------------------------------------------ the checkpoint

std::string Director::Write() const
{
	std::string S = "room1";
	auto Put = [&S](const char* K, const std::string& V) { S += std::string("|") + K + "=" + V; };
	Put("said", std::to_string(SaidBits));
	Put("lvl", std::to_string(LevelSaid));
	Put("brk", std::to_string(BreakSaid) + "," + std::to_string(FiveSaid) + "," + std::to_string(OverSaid) + "," + std::to_string(BreakLevel) + "," +
			std::to_string(BreakTick) + "," + (OnBreak ? "1" : "0") + "," + (BreakCounting ? "1" : "0"));
	Put("mood", std::to_string(static_cast<int>(Current)));
	Put("t", std::to_string(LastTick) + "," + std::to_string(Engine) + "," + std::to_string(PaFree) + "," + std::to_string(TableClock) + "," +
			std::to_string(H4HHands) + "," + std::to_string(RailRead) + "," + std::to_string(HandsSeen) + "," + std::to_string(BurstTick) + "," +
			std::to_string(ChampionTick) + "," + (Greet ? "1" : "0") + "," + std::to_string(GreetTable) + "," + std::to_string(AskTick));
	std::string Lines;
	for (const int L : TableLines)
	{
		Lines += (Lines.empty() ? "" : ",") + std::to_string(L);
	}
	Put("tl", Lines);
	// Only what can still matter: speakers within the last minute, needles within four hands, urges above nothing.
	std::map<std::string, int> Spoke, Smalls, Needles, Urges;
	for (const auto& Kv : SpokeAt)
	{
		if (Engine - Kv.second < SmallGap + HandSeconds)
		{
			Spoke.insert(Kv);
		}
	}
	for (const auto& Kv : SmallAt)
	{
		if (Engine - Kv.second < SmallGap + HandSeconds)
		{
			Smalls.insert(Kv);
		}
	}
	for (const auto& Kv : NeedleAt)
	{
		if (LastTick - Kv.second < NeedleTicks)
		{
			Needles.insert(Kv);
		}
	}
	for (const auto& Kv : Acc)
	{
		if (Kv.second != 0)
		{
			Urges.insert(Kv);
		}
	}
	Put("sp", MapText(Spoke));
	Put("sm", MapText(Smalls));
	Put("nd", MapText(Needles));
	Put("acc", MapText(Urges));
	std::string Tolds;
	for (const std::string& T : Told)
	{
		Tolds += (Tolds.empty() ? "" : ",") + Esc(T);
	}
	Put("told", Tolds);
	std::string BeatText;
	for (const std::pair<std::string, std::string>& B : Beats)
	{
		BeatText += (BeatText.empty() ? "" : ",") + Esc(B.first) + "~" + Esc(B.second);
	}
	Put("beats", BeatText);
	char Champ[64];
	std::snprintf(Champ, sizeof(Champ), "%.2f", ChampionUntil);
	Put("champ", Champ);
	return S;
}

bool Director::Read(const std::string& Text)
{
	const std::vector<std::string> Parts = Split(Text, '|');
	if (Parts.empty() || Parts[0] != "room1")
	{
		return false;
	}
	std::map<std::string, std::string> Kv;
	for (size_t I = 1; I < Parts.size(); ++I)
	{
		const size_t Eq = Parts[I].find('=');
		if (Eq == std::string::npos)
		{
			return false;
		}
		Kv[Parts[I].substr(0, Eq)] = Parts[I].substr(Eq + 1);
	}
	for (const char* Need : {"said", "lvl", "brk", "mood", "t"})
	{
		if (!Kv.count(Need))
		{
			return false;
		}
	}
	const std::vector<std::string> Brk = Split(Kv["brk"], ',');
	const std::vector<std::string> Tt = Split(Kv["t"], ',');
	if (Brk.size() < 7 || Tt.size() < 11)
	{
		return false;
	}
	SaidBits = static_cast<uint32_t>(std::strtoul(Kv["said"].c_str(), nullptr, 10));
	LevelSaid = std::atoi(Kv["lvl"].c_str());
	BreakSaid = std::atoi(Brk[0].c_str());
	FiveSaid = std::atoi(Brk[1].c_str());
	OverSaid = std::atoi(Brk[2].c_str());
	BreakLevel = std::atoi(Brk[3].c_str());
	BreakTick = std::atoi(Brk[4].c_str());
	OnBreak = Brk[5] == "1";
	BreakCounting = Brk[6] == "1";
	Current = static_cast<Mood>(std::clamp(std::atoi(Kv["mood"].c_str()), 0, static_cast<int>(Mood::After)));
	LastTick = std::atoi(Tt[0].c_str());
	Engine = std::atoi(Tt[1].c_str());
	PaFree = std::atoi(Tt[2].c_str());
	TableClock = std::atoi(Tt[3].c_str());
	H4HHands = std::atoi(Tt[4].c_str());
	RailRead = std::atoi(Tt[5].c_str());
	HandsSeen = std::atoi(Tt[6].c_str());
	BurstTick = std::atoi(Tt[7].c_str());
	ChampionTick = std::atoi(Tt[8].c_str());
	Greet = Tt[9] == "1";
	GreetTable = std::atoi(Tt[10].c_str());
	AskTick = Tt.size() >= 12 ? std::atoi(Tt[11].c_str()) : -100; // fields are appended: older text reads without them
	TableLines.clear();
	for (const std::string& L : Split(Kv["tl"], ','))
	{
		TableLines.push_back(std::atoi(L.c_str()));
	}
	SpokeAt = MapFrom(Kv["sp"]);
	SmallAt = MapFrom(Kv["sm"]);
	NeedleAt = MapFrom(Kv["nd"]);
	Acc = MapFrom(Kv["acc"]);
	Told.clear();
	for (const std::string& T : Split(Kv["told"], ','))
	{
		Told.insert(Unesc(T));
	}
	Beats.clear();
	for (const std::string& B : Split(Kv["beats"], ','))
	{
		const size_t Tilde = B.find('~');
		if (Tilde != std::string::npos)
		{
			Beats.push_back({Unesc(B.substr(0, Tilde)), Unesc(B.substr(Tilde + 1))});
		}
	}
	ChampionUntil = Kv.count("champ") ? std::atof(Kv["champ"].c_str()) : -1.0;
	return true;
}

void Director::Resume(const Tournament& T, const std::string& State, double World)
{
	WorldNow = World;
	if (!State.empty() && Read(State))
	{
		Snapshot(T);
		UpdateMood(T, World);
		return;
	}
	// Nothing kept (a checkpoint from before the room kept its own): what was said is worked out from the night itself
	// (§7.5 step 6). A story told before is lost; at worst one repeats once.
	LastTick = T.Tick;
	Engine = T.Tick * HandSeconds;
	TableClock = Engine;
	PaFree = Engine;
	HandsSeen = T.Tick > 0 ? 1 : 0;
	LevelSaid = std::max(LevelSaid, T.LevelIndex + 1);
	BreakSaid = FiveSaid = OverSaid = LevelSaid;
	const double Tw = std::max(TickWorld(T), World);
	if (T.Tick > 0)
	{
		Mark(SaidStart);
		Mark(SaidDesk);
		Mark(SaidTen);
		Greet = false;
	}
	if (Tw >= live::LastCallAt(Occ.LateRegEnds))
	{
		Mark(SaidLastCall);
	}
	if (Tw >= Occ.LateRegEnds)
	{
		Mark(SaidRegClosed);
	}
	if (T.InTheMoney() || T.Remaining <= T.PaidPlaces() + 3)
	{
		Mark(SaidAway);
	}
	if (T.InTheMoney() || T.Remaining <= T.PaidPlaces() + 1)
	{
		Mark(SaidOnBubble);
	}
	if (T.InTheMoney() || T.HandForHand())
	{
		Mark(SaidH4H);
	}
	if (T.InTheMoney())
	{
		Mark(SaidBubble);
	}
	if (T.Tables.size() <= 2)
	{
		Mark(SaidTwoTables);
	}
	if (T.Tables.size() <= 1)
	{
		Mark(SaidFinal);
		Mark(SaidIntros);
	}
	if (T.Remaining <= 2)
	{
		Mark(SaidHeadsUp);
	}
	if (T.bFinished)
	{
		Mark(SaidChampion);
	}
	if (Tw >= Occ.Start + 90.0)
	{
		Mark(SaidCourtesy);
	}
	Snapshot(T);
	UpdateMood(T, World);
}
} // namespace room
} // namespace ss
