#include "ShortStack/Game/Session.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Video.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

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
std::string EditId(const video::Choice& C)
{
	std::string Id = "video:" + std::to_string(static_cast<int>(C.F)) + ":" + std::to_string(static_cast<int>(C.T)) + ":" + std::to_string(static_cast<int>(C.Fr)) + ":";
	for (size_t I = 0; I < C.Candidates.size(); ++I)
	{
		Id += (I > 0 ? "|" : "") + C.Candidates[I];
	}
	return Id;
}

bool ReadEditId(const std::string& Id, video::Choice& C)
{
	if (Id.rfind("video:", 0) != 0)
	{
		return false;
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
	C.F = static_cast<video::Format>(std::clamp(Fields[0], 0, 3));
	C.T = static_cast<video::TitleStyle>(std::clamp(Fields[1], 0, 4));
	C.Fr = static_cast<video::Frame>(std::clamp(Fields[2], 0, 4));
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
	const double World = WorldMinutes();
	const double Minutes = video::EditMinutes(C.F);
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
	TimeSkip.Result.Energy = -video::EditEnergy(C.F);
	TimeSkip.Result.Title = video::video_copy::UploadedTitle();
	TimeSkip.Result.Body = video::Title(C, Life.Video.Open, 0);
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
	const std::string Id = video::Upload(Life.Video, C, World, SeedBase, F);
	if (Id.empty())
	{
		LastOutcome.Title = video::video_copy::EditFailedTitle();
		LastOutcome.Body = video::video_copy::WhyNot(1);
		return LastOutcome.Body;
	}
	Life.Update2LinesRead |= life::U2Video;
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
	// CheckCalendar's step after the sponsors' (§1.6): views, subscribers, the review and the monthly payout.
	video::Step(Life.Video, Life, BankrollCents, Channel, From, To);
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
	return "ss.Video.<Describe|Candidates|Fake|Upload|Advance|Grant>";
}
} // namespace ss
