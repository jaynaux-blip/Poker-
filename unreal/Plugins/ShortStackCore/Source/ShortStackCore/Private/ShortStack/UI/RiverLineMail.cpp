#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/SponsorArt.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <cmath>
#include <map>

// The Mail app (next_segment_spec §3.9): the shared inbox (life::State::Inbox) of the brands and the staking outfits,
// the laptop's eighth app. core-sponsor.
//
// Left, 360 wide: the threads, newest first, each with the brand's mark, who wrote, the subject, when, the state pill
// (OFFER, ACTIVE, STRIKE, NOTICE, ENDED) and an unread dot. Right: the thread, newest letter first, and under it the
// contract card in three columns (what you get; what they ask, each with this period's progress; the fine print) with
// its buttons: Sign, Counter (five levers, each with the rep's mood before it's sent) and Decline on an offer; Give
// notice (behind a confirm card) and Ask for a raise on a running deal. A conflict shows in red over Sign. Lantern Row's
// and Finch School's letters (Source "stake") link to the Backing page; Rerun's (Source "video": the review, the Partner
// welcome, the statements; update 2 §4.9) wear its mark and open the Channel page. Opening a thread marks its letters read.
//
// The app's own state (the thread picked, the panel open) lives here, by RiverLine: its header is Phase A's.

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlmail_detail
{
const float MailH = 962.0f; // above the taskbar
const float ListW = 380.0f;
const float PaneX = 400.0f;
const float PaneW = NetW - PaneX - 20.0f;

const Color Paper = Hex(0xf5f2ec);
const Color Side = Hex(0xebe6dc);
const Color Ink = Hex(0x1f2430);
const Color Muted = Hex(0x6b7280);
const Color Rule = Hex(0xd9d3c7);
const Color Navy = Hex(0x1f2a3a);
const Color Blue = Hex(0x2f5d9e);
const Color Red = Hex(0xc0392b);
const Color Green = Hex(0x2f8f5b);

/** Rerun's letters (Source "video", update 2 §4.9): under the name, and the button to the Channel page (§4.8's page 1). */
constexpr const char* RerunDesk = "Partner Program";
constexpr const char* RerunButton = "Open your channel";
constexpr int RerunChannelPage = 1;

struct MailState
{
	std::string Pick;   // the thread shown ("" for the newest)
	int Panel = 0;      // 0 none, 1 the counter's levers, 2 notice's confirm
	std::string Shown;  // the thread drawn last frame (a panel closes when another thread comes up)
	double PickAt = -100.0;
};

/** The app's state, by laptop (RiverLine's header is Phase A's, so it lives here). */
std::map<const RiverLine*, MailState>& MailStates()
{
	static std::map<const RiverLine*, MailState> ByLaptop;
	return ByLaptop;
}

MailState& StateOf(const RiverLine* Who)
{
	return MailStates()[Who];
}

/** A laptop going (~RiverLine): its Mail state goes with it. */
void ForgetMail(const RiverLine* Who)
{
	MailStates().erase(Who);
}

struct Thread
{
	std::string Id;
	std::string Source;
	std::string From;
	std::string Subject;
	double Last = -1.0e18;
	bool Unread = false;
	int Letters = 0;
	size_t Order = 0; // where its newest letter sits in the inbox (letters at the same minute: the later one is newer)
};

std::vector<Thread> ThreadsOf(const life::State& L)
{
	std::vector<Thread> Out;
	for (size_t I = 0; I < L.Inbox.size(); ++I)
	{
		const life::Letter& M = L.Inbox[I];
		auto It = std::find_if(Out.begin(), Out.end(), [&M](const Thread& T) { return T.Id == M.Thread && T.Source == M.Source; });
		if (It == Out.end())
		{
			Thread T;
			T.Id = M.Thread;
			T.Source = M.Source;
			Out.push_back(T);
			It = Out.end() - 1;
		}
		It->Letters += 1;
		It->Unread = It->Unread || !M.Read;
		if (M.At >= It->Last)
		{
			It->Last = M.At;
			It->Order = I;
			It->From = M.From;
			It->Subject = M.Subject;
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const Thread& A, const Thread& B) { return A.Last != B.Last ? A.Last > B.Last : A.Order > B.Order; });
	return Out;
}

Color PillColor(const std::string& P)
{
	if (P == "OFFER")
	{
		return Hex(0xc98a12);
	}
	if (P == "ACTIVE")
	{
		return Green;
	}
	if (P == "STRIKE")
	{
		return Red;
	}
	if (P == "NOTICE")
	{
		return Hex(0xd9731e);
	}
	return Hex(0x8a8f99);
}

std::string When(double At, double World)
{
	const int Day = net::DayOf(At);
	const int Today = net::DayOf(World);
	if (Day == Today)
	{
		return net::TimeLabel(At);
	}
	if (Day == Today - 1)
	{
		return "Yesterday";
	}
	return net::DateLabel(Day);
}

std::string Long(double At)
{
	const int Day = net::DayOf(At);
	return std::string(net::WeekdayName(Day, true)) + ", " + net::DateLabel(Day) + " \xC2\xB7 " + net::TimeLabel(At);
}

/** An outfit's round seal when the letter isn't a brand's (Lantern Row, Finch School): its initials in a ring. */
void Seal(Canvas& Cv, const std::string& Name, float Cx, float Cy, float R)
{
	std::string Initials;
	bool Start = true;
	for (const char Ch : Name)
	{
		if (Start && Ch >= 'A' && Ch <= 'Z' && Initials.size() < 2)
		{
			Initials.push_back(Ch);
		}
		Start = Ch == ' ';
	}
	Cv.FillCircle(Cx, Cy, R, Navy);
	Cv.StrokeEllipse(Cx, Cy, R * 0.82f, R * 0.82f, Hex(0xc9b37e), R * 0.08f);
	Cv.Text(Initials, Cx, Cy + 0.5f, Ts(R * 0.75f, 900, Hex(0xf5f2ec), Align::Center, Baseline::Middle));
}

const char* EndWords(sponsor::End Why)
{
	switch (Why)
	{
	case sponsor::End::Declined: return "You passed on it.";
	case sponsor::End::Withdrawn: return "They pulled the offer.";
	case sponsor::End::Expired: return "The offer ran out.";
	case sponsor::End::Walked: return "You gave notice. It's done.";
	case sponsor::End::Lapsed: return "The term ran its course.";
	case sponsor::End::ForCause: return "Ended for cause.";
	case sponsor::End::Conduct: return "Ended under the conduct clause.";
	case sponsor::End::NotRenewed: return "They didn't renew.";
	case sponsor::End::None: break;
	}
	return "It's over.";
}
} // namespace rlmail_detail

void RiverLine::MailApp(double Now)
{
	using namespace rlmail_detail;
	MailState& St = StateOf(this);
	life::State& L = S.Life;
	const std::vector<Thread> Threads = ThreadsOf(L);
	int Unread = 0;
	for (const Thread& T : Threads)
	{
		Unread += T.Unread ? 1 : 0;
	}
	C->FillRect({0.0f, 0.0f, NetW, MailH}, Paper);

	// The header: an envelope, the app's name, what's unread.
	C->FillRect({0.0f, 0.0f, NetW, 72.0f}, Navy);
	{
		const Rect Env{28.0f, 18.0f, 44.0f, 34.0f};
		C->FillRoundRect(Env, 5.0f, Hex(0xf5f2ec));
		C->StrokePolyline({{Env.X + 3.0f, Env.Y + 4.0f}, {Env.X + Env.W * 0.5f, Env.Y + Env.H * 0.6f}, {Env.X + Env.W - 3.0f, Env.Y + 4.0f}}, false, Navy, 3.0f, true);
	}
	const float Lw = UI.Text("Mail", 86.0f, 45.0f, Ts(24.0f, 900, Hex(0xffffff)));
	UI.Text(Unread > 0 ? std::to_string(Unread) + " unread" : std::string("All caught up"), 100.0f + Lw, 45.0f, Ts(15.0f, 600, Hex(0xaab6c8)));
	UI.Text(S.HeroName, NetW - 28.0f, 44.0f, Ts(14.0f, 600, Hex(0xc9d7ea), Align::Right));

	// The threads.
	C->FillRect({0.0f, 72.0f, ListW, MailH - 72.0f}, Side);
	C->FillRect({ListW, 72.0f, 1.0f, MailH - 72.0f}, Rule);
	UI.Text("Inbox", 24.0f, 108.0f, Ts(18.0f, 900, Ink));
	if (Threads.empty())
	{
		NetParagraph(*C, "No mail yet. Brands write when they've seen enough of you, and not before.", 24.0f, 140.0f, ListW - 48.0f, 14.0f, 500, Muted, 20.0f, 4);
	}
	std::string Pick = St.Pick;
	if (std::none_of(Threads.begin(), Threads.end(), [&Pick](const Thread& T) { return T.Id == Pick; }))
	{
		Pick = Threads.empty() ? std::string() : Threads.front().Id;
	}
	float Y = 124.0f;
	for (size_t I = 0; I < Threads.size() && Y + 86.0f <= MailH; ++I)
	{
		const Thread& T = Threads[I];
		const Rect Row{8.0f, Y, ListW - 16.0f, 82.0f};
		const Ui::ClickState Cs = Previewing ? Ui::ClickState() : UI.Clickable("mailthread" + T.Id, Row);
		const bool On = T.Id == Pick;
		if (On || Cs.Hover)
		{
			C->FillRoundRect(Row, 10.0f, On ? Hex(0xffffff) : Hex(0xf3efe7));
		}
		const sponsor::Contract* K = T.Source == "sponsor" ? sponsor::Find(L.Sponsors, T.Id) : nullptr;
		if (K)
		{
			sponsorart::Mark(*C, K->Brand, {Row.X + 14.0f, Row.Y + 16.0f, 44.0f, 44.0f});
		}
		else if (T.Source == "video")
		{
			videoart::Logo(*C, {Row.X + 14.0f, Row.Y + 16.0f, 44.0f, 44.0f});
		}
		else if (T.Source == "site")
		{
			// A poker site's letter (update 2 §1.7, §5.4): its logo.
			siteart::Logo(*C, {Row.X + 14.0f, Row.Y + 16.0f, 44.0f, 44.0f}, rlsites_detail::SitesOfMail(T.Source, T.Id, T.From));
		}
		else
		{
			Seal(*C, T.From, Row.X + 36.0f, Row.Y + 38.0f, 22.0f);
		}
		if (T.Unread)
		{
			C->FillCircle(Row.X + 5.0f, Row.Y + 38.0f, 4.0f, Blue);
		}
		const std::string Time = When(T.Last, World);
		const float Tw = UI.Measure(Time, 12.0f, 600);
		UI.Text(T.From, Row.X + 70.0f, Row.Y + 26.0f, Ts(15.0f, T.Unread ? 900 : 700, Ink, Align::Left, Baseline::Alphabetic, false, Row.W - 90.0f - Tw));
		UI.Text(Time, Row.X + Row.W - 12.0f, Row.Y + 26.0f, Ts(12.0f, 600, Muted, Align::Right));
		UI.Text(T.Subject, Row.X + 70.0f, Row.Y + 47.0f, Ts(13.5f, T.Unread ? 700 : 500, T.Unread ? Ink : Muted, Align::Left, Baseline::Alphabetic, false, Row.W - 84.0f));
		if (K)
		{
			const std::string P = sponsor::Pill(*K, World);
			NetPill(*C, P, Row.X + 70.0f, Row.Y + 56.0f, PillColor(P), true, 9.5f);
		}
		if (Cs.Clicked)
		{
			St.Pick = T.Id;
			St.Panel = 0;
			St.PickAt = Now;
			Pick = T.Id;
		}
		Y += 86.0f;
	}
	// The brands that won't write for a while (a declined offer, notice given, a deal ended): at the foot of the list.
	{
		std::vector<std::string> Quiet;
		for (const auto& Cd : L.Sponsors.Cooldown)
		{
			const sponsor::Brand* Br = sponsor::FindBrand(Cd.first);
			if (Br && Cd.second > World)
			{
				Quiet.push_back(Br->Short + " (until " + net::DateLabel(net::DayOf(Cd.second)) + ")");
			}
		}
		if (!Quiet.empty() && Y + 40.0f <= MailH - 12.0f)
		{
			std::string Line = "Not writing for now: ";
			for (size_t I = 0; I < Quiet.size(); ++I)
			{
				Line += (I == 0 ? "" : ", ") + Quiet[I];
			}
			const float Fy = std::max(Y + 24.0f, MailH - 64.0f);
			C->FillRect({16.0f, Fy - 22.0f, ListW - 32.0f, 1.0f}, Rule);
			NetParagraph(*C, Line + ".", 24.0f, Fy, ListW - 48.0f, 12.5f, 600, Muted, 17.0f, 3);
		}
	}
	if (Pick != St.Shown)
	{
		St.Panel = 0;
		St.Shown = Pick;
	}
	if (Pick.empty())
	{
		UI.Text("Nothing to read.", PaneX + PaneW * 0.5f, MailH * 0.5f, Ts(18.0f, 600, Muted, Align::Center));
		return;
	}

	// The thread: who it's from, its letters (newest first), read now.
	const Thread* T = nullptr;
	for (const Thread& Each : Threads)
	{
		T = Each.Id == Pick ? &Each : T;
	}
	const sponsor::Contract* K = T && T->Source == "sponsor" ? sponsor::Find(L.Sponsors, T->Id) : nullptr;
	const sponsor::Brand* Br = K ? sponsor::FindBrand(K->Brand) : nullptr;
	const float In = NetEase((Now - St.PickAt) / 0.35);
	C->SetAlpha(0.35f + 0.65f * In);
	if (Br)
	{
		sponsorart::Mark(*C, Br->Id, {PaneX, 92.0f, 58.0f, 58.0f});
		UI.Text(Br->Name, PaneX + 74.0f, 118.0f, Ts(24.0f, 900, Ink));
		UI.Text(Br->Rep + (Br->RepTitle.empty() ? std::string() : " \xC2\xB7 " + Br->RepTitle), PaneX + 74.0f, 142.0f, Ts(14.0f, 600, Muted));
		const std::string P = sponsor::Pill(*K, World);
		const float Pw = UI.Measure(P, 10.5f, 700) + 20.0f;
		NetPill(*C, P, PaneX + PaneW - Pw, 104.0f, PillColor(P), true, 10.5f);
	}
	else if (T)
	{
		const bool Rerun = T->Source == "video";
		const bool Site = T->Source == "site";
		const net::SiteId Who = Site ? rlsites_detail::SitesOfMail(T->Source, T->Id, T->From) : net::SiteId::Count;
		if (Rerun)
		{
			videoart::Logo(*C, {PaneX, 92.0f, 58.0f, 58.0f});
		}
		else if (Site)
		{
			siteart::Logo(*C, {PaneX, 92.0f, 58.0f, 58.0f}, Who);
		}
		else
		{
			Seal(*C, T->From, PaneX + 29.0f, 121.0f, 29.0f);
		}
		UI.Text(T->From, PaneX + 74.0f, 118.0f, Ts(24.0f, 900, Ink));
		// A site's letters say which site (RiverLine Player Relations' too).
		const std::string Desk = Site ? (Who == net::SiteId::RiverLine ? std::string("RiverLine Player Relations") : rlsites_detail::SitesLongName(Who)) : std::string();
		UI.Text(T->Source == "stake" ? std::string("Backing") : Rerun ? std::string(RerunDesk) : Site ? Desk : std::string("Mail"), PaneX + 74.0f, 142.0f, Ts(14.0f, 600, Muted));
	}
	const float LettersBottom = K ? 452.0f : MailH - 90.0f;
	C->PushClip({PaneX - 4.0f, 160.0f, PaneW + 8.0f, LettersBottom - 160.0f});
	float Ly = 184.0f;
	for (auto It = L.Inbox.rbegin(); It != L.Inbox.rend() && Ly < LettersBottom; ++It)
	{
		life::Letter& M = *It;
		if (M.Thread != Pick || (T && M.Source != T->Source))
		{
			continue;
		}
		if (!Previewing)
		{
			M.Read = true;
		}
		UI.Text(M.Subject, PaneX, Ly, Ts(16.0f, 800, Ink, Align::Left, Baseline::Alphabetic, false, PaneW - 220.0f));
		UI.Text(Long(M.At), PaneX + PaneW, Ly, Ts(12.0f, 600, Muted, Align::Right));
		Ly = NetParagraph(*C, M.Body, PaneX, Ly + 24.0f, PaneW, 14.5f, 500, Hex(0x2d3340), 21.0f, 8);
		Ly += 10.0f;
		C->FillRect({PaneX, Ly, PaneW, 1.0f}, Rule);
		Ly += 28.0f;
	}
	C->PopClip();
	C->SetAlpha(1.0f);
	if (T && T->Source == "stake" && AppButton("mailbacking", {PaneX, MailH - 70.0f, 220.0f, 44.0f}, "Open the Backing page", Navy, Hex(0xffffff), !Previewing))
	{
		OpenApp(App::RiverLine, Now);
		OpenPage(Page::Backing, Now);
		return;
	}
	// Rerun's letters, whichever they are (the review, the Partner welcome, a statement): to the Channel page, where the gate's
	// bars, the balance and the payouts are (§4.8, §4.9).
	if (T && T->Source == "video" &&
		AppButton("mailrerun", {PaneX, MailH - 70.0f, 220.0f, 44.0f}, RerunButton, Hex(videoart::Marigold), Hex(videoart::Ink), !Previewing))
	{
		rlvideo_detail::VideoOpen(*this, RerunChannelPage, Now);
		return;
	}
	// A poker site's letters (update 2 §5.4, §5.11): an invitation's Accept and Decline, then the way to the site's Cashier
	// and promotions; RiverLine Player Relations' letters go to Career, where the Rewards card is.
	if (T && T->Source == "site")
	{
		const net::SiteId Who = rlsites_detail::SitesOfMail(T->Source, T->Id, T->From);
		const site::Invitation* Inv = Who != net::SiteId::RiverLine ? rlsites_detail::SitesInvite(S, Who) : nullptr;
		const bool Joined = rlsites_detail::SitesAccount(S, Who) != nullptr;
		const float Fy = MailH - 70.0f;
		auto Refuse = [this, Now](const std::string& Why) {
			if (!Why.empty())
			{
				Toast = Why;
				ToastAt = Now;
			}
		};
		auto Visit = [this, Now, Who](Page P) {
			SiteShown = Who;
			OpenApp(App::RiverLine, Now);
			if (S.CurrentScreen != Screen::Boot)
			{
				S.ShowLobby();
			}
			OpenPage(P, Now);
		};
		if (Inv && !Joined && (Inv->State == 0 || Inv->State == 3))
		{
			if (AppButton("mailsiteaccept", {PaneX, Fy, 170.0f, 44.0f}, rlsites_detail::SitesAcceptLabel, Green, Hex(0xffffff), !Previewing))
			{
				Refuse(S.AnswerInvite(Who, true));
				return;
			}
			if (AppButton("mailsitedecline", {PaneX + 182.0f, Fy, 150.0f, 44.0f}, rlsites_detail::SitesDeclineLabel, Hex(0xe5e1d8), Ink, !Previewing))
			{
				Refuse(S.AnswerInvite(Who, false));
				return;
			}
			UI.Text(Inv->State == 3 ? rlsites_detail::SitesInviteLast : rlsites_detail::SitesInviteOnce, PaneX + 350.0f, Fy + 28.0f, Ts(13.0f, 600, Muted, Align::Left, Baseline::Alphabetic, false, PaneW - 350.0f));
			return;
		}
		if (Who == net::SiteId::RiverLine)
		{
			if (AppButton("mailsiterewards", {PaneX, Fy, 220.0f, 44.0f}, rlsites_detail::SitesRewardsLink, Navy, Hex(0xffffff), !Previewing))
			{
				Visit(Page::Career);
			}
			return;
		}
		if (Joined)
		{
			if (AppButton("mailsitecashier", {PaneX, Fy, 220.0f, 44.0f}, rlsites_detail::SitesCashierLink, Navy, Hex(0xffffff), !Previewing))
			{
				Visit(Page::Cashier);
				return;
			}
			if (AppButton("mailsitepromos", {PaneX + 232.0f, Fy, 220.0f, 44.0f}, rlsites_detail::SitesPromotionsLink, Hex(0xe5e1d8), Ink, !Previewing))
			{
				Visit(Page::Promotions);
			}
		}
		else if (Inv && Inv->State == 2)
		{
			// The second letter, declined too, is the last (core-sites marks it sent with "resent:<key>").
			const bool Last = S.Life.Sites.Done.count("resent:" + std::string(net::KeyOf(Who))) > 0;
			UI.Text(Last ? rlsites_detail::SitesInviteDone : rlsites_detail::SitesInviteDeclined, PaneX, Fy + 28.0f, Ts(13.0f, 600, Muted));
		}
		return;
	}
	if (!K)
	{
		return;
	}

	// The contract card: what you get, what they ask (this period so far), the fine print.
	const sponsor::Card Card = sponsor::CardOf(*K, L, S.Channel, World);
	const Rect Box{PaneX, 466.0f, PaneW, MailH - 466.0f - 18.0f};
	C->FillRoundRect(Box, 14.0f, Hex(0xffffff));
	C->StrokeRoundRect(Box, 14.0f, Rule, 1.0f);
	const float ColW = (Box.W - 64.0f) / 3.0f;
	const float Top = Box.Y + 34.0f;
	const Color Tone = Br ? Hex(Br->Color) : Blue;
	NetSpaced(*C, "WHAT YOU GET", Box.X + 24.0f, Top, 11.0f, 900, Muted, 1.6f);
	NetSpaced(*C, "WHAT THEY ASK", Box.X + 32.0f + ColW, Top, 11.0f, 900, Muted, 1.6f);
	NetSpaced(*C, "THE FINE PRINT", Box.X + 40.0f + 2.0f * ColW, Top, 11.0f, 900, Muted, 1.6f);
	float Gy = Top + 28.0f;
	for (const std::string& Line : Card.Get)
	{
		C->FillCircle(Box.X + 28.0f, Gy - 4.5f, 3.0f, Tone);
		Gy = NetParagraph(*C, Line, Box.X + 38.0f, Gy, ColW - 20.0f, 13.5f, 600, Ink, 18.0f, 2) + 6.0f;
	}
	float Ay = Top + 28.0f;
	const float Ax = Box.X + 32.0f + ColW;
	for (const sponsor::Card::Row& R : Card.Asks)
	{
		const bool Hours = R.Label == "Live hours";
		const std::string Count = Hours ? Fixed(R.Have, 1) + " / " + Fixed(R.Need, 0) : std::to_string(static_cast<int>(std::floor(R.Have + 1e-9))) + " / " + std::to_string(static_cast<int>(std::ceil(R.Need - 1e-9)));
		UI.Text(R.Label, Ax, Ay, Ts(13.0f, 700, Ink, Align::Left, Baseline::Alphabetic, false, ColW - 70.0f));
		UI.Text(Count, Ax + ColW - 8.0f, Ay, Ts(12.5f, 700, Muted, Align::Right, Baseline::Alphabetic, true));
		const float Share = R.Need > 0.0 ? static_cast<float>(std::clamp(R.Have / R.Need, 0.0, 1.0)) : 0.0f;
		C->FillRoundRect({Ax, Ay + 8.0f, ColW - 8.0f, 6.0f}, 3.0f, Hex(0xeae6de));
		if (Share > 0.0f)
		{
			C->FillRoundRect({Ax, Ay + 8.0f, std::max(6.0f, (ColW - 8.0f) * Share), 6.0f}, 3.0f, Share >= 1.0f ? Green : Tone);
		}
		Ay += 36.0f;
	}
	if (!Card.Deadline.empty())
	{
		UI.Text(Card.Deadline, Ax, Ay + 2.0f, Ts(12.5f, 700, Muted));
	}
	float Fy = Top + 28.0f;
	for (const std::string& Line : Card.Fine)
	{
		Fy = NetParagraph(*C, Line, Box.X + 40.0f + 2.0f * ColW, Fy, ColW - 12.0f, 12.5f, 500, Hex(0x4b5563), 17.0f, 2) + 5.0f;
	}

	// The buttons, by where the deal stands.
	const float By = Box.Y + Box.H - 62.0f;
	auto Refused = [this, Now](const std::string& Why) {
		if (!Why.empty())
		{
			Toast = Why;
			ToastAt = Now;
		}
	};
	if (K->St == sponsor::State::Offered && K->ExpiresAt > World)
	{
		const std::vector<std::string> Conflicts = sponsor::Conflicts(L.Sponsors, L, K->Id, World);
		const bool Can = sponsor::CanSign(L.Sponsors, L, K->Id, World).empty();
		float Cy = By - 10.0f;
		for (auto It = Conflicts.rbegin(); It != Conflicts.rend(); ++It)
		{
			UI.Text(*It, Box.X + 24.0f, Cy, Ts(13.0f, 700, Red, Align::Left, Baseline::Alphabetic, false, Box.W - 48.0f));
			Cy -= 18.0f;
		}
		if (St.Panel == 1)
		{
			// The counter: five levers, each with how the rep reads it before it's sent.
			const rep::Public P = S.Record();
			const Rect Pn{Box.X + 24.0f, Box.Y + 14.0f, Box.W - 48.0f, Box.H - 28.0f};
			C->FillRoundRect(Pn, 12.0f, Hex(0xfbfaf7));
			C->StrokeRoundRect(Pn, 12.0f, Rule, 1.0f);
			UI.Text("Counter. One per offer; the answer comes the next business morning.", Pn.X + 20.0f, Pn.Y + 30.0f, Ts(14.0f, 800, Ink));
			float Ry = Pn.Y + 56.0f;
			for (int Lv = 0; Lv <= static_cast<int>(sponsor::Lever::NoExclusivity); ++Lv)
			{
				const sponsor::Lever Lever = static_cast<sponsor::Lever>(Lv);
				const std::string Blocked = sponsor::LeverBlocked(*K, Lever);
				UI.Text(sponsor::LeverLabel(Lever), Pn.X + 20.0f, Ry + 21.0f, Ts(13.5f, 700, Blocked.empty() ? Ink : Muted));
				const std::string Mood = Blocked.empty() ? std::string(rep::Mood(sponsor::Margin(L.Sponsors, P, K->Id, Lever))) : Blocked;
				const bool Open = Mood == "Sounds open to it";
				UI.Text(Mood, Pn.X + Pn.W - 160.0f, Ry + 21.0f, Ts(13.0f, 700, !Blocked.empty() ? Muted : Open ? Green : Mood == "That's a stretch" ? Hex(0xb7791f) : Red, Align::Right));
				if (AppButton("mailcounter" + std::to_string(Lv), {Pn.X + Pn.W - 140.0f, Ry + 2.0f, 120.0f, 30.0f}, "Send", Navy, Hex(0xffffff), Blocked.empty() && !Previewing))
				{
					Refused(S.CounterDeal(K->Id, Lv));
					St.Panel = 0;
					return;
				}
				Ry += 38.0f;
			}
			if (AppButton("mailcounterback", {Pn.X + 20.0f, Pn.Y + Pn.H - 44.0f, 120.0f, 32.0f}, "Never mind", Hex(0xe5e1d8), Ink, !Previewing))
			{
				St.Panel = 0;
			}
			return;
		}
		if (AppButton("mailsign", {Box.X + 24.0f, By, 170.0f, 44.0f}, "Sign", Green, Hex(0xffffff), Can && !Previewing))
		{
			Refused(S.SignDeal(K->Id));
			return;
		}
		if (AppButton("mailcounteropen", {Box.X + 206.0f, By, 150.0f, 44.0f}, "Counter", Navy, Hex(0xffffff), K->Counters == 0 && !Previewing))
		{
			St.Panel = 1;
		}
		if (AppButton("maildecline", {Box.X + 368.0f, By, 130.0f, 44.0f}, "Decline", Hex(0xe5e1d8), Ink, !Previewing))
		{
			S.DeclineDeal(K->Id);
			return;
		}
		UI.Text("Offer stands until " + net::DateLabel(net::DayOf(K->ExpiresAt)) + ".", Box.X + Box.W - 24.0f, By + 28.0f, Ts(13.0f, 600, Muted, Align::Right));
		return;
	}
	if (K->St == sponsor::State::Countered)
	{
		UI.Text("Countered. Their answer comes the next business morning.", Box.X + 24.0f, By + 28.0f, Ts(14.0f, 700, Muted));
		return;
	}
	if (K->St == sponsor::State::Active && K->Until > World)
	{
		// A strike open (a missed ask, or a clause broken: update 2 §5.7): its cure date over the buttons, in words.
		if (K->CureUntil > World && St.Panel != 2)
		{
			UI.Text("Strike " + std::to_string(std::max(1, K->Strikes)) + " \xC2\xB7 cure it by " + std::string(net::WeekdayName(net::DayOf(K->CureUntil), true)) + ", " +
					net::DateLabel(net::DayOf(K->CureUntil)),
				Box.X + 24.0f, By - 10.0f, Ts(13.5f, 800, Red));
		}
		if (St.Panel == 2)
		{
			const Rect Pn{Box.X + 24.0f, By - 86.0f, Box.W - 48.0f, 138.0f};
			C->FillRoundRect(Pn, 12.0f, Hex(0xfbfaf7));
			C->StrokeRoundRect(Pn, 12.0f, Red, 1.5f);
			const double Ends = sponsor::PeriodEnd(*K, std::max(1, std::min(K->Period + 1, sponsor::Periods(*K))));
			NetParagraph(*C,
				"Give notice to " + (Br ? Br->Short : K->Brand) + "? It ends with this month, " + net::DateLabel(net::DayOf(Ends)) +
					", and they pay the month if its asks are met. They won't offer again for six months.",
				Pn.X + 20.0f, Pn.Y + 30.0f, Pn.W - 40.0f, 14.0f, 600, Ink, 20.0f, 3);
			if (AppButton("mailnoticeyes", {Pn.X + 20.0f, Pn.Y + Pn.H - 50.0f, 160.0f, 38.0f}, "Give notice", Red, Hex(0xffffff), !Previewing))
			{
				Refused(S.GiveNotice(K->Id));
				St.Panel = 0;
				return;
			}
			if (AppButton("mailnoticeno", {Pn.X + 192.0f, Pn.Y + Pn.H - 50.0f, 140.0f, 38.0f}, "Keep it", Hex(0xe5e1d8), Ink, !Previewing))
			{
				St.Panel = 0;
			}
			return;
		}
		if (K->Since <= World && AppButton("mailnotice", {Box.X + 24.0f, By, 170.0f, 44.0f}, "Give notice", Hex(0xe5e1d8), Ink, !Previewing))
		{
			St.Panel = 2;
		}
		const bool RaiseOpen = !K->Raised && K->Pending < 0 && K->Since <= World && sponsor::Tier(S.Record(), K->Brand) > K->TierAtSigning;
		if (RaiseOpen && AppButton("mailraise", {Box.X + 206.0f, By, 190.0f, 44.0f}, "Ask for a raise", Navy, Hex(0xffffff), !Previewing))
		{
			Refused(S.AskForRaise(K->Id));
			return;
		}
		if (K->Pending >= 0)
		{
			UI.Text("You asked for a raise. Their answer comes the next business morning.", Box.X + 410.0f, By + 28.0f, Ts(13.0f, 600, Muted));
		}
		return;
	}
	if (K->St == sponsor::State::Notice)
	{
		UI.Text("Notice given. It ends " + net::DateLabel(net::DayOf(sponsor::PeriodEnd(*K, std::max(1, std::min(K->Period + 1, sponsor::Periods(*K)))))) + ".", Box.X + 24.0f, By + 28.0f,
			Ts(14.0f, 700, Hex(0xd9731e)));
		return;
	}
	UI.Text(EndWords(K->Why), Box.X + 24.0f, By + 28.0f, Ts(14.0f, 700, Muted));
}
} // namespace ui
} // namespace ss
