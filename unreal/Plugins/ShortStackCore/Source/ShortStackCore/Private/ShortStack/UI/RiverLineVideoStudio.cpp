// Rerun's Studio (update 2 SPEC §4.5, §4.8; moved here unchanged from RiverLineVideo.cpp by update 3's split, SPEC §7.1,
// §8.2), ui-rerun's file: the candidates on the left, best first, each with its kind's glyph, what happened, when, its chips
// and the days it has left; the editor in the middle (the format, the title style with the real title, the thumbnail frame
// at 640 x 360, the edit's time and energy, PUBLISH through Session::Publish with the honest reason first); on the right,
// what each choice does (video::Advice) and the video's tile as the Channel will show it. Rerun 2.0's IDEAS, MOMENTS,
// SERIES, packaging, the Duel, the settings, Next-up cards and the launch card land here (G3). Shares the look and side
// state through RiverLineVideoShared.h.
//
// Rerun 2.0 (C3, ui-rerun; update 3 SPEC §6.6.1-§6.6.4, DESIGN §4.8.2-§4.8.4; SPEC_CREATORS §5.3 R1-R3, R5):
//   THE LIST (20, 72, 360, 878): MOMENTS · IDEAS · SERIES. Moments are the candidates (Update 2's rows); ideas are the
//     library's cards (Session::VideoIdeas: the fact line, formats, the edit's cost, TRENDING, LTD, the strike flag,
//     COOLDOWN, LOCKED with its hint and progress), ready first, trend matches on top, locked last; choosing one makes its
//     candidate (Session::ChooseIdea). Series are the channel's challenges and playlists.
//   THE EDITOR (396, 72, 764, 878): FORMAT (Cut · Breakdown · Vlog · Recap), LENGTH (Tight · Standard · Extended with
//     their real lengths), TITLE (the style and three options, Session::TitleOptions, with SHUFFLE), THUMBNAIL (a 504-wide
//     preview and six options, Session::ThumbOptions in the library's hash order, SHUFFLE, DUEL of two or three). Before
//     the library deals (no options), the editor offers the Update 2 title and frames: the choice is Update 2's.
//   THE SIDE (1176, 72, 404, 878): the packaging read (video::Advice, never a number), the carry while Momentum runs over
//     1.1 (R1), SETTINGS (ads, the suitability coin and Clean edit, a Next-up card, the series), when it goes up against the
//     audience's peak, the edit's real cost (+15 minutes a Duel contender and for a clean edit), PUBLISH (220 x 56).
//   THE LAUNCH CARD (48 world hours after a drop): views against the typical upload at the same age, CTR so far, likes and
//     dislikes, the ranking, impressions, "would have earned ... (not paid)" before Partner, the carry and Live Drop's
//     lines, views an hour against the dashed typical, the Duel's running CTRs and watch-time shares, the four launch
//     actions with their states, the latest comments. After the verdict, a day's one-line result at the top.
#include "RiverLineVideoShared.h"
#include "../StrictFloat.h"
#include "../Game/VideoAlgoPriv.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/ChartArt.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlvideo_detail
{
// ------------------------------------------------------------------ Studio's own copy

constexpr const char* RrNewVideo = "NEW VIDEO";
constexpr const char* RrBackToLaunch = "BACK TO THE LAUNCH";
constexpr const char* RrVerdictSoon = "The verdict comes at midnight";
constexpr const char* RrNoComments = "Comments arrive with the views.";
constexpr const char* RrPickIdea = "Pick a ready idea: Studio cuts it from your own career.";
constexpr const char* RrCleanEdit = "Clean edit: full ads, 15 more minutes";
constexpr const char* RrLimitedWhy = "Limited ads"; // the reason is the packaging read's (video::CoinReason)
constexpr const char* RrLiveNow = "You're live: keep going for 30 minutes.";

std::vector<std::string> RrStudioInventory(bool Sentences)
{
	if (Sentences)
	{
		return {RrNoComments, RrPickIdea, RrLiveNow, "Studio cuts it from your career.", "Ads on this video: Heavy, from the next midnight."};
	}
	return {RrNewVideo, RrBackToLaunch, RrVerdictSoon, RrCleanEdit, RrLimitedWhy, "Goes up 9:40 PM, when the edit ends", "Your viewers peak around 11 PM", "Cuts share the feed's ads",
		"too late for a push", "go live within 4 h", "Shown in feed", "Stayed to watch", "MOMENTS 12", "IDEAS 7", "SERIES 2", "TRENDING", "LTD", "LOCKED", "COOLDOWN 3 D", "IDEA",
		"STORYTIME", "AUTO", "LIVE", "CHALLENGE", "$50 to $500", "4 episodes", "FORMAT", "LENGTH", "Tight  6:00", "Standard  8:00", "Extended  12:00", "45 s, fixed", "TITLE",
		"SHUFFLE", "THUMBNAIL", "DUEL", "DUEL: A + B (+15 min)", "PACKAGING READ", "SETTINGS", "Ads", "Light", "Standard", "Heavy", "Coin", "Full ads", "Feed ads",
		"Next-up card", "Series", "none", "Edit 1 h 45 m (+Duel) \xC2\xB7 9 energy", "PUBLISH", "PUBLISH ANYWAY", "LAUNCH", "VIEWS", "typical 290 at 17 h", "CTR SO FAR",
		"typical 4.6%", "AVD", "LIKES \xC2\xB7 DISLIKES", "RANKING", "2 OF 10", "your last 10 uploads at 17 h", "IMPRESSIONS", "SHOWN IN FEED", "STAYED TO WATCH",
		"reported", "EST. REVENUE", "WOULD HAVE EARNED", "not paid", "SUBSCRIBERS", "31 lost", "VIEWS AN HOUR SINCE THE DROP", "dashed: your typical upload", "drop",
		"24 h", "48 h", "hour 17", "Typical", "THUMBNAIL DUEL", "decided on watch time", "so far", "WON", "of watch time", "LAUNCH ACTIONS", "Make a Cut  20 min",
		"Go live on Kast", "Post a teaser  10 min", "Reply round  20 min", "done", "not yet", "LATEST COMMENTS", "3 comments", "1 moment \xC2\xB7 Breakdown 8:00",
		"The verdict comes at midnight: 31 h",
		// C3 fix round: the pickers (M1) and the IDEAS list's NEW chip (L9).
		"CAPTION", "FACE", "PALETTE", "Neutral", "Smile", "Grin", "Laugh", "Shocked", "Scream", "Devastated", "Tilted", "Smug", "Skeptical", "Thinking", "Exhausted", "Scarlet",
		"Felt", "Money", "Ice", "Neon", "Gold on black", "Sunrise", "Night", "Mono", "Blood", "Site colours", "Outlined", "Boxed", "Two-tone", "Marker", "Stamp",
		"Number burst", "Lower third", "No caption", "Boxed \xE2\x80\xBA", "81% AND LOST", "NEW"};
}

// ------------------------------------------------------------------ the list: MOMENTS (Update 2's rows)

void RrCandidateRow(RrFrame& F, const video::Candidate& Cd, const std::vector<video::Candidate>& Open, const Rect& R, bool Clickable)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const auto At = std::find(U.Pick.begin(), U.Pick.end(), Cd.Id);
	const bool Picked = At != U.Pick.end();
	const std::string Hint = Picked ? std::string() : RrJoinHint(U, Open, Cd);
	const Ui::ClickState St = F.UI.Clickable("rrcand:" + Cd.Id, R, F.Live && Clickable);
	if (St.Clicked)
	{
		RrToggle(U, Open, Cd, F.Now);
	}
	const float A0 = F.C.GetAlpha();
	if (!Hint.empty())
	{
		F.C.SetAlpha(A0 * 0.5f);
	}
	F.C.FillRoundRect(R, 12.0f, Picked ? Mix(RrPanel, RrGold, 0.12f) : St.Hover ? RrPanel2 : RrPanel);
	F.C.StrokeRoundRect(R, 12.0f, Picked ? RrGold : RrLine, Picked ? 2.0f : 1.0f);
	const Rect Glyph{R.X + 12.0f, R.Y + 17.0f, 52.0f, 52.0f};
	F.C.FillRoundRect(Glyph, 10.0f, RrGround);
	videoart::KindGlyph(F.C, Glyph.X + 26.0f, Glyph.Y + 26.0f, 30.0f, Cd.K);
	if (Picked)
	{
		// The order it goes in, for a night's Vlog or a Recap; a check for a single moment.
		F.C.FillCircle(Glyph.X + Glyph.W - 2.0f, Glyph.Y + 2.0f, 10.0f, RrGold);
		if (U.F == video::Format::Vlog || U.F == video::Format::Recap)
		{
			F.UI.Text(std::to_string(At - U.Pick.begin() + 1), Glyph.X + Glyph.W - 2.0f, Glyph.Y + 2.5f, Ts(11.0f, 900, Hex(videoart::Ink), Align::Center, Baseline::Middle));
		}
		else
		{
			NetCheck(F.C, Glyph.X + Glyph.W - 2.0f, Glyph.Y + 2.0f, 11.0f, Hex(videoart::Ink));
		}
	}
	const float X0 = R.X + 78.0f;
	const float Right = R.X + R.W - 14.0f;
	// Line 1: the kind, the chips, the days left.
	float X = X0 + F.UI.Text(video::KindLabel(Cd.K), X0, R.Y + 28.0f, Ts(13.0f, 900, RrGold)) + 10.0f;
	const std::string Left = Hint.empty() ? RrDaysLeft(Cd.Expires, F.World) : Hint;
	const float LeftW = F.UI.Text(Left, Right, R.Y + 28.0f, Ts(12.0f, 700, !Hint.empty() ? RrMuted : Cd.Expires - F.World < 2.0 * net::MinutesPerDay ? RrWarn : RrMuted, Align::Right));
	std::vector<std::pair<std::string, Color>> Tags;
	if (Cd.Auto)
	{
		Tags.push_back({"AUTO", RrSky}); // to the player only: the video never says it (U1)
	}
	if (Cd.Storytime)
	{
		Tags.push_back({"STORYTIME", RrCash});
	}
	if (Cd.Source == 5)
	{
		Tags.push_back({"IDEA", RrGold});
	}
	const std::string Where = RrWhereChip(Cd);
	if (!Where.empty())
	{
		Tags.push_back({Where, RrMuted});
	}
	// The chips after the kind; those the line can't hold go after the date on the third line (AUTO first: it's the player's
	// only sign the plan played, §1.10, so it never drops off).
	const std::string When = RrWhen(Cd.At);
	float X3 = X0 + F.UI.Measure(When, 12.5f, 600) + 10.0f;
	for (const auto& Ch : Tags)
	{
		const float W = F.UI.Measure(Ch.first, 9.5f, 700) + 9.5f * 1.3f;
		if (X + W <= Right - LeftW - 8.0f)
		{
			X += NetPill(F.C, Ch.first, X, R.Y + 15.0f, Ch.second, false, 9.5f) + 5.0f;
		}
		else if (X3 + W <= Right)
		{
			X3 += NetPill(F.C, Ch.first, X3, R.Y + 59.0f, Ch.second, false, 9.5f) + 5.0f;
		}
	}
	// Line 2: what happened (a long event name gives way to the pot and who). Line 3: when.
	std::string Tail;
	std::string Head;
	if (Cd.Source == 5)
	{
		video::Choice One;
		One.Candidates = {Cd.Id};
		One.F = video::Format::Breakdown;
		One.T = video::TitleStyle::Plain;
		Head = video::Title(One, Open, 0);
	}
	else
	{
		Head = RrFact(Cd, Open, Tail);
	}
	if (!Tail.empty())
	{
		Head = RrShorten(F.C, Head, 15.0f, 700, std::max(80.0f, Right - X0 - F.UI.Measure(Tail, 15.0f, 700)));
	}
	F.UI.Text(Head + Tail, X0, R.Y + 51.0f, Ts(15.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Right - X0));
	F.UI.Text(When, X0, R.Y + 71.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Right - X0));
	F.C.SetAlpha(A0);
}

/** The list's scroll (one for whichever tab shows), eased: the content's height in, the offset out. */
float RrListScroll(RrFrame& F, const Rect& Area, float Content, float Step)
{
	VideoUi& U = F.U;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.ListGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * Step;
	}
	U.ListGoal = std::clamp(U.ListGoal, 0.0f, MaxScroll);
	U.ListScroll += (U.ListGoal - U.ListScroll) * rlnet_detail::NetFollow(F.Dt, 18.0);
	U.ListScroll = std::clamp(U.ListScroll, 0.0f, MaxScroll);
	if (std::fabs(U.ListGoal - U.ListScroll) < 0.5f)
	{
		U.ListScroll = U.ListGoal;
	}
	if (MaxScroll > 0.0f)
	{
		// A thin thumb on the right edge, so the player knows there's more.
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * (U.ListScroll / MaxScroll);
		F.C.FillRoundRect({Area.X + Area.W + 6.0f, Ty, 4.0f, Th}, 2.0f, rlnet_detail::NetA(RrCream, 0.25f));
	}
	return U.ListScroll;
}

void RrCandidates(RrFrame& F, const std::vector<video::Candidate>& Listed, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const float Right = RrListX + RrListW;
	float Top = RrListTop;
	if (U.Tonight)
	{
		// Kast's Cut highlights: tonight's stream only, until "Show all".
		NetPill(F.C, "TONIGHT'S STREAM", RrListX, Top, RrGold, false, 10.0f);
		if (RrLink(F, "rrshowall", "Show all", Right, Top + 14.0f, 12.0f, RrGold, Align::Right))
		{
			U.Tonight = false;
			U.ListGoal = U.ListScroll = 0.0f;
		}
		Top += 30.0f;
	}
	const Rect Area{RrListX, Top, RrListW, VideoH - 20.0f - Top};
	if (Listed.empty())
	{
		F.UI.Text(U.Tonight ? RrTonightGone : RrStudioEmpty, Area.X + Area.W / 2.0f, Area.Y + 60.0f, Ts(15.0f, 700, RrMuted, Align::Center, Baseline::Alphabetic, false, Area.W - 20.0f));
		return;
	}
	const float Content = Nf(Listed.size()) * (RrRowH + RrRowGap) - RrRowGap;
	const float Scroll = RrListScroll(F, Area, Content, (RrRowH + RrRowGap) * 1.5f);
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < Listed.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * (RrRowH + RrRowGap) - Scroll;
		if (Y + RrRowH < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		RrCandidateRow(F, Listed[I], Open, {Area.X, Y, Area.W, RrRowH}, PointerIn);
	}
	F.C.PopClip();
}

// ------------------------------------------------------------------ the list: IDEAS and SERIES

/** An idea's group glyph (§6.3's groups A..P, by the library's group index). */
void RrIdeaGlyph(Canvas& Cv, float Cx, float Cy, uint8_t Group)
{
	switch (Group)
	{
	case 0: videoart::KindGlyph(Cv, Cx, Cy, 26.0f, video::Kind::BadBeat); return;
	case 2: videoart::KindGlyph(Cv, Cx, Cy, 26.0f, video::Kind::DeepRun); return;
	case 4: chart::DrawIcon(Cv, chart::Icon::Coin, Cx, Cy, 24.0f, RrCash); return;
	case 5: chart::DrawIcon(Cv, chart::Icon::Magnifier, Cx, Cy, 24.0f, RrSky); return;
	case 6: chart::DrawIcon(Cv, chart::Icon::Share, Cx, Cy, 24.0f, RrGold); return;
	case 8: chart::DrawIcon(Cv, chart::Icon::Comment, Cx, Cy, 24.0f, RrGood); return;
	case 11: chart::DrawIcon(Cv, chart::Icon::Eye, Cx, Cy, 24.0f, RrWarn); return;
	case 1:
	case 10:
	case 15: chart::DrawIcon(Cv, chart::Icon::Clock, Cx, Cy, 24.0f, RrGold); return;
	default: chart::DrawIcon(Cv, chart::Icon::Globe, Cx, Cy, 24.0f, RrGold); return;
	}
}


void RrIdeaCard(RrFrame& F, const video::IdeaCard& I, const Rect& R, bool Clickable)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const Ui::ClickState St = F.UI.Clickable("rridea:" + std::to_string(I.Id), R, F.Live && Clickable && I.Ready);
	const float A0 = F.C.GetAlpha();
	if (!I.Ready)
	{
		F.C.SetAlpha(A0 * 0.62f);
	}
	F.C.FillRoundRect(R, 12.0f, St.Hover ? RrPanel2 : RrPanel);
	F.C.StrokeRoundRect(R, 12.0f, I.Trending && I.Ready ? NetA(RrGold, 0.7f) : RrLine, 1.0f);
	const Rect Glyph{R.X + 12.0f, R.Y + 14.0f, 44.0f, 44.0f};
	F.C.FillRoundRect(Glyph, 10.0f, RrGround);
	if (I.Ready)
	{
		RrIdeaGlyph(F.C, Glyph.X + 22.0f, Glyph.Y + 22.0f, I.Group);
	}
	else
	{
		NetLockIcon(F.C, Glyph.X + 13.0f, Glyph.Y + 11.0f, 18.0f, RrMuted);
	}
	const float X0 = R.X + 68.0f;
	const float Right = R.X + R.W - 12.0f;
	// The chips on the right: TRENDING, LTD, the flag, COOLDOWN, LOCKED.
	float Cx = Right;
	auto Chip = [&](const std::string& Label, const Color& Col, bool Solid) {
		const float W = F.C.Measure(Label, 9.0f, 900) + 16.0f;
		Cx -= W;
		RrTag(F.C, Label, Cx, R.Y + 12.0f, Col, Solid, 9.0f);
		Cx -= 5.0f;
	};
	if (!I.Ready)
	{
		Chip("LOCKED", RrMuted, false);
	}
	if (I.CooldownDays > 0)
	{
		Chip("COOLDOWN " + std::to_string(I.CooldownDays) + " D", RrWarn, false);
	}
	if (I.Strike)
	{
		Cx -= 14.0f;
		RrFlag(F.C, Cx + 2.0f, R.Y + 14.0f, RrWarn);
		Cx -= 4.0f;
	}
	if (I.Limited)
	{
		Chip("LTD", RrCash, false);
	}
	if (I.Trending)
	{
		Chip("TRENDING", RrGold, true);
	}
	if (I.Ready && U.NewIdeas.count(I.Id) != 0)
	{
		Chip("NEW", RrGood, false); // ready since the last visit (L9)
	}
	F.UI.Text(I.Name, X0, R.Y + 28.0f, Ts(14.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, std::max(60.0f, Cx - X0 - 4.0f)));
	F.UI.Text(I.Fact, X0, R.Y + 48.0f, Ts(12.5f, 800, I.Ready ? RrGold : RrMuted, Align::Left, Baseline::Alphabetic, false, Right - X0));
	if (I.Ready)
	{
		std::string Formats;
		for (const video::Format Fm : I.Formats)
		{
			Formats += (Formats.empty() ? std::string() : std::string(" \xC2\xB7 ")) + RrFormatName(Fm);
		}
		const std::string Cost = (I.EditMin > 0 ? RrEditTime(I.EditMin) : std::string()) + (I.Energy > 0 ? " \xC2\xB7 " + std::to_string(I.Energy) + " energy" : std::string());
		F.UI.Text(Formats + (Formats.empty() || Cost.empty() ? std::string() : std::string(" \xC2\xB7 ")) + Cost, X0, R.Y + 68.0f,
			Ts(11.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Right - X0));
	}
	else
	{
		F.UI.Text(I.Hint, X0, R.Y + 68.0f, Ts(11.5f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, Right - X0));
		if (I.Need > 0)
		{
			const Rect Track{X0, R.Y + 76.0f, Right - X0, 4.0f};
			F.C.FillRoundRect(Track, 2.0f, RrPanel2);
			const float Frac = Nf(std::clamp(static_cast<double>(I.Have) / static_cast<double>(I.Need), 0.0, 1.0));
			if (Frac > 0.0f)
			{
				F.C.FillRoundRect({Track.X, Track.Y, std::max(4.0f, Track.W * Frac), Track.H}, 2.0f, RrGold);
			}
		}
	}
	F.C.SetAlpha(A0);
	if (St.Clicked)
	{
		RrChooseIdea(F, I);
	}
}

bool RrChooseIdea(RrFrame& F, const video::IdeaCard& I)
{
	VideoUi& U = F.U;
	// Choosing a ready idea makes its candidate (Source 5, the idea's facts); Studio picks it.
	const std::string Why = F.S.ChooseIdea(I.Id);
	if (!Why.empty())
	{
		U.Note = Why;
		U.NoteBad = true;
		U.NoteAt = F.Now;
		return false;
	}
	const video::Candidate* Made = nullptr;
	for (const video::Candidate& C : F.S.Life.Video.Open)
	{
		if (C.Source == 5 && C.Idea == I.Id && (!Made || C.At >= Made->At))
		{
			Made = &C;
		}
	}
	if (Made)
	{
		U.Pick = {Made->Id};
		U.Touched = true;
		if (!I.Formats.empty())
		{
			RrSetFormat(U, F.S.Life.Video.Open, I.Formats.front());
		}
	}
	return Made != nullptr;
}

void RrIdeasList(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	std::vector<video::IdeaCard> Cards = RrIdeas(F);
	// Ready first, trend matches on top, locked last (§6.6.2); the library's order inside each.
	std::stable_sort(Cards.begin(), Cards.end(), [](const video::IdeaCard& A, const video::IdeaCard& B) {
		const int Ra = A.Ready ? (A.Trending ? 0 : 1) : 2, Rb = B.Ready ? (B.Trending ? 0 : 1) : 2;
		return Ra < Rb;
	});
	for (const video::IdeaCard& I : Cards)
	{
		if (I.Trending && I.Ready)
		{
			U.SeenIdeas.insert(I.Id); // seen: the taskbar's dot stops counting it
		}
		if (I.Ready)
		{
			U.ReadySeen.insert(I.Id); // seen on the list: no longer new (its NEW chip stays for this visit)
		}
	}
	float Top = RrListTop;
	RrDaily(F);
	if (!U.Trends.empty())
	{
		// TRENDING: the hottest topic this week (Research has the list).
		NetSpaced(F.C, "TRENDING", RrListX + 2.0f, Top + 14.0f, 9.5f, 900, RrGold, 1.3f);
		NetTriangle(F.C, RrListX + 84.0f, Top + 10.0f, 7.0f, true, RrGold);
		F.UI.Text(U.Trends.front().Label, RrListX + 96.0f, Top + 14.0f, Ts(12.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, RrListW - 96.0f));
		Top += 30.0f;
	}
	const Rect Area{RrListX, Top, RrListW, VideoH - 20.0f - Top};
	if (Cards.empty())
	{
		NetParagraph(F.C, RrIdeasEmpty, Area.X + 10.0f, Area.Y + 50.0f, Area.W - 20.0f, 14.0f, 600, RrMuted, 20.0f, 4);
		return;
	}
	const float H = 92.0f, Gap = 8.0f;
	const float Scroll = RrListScroll(F, Area, Nf(Cards.size()) * (H + Gap) - Gap, (H + Gap) * 1.5f);
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < Cards.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * (H + Gap) - Scroll;
		if (Y + H < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		RrIdeaCard(F, Cards[I], {Area.X, Y, Area.W, H}, PointerIn);
	}
	F.C.PopClip();
}

void RrSeriesList(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::ChannelX& X = F.S.Life.Video.X;
	const Rect Area{RrListX, RrListTop, RrListW, VideoH - 20.0f - RrListTop};
	if (X.Challenges.empty() && X.Lists.empty())
	{
		NetParagraph(F.C, RrSeriesEmpty, Area.X + 10.0f, Area.Y + 50.0f, Area.W - 20.0f, 14.0f, 600, RrMuted, 20.0f, 4);
		return;
	}
	float Y = Area.Y;
	for (const video::Challenge& Cl : X.Challenges)
	{
		const Rect R{Area.X, Y, Area.W, 76.0f};
		const bool On = U.Series == Cl.Id;
		const Ui::ClickState St = F.UI.Clickable("rrseries:" + std::to_string(Cl.Id), R, F.Live);
		F.C.FillRoundRect(R, 12.0f, On ? Mix(RrPanel, RrGold, 0.12f) : St.Hover ? RrPanel2 : RrPanel);
		F.C.StrokeRoundRect(R, 12.0f, On ? RrGold : RrLine, On ? 2.0f : 1.0f);
		F.UI.Text("CHALLENGE", R.X + 16.0f, R.Y + 24.0f, Ts(11.0f, 900, RrGold));
		F.UI.Text(Money(Cl.Start) + " to " + Money(Cl.Goal), R.X + 16.0f, R.Y + 46.0f, Ts(15.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 32.0f));
		F.UI.Text(std::to_string(Cl.Episodes) + (Cl.Episodes == 1 ? " episode" : " episodes"), R.X + 16.0f, R.Y + 66.0f, Ts(12.0f, 600, RrMuted));
		if (St.Clicked)
		{
			U.Series = On ? -1 : Cl.Id;
		}
		Y += 84.0f;
	}
	for (const video::Playlist& P : X.Lists)
	{
		const Rect R{Area.X, Y, Area.W, 62.0f};
		F.C.FillRoundRect(R, 12.0f, RrPanel);
		F.C.StrokeRoundRect(R, 12.0f, RrLine, 1.0f);
		F.UI.Text(P.Name, R.X + 16.0f, R.Y + 28.0f, Ts(15.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 32.0f));
		F.UI.Text(std::to_string(P.Videos.size()) + (P.Videos.size() == 1 ? " video" : " videos"), R.X + 16.0f, R.Y + 48.0f, Ts(12.0f, 600, RrMuted));
		Y += 70.0f;
		if (Y > Area.Y + Area.H)
		{
			break;
		}
	}
}

/** MOMENTS · IDEAS · SERIES (§6.6.2), each with its count. */
void RrListTabs(RrFrame& F, const std::vector<video::Candidate>& Listed)
{
	VideoUi& U = F.U;
	int Ready = 0;
	for (const video::IdeaCard& I : RrIdeas(F))
	{
		Ready += I.Ready ? 1 : 0;
	}
	const video::ChannelX& X = F.S.Life.Video.X;
	const std::string Labels[3] = {"MOMENTS " + std::to_string(Listed.size()), "IDEAS " + std::to_string(Ready), "SERIES " + std::to_string(X.Challenges.size() + X.Lists.size())};
	const float W = (RrListW - 12.0f) / 3.0f;
	for (int K = 0; K < 3; ++K)
	{
		if (RrChip(F, "rrlist" + std::to_string(K), {RrListX + rlnet_detail::Nf(K) * (W + 6.0f), 74.0f, W, 32.0f}, Labels[K], U.ListTab == K, 12.5f) && U.ListTab != K)
		{
			U.ListTab = K;
			U.ListGoal = U.ListScroll = 0.0f;
		}
	}
}

// ------------------------------------------------------------------ the editor

std::string RrOptionKey(const VideoUi& U)
{
	std::string Key;
	for (const std::string& Id : U.Pick)
	{
		Key += Id + "|";
	}
	return Key + std::to_string(static_cast<int>(U.F)) + ":" + std::to_string(static_cast<int>(U.T)) + ":" + std::to_string(U.TitleShuffle) + ":" + std::to_string(U.ThumbShuffle);
}

/** The library's options for the choice (title templates and recipes), dealt again only when the choice's key moves. */
void RrOptions(RrFrame& F)
{
	VideoUi& U = F.U;
	const std::string Key = RrOptionKey(U);
	if (Key == U.OptKey)
	{
		return;
	}
	U.OptKey = Key;
	video::Choice Ch;
	Ch.Candidates = U.Pick;
	Ch.F = U.F;
	Ch.T = U.T;
	Ch.Len = U.Len;
	Ch.Ads = U.Ads;
	U.Titles = U.Pick.empty() ? std::vector<video::TitleOption>() : F.S.TitleOptions(Ch, U.TitleShuffle);
	if (U.Titles.size() > 3)
	{
		U.Titles.resize(3);
	}
	U.TitlePick = std::clamp(U.TitlePick, 0, std::max(0, static_cast<int>(U.Titles.size()) - 1));
	// The six recipes are dealt again only when their own deal moves (the pick, the format, the thumbnails' shuffle: what
	// ThumbOptions reads), so a title's style or shuffle keeps them and the pickers' edits on them (M1).
	std::string ThumbKey;
	for (const std::string& Id : U.Pick)
	{
		ThumbKey += Id + "|";
	}
	ThumbKey += std::to_string(static_cast<int>(U.F)) + ":" + std::to_string(U.ThumbShuffle);
	if (ThumbKey == U.ThumbKey)
	{
		return;
	}
	U.ThumbKey = ThumbKey;
	U.CapFrom = 0;
	U.Thumbs = U.Pick.empty() || U.F == video::Format::Short ? std::vector<video::ThumbSpec>() : F.S.ThumbOptions(Ch, U.ThumbShuffle);
	if (U.Thumbs.size() > 6)
	{
		U.Thumbs.resize(6);
	}
	U.ThumbPick = std::clamp(U.ThumbPick, 0, std::max(0, static_cast<int>(U.Thumbs.size()) - 1));
	U.Duel.erase(std::remove_if(U.Duel.begin(), U.Duel.end(), [&U](int K) { return K >= static_cast<int>(U.Thumbs.size()); }), U.Duel.end());
}

/** The edit's world minutes and energy for the choice: the length's (§4.5.5), +15 minutes and +1 energy a Duel contender
 *  past the first (§4.6.8), +15 minutes for a clean edit (§4.5.4). */
void RrEditCost(const VideoUi& U, const video::Choice& Ch, bool Strong, double& Minutes, double& Energy, int& Contenders)
{
	Contenders = static_cast<int>(Ch.Thumbs.size());
	Minutes = video::valgo::EditMinutesFor(U.F, Ch.Len) + 15.0 * std::max(0, Contenders - 1) + (Strong && U.CleanEdit ? video::valgo::VaCleanEditMinutes : 0.0);
	Energy = video::valgo::EditEnergyFor(U.F, Ch.Len) + static_cast<double>(std::max(0, Contenders - 1));
}

/** The choice as an upload would see it (Studio's suitability read: strong language is the core's own test). */
bool RrStrongLanguage(const video::Choice& Ch)
{
	if (Ch.Thumbs.empty())
	{
		return false;
	}
	video::Video Probe;
	Probe.T = Ch.T;
	Probe.Fr = video::Frame::Recipe;
	Probe.X.On = true;
	Probe.X.Duels = static_cast<int8_t>(std::min<size_t>(3, Ch.Thumbs.size()));
	for (size_t K = 0; K < Ch.Thumbs.size() && K < 3; ++K)
	{
		Probe.X.Spec[K] = Ch.Thumbs[K];
	}
	return video::valgo::StrongLanguage(Probe);
}

// ------------------------------------------------------------------ the editor: CAPTION, FACE and PALETTE (M1, §6.6.2)

// The pickers' words (§6.1's names for the faces, the palettes and the caption styles).
const char* const RrFaceNames[12] = {"Neutral", "Smile", "Grin", "Laugh", "Shocked", "Scream", "Devastated", "Tilted", "Smug", "Skeptical", "Thinking", "Exhausted"};
// The Heat palette shows as "Scarlet": C9 keeps heat and fire words off Rerun's pages (Kast's Heat, Ignite's Flame).
const char* const RrPalNames[11] = {"Scarlet", "Felt", "Money", "Ice", "Neon", "Gold on black", "Sunrise", "Night", "Mono", "Blood", "Site colours"};
const char* const RrCapStyleNames[8] = {"Outlined", "Boxed", "Two-tone", "Marker", "Stamp", "Number burst", "Lower third", "No caption"};

/** A picker's chip: RrSegment's look at the pickers' size, the picked one in marigold. */
bool RrPickChip(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, bool On)
{
	using namespace rlnet_detail;
	const Ui::ClickState St = F.UI.Clickable(Id, R, F.Live);
	F.C.FillRoundRect(R, 8.0f, On ? Mix(RrPanel2, RrGold, 0.16f) : St.Hover ? RrPanel2 : RrPanel);
	F.C.StrokeRoundRect(R, 8.0f, On ? RrGold : St.Hover ? NetA(RrCream, 0.3f) : RrLine, On ? 1.6f : 1.0f);
	F.UI.Text(Label, R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, Ts(12.5f, 800, On ? RrCream : NetA(RrCream, 0.82f), Align::Center, Baseline::Middle, false, R.W - 12.0f));
	return St.Clicked;
}

/** The caption a chip offers, as the thumbnail reads it ("81% AND LOST"). */
std::string RrCaptionLabel(const std::pair<std::string, std::string>& C)
{
	return C.second.empty() ? C.first : C.first + " " + C.second;
}

/** The selected recipe's allowed values (core-videocontent's ThumbChoicesOf), asked again only when the recipe or the choice
 *  moves (it reads the moment's facts). */
const video::ThumbChoices& RrPicks(RrFrame& F, const std::vector<video::Candidate>& Open)
{
	VideoUi& U = F.U;
	const size_t K = static_cast<size_t>(std::clamp(U.ThumbPick, 0, static_cast<int>(U.Thumbs.size()) - 1));
	const video::ThumbSpec& S = U.Thumbs[K];
	const std::string Key = RrOptionKey(U) + "#" + std::to_string(K) + "#" + std::to_string(S.Arch) + "#" + std::to_string(S.Variant);
	if (Key != U.PicksKey)
	{
		U.PicksKey = Key;
		U.Picks = video::ThumbChoicesOf(S, RrChoice(U), Open, F.S.Life.Video);
	}
	return U.Picks;
}

/** A picker's press: the selected recipe with the value (the core keeps it inside the archetype's values and the facts behind
 *  the caption true; a value it doesn't allow leaves the recipe as it was). */
void RrEditThumb(RrFrame& F, const std::vector<video::Candidate>& Open, int Field, int Value)
{
	VideoUi& U = F.U;
	const size_t K = static_cast<size_t>(std::clamp(U.ThumbPick, 0, static_cast<int>(U.Thumbs.size()) - 1));
	const video::Choice Ch = RrChoice(U);
	U.Thumbs[K] = video::ThumbEdit(U.Thumbs[K], Ch, Open, F.S.Life.Video, Field, Value);
}

/** A palette's chip: its ground and its accent, as the recipe draws them (ui-thumbs' thumbart::Swatch, by c3f-ui's patch).
 *  Site: the moment's site, for the Site palette's colours; the recipe's Variant picks Mono's accent. */
void RrSwatch(RrFrame& F, const Rect& R, int Pal, int Site, const video::ThumbSpec& S)
{
	thumbart::Swatch(F.C, R, Pal, Site, S.Variant);
}

/**
 * The pickers under the preview (§6.6.2; DESIGN §4.8.2's "CAPTION [82%*][ONE CARD][RIVERED] FACE [Devastated] PALETTE
 * [Heat]"): the caption's rows the moment fills and its style, the faces and the palettes the archetype allows, each press
 * redrawing the preview and the option at once (an edit is a new thumbnail: it paints within the frame's two). The Duel's
 * contenders are edited one at a time (the selected one). Returns the bottom of the rows.
 */
float RrThumbPickers(RrFrame& F, const std::vector<video::Candidate>& Open, float X, float Y, float W)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::ThumbChoices& P = RrPicks(F, Open);
	const size_t K = static_cast<size_t>(std::clamp(U.ThumbPick, 0, static_cast<int>(U.Thumbs.size()) - 1));
	const video::ThumbSpec S = U.Thumbs[K];
	const float H = 28.0f, Lx = X + 2.0f, Cx0 = X + 82.0f;
	// CAPTION: the rows that fill on this moment (the facts' own words), then its style.
	NetSpaced(F.C, "CAPTION", Lx, Y + 19.0f, 10.0f, 900, RrMuted, 1.4f);
	float Right = X + W;
	if (P.CapStyles.size() > 1)
	{
		const int Cs = std::clamp<int>(S.CapStyle, 0, 7);
		const std::string Label = std::string(RrCapStyleNames[Cs]) + " \xE2\x80\xBA";
		const float Bw = F.UI.Measure(Label, 12.5f, 800) + 24.0f;
		Right -= Bw;
		if (RrPickChip(F, "rrcapstyle", {Right, Y, Bw, H}, Label, false))
		{
			// The next style the archetype allows (it cycles).
			size_t At = 0;
			for (size_t I = 0; I < P.CapStyles.size(); ++I)
			{
				At = P.CapStyles[I] == S.CapStyle ? I : At;
			}
			RrEditThumb(F, Open, video::TfCapStyle, P.CapStyles[(At + 1) % P.CapStyles.size()]);
		}
		Right -= 8.0f;
	}
	const std::string Own2 = video::ThumbCaption2(S);
	const int N = static_cast<int>(P.Captions.size());
	U.CapFrom = N > 0 ? std::clamp(U.CapFrom, 0, N - 1) : 0;
	float Cx = Cx0;
	int Shown = 0;
	for (int I = U.CapFrom; I < N; ++I)
	{
		const std::string Label = RrCaptionLabel(P.Captions[static_cast<size_t>(I)]);
		const float Bw = F.UI.Measure(Label, 12.5f, 800) + 24.0f;
		const bool More = I + 1 < N || U.CapFrom > 0;
		if (Cx + Bw > Right - (More ? 34.0f : 0.0f) && Shown > 0)
		{
			break;
		}
		const bool On = S.Caption == P.Captions[static_cast<size_t>(I)].first && Own2 == P.Captions[static_cast<size_t>(I)].second;
		if (RrPickChip(F, "rrcap" + std::to_string(I), {Cx, Y, std::max(40.0f, std::min(Bw, Right - Cx)), H}, Label, On) && !On)
		{
			RrEditThumb(F, Open, video::TfCaption, I);
		}
		Cx += Bw + 6.0f;
		++Shown;
	}
	if (U.CapFrom + Shown < N || U.CapFrom > 0)
	{
		// More rows than the line holds: the next ones (and round to the first).
		if (RrPickChip(F, "rrcapmore", {Cx, Y, 28.0f, H}, "\xE2\x80\xBA", false))
		{
			U.CapFrom = U.CapFrom + Shown < N ? U.CapFrom + Shown : 0;
		}
	}
	// FACE (when the archetype has the player's face to change) and PALETTE on the second row.
	const float Y2 = Y + H + 10.0f;
	float Px = X;
	if (!P.Exprs.empty())
	{
		NetSpaced(F.C, "FACE", Lx, Y2 + 19.0f, 10.0f, 900, RrMuted, 1.4f);
		float Fx = Cx0;
		for (const int16_t E : P.Exprs)
		{
			const std::string Label = RrFaceNames[std::clamp<int>(E, 0, 11)];
			const float Bw = F.UI.Measure(Label, 12.5f, 800) + 22.0f;
			if (RrPickChip(F, "rrface" + std::to_string(E), {Fx, Y2, Bw, H}, Label, S.Expr == E) && S.Expr != E)
			{
				RrEditThumb(F, Open, video::TfFace, E);
			}
			Fx += Bw + 6.0f;
		}
		Px = std::max(X + W - 340.0f, Fx + 14.0f);
	}
	if (!P.Pals.empty())
	{
		NetSpaced(F.C, "PALETTE", Px + 2.0f, Y2 + 19.0f, 10.0f, 900, RrMuted, 1.4f);
		const int Site = video::ThumbFor(RrChoice(U), Open).Site; // the Site palette's colours are the moment's site's
		float Sx = Px + 82.0f;
		for (const int16_t Pl : P.Pals)
		{
			const Rect Sw{Sx, Y2 + 2.0f, 34.0f, 24.0f};
			const bool On = S.Pal == Pl;
			const Ui::ClickState St = F.UI.Clickable("rrpal" + std::to_string(Pl), {Sw.X - 3.0f, Sw.Y - 3.0f, Sw.W + 6.0f, Sw.H + 6.0f}, F.Live);
			RrSwatch(F, Sw, Pl, Site, S);
			F.C.StrokeRoundRect({Sw.X - 3.0f, Sw.Y - 3.0f, Sw.W + 6.0f, Sw.H + 6.0f}, 7.0f, On ? RrGold : St.Hover ? NetA(RrCream, 0.5f) : RrLine, On ? 2.0f : 1.0f);
			if (St.Clicked && !On)
			{
				RrEditThumb(F, Open, video::TfPalette, Pl);
			}
			Sx += Sw.W + 10.0f;
		}
		F.UI.Text(RrPalNames[std::clamp<int>(U.Thumbs[K].Pal, 0, 10)], Sx + 4.0f, Y2 + 19.0f, Ts(12.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, std::max(40.0f, X + W - Sx - 4.0f)));
	}
	return Y2 + H;
}

void RrEditor(RrFrame& F, const std::vector<video::Candidate>& Open, float T)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const float X = RrEdX, W = RrEdW;
	// FORMAT: a Cut, a Breakdown, a night's Vlog, a run's Recap.
	NetSpaced(F.C, "FORMAT", X + 2.0f, 84.0f + T, 10.0f, 900, RrMuted, 1.4f);
	const video::Format Order[4] = {video::Format::Short, video::Format::Breakdown, video::Format::Vlog, video::Format::Recap};
	for (int K = 0; K < 4; ++K)
	{
		const video::Format Fm = Order[K];
		const float Bw = (W - 30.0f) / 4.0f;
		if (RrSegment(F, std::string("rrfmt") + RrFormatName(Fm), {X + Nf(K) * (Bw + 10.0f), 94.0f + T, Bw, 48.0f}, RrFormatName(Fm), RrFormatSub(Fm), U.F == Fm))
		{
			RrSetFormat(U, Open, Fm);
		}
	}
	// LENGTH (§4.5.5): Tight, Standard, Extended with their lengths; a Cut is 45 seconds, fixed.
	NetSpaced(F.C, "LENGTH", X + 2.0f, 164.0f + T, 10.0f, 900, RrMuted, 1.4f);
	if (U.F == video::Format::Short)
	{
		F.UI.Text("45 s, fixed", X + 2.0f, 192.0f + T, Ts(14.0f, 700, RrMuted));
	}
	else
	{
		const char* Names[3] = {"Tight", "Standard", "Extended"};
		for (int K = 0; K < 3; ++K)
		{
			const video::LengthOpt L = static_cast<video::LengthOpt>(K);
			const float Bw = (W - 20.0f) / 3.0f;
			const std::string Label = std::string(Names[K]) + "  " + RrClock(video::valgo::LengthMinFor(U.F, L));
			if (RrSegment(F, std::string("rrlen") + Names[K], {X + Nf(K) * (Bw + 10.0f), 174.0f + T, Bw, 34.0f}, Label, std::string(), U.Len == L))
			{
				U.Len = L;
			}
		}
	}
	// TITLE: the style, three options from the library (or the Update 2 title before it deals), SHUFFLE.
	NetSpaced(F.C, "TITLE", X + 2.0f, 230.0f + T, 10.0f, 900, RrMuted, 1.4f);
	const float StyleW = (W - 130.0f - 32.0f) / 5.0f;
	for (int K = 0; K < 5; ++K)
	{
		const video::TitleStyle Ts_ = static_cast<video::TitleStyle>(K);
		if (RrSegment(F, std::string("rrtitle") + RrStyleName(Ts_), {X + Nf(K) * (StyleW + 8.0f), 240.0f + T, StyleW, 32.0f}, RrStyleName(Ts_), std::string(), U.T == Ts_))
		{
			U.T = Ts_;
			U.TitlePick = 0;
		}
	}
	RrOptions(F);
	const bool HaveTitles = !U.Titles.empty();
	if (RrSegment(F, "rrtshuffle", {X + W - 120.0f, 240.0f + T, 120.0f, 32.0f}, "SHUFFLE", std::string(), false) && HaveTitles)
	{
		++U.TitleShuffle;
		U.TitlePick = 0;
		RrOptions(F);
	}
	const video::Choice Ch0 = RrChoice(U);
	std::vector<std::string> Options;
	if (HaveTitles)
	{
		for (const video::TitleOption& O : U.Titles)
		{
			Options.push_back(O.Text);
		}
	}
	else if (!U.Pick.empty())
	{
		Options.push_back(video::Title(Ch0, Open, 0));
	}
	if (Options.empty())
	{
		const Rect Box{X, 282.0f + T, W, 46.0f};
		F.C.FillRoundRect(Box, 12.0f, RrPanel);
		F.UI.Text(U.ListTab == 1 ? RrPickIdea : RrPickFirst, Box.X + 20.0f, Box.Y + 29.0f, Ts(15.0f, 600, RrMuted));
	}
	for (size_t K = 0; K < Options.size() && K < 3; ++K)
	{
		const Rect Row{X, 282.0f + T + Nf(K) * 50.0f, W, 44.0f};
		const bool On = static_cast<int>(K) == (HaveTitles ? U.TitlePick : 0);
		const Ui::ClickState St = F.UI.Clickable("rrtopt" + std::to_string(K), Row, F.Live);
		if (St.Clicked && HaveTitles)
		{
			U.TitlePick = static_cast<int>(K);
		}
		F.C.FillRoundRect(Row, 10.0f, On ? Mix(RrPanel, RrGold, 0.10f) : St.Hover ? RrPanel2 : RrPanel);
		F.C.StrokeRoundRect(Row, 10.0f, On ? RrGold : RrLine, On ? 1.6f : 1.0f);
		F.C.StrokeEllipse(Row.X + 22.0f, Row.Y + 22.0f, 8.0f, 8.0f, On ? RrGold : RrMuted, 1.6f);
		if (On)
		{
			F.C.FillCircle(Row.X + 22.0f, Row.Y + 22.0f, 4.5f, RrGold);
		}
		const std::string Quoted = "\xE2\x80\x9C" + Options[K] + "\xE2\x80\x9D";
		const std::vector<std::string> Lines = NetWrap(F.C, Quoted, Row.W - 60.0f, 15.0f, 700);
		if (Lines.size() <= 1)
		{
			F.UI.Text(Quoted, Row.X + 40.0f, Row.Y + 27.0f, Ts(15.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Row.W - 56.0f));
		}
		else
		{
			NetParagraph(F.C, Quoted, Row.X + 40.0f, Row.Y + 18.0f, Row.W - 56.0f, 13.5f, 700, RrCream, 16.0f, 2);
		}
	}
	// THUMBNAIL: the preview and six options (the library's, in its hash order: never sorted by fit), SHUFFLE, DUEL. Before
	// the library deals recipes, the Update 2 frames. A Cut has none (P0).
	const float Ty = 476.0f + T;
	NetSpaced(F.C, "THUMBNAIL", X + 2.0f, Ty - 8.0f, 10.0f, 900, RrMuted, 1.4f);
	const Rect Pv{X, Ty, RrThumbW, RrThumbH};
	if (U.F == video::Format::Short)
	{
		F.C.FillRoundRect(Pv, 10.0f, RrPanel);
		NetParagraph(F.C, RrCutNoThumb, Pv.X + 24.0f, Pv.Y + Pv.H / 2.0f, Pv.W - 48.0f, 15.0f, 600, RrMuted, 20.0f, 2);
	}
	else
	{
		if (RrSegment(F, "rrthshuffle", {X + W - 218.0f, Ty - 30.0f, 104.0f, 26.0f}, "SHUFFLE", std::string(), false) && !U.Thumbs.empty())
		{
			++U.ThumbShuffle;
			U.ThumbPick = 0;
			U.Duel.clear();
			RrOptions(F);
		}
		if (RrSegment(F, "rrduel", {X + W - 104.0f, Ty - 30.0f, 104.0f, 26.0f}, "DUEL", std::string(), U.DuelOn))
		{
			if (U.Thumbs.size() < 2)
			{
				U.Note = RrDuelNeedsNew;
				U.NoteBad = true;
				U.NoteAt = F.Now;
			}
			else
			{
				U.DuelOn = !U.DuelOn;
				U.Duel = U.DuelOn ? std::vector<int>{U.ThumbPick} : std::vector<int>();
			}
		}
		// After the clicks (a shuffle deals again): what the options are now.
		const bool Recipes = !U.Thumbs.empty();
		if (!Recipes)
		{
			U.DuelOn = false;
			U.Duel.clear();
		}
		U.OptTop = Ty;
		F.C.FillRoundRect({Pv.X - 4.0f, Pv.Y - 4.0f, Pv.W + 8.0f, Pv.H + 8.0f}, 12.0f, RrPanel);
		const video::Choice Ch = RrChoice(U);
		// The facts a recipe draws besides its captions (the cards, the opponent, the site): the choice's own Thumb.
		const video::Thumb Facts = U.Pick.empty() ? video::Thumb() : video::ThumbFor(Ch, Open);
		if (U.Pick.empty())
		{
			F.C.FillRect(Pv, RrGround);
			videoart::Logo(F.C, {Pv.X + Pv.W / 2.0f - 40.0f, Pv.Y + Pv.H / 2.0f - 40.0f, 80.0f, 80.0f});
		}
		else if (Recipes)
		{
			RrSpecThumb(F, Pv, U.Thumbs[static_cast<size_t>(std::clamp(U.ThumbPick, 0, static_cast<int>(U.Thumbs.size()) - 1))], Facts);
		}
		else
		{
			RrLegacyThumb(F, Pv, video::ThumbFor(Ch, Open));
		}
		F.C.StrokeRoundRect({Pv.X - 4.0f, Pv.Y - 4.0f, Pv.W + 8.0f, Pv.H + 8.0f}, 12.0f, RrLine, 1.0f);
		if (Recipes && U.DuelOn)
		{
			// The contender the preview (and the pickers under it) shows, lettered as on its option, after THUMBNAIL.
			const auto In = std::find(U.Duel.begin(), U.Duel.end(), U.ThumbPick);
			if (In != U.Duel.end())
			{
				const char Letter[2] = {static_cast<char>('A' + (In - U.Duel.begin())), '\0'};
				F.C.FillCircle(X + 112.0f, Ty - 12.0f, 8.0f, RrGold);
				F.UI.Text(Letter, X + 112.0f, Ty - 11.5f, Ts(10.0f, 900, Hex(videoart::Ink), Align::Center, Baseline::Middle));
			}
		}
		// The six options at 120 x 68 beside the preview: recipes, or the five Update 2 frames.
		const float Ox = X + RrThumbW + 12.0f;
		const int N = Recipes ? static_cast<int>(U.Thumbs.size()) : (U.Pick.empty() ? 0 : static_cast<int>(video::Frame::Recipe));
		for (int K = 0; K < N; ++K)
		{
			const Rect O{Ox + Nf(K % 2) * (RrOptW + 8.0f), Ty + Nf(K / 2) * (RrOptH + 8.0f), RrOptW, RrOptH};
			const auto InDuel = std::find(U.Duel.begin(), U.Duel.end(), K);
			const bool On = Recipes ? (U.DuelOn ? InDuel != U.Duel.end() : U.ThumbPick == K) : U.Fr == static_cast<video::Frame>(K);
			const Ui::ClickState St = F.UI.Clickable("rrthopt" + std::to_string(K), O, F.Live);
			F.C.FillRoundRect(O, 6.0f, RrGround);
			if (Recipes)
			{
				RrSpecThumb(F, O, U.Thumbs[static_cast<size_t>(K)], Facts);
			}
			else
			{
				video::Choice One = Ch;
				One.Fr = static_cast<video::Frame>(K);
				RrLegacyThumb(F, O, video::ThumbFor(One, Open));
			}
			F.C.StrokeRoundRect(O, 6.0f, On ? RrGold : St.Hover ? NetA(RrCream, 0.5f) : RrLine, On ? 2.5f : 1.0f);
			if (Recipes && U.DuelOn && InDuel != U.Duel.end())
			{
				// The letter on a disc at the tile's corner, mostly in the gap (c3f-thumbs' patch c3f-thumbs_to_c3f-ui_1, C3 L7): 14
				// in, it covered a caption's words on 17 of the 106 option tiles rerun_ui draws (Venue's marquee read "BERCREST",
				// "1 AWAY" lost its 1). Two units in and 8.5 wide, it clears the SHUFFLE button above the top row.
				const char Letter[2] = {static_cast<char>('A' + (InDuel - U.Duel.begin())), '\0'};
				F.C.FillCircle(O.X + 2.0f, O.Y + 2.0f, 8.5f, Hex(videoart::Ink));
				F.C.FillCircle(O.X + 2.0f, O.Y + 2.0f, 7.0f, RrGold);
				F.UI.Text(Letter, O.X + 2.0f, O.Y + 2.5f, Ts(10.0f, 900, Hex(videoart::Ink), Align::Center, Baseline::Middle));
			}
			if (!Recipes)
			{
				F.UI.Text(video::FrameName(static_cast<video::Frame>(K)), O.X + 6.0f, O.Y + O.H - 6.0f, Ts(10.5f, 800, RrCream));
			}
			if (St.Clicked)
			{
				if (!Recipes)
				{
					U.Fr = static_cast<video::Frame>(K);
				}
				else if (U.DuelOn)
				{
					if (InDuel != U.Duel.end())
					{
						// A contender that isn't the one shown is selected, for the pickers to edit (M1); a press on the one shown
						// takes it out (review-c3f-ui: any press took a contender out, so only the last one in could be edited).
						if (U.Duel.size() > 1 && U.ThumbPick == K)
						{
							U.Duel.erase(InDuel);
						}
					}
					else if (U.Duel.size() < 3)
					{
						U.Duel.push_back(K);
					}
					U.ThumbPick = K;
				}
				else
				{
					U.ThumbPick = K;
				}
			}
		}
		// Under the options: the Duel's line (who's in it and what it costs), or how the test works.
		const float Ny = Ty + 3.0f * (RrOptH + 8.0f) + 12.0f;
		if (U.DuelOn && Recipes)
		{
			std::string Who;
			for (size_t K = 0; K < U.Duel.size(); ++K)
			{
				Who += (K == 0 ? std::string() : std::string(" + ")) + static_cast<char>('A' + K);
			}
			const int Extra = std::max(0, static_cast<int>(U.Duel.size()) - 1);
			F.UI.Text("DUEL: " + Who + (Extra > 0 ? " (+" + std::to_string(15 * Extra) + " min)" : std::string()), Ox, Ny + 4.0f, Ts(12.5f, 900, RrGold, Align::Left, Baseline::Alphabetic, false, 248.0f));
			NetParagraph(F.C, RrDuelHelp, Ox, Ny + 24.0f, 248.0f, 11.5f, 600, RrMuted, 15.0f, 5);
		}
		else if (Recipes)
		{
			// The picked option's archetype and its read (core-videocontent's ArchName / ArchRead).
			const video::ThumbSpec& Sp = U.Thumbs[static_cast<size_t>(std::clamp(U.ThumbPick, 0, static_cast<int>(U.Thumbs.size()) - 1))];
			const char* Name = video::ArchName(Sp.Arch);
			F.UI.Text(Name ? std::string(Name) : std::string(), Ox, Ny + 4.0f, Ts(12.5f, 900, RrCream, Align::Left, Baseline::Alphabetic, false, 248.0f));
			NetParagraph(F.C, video::ArchRead(Sp.Arch), Ox, Ny + 22.0f, 248.0f, 11.5f, 600, RrMuted, 15.0f, 3);
		}
	}
	// Under the preview: the pickers for the selected recipe (M1), then the moments in the pick for a Vlog or a Recap and the
	// Live Drop hint (R3): on one line under the pickers, on two without them.
	const bool Pickers = U.F != video::Format::Short && !U.Pick.empty() && !U.Thumbs.empty();
	const float By = Pickers ? RrThumbPickers(F, Open, X, Ty + RrThumbH + 12.0f, W) + 24.0f : Ty + RrThumbH + 30.0f;
	if (!U.Pick.empty())
	{
		const std::string Moments = std::to_string(U.Pick.size()) + (U.Pick.size() == 1 ? " moment" : " moments") + " \xC2\xB7 " + RrFormatName(U.F) + " " +
			RrClock(video::valgo::LengthMinFor(U.F, U.F == video::Format::Short ? video::LengthOpt::Standard : U.Len));
		F.UI.Text(Moments, X, By, Ts(13.0f, 700, RrMuted));
		// Live Drop pushes a Rerun 2.0 upload only (NoteStream reads ChannelX): an Update 2 choice set on an Update 2 channel
		// (no template, no recipe) goes up as Update 2's did, with nothing to push.
		const video::Choice Up = RrChoice(U);
		if (F.S.Life.Video.X.On || !Up.TitleTpl.empty() || !Up.Thumbs.empty())
		{
			const float Hx = Pickers ? X + F.UI.Measure(Moments, 13.0f, 700) + 18.0f : X;
			F.UI.Text(video::valgo::LiveDropHint(), Hx, Pickers ? By : By + 20.0f, Ts(13.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, X + W - Hx));
		}
	}
}

// ------------------------------------------------------------------ the side: the read, the settings, PUBLISH

/** A settings row: its label on the left, the value (a link that cycles) on the right; true when the value is clicked. */
bool RrSettingRow(RrFrame& F, const std::string& Id, float Y, const std::string& Label, const std::string& Value, const Color& Col, bool Enabled)
{
	using namespace rlnet_detail;
	const float X = RrSideX, W = RrSideW;
	F.UI.Text(Label, X, Y, Ts(13.0f, 700, RrMuted));
	const float Vw = std::min(W - 120.0f, F.UI.Measure(Value, 13.5f, 800));
	const Rect R{X + W - Vw - 22.0f, Y - 18.0f, Vw + 22.0f, 26.0f};
	const Ui::ClickState St = F.UI.Clickable(Id, R, F.Live && Enabled);
	F.UI.Text(Value, X + W - 16.0f, Y, Ts(13.5f, 800, St.Hover ? RrCream : Col, Align::Right, Baseline::Alphabetic, false, W - 136.0f));
	if (Enabled)
	{
		NetChevron(F.C, X + W - 5.0f, Y - 5.0f, 8.0f, 0, St.Hover ? RrCream : RrMuted);
	}
	F.C.FillRect({X, Y + 12.0f, W, 1.0f}, RrLine);
	return St.Clicked;
}

void RrSide(RrFrame& F, const std::vector<video::Candidate>& Open, float T)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const float X = RrSideX, W = RrSideW;
	const video::Channel& Rerun = F.S.Life.Video;
	const video::Choice Ch = RrChoice(U);
	// THE PACKAGING READ (W9: facts and comparisons, never a future number).
	NetSpaced(F.C, "PACKAGING READ", X + 2.0f, 84.0f + T, 10.0f, 900, RrMuted, 1.4f);
	const bool AdsOn = Rerun.PartnerAt >= 0.0 && Rerun.PartnerAt <= F.World;
	// Rerun 2.0's choice reads through core-videocontent's PackagingRead (§6.6.2: the format, the moment, the title's
	// template, the archetype against the moment, sameness, the Duel, promise against payoff, the ad load, the coin's reason,
	// burnout); an Update 2 choice set keeps Update 2's Advice. The audience's hour sits under "Goes up" below, so the
	// read's own hour line is left out.
	const bool U3 = Rerun.X.On || !Ch.Thumbs.empty() || !Ch.TitleTpl.empty();
	std::vector<std::string> Lines;
	if (!U.Pick.empty() && U3)
	{
		std::string Key = RrOptionKey(U) + "#" + std::to_string(static_cast<int>(Ch.Len)) + std::to_string(static_cast<int>(Ch.Ads)) + (Ch.CleanEdit ? "c" : "-") +
			(AdsOn ? "p" : "-") + Ch.TitleTpl + "#" + std::to_string(static_cast<int64_t>(std::floor(F.World / 60.0))) + "#" + std::to_string(Rerun.Videos.size());
		for (const video::ThumbSpec& S : Ch.Thumbs)
		{
			// The pickers' edits too (M1: a caption, a face, a palette or a caption style moves the read's words).
			Key += "/" + std::to_string(S.Arch) + "." + std::to_string(S.Pal) + "." + std::to_string(S.Expr) + "." + std::to_string(S.Variant) + "." + std::to_string(S.CapStyle) + "." +
				S.Caption + "." + video::ThumbCaption2(S);
		}
		if (Key != U.ReadKey)
		{
			U.ReadKey = Key;
			U.Read.clear();
			for (const std::string& L : video::PackagingRead(Ch, Open, Rerun, F.World, AdsOn))
			{
				if (L.rfind("Your viewers are on at ", 0) != 0)
				{
					U.Read.push_back(L);
				}
			}
		}
		Lines = U.Read;
	}
	else if (!U.Pick.empty())
	{
		Lines = video::Advice(Ch, Open, AdsOn);
	}
	bool Auto = false;
	for (const std::string& Id : U.Pick)
	{
		const video::Candidate* C = RrFind(Open, Id);
		Auto = Auto || (C && C->Auto);
	}
	if (Auto)
	{
		Lines.push_back(RrAutoNote);
	}
	// Rerun 2.0's read is longer (up to ten lines): smaller type, and a line that wouldn't fit above the carry or the
	// settings waits (the read's order puts the format and the moment first).
	// R1's carry is the Momentum the upload starts with at the edit's end (after 21+ dark days, with welcome back's +0.2).
	double CarryMin = 0.0, CarryEnergy = 0.0;
	int CarryN = 0;
	RrEditCost(U, Ch, RrStrongLanguage(Ch), CarryMin, CarryEnergy, CarryN);
	const double CarryMomC = RrCarryAt(Rerun, F.World + CarryMin);
	const bool CarryOn = CarryMomC > 1.1;
	const float Size = U3 ? 13.0f : 14.5f, Lead = U3 ? 17.0f : 20.0f;
	const float Limit = (U3 ? (CarryOn ? 352.0f : 424.0f) : 340.0f) + T; // the last baseline a line may sit on
	float Y = 112.0f + T;
	for (const std::string& L : Lines)
	{
		const size_t N = std::min<size_t>(3, std::max<size_t>(1, NetWrap(F.C, L, W - 20.0f, Size, 500).size()));
		if (Y + Lead * static_cast<float>(N - 1) > Limit)
		{
			break;
		}
		F.C.FillCircle(X + 6.0f, Y - 5.0f, 3.5f, RrGold);
		Y = NetParagraph(F.C, L, X + 20.0f, Y, W - 20.0f, Size, 500, RrCream, Lead, 3) + (U3 ? 7.0f : 10.0f);
	}
	// R1: the carry, while the channel's Momentum gives the next upload more reach (the core's line, true at upload).
	if (CarryOn)
	{
		const Rect Carry{X, std::max(Y + 4.0f, 372.0f + T), W, 54.0f};
		F.C.FillRoundRect(Carry, 8.0f, RrGold);
		// One line or two, centred in the strip.
		const std::string Line = video::valgo::LaunchCarry(CarryMomC);
		const float Rows = Nf(std::min<size_t>(2, std::max<size_t>(1, NetWrap(F.C, Line, Carry.W - 28.0f, 14.0f, 900).size())));
		NetParagraph(F.C, Line, Carry.X + 14.0f, Carry.Y + 32.0f - 9.0f * (Rows - 1.0f), Carry.W - 28.0f, 14.0f, 900, Hex(videoart::Ink), 18.0f, 2);
	}
	// SETTINGS (§6.6.2): ads, the coin and its reason, a Next-up card, the series, when it goes up.
	const float Sy = 446.0f + T;
	NetSpaced(F.C, "SETTINGS", X + 2.0f, Sy, 10.0f, 900, RrMuted, 1.4f);
	float Ry = Sy + 30.0f;
	F.UI.Text("Ads", X, Ry, Ts(13.0f, 700, RrMuted));
	if (U.F == video::Format::Short)
	{
		F.UI.Text("Cuts share the feed's ads", X + W, Ry, Ts(13.0f, 700, RrCream, Align::Right));
	}
	else
	{
		const char* Loads[3] = {"Light", "Standard", "Heavy"};
		for (int K = 2; K >= 0; --K)
		{
			const float Bw = K == 1 ? 92.0f : 70.0f;
			const float Bx = X + W - (K == 2 ? 70.0f : K == 1 ? 168.0f : 244.0f);
			if (RrSegment(F, std::string("rrads") + Loads[K], {Bx, Ry - 20.0f, Bw, 28.0f}, Loads[K], std::string(), U.Ads == static_cast<video::AdLoad>(K)))
			{
				U.Ads = static_cast<video::AdLoad>(K);
			}
		}
	}
	F.C.FillRect({X, Ry + 14.0f, W, 1.0f}, RrLine);
	Ry += 42.0f;
	// The coin (§4.5.4): full ads, or Limited for strong language (a tilted face with a Loud title) unless the edit's clean.
	// The coin itself is core-videocontent's CoinOf (a real-money site's promotion is Limited and 18+ whatever the edit;
	// strong language unless the edit's clean), the same test Upload makes; its reason is in the read above.
	const bool Strong = RrStrongLanguage(Ch);
	F.UI.Text("Coin", X, Ry, Ts(13.0f, 700, RrMuted));
	const bool Limited = U.Pick.empty() ? false : U3 ? video::CoinOf(Ch, Open) == video::Coin::Limited : Strong && !U.CleanEdit;
	F.C.FillCircle(X + W - 8.0f, Ry - 5.0f, 7.0f, Limited ? Hex(0xf2a93b) : RrGood);
	F.UI.Text(Limited ? RrLimitedWhy : U.F == video::Format::Short ? std::string("Feed ads") : std::string("Full ads"), X + W - 22.0f, Ry, Ts(13.0f, 800, Limited ? Hex(0xf2a93b) : RrCream, Align::Right, Baseline::Alphabetic, false, W - 80.0f));
	F.C.FillRect({X, Ry + 12.0f, W, 1.0f}, RrLine);
	Ry += 34.0f;
	if (Strong)
	{
		const Rect Box{X, Ry - 16.0f, 18.0f, 18.0f};
		const Ui::ClickState St = F.UI.Clickable("rrclean", {X, Ry - 18.0f, W, 24.0f}, F.Live);
		F.C.StrokeRoundRect(Box, 4.0f, U.CleanEdit ? RrGold : RrMuted, 1.5f);
		if (U.CleanEdit)
		{
			NetCheck(F.C, Box.X + 9.0f, Box.Y + 9.0f, 12.0f, RrGold);
		}
		F.UI.Text(RrCleanEdit, X + 28.0f, Ry, Ts(13.0f, 700, St.Hover ? RrCream : RrMuted));
		if (St.Clicked)
		{
			U.CleanEdit = !U.CleanEdit;
		}
		Ry += 30.0f;
	}
	// A Next-up card: one of your kept long videos to link at the end (it sends viewers back when this one's watched).
	std::vector<const video::Video*> Linkable;
	for (const video::Video& V : Rerun.Videos)
	{
		if (V.F != video::Format::Short && !RrIsGrant(V))
		{
			Linkable.push_back(&V);
		}
	}
	std::stable_sort(Linkable.begin(), Linkable.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	const video::Video* Linked = U.NextUp.empty() ? nullptr : RrVideo(F.S, U.NextUp);
	if (!Linked)
	{
		U.NextUp.clear();
	}
	if (RrSettingRow(F, "rrnextup", Ry, "Next-up card", Linked ? RrShorten(F.C, Linked->Title, 13.5f, 800, W - 150.0f) : std::string("none"), Linked ? RrCream : RrMuted, !Linkable.empty()))
	{
		// Cycles: none, then the kept long videos newest first.
		size_t Next = 0;
		for (size_t K = 0; K < Linkable.size(); ++K)
		{
			Next = Linkable[K]->Id == U.NextUp ? K + 1 : Next;
		}
		U.NextUp = U.NextUp.empty() ? Linkable.front()->Id : Next < Linkable.size() ? Linkable[Next]->Id : std::string();
	}
	Ry += 34.0f;
	const std::vector<video::Challenge>& Ser = Rerun.X.Challenges;
	const video::Challenge* InSeries = nullptr;
	for (const video::Challenge& Cl : Ser)
	{
		InSeries = Cl.Id == U.Series ? &Cl : InSeries;
	}
	if (!InSeries)
	{
		U.Series = -1;
	}
	if (RrSettingRow(F, "rrseriesrow", Ry, "Series", InSeries ? Money(InSeries->Start) + " to " + Money(InSeries->Goal) + ", episode " + std::to_string(InSeries->Episodes + 1) : std::string("none"),
			InSeries ? RrCream : RrMuted, !Ser.empty()))
	{
		size_t Next = 0;
		for (size_t K = 0; K < Ser.size(); ++K)
		{
			Next = Ser[K].Id == U.Series ? K + 1 : Next;
		}
		U.Series = U.Series < 0 ? Ser.front().Id : Next < Ser.size() ? Ser[Next].Id : -1;
	}
	Ry += 40.0f;
	// When it goes up (§6.6.3): the edit's end, against the audience's peak hour that day (the timing lever).
	double Minutes = 0.0, Energy = 0.0;
	int Contenders = 0;
	RrEditCost(U, Ch, Strong, Minutes, Energy, Contenders);
	const double Up = F.World + Minutes;
	const double Peak = video::algo::PeakHour(Rerun, net::Weekday(net::DayOf(Up))); // the day it goes up (PackagingRead's convention)
	F.UI.Text("Goes up " + net::TimeLabel(Up) + ", when the edit ends", X, Ry, Ts(13.5f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, W));
	F.UI.Text("Your viewers peak around " + ClockString(Peak * 60.0), X, Ry + 20.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, W));
	// R2: the upload streak (Studio and the Channel's header), the core's line, once a week has counted.
	if (Rerun.X.On && Rerun.X.StreakW > 0)
	{
		F.C.FillCircle(X + 5.0f, Ry + 49.0f, 3.5f, RrGold);
		F.UI.Text(video::valgo::StreakLine(Rerun.X.StreakW, Rerun.X.RainChecks), X + 16.0f, Ry + 54.0f, Ts(13.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, W - 16.0f));
	}
	// The edit's real cost, then PUBLISH (update 2 §5.7's two-press strike warning kept).
	const std::string Edit = "Edit " + RrEditTime(Minutes) + (Contenders >= 2 ? " (+Duel)" : std::string()) + " \xC2\xB7 " + std::to_string(std::lround(Energy)) + " energy";
	F.UI.Text(Edit, X, 812.0f, Ts(16.0f, 800, RrCream));
	const std::vector<std::string> Why = RrPublishWhy(F.S, Ch);
	std::string Strike;
	std::string PickKey;
	for (const std::string& Id : U.Pick)
	{
		PickKey += Id + "|";
		const video::Candidate* Cand = RrFind(Open, Id);
		if (Strike.empty() && Cand && (Cand->Source == 0 || Cand->Source == 3) && Cand->Site >= 0 && Cand->Site < static_cast<int>(net::SiteId::Count))
		{
			Strike = rlsites_detail::SitesClauseWarning(F.S, static_cast<net::SiteId>(Cand->Site), false, true);
		}
	}
	const bool Armed = !Strike.empty() && U.StrikeArmed == PickKey;
	float Wy = 838.0f;
	for (const std::string& Line : Why)
	{
		Wy = NetParagraph(F.C, Line, X, Wy, W, 13.0f, 600, RrWarn, 17.0f, 2) + 2.0f;
	}
	if (!Strike.empty() && Why.empty())
	{
		NetParagraph(F.C, Armed ? std::string(rlsites_detail::SitesPublishArmed) : Strike, X, 940.0f, W, 12.5f, 800, RrWarn, 16.0f, 1);
	}
	const Rect Btn{X + W - 220.0f, 872.0f, 220.0f, 56.0f};
	const bool Pressed = RrButton(F, "rrpublish", Btn, Armed ? std::string("PUBLISH ANYWAY") : std::string("PUBLISH"), Why.empty());
	if (Pressed && !Strike.empty() && !Armed)
	{
		U.StrikeArmed = PickKey; // the first press only warns
	}
	else if (Pressed)
	{
		U.StrikeArmed.clear();
		// An Update 2 channel's Update 2 choice set (no template, no recipe) uploads as Update 2 did: no launch card follows,
		// so the Channel shows it the moment the edit's done (update 2's flow).
		const bool Legacy = !Rerun.X.On && Ch.TitleTpl.empty() && Ch.Thumbs.empty();
		const std::string Refused = F.S.Publish(Ch);
		if (Refused.empty())
		{
			if (Legacy)
			{
				U.Page = RrPageChannel;
				U.PageAt = F.Now;
				U.GridGoal = U.GridScroll = 0.0f;
				U.ChanTab = Ch.F == video::Format::Short ? 1 : 0;
			}
			// The edit runs as a time skip; the video goes up when it's done and its launch card takes Studio for 48 hours.
			U.Pick.clear();
			U.Touched = false;
			U.DuelOn = false;
			U.Duel.clear();
			U.CleanEdit = false;
			U.NextUp.clear();
			U.Series = -1;
			U.TitleShuffle = U.ThumbShuffle = U.TitlePick = U.ThumbPick = 0;
			U.LaunchHidden = false;
			U.OptKey.clear();
			U.ThumbKey.clear(); // the channel moved (the last upload's look): the next deal is fresh
			U.Note.clear();
		}
		else
		{
			U.Note = Refused;
			U.NoteBad = true;
			U.NoteAt = F.Now;
		}
	}
}

// ------------------------------------------------------------------ the launch card (§6.6.4)

double RrMedian(std::vector<double> V)
{
	if (V.empty())
	{
		return 0.0;
	}
	std::sort(V.begin(), V.end());
	return V[V.size() / 2];
}

/** The launch card's comparison: the last 10 uploads before this one at the same age (their own closed forms). */
void RrLaunchNumbers(RrFrame& F, const video::Video& V)
{
	VideoUi& U = F.U;
	const double Minute = std::floor(F.World);
	if (U.LaunchId == V.Id && U.LaunchMinute == Minute)
	{
		return;
	}
	U.LaunchId = V.Id;
	U.LaunchMinute = Minute;
	std::vector<const video::Video*> Before;
	for (const video::Video& O : F.S.Life.Video.Videos)
	{
		if (&O != &V && O.At < V.At && !RrIsGrant(O))
		{
			Before.push_back(&O);
		}
	}
	std::stable_sort(Before.begin(), Before.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	if (Before.size() > 9)
	{
		Before.resize(9);
	}
	const double Age = std::max(0.0, F.World - V.At);
	U.LaunchHours.assign(48, std::numeric_limits<double>::quiet_NaN());
	U.LaunchTypical.assign(48, 0.0);
	for (int H = 0; H < 48; ++H)
	{
		const double A = static_cast<double>(H) * 60.0;
		if (A < Age)
		{
			U.LaunchHours[static_cast<size_t>(H)] = std::max(0.0, video::Views(V, V.At + std::min(Age, A + 60.0)) - video::Views(V, V.At + A));
		}
		std::vector<double> Typ;
		for (const video::Video* O : Before)
		{
			Typ.push_back(std::max(0.0, video::Views(*O, O->At + A + 60.0) - video::Views(*O, O->At + A)));
		}
		U.LaunchTypical[static_cast<size_t>(H)] = RrMedian(Typ);
	}
	std::vector<double> AtAge, Ctr, Avd;
	const double Mine = video::Views(V, F.World);
	int Rank = 1;
	const int Today = net::DayOf(F.World);
	for (const video::Video* O : Before)
	{
		const double Seen = video::Views(*O, O->At + Age);
		AtAge.push_back(Seen);
		Rank += Seen > Mine ? 1 : 0;
		Ctr.push_back(video::valgo::RolledCtr(*O));
		Avd.push_back(O->LengthMin * video::valgo::RetentionOn(*O, Today));
	}
	U.TypViews = RrMedian(AtAge);
	U.TypCtr = RrMedian(Ctr);
	U.TypAvd = RrMedian(Avd);
	U.LaunchRank = Rank;
	U.LaunchOf = static_cast<int>(Before.size()) + 1;
}

/** "31 h", "45 min": how long until a world minute. */
std::string RrUntil(double Minutes)
{
	if (Minutes < 60.0)
	{
		return std::to_string(std::max(1, static_cast<int>(std::ceil(Minutes)))) + " min";
	}
	return std::to_string(static_cast<int>(std::floor(Minutes / 60.0))) + " h";
}

void RrLaunchCard(RrFrame& F, const video::Video& V)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const video::VideoStats& St = RrStats(F, V);
	RrLaunchNumbers(F, V);
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	const double Age = std::max(0.0, F.World - V.At);
	// The title and when the verdict comes.
	NetSpaced(F.C, "LAUNCH", 22.0f, 98.0f, 11.0f, 900, RrGold, 1.6f);
	F.UI.Text(V.Title, 110.0f, 100.0f, Ts(22.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, 980.0f));
	const double Verdict = static_cast<double>(video::valgo::VerdictDayOf(V)) * net::MinutesPerDay;
	F.UI.Text(std::string(RrVerdictSoon) + ": " + RrUntil(Verdict - F.World), NetW - 20.0f, 92.0f, Ts(14.0f, 800, RrGold, Align::Right));
	if (RrLink(F, "rrnewvideo", std::string(RrNewVideo) + " \xE2\x80\xBA", NetW - 20.0f, 112.0f, 12.0f, RrMuted, Align::Right))
	{
		U.LaunchHidden = true;
	}
	// The thumbnail as it went up.
	const Rect Th{20.0f, 124.0f, 320.0f, 180.0f};
	F.C.FillRoundRect(Th, 8.0f, RrGround);
	RrThumb(F, Th, V);
	F.C.StrokeRoundRect(Th, 8.0f, RrLine, 1.0f);
	RrLengthBadge(F, Th, V);
	// The numbers so far, each against the typical upload at the same age (outcomes, never a forecast: W9, V5).
	const chart::Theme& T = RrTheme();
	const float Kx = 356.0f, Kw = (NetW - 20.0f - Kx - 3.0f * 12.0f) / 4.0f;
	const int Hours = static_cast<int>(std::floor(Age / 60.0));
	const bool Cut = V.F == video::Format::Short;
	struct Tile
	{
		std::string Label, Value, Sub, Note;
		Color Col;
	};
	std::vector<Tile> Tiles = {
		{"VIEWS", RrCount(St.Views), "", "typical " + RrCount(U.TypViews) + " at " + std::to_string(Hours) + " h", Color{0, 0, 0, 0}},
		{Cut ? "STAYED TO WATCH" : "CTR SO FAR", RrPct(St.Ctr48), "", U.TypCtr > 0.0 ? "typical " + RrPct(U.TypCtr) : std::string(), Color{0, 0, 0, 0}},
		{"AVD", RrClock(St.AvdMin), "", U.TypAvd > 0.0 ? "typical " + RrClock(U.TypAvd) : std::string(), Color{0, 0, 0, 0}},
		{"LIKES \xC2\xB7 DISLIKES", RrCount(St.Likes) + " \xC2\xB7 " + RrCount(St.Dislikes), "", RrDislikesPrivate, Color{0, 0, 0, 0}},
		{"RANKING", std::to_string(U.LaunchRank) + " OF " + std::to_string(U.LaunchOf), "", "your last " + std::to_string(U.LaunchOf) + " uploads at " + std::to_string(Hours) + " h",
			Color{0, 0, 0, 0}},
		{Cut ? "SHOWN IN FEED" : "IMPRESSIONS", RrShort(St.Impressions), "", "reported", Color{0, 0, 0, 0}},
		{Partner ? "EST. REVENUE" : "WOULD HAVE EARNED", RrCents(static_cast<double>(Partner ? St.EarnedCents : St.WouldEarnCents)), "", Partner ? "RPM " + Money(static_cast<Chips>(std::llround(St.Rpm * 100.0))) : std::string("not paid"), RrCash},
		{"SUBSCRIBERS", "+" + RrCount(St.SubsNet), "", RrCount(St.SubsLost) + " lost", Color{0, 0, 0, 0}},
	};
	for (size_t K = 0; K < Tiles.size(); ++K)
	{
		chart::KpiCard Kc;
		Kc.Label = Tiles[K].Label;
		Kc.Value = Tiles[K].Value;
		Kc.Note = Tiles[K].Note;
		Kc.ValueCol = Tiles[K].Col;
		chart::KpiOpts Ko;
		Ko.ValueSize = 26.0f;
		Ko.NoteSize = 11.5f;
		chart::Kpi(F.C, {Kx + Nf(K % 4) * (Kw + 12.0f), 124.0f + Nf(K / 4) * 92.0f, Kw, 84.0f}, Kc, T, Ko);
	}
	// The lines that tell what's pushing it: the carry (R1), Live Drop (R3), "would have earned" before Partner (§4.5.8).
	float Lx = 20.0f;
	const float Ly = 318.0f;
	auto Strip = [&](const std::string& Line, const Color& Col, bool Solid) {
		const float W = std::min(NetW - 20.0f - Lx, F.UI.Measure(Line, 13.0f, 800) + 40.0f);
		if (W < 80.0f)
		{
			return;
		}
		F.C.FillRoundRect({Lx, Ly, W, 30.0f}, 8.0f, Solid ? Col : NetA(Col, 0.16f));
		F.UI.Text(Line, Lx + 14.0f, Ly + 20.0f, Ts(13.0f, 800, Solid ? Hex(videoart::Ink) : Col, Align::Left, Baseline::Alphabetic, false, W - 28.0f));
		Lx += W + 10.0f;
	};
	if (St.MomCAtUpload > 1.1)
	{
		Strip(video::valgo::LaunchCarry(St.MomCAtUpload), RrGold, true);
	}
	if (St.LiveDropAvg > 0)
	{
		Strip(video::valgo::LiveDropLine(St.LiveDropAvg), RrGood, false);
	}
	else if (!St.LiveDrop && Age < video::valgo::VaDropWindow)
	{
		Strip(video::valgo::LiveDropHint(), RrCream, false);
	}
	if (!Partner && St.WouldEarnCents > 0)
	{
		Strip(video::valgo::WouldEarnLine(St.WouldEarnCents), RrCash, false);
	}
	// Views an hour since the drop, against the typical upload's (dashed).
	const Rect Cr{20.0f, 362.0f, 1000.0f, 300.0f};
	RrCard(F, Cr, "VIEWS AN HOUR SINCE THE DROP", "dashed: your typical upload");
	{
		chart::BarSet B;
		for (const double H : U.LaunchHours)
		{
			B.V.push_back(std::isnan(H) ? 0.0 : H);
		}
		B.Col = RrGold;
		chart::BarOpts O;
		O.Gap = 0.3f;
		O.X.Labels.assign(48, "");
		O.X.Labels[0] = "drop";
		O.X.Labels[23] = "24 h";
		O.X.Labels[47] = "48 h";
		double Top = 1.0;
		for (size_t K = 0; K < 48; ++K)
		{
			Top = std::max(Top, std::max(B.V[K], U.LaunchTypical[K]));
		}
		O.YMax = Top;
		O.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
		const chart::BarOut Bo = chart::Bars(F.C, {Cr.X + 10.0f, Cr.Y + 40.0f, Cr.W - 20.0f, Cr.H - 50.0f}, {B}, O, T);
		std::vector<Vec2> Typ;
		for (int K = 0; K < Bo.Slots && K < 48; ++K)
		{
			Typ.push_back({Bo.Plot.X + (Nf(K) + 0.5f) / Nf(Bo.Slots) * Bo.Plot.W, Bo.Ys.Map(U.LaunchTypical[static_cast<size_t>(K)], Bo.Plot.Y + Bo.Plot.H, Bo.Plot.Y)});
		}
		if (Typ.size() >= 2)
		{
			F.C.StrokeDashed(Typ, NetA(RrCream, 0.55f), 1.6f, 5.0f, 4.0f);
		}
		if (Bo.Hover >= 0 && Bo.Hover < 48)
		{
			const size_t H = static_cast<size_t>(Bo.Hover);
			chart::Tooltip(F.C, {Bo.HoverSlot.X + Bo.HoverSlot.W / 2.0f, Bo.HoverSlot.Y}, "hour " + std::to_string(Bo.Hover + 1),
				{{"Views", std::isnan(U.LaunchHours[H]) ? std::string("\xE2\x80\x94") : RrCount(U.LaunchHours[H]), RrGold, Color{0, 0, 0, 0}},
					{"Typical", RrCount(U.LaunchTypical[H]), NetA(RrCream, 0.55f), Color{0, 0, 0, 0}}},
				T, Cr);
		}
	}
	// The Duel so far (§4.6.8): each contender's realized CTR and share of watch time; or what Rerun did with it.
	const Rect Dr{1036.0f, 362.0f, NetW - 20.0f - 1036.0f, 300.0f};
	if (V.X.Duels >= 2)
	{
		RrCard(F, Dr, "THUMBNAIL DUEL", St.DuelWin >= 0 ? "decided on watch time" : "so far");
		for (int K = 0; K < V.X.Duels && K < 3; ++K)
		{
			const float Ry = Dr.Y + 44.0f + Nf(K) * 82.0f;
			const Rect O{Dr.X + 18.0f, Ry, RrOptW, RrOptH};
			F.C.FillRoundRect(O, 6.0f, RrGround);
			RrSpecThumb(F, O, V.X.Spec[static_cast<size_t>(K)], V.Th);
			const bool Won = St.DuelWin == K;
			F.C.StrokeRoundRect(O, 6.0f, Won ? RrGold : RrLine, Won ? 2.5f : 1.0f);
			const char Letter[2] = {static_cast<char>('A' + K), '\0'};
			F.UI.Text(Letter, O.X + O.W + 18.0f, Ry + 28.0f, Ts(18.0f, 900, Won ? RrGold : RrCream));
			F.UI.Text(RrPct(St.DuelCtr[static_cast<size_t>(K)]) + " CTR", O.X + O.W + 44.0f, Ry + 28.0f, Ts(15.0f, 800, RrCream));
			F.UI.Text(RrPct(St.DuelShare[static_cast<size_t>(K)], 0) + " of watch time", O.X + O.W + 44.0f, Ry + 50.0f, Ts(12.5f, 600, RrMuted));
			if (Won)
			{
				RrTag(F.C, "WON", Dr.X + Dr.W - 66.0f, Ry + 14.0f, RrGold, true, 10.0f);
			}
		}
	}
	else
	{
		RrCard(F, Dr, "WHAT RERUN DID WITH IT");
		float Ly2 = Dr.Y + 52.0f;
		for (const auto& L : St.Log)
		{
			Ly2 = NetParagraph(F.C, L.second, Dr.X + 18.0f, Ly2, Dr.W - 36.0f, 13.0f, 600, RrCream, 18.0f, 2) + 8.0f;
			if (Ly2 > Dr.Y + Dr.H - 20.0f)
			{
				break;
			}
		}
	}
	// LAUNCH ACTIONS (§4.6.9): each once, each costing an evening's time; their states; the hint never promises a number.
	const Rect Ar{20.0f, 678.0f, NetW - 40.0f, 140.0f};
	RrCard(F, Ar, "LAUNCH ACTIONS");
	F.UI.Text(RrEarlyHint, Ar.X + Ar.W - 18.0f, Ar.Y + 26.0f, Ts(12.5f, 700, RrMuted, Align::Right));
	struct Act
	{
		video::Launch A;
		const char* Label;
		const char* Cost;
	};
	const Act Acts[4] = {{video::Launch::Cut, "Make a Cut", "20 min"}, {video::Launch::Kast, "Go live on Kast", ""}, {video::Launch::Teaser, "Post a teaser", "10 min"},
		{video::Launch::Reply, "Reply round", "20 min"}};
	const float Aw = (Ar.W - 36.0f - 3.0f * 16.0f) / 4.0f;
	for (int K = 0; K < 4; ++K)
	{
		const Act& A = Acts[K];
		const bool Done = (V.X.Launch & (1u << static_cast<int>(A.A))) != 0;
		const Rect B{Ar.X + 18.0f + Nf(K) * (Aw + 16.0f), Ar.Y + 44.0f, Aw, 48.0f};
		std::string State = Done ? std::string("done") : std::string("not yet");
		bool Can = !Done;
		if (A.A == video::Launch::Kast)
		{
			State = St.LiveDropAvg > 0 ? video::valgo::LiveDropLine(St.LiveDropAvg) : Done ? std::string("done") : F.S.Stream.Live ? std::string(RrLiveNow)
				: Age < video::valgo::VaDropWindow ? "go live within " + RrUntil(video::valgo::VaDropWindow - Age) : std::string("too late for a push");
			Can = !Done && Age < video::valgo::VaDropWindow;
		}
		const std::string Label = std::string(A.Label) + (A.Cost[0] != '\0' ? std::string("  ") + A.Cost : std::string());
		if (RrSegment(F, std::string("rract") + A.Label, B, Label, std::string(), Done) && Can)
		{
			if (A.A == video::Launch::Kast && !F.S.Stream.Live)
			{
				F.RL.OpenApp(RiverLine::App::Kast, F.Now); // the stream itself is the action (Live Drop)
			}
			else
			{
				const std::string Why = F.S.LaunchAction(V.Id, A.A);
				if (!Why.empty())
				{
					U.Note = Why;
					U.NoteBad = true;
					U.NoteAt = F.Now;
				}
			}
		}
		F.UI.Text(State, B.X + B.W / 2.0f, B.Y + B.H + 20.0f, Ts(12.0f, 700, Done ? RrGood : RrMuted, Align::Center, Baseline::Alphabetic, false, B.W));
	}
	// LATEST COMMENTS (generated, read-only: the library's; their count is the core's).
	const Rect Cm{20.0f, 832.0f, NetW - 40.0f, 110.0f};
	RrCard(F, Cm, "LATEST COMMENTS", RrCount(St.Comments) + (std::floor(St.Comments) == 1.0 ? " comment" : " comments"));
	const int Count = static_cast<int>(std::floor(St.Comments));
	const std::vector<video::Comment> Latest = Count > 0 ? video::CommentsOf(V, F.World, std::max(0, Count - 3), 3) : std::vector<video::Comment>();
	if (Latest.empty())
	{
		F.UI.Text(RrNoComments, Cm.X + 18.0f, Cm.Y + 64.0f, Ts(13.5f, 600, RrMuted));
	}
	for (size_t K = 0; K < Latest.size() && K < 3; ++K)
	{
		const float Cx = Cm.X + 18.0f + Nf(K) * ((Cm.W - 36.0f) / 3.0f);
		F.UI.Text(Latest[K].Author, Cx, Cm.Y + 56.0f, Ts(12.0f, 800, RrGold, Align::Left, Baseline::Alphabetic, false, (Cm.W - 60.0f) / 3.0f));
		NetParagraph(F.C, "\xE2\x80\x9C" + Latest[K].Text + "\xE2\x80\x9D", Cx, Cm.Y + 76.0f, (Cm.W - 60.0f) / 3.0f, 13.0f, 600, RrCream, 17.0f, 2);
	}
}

/** The day after a verdict: its one-line result at the top of Studio (the bell's sentence, with the label in its colour). */
float RrVerdictLine(RrFrame& F)
{
	using namespace rlnet_detail;
	const video::Video* Best = nullptr;
	for (const video::Video& V : F.S.Life.Video.Videos)
	{
		if (!V.X.On || V.X.Verdict < 0 || RrIsGrant(V))
		{
			continue;
		}
		const double At = static_cast<double>(video::valgo::VerdictDayOf(V)) * net::MinutesPerDay;
		if (F.World >= At && F.World < At + net::MinutesPerDay && (!Best || V.At > Best->At))
		{
			Best = &V;
		}
	}
	if (!Best)
	{
		return 0.0f;
	}
	const Rect R{RrEdX, 68.0f, NetW - 20.0f - RrEdX, 38.0f};
	F.C.FillRoundRect(R, 10.0f, RrPanel);
	const int Label = Best->X.Verdict;
	const float Tw = RrTag(F.C, NetUpper(RrVerdictName(Label)), R.X + 10.0f, R.Y + 9.0f, RrVerdictColor(Label), true, 10.0f);
	std::string Line = video::valgo::BellVerdict(Best->Title, Label);
	const video::VideoStats& St = RrStats(F, *Best);
	if (St.MomCAtUpload > 1.1 && !video::valgo::VerdictDespite(Label).empty())
	{
		Line = video::valgo::VerdictDespite(Label) + ". " + Line;
	}
	const Ui::ClickState Cs = F.UI.Clickable("rrverdictline", R, F.Live);
	F.UI.Text(Line, R.X + Tw + 22.0f, R.Y + 24.0f, Ts(13.5f, 700, Cs.Hover ? RrGold : RrCream, Align::Left, Baseline::Alphabetic, false, R.W - Tw - 40.0f));
	if (Cs.Clicked)
	{
		RrOpenVideo(F, Best->Id);
	}
	return 46.0f;
}

// ------------------------------------------------------------------ Studio

/**
 * A visit's first frame (L9, update 3 §4.1): Studio opens on IDEAS when a new or trending idea waits (a ready idea the IDEAS
 * list hasn't shown since it got ready, or a trending one not yet seen), else on MOMENTS. A caller that means a tab says so
 * (OpenTab: Research's links open IDEAS, Kast's Cut highlights MOMENTS). The ideas that were ready at the first look aren't
 * new (a load's, as VideoPrime's twelve hours for moments); the new ones wear a NEW chip for the visit.
 */
void RrStudioVisit(RrFrame& F)
{
	VideoUi& U = F.U;
	const std::vector<video::IdeaCard>& Ideas = RrIdeas(F);
	if (!U.ReadyPrimed)
	{
		U.ReadyPrimed = true;
		for (const video::IdeaCard& I : Ideas)
		{
			if (I.Ready)
			{
				U.ReadySeen.insert(I.Id);
			}
		}
	}
	U.NewIdeas.clear();
	bool Waiting = false;
	for (const video::IdeaCard& I : Ideas)
	{
		if (!I.Ready)
		{
			continue;
		}
		if (U.ReadySeen.count(I.Id) == 0)
		{
			U.NewIdeas.insert(I.Id);
			Waiting = true;
		}
		Waiting = Waiting || (I.Trending && U.SeenIdeas.count(I.Id) == 0);
	}
	const int Tab = U.OpenTab >= 0 ? U.OpenTab : U.Tonight ? 0 : Waiting ? 1 : 0;
	U.OpenTab = -1;
	if (Tab != U.ListTab)
	{
		U.ListTab = Tab;
		U.ListGoal = U.ListScroll = 0.0f;
	}
}

void RrStudio(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const std::vector<video::Candidate>& Open = F.S.Life.Video.Open;
	if (!U.StudioShown)
	{
		RrStudioVisit(F);
		U.StudioShown = true;
	}
	// For 48 world hours after a drop, the launch card (a new drop shows its card even if the editor was up for the last).
	const video::Video* Launch = RrLaunchVideo(F.S, F.World);
	if (Launch && Launch->Id != U.LaunchSeen)
	{
		U.LaunchSeen = Launch->Id;
		// Kast's Cut highlights (VideoOpenTonight, VideoOpenCut) came to cut: the editor stays up, the card one link away.
		if (!U.Tonight)
		{
			U.LaunchHidden = false;
		}
	}
	if (Launch && !U.LaunchHidden)
	{
		RrLaunchCard(F, *Launch);
		return;
	}
	if (Open.empty() && RrIdeas(F).empty() && F.S.Life.Video.X.Challenges.empty())
	{
		// The empty state (§4.8), under a day's verdict line and the way back to a running launch.
		const float Cx = NetW / 2.0f;
		RrVerdictLine(F);
		videoart::Logo(F.C, {Cx - 44.0f, 300.0f, 88.0f, 88.0f});
		F.UI.Text(RrStudioEmpty, Cx, 448.0f, Ts(26.0f, 900, RrCream, Align::Center));
		F.UI.Text(RrStudioEmptyMore, Cx, 482.0f, Ts(16.0f, 500, RrMuted, Align::Center, Baseline::Alphabetic, false, 900.0f));
		if (Launch && RrButton(F, "rrbacklaunch", {Cx - 140.0f, 520.0f, 280.0f, 44.0f}, RrBackToLaunch, true))
		{
			U.LaunchHidden = false;
		}
		return;
	}
	const std::vector<video::Candidate> Listed = RrListed(F.S, U);
	// The pick keeps only moments still open (posted from the console, gone stale); emptied that way, or never picked by
	// hand, it takes the best one listed.
	const size_t Had = U.Pick.size();
	U.Pick.erase(std::remove_if(U.Pick.begin(), U.Pick.end(), [&Open](const std::string& Id) { return RrFind(Open, Id) == nullptr; }), U.Pick.end());
	if (Had > 0 && U.Pick.empty())
	{
		U.Touched = false;
	}
	if (U.Pick.empty() && !U.Touched && !Listed.empty())
	{
		U.Pick = {Listed.front().Id};
		RrSetFormat(U, Open, U.F);
	}
	if (U.LinkNewest)
	{
		// Kast's "Make a Cut and link it": the Cut's Next-up card is the newest kept long video.
		U.LinkNewest = false;
		const video::Video* Newest = nullptr;
		for (const video::Video& V : F.S.Life.Video.Videos)
		{
			Newest = V.F != video::Format::Short && !RrIsGrant(V) && (!Newest || V.At > Newest->At) ? &V : Newest;
		}
		U.NextUp = Newest ? Newest->Id : std::string();
	}
	RrListTabs(F, Listed);
	if (U.ListTab == 1)
	{
		RrIdeasList(F);
	}
	else if (U.ListTab == 2)
	{
		RrSeriesList(F);
	}
	else
	{
		RrCandidates(F, Listed, Open);
	}
	const float T = RrVerdictLine(F);
	RrEditor(F, Open, T);
	RrSide(F, Open, T);
	if (Launch)
	{
		// The launch still runs: back to its card from the editor.
		const std::string Back = std::string(RrBackToLaunch) + " \xC2\xB7 " + RrUntil(static_cast<double>(video::valgo::VerdictDayOf(*Launch)) * net::MinutesPerDay - F.World);
		if (RrLink(F, "rrbacklaunch", Back, RrEdX + RrEdW, 84.0f + T, 11.5f, RrGold, Align::Right))
		{
			U.LaunchHidden = false;
		}
	}
	if (!U.Note.empty() && F.Now - U.NoteAt < 6.0)
	{
		const float Tw = std::min(NetW - 80.0f, F.UI.Measure(U.Note, 14.0f, 700) + 32.0f);
		const Rect Tr{NetW / 2.0f - Tw / 2.0f, VideoH - 64.0f, Tw, 40.0f};
		F.C.FillRoundRect(Tr, 12.0f, RrGround);
		F.C.StrokeRoundRect(Tr, 12.0f, U.NoteBad ? RrWarn : RrGold, 1.5f);
		F.UI.Text(U.Note, Tr.X + Tr.W / 2.0f, Tr.Y + 21.0f, Ts(14.0f, 700, RrCream, Align::Center, Baseline::Middle, false, Tw - 24.0f));
	}
}
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss
