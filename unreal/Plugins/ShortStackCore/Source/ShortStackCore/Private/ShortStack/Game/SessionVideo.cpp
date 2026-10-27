#include "ShortStack/Game/Session.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "VideoContentPriv.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <set>

// Rerun's side of the session (update 2 §4, §6.2): the candidates the hands make on both online paths (watched and
// skipped) and the event's recap, life stories, publishing (the edit's TimeSkip and its settle), deleting, the channel's
// view, the calendar's step and ss.Video.*. core-video.
//
// Capture reads the hand and the session and changes nothing but the channel (and the stream's moment notes, which it
// links to the candidate): it draws from no Rng, so a night plays the same with it or without it.

namespace ss
{
namespace video
{
namespace video_detail
{
double EquityAt(const std::vector<Card>& Mine, const std::vector<std::vector<Card>>& Theirs, const std::vector<Card>& Board, const std::string& Seed); // Video.cpp
} // namespace video_detail
namespace video_copy
{
std::string EditLabel(Format F); // VideoCopy.cpp
std::string UploadedTitle();
std::string UploadedBody(const std::string& Title);
std::string EditFailedTitle();
std::string DeeStorytime();
std::string NoSuchVideo();
std::string WhyNot(int Reason);
} // namespace video_copy
} // namespace video

namespace session_video3_detail
{
/** After an upload lands (SessionVideo3.cpp): a series' episode counted, its playlist and its challenge's state. */
void AfterUpload(life::State& L, const std::vector<HistoryEntry>& History, const world::World& W, const kast::Channel& K, const gear::Owned& G,
	const std::set<std::string>& Told, Chips Bankroll, const std::string& Hero, bool Evicted, double World, const video::Choice& C,
	const std::vector<video::Candidate>& Picked, const std::string& VideoId);
} // namespace session_video3_detail

namespace session_video_detail
{
/** The life beats a story candidate is cut from (§4.4), by the StoryText key the session already sends once. */
const char* const StoryKeys[] = {"rent-paid", "seat-rcop-main", "bracelet-first", "ring-first", "mm-title", "series-title"};

/** The key for "back home after an eviction": one per eviction. */
std::string BackKey(int Evictions)
{
	return "back:" + std::to_string(Evictions);
}

/** In Channel::Paid once the stories from before the channel had state were marked told (no story key starts with '*'). */
const char* const StoriesMarked = "story:*";

/**
 * Once, the first time the channel has state: the life's stories that already happened (before the channel existed) are
 * marked as told, so an old career doesn't get a pile of stale ones. The card room can set the channel's bit first
 * (video::NoteLiveHand has no Session), so the session's next calendar step does it then.
 */
void MarkOldStories(life::State& L, const std::set<std::string>& TextsSeen, bool Evicted)
{
	if (!L.Video.Paid.insert(StoriesMarked).second)
	{
		return;
	}
	for (const char* Key : StoryKeys)
	{
		if (TextsSeen.count(Key) > 0)
		{
			L.Video.Paid.insert(std::string("story:") + Key);
		}
	}
	for (int N = 1; N <= L.Evictions; ++N)
	{
		if (N < L.Evictions || !Evicted)
		{
			L.Video.Paid.insert("story:" + BackKey(N));
		}
	}
}

/** The channel has state to keep from now on (§6.2): its lines are written, and the old stories are marked told. */
void Touch(life::State& L, const std::set<std::string>& TextsSeen, bool Evicted)
{
	if ((L.Update2LinesRead & life::U2Video) != 0)
	{
		return;
	}
	L.Update2LinesRead |= life::U2Video;
	MarkOldStories(L, TextsSeen, Evicted);
}

/**
 * The rent-paid story's titles say poker paid the rent ("I paid rent with poker money"), so it's a story only when that's
 * true: the poker money in (ledger kinds 0 and 4: prizes, cashes, the Night Shift) in the 30 days to the rent covered it.
 * A refunded buy-in is the player's own money back, not money won (the Bank counts it off the spending the same way).
 * The rent is the newest rent line on the ledger (PayRent's, a moment before the calendar's step reads it).
 */
bool RentPaidByPoker(const life::State& L)
{
	const life::LedgerEntry* Rent = nullptr;
	for (const life::LedgerEntry& E : L.Ledger) // newest first
	{
		if (E.Kind == 3 && E.Amount < 0 && E.Label.rfind("Rent", 0) == 0)
		{
			Rent = &E;
			break;
		}
	}
	if (!Rent)
	{
		return false;
	}
	Chips Poker = 0;
	for (const life::LedgerEntry& E : L.Ledger)
	{
		if ((E.Kind == 0 || E.Kind == 4) && E.Amount > 0 && E.Label.find(": refunded") == std::string::npos && E.At <= Rent->At &&
			E.At >= Rent->At - 30.0 * net::MinutesPerDay)
		{
			Poker += E.Amount;
		}
	}
	return Poker >= -Rent->Amount;
}

/**
 * A moment of the hand itself, the kind a candidate links (§4.4's stream add-on): its all-in and how it went, its pot, a
 * decision graded in it, a knockout, and the bust, cash or win it ended the night with. The tick's own moments (a new
 * tournament, a level, the bubble, the money, the final table, the rival sitting down) land on the clock between hands,
 * so the window would hand them to the next hand's candidate (its chat, its face), and they aren't that hand's.
 */
bool VideoHandMoment(kast::Moment M)
{
	switch (M)
	{
	case kast::Moment::AllIn:
	case kast::Moment::WonAllIn:
	case kast::Moment::LostAllIn:
	case kast::Moment::BadBeat:
	case kast::Moment::BigPot:
	case kast::Moment::Knockout:
	case kast::Moment::Win:
	case kast::Moment::Bust:
	case kast::Moment::Cashed:
	case kast::Moment::BestPlay:
	case kast::Moment::Blunder: return true;
	default: return false;
	}
}

/** "rl:" for RiverLine, the site's key for a rival (§4.4): KeyOf, exact for every SiteId (Def falls back to RiverLine's). */
std::string SitePrefix(const std::string& EventId)
{
	const net::SiteId Site = net::SiteOf(EventId);
	return Site == net::SiteId::RiverLine ? std::string("rl:") : std::string(net::KeyOf(Site)) + ":";
}

bool SeriesEvent(const Tournament& T)
{
	const net::EventTemplate* Tmpl = net::ForEvent(T.Spec.Id).FindTemplate(T.Spec.Id.substr(0, T.Spec.Id.find('@')));
	return (Tmpl && (!Tmpl->Series.empty() || Tmpl->Bracelet || Tmpl->Ring || Tmpl->Main)) || T.Spec.Name.rfind("MM #", 0) == 0;
}

/** A format, title style or frame by its name (ss.Video.Upload), or -1. */
int NamedIndex(const std::string& Name, const char* (*NameOf)(int), int Count)
{
	for (int I = 0; I < Count; ++I)
	{
		std::string N = NameOf(I);
		for (char& Ch : N)
		{
			Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
		}
		if (N == Name)
		{
			return I;
		}
	}
	return -1;
}
const char* FormatAt(int I)
{
	return video::FormatName(static_cast<video::Format>(I));
}
const char* StyleAt(int I)
{
	return video::TitleStyleName(static_cast<video::TitleStyle>(I));
}
const char* FrameAt(int I)
{
	return video::FrameName(static_cast<video::Frame>(I));
}

std::vector<std::string> Words(const std::string& Args)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char Ch : Args)
	{
		if (Ch == ' ' || Ch == '\t')
		{
			if (!Cur.empty())
			{
				Out.push_back(Cur);
			}
			Cur.clear();
		}
		else
		{
			Cur.push_back(Ch);
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

/** The edit's activity id: "video:<format>:<title style>:<frame>:<id>|<id>|..." (candidate ids carry no '|'). */
/** A field of the edit's id, percent-escaped (nothing but letters, digits, '-', '_' and '.' stays as it is). */
std::string EditEsc(const std::string& S)
{
	static const char* const Hex = "0123456789ABCDEF";
	std::string Out;
	for (const char Ch : S)
	{
		const unsigned char U = static_cast<unsigned char>(Ch);
		if (std::isalnum(U) || Ch == '-' || Ch == '_' || Ch == '.')
		{
			Out.push_back(Ch);
		}
		else
		{
			Out.push_back('%');
			Out.push_back(Hex[U >> 4]);
			Out.push_back(Hex[U & 15]);
		}
	}
	return Out;
}

std::string EditUnesc(const std::string& S)
{
	std::string Out;
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (S[I] == '%' && I + 2 < S.size())
		{
			Out.push_back(static_cast<char>(std::strtol(S.substr(I + 1, 2).c_str(), nullptr, 16)));
			I += 2;
		}
		else
		{
			Out.push_back(S[I]);
		}
	}
	return Out;
}

std::string EditId(const video::Choice& C)
{
	std::string Id = "video:" + std::to_string(static_cast<int>(C.F)) + ":" + std::to_string(static_cast<int>(C.T)) + ":" + std::to_string(static_cast<int>(C.Fr)) + ":";
	for (size_t I = 0; I < C.Candidates.size(); ++I)
	{
		Id += (I > 0 ? "|" : "") + C.Candidates[I];
	}
	// Update 3 (core-videocontent): Studio's template, recipes and levers ride after a tab; an Update 2 choice has none.
	if (!C.TitleTpl.empty() || !C.Thumbs.empty())
	{
		Id += "\tu3|tpl=" + EditEsc(C.TitleTpl) + "|sh=" + std::to_string(C.Shuffle) + "|ads=" + std::to_string(static_cast<int>(C.Ads)) +
			"|len=" + std::to_string(static_cast<int>(C.Len)) + "|clean=" + (C.CleanEdit ? "1" : "0") + "|next=" + EditEsc(C.NextUp) + "|ser=" + std::to_string(C.Series);
		for (const video::ThumbSpec& T : C.Thumbs)
		{
			const int Fields[14] = {T.Arch, T.Layout, T.Mirror, T.Bg, T.Pal, T.Subj, T.Expr, T.Pose, T.PropA, T.PropB, T.CapStyle, T.Sticker, T.Corner, T.LookRef};
			Id += "|th=";
			for (const int F : Fields)
			{
				Id += std::to_string(F) + ",";
			}
			Id += std::to_string(T.Variant) + "," + EditEsc(T.Caption) + "," + EditEsc(T.Caption2);
		}
	}
	return Id;
}

/** Reads the Update 3 tail of an edit's id into C (EditId's "u3|..."). */
void ReadEditTail(const std::string& Tail, video::Choice& C)
{
	size_t At = 0;
	while (At <= Tail.size())
	{
		size_t End = Tail.find('|', At);
		End = End == std::string::npos ? Tail.size() : End;
		const std::string Tok = Tail.substr(At, End - At);
		const size_t Eq = Tok.find('=');
		const std::string K = Eq == std::string::npos ? Tok : Tok.substr(0, Eq), V = Eq == std::string::npos ? std::string() : Tok.substr(Eq + 1);
		if (K == "tpl")
		{
			C.TitleTpl = EditUnesc(V);
		}
		else if (K == "sh")
		{
			C.Shuffle = std::atoi(V.c_str());
		}
		else if (K == "ads")
		{
			C.Ads = static_cast<video::AdLoad>(std::clamp(std::atoi(V.c_str()), 0, static_cast<int>(video::AdLoad::Count) - 1));
		}
		else if (K == "len")
		{
			C.Len = static_cast<video::LengthOpt>(std::clamp(std::atoi(V.c_str()), 0, 2));
		}
		else if (K == "clean")
		{
			C.CleanEdit = V == "1";
		}
		else if (K == "next")
		{
			C.NextUp = EditUnesc(V);
		}
		else if (K == "ser")
		{
			C.Series = std::atoi(V.c_str());
		}
		else if (K == "th")
		{
			std::vector<std::string> P;
			size_t B = 0;
			for (size_t I = 0; I <= V.size(); ++I)
			{
				if (I == V.size() || V[I] == ',')
				{
					P.push_back(V.substr(B, I - B));
					B = I + 1;
				}
			}
			if (P.size() >= 17)
			{
				video::ThumbSpec T;
				int16_t* Fields[14] = {&T.Arch, &T.Layout, &T.Mirror, &T.Bg, &T.Pal, &T.Subj, &T.Expr, &T.Pose, &T.PropA, &T.PropB, &T.CapStyle, &T.Sticker, &T.Corner, &T.LookRef};
				for (int I = 0; I < 14; ++I)
				{
					*Fields[I] = static_cast<int16_t>(std::atoi(P[static_cast<size_t>(I)].c_str()));
				}
				T.Variant = static_cast<uint32_t>(std::strtoul(P[14].c_str(), nullptr, 10));
				T.Caption = EditUnesc(P[15]);
				T.Caption2 = EditUnesc(P[16]);
				if (C.Thumbs.size() < 3)
				{
					C.Thumbs.push_back(T);
				}
			}
		}
		At = End + 1;
	}
}

bool ReadEditId(const std::string& Full, video::Choice& C)
{
	if (Full.rfind("video:", 0) != 0)
	{
		return false;
	}
	const size_t Tab = Full.find('\t');
	const std::string Id = Full.substr(0, Tab);
	if (Tab != std::string::npos && Full.compare(Tab + 1, 3, "u3|") == 0)
	{
		ReadEditTail(Full.substr(Tab + 4), C);
	}
	size_t At = 6;
	int Fields[3] = {0, 0, 0};
	for (int I = 0; I < 3; ++I)
	{
		const size_t Colon = Id.find(':', At);
		if (Colon == std::string::npos)
		{
			return false;
		}
		Fields[I] = std::atoi(Id.substr(At, Colon - At).c_str());
		At = Colon + 1;
	}
	// (Update 3, §7.3: clamped to each enum's Count, so an appended value reads as itself; every old value as before.)
	C.F = static_cast<video::Format>(std::clamp(Fields[0], 0, static_cast<int>(video::Format::Count) - 1));
	C.T = static_cast<video::TitleStyle>(std::clamp(Fields[1], 0, static_cast<int>(video::TitleStyle::Count) - 1));
	C.Fr = static_cast<video::Frame>(std::clamp(Fields[2], 0, static_cast<int>(video::Frame::Count) - 1));
	C.Candidates.clear();
	std::string Cur;
	for (size_t I = At; I <= Id.size(); ++I)
	{
		if (I == Id.size() || Id[I] == '|')
		{
			if (!Cur.empty())
			{
				C.Candidates.push_back(Cur);
			}
			Cur.clear();
		}
		else
		{
			Cur.push_back(Id[I]);
		}
	}
	return !C.Candidates.empty();
}

/** A scripted hand for ss.Video.Fake: the kind's shape, with real cards and real arithmetic (dev only). */
video::Candidate Fake(video::Kind K, int Source, double World)
{
	using video::Kind;
	video::Candidate C;
	C.K = K;
	C.At = World;
	C.Source = Source;
	C.Event = Source == 1 ? "Embercrest Nightly" : Source == 2 ? "Dee's game" : "Night Owl Turbo";
	C.BuyIn = Source == 2 ? 4000 : Source == 1 ? 12000 : 550;
	C.Field = Source == 2 ? 0 : 104;
	C.Storytime = Source == 2;
	C.Auto = Source == 3;
	recall::Record& R = C.Rec;
	R.Number = 214;
	R.Level = 9;
	R.Blinds.Sb = 100;
	R.Blinds.Bb = 200;
	const std::string Villain = Source == 2 ? "Big Lou" : "Mike_R";
	C.Opponent = Villain;
	auto Line = [&R](Street S, const std::string& Who, const std::string& What, Chips To) { R.Lines.push_back({S, Who, What, To}); };
	auto Cards = [](const char* Text) { return ParseCards(Text); };
	bool Shown = false;
	switch (K)
	{
	case Kind::BadBeat:
	case Kind::Suckout:
	case Kind::Cooler:
	case Kind::Bubble:
	case Kind::Knockout:
	case Kind::Rival:
	{
		const bool Hero = K == Kind::Suckout || K == Kind::Knockout || K == Kind::Rival;
		R.Mine = Cards(K == Kind::Cooler ? "KdKs" : Hero && K == Kind::Suckout ? "AcJd" : "AsAd");
		const std::vector<Card> Theirs = Cards(K == Kind::Cooler ? "AhAc" : K == Kind::Suckout ? "QhQc" : "7c7h");
		// Each board deals the hand its record says: the kings' set under the aces' (the cooler), ace-jack two pair on the river
		// past the queens (the suckout), the sevens' river set (the bad beat), the aces holding (the rest).
		R.Board = Cards(K == Kind::Cooler ? "Kh7dAs9s4h" : K == Kind::Suckout ? "8d3cJs5hAh" : K == Kind::BadBeat ? "Kd8s2c9h7d" : "Kd8s2c9h4d");
		Line(Street::Preflop, "You", "raises to", 600);
		Line(Street::Preflop, Villain, "all in", 6400);
		Line(Street::Preflop, "You", "all in", 6400);
		R.Shown.push_back({Villain, Theirs});
		R.Pot = 12900;
		const bool Won = K == Kind::Suckout || K == Kind::Knockout || K == Kind::Rival || K == Kind::Bubble;
		R.HeroNet = Won ? 6500 : -6400;
		C.EquityAtAllIn = K == Kind::BadBeat ? 0.8143 : K == Kind::Suckout ? 0.2988 : -1.0;
		C.Bubble = K == Kind::Bubble;
		if (K == Kind::Rival)
		{
			C.Opponent = RivalName;
			R.Lines[1].Who = RivalName;
			R.Lines[2].Who = "You";
			R.Shown[0].first = RivalName;
			C.Rival = true;
		}
		Shown = true;
		break;
	}
	case Kind::Bluff:
	case Kind::BluffCaught:
	{
		R.Mine = Cards("6s5s");
		R.Board = Cards("KhQd2cJs8h");
		Line(Street::Preflop, "You", "raises to", 500);
		Line(Street::Preflop, Villain, "calls", 500);
		Line(Street::Flop, "You", "bets", 700);
		Line(Street::Flop, Villain, "calls", 700);
		Line(Street::Turn, "You", "bets", 1800);
		Line(Street::Turn, Villain, "calls", 1800);
		Line(Street::River, "You", "bets", 4200);
		if (K == Kind::Bluff)
		{
			Line(Street::River, Villain, "folds", 0);
			R.Pot = 6400;
			R.HeroNet = 3000;
		}
		else
		{
			Line(Street::River, Villain, "calls", 4200);
			R.Shown.push_back({Villain, Cards("KcTd")});
			R.Pot = 14700;
			R.HeroNet = -7200;
			Shown = true;
		}
		break;
	}
	case Kind::HeroCall:
	{
		R.Mine = Cards("Td9d");
		R.Board = Cards("Th6c3s2h8c");
		Line(Street::Preflop, Villain, "raises to", 500);
		Line(Street::Preflop, "You", "calls", 500);
		Line(Street::Flop, Villain, "bets", 600);
		Line(Street::Flop, "You", "calls", 600);
		Line(Street::River, Villain, "bets", 3300);
		Line(Street::River, "You", "calls", 3300);
		R.Shown.push_back({Villain, Cards("AsQs")});
		R.Pot = 9100;
		R.HeroNet = 4400;
		Shown = true;
		break;
	}
	case Kind::HeroFold:
	{
		R.Mine = Cards("QhQd");
		R.Board = Cards("Ks9c4h");
		Line(Street::Preflop, "You", "raises to", 500);
		Line(Street::Preflop, Villain, "calls", 500);
		Line(Street::Flop, "You", "bets", 600);
		Line(Street::Flop, Villain, "all in", 5800);
		Line(Street::Flop, "You", "folds", 0);
		R.Pot = 7300;
		R.HeroNet = -1100;
		break;
	}
	case Kind::BigPot:
	{
		R.Mine = Cards("JhJc");
		R.Board = Cards("Jd8s3c2h9c");
		Line(Street::Preflop, Villain, "raises to", 500);
		Line(Street::Preflop, "You", "raises to", 1500);
		Line(Street::Preflop, Villain, "calls", 1500);
		Line(Street::Flop, "You", "bets", 1800);
		Line(Street::Flop, Villain, "raises to", 5200);
		Line(Street::Flop, "You", "all in", 9600);
		Line(Street::Flop, Villain, "calls", 9600);
		R.Shown.push_back({Villain, Cards("8h8d")});
		R.Pot = 22500;
		R.HeroNet = 11100;
		Shown = true;
		break;
	}
	case Kind::DeepRun:
	case Kind::FinalTable:
	case Kind::Win:
	{
		C.Rec = recall::Record();
		C.PlaceSoFar = K == Kind::Win ? 1 : K == Kind::FinalTable ? 4 : 15;
		C.Rec.Pot = K == Kind::Win ? 13900 : K == Kind::FinalTable ? 4200 : 1900;
		C.Rec.HeroNet = C.Rec.Pot - C.BuyIn;
		C.Final = K != Kind::DeepRun;
		C.Opponent.clear();
		break;
	}
	case Kind::Story:
	case Kind::StreamMoment:
	case Kind::Count: break;
	}
	(void)Shown;
	if (Source == 0 || Source == 3)
	{
		C.Id = "rl:fake@" + std::to_string(static_cast<long long>(std::llround(World))) + ":" + std::to_string(static_cast<int>(K));
	}
	else if (Source == 1)
	{
		C.Id = "lv:fake@" + std::to_string(static_cast<long long>(std::llround(World))) + ":" + std::to_string(static_cast<int>(K));
	}
	else
	{
		C.Id = "dee:" + std::to_string(net::DayOf(World)) + ":" + std::to_string(500 + static_cast<int>(K));
	}
	C.Expires = World + 21.0 * net::MinutesPerDay;
	return C;
}
} // namespace session_video_detail

void Session::VideoNoteHand(const Hand& H, const HandFacts& F)
{
	using namespace session_video_detail;
	if (!T)
	{
		return;
	}
	const HandSeat* HeroSeat = nullptr;
	for (const HandSeat& S : H.Seats)
	{
		HeroSeat = S.Id == HeroId ? &S : HeroSeat;
	}
	if (!HeroSeat || HeroSeat->Hole.size() < 2)
	{
		return;
	}
	// The hand as the player saw it: their name is "You" (the live recall's), the others by screen name.
	std::vector<std::string> Names;
	for (const HandSeat& S : H.Seats)
	{
		if (S.Seat >= static_cast<int>(Names.size()))
		{
			Names.resize(static_cast<size_t>(S.Seat) + 1);
		}
		const TPlayer* P = S.Id == HeroId ? nullptr : PlayerById(S.Id);
		Names[static_cast<size_t>(S.Seat)] = S.Id == HeroId ? std::string("You") : P ? P->Name : "Seat " + std::to_string(S.Seat + 1);
	}
	std::vector<int> ShownSeats;
	bool Showdown = false;
	bool Reveal = false;
	size_t BoardNow = 0, BoardAtReveal = 0;
	for (const HandEvent& E : H.Events)
	{
		if (E.Type == EventType::Street)
		{
			BoardNow = E.Board.size();
		}
		else if (E.Type == EventType::Showdown)
		{
			Showdown = true;
			for (const ShowdownHand& S : E.Hands)
			{
				ShownSeats.push_back(S.Seat);
			}
		}
		else if (E.Type == EventType::Reveal)
		{
			BoardAtReveal = Reveal ? BoardAtReveal : BoardNow;
			Reveal = true;
			ShownSeats.insert(ShownSeats.end(), E.Seats.begin(), E.Seats.end());
		}
	}
	std::sort(ShownSeats.begin(), ShownSeats.end());
	ShownSeats.erase(std::unique(ShownSeats.begin(), ShownSeats.end()), ShownSeats.end()); // turned up at the all-in and again at the showdown
	const int HandNo = F.Skipped ? T->Tick : T->Tick + 1;
	const recall::Record Rec = recall::Of(H, HeroSeat->Seat, Names, ShownSeats, HandNo, T->LevelIndex + 1);
	// The equity at the all-in, on the board as it stood at the reveal. The skip path's figure is that (autoplay's
	// AllInEquity). The watch path's isn't: the table keeps its equities current street by street through the run-out
	// (Session.cpp's Street case), so by the award they're the river's, 1 or 0, and a bad beat would never be one. Watched
	// hands get the same computation, stateless, here.
	double Eq = F.Skipped ? F.EquityAtAllIn : -1.0;
	if (Eq < 0.0 && Reveal && !HeroSeat->Folded)
	{
		std::vector<std::vector<Card>> Theirs;
		for (const HandSeat& S : H.Seats)
		{
			if (S.Id != HeroId && !S.Folded && S.Hole.size() >= 2)
			{
				Theirs.push_back(S.Hole);
			}
		}
		const std::vector<Card> AllInBoard(H.Board.begin(), H.Board.begin() + static_cast<std::ptrdiff_t>(std::min(BoardAtReveal, H.Board.size())));
		Eq = video::video_detail::EquityAt(HeroSeat->Hole, Theirs, AllInBoard, T->Spec.Id + ":" + std::to_string(HandNo));
	}
	const double Bb = static_cast<double>(std::max<Chips>(1, H.BigBlind));
	const double AvgBb = T->AverageStack() / Bb;
	bool HeroAllIn = HeroSeat->AllIn;
	// Hand for hand, as the hand was dealt. The watch path's hook runs before the eliminations (the tournament's state is
	// the hand's); the skip path's after them, so the bursting hand already reads as in the money there: it's the one where
	// somebody at this table went out short of the money. (A hand after the burst is never the bubble.)
	bool BurstHere = false;
	for (const HandSeat& S : H.Seats)
	{
		const TPlayer* P = S.Stack > 0 || S.Folded ? nullptr : PlayerById(S.Id);
		BurstHere = BurstHere || (P && P->Busted && P->Place > T->PaidPlaces());
	}
	const bool BubbleZone = T->HandForHand() || (T->InTheMoney() && HeroAllIn && BurstHere);
	bool Is = false;
	video::Kind K = video::Classify(Rec, Eq, BubbleZone, Showdown, AvgBb, Is);
	// The opponent most in it with the player: a hand turned up first, else whoever put the most in.
	const HandSeat* Villain = nullptr;
	bool VillainShown = false;
	for (const HandSeat& S : H.Seats)
	{
		if (S.Id == HeroId)
		{
			continue;
		}
		const bool ShownHere = !S.Folded && std::find(ShownSeats.begin(), ShownSeats.end(), S.Seat) != ShownSeats.end();
		if (!Villain || (ShownHere && !VillainShown) || (ShownHere == VillainShown && S.Committed > Villain->Committed))
		{
			Villain = &S;
			VillainShown = ShownHere;
		}
	}
	const double PotBb = static_cast<double>(Rec.Pot) / Bb;
	// The rival (§4.4: any pot of 20 BB or more with gh0stfold in it), a knockout, a name people know.
	const bool RivalIn = Villain && Villain->Id == RivalId() && Villain->Committed >= 2 * H.BigBlind && HeroSeat->Committed >= 2 * H.BigBlind;
	if (RivalIn && PotBb >= 20.0 && (!Is || video::KindWeight(video::Kind::Rival) > video::KindWeight(K)))
	{
		K = video::Kind::Rival;
		Is = true;
	}
	bool KnockedOut = false;
	for (const HandSeat& S : H.Seats)
	{
		if (S.Id == HeroId || S.Stack > 0 || S.Folded)
		{
			continue;
		}
		for (const PotResult& P : H.PotResults)
		{
			const bool Eligible = std::find(P.Eligible.begin(), P.Eligible.end(), S.Seat) != P.Eligible.end();
			const bool HeroWon = std::find(P.Winners.begin(), P.Winners.end(), HeroSeat->Seat) != P.Winners.end();
			KnockedOut = KnockedOut || (Eligible && HeroWon);
		}
	}
	if (KnockedOut && (!Is || video::KindWeight(video::Kind::Knockout) > video::KindWeight(K)))
	{
		K = video::Kind::Knockout;
		Is = true;
	}
	// Rerun 2.0's kinds that need the event's context (§6.3 A13, A20, A21, A22, A24; core-videocontent), on a channel that
	// runs it: two players out in one pot of the player's, a strong hand folded near a pay jump, the money made from a short
	// stack, the heads-up for the title, the hand the player went out on. Their facts ride in IdeaFacts (the title's slots).
	std::string ContextFacts;
	if (Life.Video.X.On)
	{
		auto Take = [&](video::Kind Nk) {
			if (!Is || video::KindWeight(Nk) > video::KindWeight(K))
			{
				K = Nk;
				Is = true;
				return true;
			}
			return false;
		};
		int Busted = 0;
		for (const HandSeat& S : H.Seats)
		{
			if (S.Id == HeroId || S.Stack > 0 || S.Folded)
			{
				continue;
			}
			bool ByHero = false;
			for (const PotResult& P : H.PotResults)
			{
				ByHero = ByHero || (std::find(P.Eligible.begin(), P.Eligible.end(), S.Seat) != P.Eligible.end() &&
									   std::find(P.Winners.begin(), P.Winners.end(), HeroSeat->Seat) != P.Winners.end());
			}
			Busted += ByHero ? 1 : 0;
		}
		const int Hi = std::max(RankOf(HeroSeat->Hole[0]), RankOf(HeroSeat->Hole[1])), Lo = std::min(RankOf(HeroSeat->Hole[0]), RankOf(HeroSeat->Hole[1]));
		const bool Strong = (Hi == Lo && Hi >= 8) || (Hi == 12 && Lo >= 11); // tens or better, ace-king or ace-queen
		const bool NearJump = T->HandForHand() || (T->InTheMoney() && T->Remaining <= T->TableSize);
		const bool HeroOut = HeroSeat->Stack <= 0 && !HeroSeat->Folded;
		const double StackBb = static_cast<double>(HeroSeat->Stack) / Bb;
		if (Busted >= 2 && Take(video::Kind::DoubleKo))
		{
			ContextFacts.clear();
		}
		if (HeroSeat->Folded && Strong && NearJump && HeroSeat->Committed < 4 * H.BigBlind && Take(video::Kind::LadderFold))
		{
			ContextFacts = "Left=" + std::to_string(T->Remaining);
		}
		if (BurstHere && !HeroOut && StackBb > 0.0 && StackBb <= 5.0 && Take(video::Kind::MinCash))
		{
			ContextFacts = "StackBb=" + std::to_string(static_cast<int>(std::max(1.0, std::round(StackBb))));
		}
		if (T->Remaining == 2 && HeroAllIn && Take(video::Kind::HeadsUp))
		{
			ContextFacts.clear();
		}
		if (HeroOut && HeroAllIn && T->Spec.Entrants > 1 && Take(video::Kind::LastHand))
		{
			ContextFacts.clear();
		}
		// A mystery bounty's envelope (A16): a knockout in the money. Its multiple is known once the envelope opens (the
		// elimination's, which on the watch path comes after this hook): VideoStep fills Mult and Bounty from it.
		if (KnockedOut && T->Spec.MysteryBounty && T->InTheMoney() && T->Spec.BountyCents > 0 && Take(video::Kind::Envelope))
		{
			ContextFacts = "envbase=" + std::to_string(static_cast<long long>(T->Spec.BountyCents));
		}
	}
	if (!Is)
	{
		return;
	}
	video::Candidate C;
	C.Id = SitePrefix(T->Spec.Id) + T->Spec.Id + ":" + std::to_string(HandNo);
	C.At = WorldMinutes();
	C.Source = F.Skipped ? 3 : 0;
	C.Site = static_cast<int>(net::SiteOf(T->Spec.Id));
	C.K = K;
	C.Rec = Rec;
	C.EquityAtAllIn = Eq;
	C.IdeaFacts = ContextFacts;
	if (Villain)
	{
		const TPlayer* P = PlayerById(Villain->Id);
		C.Opponent = P ? P->Name : std::string();
		const auto Npc = FieldNpc.find(Villain->Id);
		C.OpponentNpc = Npc != FieldNpc.end() ? Npc->second : -1;
		const world::Npc* Who = C.OpponentNpc >= 0 ? LivingWorld.Get(C.OpponentNpc) : nullptr;
		C.Rival = RivalIn || (Who && Who->RepOf(world::Rep::Overall) >= 35.0f);
	}
	C.Event = T->Spec.Name;
	C.BuyIn = T->Spec.BuyInCents;
	C.Field = T->Spec.Entrants;
	C.PlaceSoFar = T->HeroRank();
	C.Bubble = BubbleZone;
	C.Final = T->Spec.Entrants > T->TableSize && T->Remaining <= T->TableSize;
	C.Series = SeriesEvent(*T);
	C.Auto = F.ByPlan;
	C.Style = F.ByPlan ? static_cast<int>(Auto.S) : 0;
	// The stream around it (§4.4's add-on): the moments this hand fired, linked to the candidate.
	if (Stream.Live)
	{
		C.Stream.Viewers = static_cast<int>(std::lround(Stream.Viewers));
		C.Stream.HypeAfter = Stream.Hype;
		C.Stream.Face = static_cast<int>(Stream.Face);
		// This hand's moments only: the tick clock stands still through a hand and moves a hand's length after it, so a
		// window under half a hand never reaches the last hand's (its chat, its clip) and claims them for this one.
		const double World = WorldMinutes();
		const double Window = std::min(1.5, 0.5 * T->Spec.SecondsPerHand / 60.0);
		for (kast::MomentNote& M : Stream.Moments)
		{
			// (The hand's own moments only, never the tick's, which land on this hand's clock from the last tick: VideoHandMoment.)
			if (!M.Candidate.empty() || M.World < World - Window || !VideoHandMoment(M.Kind))
			{
				continue;
			}
			M.Candidate = C.Id;
			C.Stream.Viewers = std::max(C.Stream.Viewers, M.Viewers);
			C.Stream.HypeAfter = std::max(C.Stream.HypeAfter, M.HypeAfter);
			C.Stream.Face = static_cast<int>(M.Face);
			for (const std::string& Line : M.Chat)
			{
				if (C.Stream.Chat.size() < 3)
				{
					C.Stream.Chat.push_back(Line);
				}
			}
			C.Stream.ClipAt = C.Stream.ClipAt >= 0.0 ? C.Stream.ClipAt : M.ClipAt;
		}
	}
	C.Expires = C.At + 21.0 * net::MinutesPerDay;
	Touch(Life, TextsSeen, Evicted());
	video::NoteHand(Life.Video, C);
}

void Session::VideoRecap(const Results& Res)
{
	using namespace session_video_detail;
	if (!T || Res.Place <= 0)
	{
		return;
	}
	const bool Won = Res.Won;
	const bool Final = Res.Place <= T->TableSize && T->Spec.Entrants > T->TableSize;
	const bool Deep = Res.PrizeCents > 0 && Res.PrizeCents >= 3 * std::max<Chips>(1, Res.BuyInCents);
	if (!Won && !Final && !Deep)
	{
		return;
	}
	video::Candidate C;
	C.Id = SitePrefix(T->Spec.Id) + T->Spec.Id + ":recap";
	C.At = WorldMinutes();
	C.Source = Sprinting || Auto.M == autoplay::Mode::Skip ? 3 : 0;
	C.Site = static_cast<int>(net::SiteOf(T->Spec.Id));
	C.K = Won ? video::Kind::Win : Final ? video::Kind::FinalTable : video::Kind::DeepRun;
	// The event, not a hand: the record holds the money (Video.h's conventions).
	C.Rec.Level = T->LevelIndex + 1;
	C.Rec.Pot = Res.PrizeCents;
	C.Rec.HeroNet = Res.PrizeCents - Res.BuyInCents;
	C.Event = T->Spec.Name;
	C.BuyIn = T->Spec.BuyInCents;
	C.Field = T->Spec.Entrants;
	C.PlaceSoFar = Res.Place;
	C.Final = Final || Won;
	C.Series = SeriesEvent(*T);
	C.Auto = Auto.M != autoplay::Mode::Off;
	C.Style = C.Auto ? static_cast<int>(Auto.S) : 0;
	if (Stream.Live)
	{
		C.Stream.Viewers = static_cast<int>(std::lround(Stream.Viewers));
		C.Stream.HypeAfter = Stream.Hype;
		C.Stream.Face = static_cast<int>(Stream.Face);
	}
	C.Expires = C.At + 21.0 * net::MinutesPerDay;
	Touch(Life, TextsSeen, Evicted());
	video::NoteHand(Life.Video, C);
}

std::vector<video::Candidate> Session::VideoCandidates() const
{
	// Studio's list: the best first (Drama), the newest of a tie.
	std::vector<video::Candidate> Out = Life.Video.Open;
	std::stable_sort(Out.begin(), Out.end(), [](const video::Candidate& A, const video::Candidate& B) { return A.Drama != B.Drama ? A.Drama > B.Drama : A.At > B.At; });
	return Out;
}

std::string Session::Publish(const video::Choice& C)
{
	using namespace session_video_detail;
	if (TimeSkip.Active)
	{
		return video::PublishWhy(1);
	}
	if (T || TableCount() > 0)
	{
		return video::PublishWhy(2);
	}
	if (Stream.Live)
	{
		return video::PublishWhy(3);
	}
	if (Life.Energy < 10.0)
	{
		return video::PublishWhy(4);
	}
	const std::string Why = video::CanUpload(C, Life.Video.Open);
	if (!Why.empty())
	{
		return Why;
	}
	// Studio's CAPTION, FACE and PALETTE pickers (§6.6.2) change a recipe only inside its archetype's values on this moment: a
	// contender outside them isn't one of the moment's looks (and its caption could say what didn't happen).
	if (C.F != video::Format::Short)
	{
		for (const video::ThumbSpec& Th : C.Thumbs)
		{
			if (!video::ThumbWithin(Th, C, Life.Video.Open, Life.Video))
			{
				return video::vcontent::ThumbNotALook();
			}
		}
	}
	const double World = WorldMinutes();
	// Update 3 (core-videocontent): a Studio choice's edit is its length option's, plus a Duel's contenders and Clean edit.
	const bool U3 = !C.TitleTpl.empty() || !C.Thumbs.empty();
	const double Minutes = video::EditMinutesOf(C);
	// The moments can't go stale under the editor's hands.
	for (video::Candidate& X : Life.Video.Open)
	{
		if (std::find(C.Candidates.begin(), C.Candidates.end(), X.Id) != C.Candidates.end())
		{
			X.Expires = std::max(X.Expires, World + Minutes + 24.0 * 60.0);
		}
	}
	// A TimeSkip like a shift (§4.5): the clock races, the calendar runs, 5 energy an hour.
	TimeSkip = Skip();
	TimeSkip.Active = true;
	TimeSkip.From = World;
	TimeSkip.To = World + Minutes;
	TimeSkip.RealStart = Now;
	TimeSkip.RealSeconds = std::min(5.0, 2.0 + Minutes / 60.0 * 0.6);
	TimeSkip.Rest = 1.0;
	TimeSkip.Couch = Evicted();
	TimeSkip.Label = video::video_copy::EditLabel(C.F);
	TimeSkip.Result.ActivityId = EditId(C);
	TimeSkip.Result.Start = World;
	TimeSkip.Result.End = TimeSkip.To;
	TimeSkip.Result.Energy = -video::EditEnergyOf(C);
	TimeSkip.Result.Title = video::video_copy::UploadedTitle();
	TimeSkip.Result.Body = video::Title(C, Life.Video.Open, 0);
	if (U3)
	{
		// The title Studio picked: its template's text over these moments (the Shuffle-th deal and the next ones).
		for (int S = 0; S < 8; ++S)
		{
			bool Found = false;
			for (const video::TitleOption& O : video::TitleOptions(C, Life.Video.Open, Life.Video, C.Shuffle + S, 3))
			{
				if (O.Key == C.TitleTpl)
				{
					TimeSkip.Result.Body = O.Text;
					Found = true;
				}
			}
			if (Found)
			{
				break;
			}
		}
	}
	HasOutcome = false;
	ConfirmRegister = false;
	Touch(Life, TextsSeen, Evicted());
	// Update 2 (§5.7, core-deals): a video that shows a site's table (an online hand, watched or skipped) is a strike under a
	// deal that plays only on another site; at most one a day (sponsor::NoteSitePlay). Studio's PUBLISH warned first
	// (sponsor::WouldStrike with Video); this is the second press. Live and story candidates show no site.
	for (const video::Candidate& X : Life.Video.Open)
	{
		if ((X.Source == 0 || X.Source == 3) && std::find(C.Candidates.begin(), C.Candidates.end(), X.Id) != C.Candidates.end())
		{
			sponsor::NoteSitePlay(Life.Sponsors, Life, X.Site, World, false, true);
		}
	}
	Sound(SoundId::Click, 0.6);
	return "";
}

std::string Session::SettleEdit(const std::string& ActivityId)
{
	using namespace session_video_detail;
	video::Choice C;
	if (!ReadEditId(ActivityId, C))
	{
		return video::video_copy::WhyNot(0);
	}
	const std::vector<video::Candidate> Picked = video::Picked(C, Life.Video.Open);
	const std::string Why = video::CanUpload(C, Life.Video.Open);
	if (!Why.empty())
	{
		LastOutcome.Title = video::video_copy::EditFailedTitle();
		LastOutcome.Body = Why;
		return Why;
	}
	video::UploadFacts F;
	F.GearQuality = Fx.Quality;
	F.Sharp = static_cast<int>(std::lround(Sharp() * 100.0));
	F.KastFollowers = Channel.Followers;
	F.Energy = Life.Energy;
	const double World = WorldMinutes();
	// Rerun 2.0 (core-videoalgo's patch): the review waits for the second rent (W11); burnout reads Update 3's extra editing. A
	// career whose save has Update 3's video lines on from its start (NewCareer's) uploads Rerun 2.0 videos from its first; an
	// older channel's first upload with a recipe or a title template turns it on itself (video::Upload).
	F.RentsPaid = Life.RentsPaid;
	F.ExtraHours7 = video::algo::ExtraHours7(Life.Video, World);
	if ((Life.Update3LinesRead & life::U3Video) != 0 && !Life.Video.X.On && Life.Video.Videos.empty())
	{
		Life.Video.X.On = true;
	}
	const std::string Id = video::Upload(Life.Video, C, World, SeedBase, F);
	if (Id.empty())
	{
		LastOutcome.Title = video::video_copy::EditFailedTitle();
		LastOutcome.Body = video::video_copy::WhyNot(1);
		return LastOutcome.Body;
	}
	// Update 3 (core-videocontent): an episode of a series is counted (its playlist, the challenge's state) once it's up.
	session_video3_detail::AfterUpload(Life, History, LivingWorld, Channel, Gear, TextsSeen, BankrollCents, HeroName, Evicted(), World, C, Picked, Id);
	// The face its thumbnails were made with stays theirs (§6.1, ChannelX::Looks): the look now, frozen once per look.
	for (video::Video& V : Life.Video.Videos)
	{
		if (V.Id != Id || !V.X.On || V.X.Spec[0].Arch < 0)
		{
			continue;
		}
		std::array<int16_t, 14> Look{};
		for (int K = 0; K < hero::SlotCount && K < 14; ++K)
		{
			Look[static_cast<size_t>(K)] = static_cast<int16_t>(Person.Appearance.Get(static_cast<hero::Slot>(K)));
		}
		int Ref = -1, Next = 0;
		for (const video::FrozenLook& Fz : Life.Video.X.Looks)
		{
			Ref = Fz.Look == Look ? Fz.Ref : Ref;
			Next = std::max(Next, Fz.Ref + 1);
		}
		if (Ref < 0)
		{
			if (Life.Video.X.Looks.size() >= 8)
			{
				Life.Video.X.Looks.erase(Life.Video.X.Looks.begin()); // the oldest look goes: its videos draw today's face
			}
			video::FrozenLook Fz;
			Fz.Ref = Next;
			Fz.Look = Look;
			Life.Video.X.Looks.push_back(Fz);
			Ref = Next;
		}
		for (video::ThumbSpec& S : V.X.Spec)
		{
			S.LookRef = S.Arch >= 0 ? static_cast<int16_t>(Ref) : S.LookRef;
		}
	}
	Life.Update2LinesRead |= life::U2Video;
	Life.Update3LinesRead |= Life.Video.X.On ? static_cast<uint32_t>(life::U3Video) : 0u;
	std::string Title;
	for (const video::Video& V : Life.Video.Videos)
	{
		Title = V.Id == Id ? V.Title : Title;
	}
	LastOutcome.Title = video::video_copy::UploadedTitle();
	LastOutcome.Body = video::video_copy::UploadedBody(Title);
	bool Storytime = false;
	for (const video::Candidate& X : Picked)
	{
		Storytime = Storytime || X.Storytime;
		// A Kast clip of the same moment stops bringing its own follows (§4.7: no double count): its views stop where they are.
		if (X.Stream.ClipAt >= 0.0)
		{
			for (kast::Clip& K : Channel.Clips)
			{
				if (std::fabs(K.At - X.Stream.ClipAt) < 0.01 && K.Candidate.empty())
				{
					K.Reach = K.Views;
					K.Candidate = X.Id;
				}
			}
		}
	}
	if (Storytime)
	{
		StoryText("rerun-storytime", "Dee", video::video_copy::DeeStorytime());
	}
	return "";
}

std::string Session::DeleteVideo(const std::string& VideoId)
{
	using namespace session_video_detail;
	std::vector<video::Video>& Videos = Life.Video.Videos;
	const auto It = std::find_if(Videos.begin(), Videos.end(), [&VideoId](const video::Video& V) { return V.Id == VideoId; });
	if (It == Videos.end())
	{
		return video::video_copy::NoSuchVideo();
	}
	// Its views stop; the subscribers it brought in stay (they're in the channel's count already).
	Videos.erase(It);
	Touch(Life, TextsSeen, Evicted());
	Save();
	return "";
}

video::ChannelView Session::VideoView() const
{
	return video::View(Life.Video, WorldMinutes());
}

void Session::VideoStep(double From, double To)
{
	using namespace session_video_detail;
	// CheckCalendar's step after the sponsors' (§1.6): views, subscribers, the review and the monthly payout. Update 3
	// (§4.6.10): with the session's world, whose log the week's trends read.
	video::Step(Life.Video, Life, BankrollCents, Channel, From, To, &LivingWorld);
	// An envelope's multiple (core-videocontent): the knockout's envelope opened after the hand was captured.
	if (LastBountyCents > 0 && !LastBountyName.empty())
	{
		for (video::Candidate& X : Life.Video.Open)
		{
			const size_t At = X.IdeaFacts.find("envbase=");
			if (X.K != video::Kind::Envelope || At == std::string::npos || X.IdeaFacts.find("Mult=") != std::string::npos || X.Opponent != LastBountyName)
			{
				continue;
			}
			const double Base = std::atof(X.IdeaFacts.c_str() + At + 8);
			if (Base > 0.0)
			{
				const long long Mult = std::llround(static_cast<double>(LastBountyCents) / Base);
				X.IdeaFacts += ";Mult=" + std::to_string(std::max(1LL, Mult)) + ";Bounty=" + video::vcontent::FactEsc(Money(LastBountyCents));
			}
		}
	}
	if ((Life.Update2LinesRead & life::U2Video) == 0)
	{
		return; // the channel has nothing yet: the stories start once it does (Touch)
	}
	// The card room may have given the channel its first state (a live hand): the stories before it are told already.
	MarkOldStories(Life, TextsSeen, Evicted());
	// Life stories (§4.4): a beat the session told once becomes a candidate once.
	for (const char* Key : StoryKeys)
	{
		if (TextsSeen.count(Key) == 0 || Life.Video.Paid.count(std::string("story:") + Key) > 0)
		{
			continue;
		}
		if (std::string(Key) == "rent-paid" && !RentPaidByPoker(Life))
		{
			Life.Video.Paid.insert(std::string("story:") + Key); // told, with no story in it: poker didn't pay that rent
			continue;
		}
		video::NoteStory(Life.Video, Key, To);
	}
	if (Life.Evictions > 0 && !Evicted())
	{
		video::NoteStory(Life.Video, BackKey(Life.Evictions), To);
	}
}

std::string Session::DebugVideo(const std::string& Args)
{
	using namespace session_video_detail;
	const std::vector<std::string> W = Words(Args);
	const std::string Verb = W.empty() ? std::string("describe") : W[0];
	const double World = WorldMinutes();
	const video::Channel& Ch = Life.Video;
	if (Verb == "describe")
	{
		const video::ChannelView V = VideoView();
		const double Buzz = video::Buzz(Ch, World);
		std::string Out = "Rerun: " + Grouped(static_cast<int64_t>(V.Subs)) + " subscribers, " + Grouped(static_cast<int64_t>(video::TotalViews(Ch, World))) + " views, " +
			std::to_string(Ch.Videos.size()) + " videos (" + std::to_string(Ch.Stubs.size()) + " evergreen stubs), " + std::to_string(Ch.Open.size()) + " candidates.";
		Out += "\nGate: subscribers " + Grouped(static_cast<int64_t>(V.Subs)) + " / 1,000; watch hours " + Grouped(static_cast<int64_t>(V.WatchHours365)) + " / 4,000.";
		Out += std::string("\nPartner: ") + (V.Partner ? "yes, since " + net::DateLabel(net::DayOf(Ch.PartnerAt)) : Ch.ReviewAt >= 0.0 ? "under review" : "not yet") +
			". Balance " + Money(Ch.BalanceCents) + ", paid out " + Money(Ch.PaidCents) + ".";
		Out += "\nLast 28 days: " + Grouped(static_cast<int64_t>(V.Views28)) + " views. Last 48 hours: " + Grouped(static_cast<int64_t>(Buzz)) + " views (tonight's strangers x" +
			Fixed(1.0 + 0.3 * std::min(1.0, Buzz / 5000.0), 2) + ").";
		Out += "\nKast term in reach: +" + Grouped(static_cast<int64_t>(0.25 * std::min(20000.0, static_cast<double>(Channel.Followers)))) + " (" +
			Grouped(Channel.Followers) + " Kast followers). Stepped to " + net::DateLabel(net::DayOf(Ch.SteppedTo)) + ".";
		return Out;
	}
	if (Verb == "candidates")
	{
		std::string Out = std::to_string(Ch.Open.size()) + " candidates:";
		int N = 0;
		for (const video::Candidate& C : VideoCandidates())
		{
			video::Choice One;
			One.Candidates = {C.Id};
			Out += "\n" + std::to_string(++N) + ". " + C.Id + "  " + video::KindLabel(C.K) + "  drama " + Fixed(C.Drama, 2) + "  " +
				Fixed(std::max(0.0, (C.Expires - World) / net::MinutesPerDay), 1) + " days left" + (C.Auto ? "  AUTO" : "") + (C.Storytime ? "  storytime" : "") + "  \xE2\x80\x9C" +
				video::Title(One, Ch.Open, 0) + "\xE2\x80\x9D";
		}
		return Out;
	}
	if (Verb == "fake")
	{
		static const char* const Kinds[] = {"badbeat", "suckout", "cooler", "bluff", "bluffcaught", "herocall", "herofold", "bigpot", "knockout", "bubble", "deeprun", "finaltable", "win", "rival"};
		int K = -1;
		for (int I = 0; I < 14; ++I)
		{
			K = W.size() > 1 && W[1] == Kinds[I] ? I : K;
		}
		const std::string Where = W.size() > 2 ? W[2] : std::string("online");
		const int Source = Where == "embercrest" ? 1 : Where == "dees" ? 2 : Where == "skip" ? 3 : Where == "online" ? 0 : -1;
		if (K < 0 || Source < 0)
		{
			return "ss.Video.Fake <badbeat|suckout|cooler|bluff|bluffcaught|herocall|herofold|bigpot|knockout|bubble|deeprun|finaltable|win|rival> <online|embercrest|dees|skip>";
		}
		video::Candidate C = Fake(static_cast<video::Kind>(K), Source, World);
		if (Stream.Live && (Source == 0 || Source == 3))
		{
			C.Stream.Viewers = static_cast<int>(std::lround(Stream.Viewers));
			C.Stream.Face = static_cast<int>(Stream.Face);
		}
		Touch(Life, TextsSeen, Evicted());
		video::NoteHand(Life.Video, C);
		Save();
		return "Faked " + C.Id + " (" + video::KindLabel(C.K) + ").";
	}
	if (Verb == "upload")
	{
		if (W.size() < 5)
		{
			return "ss.Video.Upload <candidate id or number> <short|breakdown|vlog|recap> <story|hype|teaching|question|plain> <reaction|cards|number|versus|trophy>";
		}
		video::Choice C;
		const std::vector<video::Candidate> List = VideoCandidates();
		for (const std::string& Pick : Words([&]() {
				 std::string S = W[1];
				 std::replace(S.begin(), S.end(), ',', ' ');
				 return S;
			 }()))
		{
			const int Index = std::atoi(Pick.c_str());
			C.Candidates.push_back(Index >= 1 && Index <= static_cast<int>(List.size()) && std::to_string(Index) == Pick ? List[static_cast<size_t>(Index - 1)].Id : Pick);
		}
		const int F = NamedIndex(W[2], FormatAt, 4), T_ = NamedIndex(W[3], StyleAt, 5), Fr = NamedIndex(W[4], FrameAt, 5);
		if (F < 0 || T_ < 0 || Fr < 0)
		{
			return "Unknown format, title style or frame.";
		}
		C.F = static_cast<video::Format>(F);
		C.T = static_cast<video::TitleStyle>(T_);
		C.Fr = static_cast<video::Frame>(Fr);
		const std::string Why = Publish(C);
		return Why.empty() ? "Editing: \xE2\x80\x9C" + video::Title(C, Ch.Open, 0) + "\xE2\x80\x9D (" + Fixed(video::EditMinutes(C.F), 0) + " minutes)." : Why;
	}
	if (Verb == "advance")
	{
		const int Days = W.size() > 1 ? std::atoi(W[1].c_str()) : 0;
		if (Days <= 0)
		{
			return "ss.Video.Advance <days>";
		}
		if (T)
		{
			return "Not with tables open.";
		}
		const double From = World;
		WorldSkip(Days);
		VideoStep(From, WorldMinutes());
		Save();
		return "Advanced " + std::to_string(Days) + " days. " + DebugVideo("describe");
	}
	if (Verb == "grant")
	{
		const double Subs = W.size() > 1 ? std::atof(W[1].c_str()) : 0.0;
		const double Hours = W.size() > 2 ? std::atof(W[2].c_str()) : 0.0;
		const double Views = W.size() > 3 ? std::atof(W[3].c_str()) : 0.0;
		Touch(Life, TextsSeen, Evicted());
		video::Grant(Life.Video, Subs, Hours, Views, World);
		Save();
		return "Granted. " + DebugVideo("describe");
	}
	if (Verb == "delete" && W.size() > 1)
	{
		const std::string Why = DeleteVideo(W[1]);
		return Why.empty() ? video::Deleted() : Why;
	}
	// Update 3 (§7.6): Rerun 2.0's levers and numbers through the session's own methods (their bodies are core-videocontent's
	// and core-videoalgo's, G3); the rest of the new verbs land with those tracks.
	if (Verb == "ads")
	{
		static const char* const Loads[] = {"light", "standard", "heavy"};
		for (int I = 0; W.size() > 2 && I < static_cast<int>(video::AdLoad::Count); ++I)
		{
			if (W[2] == Loads[I])
			{
				const std::string Why = SetAdLoad(W[1], static_cast<video::AdLoad>(I));
				return Why.empty() ? "Ads on " + W[1] + ": " + Loads[I] + ", from the next midnight." : Why;
			}
		}
		return "ss.Video.Ads <id> <light|standard|heavy>";
	}
	if (Verb == "launch")
	{
		static const char* const Acts[] = {"cut", "kast", "teaser", "reply"};
		for (int I = 0; W.size() > 2 && I < static_cast<int>(video::Launch::Count); ++I)
		{
			if (W[2] == Acts[I])
			{
				const std::string Why = LaunchAction(W[1], static_cast<video::Launch>(I));
				return Why.empty() ? std::string("Launch action: ") + Acts[I] + " for " + W[1] + "." : Why;
			}
		}
		return "ss.Video.Launch <id> <cut|kast|teaser|reply>";
	}
	if (Verb == "stats" && W.size() > 1)
	{
		const video::VideoStats V = VideoStatsOf(W[1]);
		return W[1] + (V.Estimated ? " (est.)" : "") + ": " + Grouped(static_cast<int64_t>(V.Views)) + " views, " + Grouped(static_cast<int64_t>(V.Impressions)) +
			" impressions, CTR " + Fixed(100.0 * V.CtrLife, 1) + "%, " + Fixed(V.AvdMin, 1) + " min average view, " + Grouped(static_cast<int64_t>(V.Likes)) + " likes, " +
			Grouped(static_cast<int64_t>(V.Dislikes)) + " dislikes, " + Grouped(static_cast<int64_t>(V.Comments)) + " comments, " + Grouped(static_cast<int64_t>(V.SubsNet)) +
			" subscribers net, " + Money(V.EarnedCents) + " earned (RPM " + Fixed(V.Rpm, 2) + ", CPM " + Fixed(V.Cpm, 2) + "), verdict " + std::to_string(V.Verdict) + ".";
	}
	if (Verb == "ideas")
	{
		const std::vector<video::IdeaCard> Cards = VideoIdeas();
		std::string Out = std::to_string(Cards.size()) + " ideas:";
		for (const video::IdeaCard& I : Cards)
		{
			Out += "\n" + std::to_string(I.Id) + ". " + I.Name + (I.Ready ? "" : "  locked: " + I.Hint) + (I.Strike ? "  (a strike)" : "");
		}
		return Out;
	}
	// §7.6's Update 3 verbs (core-videocontent): the ideas, a check-in's score, the week's trends, the contact sheet, a
	// repackage and a Duel without the laptop.
	if (Verb == "idea" && W.size() > 1)
	{
		const std::string Why = ChooseIdea(std::atoi(W[1].c_str()));
		return Why.empty() ? "Idea " + W[1] + " is in MOMENTS (" + video::IdeaName(std::atoi(W[1].c_str())) + ")." : Why;
	}
	if (Verb == "checkin")
	{
		std::string Out;
		for (const video::Video& V : Ch.Videos)
		{
			if (!V.X.On || (W.size() > 1 && V.Id != W[1]))
			{
				continue;
			}
			// The decision midnights (§4.6.8): the verdict's (the first after 48 hours), day 7's and day 14's.
			const int Up = net::DayOf(V.At);
			const int Verdict = net::DayOf(V.At + 48.0 * 60.0) + 1;
			const int Today = net::DayOf(World);
			const int Next = Today < Verdict ? Verdict : Today < Up + 7 ? Up + 7 : Today < Up + 14 ? Up + 14 : -1;
			const int Stage = Today < Verdict ? 1 : Today < Up + 7 ? 2 : 3;
			const video::algo::Score S = video::algo::ScoreAt(V, Ch, World, Stage);
			Out += (Out.empty() ? "" : "\n") + V.Id + ": " + (Next < 0 ? std::string("no decision left") : "next decision " + net::DateLabel(Next)) + ", score " +
				Fixed(S.S, 2) + " (CTR " + Fixed(S.Ctr, 2) + ", quality " + Fixed(S.Q, 2) + ", viewers " + Fixed(S.V, 2) + ", trend " + Fixed(S.Tr, 2) + ", season " +
				Fixed(S.Se, 2) + ").";
		}
		return Out.empty() ? std::string("No Rerun 2.0 video to check.") : Out;
	}
	if (Verb == "trends")
	{
		std::string Out;
		for (const video::Trend& Tr : video::Trends(Life, LivingWorld, World))
		{
			Out += (Out.empty() ? "" : "\n") + Tr.Label + "  heat " + Fixed(Tr.Heat, 2) + "  x" + Fixed(Tr.Strength, 2) + "  (yours " + std::to_string(Tr.Mine) + ", others " +
				std::to_string(Tr.Others) + ")" + (Tr.Limited ? "  limited ads" : "") + (Tr.Strike ? "  a strike" : "");
		}
		return Out.empty() ? std::string("Nothing is trending on Rerun this week.") : Out;
	}
	if (Verb == "sheet")
	{
		static const char* const Groups[] = {"beats", "reads", "pots", "runs", "rival", "stories", "ideas"};
		int G = -1;
		for (int I = 0; I < 7; ++I)
		{
			G = W.size() > 1 && W[1] == Groups[I] ? I : G;
		}
		const int N = W.size() > 2 ? std::clamp(std::atoi(W[2].c_str()), 1, 48) : 24;
		if (G < 0)
		{
			return "ss.Video.Sheet <beats|reads|pots|runs|rival|stories|ideas> <n>";
		}
		std::string Out = W[1] + ": ";
		std::set<int> Archs;
		const std::vector<video::ThumbSpec> Sheet = video::ContactSheet(G, N);
		for (const video::ThumbSpec& S : Sheet)
		{
			Archs.insert(S.Arch);
		}
		Out += std::to_string(Sheet.size()) + " thumbnails, " + std::to_string(Archs.size()) + " archetypes (the laptop draws the sheet).";
		return Out;
	}
	if (Verb == "repack")
	{
		if (W.size() < 4 || (W[2] != "title" && W[2] != "thumb"))
		{
			return "ss.Video.Repack <id> <title|thumb> <n>";
		}
		const video::Video* V = nullptr;
		for (const video::Video& X : Ch.Videos)
		{
			V = X.Id == W[1] ? &X : V;
		}
		if (!V)
		{
			return video::video_copy::NoSuchVideo();
		}
		const int N = std::max(1, std::atoi(W[3].c_str()));
		video::Repack Rp;
		if (W[2] == "title")
		{
			const std::vector<video::TitleOption> Os = video::RepackTitles(*V, Ch, 0, 3);
			if (N > static_cast<int>(Os.size()))
			{
				return "There are " + std::to_string(Os.size()) + " title options.";
			}
			Rp.TitleKey = Os[static_cast<size_t>(N - 1)].Key;
		}
		else
		{
			const std::vector<video::ThumbSpec> Os = video::RepackThumbs(*V, Ch, 0, 6);
			if (N > static_cast<int>(Os.size()))
			{
				return "There are " + std::to_string(Os.size()) + " thumbnail options.";
			}
			Rp.Spec = Os[static_cast<size_t>(N - 1)];
		}
		const std::string Id = V->Id;
		const std::string Why = Repackage(Id, Rp);
		if (!Why.empty())
		{
			return Why;
		}
		for (const video::Video& X : Ch.Videos)
		{
			if (X.Id == Id)
			{
				return "Repackaged " + Id + ": \xE2\x80\x9C" + X.Title + "\xE2\x80\x9D, " + video::ArchName(X.X.Spec[0].Arch) + ".";
			}
		}
		return "Repackaged.";
	}
	if (Verb == "duel")
	{
		// The Thumbnail Duel without the laptop: a candidate's Breakdown with Studio's first title and three recipes.
		if (W.size() < 2)
		{
			return "ss.Video.Duel <candidate id or number>";
		}
		const std::vector<video::Candidate> List = VideoCandidates();
		const int Index = std::atoi(W[1].c_str());
		video::Choice C;
		C.Candidates = {Index >= 1 && Index <= static_cast<int>(List.size()) && std::to_string(Index) == W[1] ? List[static_cast<size_t>(Index - 1)].Id : W[1]};
		C.F = video::Format::Breakdown;
		for (const video::TitleOption& O : TitleOptions(C, 0))
		{
			C.TitleTpl = C.TitleTpl.empty() ? O.Key : C.TitleTpl;
		}
		for (const video::ThumbSpec& S : ThumbOptions(C, 0))
		{
			if (C.Thumbs.size() < 3)
			{
				C.Thumbs.push_back(S);
			}
		}
		if (C.TitleTpl.empty() || C.Thumbs.empty())
		{
			return "Nothing to package for " + C.Candidates.front() + ".";
		}
		const std::string Why = Publish(C);
		return Why.empty() ? "Editing a Duel of " + std::to_string(C.Thumbs.size()) + " thumbnails (" + Fixed(video::EditMinutesOf(C), 0) + " minutes)." : Why;
	}
	return "ss.Video.<Describe|Candidates|Fake|Upload|Advance|Grant|Ads|Launch|Stats|Ideas|Idea|Checkin|Trends|Sheet|Repack|Duel>";
}
} // namespace ss
