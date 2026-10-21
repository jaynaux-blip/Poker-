// rerun_ui: ui-thumbs' checks (update 3 SPEC §7.7): every archetype inside its box at 640 x 360, 300 x 169, 211 x 119 and
// 96 x 54; the legacy frames pixel-identical; DrawPortrait's default byte-identical (the ID card, the creator, the barber);
// the cache key moving with the Look and the spec; Rerun's pool evicting least-recently-used, never the shared pool.
// Phase A: video_main.cpp's Update 2 thumbnail draws moved here verbatim (Thumbnails, with the scripted hand and the
// candidate it draws from, video_main.cpp's own helpers copied as they are). Compiled into rerun_ui_test and called from
// rerun_ui_main.cpp (the kit is UiSeg.h); the helpers are in ui_test::thumbs_seg.
#include "UiSeg.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace ui_test
{
namespace thumbs_seg
{
namespace video = ss::video;
using ss::Card;
using ss::Chips;
const double Day = 1440.0;

/** video_main.cpp's Expect, through the suite's (its messages are built strings). */
void Expect(bool Condition, const std::string& What)
{
	ui_test::Expect(Condition, What.c_str());
}

// ------------------------------------------------------------------ video_main.cpp's scripted hand and candidate, as they are

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

ss::recall::Record BadBeatHand(bool& Showdown, bool Swap = false)
{
	auto H = Swap ? HeadsUp("7c7h", "AsAd", "Kd8s2c9h7d", 6400) : HeadsUp("AsAd", "7c7h", "Kd8s2c9h7d", 6400);
	Play(*H, {PA::RaiseTo(600), PA::RaiseTo(6400), PA::Call()});
	Expect(H->bComplete, "hands: the all-in runs out");
	return RecordOf(*H, Showdown);
}

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

// ------------------------------------------------------------------ the thumbnail draws (moved from video_main.cpp)

void Thumbnails()
{
	struct Measure : ss::ui::TextMeasurer
	{
		float Width(const std::string& Text, ss::ui::Font, float Size) const override { return 0.55f * Size * static_cast<float>(Text.size()); }
		float Ascent(ss::ui::Font, float Size) const override { return Size * 0.93f; }
	};
	const Measure M;
	bool Sd = false;
	const ss::recall::Record Beat = BadBeatHand(Sd);
	const video::Kind Groups[] = {video::Kind::BadBeat, video::Kind::Bluff, video::Kind::BigPot, video::Kind::Win, video::Kind::Rival, video::Kind::Story};
	int Drawn = 0;
	for (const video::Kind K : Groups)
	{
		for (int Source = 0; Source <= 2; Source += 1)
		{
			video::Channel C;
			video::Candidate X = Moment("rl:thumb@1:" + std::to_string(static_cast<int>(K)), 40.0 * Day);
			X.K = K;
			X.Rec = Beat;
			X.Source = Source;
			X.Storytime = Source == 2;
			X.Opponent = K == video::Kind::Rival ? ss::RivalName : "Mike_R";
			X.Rival = K == video::Kind::Rival;
			X.PlaceSoFar = K == video::Kind::Win ? 1 : 0;
			if (K == video::Kind::Win)
			{
				X.Rec = ss::recall::Record();
				X.Rec.Pot = 119000;
			}
			video::NoteHand(C, X);
			for (int Fr = 0; Fr < 5; ++Fr)
			{
				video::Choice Ch;
				Ch.Candidates = {C.Open[0].Id};
				Ch.Fr = static_cast<video::Frame>(Fr);
				const video::Thumb T = video::ThumbFor(Ch, C.Open);
				ss::ui::DrawList L;
				ss::ui::Canvas Cv(L, M, 640.0f, 360.0f, 1.0f);
				ss::ui::videoart::Thumbnail(Cv, {0.0f, 0.0f, 640.0f, 360.0f}, T);
				Drawn += L.Vertices.size() > 100 && !L.Cmds.empty() ? 1 : 0;
				if (Source == 2)
				{
					Expect(T.Opponent.empty() && T.Mood >= 10, "thumbs: storytime draws no one but the player");
				}
			}
		}
	}
	Expect(Drawn == 6 * 3 * 5, "thumbs: every frame draws for every kind group (" + std::to_string(Drawn) + " of 90)");
	ss::ui::DrawList L;
	ss::ui::Canvas Cv(L, M, 400.0f, 400.0f, 1.0f);
	ss::ui::videoart::Logo(Cv, {0.0f, 0.0f, 120.0f, 120.0f});
	video::Video V;
	V.Title = "the hand that wrecked my Night Owl Turbo and then some more words that wrap";
	V.P = 1200.0;
	V.A = 0.75;
	V.Tau1 = 1.5;
	V.Tau2 = 12.0;
	V.LengthMin = 8.0;
	ss::ui::videoart::Tile(Cv, {130.0f, 0.0f, 260.0f, 220.0f}, V, 3.0 * Day);
	for (int K = 0; K < video::KindCountU2; ++K)
	{
		ss::ui::videoart::KindGlyph(Cv, 20.0f, 300.0f, 24.0f, static_cast<video::Kind>(K));
	}
	Expect(L.Vertices.size() > 200, "thumbs: the logo, a tile and every kind's glyph draw");
}
} // namespace thumbs_seg

void SegThumbs()
{
	thumbs_seg::Thumbnails();
}
} // namespace ui_test
