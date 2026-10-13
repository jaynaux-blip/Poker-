#include "ShortStack/Game/Autoplay.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Common.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

// Autoplay's bookkeeping (update 2 §2.5, §2.6.4, §2.9, §6.3-§6.5): Form and Sharpness, the plan's tilt, the stop rules,
// what each event's plan played, the notes on regulars, the live host text's auto line, the save lines and their
// migration, and the copy the panel, the strip, the skip card and the PLAY page draw. Everything here is a pure function
// of what it's given (nothing draws from a tournament's or the session's Rng), so the watch and skip paths, and a resumed
// night, see the same numbers. core-autoplay's file.

namespace ss
{
namespace autoplay
{
namespace autoplay_detail
{
constexpr double AutoMinutesPerWeek = 7.0 * 24.0 * 60.0;
constexpr size_t AutoKeepPlayed = 200;
constexpr size_t AutoKeepNotes = 300;
constexpr int AutoFormWeekQuota = 100; // graded decisions of the player's own a week that keep Form from rusting

/** Thousandths (tilt, Form) and hundredths (Sharpness): the saved and the held numbers are the same doubles. */
long long AutoMilli(double V)
{
	return std::llround(V * 1000.0);
}

double AutoFromMilli(long long Milli)
{
	return static_cast<double>(Milli) / 1000.0;
}

double AutoQuantizeMilli(double V)
{
	return AutoFromMilli(AutoMilli(V));
}

long long AutoCenti(double V)
{
	return std::llround(V * 100.0);
}

std::string AutoEscape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\')
		{
			Out += "\\\\";
		}
		else if (Ch == '\n')
		{
			Out += "\\n";
		}
		else if (Ch == '\t')
		{
			Out += "\\t";
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::string AutoUnescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

std::vector<std::string> AutoSplit(const std::string& Line)
{
	std::vector<std::string> Parts;
	std::string Cur;
	for (const char Ch : Line)
	{
		if (Ch == '\t')
		{
			Parts.push_back(Cur);
			Cur.clear();
		}
		else if (Ch != '\r' && Ch != '\n')
		{
			Cur.push_back(Ch);
		}
	}
	Parts.push_back(Cur);
	return Parts;
}

long long AutoInt(const std::string& S, long long Default = 0)
{
	if (S.empty())
	{
		return Default;
	}
	char* End = nullptr;
	const long long V = std::strtoll(S.c_str(), &End, 10);
	return End && End != S.c_str() ? V : Default;
}

bool AutoStarts(const std::string& S, const char* Head)
{
	return S.rfind(Head, 0) == 0;
}

/** A style from its saved id ("gto"), or from its number in older text; Autopilot when it's neither. */
style::Style AutoStyleFrom(const std::string& Text)
{
	style::Style S = style::Style::Autopilot;
	if (style::FromId(Text, S))
	{
		return S;
	}
	const long long N = AutoInt(Text, -1);
	return N >= 0 && N < static_cast<long long>(style::Style::Count) ? static_cast<style::Style>(N) : style::Style::Autopilot;
}

int AutoSpeedOf(long long Speed)
{
	return Speed >= 8 ? 8 : Speed >= 4 ? 4 : Speed >= 2 ? 2 : 1;
}

Mode AutoModeOf(long long M)
{
	return M == static_cast<long long>(Mode::Watch) ? Mode::Watch : M == static_cast<long long>(Mode::Skip) ? Mode::Skip : Mode::Off;
}

Target AutoTargetOf(long long T)
{
	return T >= 0 && T <= static_cast<long long>(Target::Closing) ? static_cast<Target>(T) : Target::Bubble;
}

constexpr uint32_t AutoStopMask = StopFinalTable | StopBubble | StopMyAllIn | StopBigPot | StopPremium;

/** Form after the weekly decay up to World (§2.5): a week with under 100 graded decisions of the player's own, and every
 *  idle week after it, moves Form a point toward 70. Applied to P; the result. */
double AutoDecay(Prefs& P, double World)
{
	const int Week = WeekOf(World);
	if (P.FormWeek < 0)
	{
		P.FormWeek = Week;
		P.WeekDecisions = 0;
		return P.Form;
	}
	if (Week > P.FormWeek)
	{
		const double Steps = static_cast<double>((P.WeekDecisions < AutoFormWeekQuota ? 1 : 0) + (Week - P.FormWeek - 1));
		if (P.Form > FormStart)
		{
			P.Form = std::max(FormStart, P.Form - Steps);
		}
		else if (P.Form < FormStart)
		{
			P.Form = std::min(FormStart, P.Form + Steps);
		}
		P.Form = AutoQuantizeMilli(P.Form);
		P.FormWeek = Week;
		P.WeekDecisions = 0;
	}
	return P.Form;
}

std::string AutoPercent(double Fraction)
{
	return std::to_string(static_cast<int>(std::lround(Fraction * 100.0))) + "%";
}

std::string AutoUpper(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return S;
}

/** The causes of a Sharpness under its best, as words: "on tilt", "tired", "running on empty", joined. */
std::string AutoConditions(double Tilt, bool Exhausted, bool Needy)
{
	std::vector<std::string> Words;
	if (Tilt > 0.2)
	{
		Words.push_back("on tilt");
	}
	if (Exhausted)
	{
		Words.push_back("tired");
	}
	if (Needy)
	{
		Words.push_back("running on empty");
	}
	std::string Out;
	for (size_t I = 0; I < Words.size(); ++I)
	{
		Out += (I == 0 ? "" : I + 1 == Words.size() ? " and " : ", ") + Words[I];
	}
	return Out;
}

/** How far the skip's target is (the tile and the skip card): "31 to the bubble", "12 to the final table", "88 left". */
std::string AutoProgress(const TableAuto& A, const Tournament& T)
{
	switch (A.To)
	{
	case Target::Bubble:
		return T.InTheMoney() ? std::string("in the money") : Grouped(std::max(0, T.Remaining - T.PaidPlaces())) + " to the bubble";
	case Target::FinalTable:
		return T.Tables.size() <= 1 ? std::string("at the final table") : Grouped(std::max(0, T.Remaining - T.TableSize)) + " to the final table";
	case Target::NextBreak: return T.Spec.BreakEvery > 0 ? std::string("to the break") : std::string("to the next level");
	case Target::Hands: return Grouped(std::max(0, A.Hands)) + (A.Hands == 1 ? " hand to go" : " hands to go");
	default: return Grouped(T.Remaining) + " left";
	}
}
} // namespace autoplay_detail

double SkillTerm(double Form)
{
	// 0.80 up to a Form of 80, 1.00 at 95 (§2.5).
	return 0.80 + 0.20 * std::min(1.0, std::max(0.0, (Form - 80.0) / 15.0));
}

double SharpnessOf(double Form, double Tilt, bool Exhausted, bool Needy)
{
	// SkillTerm with the player's condition (§2.5), 0.50 to 1.00, to the hundredth.
	double S = SkillTerm(Form) * (1.0 - 0.3 * std::max(0.0, Tilt - 0.2));
	S *= Exhausted ? 0.85 : 1.0;
	S *= Needy ? 0.92 : 1.0;
	return std::round(std::min(1.0, std::max(0.5, S)) * 100.0) / 100.0;
}

int WeekOf(double World)
{
	return static_cast<int>(std::floor(World / autoplay_detail::AutoMinutesPerWeek));
}

double FormAt(Book& B, double World)
{
	return autoplay_detail::AutoDecay(B.Pref, World);
}

double FormNow(const Book& B, double World, bool* Rusty)
{
	Prefs P = B.Pref;
	const double Before = P.Form;
	const double Now = autoplay_detail::AutoDecay(P, World);
	if (Rusty)
	{
		*Rusty = Now < Before;
	}
	return Now;
}

style::HeroBrain BrainFor(const TableAuto& A, const Run& R, double TiltGain, int Where, const reads::Table* Notes, const style::Learned* Book)
{
	style::HeroBrain B;
	B.S = A.S;
	B.Sharp = R.Sharp;
	// Autopilot is today's Sprint exactly: it never tilts (§2.4.1, §2.5).
	B.Tilt = A.S == style::Style::Autopilot ? 0.0 : R.Tilt;
	B.TiltGain = TiltGain;
	B.Seen = &R.Seen;
	B.Notes = Notes;
	B.Book = Book;
	B.Where = Where;
	return B;
}

void AfterHand(Run& R, const HandOutcome& O, double TiltGain)
{
	// The session's rules for the plan's own tilt (§2.5): a bad beat +0.45, a lost pot over 20 BB +0.20 (both by the
	// background's TiltGain), a won pot -0.15, then x0.93 a hand, to the thousandth.
	double Tilt = R.Tilt;
	if (O.HeroIn)
	{
		if (!O.Won && O.BadBeat)
		{
			Tilt += 0.45 * TiltGain;
		}
		else if (!O.Won && O.Pot > O.BigBlind * 20)
		{
			Tilt += 0.20 * TiltGain;
		}
		else if (O.Won)
		{
			Tilt = std::max(0.0, Tilt - 0.15);
		}
		Tilt = std::min(1.0, Tilt);
	}
	R.Tilt = autoplay_detail::AutoQuantizeMilli(std::max(0.0, Tilt * 0.93));
	if (O.ByPlan)
	{
		++R.AiHands;
		R.AiDecisions += O.Decisions;
	}
	else
	{
		++R.OwnHands;
		R.OwnDecisions += O.Decisions;
	}
}

void NoteOwnDecision(Book& B, life::State& L, int AccuracyPoints, double World, bool TakenMidThink)
{
	// Each of the player's own graded decisions moves Form 1/400 of the way to its accuracy points (§2.5), unless it was
	// made in a hand taken over after the plan's think began (it could copy the plan). Every one counts toward the week.
	FormAt(B, World);
	++B.Pref.WeekDecisions;
	if (!TakenMidThink)
	{
		const double Points = std::min(100.0, std::max(0.0, static_cast<double>(AccuracyPoints)));
		B.Pref.Form = autoplay_detail::AutoQuantizeMilli(std::min(100.0, std::max(0.0, B.Pref.Form + (Points - B.Pref.Form) / 400.0)));
	}
	L.Update2LinesRead |= life::U2AutoPref;
}

bool ShouldStop(const TableAuto& A, const Hand& H, const BotDecision& Planned, int HeroSeat, const Tournament* T)
{
	// The decision-level stop rules (§2.6.4), against the move the plan drew: true hands this decision to the player. The
	// final-table and bubble rules are the skip's, between hands.
	(void)T;
	if ((A.Stops & (StopMyAllIn | StopBigPot | StopPremium)) == 0 || HeroSeat < 0 || HeroSeat >= static_cast<int>(H.Seats.size()))
	{
		return false;
	}
	const HandSeat& Me = H.Seats[static_cast<size_t>(HeroSeat)];
	if ((A.Stops & StopPremium) && Me.Hole.size() == 2)
	{
		bool First = true;
		for (const ActionRecord& Rec : H.History)
		{
			First = First && Rec.Seat != Me.Seat;
		}
		const int Hi = std::max(RankOf(Me.Hole[0]), RankOf(Me.Hole[1]));
		const int Lo = std::min(RankOf(Me.Hole[0]), RankOf(Me.Hole[1]));
		const bool Premium = (Hi == Lo && Hi >= 10) || (Hi == 12 && Lo == 11); // QQ+, AK
		if (First && Premium)
		{
			return true;
		}
	}
	if ((A.Stops & StopBigPot) && H.Pot() > H.BigBlind * 40)
	{
		return true;
	}
	if (A.Stops & StopMyAllIn)
	{
		const Chips ToCall = std::max<Chips>(0, std::min(H.CurrentBet - Me.StreetBet, Me.Stack));
		if (Planned.Action.Type == PlayerAction::Kind::Raise && Planned.Action.To >= static_cast<double>(Me.StreetBet + Me.Stack))
		{
			return true;
		}
		if (Planned.Action.Type == PlayerAction::Kind::Call && ToCall > 0 && ToCall * 2 >= Me.Stack)
		{
			return true; // all in, or a call of half the stack or more
		}
	}
	return false;
}

double AllInEquity(const Hand& H, int HeroSeat)
{
	if (HeroSeat < 0 || HeroSeat >= static_cast<int>(H.Seats.size()) || H.Seats[static_cast<size_t>(HeroSeat)].Folded)
	{
		return -1.0;
	}
	// The cards went on their backs (the Reveal) with the player still in: the board as it was then.
	size_t Reveal = H.Events.size();
	for (size_t I = 0; I < H.Events.size(); ++I)
	{
		if (H.Events[I].Type == EventType::Reveal)
		{
			Reveal = I;
			break;
		}
	}
	if (Reveal == H.Events.size())
	{
		return -1.0;
	}
	std::vector<Card> Board;
	for (size_t I = 0; I < Reveal; ++I)
	{
		if (H.Events[I].Type == EventType::Street)
		{
			Board = H.Events[I].Board;
		}
	}
	std::vector<std::vector<Card>> Holes;
	int Mine = -1;
	std::string Seed = "autoplay-allin";
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		const HandSeat& S = H.Seats[I];
		if (S.Folded || S.Hole.size() != 2)
		{
			continue;
		}
		if (static_cast<int>(I) == HeroSeat)
		{
			Mine = static_cast<int>(Holes.size());
		}
		Holes.push_back(S.Hole);
		Seed += ":" + std::to_string(S.Seat) + "-" + std::to_string(S.Hole[0]) + "-" + std::to_string(S.Hole[1]);
	}
	if (Mine < 0 || Holes.size() < 2)
	{
		return -1.0;
	}
	// A seed from the hand itself (who held what, where the button was): the same number on every path.
	Seed += ":b" + std::to_string(H.ButtonSeat);
	Rng R(Seed);
	const std::vector<double> Eq = EquityKnownHands(Holes, Board, R, 2000);
	return Mine < static_cast<int>(Eq.size()) ? Eq[static_cast<size_t>(Mine)] : -1.0;
}

HandOutcome OutcomeOf(const Hand& H, int HeroSeat, bool ByPlan)
{
	HandOutcome O;
	O.ByPlan = ByPlan;
	O.BigBlind = H.BigBlind;
	for (const PotResult& P : H.PotResults)
	{
		O.Pot += P.Amount;
	}
	if (HeroSeat < 0 || HeroSeat >= static_cast<int>(H.Seats.size()))
	{
		return O;
	}
	const HandSeat& Me = H.Seats[static_cast<size_t>(HeroSeat)];
	O.HeroNet = Me.Stack - Me.StartStack;
	O.HeroIn = !Me.Folded;
	for (const PotResult& P : H.PotResults)
	{
		O.Won = O.Won || std::find(P.Winners.begin(), P.Winners.end(), Me.Seat) != P.Winners.end();
	}
	const double Eq = O.HeroIn ? AllInEquity(H, HeroSeat) : -1.0;
	O.AllIn = Eq >= 0.0;
	O.BadBeat = O.AllIn && !O.Won && Eq >= 0.6;
	return O;
}

HandOutcome NoteHand(Run& R, const Hand& H, int HeroSeat, bool ByPlan, double TiltGain)
{
	const HandOutcome O = OutcomeOf(H, HeroSeat, ByPlan);
	AfterHand(R, O, TiltGain);
	if (HeroSeat >= 0 && HeroSeat < static_cast<int>(H.Seats.size()))
	{
		reads::Count(H, HeroSeat, R.Seen);
	}
	return O;
}

HandOutcome NoteSkippedHand(Run& R, const Hand& H, int HeroSeat, double TiltGain)
{
	// The live skip's hand: every action of the player's seat was the plan's (nothing is handed over mid-hand there).
	HandOutcome O = OutcomeOf(H, HeroSeat, true);
	const bool Seated = HeroSeat >= 0 && HeroSeat < static_cast<int>(H.Seats.size());
	if (Seated)
	{
		const int Seat = H.Seats[static_cast<size_t>(HeroSeat)].Seat;
		for (const ActionRecord& Rec : H.History)
		{
			O.Decisions += Rec.Seat == Seat ? 1 : 0;
		}
	}
	AfterHand(R, O, TiltGain);
	if (Seated)
	{
		reads::Count(H, HeroSeat, R.Seen);
	}
	return O;
}

std::string NoteKey(const Tournament* T, const std::string& PlayerId)
{
	if (PlayerId.empty() || PlayerId == HeroId)
	{
		return std::string();
	}
	if (autoplay_detail::AutoStarts(PlayerId, "npc:"))
	{
		return PlayerId;
	}
	if (T)
	{
		const int I = T->PlayerIndex(PlayerId);
		return I >= 0 && !T->Players[static_cast<size_t>(I)].IsHero ? "npc:" + T->Players[static_cast<size_t>(I)].Name : std::string();
	}
	return "cast:" + PlayerId;
}

const reads::Stats* NoteFor(const reads::Table* Notes, const Tournament* T, const std::string& PlayerId)
{
	if (!Notes)
	{
		return nullptr;
	}
	const std::string Key = NoteKey(T, PlayerId);
	const auto It = Key.empty() ? Notes->end() : Notes->find(Key);
	return It == Notes->end() ? nullptr : &It->second;
}

TableAuto DefaultPlan(const Book& B)
{
	TableAuto A;
	A.S = B.Pref.S;
	A.M = Mode::Off;
	A.Speed = autoplay_detail::AutoSpeedOf(B.Pref.Speed);
	A.To = B.Pref.To;
	A.Hands = 0;
	A.Stops = B.Pref.Stops & autoplay_detail::AutoStopMask;
	return A;
}

void SetDefault(Book& B, life::State& L, const TableAuto& A)
{
	B.Pref.S = A.S;
	B.Pref.Speed = autoplay_detail::AutoSpeedOf(A.Speed);
	B.Pref.To = A.To;
	B.Pref.Stops = A.Stops & autoplay_detail::AutoStopMask;
	L.Update2LinesRead |= life::U2AutoPref;
}

std::string CantPick(style::Style S, int Where, bool Knockout)
{
	return style::InfoOf(S).Hidden ? std::string("Not on the menu") : style::Unavailable(S, Where, Knockout);
}

style::Style PlaysAs(style::Style S, int Where)
{
	const style::Info& I = style::InfoOf(S);
	return Where == 2 && I.TourneyOnly ? I.AtCash : S;
}

void Record(Book& B, const std::string& Id, int Venue, int Site, const TableAuto& A, const Run& R)
{
	// The event's played line (§2.9, §6.4): newest first, one per event, the last 200. The style is the plan's when the
	// event ended (Run keeps no count by style: it froze with Life.h).
	if (Id.empty())
	{
		return;
	}
	Played P;
	P.Id = Id;
	P.Venue = Venue;
	P.Site = Site;
	P.S = A.S;
	P.AiDecisions = R.AiDecisions;
	P.OwnDecisions = R.OwnDecisions;
	P.AiHands = R.AiHands;
	P.OwnHands = R.OwnHands;
	B.Results.erase(std::remove_if(B.Results.begin(), B.Results.end(), [&](const Played& Q) { return Q.Id == Id; }), B.Results.end());
	B.Results.insert(B.Results.begin(), P);
	if (B.Results.size() > autoplay_detail::AutoKeepPlayed)
	{
		B.Results.resize(autoplay_detail::AutoKeepPlayed);
	}
}

void FoldNotes(Book& B, const reads::Table& Event, double World)
{
	// Only the world's people and the cast (keys "npc:" and "cast:"; NoteKey makes them), at half weight; at most 300
	// rows, the longest unseen dropped first (§2.4.3).
	reads::Table Named;
	for (const auto& It : Event)
	{
		if ((autoplay_detail::AutoStarts(It.first, "npc:") || autoplay_detail::AutoStarts(It.first, "cast:")) && It.second.Hands > 0)
		{
			Named[It.first] = It.second;
		}
	}
	if (Named.empty())
	{
		return;
	}
	reads::Fold(B.Notes, Named, 0.5);
	const double Seen = std::floor(World);
	for (const auto& It : Named)
	{
		if (B.Notes.count(It.first))
		{
			B.NoteSeen[It.first] = Seen;
		}
	}
	while (B.Notes.size() > autoplay_detail::AutoKeepNotes)
	{
		auto Oldest = B.Notes.begin();
		double OldestAt = std::numeric_limits<double>::infinity();
		for (auto It = B.Notes.begin(); It != B.Notes.end(); ++It)
		{
			const auto At = B.NoteSeen.find(It->first);
			const double When = At == B.NoteSeen.end() ? -std::numeric_limits<double>::infinity() : At->second;
			if (When < OldestAt)
			{
				OldestAt = When;
				Oldest = It;
			}
		}
		B.NoteSeen.erase(Oldest->first);
		B.Notes.erase(Oldest);
	}
	for (auto It = B.NoteSeen.begin(); It != B.NoteSeen.end();)
	{
		It = B.Notes.count(It->first) ? std::next(It) : B.NoteSeen.erase(It);
	}
}

const Played* PlayedFor(const Book& B, const std::string& Id)
{
	for (const Played& P : B.Results)
	{
		if (P.Id == Id)
		{
			return &P;
		}
	}
	return nullptr;
}

std::string RunText(const TableAuto& A, const Run& R)
{
	// auto\t<style>\t<mode>\t<speed>\t<tiltMilli>\t<sharpCenti>\t<target>\t<stops>\t<aiDec>\t<ownDec>\t<aiHands>\t<ownHands> (§2.12)
	std::string Out = "auto\t";
	Out += style::InfoOf(A.S).Id;
	Out += "\t" + std::to_string(static_cast<int>(A.M));
	Out += "\t" + std::to_string(autoplay_detail::AutoSpeedOf(A.Speed));
	Out += "\t" + std::to_string(autoplay_detail::AutoMilli(R.Tilt));
	Out += "\t" + std::to_string(autoplay_detail::AutoCenti(R.Sharp));
	Out += "\t" + std::to_string(static_cast<int>(A.To));
	Out += "\t" + std::to_string(A.Stops & autoplay_detail::AutoStopMask);
	Out += "\t" + std::to_string(R.AiDecisions);
	Out += "\t" + std::to_string(R.OwnDecisions);
	Out += "\t" + std::to_string(R.AiHands);
	Out += "\t" + std::to_string(R.OwnHands);
	return Out;
}

bool ReadRun(const std::string& Text, TableAuto& A, Run& R)
{
	std::vector<std::string> F = autoplay_detail::AutoSplit(Text);
	if (!F.empty() && F[0] == "auto")
	{
		F.erase(F.begin());
	}
	if (F.size() < 11)
	{
		return false;
	}
	using namespace autoplay_detail;
	A.S = AutoStyleFrom(F[0]);
	A.M = AutoModeOf(AutoInt(F[1]));
	A.Speed = AutoSpeedOf(AutoInt(F[2], 1));
	R.Tilt = std::min(1.0, std::max(0.0, AutoFromMilli(AutoInt(F[3]))));
	R.Sharp = std::min(1.0, std::max(0.5, static_cast<double>(AutoInt(F[4], 80)) / 100.0));
	A.To = AutoTargetOf(AutoInt(F[5], static_cast<long long>(Target::Bubble)));
	A.Stops = static_cast<uint32_t>(AutoInt(F[6])) & AutoStopMask;
	R.AiDecisions = static_cast<int>(AutoInt(F[7]));
	R.OwnDecisions = static_cast<int>(AutoInt(F[8]));
	R.AiHands = static_cast<int>(AutoInt(F[9]));
	R.OwnHands = static_cast<int>(AutoInt(F[10]));
	return true;
}

const char* TargetName(Target To)
{
	switch (To)
	{
	case Target::NextBreak: return "Next break";
	case Target::Bubble: return "Bubble";
	case Target::FinalTable: return "Final table";
	case Target::Result: return "Result";
	case Target::Hands: return "50 hands";
	case Target::Closing: return "Closing";
	}
	return "Result";
}

const char* StopName(uint32_t Bit)
{
	switch (Bit)
	{
	case StopFinalTable: return "Final table";
	case StopBubble: return "Bubble";
	case StopMyAllIn: return "My all-in decisions";
	case StopBigPot: return "Pots over 40 BB";
	case StopPremium: return "Premium hands (QQ+, AK)";
	default: return "";
	}
}

std::string SpeedName(int Speed)
{
	return std::to_string(autoplay_detail::AutoSpeedOf(Speed)) + "x";
}

std::string ThinkingLine(style::Style S)
{
	return std::string(style::InfoOf(S).Name) + " is thinking\xE2\x80\xA6";
}

std::string ActionLine(const ActionRecord& A)
{
	if (A.AllIn && A.Type != ActionType::Fold && A.Type != ActionType::Check)
	{
		return "ALL IN " + Grouped(A.To);
	}
	switch (A.Type)
	{
	case ActionType::Fold: return "FOLDED";
	case ActionType::Check: return "CHECKED";
	case ActionType::Call: return "CALLED " + Grouped(A.Added);
	case ActionType::Bet: return "BET " + Grouped(A.To);
	case ActionType::Raise: return "RAISED TO " + Grouped(A.To);
	}
	return std::string();
}

std::string StatusLine(const TableAuto& A, double Sharp)
{
	const std::string Name = style::InfoOf(A.S).Name;
	if (A.M == Mode::Watch)
	{
		return "AUTOPLAY \xC2\xB7 " + Name + " \xC2\xB7 Sharpness " + autoplay_detail::AutoPercent(Sharp) + " \xC2\xB7 [K] take over";
	}
	if (A.M == Mode::Skip)
	{
		return "AUTOPLAY \xC2\xB7 " + Name + " \xC2\xB7 skipping \xC2\xB7 [K] take the wheel";
	}
	return std::string();
}

std::string TileStatus(const TableAuto& A, const Tournament* T)
{
	if (A.M == Mode::Watch)
	{
		return "AUTO \xC2\xB7 " + std::string(style::InfoOf(A.S).Name) + " " + SpeedName(A.Speed);
	}
	if (A.M == Mode::Skip)
	{
		return "SKIP \xC2\xB7 " + (T ? autoplay_detail::AutoProgress(A, *T) : std::string(TargetName(A.To)));
	}
	return std::string();
}

std::string SkipLine(const TableAuto& A, const Tournament& T)
{
	return "SKIPPING \xC2\xB7 " + autoplay_detail::AutoUpper(style::InfoOf(A.S).Name) + " \xC2\xB7 hand " + Grouped(T.Tick + 1) + " \xC2\xB7 level " +
		std::to_string(T.LevelIndex + 1) + " \xC2\xB7 " + autoplay_detail::AutoProgress(A, T);
}

std::string PlayedLine(const Played& P)
{
	const int All = P.AiDecisions + P.OwnDecisions;
	const std::string Name = style::InfoOf(P.S).Name;
	if (All <= 0)
	{
		return "Played by your plan (" + Name + ")";
	}
	return "Played by your plan: " + Grouped(P.AiDecisions) + " of " + Grouped(All) + " decisions (" + Name + ")";
}

std::string CareerTag(const Played& P)
{
	const int All = P.AiDecisions + P.OwnDecisions;
	const std::string Head = "AUTO \xC2\xB7 " + std::string(style::InfoOf(P.S).Name);
	return All <= 0 ? Head : Head + " " + autoplay_detail::AutoPercent(static_cast<double>(P.AiDecisions) / static_cast<double>(All));
}

std::string SharpWhy(double Form, bool Rusty, double Tilt, bool Exhausted, bool Needy)
{
	std::string Out = "Form " + std::to_string(static_cast<int>(std::lround(Form)));
	const std::string Conditions = autoplay_detail::AutoConditions(Tilt, Exhausted, Needy);
	if (Rusty)
	{
		Out += ", rusty: play some hands";
	}
	if (!Conditions.empty())
	{
		Out += (Rusty ? "; " : ", ") + Conditions;
	}
	else if (!Rusty)
	{
		Out += ", rested";
	}
	return Out;
}

std::string SharpLine(double Sharp, const std::string& Why)
{
	return "Sharpness " + autoplay_detail::AutoPercent(Sharp) + ": " + Why;
}

std::string SharpWarning(double Sharp, double Form, double Tilt, bool Exhausted, bool Needy)
{
	if (Sharp >= 0.70)
	{
		return std::string();
	}
	const std::string Conditions = autoplay_detail::AutoConditions(Tilt, Exhausted, Needy);
	const std::string FormText = std::to_string(static_cast<int>(std::lround(Form)));
	std::string Out = "Sharpness " + autoplay_detail::AutoPercent(Sharp) + ": ";
	if (Conditions.empty())
	{
		Out += "your Form's only " + FormText;
	}
	else
	{
		Out += Conditions + (Form < 80.0 ? ", and your Form's still " + FormText : std::string());
	}
	return Out + ". Your plan gets sloppy.";
}

const char* Text(Words W)
{
	switch (W)
	{
	case Words::PanelTitle: return "AI AUTOPLAY";
	case Words::TakeOverKey: return "Take over [K]";
	case Words::Resumes: return "AUTO RESUMES \xC2\xB7 [K] keep playing";
	case Words::SkipKeys: return "[K] Take the wheel \xC2\xB7 [Backspace] Stop";
	case Words::MakeDefault: return "Make this my default";
	case Words::WantToSee: return "You'll want to see this.";
	case Words::DeeFirstNight: return "Dee: \xE2\x80\x9C" "First night, you play your own cards.\xE2\x80\x9D";
	case Words::Paused: return "Your plan's paused while you're up.";
	case Words::StandUpAway: return "Standing up pauses your plan.";
	case Words::AtBreak: return "The break.";
	case Words::AtLevel: return "A new level.";
	case Words::AtBubble: return "The bubble.";
	case Words::AtHandForHand: return "Hand for hand.";
	case Words::AtFinalTable: return "The final table. You'll want to see this.";
	case Words::AtHands: return "That's the hands you asked for.";
	case Words::HandedOver: return "A stop rule: this decision's yours.";
	case Words::StoppedByYou: return "Stopped. Your plan has the seat at 1x.";
	case Words::NoTable: return "There's no table there.";
	case Words::TableDone: return "That table's finished.";
	case Words::ClosingIsCash: return "Closing time is for a cash game.";
	case Words::AlreadyInMoney: return "You're already in the money.";
	case Words::AlreadyFinal: return "You're at the final table already.";
	case Words::NotOn: return "Autoplay isn't on at this table.";
	case Words::BustedSkipping: return "Busted while skipping";
	case Words::Count: break;
	}
	return "";
}

std::string BackOnLine(style::Style S)
{
	return "Your plan's back on: " + std::string(style::InfoOf(S).Name) + ".";
}

std::string WasPlayingLine(style::Style S)
{
	return "Your plan was playing: " + std::string(style::InfoOf(S).Name) + ". [K] carry on.";
}

std::string WatchToast(int Speed)
{
	return "WATCH " + SpeedName(Speed);
}

std::vector<std::string> Inventory()
{
	// Every string autoplay's screens draw (system UI: "AI autoplay" may appear here and nowhere in the world, U1).
	std::vector<std::string> Out;
	for (int W = 0; W < static_cast<int>(Words::Count); ++W)
	{
		Out.push_back(Text(static_cast<Words>(W)));
	}
	for (int T = 0; T <= static_cast<int>(Target::Closing); ++T)
	{
		Out.push_back(TargetName(static_cast<Target>(T)));
	}
	for (const uint32_t Bit : {StopFinalTable, StopBubble, StopMyAllIn, StopBigPot, StopPremium})
	{
		Out.push_back(StopName(Bit));
	}
	for (const int Speed : {1, 2, 4, 8})
	{
		Out.push_back(WatchToast(Speed));
	}
	for (const style::Info& I : style::Catalog())
	{
		// A hidden style's line (the other greyed reasons are style::Inventory()'s).
		if (I.Hidden)
		{
			Out.push_back(CantPick(I.S, 0, true));
			break;
		}
	}
	Played Sample;
	Sample.S = style::Style::Lag;
	Sample.AiDecisions = 214;
	Sample.OwnDecisions = 46;
	Out.push_back(PlayedLine(Sample));
	Out.push_back(CareerTag(Sample));
	Sample.AiDecisions = 0;
	Sample.OwnDecisions = 0;
	Out.push_back(PlayedLine(Sample));
	for (const style::Info& I : style::Catalog())
	{
		Out.push_back(ThinkingLine(I.S));
		Out.push_back(BackOnLine(I.S));
		Out.push_back(WasPlayingLine(I.S));
		TableAuto A;
		A.S = I.S;
		A.M = Mode::Watch;
		A.Speed = 4;
		Out.push_back(StatusLine(A, 0.93));
		Out.push_back(TileStatus(A, nullptr));
		A.M = Mode::Skip;
		Out.push_back(StatusLine(A, 0.93));
		for (int T = 0; T <= static_cast<int>(Target::Closing); ++T)
		{
			A.To = static_cast<Target>(T);
			Out.push_back(TileStatus(A, nullptr));
		}
	}
	for (const bool Rusty : {false, true})
	{
		for (int C = 0; C < 8; ++C)
		{
			const std::string Why = SharpWhy(84.0, Rusty, (C & 1) ? 0.6 : 0.0, (C & 2) != 0, (C & 4) != 0);
			Out.push_back(Why);
			Out.push_back(SharpLine(0.92, Why));
			const std::string Warn = SharpWarning(0.63, 70.0, (C & 1) ? 0.6 : 0.0, (C & 2) != 0, (C & 4) != 0);
			if (!Warn.empty())
			{
				Out.push_back(Warn);
			}
		}
	}
	ActionRecord Act;
	Act.To = 9000;
	Act.Added = 3200;
	for (const ActionType T : {ActionType::Fold, ActionType::Check, ActionType::Call, ActionType::Bet, ActionType::Raise})
	{
		Act.Type = T;
		Out.push_back(ActionLine(Act));
	}
	Act.AllIn = true;
	Out.push_back(ActionLine(Act));
	return Out;
}

void WriteLines(const life::State& L, std::string& Out)
{
	// Each line only once there's something to keep (§6.4): the pref line once U2AutoPref is set, a played line per
	// event the plan played, a note per regular; an old save nobody touched writes back as it was read.
	using namespace autoplay_detail;
	const Book& B = L.Auto;
	if (L.Update2LinesRead & life::U2AutoPref)
	{
		const Prefs& P = B.Pref;
		Out += std::string("autoplay\tpref\t") + style::InfoOf(P.S).Id + "\t" + std::to_string(AutoSpeedOf(P.Speed)) + "\t" + std::to_string(static_cast<int>(P.To)) + "\t" +
			std::to_string(P.Stops & AutoStopMask) + "\t" + std::to_string(AutoMilli(P.Form)) + "\t" + std::to_string(P.FormWeek) + "\t" + std::to_string(P.WeekDecisions) + "\n";
	}
	for (const Played& P : B.Results)
	{
		Out += "autoplay\tplayed\t" + AutoEscape(P.Id) + "\t" + std::to_string(P.Venue) + "\t" + std::to_string(P.Site) + "\t" + style::InfoOf(P.S).Id + "\t" +
			std::to_string(P.AiDecisions) + "\t" + std::to_string(P.OwnDecisions) + "\t" + std::to_string(P.AiHands) + "\t" + std::to_string(P.OwnHands) + "\n";
	}
	for (const auto& It : B.Notes)
	{
		const auto Seen = B.NoteSeen.find(It.first);
		const long long At = Seen == B.NoteSeen.end() ? 0 : static_cast<long long>(std::floor(Seen->second));
		Out += "autoplay\tnote\t" + AutoEscape(It.first) + "\t" + std::to_string(At) + "\t" + reads::Write(It.second) + "\n";
	}
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	using namespace autoplay_detail;
	if (P.size() < 2 || P[0] != "autoplay")
	{
		return false;
	}
	Book& B = L.Auto;
	if (P[1] == "pref" && P.size() >= 9)
	{
		Prefs& Pr = B.Pref;
		Pr.S = AutoStyleFrom(P[2]);
		Pr.Speed = AutoSpeedOf(AutoInt(P[3], 1));
		Pr.To = AutoTargetOf(AutoInt(P[4], static_cast<long long>(Target::Bubble)));
		Pr.Stops = static_cast<uint32_t>(AutoInt(P[5], StopFinalTable | StopBubble)) & AutoStopMask;
		Pr.Form = std::min(100.0, std::max(0.0, AutoFromMilli(AutoInt(P[6], AutoMilli(FormStart)))));
		Pr.FormWeek = static_cast<int>(AutoInt(P[7], -1));
		Pr.WeekDecisions = static_cast<int>(AutoInt(P[8]));
		L.Update2LinesRead |= life::U2AutoPref;
	}
	else if (P[1] == "played" && P.size() >= 10)
	{
		if (B.Results.size() < AutoKeepPlayed)
		{
			Played Pl;
			Pl.Id = AutoUnescape(P[2]);
			Pl.Venue = static_cast<int>(AutoInt(P[3]));
			Pl.Site = static_cast<int>(AutoInt(P[4]));
			Pl.S = AutoStyleFrom(P[5]);
			Pl.AiDecisions = static_cast<int>(AutoInt(P[6]));
			Pl.OwnDecisions = static_cast<int>(AutoInt(P[7]));
			Pl.AiHands = static_cast<int>(AutoInt(P[8]));
			Pl.OwnHands = static_cast<int>(AutoInt(P[9]));
			B.Results.push_back(Pl);
		}
		L.Update2LinesRead |= life::U2AutoPlayed;
	}
	else if (P[1] == "note" && P.size() >= 19)
	{
		std::string Counts;
		for (size_t I = 4; I < 19; ++I)
		{
			Counts += (I == 4 ? "" : "\t") + P[I];
		}
		reads::Stats S;
		if (reads::Parse(Counts, S))
		{
			const std::string Key = AutoUnescape(P[2]);
			B.Notes[Key] = S;
			B.NoteSeen[Key] = static_cast<double>(AutoInt(P[3]));
		}
		L.Update2LinesRead |= life::U2Notes;
	}
	// (An autoplay line of a kind this build doesn't know is claimed and dropped, as older builds drop the update's.)
	return true;
}

void AfterParse(SaveData& D)
{
	// §6.5 step 1: a save with no autoplay\tpref gets Autopilot, 1x, the final-table and bubble stops, and an honest Form:
	// the mean accuracy of its last 20 results that have one (else 70). Nothing is marked, so it isn't written back until
	// the player's first graded decision or any autoplay use; each reload derives the same Form.
	life::State& L = D.Life;
	if (L.Update2LinesRead & life::U2AutoPref)
	{
		return;
	}
	Prefs P;
	double Sum = 0.0;
	int Count = 0;
	for (const HistoryEntry& H : D.History)
	{
		if (Count >= 20)
		{
			break;
		}
		if (H.AccuracyPct > 0.0)
		{
			Sum += H.AccuracyPct;
			++Count;
		}
	}
	P.Form = Count > 0 ? autoplay_detail::AutoQuantizeMilli(std::min(100.0, Sum / static_cast<double>(Count))) : FormStart;
	L.Auto.Pref = P;
}
} // namespace autoplay
} // namespace ss
