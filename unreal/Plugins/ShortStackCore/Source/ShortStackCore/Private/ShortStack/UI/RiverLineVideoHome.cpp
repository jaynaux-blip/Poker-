// Update 3 (SPEC §6.5, §6.6.6, §7.1): Rerun 2.0's Home (the feed with RECOMMENDED) and Next up, ui-rerun's file. Shares the
// look and side state through RiverLineVideoShared.h.
//
// C3 (ui-rerun): HOME (page 3) is the feed: the chips All · Bad beats · Strategy · Live · Vlogs · Challenges and the sites the
// player has joined (RiverLine, Ignite, RunItTwice); a 4-column grid of 375 x 211 tiles of the world's creators' videos
// (video::WorldVideos: the Kast directory's creators and Overlay Weekly, from the world log and the calendar) and yours.
// Your video is on Home only while Rerun tests it there (a Home-tagged part in its fast phase: Realtime's OnHomeNow) or a
// breakout or a narrated viral roll is two days old or less (SPEC_CREATORS R1), tagged RECOMMENDED: the recommender made
// visible. Public numbers only: views and ages, never dislikes. Thumbnails paint at most two new ones a frame (the
// skeleton until their turn); a creator's face is theirs (hero::CharacterFor).
#include "RiverLineVideoShared.h"
#include "../StrictFloat.h"
#include "../Game/VideoAlgoPriv.h"

#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/ThumbArt.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlvideo_detail
{
std::vector<std::string> RrHomeInventory(bool Sentences)
{
	if (Sentences)
	{
		return {RrHomeEmpty};
	}
	return {"RECOMMENDED", "All", "Bad beats", "Strategy", "Live", "Vlogs", "Challenges", "MissFinch \xC2\xB7 12K views \xC2\xB7 3 days ago"};
}

namespace
{
/** A title's words in lower case (the chips match the world's videos by what their titles say). */
std::string RrLowerTitle(const std::string& S)
{
	std::string Out = S;
	for (char& C : Out)
	{
		C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
	}
	return Out;
}

bool RrSays(const std::string& Lower, std::initializer_list<const char*> Words)
{
	for (const char* W : Words)
	{
		if (Lower.find(W) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** Whether a title belongs under a chip (0 All .. 5 Challenges, then a site's name). */
bool RrChipFits(int Chip, const std::string& Title, const std::string& Site)
{
	const std::string L = RrLowerTitle(Title);
	switch (Chip)
	{
	case 0: return true;
	case 1: return RrSays(L, {"beat", "river", "cooler", "suckout", "82%", "81%", "lost"});
	case 2: return RrSays(L, {"icm", "lesson", "how ", "why ", "strategy", "explained", "fold", "push"});
	case 3: return RrSays(L, {"embercrest", "live", "card room"});
	case 4: return RrSays(L, {"vlog", "night", "my day", "sunday", "session"});
	case 5: return RrSays(L, {"challenge", " to $", "day "});
	default: return !Site.empty() && L.find(RrLowerTitle(Site)) != std::string::npos;
	}
}

bool RrMineFits(int Chip, const video::Video& V, const std::string& Site)
{
	switch (Chip)
	{
	case 0: return true;
	case 1: return video::valgo::KindGroup(V.K) == 0;
	case 2: return V.T == video::TitleStyle::Teaching || V.C == video::Class::Strategy;
	case 3: return V.Th.Mood >= 10; // a live hand's portrait frame (Video.h: 10 + mood for the live room)
	case 4: return V.F == video::Format::Vlog;
	case 5: return V.X.On && V.X.Series >= 0;
	default: return RrChipFits(Chip, V.Title, Site);
	}
}
} // namespace

/** Who posted it: initials on the creator's colour (yours on marigold), never a glyph and never a second portrait (a feed of
 *  twelve tiles keeps its vertices for the thumbnails). */
void RrWho(RrFrame& F, float Cx, float Cy, float R, const std::string& Name, bool Mine)
{
	uint32_t Col = Mine ? videoart::Marigold : 0x5a4a30;
	for (const kast::Streamer& K : kast::Directory())
	{
		Col = K.Name == Name ? K.Color : Col;
	}
	F.C.FillCircle(Cx, Cy, R, Mine ? Hex(Col) : Mix(Hex(Col), RrGround, 0.25f));
	std::string Initials;
	for (const char C : Name)
	{
		if (std::isalpha(static_cast<unsigned char>(C)) && Initials.size() < 2 && (Initials.empty() || std::isupper(static_cast<unsigned char>(C))))
		{
			Initials.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(C))));
		}
	}
	F.UI.Text(Initials, Cx, Cy + 0.5f, Ts(R * 0.8f, 900, Mine ? Hex(videoart::Ink) : RrCream, Align::Center, Baseline::Middle));
}

// ------------------------------------------------------------------ ss.Video.Sheet's contact sheet (VT1)

namespace
{
const char* const RrSheetGroups[7] = {"beats", "reads", "pots", "runs", "rival", "stories", "ideas"};
} // namespace

bool VideoOpenSheet(RiverLine& RL, const std::string& Group, int N, double Now)
{
	int G = -1;
	for (int K = 0; K < 7; ++K)
	{
		G = Group == RrSheetGroups[K] ? K : G;
	}
	if (G < 0)
	{
		return false;
	}
	VideoUi& U = VideoUiOf(&RL);
	U.Sheet = video::ContactSheet(G, std::clamp(N, 1, 24));
	U.SheetGroup = Group;
	VideoOpen(RL, 0, Now);
	U.Page = RrPageSheet;
	return true;
}

/** The sheet: up to 24 of the library's recipes for one kind group, 6 a row at 248 wide, each named (no two alike, VT1). */
void RrSheet(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	F.UI.Text("THUMBNAILS", 20.0f, 98.0f, Ts(20.0f, 900, RrCream));
	F.UI.Text(std::to_string(U.Sheet.size()) + " for " + U.SheetGroup + ", no two alike", 176.0f, 97.0f, Ts(13.0f, 600, RrMuted));
	if (U.Sheet.empty())
	{
		F.UI.Text(RrHomeEmpty, 20.0f, 160.0f, Ts(14.0f, 600, RrMuted));
		return;
	}
	for (size_t I = 0; I < U.Sheet.size() && I < 24; ++I)
	{
		const float X = 26.0f + Nf(I % 6) * 260.0f;
		const float Y = 122.0f + Nf(I / 6) * 207.0f;
		const Rect Th{X, Y, 248.0f, 139.5f};
		F.C.FillRoundRect(Th, 8.0f, RrGround);
		RrSpecThumb(F, Th, U.Sheet[I], video::Thumb());
		F.C.StrokeRoundRect(Th, 8.0f, Hex(0xffffff, 0.08f), 1.0f);
		const char* Name = video::ArchName(U.Sheet[I].Arch);
		F.UI.Text(Name ? std::string(Name) : std::string(), X + 2.0f, Y + 160.0f, Ts(12.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 244.0f));
		F.UI.Text(U.Sheet[I].Caption, X + 2.0f, Y + 178.0f, Ts(11.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 244.0f));
	}
}

bool RrHomeTested(const video::Video& V, double World)
{
	// VideoStats.cpp's Realtime: a part (the first wave at the upload, then each component) whose source mix sends viewers
	// from Home, younger than twice the class's tau1, with views to come.
	if (!V.X.On)
	{
		return false;
	}
	const video::valgo::VaShape HomeShape = video::valgo::ShapeU3(V.C);
	const double HomeFast = 2.0 * HomeShape.Tau1 * net::MinutesPerDay;
	auto HomeHot = [&V, World, HomeFast](double T, double P, uint8_t Mix) {
		return P > 0.0 && World >= T && World - T < HomeFast && video::valgo::MixOf(V, Mix)[0] > 0.0;
	};
	if (HomeHot(V.At, V.X.P0, 0))
	{
		return true;
	}
	for (const video::Comp& K : V.X.Comps)
	{
		if (HomeHot(K.T, K.P, K.SrcMix))
		{
			return true;
		}
	}
	return false;
}

/** Whether one of your videos is on Home right now (RECOMMENDED). */
bool RrOnHome(RrFrame& F, const video::Video& V)
{
	if (!V.X.On || RrIsGrant(V))
	{
		return false;
	}
	if (RrHomeTested(V, F.World))
	{
		return true;
	}
	const double Verdict = static_cast<double>(video::valgo::VerdictDayOf(V)) * net::MinutesPerDay;
	if (V.X.Viral && V.X.Verdict >= 0 && F.World >= Verdict && F.World < Verdict + 2.0 * net::MinutesPerDay)
	{
		return true;
	}
	for (const video::Comp& K : V.X.Comps)
	{
		if (K.Kind == video::CompKind::Breakout && F.World >= K.T && F.World < K.T + 2.0 * net::MinutesPerDay)
		{
			return true;
		}
	}
	return false;
}

void RrHome(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	RrDaily(F);
	// The chips: the topics, then the sites you've joined (§6.6.6).
	std::vector<std::string> Chips = {"All", "Bad beats", "Strategy", "Live", "Vlogs", "Challenges"};
	std::vector<std::string> Sites;
	for (const net::SiteId Site : rlsites_detail::SitesPlayed(F.S))
	{
		Sites.push_back(net::Def(Site).Name);
		Chips.push_back(net::Def(Site).Name);
	}
	U.HomeChip = std::clamp(U.HomeChip, 0, static_cast<int>(Chips.size()) - 1);
	float X = 20.0f;
	for (size_t K = 0; K < Chips.size(); ++K)
	{
		const float W = F.UI.Measure(Chips[K], 13.0f, 800) + 28.0f;
		const Rect R{X, 74.0f, W, 32.0f};
		const bool On = U.HomeChip == static_cast<int>(K);
		const Ui::ClickState St = F.UI.Clickable("rrhomechip" + std::to_string(K), R, F.Live);
		F.C.FillRoundRect(R, 16.0f, On ? RrCream : St.Hover ? RrPanel2 : RrPanel);
		F.UI.Text(Chips[K], R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, Ts(13.0f, 800, On ? Hex(videoart::Ink) : RrCream, Align::Center, Baseline::Middle));
		if (St.Clicked)
		{
			U.HomeChip = static_cast<int>(K);
			U.HomeGoal = U.HomeScroll = 0.0f;
		}
		X += W + 8.0f;
	}
	const std::string Site = U.HomeChip >= 6 && U.HomeChip - 6 < static_cast<int>(Sites.size()) ? Sites[static_cast<size_t>(U.HomeChip - 6)] : std::string();
	// The feed: yours while Rerun shows it on Home, then the world's creators' videos, newest first.
	struct Item
	{
		const video::Video* Mine = nullptr;
		const video::WorldVideo* World = nullptr;
		double At = 0.0;
	};
	std::vector<Item> Feed;
	for (const video::Video& V : F.S.Life.Video.Videos)
	{
		if (RrOnHome(F, V) && RrMineFits(U.HomeChip, V, Site))
		{
			Feed.push_back({&V, nullptr, V.At});
		}
	}
	const size_t Mine = Feed.size();
	for (const video::WorldVideo& W : U.WorldVids)
	{
		if (W.At <= F.World && RrChipFits(U.HomeChip, W.Title, Site))
		{
			Feed.push_back({nullptr, &W, W.At});
		}
	}
	std::stable_sort(Feed.begin() + static_cast<std::ptrdiff_t>(Mine), Feed.end(), [](const Item& A, const Item& B) { return A.At > B.At; });
	const Rect Area{20.0f, 122.0f, NetW - 40.0f, VideoH - 14.0f - 122.0f};
	if (Feed.empty())
	{
		F.UI.Text(RrHomeEmpty, Area.X + Area.W / 2.0f, Area.Y + 120.0f, Ts(16.0f, 700, RrMuted, Align::Center, Baseline::Alphabetic, false, Area.W - 80.0f));
		return;
	}
	const int Cols = 4;
	const float TileW = 375.0f, TileH = 211.0f, Gap = (Area.W - 4.0f * TileW) / 3.0f, Pitch = TileH + 86.0f;
	const int Rows = (static_cast<int>(Feed.size()) + Cols - 1) / Cols;
	const float Content = Nf(Rows) * Pitch;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.HomeGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * Pitch * 0.75f;
	}
	U.HomeGoal = std::clamp(U.HomeGoal, 0.0f, MaxScroll);
	U.HomeScroll += (U.HomeGoal - U.HomeScroll) * NetFollow(F.Dt, 18.0);
	U.HomeScroll = std::clamp(U.HomeScroll, 0.0f, MaxScroll);
	if (std::fabs(U.HomeGoal - U.HomeScroll) < 0.5f)
	{
		U.HomeScroll = U.HomeGoal;
	}
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < Feed.size(); ++I)
	{
		const float Tx = Area.X + Nf(static_cast<int>(I) % Cols) * (TileW + Gap);
		const float Ty = Area.Y + Nf(static_cast<int>(I) / Cols) * Pitch - U.HomeScroll;
		if (Ty + Pitch < Area.Y || Ty > Area.Y + Area.H)
		{
			continue;
		}
		const Rect Th{Tx, Ty, TileW, TileH};
		const Item& It = Feed[I];
		F.C.FillRoundRect(Th, 10.0f, RrGround);
		std::string Title, Who;
		double Views = 0.0, At = 0.0;
		if (It.Mine)
		{
			const video::Video& V = *It.Mine;
			const Ui::ClickState Cs = F.UI.Clickable("rrhomemine:" + V.Id, {Th.X, Th.Y, Th.W, Pitch - 12.0f}, F.Live && PointerIn);
			RrThumb(F, Th, V, RrGridLod);
			F.C.StrokeRoundRect(Th, 10.0f, Cs.Hover ? RrGold : NetA(RrGold, 0.6f), 2.0f);
			RrLengthBadge(F, Th, V);
			RrTag(F.C, "RECOMMENDED", Th.X + 10.0f, Th.Y + 10.0f, RrGold, true, 10.0f);
			Title = V.Title;
			Who = F.S.HeroName;
			Views = video::Views(V, F.World);
			At = V.At;
			if (Cs.Clicked)
			{
				RrOpenVideo(F, V.Id);
			}
		}
		else
		{
			const video::WorldVideo& W = *It.World;
			RrWorldThumb(F, Th, W, RrGridLod);
			F.C.StrokeRoundRect(Th, 10.0f, RrLine, 1.0f);
			Title = W.Title;
			Who = W.Creator;
			Views = W.Views;
			At = W.At;
		}
		RrWho(F, Th.X + 18.0f, Th.Y + TileH + 30.0f, 16.0f, Who, It.Mine != nullptr);
		NetParagraph(F.C, Title, Th.X + 44.0f, Th.Y + TileH + 26.0f, TileW - 50.0f, 14.0f, 700, RrCream, 18.0f, 2);
		F.UI.Text(Who + " \xC2\xB7 " + RrViews(Views) + " \xC2\xB7 " + RrAge(F.World - At), Th.X + 44.0f, Th.Y + TileH + 70.0f, Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, TileW - 50.0f));
	}
	F.C.PopClip();
	if (MaxScroll > 0.0f)
	{
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * (U.HomeScroll / MaxScroll);
		F.C.FillRoundRect({Area.X + Area.W + 8.0f, Ty, 4.0f, Th}, 2.0f, NetA(RrCream, 0.25f));
	}
}
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss
