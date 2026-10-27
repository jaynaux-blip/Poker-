// video_channel: Update 2's title and copy checks (update 2 SPEC §6.7; moved verbatim from video_main.cpp at update 3's
// Phase A, SPEC §7.7, §8.2; core-videocontent's file from then on). The copy lint and the trademark screen (tests/Trademarks.h)
// over Rerun's inventory, every generated title of every kind, style and format filled, linted and clean; storytime titles
// that name nobody; a live event's price said once; a bluff's "nothing"; Studio's rules for what goes up together and the
// Recap's title and trophy; a cooler the player won never told as a loss. Compiled into video_test and called from main()
// in video_main.cpp (the kit is VideoSeg.h).
#include "VideoSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoContent.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace video_test
{
namespace video = ss::video;
using ss::Chips;

/** No "AI", "bot" or "autoplay" in the world's words (U1), as whole words. */
bool SaysAutoplay(const std::string& Text)
{
	const std::string Low = Lower(Text);
	for (const char* Word : {"ai", "bot", "bots", "autoplay", "autopilot"})
	{
		const std::string W(Word);
		for (size_t At = Low.find(W); At != std::string::npos; At = Low.find(W, At + 1))
		{
			const bool Start = At == 0 || !std::isalnum(static_cast<unsigned char>(Low[At - 1]));
			const bool End = At + W.size() >= Low.size() || !std::isalnum(static_cast<unsigned char>(Low[At + W.size()]));
			if (Start && End)
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * The trademark screen (tests/Trademarks.h, both tiers, whole words): no real video platform, feature, payment wallet, poker
 * site or creator. "shorts" is on Rerun's screen since C3's Cut rename reworded Studio's Short line (VideoCopy.cpp).
 */
bool RealBrand(const std::string& Text, std::string& Which)
{
	const char* Hit = trademarks::Find(Text, trademarks::kSourceScan);
	Hit = Hit ? Hit : trademarks::Find(Text, trademarks::kCopyScan);
	if (Hit)
	{
		Which = Hit;
		return true;
	}
	return false;
}

void Copy()
{
	std::map<std::string, std::string> Vars = {{"Event", "Night Owl Turbo"}, {"Stakes", "$5.50 Night Owl Turbo"}, {"BuyIn", "$5.50"}, {"Bb", "64"}, {"Eq", "81"},
		{"Mine", "AA"}, {"MineWords", "aces"}, {"Theirs", "77"}, {"TheirsWords", "sevens"}, {"Killer", "a seven on the river"}, {"KillStreet", "river"},
		{"Saver", "a king on the turn"}, {"SaveStreet", "turn"}, {"Street", "river"}, {"Spot", "on the bubble"}, {"Opp", "gh0stfold"}, {"HandNo", "214"}, {"Place", "4th"},
		{"PlaceWord", "fourth"}, {"Field", "104"}, {"Prize", "$1,190"}, {"Count", "5"}, {"Weekday", "Tuesday"}, {"Ender", "the river that ended it"}};
	std::vector<std::pair<std::string, std::string>> Titles, Lines, Subjects, Bodies, Labels;
	for (const std::string& E : video::Inventory())
	{
		const size_t Tab = E.find('\t');
		const std::string Kind = E.substr(0, Tab), Text = E.substr(Tab + 1);
		(Kind == "title" ? Titles : Kind == "line" ? Lines : Kind == "subject" ? Subjects : Kind == "body" ? Bodies : Labels).push_back({Kind, Text});
	}
	Expect(Titles.size() >= 250 && !Lines.empty() && Subjects.size() == 4 && Bodies.size() == 4, "copy: the inventory has every kind of line (" + std::to_string(Titles.size()) + " titles)");
	// Titles aren't sentences: the lint's closing stop is the one rule they skip.
	std::vector<std::pair<std::string, std::string>> Stopped;
	for (const auto& T : Titles)
	{
		Stopped.push_back({T.first, T.second + "."});
	}
	int Problems = copylint::Run("rerun titles", Stopped, 101, Vars);
	Problems += copylint::Run("rerun lines", Lines, copylint::Pa, Vars);
	Problems += copylint::Run("rerun subjects", Subjects, copylint::MailSubject, Vars);
	Problems += copylint::Run("rerun letters", Bodies, copylint::LetterBody, Vars);
	Expect(Problems == 0 && copylint::SelfCheck() == 0, "copy: the lint passes (" + std::to_string(Problems) + " problems)");
	// The world never says who decided (U1), and no real brand anywhere (the trademark screen).
	int Said = 0, Brands = 0;
	for (const std::string& E : video::Inventory())
	{
		const std::string Filled = copylint::Fill(E.substr(E.find('\t') + 1), Vars);
		std::string Which;
		if (SaysAutoplay(Filled))
		{
			++Said;
			std::printf("  U1: \"%s\"\n", Filled.c_str());
		}
		if (RealBrand(Filled, Which))
		{
			++Brands;
			std::printf("  trademark '%s': \"%s\"\n", Which.c_str(), Filled.c_str());
		}
	}
	// Rerun 2.0's library (C3): every title template, the comments, the shows, the lines and the letters, unfilled.
	int SaidU3 = 0, BrandsU3 = 0;
	for (const std::string& E : video::InventoryU3())
	{
		const std::string Text = E.substr(E.find('\t') + 1);
		std::string Hit;
		SaidU3 += SaysAutoplay(Text) ? 1 : 0;
		if (RealBrand(Text, Hit))
		{
			++BrandsU3;
			std::printf("  trademark '%s' (Update 3): \"%s\"\n", Hit.c_str(), Text.c_str());
		}
	}
	Expect(video::InventoryU3().size() >= 2000 && SaidU3 == 0 && BrandsU3 == 0, "copy: Rerun 2.0's library says no AI, bot or autoplay and no real brand (" +
		std::to_string(SaidU3) + ", " + std::to_string(BrandsU3) + ")");
	std::string Which;
	Expect(Said == 0, "copy: no line says AI, bot or autoplay");
	Expect(Brands == 0 && !RealBrand("Rerun", Which), "copy: the trademark screen passes over Rerun and every line");
	// Titles generated from real candidates of every kind, by every style and format: every one non-empty, linted, and clean.
	int Bad = 0;
	bool Sd = false;
	const ss::recall::Record Beat = BadBeatHand(Sd);
	for (int K = 0; K < video::KindCountU2; ++K)
	{
		video::Channel C;
		for (int I = 0; I < 3; ++I)
		{
			video::Candidate X = Moment("rl:copy@9:" + std::to_string(I), 50.0 * Day + I);
			X.K = static_cast<video::Kind>(K);
			X.Rec = Beat;
			X.Rec.Number = 200 + I;
			X.EquityAtAllIn = 0.81;
			X.Opponent = K == static_cast<int>(video::Kind::Rival) ? ss::RivalName : "Mike_R";
			X.Rival = K == static_cast<int>(video::Kind::Rival);
			if (X.K == video::Kind::Story)
			{
				X.Id = "story:rent-paid";
				X.Event = "rent-paid";
				X.Rec = ss::recall::Record();
			}
			if (X.K == video::Kind::Win || X.K == video::Kind::FinalTable || X.K == video::Kind::DeepRun)
			{
				X.Rec = ss::recall::Record();
				X.Rec.Pot = 119000;
				X.PlaceSoFar = X.K == video::Kind::Win ? 1 : 4;
				X.Field = 104;
			}
			video::NoteHand(C, X);
		}
		for (int F = 0; F < 4; ++F)
		{
			for (int T = 0; T < 5; ++T)
			{
				video::Choice Ch;
				Ch.F = static_cast<video::Format>(F);
				Ch.T = static_cast<video::TitleStyle>(T);
				for (const video::Candidate& X : C.Open)
				{
					Ch.Candidates.push_back(X.Id);
					if (F < 2)
					{
						break;
					}
				}
				if (!video::CanUpload(Ch, C.Open).empty())
				{
					continue;
				}
				const std::string Title = video::Title(Ch, C.Open, 0);
				std::string Brand;
				const std::string Why = copylint::Check(Title + ".", 101, {});
				if (Title.empty() || !Why.empty() || SaysAutoplay(Title) || RealBrand(Title, Brand) || Title.find('{') != std::string::npos)
				{
					++Bad;
					std::printf("  title: \"%s\" %s\n", Title.c_str(), Why.c_str());
				}
			}
		}
	}
	Expect(Bad == 0, "copy: every generated title is filled, linted and clean");
}

/** Every title a storytime moment can get (every template, by sweeping the seed): no name, and it reads as English. */
void Storytime()
{
	video::Channel C;
	ss::room::HandNote N;
	N.AllIn = true;
	N.Called = true;
	std::vector<std::string> Ids;
	const video::Kind Kinds[] = {video::Kind::BadBeat, video::Kind::Suckout, video::Kind::Cooler, video::Kind::Bluff, video::Kind::BluffCaught, video::Kind::HeroCall,
		video::Kind::HeroFold, video::Kind::BigPot, video::Kind::Knockout};
	for (int I = 0; I < 9; ++I)
	{
		video::Candidate X;
		X.Id = "dee:9:" + std::to_string(40 + I);
		X.At = 9.0 * Day + 23.0 * 60.0 + I;
		X.Source = 2;
		X.K = Kinds[I];
		X.Rec = DeesHand();
		X.Rec.Number = 40 + I;
		X.EquityAtAllIn = 0.81;
		X.Opponent = "Big Lou";
		X.Event = "Dee's game";
		X.BuyIn = 4000;
		X.Storytime = true;
		video::NoteHand(C, X);
		Ids.push_back(X.Id);
	}
	int Bad = 0, Seen = 0;
	for (int F = 0; F < 3; ++F)
	{
		for (int T = 0; T < 5; ++T)
		{
			for (size_t K = 0; K < Ids.size(); ++K)
			{
				video::Choice Ch;
				Ch.F = static_cast<video::Format>(F);
				Ch.T = static_cast<video::TitleStyle>(T);
				Ch.Candidates = {Ids[K]};
				if (Ch.F == video::Format::Vlog)
				{
					Ch.Candidates.push_back(Ids[(K + 1) % Ids.size()]);
					Ch.Candidates.push_back(Ids[(K + 2) % Ids.size()]);
				}
				if (!video::CanUpload(Ch, C.Open).empty())
				{
					continue;
				}
				for (uint32_t Seed = 1; Seed <= 40; ++Seed)
				{
					const std::string Title = video::Title(Ch, C.Open, Seed);
					const std::string Low = " " + Lower(Title) + " ";
					++Seen;
					if (Title.empty() || Low.find(" my a ") != std::string::npos || Low.find(" the a ") != std::string::npos || Low.find(" of a back") != std::string::npos ||
						Low.find("lou") != std::string::npos || Low.find("dee") != std::string::npos || !copylint::Check(Title + ".", 101, {}).empty())
					{
						++Bad;
						std::printf("  storytime title: \"%s\"\n", Title.c_str());
					}
				}
			}
		}
	}
	Expect(Seen > 1000 && Bad == 0, "storytime: every title a back-room hand can get names nobody and reads right (" + std::to_string(Bad) + " of " + std::to_string(Seen) + ")");
	// A live event's name with its price on it (the card room's "Embercrest Nightly $120") says the price once, in front.
	video::Channel Live;
	video::Candidate X = Moment("lv:embercrest-nightly@9:12", 9.0 * Day + 21.0 * 60.0);
	X.Source = 1;
	X.Event = "Embercrest Nightly $120";
	X.BuyIn = 12000;
	video::NoteHand(Live, X);
	int Twice = 0, Priced = 0;
	for (int T = 0; T < 5; ++T)
	{
		video::Choice Ch;
		Ch.Candidates = {X.Id};
		Ch.T = static_cast<video::TitleStyle>(T);
		for (uint32_t Seed = 1; Seed <= 40; ++Seed)
		{
			const std::string Title = video::Title(Ch, Live.Open, Seed);
			const size_t First = Title.find("$120");
			Twice += First != std::string::npos && Title.find("$120", First + 1) != std::string::npos ? 1 : 0;
			Twice += Lower(Title).find("nightly $120") != std::string::npos ? 1 : 0;
			Priced += First != std::string::npos ? 1 : 0;
		}
	}
	Expect(Twice == 0 && Priced > 0, "titles: a live event's price is said once, in front (" + video::Title({{X.Id}, video::Format::Breakdown, video::TitleStyle::Plain}, Live.Open, 3) + ")");
	// A bluff with bottom pair didn't have "nothing"; one with no pair can say so.
	int SaidNothing[2] = {0, 0};
	for (int Pair = 0; Pair < 2; ++Pair)
	{
		video::Channel B;
		video::Candidate Y = Moment("rl:bluff@9:" + std::to_string(Pair), 9.0 * Day);
		Y.K = video::Kind::Bluff;
		Y.Rec.Mine = ss::ParseCards(Pair == 1 ? "3d2d" : "6s5s");
		Y.Rec.Board = ss::ParseCards("KhQd2cJs8h");
		Y.Rec.HeroNet = 3000;
		video::NoteHand(B, Y);
		for (int T = 0; T < 5; ++T)
		{
			video::Choice Ch;
			Ch.Candidates = {Y.Id};
			Ch.T = static_cast<video::TitleStyle>(T);
			for (uint32_t Seed = 1; Seed <= 40; ++Seed)
			{
				SaidNothing[Pair] += Lower(video::Title(Ch, B.Open, Seed)).find("nothing") != std::string::npos ? 1 : 0;
			}
		}
	}
	Expect(SaidNothing[0] > 0 && SaidNothing[1] == 0, "titles: a bluff says it had nothing only when it had no pair");
}

void Rules()
{
	// Studio's rules for what goes up together.
	video::Channel C;
	for (int I = 0; I < 9; ++I)
	{
		video::NoteHand(C, Moment("lv:sunday@40:" + std::to_string(I), 40.0 * Day + 20.0 * 60.0 + I));
	}
	video::Candidate Recap = Moment("lv:sunday@40:recap", 40.0 * Day + 23.0 * 60.0);
	Recap.K = video::Kind::FinalTable;
	Recap.PlaceSoFar = 4;
	Recap.Field = 88;
	Recap.Rec = ss::recall::Record();
	Recap.Rec.Pot = 119000;
	video::NoteHand(C, Recap);
	video::NoteHand(C, Moment("rl:other@41:1", 42.0 * Day));
	video::Choice Ch;
	Ch.F = video::Format::Vlog;
	Ch.Candidates = {"lv:sunday@40:0", "lv:sunday@40:1"};
	Expect(!video::CanUpload(Ch, C.Open).empty(), "rules: a Vlog needs three moments");
	Ch.Candidates.push_back("lv:sunday@40:recap");
	Expect(video::CanUpload(Ch, C.Open).empty(), "rules: three moments from one night make a Vlog");
	Ch.Candidates.push_back("rl:other@41:1");
	Expect(!video::CanUpload(Ch, C.Open).empty(), "rules: a Vlog's moments are one night's");
	Ch.F = video::Format::Recap;
	Ch.Candidates = {"lv:sunday@40:recap", "lv:sunday@40:2"};
	Expect(!video::CanUpload(Ch, C.Open).empty(), "rules: a Recap needs two hands at least");
	Ch.Candidates.push_back("lv:sunday@40:3");
	Expect(video::CanUpload(Ch, C.Open).empty(), "rules: a final table and two of its hands make a Recap");
	Ch.Candidates = {"lv:sunday@40:2", "lv:sunday@40:3", "lv:sunday@40:4"};
	Expect(!video::CanUpload(Ch, C.Open).empty(), "rules: a Recap needs the run itself");
	Ch.F = video::Format::Breakdown;
	Ch.Candidates = {"lv:sunday@40:2", "lv:sunday@40:3"};
	Expect(!video::CanUpload(Ch, C.Open).empty(), "rules: a Breakdown is one hand");
	Ch.Candidates = {"gone"};
	Expect(!video::CanUpload(Ch, C.Open).empty(), "rules: a moment that's gone can't go up");
	// The recap's title and frame read the run.
	Ch.F = video::Format::Recap;
	Ch.T = video::TitleStyle::Plain;
	Ch.Fr = video::Frame::Trophy;
	Ch.Candidates = {"lv:sunday@40:recap", "lv:sunday@40:2", "lv:sunday@40:3"};
	const std::string Title = video::Title(Ch, C.Open, 0);
	const video::Thumb T = video::ThumbFor(Ch, C.Open);
	Expect(Title.find("4th") != std::string::npos && T.BigText.find("4TH") != std::string::npos, "rules: a Recap's title and trophy carry the place (" + Title + ", " + T.BigText + ")");
	// "{Count} hands" counts the two hands, not the run that leads the Recap.
	bool Counted = true;
	for (int S = 0; S < 5; ++S)
	{
		for (uint32_t Seed = 1; Seed <= 30; ++Seed)
		{
			video::Choice Any = Ch;
			Any.T = static_cast<video::TitleStyle>(S);
			const std::string X = Lower(video::Title(Any, C.Open, Seed));
			Counted = Counted && X.find("3 hands") == std::string::npos && X.find("3 spots") == std::string::npos;
		}
	}
	Expect(Counted, "rules: a Recap of a final table and two hands says two hands");
	// Advice never promises a number.
	for (const std::string& A : video::Advice(Ch, C.Open))
	{
		bool Digit = false;
		for (const char X : A)
		{
			Digit = Digit || (X >= '0' && X <= '9');
		}
		Expect(!Digit, "rules: Studio's advice promises no number (" + A + ")");
	}
	// Upload uses them up, and the video keeps its own copy of what it needs.
	video::UploadFacts F;
	const std::string Id = video::Upload(C, Ch, 41.0 * Day, "rules", F);
	Expect(!Id.empty() && C.Open.size() == 8 && C.Videos.size() == 1 && C.Videos[0].K == video::Kind::FinalTable && C.Videos[0].F == video::Format::Recap,
		"rules: the Recap goes up, its moments used");
	Expect(video::Upload(C, Ch, 41.0 * Day, "rules", F).empty() && C.Videos.size() == 1, "rules: and can't go up twice");
}

void CoolerOutcome()
{
	// C2's verify: a cooler is two big hands, and the player wins half of them (set over set, either way). The titles that
	// tell a loss ("nothing I could do", "sent my night sideways") are the lost one's alone; the won one gets the rest.
	int WonSaidLoss = 0, LostSaidLoss = 0, WonTitles = 0;
	for (const Chips Net : {Chips(6400), Chips(-6400)})
	{
		video::Channel C;
		video::Candidate X = Moment("rl:cooler@9:" + std::to_string(Net > 0 ? 1 : 2), 9.0 * Day + 20.0 * 60.0);
		X.Rec.Mine = ss::ParseCards(Net > 0 ? "AhAc" : "KdKs");
		X.Rec.Shown = {{"Mike_R", ss::ParseCards(Net > 0 ? "KdKs" : "AhAc")}};
		X.Rec.Board = ss::ParseCards("Kh7dAs9s4h");
		X.Rec.HeroNet = Net;
		X.Opponent = "Mike_R";
		video::NoteHand(C, X);
		for (int T = 0; T < 5; ++T)
		{
			for (uint32_t Seed = 0; Seed < 48; ++Seed)
			{
				video::Choice Ch;
				Ch.Candidates = {C.Open[0].Id};
				Ch.T = static_cast<video::TitleStyle>(T);
				const std::string Title = video::Title(Ch, C.Open, Seed);
				const bool Loss = Title.find("nothing I could do") != std::string::npos || Title.find("sideways") != std::string::npos ||
					Title.find("RAN INTO") != std::string::npos || Title.find("always going to lose") != std::string::npos;
				WonSaidLoss += Net > 0 && Loss ? 1 : 0;
				LostSaidLoss += Net < 0 && Loss ? 1 : 0;
				WonTitles += Net > 0 && !Title.empty() ? 1 : 0;
				if (Net > 0 && Loss)
				{
					std::printf("  a won cooler: \"%s\"\n", Title.c_str());
				}
			}
		}
	}
	Expect(WonTitles == 5 * 48 && WonSaidLoss == 0, "cooler: a cooler the player won never gets a title that tells a loss");
	Expect(LostSaidLoss > 0, "cooler: a lost one can");
}
} // namespace video_test
