// video_channel (update 2 SPEC §6.7): Rerun. Classification vectors from scripted hands, privacy (no unshown card
// anywhere, storytime names nobody), the model's goldens (Tests/video_vectors.txt; "video_test --write" rewrites them,
// the diff reviewed), the partner gate's edges, idempotence (a span stepped twice, a save mid-span, a 30-day skip against
// 30 one-day steps), the $100 rollover and one ledger line a month, the fold (folded tails and stubs against every video
// kept whole, over three years; nothing overflows over fourteen), the saturation and fatigue terms, the bounds,
// determinism, the save's round trip, the thumbnails' frames, and the copy lint and trademark screen. core-video.
// C2 (the G2 audit): a heavy uploader's 28 and 30 days never over the views (B1), a Recap mixing a live night's skipped
// hands (S1), FormatSubs for Shorts (S2), payout marks only when the 21st pays (S5), no ad talk before the gate (N3), the
// statement with every payout (N5), PublishWhy and Deleted in Video.h (N6).
// Update 3 (SPEC §7.7, §8.2, Phase A): the title and copy checks (the copy lint, the trademark screen, every generated
// title, storytime, Studio's rules, the cooler's titles) moved verbatim to tests/video_seg_copy.cpp (core-videocontent's),
// still called from main() below in their order; the thumbnail draws moved to tests/ui_seg_thumbs.cpp (rerun_ui,
// ui-thumbs'). The helpers they share are this file's, declared in VideoSeg.h.
#include "CopyLint.h"
#include "VideoSeg.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef SS_TESTS_DIR
#define SS_TESTS_DIR "."
#endif

namespace ss
{
namespace video
{
// Video.cpp's seam for the fold test: the same step with every video kept whole.
namespace video_test_seam
{
void StepUnfolded(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To);
}
} // namespace video
} // namespace ss

namespace video_test
{
namespace video = ss::video;
namespace life = ss::life;
using ss::Card;
using ss::Chips;
// Day: VideoSeg.h's.

int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const char* What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Expect(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return S;
}

// ------------------------------------------------------------------ scripted hands (heads-up)

/** A full deck with these cards on top in this order, the rest after. */
std::vector<Card> DeckWith(const std::string& Top)
{
	std::vector<Card> D = ss::ParseCards(Top);
	for (int C = 0; C < 52; ++C)
	{
		if (std::find(D.begin(), D.end(), C) == D.end())
		{
			D.push_back(C);
		}
	}
	return D;
}

/**
 * Heads-up, the player on the button (seat 0: the small blind, first to act preflop, last after), the villain seat 1.
 * Deal order: villain, player, villain, player; then burn, flop, burn, turn, burn, river.
 */
std::unique_ptr<ss::Hand> HeadsUp(const std::string& Mine, const std::string& Theirs, const std::string& Board, Chips Stack, Chips VillainStack = 0)
{
	const std::vector<Card> M = ss::ParseCards(Mine), T = ss::ParseCards(Theirs), B = ss::ParseCards(Board);
	std::vector<Card> Used = M;
	Used.insert(Used.end(), T.begin(), T.end());
	Used.insert(Used.end(), B.begin(), B.end());
	std::vector<Card> Burns;
	for (int C = 51; C >= 0 && Burns.size() < 3; --C)
	{
		if (std::find(Used.begin(), Used.end(), C) == Used.end())
		{
			Burns.push_back(C);
		}
	}
	std::vector<Card> Top = {T[0], M[0], T[1], M[1], Burns[0]};
	for (size_t I = 0; I < 3 && I < B.size(); ++I)
	{
		Top.push_back(B[I]);
	}
	Top.push_back(Burns[1]);
	Top.push_back(B.size() > 3 ? B[3] : Burns[2]);
	Top.push_back(Burns[2]);
	if (B.size() > 4)
	{
		Top.push_back(B[4]);
	}
	std::string TopText;
	for (const Card C : Top)
	{
		TopText += ss::CardToString(C);
	}
	ss::HandConfig Cfg;
	Cfg.Players = {{0, "hero", Stack}, {1, "villain", VillainStack > 0 ? VillainStack : Stack}};
	Cfg.ButtonSeat = 0;
	Cfg.SmallBlind = 100;
	Cfg.BigBlind = 200;
	Cfg.Deck = DeckWith(TopText);
	ss::Rng R("video-test-hand");
	return std::make_unique<ss::Hand>(Cfg, R);
}

void Play(ss::Hand& H, const std::vector<ss::PlayerAction>& Script)
{
	for (const ss::PlayerAction& A : Script)
	{
		if (H.bComplete)
		{
			break;
		}
		const bool Ok = H.Act(A);
		Expect(Ok, "hands: a scripted action is legal");
	}
}

/** The hand as the player saw it ("You" and "Mike_R"), and whether it went to a showdown. */
ss::recall::Record RecordOf(const ss::Hand& H, bool& Showdown)
{
	std::vector<int> Shown;
	Showdown = false;
	for (const ss::HandEvent& E : H.Events)
	{
		if (E.Type == ss::EventType::Showdown)
		{
			Showdown = true;
			for (const ss::ShowdownHand& S : E.Hands)
			{
				Shown.push_back(S.Seat);
			}
		}
		else if (E.Type == ss::EventType::Reveal)
		{
			Shown.insert(Shown.end(), E.Seats.begin(), E.Seats.end());
		}
	}
	std::sort(Shown.begin(), Shown.end());
	Shown.erase(std::unique(Shown.begin(), Shown.end()), Shown.end());
	return ss::recall::Of(H, 0, {"You", "Mike_R"}, Shown, 214, 9);
}

using PA = ss::PlayerAction;

ss::recall::Record BadBeatHand(bool& Showdown, bool Swap)
{
	auto H = Swap ? HeadsUp("7c7h", "AsAd", "Kd8s2c9h7d", 6400) : HeadsUp("AsAd", "7c7h", "Kd8s2c9h7d", 6400);
	Play(*H, {PA::RaiseTo(600), PA::RaiseTo(6400), PA::Call()});
	Expect(H->bComplete, "hands: the all-in runs out");
	return RecordOf(*H, Showdown);
}

void Classification()
{
	bool Sd = false, Is = false;
	// A bad beat: aces all in preflop for 64 big blinds against sevens, a seven on the river.
	const ss::recall::Record Beat = BadBeatHand(Sd);
	if (!(Sd && Beat.HeroNet == -6400))
	{
		std::printf("  bad beat: showdown %d, net %lld, pot %lld, shown %zu, mine %s, board %zu\n", Sd ? 1 : 0, static_cast<long long>(Beat.HeroNet), static_cast<long long>(Beat.Pot), Beat.Shown.size(), Beat.Mine.empty() ? "-" : ss::CardToString(Beat.Mine[0]).c_str(), Beat.Board.size());
	}
	Expect(Sd && Beat.HeroNet == -6400 && Beat.Pot == 12800 && Beat.Shown.size() == 1, "classify: the bad beat hand is as scripted");
	Expect(video::Classify(Beat, 0.81, false, Sd, 20.0, Is) == video::Kind::BadBeat && Is, "classify: 81% and lost is a bad beat");
	Expect(video::Classify(Beat, 0.60, false, Sd, 20.0, Is) == video::Kind::BadBeat && Is, "classify: exactly 60% is a bad beat");
	video::Classify(Beat, 0.599, false, Sd, 20.0, Is);
	Expect(!Is, "classify: 59.9% and lost is nothing (not a bad beat, not a big pot)");
	Expect(video::Classify(Beat, -1.0, true, Sd, 20.0, Is) == video::Kind::Bubble && Is, "classify: an all-in on the bubble with no equity known is a bubble hand");
	Expect(video::Classify(Beat, 0.81, true, Sd, 20.0, Is) == video::Kind::BadBeat, "classify: a bad beat on the bubble is still a bad beat (the heavier kind)");
	// The suckout: the same hand from the other chair.
	const ss::recall::Record Suck = BadBeatHand(Sd, true);
	Expect(Suck.HeroNet == 6400 && video::Classify(Suck, 0.19, false, Sd, 20.0, Is) == video::Kind::Suckout && Is, "classify: 19% and won is a suckout");
	Expect(video::Classify(Suck, 0.41, false, Sd, 100.0, Is) != video::Kind::Suckout, "classify: 41% and won isn't a suckout");
	// A cooler: kings full against a set of sevens, 64 big blinds.
	{
		auto H = HeadsUp("KhKs", "7s7c", "Kd7d2c9s4h", 6400);
		Play(*H, {PA::RaiseTo(600), PA::RaiseTo(1800), PA::RaiseTo(6400), PA::Call()});
		const ss::recall::Record R = RecordOf(*H, Sd);
		Expect(R.HeroNet == 6400 && video::Classify(R, 0.95, false, Sd, 20.0, Is) == video::Kind::Cooler && Is, "classify: set over set for 64 big blinds is a cooler");
		auto Small = HeadsUp("KhKs", "7s7c", "Kd7d2c9s4h", 1900);
		Play(*Small, {PA::RaiseTo(600), PA::RaiseTo(1900), PA::Call()});
		const ss::recall::Record S = RecordOf(*Small, Sd);
		Expect(S.Pot == 3800 && video::Classify(S, 0.95, false, Sd, 20.0, Is) != video::Kind::Cooler, "classify: set over set for 19 big blinds isn't a cooler (40 or more)");
		// A split: kings and nines both ways for 64 big blinds (the C2 verify's chop, titled "nothing I could do").
		auto Chop = HeadsUp("KhJd", "KcTc", "QsKd9c9h8h", 6400);
		Play(*Chop, {PA::RaiseTo(6400), PA::Call()});
		const ss::recall::Record C = RecordOf(*Chop, Sd);
		Expect(C.Pot == 12800 && C.HeroNet * 4 < C.Pot && video::Classify(C, 0.5, false, Sd, 20.0, Is) != video::Kind::Cooler,
			"classify: a split pot of two good hands isn't a cooler");
	}
	// A bluff: six-five of spades, three barrels, a fold on the river (30 big blinds).
	{
		auto H = HeadsUp("6s5s", "AhJd", "KhQd2cJs8h", 10000);
		Play(*H, {PA::RaiseTo(500), PA::Call(), PA::Check(), PA::RaiseTo(700), PA::Call(), PA::Check(), PA::RaiseTo(1800), PA::Call(), PA::Check(), PA::RaiseTo(4200), PA::Fold()});
		const ss::recall::Record R = RecordOf(*H, Sd);
		Expect(!Sd && R.HeroNet > 0 && R.Pot == 6000 && R.Shown.empty(), "classify: the bluff hand is as scripted");
		Expect(video::Classify(R, -1.0, false, Sd, 20.0, Is) == video::Kind::Bluff && Is, "classify: three barrels with six high, a fold on the river: a bluff");
		// A preflop steal doesn't count.
		auto Steal = HeadsUp("6s5s", "AhJd", "KhQd2cJs8h", 10000);
		Play(*Steal, {PA::RaiseTo(500), PA::Fold()});
		const ss::recall::Record St = RecordOf(*Steal, Sd);
		video::Classify(St, -1.0, false, Sd, 20.0, Is);
		Expect(!Is, "classify: a preflop steal isn't a bluff");
		// Nor a small pot after the flop.
		auto Small = HeadsUp("6s5s", "AhJd", "KhQd2cJs8h", 10000);
		Play(*Small, {PA::RaiseTo(400), PA::Call(), PA::Check(), PA::RaiseTo(400), PA::Fold()});
		const ss::recall::Record Sm = RecordOf(*Small, Sd);
		video::Classify(Sm, -1.0, false, Sd, 20.0, Is);
		Expect(Sm.Pot == 800 && !Is, "classify: a 4 big blind flop stab isn't a bluff (15 or more)");
		// Caught: the river bet called and shown down.
		auto Caught = HeadsUp("6s5s", "KcTd", "KhQd2cJs8h", 10000);
		Play(*Caught, {PA::RaiseTo(500), PA::Call(), PA::Check(), PA::RaiseTo(700), PA::Call(), PA::Check(), PA::RaiseTo(1800), PA::Call(), PA::Check(), PA::RaiseTo(4200), PA::Call()});
		const ss::recall::Record C = RecordOf(*Caught, Sd);
		Expect(Sd && C.HeroNet < 0 && video::Classify(C, -1.0, false, Sd, 20.0, Is) == video::Kind::BluffCaught && Is, "classify: a river bluff called and shown is caught");
	}
	// A hero call: a pair of tens calls the river bet and wins.
	{
		auto H = HeadsUp("Td9d", "AsQs", "Th6c3s2h8c", 10000);
		Play(*H, {PA::RaiseTo(500), PA::Call(), PA::RaiseTo(600), PA::Call(), PA::Check(), PA::Check(), PA::RaiseTo(3300), PA::Call()});
		const ss::recall::Record R = RecordOf(*H, Sd);
		Expect(Sd && R.HeroNet > 0 && video::Classify(R, -1.0, false, Sd, 20.0, Is) == video::Kind::HeroCall && Is, "classify: one pair calls the river and wins: a hero call");
	}
	// A hero fold: queens to a shove on the flop, the villain's cards never shown.
	{
		auto H = HeadsUp("QhQd", "AhKh", "Ks9c4hJd2s", 10000);
		Play(*H, {PA::RaiseTo(500), PA::Call(), PA::RaiseTo(10000 - 500), PA::Fold()});
		const ss::recall::Record R = RecordOf(*H, Sd);
		Expect(!Sd && R.Shown.empty(), "classify: the hero fold's villain cards stay hidden");
		Expect(video::Classify(R, -1.0, false, Sd, 20.0, Is) == video::Kind::HeroFold && Is, "classify: queens folded to a shove: a hero fold (no shown cards)");
		ss::recall::Record WithShown = R;
		WithShown.Shown.push_back({"Mike_R", ss::ParseCards("AhKh")});
		Expect(video::Classify(WithShown, -1.0, false, Sd, 20.0, Is) == video::Kind::HeroFold, "classify: and with the villain's cards shown, still a hero fold");
		auto Junk = HeadsUp("7d2c", "AhKh", "Ks9c4hJd2s", 10000);
		Play(*Junk, {PA::RaiseTo(500), PA::Call(), PA::RaiseTo(10000 - 500), PA::Fold()});
		const ss::recall::Record J = RecordOf(*Junk, Sd);
		video::Classify(J, -1.0, false, Sd, 20.0, Is);
		Expect(!Is, "classify: seven-deuce folded to a shove is nothing");
		auto Small = HeadsUp("QhQd", "AhKh", "Ks9c4hJd2s", 10000);
		Play(*Small, {PA::RaiseTo(500), PA::Call(), PA::RaiseTo(200), PA::Fold()});
		const ss::recall::Record Sm = RecordOf(*Small, Sd);
		video::Classify(Sm, -1.0, false, Sd, 20.0, Is);
		Expect(!Is, "classify: queens folded to a fifth of the pot isn't a hero fold (half or more)");
	}
	// A big pot: 40 big blinds won, against the table's average stack (online) and the live rule (50).
	{
		auto H = HeadsUp("AsKs", "QhJh", "As8d2c5h9s", 10000);
		Play(*H, {PA::RaiseTo(500), PA::Call(), PA::RaiseTo(1000), PA::RaiseTo(3500), PA::Call(), PA::Check(), PA::Check(), PA::Check(), PA::Check()});
		const ss::recall::Record R = RecordOf(*H, Sd);
		Expect(R.Pot == 8000 && R.HeroNet == 4000, "classify: the big pot is 40 big blinds");
		Expect(video::Classify(R, -1.0, false, Sd, 20.0, Is) == video::Kind::BigPot && Is, "classify: 40 big blinds against a 20 big blind average: a big pot");
		video::Classify(R, -1.0, false, Sd, 30.0, Is);
		Expect(!Is, "classify: 40 big blinds against a 30 big blind average isn't (1.5x)");
		video::Classify(R, -1.0, false, Sd, 0.0, Is);
		Expect(!Is, "classify: live, 40 big blinds isn't a big pot (50)");
	}
}

// ------------------------------------------------------------------ candidates

video::LiveContext LiveCtx(int Source, double World)
{
	video::LiveContext X;
	X.Source = Source;
	X.EntryId = Source == 2 ? "dee" : "embercrest-nightly@9";
	X.Event = Source == 2 ? "Dee's game" : "Embercrest Nightly";
	X.BuyIn = Source == 2 ? 4000 : 12000;
	X.Field = Source == 2 ? 0 : 88;
	X.World = World;
	return X;
}

/** A Dee's hand: the player, Big Lou and Mei, a bad beat for 64 big blinds. */
ss::recall::Record DeesHand()
{
	ss::recall::Record R;
	R.Number = 12;
	R.Blinds.Sb = 25;
	R.Blinds.Bb = 50;
	R.Mine = ss::ParseCards("AsAd");
	R.Board = ss::ParseCards("Kd8s2c9h7d");
	R.Lines = {{ss::Street::Preflop, "Mei", "folds", 0}, {ss::Street::Preflop, "You", "raises to", 150}, {ss::Street::Preflop, "Big Lou", "all in", 1600},
		{ss::Street::Preflop, "You", "all in", 1600}};
	R.Shown = {{"Big Lou", ss::ParseCards("7c7h")}};
	R.Pot = 3225;
	R.HeroNet = -1600;
	return R;
}

void Candidates()
{
	// The Embercrest: a live hand, equity from the note's board at the all-in (preflop: stateless samples).
	{
		life::State L;
		bool Sd = false;
		const ss::recall::Record Beat = BadBeatHand(Sd);
		ss::room::HandNote N;
		N.AllIn = true;
		N.Called = true;
		N.Shown = {{"You", ss::ParseCards("AsAd")}, {"Mike_R", ss::ParseCards("7c7h")}};
		video::NoteLiveHand(L, Beat, &N, LiveCtx(1, 9.0 * Day + 20.0 * 60.0));
		Expect(L.Video.Open.size() == 1 && L.Video.Open[0].K == video::Kind::BadBeat && L.Video.Open[0].Source == 1, "live: the Embercrest's bad beat is a candidate");
		Expect((L.Update2LinesRead & life::U2Video) != 0, "live: the channel has state to keep (its bit is set)");
		const double Eq = L.Video.Open.empty() ? 0.0 : L.Video.Open[0].EquityAtAllIn;
		Expect(Eq > 0.75 && Eq < 0.86, "live: aces against sevens preflop are about 80% (" + std::to_string(Eq) + ")");
		Expect(!L.Video.Open.empty() && L.Video.Open[0].Id == "lv:embercrest-nightly@9:214", "live: the candidate's id is the entry's and the hand's");
		// The same hand noted twice is one candidate.
		video::NoteLiveHand(L, Beat, &N, LiveCtx(1, 9.0 * Day + 20.0 * 60.0));
		Expect(L.Video.Open.size() == 1, "live: the same hand twice is one candidate");
		// A knockout: the room's note names the busted, the player won it.
		ss::recall::Record Ko = BadBeatHand(Sd, true);
		Ko.Number = 215;
		ss::room::HandNote K = N;
		K.Busted = {"Mike_R"};
		video::NoteLiveHand(L, Ko, &K, LiveCtx(1, 9.0 * Day + 21.0 * 60.0));
		Expect(L.Video.Open.size() == 2 && L.Video.Open[1].K == video::Kind::Suckout, "live: the suckout that knocks someone out stays a suckout (the heavier kind)");
		// The night's recap at the cage.
		video::NoteLiveRecap(L, LiveCtx(1, 9.0 * Day + 23.0 * 60.0), 4, 119000);
		Expect(L.Video.Open.size() == 3 && L.Video.Open[2].K == video::Kind::FinalTable && L.Video.Open[2].Rec.Pot == 119000 && L.Video.Open[2].PlaceSoFar == 4,
			"live: 4th of 88 for $1,190 is a final table candidate, its prize in the record");
		// The Embercrest is 6-max: seventh is a deep run when it paid three buy-ins, never a final table.
		video::LiveContext Seventh = LiveCtx(1, 10.0 * Day + 23.0 * 60.0);
		Seventh.EntryId = "embercrest-nightly@10";
		video::NoteLiveRecap(L, Seventh, 7, 40000);
		Expect(L.Video.Open.size() == 4 && L.Video.Open[3].K == video::Kind::DeepRun && !L.Video.Open[3].Final, "live: 7th of 88 at a 6-max event is a deep run, not a final table");
	}
	// A live hand played through live::SkipSlice (no room note): the all-in's board and equity from the record itself.
	{
		life::State L;
		bool Sd = false;
		video::NoteLiveHand(L, BadBeatHand(Sd), nullptr, LiveCtx(1, 9.0 * Day));
		Expect(L.Video.Open.size() == 1 && L.Video.Open[0].K == video::Kind::BadBeat && L.Video.Open[0].EquityAtAllIn > 0.75,
			"live: a skipped hand (no room note) is classified from its record");
	}
	// Dee's game: storytime.
	{
		life::State L;
		ss::room::HandNote N;
		N.AllIn = true;
		N.Called = true;
		video::NoteLiveHand(L, DeesHand(), &N, LiveCtx(2, 9.0 * Day + 23.0 * 60.0));
		Expect(L.Video.Open.size() == 1, "storytime: Dee's bad beat is a candidate");
		if (!L.Video.Open.empty())
		{
			const video::Candidate& C = L.Video.Open[0];
			std::string Everything = C.Opponent + "|" + C.Event + "|" + ss::recall::Write(C.Rec);
			Expect(C.Storytime && C.Source == 2 && C.OpponentNpc < 0 && C.Event == "a back-room game" && C.Id.rfind("dee:9:", 0) == 0, "storytime: the candidate is storytime");
			Expect(Lower(Everything).find("lou") == std::string::npos && Lower(Everything).find("mei") == std::string::npos && Lower(Everything).find("dee") == std::string::npos,
				"storytime: no name in the candidate (" + Everything + ")");
			for (int F = 0; F < 2; ++F)
			{
				for (int T = 0; T < 5; ++T)
				{
					for (int Fr = 0; Fr < 5; ++Fr)
					{
						video::Choice Ch;
						Ch.Candidates = {C.Id};
						Ch.F = static_cast<video::Format>(F);
						Ch.T = static_cast<video::TitleStyle>(T);
						Ch.Fr = static_cast<video::Frame>(Fr);
						const std::string Title = Lower(video::Title(Ch, L.Video.Open, 0));
						const video::Thumb Th = video::ThumbFor(Ch, L.Video.Open);
						Expect(Title.find("lou") == std::string::npos && Title.find("mei") == std::string::npos && Title.find("dee") == std::string::npos,
							"storytime: no name in the title \"" + Title + "\"");
						Expect(Th.Opponent.empty() && Th.Mood >= 10, "storytime: no face but the player's on the thumbnail (and it's the portrait)");
					}
				}
			}
		}
	}
}

void Privacy()
{
	// The hero fold: the villain's ace-king of hearts never reaches a candidate, a title or a thumbnail.
	bool Sd = false;
	auto H = HeadsUp("QhQd", "AhKh", "Ks9c4hJd2s", 10000);
	Play(*H, {PA::RaiseTo(500), PA::Call(), PA::RaiseTo(10000 - 500), PA::Fold()});
	const ss::recall::Record R = RecordOf(*H, Sd);
	video::Channel C;
	video::Candidate X;
	X.Id = "rl:mm-26@1530:214";
	X.At = 12.0 * Day;
	X.K = video::Kind::HeroFold;
	X.Rec = R;
	X.Event = "Night Owl Turbo";
	X.BuyIn = 550;
	X.Opponent = "Mike_R";
	X.Bubble = true;
	video::NoteHand(C, X);
	Expect(C.Open.size() == 1 && C.Open[0].Rec.Shown.empty(), "privacy: a folded-to hand keeps no villain cards");
	life::State L;
	L.Video = C;
	L.Update2LinesRead |= life::U2Video;
	std::string Saved;
	video::WriteLines(L, Saved);
	const size_t At = Saved.find("video\tcand\t");
	Expect(At != std::string::npos, "privacy: the candidate is saved");
	for (int F = 0; F < 2; ++F)
	{
		for (int T = 0; T < 5; ++T)
		{
			for (int Fr = 0; Fr < 5; ++Fr)
			{
				video::Choice Ch;
				Ch.Candidates = {X.Id};
				Ch.F = static_cast<video::Format>(F);
				Ch.T = static_cast<video::TitleStyle>(T);
				Ch.Fr = static_cast<video::Frame>(Fr);
				const std::string Title = video::Title(Ch, C.Open, 0);
				const video::Thumb Th = video::ThumbFor(Ch, C.Open);
				const std::string Low = Lower(Title);
				Expect(Title.find("AK") == std::string::npos && Low.find("ace-king") == std::string::npos && Low.find("ahkh") == std::string::npos,
					"privacy: no unshown card in the title \"" + Title + "\"");
				Expect(Th.Cards.find("Ah") == std::string::npos && Th.Cards.find("Kh") == std::string::npos && Th.Cards.rfind("QhQd/", 0) == 0,
					"privacy: the thumbnail's cards are the player's and the board (" + Th.Cards + ")");
			}
		}
	}
	// A bluff shows the player's own cards (nobody at the table saw them; the video is where they're shown).
	{
		auto B = HeadsUp("6s5s", "AhJd", "KhQd2cJs8h", 10000);
		Play(*B, {PA::RaiseTo(500), PA::Call(), PA::Check(), PA::RaiseTo(700), PA::Call(), PA::Check(), PA::RaiseTo(1800), PA::Call(), PA::Check(), PA::RaiseTo(4200), PA::Fold()});
		video::Candidate Y = X;
		Y.Id = "rl:mm-26@1530:215";
		Y.K = video::Kind::Bluff;
		Y.Rec = RecordOf(*B, Sd);
		video::Channel C2;
		video::NoteHand(C2, Y);
		video::Choice Ch;
		Ch.Candidates = {Y.Id};
		Ch.Fr = video::Frame::Cards;
		const video::Thumb Th = video::ThumbFor(Ch, C2.Open);
		Expect(Th.Cards == "6s5s//KhQd2cJs8h" && Th.BigText != "", "privacy: a bluff's thumbnail shows the player's cards and the board, nothing of the villain's (" + Th.Cards + ")");
	}
}

// ------------------------------------------------------------------ the model

/** A candidate for the creator careers: a cooler at $5.50 (Topic 1.8 with the Versus frame's 1.0: the Python model's 1.5 x 1.2). */
video::Candidate Moment(const std::string& Id, double World)
{
	video::Candidate C;
	C.Id = Id;
	C.At = World;
	C.K = video::Kind::Cooler;
	C.Event = "Night Owl Turbo";
	C.BuyIn = 550;
	C.Rec.Blinds.Bb = 200;
	C.Rec.Pot = 12800;
	C.Rec.HeroNet = -6400;
	C.Rec.Mine = ss::ParseCards("KhKs");
	C.Expires = World + 21.0 * Day;
	return C;
}

struct Career
{
	video::Channel C;
	life::State L;
	ss::kast::Channel K;
	Chips Bank = 0;
	std::vector<video::Video> All; // every video as uploaded (the unfolded reference)
	int Uploads = 0;
};

/** Uploads on these weekdays from day 7 (Short, Breakdown, Vlog in turn), then steps to the end of the day. */
void CreatorDay(Career& Cr, int D, const std::string& Seed, const std::vector<int>& Weekdays, bool Unfolded, bool Mixed = false)
{
	const double At = static_cast<double>(D) * Day + 3.0 * 60.0;
	if (D >= 7 && std::find(Weekdays.begin(), Weekdays.end(), D % 7) != Weekdays.end())
	{
		const video::Format F = static_cast<video::Format>(Cr.Uploads % 3 == 0 ? 0 : Cr.Uploads % 3 == 1 ? 1 : 2);
		video::Choice Ch;
		Ch.F = F;
		// Mixed: every other Breakdown is a Teaching title on a bluff (the evergreen class: stubs and the strategy tail).
		const bool Teach = Mixed && F == video::Format::Breakdown && (Cr.Uploads / 3) % 2 == 0;
		Ch.T = Teach ? video::TitleStyle::Teaching : video::TitleStyle::Story;
		Ch.Fr = video::Frame::Versus;
		const int Count = F == video::Format::Vlog ? 3 : 1;
		for (int I = 0; I < Count; ++I)
		{
			const std::string Id = "rl:grind@" + std::to_string(D) + ":" + std::to_string(I);
			video::Candidate X = Moment(Id, At);
			X.K = Teach ? video::Kind::Bluff : X.K;
			video::NoteHand(Cr.C, X);
			Ch.Candidates.push_back(Id);
		}
		video::UploadFacts Facts;
		Facts.GearQuality = 0.5;
		const std::string Id = video::Upload(Cr.C, Ch, At + 30.0, Seed, Facts);
		Expect(!Id.empty(), "creator: an upload goes up");
		for (const video::Video& V : Cr.C.Videos)
		{
			if (V.Id == Id)
			{
				Cr.All.push_back(V);
			}
		}
		++Cr.Uploads;
	}
	const double From = static_cast<double>(D) * Day;
	if (Unfolded)
	{
		video::video_test_seam::StepUnfolded(Cr.C, Cr.L, Cr.Bank, Cr.K, From, From + Day);
	}
	else
	{
		video::Step(Cr.C, Cr.L, Cr.Bank, Cr.K, From, From + Day);
	}
}

std::string Num(double X, int Digits = 9)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.*g", Digits, X);
	return Buf;
}

/** The model's goldens: one upload at five seeds, and a three-a-week creator for 400 days. */
std::string Goldens()
{
	std::string Out;
	for (int S = 1; S <= 5; ++S)
	{
		video::Channel C;
		video::Candidate M = Moment("rl:golden@12:1", 12.0 * Day + 3.0 * 60.0 + 40.0);
		M.K = video::Kind::BadBeat;
		M.Stream.Viewers = 4;
		M.EquityAtAllIn = 0.81;
		M.Rec.Mine = ss::ParseCards("AsAd");
		M.Rec.Shown = {{"Mike_R", ss::ParseCards("7c7h")}};
		M.Rec.Board = ss::ParseCards("Kd8s2c9h7d");
		M.Opponent = "Mike_R";
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = static_cast<video::Format>(S % 4);
		Ch.T = static_cast<video::TitleStyle>(S % 5);
		Ch.Fr = video::Frame::Reaction;
		if (Ch.F == video::Format::Vlog || Ch.F == video::Format::Recap)
		{
			Ch.F = video::Format::Breakdown;
		}
		video::UploadFacts F;
		F.GearQuality = 0.5;
		F.KastFollowers = 4;
		// Update 3 (§4.4, Phase A): Update 2's P for these choices, read before the upload makes the video (UploadU2).
		const double PU2 = video::UploadU2(C, Ch, 12.0 * Day + 5.0 * 60.0, "golden:" + std::to_string(S), F);
		const std::string Id = video::Upload(C, Ch, 12.0 * Day + 5.0 * 60.0, "golden:" + std::to_string(S), F);
		const video::Video& V = C.Videos.back();
		Expect(PU2 == V.P && PU2 > 0.0, "goldens: UploadU2 gives the upload's own P, exactly (update 3 §4.4)");
		Out += "upload\t" + std::to_string(S) + "\t" + Id + "\tP=" + Num(V.P) + "\tc=" + Num(V.Conv) + "\tr=" + Num(V.Retention) + "\trpm=" + Num(V.Rpm) + "\tE=" + Num(V.E) +
			"\tclass=" + std::to_string(static_cast<int>(V.C)) + "\ttitle=" + V.Title + "\tbig=" + V.Th.BigText + "\n";
		for (const double T : {1.0, 7.0, 30.0, 365.0})
		{
			Out += "views\t" + std::to_string(S) + "\t" + Num(T) + "\t" + Num(video::Views(V, V.At + T * Day)) + "\n";
		}
	}
	Career Cr;
	for (int D = 0; D <= 400; ++D)
	{
		CreatorDay(Cr, D, "golden:creator", {0, 2, 4}, false);
		if (D == 30 || D == 90 || D == 180 || D == 365 || D == 400)
		{
			const double W = static_cast<double>(D + 1) * Day;
			Out += "creator\t" + std::to_string(D) + "\tsubs=" + Num(Cr.C.Subs) + "\tviews=" + Num(video::TotalViews(Cr.C, W)) + "\treview=" + Num(Cr.C.ReviewAt) +
				"\tpartner=" + Num(Cr.C.PartnerAt) + "\tbalance=" + std::to_string(Cr.C.BalanceCents) + "\tpaid=" + std::to_string(Cr.C.PaidCents) + "\n";
		}
	}
	return Out;
}

void ModelGoldens(bool Write)
{
	const std::string Path = std::string(SS_TESTS_DIR) + "/video_vectors.txt";
	const std::string Now = Goldens();
	if (Write)
	{
		std::FILE* F = std::fopen(Path.c_str(), "wb");
		Expect(F != nullptr, "goldens: video_vectors.txt can be written");
		if (F)
		{
			std::fwrite(Now.data(), 1, Now.size(), F);
			std::fclose(F);
			std::printf("  wrote %s (%zu bytes)\n", Path.c_str(), Now.size());
		}
		return;
	}
	std::ifstream In(Path, std::ios::binary);
	std::stringstream Was;
	Was << In.rdbuf();
	std::string Old = Was.str();
	Old.erase(std::remove(Old.begin(), Old.end(), '\r'), Old.end());
	Expect(!Old.empty(), "goldens: Tests/video_vectors.txt is there (video_test --write makes it)");
	if (Old != Now)
	{
		auto Lines = [](const std::string& S) {
			std::vector<std::string> Out;
			std::istringstream In(S);
			std::string L;
			while (std::getline(In, L))
			{
				Out.push_back(L);
			}
			return Out;
		};
		const std::vector<std::string> A = Lines(Old), B = Lines(Now);
		int Shown = 0;
		for (size_t I = 0; I < std::max(A.size(), B.size()) && Shown < 5; ++I)
		{
			const std::string La = I < A.size() ? A[I] : std::string(), Lb = I < B.size() ? B[I] : std::string();
			if (La != Lb)
			{
				std::printf("  golden: was \"%s\"\n          now \"%s\"\n", La.c_str(), Lb.c_str());
				++Shown;
			}
		}
	}
	Expect(Old == Now, "goldens: the model matches Tests/video_vectors.txt");
}

/** A channel at the gate's edge: Subs subscribers, Hours watch hours on day Day0 (in the ring). */
video::Channel AtTheGate(double Subs, double Hours, int HoursDay, int Today)
{
	video::Channel C;
	C.Subs = Subs;
	C.SteppedTo = static_cast<double>(Today) * Day;
	C.HoursDay0 = Today - 365;
	if (HoursDay >= C.HoursDay0)
	{
		C.HoursByDay[static_cast<size_t>(HoursDay - C.HoursDay0)] = static_cast<float>(Hours);
	}
	return C;
}

void Gate()
{
	life::State L;
	ss::kast::Channel K;
	Chips Bank = 0;
	const int Today = 400;
	const double D0 = static_cast<double>(Today) * Day;
	auto Reviewed = [&](video::Channel C) {
		video::Step(C, L, Bank, K, D0, D0 + Day);
		return C.ReviewAt >= 0.0;
	};
	Expect(Reviewed(AtTheGate(1000.0, 4000.0, Today - 10, Today)), "gate: 1,000 subscribers and 4,000 hours: under review");
	Expect(!Reviewed(AtTheGate(999.9, 4000.0, Today - 10, Today)), "gate: 999 subscribers: not yet");
	Expect(!Reviewed(AtTheGate(1000.0, 3999.0, Today - 10, Today)), "gate: 3,999 hours: not yet");
	Expect(Reviewed(AtTheGate(1000.0, 4000.0, Today - 364, Today)), "gate: hours 364 days back still count");
	Expect(!Reviewed(AtTheGate(1000.0, 4000.0, Today - 365, Today)), "gate: hours 365 days back don't (the trailing 365 days)");
	// The review, a week, then Partner: letters, the bell, and the ring goes.
	{
		video::Channel C = AtTheGate(1000.0, 4000.0, Today - 10, Today);
		life::State M;
		for (int D = 0; D < 8; ++D)
		{
			video::Step(C, M, Bank, K, D0 + D * Day, D0 + (D + 1) * Day);
		}
		Expect(C.ReviewAt == D0 + Day && C.PartnerAt == D0 + 8.0 * Day, "gate: Partner a week after the review");
		int Review = 0, Welcome = 0;
		for (const life::Letter& X : M.Inbox)
		{
			Review += X.Source == "video" && X.Kind == video::LetterReview ? 1 : 0;
			Welcome += X.Source == "video" && X.Kind == video::LetterWelcome ? 1 : 0;
		}
		Expect(Review == 1 && Welcome == 1, "gate: one review letter, one welcome");
		Expect(std::all_of(C.HoursByDay.begin(), C.HoursByDay.end(), [](float H) { return H == 0.0f; }), "gate: the ring is dropped at Partner");
	}
	// Shorts earn subscribers and no watch hours.
	{
		video::Channel C;
		life::State M;
		for (int I = 0; I < 6; ++I)
		{
			const double At = (20.0 + I) * Day;
			video::NoteHand(C, Moment("rl:short@" + std::to_string(I) + ":1", At));
			video::Choice Ch;
			Ch.Candidates = {"rl:short@" + std::to_string(I) + ":1"};
			Ch.F = video::Format::Short;
			video::UploadFacts F;
			video::Upload(C, Ch, At + 60.0, "shorts", F);
			video::Step(C, M, Bank, K, At, At + Day);
		}
		video::Step(C, M, Bank, K, 26.0 * Day, 60.0 * Day);
		const video::ChannelView V = video::View(C, 60.0 * Day);
		Expect(C.Subs > 0.0 && V.WatchHours365 == 0.0, "gate: Shorts bring subscribers (" + Num(C.Subs) + ") and no watch hours");
	}
}

void Idempotence()
{
	// A three-a-week creator to day 260: past the gate and a payout or two.
	Career A;
	for (int D = 0; D <= 230; ++D)
	{
		CreatorDay(A, D, "idem", {0, 2, 4}, false);
	}
	// Stepping the same span twice changes nothing.
	{
		Career B = A;
		const double From = 231.0 * Day;
		video::Step(B.C, B.L, B.Bank, B.K, From, From + 30.0 * Day);
		const std::string Once = [&]() {
			std::string S;
			B.L.Video = B.C;
			B.L.Update2LinesRead |= life::U2Video;
			video::WriteLines(B.L, S);
			return S;
		}();
		const Chips BankOnce = B.Bank;
		const size_t LedgerOnce = B.L.Ledger.size();
		video::Step(B.C, B.L, B.Bank, B.K, From, From + 30.0 * Day);
		video::Step(B.C, B.L, B.Bank, B.K, From + 10.0 * Day, From + 20.0 * Day);
		std::string Twice;
		B.L.Video = B.C;
		video::WriteLines(B.L, Twice);
		Expect(Once == Twice && BankOnce == B.Bank && LedgerOnce == B.L.Ledger.size(), "idempotence: a span stepped twice pays once");
	}
	// A 30-day skip equals 30 one-day steps, and a frame-sized walk (15 minutes at a time) too.
	{
		Career Skip = A, Days = A, Walk = A;
		const double From = 231.0 * Day;
		video::Step(Skip.C, Skip.L, Skip.Bank, Skip.K, From, From + 30.0 * Day);
		for (int D = 0; D < 30; ++D)
		{
			video::Step(Days.C, Days.L, Days.Bank, Days.K, From + D * Day, From + (D + 1) * Day);
		}
		for (double T = From; T < From + 30.0 * Day; T += 15.0)
		{
			video::Step(Walk.C, Walk.L, Walk.Bank, Walk.K, T, T + 15.0);
		}
		auto Text = [](Career& C) {
			std::string S;
			C.L.Video = C.C;
			C.L.Update2LinesRead |= life::U2Video;
			video::WriteLines(C.L, S);
			return S + "|" + std::to_string(C.Bank) + "|" + std::to_string(C.L.Ledger.size()) + "|" + Num(C.K.FollowFrac, 17);
		};
		const std::string S = Text(Skip);
		Expect(S == Text(Days), "idempotence: a 30-day skip equals 30 one-day steps, to the bit");
		Expect(S == Text(Walk), "idempotence: and 15 minutes at a time");
	}
	// A save in the middle of the span: the same days, the same money.
	{
		Career Whole = A, Half = A;
		const double From = 231.0 * Day;
		video::Step(Whole.C, Whole.L, Whole.Bank, Whole.K, From, From + 60.0 * Day);
		video::Step(Half.C, Half.L, Half.Bank, Half.K, From, From + 27.5 * Day);
		Half.L.Video = Half.C;
		Half.L.Update2LinesRead |= life::U2Video;
		std::string Saved;
		video::WriteLines(Half.L, Saved);
		life::State Back = Half.L;
		Back.Video = video::Channel();
		std::istringstream In(Saved);
		std::string Line;
		while (std::getline(In, Line))
		{
			std::vector<std::string> P;
			std::string Cur;
			for (const char Ch : Line)
			{
				if (Ch == '\t')
				{
					P.push_back(Cur);
					Cur.clear();
				}
				else
				{
					Cur.push_back(Ch);
				}
			}
			P.push_back(Cur);
			video::ReadLine(P, Back);
		}
		Half.C = Back.Video;
		video::Step(Half.C, Half.L, Half.Bank, Half.K, From + 27.5 * Day, From + 60.0 * Day);
		Expect(Whole.Bank == Half.Bank && Whole.C.PaidCents == Half.C.PaidCents && Whole.C.BalanceCents == Half.C.BalanceCents && Whole.L.Ledger.size() == Half.L.Ledger.size(),
			"idempotence: a save mid-span pays the same, once");
		Expect(std::fabs(Whole.C.Subs - Half.C.Subs) < 1e-6, "idempotence: and the subscribers match");
	}
	// The $100 minimum rolls over; one ledger line a month at most.
	{
		Career Long = A;
		for (int D = 231; D <= 600; ++D)
		{
			CreatorDay(Long, D, "idem", {0, 2, 4}, false);
		}
		std::map<std::string, int> Lines;
		bool Under = false;
		for (const life::LedgerEntry& E : Long.L.Ledger)
		{
			if (E.Kind == life::LedgerVideo)
			{
				++Lines[E.Label];
				Under = Under || E.Amount < 10000;
			}
		}
		bool OneEach = !Lines.empty();
		for (const auto& Ln : Lines)
		{
			OneEach = OneEach && Ln.second == 1;
		}
		Expect(OneEach && !Under, "payout: at most one line a month, and never under $100 (" + std::to_string(Lines.size()) + " payouts)");
		int Rolled = 0;
		for (const life::Letter& X : Long.L.Inbox)
		{
			Rolled += X.Source == "video" && X.Kind == video::LetterStatement && X.Subject.find("statement") != std::string::npos ? 1 : 0;
		}
		Expect(Long.C.PartnerAt > 0.0 && Long.C.PaidCents > 0, "payout: the creator is a Partner and was paid");
		std::printf("  payout: partner on day %d, %zu payouts, %d rolled statements, $%.2f paid by day 600\n", static_cast<int>(Long.C.PartnerAt / Day), Lines.size(), Rolled,
			static_cast<double>(Long.C.PaidCents) / 100.0);
		// A balance under $100 on the 21st rolls: a channel made a Partner with one small video.
		video::Channel Small;
		life::State M;
		Chips Bank = 0;
		ss::kast::Channel K;
		Small.Subs = 1500.0;
		Small.SteppedTo = 400.0 * Day;
		Small.PartnerAt = 400.0 * Day;
		video::Video V;
		V.Id = "v1";
		V.At = 399.0 * Day;
		V.P = 20000.0;
		V.A = 0.75;
		V.Tau1 = 1.5;
		V.Tau2 = 12.0;
		V.Rpm = 2.5;
		V.F = video::Format::Breakdown;
		V.LengthMin = 8.0;
		V.Retention = 0.5;
		Small.Videos.push_back(V);
		video::Step(Small, M, Bank, K, 400.0 * Day, 460.0 * Day);
		Expect(Bank == 0 && Small.BalanceCents > 0 && Small.BalanceCents < 10000, "payout: under $100 it rolls over (" + std::to_string(Small.BalanceCents) + " cents)");
	}
}

void Fold()
{
	// Three years of three a week, the newest 60 kept whole against every video kept whole.
	Career Folded, Whole;
	const auto T0 = std::chrono::steady_clock::now();
	for (int D = 0; D <= 1095; ++D)
	{
		CreatorDay(Folded, D, "fold", {0, 2, 4}, false, true);
		CreatorDay(Whole, D, "fold", {0, 2, 4}, true, true);
	}
	const double World = 1096.0 * Day;
	Expect(Folded.C.Videos.size() <= 60 && Whole.C.Videos.size() > 400, "fold: 60 kept whole, the rest folded (" + std::to_string(Folded.C.Videos.size()) + ")");
	const double Rel = std::fabs(Folded.C.Subs - Whole.C.Subs) / std::max(1.0, Whole.C.Subs);
	Expect(Rel < 1e-6 || std::fabs(Folded.C.Subs - Whole.C.Subs) < 0.5, "fold: subscribers to 1e-6 (" + Num(Folded.C.Subs, 12) + " against " + Num(Whole.C.Subs, 12) + ")");
	const double Va = video::TotalViews(Folded.C, World), Vb = video::TotalViews(Whole.C, World);
	Expect(std::fabs(Va - Vb) / std::max(1.0, Vb) < 1e-6 || std::fabs(Va - Vb) < 0.5, "fold: views to 1e-6 (" + Num(Va, 12) + " against " + Num(Vb, 12) + ")");
	const Chips Ca = Folded.C.BalanceCents + Folded.C.PaidCents, Cb = Whole.C.BalanceCents + Whole.C.PaidCents;
	Expect(std::llabs(Ca - Cb) <= 1, "fold: ad money to the cent (" + std::to_string(Ca) + " against " + std::to_string(Cb) + ")");
	// And against the closed form itself: every video's own curve, summed.
	double Closed = 0.0;
	for (const video::Video& V : Whole.All)
	{
		Closed += V.Conv * video::Views(V, World);
	}
	Expect(std::fabs(Closed - Whole.C.Subs) / std::max(1.0, Closed) < 1e-9, "fold: the stepped subscribers are the closed form's (" + Num(Closed, 12) + ")");
	Expect(!Folded.C.Stubs.empty() && Folded.C.Tails[1].Total[0] > 0.0, "fold: evergreen videos left the 60 as stubs, and their curves folded into the strategy tail");
	// Fourteen years: nothing overflows, nothing goes negative.
	Career Long;
	for (int D = 0; D <= 14 * 365; ++D)
	{
		CreatorDay(Long, D, "long", {0, 1, 2, 3, 4, 5, 6}, false, true);
	}
	bool Finite = std::isfinite(Long.C.Subs) && Long.C.Subs >= 0.0 && Long.C.BalanceCents >= 0 && Long.C.PaidCents >= 0;
	for (const video::Tail& T : Long.C.Tails)
	{
		for (int W = 0; W < 3; ++W)
		{
			Finite = Finite && std::isfinite(T.Total[W]) && T.Total[W] >= 0.0 && std::isfinite(T.Decay[0][W]) && T.Decay[0][W] >= 0.0 && std::isfinite(T.Decay[1][W]) &&
				T.Decay[1][W] >= 0.0;
		}
	}
	Expect(Finite && Long.C.Videos.size() <= 60, "fold: fourteen years of a daily uploader stay finite (" + Num(Long.C.Subs) + " subscribers)");
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	std::printf("  fold: 3 years twice and 14 years once in %.0f ms; %zu stubs, tails %.0f/%.0f/%.0f views\n", Ms, Folded.C.Stubs.size(), Folded.C.Tails[0].Total[0],
		Folded.C.Tails[1].Total[0], Folded.C.Tails[2].Total[0]);
	// The calendar step's cost over a 30-day skip (§9: VideoStep + SiteStep <= 2 ms).
	Career Sk = Folded;
	const auto S0 = std::chrono::steady_clock::now();
	video::Step(Sk.C, Sk.L, Sk.Bank, Sk.K, 1096.0 * Day, 1126.0 * Day);
	const double StepMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - S0).count();
	std::printf("  bench: a 30-day VideoStep over 60 videos, three tails and the stubs: %.3f ms\n", StepMs);
	Expect(StepMs < 2.0, "bench: a 30-day step under 2 ms");
}

/** A channel with these uploads' times (dummy videos: no views, no subscribers), so only the reach terms differ. */
video::Channel Uploaded(const std::vector<double>& Times, double Subs)
{
	video::Channel C;
	C.Subs = Subs;
	C.SteppedTo = 100.0 * Day;
	for (size_t I = 0; I < Times.size(); ++I)
	{
		video::Video V;
		V.Id = "d" + std::to_string(I);
		V.At = Times[I];
		V.Tau1 = 1.5;
		V.Tau2 = 12.0;
		V.K = video::Kind::Cooler;
		C.Videos.push_back(V);
	}
	return C;
}

double PFor(video::Channel C, double World)
{
	video::NoteHand(C, Moment("rl:reach@1:1", World));
	video::Choice Ch;
	Ch.Candidates = {"rl:reach@1:1"};
	Ch.F = video::Format::Breakdown;
	video::UploadFacts F;
	const std::string Id = video::Upload(C, Ch, World, "reach", F);
	return Id.empty() ? -1.0 : C.Videos.back().P;
}

void Terms()
{
	const double World = 106.0 * Day;
	// Fatigue: six in the last week against six the week before (the same cadence over 14 days, the same library).
	std::vector<double> Week, Before;
	for (int I = 0; I < 6; ++I)
	{
		Week.push_back(World - (0.5 + I) * Day);
		Before.push_back(World - (7.5 + I) * Day);
	}
	const double Tired = PFor(Uploaded(Week, 0.0), World), Fresh = PFor(Uploaded(Before, 0.0), World);
	const double Ratio = Tired / Fresh;
	Expect(std::fabs(Ratio - std::sqrt(3.0 / 7.0)) < 2e-3, "terms: a seventh upload in a week reaches sqrt(3/7) as far (" + Num(Ratio, 5) + ")");
	// Three a week isn't fatigue.
	std::vector<double> Three(Week.begin(), Week.begin() + 2);
	std::vector<double> ThreeBefore = Three;
	for (double& T : ThreeBefore)
	{
		T -= 7.0 * Day;
	}
	Expect(std::fabs(PFor(Uploaded(Three, 0.0), World) / PFor(Uploaded(ThreeBefore, 0.0), World) - 1.0) < 2e-3, "terms: a third upload in a week isn't tired");
	// Saturation: past about 20,000 subscribers each one adds less (S' = S / (1 + S / 20,000)).
	const double P0 = PFor(Uploaded({}, 0.0), World), P20 = PFor(Uploaded({}, 20000.0), World), P200 = PFor(Uploaded({}, 200000.0), World),
				 P2M = PFor(Uploaded({}, 2000000.0), World);
	auto BaseOf = [](double S) {
		const double Sat = S / (1.0 + S / 20000.0);
		return 1200.0 + 180.0 * std::sqrt(Sat) + 3.5 * Sat;
	};
	Expect(std::fabs(P20 / P0 - BaseOf(20000.0) / BaseOf(0.0)) < 2e-3, "terms: 20,000 subscribers reach as the saturated formula says (" + Num(P20 / P0, 5) + ")");
	Expect(P2M / P200 < 1.12 && P2M / P200 > 1.0, "terms: ten times the subscribers past the niche adds little (" + Num(P2M / P200, 4) + ")");
	// The Kast term, capped at 20,000 followers.
	{
		video::Channel C = Uploaded({}, 0.0);
		video::NoteHand(C, Moment("rl:kast@1:1", World));
		video::Choice Ch;
		Ch.Candidates = {"rl:kast@1:1"};
		video::UploadFacts F;
		F.KastFollowers = 20000;
		video::Upload(C, Ch, World, "reach", F);
		const double Capped = C.Videos.back().P;
		video::Channel D = Uploaded({}, 0.0);
		video::NoteHand(D, Moment("rl:kast@1:1", World));
		F.KastFollowers = 400000;
		video::Upload(D, Ch, World, "reach", F);
		Expect(std::fabs(D.Videos.back().P - Capped) < 0.02 && Capped > P0 * 4.0, "terms: Kast followers seed the reach, capped at 20,000");
	}
}

void Bounds()
{
	video::Channel C;
	for (int I = 0; I < 45; ++I)
	{
		video::Candidate X = Moment("rl:bound@" + std::to_string(I) + ":1", 10.0 * Day + I);
		X.Rec.Pot = 2000 + 400 * ((I * 7) % 45); // varied pots, so varied Drama
		video::NoteHand(C, X);
	}
	double Lowest = 1.0;
	for (const video::Candidate& X : C.Open)
	{
		Lowest = std::min(Lowest, X.Drama);
	}
	Expect(C.Open.size() == 30, "bounds: at most 30 open candidates");
	Expect(Lowest >= 0.6, "bounds: the lowest Drama went first (" + Num(Lowest, 4) + ")");
	// Three weeks and they're gone.
	life::State L;
	ss::kast::Channel K;
	Chips Bank = 0;
	video::Step(C, L, Bank, K, 10.0 * Day, 32.0 * Day);
	Expect(C.Open.empty(), "bounds: candidates expire after 21 days");
	// 70 uploads: 60 kept.
	Career Cr;
	for (int D = 0; D <= 170; ++D)
	{
		CreatorDay(Cr, D, "bounds", {0, 2, 4, 6}, false);
	}
	Expect(Cr.Uploads > 60 && Cr.C.Videos.size() <= 60, "bounds: at most 60 videos kept whole (" + std::to_string(Cr.Uploads) + " uploaded)");
}

std::string SaveText(const life::State& L)
{
	std::string S;
	video::WriteLines(L, S);
	return S;
}

life::State Reload(const std::string& Text)
{
	life::State L;
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		std::vector<std::string> P;
		std::string Cur;
		for (const char Ch : Line)
		{
			if (Ch == '\t')
			{
				P.push_back(Cur);
				Cur.clear();
			}
			else
			{
				Cur.push_back(Ch);
			}
		}
		P.push_back(Cur);
		Expect(video::ReadLine(P, L), "save: every video line reads");
	}
	return L;
}

void Saves()
{
	// A full channel: videos, tails, stubs, the ring, candidates of every source, payout and story keys, the bell, a grant.
	Career Cr;
	for (int D = 0; D <= 260; ++D)
	{
		CreatorDay(Cr, D, "save", {0, 2, 4}, false, true);
	}
	life::State L = Cr.L;
	L.Video = Cr.C;
	L.Update2LinesRead |= life::U2Video;
	bool Sd = false;
	ss::room::HandNote N;
	N.AllIn = true;
	N.Called = true;
	video::NoteLiveHand(L, DeesHand(), &N, LiveCtx(2, 261.0 * Day));
	video::NoteLiveHand(L, BadBeatHand(Sd), &N, LiveCtx(1, 261.0 * Day));
	video::Candidate S = Moment("rl:stream@5:1", 261.0 * Day);
	S.Stream.Viewers = 41;
	S.Stream.HypeAfter = 72.5;
	S.Stream.Face = 3;
	S.Stream.Chat = {"NOOOO", "rigged | deck", "gg\tgo next"};
	S.Stream.ClipAt = 261.0 * Day - 0.5;
	S.Auto = true;
	S.Style = 6;
	video::NoteHand(L.Video, S);
	video::NoteStory(L.Video, "rent-paid", 261.0 * Day);
	video::Grant(L.Video, 10.0, 25.5, 300.0, 261.0 * Day);
	const std::string Text = SaveText(L);
	const life::State Back = Reload(Text);
	Expect(SaveText(Back) == Text, "save: the channel writes back as it was read");
	Expect((Back.Update2LinesRead & life::U2Video) != 0, "save: reading a video line sets the channel's bit");
	Expect(Back.Video.Open.size() == L.Video.Open.size() && Back.Video.Videos.size() == L.Video.Videos.size() && Back.Video.Subs == L.Video.Subs,
		"save: candidates, videos and subscribers come back");
	bool ChatBack = false;
	for (const video::Candidate& X : Back.Video.Open)
	{
		ChatBack = ChatBack || (X.Id == S.Id && X.Stream.Chat == S.Stream.Chat && X.Stream.ClipAt == S.Stream.ClipAt && X.Auto && X.Style == 6);
	}
	Expect(ChatBack, "save: the stream's bit (chat with a tab and a bar in it, the clip's time) comes back");
	std::printf("  save: %zu bytes for %zu videos, %zu stubs, %zu candidates\n", Text.size(), L.Video.Videos.size(), L.Video.Stubs.size(), L.Video.Open.size());
	// An untouched life writes nothing.
	life::State Untouched;
	Expect(SaveText(Untouched).empty(), "save: an untouched channel writes no line (an old save writes back as it was read)");
	// A reload never rerolls: the same choice and seed give the same video; after a reload the numbers are the same.
	{
		video::Channel A, B;
		video::NoteHand(A, Moment("rl:det@1:1", 30.0 * Day));
		B = A;
		video::Choice Ch;
		Ch.Candidates = {"rl:det@1:1"};
		video::UploadFacts F;
		F.GearQuality = 0.3;
		video::Upload(A, Ch, 30.0 * Day + 90.0, "det", F);
		video::Upload(B, Ch, 30.0 * Day + 90.0, "det", F);
		Expect(A.Videos.back().P == B.Videos.back().P && A.Videos.back().Title == B.Videos.back().Title, "determinism: the same upload rolls the same numbers");
		life::State La;
		La.Video = A;
		La.Update2LinesRead |= life::U2Video;
		const life::State Lb = Reload(SaveText(La));
		const video::Video& Va = A.Videos.back();
		const video::Video& Vb = Lb.Video.Videos.back();
		Expect(Va.P == Vb.P && Va.Conv == Vb.Conv && Va.Retention == Vb.Retention && Va.Rpm == Vb.Rpm && Va.At == Vb.At && Va.Th.Cards == Vb.Th.Cards,
			"determinism: a reload keeps every rolled number exactly");
	}
}

// ------------------------------------------------------------------ update 3, Phase A: the save seams (SPEC §7.3, §7.4)

/** A save's lines, each split on its tabs. */
std::vector<std::vector<std::string>> SplitLines(const std::string& Text)
{
	std::vector<std::vector<std::string>> Out;
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		Out.emplace_back(1);
		for (const char Ch : Line)
		{
			if (Ch == '\t')
			{
				Out.back().emplace_back();
			}
			else
			{
				Out.back().back().push_back(Ch);
			}
		}
	}
	return Out;
}

/**
 * Update 3 (Phase A): every saved enum reads clamped to its Count, so an appended value reads as itself and a value past
 * the last reads as the last (they read as the last old value before); Rerun 2.0's state (an Update 3 video's block and
 * its components, an idea candidate, the channel's X) writes back byte for byte, and only once U3Video is set, so a channel
 * nobody touched since Update 2 writes what it always did.
 */
void SavesU3()
{
	auto Basic = [](const std::string& Id, double At) {
		video::Video V;
		V.Id = Id;
		V.At = At;
		V.P = 2400.0;
		V.A = 0.6;
		V.Tau1 = 1.5;
		V.Tau2 = 12.0;
		V.Conv = 0.006;
		V.Retention = 0.45;
		V.LengthMin = 9.0;
		V.Rpm = 2.4;
		V.Title = "Kings, 82% to win";
		return V;
	};
	{
		life::State L;
		L.Update2LinesRead |= life::U2Video;
		L.Video.SteppedTo = 30.0 * Day;
		video::Video V = Basic("v-enums", 29.0 * Day);
		V.K = static_cast<video::Kind>(static_cast<int>(video::Kind::Count) - 1);
		V.F = static_cast<video::Format>(static_cast<int>(video::Format::Count) - 1);
		V.T = static_cast<video::TitleStyle>(static_cast<int>(video::TitleStyle::Count) - 1);
		V.Fr = video::Frame::Recipe;
		V.C = static_cast<video::Class>(static_cast<int>(video::Class::Count) - 1);
		V.Th.Fr = video::Frame::Recipe;
		L.Video.Videos.push_back(V);
		const std::string Text = SaveText(L);
		const life::State Back = Reload(Text);
		const video::Video& B = Back.Video.Videos.back();
		Expect(B.K == V.K && B.F == V.F && B.T == V.T && B.Fr == video::Frame::Recipe && B.C == V.C && B.Th.Fr == video::Frame::Recipe && SaveText(Back) == Text,
			"update 3 save: each saved enum's last value, the appended ones included, reads back as itself");
		// Past every enum's last value: the last value.
		life::State Past;
		for (std::vector<std::string> P : SplitLines(Text))
		{
			if (P.size() > 21 && P[1] == "vid")
			{
				for (const size_t K : std::vector<size_t>{4, 5, 6, 7, 8, 21})
				{
					P[K] = "99";
				}
			}
			video::ReadLine(P, Past);
		}
		const video::Video* Pv = Past.Video.Videos.empty() ? nullptr : &Past.Video.Videos.back();
		Expect(Pv && Pv->K == V.K && Pv->F == V.F && Pv->T == V.T && Pv->Fr == video::Frame::Recipe && Pv->C == V.C && Pv->Th.Fr == video::Frame::Recipe,
			"update 3 save: a value past an enum's last reads as its last (the Count sentinels)");
		// An Update 2 channel writes the same bytes with U3Video set: nothing of Update 3's until something holds it.
		life::State Set = L;
		Set.Update3LinesRead |= life::U3Video;
		Expect(SaveText(Set) == Text && Text.find("\tvlife\t") == std::string::npos && Text.find("\tvcomp\t") == std::string::npos && (Back.Update3LinesRead & life::U3Video) == 0,
			"update 3 save: an Update 2 channel writes no Update 3 line with or without the bit, and reading it sets no bit");
	}
	{
		life::State L;
		L.Update2LinesRead |= life::U2Video;
		L.Update3LinesRead |= life::U3Video;
		video::Channel& C = L.Video;
		C.Subs = 1234.5;
		C.SteppedTo = 40.0 * Day;
		C.Videos.push_back(Basic("v-legacy", 20.0 * Day));
		video::Video V = Basic("v-u3", 39.0 * Day + 21.0 * 60.0);
		V.Fr = video::Frame::Recipe;
		video::VideoX& X = V.X;
		X.On = true;
		X.Fate = 4000000000u;
		X.Ctr = 0.0612345;
		X.Sigma = 0.9;
		X.KastShare = 0.125;
		X.S0 = -0.25;
		X.S0Base = 0.1;
		X.P0 = 812.5;
		X.CtrNoise = 1.0731;
		X.Earned = 123456.789;
		X.Idea = 42;
		X.Series = 3;
		X.TitleTpl = "beats.loud|7\tb";
		X.Repacks = 2;
		X.LastRepackDay = 38;
		X.Promise = 3;
		X.Payoff = 2;
		X.C = video::Coin::Limited;
		X.Ads = video::AdLoad::Heavy;
		X.AdsAtUpload = video::AdLoad::Light;
		X.Len = video::LengthOpt::Extended;
		X.AdsChangedDay = 37;
		X.Launch = 0x5;
		X.DuelCtr = {0.071f, 0.0645f, 0.0f};
		X.Duels = 2;
		X.DuelWin = 1;
		X.Verdict = 3;
		X.SettledS = 0.4375f;
		X.Spec[0].Arch = 7;
		X.Spec[0].Layout = 11;
		X.Spec[0].Mirror = 1;
		X.Spec[0].Pal = 4;
		X.Spec[0].Expr = 9;
		X.Spec[0].PropA = 12;
		X.Spec[0].Sticker = 3;
		X.Spec[0].LookRef = 0;
		X.Spec[0].Variant = 3000000000u;
		X.Spec[0].Caption = "82%; then \\ the river | a tab\there";
		X.Spec[0].Caption2 = "KINGS";
		X.Spec[1].Arch = 2;
		X.Spec[1].Caption = ";";
		X.Comps.push_back({39.0 * Day + 21.0 * 60.0, 812.5, 0.6, video::CompKind::Verdict, 0, 1.0f});
		X.Comps.push_back({46.0 * Day, 140.25, 0.35, video::CompKind::Check7, 3, 1.05f});
		X.NextUp = "v-legacy";
		C.Videos.push_back(V);
		video::Candidate Ic = Moment("idea:42@40:1", 40.0 * Day);
		Ic.Source = 5;
		Ic.K = video::Kind::Idea;
		Ic.Idea = 42;
		Ic.IdeaFacts = "month=2026-10\tnet=12345|events=31";
		C.Open.push_back(Ic);
		video::ChannelX& Cx = C.X;
		Cx.On = true;
		video::DayRow Row;
		Row.Key = 39;
		Row.Views = 1540.5f;
		Row.WatchH = 88.25f;
		Row.Gained = 12.0f;
		Row.Lost = 1.5f;
		Row.Likes = 61.0f;
		Row.Dislikes = 2.0f;
		Row.Impr = 21000.0f;
		Row.Comments = 9.0f;
		Row.Cents = 412;
		Cx.Days = {Row, Row};
		Cx.Days[1].Key = 40;
		Cx.Months = {Row};
		Cx.Months[0].Key = 2026 * 12 + 9;
		Cx.Life = Row;
		Cx.Life.Key = 0;
		Cx.Top.push_back({"v-old", "An old \t one | top", 50000.0, 8800, 2.0 * Day});
		Cx.TailsX[1].Total[0] = 10.5;
		Cx.TailsX[1].Decay[1][3] = 0.25;
		Cx.Momentum = 1.0625;
		Cx.Settled = {0.5f, -0.25f, 0.0f, 0.125f, 0.0f, 0.0f};
		Cx.SettledN = 4;
		Cx.LastUploadDay = 39;
		Cx.WelcomeBack = true;
		video::FrozenLook Look;
		Look.Ref = 1;
		for (size_t K = 0; K < Look.Look.size(); ++K)
		{
			Look.Look[K] = static_cast<int16_t>(static_cast<int>(K) * 3 - 5);
		}
		Cx.Looks.push_back(Look);
		Cx.Lists.push_back({"pl1", "Coolers | and \t beats", {"v-legacy", "v-u3"}});
		Cx.Challenges.push_back({1, 2, 1, 2000, 100000, 35.0 * Day, 1, 3});
		Cx.IdeaCool.push_back({42, 39, 2});
		Cx.TrendCool.push_back({7, 40, 1});
		Cx.Snaps.push_back({2026 * 12 + 9, 31, 6, -4500, 3100, 330, -87, 41.5f});
		Cx.StubDays.push_back({39, 1.5, 0.009, 3.6, 0.75, 0.0045, 1.8});
		Cx.TrendWeek = 5;
		Cx.Since = 10;
		const std::string Text = SaveText(L);
		const life::State Back = Reload(Text);
		const video::Video* Bv = nullptr;
		for (const video::Video& Each : Back.Video.Videos)
		{
			Bv = Each.Id == "v-u3" ? &Each : Bv;
		}
		const bool Legacy = !Back.Video.Videos.empty() && !Back.Video.Videos.front().X.On;
		Expect(SaveText(Back) == Text, "update 3 save: Rerun 2.0's state writes back byte for byte");
		Expect((Back.Update3LinesRead & life::U3Video) != 0 && Back.Video.X.On && Legacy && Bv && Bv->X.On && Bv->X.Comps.size() == 2 && Bv->X.Fate == X.Fate &&
				   Bv->X.Spec[0].Caption == X.Spec[0].Caption && Bv->X.Spec[0].Variant == X.Spec[0].Variant && Bv->X.Spec[1].Caption == ";" && Bv->X.Spec[2].Arch == -1 &&
				   Bv->X.TitleTpl == X.TitleTpl && Bv->X.Ads == video::AdLoad::Heavy && Bv->X.DuelCtr[1] == 0.0645f && !Back.Video.Open.empty() &&
				   Back.Video.Open.back().Idea == 42 && Back.Video.Open.back().IdeaFacts == Ic.IdeaFacts && Back.Video.X.Lists.size() == 1 &&
				   Back.Video.X.Lists[0].Videos.size() == 2 && Back.Video.X.TrendWeek == 5 && Back.Video.X.Since == 10,
			"update 3 save: the video's block, its components, the idea candidate and the channel's X come back as they were, the legacy video legacy");
		// core-videoalgo: a Rerun 2.0 channel (its X on) writes its lines with the bit unset too (its first Update 3 upload is its
		// own, before the Session sets the bit), and reading them sets the bit, so the next save is the same text.
		life::State Unset = L;
		Unset.Update3LinesRead = 0;
		const std::string Same = SaveText(Unset);
		const life::State Again = Reload(Same);
		Expect(Same == Text && (Again.Update3LinesRead & life::U3Video) != 0 && SaveText(Again) == Text,
			"update 3 save: a Rerun 2.0 channel writes its X and components without U3Video, and the bit comes back with them");
		// The channel's X off (an Update 2 channel with one Update 3 video someone made by hand): the X and the components wait
		// for the bit, as Phase A wrote them.
		life::State Off = Unset;
		Off.Video.X.On = false;
		const std::string Plain = SaveText(Off);
		Expect(Plain.find("\tvcomp\t") == std::string::npos && Plain.find("\tvlife\t") == std::string::npos && Plain.find("\tvday\t") == std::string::npos,
			"update 3 save: nothing of the channel's X or the components without U3Video on a channel whose X is off");
		std::printf("  update 3 save: %zu bytes of Rerun 2.0 lines for one Update 3 video and a channel's X\n", Text.size() - Plain.size());
	}
}

// ------------------------------------------------------------------ C2: the G2 audit's fixes (B1, S1, S2, S5, N3, N5, N6)

/** The first world day after From that falls on the 21st (Rerun's payday). */
int NextPayday(int From)
{
	for (int D = From; D < From + 40; ++D)
	{
		int Y = 0, M = 0, Dm = 0;
		ss::world::CivilDate(D, Y, M, Dm);
		if (Dm == 21)
		{
			return D;
		}
	}
	return -1;
}

void HeavyUploader()
{
	// B1: three Shorts a day for 400 days (the plan that broke the Channel's chart). Read mid-day, past the stepped clock,
	// and at each midnight: the Channel page's 28 days and the public record's 30 (a sponsor's letter quotes it) never claim
	// more views than the channel has had, ever; and at each midnight, never more than the month really brought (the views
	// to date then, less the views to date as read forward that many days before).
	life::State L;
	video::Channel& C = L.Video;
	ss::kast::Channel K;
	Chips Bank = 0;
	int Over28 = 0, Over30 = 0, OverMonth28 = 0, OverMonth30 = 0, Reads = 0;
	double Worst = 0.0;
	std::vector<double> ToDateAt(402, 0.0); // the views to date at each midnight, read forward (exact)
	for (int D = 0; D <= 400; ++D)
	{
		if (D >= 7)
		{
			for (int Slot = 0; Slot < 3; ++Slot)
			{
				const double At = static_cast<double>(D) * Day + (3.0 + 8.0 * Slot) * 60.0;
				const std::string Id = "rl:heavy@" + std::to_string(D) + ":" + std::to_string(Slot);
				video::NoteHand(C, Moment(Id, At));
				video::Choice Ch;
				Ch.Candidates = {Id};
				Ch.F = video::Format::Short;
				Ch.Fr = video::Frame::Versus;
				video::UploadFacts F;
				F.GearQuality = 0.5;
				video::Upload(C, Ch, At + 30.0, "heavy", F);
			}
		}
		for (int Read = 0; Read < 2; ++Read)
		{
			const double W = Read == 0 ? static_cast<double>(D) * Day + 20.0 * 60.0 : static_cast<double>(D + 1) * Day;
			if (Read == 1)
			{
				video::Step(C, L, Bank, K, static_cast<double>(D) * Day, W);
			}
			const double ToDate = video::TotalViews(C, W);
			const video::ChannelView V = video::View(C, W);
			const ss::rep::Public P = ss::rep::Of(L, K, nullptr, W);
			Over28 += V.Views28 > ToDate + 0.5 ? 1 : 0;
			Over30 += static_cast<double>(P.Views30) > ToDate + 1.0 ? 1 : 0;
			Worst = std::max(Worst, ToDate > 0.0 ? std::max(V.Views28, static_cast<double>(P.Views30)) / ToDate : 0.0);
			++Reads;
			if (Read == 1)
			{
				ToDateAt[static_cast<size_t>(D + 1)] = ToDate;
				const double Month28 = ToDate - ToDateAt[static_cast<size_t>(std::max(0, D + 1 - 28))];
				const double Month30 = ToDate - ToDateAt[static_cast<size_t>(std::max(0, D + 1 - 30))];
				OverMonth28 += V.Views28 > Month28 + 0.5 ? 1 : 0;
				OverMonth30 += static_cast<double>(P.Views30) > Month30 + 1.0 ? 1 : 0;
			}
		}
	}
	std::printf("  heavy uploader: %d reads over 400 days of three Shorts a day, %zu kept, the 28 and 30 days at most %.3f of views to date\n", Reads, C.Videos.size(), Worst);
	Expect(C.Videos.size() == 60 && C.Tails[static_cast<size_t>(video::Class::Short)].Total[0] > 0.0, "heavy uploader: Shorts fold younger than a month (the case B1 broke)");
	Expect(Over28 == 0, "heavy uploader: View().Views28 never exceeds the views to date (" + std::to_string(Over28) + " reads over)");
	Expect(Over30 == 0, "heavy uploader: rep's Views30 never exceeds the views to date (" + std::to_string(Over30) + " reads over)");
	Expect(OverMonth28 == 0 && OverMonth30 == 0,
		"heavy uploader: and never more than the month brought (" + std::to_string(OverMonth28) + " and " + std::to_string(OverMonth30) + " midnights over)");
}

void SkipRecap()
{
	// S1: a hand the plan played through a live skip is "lv:<entry>:skip:<n>" (BackRoomAuto.cpp). It's the same night's
	// event as the hands watched and the recap ("lv:<entry>:recap"), so a Recap can mix them.
	life::State L;
	bool Sd = false;
	ss::room::HandNote N;
	N.AllIn = true;
	N.Called = true;
	N.Shown = {{"You", ss::ParseCards("AsAd")}, {"Mike_R", ss::ParseCards("7c7h")}};
	video::NoteLiveHand(L, BadBeatHand(Sd), &N, LiveCtx(1, 9.0 * Day + 20.0 * 60.0));
	video::LiveContext Skip = LiveCtx(1, 9.0 * Day + 21.0 * 60.0);
	Skip.EntryId += ":skip"; // BackRoomAuto.cpp's skipped hands
	Skip.Auto = true;
	ss::recall::Record Second = BadBeatHand(Sd, true);
	Second.Number = 230;
	video::NoteLiveHand(L, Second, nullptr, Skip);
	video::NoteLiveRecap(L, LiveCtx(1, 9.0 * Day + 23.0 * 60.0), 4, 119000);
	const std::vector<std::string> Ids = {"lv:embercrest-nightly@9:recap", "lv:embercrest-nightly@9:214", "lv:embercrest-nightly@9:skip:230"};
	bool All = L.Video.Open.size() == 3;
	for (const std::string& Id : Ids)
	{
		All = All && std::any_of(L.Video.Open.begin(), L.Video.Open.end(), [&Id](const video::Candidate& X) { return X.Id == Id; });
	}
	Expect(All, "skip recap: the night's watched hand, its skipped hand and its recap are candidates");
	video::Choice Ch;
	Ch.F = video::Format::Recap;
	Ch.T = video::TitleStyle::Story;
	Ch.Fr = video::Frame::Trophy;
	Ch.Candidates = Ids;
	Expect(video::CanUpload(Ch, L.Video.Open).empty(), "skip recap: a Recap mixing the night's watched and skipped hands can go up (" + video::CanUpload(Ch, L.Video.Open) + ")");
	Ch.Candidates = {Ids[0], Ids[2], "lv:embercrest-nightly@9:214"};
	Expect(video::CanUpload(Ch, L.Video.Open).empty(), "skip recap: in any order");
	// Another night's skipped hand is still another event.
	video::LiveContext Other = LiveCtx(1, 10.0 * Day + 21.0 * 60.0);
	Other.EntryId = "embercrest-nightly@10:skip";
	ss::recall::Record Third = BadBeatHand(Sd);
	Third.Number = 240;
	video::NoteLiveHand(L, Third, nullptr, Other);
	Ch.Candidates = {Ids[0], Ids[2], "lv:embercrest-nightly@10:skip:240"};
	Expect(!video::CanUpload(Ch, L.Video.Open).empty(), "skip recap: another night's skipped hand doesn't join the run's Recap");
	Ch.Candidates = Ids;
	video::UploadFacts F;
	Expect(!video::Upload(L.Video, Ch, 10.0 * Day, "skip-recap", F).empty() && L.Video.Videos.size() == 1 && L.Video.Videos[0].F == video::Format::Recap,
		"skip recap: and it goes up");
}

void ShortsConvert()
{
	// S2 (v1.2): FormatSubs in c. A Short holds people best and converts worst: its c is the formula's times 0.5, where it
	// sat on the 0.012 ceiling before. The long formats are as they were.
	video::Channel C;
	auto Conv = [&C](video::Format Fm, const std::string& Id) {
		video::NoteHand(C, Moment(Id, 30.0 * Day));
		video::Choice Ch;
		Ch.Candidates = {Id};
		Ch.F = Fm;
		Ch.T = video::TitleStyle::Story;
		video::UploadFacts F;
		F.GearQuality = 0.5;
		const std::string V = video::Upload(C, Ch, 30.0 * Day + 60.0, "convert", F);
		return V.empty() ? -1.0 : C.Videos.back().Conv;
	};
	const double Long = Conv(video::Format::Breakdown, "rl:conv@1:1");
	const double Short = Conv(video::Format::Short, "rl:conv@1:2");
	// Story: TitleSubs 1.15, title retention 1.10; a mid rig: x1.0; Niche 1.15 (every upload so far one kind group); fewer
	// than two uploads in 14 days before each: no Consistency.
	const double ShortFormula = std::clamp(0.0065 * 1.15 * (0.70 * 1.10 / 0.45) * 1.15 * 0.5, 0.002, 0.012);
	const double LongFormula = std::clamp(0.0065 * 1.15 * (0.45 * 1.10 / 0.45) * 1.15, 0.002, 0.012);
	Expect(std::fabs(Long - LongFormula) < 1e-6, "shorts: a Breakdown converts as before (" + Num(Long) + " against " + Num(LongFormula) + ")");
	Expect(std::fabs(Short - ShortFormula) < 1e-6 && Short < Long, "shorts: a Short converts at half the formula, under a Breakdown (" + Num(Short) + " against " + Num(ShortFormula) + ")");
}

void PayoutMarks()
{
	// S5: the wall calendar marks the 21st only when it pays: a Partner (the welcome came) with $100 in the balance as the
	// marks are read. The review week and a month that will roll over get no payout mark.
	const int Pay = NextPayday(60);
	Expect(Pay > 0, "marks: a 21st after day 60");
	auto PayMarks = [Pay](const video::Channel& C) {
		int N = 0;
		for (const ss::life::Mark& M : video::Marks(C, (Pay - 6.0) * Day, (Pay + 5.0) * Day))
		{
			N += M.Kind == video::MarkPayout && M.At == Pay * Day && M.Label == "Rerun payout" ? 1 : 0;
		}
		return N;
	};
	video::Channel Review;
	Review.Subs = 1100.0;
	Review.SteppedTo = (Pay - 6.0) * Day;
	Review.ReviewAt = (Pay - 8.0) * Day;
	const std::vector<ss::life::Mark> InReview = video::Marks(Review, (Pay - 6.0) * Day, (Pay + 5.0) * Day);
	Expect(PayMarks(Review) == 0 && std::any_of(InReview.begin(), InReview.end(), [](const ss::life::Mark& M) { return M.Kind == video::MarkReview; }),
		"marks: the review week marks the review's answer, never a payout");
	video::Channel Low = Review;
	Low.PartnerAt = (Pay - 1.0) * Day;
	Low.BalanceCents = 9999;
	Expect(PayMarks(Low) == 0, "marks: a Partner with $99.99 gets no payout mark (it rolls over)");
	video::Channel Full = Low;
	Full.BalanceCents = 10000;
	Expect(PayMarks(Full) == 1, "marks: a Partner with $100 gets the 21st marked");
}

void StatementWithPayout()
{
	// N5: a payout always comes with its statement, even when Rerun wrote that week (the welcome three days before); a
	// month that rolls over keeps to one routine letter a week.
	const int Pay = NextPayday(60);
	for (const Chips Balance : {Chips(12000), Chips(5000)})
	{
		video::Channel C;
		life::State M;
		Chips Bank = 0;
		ss::kast::Channel K;
		C.Subs = 1500.0;
		C.SteppedTo = (Pay - 3.0) * Day;
		C.ReviewAt = (Pay - 10.0) * Day;
		C.PartnerAt = (Pay - 3.0) * Day;
		C.BalanceCents = Balance;
		life::Letter Welcome;
		Welcome.At = (Pay - 3.0) * Day;
		Welcome.Thread = "rerun:partner";
		Welcome.Source = "video";
		Welcome.From = "Rerun";
		Welcome.Kind = video::LetterWelcome;
		Welcome.Subject = "Welcome to the Rerun Partner Program.";
		ss::life::Post(M, Welcome);
		video::Step(C, M, Bank, K, (Pay - 3.0) * Day, (Pay + 2.0) * Day);
		int Statements = 0, Lines = 0;
		for (const life::Letter& X : M.Inbox)
		{
			Statements += X.Source == "video" && X.Kind == video::LetterStatement ? 1 : 0;
		}
		for (const life::LedgerEntry& E : M.Ledger)
		{
			Lines += E.Kind == life::LedgerVideo ? 1 : 0;
		}
		if (Balance >= 10000)
		{
			Expect(Bank == Balance && Lines == 1 && Statements == 1, "statement: a payout the week of the welcome still comes with its statement");
		}
		else
		{
			Expect(Bank == 0 && Lines == 0 && Statements == 0, "statement: a rollover the week of the welcome stays quiet (one routine letter a week)");
		}
	}
}

/** True when a line talks about ads ("ad", "ads", "advert..."). */
bool TalksAds(const std::string& Line)
{
	std::string Word;
	for (size_t I = 0; I <= Line.size(); ++I)
	{
		const char Ch = I < Line.size() ? static_cast<char>(std::tolower(static_cast<unsigned char>(Line[I]))) : ' ';
		if (Ch >= 'a' && Ch <= 'z')
		{
			Word.push_back(Ch);
			continue;
		}
		if (Word == "ad" || Word == "ads" || Word.rfind("advert", 0) == 0)
		{
			return true;
		}
		Word.clear();
	}
	return false;
}

void AdviceBeforeTheGate()
{
	// N3: Studio says nothing about ads before they run (the gate, then the review); once they do, a Vlog says longer
	// videos carry more. The two-argument Advice is the before-the-gate one.
	video::Channel C;
	for (int I = 0; I < 3; ++I)
	{
		video::NoteHand(C, Moment("lv:ads@5:" + std::to_string(I), 5.0 * Day + 20.0 * 60.0 + I));
	}
	int Before = 0, Lines = 0;
	for (int F = 0; F < 4; ++F)
	{
		for (int T = 0; T < 5; ++T)
		{
			for (int Fr = 0; Fr < 5; ++Fr)
			{
				video::Choice Ch;
				Ch.F = static_cast<video::Format>(F);
				Ch.T = static_cast<video::TitleStyle>(T);
				Ch.Fr = static_cast<video::Frame>(Fr);
				Ch.Candidates = F >= 2 ? std::vector<std::string>{"lv:ads@5:0", "lv:ads@5:1", "lv:ads@5:2"} : std::vector<std::string>{"lv:ads@5:0"};
				for (const std::vector<std::string>& Set : {video::Advice(Ch, C.Open), video::Advice(Ch, C.Open, false)})
				{
					for (const std::string& A : Set)
					{
						Before += TalksAds(A) ? 1 : 0;
						++Lines;
					}
				}
			}
		}
	}
	Expect(Lines > 0 && Before == 0, "advice: no line about ads before the gate (" + std::to_string(Before) + " of " + std::to_string(Lines) + ")");
	video::Choice Vlog;
	Vlog.F = video::Format::Vlog;
	Vlog.Candidates = {"lv:ads@5:0", "lv:ads@5:1", "lv:ads@5:2"};
	const std::vector<std::string> After = video::Advice(Vlog, C.Open, true);
	Expect(std::any_of(After.begin(), After.end(), [](const std::string& A) { return TalksAds(A); }), "advice: once ads run, a Vlog says longer videos carry more");
	// N6: Session::Publish's refusals and the take-down line are public (Video.h), word for word what Studio shows.
	Expect(video::PublishWhy(4) == "Too tired to edit. Get some sleep first." && video::PublishWhy(2) == "Close your tables first. Editing needs your whole attention." &&
			   video::Deleted() == "Taken down. The subscribers it brought in stay.",
		"copy: PublishWhy and Deleted come from Video.h");
}
} // namespace video_test

int main(int Argc, char** Argv)
{
	const bool Write = Argc > 1 && std::string(Argv[1]) == "--write";
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	if (Argc > 1 && std::string(Argv[1]) == "titles")
	{
		// "video_test titles": a sample of what Studio would offer, every kind by every style (for the copy review).
		bool Sd = false;
		const ss::recall::Record Beat = video_test::BadBeatHand(Sd);
		for (int K = 0; K < ss::video::KindCountU2; ++K)
		{
			ss::video::Channel C;
			ss::video::Candidate X = video_test::Moment("rl:sample@9:1", 50.0 * video_test::Day);
			X.K = static_cast<ss::video::Kind>(K);
			X.Rec = Beat;
			X.EquityAtAllIn = 0.81;
			X.Opponent = X.K == ss::video::Kind::Rival ? ss::RivalName : "Mike_R";
			X.Rival = X.K == ss::video::Kind::Rival;
			X.Field = 104;
			if (X.K == ss::video::Kind::Story)
			{
				X.Id = "story:rent-paid";
				X.Event = "rent-paid";
			}
			if (X.K == ss::video::Kind::Win || X.K == ss::video::Kind::FinalTable || X.K == ss::video::Kind::DeepRun)
			{
				X.Rec = ss::recall::Record();
				X.Rec.Pot = 119000;
				X.PlaceSoFar = X.K == ss::video::Kind::Win ? 1 : 4;
			}
			ss::video::NoteHand(C, X);
			for (int T = 0; T < 5; ++T)
			{
				ss::video::Choice Ch;
				Ch.Candidates = {C.Open[0].Id};
				Ch.T = static_cast<ss::video::TitleStyle>(T);
				std::printf("%-12s %-8s %s\n", ss::video::KindLabel(X.K), ss::video::TitleStyleName(Ch.T), ss::video::Title(Ch, C.Open, 0).c_str());
			}
		}
		return 0;
	}
	const auto T0 = std::chrono::steady_clock::now();
	video_test::Classification();
	video_test::Candidates();
	video_test::Privacy();
	video_test::ModelGoldens(Write);
	video_test::Gate();
	video_test::Idempotence();
	video_test::Fold();
	video_test::Terms();
	video_test::Bounds();
	video_test::Saves();
	video_test::SavesU3();
	// Update 3, Phase A: Thumbnails moved to rerun_ui (tests/ui_seg_thumbs.cpp); Copy, Storytime, Rules and CoolerOutcome
	// are tests/video_seg_copy.cpp's.
	video_test::Copy();
	video_test::Storytime();
	video_test::Rules();
	video_test::HeavyUploader();
	video_test::SkipRecap();
	video_test::ShortsConvert();
	video_test::PayoutMarks();
	video_test::StatementWithPayout();
	video_test::AdviceBeforeTheGate();
	video_test::CoolerOutcome();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("video tests: %d checks, %d failed, %.1f s\n", video_test::Checks, video_test::Failures, Seconds);
	return video_test::Failures == 0 ? 0 : 1;
}
