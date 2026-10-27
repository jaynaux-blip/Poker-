// rerun_ui: ui-thumbs' checks (update 3 SPEC §7.7): every archetype inside its box at 640 x 360, 300 x 169, 211 x 119 and
// 96 x 54; the legacy frames pixel-identical; DrawPortrait's default byte-identical (the ID card, the creator, the barber);
// the cache key moving with the Look and the spec; Rerun's pool evicting least-recently-used, never the shared pool.
// Phase A: video_main.cpp's Update 2 thumbnail draws moved here verbatim (Thumbnails, with the scripted hand and the
// candidate it draws from, video_main.cpp's own helpers copied as they are). Compiled into rerun_ui_test and called from
// rerun_ui_main.cpp (the kit is UiSeg.h); the helpers are in ui_test::thumbs_seg.
#include "UiSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/VideoContentPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/ThumbArt.h"
#include "ShortStack/UI/VideoArt.h"
#include "Trademarks.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace ss
{
namespace video
{
namespace video_detail
{
int GroupOf(Kind K); // Video.cpp: the kind group a moment's thumbnails read (0 beats .. 5 stories)
} // namespace video_detail
} // namespace video
} // namespace ss

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

/** Studio's Duel letter on a 120 x 68 option (RiverLineVideoStudio.cpp): a gold disc on an ink rim, centred DuelIn units in
 * from the tile's top-left corner. */
const float DuelIn = 2.0f, DuelRim = 8.5f, DuelGold = 7.0f;

/** One decimal, for the printed lines. */
std::string Fixed1(float V)
{
	char B[32];
	std::snprintf(B, sizeof(B), "%.1f", V);
	return B;
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

// ------------------------------------------------------------------ C3: the compositor (ui-thumbs, SPEC §6.1, §10, S-T)

namespace ta = ss::ui::thumbart;
using ss::ui::Canvas;
using ss::ui::DrawList;
using ss::ui::Rect;

/** FNV-1a 64 of a string (draw lists compared by their JSON). */
uint64_t Hash64(const std::string& S, uint64_t H = 0xcbf29ce484222325ull)
{
	for (const char Ch : S)
	{
		H ^= static_cast<unsigned char>(Ch);
		H *= 0x100000001b3ull;
	}
	return H;
}

double Ms(std::chrono::steady_clock::time_point A, std::chrono::steady_clock::time_point B)
{
	return std::chrono::duration<double, std::milli>(B - A).count();
}

/** The player the fixtures' "you" is: a made character (hoodie, stubble, the textured crop). */
ss::hero::Character Player()
{
	ss::hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Age = 26;
	C.Created = true;
	C.Appearance.OutfitColor = 5;
	return C;
}

struct Fixture
{
	std::string Name;
	video::ThumbSpec S;
	video::Thumb F;
};

/**
 * One recipe, as core-videocontent's generator would deal it for a real moment (DESIGN §4.9.1's worked examples for 24 of
 * them; the rest from the same career). Every number in a caption is the moment's own.
 */
/** A facts block as core-videocontent writes it at the end of Caption2 (video::thumb: FactSep, then "key=value" pairs). */
std::string Facts(std::initializer_list<std::pair<const char*, std::string>> Kv)
{
	std::string Out;
	for (const auto& P : Kv)
	{
		Out += '\x1f';
		Out += P.first;
		Out += '=';
		Out += P.second;
	}
	return Out;
}

/**
 * An archetype's first layout (VtTable's first column, Game/VideoThumbs.cpp): the fixtures draw their first composition
 * (C3 fix L6: five archetypes' second layout is layout 0, so a fixture at 0 would draw its second).
 */
int FirstOf(ta::Arch A)
{
	using L = ta::Layout;
	static const L First[ta::ArchCount] = {L::FaceLeftWordsRight, L::RiverCircled, L::CardsLeftFaceRight, L::FaceCentreFanBelow, L::FaceLeftWordsRight, L::Question, L::TableTopDown,
		L::BigNumberFace, L::BeforeAfter, L::TrophyHero, L::SplitVs, L::FaceLeftWordsRight, L::SplitVs, L::BigNumberFace, L::Graph, L::BigNumberFace, L::Desk, L::Venue, L::Collage, L::Letter,
		L::FaceLeftWordsRight, L::Question, L::TierList, L::TierList, L::Desk, L::SplitVs, L::Collage, L::StreamClip, L::BigNumberFace, L::TextSlab, L::FaceLeftWordsRight, L::Letter,
		L::FaceCentreFanBelow, L::CardsLeftFaceRight, L::FaceLeftWordsRight, L::CardsLeftFaceRight};
	const int I = static_cast<int>(A);
	return I >= 0 && I < ta::ArchCount ? static_cast<int>(First[I]) : 0;
}

Fixture Fx(ta::Arch A, ta::Bg B, ta::Pal P, int Expr, ss::ui::PortraitPose Pose, ta::CapStyle Cs, const char* Cap1, const std::string& Cap2, const char* Cards, const char* Opp, int Site,
	int Sticker, ta::Corner Co, int PropA, int PropB, uint32_t Variant, ta::Subject Subj = ta::Subject::You)
{
	Fixture X;
	X.Name = ta::ArchName(static_cast<int>(A));
	X.S.Arch = static_cast<int16_t>(A);
	X.S.Layout = static_cast<int16_t>(FirstOf(A));
	X.S.Bg = static_cast<int16_t>(B);
	X.S.Pal = static_cast<int16_t>(P);
	X.S.Subj = static_cast<int16_t>(Subj);
	X.S.Expr = static_cast<int16_t>(Expr);
	X.S.Pose = static_cast<int16_t>(Pose);
	X.S.CapStyle = static_cast<int16_t>(Cs);
	X.S.Sticker = static_cast<int16_t>(Sticker);
	X.S.Corner = static_cast<int16_t>(Co);
	X.S.PropA = static_cast<int16_t>(PropA);
	X.S.PropB = static_cast<int16_t>(PropB);
	X.S.Variant = Variant;
	X.S.Caption = Cap1;
	X.S.Caption2 = Cap2;
	X.F.Fr = video::Frame::Recipe;
	X.F.Cards = Cards;
	X.F.Opponent = Opp;
	X.F.Site = Site;
	return X;
}

/** The 35 P0 archetypes, one recipe each (POV, P1, is drawn as the text slab and checked apart). */
std::vector<Fixture> Fixtures()
{
	using A = ta::Arch;
	using B = ta::Bg;
	using P = ta::Pal;
	using C = ta::CapStyle;
	using Po = ss::ui::PortraitPose;
	using Co = ta::Corner;
	using St = ta::Sticker;
	using Pr = ta::Prop;
	const int N = -1;
	auto S = [](St V) { return static_cast<int>(V); };
	auto R = [](Pr V) { return static_cast<int>(V); };
	const char* Beat = "AsAd/7c7h/Kd8s2c9h7h";
	return {
		Fx(A::GutPunch, B::RadialBurst, P::Heat, 6, Po::HandsOnHead, C::TwoTone, "81%", "AND LOST", Beat, "Mike_R", 0, N, Co::Initials, N, N, 0x00u),
		Fx(A::CircledRiver, B::Spotlight, P::Night, 4, Po::None, C::Boxed, "THE RIVER.", Facts({{"c", Beat}, {"k", "4"}}), Beat, "Mike_R", 0, N, Co::None, N, N, 0x01u),
		Fx(A::TheRead, B::FeltBokeh, P::GoldOnBlack, 9, Po::None, C::Outlined, "HERO CALL", "", "KhJh//Qs9d4c2h3s", "Mike_R", 0, N, Co::Logo, N, N, 0x21u),
		Fx(A::NervesOfSteel, B::SpeedLines, P::Neon, 8, Po::None, C::Marker, "7-2", "46 BB", "7c2d//Kh9s4d", "", 0, S(St::Crown), Co::None, N, N, 0x20u),
		Fx(A::Caught, B::FlatGradient, P::Blood, 11, Po::Facepalm, C::Outlined, "CAUGHT", "38 BB", "Jc9c/QhQd/Ah8c3c5s2d", "Lou_P", 0, S(St::Skull), Co::Initials, N, N, 0x10u),
		Fx(A::TheFold, B::FlatGradient, P::Ice, 10, Po::None, C::TwoTone, "FOLD?", "9 LEFT", "KsKh//", "", 0, N, Co::Initials, N, N, 0x00u),
		Fx(A::ThePile, B::RadialBurst, P::Money, 2, Po::None, C::NumberBurst, "64 BB", "", "AhKh//", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::BigNumber, B::RadialBurst, P::Sunrise, 3, Po::FistPump, C::Outlined, "4TH OF 1,530", Facts({{"n", "$1,190"}}), "", "", 0, S(St::MoneyBag), Co::Initials, R(Pr::CashStack), N, 0x11u),
		Fx(A::BeforeAfter, B::FlatGradient, P::Sunrise, 3, Po::FistPump, C::Outlined, "4TH OF 1,530", Facts({{"f", "$11"}, {"t", "$1,190"}}), "", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::TrophyShot, B::RadialBurst, P::GoldOnBlack, 3, Po::None, C::Outlined, "I WON", Facts({{"e", "EMBERCREST SUNDAY"}}), "", "", 0, N, Co::Initials, N, R(Pr::EventCrest), 0x21u),
		Fx(A::VsSplit, B::DiagonalSplit, P::Blood, 4, Po::None, C::Stamp, "SET vs SET", Facts({{"c", "8c8d/9s9h/9c8s2h"}, {"o", "Nines_NY"}}), "", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::TheBubble, B::Spotlight, P::Night, 4, Po::HandsOnHead, C::TwoTone, "1 AWAY", "18 BB", "", "", 0, S(St::Siren), Co::Initials, N, N, 0x01u),
		Fx(A::Headhunter, B::SpeedLines, P::Heat, 8, Po::None, C::Outlined, "KNOCKOUT", Facts({{"n", "$110"}, {"o", "VillainSeven"}}), "", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::Envelope, B::MoneyRain, P::GoldOnBlack, 5, Po::None, C::NumberBurst, "25X", Facts({{"m", "25"}}), "", "", 0, N, Co::Initials, N, N, 0x00u),
		Fx(A::TheGraph, B::FlatGradient, P::Money, 1, Po::Point, C::NumberBurst, "+$412", "214 EVENTS" + Facts({{"g", "u13245789"}}), "", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::DayCounter, B::SpeedLines, P::Site, 2, Po::FistPump, C::Outlined, "DAY 19", "$50 \xC2\xB7 $214 \xC2\xB7 $500" + Facts({{"p", "36"}, {"s", "1"}}), "", "", 0, N, Co::None, N, N, 0x10u),
		Fx(A::NightShift, B::FlatGradient, P::Night, 11, Po::None, C::Outlined, "4:12 AM", "6 TABLES", "", "", 0, N, Co::Initials, R(Pr::PhoneChat), N, 0x20u),
		Fx(A::Venue, B::Spotlight, P::Heat, 1, Po::None, C::Boxed, "FIRST LIVE", "TOURNAMENT", "", "", 0, N, Co::ColourBar, N, N, 0x00u),
		Fx(A::RoadTrip, B::FlatGradient, P::Night, 2, Po::Point, C::TwoTone, "ROAD TRIP", "BUS + BUY-IN", "", "", 0, N, Co::None, R(Pr::Flag), N, 0x00u),
		Fx(A::RentLetter, B::Spotlight, P::Money, 3, Po::None, C::Boxed, "RENT PAID.", "$1,075", "", "", 0, N, Co::Initials, N, N, 0x20u),
		Fx(A::Classroom, B::FlatGradient, P::Night, 1, Po::Point, C::Boxed, "ICM", "18 BB", "AhQh//", "", 0, N, Co::None, N, N, 0x20u),
		Fx(A::Quiz, B::DiagonalSplit, P::Ice, 9, Po::None, C::TwoTone, "CALL?", "40 BB RIVER", "", "", 0, N, Co::Logo, N, N, 0x00u),
		Fx(A::TierList, B::FlatGradient, P::Mono, 1, Po::None, C::Outlined, "EVERY SUNDAY", "RANKED" + Facts({{"r", "SUNDAY BLAZE|NIGHT OWL TURBO|THE DEEP STACK|MIDNIGHT KO"}}), "", "", 0, N, Co::None, N, N, 0x020u),
		Fx(A::TopFive, B::RadialBurst, P::Sunrise, 2, Po::None, C::Outlined, "TOP 5 HANDS", "OCTOBER" + Facts({{"r", "QUADS NINES|ACES HOLD|THE 7-2 BLUFF|SET OVER SET|RIVER FLUSH"}}), "", "", 0, N,
			Co::Initials, N, N, 0x00u),
		Fx(A::DeskSetup, B::FlatGradient, P::Brand, 1, Po::None, C::Outlined, "$1,407 SETUP", "", "", "", 0, N, Co::None, R(Pr::Gear), N, 0x00u),
		Fx(A::CollabDuo, B::RadialBurst, P::Neon, 2, Po::None, C::Boxed, "COLLAB", "WITH MISSFINCH", "", "MissFinch", 0, N, Co::Initials, N, N, 0x00u),
		Fx(A::ReactionBox, B::RadialBurst, P::GoldOnBlack, 4, Po::None, C::NumberBurst, "$1.2M", "", "", "Tomas Ekdahl", 0, N, Co::None, N, N, 0x00u),
		Fx(A::StreamClip, B::Scanlines, P::Neon, 5, Po::FistPump, C::Stamp, "CHAT LOST IT",
			Facts({{"c", "//JdJc4sQhJs"}, {"h", "tilted_teddy: NO WAY"}, {"h", "ruby_rail: QUADS??"}}), "", "", 0, N, Co::None, N, N, 0x20u),
		Fx(A::Milestone, B::RadialBurst, P::Sunrise, 3, Po::None, C::Outlined, "SUBSCRIBERS", Facts({{"n", "1,000"}}), "", "", 0, N, Co::Initials, N, N, 0x00u),
		Fx(A::TextSlab, B::FlatGradient, P::Mono, 11, Po::None, C::None, "HE HAD IT.", "", "", "", 0, N, Co::None, N, N, 0x000u),
		Fx(A::TheGhost, B::FlatGradient, P::Neon, 7, Po::None, C::Stamp, "HIM AGAIN", "44 BB", "", ss::RivalName, 0, N, Co::Initials, N, N, 0x20u),
		Fx(A::Payday, B::Spotlight, P::Money, 3, Po::None, C::Boxed, "FIRST PAYOUT", "$163.71", "", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::MonsterHand, B::RadialBurst, P::Felt, 4, Po::None, C::Outlined, "QUADS", "", "9h9d//9s9c4hKd2s", "", 0, N, Co::Initials, N, N, 0x20u),
		Fx(A::SiteCard, B::RadialBurst, P::Site, 5, Po::HandsOnHead, C::None, "$27.50", Facts({{"m", "25"}, {"s", "1"}}), "", "", 0, N, Co::None, N, N, 0x00u),
		Fx(A::Mistake, B::FlatGradient, P::Blood, 11, Po::Facepalm, C::Marker, "MY WORST FOLD", "", "QsQd//", "", 0, N, Co::Initials, N, N, 0x00u),
	};
}

DrawList DrawOne(const video::ThumbSpec& S, const video::Thumb& F, const ss::hero::Character& Look, Rect R, bool Lod)
{
	TableMeasurer M;
	DrawList L;
	Canvas C(L, M, R.X + R.W + 10.0f, R.Y + R.H + 10.0f, 1.0f);
	ta::Compose(C, R, S, Look, Lod, F);
	return L;
}

/** Every triangle and text run sits inside a clip no larger than R (nothing escapes the tile), and the clip is R. */
bool InsideBox(const DrawList& L, Rect R)
{
	int Depth = 0;
	bool Ok = !L.Cmds.empty();
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		switch (Cmd.Type)
		{
		case ss::ui::DrawCmd::Kind::PushClip:
			if (Depth == 0)
			{
				Ok = Ok && std::fabs(Cmd.Clip.X - R.X) < 0.01f && std::fabs(Cmd.Clip.Y - R.Y) < 0.01f && std::fabs(Cmd.Clip.W - R.W) < 0.01f && std::fabs(Cmd.Clip.H - R.H) < 0.01f;
			}
			else
			{
				Ok = Ok && Cmd.Clip.X >= R.X - 0.01f && Cmd.Clip.Y >= R.Y - 0.01f && Cmd.Clip.X + Cmd.Clip.W <= R.X + R.W + 0.01f && Cmd.Clip.Y + Cmd.Clip.H <= R.Y + R.H + 0.01f;
			}
			++Depth;
			break;
		case ss::ui::DrawCmd::Kind::PopClip: --Depth; break;
		default: Ok = Ok && Depth > 0; break;
		}
	}
	return Ok && Depth == 0;
}

/** The widest text run's overflow past R's sides (0 when every caption fits), in units. */
float TextOverflow(const DrawList& L, Rect R)
{
	TableMeasurer M;
	float Worst = 0.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		Worst = std::max(Worst, std::max(R.X - Cmd.Text.X, Cmd.Text.X + W - (R.X + R.W)));
	}
	return Worst;
}

int Runs(const DrawList& L)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	return N;
}

const float Sizes[4][2] = {{640.0f, 360.0f}, {300.0f, 169.0f}, {211.0f, 119.0f}, {96.0f, 54.0f}};

void Archetypes()
{
	const ss::hero::Character Me = Player();
	const std::vector<Fixture> All = Fixtures();
	Expect(All.size() == 35, "compositor: 35 P0 archetypes in the fixtures");
	std::vector<int> Seen(ta::ArchCount, 0);
	int Inside = 0, Fits = 0, Same = 0, Drawn = 0, LodFewer = 0, Mirrored = 0, UnderBudget = 0;
	size_t MaxVerts = 0;
	std::string MaxName;
	for (const Fixture& X : All)
	{
		Seen[static_cast<size_t>(X.S.Arch)] += 1;
		for (const auto& Sz : Sizes)
		{
			const Rect R{7.0f, 5.0f, Sz[0], Sz[1]};
			const bool Lod = ta::LodFor(Sz[0]);
			const DrawList L = DrawOne(X.S, X.F, Me, R, Lod);
			Drawn += L.Vertices.size() > 200 ? 1 : 0;
			Inside += InsideBox(L, R) ? 1 : 0;
			const float Over = TextOverflow(L, R);
			Fits += Over <= Sz[0] * 0.03f ? 1 : 0;
			if (Over > Sz[0] * 0.03f)
			{
				std::printf("  thumbs overflow %-16s at %3.0f x %3.0f: %.1f units\n", X.Name.c_str(), Sz[0], Sz[1], Over);
			}
			Same += DrawOne(X.S, X.F, Me, R, Lod).ToJson() == L.ToJson() ? 1 : 0;
			if (Sz[0] == 640.0f)
			{
				if (L.Vertices.size() > MaxVerts)
				{
					MaxVerts = L.Vertices.size();
					MaxName = X.Name;
				}
				UnderBudget += L.Vertices.size() <= 14000 ? 1 : 0;
				if (L.Vertices.size() > 14000)
				{
					std::printf("  thumbs budget   %-16s %zu vertices at 640 x 360 (> 14k)\n", X.Name.c_str(), L.Vertices.size());
				}
				const DrawList Small = DrawOne(X.S, X.F, Me, R, true);
				LodFewer += Small.Vertices.size() < L.Vertices.size() ? 1 : 0;
				video::ThumbSpec Mir = X.S;
				Mir.Mirror = 1;
				Mirrored += DrawOne(Mir, X.F, Me, R, false).ToJson() != L.ToJson() ? 1 : 0;
			}
		}
	}
	const int Cells = static_cast<int>(All.size()) * 4;
	int Covered = 0;
	for (int A = 0; A < ta::ArchCount; ++A)
	{
		Covered += Seen[static_cast<size_t>(A)] > 0 ? 1 : 0;
	}
	std::printf("  thumbs compose  35 archetypes x 4 sizes: drew %d, inside %d, captions fit %d, deterministic %d of %d; LOD lighter %d, mirror moves %d of 35; "
				"max %zu vertices (%s)\n",
		Drawn, Inside, Fits, Same, Cells, LodFewer, Mirrored, MaxVerts, MaxName.c_str());
	Expect(Covered == 35 && Seen[static_cast<size_t>(ta::Arch::Pov)] == 0, "compositor: every P0 archetype (not POV) has a recipe");
	Expect(Drawn == Cells, "compositor: every archetype draws at 640, 300, 211 and 96 wide");
	Expect(Inside == Cells, "compositor: every archetype stays inside its box (one clip, the tile's)");
	Expect(Fits == Cells, "compositor: every caption fits its tile");
	Expect(Same == Cells, "compositor: a recipe draws the same every time");
	Expect(LodFewer == 35, "compositor: the LOD draws fewer vertices than the full thumbnail");
	Expect(Mirrored == 35, "compositor: Mirror moves every composition");
	Expect(UnderBudget == 35, "compositor: every thumbnail at 640 x 360 within 14k vertices with its portrait (§10)");
}

/** video::thumb's facts block: read, drawn, never shown as text, and it outranks the Thumb (storytime names nobody). */
void FactsBlock()
{
	const ss::hero::Character Me = Player();
	const std::vector<Fixture> All = Fixtures();
	const Rect R{0.0f, 0.0f, 640.0f, 360.0f};
	auto Find = [&All](ta::Arch A) {
		for (const Fixture& X : All)
		{
			if (X.S.Arch == static_cast<int16_t>(A))
			{
				return X;
			}
		}
		return All[0];
	};
	int Hidden = 0;
	for (const Fixture& X : All)
	{
		const DrawList L = DrawOne(X.S, X.F, Me, R, false);
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			Hidden += Cmd.Type == ss::ui::DrawCmd::Kind::Text && (Cmd.Text.Text.find('\x1f') != std::string::npos || Cmd.Text.Text.find('=') != std::string::npos) ? 1 : 0;
		}
	}
	Expect(Hidden == 0, "facts: a facts block is never drawn as words");
	Expect(Drew(DrawOne(Find(ta::Arch::StreamClip).S, video::Thumb(), Me, R, false), "tilted_teddy: NO WAY"), "facts: the moment's chat lines (h) are the clip's");
	Expect(Drew(DrawOne(Find(ta::Arch::TierList).S, video::Thumb(), Me, R, false), "SUNDAY BLAZE"), "facts: a tier list's rows (r) are drawn");
	Expect(Drew(DrawOne(Find(ta::Arch::TrophyShot).S, video::Thumb(), Me, R, false), "EMBERCREST SUNDAY"), "facts: the event's name (e) is on the trophy");
	Expect(Drew(DrawOne(Find(ta::Arch::BigNumber).S, video::Thumb(), Me, R, false), "$1,190"), "facts: the big number (n) is drawn");
	Expect(Drew(DrawOne(Find(ta::Arch::BeforeAfter).S, video::Thumb(), Me, R, false), "$11") && Drew(DrawOne(Find(ta::Arch::BeforeAfter).S, video::Thumb(), Me, R, false), "$1,190"),
		"facts: Before -> After's panels (f, t)");
	// A fact the block leaves out comes from the Thumb (ThumbFor names nobody in storytime): no o and an empty Thumb is
	// nobody's silhouette; no o and a named Thumb, that opponent's face; an o in the block wins over the Thumb's name.
	Fixture Vs = Find(ta::Arch::VsSplit);
	video::Thumb Named;
	Named.Opponent = "Lou_P";
	video::ThumbSpec NoOne = Vs.S;
	NoOne.Caption2 = Facts({{"c", "8c8d//9c8s2h"}});
	Expect(Drew(DrawOne(NoOne, video::Thumb(), Me, R, false), "?") && !Drew(DrawOne(NoOne, Named, Me, R, false), "?"), "facts: no o in the block reads the Thumb's opponent (storytime: nobody)");
	Expect(DrawOne(Vs.S, Named, Me, R, false).ToJson() == DrawOne(Vs.S, video::Thumb(), Me, R, false).ToJson(), "facts: the block's o wins over the Thumb's");
	video::ThumbSpec Ev = Find(ta::Arch::ReactionBox).S;
	Ev.Caption2 = Facts({{"o", "Tomas Ekdahl"}, {"e", "SUNDAY BLAZE"}});
	video::ThumbSpec Venue = Find(ta::Arch::Venue).S;
	Venue.Caption2 = "TOURNAMENT" + Facts({{"e", "EMBERCREST NIGHTLY"}});
	Expect(Drew(DrawOne(Ev, video::Thumb(), Me, R, false), "SUNDAY BLAZE") && Drew(DrawOne(Venue, video::Thumb(), Me, R, false), "EMBERCREST NIGHTLY"), "facts: the event (e) on Venue and Reaction Box");
	// The old-style spec (no block) reads the Thumb's facts.
	video::ThumbSpec Old = Vs.S;
	Old.Caption2.clear();
	Expect(Drew(DrawOne(Old, video::Thumb(), Me, R, false), "?") && !Drew(DrawOne(Old, Named, Me, R, false), "?"), "facts: a spec with no block reads the Thumb");
	// Mirror's bit 1 is the wide zoom; bit 0 the mirror.
	video::ThumbSpec Wide = Find(ta::Arch::GutPunch).S;
	const std::string Tight = DrawOne(Wide, All[0].F, Me, R, false).ToJson();
	Wide.Mirror = 2;
	const std::string WideJ = DrawOne(Wide, All[0].F, Me, R, false).ToJson();
	Wide.Mirror = 3;
	const std::string Both = DrawOne(Wide, All[0].F, Me, R, false).ToJson();
	Expect(Tight != WideJ && WideJ != Both && Tight != Both, "facts: Mirror bit 1 zooms wide, bit 0 mirrors");
	// The arrow in a caption still splits Before -> After when the recipe has no f and t.
	video::ThumbSpec Arrow = Find(ta::Arch::BeforeAfter).S;
	Arrow.Caption = "$11 \xE2\x86\x92 $1,190";
	Arrow.Caption2 = "4TH OF 1,530";
	const DrawList Al = DrawOne(Arrow, video::Thumb(), Me, R, false);
	Expect(DrewRun(Al, "$11") && DrewRun(Al, "$1,190") && DrewRun(Al, "4TH OF 1,530"), "facts: a caption's arrow splits Before -> After's panels");
}

/** Values out of range (a newer save's, a bad recipe) draw as the first value; POV draws (as the text slab in P0). */
void OutOfRange()
{
	const ss::hero::Character Me = Player();
	const Rect R{0.0f, 0.0f, 300.0f, 169.0f};
	video::ThumbSpec S;
	S.Arch = 99;
	S.Layout = 77;
	S.Bg = -4;
	S.Pal = 99;
	S.Subj = 40;
	S.Expr = 55;
	S.Pose = 90;
	S.PropA = 500;
	S.PropB = -9;
	S.CapStyle = 31;
	S.Sticker = 70;
	S.Corner = 12;
	S.Caption = "ODD";
	video::Thumb F;
	F.Cards = "garbage/zz/";
	F.Site = 99;
	const DrawList L = DrawOne(S, F, Me, R, false);
	Expect(L.Vertices.size() > 100 && InsideBox(L, R), "compositor: out-of-range values draw, inside the tile");
	video::ThumbSpec Pov = Fixtures()[29].S;
	Pov.Arch = static_cast<int16_t>(ta::Arch::Pov);
	Expect(DrawOne(Pov, video::Thumb(), Me, R, false).ToJson() == DrawOne(Fixtures()[29].S, video::Thumb(), Me, R, false).ToJson(), "compositor: POV (P1) draws as the text slab");
	video::ThumbSpec None = Fixtures()[0].S;
	const DrawList Empty = DrawOne(None, video::Thumb(), Me, {0.0f, 0.0f, 0.0f, 0.0f}, false);
	Expect(Empty.Cmds.empty(), "compositor: an empty rect draws nothing");
}

/** The faces: 12 distinct expressions, the 4 poses' hands, P1 poses as none, the LOD lighter, CharacterFor stable. */
void Faces()
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	auto Face = [&](int Expr, ss::ui::PortraitPose P, bool Lod) {
		DrawList L;
		Canvas C(L, M, 600.0f, 600.0f, 1.0f);
		ss::ui::DrawPortrait(C, Me, 300.0f, 260.0f, 1.0f, 0.0, ss::ui::PortraitLight(), ss::ui::ExpressionOf(Expr), P, Lod);
		return L;
	};
	std::vector<uint64_t> Hashes;
	for (int E = 0; E < 12; ++E)
	{
		Hashes.push_back(Hash64(Face(E, ss::ui::PortraitPose::None, false).ToJson()));
	}
	std::vector<uint64_t> Sorted = Hashes;
	std::sort(Sorted.begin(), Sorted.end());
	Expect(std::unique(Sorted.begin(), Sorted.end()) == Sorted.end(), "faces: the 12 expressions all differ");
	Expect(&ss::ui::ExpressionOf(12) == &ss::ui::ExpressionOf(0) && &ss::ui::ExpressionOf(-3) == &ss::ui::ExpressionOf(0), "faces: an expression out of range is the neutral one");
	const size_t Bare = Face(1, ss::ui::PortraitPose::None, false).Vertices.size();
	int Hands = 0;
	for (int P = 1; P <= 4; ++P)
	{
		const size_t V = Face(1, static_cast<ss::ui::PortraitPose>(P), false).Vertices.size();
		Hands += V > Bare + 150 ? 1 : 0;
	}
	Expect(Hands == 4, "faces: each of the 4 P0 poses draws its arms and hands");
	Expect(Face(1, ss::ui::PortraitPose::Shrug, false).ToJson() == Face(1, ss::ui::PortraitPose::None, false).ToJson(), "faces: a P1 pose draws no hands until it lands");
	// The LOD on the heaviest looks (a buzz cut and stubble: stipple; braids; an afro's curls).
	int Lighter = 0;
	for (const int Hair : {1, 7, 10})
	{
		ss::hero::Character H = Me;
		H.Appearance.Hair = Hair;
		DrawList Full, Small;
		Canvas Cf(Full, M, 600.0f, 600.0f, 1.0f), Cs(Small, M, 600.0f, 600.0f, 1.0f);
		ss::ui::DrawPortrait(Cf, H, 300.0f, 260.0f, 1.0f, 0.0, ss::ui::PortraitLight(), ss::ui::ExpressionOf(2), ss::ui::PortraitPose::None, false);
		ss::ui::DrawPortrait(Cs, H, 300.0f, 260.0f, 1.0f, 0.0, ss::ui::PortraitLight(), ss::ui::ExpressionOf(2), ss::ui::PortraitPose::None, true);
		Lighter += Small.Vertices.size() * 10 < Full.Vertices.size() * 8 ? 1 : 0;
		std::printf("  thumbs portrait hair %2d: %zu vertices full, %zu at the LOD\n", Hair, Full.Vertices.size(), Small.Vertices.size());
	}
	Expect(Lighter == 3, "faces: the LOD drops a fifth or more of a detailed head's vertices");
	// hero::CharacterFor: the same name, the same face; names spread over many looks.
	Expect(ss::hero::CharacterFor("Nines_NY").Appearance == ss::hero::CharacterFor("Nines_NY").Appearance && ss::hero::CharacterFor("Nines_NY").FirstName == "Nines_NY",
		"faces: CharacterFor is the same for a name, named by it");
	std::vector<std::string> Looks;
	for (int I = 0; I < 60; ++I)
	{
		const ss::hero::Look L = ss::hero::CharacterFor("player_" + std::to_string(I)).Appearance;
		Looks.push_back(std::to_string(L.Face) + "." + std::to_string(L.Skin) + "." + std::to_string(L.Hair) + "." + std::to_string(L.HairColor) + "." + std::to_string(L.Outfit));
	}
	std::sort(Looks.begin(), Looks.end());
	const size_t Distinct = static_cast<size_t>(std::unique(Looks.begin(), Looks.end()) - Looks.begin());
	Expect(Distinct >= 50, "faces: 60 names give at least 50 different looks");
}

/** The cache key moves with everything a thumbnail draws from, and only with that. */
void Keys()
{
	const ss::hero::Character Me = Player();
	const Fixture X = Fixtures()[0];
	const uint64_t K = ta::KeyOf(X.S, Me, X.F, 300.0f, 169.0f, false);
	Expect(K == ta::KeyOf(X.S, Me, X.F, 300.0f, 169.0f, false), "keys: the same inputs, the same key");
	int Moves = 0, Tries = 0;
	auto Try = [&](uint64_t Other) {
		++Tries;
		Moves += Other != K ? 1 : 0;
	};
	ss::hero::Character Cut = Me;
	Cut.Appearance.Hair = 5;
	Try(ta::KeyOf(X.S, Cut, X.F, 300.0f, 169.0f, false));
	ss::hero::Character Older = Me;
	Older.Age = 52;
	Try(ta::KeyOf(X.S, Older, X.F, 300.0f, 169.0f, false));
	Try(ta::KeyOf(X.S, Me, X.F, 301.0f, 169.0f, false));
	Try(ta::KeyOf(X.S, Me, X.F, 300.0f, 170.0f, false));
	Try(ta::KeyOf(X.S, Me, X.F, 300.0f, 169.0f, true));
	video::Thumb F2 = X.F;
	F2.Cards = "KsKd/7c7h/Kd8s2c9h7h";
	Try(ta::KeyOf(X.S, Me, F2, 300.0f, 169.0f, false));
	F2 = X.F;
	F2.Opponent = "Lou_P";
	Try(ta::KeyOf(X.S, Me, F2, 300.0f, 169.0f, false));
	for (int Field = 0; Field < 17; ++Field)
	{
		video::ThumbSpec S = X.S;
		int16_t* Fields[14] = {&S.Arch, &S.Layout, &S.Mirror, &S.Bg, &S.Pal, &S.Subj, &S.Expr, &S.Pose, &S.PropA, &S.PropB, &S.CapStyle, &S.Sticker, &S.Corner, &S.LookRef};
		if (Field < 14)
		{
			*Fields[Field] = static_cast<int16_t>(*Fields[Field] + 1);
		}
		else if (Field == 14)
		{
			S.Variant += 1;
		}
		else if (Field == 15)
		{
			S.Caption += "!";
		}
		else
		{
			S.Caption2 += "!";
		}
		Try(ta::KeyOf(S, Me, X.F, 300.0f, 169.0f, false));
	}
	Expect(Moves == Tries, "keys: the Look, the size, the LOD, the facts and every recipe field move the key");
}

/** Progressive paint (V12): at most 2 uncached a frame, skeletons until their turn; Rerun's pool never flushes the shared one. */
void Progressive()
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	std::vector<Fixture> All = Fixtures();
	// Six thumbnails no frame has painted yet (a variant seed of their own).
	for (size_t I = 0; I < 6; ++I)
	{
		All[I].S.Variant = 0x51ee7000u + static_cast<uint32_t>(I);
	}
	// A shared-pool fragment that must survive Rerun's churn.
	const uint64_t SharedKey = 0x5348415245440001ull;
	{
		DrawList L;
		Canvas C(L, M, 400.0f, 400.0f, 1.0f);
		C.Cached(SharedKey, 0.0f, 0.0f, [](Canvas& Cv) { Cv.FillRect({0.0f, 0.0f, 10.0f, 10.0f}, ss::ui::Hex(0x123456)); });
	}
	std::vector<int> Painted, Waiting;
	for (int Frame = 0; Frame < 4; ++Frame)
	{
		DrawList L;
		Canvas C(L, M, 1400.0f, 400.0f, 1.0f);
		ta::Budget B;
		for (size_t I = 0; I < 6; ++I)
		{
			ta::Draw(C, {static_cast<float>(I) * 220.0f, 10.0f, 211.0f, 119.0f}, All[I].S, Me, All[I].F, B);
		}
		Painted.push_back(B.Painted);
		Waiting.push_back(B.Waiting);
	}
	Expect(Painted[0] == 2 && Waiting[0] == 4 && Painted[1] == 2 && Waiting[1] == 2 && Painted[2] == 2 && Waiting[2] == 0 && Painted[3] == 0 && Waiting[3] == 0,
		"progressive: 2 painted a frame, the rest skeletons until their turn, then all replayed");
	{
		DrawList L;
		Canvas C(L, M, 400.0f, 400.0f, 1.0f);
		Expect(C.IsCached(ta::KeyOf(All[5].S, Me, All[5].F, 211.0f, 119.0f, false)), "progressive: a painted thumbnail is cached (IsCached)");
		// Fill Rerun's pool past a small budget: its own oldest go, the shared fragment stays.
		Canvas::SetPoolBudget(Canvas::CachePool::Rerun, 30000);
		ta::Budget B;
		B.Max = 1000;
		for (size_t I = 0; I < All.size(); ++I)
		{
			ta::Draw(C, {0.0f, 0.0f, 300.0f, 169.0f}, All[I].S, Me, All[I].F, B);
		}
		Expect(C.IsCached(SharedKey), "progressive: Rerun's pool never flushes the shared pool");
		Expect(!C.IsCached(ta::KeyOf(All[0].S, Me, All[0].F, 300.0f, 169.0f, false)) && C.IsCached(ta::KeyOf(All.back().S, Me, All.back().F, 300.0f, 169.0f, false)),
			"progressive: Rerun's pool drops its least recently used past its budget");
		Canvas::SetPoolBudget(Canvas::CachePool::Rerun, 0);
	}
	// A skeleton is cheap and stays in the tile.
	DrawList Sk;
	Canvas Cs(Sk, M, 400.0f, 400.0f, 1.0f);
	ta::Skeleton(Cs, {10.0f, 10.0f, 300.0f, 169.0f}, All[0].S);
	Expect(Sk.Vertices.size() < 200 && Runs(Sk) == 0, "progressive: a skeleton is a few rects, no words");
}

/** The legacy path: an Update 2 video's frame through VideoThumb is videoart::Thumbnail's, byte for byte; frames pinned. */
void Legacy()
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	bool Sd = false;
	const ss::recall::Record Beat = BadBeatHand(Sd);
	const video::Kind Groups[] = {video::Kind::BadBeat, video::Kind::Bluff, video::Kind::BigPot, video::Kind::Win, video::Kind::Rival, video::Kind::Story};
	uint64_t All = 0xcbf29ce484222325ull;
	int Same = 0, Total = 0;
	for (const video::Kind K : Groups)
	{
		for (int Source = 0; Source <= 2; ++Source)
		{
			video::Channel C;
			video::Candidate X = Moment("rl:thumb@1:" + std::to_string(static_cast<int>(K)), 40.0 * Day);
			X.K = K;
			X.Rec = Beat;
			X.Source = Source;
			X.Storytime = Source == 2;
			X.Opponent = K == video::Kind::Rival ? ss::RivalName : "Mike_R";
			video::NoteHand(C, X);
			for (int Fr = 0; Fr < 5; ++Fr)
			{
				video::Choice Ch;
				Ch.Candidates = {C.Open[0].Id};
				Ch.Fr = static_cast<video::Frame>(Fr);
				video::Video V;
				V.Th = video::ThumbFor(Ch, C.Open);
				DrawList A, B;
				Canvas Ca(A, M, 700.0f, 400.0f, 1.0f), Cb(B, M, 700.0f, 400.0f, 1.0f);
				ss::ui::videoart::Thumbnail(Ca, {10.0f, 10.0f, 640.0f, 360.0f}, V.Th, &Me);
				ta::Budget Bu;
				ta::VideoThumb(Cb, {10.0f, 10.0f, 640.0f, 360.0f}, V, C, Me, Bu);
				const std::string J = A.ToJson();
				Same += J == B.ToJson() ? 1 : 0;
				++Total;
				All = Hash64(J, All);
			}
		}
	}
	// Videoart's own tile for an Update 2 video, as Update 2 drew it.
	video::Video U2;
	U2.Title = "the hand that wrecked my Night Owl Turbo and then some more words that wrap";
	U2.P = 1200.0;
	U2.A = 0.75;
	U2.Tau1 = 1.5;
	U2.Tau2 = 12.0;
	U2.LengthMin = 8.0;
	U2.Th.Fr = video::Frame::Cards;
	U2.Th.Cards = "AsAd/7c7h/Kd8s2c9h7h";
	U2.Th.BigText = "ALL IN";
	DrawList Tl;
	Canvas Ct(Tl, M, 400.0f, 400.0f, 1.0f);
	ss::ui::videoart::Tile(Ct, {20.0f, 10.0f, 300.0f, 260.0f}, U2, 3.0 * Day);
	const uint64_t TileHash = Hash64(Tl.ToJson());
	std::printf("  thumbs legacy   %d of %d Update 2 frames identical through VideoThumb; the frames' fnv %016llx, the tile's %016llx\n", Same, Total, static_cast<unsigned long long>(All),
		static_cast<unsigned long long>(TileHash));
	Expect(Same == Total, "legacy: an Update 2 video's frame through VideoThumb is videoart::Thumbnail's, byte for byte");
	// Pinned from ecdf2de's VideoArt.cpp (c3-day0): the 90 frames and an Update 2 tile, pixel for pixel.
	Expect(All == 0xa1d532b0ac6c6253ull, "legacy: Update 2's 90 frames draw exactly as at C3's day 0");
	Expect(TileHash == 0x2c0c5be55542a1d7ull, "legacy: an Update 2 video's tile draws exactly as at C3's day 0");
	// The same tile for a Rerun 2.0 video draws its recipe.
	video::Video U3 = U2;
	U3.X.On = true;
	U3.X.Spec[0] = Fixtures()[0].S;
	DrawList T3;
	Canvas C3(T3, M, 400.0f, 400.0f, 1.0f);
	ss::ui::videoart::Tile(C3, {20.0f, 10.0f, 300.0f, 260.0f}, U3, 3.0 * Day);
	Expect(Drew(T3, "81%") && !Drew(T3, "ALL IN"), "legacy: videoart::Tile draws a Rerun 2.0 video's recipe, not a frame");
	// An Update 3 video draws its shown recipe (the Duel's winner), not a frame.
	const Fixture Fx0 = Fixtures()[0];
	video::Video V3;
	V3.X.On = true;
	V3.X.Spec[0] = Fixtures()[1].S;
	V3.X.Spec[1] = Fx0.S;
	V3.X.DuelWin = 1;
	V3.Th = Fx0.F;
	DrawList A, B;
	Canvas Ca(A, M, 700.0f, 400.0f, 1.0f), Cb(B, M, 700.0f, 400.0f, 1.0f);
	ta::Budget Bu;
	Bu.Max = 100;
	video::Channel Ch;
	ta::VideoThumb(Ca, {0.0f, 0.0f, 300.0f, 169.0f}, V3, Ch, Me, Bu);
	ta::Draw(Cb, {0.0f, 0.0f, 300.0f, 169.0f}, Fx0.S, Me, Fx0.F, Bu);
	Expect(A.ToJson() == B.ToJson(), "legacy: an Update 3 video draws its Duel winner's recipe");
	// The frozen Look: a haircut after upload never redraws the old thumbnail.
	video::FrozenLook Frozen;
	Frozen.Ref = 3;
	for (int K = 0; K < ss::hero::SlotCount; ++K)
	{
		Frozen.Look[static_cast<size_t>(K)] = static_cast<int16_t>(Me.Appearance.Get(static_cast<ss::hero::Slot>(K)));
	}
	Ch.X.Looks.push_back(Frozen);
	video::ThumbSpec Ref = Fx0.S;
	Ref.LookRef = 3;
	ss::hero::Character Now = Me;
	Now.Appearance.Hair = 7;
	Expect(ta::LookFor(Ref, Ch, Now).Appearance == Me.Appearance && ta::LookFor(Fx0.S, Ch, Now).Appearance == Now.Appearance, "legacy: LookFor reads the frozen Look its recipe names");
}

/** The compositor's own words: no digit, no real name, nothing that says AI, a bot or autoplay (U1). */
void Words()
{
	int Bad = 0;
	for (const std::string& W : ta::Words())
	{
		bool Digit = false;
		for (const char Ch : W)
		{
			Digit = Digit || (Ch >= '0' && Ch <= '9');
		}
		const std::string Low = trademarks::Lower(W);
		const bool U1 = trademarks::HasWord(Low, "ai") || trademarks::HasWord(Low, "bot") || Low.find("autoplay") != std::string::npos || Low.find("auto-play") != std::string::npos;
		if (Digit || U1 || trademarks::Find(W, trademarks::kSourceScan) || trademarks::Find(W, trademarks::kCopyScan))
		{
			std::printf("  thumbs word     \"%s\" fails the screen\n", W.c_str());
			++Bad;
		}
	}
	for (int A = 0; A < ta::ArchCount; ++A)
	{
		const std::string N = ta::ArchName(A);
		Bad += trademarks::Find(N, trademarks::kSourceScan) || trademarks::Find(N, trademarks::kCopyScan) ? 1 : 0;
	}
	Expect(Bad == 0 && !ta::Words().empty(), "copy: the compositor's words and the archetypes' names pass the screens (no digit, no real name, U1)");
}

// ------------------------------------------------------------------ contact sheets (rasterized for the eye)

/** The 35 at one size, in Cols columns, each with its archetype's name under it (the sheet's own label, not the game's). */
void Sheet(const std::string& Name, const std::vector<Fixture>& All, size_t From, size_t Count, float W, float H, int Cols, bool Lod, bool Mirror)
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	const float Gap = W >= 300.0f ? 18.0f : 10.0f;
	const float Lab = W >= 200.0f ? 22.0f : 14.0f;
	const int Rows = static_cast<int>((Count + static_cast<size_t>(Cols) - 1) / static_cast<size_t>(Cols));
	const float Sw = Gap + static_cast<float>(Cols) * (W + Gap), Sh = Gap + static_cast<float>(Rows) * (H + Lab + Gap);
	DrawList L;
	Canvas C(L, M, Sw, Sh, 1.0f);
	C.FillRect({0.0f, 0.0f, Sw, Sh}, ss::ui::Hex(0x0b0c10));
	for (size_t I = 0; I < Count && From + I < All.size(); ++I)
	{
		const Fixture& X = All[From + I];
		const float Px = Gap + static_cast<float>(static_cast<int>(I) % Cols) * (W + Gap);
		const float Py = Gap + static_cast<float>(static_cast<int>(I) / Cols) * (H + Lab + Gap);
		video::ThumbSpec S = X.S;
		S.Mirror = Mirror ? 1 : 0;
		ta::Compose(C, {Px, Py, W, H}, S, Me, Lod, X.F);
		ss::ui::TextStyle T;
		T.Size = Lab * 0.62f;
		T.Weight = 700;
		T.Col = ss::ui::Hex(0xc8ccd6);
		T.VAlign = ss::ui::Baseline::Top;
		T.MaxWidth = W;
		C.Text(std::to_string(X.S.Arch + 1) + ". " + X.Name, Px, Py + H + 4.0f, T);
	}
	WriteList(Name, L);
}

/**
 * C3 fix (c3f-thumbs, L7): Studio's options at their real size (120 x 68, the LOD a page draws there), magnified Zoom times
 * for the eye, each with Studio's Duel letter where RiverLineVideoStudio.cpp draws it (since c3f-thumbs_to_c3f-ui_1: a gold
 * disc on an ink rim on the tile's corner, DuelRim round (DuelIn, DuelIn); it was a 10-unit circle 14 units in).
 */
void OptionSheet(const std::string& Name, const std::vector<Fixture>& All, float W, float H, float Zoom, bool Mirror)
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	const int Cols = 6;
	const float Gap = 8.0f, Lab = 12.0f;
	const int Rows = static_cast<int>((All.size() + static_cast<size_t>(Cols) - 1) / static_cast<size_t>(Cols));
	const float Sw = Gap + static_cast<float>(Cols) * (W + Gap), Sh = Gap + static_cast<float>(Rows) * (H + Lab + Gap);
	DrawList L;
	Canvas C(L, M, Sw * Zoom, Sh * Zoom, 1.0f);
	C.FillRect({0.0f, 0.0f, Sw * Zoom, Sh * Zoom}, ss::ui::Hex(0x0b0c10));
	C.Save();
	C.Scale(Zoom, Zoom);
	for (size_t I = 0; I < All.size(); ++I)
	{
		const float Px = Gap + static_cast<float>(static_cast<int>(I) % Cols) * (W + Gap);
		const float Py = Gap + static_cast<float>(static_cast<int>(I) / Cols) * (H + Lab + Gap);
		video::ThumbSpec S = All[I].S;
		S.Mirror = static_cast<int16_t>(Mirror ? (S.Mirror | 1) : (S.Mirror & ~1));
		ta::Compose(C, {Px, Py, W, H}, S, Me, ta::LodFor(W), All[I].F);
		if (W <= 130.0f)
		{
			C.FillCircle(Px + DuelIn, Py + DuelIn, DuelRim, ss::ui::Hex(0x0b0c10));
			C.FillCircle(Px + DuelIn, Py + DuelIn, DuelGold, ss::ui::Hex(0xf2a33a));
		}
		ss::ui::TextStyle T;
		T.Size = 7.0f;
		T.Weight = 700;
		T.Col = ss::ui::Hex(0xc8ccd6);
		T.VAlign = ss::ui::Baseline::Top;
		T.MaxWidth = W;
		C.Text(All[I].Name + " L" + std::to_string(All[I].S.Layout), Px, Py + H + 2.0f, T);
	}
	C.Restore();
	WriteList(Name, L);
}

void Sheets()
{
	if (OutDir.empty())
	{
		return;
	}
	const std::vector<Fixture> All = Fixtures();
	OptionSheet("thumbs_sheet_option", All, 120.0f, 68.0f, 4.0f, false);
	Sheet("thumbs_sheet_hero_1", All, 0, 12, 640.0f, 360.0f, 3, false, false);
	Sheet("thumbs_sheet_hero_2", All, 12, 12, 640.0f, 360.0f, 3, false, false);
	Sheet("thumbs_sheet_hero_3", All, 24, 11, 640.0f, 360.0f, 3, false, false);
	Sheet("thumbs_sheet_tile", All, 0, 35, 300.0f, 169.0f, 7, false, false);
	Sheet("thumbs_sheet_tile_mirror", All, 0, 35, 300.0f, 169.0f, 7, false, true);
	Sheet("thumbs_sheet_lod", All, 0, 35, 211.0f, 119.0f, 7, false, false);
	Sheet("thumbs_sheet_row", All, 0, 35, 96.0f, 54.0f, 7, true, false);
	Sheet("thumbs_sheet_home_lod", All, 0, 35, 375.0f, 211.0f, 7, true, false);
	// The 12 faces and the 4 poses, at a thumbnail's scale.
	TableMeasurer M;
	DrawList L;
	Canvas C(L, M, 1700.0f, 760.0f, 1.0f);
	C.FillRect({0.0f, 0.0f, 1700.0f, 760.0f}, ss::ui::Hex(0x2a3140));
	for (int E = 0; E < 12; ++E)
	{
		ss::ui::DrawPortrait(C, Player(), 80.0f + static_cast<float>(E % 6) * 140.0f, 110.0f + static_cast<float>(E / 6) * 250.0f, 0.5f, 0.0, ss::ui::PortraitLight(), ss::ui::ExpressionOf(E), ss::ui::PortraitPose::None,
			false);
	}
	for (int P = 1; P <= 4; ++P)
	{
		ss::ui::PortraitExpression E = ss::ui::ExpressionOf(P == 4 ? 3 : P == 3 ? 1 : P == 2 ? 11 : 6);
		E.LookX = 0.01f;
		ss::ui::DrawPortrait(C, ss::hero::CharacterFor("pose_" + std::to_string(P)), 960.0f + static_cast<float>((P - 1) % 2) * 360.0f, 150.0f + static_cast<float>((P - 1) / 2) * 330.0f, 0.55f, 0.0,
			ss::ui::PortraitLight(), E, static_cast<ss::ui::PortraitPose>(P), false);
	}
	WriteList("thumbs_faces", L);
}

// ------------------------------------------------------------------ spike S-T (SPEC §8.2, §10): the measures that size the pool

struct Page
{
	const char* Name;
	float W, H;
	int Count;
	bool Preview; // a 640 x 360 hero above the tiles (Studio's selected option, the launch card)
	int Lod;      // -1 by width (LodFor), 1 forced (S-T's fail-over)
	bool Pool;    // counted in the pool's need
};

/** S-T: each archetype's vertices and runs at each size, cold and warm; pages of thumbnails; the pool that holds them. */
void SpikeST(bool Full)
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	const std::vector<Fixture> All = Fixtures();
	const float Widths[5][2] = {{640.0f, 360.0f}, {375.0f, 211.0f}, {300.0f, 169.0f}, {211.0f, 119.0f}, {96.0f, 54.0f}};
	double SumCold[5] = {}, MaxCold[5] = {};
	size_t SumVerts[5] = {}, MaxVerts[5] = {}, SumIdx[5] = {};
	int SumRuns[5] = {}, MaxRuns[5] = {};
	double SumWarm[5] = {};
	for (size_t I = 0; I < All.size(); ++I)
	{
		for (int Z = 0; Z < 5; ++Z)
		{
			const Rect R{0.0f, 0.0f, Widths[Z][0], Widths[Z][1]};
			const bool Lod = ta::LodFor(R.W);
			double Best = 1e9;
			DrawList Keep;
			for (int Rep = 0; Rep < (Full ? 3 : 1); ++Rep)
			{
				DrawList L;
				Canvas C(L, M, 700.0f, 400.0f, 1.0f);
				const auto T0 = std::chrono::steady_clock::now();
				ta::Compose(C, R, All[I].S, Me, Lod, All[I].F);
				Best = std::min(Best, Ms(T0, std::chrono::steady_clock::now()));
				Keep = L;
			}
			// Warm: replayed from Rerun's pool.
			DrawList W;
			Canvas Cw(W, M, 700.0f, 400.0f, 1.0f);
			ta::Budget B;
			B.Max = 10;
			ta::Draw(Cw, R, All[I].S, Me, All[I].F, B);
			double WarmBest = 1e9;
			for (int Rep = 0; Rep < 5; ++Rep)
			{
				DrawList W2;
				Canvas Cw2(W2, M, 700.0f, 400.0f, 1.0f);
				const auto T0 = std::chrono::steady_clock::now();
				ta::Draw(Cw2, R, All[I].S, Me, All[I].F, B);
				WarmBest = std::min(WarmBest, Ms(T0, std::chrono::steady_clock::now()));
			}
			SumCold[Z] += Best;
			MaxCold[Z] = std::max(MaxCold[Z], Best);
			SumWarm[Z] += WarmBest;
			SumVerts[Z] += Keep.Vertices.size();
			SumIdx[Z] += Keep.Indices.size();
			MaxVerts[Z] = std::max(MaxVerts[Z], Keep.Vertices.size());
			SumRuns[Z] += Runs(Keep);
			MaxRuns[Z] = std::max(MaxRuns[Z], Runs(Keep));
			if (Full)
			{
				std::printf("  S-T %-16s %3.0f x %3.0f%s: %6zu vertices %6zu indices %3d runs, %.3f ms cold, %.4f ms warm\n", All[I].Name.c_str(), R.W, R.H, Lod ? " LOD" : "    ", Keep.Vertices.size(),
					Keep.Indices.size(), Runs(Keep), Best, WarmBest);
			}
		}
	}
	const double N = static_cast<double>(All.size());
	for (int Z = 0; Z < 5; ++Z)
	{
		std::printf("  S-T size %3.0f x %3.0f%s: mean %6.0f vertices (max %6zu), %4.1f runs (max %2d), %.3f ms cold (max %.3f), %.4f ms warm; %.1f B a vertex with its indices\n", Widths[Z][0],
			Widths[Z][1], ta::LodFor(Widths[Z][0]) ? " LOD" : "    ", static_cast<double>(SumVerts[Z]) / N, MaxVerts[Z], static_cast<double>(SumRuns[Z]) / N, MaxRuns[Z], SumCold[Z] / N, MaxCold[Z],
			SumWarm[Z] / N, (static_cast<double>(SumVerts[Z]) * sizeof(ss::ui::Vertex) + static_cast<double>(SumIdx[Z]) * 4.0) / static_cast<double>(SumVerts[Z]));
	}
	// S-T's tiers (§10): the hero within 14k with its portrait (Archetypes checks each), a tile's mean within 4k (the coarse
	// portrait), the LOD row's within 1.3k (the flat face) and 8 runs (a list's rows are words: 20 at most).
	Expect(static_cast<double>(SumVerts[1]) / N <= 4000.0 && static_cast<double>(SumVerts[2]) / N <= 4000.0, "S-T: a tile (300-375 wide) averages within 4k vertices");
	Expect(static_cast<double>(SumVerts[4]) / N <= 1300.0 && static_cast<double>(SumRuns[4]) / N <= 8.0 && MaxRuns[4] <= 20, "S-T: an LOD row averages within 1.3k vertices and 8 runs");
	// Faces off: the same recipes with nobody in them (the silhouette fallback's ceiling).
	{
		size_t Off = 0, On = 0;
		for (const Fixture& X : All)
		{
			video::ThumbSpec S = X.S;
			S.Subj = static_cast<int16_t>(ta::Subject::Nobody);
			On += DrawOne(X.S, X.F, Me, {0.0f, 0.0f, 375.0f, 211.0f}, false).Vertices.size();
			Off += DrawOne(S, X.F, Me, {0.0f, 0.0f, 375.0f, 211.0f}, false).Vertices.size();
		}
		std::printf("  S-T faces at 375 x 211: %.0f vertices a tile with faces, %.0f without (the face's share %.0f%%)\n", static_cast<double>(On) / N, static_cast<double>(Off) / N,
			100.0 * (1.0 - static_cast<double>(Off) / static_cast<double>(On)));
	}
	// Pages: what a full page of thumbnails holds, painted 2 a frame from cold, then its warm frame.
	const Page Pages[7] = {{"Studio (6 options + the 640 preview)", 300.0f, 169.0f, 6, true, -1, true}, {"Home (12 tiles)", 375.0f, 211.0f, 12, false, -1, true},
		{"Content (60 rows)", 96.0f, 54.0f, 60, false, -1, true}, {"Channel (12 tiles)", 300.0f, 169.0f, 12, false, -1, true},
		{"Research (8 world tiles + a 640 hero)", 300.0f, 169.0f, 8, true, -1, true}, {"Content (12 rows on screen)", 96.0f, 54.0f, 12, false, -1, false},
		{"Home, the fail-over (12 tiles at the LOD)", 375.0f, 211.0f, 12, false, 1, false}};
	size_t PoolNeed = 0;
	uint32_t PageNo = 0;
	for (const Page& P : Pages)
	{
		++PageNo;
		Canvas::SetPoolBudget(Canvas::CachePool::Rerun, 4000000);
		size_t Verts = 0;
		int Frames = 0;
		double WorstColdFrame = 0.0;
		std::vector<Fixture> Mine;
		for (int I = 0; I < P.Count; ++I)
		{
			Fixture X = All[static_cast<size_t>(I) % All.size()];
			X.S.Variant = 0x57000000u + static_cast<uint32_t>(I) + PageNo * 100000u;
			Mine.push_back(X);
		}
		for (;;)
		{
			DrawList L;
			Canvas C(L, M, 2400.0f, 1500.0f, 1.0f);
			ta::Budget B;
			const auto T0 = std::chrono::steady_clock::now();
			if (P.Preview)
			{
				ta::Draw(C, {20.0f, 20.0f, 640.0f, 360.0f}, Mine[0].S, Me, Mine[0].F, B);
			}
			for (int I = 0; I < P.Count; ++I)
			{
				const Rect R{20.0f + static_cast<float>(I % 6) * (P.W + 12.0f), 400.0f + static_cast<float>(I / 6) * (P.H + 12.0f), P.W, P.H};
				const Fixture& X = Mine[static_cast<size_t>(I)];
				if (P.Lod > 0)
				{
					ta::Draw(C, R, X.S, Me, X.F, B, true);
				}
				else
				{
					ta::Draw(C, R, X.S, Me, X.F, B);
				}
			}
			WorstColdFrame = std::max(WorstColdFrame, Ms(T0, std::chrono::steady_clock::now()));
			++Frames;
			if (B.Waiting == 0)
			{
				Verts = L.Vertices.size();
				break;
			}
		}
		// The warm frame, best of 15 (review C3: best of 5 read Home at 1.506 ms under ctest -j 3 beside three tracks' builds).
		double Warm = 1e9;
		int WarmRuns = 0;
		for (int Rep = 0; Rep < 15; ++Rep)
		{
			DrawList L;
			Canvas C(L, M, 2400.0f, 1500.0f, 1.0f);
			ta::Budget B;
			const auto T0 = std::chrono::steady_clock::now();
			if (P.Preview)
			{
				ta::Draw(C, {20.0f, 20.0f, 640.0f, 360.0f}, Mine[0].S, Me, Mine[0].F, B);
			}
			for (int I = 0; I < P.Count; ++I)
			{
				const Rect R{20.0f + static_cast<float>(I % 6) * (P.W + 12.0f), 400.0f + static_cast<float>(I / 6) * (P.H + 12.0f), P.W, P.H};
				const Fixture& X = Mine[static_cast<size_t>(I)];
				if (P.Lod > 0)
				{
					ta::Draw(C, R, X.S, Me, X.F, B, true);
				}
				else
				{
					ta::Draw(C, R, X.S, Me, X.F, B);
				}
			}
			Warm = std::min(Warm, Ms(T0, std::chrono::steady_clock::now()));
			WarmRuns = Runs(L);
		}
		PoolNeed += P.Pool ? Verts : 0;
		std::printf("  S-T page %-42s %6zu vertices, %3d runs, %2d frames to paint (worst %.2f ms), %.3f ms warm\n", P.Name, Verts, WarmRuns, Frames, WorstColdFrame, Warm);
		Expect(Frames == (P.Count + (P.Preview ? 1 : 0) + 1) / 2, "S-T: a page paints 2 thumbnails a frame until all are cached");
		// The 60-row table is the stress case (printed: about 1.1 ms quiet); a page as drawn holds its thumbnails' replay
		// within the page's 1.5 ms (about a third of it, quiet).
		Expect(P.Count == 60 || Warm <= 1.5, "S-T: a page's thumbnails replay within 1.5 ms warm (best of 15)");
		Expect(WarmRuns <= 600, "S-T: a page's thumbnails hold at most 600 text runs");
	}
	Canvas::SetPoolBudget(Canvas::CachePool::Rerun, 0);
	std::printf("  S-T pool: every page together %zu vertices (%.1f MB at 41 B); Rerun's pool %zu vertices (%.1f MB)\n", PoolNeed, static_cast<double>(PoolNeed) * 41.0 / 1048576.0, ta::PoolVertices,
		static_cast<double>(ta::PoolVertices) * 41.0 / 1048576.0);
	Expect(PoolNeed * 3 / 2 <= ta::PoolVertices, "S-T: Rerun's pool holds every page's thumbnails at once, with half again to spare");
}

// ------------------------------------------------------------------ C3's review (review-ui-thumbs): the generator's own deals

/** How much of a run's body (its capitals, 0.72 of its size above the baseline, down to the baseline) is inside R, 0..1. */
float Visible(const ss::ui::TextItem& T, Rect R)
{
	const float Top = T.BaselineY - 0.72f * T.SizePx, Bot = T.BaselineY;
	const float In = std::min(Bot, R.Y + R.H) - std::max(Top, R.Y);
	return std::clamp(In / std::max(0.01f, Bot - Top), 0.0f, 1.0f);
}

/** Whether a run can be read inside R: its body in, and across it within R (half a percent of R's width to spare). */
bool Readable(const ss::ui::TextItem& T, Rect R)
{
	TableMeasurer M;
	const float W = M.Width(T.Text, T.Face, T.SizePx), Slack = R.W * 0.005f;
	return Visible(T, R) >= 0.85f && T.X >= R.X - Slack && T.X + W <= R.X + R.W + Slack;
}

/** A caption line's words (the runs between spaces that carry a letter or a digit), the facts block left out. */
std::vector<std::string> WordsOf(const std::string& S)
{
	std::vector<std::string> Out(1);
	for (const char Ch : S.substr(0, S.find('\x1f')))
	{
		if (Ch == ' ')
		{
			Out.emplace_back();
		}
		else
		{
			Out.back().push_back(Ch);
		}
	}
	Out.erase(std::remove_if(Out.begin(), Out.end(), [](const std::string& W) { return std::none_of(W.begin(), W.end(), [](char Ch) { return std::isalnum(static_cast<unsigned char>(Ch)) != 0; }); }),
		Out.end());
	return Out;
}

/** Whether Word is drawn where it can be read (in a readable run inside R). */
bool Reads(const DrawList& L, Rect R, const std::string& Word)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(Word) != std::string::npos && Readable(Cmd.Text, R))
		{
			return true;
		}
	}
	return false;
}

/** Runs whose text is exactly S (Loud draws a line as its shadow, outline and fill: compare counts, not 1). */
int RunsOf(const DrawList& L, const std::string& S)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == S ? 1 : 0;
	}
	return N;
}

/** The first run that is exactly S (null when none). */
const ss::ui::TextItem* RunOf(const DrawList& L, const std::string& S)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == S)
		{
			return &Cmd.Text;
		}
	}
	return nullptr;
}

Fixture FixtureOf(ta::Arch A)
{
	for (const Fixture& X : Fixtures())
	{
		if (X.S.Arch == static_cast<int16_t>(A))
		{
			return X;
		}
	}
	return Fixtures()[0];
}

/** A contact sheet of the generator's deals for the eye (each labelled with its archetype, caption style, mirror and props). */
void GenSheet(const std::string& Name, const std::vector<video::ThumbSpec>& All, float W, float H, int Cols)
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	const float Gap = 14.0f, Lab = 18.0f;
	const int Rows = static_cast<int>((All.size() + static_cast<size_t>(Cols) - 1) / static_cast<size_t>(Cols));
	const float Sw = Gap + static_cast<float>(Cols) * (W + Gap), Sh = Gap + static_cast<float>(Rows) * (H + Lab + Gap);
	DrawList L;
	Canvas C(L, M, Sw, Sh, 1.0f);
	C.FillRect({0.0f, 0.0f, Sw, Sh}, ss::ui::Hex(0x0b0c10));
	for (size_t I = 0; I < All.size(); ++I)
	{
		const float Px = Gap + static_cast<float>(static_cast<int>(I) % Cols) * (W + Gap);
		const float Py = Gap + static_cast<float>(static_cast<int>(I) / Cols) * (H + Lab + Gap);
		ta::Compose(C, {Px, Py, W, H}, All[I], Me, ta::LodFor(W), video::Thumb());
		ss::ui::TextStyle T;
		T.Size = 11.0f;
		T.Weight = 700;
		T.Col = ss::ui::Hex(0xc8ccd6);
		T.VAlign = ss::ui::Baseline::Top;
		T.MaxWidth = W;
		C.Text(std::string(ta::ArchName(All[I].Arch)) + " cs" + std::to_string(All[I].CapStyle) + " m" + std::to_string(All[I].Mirror) + " pa" + std::to_string(All[I].PropA) + " pb" +
				   std::to_string(All[I].PropB),
			Px, Py + H + 3.0f, T);
	}
	WriteList(Name, L);
}

/**
 * The recipes core-videocontent's generator deals (video::ContactSheet: VT1's sheets for every kind group and the ideas, the
 * facts blocks as written), drawn as a page draws a world video (an empty Thumb) at the four sizes: every text run readable
 * inside its tile (no line under an edge), every caption word drawn, nothing under the corner mark. Then every archetype in
 * every caption style with one- and two-line captions, both mirrors and zooms: every word drawn inside the frame. The fixtures
 * above deal no props; the generator's deals do (review C3: a second whiteboard, crosshair or bus; line 2 lost under the
 * burst; a white stamp on a white plate).
 */
void Generated()
{
	TableMeasurer Tm;
	const ss::hero::Character Me = Player();
	if (!OutDir.empty())
	{
		for (int G = 0; G <= 6; ++G)
		{
			GenSheet("thumbs_gen_g" + std::to_string(G), video::ContactSheet(G, 24), 300.0f, 169.0f, 6);
		}
	}
	int Specs = 0, Escapes = 0, Lost = 0, Cornered = 0;
	size_t MaxDealt = 0;
	for (int G = 0; G <= 6; ++G)
	{
		for (const video::ThumbSpec& S : video::ContactSheet(G, 400))
		{
			++Specs;
			for (const auto& Sz : Sizes)
			{
				const Rect R{7.0f, 5.0f, Sz[0], Sz[1]};
				const DrawList L = DrawOne(S, video::Thumb(), Me, R, ta::LodFor(Sz[0]));
				// The corner mark's box (578 .. 626 x 14 .. 48 of the 640 x 360 design, the top left mirrored).
				const float K = Sz[0] / 640.0f;
				const bool Mark = S.Corner == static_cast<int16_t>(ta::Corner::Initials) || S.Corner == static_cast<int16_t>(ta::Corner::Logo);
				const Rect Cn{R.X + ((S.Mirror & 1) != 0 ? 8.0f : 572.0f) * K, R.Y + 10.0f * K, 60.0f * K, 46.0f * K};
				bool Esc = false, Under = false;
				std::vector<std::string> Words = WordsOf(S.Caption);
				for (const std::string& W : WordsOf(S.Caption2))
				{
					Words.push_back(W);
				}
				for (const ss::ui::DrawCmd& Cmd : L.Cmds)
				{
					if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
					{
						continue;
					}
					const ss::ui::TextItem& T = Cmd.Text;
					Esc = Esc || !Readable(T, R);
					// A caption's words under the mark (a sticker's own glyph, a money bag's "$", may tuck under it).
					const float W = Tm.Width(T.Text, T.Face, T.SizePx);
					const bool Caption = std::any_of(Words.begin(), Words.end(), [&T](const std::string& Wd) { return T.Text.find(Wd) != std::string::npos; });
					Under = Under || (Mark && Caption && T.X < Cn.X + Cn.W && T.X + W > Cn.X && T.BaselineY - 0.72f * T.SizePx < Cn.Y + Cn.H && T.BaselineY > Cn.Y);
				}
				if (Esc || Under)
				{
					std::printf("  thumbs dealt    %-16s %3.0f wide, style %d, mirror %d: %s (\"%s\" / \"%s\")\n", ta::ArchName(S.Arch), Sz[0], S.CapStyle, S.Mirror,
						Esc ? "a run outside its tile" : "words under the corner mark", S.Caption.c_str(), S.Caption2.substr(0, S.Caption2.find('\x1f')).c_str());
				}
				Escapes += Esc ? 1 : 0;
				Cornered += Under ? 1 : 0;
				MaxDealt = Sz[0] == 640.0f ? std::max(MaxDealt, L.Vertices.size()) : MaxDealt;
				if (Sz[0] == 640.0f && L.Vertices.size() > 14000)
				{
					std::printf("  thumbs dealt    %-16s 640 wide: %zu vertices (> 14k; subject %d, pose %d, props %d %d, sticker %d, bg %d)\n", ta::ArchName(S.Arch), L.Vertices.size(), S.Subj, S.Pose, S.PropA,
						S.PropB, S.Sticker, S.Bg);
				}
				if (Sz[0] >= 300.0f)
				{
					for (const std::string* Line : {&S.Caption, &S.Caption2})
					{
						for (const std::string& W : WordsOf(*Line))
						{
							if (!Reads(L, R, W))
							{
								++Lost;
								std::printf("  thumbs dealt    %-16s %3.0f wide, style %d, mirror %d: \"%s\" not drawn\n", ta::ArchName(S.Arch), Sz[0], S.CapStyle, S.Mirror, W.c_str());
							}
						}
					}
				}
			}
		}
	}
	// Every archetype in every caption style (None aside), one- and two-line captions, each mirror and zoom.
	int Swept = 0, SweptLost = 0;
	const std::pair<const char*, const char*> Caps[3] = {{"4TH OF", "1,530"}, {"$1,190", "I WON"}, {"SET vs SET", ""}};
	for (int A = 0; A < ta::ArchCount; ++A)
	{
		for (int Cs = 0; Cs < static_cast<int>(ta::CapStyle::None); ++Cs)
		{
			for (int Mi = 0; Mi < 4; ++Mi)
			{
				for (const auto& Cp : Caps)
				{
					video::ThumbSpec S = Fixtures()[0].S;
					S.Arch = static_cast<int16_t>(A);
					S.CapStyle = static_cast<int16_t>(Cs);
					S.Mirror = static_cast<int16_t>(Mi);
					S.Caption = Cp.first;
					S.Caption2 = Cp.second;
					const Rect R{0.0f, 0.0f, 640.0f, 360.0f};
					const DrawList L = DrawOne(S, video::Thumb(), Me, R, false);
					++Swept;
					bool Ok = true;
					for (const std::string* Line : {&S.Caption, &S.Caption2})
					{
						for (const std::string& W : WordsOf(*Line))
						{
							Ok = Ok && Reads(L, R, W);
						}
					}
					SweptLost += Ok ? 0 : 1;
					if (!Ok)
					{
						std::printf("  thumbs sweep    %-16s style %d, mirror %d: \"%s\" / \"%s\" not all drawn inside the frame\n", ta::ArchName(A), Cs, Mi, Cp.first, Cp.second);
					}
				}
			}
		}
	}
	std::printf("  thumbs dealt    %d recipes from video::ContactSheet x 4 sizes: %d with a run outside its tile, %d words not drawn, %d with words under the corner mark; "
				"sweep %d of %d lose a word; the heaviest at 640 x 360 %zu vertices\n",
		Specs, Escapes, Lost, Cornered, SweptLost, Swept, MaxDealt);
	Expect(Specs >= 100, "dealt: the generator's contact sheets deal recipes for every group");
	Expect(MaxDealt > 0 && MaxDealt <= 14000, "dealt: every dealt recipe at 640 x 360 within 14k vertices with its portrait and props (§10)");
	Expect(Escapes == 0, "dealt: every text run of a dealt recipe is readable inside its tile (no line under an edge)");
	Expect(Lost == 0, "dealt: every word of a dealt recipe's caption is drawn");
	Expect(Cornered == 0, "dealt: no word under the corner mark (it takes the top corner the mirrored layout leaves free)");
	Expect(SweptLost == 0, "dealt: every archetype draws every word of a caption in every style, mirror and zoom, inside the frame");
}

/** Each review fix, one check (each failed on ui-thumbs' first cut). */
void ReviewFixes()
{
	const ss::hero::Character Me = Player();
	const Rect R{0.0f, 0.0f, 640.0f, 360.0f};
	using A = ta::Arch;
	using P = ta::Prop;
	// 1. The archetype's defining prop (the generator deals it as PropA) is the composition: never drawn a second time.
	const std::pair<A, P> Own[] = {{A::GutPunch, P::HoleCards}, {A::CircledRiver, P::Board}, {A::TheRead, P::CardBacks}, {A::NervesOfSteel, P::HoleCards}, {A::Caught, P::HoleCards},
		{A::TheFold, P::OptionCards}, {A::ThePile, P::FeltTop}, {A::ThePile, P::ChipPile}, {A::TheBubble, P::ChipPile}, {A::VsSplit, P::ShownCards}, {A::Headhunter, P::Crosshair}, {A::Envelope, P::MysteryEnvelope},
		{A::TheGraph, P::GraphLine}, {A::DayCounter, P::ProgressBar}, {A::NightShift, P::LaptopLobby}, {A::Venue, P::VenueMark}, {A::RoadTrip, P::Bus}, {A::Classroom, P::Whiteboard},
		{A::Quiz, P::OptionCards}, {A::TierList, P::TierRows}, {A::TopFive, P::NumberedCells}, {A::DeskSetup, P::Gear}, {A::Milestone, P::Confetti}, {A::MonsterHand, P::CardFan},
		{A::SiteCard, P::FuseReel}, {A::Mistake, P::HoleCards}};
	int Twice = 0;
	for (const auto& O : Own)
	{
		Fixture X = FixtureOf(O.first);
		X.S.PropA = -1;
		X.S.PropB = -1;
		const std::string Bare = DrawOne(X.S, X.F, Me, R, false).ToJson();
		X.S.PropA = static_cast<int16_t>(O.second);
		const bool AsA = DrawOne(X.S, X.F, Me, R, false).ToJson() != Bare;
		X.S.PropA = -1;
		X.S.PropB = static_cast<int16_t>(O.second);
		const bool AsB = DrawOne(X.S, X.F, Me, R, false).ToJson() != Bare;
		if (AsA || AsB)
		{
			std::printf("  thumbs review   %s draws its own %d a second time\n", ta::ArchName(static_cast<int>(O.first)), static_cast<int>(O.second));
		}
		Twice += AsA || AsB ? 1 : 0;
	}
	Fixture Big = FixtureOf(A::BigNumber);
	const std::string WithCash = DrawOne(Big.S, Big.F, Me, R, false).ToJson();
	Big.S.PropA = -1;
	Expect(Twice == 0 && WithCash != DrawOne(Big.S, Big.F, Me, R, false).ToJson(), "review: an archetype's own prop is never drawn twice; an extra prop still draws");
	// 2. A stamp on a white plate reads: its ink holds 3:1 against white in every palette.
	auto Lum = [](const ss::ui::Color& C) {
		auto Lin = [](float V) { return V <= 0.03928f ? V / 12.92f : std::pow((V + 0.055f) / 1.055f, 2.4f); };
		return 0.2126f * Lin(C.R) + 0.7152f * Lin(C.G) + 0.0722f * Lin(C.B);
	};
	int Pale = 0;
	for (int Pl = 0; Pl < static_cast<int>(ta::Pal::Count); ++Pl)
	{
		Fixture X = FixtureOf(A::TheGhost);
		X.S.Pal = static_cast<int16_t>(Pl);
		X.S.Variant = 0x60u;
		const DrawList L = DrawOne(X.S, X.F, Me, R, false);
		const ss::ui::TextItem* T = RunOf(L, "HIM AGAIN 44 BB");
		Pale += T == nullptr || 1.05f / (Lum(T->Col) + 0.05f) < 3.0f ? 1 : 0;
	}
	Expect(Pale == 0, "review: a stamp on a white plate holds 3:1 against it in every palette");
	// 3. An annotation aimed into an upright group marks the same thing on either side of the mirror (Gut Punch's red X on their
	//    cards; Before -> After's on the After panel): the group moves with the mirror, its order doesn't.
	auto XAt = [&](A Arch, int16_t Mirror) {
		Fixture X = FixtureOf(Arch);
		X.S.Mirror = Mirror;
		X.S.Corner = static_cast<int16_t>(ta::Corner::None);
		X.S.Sticker = -1;
		const size_t Before = DrawOne(X.S, X.F, Me, R, false).Vertices.size();
		X.S.Sticker = static_cast<int16_t>(ta::Sticker::RedX);
		const DrawList L = DrawOne(X.S, X.F, Me, R, false);
		double Sum = 0.0;
		for (size_t I = Before; I < L.Vertices.size(); ++I)
		{
			Sum += L.Vertices[I].X;
		}
		return L.Vertices.size() > Before ? static_cast<float>(Sum / static_cast<double>(L.Vertices.size() - Before)) : -1.0f;
	};
	Expect(std::fabs(XAt(A::GutPunch, 0) - 568.0f) < 6.0f && std::fabs(XAt(A::GutPunch, 1) - (640.0f - 372.0f - 622.0f + 568.0f)) < 6.0f &&
			   std::fabs(XAt(A::BeforeAfter, 0) - 488.0f) < 6.0f && std::fabs(XAt(A::BeforeAfter, 1) - (640.0f - 30.0f - 618.0f + 488.0f)) < 6.0f,
		"review: a red X aimed into an upright group marks the same cards (the same panel) mirrored");
	// 4. Mirrored, the reading order holds: a lower third's two lines, the board's flop-turn-river where it was, before then after.
	Fixture Lt = FixtureOf(A::CircledRiver);
	Lt.S.CapStyle = static_cast<int16_t>(ta::CapStyle::LowerThird);
	Lt.S.Caption = "WHY THE";
	Lt.S.Caption2 = "RIVER" + Lt.S.Caption2.substr(Lt.S.Caption2.find('\x1f'));
	bool Order = true;
	TableMeasurer Lm;
	for (int16_t Mi = 0; Mi < 2; ++Mi)
	{
		Lt.S.Mirror = Mi;
		const DrawList L = DrawOne(Lt.S, Lt.F, Me, R, false);
		const ss::ui::TextItem* A1 = RunOf(L, "WHY THE");
		const ss::ui::TextItem* A2 = RunOf(L, "RIVER");
		// Line 1 ends before line 2 starts.
		Order = Order && A1 != nullptr && A2 != nullptr && A1->X + Lm.Width(A1->Text, A1->Face, A1->SizePx) < A2->X;
	}
	auto Ranks = [&](int16_t Mirror) {
		Fixture X = FixtureOf(A::CircledRiver);
		X.S.Mirror = Mirror;
		X.S.Subj = static_cast<int16_t>(ta::Subject::Nobody);
		X.S.CapStyle = static_cast<int16_t>(ta::CapStyle::None);
		X.S.Corner = static_cast<int16_t>(ta::Corner::None);
		std::vector<std::pair<float, std::string>> Out;
		for (const ss::ui::DrawCmd& Cmd : DrawOne(X.S, X.F, Me, R, false).Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.SizePx > 14.0f)
			{
				Out.push_back({std::round(Cmd.Text.X), Cmd.Text.Text});
			}
		}
		std::sort(Out.begin(), Out.end());
		return Out;
	};
	const auto Unmirrored = Ranks(0);
	Order = Order && Unmirrored.size() >= 5 && Unmirrored == Ranks(1);
	Fixture Ba = FixtureOf(A::BeforeAfter);
	Ba.S.Mirror = 1;
	const DrawList BaL = DrawOne(Ba.S, Ba.F, Me, R, false);
	Order = Order && RunOf(BaL, "$11") != nullptr && RunOf(BaL, "$1,190") != nullptr && RunOf(BaL, "$11")->X < RunOf(BaL, "$1,190")->X;
	Expect(Order, "review: mirrored, a lower third reads line 1 then 2, the board stays flop-turn-river in place, before reads before after");
	// 5. One fact, told once: the event's crest beside a trophy (or the news, or the marquee) and Before -> After's division
	//    shield carry no second copy of the name the composition prints.
	Fixture Tr = FixtureOf(A::TrophyShot); // PropB the event's crest
	const int Crested = RunsOf(DrawOne(Tr.S, Tr.F, Me, R, false), "EMBERCREST SUNDAY");
	Tr.S.PropB = -1;
	const int Plain = RunsOf(DrawOne(Tr.S, Tr.F, Me, R, false), "EMBERCREST SUNDAY");
	Fixture Bs = FixtureOf(A::BeforeAfter);
	Bs.S.PropA = static_cast<int16_t>(P::Shield);
	const int Shielded = RunsOf(DrawOne(Bs.S, Bs.F, Me, R, false), "$1,190");
	Bs.S.PropA = -1;
	Expect(Plain > 0 && Crested == Plain && Shielded == RunsOf(DrawOne(Bs.S, Bs.F, Me, R, false), "$1,190"), "review: a crest or a shield never repeats the name the composition prints");
	// 6. The multiple, told once: a caption that is the multiple gives way to its second line; any other caption keeps both.
	Fixture Sc = FixtureOf(A::SiteCard);
	Sc.S.Caption = "x25";
	Sc.S.Caption2 = Facts({{"m", "25"}, {"s", "1"}});
	const DrawList ScL = DrawOne(Sc.S, Sc.F, Me, R, false);
	Fixture Ev = FixtureOf(A::Envelope);
	Ev.S.Caption = "25X";
	Ev.S.Caption2 = "BOUNTY" + Facts({{"m", "25"}});
	const DrawList EvL = DrawOne(Ev.S, Ev.F, Me, R, false);
	Sc.S.Caption = "SPECIAL";
	Sc.S.Caption2 = "GUEST" + Facts({{"m", "25"}, {"s", "1"}});
	const DrawList Sc2 = DrawOne(Sc.S, Sc.F, Me, R, false);
	Expect(!Drew(ScL, "x25") && Drew(ScL, "\xC3\x97" "25") && !Drew(EvL, "25X") && Drew(EvL, "BOUNTY") && Drew(Sc2, "SPECIAL") && Drew(Sc2, "GUEST"),
		"review: Envelope and Site Card show the multiple once and keep the caption's other words");
	// 7. Day Counter's one amount beside the p fact is today's: drawn where the fill has got to, not at the bar's start.
	Fixture Dc = FixtureOf(A::DayCounter);
	Dc.S.Caption2 = "$214" + Facts({{"p", "50"}});
	const DrawList DcL = DrawOne(Dc.S, Dc.F, Me, R, false);
	const ss::ui::TextItem* Now = RunOf(DcL, "$214");
	TableMeasurer Tm;
	const float Mid = Now == nullptr ? -1.0f : Now->X + Tm.Width(Now->Text, Now->Face, Now->SizePx) * 0.5f;
	Expect(Mid > 34.0f + 380.0f * 0.35f && Mid < 34.0f + 380.0f * 0.65f, "review: Day Counter's lone amount sits at the fill's end (p 50: mid-bar)");
	// 8. gh0stfold has no face: as the news's player he is his ghost, never a procedural portrait.
	Fixture Rb = FixtureOf(A::ReactionBox);
	Rb.S.Caption2 = Facts({{"o", ss::RivalName}});
	const size_t Ghost = DrawOne(Rb.S, Rb.F, Me, R, false).Vertices.size();
	Rb.S.Caption2 = Facts({{"o", "Tomas Ekdahl"}});
	Expect(Ghost + 2000 < DrawOne(Rb.S, Rb.F, Me, R, false).Vertices.size(), "review: gh0stfold on Reaction Box is his ghost, not a face");
	// ... and on his own world videos (a page draws a creator's video with hero::CharacterFor(Creator) as "you").
	const Fixture Gp = FixtureOf(A::GutPunch);
	const size_t HisOwn = DrawOne(Gp.S, Gp.F, ss::hero::CharacterFor(ss::RivalName), R, false).Vertices.size();
	Expect(HisOwn + 2000 < DrawOne(Gp.S, Gp.F, ss::hero::CharacterFor("VikingVolta"), R, false).Vertices.size(), "review: gh0stfold's own thumbnails draw his ghost, never a face");
	// 9. The corner mark sits in the top corner the layout keeps free: top right, top left mirrored.
	Fixture Cm = FixtureOf(A::TheBubble); // the initials
	const DrawList CmR = DrawOne(Cm.S, Cm.F, Me, R, false);
	Cm.S.Mirror = 1;
	const DrawList CmL = DrawOne(Cm.S, Cm.F, Me, R, false);
	const ss::ui::TextItem* Right = RunOf(CmR, "JC");
	const ss::ui::TextItem* Left = RunOf(CmL, "JC");
	Expect(Right != nullptr && Left != nullptr && Right->X > 560.0f && Left->X < 80.0f, "review: the corner mark moves with the mirror");
	// 10. The cache key moves with every slot of the Look and both names (a pair of glasses on is a new thumbnail).
	const Fixture F0 = Fixtures()[0];
	const uint64_t K0 = ta::KeyOf(F0.S, Me, F0.F, 300.0f, 169.0f, false);
	int Still = 0;
	for (int Sl = 0; Sl < ss::hero::SlotCount; ++Sl)
	{
		ss::hero::Character Other = Me;
		const ss::hero::Slot Slot = static_cast<ss::hero::Slot>(Sl);
		Other.Appearance.Set(Slot, Me.Appearance.Get(Slot) + 1, true);
		if (Other.Appearance.Get(Slot) == Me.Appearance.Get(Slot))
		{
			Other.Appearance.Set(Slot, Me.Appearance.Get(Slot) - 1, true);
		}
		Still += ta::KeyOf(F0.S, Other, F0.F, 300.0f, 169.0f, false) == K0 ? 1 : 0;
	}
	ss::hero::Character Renamed = Me;
	Renamed.FirstName = "Jessie";
	Still += ta::KeyOf(F0.S, Renamed, F0.F, 300.0f, 169.0f, false) == K0 ? 1 : 0;
	Renamed = Me;
	Renamed.LastName = "Coles";
	Still += ta::KeyOf(F0.S, Renamed, F0.F, 300.0f, 169.0f, false) == K0 ? 1 : 0;
	Expect(Still == 0, "review: every Look slot and both names move the cache key");
	// 11. The k fact marks its card: lifted, the largest on the board (no k: the river).
	auto Lifted = [&](const std::string& Facts2) {
		Fixture X = FixtureOf(A::CircledRiver);
		X.S.Caption2 = Facts2;
		X.S.Subj = static_cast<int16_t>(ta::Subject::Nobody);
		X.S.CapStyle = static_cast<int16_t>(ta::CapStyle::None);
		X.S.Corner = static_cast<int16_t>(ta::Corner::None);
		std::string Best;
		float Size = 0.0f;
		for (const ss::ui::DrawCmd& Cmd : DrawOne(X.S, X.F, Me, R, false).Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.SizePx > Size)
			{
				Size = Cmd.Text.SizePx;
				Best = Cmd.Text.Text;
			}
		}
		return Best;
	};
	Expect(Lifted(Facts({{"c", "AsAd/7c7h/Kd8s2c9h7h"}, {"k", "2"}})) == "2" && Lifted(Facts({{"c", "AsAd/7c7h/Kd8s2c9h7h"}})) == "7", "review: the k fact lifts and circles its own card");
}

// ------------------------------------------------------------------ C3 fixes (c3f-thumbs): L6, M1's art, L7

/** Studio's preview, Home's tile, the 300 tile, the 211 row, Studio's option and the 96 row, with the hero (C3 fix M1: every size). */
const float AllSizes[7][2] = {{640.0f, 360.0f}, {512.0f, 288.0f}, {375.0f, 211.0f}, {300.0f, 169.0f}, {211.0f, 119.0f}, {120.0f, 68.0f}, {96.0f, 54.0f}};

/** The 18 archetypes with a second layout and that layout (VtTable's second column, Game/VideoThumbs.cpp). */
std::vector<std::pair<ta::Arch, ta::Layout>> Seconds()
{
	using A = ta::Arch;
	using L = ta::Layout;
	return {{A::GutPunch, L::CardsLeftFaceRight}, {A::TheRead, L::FaceCentreFanBelow}, {A::Caught, L::FaceCentreFanBelow}, {A::TheFold, L::CardsLeftFaceRight},
		{A::TheBubble, L::BigNumberFace}, {A::Headhunter, L::FaceLeftWordsRight}, {A::Envelope, L::Collage}, {A::DayCounter, L::Graph}, {A::NightShift, L::FaceLeftWordsRight},
		{A::RoadTrip, L::FaceLeftWordsRight}, {A::Classroom, L::Collage}, {A::TopFive, L::Collage}, {A::ReactionBox, L::FaceLeftWordsRight}, {A::TheGhost, L::SplitVs},
		{A::Payday, L::BigNumberFace}, {A::MonsterHand, L::CardsLeftFaceRight}, {A::SiteCard, L::BigNumberFace}, {A::Mistake, L::FaceLeftWordsRight}};
}

/** The fixtures in their second layouts (the 18), each named "<archetype> 2". */
std::vector<Fixture> SecondFixtures()
{
	std::vector<Fixture> Out;
	for (const auto& Sd : Seconds())
	{
		Fixture X = FixtureOf(Sd.first);
		X.S.Layout = static_cast<int16_t>(Sd.second);
		X.Name += " 2";
		Out.push_back(X);
	}
	return Out;
}

/** Every composition: the 35 fixtures in their first layout, then the 18 in their second. */
std::vector<Fixture> EveryComposition()
{
	std::vector<Fixture> All = Fixtures();
	for (const Fixture& X : SecondFixtures())
	{
		All.push_back(X);
	}
	return All;
}

/** A cheap fingerprint of a draw list: its vertices (place and colour) and its text runs. */
uint64_t Print(const DrawList& L)
{
	uint64_t H = 0xcbf29ce484222325ull;
	auto Bytes = [&H](const void* P, size_t N) {
		const unsigned char* B = static_cast<const unsigned char*>(P);
		for (size_t I = 0; I < N; ++I)
		{
			H ^= B[I];
			H *= 0x100000001b3ull;
		}
	};
	Bytes(L.Vertices.data(), L.Vertices.size() * sizeof(L.Vertices[0]));
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Bytes(Cmd.Text.Text.data(), Cmd.Text.Text.size());
			Bytes(&Cmd.Text.X, sizeof(float));
			Bytes(&Cmd.Text.BaselineY, sizeof(float));
			Bytes(&Cmd.Text.Col, sizeof(Cmd.Text.Col));
		}
	}
	return H;
}

/**
 * How differently two thumbnails of one tile are laid out: the total variation distance between where their vertices fall on
 * a 16 x 9 grid of the tile (0 the same arrangement, 1 nothing where the other has anything).
 */
double Moved(const DrawList& A, const DrawList& B, Rect R)
{
	auto Hist = [&R](const DrawList& L) {
		std::vector<double> H(16 * 9, 0.0);
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			const int Cx = std::clamp(static_cast<int>((V.X - R.X) / R.W * 16.0f), 0, 15);
			const int Cy = std::clamp(static_cast<int>((V.Y - R.Y) / R.H * 9.0f), 0, 8);
			H[static_cast<size_t>(Cy * 16 + Cx)] += 1.0;
		}
		const double N = std::max<double>(1.0, static_cast<double>(L.Vertices.size()));
		for (double& V : H)
		{
			V /= N;
		}
		return H;
	};
	const std::vector<double> Ha = Hist(A), Hb = Hist(B);
	double D = 0.0;
	for (size_t I = 0; I < Ha.size(); ++I)
	{
		D += std::fabs(Ha[I] - Hb[I]);
	}
	return D * 0.5;
}

/**
 * L6: each of the 18 second layouts draws a composition of its own (it drew the first's), passing the first's checks at all
 * seven sizes (inside its tile, every caption in, deterministic, the LOD lighter, the mirror moving it, <= 14k at 640). And
 * the generator's own deals use exactly these layouts: an archetype dealt in two layouts deals its first and this second.
 */
void Layouts()
{
	const ss::hero::Character Me = Player();
	const std::vector<Fixture> Two = SecondFixtures();
	int Inside = 0, Fits = 0, Same = 0, Drawn = 0, LodFewer = 0, Mirrored = 0, UnderBudget = 0, Apart = 0, Cells = 0;
	double Least = 1.0;
	std::string LeastName;
	size_t MaxVerts = 0;
	for (const Fixture& X : Two)
	{
		Fixture One = X;
		One.S.Layout = static_cast<int16_t>(FirstOf(static_cast<ta::Arch>(X.S.Arch)));
		const Rect Hero{0.0f, 0.0f, 640.0f, 360.0f};
		const double D = Moved(DrawOne(One.S, One.F, Me, Hero, false), DrawOne(X.S, X.F, Me, Hero, false), Hero);
		Apart += D >= 0.2 ? 1 : 0;
		if (D < Least)
		{
			Least = D;
			LeastName = X.Name;
		}
		if (D < 0.2)
		{
			std::printf("  thumbs layout   %-18s draws much as its first (%.2f moved)\n", X.Name.c_str(), D);
		}
		for (const auto& Sz : AllSizes)
		{
			++Cells;
			const Rect R{7.0f, 5.0f, Sz[0], Sz[1]};
			const bool Lod = ta::LodFor(Sz[0]);
			const DrawList L = DrawOne(X.S, X.F, Me, R, Lod);
			Drawn += L.Vertices.size() > 200 ? 1 : 0;
			Inside += InsideBox(L, R) ? 1 : 0;
			const float Over = TextOverflow(L, R);
			Fits += Over <= Sz[0] * 0.03f ? 1 : 0;
			if (Over > Sz[0] * 0.03f)
			{
				std::printf("  thumbs overflow %-18s at %3.0f x %3.0f: %.1f units\n", X.Name.c_str(), Sz[0], Sz[1], Over);
			}
			Same += Print(DrawOne(X.S, X.F, Me, R, Lod)) == Print(L) ? 1 : 0;
			if (Sz[0] == 640.0f)
			{
				MaxVerts = std::max(MaxVerts, L.Vertices.size());
				UnderBudget += L.Vertices.size() <= 14000 ? 1 : 0;
				if (L.Vertices.size() > 14000)
				{
					std::printf("  thumbs budget   %-18s %zu vertices at 640 x 360 (> 14k)\n", X.Name.c_str(), L.Vertices.size());
				}
				LodFewer += DrawOne(X.S, X.F, Me, R, true).Vertices.size() < L.Vertices.size() ? 1 : 0;
				video::ThumbSpec Mir = X.S;
				Mir.Mirror = static_cast<int16_t>(Mir.Mirror ^ 1);
				Mirrored += Print(DrawOne(Mir, X.F, Me, R, false)) != Print(L) ? 1 : 0;
			}
		}
	}
	// The generator's deals: per archetype, the layouts it deals.
	std::vector<std::vector<int>> Dealt(ta::ArchCount);
	for (int G = 0; G <= 6; ++G)
	{
		for (const video::ThumbSpec& S : video::ContactSheet(G, 400))
		{
			if (S.Arch >= 0 && S.Arch < ta::ArchCount)
			{
				std::vector<int>& D = Dealt[static_cast<size_t>(S.Arch)];
				if (std::find(D.begin(), D.end(), S.Layout) == D.end())
				{
					D.push_back(S.Layout);
				}
			}
		}
	}
	int Agree = 0, Twos = 0;
	for (int A = 0; A < ta::ArchCount; ++A)
	{
		const std::vector<int>& D = Dealt[static_cast<size_t>(A)];
		int Mine = -1;
		for (const auto& Sd : Seconds())
		{
			Mine = static_cast<int>(Sd.first) == A ? static_cast<int>(Sd.second) : Mine;
		}
		Twos += D.size() >= 2 ? 1 : 0;
		const int First = FirstOf(static_cast<ta::Arch>(A));
		bool Ok = true;
		for (const int Ly : D)
		{
			Ok = Ok && (Ly == First || (Mine >= 0 && Ly == Mine));
		}
		Agree += Ok ? 1 : 0;
		if (!Ok)
		{
			std::printf("  thumbs layout   %s: the generator deals a layout that is neither its first (%d) nor the compositor's second (%d)\n", ta::ArchName(A), First, Mine);
		}
	}
	std::printf("  thumbs layouts  18 second compositions x 7 sizes: drew %d, inside %d, captions fit %d, deterministic %d of %d; LOD lighter %d, mirror moves %d, "
				"apart from the first %d of 18 (least %.2f, %s); max %zu vertices; the generator deals two layouts for %d archetypes, as the compositor draws them %d of %d\n",
		Drawn, Inside, Fits, Same, Cells, LodFewer, Mirrored, Apart, Least, LeastName.c_str(), MaxVerts, Twos, Agree, ta::ArchCount);
	Expect(Two.size() == 18, "layouts: 18 archetypes have a second layout");
	Expect(Drawn == Cells && Inside == Cells && Fits == Cells && Same == Cells, "layouts: every second composition draws inside its tile at every size, every caption in, the same every time");
	Expect(LodFewer == 18 && Mirrored == 18 && UnderBudget == 18, "layouts: every second composition's LOD is lighter, its mirror moves it, and it holds 14k vertices at 640");
	Expect(Apart == 18, "layouts: every second layout draws its own composition, not the first's (L6)");
	Expect(Agree == ta::ArchCount, "layouts: the generator deals each archetype's first layout and, for these 18, the second the compositor draws");
}

/**
 * M1's art (Studio's CAPTION, FACE and PALETTE pickers edit a dealt recipe within its archetype's values): every composition
 * draws each of the 12 faces and each palette (the 11 and Brand) differently from the others at Studio's preview (512) and
 * at its option (120, the LOD), inside the tile, within 14k at 640; each edit is a new cache key, painted through the frame's
 * budget; and the generator's deals (their captions) read at all seven sizes.
 */
void Variants()
{
	TableMeasurer M;
	const ss::hero::Character Me = Player();
	const std::vector<Fixture> All = EveryComposition();
	int FaceShown = 0, PalShown = 0, Inside = 0, Draws = 0, UnderBudget = 0, Budgeted = 0;
	for (const Fixture& X : All)
	{
		for (const float W : {512.0f, 120.0f})
		{
			const Rect R{3.0f, 2.0f, W, W * 0.5625f};
			const bool Lod = ta::LodFor(W);
			std::vector<uint64_t> Faces, Pals;
			for (int E = 0; E < 12; ++E)
			{
				video::ThumbSpec S = X.S;
				S.Expr = static_cast<int16_t>(E);
				const DrawList L = DrawOne(S, X.F, Me, R, Lod);
				Faces.push_back(Print(L));
				Inside += InsideBox(L, R) ? 1 : 0;
				++Draws;
			}
			for (int P = 0; P < static_cast<int>(ta::Pal::Count); ++P)
			{
				video::ThumbSpec S = X.S;
				S.Pal = static_cast<int16_t>(P);
				// The Site palette draws the hand's site (the s fact the generator writes with it).
				video::Thumb F = X.F;
				F.Site = P == static_cast<int>(ta::Pal::Site) ? 2 : F.Site;
				const DrawList L = DrawOne(S, F, Me, R, Lod);
				Pals.push_back(Print(L));
				Inside += InsideBox(L, R) ? 1 : 0;
				++Draws;
			}
			auto Distinct = [](std::vector<uint64_t> V) {
				std::sort(V.begin(), V.end());
				return static_cast<size_t>(std::unique(V.begin(), V.end()) - V.begin());
			};
			const size_t Df = Distinct(Faces), Dp = Distinct(Pals);
			FaceShown += Df == Faces.size() ? 1 : 0;
			PalShown += Dp == Pals.size() ? 1 : 0;
			if (Df != Faces.size() || Dp != Pals.size())
			{
				std::printf("  thumbs variants %-18s at %3.0f: %zu of 12 faces, %zu of %zu palettes look different\n", X.Name.c_str(), W, Df, Dp, Pals.size());
			}
		}
		// The heaviest face and palette at 640 within the budget.
		size_t Heaviest = 0;
		for (int E = 0; E < 12; ++E)
		{
			video::ThumbSpec S = X.S;
			S.Expr = static_cast<int16_t>(E);
			Heaviest = std::max(Heaviest, DrawOne(S, X.F, Me, {0.0f, 0.0f, 640.0f, 360.0f}, false).Vertices.size());
		}
		UnderBudget += Heaviest <= 14000 ? 1 : 0;
		if (Heaviest > 14000)
		{
			std::printf("  thumbs variants %-18s %zu vertices at 640 with its heaviest face (> 14k)\n", X.Name.c_str(), Heaviest);
		}
		// Each picker's edit is a new key, painted through the budget (not replayed as the old one).
		video::ThumbSpec Face = X.S, Pal = X.S, Cap = X.S;
		Face.Expr = static_cast<int16_t>((X.S.Expr + 1) % 12);
		Pal.Pal = static_cast<int16_t>((X.S.Pal + 1) % static_cast<int>(ta::Pal::Count));
		Cap.Caption = Cap.Caption == "WHY." ? "ONE CARD." : "WHY.";
		const float W = 120.0f, H = 68.0f;
		const uint64_t K0 = ta::KeyOf(X.S, Me, X.F, W, H, true);
		bool Fresh = true;
		DrawList L;
		Canvas C(L, M, 400.0f, 200.0f, 1.0f);
		for (const video::ThumbSpec* S : {&Face, &Pal, &Cap})
		{
			const uint64_t K = ta::KeyOf(*S, Me, X.F, W, H, true);
			ta::Budget B;
			Fresh = Fresh && K != K0 && !C.IsCached(K) && ta::Draw(C, {0.0f, 0.0f, W, H}, *S, Me, X.F, B) && B.Painted == 1 && C.IsCached(K);
		}
		Budgeted += Fresh ? 1 : 0;
	}
	const int Comps = static_cast<int>(All.size());
	std::printf("  thumbs variants %d compositions x 12 faces and %d palettes at 512 and 120: faces all different %d of %d, palettes %d of %d, inside %d of %d; "
				"640 within 14k %d of %d; each edit a new key painted through the budget %d of %d\n",
		Comps, static_cast<int>(ta::Pal::Count), FaceShown, Comps * 2, PalShown, Comps * 2, Inside, Draws, UnderBudget, Comps, Budgeted, Comps);
	Expect(Comps == 53, "variants: 53 compositions (35 archetypes, 18 second layouts)");
	Expect(FaceShown == Comps * 2, "variants: each of the 12 faces looks different on every composition, at Studio's preview and at its option (FACE)");
	Expect(PalShown == Comps * 2, "variants: each palette looks different on every composition, at the preview and at the option (PALETTE)");
	Expect(Inside == Draws && UnderBudget == Comps, "variants: every face and palette draws inside its tile, within 14k vertices at 640");
	Expect(Budgeted == Comps, "variants: a changed caption, face or palette is a new cache key, painted through the frame's budget");
	// Studio's PALETTE chips (thumbart::Swatch, c3f-ui's T1): every palette's chip is its own, inside its rect.
	std::vector<uint64_t> Chips;
	int ChipIn = 0;
	for (int P = 0; P < static_cast<int>(ta::Pal::Count); ++P)
	{
		DrawList L;
		Canvas C(L, M, 80.0f, 40.0f, 1.0f);
		const Rect R{4.0f, 4.0f, 44.0f, 24.0f};
		ta::Swatch(C, R, P, P == static_cast<int>(ta::Pal::Site) ? 2 : 0, 0x300u);
		bool In = !L.Vertices.empty();
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			// (Inside the rect and its edge's one-unit anti-aliasing fringe.)
			In = In && V.X >= R.X - 1.01f && V.X <= R.X + R.W + 1.01f && V.Y >= R.Y - 1.01f && V.Y <= R.Y + R.H + 1.01f;
		}
		ChipIn += In ? 1 : 0;
		Chips.push_back(Print(L));
	}
	std::sort(Chips.begin(), Chips.end());
	const size_t ChipKinds = static_cast<size_t>(std::unique(Chips.begin(), Chips.end()) - Chips.begin());
	std::printf("  thumbs swatch   %d of %d palette chips inside their rect, %zu different\n", ChipIn, static_cast<int>(ta::Pal::Count), ChipKinds);
	Expect(ChipIn == static_cast<int>(ta::Pal::Count) && ChipKinds == static_cast<size_t>(ta::Pal::Count), "variants: each palette's chip (Swatch) is its own, inside its rect");
	// CAPTION: the generator's deals (each archetype's caption rows as they fill) read at all seven sizes, nothing under the mark.
	int Specs = 0, Bad = 0;
	for (int G = 0; G <= 6; ++G)
	{
		for (const video::ThumbSpec& S : video::ContactSheet(G, 400))
		{
			++Specs;
			for (const auto& Sz : AllSizes)
			{
				const Rect R{7.0f, 5.0f, Sz[0], Sz[1]};
				const DrawList L = DrawOne(S, video::Thumb(), Me, R, ta::LodFor(Sz[0]));
				bool Ok = InsideBox(L, R);
				for (const ss::ui::DrawCmd& Cmd : L.Cmds)
				{
					Ok = Ok && (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Readable(Cmd.Text, R));
				}
				for (const std::string* Line : {&S.Caption, &S.Caption2})
				{
					for (const std::string& Wd : WordsOf(*Line))
					{
						Ok = Ok && Reads(L, R, Wd);
					}
				}
				if (!Ok)
				{
					++Bad;
					std::printf("  thumbs captions %-16s L%d %3.0f wide: \"%s\" / \"%s\" not all read inside the tile\n", ta::ArchName(S.Arch), S.Layout, Sz[0], S.Caption.c_str(),
						S.Caption2.substr(0, S.Caption2.find('\x1f')).c_str());
				}
			}
		}
	}
	std::printf("  thumbs captions %d dealt recipes x 7 sizes (640 to 96): %d with a word that doesn't read inside the tile\n", Specs, Bad);
	Expect(Specs >= 100 && Bad == 0, "variants: every dealt caption reads inside its tile at 640, 512, 375, 300, 211, 120 and 96 wide (CAPTION)");
}

/**
 * M1's art against the generator's own contract (c3f-content: video::ThumbChoicesOf / ThumbEdit, VideoContent.h): every
 * recipe the library deals for its fixture moments (each kind group and the ideas, both layouts), edited to every palette,
 * face, caption style and caption row its archetype allows there, is a recipe ThumbWithin accepts and draws correctly: at 640
 * and at Studio's 120 option (the LOD) inside the tile with every run readable and every caption word read, none under the
 * corner mark, within 14k vertices at 640, and visibly changed by the edit; a caption or a style edit at all seven sizes.
 */
void Pickers()
{
	const ss::hero::Character Me = Player();
	TableMeasurer Tm;
	int Specs = 0, Edits = 0, Within = 0, Drawn = 0, Bad = 0, Unchanged = 0, Heavy = 0, DuelOn = 0, Small120 = 0;
	float DuelMost = 0.0f;
	std::string DuelWorst;
	int Per[4] = {0, 0, 0, 0};
	const char* const FieldName[4] = {"palette", "face", "style", "caption"};
	std::vector<int> Archs(ta::ArchCount, 0);
	auto Check = [&](const video::ThumbSpec& E, float W, float H, const char* What) {
		const Rect R{3.0f, 2.0f, W, H};
		const DrawList L = DrawOne(E, video::Thumb(), Me, R, ta::LodFor(W));
		bool Ok = InsideBox(L, R);
		std::vector<std::string> Words = WordsOf(E.Caption);
		for (const std::string& Wd : WordsOf(E.Caption2))
		{
			Words.push_back(Wd);
		}
		const float K = W / 640.0f;
		const bool Mark = E.Corner == static_cast<int16_t>(ta::Corner::Initials) || E.Corner == static_cast<int16_t>(ta::Corner::Logo);
		const Rect Cn{R.X + ((E.Mirror & 1) != 0 ? 8.0f : 572.0f) * K, R.Y + 10.0f * K, 60.0f * K, 46.0f * K};
		float Duel = 0.0f; // at the 120 option: how far Studio's Duel letter (its ink rim on the corner) reaches into a word
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
			{
				continue;
			}
			const ss::ui::TextItem& T = Cmd.Text;
			Ok = Ok && Readable(T, R);
			const float Tw = Tm.Width(T.Text, T.Face, T.SizePx);
			const bool Caption = std::any_of(Words.begin(), Words.end(), [&T](const std::string& Wd) { return T.Text.find(Wd) != std::string::npos; });
			Ok = Ok && !(Mark && Caption && T.X < Cn.X + Cn.W && T.X + Tw > Cn.X && T.BaselineY - 0.72f * T.SizePx < Cn.Y + Cn.H && T.BaselineY > Cn.Y);
			if (W == 120.0f && Caption && Tw > 0.0f)
			{
				const float Dx = std::clamp(R.X + DuelIn, T.X, T.X + Tw) - (R.X + DuelIn);
				const float Dy = std::clamp(R.Y + DuelIn, T.BaselineY - 0.72f * T.SizePx, T.BaselineY) - (R.Y + DuelIn);
				Duel = std::max(Duel, DuelRim - std::sqrt(Dx * Dx + Dy * Dy));
			}
		}
		if (Duel > 0.0f)
		{
			++DuelOn;
			if (Duel > DuelMost)
			{
				DuelMost = Duel;
				DuelWorst = std::string(ta::ArchName(E.Arch)) + " L" + std::to_string(E.Layout) + " " + What + " \"" + E.Caption + "\"";
			}
		}
		for (const std::string& Wd : Words)
		{
			Ok = Ok && Reads(L, R, Wd);
		}
		++Drawn;
		Small120 += W == 120.0f ? 1 : 0;
		if (!Ok)
		{
			++Bad;
			std::printf("  thumbs pickers  %-16s L%d m%d %s edit at %3.0f wide: \"%s\" / \"%s\" not drawn right\n", ta::ArchName(E.Arch), E.Layout, E.Mirror, What, W, E.Caption.c_str(),
				ss::video::ThumbCaption2(E).c_str());
		}
		if (W == 640.0f && L.Vertices.size() > 14000)
		{
			++Heavy;
			std::printf("  thumbs pickers  %-16s %s edit: %zu vertices at 640 (> 14k)\n", ta::ArchName(E.Arch), What, L.Vertices.size());
		}
		return Print(L);
	};
	const std::vector<video::Candidate> All = video::vcontent::FixtureMoments();
	for (int G = 0; G <= 6; ++G)
	{
		video::Channel C;
		for (const video::Candidate& M : All)
		{
			const bool Idea = M.K == video::Kind::Idea;
			if (G == 6 ? Idea : (!Idea && ss::video::video_detail::GroupOf(M.K) == G))
			{
				C.Open.push_back(M);
			}
		}
		std::vector<std::tuple<int, int, int>> Seen;
		for (int Shuffle = 0; Shuffle < 12 && !C.Open.empty(); ++Shuffle)
		{
			video::Choice Ch;
			Ch.Candidates = {C.Open[static_cast<size_t>(Shuffle) % C.Open.size()].Id};
			Ch.F = video::Format::Breakdown;
			for (const video::ThumbSpec& S : video::ThumbOptions(Ch, C.Open, C, Shuffle, 6))
			{
				const std::tuple<int, int, int> Key{S.Arch, S.Layout, S.Pal};
				if (std::find(Seen.begin(), Seen.end(), Key) != Seen.end())
				{
					continue;
				}
				Seen.push_back(Key);
				++Specs;
				Archs[static_cast<size_t>(std::clamp<int>(S.Arch, 0, ta::ArchCount - 1))] += 1;
				const uint64_t Base = Check(S, 120.0f, 67.5f, "dealt");
				const video::ThumbChoices Cs = video::ThumbChoicesOf(S, Ch, C.Open, C);
				std::vector<std::pair<int, int>> Todo;
				for (const int16_t V : Cs.Pals)
				{
					Todo.push_back({video::TfPalette, V});
				}
				for (const int16_t V : Cs.Exprs)
				{
					Todo.push_back({video::TfFace, V});
				}
				for (const int16_t V : Cs.CapStyles)
				{
					Todo.push_back({video::TfCapStyle, V});
				}
				for (size_t I = 0; I < Cs.Captions.size(); ++I)
				{
					Todo.push_back({video::TfCaption, static_cast<int>(I)});
				}
				for (const auto& Td : Todo)
				{
					const video::ThumbSpec E = video::ThumbEdit(S, Ch, C.Open, C, Td.first, Td.second);
					++Edits;
					++Per[Td.first];
					Within += video::ThumbWithin(E, Ch, C.Open, C) ? 1 : 0;
					const bool Same = E.Pal == S.Pal && E.Expr == S.Expr && E.CapStyle == S.CapStyle && E.Caption == S.Caption && E.Caption2 == S.Caption2;
					const uint64_t Small = Check(E, 120.0f, 67.5f, FieldName[Td.first]);
					if (Td.first != video::TfPalette)
					{
						Check(E, 640.0f, 360.0f, FieldName[Td.first]); // (a palette moves no geometry: the 120 draw holds it)
					}
					if (!Same && Small == Base)
					{
						++Unchanged;
						std::printf("  thumbs pickers  %-16s L%d: the %s edit to %d draws as the dealt recipe at 120\n", ta::ArchName(S.Arch), S.Layout, FieldName[Td.first], Td.second);
					}
					if (Td.first == video::TfCaption)
					{
						for (const auto& Sz : AllSizes)
						{
							if (Sz[0] != 640.0f)
							{
								Check(E, Sz[0], Sz[1], FieldName[Td.first]);
							}
						}
					}
				}
			}
		}
	}
	int Reached = 0;
	for (const int N : Archs)
	{
		Reached += N > 0 ? 1 : 0;
	}
	std::printf("  thumbs pickers  %d dealt recipes over %d archetypes, %d edits (%d palettes, %d faces, %d styles, %d captions): within %d, %d draws (every edit at 120, a face, "
				"style or caption at 640 too, a caption at all 7 sizes) with %d wrong, %d over 14k at 640, %d edits that change nothing on screen\n",
		Specs, Reached, Edits, Per[0], Per[1], Per[2], Per[3], Within, Drawn, Bad, Heavy, Unchanged);
	std::printf("  thumbs pickers  Studio's Duel letter (the 120 option's corner) reaches a caption word's box in %d of %d draws at 120%s%s\n", DuelOn, Small120,
		DuelOn > 0 ? "; deepest: " : "", DuelOn > 0 ? (Fixed1(DuelMost) + " in, " + DuelWorst).c_str() : "");
	Expect(DuelOn == 0, "pickers: no dealt or edited recipe puts a caption word under Studio's Duel letter in the 120 option");
	Expect(Specs >= 100 && Edits >= 1000, "pickers: the library's deals and every value their archetypes allow are swept");
	Expect(Within == Edits, "pickers: every edit through ThumbEdit is a recipe ThumbWithin accepts");
	Expect(Bad == 0 && Heavy == 0, "pickers: every edited recipe draws inside its tile, every word read, none under the corner mark, within 14k at 640");
	Expect(Unchanged == 0, "pickers: every palette, face, style and caption edit changes the thumbnail at Studio's option size");
}

/**
 * L7. Venue's name on the marquee reads whole in Studio's 120 x 68 option, clear of the Duel's letter Studio draws in its top
 * left corner; and Desk Setup draws the player's own gear (the d fact) and nothing they don't own.
 */
void VenueAndDesk()
{
	const ss::hero::Character Me = Player();
	// Studio's option: the Duel letter was a 10-unit circle 14 units in from the top left (now on the corner, inside this box
	// too): the name keeps clear of the old place, the stricter box.
	const Rect Opt{0.0f, 0.0f, 120.0f, 68.0f};
	const Rect Letter{0.0f, 0.0f, 26.0f, 26.0f};
	int Whole = 0, Tries = 0;
	TableMeasurer Tm;
	for (int Mi = 0; Mi < 4; ++Mi)
	{
		for (const char* Cap : {"LIVE", "EMBERCREST", "FIRST"})
		{
			Fixture X = FixtureOf(ta::Arch::Venue);
			X.S.Mirror = static_cast<int16_t>(Mi);
			X.S.Caption = Cap;
			X.S.Caption2 = "";
			const DrawList L = DrawOne(X.S, X.F, Me, Opt, true);
			bool Ok = false;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "EMBERCREST")
				{
					const ss::ui::TextItem& T = Cmd.Text;
					const float W = Tm.Width(T.Text, T.Face, T.SizePx);
					const bool UnderLetter = T.X < Letter.X + Letter.W && T.BaselineY - 0.72f * T.SizePx < Letter.Y + Letter.H;
					Ok = Ok || (Readable(T, Opt) && !UnderLetter && W > 0.0f);
				}
			}
			Whole += Ok ? 1 : 0;
			++Tries;
		}
	}
	std::printf("  thumbs venue    EMBERCREST whole in Studio's 120 x 68 option, clear of the Duel letter: %d of %d (mirrors and zooms x captions)\n", Whole, Tries);
	Expect(Whole == Tries, "L7: Venue's EMBERCREST reads whole in the 120-wide option, clear of Studio's Duel letter");
	// For ui-rerun (Studio's letter is its file; patches/c3f-thumbs_to_c3f-ui_1.diff, applied): on how many of the 53
	// compositions x both mirrors the letter's disc meets a caption's words (their ink box, cap height to baseline) in the
	// option: as it was drawn (10 units round (14, 14)) and as Studio draws it now (its ink rim on the corner, DuelRim round
	// (DuelIn, DuelIn)).
	int Today = 0, Corner = 0, Tiles = 0;
	float Deepest = 0.0f;
	std::string CornerOn;
	for (const Fixture& X : EveryComposition())
	{
		for (int16_t Mi = 0; Mi < 2; ++Mi)
		{
			video::ThumbSpec S = X.S;
			S.Mirror = static_cast<int16_t>((S.Mirror & ~1) | Mi);
			const DrawList L = DrawOne(S, X.F, Me, Opt, true);
			std::vector<std::string> Words = WordsOf(S.Caption);
			for (const std::string& Wd : WordsOf(S.Caption2))
			{
				Words.push_back(Wd);
			}
			// How far the disc reaches into a caption word's box (<= 0: clear of it).
			auto Into = [&](float Cx, float Cy, float Rad) {
				float Most = -Rad;
				for (const ss::ui::DrawCmd& Cmd : L.Cmds)
				{
					if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
					{
						continue;
					}
					const ss::ui::TextItem& T = Cmd.Text;
					const bool Caption = std::any_of(Words.begin(), Words.end(), [&T](const std::string& Wd) { return T.Text.find(Wd) != std::string::npos; });
					const float W = Tm.Width(T.Text, T.Face, T.SizePx);
					if (!Caption || W <= 0.0f)
					{
						continue;
					}
					const float Dx = std::clamp(Cx, T.X, T.X + W) - Cx;
					const float Dy = std::clamp(Cy, T.BaselineY - 0.72f * T.SizePx, T.BaselineY) - Cy;
					Most = std::max(Most, Rad - std::sqrt(Dx * Dx + Dy * Dy));
				}
				return Most;
			};
			Today += Into(14.0f, 14.0f, 10.0f) > 0.0f ? 1 : 0;
			const float Now = Into(DuelIn, DuelIn, DuelRim);
			if (Now > 0.0f)
			{
				++Corner;
				Deepest = std::max(Deepest, Now);
				CornerOn += std::string(CornerOn.empty() ? "" : ", ") + ta::ArchName(S.Arch) + " L" + std::to_string(S.Layout) + (Mi != 0 ? " m" : "") + " " + Fixed1(Now) + " in";
			}
			++Tiles;
		}
	}
	std::printf("  thumbs duel     Studio's Duel letter over a caption word's box in the 120 option: %d of %d tiles as it was (14 in), %d on the tile's corner (Studio's now%s%s)\n", Today, Tiles, Corner, CornerOn.empty() ? "" : ": ", CornerOn.c_str());
	// On the corner, clear of every word (Tier List, The Bubble, The Read 2, Caught 2 and Day Counter 2 keep their words out of
	// it; 14 in, it covered 17 tiles' words).
	Expect(Corner == 0 && Deepest == 0.0f, "the Duel letter on the option's corner is clear of every composition's caption words");
	// Desk Setup: the d fact's gear on the desk; none without it; an id the store doesn't sell draws nothing.
	const Rect R{0.0f, 0.0f, 640.0f, 360.0f};
	Fixture D = FixtureOf(ta::Arch::DeskSetup);
	D.S.Caption2 = "";
	const size_t Bare = DrawOne(D.S, D.F, Me, R, false).Vertices.size();
	const uint64_t BarePrint = Print(DrawOne(D.S, D.F, Me, R, false));
	D.S.Caption2 = Facts({{"d", "spaceship|hoverboard"}});
	const bool Unknown = Print(DrawOne(D.S, D.F, Me, R, false)) == BarePrint;
	D.S.Caption2 = Facts({{"d", "monitor-24"}});
	const size_t One = DrawOne(D.S, D.F, Me, R, false).Vertices.size();
	D.S.Caption2 = Facts({{"d", "monitor-24|monitor-27|tower-dual|mirrorless|mic-xlr|key-lights|ring-light|green-screen|led-kit|plant|espresso|headphones|macro-pad|curtains|fiber|chair"}});
	const DrawList Full = DrawOne(D.S, D.F, Me, R, false);
	const DrawList FullRow = DrawOne(D.S, D.F, Me, {0.0f, 0.0f, 120.0f, 68.0f}, true);
	std::printf("  thumbs desk     Desk Setup at 640: %zu vertices bare, %zu with a monitor, %zu with every piece (%zu at the 120 option)\n", Bare, One, Full.Vertices.size(), FullRow.Vertices.size());
	Expect(Unknown && One > Bare + 40 && Full.Vertices.size() > One, "L7: Desk Setup draws the gear the d fact names, nothing without it, nothing the store doesn't sell");
	Expect(Full.Vertices.size() <= 14000 && InsideBox(Full, R) && InsideBox(FullRow, {0.0f, 0.0f, 120.0f, 68.0f}), "L7: Desk Setup with every piece of gear inside its tile and within 14k vertices");
}

/** A colour's relative luminance (WCAG). */
float Luminance(const ss::ui::Color& C)
{
	auto Lin = [](float V) { return V <= 0.04045f ? V / 12.92f : std::pow((V + 0.055f) / 1.055f, 2.4f); };
	return 0.2126f * Lin(C.R) + 0.7152f * Lin(C.G) + 0.0722f * Lin(C.B);
}

/** The colour the list has painted at (Px, Py) before command Upto, over black: its triangles in order, blended, clips kept. */
ss::ui::Color PaintedAt(const DrawList& L, size_t Upto, float Px, float Py)
{
	ss::ui::Color Acc{0.0f, 0.0f, 0.0f, 1.0f};
	std::vector<Rect> Clips;
	for (size_t K = 0; K < Upto && K < L.Cmds.size(); ++K)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[K];
		if (Cmd.Type == ss::ui::DrawCmd::Kind::PushClip || Cmd.Type == ss::ui::DrawCmd::Kind::PopClip)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::PushClip)
			{
				Clips.push_back(Cmd.Clip);
			}
			else if (!Clips.empty())
			{
				Clips.pop_back();
			}
			continue;
		}
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles ||
			!std::all_of(Clips.begin(), Clips.end(), [Px, Py](const Rect& R) { return Px >= R.X && Px <= R.X + R.W && Py >= R.Y && Py <= R.Y + R.H; }))
		{
			continue;
		}
		for (uint32_t I = 0; I + 2 < Cmd.IndexCount; I += 3)
		{
			const ss::ui::Vertex& A = L.Vertices[Cmd.VertexStart + L.Indices[Cmd.IndexStart + I]];
			const ss::ui::Vertex& B = L.Vertices[Cmd.VertexStart + L.Indices[Cmd.IndexStart + I + 1]];
			const ss::ui::Vertex& C = L.Vertices[Cmd.VertexStart + L.Indices[Cmd.IndexStart + I + 2]];
			const float D = (B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y);
			if (std::fabs(D) < 1e-6f)
			{
				continue;
			}
			const float Wa = ((B.Y - C.Y) * (Px - C.X) + (C.X - B.X) * (Py - C.Y)) / D;
			const float Wb = ((C.Y - A.Y) * (Px - C.X) + (A.X - C.X) * (Py - C.Y)) / D;
			const float Wc = 1.0f - Wa - Wb;
			if (Wa < 0.0f || Wb < 0.0f || Wc < 0.0f)
			{
				continue;
			}
			const float Al = Wa * A.Col.A + Wb * B.Col.A + Wc * C.Col.A;
			Acc.R = Acc.R * (1.0f - Al) + (Wa * A.Col.R + Wb * B.Col.R + Wc * C.Col.R) * Al;
			Acc.G = Acc.G * (1.0f - Al) + (Wa * A.Col.G + Wb * B.Col.G + Wc * C.Col.G) * Al;
			Acc.B = Acc.B * (1.0f - Al) + (Wa * A.Col.B + Wb * B.Col.B + Wc * C.Col.B) * Al;
		}
	}
	return Acc;
}

/**
 * The lowest contrast (WCAG) of a caption's plain runs (drawn once: a plate's or a stamp's ink, with no outline under it)
 * against what is painted under them, across each run at a quarter, a half and three quarters; 99 when there are none.
 */
float PlainCaptionContrast(const DrawList& L, const video::ThumbSpec& S, std::string* Which = nullptr)
{
	TableMeasurer M;
	std::vector<std::string> Words = WordsOf(S.Caption);
	for (const std::string& W : WordsOf(S.Caption2))
	{
		Words.push_back(W);
	}
	float Worst = 99.0f;
	for (size_t K = 0; K < L.Cmds.size(); ++K)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[K];
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || RunsOf(L, Cmd.Text.Text) != 1 ||
			std::none_of(Words.begin(), Words.end(), [&Cmd](const std::string& W) { return Cmd.Text.Text.find(W) != std::string::npos; }))
		{
			continue;
		}
		const ss::ui::TextItem& T = Cmd.Text;
		const float W = M.Width(T.Text, T.Face, T.SizePx), Ink = Luminance(T.Col);
		for (const float U : {0.25f, 0.5f, 0.75f})
		{
			const float Bg = Luminance(PaintedAt(L, K, T.X + W * U, T.BaselineY - T.SizePx * 0.36f));
			const float Ct = (std::max(Ink, Bg) + 0.05f) / (std::min(Ink, Bg) + 0.05f);
			if (Ct < Worst)
			{
				Worst = Ct;
				if (Which != nullptr)
				{
					*Which = T.Text;
				}
			}
		}
	}
	return Worst;
}

/**
 * review-c3f-thumbs (mutants of the second compositions that the checks above let through): Day Counter 2's road goes as far
 * as the challenge has (the p fact, else the caption's start, now and goal), not always to the flag; Top Five 2's cells hold
 * the r fact's rows in their numbered order; and every composition draws the recipe's pose on the player (Headhunter's
 * hunter wore the dealt face but always the same arms).
 */
void Truths()
{
	const ss::hero::Character Me = Player();
	const Rect R{0.0f, 0.0f, 640.0f, 360.0f};
	TableMeasurer Tm;
	// The fill run of a line (Loud draws its shadow and outline first): the last run that is exactly S.
	auto FillOf = [](const DrawList& L, const std::string& S) {
		const ss::ui::TextItem* Out = nullptr;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			Out = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == S ? &Cmd.Text : Out;
		}
		return Out;
	};
	auto Mid = [&Tm](const ss::ui::TextItem& T) { return T.X + Tm.Width(T.Text, T.Face, T.SizePx) * 0.5f; };
	// Day Counter 2 (DayCounter2: the road from (64, 290) to the goal's flag at (396, 176), the amount now over where it has
	// got to): the p fact's 36%, the caption's own $50 / $214 / $500 (36.4%) with no p, and a p of 80%.
	Fixture Dc = FixtureOf(ta::Arch::DayCounter);
	Dc.S.Layout = static_cast<int16_t>(ta::Layout::Graph);
	const std::string Amounts = "$50 \xC2\xB7 $214 \xC2\xB7 $500";
	int Roads = 0;
	std::string RoadsLine;
	for (const auto& Case : std::vector<std::pair<std::string, float>>{{"36", 0.36f}, {"", 164.0f / 450.0f}, {"80", 0.8f}})
	{
		video::ThumbSpec S = Dc.S;
		S.Caption2 = Amounts + (Case.first.empty() ? Facts({{"s", "1"}}) : Facts({{"p", Case.first}, {"s", "1"}}));
		const DrawList L = DrawOne(S, Dc.F, Me, R, false);
		const ss::ui::TextItem* Now = FillOf(L, "$214");
		const float Want = 64.0f + (396.0f - 64.0f) * Case.second;
		const bool Ok = Now != nullptr && std::fabs(Mid(*Now) - Want) <= 2.0f;
		Roads += Ok ? 1 : 0;
		RoadsLine += std::string(RoadsLine.empty() ? "" : ", ") + Fixed1(Now != nullptr ? Mid(*Now) : -1.0f) + " (want " + Fixed1(Want) + ")";
	}
	std::printf("  thumbs truths   Day Counter 2's amount now along the road: %d of 3 where the challenge has got to: %s\n", Roads, RoadsLine.c_str());
	Expect(Roads == 3, "truths: Day Counter 2's road goes as far as the challenge has (the p fact, else the caption's start, now and goal)");
	// Top Five 2 (TopFive2): number one's panel holds row 1 beside its "1"; each of the four cells holds the row of its number,
	// to the right of it and under it, mirrored or not.
	const Fixture Tf = FixtureOf(ta::Arch::TopFive);
	const std::vector<std::string> Rows = {"QUADS NINES", "ACES HOLD", "THE 7-2 BLUFF", "SET OVER SET", "RIVER FLUSH"};
	int Cells = 0, CellTries = 0;
	for (const int16_t Mi : {static_cast<int16_t>(0), static_cast<int16_t>(1)})
	{
		video::ThumbSpec S = Tf.S;
		S.Layout = static_cast<int16_t>(ta::Layout::Collage);
		S.Mirror = Mi;
		const DrawList L = DrawOne(S, Tf.F, Me, R, false);
		for (size_t K = 0; K < Rows.size(); ++K)
		{
			++CellTries;
			const ss::ui::TextItem* Num = FillOf(L, std::to_string(K + 1));
			const ss::ui::TextItem* Row = FillOf(L, Rows[K]);
			const bool Ok = Num != nullptr && Row != nullptr && RunsOf(L, Rows[K]) == 1 && Mid(*Row) > Mid(*Num) && Mid(*Row) - Mid(*Num) < 150.0f && Row->BaselineY >= Num->BaselineY - 14.0f &&
				Row->BaselineY - Num->BaselineY < 40.0f;
			Cells += Ok ? 1 : 0;
			if (!Ok)
			{
				std::printf("  thumbs truths   Top Five 2%s: row %zu (\"%s\") not beside its number\n", Mi != 0 ? " mirrored" : "", K + 1, Rows[K].c_str());
			}
		}
	}
	std::printf("  thumbs truths   Top Five 2's rows beside their numbers: %d of %d (mirrored and not)\n", Cells, CellTries);
	Expect(Cells == CellTries, "truths: Top Five 2 holds the r fact's rows in their numbered order, each once");
	// The pose: every composition with the player in it draws Point and Fist pump differently at Studio's preview. (Reaction
	// Box's first layout puts your face alone in its corner box, no hands: the one that doesn't, by its composition.)
	int Posed = 0, People = 0;
	std::string Same;
	for (const Fixture& X : EveryComposition())
	{
		if ((X.S.Subj != static_cast<int16_t>(ta::Subject::You) && X.S.Arch != static_cast<int16_t>(ta::Arch::Headhunter)) ||
			(X.S.Arch == static_cast<int16_t>(ta::Arch::ReactionBox) && X.S.Layout == FirstOf(ta::Arch::ReactionBox)))
		{
			continue;
		}
		++People;
		video::ThumbSpec A = X.S, B = X.S;
		A.Pose = static_cast<int16_t>(ss::ui::PortraitPose::Point);
		B.Pose = static_cast<int16_t>(ss::ui::PortraitPose::FistPump);
		const Rect P{0.0f, 0.0f, 512.0f, 288.0f};
		const bool Differ = Print(DrawOne(A, X.F, Me, P, false)) != Print(DrawOne(B, X.F, Me, P, false));
		Posed += Differ ? 1 : 0;
		Same += Differ ? std::string() : std::string(Same.empty() ? "" : ", ") + X.Name;
	}
	std::printf("  thumbs truths   the recipe's pose drawn on the player: %d of %d compositions%s%s\n", Posed, People, Same.empty() ? "" : "; the same: ", Same.c_str());
	Expect(People >= 30 && Posed == People, "truths: every composition with the player in it draws the recipe's pose (Headhunter's hunter too)");
	// A stamp's ink reads on its plate: Before -> After (the lower line and, with none, the After panel) and Trophy Shot
	// stamped in every palette and plate (dark, the accent's ink, white) hold 3:1 at Studio's preview. (Sunrise's crimson
	// plate under HotRed was about 2:1.)
	const Rect Pv{0.0f, 0.0f, 512.0f, 288.0f};
	int Stamps = 0, Inked = 0;
	float Faintest = 99.0f;
	std::string FaintOn;
	for (const ta::Arch Ar : {ta::Arch::BeforeAfter, ta::Arch::TrophyShot})
	{
		for (int Pl = 0; Pl < static_cast<int>(ta::Pal::Count); ++Pl)
		{
			for (const uint32_t Bits : {0x00u, 0x20u, 0x40u})
			{
				for (const bool Lower : {true, false})
				{
					Fixture X = FixtureOf(Ar);
					X.S.CapStyle = static_cast<int16_t>(ta::CapStyle::Stamp);
					X.S.Pal = static_cast<int16_t>(Pl);
					X.S.Variant = (X.S.Variant & ~0x60u) | Bits;
					if (!Lower && Ar == ta::Arch::BeforeAfter)
					{
						X.S.Caption = "$11";
						X.S.Caption2 = "$1,190";
					}
					std::string Which;
					const float Ct = PlainCaptionContrast(DrawOne(X.S, X.F, Me, Pv, false), X.S, &Which);
					++Stamps;
					Inked += Ct >= 3.0f ? 1 : 0;
					if (Ct < Faintest)
					{
						Faintest = Ct;
						FaintOn = std::string(ta::ArchName(static_cast<int>(Ar))) + " palette " + std::to_string(Pl) + " \"" + Which + "\"";
					}
				}
			}
		}
	}
	std::printf("  thumbs truths   stamped captions holding 3:1 on their plates: %d of %d (faintest %.2f, %s)\n", Inked, Stamps, Faintest, FaintOn.c_str());
	Expect(Inked == Stamps, "truths: a stamp's words hold 3:1 on their plate in every palette (Before -> After, Trophy Shot)");
	// Before -> After's lower line in each of its three styles reads at a like size (a stamp's words were 0.58 of the band's).
	const Fixture Ba = FixtureOf(ta::Arch::BeforeAfter);
	float Sizes3[3] = {0.0f, 0.0f, 0.0f};
	const ta::CapStyle Three[3] = {ta::CapStyle::LowerThird, ta::CapStyle::Stamp, ta::CapStyle::Outlined};
	for (int K = 0; K < 3; ++K)
	{
		video::ThumbSpec S = Ba.S;
		S.CapStyle = static_cast<int16_t>(Three[K]);
		const DrawList L = DrawOne(S, Ba.F, Me, R, false);
		const ss::ui::TextItem* T = FillOf(L, S.Caption2.empty() ? S.Caption : S.Caption + " \xC2\xB7 " + S.Caption2.substr(0, S.Caption2.find('\x1f')));
		T = T != nullptr ? T : FillOf(L, S.Caption);
		Sizes3[K] = T != nullptr ? T->SizePx : 0.0f;
	}
	std::printf("  thumbs truths   Before -> After's lower line at 640: lower third %.1f, stamp %.1f, outlined %.1f units\n", Sizes3[0], Sizes3[1], Sizes3[2]);
	Expect(Sizes3[0] > 0.0f && Sizes3[1] >= Sizes3[0] * 0.8f && Sizes3[2] >= Sizes3[0] * 0.8f, "truths: Before -> After's lower line reads at a like size in each of its styles");
	// And the generator's own deals: every plain caption run holds 3:1 at Studio's preview.
	int Deals = 0, Held = 0;
	for (int G = 0; G <= 6; ++G)
	{
		for (const video::ThumbSpec& S : video::ContactSheet(G, 400))
		{
			std::string Which;
			const float Ct = PlainCaptionContrast(DrawOne(S, video::Thumb(), Me, Pv, false), S, &Which);
			++Deals;
			Held += Ct >= 3.0f ? 1 : 0;
			if (Ct < 3.0f)
			{
				std::printf("  thumbs truths   %s L%d palette %d: \"%s\" at %.2f:1\n", ta::ArchName(S.Arch), S.Layout, S.Pal, Which.c_str(), Ct);
			}
		}
	}
	std::printf("  thumbs truths   the generator's deals: %d of %d with every plain caption run at 3:1 or more\n", Held, Deals);
	Expect(Deals >= 100 && Held == Deals, "truths: the generator's deals' plain caption words hold 3:1 on what's under them");
}

/** The C3 fixes' sheets, rasterized for the eye. */
void FixSheets()
{
	if (OutDir.empty())
	{
		return;
	}
	const std::vector<Fixture> Two = SecondFixtures();
	std::vector<Fixture> Pairs;
	for (const Fixture& X : Two)
	{
		Fixture One = X;
		One.S.Layout = static_cast<int16_t>(FirstOf(static_cast<ta::Arch>(X.S.Arch)));
		One.Name = ta::ArchName(X.S.Arch);
		Pairs.push_back(One);
		Pairs.push_back(X);
	}
	Sheet("thumbs_sheet_layouts", Pairs, 0, Pairs.size(), 300.0f, 169.0f, 6, false, false);
	Sheet("thumbs_sheet_layouts_mirror", Two, 0, Two.size(), 300.0f, 169.0f, 6, false, true);
	Sheet("thumbs_sheet_layouts_hero_1", Two, 0, 9, 640.0f, 360.0f, 3, false, false);
	Sheet("thumbs_sheet_layouts_hero_2", Two, 9, 9, 640.0f, 360.0f, 3, false, false);
	OptionSheet("thumbs_sheet_option_layouts", Two, 120.0f, 68.0f, 4.0f, false);
	// The pickers: one composition's 12 faces and 12 palettes at the option size.
	std::vector<Fixture> Picks;
	for (const ta::Arch A : {ta::Arch::GutPunch, ta::Arch::Headhunter})
	{
		for (int E = 0; E < 12; ++E)
		{
			Fixture X = FixtureOf(A);
			X.S.Expr = static_cast<int16_t>(E);
			X.Name = "face " + std::to_string(E);
			Picks.push_back(X);
		}
	}
	for (int P = 0; P < static_cast<int>(ta::Pal::Count); ++P)
	{
		Fixture X = FixtureOf(ta::Arch::TheBubble);
		X.S.Pal = static_cast<int16_t>(P);
		X.F.Site = P == static_cast<int>(ta::Pal::Site) ? 2 : X.F.Site;
		X.Name = "palette " + std::to_string(P);
		Picks.push_back(X);
	}
	OptionSheet("thumbs_sheet_pickers", Picks, 120.0f, 68.0f, 4.0f, false);
	// Desk Setup: bare, a starter desk, every piece.
	std::vector<Fixture> Desks;
	for (const char* G : {"", "webcam-720|mic-usb|ring-light", "monitor-24|monitor-27|tower-dual|mirrorless|mic-xlr|key-lights|green-screen|plant|espresso|headphones|chair"})
	{
		Fixture X = FixtureOf(ta::Arch::DeskSetup);
		X.S.Caption2 = G[0] != '\0' ? Facts({{"d", G}}) : std::string();
		Desks.push_back(X);
	}
	Sheet("thumbs_sheet_desk", Desks, 0, Desks.size(), 640.0f, 360.0f, 3, false, false);
	OptionSheet("thumbs_sheet_option_desk", Desks, 120.0f, 68.0f, 4.0f, false);
}
} // namespace thumbs_seg

void SegThumbs()
{
	thumbs_seg::Thumbnails();
	const auto T0 = std::chrono::steady_clock::now();
	thumbs_seg::Archetypes();
	thumbs_seg::FactsBlock();
	thumbs_seg::OutOfRange();
	thumbs_seg::Faces();
	thumbs_seg::Keys();
	thumbs_seg::Progressive();
	thumbs_seg::Legacy();
	thumbs_seg::Words();
	thumbs_seg::Generated();
	thumbs_seg::ReviewFixes();
	thumbs_seg::Layouts();
	thumbs_seg::Variants();
	thumbs_seg::Pickers();
	thumbs_seg::VenueAndDesk();
	thumbs_seg::Truths();
	thumbs_seg::Sheets();
	thumbs_seg::FixSheets();
	const char* St = std::getenv("SS_THUMBS_ST");
	thumbs_seg::SpikeST(St != nullptr && St[0] == '1');
	std::printf("  thumbs          ui-thumbs' checks in %.1f s\n", thumbs_seg::Ms(T0, std::chrono::steady_clock::now()) / 1000.0);
}
} // namespace ui_test
