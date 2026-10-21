// Rerun's Studio (update 2 SPEC §4.5, §4.8; moved here unchanged from RiverLineVideo.cpp by update 3's split, SPEC §7.1,
// §8.2), ui-rerun's file: the candidates on the left, best first, each with its kind's glyph, what happened, when, its chips
// and the days it has left; the editor in the middle (the format, the title style with the real title, the thumbnail frame
// at 640 x 360, the edit's time and energy, PUBLISH through Session::Publish with the honest reason first); on the right,
// what each choice does (video::Advice) and the video's tile as the Channel will show it. Rerun 2.0's IDEAS, MOMENTS,
// SERIES, packaging, the Duel, the settings, Next-up cards and the launch card land here (G3). Shares the look and side
// state through RiverLineVideoShared.h.
#include "RiverLineVideoShared.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlvideo_detail
{
// ------------------------------------------------------------------ STUDIO

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
	const std::string Where = RrWhereChip(Cd);
	if (!Where.empty())
	{
		Tags.push_back({Where, RrMuted});
	}
	for (const auto& Ch : Tags)
	{
		const float W = F.UI.Measure(Ch.first, 9.5f, 700) + 9.5f * 1.3f;
		if (X + W > Right - LeftW - 8.0f)
		{
			break;
		}
		X += NetPill(F.C, Ch.first, X, R.Y + 15.0f, Ch.second, false, 9.5f) + 5.0f;
	}
	// Line 2: what happened (a long event name gives way to the pot and who). Line 3: when.
	std::string Tail;
	std::string Head = RrFact(Cd, Open, Tail);
	if (!Tail.empty())
	{
		Head = RrShorten(F.C, Head, 15.0f, 700, std::max(80.0f, Right - X0 - F.UI.Measure(Tail, 15.0f, 700)));
	}
	F.UI.Text(Head + Tail, X0, R.Y + 51.0f, Ts(15.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Right - X0));
	F.UI.Text(RrWhen(Cd.At), X0, R.Y + 71.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Right - X0));
	F.C.SetAlpha(A0);
}

void RrCandidates(RrFrame& F, const std::vector<video::Candidate>& Listed, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	NetSpaced(F.C, "CANDIDATES", RrListX + 2.0f, 100.0f, 10.0f, 900, RrMuted, 1.4f);
	const float Right = RrListX + RrListW;
	if (U.Tonight)
	{
		const std::string Chip = "TONIGHT'S STREAM";
		const float Cw = F.UI.Measure(Chip, 10.0f, 700) + 13.0f;
		const float ShowW = F.UI.Measure("Show all", 12.0f, 800);
		NetPill(F.C, Chip, Right - ShowW - 14.0f - Cw, 86.0f, RrGold, false, 10.0f);
		if (RrLink(F, "rrshowall", "Show all", Right, 100.0f, 12.0f, RrGold, Align::Right))
		{
			U.Tonight = false;
			U.ListGoal = U.ListScroll = 0.0f;
		}
	}
	else
	{
		F.UI.Text(std::to_string(Listed.size()) + " open", Right, 100.0f, Ts(12.0f, 700, RrMuted, Align::Right));
	}
	const Rect Area{RrListX, RrListTop, RrListW, VideoH - 20.0f - RrListTop};
	if (Listed.empty())
	{
		F.UI.Text(RrTonightGone, Area.X + Area.W / 2.0f, Area.Y + 60.0f, Ts(15.0f, 700, RrMuted, Align::Center, Baseline::Alphabetic, false, Area.W - 20.0f));
		return;
	}
	const float Content = Nf(Listed.size()) * (RrRowH + RrRowGap) - RrRowGap;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.ListGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * (RrRowH + RrRowGap) * 1.5f;
	}
	U.ListGoal = std::clamp(U.ListGoal, 0.0f, MaxScroll);
	U.ListScroll += (U.ListGoal - U.ListScroll) * NetFollow(F.Dt, 18.0);
	U.ListScroll = std::clamp(U.ListScroll, 0.0f, MaxScroll);
	if (std::fabs(U.ListGoal - U.ListScroll) < 0.5f)
	{
		U.ListScroll = U.ListGoal;
	}
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < Listed.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * (RrRowH + RrRowGap) - U.ListScroll;
		if (Y + RrRowH < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		RrCandidateRow(F, Listed[I], Open, {Area.X, Y, Area.W, RrRowH}, PointerIn);
	}
	F.C.PopClip();
	if (MaxScroll > 0.0f)
	{
		// A thin thumb on the right edge, so the player knows there's more.
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * (U.ListScroll / MaxScroll);
		F.C.FillRoundRect({Area.X + Area.W + 6.0f, Ty, 4.0f, Th}, 2.0f, NetA(RrCream, 0.25f));
	}
}

void RrEditor(RrFrame& F, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const float X = RrEdX, W = RrEdW;
	// FORMAT.
	NetSpaced(F.C, "FORMAT", X + 2.0f, 100.0f, 10.0f, 900, RrMuted, 1.4f);
	for (int K = 0; K < 4; ++K)
	{
		const video::Format Fm = static_cast<video::Format>(K);
		const float Bw = (W - 30.0f) / 4.0f;
		if (RrSegment(F, std::string("rrfmt") + video::FormatName(Fm), {X + Nf(K) * (Bw + 10.0f), 112.0f, Bw, 56.0f}, video::FormatName(Fm), RrFormatSub(Fm), U.F == Fm))
		{
			RrSetFormat(U, Open, Fm);
		}
	}
	// TITLE: the style, and the real title it makes from the moment's own facts.
	NetSpaced(F.C, "TITLE", X + 2.0f, 194.0f, 10.0f, 900, RrMuted, 1.4f);
	for (int K = 0; K < 5; ++K)
	{
		const video::TitleStyle Ts_ = static_cast<video::TitleStyle>(K);
		const float Bw = (W - 32.0f) / 5.0f;
		if (RrSegment(F, std::string("rrtitle") + video::TitleStyleName(Ts_), {X + Nf(K) * (Bw + 8.0f), 206.0f, Bw, 36.0f}, video::TitleStyleName(Ts_), std::string(), U.T == Ts_))
		{
			U.T = Ts_;
		}
	}
	const video::Choice Ch = RrChoice(U);
	const std::string Title = U.Pick.empty() ? std::string() : video::Title(Ch, Open, 0);
	const Rect Box{X, 252.0f, W, 68.0f};
	F.C.FillRoundRect(Box, 12.0f, RrPanel);
	F.C.StrokeRoundRect(Box, 12.0f, RrLine, 1.0f);
	if (Title.empty())
	{
		F.UI.Text(RrPickFirst, Box.X + 20.0f, Box.Y + 41.0f, Ts(16.0f, 600, RrMuted));
	}
	else
	{
		const std::string Quoted = "\xE2\x80\x9C" + Title + "\xE2\x80\x9D";
		const std::vector<std::string> Lines = NetWrap(F.C, Quoted, Box.W - 40.0f, 19.0f, 800);
		if (Lines.size() <= 1)
		{
			F.UI.Text(Quoted, Box.X + 20.0f, Box.Y + 42.0f, Ts(19.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, Box.W - 40.0f));
		}
		else
		{
			NetParagraph(F.C, Quoted, Box.X + 20.0f, Box.Y + 29.0f, Box.W - 40.0f, 17.0f, 800, RrCream, 22.0f, 2);
		}
	}
	// THUMBNAIL: the frame, drawn for real at 640 x 360.
	NetSpaced(F.C, "THUMBNAIL", X + 2.0f, 342.0f, 10.0f, 900, RrMuted, 1.4f);
	for (int K = 0; K < 5; ++K)
	{
		const video::Frame Fm = static_cast<video::Frame>(K);
		const float Bw = (W - 32.0f) / 5.0f;
		if (RrSegment(F, std::string("rrframe") + video::FrameName(Fm), {X + Nf(K) * (Bw + 8.0f), 354.0f, Bw, 34.0f}, video::FrameName(Fm), std::string(), U.Fr == Fm))
		{
			U.Fr = Fm;
		}
	}
	const Rect Th{X + (W - RrThumbW) / 2.0f, 400.0f, RrThumbW, RrThumbH};
	F.C.FillRoundRect({Th.X - 4.0f, Th.Y - 4.0f, Th.W + 8.0f, Th.H + 8.0f}, 12.0f, RrPanel);
	if (U.Pick.empty())
	{
		F.C.FillRect(Th, RrGround);
		videoart::Logo(F.C, {Th.X + Th.W / 2.0f - 40.0f, Th.Y + Th.H / 2.0f - 40.0f, 80.0f, 80.0f});
	}
	else
	{
		F.C.PushClip(Th);
		videoart::Thumbnail(F.C, Th, video::ThumbFor(Ch, Open), &F.S.Person);
		F.C.PopClip();
	}
	F.C.StrokeRoundRect({Th.X - 4.0f, Th.Y - 4.0f, Th.W + 8.0f, Th.H + 8.0f}, 12.0f, RrLine, 1.0f);
	// The edit: like a shift, the clock moves and the energy goes (§4.5); then PUBLISH.
	const double Minutes = video::EditMinutes(U.F);
	const std::string Edit = "Edit: " + RrEditTime(Minutes) + " \xC2\xB7 " + std::to_string(std::lround(video::EditEnergy(U.F))) + " energy";
	F.UI.Text(Edit, X, 800.0f, Ts(18.0f, 800, RrCream));
	F.UI.Text("It goes up at " + net::TimeLabel(F.World + Minutes) + ".", X, 824.0f, Ts(13.0f, 600, RrMuted));
	const std::vector<std::string> Why = RrPublishWhy(F.S, Ch);
	const Rect Btn{X + W - 220.0f, 776.0f, 220.0f, 56.0f};
	// Update 2 (§5.7): a video of an online table (any site's) is a strike under a deal that plays only on another one. PUBLISH
	// says so first, and the strike lands only on the second press.
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
	if (!Strike.empty() && Why.empty())
	{
		// Under the button: above it the line sat on the thumbnail's frame (it ends at 764).
		F.UI.Text(Armed ? std::string(rlsites_detail::SitesPublishArmed) : Strike, X + W, 856.0f, Ts(14.0f, 800, RrWarn, Align::Right, Baseline::Alphabetic, false, W));
	}
	const bool Pressed = RrButton(F, "rrpublish", Btn, Armed ? std::string("PUBLISH ANYWAY") : std::string("PUBLISH"), Why.empty());
	if (Pressed && !Strike.empty() && !Armed)
	{
		U.StrikeArmed = PickKey; // the first press only warns
	}
	else if (Pressed)
	{
		U.StrikeArmed.clear();
		const std::string Refused = F.S.Publish(Ch);
		if (Refused.empty())
		{
			// The edit runs as a time skip; the video's on the Channel the moment it's done.
			U.Pick.clear();
			U.Touched = false;
			U.Page = 1;
			U.PageAt = F.Now;
			U.GridGoal = U.GridScroll = 0.0f;
			U.Note.clear();
		}
		else
		{
			U.Note = Refused;
			U.NoteBad = true;
			U.NoteAt = F.Now;
		}
	}
	float Ry = 860.0f;
	for (const std::string& Line : Why)
	{
		Ry = NetParagraph(F.C, Line, X, Ry, W, 14.0f, 600, RrWarn, 19.0f, 2) + 4.0f;
	}
	if (!U.Note.empty() && F.Now - U.NoteAt < 6.0)
	{
		const float A0 = F.C.GetAlpha();
		F.C.SetAlpha(A0 * Nf(std::min(1.0, (6.0 - (F.Now - U.NoteAt)) / 0.6)));
		NetParagraph(F.C, U.Note, X, std::max(Ry + 2.0f, 900.0f), W, 14.0f, 700, U.NoteBad ? RrWarn : RrGold, 19.0f, 2);
		F.C.SetAlpha(A0);
	}
}

void RrSide(RrFrame& F, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	const VideoUi& U = F.U;
	const float X = RrSideX, W = RrSideW;
	NetSpaced(F.C, "WHAT IT DOES", X + 2.0f, 100.0f, 10.0f, 900, RrMuted, 1.4f);
	const video::Choice Ch = RrChoice(U);
	// Ads come up only once they run (a Partner, past the review): before that they're no reason to pick a format.
	const video::Channel& Rerun = F.S.Life.Video;
	std::vector<std::string> Lines = video::Advice(Ch, Open, Rerun.PartnerAt >= 0.0 && Rerun.PartnerAt <= F.World);
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
	float Y = 130.0f;
	for (const std::string& L : Lines)
	{
		F.C.FillCircle(X + 6.0f, Y - 5.0f, 3.5f, RrGold);
		Y = NetParagraph(F.C, L, X + 20.0f, Y, W - 20.0f, 15.0f, 500, RrCream, 21.0f, 4) + 12.0f;
	}
	if (U.Pick.empty())
	{
		return;
	}
	// The tile as it'll sit on the Channel (§4.5: the preview shows it there).
	const float Ty = std::max(Y + 28.0f, 330.0f);
	NetSpaced(F.C, "ON YOUR CHANNEL", X + 2.0f, Ty, 10.0f, 900, RrMuted, 1.4f);
	video::Video V;
	V.Title = video::Title(Ch, Open, 0);
	V.Th = video::ThumbFor(Ch, Open);
	V.F = U.F;
	V.LengthMin = RrLengthMin(U.F);
	V.At = F.World;
	RrTile(F, {X, Ty + 14.0f, 300.0f, 240.0f}, V, false, 0.0);
}

void RrStudio(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const std::vector<video::Candidate>& Open = F.S.Life.Video.Open;
	if (Open.empty())
	{
		// The empty state (§4.8).
		const float Cx = NetW / 2.0f;
		videoart::Logo(F.C, {Cx - 44.0f, 300.0f, 88.0f, 88.0f});
		F.UI.Text(RrStudioEmpty, Cx, 448.0f, Ts(26.0f, 900, RrCream, Align::Center));
		F.UI.Text(RrStudioEmptyMore, Cx, 482.0f, Ts(16.0f, 500, RrMuted, Align::Center, Baseline::Alphabetic, false, 900.0f));
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
	RrCandidates(F, Listed, Open);
	RrEditor(F, Open);
	RrSide(F, Open);
}
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss
