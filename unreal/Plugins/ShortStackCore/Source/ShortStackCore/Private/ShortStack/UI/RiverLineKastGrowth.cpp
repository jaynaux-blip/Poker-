// The creator update (SPEC_CREATORS §5.1, §5.3): Kast's new widgets, ui-kast's file, called from RiverLineKast.cpp (declared in
// RiverLineKastGrowth.h). The count every surface shares and its count-up, the Heat thermometer with its ghost fill and why
// lines, the rank chip, Event Mode's bar and gold pill, Hold or Crack's bar, StreamSide's one action slot with the waiting chips
// and the toast line, the name menu with Nod, "N watching · M chatting", the forecast under Go live, the peak-first end card,
// the Community page's Heat, Ping, raids, week and payouts, and Browse's shelves. Everything reads the stream's const
// kast::View, its last Summary or the Channel offline, and acts only through the Session's verbs; every word is KastCopy.h's
// (core-kastmath) or one of KastUiInventory()'s. The per-laptop state lives here, by laptop, and ~RiverLine drops it
// (rlkast_detail::Forget); while the laptop is Previewing every widget is a still and changes nothing.
#include "RiverLineKastGrowth.h"

#include "../Game/KastCopy.h"
#include "RiverLineShared.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/SiteMotion.h"
#include "ShortStack/UI/StreamArt.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace session_kast_detail
{
// SessionKast.cpp (the Session glue): Heat as the next go-live will seed it, without touching the channel (L8).
bool KastHeatAtGoLive(const kast::Channel& Ch, int Update3LinesRead, double World, double& Heat, double& HeatAt, int& LastStreamDay);
} // namespace session_kast_detail
namespace ui
{
namespace rlvideo_detail
{
// Rerun's (RiverLineVideo.cpp, ui-video's): Studio filtered to a stream's window, and how many moments it holds.
int VideoCutCount(const Session& S, double From, double To);
void VideoOpenTonight(RiverLine& RL, double From, double To, double Now);
} // namespace rlvideo_detail

namespace rlkast_detail
{
// ------------------------------------------------------------------ the copy the widgets add (KastUiInventory)

const char* const KwBuildingTitle = "BUILDING A COMMUNITY";
const char* const KwRaidsTitle = "RAIDS";
const char* const KwRaidsRight = "raid a channel at the end of a stream; it remembers";
const char* const KwWeekTitle = "THIS WEEK";
const char* const KwPayoutsTitle = "KAST PAYOUTS";
const char* const KwPayoutsRight = "paid at each stream's end";
const char* const KwHeatRight = "the last 30 streams";
const char* const KwPingRight = "when you go live, Kast tells your followers";
const char* const KwPingHot = "a hot channel reaches lapsed followers";
const char* const KwPingNone = "Your next stream shows how far your ping reaches.";
const char* const KwHeatNoHistory = "Heat's history starts with your next stream of an hour or more.";
const char* const KwStreakNote = "a missed week halves it";
const char* const KwStaminaCost = "shorter stays, fewer follows until you rest";
const char* const KwNobodyOwes = "Nobody owes you a raid yet.";
const char* const KwNoPayouts = "Nothing paid out yet. Kast pays the channel's balance at each stream's end.";
const char* const KwKastPayoutSub = "tips, cheers, subs, ads";
const char* const KwSponsorSub = "sponsor pay";
const char* const KwPendingTitle = "Waiting for your second rent";
const char* const KwNextPartner = "Next: Partner";
const char* const KwNextAffiliate = "Next: Affiliate";
const char* const KwPartnerBar = "over the last 30 days: 25 hours, 12 days live, an average of 75";
const char* const KwPartnerPerks = "Partner: verified, 70% of every sub, better ad rates.";
const char* const KwPartnerDone = "Partner: verified, 70% of every sub, better ad rates. Every stage of Kast's road is behind you.";
const char* const KwSentToBank = "Sent to your bank";
const char* const KwWaitingChips = "waiting:";
const char* const KwHoldOrCrackChip = "HOLD OR CRACK";
const char* const KwSweatingNobody = "Nobody's at a deep run on stream right now.";
const char* const KwFreshNotYou = "Kast shows a new channel here for its first 12 streams.";
const char* const KwRisingNotYou = "Kast shows channels that usually have under 15 viewers here.";
const char* const KwRaidThem = "Raid them at the end of a stream and they'll remember.";
const char* const KwGoLiveHere = "Go live and your channel shows up here.";
const char* const KwYou = "  \xC2\xB7 you";
const char* const KwAffiliateUnlock = "affiliate unlocks subs";
const char* const KwHoldsShort = "HOLDS";
const char* const KwCracksShort = "CRACKS";
// Labels drawn without a full stop (the mockups' sub-labels); KastUiInventory lints each as the sentence it would be.
const char* const KwRailLabel = "present from the first hand to the last: your rail";
const char* const KwShownSoon = "Levels and streaks get you shown to more people browsing Poker";
// The alert's headline for the site's Broadcast (K6; the mockup's): the site sends viewers, it isn't a raid.
const char* const KwBroadcastTitle = "THE BROADCAST";

/** The end card's troll line: "1 troll showed up." / "3 trolls showed up: a couple of mods would help." */
std::string KwTrollsLine(int Trolls, bool HasMods)
{
	return std::to_string(Trolls) + (Trolls == 1 ? " troll showed up" : " trolls showed up") + (HasMods ? "." : ": a couple of mods would help.");
}

/** The Community page's level strip: "Kast shows you to 12% more people browsing Poker" (a label, no full stop). */
std::string KwShownMore(int More)
{
	return More > 0 ? "Kast shows you to " + std::to_string(More) + "% more people browsing Poker" : std::string(KwShownSoon);
}

// ------------------------------------------------------------------ the per-laptop state

/** One laptop's widget state (RiverLine.h is frozen, so it lives here by laptop; Forget drops it). */
struct KwLaptop
{
	int MenuMsg = -1;          // the chat line the name menu is open on (-1: closed)
	std::string MenuWho;
	float MenuX = 0.0f;
	float MenuY = 0.0f;
	std::vector<std::string> Toasts; // waiting for the toast line
	std::string Toast;               // the one up
	double ToastAt = -1000.0;        // real seconds it went up
	int ChatSeen = 0;                // the newest chat line the toast line has read
	double PingFor = -1.0;           // the stream (its StartWorld) whose ping toast is queued
	double StreamSeen = -1.0;        // the stream the toast line last read
	double FeedAt = -1.0e9;          // the newest alert it has counted (the games' toasts share the minute)
	double GameAt = -1000.0;         // real seconds a game's own toast went up (it takes the line's minute)
	long long FcMinute = LLONG_MIN;  // the forecast, recomputed once a world minute while Studio is open
	kast::growth::Band Fc;
	Fragment Cap;                    // the stream preview's capture (KastPreviewCapture), what it was drawn for and when
	KastCaptureKey CapKey;
	double CapAt = -1.0e9;
	double CapDrawn = -1.0e9;        // the clock at the preview's last draw (captured or copied)
	bool CapHeld = false;
	long long CapCaptures = 0;
	long long CapCopies = 0;
};

std::map<const RiverLine*, KwLaptop>& KwLaptops()
{
	static std::map<const RiverLine*, KwLaptop> KwAll;
	return KwAll;
}

KwLaptop& KwOf(const RiverLine* RL)
{
	return KwLaptops()[RL];
}

/** The count's clock, one a session (the laptop and the 24-inch Studio share it), with the deepest run's table size. */
struct KwClock
{
	const Session* Of = nullptr;
	unsigned long long Used = 0;
	double Stream = -1.0;
	int From = 0;
	int To = 0;
	double At = -1000.0;
	long long TableMinute = LLONG_MIN;
	int TableSize = 0;
	int Site = 0;
};

KwClock& KwClockOf(const Session& S)
{
	static std::array<KwClock, 6> KwClocks;
	static unsigned long long KwTick = 0;
	++KwTick;
	KwClock* Oldest = &KwClocks[0];
	for (KwClock& K : KwClocks)
	{
		if (K.Of == &S)
		{
			K.Used = KwTick;
			return K;
		}
		Oldest = K.Used < Oldest->Used ? &K : Oldest;
	}
	*Oldest = KwClock();
	Oldest->Of = &S;
	Oldest->Used = KwTick;
	return *Oldest;
}

/** The deepest run's table size and site (Inputs, read once a world minute while Event Mode needs them). */
void KwRunFacts(const Session& S, double World, int& TableSize, int& Site)
{
	KwClock& K = KwClockOf(S);
	const long long Minute = static_cast<long long>(std::floor(World));
	if (K.TableMinute != Minute)
	{
		const kast::Inputs In = S.StreamInputs();
		K.TableMinute = Minute;
		K.TableSize = In.DeepTableSize;
		K.Site = In.DeepSite;
	}
	TableSize = K.TableSize;
	Site = K.Site;
}

void Forget(const RiverLine* RL)
{
	KwLaptops().erase(RL);
}

// ------------------------------------------------------------------ the stream preview's capture

bool KastCaptureKey::Same(const KastCaptureKey& O) const
{
	return Frame.X == O.Frame.X && Frame.Y == O.Frame.Y && Frame.W == O.Frame.W && Frame.H == O.Frame.H && Client == O.Client && Screen == O.Screen && Tables == O.Tables &&
		   PxScale == O.PxScale && Xf.A == O.Xf.A && Xf.B == O.Xf.B && Xf.C == O.Xf.C && Xf.D == O.Xf.D && Xf.Tx == O.Xf.Tx && Xf.Ty == O.Xf.Ty;
}

double KastCaptureFrameFor(const KastCaptureKey& Key)
{
	return Key.Screen == static_cast<int>(ss::Screen::Table) ? KastCaptureFrame : KastCaptureFrameStill;
}

/** The capture's command slot N: assigned over the last capture's (a text run's string keeps its storage, C-K's M5), else added. */
void KwCapturePut(Fragment& F, size_t& N, const DrawCmd& Cmd)
{
	if (N < F.Cmds.size())
	{
		F.Cmds[N] = Cmd;
	}
	else
	{
		F.Cmds.push_back(Cmd);
	}
	++N;
}

/** One triangle run of the list into the capture: vertices [V, V + Vc), indices [I, I + Ic) less Rebase (relative to V). */
void KwCaptureRun(Fragment& F, size_t& N, const DrawList& L, size_t V, uint32_t Vc, size_t I, uint32_t Ic, uint32_t Rebase)
{
	DrawCmd Run;
	Run.Type = DrawCmd::Kind::Triangles;
	Run.VertexStart = static_cast<uint32_t>(F.Vertices.size());
	Run.IndexStart = static_cast<uint32_t>(F.Indices.size());
	Run.VertexCount = Vc;
	Run.IndexCount = Ic;
	F.Vertices.insert(F.Vertices.end(), L.Vertices.begin() + static_cast<std::ptrdiff_t>(V), L.Vertices.begin() + static_cast<std::ptrdiff_t>(V + Vc));
	const size_t At = F.Indices.size();
	F.Indices.insert(F.Indices.end(), L.Indices.begin() + static_cast<std::ptrdiff_t>(I), L.Indices.begin() + static_cast<std::ptrdiff_t>(I + Ic));
	if (Rebase != 0)
	{
		for (size_t K = At; K < F.Indices.size(); ++K)
		{
			F.Indices[K] -= Rebase;
		}
	}
	KwCapturePut(F, N, Run);
}

/** The capture copied into the list as drawn (its clips were clipped to the preview when it was captured). C-K's M5: the list grows
 *  once, each run's indices are written rebased in one pass, and a text run or clip goes in as the captured command (one copy;
 *  DrawList::AddText would copy its string twice): the same commands, vertices and indices as before. */
void KwCaptureReplay(DrawList& L, const Fragment& F)
{
	L.Vertices.reserve(L.Vertices.size() + F.Vertices.size());
	L.Indices.reserve(L.Indices.size() + F.Indices.size());
	L.Cmds.reserve(L.Cmds.size() + F.Cmds.size());
	for (const DrawCmd& Cmd : F.Cmds)
	{
		if (Cmd.Type != DrawCmd::Kind::Triangles)
		{
			L.Cmds.push_back(Cmd);
			continue;
		}
		if (Cmd.VertexCount == 0)
		{
			continue;
		}
		const uint32_t Base = L.BeginTriangles(Cmd.VertexCount);
		L.Vertices.insert(L.Vertices.end(), F.Vertices.begin() + Cmd.VertexStart, F.Vertices.begin() + Cmd.VertexStart + Cmd.VertexCount);
		const size_t At = L.Indices.size();
		L.Indices.resize(At + Cmd.IndexCount);
		const uint32_t* Src = F.Indices.data() + Cmd.IndexStart;
		uint32_t* Dst = L.Indices.data() + At;
		for (uint32_t K = 0; K < Cmd.IndexCount; ++K)
		{
			Dst[K] = Src[K] + Base;
		}
		L.Cmds.back().VertexCount += Cmd.VertexCount;
		L.Cmds.back().IndexCount += Cmd.IndexCount;
	}
}
} // namespace rlkast_detail

void rlkast_detail::KastPreviewCapture(Canvas& Cv, const RiverLine* RL, const KastCaptureKey& Key, double Now, const std::function<void()>& Draw)
{
	KwLaptop& Lp = KwOf(RL);
	DrawList& L = Cv.List();
	const bool Onward = Now > Lp.CapDrawn; // the clock moved on since the preview last drew (not a redraw of that instant)
	Lp.CapDrawn = Now;
	if (Lp.CapHeld && Onward && Key.Same(Lp.CapKey) && Now - Lp.CapAt < KastCaptureFrameFor(Key) - 1.0e-6)
	{
		KwCaptureReplay(L, Lp.Cap);
		++Lp.CapCopies;
		return;
	}
	++Lp.CapCaptures;
	// What the list holds before the client draws: its last triangle run may be continued by the first shape Draw adds.
	const size_t Cmd0 = L.Cmds.size();
	const bool Open = Cmd0 > 0 && L.Cmds.back().Type == DrawCmd::Kind::Triangles;
	const uint32_t OpenVc = Open ? L.Cmds.back().VertexCount : 0;
	const uint32_t OpenIc = Open ? L.Cmds.back().IndexCount : 0;
	Draw();
	// Into the capture: its arrays keep their capacity from capture to capture, and its commands are assigned over the last
	// capture's (C-K's M5: no allocation for a text run whose string fits where the last one was).
	Fragment& F = Lp.Cap;
	F.Vertices.clear();
	F.Indices.clear();
	size_t N = 0;
	if (Open)
	{
		const DrawCmd& Run = L.Cmds[Cmd0 - 1];
		if (Run.VertexCount > OpenVc || Run.IndexCount > OpenIc)
		{
			KwCaptureRun(F, N, L, Run.VertexStart + OpenVc, Run.VertexCount - OpenVc, Run.IndexStart + OpenIc, Run.IndexCount - OpenIc, OpenVc);
		}
	}
	for (size_t K = Cmd0; K < L.Cmds.size(); ++K)
	{
		const DrawCmd& Cmd = L.Cmds[K];
		if (Cmd.Type == DrawCmd::Kind::Triangles)
		{
			KwCaptureRun(F, N, L, Cmd.VertexStart, Cmd.VertexCount, Cmd.IndexStart, Cmd.IndexCount, 0);
		}
		else
		{
			KwCapturePut(F, N, Cmd);
		}
	}
	F.Cmds.resize(N);
	Lp.CapKey = Key;
	Lp.CapAt = Now;
	Lp.CapHeld = true;
}

void rlkast_detail::KastCaptureCounts(const RiverLine* RL, long long& Captures, long long& Copies)
{
	const auto It = KwLaptops().find(RL);
	Captures = It != KwLaptops().end() ? It->second.CapCaptures : 0;
	Copies = It != KwLaptops().end() ? It->second.CapCopies : 0;
}

namespace rlkast_detail
{

// ------------------------------------------------------------------ small pieces

using KwView = kast::View;

/** A button in AppButton's look (RiverLineApps.cpp), for the widgets here: true when pressed. */
bool KwButton(KastFrame& F, const std::string& Id, const Rect& R, const std::string& Label, const Color& Fill, const Color& Ink, bool Enabled, float Size = 13.0f)
{
	using namespace rlnet_detail;
	const Ui::ClickState St = F.UI.Clickable(Id, R, Enabled && !F.Previewing);
	const float Shift = St.Down ? 2.0f : 0.0f;
	const Rect B{R.X, R.Y + Shift, R.W, R.H};
	if (Enabled)
	{
		if (St.Hover)
		{
			F.C.GlowRoundRect(B, 9.0f, NetA(Fill, 0.35f), 12.0f);
		}
		F.C.FillRoundRect(B, 9.0f, Paint::Linear({0.0f, B.Y}, {0.0f, B.Y + B.H}, St.Hover ? Mix(Fill, Hex(0xffffff), 0.12f) : Fill, Mix(Fill, Hex(0x000000), 0.12f)));
	}
	else
	{
		F.C.FillRoundRect(B, 9.0f, NetA(Fill, 0.22f));
		F.C.StrokeRoundRect(B, 9.0f, NetA(Fill, 0.4f), 1.0f);
	}
	F.UI.Text(Label, B.X + B.W / 2.0f, B.Y + B.H / 2.0f + 1.0f, Ts(Size, 900, Enabled ? Ink : NetA(Fill, 0.9f), Align::Center, Baseline::Middle, false, B.W - 16.0f));
	return St.Clicked;
}

/** The world week (Monday to Sunday, as the schedule's days count them). */
int KwWeekOf(int Day)
{
	return static_cast<int>(std::floor(static_cast<double>(Day) / 7.0));
}

int KwBits(int Mask)
{
	int N = 0;
	for (int M = Mask; M != 0; M &= M - 1)
	{
		++N;
	}
	return N;
}

/** "the ping", "the directory"... colours a source wears on the end card and the VIEWERS tile. */
Color KwSourceColor(int K)
{
	static const uint32_t Cols[11] = {0x9b5cff, 0x38bdf8, 0xc6f432, 0x60a5fa, 0xf5c542, 0xff3b5c, 0xfb7185, 0xf472b6, 0xf2a33a, 0x22d3ee, 0x6b6385};
	return Hex(Cols[std::max(0, std::min(10, K))]);
}

const std::vector<KwView::Source>& KwSources()
{
	static const std::vector<KwView::Source> All = {KwView::Source::Members, KwView::Source::Directory, KwView::Source::Fresh, KwView::Source::Rising, KwView::Source::Rail,
		KwView::Source::Raid, KwView::Source::Broadcast, KwView::Source::Word, KwView::Source::Video, KwView::Source::Clip, KwView::Source::Lurkers};
	return All;
}

/** The sources with anyone in them, biggest first (ties in KwView::Source's order). */
std::vector<std::pair<KwView::Source, int>> KwTopSources(const std::array<int, 11>& Counts)
{
	std::vector<std::pair<KwView::Source, int>> Out;
	for (const KwView::Source Src : KwSources())
	{
		const int N = Counts[static_cast<size_t>(Src)];
		if (N > 0)
		{
			Out.push_back({Src, N});
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const std::pair<KwView::Source, int>& A, const std::pair<KwView::Source, int>& B) { return A.second > B.second; });
	return Out;
}

/** The facts' why lines while offline (each a fact the channel holds): cooling, the week's cap, showing up, a new channel. The
 *  rule is kast::copy::WhyOffline's, the one ss.Kast.Describe prints too (L10). */
std::array<std::string, 2> KwWhyOffline(const kast::Channel& Ch, double World)
{
	return kast::copy::WhyOffline(Ch, Ch.LastStreamDay, World);
}

/** The forecast for a stream started now, once a world minute (§2.11: tens of µs over 1,500 members). */
const kast::growth::Band& KwForecast(KastFrame& F)
{
	KwLaptop& K = KwOf(&F.RL);
	const long long Minute = static_cast<long long>(std::floor(F.World));
	if (K.FcMinute != Minute && !F.Previewing)
	{
		K.FcMinute = Minute;
		K.Fc = F.S.KastForecast();
	}
	return K.Fc;
}

/** The toast line (§5.1): the go-live's ping, Front Row, Welcome back and a top-of-feed clip, one a real minute, none while Hold
 *  or Crack is open; returns the toast up now ("" for none). Reads only while the laptop isn't Previewing. */
const std::string& KwToast(KastFrame& F)
{
	static const std::string KwNone;
	KwLaptop& K = KwOf(&F.RL);
	const kast::Stream& St = F.S.Stream;
	if (!St.Live)
	{
		K.Toasts.clear();
		return F.Now - K.ToastAt < 6.0 ? K.Toast : KwNone;
	}
	if (!F.Previewing)
	{
		if (K.StreamSeen != St.StartWorld)
		{
			// A new stream: nothing from before it, and the chat it starts with is read from here on.
			K.StreamSeen = St.StartWorld;
			K.Toasts.clear();
			K.ChatSeen = St.Chat.empty() ? 0 : St.Chat.front().Id - 1;
			K.FeedAt = St.Feed.empty() ? -1.0e7 : St.Feed.front().At;
		}
		const KwView& V = St.GetView();
		if (V.PingOf > 0 && K.PingFor != St.StartWorld)
		{
			K.PingFor = St.StartWorld;
			K.Toasts.push_back(kast::copy::PingToast(V.PingReached, V.PingOf));
		}
		const std::string Front = kast::copy::FrontRow();
		const std::string Top = kast::copy::ClipTopOfFeed();
		const std::string Back = kast::copy::WelcomeBack(1);
		const std::string BackHead = Back.substr(0, Back.find(':') == std::string::npos ? 0 : Back.find(':') + 1);
		for (const kast::ChatMsg& M : St.Chat)
		{
			if (M.Id <= K.ChatSeen)
			{
				continue;
			}
			K.ChatSeen = M.Id;
			if (M.Kind != kast::LineKind::System)
			{
				continue;
			}
			if (M.Text == Front || M.Text.rfind(Top, 0) == 0 || (!BackHead.empty() && M.Text.rfind(BackHead, 0) == 0))
			{
				K.Toasts.push_back(M.Text);
			}
		}
		// The games' own toasts (a milestone, Affiliate, Partner, the first follower, a top clip) go up as alerts through their
		// one-a-minute gate (play::ToastFree); the line here counts them, so the two never stack inside a real minute.
		const std::string FirstSub = kast::copy::FirstFollowerSub();
		double Newest = K.FeedAt;
		for (const kast::Alert& A : St.Feed)
		{
			if (A.At <= K.FeedAt)
			{
				break;
			}
			Newest = std::max(Newest, A.At);
			const bool Toasty = A.Kind == kast::AlertKind::Milestone || A.Kind == kast::AlertKind::Affiliate || A.Kind == kast::AlertKind::Partner ||
				A.Kind == kast::AlertKind::Clip || (A.Kind == kast::AlertKind::Follow && A.Text == FirstSub);
			if (Toasty && K.FeedAt > -1.0e8)
			{
				// The game's toast takes the minute: the line's own one steps down and the next waits a full minute.
				K.GameAt = F.Now;
			}
		}
		K.FeedAt = Newest;
		while (K.Toasts.size() > 3)
		{
			K.Toasts.erase(K.Toasts.begin());
		}
		if (!K.Toasts.empty() && !V.HoC.Open && F.Now - std::max(K.ToastAt, K.GameAt) >= 60.0)
		{
			K.Toast = K.Toasts.front();
			K.Toasts.erase(K.Toasts.begin());
			K.ToastAt = F.Now;
		}
	}
	// None while Hold or Crack is open (§5.1), even one already up; none under a game's own toast.
	return F.Now - K.ToastAt < 6.0 && F.Now >= K.ToastAt && K.ToastAt >= K.GameAt && !St.GetView().HoC.Open ? K.Toast : KwNone;
}

/** "M chatting" never reads above watching (C12): at most the count, and at most the count shown while it steps. */
int KwChatting(const KwView& V, int Shown)
{
	return std::max(0, std::min(V.Chatting, std::min(std::max(0, V.Count), Shown)));
}

/** The hype's words under the HEAT tile and on StreamSide (today's Studio words). */
const char* KwHypeWords(double Hype)
{
	return Hype > 60.0 ? "chat is losing it" : Hype > 30.0 ? "chat is awake" : "quiet grind";
}
} // namespace rlkast_detail

// ------------------------------------------------------------------ the count (K1)

int rlkast_detail::KastShown(const Session& S, double Now, bool Still)
{
	if (!S.Streaming())
	{
		return 0;
	}
	const kast::View& V = S.Stream.GetView();
	const int Target = std::max(0, V.Count);
	if (Still)
	{
		return Target;
	}
	KwClock& K = KwClockOf(S);
	// A new stream, or a clock that ran backwards (a session gone and another in its place): start where the count is.
	if (K.Stream != S.Stream.StartWorld || Now < K.At)
	{
		K.Stream = S.Stream.StartWorld;
		K.From = K.To = Target;
		K.At = -1000.0;
	}
	if (Target != K.To)
	{
		const int Shown = static_cast<int>(std::lround(motion::CountUp(K.From, K.To, Now, K.At, 0.3)));
		// Up from what's shown, never below the Room's floor (the distinct speakers on screen); down from what's shown.
		K.From = Target > Shown ? std::max(Shown, std::min(V.ShownCount, Target)) : Shown;
		K.To = Target;
		K.At = Now;
	}
	return static_cast<int>(std::lround(motion::CountUp(K.From, K.To, Now, K.At, 0.3)));
}

float rlkast_detail::KastPillWidth(Canvas& Cv, const Session& S)
{
	return Cv.Measure(KastCount(std::max(0, S.Stream.GetView().Count)), 13.0f, 900, true) + 80.0f;
}

void rlkast_detail::KastPill(KastFrame& F, float Right, float Y)
{
	using namespace rlnet_detail;
	const KwView& V = F.S.Stream.GetView();
	const float W = KastPillWidth(F.C, F.S);
	const Rect R{Right - W, Y, W, 30.0f};
	const Ui::ClickState St = F.UI.Clickable("kastpill", R, !F.Previewing);
	const double T = F.Previewing ? 0.0 : F.Now;
	const std::string L = KastCount(KastShown(F.S, F.Now, F.Previewing));
	if (V.EventMode)
	{
		// Event Mode (§5.3 K5): the pill turns gold, its words dark.
		F.C.FillRoundRect(R, 15.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, St.Hover ? Mix(KGold, Hex(0xffffff), 0.15f) : KGold, KGold2));
		F.C.StrokeRoundRect(R, 15.0f, KGold2, 1.5f);
		F.C.FillCircle(R.X + 15.0f, R.Y + 15.0f, 4.5f, NetA(KRed, 0.75f + 0.25f * Nf(std::sin(T * 4.0))));
		F.UI.Text("KAST", R.X + 24.0f, R.Y + 19.5f, Ts(11.0f, 900, KDeep));
		F.UI.Text(L, R.X + W - 12.0f, R.Y + 20.0f, Ts(13.0f, 900, KDeep, Align::Right, Baseline::Alphabetic, true));
	}
	else
	{
		F.C.FillRoundRect(R, 15.0f, St.Hover ? Hex(0x6d28d9) : Hex(0x2a1652));
		F.C.StrokeRoundRect(R, 15.0f, KViolet, 1.5f);
		F.C.FillCircle(R.X + 15.0f, R.Y + 15.0f, 4.5f, NetA(KRed, 0.6f + 0.4f * Nf(std::sin(T * 4.0))));
		F.UI.Text("KAST", R.X + 24.0f, R.Y + 19.5f, Ts(11.0f, 900, KLav));
		F.UI.Text(L, R.X + W - 12.0f, R.Y + 20.0f, Ts(13.0f, 900, Hex(0xffffff), Align::Right, Baseline::Alphabetic, true));
	}
	if (St.Clicked)
	{
		F.RL.OpenApp(RiverLine::App::Kast, F.Now);
	}
}

// ------------------------------------------------------------------ Heat (K4)

bool rlkast_detail::KastGhostSettled(const Session& S, double World)
{
	const kast::Stream& St = S.Stream;
	const KwView& V = St.GetView();
	const double Up = St.Uptime(World);
	if (Up < 15.0)
	{
		return false;
	}
	if (Up >= 60.0 || V.Fc.Hi <= 0.0)
	{
		return true;
	}
	// The running average the ghost's perf reads (the Room's viewer-minutes over its minutes: one graph sample a world minute), as
	// it stood at each world minute from 15 on: once it has read inside or above the band, the night has settled and stays settled
	// (a number shown never goes back to "settling in" when an early raid leaves and the average dips under the band).
	double Sum = 0.0;
	for (size_t I = 0; I < St.Graph.size(); ++I)
	{
		Sum += static_cast<double>(St.Graph[I]);
		const double Avg = Sum / static_cast<double>(I + 1);
		if (I >= 15 && Avg > 0.0 && Avg >= V.Fc.Lo)
		{
			return true;
		}
	}
	return false;
}

std::string rlkast_detail::KastGhostLine(const KastHeat& H)
{
	return H.Settled ? kast::copy::GhostTonight(H.Ghost) : kast::copy::GhostSettling();
}

rlkast_detail::KastHeat rlkast_detail::KastHeatNow(const Session& S, double World)
{
	KastHeat H;
	if (S.Streaming())
	{
		const KwView& V = S.Stream.GetView();
		if (V.Heat > 0.0)
		{
			H.Live = true;
			H.Shown = V.Heat;
			H.Settled = KastGhostSettled(S, World);
			H.Ghost = H.Settled ? V.GhostGain : 0.0;
			H.Tier = V.Tier;
			H.Why = V.Why;
			return H;
		}
	}
	// Off air: before an old save's first stream under this build, the Heat (and the last day live) its go-live will seed, so a
	// channel dark 8+ days reads Cold here as its night will start (the C-K gate's L8; the forecast previews the same).
	const kast::Channel& Ch = S.Channel;
	double Heat = Ch.Heat;
	double HeatAt = Ch.HeatAt;
	int LastDay = Ch.LastStreamDay;
	session_kast_detail::KastHeatAtGoLive(Ch, S.Life.Update3LinesRead, World, Heat, HeatAt, LastDay);
	H.Shown = kast::growth::HeatShown(Heat, HeatAt, World, LastDay);
	H.Tier = kast::growth::Tier(H.Shown);
	H.Why = kast::copy::WhyOffline(Ch, LastDay, World);
	return H;
}

const char* rlkast_detail::KastTierWord(int Tier)
{
	static const std::string Words[5] = {kast::copy::TierName(0), kast::copy::TierName(1), kast::copy::TierName(2), kast::copy::TierName(3), kast::copy::TierName(4)};
	return Words[std::max(0, std::min(4, Tier))].c_str();
}

Color rlkast_detail::KastTierTint(int Tier, double Now)
{
	switch (std::max(0, std::min(4, Tier)))
	{
	case 0: return Hex(0x8fb3d9);
	case 1: return Hex(0xe9b872);
	case 2: return Hex(0xf59e0b);
	case 3: return Hex(0xef6c2f);
	default:
	{
		// Sun run: white-gold with a slow shimmer (warm light, not fire).
		const float Sh = 0.5f + 0.5f * static_cast<float>(std::sin(Now * 1.3));
		return Mix(Hex(0xffe9a8), Hex(0xfff7dc), 0.45f * Sh);
	}
	}
}

void rlkast_detail::KastThermo(Canvas& Cv, float X, float Y, float H, double Fill, double Ghost, const Color& Col)
{
	using namespace rlnet_detail;
	const float Tw = H * 0.30f;
	const float Br = H * 0.22f;
	const float Tx = X + Br - Tw / 2.0f;
	const float Top = Y;
	const float Bot = Y + H - Br * 1.6f;
	const float Inner = std::max(1.0f, Bot - Top);
	Cv.FillRoundRect({Tx - 2.0f, Top - 2.0f, Tw + 4.0f, Inner + Br + 4.0f}, (Tw + 4.0f) / 2.0f, Rgba(0, 0, 0, 0.35f));
	Cv.FillCircle(X + Br, Y + H - Br, Br + 2.0f, Rgba(0, 0, 0, 0.35f));
	Cv.FillRoundRect({Tx, Top, Tw, Inner + Br}, Tw / 2.0f, KPanel2);
	Cv.FillCircle(X + Br, Y + H - Br, Br, KPanel2);
	const float Fl = Nf(std::max(0.0, std::min(1.0, Fill)));
	const float Gh = Nf(std::max(0.0, std::min(1.0, Fill + std::max(0.0, Ghost))));
	if (Gh > Fl)
	{
		Cv.FillRoundRect({Tx + 1.5f, Bot - Inner * Gh, Tw - 3.0f, Inner * Gh + Br}, (Tw - 3.0f) / 2.0f, NetA(Col, 0.38f));
	}
	Cv.FillRoundRect({Tx + 1.5f, Bot - Inner * Fl, Tw - 3.0f, Inner * Fl + Br}, (Tw - 3.0f) / 2.0f, Col);
	Cv.FillCircle(X + Br, Y + H - Br, Br - 1.5f, Col);
	Cv.FillCircle(X + Br - Br * 0.35f, Y + H - Br - Br * 0.35f, Br * 0.3f, Rgba(255, 255, 255, 0.35f));
	for (int K = 1; K < 4; ++K)
	{
		Cv.FillRect({Tx + Tw + 2.5f, Top + Inner * Nf(K) / 4.0f, std::max(2.0f, H * 0.08f), 1.0f}, NetA(KMuted, 0.6f));
	}
}

void rlkast_detail::KastHeatBar(Canvas& Cv, const Rect& R, double Heat, double GhostTo)
{
	using namespace rlnet_detail;
	static const double Lo[5] = {8.0, 20.0, 35.0, 55.0, 75.0};
	static const double Hi[5] = {20.0, 35.0, 55.0, 75.0, 100.0};
	const float Gap = 3.0f;
	const float Sw = (R.W - Gap * 4.0f) / 5.0f;
	for (int I = 0; I < 5; ++I)
	{
		const float Sx = R.X + Nf(I) * (Sw + Gap);
		const Color Col = KastTierTint(I, 0.0);
		Cv.FillRoundRect({Sx, R.Y, Sw, R.H}, R.H / 2.0f, KPanel2);
		const float Fl = Nf(std::max(0.0, std::min(1.0, (Heat - Lo[I]) / (Hi[I] - Lo[I]))));
		if (GhostTo > Heat)
		{
			const float Gf = Nf(std::max(0.0, std::min(1.0, (GhostTo - Lo[I]) / (Hi[I] - Lo[I]))));
			if (Gf > Fl)
			{
				Cv.FillRoundRect({Sx, R.Y, std::max(R.H, Sw * Gf), R.H}, R.H / 2.0f, NetA(Col, 0.35f));
			}
		}
		if (Fl > 0.0f)
		{
			Cv.FillRoundRect({Sx, R.Y, std::max(R.H, Sw * Fl), R.H}, R.H / 2.0f, Col);
		}
	}
}

void rlkast_detail::KastHeatChip(Canvas& Cv, float X, float Y, const KastHeat& H, double Now)
{
	using namespace rlnet_detail;
	const Color Col = KastTierTint(H.Tier, Now);
	Cv.FillRoundRect({X, Y, 200.0f, 32.0f}, 9.0f, NetA(Col, 0.13f));
	Cv.StrokeRoundRect({X, Y, 200.0f, 32.0f}, 9.0f, NetA(Col, 0.45f), 1.0f);
	KastThermo(Cv, X + 10.0f, Y + 4.0f, 24.0f, (H.Shown - 8.0) / 92.0, H.Live ? H.Ghost / 92.0 : 0.0, Col);
	NetSpaced(Cv, kast::copy::HeatTitle(), X + 32.0f, Y + 20.5f, 9.5f, 900, KMuted, 1.2f);
	Cv.Text(KastTierWord(H.Tier), X + 68.0f, Y + 21.5f, Ts(14.0f, 900, Col, Align::Left, Baseline::Alphabetic, false, 124.0f));
}

float rlkast_detail::KastRankChip(Canvas& Cv, float Right, float Y, int Rank, int Of)
{
	if (Rank <= 0 || Of <= 0)
	{
		return 0.0f;
	}
	const std::string L = kast::copy::RankHeader(Rank, Of);
	const float W = Cv.Measure(L, 13.0f, 800) + 26.0f;
	Cv.FillRoundRect({Right - W, Y, W, 28.0f}, 8.0f, KPanel2);
	Cv.StrokeRoundRect({Right - W, Y, W, 28.0f}, 8.0f, KLine, 1.0f);
	Cv.Text(L, Right - W / 2.0f, Y + 19.0f, Ts(13.0f, 800, KLav, Align::Center));
	return W;
}

// ------------------------------------------------------------------ Studio

void rlkast_detail::KastViewersTile(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Stream& St = F.S.Stream;
	const KwView& V = St.GetView();
	const bool Live = St.Live;
	if (!Live)
	{
		F.UI.Text("\xE2\x80\x94", R.X + 16.0f, R.Y + 68.0f, Ts(30.0f, 900, KDim, Align::Left, Baseline::Alphabetic, true));
		F.UI.Text("offline", R.X + 16.0f, R.Y + 92.0f, Ts(12.0f, 600, KMuted));
		KastSpark(F.C, {R.X + 12.0f, R.Y + 106.0f, R.W - 24.0f, 54.0f}, St.Graph, KDim);
		return;
	}
	const int N = KastShown(F.S, F.Now, F.Previewing);
	const std::string Big = KastCount(N);
	const float Bw = F.UI.Text(Big, R.X + 16.0f, R.Y + 64.0f, Ts(30.0f, 900, KRed, Align::Left, Baseline::Alphabetic, true));
	F.UI.Text("watching", R.X + 22.0f + Bw, R.Y + 64.0f, Ts(14.0f, 800, KRed));
	// Where they came from (K8): the top four sources on two lines, as many as fit whole; at a true 0, the honest line.
	const std::vector<std::pair<KwView::Source, int>> Top = KwTopSources(V.Sources);
	std::string L1;
	std::string L2;
	const float Avail = R.W - 24.0f;
	for (size_t I = 0; I < Top.size() && I < 4; ++I)
	{
		const std::string Part = kast::copy::SourceCount(Top[I].first, Top[I].second);
		const std::string Try1 = L1 + (L1.empty() ? "" : " \xC2\xB7 ") + Part;
		const std::string Try2 = L2 + (L2.empty() ? "" : " \xC2\xB7 ") + Part;
		if (L2.empty() && F.UI.Measure(Try1, 11.5f, 600) <= Avail)
		{
			L1 = Try1;
		}
		else if (F.UI.Measure(Try2, 11.5f, 600) <= Avail)
		{
			L2 = Try2;
		}
	}
	if (V.Count <= 0)
	{
		// The honest zero (K2, R6): nobody here, and what the directory did for the stream in the last 30 world minutes ("41 saw
		// you in the directory", the Room's impressions: C-K's L11).
		L1 = kast::copy::NobodyHere();
		L2 = V.Impressions > 0 ? kast::copy::SawYou(V.Impressions) : std::string();
	}
	if (!L1.empty())
	{
		F.UI.Text(L1, R.X + 16.0f, R.Y + 84.0f, Ts(11.5f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, R.W - 24.0f));
	}
	if (!L2.empty())
	{
		// Whole when a smaller size holds it ("41 saw you in the directory" is a fact; an ellipsis only past 10 units).
		const float L2Size = F.UI.Measure(L2, 11.5f, 600) <= Avail ? 11.5f : F.UI.Measure(L2, 10.5f, 600) <= Avail ? 10.5f : 10.0f;
		F.UI.Text(L2, R.X + 16.0f, R.Y + 99.0f, Ts(L2Size, 600, KMuted, Align::Left, Baseline::Alphabetic, false, R.W - 24.0f));
	}
	F.UI.Text(kast::copy::Chatting(KwChatting(V, N)), R.X + 16.0f, R.Y + 115.0f, Ts(11.5f, 700, KLav, Align::Left, Baseline::Alphabetic, false, R.W - 24.0f));
	// The night's graph with tonight's forecast band (K9).
	const Rect G{R.X + 12.0f, R.Y + 122.0f, R.W - 24.0f, 42.0f};
	const size_t Nn = std::min<size_t>(St.Graph.size(), 90);
	float Hi = 1.0f;
	for (size_t I = St.Graph.size() - Nn; I < St.Graph.size(); ++I)
	{
		Hi = std::max(Hi, St.Graph[I]);
	}
	const bool Band = V.Fc.Hi > 0.0;
	if (Band)
	{
		Hi = std::max(Hi, Nf(V.Fc.Hi));
	}
	Hi *= 1.15f;
	auto Py = [&](double Vv) { return G.Y + G.H - G.H * Nf(Vv) / Hi; };
	if (Band)
	{
		const float Y0 = Py(V.Fc.Hi);
		const float Y1 = Py(std::max(0.0, V.Fc.Lo));
		F.C.FillRect({G.X, Y0, G.W, std::max(1.0f, Y1 - Y0)}, Rgba(255, 255, 255, 0.07f));
		F.C.StrokeDashed({{G.X, Y0}, {G.X + G.W, Y0}}, NetA(KMuted, 0.45f), 1.0f, 3.0f, 3.0f);
		F.C.StrokeDashed({{G.X, Y1}, {G.X + G.W, Y1}}, NetA(KMuted, 0.45f), 1.0f, 3.0f, 3.0f);
		F.UI.Text(kast::copy::ForecastShort(V.Fc.Lo, V.Fc.Hi), G.X + G.W - 2.0f, Y0 - 3.0f, Ts(9.5f, 700, KMuted, Align::Right));
	}
	if (Nn >= 2)
	{
		std::vector<Vec2> P;
		for (size_t I = 0; I < Nn; ++I)
		{
			P.push_back({G.X + G.W * Nf(I) / Nf(Nn - 1), Py(St.Graph[St.Graph.size() - Nn + I])});
		}
		std::vector<Vec2> Area = P;
		Area.push_back({G.X + G.W, G.Y + G.H});
		Area.push_back({G.X, G.Y + G.H});
		F.C.FillPolygon(Area, Paint::Linear({0.0f, G.Y}, {0.0f, G.Y + G.H}, NetA(KRed, 0.3f), NetA(KRed, 0.0f)));
		F.C.StrokePolyline(P, false, KRed, 2.0f, true);
		F.C.FillCircle(P.back().X, P.back().Y, 3.0f, KRed);
	}
}

void rlkast_detail::KastHeatTile(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Stream& St = F.S.Stream;
	const KastHeat H = KastHeatNow(F.S, F.World);
	const double T = F.Previewing ? 0.0 : F.Now;
	const Color Col = KastTierTint(H.Tier, T);
	KastThermo(F.C, R.X + R.W - 30.0f, R.Y + 10.0f, 34.0f, (H.Shown - 8.0) / 92.0, H.Live ? H.Ghost / 92.0 : 0.0, Col);
	const std::string Word = KastTierWord(H.Tier);
	const float Size = F.UI.Measure(Word, 24.0f, 900) < R.W - 30.0f ? 24.0f : F.UI.Measure(Word, 21.0f, 900) < R.W - 30.0f ? 21.0f : 18.0f;
	F.UI.Text(Word, R.X + 16.0f, R.Y + 64.0f, Ts(Size, 900, Col));
	// Live: tonight's ghost ("tonight: settling in" until the night has settled, M4); offline: the top why line (two lines).
	const std::string Sub = H.Live ? KastGhostLine(H) : H.Why[0];
	std::vector<std::string> Lines = Sub.empty() ? std::vector<std::string>() : NetWrap(F.C, Sub, R.W - 30.0f, 11.5f, 600);
	if (Lines.size() > 2)
	{
		Lines.resize(2);
	}
	const bool Digits = H.Live && H.Settled;
	for (size_t I = 0; I < Lines.size(); ++I)
	{
		F.UI.Text(Lines[I], R.X + 16.0f, R.Y + 88.0f + Nf(I) * 15.0f, Ts(Lines.size() == 1 ? 12.0f : 11.5f, Digits ? 700 : 600, Digits ? KLav : KMuted));
	}
	const float Dy = Lines.size() > 1 ? 15.0f : 0.0f;
	const double Ghost = H.Live && H.Ghost > 0.0 ? std::min(100.0, H.Shown + H.Ghost) : 0.0;
	KastHeatBar(F.C, {R.X + 16.0f, R.Y + 106.0f + Dy, R.W - 32.0f, 10.0f}, H.Shown, Ghost);
	const std::string Third = St.Live ? "hype " + std::to_string(static_cast<int>(St.Hype)) + " \xC2\xB7 " + KwHypeWords(St.Hype) : std::string("big hands make hype");
	F.UI.Text(Third, R.X + 16.0f, R.Y + 140.0f + Dy * 0.6f, Ts(11.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 24.0f));
}

std::string rlkast_detail::KastFollowerFoot(const kast::Channel& Ch, int& Goal)
{
	Goal = kast::AffiliateFollowersC8;
	if (Ch.Followers < kast::AffiliateFollowersC8)
	{
		return kast::copy::AffiliateToGo(kast::AffiliateFollowersC8 - Ch.Followers, kast::AffiliateFollowersC8);
	}
	return "Affiliate at " + kast::copy::Number(kast::AffiliateFollowersC8);
}

bool rlkast_detail::KastGoBlock(KastFrame& F, const Rect& R, bool Open, bool Enabled, const std::string& Locked)
{
	using namespace rlnet_detail;
	const Ui::ClickState St = F.UI.Clickable("kastlive", R, Enabled && !F.Previewing);
	const float Shift = St.Down ? 2.0f : 0.0f;
	const Rect B{R.X, R.Y + Shift, R.W, R.H};
	const Color Fill = KLime;
	if (Enabled)
	{
		if (St.Hover)
		{
			F.C.GlowRoundRect(B, 12.0f, NetA(Fill, 0.35f), 14.0f);
		}
		F.C.FillRoundRect(B, 12.0f, Paint::Linear({0.0f, B.Y}, {0.0f, B.Y + B.H}, St.Hover ? Mix(Fill, Hex(0xffffff), 0.12f) : Fill, Mix(Fill, Hex(0x000000), 0.12f)));
	}
	else
	{
		F.C.FillRoundRect(B, 12.0f, Rgba(128, 128, 128, 0.16f));
		F.C.StrokeRoundRect(B, 12.0f, Rgba(128, 128, 128, 0.3f), 1.0f);
	}
	const Color Ink = Enabled ? KDeep : Rgba(128, 128, 128, 0.95f);
	const float Cx = B.X + B.W / 2.0f;
	if (!Open)
	{
		F.UI.Text("Go live", Cx, B.Y + B.H / 2.0f - 3.0f, Ts(17.0f, 800, Ink, Align::Center, Baseline::Alphabetic, false, B.W - 24.0f));
		F.UI.Text(Locked, Cx, B.Y + B.H / 2.0f + 15.0f, Ts(12.5f, 600, NetA(Ink, 0.85f), Align::Center, Baseline::Alphabetic, false, B.W - 24.0f));
		return St.Clicked;
	}
	// The forecast (K9): "Tonight: likely 8-14" and "Tuesday · on schedule · Running hot". Until the Room's forecast lands
	// (an empty band) the old estimate stands.
	const kast::growth::Band& Fc = KwForecast(F);
	F.UI.Text("Go live", Cx, B.Y + 23.0f, Ts(18.0f, 900, Ink, Align::Center, Baseline::Alphabetic, false, B.W - 24.0f));
	if (Fc.Hi > 0.0)
	{
		const kast::Channel& Ch = F.S.Channel;
		const int Schedule = Ch.ScheduleDays == 0 ? 0 : Ch.OnSchedule(F.World) ? 1 : 2;
		const KastHeat H = KastHeatNow(F.S, F.World);
		F.UI.Text(kast::copy::ForecastBand(Fc.Lo, Fc.Hi), Cx, B.Y + 41.0f, Ts(12.5f, 800, Ink, Align::Center, Baseline::Alphabetic, false, B.W - 16.0f));
		const std::string Context = kast::copy::ForecastContext(net::WeekdayName(net::DayOf(F.World), true), Schedule, H.Tier);
		const float Cs = F.UI.Measure(Context, 10.5f, 700) <= B.W - 16.0f ? 10.5f : 9.5f;
		F.UI.Text(Context, Cx, B.Y + 56.0f, Ts(Cs, 700, NetA(Ink, 0.8f), Align::Center, Baseline::Alphabetic, false, B.W - 12.0f));
	}
	else
	{
		const int Est = static_cast<int>(std::round(kast::Stream::Expected(F.S.Channel, F.S.StreamInputs())));
		F.UI.Text("~" + std::to_string(Est) + " viewers to start", Cx, B.Y + 46.0f, Ts(12.5f, 600, NetA(Ink, 0.85f), Align::Center, Baseline::Alphabetic, false, B.W - 24.0f));
	}
	return St.Clicked;
}

namespace rlkast_detail
{
/** Hold or Crack's two call buttons in R (Studio's short labels or StreamSide's), the streamer's own call lit once made. */
void KwCallButtons(KastFrame& F, const Rect& R, bool Short, const std::string& IdPrefix)
{
	using namespace rlnet_detail;
	const KwView& V = F.S.Stream.GetView();
	const float Bw = (R.W - 8.0f) / 2.0f;
	for (int K = 0; K < 2; ++K)
	{
		const bool Holds = K == 0;
		const Rect B{R.X + Nf(K) * (Bw + 8.0f), R.Y, Bw, R.H};
		const bool Called = V.HoC.Called != 0;
		const bool Mine = V.HoC.Called == (Holds ? 1 : 2);
		const std::string Label = Short ? std::string(Holds ? KwHoldsShort : KwCracksShort) : Holds ? kast::copy::CallIt(true) : kast::copy::CallIt(false);
		const Color Fill = Holds ? KLime : KRed;
		if (Called)
		{
			// The streamer's call stands lit (white-ringed), the other fades; neither takes a press again.
			F.C.FillRoundRect(B, 9.0f, Mine ? Fill : NetA(Fill, 0.12f));
			F.UI.Text(Label, B.X + B.W / 2.0f, B.Y + B.H / 2.0f + 1.0f, Ts(13.0f, 900, Mine ? (Holds ? KDeep : Hex(0xffffff)) : NetA(Fill, 0.45f), Align::Center, Baseline::Middle, false, B.W - 12.0f));
			if (Mine)
			{
				F.C.StrokeRoundRect({B.X - 2.0f, B.Y - 2.0f, B.W + 4.0f, B.H + 4.0f}, 11.0f, Hex(0xffffff), 1.5f);
			}
			continue;
		}
		if (KwButton(F, IdPrefix + (Holds ? "holds" : "cracks"), B, Label, Fill, Holds ? KDeep : Hex(0xffffff), true, 13.0f))
		{
			F.S.StreamCallIt(Holds);
		}
	}
}

/** One action button (RAIL CALL, WELCOME THE RAID, Set the scene) with the soft halo the slot wears; true when pressed. */
bool KwSlotButton(KastFrame& F, const std::string& Id, const Rect& B, const std::string& Label, const Color& Fill, const Color& Ink)
{
	using namespace rlnet_detail;
	F.C.GlowRoundRect(B, 9.0f, NetA(Fill, 0.25f), 8.0f);
	return KwButton(F, Id, B, Label, Fill, Ink, true, 13.5f);
}

/** WELCOME THE RAID with its window left, "WELCOME THE RAID 1:52" (View::WelcomeRaidSecs, world seconds; the mockup's chip). */
std::string KwWelcomeLabel(const KwView& V)
{
	const std::string Button = kast::copy::WelcomeTheRaidButton();
	if (V.WelcomeRaidSecs <= 0)
	{
		return Button;
	}
	const int Secs = V.WelcomeRaidSecs;
	return Button + " " + std::to_string(Secs / 60) + ":" + (Secs % 60 < 10 ? "0" : "") + std::to_string(Secs % 60);
}

/** After the slot: the rail call's reach, or the whole rail here. "" when neither applies. */
std::string KwRailLine(const KwView& V)
{
	if (V.Action != KwView::Slot::RailCall && V.RailReach > 0)
	{
		return kast::copy::RailCallReached(V.RailReach);
	}
	if (V.WholeRail)
	{
		return kast::copy::WholeRailHere();
	}
	return std::string();
}
} // namespace rlkast_detail

void rlkast_detail::KastStudioSlot(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const KwView& V = F.S.Stream.GetView();
	switch (V.Action)
	{
	case KwView::Slot::HoldCrack:
		if (V.HoC.Open)
		{
			KwCallButtons(F, R, true, "kaststudio");
			return;
		}
		break;
	case KwView::Slot::WelcomeRaid:
		if (KwSlotButton(F, "kaststudiowelcome", R, KwWelcomeLabel(V), KRed, Hex(0xffffff)))
		{
			F.S.StreamWelcomeRaid();
		}
		return;
	case KwView::Slot::RailCall:
		if (KwSlotButton(F, "kaststudiorail", R, kast::copy::RailCallButton(), KLime, KDeep))
		{
			F.S.StreamRailCall();
		}
		return;
	default: break;
	}
	if (V.SceneOpen)
	{
		if (KwSlotButton(F, "kaststudioscene", R, kast::copy::SetTheSceneButton(), KLav, KDeep))
		{
			F.S.StreamSetScene();
		}
		return;
	}
	const std::string Line = KwRailLine(V);
	if (!Line.empty())
	{
		F.UI.Text(Line, R.X + 2.0f, R.Y + 21.0f, Ts(12.0f, 800, KLime, Align::Left, Baseline::Alphabetic, false, R.W));
	}
}

void rlkast_detail::KastStudioToast(KastFrame& F, const Rect& Chat)
{
	using namespace rlnet_detail;
	KwLaptop& K = KwOf(&F.RL);
	const std::string& T = KwToast(F);
	if (T.empty())
	{
		return;
	}
	const double Age = F.Now - K.ToastAt;
	const float A = F.Previewing ? 1.0f : NetEase(Age / 0.3) * (1.0f - NetEase((Age - 5.4) / 0.6));
	const float A0 = F.C.GetAlpha();
	F.C.SetAlpha(A0 * A);
	const Rect B{Chat.X + 12.0f, Chat.Y + 56.0f, Chat.W - 24.0f, 34.0f};
	F.C.GlowRoundRect(B, 10.0f, NetA(KViolet, 0.4f), 12.0f);
	F.C.FillRoundRect(B, 10.0f, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, Hex(0x3b1d7a), Hex(0x2a1652)));
	F.C.StrokeRoundRect(B, 10.0f, KViolet, 1.0f);
	streamart::Emote(F.C, static_cast<int>(kast::Emote::Clap), B.X + 8.0f, B.Y + 6.0f, 22.0f);
	F.UI.Text(T, B.X + 38.0f, B.Y + 22.0f, Ts(13.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, B.W - 48.0f));
	F.C.SetAlpha(A0);
}

// ------------------------------------------------------------------ the preview's overlay

namespace rlkast_detail
{
/** Whether the deepest run is at its final table (Remaining within its table size; 9 when the size isn't known). */
bool KwFinalTable(const Session& S, double World)
{
	const KwView& V = S.Stream.GetView();
	int Size = 0;
	int Site = 0;
	KwRunFacts(S, World, Size, Site);
	return V.DeepRemaining > 0 && V.DeepRemaining <= (Size > 0 ? Size : 9);
}
} // namespace rlkast_detail

std::string rlkast_detail::KastStageWord(const Session& S, double World)
{
	return KwFinalTable(S, World) ? kast::copy::FinalTableChip() : std::string("LAST TWO TABLES");
}

bool rlkast_detail::KastIsBroadcast(const kast::Alert& A)
{
	if (A.Kind != kast::AlertKind::Raid || A.Text.empty())
	{
		return false;
	}
	for (int Site = 0; Site < static_cast<int>(net::SiteId::Count); ++Site)
	{
		if (A.Text == kast::copy::Broadcast(Site, A.Count))
		{
			return true;
		}
	}
	return false;
}

const char* rlkast_detail::KastBroadcastHeadline()
{
	return KwBroadcastTitle;
}

kast::Mood rlkast_detail::KastCamMood(const Session& S)
{
	const kast::Stream& St = S.Stream;
	return St.Live && St.GetView().EventMode && St.Face == kast::Mood::Focus ? kast::Mood::Hyped : St.Face;
}

float rlkast_detail::KastAlertY(const Session& S, const Rect& Prev, float Scale)
{
	const KwView& V = S.Stream.GetView();
	// The mockup's collision (kast_event_mode, frame B): at the top the alert sat on the EVENT bar and across the Hold or Crack
	// bar ({Prev.X + Prev.W - 300, Prev.Y + 44, 280, 40}); it drops below both while either is up.
	return S.Streaming() && (V.EventMode || V.HoC.Open) ? Prev.Y + 94.0f * Scale : Prev.Y + 20.0f * Scale;
}

namespace rlkast_detail
{
/** Hold or Crack's bar (StreamSide's strip and the preview): chat's calls as counts under 10 callers, shares from 10, and the
 *  hero's equity as a marker. */
void KwHocBar(Canvas& Cv, const Rect& Bar, const KwView::HoldCrack& H)
{
	using namespace rlnet_detail;
	const int Callers = H.Holds + H.Cracks;
	if (Callers <= 0)
	{
		// Nobody has called yet: an empty track, not a 50/50 split chat never made.
		Cv.FillRoundRect(Bar, Bar.H / 2.0f, KPanel2);
		Cv.StrokeRoundRect(Bar, Bar.H / 2.0f, NetA(KLav, 0.35f), 1.0f);
	}
	else
	{
		const float Share = Nf(static_cast<double>(H.Holds) / static_cast<double>(Callers));
		Cv.FillRoundRect(Bar, Bar.H / 2.0f, KRed);
		if (Share > 0.0f)
		{
			Cv.FillRoundRect({Bar.X, Bar.Y, std::max(Bar.H, Bar.W * Share), Bar.H}, Bar.H / 2.0f, KLime);
		}
	}
	if (H.Equity >= 0.0)
	{
		const float Ex = Bar.X + Bar.W * Nf(std::max(0.0, std::min(1.0, H.Equity)));
		Cv.FillRect({Ex - 1.0f, Bar.Y - 3.0f, 2.2f, Bar.H + 6.0f}, Hex(0xffffff));
		Cv.FillPolygon({{Ex - 4.0f, Bar.Y - 6.0f}, {Ex + 4.0f, Bar.Y - 6.0f}, {Ex, Bar.Y - 1.0f}}, Hex(0xffffff));
	}
}

std::string KwHocLabel(const KwView::HoldCrack& H)
{
	return H.Holds + H.Cracks >= 10 ? kast::copy::HoldOrCrackShares(H.Holds, H.Cracks) : kast::copy::HoldOrCrackCounts(H.Holds, H.Cracks);
}

/** The prestige diamonds: one to four by P (1 under 1, 2 under 2, 3 under 3, else 4). */
int KwDiamonds(double P)
{
	return std::max(1, std::min(4, static_cast<int>(std::ceil(P - 1e-9))));
}
} // namespace rlkast_detail

void rlkast_detail::KastOverlay(KastFrame& F, const Rect& Prev, float Scale, const Rect& Cam)
{
	using namespace rlnet_detail;
	const kast::Stream& St = F.S.Stream;
	if (!St.Live)
	{
		return;
	}
	const KwView& V = St.GetView();
	// The names a Nod lit (K11): a glowing tag beside the facecam for a world minute each.
	float Ny = Cam.Y + Cam.H - 34.0f * Scale;
	for (size_t I = 0; I < V.Lit.size() && I < 3; ++I)
	{
		const std::string& Who = V.Lit[V.Lit.size() - 1 - I];
		const float Nw = F.UI.Measure(Who, 14.0f * Scale, 900) + 46.0f * Scale;
		const Rect B{Cam.X - Nw - 10.0f * Scale, Ny, Nw, 30.0f * Scale};
		F.C.GlowRoundRect(B, 12.0f * Scale, NetA(KViolet, 0.45f), 10.0f * Scale);
		F.C.FillRoundRect(B, 12.0f * Scale, Hex(0x2a1652));
		F.C.StrokeRoundRect(B, 12.0f * Scale, KLav, 1.5f * Scale);
		streamart::Emote(F.C, static_cast<int>(kast::Emote::Love), B.X + 6.0f * Scale, B.Y + 5.0f * Scale, 20.0f * Scale);
		F.UI.Text(Who, B.X + 32.0f * Scale, B.Y + 20.0f * Scale, Ts(14.0f * Scale, 900, KLav));
		Ny -= 38.0f * Scale;
	}
	// Event Mode (K5): the EVENT bar across the top, "FINAL TABLE · $11 Big Stack · 7 left of 980", with the prestige diamonds.
	if (V.EventMode)
	{
		const float Eh = 34.0f;
		F.C.FillRect({Prev.X, Prev.Y, Prev.W, Eh}, Paint::Linear({Prev.X, 0.0f}, {Prev.X + Prev.W, 0.0f}, Rgba(74, 52, 8, 0.94f), Rgba(42, 30, 5, 0.94f)));
		F.C.FillRect({Prev.X, Prev.Y + Eh - 2.0f, Prev.W, 2.0f}, Paint::Linear({Prev.X, 0.0f}, {Prev.X + Prev.W, 0.0f}, KGold, KGold2));
		float Ex = Prev.X + 104.0f;
		Ex += NetSpaced(F.C, "EVENT", Ex, Prev.Y + 22.0f, 11.0f, 900, KGold, 2.0f) + 22.0f;
		const int Dia = KwDiamonds(V.Prestige);
		for (int K = 0; K < 4; ++K)
		{
			const float Dx = Ex + Nf(K) * 15.0f;
			const std::vector<Vec2> D = {{Dx, Prev.Y + 11.0f}, {Dx + 6.0f, Prev.Y + 17.0f}, {Dx, Prev.Y + 23.0f}, {Dx - 6.0f, Prev.Y + 17.0f}};
			if (K < Dia)
			{
				F.C.FillPolygon(D, KGold);
			}
			else
			{
				F.C.FillPolygon(D, Rgba(0, 0, 0, 0.35f));
				F.C.StrokePolyline(D, true, NetA(KGold, 0.8f), 1.0f);
			}
		}
		Ex += 4.0f * 15.0f + 10.0f;
		const bool Final = KwFinalTable(F.S, F.World);
		F.UI.Text(kast::copy::EventBar(Final, V.DeepEvent, V.DeepRemaining, V.DeepEntrants), Ex, Prev.Y + 23.0f, Ts(15.0f, 900, Hex(0xfff6dc), Align::Left, Baseline::Alphabetic, false, Prev.X + Prev.W - Ex - 12.0f));
	}
	// Hold or Crack's bar (K12) at {Prev.X + Prev.W - 300, Prev.Y + 44, 280, 40}.
	if (V.HoC.Open)
	{
		const Rect B{Prev.X + Prev.W - 300.0f, Prev.Y + 44.0f, 280.0f, 40.0f};
		F.C.GlowRoundRect(B, 10.0f, NetA(KViolet, 0.3f), 8.0f);
		F.C.FillRoundRect(B, 10.0f, Hex(0x140c24));
		F.C.StrokeRoundRect(B, 10.0f, KViolet, 1.5f);
		F.UI.Text(kast::copy::HoldOrCrackTitle(), B.X + 10.0f, B.Y + 14.0f, Ts(9.5f, 900, KLav));
		F.UI.Text(KwHocLabel(V.HoC), B.X + B.W - 10.0f, B.Y + 14.0f, Ts(10.0f, 800, KInk, Align::Right));
		KwHocBar(F.C, {B.X + 10.0f, B.Y + 21.0f, B.W - 20.0f, 11.0f}, V.HoC);
	}
}

// ------------------------------------------------------------------ StreamSide

float rlkast_detail::KastSideTop(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Stream& St = F.S.Stream;
	const KwView& V = St.GetView();
	const double T = F.Previewing ? 0.0 : F.Now;
	const bool HoC = V.HoC.Open;
	// Hold or Crack grows the strip to 96; heads-down (never at once: Hold or Crack needs attended minutes) adds a row under.
	const bool Quiet = V.HeadsDown && !HoC;
	const float Sh = HoC ? 96.0f : Quiet ? 74.0f : 58.0f;
	const Rect Top{R.X, R.Y, R.W, Sh};
	F.C.FillRoundRect(Top, 12.0f, KPanel);
	if (V.EventMode)
	{
		F.C.StrokeRoundRect(Top, 12.0f, NetA(KGold, 0.6f), 1.2f);
	}
	F.C.FillCircle(Top.X + 16.0f, Top.Y + 20.0f, 4.5f, NetA(KRed, 0.6f + 0.4f * Nf(std::sin(T * 4.0))));
	// "12 watching · 5 chatting" (K1, C12): the count every surface shares, and chatting never above it.
	const int N = KastShown(F.S, F.Now, F.Previewing);
	const std::string Cnt = KastCount(N);
	const float Cw = F.UI.Text(Cnt, Top.X + 28.0f, Top.Y + 26.0f, Ts(18.0f, 900, KInk, Align::Left, Baseline::Alphabetic, true));
	// The copy's "12 watching · 5 chatting", its number drawn big: the words after it.
	const std::string Wc = kast::copy::WatchingChatting(N, KwChatting(V, N));
	const size_t Sp = Wc.find(' ');
	const std::string Rest = Sp == std::string::npos ? Wc : Wc.substr(Sp + 1);
	const float Clock = F.UI.Measure(KastClock(St.Uptime(F.World)), 13.0f, 700, true);
	F.UI.Text(Rest, Top.X + 34.0f + Cw, Top.Y + 26.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, Top.W - 62.0f - Cw - Clock));
	F.UI.Text(KastClock(St.Uptime(F.World)), Top.X + Top.W - 14.0f, Top.Y + 26.0f, Ts(13.0f, 700, KMuted, Align::Right, Baseline::Alphabetic, true));
	// The tier word beside it (K4), then tonight's follows, or heads-down (K1).
	const KastHeat H = KastHeatNow(F.S, F.World);
	const Color Col = KastTierTint(H.Tier, T);
	KastThermo(F.C, Top.X + 13.0f, Top.Y + 33.0f, 16.0f, (H.Shown - 8.0) / 92.0, H.Live ? H.Ghost / 92.0 : 0.0, Col);
	const float Ww = F.UI.Text(KastTierWord(H.Tier), Top.X + 28.0f, Top.Y + 46.0f, Ts(12.0f, 800, Col));
	float Chip = 0.0f;
	if (V.RunAlertSent > 0)
	{
		// The Run Alert's counter, ticking as they arrive (K3).
		const std::string C = kast::copy::RunAlertStrip(V.RunAlertBack);
		Chip = F.UI.Measure(C, 10.0f, 800) + 16.0f;
		F.C.FillRoundRect({Top.X + Top.W - 14.0f - Chip, Top.Y + 34.0f, Chip, 17.0f}, 8.5f, NetA(KGold, 0.16f));
		F.UI.Text(C, Top.X + Top.W - 14.0f - Chip / 2.0f, Top.Y + 46.0f, Ts(10.0f, 800, KGold, Align::Center));
		Chip += 8.0f;
	}
	const float Lx = Top.X + 34.0f + Ww;
	const float Lw = std::max(10.0f, Top.X + Top.W - 14.0f - Chip - Lx);
	F.UI.Text("+" + kast::copy::Number(St.Tonight.Follows) + " follows tonight", Lx, Top.Y + 46.0f, Ts(12.0f, 700, KLime, Align::Left, Baseline::Alphabetic, false, Lw));
	if (Quiet)
	{
		// Under the count (§5.3 K1): the pace in-world words, never a speed.
		F.UI.Text(kast::copy::HeadsDown(), Top.X + 28.0f, Top.Y + 64.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, Top.W - 42.0f));
	}
	if (HoC)
	{
		// Hold or Crack (K12): the strip grows to 96 with chat's calls and the equity marker.
		const float Hy = Top.Y + 51.0f;
		F.C.FillRoundRect({Top.X + 10.0f, Hy, Top.W - 20.0f, 42.0f}, 8.0f, Hex(0x140c24));
		F.C.StrokeRoundRect({Top.X + 10.0f, Hy, Top.W - 20.0f, 42.0f}, 8.0f, NetA(KViolet, 0.6f), 1.0f);
		F.UI.Text(kast::copy::HoldOrCrackTitle(), Top.X + 20.0f, Hy + 13.0f, Ts(9.5f, 900, KLav));
		if (V.HoC.Equity >= 0.0)
		{
			F.UI.Text(kast::copy::YourEquity(V.HoC.Equity), Top.X + Top.W - 20.0f, Hy + 13.0f, Ts(10.5f, 800, Hex(0xffffff), Align::Right));
		}
		const Rect Bar{Top.X + 20.0f, Hy + 18.0f, Top.W - 40.0f, 9.0f};
		KwHocBar(F.C, Bar, V.HoC);
		F.UI.Text(KwHocLabel(V.HoC), Bar.X, Hy + 38.0f, Ts(10.5f, 800, KLime, Align::Left, Baseline::Alphabetic, false, Bar.W));
	}
	float Y = R.Y + Sh + 6.0f;
	// The one action slot (§5.1): Hold or Crack > Welcome the raid > Rail Call; the others wait as chips.
	switch (V.Action)
	{
	case KwView::Slot::HoldCrack:
		if (HoC)
		{
			KwCallButtons(F, {R.X, Y, R.W, 32.0f}, false, "kastside");
			Y += 38.0f;
		}
		break;
	case KwView::Slot::WelcomeRaid:
	{
		// With its countdown (C-K's L11): the button grows to hold "WELCOME THE RAID 1:52".
		const std::string Welcome = KwWelcomeLabel(V);
		const float WelcomeW = std::min(R.W, std::max(200.0f, F.UI.Measure(Welcome, 13.5f, 900) + 32.0f));
		if (KwSlotButton(F, "kastsidewelcome", {R.X, Y, WelcomeW, 32.0f}, Welcome, KRed, Hex(0xffffff)))
		{
			F.S.StreamWelcomeRaid();
		}
		Y += 38.0f;
		break;
	}
	case KwView::Slot::RailCall:
		if (KwSlotButton(F, "kastsiderail", {R.X, Y, std::min(R.W, 160.0f), 32.0f}, kast::copy::RailCallButton(), KLime, KDeep))
		{
			F.S.StreamRailCall();
		}
		Y += 38.0f;
		break;
	default: break;
	}
	const std::string Rail = KwRailLine(V);
	if (!Rail.empty())
	{
		F.UI.Text(Rail, R.X + 4.0f, Y + 13.0f, Ts(11.5f, 800, KLime, Align::Left, Baseline::Alphabetic, false, R.W - 8.0f));
		Y += 20.0f;
	}
	// The waiting chips.
	const int Waiting = V.Waiting & ~(1 << static_cast<int>(V.Action));
	if (Waiting != 0)
	{
		float Cx = R.X + 2.0f;
		Cx += F.UI.Text(KwWaitingChips, Cx, Y + 14.0f, Ts(10.5f, 700, KDim)) + 8.0f;
		const std::pair<KwView::Slot, std::pair<std::string, Color>> Chips[3] = {{KwView::Slot::HoldCrack, {KwHoldOrCrackChip, KViolet}},
			{KwView::Slot::WelcomeRaid, {KwWelcomeLabel(V), KRed}}, {KwView::Slot::RailCall, {kast::copy::RailCallButton(), KLime}}};
		for (const auto& Cp : Chips)
		{
			if (!(Waiting & (1 << static_cast<int>(Cp.first))))
			{
				continue;
			}
			const float W = F.UI.Measure(Cp.second.first, 10.0f, 800) + 18.0f;
			if (Cx + W > R.X + R.W)
			{
				break;
			}
			F.C.FillRoundRect({Cx, Y + 2.0f, W, 18.0f}, 9.0f, NetA(Cp.second.second, 0.14f));
			F.C.StrokeRoundRect({Cx, Y + 2.0f, W, 18.0f}, 9.0f, NetA(Cp.second.second, 0.6f), 1.0f);
			F.UI.Text(Cp.second.first, Cx + W / 2.0f, Y + 15.0f, Ts(10.0f, 800, Cp.second.second, Align::Center));
			Cx += W + 6.0f;
		}
		Y += 26.0f;
	}
	// The toast line: one a real minute.
	const std::string& Toast = KwToast(F);
	if (!Toast.empty())
	{
		F.C.FillRoundRect({R.X, Y, R.W, 22.0f}, 8.0f, NetA(KViolet, 0.18f));
		F.UI.Text(Toast, R.X + 10.0f, Y + 15.5f, Ts(11.5f, 800, KLav, Align::Left, Baseline::Alphabetic, false, R.W - 20.0f));
		Y += 28.0f;
	}
	return Y;
}

// ------------------------------------------------------------------ chat

void rlkast_detail::KastWatchingChatting(KastFrame& F, float Right, float Y)
{
	const KwView& V = F.S.Stream.GetView();
	const int N = KastShown(F.S, F.Now, F.Previewing);
	const std::string Chat = "  \xC2\xB7  " + kast::copy::Chatting(KwChatting(V, N));
	const float Cw = F.UI.Text(Chat, Right, Y, Ts(12.5f, 700, KLav, Align::Right));
	F.UI.Text(kast::copy::Watching(N), Right - Cw, Y, Ts(12.5f, 800, KInk, Align::Right));
}

std::string rlkast_detail::KastModsHere(const Session& S)
{
	if (S.Channel.Mods.empty())
	{
		return std::string();
	}
	int Here = 0;
	for (const kast::Moderator& M : S.Channel.Mods)
	{
		Here += S.Streaming() && KastModHere(S, M.Name) ? 1 : 0;
	}
	return kast::copy::ModsHere(Here, static_cast<int>(S.Channel.Mods.size()));
}

bool rlkast_detail::KastModHere(const Session& S, const std::string& Name)
{
	const std::vector<std::string>& Here = S.Stream.ModsHere();
	return std::find(Here.begin(), Here.end(), Name) != Here.end();
}

std::string rlkast_detail::KastThankLabel(const Session& S)
{
	const KwView& V = S.Stream.GetView();
	return S.Streaming() && V.Thanks10 >= 2 ? "Talk to chat " + kast::copy::ThankHeard() : std::string("Talk to chat");
}

bool rlkast_detail::KastLit(const Session& S, const std::string& Name)
{
	if (!S.Streaming())
	{
		return false;
	}
	const std::vector<std::string>& Lit = S.Stream.GetView().Lit;
	return std::find(Lit.begin(), Lit.end(), Name) != Lit.end();
}

void rlkast_detail::KastOpenNameMenu(KastFrame& F, const kast::ChatMsg& M, float X, float Y)
{
	if (F.Previewing)
	{
		return;
	}
	KwLaptop& K = KwOf(&F.RL);
	K.MenuMsg = M.Id;
	K.MenuWho = M.Who;
	K.MenuX = X;
	K.MenuY = Y;
}

bool rlkast_detail::KastNameMenuOpen(const RiverLine* RL)
{
	const auto It = KwLaptops().find(RL);
	return It != KwLaptops().end() && It->second.MenuMsg >= 0;
}

bool rlkast_detail::KastNameMenu(KastFrame& F, const Rect& Bounds)
{
	using namespace rlnet_detail;
	if (F.Previewing)
	{
		return false;
	}
	KwLaptop& K = KwOf(&F.RL);
	if (K.MenuMsg < 0)
	{
		return false;
	}
	const kast::Stream& St = F.S.Stream;
	const kast::ChatMsg* M = nullptr;
	for (const kast::ChatMsg& Line : St.Chat)
	{
		M = Line.Id == K.MenuMsg ? &Line : M;
	}
	if (!St.Live || !M || M->Deleted)
	{
		K.MenuMsg = -1;
		return false;
	}
	const float W = 196.0f;
	const float H = 132.0f;
	const float X = std::max(Bounds.X + 8.0f, std::min(K.MenuX, Bounds.X + Bounds.W - W - 8.0f));
	float Y = K.MenuY;
	if (Y + H > Bounds.Y + Bounds.H - 8.0f)
	{
		Y = std::max(Bounds.Y + 8.0f, K.MenuY - H - 26.0f);
	}
	const Rect Box{X, Y, W, H};
	F.C.GlowRoundRect({X + 3.0f, Y + 5.0f, W, H}, 10.0f, Rgba(0, 0, 0, 0.45f), 10.0f);
	F.C.FillRoundRect(Box, 10.0f, Hex(0x231a38));
	F.C.StrokeRoundRect(Box, 10.0f, KViolet, 1.2f);
	const KwView& V = St.GetView();
	F.UI.Text(M->Who, X + 14.0f, Y + 22.0f, Ts(13.0f, 900, Hex(M->NameColor), Align::Left, Baseline::Alphabetic, false, W - 110.0f));
	F.UI.Text(kast::copy::NodsLeft(V.NodsLeft), X + W - 12.0f, Y + 22.0f, Ts(10.5f, 700, KMuted, Align::Right));
	bool Done = false;
	// Nod (K11): their name lights on the overlay for a world minute; 3 a stream, one a member.
	bool CanNod = V.NodsLeft > 0 && !KastLit(F.S, M->Who);
	// Anyone Nodded tonight, a guest included (View::Nodded, C-K's L11), can't take a second one.
	CanNod = CanNod && std::find(V.Nodded.begin(), V.Nodded.end(), M->Who) == V.Nodded.end();
	// A member already Nodded tonight, or out of the room now, can't take one (the verb refuses): the View's flags say (4 Nodded,
	// 8 present), read for the one name while the menu is open.
	const std::vector<kast::Member>& Members = F.S.Channel.Members;
	for (size_t I = 0; CanNod && V.MemberFlags.size() == Members.size() && I < Members.size(); ++I)
	{
		if (Members[I].Name == M->Who)
		{
			CanNod = (V.MemberFlags[I] & 4) == 0 && (V.MemberFlags[I] & 8) != 0;
			break;
		}
	}
	const Rect Nod{X + 8.0f, Y + 32.0f, W - 16.0f, 30.0f};
	const Ui::ClickState Ns = F.UI.Clickable("kastnod" + std::to_string(M->Id), Nod, CanNod);
	F.C.FillRoundRect(Nod, 8.0f, CanNod ? (Ns.Hover ? Mix(KViolet, Hex(0xffffff), 0.15f) : KViolet) : NetA(KViolet, 0.2f));
	streamart::Emote(F.C, static_cast<int>(kast::Emote::Love), Nod.X + 8.0f, Nod.Y + 5.0f, 20.0f);
	F.UI.Text(kast::copy::NodButton(), Nod.X + 34.0f, Nod.Y + 20.0f, Ts(13.5f, 900, CanNod ? Hex(0xffffff) : KDim));
	F.UI.Text("they light up", Nod.X + Nod.W - 8.0f, Nod.Y + 20.0f, Ts(10.5f, 600, CanNod ? KLav : KDim, Align::Right));
	if (Ns.Clicked)
	{
		F.S.StreamNod(M->Who);
		Done = true;
	}
	const Rect To{X + 8.0f, Y + 66.0f, W - 16.0f, 26.0f};
	const Ui::ClickState Ts0 = F.UI.Clickable("kastnameto" + std::to_string(M->Id), To);
	if (Ts0.Hover)
	{
		F.C.FillRoundRect(To, 7.0f, NetA(KRed, 0.25f));
	}
	F.UI.Text("Timeout", To.X + 8.0f, To.Y + 18.0f, Ts(13.0f, 700, KInk));
	if (Ts0.Clicked)
	{
		F.S.StreamTimeout(M->Id);
		Done = true;
	}
	F.C.FillRect({X + 10.0f, Y + 96.0f, W - 20.0f, 1.0f}, KLine);
	const bool CanPromote = !F.S.Channel.IsMod(M->Who) && static_cast<int>(F.S.Channel.Mods.size()) < kast::Stream::MaxMods;
	const Rect Pr{X + 8.0f, Y + 100.0f, W - 16.0f, 26.0f};
	const Ui::ClickState Ps = F.UI.Clickable("kastnamemod" + std::to_string(M->Id), Pr, CanPromote);
	if (Ps.Hover && CanPromote)
	{
		F.C.FillRoundRect(Pr, 7.0f, NetA(KGreen, 0.25f));
	}
	F.UI.Text("Promote to mod", Pr.X + 8.0f, Pr.Y + 18.0f, Ts(13.0f, 700, CanPromote ? KInk : KDim));
	if (Ps.Clicked)
	{
		F.S.StreamPromote(M->Who);
		Done = true;
	}
	const bool Inside = F.UI.Hover(Box);
	if (Done || (F.UI.Ptr.Pressed && !Inside))
	{
		K.MenuMsg = -1;
	}
	return Inside;
}

// ------------------------------------------------------------------ the end card

namespace rlkast_detail
{
/** The end card's money and the bank line: the Kast payout booked at this stream's end (the ledger's streaming line). */
Chips KwPaidTonight(const Session& S)
{
	const kast::Stream& St = S.Stream;
	const double End = St.StartWorld + St.Last.Minutes;
	for (size_t I = 0; I < S.Life.Ledger.size() && I < 12; ++I)
	{
		const life::LedgerEntry& E = S.Life.Ledger[I];
		if (E.Kind == 6 && E.At >= End - 1.0 && E.Amount > 0)
		{
			return E.Amount;
		}
	}
	return 0;
}

/** The newest tip by name tonight (before Affiliate the card names the tipper: "Dee tipped $5"). */
const kast::Alert* KwOneTip(const kast::Stream& St, int& Tippers)
{
	const kast::Alert* One = nullptr;
	std::vector<std::string> Who;
	for (const kast::Alert& A : St.Feed)
	{
		if (A.Kind == kast::AlertKind::Tip && !A.Who.empty())
		{
			One = One ? One : &A;
			if (std::find(Who.begin(), Who.end(), A.Who) == Who.end())
			{
				Who.push_back(A.Who);
			}
		}
	}
	Tippers = static_cast<int>(Who.size());
	return One;
}

/** The next follower step the card ends on (the FOLLOWERS tile's steps). */
int KwNextStep(int Followers)
{
	static const int Steps[] = {10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000};
	for (const int Step : Steps)
	{
		if (Followers < Step)
		{
			return Step;
		}
	}
	return Followers + 1;
}

/** Whether tonight's Peak (or Follows) beat every older row in the log. The newest row is tonight's only when the stream wrote one
 *  (its Start is tonight's StartWorld); a night under 60 world minutes writes none, and then every row is an older night. */
bool KwBestEver(const kast::Channel& Ch, double Start, int Value, bool Peak)
{
	const size_t From = !Ch.Log.empty() && Ch.Log.front().Start == Start ? 1 : 0;
	if (Ch.Log.size() <= From)
	{
		return false;
	}
	for (size_t I = From; I < Ch.Log.size(); ++I)
	{
		if ((Peak ? Ch.Log[I].Peak : Ch.Log[I].Follows) >= Value)
		{
			return false;
		}
	}
	return Value > 0;
}

/** A labelled row: a small spaced caps label and the line after it. */
float KwLabel(KastFrame& F, const std::string& Label, float X, float Y, bool Draw)
{
	if (Draw)
	{
		rlnet_detail::NetSpaced(F.C, Label, X, Y, 9.5f, 900, KDim, 1.2f);
	}
	return 0.0f;
}
} // namespace rlkast_detail

float rlkast_detail::KastEndCardBody(KastFrame& F, float X, float Y0, float W, bool Draw)
{
	using namespace rlnet_detail;
	const kast::Summary& L = F.S.Stream.Last;
	const kast::Channel& Ch = F.S.Channel;
	const kast::Stream& St = F.S.Stream;
	const float R = X + W;
	float Y = Y0;
	// The peak first (K17): "Peak 41 at the final table, 9:48 PM", the forecast's verdict beside it.
	{
		// When the peak's minute isn't known (a summary from before the Room), the number stands alone.
		const bool Known = L.PeakAt >= 0.0 || L.PeakMoment >= 0;
		const std::string Clock = L.PeakAt >= 0.0 ? ClockString(St.StartWorld + L.PeakAt) : std::string();
		const std::string Num = std::to_string(L.Peak);
		const std::string Line = Known ? kast::copy::PeakLine(L.Peak, L.PeakMoment, Clock) : Num;
		const size_t At = Line.find(Num);
		if (Draw)
		{
			KwLabel(F, "PEAK", X, Y + 14.0f, true);
			if (At != std::string::npos)
			{
				const float Nw = F.UI.Text(Num, X, Y + 64.0f, Ts(52.0f, 900, KRed, Align::Left, Baseline::Alphabetic, true));
				std::string After = Line.substr(At + Num.size());
				After = !After.empty() && After[0] == ' ' ? After.substr(1) : After;
				if (!After.empty())
				{
					F.UI.Text(After, X + Nw + 14.0f, Y + 46.0f, Ts(18.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, W - Nw - 200.0f));
				}
			}
			else
			{
				F.UI.Text(Line, X, Y + 50.0f, Ts(22.0f, 900, KInk, Align::Left, Baseline::Alphabetic, false, W - 190.0f));
			}
			if (KwBestEver(Ch, St.StartWorld, L.Peak, true))
			{
				const float Nw = At != std::string::npos ? F.UI.Measure(Num, 52.0f, 900, true) : 0.0f;
				F.UI.Text("your best room ever", X + Nw + 14.0f, Y + 66.0f, Ts(13.0f, 600, KMuted));
			}
			if (L.Fc.Hi > 0.0)
			{
				const std::string Verdict = NetUpper(kast::copy::ForecastVerdict(L.Avg, L.Fc));
				const float Pw = F.UI.Measure(Verdict, 10.5f, 800) + 18.0f;
				NetPill(F.C, Verdict, R - Pw, Y, L.Avg > L.Fc.Hi ? KLime : KLav, false, 10.5f);
				F.UI.Text("average " + kast::copy::Number(L.Avg) + " \xC2\xB7 " + kast::copy::ForecastShort(L.Fc.Lo, L.Fc.Hi), R, Y + 36.0f, Ts(12.0f, 700, KMuted, Align::Right));
			}
		}
		Y += 84.0f;
	}
	// The night's numbers: the average, the new followers, the new subs, who chatted (C12).
	{
		if (Draw)
		{
			F.C.FillRect({X, Y, W, 1.0f}, KLine);
			struct Num
			{
				const char* Label;
				std::string Value;
				Color Col;
				std::string Sub;
			};
			// "+46 · best night ever" is KastCopy's line (NewFollowersLine): its number big, the words after the dot beside it.
			const std::string Fl = kast::copy::NewFollowersLine(L.Follows, KwBestEver(Ch, St.StartWorld, L.Follows, false));
			const size_t FlDot = Fl.find(" \xC2\xB7 ");
			const Num Nums[4] = {{"AVERAGE", kast::copy::Number(L.Avg), KInk, std::string()},
				{"NEW FOLLOWERS", FlDot == std::string::npos ? Fl : Fl.substr(0, FlDot), KViolet, FlDot == std::string::npos ? std::string() : Fl.substr(FlDot + 4)},
				{"NEW SUBS", "+" + kast::copy::Number(L.Subs + L.Gifted), KLime, L.Gifted > 0 ? kast::copy::Number(L.Gifted) + " gifted" : std::string()},
				{"CHATTED TONIGHT", kast::copy::Number(L.Chatted), KLav, L.Chatted == 1 ? std::string("person") : std::string("people")}};
			for (int K = 0; K < 4; ++K)
			{
				const float Nx = X + Nf(K) * (W / 4.0f);
				NetSpaced(F.C, Nums[K].Label, Nx, Y + 26.0f, 9.5f, 900, KDim, 1.2f);
				const float Vw = F.UI.Text(Nums[K].Value, Nx, Y + 60.0f, Ts(28.0f, 900, Nums[K].Col, Align::Left, Baseline::Alphabetic, true));
				if (!Nums[K].Sub.empty())
				{
					F.UI.Text(Nums[K].Sub, Nx + Vw + 8.0f, Y + 58.0f, Ts(11.5f, 700, KMuted, Align::Left, Baseline::Alphabetic, false, W / 4.0f - Vw - 12.0f));
				}
			}
		}
		Y += 76.0f;
	}
	// Where they came from (K8): the bar, its legend, who followed from where, who came back.
	{
		const std::vector<std::pair<KwView::Source, int>> Came = KwTopSources(L.Sources);
		if (!Came.empty())
		{
			int Total = 0;
			for (const auto& C : Came)
			{
				Total += C.second;
			}
			if (Draw)
			{
				const float Lw = F.UI.Text(kast::copy::CameBecauseOf(), X, Y + 14.0f, Ts(12.5f, 700, KDim)) + 12.0f;
				float Bx = X + Lw;
				const float Bw = W - Lw;
				for (const auto& C : Came)
				{
					const float Sw = Bw * Nf(static_cast<double>(C.second) / static_cast<double>(std::max(1, Total)));
					F.C.FillRoundRect({Bx, Y + 3.0f, std::max(2.0f, Sw - 2.0f), 12.0f}, 3.0f, KwSourceColor(static_cast<int>(C.first)));
					Bx += Sw;
				}
			}
			// The legend wraps the same way measured and drawn, so the card's height holds what it draws.
			float Lx = X;
			float Ly = Y + 36.0f;
			for (const auto& C : Came)
			{
				const std::string Lab = kast::copy::SourceCause(C.first) + " " + kast::copy::Number(C.second);
				const float Tw = F.UI.Measure(Lab, 12.0f, 600) + 30.0f;
				if (Lx + Tw > R && Lx > X)
				{
					Lx = X;
					Ly += 18.0f;
				}
				if (Draw)
				{
					F.C.FillRoundRect({Lx, Ly - 9.0f, 9.0f, 9.0f}, 2.0f, KwSourceColor(static_cast<int>(C.first)));
					F.UI.Text(Lab, Lx + 14.0f, Ly, Ts(12.0f, 600, KInk));
				}
				Lx += Tw;
			}
			Y = Ly - 36.0f + 46.0f;
		}
		const std::vector<std::pair<KwView::Source, int>> Followed = KwTopSources(L.FollowSources);
		if (!Followed.empty())
		{
			std::string Line = kast::copy::FollowedFrom();
			for (size_t I = 0; I < Followed.size(); ++I)
			{
				Line += (I == 0 ? " " : " \xC2\xB7 ") + kast::copy::SourceCause(Followed[I].first) + " " + kast::copy::Number(Followed[I].second);
			}
			if (Draw)
			{
				F.UI.Text(Line, X, Y + 6.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, W));
			}
			Y += 20.0f;
		}
		if (L.Returning > 0 || L.NewFaces > 0)
		{
			std::string Line = "Came back " + kast::copy::Number(L.Returning) + " \xC2\xB7 new faces " + kast::copy::Number(L.NewFaces);
			if (L.NewRegulars > 0)
			{
				Line += " \xC2\xB7 " + kast::copy::Number(L.NewRegulars) + (L.NewRegulars == 1 ? " new regular" : " new regulars");
			}
			if (Draw)
			{
				F.UI.Text(Line, X, Y + 6.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, W));
			}
			Y += 20.0f;
		}
		Y += 6.0f;
	}
	// The roll call: the regulars who sweated the whole run.
	{
		const std::string Roll = kast::copy::RollCall(L.Regulars);
		if (!Roll.empty())
		{
			if (Draw)
			{
				F.C.FillRoundRect({X, Y, W, 54.0f}, 12.0f, NetA(KViolet, 0.12f));
				const int Shown = static_cast<int>(std::min<size_t>(5, L.Regulars.size()));
				for (int I = Shown - 1; I >= 0; --I)
				{
					F.C.FillCircle(X + 26.0f + Nf(I) * 20.0f, Y + 27.0f, 14.0f, Hex(0x221640));
					DrawAvatar(F.C, X + 26.0f + Nf(I) * 20.0f, Y + 27.0f, 12.0f, AvatarFor(L.Regulars[static_cast<size_t>(I)]));
				}
				const float Tx = X + 26.0f + Nf(Shown) * 20.0f + 12.0f;
				F.UI.Text(Roll, Tx, Y + 24.0f, Ts(15.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, R - Tx - 12.0f));
				F.UI.Text(KwRailLabel, Tx, Y + 43.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, R - Tx - 12.0f));
			}
			Y += 70.0f;
		}
	}
	// Heat's move and the night's best caller.
	{
		const bool Heat = L.HeatBefore > 0.0;
		const bool Caller = L.BestCallsOf > 0 && !L.BestCaller.empty();
		if (Heat || Caller)
		{
			if (Draw && Heat)
			{
				// Where Heat stands now: tonight's log row holds the committed Heat (the commit, the night's raids, a record peak),
				// the tier every surface shows from here on; a night under 60 world minutes writes no row and moves by the commit.
				const bool HeatRow = !Ch.Log.empty() && Ch.Log.front().Start == St.StartWorld && Ch.Log.front().HeatAfter >= 0.0;
				const double After = std::max(8.0, std::min(100.0, HeatRow ? Ch.Log.front().HeatAfter : L.HeatBefore + L.GainSoFar));
				const double HeatBy = HeatRow ? After - L.HeatBefore : L.GainSoFar;
				const int T0 = kast::growth::Tier(L.HeatBefore);
				const int T1 = kast::growth::Tier(After);
				const Color Col = KastTierTint(T1, 0.0);
				KastThermo(F.C, X, Y - 2.0f, 46.0f, (After - 8.0) / 92.0, 0.0, Col);
				NetSpaced(F.C, kast::copy::HeatTitle(), X + 30.0f, Y + 8.0f, 9.5f, 900, KDim, 1.2f);
				// The night's own why line once the Room commits it (Summary::Why, C-K's L5), else what the channel's facts say. A night
				// under 60 world minutes commits nothing and reads no forecast (§7), so the Room's lines speak of another night or fall
				// back to "Steady: nights near your forecast" beside this card's UNDER YOUR FORECAST: the channel's facts then.
				const std::array<std::string, 2> Why = L.Minutes >= 60.0 && !L.Why[0].empty() ? L.Why : KwWhyOffline(Ch, F.World);
				if (!Why[0].empty())
				{
					F.UI.Text(Why[0], X + 74.0f, Y + 8.0f, Ts(11.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, W / 2.0f - 80.0f));
				}
				F.UI.Text(kast::copy::HeatMove(T0, T1, HeatBy), X + 30.0f, Y + 32.0f, Ts(16.0f, 900, Col, Align::Left, Baseline::Alphabetic, false, W / 2.0f - 30.0f));
				KastHeatBar(F.C, {X + 30.0f, Y + 42.0f, 250.0f, 8.0f}, After, 0.0);
			}
			if (Draw && Caller)
			{
				const float Mx = X + W / 2.0f + 40.0f;
				NetSpaced(F.C, "HOLD OR CRACK", Mx, Y + 8.0f, 9.5f, 900, KDim, 1.2f);
				F.UI.Text(kast::copy::BestCaller(L.BestCaller, L.BestCalls, L.BestCallsOf), Mx, Y + 32.0f, Ts(17.0f, 900, KInk, Align::Left, Baseline::Alphabetic, false, R - Mx));
				F.UI.Text("the night's best caller", Mx, Y + 50.0f, Ts(12.0f, 600, KMuted));
			}
			Y += 66.0f;
		}
	}
	// Raids with their 15-minute retention, and the rank climb.
	{
		const bool Raids = !L.RaidsHeld.empty();
		const bool Rank = L.RankStart > 0 && L.RankBest > 0;
		if (Raids || Rank)
		{
			if (Draw)
			{
				F.C.FillRect({X, Y, W, 1.0f}, KLine);
			}
			Y += 18.0f;
			if (Raids)
			{
				if (Draw)
				{
					NetSpaced(F.C, "RAIDS", X, Y + 4.0f, 9.5f, 900, KDim, 1.2f);
				}
				for (size_t I = 0; I < L.RaidsHeld.size() && I < 3; ++I)
				{
					const kast::Summary::RaidHeld& Rh = L.RaidsHeld[I];
					if (Draw)
					{
						// The Broadcast is the site's own send (K6: "RiverLine Live sent 41 viewers"), not a raid: its line says sent.
						const std::string Held = Rh.Broadcast ? kast::copy::BroadcastRetention(Rh.From, Rh.Size, Rh.At15, Rh.Followed)
															  : kast::copy::RaidRetention(Rh.From, Rh.Size, Rh.At15, Rh.Followed);
						F.UI.Text(Held, X + 70.0f, Y + 4.0f, Ts(13.5f, 700, KInk, Align::Left, Baseline::Alphabetic, false, W - 70.0f));
					}
					Y += 22.0f;
				}
				Y += 8.0f;
			}
			if (Rank)
			{
				if (Draw)
				{
					NetSpaced(F.C, "RANK", X, Y + 4.0f, 9.5f, 900, KDim, 1.2f);
					F.UI.Text(kast::copy::RankClimb(L.RankStart, L.RankBest), X + 70.0f, Y + 4.0f, Ts(15.0f, 900, L.RankBest < L.RankStart ? KLime : KInk, Align::Left, Baseline::Alphabetic, false, W - 70.0f));
				}
				Y += 30.0f;
			}
		}
	}
	// The money (K20): tonight apart from what came in while away, pending before the second rent, what went to the bank,
	// sponsor pay on its own lines.
	{
		if (Draw)
		{
			F.C.FillRect({X, Y, W, 1.0f}, KLine);
		}
		Y += 22.0f;
		int Tippers = 0;
		const kast::Alert* Tip = KwOneTip(St, Tippers);
		const bool Earned = L.TipCents + L.BitCents + L.SubCents + L.AdCents > 0;
		if (Draw && Earned)
		{
			NetSpaced(F.C, "TONIGHT", X, Y + 6.0f, 9.5f, 900, KDim, 1.2f);
			const std::string Line = !Ch.Affiliate && Tip && Tippers == 1 ? kast::copy::Tipped(Tip->Who, L.TipCents) : kast::copy::TipsTonight(L.TipCents, L.BitCents, Ch.Affiliate);
			const float Lw = F.UI.Text(Line, X + 88.0f, Y + 7.0f, Ts(13.5f, 700, KInk, Align::Left, Baseline::Alphabetic, false, W - 200.0f));
			if (Ch.Affiliate && (L.SubCents > 0 || L.AdCents > 0))
			{
				F.UI.Text("subs " + kast::copy::Dollars(L.SubCents) + " \xC2\xB7 ads " + kast::copy::Dollars(L.AdCents), X + 100.0f + Lw, Y + 7.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, W - 210.0f - Lw));
			}
			F.UI.Text(Money(L.TipCents + L.BitCents + L.SubCents + L.AdCents), R, Y + 7.0f, Ts(16.0f, 900, KInk, Align::Right, Baseline::Alphabetic, true));
		}
		Y += Earned ? 28.0f : 0.0f;
		if (L.RenewCents > 0)
		{
			if (Draw)
			{
				F.UI.Text(kast::copy::RenewalsAway(L.RenewCents), X, Y + 7.0f, Ts(13.5f, 700, KInk));
				F.UI.Text(Money(L.RenewCents), R, Y + 7.0f, Ts(16.0f, 900, KInk, Align::Right, Baseline::Alphabetic, true));
			}
			Y += 28.0f;
		}
		if (Ch.PendingCents > 0)
		{
			if (Draw)
			{
				F.UI.Text(kast::copy::PendingLine(Ch.PendingCents), X, Y + 7.0f, Ts(13.5f, 800, KGold));
			}
			Y += 28.0f;
		}
		if (Draw)
		{
			F.UI.Text(KwSentToBank, X, Y + 10.0f, Ts(15.0f, 700, KMuted));
			F.UI.Text(Money(KwPaidTonight(F.S)), R, Y + 14.0f, Ts(30.0f, 900, KLime, Align::Right, Baseline::Alphabetic, true));
		}
		Y += 26.0f;
		for (const kast::Summary::SponsorPay& P : L.SponsorPaid)
		{
			const kast::Sponsor* Sp = kast::FindSponsor(P.Id);
			const std::string Brand = Sp ? Sp->Brand : P.Id;
			if (P.ReadCents > 0)
			{
				if (Draw)
				{
					F.UI.Text(kast::copy::SponsorReadLabel(Brand) + " \xC2\xB7 " + kast::copy::Dollars(P.ReadCents) + " \xC2\xB7 " + KwSponsorSub, X, Y + 6.0f,
						Ts(11.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, W - 180.0f));
				}
				Y += 18.0f;
			}
			if (P.HourCents > 0)
			{
				if (Draw)
				{
					const int Hours = std::max(1, static_cast<int>(std::lround(L.Minutes / 60.0)));
					F.UI.Text(kast::copy::SponsorHoursLabel(Brand, Hours) + " \xC2\xB7 " + kast::copy::Dollars(P.HourCents) + " \xC2\xB7 " + KwSponsorSub, X, Y + 6.0f,
						Ts(11.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, W - 180.0f));
				}
				Y += 18.0f;
			}
		}
		Y += 10.0f;
	}
	// Tonight's one-time firsts (the first stranger's tip, the first raid in: Summary::Firsts as the Room posted them live, C-K's
	// M3), one line each, so the card says why its sting played.
	for (const std::string& First : L.Firsts)
	{
		if (Draw)
		{
			F.UI.Text(First, X, Y + 6.0f, Ts(13.5f, 800, KGold, Align::Left, Baseline::Alphabetic, false, W));
		}
		Y += 24.0f;
	}
	// The next goal, partly filled.
	{
		const int Goal = KwNextStep(Ch.Followers);
		if (Draw)
		{
			const float Gw = F.UI.Text(kast::copy::NextGoal(Goal - Ch.Followers, Goal), X, Y + 10.0f, Ts(14.0f, 800, KInk)) + 18.0f;
			const float Bw = std::min(230.0f, W - Gw - 120.0f);
			const float P = Nf(std::min(1.0, static_cast<double>(Ch.Followers) / static_cast<double>(std::max(1, Goal))));
			F.C.FillRoundRect({X + Gw, Y, Bw, 10.0f}, 5.0f, KPanel2);
			F.C.FillRoundRect({X + Gw, Y, std::max(10.0f, Bw * P), 10.0f}, 5.0f, KViolet);
			F.UI.Text(kast::copy::Number(Ch.Followers) + " / " + kast::copy::Number(Goal), X + Gw + Bw + 10.0f, Y + 10.0f, Ts(12.0f, 700, KMuted, Align::Left, Baseline::Alphabetic, true));
		}
		Y += 28.0f;
	}
	if (L.Trolls > 0)
	{
		if (Draw)
		{
			F.UI.Text(KwTrollsLine(L.Trolls, !Ch.Mods.empty()), X, Y + 6.0f, Ts(12.5f, 600, KDim));
		}
		Y += 22.0f;
	}
	return Y - Y0;
}

float rlkast_detail::KastEndCardClip(KastFrame& F, float X, float Y, float W, bool Draw)
{
	using namespace rlnet_detail;
	const kast::Summary& L = F.S.Stream.Last;
	if (L.BestClip.empty() && !L.ClipItOpen)
	{
		return 0.0f;
	}
	if (Draw)
	{
		if (!L.BestClip.empty())
		{
			F.UI.Text(kast::copy::ClipOfTheNight(L.BestClip, L.BestClipLane), X, Y + 14.0f, Ts(13.5f, 700, KViolet, Align::Left, Baseline::Alphabetic, false, L.ClipItOpen ? W - 150.0f : W));
		}
		if (L.ClipItOpen)
		{
			// Clip of the Night (K15): one click clips the night's best moment, even at 0 viewers.
			const Rect B{X + W - 130.0f, Y, 130.0f, 26.0f};
			if (KwButton(F, "kastclipit", B, kast::copy::ClipItButton(), KViolet, Hex(0xffffff), true, 12.5f))
			{
				F.S.StreamClipIt();
			}
		}
	}
	return 36.0f;
}

// ------------------------------------------------------------------ the Community page

namespace rlkast_detail
{
void KwBox(KastFrame& F, const Rect& R, const std::string& Title, const std::string& Right)
{
	F.C.FillRoundRect(R, 16.0f, KPanel);
	rlnet_detail::NetSpaced(F.C, Title, R.X + 20.0f, R.Y + 30.0f, 10.0f, 900, KViolet, 1.6f);
	if (!Right.empty())
	{
		F.UI.Text(Right, R.X + R.W - 20.0f, R.Y + 30.0f, Ts(12.0f, 700, KMuted, Align::Right));
	}
}
} // namespace rlkast_detail

void rlkast_detail::KastCommunityHeat(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	KwBox(F, R, kast::copy::HeatTitle(), KwHeatRight);
	const KastHeat H = KastHeatNow(F.S, F.World);
	const Color Col = KastTierTint(H.Tier, F.Previewing ? 0.0 : F.Now);
	KastThermo(F.C, R.X + 22.0f, R.Y + 42.0f, 62.0f, (H.Shown - 8.0) / 92.0, H.Live ? H.Ghost / 92.0 : 0.0, Col);
	F.UI.Text(KastTierWord(H.Tier), R.X + 64.0f, R.Y + 74.0f, Ts(28.0f, 900, Col));
	F.UI.Text(kast::copy::HeatEffects(static_cast<int>(std::lround(H.Shown))), R.X + 64.0f, R.Y + 96.0f, Ts(13.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, 330.0f));
	for (int I = 0; I < 2; ++I)
	{
		if (H.Why[static_cast<size_t>(I)].empty())
		{
			continue;
		}
		const Rect B{R.X + 420.0f, R.Y + 46.0f + Nf(I) * 34.0f, R.W - 440.0f, 28.0f};
		F.C.FillRoundRect(B, 8.0f, NetA(Col, 0.12f));
		F.UI.Text(H.Why[static_cast<size_t>(I)], B.X + 14.0f, B.Y + 19.0f, Ts(13.0f, 700, KInk, Align::Left, Baseline::Alphabetic, false, B.W - 24.0f));
	}
	// Heat over the last 30 streams (the log's HeatAfter, oldest first), on the tiers' bands; final tables marked.
	const Rect G{R.X + 20.0f, R.Y + 118.0f, R.W - 40.0f, R.H - 152.0f};
	std::vector<std::pair<double, bool>> Pts;
	for (size_t I = Ch.Log.size(); I-- > 0;)
	{
		const kast::StreamLog& Lg = Ch.Log[I];
		if (Lg.HeatAfter >= 0.0)
		{
			Pts.push_back({Lg.HeatAfter, Lg.PeakMoment == static_cast<int>(kast::Moment::FinalTable) || Lg.PeakMoment == static_cast<int>(kast::Moment::Top3) ||
											  Lg.PeakMoment == static_cast<int>(kast::Moment::Win)});
		}
	}
	if (Pts.size() > 30)
	{
		Pts.erase(Pts.begin(), Pts.end() - 30);
	}
	static const double Lo[5] = {8.0, 20.0, 35.0, 55.0, 75.0};
	static const double Hi[5] = {20.0, 35.0, 55.0, 75.0, 100.0};
	auto Py = [&](double V) { return G.Y + G.H - G.H * Nf((std::max(8.0, std::min(100.0, V)) - 8.0) / 92.0); };
	for (int I = 0; I < 5; ++I)
	{
		const Color Tc = KastTierTint(I, 0.0);
		F.C.FillRect({G.X, Py(Hi[I]), G.W, Py(Lo[I]) - Py(Hi[I])}, NetA(Tc, 0.06f));
		F.UI.Text(KastTierWord(I), G.X + G.W - 6.0f, Py(Hi[I]) + 12.0f, Ts(9.5f, 800, NetA(Tc, 0.8f), Align::Right));
	}
	if (Pts.size() >= 2)
	{
		std::vector<Vec2> Line;
		for (size_t I = 0; I < Pts.size(); ++I)
		{
			Line.push_back({G.X + G.W * Nf(I) / Nf(Pts.size() - 1), Py(Pts[I].first)});
		}
		F.C.StrokePolyline(Line, false, Hex(0xf59e0b), 2.2f, true);
		for (size_t I = 0; I < Pts.size(); ++I)
		{
			if (Pts[I].second)
			{
				F.C.FillCircle(Line[I].X, Line[I].Y, 4.5f, Hex(0xef6c2f));
				F.UI.Text("final table", Line[I].X, Line[I].Y - 9.0f, Ts(10.5f, 800, Hex(0xef6c2f), Align::Center));
			}
		}
	}
	else
	{
		F.UI.Text(KwHeatNoHistory, G.X + G.W / 2.0f, G.Y + G.H / 2.0f + 4.0f, Ts(13.0f, 600, KMuted, Align::Center));
	}
	F.UI.Text(kast::copy::ColdMeans(), R.X + 20.0f, R.Y + R.H - 14.0f, Ts(11.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
}

void rlkast_detail::KastCommunityPing(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	KwBox(F, R, kast::copy::PingTitle(), KwPingRight);
	// The last stream's ping (tonight's while live): its reach and the three parts it reached.
	const kast::Stream& St = F.S.Stream;
	int Reached = St.Last.PingReached;
	int Of = St.Last.PingOf;
	std::array<int, 3> Parts = St.Last.PingParts;
	if (St.Live && St.GetView().PingOf > 0)
	{
		Reached = St.GetView().PingReached;
		Of = St.GetView().PingOf;
		Parts = St.Tonight.PingParts;
	}
	if (Of <= 0)
	{
		F.UI.Text(KwPingNone, R.X + 20.0f, R.Y + 70.0f, Ts(14.0f, 600, KMuted));
		return;
	}
	const int Pct = static_cast<int>(std::lround(100.0 * Reached / std::max(1, Of)));
	const std::string Head = kast::copy::PingReach(Pct);
	const std::string P = std::to_string(Pct) + "%";
	const size_t At = Head.rfind(P);
	if (At != std::string::npos && At > 0)
	{
		std::string Lead = Head.substr(0, At);
		Lead = !Lead.empty() && Lead.back() == ' ' ? Lead.substr(0, Lead.size() - 1) : Lead;
		const float Hw = F.UI.Text(Lead, R.X + 20.0f, R.Y + 64.0f, Ts(15.0f, 800, KInk));
		F.UI.Text(P, R.X + 30.0f + Hw, R.Y + 64.0f, Ts(22.0f, 900, KLime, Align::Left, Baseline::Alphabetic, true));
	}
	else
	{
		F.UI.Text(Head, R.X + 20.0f, R.Y + 64.0f, Ts(15.0f, 800, KInk));
	}
	F.UI.Text(kast::copy::Number(Reached) + " of " + kast::copy::Number(Of) + " followers", R.X + R.W - 20.0f, R.Y + 64.0f, Ts(14.0f, 700, KMuted, Align::Right));
	const Rect Bar{R.X + 20.0f, R.Y + 76.0f, R.W - 40.0f, 12.0f};
	F.C.FillRoundRect(Bar, 6.0f, KPanel2);
	const Color Cols[3] = {KLime, KViolet, Hex(0x6b6385)};
	float Bx = Bar.X;
	float Lx = Bar.X;
	for (int I = 0; I < 3; ++I)
	{
		const double Share = static_cast<double>(Parts[static_cast<size_t>(I)]) / static_cast<double>(std::max(1, Of));
		const float W = Bar.W * Nf(std::max(0.0, std::min(1.0, Share)));
		if (W > 0.5f)
		{
			F.C.FillRect({Bx, Bar.Y, W, Bar.H}, Cols[I]);
		}
		Bx += W;
		const std::string Lab = kast::copy::PingPart(I) + " " + std::to_string(static_cast<int>(std::lround(Share * 100.0))) + "%";
		F.C.FillRoundRect({Lx, Bar.Y + 20.0f, 9.0f, 9.0f}, 2.0f, Cols[I]);
		Lx += 14.0f + F.UI.Text(Lab, Lx + 14.0f, Bar.Y + 29.0f, Ts(12.0f, 600, KInk)) + 22.0f;
	}
	F.UI.Text(KwPingHot, R.X + R.W - 20.0f, Bar.Y + 29.0f, Ts(11.5f, 600, KDim, Align::Right));
}

void rlkast_detail::KastCommunityPath(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	const std::vector<kast::StageSpec>& Stg = kast::Stages();
	const int Last = static_cast<int>(Stg.size()) - 1;
	const int Reached = Ch.Streams > 0 ? std::min(Ch.Stage, Last) : -1;
	KwBox(F, R, KwBuildingTitle, "Stage " + std::to_string(Reached + 1) + " of " + std::to_string(Last + 1) + (Reached >= 0 ? " \xC2\xB7 " + Stg[static_cast<size_t>(Reached)].Name : std::string()));
	// The ladder, compact: a dot a stage.
	const float L0 = R.X + 20.0f;
	const float Step = (R.W - 40.0f) / Nf(std::max(1, Last));
	const float Ly = R.Y + 48.0f;
	F.C.FillRoundRect({L0, Ly - 1.5f, Step * Nf(Last), 3.0f}, 1.5f, KPanel2);
	if (Reached > 0)
	{
		F.C.FillRoundRect({L0, Ly - 1.5f, Step * Nf(Reached), 3.0f}, 1.5f, KLime);
	}
	for (int K = 0; K <= Last; ++K)
	{
		const bool Got = K <= Reached;
		F.C.FillCircle(L0 + Step * Nf(K), Ly, Got ? 6.0f : 5.0f, Got ? KLime : K == Reached + 1 ? KViolet : KPanel2);
	}
	float Y = R.Y + 82.0f;
	double Minutes = 0.0;
	int Days = 0;
	double Avg = 0.0;
	Ch.Window30(F.World, Minutes, Days, Avg);
	struct Row
	{
		std::string Label;
		double Have;
		double Need;
		std::string Text;
	};
	std::vector<Row> Rows;
	if (!Ch.Affiliate)
	{
		F.UI.Text(KwNextAffiliate, R.X + 20.0f, Y, Ts(18.0f, 900, KInk));
		F.UI.Text(kast::copy::AffiliateBar(kast::AffiliateFollowersC8, static_cast<int>(kast::AffiliateMinutesC8), kast::AffiliateDaysC8, static_cast<int>(kast::AffiliateAvg)), R.X + 20.0f, Y + 20.0f,
			Ts(12.5f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
		Rows.push_back({"Followers", static_cast<double>(Ch.Followers), static_cast<double>(kast::AffiliateFollowersC8), kast::copy::Number(Ch.Followers) + " / " + kast::copy::Number(kast::AffiliateFollowersC8)});
		Rows.push_back({"Minutes live", Minutes, kast::AffiliateMinutesC8, Grouped(static_cast<int64_t>(std::floor(Minutes))) + " / " + std::to_string(static_cast<int>(kast::AffiliateMinutesC8))});
		Rows.push_back({"Days live", static_cast<double>(Days), static_cast<double>(kast::AffiliateDaysC8), std::to_string(Days) + " / " + std::to_string(kast::AffiliateDaysC8)});
		Rows.push_back({"Average viewers", Avg, kast::AffiliateAvg, Fixed(Avg, 1) + " / " + std::to_string(static_cast<int>(kast::AffiliateAvg))});
	}
	else if (!Ch.Partner)
	{
		F.UI.Text(KwNextPartner, R.X + 20.0f, Y, Ts(18.0f, 900, KInk));
		F.UI.Text(KwPartnerBar, R.X + 20.0f, Y + 20.0f, Ts(12.5f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
		Rows.push_back({"Hours live", Minutes / 60.0, kast::PartnerMinutes / 60.0, Fixed(Minutes / 60.0, 1) + " / " + std::to_string(static_cast<int>(kast::PartnerMinutes / 60.0)) + " h"});
		Rows.push_back({"Days live", static_cast<double>(Days), static_cast<double>(kast::PartnerDays), std::to_string(Days) + " / " + std::to_string(kast::PartnerDays)});
		// Partner's average is the channel's own viewers, raiders and the Broadcast left out (C8): the core's own goal row.
		double OwnAvg = Avg;
		for (size_t I = 0; I < Stg.size(); ++I)
		{
			if (Stg[I].Tier == 2 && Stg[I].Streams == 0)
			{
				for (const kast::Goal& Pg : kast::StageGoals(Ch, F.World, static_cast<int>(I)))
				{
					OwnAvg = Pg.Label.rfind("Average viewers", 0) == 0 ? Pg.Have : OwnAvg;
				}
				break;
			}
		}
		Rows.push_back({"Average viewers", OwnAvg, kast::PartnerAvg, Fixed(OwnAvg, 1) + " / " + std::to_string(static_cast<int>(kast::PartnerAvg))});
	}
	else
	{
		F.UI.Text("Partner", R.X + 20.0f, Y, Ts(18.0f, 900, KLime));
		NetParagraph(F.C, KwPartnerDone, R.X + 20.0f, Y + 22.0f, R.W - 40.0f, 13.0f, 500, KMuted, 19.0f, 3);
		return;
	}
	Y += 34.0f;
	const float Gap = Rows.size() > 3 ? 22.0f : 26.0f;
	for (const Row& Rw : Rows)
	{
		const bool Done = Rw.Have >= Rw.Need;
		const float P = Nf(std::min(1.0, Rw.Have / std::max(1e-9, Rw.Need)));
		F.UI.Text(Rw.Label, R.X + 20.0f, Y + 12.0f, Ts(13.0f, 700, KInk));
		F.C.FillRoundRect({R.X + 200.0f, Y + 3.0f, R.W - 340.0f, 10.0f}, 5.0f, KPanel2);
		F.C.FillRoundRect({R.X + 200.0f, Y + 3.0f, std::max(10.0f, (R.W - 340.0f) * P), 10.0f}, 5.0f, Done ? KLime : KViolet);
		F.UI.Text(Rw.Text, R.X + R.W - 20.0f, Y + 12.0f, Ts(13.0f, 800, Done ? KLime : KInk, Align::Right, Baseline::Alphabetic, true));
		Y += Gap;
	}
	if (Ch.Affiliate)
	{
		F.UI.Text(kast::copy::PartnerAverage(), R.X + 20.0f, Y + 10.0f, Ts(11.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
	}
}

void rlkast_detail::KastCommunityRaids(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	KwBox(F, R, KwRaidsTitle, KwRaidsRight);
	// Who owes a raid (the channels holding a token from a raid out, at most 3 each), biggest first.
	std::vector<std::pair<std::string, int>> Owe;
	for (const auto& G : Ch.Goodwill)
	{
		if (G.second > 0)
		{
			Owe.push_back({G.first, std::min(3, G.second)});
		}
	}
	std::stable_sort(Owe.begin(), Owe.end(), [](const std::pair<std::string, int>& A, const std::pair<std::string, int>& B) { return A.second > B.second; });
	F.UI.Text(Owe.empty() ? std::string(KwNobodyOwes) : kast::copy::RaidsOwed(static_cast<int>(Owe.size())), R.X + 20.0f, R.Y + 62.0f, Ts(20.0f, 900, KInk));
	float Cx = R.X + 20.0f;
	for (size_t I = 0; I < Owe.size() && I < 3; ++I)
	{
		const Rect B{Cx, R.Y + 76.0f, (R.W - 64.0f) / 3.0f, 48.0f};
		F.C.FillRoundRect(B, 12.0f, KPanel2);
		DrawAvatar(F.C, B.X + 24.0f, B.Y + 24.0f, 14.0f, AvatarFor(Owe[I].first));
		F.UI.Text(Owe[I].first, B.X + 46.0f, B.Y + 21.0f, Ts(13.5f, 800, KInk, Align::Left, Baseline::Alphabetic, false, B.W - 52.0f));
		F.UI.Text(kast::copy::RaidsOwedShort(Owe[I].second), B.X + 46.0f, B.Y + 38.0f, Ts(12.0f, 700, KViolet));
		Cx += B.W + 12.0f;
	}
	// The 7-day rule: the latest raid out inside its week.
	const int Today = net::DayOf(F.World);
	std::string Latest;
	int LatestDay = INT_MIN;
	for (const auto& D : Ch.RaidedDay)
	{
		if (D.second > LatestDay && D.second <= Today && Today - D.second < 7)
		{
			Latest = D.first;
			LatestDay = D.second;
		}
	}
	float Y = R.Y + (Owe.empty() ? 92.0f : 150.0f);
	if (!Latest.empty())
	{
		F.UI.Text(kast::copy::RaidedAgo(Latest, Today - LatestDay, net::WeekdayName(LatestDay + 7, true)), R.X + 20.0f, Y, Ts(13.5f, 700, KMuted, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
		Y += 22.0f;
	}
	F.UI.Text(kast::copy::TokenRule(), R.X + 20.0f, Y, Ts(12.0f, 600, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
}

void rlkast_detail::KastCommunityWeek(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	KwBox(F, R, KwWeekTitle, std::string());
	const float Sw = F.UI.Text(kast::copy::StreakRow(Ch.Streak), R.X + 20.0f, R.Y + 62.0f, Ts(20.0f, 900, KInk)) + 18.0f;
	for (int K = 0; K < 8; ++K)
	{
		F.C.FillRoundRect({R.X + 20.0f + Sw + Nf(K) * 22.0f, R.Y + 48.0f, 16.0f, 16.0f}, 4.0f, K < std::min(8, Ch.Streak) ? KLime : KPanel2);
	}
	F.UI.Text(KwStreakNote, R.X + 20.0f + Sw + 8.0f * 22.0f + 10.0f, R.Y + 62.0f, Ts(12.0f, 600, KDim));
	// The week's goals, four across.
	const std::vector<kast::Goal> G = kast::WeekGoals(Ch, F.World);
	const float Cw = (R.W - 40.0f - 30.0f) / 4.0f;
	for (size_t I = 0; I < G.size() && I < 4; ++I)
	{
		const Rect Cell{R.X + 20.0f + Nf(I) * (Cw + 10.0f), R.Y + 80.0f, Cw, 40.0f};
		const bool Met = G[I].Have >= G[I].Need;
		F.C.FillRoundRect(Cell, 9.0f, Met ? NetA(KLime, 0.1f) : KPanel2);
		const std::string Xp = "+" + std::to_string(G[I].Xp) + " XP";
		const float Xw = F.UI.Measure(Xp, 11.0f, 800);
		F.UI.Text(G[I].Label, Cell.X + 10.0f, Cell.Y + 17.0f, Ts(12.0f, 800, Met ? KLime : KInk, Align::Left, Baseline::Alphabetic, false, Cell.W - Xw - 26.0f));
		F.UI.Text(Xp, Cell.X + Cell.W - 10.0f, Cell.Y + 17.0f, Ts(11.0f, 800, KGold, Align::Right));
		const float P = Nf(std::min(1.0, G[I].Have / std::max(1e-9, G[I].Need)));
		F.C.FillRoundRect({Cell.X + 10.0f, Cell.Y + 26.0f, Cell.W - 20.0f, 5.0f}, 2.5f, Hex(0x1a1426));
		F.C.FillRoundRect({Cell.X + 10.0f, Cell.Y + 26.0f, std::max(5.0f, (Cell.W - 20.0f) * P), 5.0f}, 2.5f, Met ? KLime : KViolet);
	}
	// Stamina (K16), shown from 24 world hours live in 7 days.
	const double H7 = kast::growth::H7(Ch.LiveMin, Ch.LiveDay, net::DayOf(F.World));
	const float Sy = R.Y + 132.0f;
	if (H7 >= 24.0)
	{
		const double Stamina = kast::growth::Stamina(H7);
		const Color Amber = Hex(0xf59e0b);
		const Rect B{R.X + 16.0f, Sy, R.W - 32.0f, R.Y + R.H - 12.0f - Sy};
		F.C.FillRoundRect(B, 12.0f, NetA(Amber, 0.10f));
		F.C.StrokeRoundRect(B, 12.0f, NetA(Amber, 0.5f), 1.0f);
		NetSpaced(F.C, kast::copy::StaminaTitle(), B.X + 18.0f, B.Y + 22.0f, 10.0f, 900, Amber, 1.6f);
		F.UI.Text(kast::copy::StaminaLine(static_cast<int>(std::floor(H7))), B.X + 18.0f, B.Y + 48.0f, Ts(17.0f, 900, KInk, Align::Left, Baseline::Alphabetic, false, B.W - 36.0f));
		F.C.FillRoundRect({B.X + 18.0f, B.Y + 60.0f, 260.0f, 9.0f}, 4.5f, KPanel2);
		F.C.FillRoundRect({B.X + 18.0f, B.Y + 60.0f, std::max(9.0f, 260.0f * Nf(Stamina)), 9.0f}, 4.5f, Amber);
		F.UI.Text(KwStaminaCost, B.X + 292.0f, B.Y + 69.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, B.W - 310.0f));
		F.UI.Text(kast::copy::StaminaNote(), B.X + 18.0f, B.Y + 90.0f, Ts(11.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, B.W - 36.0f));
	}
	else
	{
		const int Lv = Ch.Level();
		const double Lo = kast::Channel::LevelXp(Lv);
		const double Hi = kast::Channel::LevelXp(std::min(kast::MaxLevel, Lv + 1));
		const float P = Lv >= kast::MaxLevel ? 1.0f : Nf(std::min(1.0, (Ch.Xp - Lo) / std::max(1.0, Hi - Lo)));
		F.UI.Text("Level " + std::to_string(Lv), R.X + 20.0f, Sy + 30.0f, Ts(18.0f, 900, KGold));
		F.C.FillRoundRect({R.X + 120.0f, Sy + 21.0f, 260.0f, 9.0f}, 4.5f, KPanel2);
		F.C.FillRoundRect({R.X + 120.0f, Sy + 21.0f, std::max(9.0f, 260.0f * P), 9.0f}, 4.5f, KGold);
		F.UI.Text(Lv >= kast::MaxLevel ? std::string("max level") : Grouped(static_cast<int64_t>(Ch.Xp - Lo)) + " / " + Grouped(static_cast<int64_t>(Hi - Lo)) + " XP", R.X + 392.0f, Sy + 30.0f,
			Ts(12.0f, 700, KMuted, Align::Left, Baseline::Alphabetic, true));
		const int More = static_cast<int>(std::round((Ch.Discoverability() - 1.0) * 100.0));
		F.UI.Text(KwShownMore(More), R.X + 20.0f,
			Sy + 54.0f, Ts(12.0f, 600, More > 0 ? KLav : KDim, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
	}
}

void rlkast_detail::KastCommunityPayouts(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	KwBox(F, R, KwPayoutsTitle, KwPayoutsRight);
	float Y = R.Y + 52.0f;
	// What waits for the second rent (C14).
	if (Ch.PendingCents > 0)
	{
		const Rect Row{R.X + 16.0f, Y, R.W - 32.0f, 52.0f};
		F.C.FillRoundRect(Row, 10.0f, KPanel2);
		F.C.FillRoundRect({Row.X, Row.Y, 4.0f, Row.H}, 2.0f, KGold);
		F.UI.Text(KwPendingTitle, Row.X + 18.0f, Row.Y + 22.0f, Ts(14.0f, 800, KInk));
		F.UI.Text(kast::copy::PendingNote(), Row.X + 18.0f, Row.Y + 40.0f, Ts(11.5f, 600, KMuted));
		F.UI.Text(Money(Ch.PendingCents), Row.X + Row.W - 18.0f, Row.Y + 32.0f, Ts(16.0f, 900, KGold, Align::Right, Baseline::Alphabetic, true));
		Y += 58.0f;
	}
	// The ledger's Kast payouts and sponsor pay, newest first, each on its own line (C13).
	int Shown = 0;
	for (const life::LedgerEntry& E : F.S.Life.Ledger)
	{
		if ((E.Kind != 6 && E.Kind != 11) || E.Amount <= 0)
		{
			continue;
		}
		if (Y + 52.0f > R.Y + R.H - 34.0f)
		{
			break;
		}
		const bool Kast = E.Kind == 6;
		const Color Col = Kast ? KLime : KSky;
		const Rect Row{R.X + 16.0f, Y, R.W - 32.0f, 52.0f};
		F.C.FillRoundRect(Row, 10.0f, KPanel2);
		F.C.FillRoundRect({Row.X, Row.Y, 4.0f, Row.H}, 2.0f, Col);
		F.UI.Text(E.Label, Row.X + 18.0f, Row.Y + 22.0f, Ts(14.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, Row.W - 140.0f));
		const int Day = net::DayOf(E.At);
		F.UI.Text(std::string(net::WeekdayName(Day)) + " " + net::DateLabel(Day) + " \xC2\xB7 " + (Kast ? KwKastPayoutSub : KwSponsorSub), Row.X + 18.0f, Row.Y + 40.0f,
			Ts(11.5f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, Row.W - 140.0f));
		F.UI.Text(Money(E.Amount), Row.X + Row.W - 18.0f, Row.Y + 32.0f, Ts(16.0f, 900, Col, Align::Right, Baseline::Alphabetic, true));
		Y += 58.0f;
		++Shown;
	}
	if (Shown == 0 && Ch.PendingCents <= 0)
	{
		NetParagraph(F.C, KwNoPayouts, R.X + 20.0f, Y + 20.0f, R.W - 40.0f, 13.5f, 500, KMuted, 20.0f, 3);
	}
	F.UI.Text(kast::copy::SponsorApart(), R.X + 20.0f, R.Y + R.H - 16.0f, Ts(12.0f, 600, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
}

// ------------------------------------------------------------------ Browse

float rlkast_detail::KastOwnCardChips(KastFrame& F, const Rect& Thumb, float X)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	const KwView& V = F.S.Stream.GetView();
	if (V.EventMode)
	{
		// The directory card's FINAL TABLE chip (K5), beside LIVE.
		const std::string Ft = kast::copy::DirectoryEvent(V.DeepEvent, V.DeepRemaining);
		const float Fw = std::min(F.UI.Measure(Ft, 10.5f, 900) + 22.0f, Thumb.W - 80.0f);
		const float Fx = Thumb.X + 18.0f + F.UI.Measure("LIVE", 10.5f, 800) + 18.0f;
		F.C.FillRoundRect({Fx, Thumb.Y + 10.0f, Fw, 20.0f}, 10.0f, Paint::Linear({Fx, 0.0f}, {Fx + Fw, 0.0f}, KGold, KGold2));
		F.UI.Text(Ft, Fx + Fw / 2.0f, Thumb.Y + 24.0f, Ts(10.5f, 900, KDeep, Align::Center, Baseline::Alphabetic, false, Fw - 12.0f));
	}
	// The lanes (K7): NEW ON KAST for streams 1-12, RISING IN POKER after stream 3 while the usual is under 15.
	float Cx = X;
	const std::pair<bool, std::pair<std::string, Color>> Lanes[2] = {{Ch.Streams >= 1 && Ch.Streams <= 12, {kast::copy::NewOnKastChip(), KLime}},
		{Ch.Streams > 3 && Ch.Norm < 15.0, {kast::copy::RisingChip(), KSky}}};
	for (const auto& Ln : Lanes)
	{
		if (!Ln.first)
		{
			continue;
		}
		const float W = F.UI.Measure(Ln.second.first, 9.5f, 800) + 18.0f;
		F.C.FillRoundRect({Cx, Thumb.Y + Thumb.H - 31.0f, W, 20.0f}, 10.0f, Ln.second.second);
		F.UI.Text(Ln.second.first, Cx + W / 2.0f, Thumb.Y + Thumb.H - 17.5f, Ts(9.5f, 800, KDeep, Align::Center));
		Cx += W + 6.0f;
	}
	return Cx - X;
}

namespace rlkast_detail
{
struct KwShelfRow
{
	std::string Name;
	std::string Line;
	Color LineCol;
	int Viewers = 0;
	uint32_t Rgb = 0;
	bool You = false;
};

void KwShelf(KastFrame& F, const Rect& R, const std::string& Title, const std::string& Sub, const std::vector<KwShelfRow>& Rows, const std::string& Foot, const Color& Accent, float RowsFrom)
{
	using namespace rlnet_detail;
	F.C.FillRoundRect(R, 16.0f, KPanel);
	F.UI.Text(Title, R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, KInk));
	F.UI.Text(Sub, R.X + 20.0f, R.Y + 56.0f, Ts(12.5f, 600, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
	float Y = R.Y + RowsFrom;
	for (const KwShelfRow& Rw : Rows)
	{
		if (Y + 56.0f > R.Y + R.H - 58.0f)
		{
			break;
		}
		if (Rw.You)
		{
			F.C.FillRoundRect({R.X + 10.0f, Y, R.W - 20.0f, 56.0f}, 10.0f, NetA(KLime, 0.08f));
			F.C.StrokeRoundRect({R.X + 10.0f, Y, R.W - 20.0f, 56.0f}, 10.0f, NetA(KLime, 0.7f), 1.2f);
		}
		AvatarSpec Av = AvatarFor(Rw.Name);
		DrawAvatar(F.C, R.X + 34.0f, Y + 28.0f, 16.0f, Av);
		F.UI.Text(Rw.Name + (Rw.You ? KwYou : ""), R.X + 60.0f, Y + 23.0f, Ts(14.0f, 800, Rw.You ? KLime : KInk, Align::Left, Baseline::Alphabetic, false, R.W - 150.0f));
		F.UI.Text(Rw.Line, R.X + 60.0f, Y + 42.0f, Ts(12.0f, 600, Rw.LineCol, Align::Left, Baseline::Alphabetic, false, R.W - 150.0f));
		F.UI.Text(kast::copy::Number(Rw.Viewers), R.X + R.W - 22.0f, Y + 23.0f, Ts(15.0f, 900, KInk, Align::Right, Baseline::Alphabetic, true));
		F.UI.Text("watching", R.X + R.W - 22.0f, Y + 41.0f, Ts(11.0f, 600, KDim, Align::Right));
		Y += 62.0f;
	}
	if (!Foot.empty())
	{
		F.C.FillRoundRect({R.X + 10.0f, R.Y + R.H - 54.0f, R.W - 20.0f, 44.0f}, 10.0f, NetA(Accent, 0.12f));
		F.UI.Text(Foot, R.X + 24.0f, R.Y + R.H - 26.0f, Ts(13.0f, 800, Accent, Align::Left, Baseline::Alphabetic, false, R.W - 48.0f));
	}
}
} // namespace rlkast_detail

void rlkast_detail::KastShelves(KastFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	const kast::Channel& Ch = F.S.Channel;
	const bool Live = F.S.Streaming();
	const KwView& V = F.S.Stream.GetView();
	const int N = KastShown(F.S, F.Now, F.Previewing);
	const float W3 = (R.W - 32.0f) / 3.0f;
	// Sweating now (K6): your stream at a deep run, and the Broadcast's tease in the site's voice.
	{
		std::vector<KwShelfRow> Rows;
		int Site = 0;
		if (Live && V.EventMode)
		{
			int Size = 0;
			KwRunFacts(F.S, F.World, Size, Site);
			Rows.push_back({F.S.HeroName, kast::copy::DirectoryEvent(V.DeepEvent, V.DeepRemaining), KGold, N, kast::Lime, true});
		}
		const Rect B{R.X, R.Y, W3, R.H};
		KwShelf(F, B, kast::copy::SweatingNowShelf(), kast::copy::SweatingNowSub(), Rows, kast::copy::BroadcastTease(Site), KGold, 72.0f);
		if (Rows.empty())
		{
			F.UI.Text(KwSweatingNobody, B.X + 20.0f, B.Y + 100.0f, Ts(13.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, B.W - 40.0f));
		}
	}
	// Fresh on Kast (K7): new channels, streams 1-12.
	{
		std::vector<KwShelfRow> Rows;
		const bool Fresh = Ch.Streams >= 1 && Ch.Streams <= 12;
		if (Live && Fresh)
		{
			Rows.push_back({F.S.HeroName, "stream " + std::to_string(Ch.Streams) + " \xC2\xB7 " + kast::copy::NewOnKastChip(), KLime, N, kast::Lime, true});
		}
		const Rect B{R.X + W3 + 16.0f, R.Y, W3, R.H};
		KwShelf(F, B, kast::copy::FreshShelf(), kast::copy::FreshShelfSub(), Rows, Live && Fresh ? kast::copy::EmptyFresh() : std::string(KwFreshNotYou), KLime, 72.0f);
		if (Rows.empty())
		{
			F.UI.Text(Live ? std::string(KwFreshNotYou) : std::string(KwGoLiveHere), B.X + 20.0f, B.Y + 100.0f, Ts(13.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, B.W - 40.0f));
		}
	}
	// Rising in Poker (K7): small channels, yours while the usual is under 15, and the network's small channels.
	{
		std::vector<KwShelfRow> Rows;
		const bool Rising = Ch.Streams > 3 && Ch.Norm < 15.0;
		if (Live && Rising)
		{
			Rows.push_back({F.S.HeroName, "usually " + std::to_string(static_cast<int>(std::lround(Ch.Norm))) + " watching \xC2\xB7 " + kast::copy::RisingChip(), KSky, N, kast::Lime, true});
		}
		for (const kast::SmallChannel& Sc : kast::Network(F.World))
		{
			if (Rows.size() >= 3)
			{
				break;
			}
			const auto It = Ch.Goodwill.find(Sc.Name);
			const std::string Line = Sc.Title + (It != Ch.Goodwill.end() ? " \xC2\xB7 raided " + std::to_string(It->second) + "x" : std::string());
			Rows.push_back({Sc.Name, Line, It != Ch.Goodwill.end() ? KViolet : KMuted, Sc.Viewers, Sc.Color, false});
		}
		const Rect B{R.X + 2.0f * (W3 + 16.0f), R.Y, W3, R.H};
		KwShelf(F, B, kast::copy::RisingShelf(), kast::copy::RisingShelfSub(), Rows, Live && Rising ? kast::copy::EmptyRising() : std::string(KwRaidThem), KSky, 72.0f);
	}
}

// ------------------------------------------------------------------ the Channel page

std::string rlkast_detail::KastClipLaneTag()
{
	// The Clip of the night line's own words after its last " · " ("... · in the Kast feed").
	const std::string Line = kast::copy::ClipOfTheNight(std::string(), true);
	const size_t At = Line.rfind("\xC2\xB7 ");
	return At == std::string::npos ? Line : Line.substr(At + 3);
}

// ------------------------------------------------------------------ the copy

std::vector<std::string> rlkast_detail::KastUiInventory()
{
	return {KwPingNone, KwHeatNoHistory, KwNobodyOwes, KwNoPayouts, KwPartnerPerks, KwPartnerDone, KwSweatingNobody, KwFreshNotYou, KwRisingNotYou, KwRaidThem, KwGoLiveHere,
		std::string(KwRailLabel) + ".", KwTrollsLine(1, true), KwTrollsLine(3, false), KwShownMore(12) + ".", std::string(KwShownSoon) + ".", std::string(KwBroadcastTitle) + "."};
}
} // namespace ui
} // namespace ss
