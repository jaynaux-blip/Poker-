#include "ShortStack/Game/Autoplay.h"

#include "ShortStack/Common.h"
#include "ShortStack/Game/Life.h"

#include <algorithm>
#include <cmath>

// Autoplay's bookkeeping (update 2 §2.5, §2.9, §6.3-§6.5). Phase A, day 1 (the lead): the formulas the spec gives in full
// (SkillTerm, SharpnessOf, BrainFor) and stubs that keep today's behaviour for the rest (Form doesn't move, nothing is
// recorded, no autoplay line is written or read). core-autoplay owns this file from here.

namespace ss
{
namespace autoplay
{
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

double FormAt(Book& B, double World)
{
	// Phase A's stub: no weekly decay yet (core-autoplay).
	(void)World;
	return B.Pref.Form;
}

style::HeroBrain BrainFor(const TableAuto& A, const Run& R, double TiltGain, int Where, const reads::Table* Notes, const style::Learned* Book)
{
	style::HeroBrain B;
	B.S = A.S;
	B.Sharp = R.Sharp;
	B.Tilt = R.Tilt;
	B.TiltGain = TiltGain;
	B.Seen = &R.Seen;
	B.Notes = Notes;
	B.Book = Book;
	B.Where = Where;
	return B;
}

void AfterHand(Run& R, const HandOutcome& O, double TiltGain)
{
	// Phase A's stub: the plan's tilt and counters are core-autoplay's.
	(void)R;
	(void)O;
	(void)TiltGain;
}

void NoteOwnDecision(Book& B, life::State& L, int AccuracyPoints, double World, bool TakenMidThink)
{
	// Phase A's stub: Form doesn't move yet, and U2AutoPref stays as it was (the line isn't written yet either).
	(void)B;
	(void)L;
	(void)AccuracyPoints;
	(void)World;
	(void)TakenMidThink;
}

bool ShouldStop(const TableAuto& A, const Hand& H, const BotDecision& Planned, int HeroSeat, const Tournament* T)
{
	// Phase A's stub: no stop rule hands a decision over yet.
	(void)A;
	(void)H;
	(void)Planned;
	(void)HeroSeat;
	(void)T;
	return false;
}

void Record(Book& B, const std::string& Id, int Venue, int Site, const TableAuto& A, const Run& R)
{
	// Phase A's stub: the played line is core-autoplay's.
	(void)B;
	(void)Id;
	(void)Venue;
	(void)Site;
	(void)A;
	(void)R;
}

void FoldNotes(Book& B, const reads::Table& Event, double World)
{
	(void)B;
	(void)Event;
	(void)World;
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
	// Phase A's stub: the auto host line is core-autoplay's (with ue-auto).
	(void)A;
	(void)R;
	return std::string();
}

bool ReadRun(const std::string& Text, TableAuto& A, Run& R)
{
	(void)Text;
	(void)A;
	(void)R;
	return false;
}

std::vector<std::string> Inventory()
{
	return {};
}

void WriteLines(const life::State& L, std::string& Out)
{
	// Phase A's stub: no autoplay line yet, so every save writes back as it was read.
	(void)L;
	(void)Out;
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	(void)P;
	(void)L;
	return false;
}

void AfterParse(SaveData& D)
{
	// Phase A's stub: an older save's Form (the mean accuracy of its last results) is core-autoplay's migration (§6.5).
	(void)D;
}
} // namespace autoplay
} // namespace ss
