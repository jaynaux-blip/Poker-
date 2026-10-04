// The character creator: New Game's four steps (who, how they got here, how they look, the card that
// says so) around a live portrait of the person being made.
#include "ShortStack/UI/FrontEnd.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"
#include "RiverLineShared.h"

#include "ShortStack/Rng.h"
#include "ShortStack/UI/Portrait.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace creator_detail
{
const char* const StepNames[4] = {"IDENTITY", "BACKGROUND", "LOOK", "REVIEW"};
const char* const StepTitles[4] = {"WHO ARE YOU?", "HOW YOU GOT HERE", "HOW YOU LOOK", "THIS IS YOU"};
const char* const LookTabs[4] = {"FACE", "HAIR", "BODY", "STYLE"};
constexpr int IdentityRows = 5; // first name, last name, screen name, age, country
/** Drafts kept to undo (each randomize, and a typed name a suggestion replaced). */
constexpr size_t UndoDepth = 12;

// The Embercrest's brand (art/blender/brand.py): midnight navy grounds, ash-white peaks and type, ember and gold light.
const uint32_t EcMidnight = 0x0a1020;
const uint32_t EcSlate = 0x1d2a47;
const uint32_t EcIce = 0x8d9ab4;
const uint32_t EcAsh = 0xebe7df;
const uint32_t EcEmberDeep = 0x9e2f0c;
const uint32_t EcEmber = 0xff6a24;
const uint32_t EcEmberHot = 0xf2832e;
const uint32_t EcGold = 0xf6b04a;
const uint32_t EcFlame = 0xffd78a;

float Fc(int V)
{
	return static_cast<float>(V);
}

Color WithAlpha(Color Col, float A)
{
	Col.A *= A;
	return Col;
}

/** The look's slots on one tab of the LOOK step, in order. */
std::vector<hero::Slot> TabSlots(int Tab)
{
	std::vector<hero::Slot> Out;
	for (int I = 0; I < hero::SlotCount; ++I)
	{
		if (hero::SlotGroup(static_cast<hero::Slot>(I)) == Tab)
		{
			Out.push_back(static_cast<hero::Slot>(I));
		}
	}
	return Out;
}

bool IsColorSlot(hero::Slot S)
{
	return S == hero::Slot::Skin || S == hero::Slot::Eyes || S == hero::Slot::HairColor || S == hero::Slot::OutfitColor;
}

uint32_t SwatchOf(hero::Slot S, int I)
{
	switch (S)
	{
	case hero::Slot::Skin: return hero::SkinTone(I);
	case hero::Slot::Eyes: return hero::EyeTone(I);
	case hero::Slot::HairColor: return hero::HairTone(I);
	default: return hero::OutfitTone(I);
	}
}

/** UTF-8 for a codepoint (the creator takes Latin letters with accents). */
std::string Utf8(uint32_t Cp)
{
	std::string Out;
	if (Cp < 0x80)
	{
		Out += static_cast<char>(Cp);
	}
	else if (Cp < 0x800)
	{
		Out += static_cast<char>(0xC0 | (Cp >> 6));
		Out += static_cast<char>(0x80 | (Cp & 0x3F));
	}
	return Out;
}

void PopGlyph(std::string& S)
{
	while (!S.empty())
	{
		const unsigned char Ch = static_cast<unsigned char>(S.back());
		S.pop_back();
		if ((Ch & 0xC0) != 0x80)
		{
			break;
		}
	}
}

/** A typed name, tidied: no space at either end and never two in a row (a stray one is invisible on screen). */
void TidyName(std::string& S)
{
	std::string Out;
	for (const char Ch : S)
	{
		if (Ch == ' ' && (Out.empty() || Out.back() == ' '))
		{
			continue;
		}
		Out += Ch;
	}
	while (!Out.empty() && Out.back() == ' ')
	{
		Out.pop_back();
	}
	S = Out;
}

/** The same person, as far as the creator shows: names, age, home, story and look. */
bool SameDraft(const hero::Character& A, const hero::Character& B)
{
	return A.FirstName == B.FirstName && A.LastName == B.LastName && A.Age == B.Age && A.Country == B.Country && A.Story == B.Story && A.Appearance == B.Appearance;
}

/** Where a country sits in the creator's grid (the first when unknown). */
int CountryIndex(const std::string& Code)
{
	const std::vector<hero::CountryInfo>& All = hero::Countries();
	for (size_t K = 0; K < All.size(); ++K)
	{
		if (Code == All[K].Code)
		{
			return static_cast<int>(K);
		}
	}
	return 0;
}

/** Screen names a gamepad player can flick through, made from the person's name and the year they were born. */
std::vector<std::string> ScreenNameIdeas(const hero::Character& Who, const std::string& Default)
{
	auto Plain = [](const std::string& S) {
		std::string Out;
		for (const char Ch : S)
		{
			if (Ch >= 'A' && Ch <= 'Z')
			{
				Out += static_cast<char>(Ch - 'A' + 'a');
			}
			else if ((Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9'))
			{
				Out += Ch;
			}
		}
		return Out;
	};
	const std::string F = Plain(Who.FirstName);
	const std::string L = Plain(Who.LastName);
	const std::string Year = std::to_string((2026 - Who.Age) % 100 + 100).substr(1);
	const std::vector<std::string> Raw = {Default, F + "_" + L.substr(0, 1), F + L, L + "_" + Year, F + Year, "the_" + F, F + ".grinds", L + "_shoves"};
	std::vector<std::string> Out;
	for (const std::string& S : Raw)
	{
		const bool Shaped = S.size() >= 3 && S.size() <= 16 && S.front() != '_' && S.front() != '.' && S.back() != '_' && S.back() != '.';
		if (Shaped && std::find(Out.begin(), Out.end(), S) == Out.end())
		{
			Out.push_back(S);
		}
	}
	return Out;
}

uint32_t NameHash(const std::string& S)
{
	uint32_t H = 2166136261u;
	for (const char Ch : S)
	{
		H ^= static_cast<unsigned char>(Ch);
		H *= 16777619u;
	}
	return H;
}

std::string HeightLabel(int Cm)
{
	const int Inches = static_cast<int>(static_cast<double>(Cm) / 2.54 + 0.5);
	return std::to_string(Inches / 12) + "'" + std::to_string(Inches % 12) + "\"";
}

/**
 * A paragraph that has to fit between Y and Bottom: it steps down a size or two first, then the last line that fits
 * ends in an ellipsis. Returns the last baseline.
 */
float FitLines(Canvas& C, const std::string& Text, float X, float Y, float W, float Bottom, float Size, float MinSize, int Weight, const Color& Col)
{
	float S = Size;
	std::vector<std::string> Lines = frontend_detail::WrapText(C, Text, W, S, Weight);
	auto Lead = [](float Sz) { return std::floor(Sz * 1.47f + 0.5f); };
	while (S > MinSize && Y + Lead(S) * static_cast<float>(Lines.size()) > Bottom)
	{
		S -= 1.0f;
		Lines = frontend_detail::WrapText(C, Text, W, S, Weight);
	}
	const int Fit = std::max(1, static_cast<int>((Bottom - Y) / Lead(S)));
	float Yl = Y;
	for (int I = 0; I < static_cast<int>(Lines.size()) && I < Fit; ++I)
	{
		Yl += Lead(S);
		std::string Line = Lines[static_cast<size_t>(I)];
		if (I == Fit - 1 && I + 1 < static_cast<int>(Lines.size()))
		{
			// The rest runs on into an ellipsis at the column's edge.
			Line += " " + Lines[static_cast<size_t>(I) + 1];
		}
		C.Text(Line, X, Yl, Ts(S, Weight, Col, Align::Left, Baseline::Alphabetic, false, W));
	}
	return Yl;
}

/**
 * The Embercrest's mark (brand.py's mark()) in a Size box centered at (Cx, Cy): a faceted three-peak crest with the
 * ember rising behind it, parted from it by a rim of the ground it sits on.
 */
void EmbercrestMark(Canvas& C, float Cx, float Cy, float Size, const Color& Ground)
{
	const float U = Size / 100.0f;
	const float Base = Cy + 28.0f * U; // the ridge's foot (brand.py draws y up; the canvas draws it down)
	auto P = [&](float Px, float Py) { return Vec2{Cx + Px * U, Base - Py * U}; };
	// The ember: a glow, then the disc from deep ember at its rim to gold low down, where it breaks over the ridge.
	const Vec2 E = P(0.0f, 42.0f);
	C.FillCircle(E.X, E.Y, 50.0f * U, Paint::Radial(E, 0.0f, E, 50.0f * U, Hex(EcEmberHot, 0.3f), 0.55f, Hex(EcEmberHot, 0.1f), Hex(EcEmberHot, 0.0f)));
	C.FillCircle(E.X, E.Y, 34.0f * U, Paint::Radial({E.X, E.Y + 9.0f * U}, 0.0f, E, 34.0f * U, Hex(EcFlame), 0.45f, Hex(EcEmberHot), Hex(EcEmberDeep)));
	// The crest in front of it.
	C.FillPolygon({P(-51.0f, -1.5f), P(-26.0f, 37.5f), P(-15.0f, 25.5f), P(0.0f, 64.0f), P(13.0f, 32.5f), P(25.0f, 43.5f), P(51.0f, -1.5f)}, Paint(Ground));
	C.FillPolygon({P(-48.0f, 0.0f), P(-26.0f, 34.0f), P(-15.0f, 22.0f), P(0.0f, 60.0f), P(13.0f, 29.0f), P(25.0f, 40.0f), P(48.0f, 0.0f)}, Paint(Hex(EcAsh)));
	// The faces away from the light, one per peak, and snow on the high one's lit face.
	C.FillPolygon({P(-26.0f, 34.0f), P(-15.0f, 22.0f), P(-13.0f, 0.0f), P(-21.0f, 0.0f)}, Paint(Hex(EcIce)));
	C.FillPolygon({P(0.0f, 60.0f), P(13.0f, 29.0f), P(15.0f, 0.0f), P(3.0f, 0.0f)}, Paint(Hex(EcIce)));
	C.FillPolygon({P(25.0f, 40.0f), P(48.0f, 0.0f), P(31.0f, 0.0f)}, Paint(Hex(EcIce)));
	C.FillPolygon({P(0.0f, 60.0f), P(-8.0f, 43.0f), P(-4.5f, 46.5f), P(-1.5f, 42.0f), P(1.6f, 49.0f)}, Paint(Hex(0xffffff)));
	// The ground line, parted under the high peak.
	C.FillRect({Cx - 48.0f * U, Base + 4.1f * U, 45.0f * U, 2.4f * U}, Paint(Hex(EcEmberHot)));
	C.FillRect({Cx + 3.0f * U, Base + 4.1f * U, 45.0f * U, 2.4f * U}, Paint(Hex(EcEmberHot)));
}

/** A die, for the randomize button when its label won't fit. */
void CreatorDie(Canvas& C, float Cx, float Cy, const Color& Col)
{
	C.StrokeRoundRect({Cx - 13.0f, Cy - 13.0f, 26.0f, 26.0f}, 5.0f, Col, 2.0f);
	C.FillCircle(Cx - 6.0f, Cy - 6.0f, 2.4f, Paint(Col));
	C.FillCircle(Cx, Cy, 2.4f, Paint(Col));
	C.FillCircle(Cx + 6.0f, Cy + 6.0f, 2.4f, Paint(Col));
}
} // namespace creator_detail

using namespace frontend_detail;
using namespace creator_detail;

/** A vector icon for each background, drawn in its color (declared in FrontEndShared.h for the menu's cards). */
void frontend_detail::BackgroundIcon(Canvas& C, hero::Background B, float Cx, float Cy, float S, const Color& Col)
{
	const float W = std::max(1.6f, S * 0.07f);
	switch (B)
	{
	case hero::Background::Kitchen:
	{
		// A skillet with steam.
		C.StrokeEllipse(Cx - S * 0.12f, Cy + S * 0.16f, S * 0.3f, S * 0.12f, Col, W);
		C.StrokePolyline({{Cx + S * 0.18f, Cy + S * 0.14f}, {Cx + S * 0.48f, Cy + S * 0.06f}}, false, Col, W * 1.3f, true);
		for (int I = -1; I <= 1; ++I)
		{
			const float X = Cx - S * 0.12f + creator_detail::Fc(I) * S * 0.13f;
			C.StrokePolyline({{X, Cy}, {X - S * 0.05f, Cy - S * 0.12f}, {X + S * 0.04f, Cy - S * 0.24f}, {X - S * 0.02f, Cy - S * 0.36f}}, false, Col, W * 0.8f, true);
		}
		break;
	}
	case hero::Background::DealersKid:
	{
		// Two cards fanned, an ace of spades in front.
		for (int I = 0; I < 2; ++I)
		{
			C.Save();
			C.Translate(Cx + (I == 0 ? -S * 0.13f : S * 0.1f), Cy + (I == 0 ? S * 0.02f : 0.0f));
			C.Rotate(I == 0 ? -0.34f : 0.2f);
			const Rect Rc{-S * 0.19f, -S * 0.27f, S * 0.38f, S * 0.54f};
			C.FillRoundRect(Rc, S * 0.05f, Paint(Hex(0x0b0e16, Col.A)));
			C.StrokeRoundRect(Rc, S * 0.05f, I == 0 ? creator_detail::WithAlpha(Col, 0.6f) : Col, W);
			if (I == 1)
			{
				C.FillCircle(-S * 0.05f, S * 0.0f, S * 0.065f, Paint(Col));
				C.FillCircle(S * 0.05f, S * 0.0f, S * 0.065f, Paint(Col));
				C.FillPolygon({{-S * 0.11f, -S * 0.005f}, {0.0f, -S * 0.13f}, {S * 0.11f, -S * 0.005f}}, Paint(Col));
				C.FillPolygon({{0.0f, S * 0.0f}, {S * 0.045f, S * 0.11f}, {-S * 0.045f, S * 0.11f}}, Paint(Col));
			}
			C.Restore();
		}
		break;
	}
	case hero::Background::Dropout:
	{
		// A bell curve over an axis, the mean marked.
		std::vector<Vec2> Curve;
		for (int I = 0; I <= 24; ++I)
		{
			const float T = creator_detail::Fc(I) / 24.0f * 2.0f - 1.0f;
			Curve.push_back({Cx + T * S * 0.42f, Cy + S * 0.24f - S * 0.48f * std::exp(-T * T * 5.0f)});
		}
		C.StrokePolyline(Curve, false, Col, W, true);
		C.StrokePolyline({{Cx - S * 0.46f, Cy + S * 0.26f}, {Cx + S * 0.46f, Cy + S * 0.26f}}, false, Col, W * 0.8f, true);
		for (float Y = Cy - S * 0.22f; Y < Cy + S * 0.24f; Y += S * 0.1f)
		{
			C.StrokePolyline({{Cx, Y}, {Cx, Y + S * 0.05f}}, false, Col, W * 0.7f);
		}
		break;
	}
	case hero::Background::Bouncer:
	{
		// Two posts and the velvet rope.
		for (int I = -1; I <= 1; I += 2)
		{
			const float X = Cx + creator_detail::Fc(I) * S * 0.32f;
			C.StrokePolyline({{X, Cy - S * 0.2f}, {X, Cy + S * 0.3f}}, false, Col, W * 1.2f, true);
			C.FillCircle(X, Cy - S * 0.24f, S * 0.06f, Paint(Col));
			C.StrokePolyline({{X - S * 0.1f, Cy + S * 0.31f}, {X + S * 0.1f, Cy + S * 0.31f}}, false, Col, W, true);
		}
		std::vector<Vec2> Rope;
		for (int I = 0; I <= 12; ++I)
		{
			const float T = creator_detail::Fc(I) / 12.0f;
			Rope.push_back({Cx - S * 0.32f + T * S * 0.64f, Cy - S * 0.14f + S * 0.2f * std::sin(T * 3.1415926f)});
		}
		C.StrokePolyline(Rope, false, Col, W * 1.6f, true);
		break;
	}
	case hero::Background::Hustler:
	{
		// A burner flip phone.
		C.StrokeRoundRect({Cx - S * 0.17f, Cy - S * 0.28f, S * 0.34f, S * 0.6f}, S * 0.06f, Col, W);
		C.StrokePolyline({{Cx + S * 0.1f, Cy - S * 0.28f}, {Cx + S * 0.12f, Cy - S * 0.44f}}, false, Col, W, true);
		C.FillRoundRect({Cx - S * 0.11f, Cy - S * 0.21f, S * 0.22f, S * 0.16f}, S * 0.02f, Paint(creator_detail::WithAlpha(Col, 0.45f)));
		for (int R = 0; R < 3; ++R)
		{
			for (int K = 0; K < 3; ++K)
			{
				C.FillCircle(Cx + creator_detail::Fc(K - 1) * S * 0.08f, Cy + S * 0.04f + creator_detail::Fc(R) * S * 0.08f, S * 0.022f, Paint(Col));
			}
		}
		break;
	}
	default:
	{
		// A suitcase.
		C.StrokeRoundRect({Cx - S * 0.36f, Cy - S * 0.16f, S * 0.72f, S * 0.46f}, S * 0.06f, Col, W);
		C.StrokeRoundRect({Cx - S * 0.12f, Cy - S * 0.3f, S * 0.24f, S * 0.16f}, S * 0.04f, Col, W);
		for (int I = -1; I <= 1; I += 2)
		{
			C.StrokePolyline({{Cx + creator_detail::Fc(I) * S * 0.2f, Cy - S * 0.16f}, {Cx + creator_detail::Fc(I) * S * 0.2f, Cy + S * 0.3f}}, false, Col, W * 0.8f);
		}
		break;
	}
	}
}

// ------------------------------------------------------------------ flow

void FrontEnd::CreatorOpen(double Now)
{
	Stage = CreatorStage::Identity;
	Field = 0;
	LookTab = 0;
	StageAt = Now;
	Refusal.clear();
	RefusalAt = -10.0;
	UndoAt = -10.0;
	RimFrom = MenuTeal;
	RimTo = MenuTeal;
	RimAt = -10.0;
	if (DraftTouched)
	{
		// Back in after backing out: the person they were making is still here.
		return;
	}
	// A new face each time the creator opens: somewhere to start, not a template to fill in. The host's salt and the
	// moment it opens make it a different face from one launch to the next.
	Rng R("creator:" + CreatorSalt + ":" + std::to_string(Creations++) + ":" + std::to_string(std::llround(Now * 1000.0)));
	Draft = hero::Random(R);
	Rolled = Draft;
	Undo.clear();
}

void FrontEnd::CreatorGo(CreatorStage Target, double Now)
{
	Stage = Target;
	StageAt = Now;
	SelAt = Now;
	Field = 0;
	Refusal.clear();
	Hot.assign(8, 0.0f);
}

std::string FrontEnd::CreatorProblem() const
{
	const std::string Who = Draft.Problem();
	if (!Who.empty())
	{
		return Who;
	}
	if (!NameValid())
	{
		return "Screen names are 3 to 16 characters: letters, numbers, _ . -";
	}
	return "";
}

void FrontEnd::CreatorRefuse(const std::string& Why, double Now)
{
	Refusal = Why;
	RefusalAt = Now;
	NameErrorAt = Now;
	Sound(SoundId::Fold, 0.5);
}

void FrontEnd::TidyNames()
{
	TidyName(Draft.FirstName);
	TidyName(Draft.LastName);
}

void FrontEnd::CreatorNext(double Now)
{
	if (Stage == CreatorStage::Identity)
	{
		// A space at either end is invisible: take it off rather than refuse the name for it.
		TidyNames();
		const std::string Why = CreatorProblem();
		if (!Why.empty())
		{
			// Point at the field that needs fixing.
			Field = !hero::NameValid(Draft.FirstName) ? 0 : !Draft.LastName.empty() && !hero::NameValid(Draft.LastName) ? 1 : !NameValid() ? 2 : 4;
			CreatorRefuse(Why, Now);
			return;
		}
	}
	if (Stage == CreatorStage::Review)
	{
		BeginNewGame(Now);
		return;
	}
	Sound(SoundId::Chip, 0.5);
	if (Stage == CreatorStage::Look)
	{
		// The card's photo is taken.
		Sound(SoundId::Flip, 0.45);
	}
	CreatorGo(static_cast<CreatorStage>(static_cast<int>(Stage) + 1), Now);
}

void FrontEnd::CreatorBack(double Now)
{
	Sound(SoundId::Check, 0.4);
	if (Stage == CreatorStage::Identity)
	{
		Go(Page::Main, Now);
		Sel = 1;
		return;
	}
	CreatorGo(static_cast<CreatorStage>(static_cast<int>(Stage) - 1), Now);
}

void FrontEnd::BeginNewGame(double Now)
{
	TidyNames();
	const std::string Why = CreatorProblem();
	if (!Why.empty())
	{
		CreatorGo(CreatorStage::Identity, Now);
		CreatorRefuse(Why, Now);
		return;
	}
	if (Info.HasSave)
	{
		// Four presses from the main menu mustn't wipe a career: ask first, with CANCEL the default.
		Confirm = Modal::Overwrite;
		ConfirmAt = Now;
		ConfirmSel = 1;
		Sound(SoundId::Chip, 0.5);
		return;
	}
	StartCareer(Now);
}

void FrontEnd::StartCareer(double Now)
{
	Sound(SoundId::ChipStack, 0.8);
	Draft.Created = true;
	// The draft is spent: the next New Game starts from a fresh face.
	DraftTouched = false;
	Undo.clear();
	Close(Now);
	Hooks.NewCareer(ScreenName, Draft);
}

void FrontEnd::PushUndo()
{
	Undo.push_back(Draft);
	if (Undo.size() > UndoDepth)
	{
		Undo.erase(Undo.begin());
	}
}

void FrontEnd::CreatorRandomize(bool LookOnly, double Now)
{
	PushUndo();
	Rng R("creator:roll:" + CreatorSalt + ":" + std::to_string(Rolls++) + ":" + std::to_string(Creations) + ":" + std::to_string(std::llround(Now * 1000.0)));
	if (LookOnly)
	{
		// This person's look, rolled for their own age (grey comes with the years) and body type.
		Draft.Appearance = hero::RandomLook(R, Draft.Age, Draft.Appearance.Body);
	}
	else
	{
		const hero::Background Kept = Draft.Story;
		Draft = hero::Random(R);
		Draft.Story = Kept;
	}
	Rolled = Draft;
	CreatorTouched();
	SelAt = Now;
	Sound(SoundId::ChipStack, 0.5);
}

bool FrontEnd::CreatorCanUndo() const
{
	// Only where the dice are, and only while the roll stands as it landed: undo takes back a roll, never the names,
	// background or look the player set by hand since.
	return !Undo.empty() && (Stage == CreatorStage::Identity || Stage == CreatorStage::Look) && SameDraft(Draft, Rolled);
}

void FrontEnd::CreatorUndo(double Now)
{
	if (!CreatorCanUndo())
	{
		return;
	}
	Draft = Undo.back();
	Undo.pop_back();
	Rolled = Draft;
	CreatorTouched();
	UndoAt = Now;
	SelAt = Now;
	Sound(SoundId::Check, 0.45);
}

std::string* FrontEnd::FocusedText()
{
	if (Stage != CreatorStage::Identity)
	{
		return nullptr;
	}
	switch (Field)
	{
	case 0: return &Draft.FirstName;
	case 1: return &Draft.LastName;
	case 2: return &ScreenName;
	default: return nullptr;
	}
}

int FrontEnd::CreatorRows() const
{
	switch (Stage)
	{
	case CreatorStage::Identity: return IdentityRows;
	case CreatorStage::Background: return hero::BackgroundCount;
	case CreatorStage::Look: return static_cast<int>(TabSlots(LookTab).size());
	default: return 1;
	}
}

/** Left/Right on the focused row. */
void FrontEnd::CreatorChange(int Delta, bool Wrap)
{
	if (Stage == CreatorStage::Identity)
	{
		if (Field == 3)
		{
			Draft.Age = std::clamp(Draft.Age + Delta, hero::MinAge, hero::MaxAge);
		}
		else if (Field == 4)
		{
			const std::vector<hero::CountryInfo>& All = hero::Countries();
			const int N = static_cast<int>(All.size());
			const int I = CountryIndex(Draft.Country);
			const int Next = Wrap ? (I + Delta + N) % N : std::clamp(I + Delta, 0, N - 1);
			Draft.Country = All[static_cast<size_t>(Next)].Code;
		}
		else
		{
			return;
		}
	}
	else if (Stage == CreatorStage::Background)
	{
		const int N = hero::BackgroundCount;
		Draft.Story = static_cast<hero::Background>((static_cast<int>(Draft.Story) + Delta + N) % N);
	}
	else if (Stage == CreatorStage::Look)
	{
		const std::vector<hero::Slot> Slots = TabSlots(LookTab);
		if (Field < 0 || Field >= static_cast<int>(Slots.size()))
		{
			return;
		}
		const hero::Slot S = Slots[static_cast<size_t>(Field)];
		Draft.Appearance.Set(S, Draft.Appearance.Get(S) + Delta, Wrap && S != hero::Slot::Height);
	}
	else
	{
		return;
	}
	CreatorTouched();
	SelAt = LastNow;
	Sound(SoundId::Click, 0.3);
}

/** A gamepad on a name field: Left and Right flick through names from their country (screen names from their name). */
void FrontEnd::CreatorSuggest(int Delta)
{
	std::string* Text = FocusedText();
	if (!Text)
	{
		return;
	}
	const std::vector<std::string> Pool = Field == 2 ? ScreenNameIdeas(Draft, Info.HeroName) : hero::NameSuggestions(Draft.Country, Field == 1);
	if (Pool.empty())
	{
		return;
	}
	const int N = static_cast<int>(Pool.size());
	int I = -1;
	for (int K = 0; K < N; ++K)
	{
		I = Pool[static_cast<size_t>(K)] == *Text ? K : I;
	}
	// A first or last name is part of the person (the screen name isn't, and undo leaves it be).
	const bool Person = Field != 2;
	if (Person && ((I < 0 && !Text->empty()) || !SameDraft(Draft, Rolled)))
	{
		// A name they typed, or anything changed by hand since the last roll, isn't lost to the list: undo brings it back.
		PushUndo();
	}
	I = I < 0 ? (Delta > 0 ? 0 : N - 1) : (I + Delta + N) % N;
	*Text = Pool[static_cast<size_t>(I)];
	if (Person)
	{
		Rolled = Draft;
	}
	CreatorTouched();
	SelAt = LastNow;
	Sound(SoundId::Click, 0.3);
}

void FrontEnd::CreatorKey(const std::string& Name, double Now)
{
	const bool Typing = FocusedText() != nullptr;
	if (Name == "Enter")
	{
		CreatorNext(Now);
		return;
	}
	if (Name == "Escape" || (Name == "P" && InGame))
	{
		CreatorBack(Now);
		return;
	}
	if (Name == "Undo" || Name == "PutBack")
	{
		CreatorUndo(Now);
		return;
	}
	if (Name == "Backspace")
	{
		if (std::string* Text = FocusedText())
		{
			if (!Text->empty())
			{
				PopGlyph(*Text);
				CreatorTouched();
				Sound(SoundId::Click, 0.2);
			}
		}
		else if (Stage == CreatorStage::Look)
		{
			// Nothing to type on the look: Backspace takes back the last roll of the dice (CreatorUndo checks there is one
			// to take back, with nothing changed since). Not on the identity's age and country: next to the name fields,
			// a Backspace meant for the age box mustn't swap the person out.
			CreatorUndo(Now);
		}
		return;
	}
	const int Rows = CreatorRows();
	if (Stage == CreatorStage::Background)
	{
		// Three across, two down.
		const int I = static_cast<int>(Draft.Story);
		int Next = I;
		if (Name == "Left" || Name == "Right")
		{
			Next = (I / 3) * 3 + (I % 3 + (Name == "Left" ? 2 : 1)) % 3;
		}
		else if (Name == "Up" || Name == "Down")
		{
			Next = (I + 3) % 6;
		}
		if (Next != I)
		{
			Draft.Story = static_cast<hero::Background>(Next);
			CreatorTouched();
			SelAt = Now;
			Sound(SoundId::Click, 0.3);
		}
		return;
	}
	if (Stage == CreatorStage::Identity && Field == 4 && (Name == "Up" || Name == "Down"))
	{
		// Up and down move through the country grid a row at a time, out of it at the top, and stop at the bottom.
		const std::vector<hero::CountryInfo>& All = hero::Countries();
		const int I = CountryIndex(Draft.Country);
		if (Name == "Up")
		{
			if (I < 7)
			{
				Field = 3;
			}
			else
			{
				Draft.Country = All[static_cast<size_t>(I - 7)].Code;
				CreatorTouched();
			}
		}
		else if (I + 7 < static_cast<int>(All.size()))
		{
			Draft.Country = All[static_cast<size_t>(I + 7)].Code;
			CreatorTouched();
		}
		else
		{
			return;
		}
		SelAt = Now;
		Sound(SoundId::Click, 0.3);
		return;
	}
	if (Name == "Up" || Name == "Down")
	{
		Field = (Field + (Name == "Up" ? Rows - 1 : 1)) % std::max(1, Rows);
		SelAt = Now;
		Sound(SoundId::Click, 0.3);
	}
	else if (Name == "Tab")
	{
		if (Stage == CreatorStage::Look)
		{
			LookTab = (LookTab + 1) % 4;
			Field = 0;
		}
		else
		{
			Field = (Field + 1) % std::max(1, Rows);
		}
		SelAt = Now;
		Sound(SoundId::Click, 0.3);
	}
	else if (Name == "Left" || Name == "Right")
	{
		const int Delta = Name == "Left" ? -1 : 1;
		if (Stage == CreatorStage::Identity && Field <= 2 && Gamepad)
		{
			// No keyboard to hand: suggestions instead.
			CreatorSuggest(Delta);
		}
		else
		{
			CreatorChange(Delta, Stage == CreatorStage::Look);
		}
	}
	else if (Stage == CreatorStage::Look && (Name == "Q" || Name == "E" || Name == "TabPrev" || Name == "TabNext"))
	{
		LookTab = (LookTab + (Name == "Q" || Name == "TabPrev" ? 3 : 1)) % 4;
		Field = 0;
		SelAt = Now;
		Sound(SoundId::Click, 0.4);
	}
	else if (((Name == "R" && !Typing) || Name == "Reset") && (Stage == CreatorStage::Identity || Stage == CreatorStage::Look))
	{
		// Only where the dice are: on Background and Review a stray R or Y mustn't reroll the person made.
		CreatorRandomize(Stage == CreatorStage::Look, Now);
	}
}

void FrontEnd::CreatorChar(uint32_t Codepoint, double Now)
{
	std::string* Text = FocusedText();
	if (!Text)
	{
		return;
	}
	const bool Screen = Field == 2;
	bool Ok = false;
	if (Screen)
	{
		Ok = (Codepoint >= 'a' && Codepoint <= 'z') || (Codepoint >= 'A' && Codepoint <= 'Z') || (Codepoint >= '0' && Codepoint <= '9') || Codepoint == '_' || Codepoint == '.' || Codepoint == '-';
	}
	else
	{
		// Names: letters (with accents), and spaces, apostrophes and hyphens between them (never first, never two in a row).
		const bool Letter = (Codepoint >= 'a' && Codepoint <= 'z') || (Codepoint >= 'A' && Codepoint <= 'Z') || (Codepoint >= 0xC0 && Codepoint <= 0x24F && Codepoint != 0xD7 && Codepoint != 0xF7);
		const bool Joiner = Codepoint == ' ' || Codepoint == '\'' || Codepoint == '-';
		const char Last = Text->empty() ? ' ' : Text->back();
		Ok = Letter || (Joiner && Last != ' ' && Last != '\'' && Last != '-');
	}
	const std::string Add = Utf8(Codepoint);
	const bool Full = Screen ? Text->size() + Add.size() > 16 : hero::NameLength(*Text) >= hero::MaxNameLength;
	if (!Ok || Add.empty() || Full)
	{
		return;
	}
	*Text += Add;
	CreatorTouched();
	Sound(SoundId::Click, 0.2);
	(void)Now;
}

// ------------------------------------------------------------------ drawing

bool FrontEnd::TextBox(const Rect& R, const std::string& Label, const std::string& Value, int Index, double Now)
{
	const bool Focus = Field == Index;
	const bool Over = Interactive && Inside(R, Ptr.X, Ptr.Y);
	TrackedText(*C, Label, R.X, R.Y - 14.0f, 13.0f, 700, F(Focus ? MenuInk : MenuMuted), 3.2f);
	C->FillRect(R, Paint(F(MenuInk, Focus ? 0.09f : Over ? 0.07f : 0.05f)));
	const bool Bad = Focus && Now - RefusalAt < 1.6;
	C->FillRect({R.X, R.Y + R.H - 3.0f, R.W, 3.0f}, Paint(F(Bad ? MenuWarn : Focus ? MenuNeon : MenuInk, Focus ? 1.0f : 0.18f)));
	// A long value scrolls while it's typed, so its end and the caret stay in view; at rest it ends in an ellipsis.
	const float Size = R.W < 300.0f ? 26.0f : 30.0f;
	const float Room = R.W - 90.0f;
	const float Base = R.Y + R.H * 0.5f + Size * 0.367f;
	float Tw = 0.0f;
	if (Focus)
	{
		const float Full = C->Measure(Value, Size, 700);
		const float Shift = std::max(0.0f, Full - Room);
		C->PushClip({R.X + 12.0f, R.Y, Room + 18.0f, R.H});
		C->Text(Value, R.X + 22.0f - Shift, Base, Ts(Size, 700, F(MenuInk)));
		C->PopClip();
		Tw = Full - Shift;
	}
	else
	{
		Tw = C->Text(Value, R.X + 22.0f, Base, Ts(Size, 700, F(MenuInk), Align::Left, Baseline::Alphabetic, false, Room));
	}
	if (Value.empty() && !Focus)
	{
		C->Text("Type here", R.X + 22.0f, R.Y + R.H * 0.5f + 9.0f, Ts(24.0f, 400, F(MenuDim)));
	}
	if (Focus && std::fmod(Now, 1.0) < 0.55)
	{
		C->FillRect({R.X + 25.0f + Tw, R.Y + 17.0f, 3.0f, R.H - 34.0f}, Paint(F(MenuNeon)));
	}
	// Letters, not bytes: an accented letter counts once.
	TrackedText(*C, std::to_string(hero::NameLength(Value)) + "/16", R.X + R.W - 18.0f, R.Y + R.H * 0.5f + 5.0f, 11.0f, 700, F(MenuMuted, 0.8f), 2.0f, Align::Right);
	if (Released(R))
	{
		Field = Index;
		SelAt = Now;
		Sound(SoundId::Click, 0.3);
		return true;
	}
	return false;
}

void FrontEnd::CreatorHeader(double Now)
{
	const float X = MenuMargin;
	const float In = Ease(PageTime / 0.45);
	const float Dx = (1.0f - In) * -30.0f;
	const int S = static_cast<int>(Stage);
	C->FillRect({X + Dx + 2.0f, 143.0f, 44.0f, 3.0f}, Paint(F(MenuNeon, In)));
	TrackedText(*C, "NEW CAREER \xC2\xB7 STEP " + std::to_string(S + 1) + " OF 4", X + Dx + 62.0f, 151.0f, 15.0f, 700, F(MenuNeon, In), 6.0f);
	const float Ti = Ease((Now - StageAt) / 0.35);
	TrackedText(*C, StepTitles[S], X + (1.0f - Ti) * -18.0f, 230.0f, 62.0f, 900, F(MenuInk, Ti), 3.0f);
	// The stepper: done steps teal, this one neon, the rest waiting (dimmer, but still readable over the room). Narrow
	// screens tighten the names, then drop the numbers, so neighbours never touch.
	const float W = CreatorFormWidth(ViewW);
	const float Seg = W / 4.0f;
	float Track = 3.0f;
	auto Widest = [&](float T) {
		float Most = 0.0f;
		for (const char* Name : StepNames)
		{
			Most = std::max(Most, TrackedWidth(*C, Name, 13.0f, 700, T));
		}
		return Most;
	};
	bool Numbers = true;
	if (34.0f + Widest(Track) > Seg - 16.0f)
	{
		Track = 1.4f;
		Numbers = 34.0f + Widest(Track) <= Seg - 16.0f;
	}
	const Color Waiting = Mix(MenuDim, MenuMuted, 0.6f);
	for (int I = 0; I < 4; ++I)
	{
		const float Sx = X + Fc(I) * Seg;
		const Rect Hit{Sx, 252.0f, Seg - 12.0f, 44.0f};
		const bool Done = I < S;
		const bool Here = I == S;
		const Color Col = Here ? MenuNeon : Done ? MenuTeal : Waiting;
		float Nx = Sx;
		if (Numbers)
		{
			TrackedText(*C, (I < 9 ? "0" : "") + std::to_string(I + 1), Sx, 276.0f, 13.0f, 900, F(Col), 2.0f);
			Nx += 34.0f;
		}
		TrackedText(*C, StepNames[I], Nx, 276.0f, 13.0f, 700, F(Here ? MenuInk : Done ? MenuMuted : Waiting, Here || Done ? 1.0f : 0.9f), Track);
		C->FillRect({Sx, 290.0f, Seg - 12.0f, 3.0f}, Paint(F(Col, Here ? 1.0f : Done ? 0.7f : 0.3f)));
		if (Here)
		{
			C->FillRect({Sx, 290.0f, (Seg - 12.0f) * Ti, 3.0f}, Paint(F(Hex(0xff8ac0))));
		}
		if (I != S && Released(Hit))
		{
			if (I < S)
			{
				Sound(SoundId::Check, 0.4);
				CreatorGo(static_cast<CreatorStage>(I), Now);
			}
			else if (I == S + 1)
			{
				CreatorNext(Now);
			}
		}
	}
}

void FrontEnd::CreatorPortrait(const Rect& R, double Now)
{
	const hero::BackgroundInfo& Bi = hero::InfoOf(Draft.Story);
	const float In = Ease(PageTime / 0.6);
	C->GlowRoundRect({R.X, R.Y + 12.0f, R.W, R.H}, 18.0f, F(Hex(0x000000), 0.55f * In), 40.0f);
	C->FillRoundRect(R, 18.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, F(Hex(0x101830)), F(Hex(0x05070d))));
	C->PushClip(R);
	// The booth: a magenta wash from the street side, the rim light behind on the right, rain on the glass. The rim
	// takes the chosen background's color, easing over from the last one rather than snapping.
	const Color Want = Stage == CreatorStage::Identity ? MenuTeal : Hex(Bi.Color);
	if (std::fabs(Want.R - RimTo.R) + std::fabs(Want.G - RimTo.G) + std::fabs(Want.B - RimTo.B) > 0.002f)
	{
		RimFrom = Mix(RimFrom, RimTo, Ease((Now - RimAt) / 0.35));
		RimTo = Want;
		RimAt = Now;
	}
	const Color RimCol = Mix(RimFrom, RimTo, Ease((Now - RimAt) / 0.35));
	C->FillCircle(R.X + R.W * 0.18f, R.Y + R.H * 0.26f, R.W * 0.62f,
		Paint::Radial({R.X + R.W * 0.18f, R.Y + R.H * 0.26f}, 0.0f, {R.X + R.W * 0.18f, R.Y + R.H * 0.26f}, R.W * 0.62f, F(MenuNeon, 0.2f), 0.5f, F(MenuNeon, 0.06f), F(MenuNeon, 0.0f)));
	C->FillCircle(R.X + R.W * 0.95f, R.Y + R.H * 0.42f, R.W * 0.5f,
		Paint::Radial({R.X + R.W * 0.95f, R.Y + R.H * 0.42f}, 0.0f, {R.X + R.W * 0.95f, R.Y + R.H * 0.42f}, R.W * 0.5f, F(RimCol, 0.2f), 0.5f, F(RimCol, 0.06f), F(RimCol, 0.0f)));
	C->FillRect({R.X + R.W - 6.0f, R.Y + R.H * 0.1f, 6.0f, R.H * 0.62f}, Paint::Linear({0.0f, R.Y + R.H * 0.1f}, {0.0f, R.Y + R.H * 0.72f}, F(RimCol, 0.0f), F(RimCol, 0.5f)));
	for (int I = 0; I < 46; ++I)
	{
		const float Fx = static_cast<float>((I * 7919) % 997) / 997.0f;
		const float Speed = 0.55f + static_cast<float>((I * 104729) % 89) / 89.0f * 0.6f;
		const float Fy = static_cast<float>(std::fmod(static_cast<double>(I) * 0.137 + Now * static_cast<double>(Speed) * 0.35, 1.0));
		const float Sx = R.X + Fx * R.W;
		const float Sy = R.Y + Fy * (R.H + 80.0f) - 60.0f;
		C->StrokePolyline({{Sx, Sy}, {Sx - 5.0f, Sy + 34.0f}}, false, F(Hex(0xbfd6ff), 0.06f), 1.0f, true);
	}
	// The person.
	const float K = R.H / 836.0f;
	PortraitLight Light;
	Light.Rim = RimCol;
	Light.RimStrength = 0.9f;
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * Fade);
	DrawPortrait(*C, Draft, R.X + R.W * 0.5f, R.Y + 318.0f * K, 1.5f * K, Now, Light);
	C->SetAlpha(A0);
	// The nameplate.
	C->FillRect({R.X, R.Y + R.H - 250.0f, R.W, 250.0f}, Paint::Linear({0.0f, R.Y + R.H - 250.0f}, {0.0f, R.Y + R.H - 90.0f}, F(Hex(0x05070d), 0.0f), F(Hex(0x05070d), 0.94f)));
	const std::string Full = Draft.FullName().empty() || Draft.FirstName.empty() ? std::string("YOUR NAME") : Draft.FullName();
	C->Text(Full, R.X + 32.0f, R.Y + R.H - 112.0f, Ts(40.0f, 900, F(Draft.FirstName.empty() ? MenuDim : MenuInk), Align::Left, Baseline::Alphabetic, false, R.W - 64.0f));
	const hero::CountryInfo* Ci = hero::FindCountry(Draft.Country);
	float Lx = R.X + 32.0f;
	if (Ci)
	{
		const float Fa = C->GetAlpha();
		C->SetAlpha(Fa * Fade);
		rlnet_detail::NetFlag(*C, Ci->Code, Lx, R.Y + R.H - 90.0f, 30.0f, 20.0f);
		C->SetAlpha(Fa);
		Lx += 42.0f;
	}
	const std::string HomeTown = Ci ? std::string(Ci->City) + ", " + Ci->Name : std::string("Somewhere");
	Lx += TrackedText(*C, std::to_string(Draft.Age) + " \xC2\xB7 " + HomeTown, Lx, R.Y + R.H - 74.0f, 14.0f, 700, F(MenuMuted), 2.0f);
	// Background pill and the screen name (which gives way to the pill on a narrow card).
	const std::string Pill = Bi.Name;
	const float Pw = TrackedWidth(*C, Pill, 12.0f, 800, 2.4f) + 54.0f;
	const Rect Pr{R.X + 32.0f, R.Y + R.H - 54.0f, Pw, 30.0f};
	C->FillRoundRect(Pr, 15.0f, Paint(F(Hex(Bi.Color), 0.14f)));
	C->StrokeRoundRect(Pr, 15.0f, F(Hex(Bi.Color), 0.7f), 1.2f);
	BackgroundIcon(*C, Draft.Story, Pr.X + 17.0f, Pr.Y + 15.0f, 22.0f, F(Hex(Bi.Color)));
	TrackedText(*C, Pill, Pr.X + 34.0f, Pr.Y + 20.0f, 12.0f, 800, F(Hex(Bi.Color)), 2.4f);
	C->Text("@" + (ScreenName.empty() ? std::string("...") : ScreenName), R.X + R.W - 32.0f, Pr.Y + 20.0f,
		Ts(14.0f, 700, F(MenuMuted), Align::Right, Baseline::Alphabetic, false, std::max(60.0f, R.W - 64.0f - Pw - 18.0f)));
	// Corner marks: live, and the height.
	const float Pulse = 0.5f + 0.5f * static_cast<float>(std::sin(Now * 3.0));
	C->FillCircle(R.X + 36.0f, R.Y + 34.0f, 5.0f, Paint(F(MenuWarn, 0.5f + 0.5f * Pulse)));
	TrackedText(*C, "LIVE PREVIEW", R.X + 50.0f, R.Y + 39.0f, 12.0f, 800, F(MenuInk, 0.75f), 3.0f);
	TrackedText(*C, HeightLabel(Draft.Appearance.Height) + " \xC2\xB7 " + hero::OptionName(hero::Slot::Build, Draft.Appearance.Build), R.X + R.W - 32.0f, R.Y + 39.0f, 12.0f, 800, F(MenuMuted), 2.4f,
		Align::Right);
	C->PopClip();
	C->StrokeRoundRect({R.X + 0.5f, R.Y + 0.5f, R.W - 1.0f, R.H - 1.0f}, 18.0f, F(MenuInk, 0.1f), 1.0f);
	C->FillRect({R.X + 18.0f, R.Y, 56.0f * In, 3.0f}, Paint(F(MenuNeon)));
}

void FrontEnd::IdentityStep(double Now)
{
	const float X = MenuMargin;
	const float W = CreatorFormWidth(ViewW);
	const float Half = (W - 24.0f) * 0.5f;
	const double Err = Now - RefusalAt;
	const float Shake = Err < 0.4 ? static_cast<float>(std::sin(Err * 70.0) * 8.0 * (1.0 - Err / 0.4)) : 0.0f;
	auto Sh = [&](int Index) { return Field == Index ? Shake : 0.0f; };
	TextBox({X + Sh(0), 356.0f, Half, 70.0f}, "FIRST NAME", Draft.FirstName, 0, Now);
	TextBox({X + Half + 24.0f + Sh(1), 356.0f, Half, 70.0f}, "LAST NAME", Draft.LastName, 1, Now);
	TextBox({X + Sh(2), 484.0f, Half, 70.0f}, "SCREEN NAME", ScreenName, 2, Now);
	C->Text("What RiverLine and the tables call you.", X, 584.0f, Ts(15.0f, 400, F(MenuMuted)));

	// Age: minus, the number, plus, and where it sits between 18 and 65 (drag it, or the wheel over it).
	const Rect Ar{X + Half + 24.0f, 484.0f, Half, 70.0f};
	const bool AgeFocus = Field == 3;
	TrackedText(*C, "AGE", Ar.X, Ar.Y - 14.0f, 13.0f, 700, F(AgeFocus ? MenuInk : MenuMuted), 3.2f);
	C->FillRect(Ar, Paint(F(MenuInk, AgeFocus ? 0.09f : 0.05f)));
	C->FillRect({Ar.X, Ar.Y + Ar.H - 3.0f, Ar.W, 3.0f}, Paint(F(AgeFocus ? MenuNeon : MenuInk, AgeFocus ? 1.0f : 0.18f)));
	const Rect Minus{Ar.X, Ar.Y, 70.0f, Ar.H};
	const Rect Plus{Ar.X + Ar.W - 70.0f, Ar.Y, 70.0f, Ar.H};
	Chevron(*C, Minus.X + 35.0f, Ar.Y + 35.0f, 9.0f, false, F(MenuInk, Draft.Age > hero::MinAge ? 0.9f : 0.25f));
	Chevron(*C, Plus.X + 35.0f, Ar.Y + 35.0f, 9.0f, true, F(MenuInk, Draft.Age < hero::MaxAge ? 0.9f : 0.25f));
	C->Text(std::to_string(Draft.Age), Ar.X + Ar.W * 0.5f, Ar.Y + 47.0f, Ts(34.0f, 900, F(MenuInk), Align::Center));
	const float Tk = Fc(Draft.Age - hero::MinAge) / Fc(hero::MaxAge - hero::MinAge);
	const Rect AgeTrack{Ar.X - 8.0f, Ar.Y + Ar.H + 2.0f, Ar.W + 16.0f, 24.0f};
	const bool AgeDrag = Interactive && Ptr.Down && Inside(AgeTrack, PressX, PressY);
	C->FillRect({Ar.X, Ar.Y + Ar.H + 12.0f, Ar.W, 2.0f}, Paint(F(MenuInk, 0.12f)));
	C->FillRect({Ar.X, Ar.Y + Ar.H + 12.0f, Ar.W * Tk, 2.0f}, Paint(F(MenuNeon, 0.8f)));
	C->FillCircle(Ar.X + Ar.W * Tk, Ar.Y + Ar.H + 13.0f, AgeDrag ? 7.0f : 5.0f, Paint(F(MenuInk)));
	TrackedText(*C, std::to_string(hero::MinAge), Ar.X, Ar.Y + Ar.H + 34.0f, 11.0f, 700, F(MenuMuted, 0.8f), 2.0f);
	TrackedText(*C, std::to_string(hero::MaxAge), Ar.X + Ar.W, Ar.Y + Ar.H + 34.0f, 11.0f, 700, F(MenuMuted, 0.8f), 2.0f, Align::Right);
	if (AgeDrag)
	{
		Field = 3;
		const float Pick = std::clamp((Ptr.X - Ar.X) / Ar.W, 0.0f, 1.0f);
		const int Age = hero::MinAge + static_cast<int>(std::lround(Pick * Fc(hero::MaxAge - hero::MinAge)));
		if (Age != Draft.Age)
		{
			Draft.Age = Age;
			CreatorTouched();
			SelAt = Now;
			Sound(SoundId::Click, 0.15);
		}
	}
	else if (Interactive && Ptr.Wheel != 0.0f && Inside({Ar.X, Ar.Y, Ar.W, Ar.H + 26.0f}, Ptr.X, Ptr.Y))
	{
		Field = 3;
		CreatorChange(Ptr.Wheel < 0.0f ? 1 : -1, false);
	}
	if (Released(Minus) || Released(Plus))
	{
		Field = 3;
		CreatorChange(Released(Plus) ? 1 : -1, false);
	}
	else if (Released(Ar))
	{
		Field = 3;
		SelAt = Now;
	}

	// Where they're from.
	const hero::CountryInfo* Home = hero::FindCountry(Draft.Country);
	const float Gy = 640.0f;
	const float Lw = TrackedText(*C, "FROM", X, Gy - 16.0f, 13.0f, 700, F(Field == 4 ? MenuInk : MenuMuted), 3.2f);
	if (Home)
	{
		C->Text(std::string(Home->City) + ", " + Home->Name, X + Lw + 18.0f, Gy - 15.0f, Ts(16.0f, 600, F(MenuInk, 0.85f), Align::Left, Baseline::Alphabetic, false, W - Lw - 18.0f));
	}
	const std::vector<hero::CountryInfo>& All = hero::Countries();
	const float Gap = 8.0f;
	const float Tw = (W - Gap * 6.0f) / 7.0f;
	const float Th = 52.0f;
	// Too narrow for the names (4:3, 5:4): flag and code alone, with the name in the FROM line above.
	const bool Names = Tw - 58.0f >= 64.0f;
	for (size_t I = 0; I < All.size(); ++I)
	{
		const hero::CountryInfo& Ci = All[I];
		const Rect T{X + Fc(static_cast<int>(I % 7)) * (Tw + Gap), Gy + Fc(static_cast<int>(I / 7)) * (Th + Gap), Tw, Th};
		const bool Picked = Draft.Country == Ci.Code;
		const bool Over = Interactive && Inside(T, Ptr.X, Ptr.Y);
		C->FillRect(T, Paint(F(Picked ? MenuNeon : MenuInk, Picked ? 0.16f : Over ? 0.09f : 0.05f)));
		if (Picked)
		{
			C->StrokeRoundRect({T.X + 0.75f, T.Y + 0.75f, T.W - 1.5f, T.H - 1.5f}, 1.0f, F(MenuNeon, Field == 4 ? 1.0f : 0.6f), 1.5f);
		}
		const float Fa = C->GetAlpha();
		C->SetAlpha(Fa * Fade);
		rlnet_detail::NetFlag(*C, Ci.Code, T.X + 12.0f, T.Y + 16.0f, 30.0f, 20.0f);
		C->SetAlpha(Fa);
		C->Text(Ci.Code, T.X + 52.0f, T.Y + (Names ? 26.0f : 32.0f), Ts(15.0f, 900, F(Picked ? MenuInk : Mix(MenuMuted, MenuInk, 0.35f))));
		if (Names)
		{
			C->Text(Ci.Name, T.X + 52.0f, T.Y + 42.0f, Ts(11.0f, 600, F(MenuMuted), Align::Left, Baseline::Alphabetic, false, T.W - 58.0f));
		}
		if (Released(T))
		{
			Draft.Country = Ci.Code;
			Field = 4;
			CreatorTouched();
			SelAt = Now;
			Sound(SoundId::Click, 0.3);
		}
	}
	if (Err < 3.0 && !Refusal.empty())
	{
		C->Text(Refusal, X, 892.0f, Ts(16.0f, 600, F(MenuWarn), Align::Left, Baseline::Alphabetic, false, W));
	}
}

void FrontEnd::BackgroundStep(double Now)
{
	const float X = MenuMargin;
	const float W = CreatorFormWidth(ViewW);
	const float Cw = (W - 32.0f) / 3.0f;
	const float Ch = 172.0f;
	// Narrow cards (4:3, 5:4) tuck the icon into the corner so the tagline keeps its room.
	const bool Compact = Cw < 250.0f;
	for (int I = 0; I < hero::BackgroundCount; ++I)
	{
		const hero::BackgroundInfo& B = hero::InfoOf(static_cast<hero::Background>(I));
		const float Ci = Ease((Now - StageAt - 0.04 * I) / 0.4);
		const Rect R{X + Fc(I % 3) * (Cw + 16.0f), 340.0f + Fc(I / 3) * (Ch + 16.0f) + (1.0f - Ci) * 16.0f, Cw, Ch};
		const bool Picked = Draft.Story == B.Id;
		const bool Over = Interactive && Inside(R, Ptr.X, Ptr.Y);
		const Color Bc = Hex(B.Color);
		if (Picked)
		{
			C->GlowRoundRect(R, 4.0f, F(Bc, 0.28f * Ci), 26.0f);
		}
		C->FillRect(R, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, F(Picked ? Mix(MenuPanel, Bc, 0.16f) : MenuPanel, 0.92f * Ci), F(MenuPanel, 0.8f * Ci)));
		C->StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 2.0f, F(Picked ? Bc : MenuInk, (Picked ? 0.9f : Over ? 0.24f : 0.1f) * Ci), Picked ? 2.0f : 1.0f);
		C->FillRect({R.X, R.Y, 4.0f, R.H}, Paint(F(Bc, (Picked ? 1.0f : 0.45f) * Ci)));
		const float Ix = R.X + (Compact ? 40.0f : 52.0f);
		const float Iy = R.Y + (Compact ? 40.0f : 52.0f);
		C->FillCircle(Ix, Iy, Compact ? 22.0f : 30.0f, Paint(F(Bc, 0.12f * Ci)));
		BackgroundIcon(*C, B.Id, Ix, Iy, Compact ? 32.0f : 44.0f, F(Bc, Ci));
		const float Ny = R.Y + (Compact ? 96.0f : 118.0f);
		C->Text(B.Name, R.X + 22.0f, Ny, Ts(Compact ? 20.0f : 23.0f, 800, F(MenuInk, Ci), Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
		FitLines(*C, B.Tagline, R.X + 22.0f, Ny + 2.0f, R.W - 40.0f, R.Y + Ch - 8.0f, 14.0f, 12.0f, 400, F(MenuMuted, Ci));
		if (Picked)
		{
			TrackedText(*C, "CHOSEN", R.X + R.W - 18.0f, R.Y + 30.0f, 11.0f, 800, F(Bc, Ci), 2.6f, Align::Right);
		}
		if (Released(R))
		{
			Draft.Story = B.Id;
			CreatorTouched();
			SelAt = Now;
			Sound(SoundId::Click, 0.35);
		}
	}
	// The story and what it gives, kept above the buttons however narrow the column.
	const hero::BackgroundInfo& B = hero::InfoOf(Draft.Story);
	const Color Bc = Hex(B.Color);
	const float A = Ease((Now - SelAt) / 0.3);
	const float Py = 724.0f + (1.0f - A) * 8.0f;
	const float StoryW = W * 0.58f;
	FitLines(*C, B.Story, X, Py, StoryW, 892.0f, 18.0f, 15.0f, 300, F(MenuInk, 0.9f * A));
	const float PerkW = W - StoryW - 32.0f;
	const float TextLines = static_cast<float>(WrapText(*C, B.PerkText, PerkW - 44.0f, 15.0f, 400).size());
	const Rect Perk{X + StoryW + 32.0f, Py + 4.0f, PerkW, std::clamp(86.0f + 22.0f * TextLines, 156.0f, 892.0f - Py - 4.0f)};
	C->FillRect(Perk, Paint(F(Bc, 0.08f * A)));
	C->FillRect({Perk.X, Perk.Y, 3.0f, Perk.H}, Paint(F(Bc, A)));
	TrackedText(*C, "PERK", Perk.X + 22.0f, Perk.Y + 32.0f, 12.0f, 800, F(Bc, A), 3.0f);
	C->Text(B.PerkName, Perk.X + 22.0f, Perk.Y + 64.0f, Ts(24.0f, 900, F(MenuInk, A), Align::Left, Baseline::Alphabetic, false, Perk.W - 40.0f));
	FitLines(*C, B.PerkText, Perk.X + 22.0f, Perk.Y + 68.0f, Perk.W - 44.0f, Perk.Y + Perk.H - 8.0f, 15.0f, 13.0f, 400, F(MenuMuted, A));
}

void FrontEnd::LookStep(double Now)
{
	const float X = MenuMargin;
	const float W = CreatorFormWidth(ViewW);
	// Tabs.
	float Tx = X;
	const float TabY = 352.0f;
	Tx += Glyph(*C, Gamepad ? "LB" : "Q", Tx, TabY + 3.0f, Fade) + 26.0f;
	for (int I = 0; I < 4; ++I)
	{
		const bool On = I == LookTab;
		const float Lw = TrackedWidth(*C, LookTabs[I], 17.0f, 800, 4.0f);
		const Rect Hit{Tx - 10.0f, TabY - 26.0f, Lw + 20.0f, 40.0f};
		TrackedText(*C, LookTabs[I], Tx, TabY, 17.0f, 800, F(On ? MenuInk : MenuMuted, On ? 1.0f : 0.85f), 4.0f);
		if (On)
		{
			C->FillRect({Tx, TabY + 12.0f, Lw, 3.0f}, Paint(F(MenuNeon)));
		}
		if (Released(Hit) && !On)
		{
			LookTab = I;
			Field = 0;
			SelAt = Now;
			Sound(SoundId::Click, 0.4);
		}
		Tx += Lw + 40.0f;
	}
	Glyph(*C, Gamepad ? "RB" : "E", Tx - 14.0f, TabY + 3.0f, Fade);

	// Rows. Every control starts clear of its row's label, however narrow the column (4:3, 5:4).
	const std::vector<hero::Slot> Slots = TabSlots(LookTab);
	const float Rh = 72.0f;
	for (size_t I = 0; I < Slots.size(); ++I)
	{
		const hero::Slot S = Slots[I];
		const int Index = static_cast<int>(I);
		const float Ri = Ease((Now - StageAt - 0.03 * static_cast<double>(I)) / 0.35);
		const Rect R{X, 392.0f + Fc(Index) * (Rh + 8.0f), W, Rh};
		const bool Focus = Field == Index;
		const bool Over = Interactive && Inside(R, Ptr.X, Ptr.Y);
		C->FillRect(R, Paint(F(MenuInk, (Focus ? 0.08f : Over ? 0.06f : 0.04f) * Ri)));
		if (Focus)
		{
			C->FillRect({R.X, R.Y, 3.0f, R.H}, Paint(F(MenuNeon, Ri)));
		}
		const int V = Draft.Appearance.Get(S);
		const float Lw = TrackedText(*C, hero::SlotName(S), R.X + 24.0f, R.Y + (IsColorSlot(S) ? 32.0f : 43.0f), 13.0f, 800, F(Focus ? MenuInk : MenuMuted, Ri), 3.0f);
		if (IsColorSlot(S))
		{
			const std::string Value = hero::OptionName(S, V);
			C->Text(Value, R.X + 24.0f, R.Y + 56.0f, Ts(15.0f, 600, F(MenuInk, 0.85f * Ri)));
			const int N = hero::OptionCount(S);
			const float Left = R.X + 24.0f + std::max(Lw, C->Measure(Value, 15.0f, 600)) + 24.0f;
			const float Right = R.X + R.W - 34.0f;
			const float Sw = N > 1 ? std::clamp((Right - Left - 19.0f) / Fc(N - 1), 30.0f, 40.0f) : 40.0f;
			const float Rad = Sw < 36.0f ? 12.0f : 14.0f;
			for (int K = 0; K < N; ++K)
			{
				const float Cx = Right - Fc(N - 1 - K) * Sw;
				const float Cy = R.Y + R.H * 0.5f;
				if (K == V)
				{
					C->StrokeEllipse(Cx, Cy, Rad + 5.0f, Rad + 5.0f, F(Focus ? MenuNeon : MenuInk, Ri), 2.0f);
				}
				C->FillCircle(Cx, Cy, Rad, Paint::Linear({Cx - Rad, Cy - Rad}, {Cx + Rad, Cy + Rad}, F(Mix(Hex(SwatchOf(S, K)), Hex(0xffffff), 0.12f), Ri), F(Mix(Hex(SwatchOf(S, K)), Hex(0x000000), 0.2f), Ri)));
				C->StrokeEllipse(Cx, Cy, Rad, Rad, F(Hex(0xffffff), 0.12f * Ri), 1.0f);
				if (Released({Cx - Sw * 0.45f, Cy - 18.0f, Sw * 0.9f, 36.0f}))
				{
					Field = Index;
					Draft.Appearance.Set(S, K);
					CreatorTouched();
					SelAt = Now;
					Sound(SoundId::Click, 0.3);
				}
			}
		}
		else if (S == hero::Slot::Height)
		{
			const std::string Value = hero::OptionName(S, V);
			const float Vw = C->Measure(Value, 18.0f, 700);
			const float Sx = std::max(R.X + 24.0f + Lw + 28.0f, R.X + R.W - 520.0f);
			const float Sw = std::max(80.0f, std::min(300.0f, R.X + R.W - 24.0f - Vw - 36.0f - Sx));
			const float T = Fc(V - hero::MinHeight) / Fc(hero::MaxHeight - hero::MinHeight);
			C->FillRect({Sx, R.Y + 35.0f, Sw, 3.0f}, Paint(F(MenuInk, 0.15f * Ri)));
			C->FillRect({Sx, R.Y + 35.0f, Sw * T, 3.0f}, Paint(F(MenuNeon, Ri)));
			C->FillCircle(Sx + Sw * T, R.Y + 36.5f, 8.0f, Paint(F(MenuInk, Ri)));
			C->Text(Value, R.X + R.W - 24.0f, R.Y + 44.0f, Ts(18.0f, 700, F(MenuInk, Ri), Align::Right));
			const Rect Track{Sx - 10.0f, R.Y, Sw + 20.0f, R.H};
			if (Interactive && Ptr.Down && Inside(Track, PressX, PressY))
			{
				Field = Index;
				const float Pick = std::clamp((Ptr.X - Sx) / Sw, 0.0f, 1.0f);
				const int Cm = hero::MinHeight + static_cast<int>(std::lround(Pick * Fc(hero::MaxHeight - hero::MinHeight)));
				if (Cm != V)
				{
					Draft.Appearance.Set(S, Cm);
					CreatorTouched();
				}
			}
		}
		else
		{
			const float Cl = std::max(R.X + 24.0f + Lw + 40.0f, R.X + R.W - 500.0f);
			const float Cr = R.X + R.W - 40.0f;
			const float Mid = (Cl + Cr) * 0.5f;
			Chevron(*C, Cl, R.Y + 34.0f, 9.0f, false, F(MenuInk, (Focus ? 0.95f : 0.55f) * Ri));
			Chevron(*C, Cr, R.Y + 34.0f, 9.0f, true, F(MenuInk, (Focus ? 0.95f : 0.55f) * Ri));
			C->Text(hero::OptionName(S, V), Mid, R.Y + 41.0f, Ts(20.0f, 700, F(MenuInk, Ri), Align::Center, Baseline::Alphabetic, false, std::max(60.0f, Cr - Cl - 44.0f)));
			const int N = hero::OptionCount(S);
			const float Pip = std::min(10.0f, (Cr - Cl - 44.0f) / Fc(std::max(1, N)));
			const float P0 = Mid - Fc(N - 1) * Pip * 0.5f;
			for (int K = 0; K < N; ++K)
			{
				C->FillCircle(P0 + Fc(K) * Pip, R.Y + 58.0f, K == V ? 2.6f : 1.8f, Paint(F(K == V ? MenuNeon : MenuInk, (K == V ? 1.0f : 0.25f) * Ri)));
			}
			const Rect Lh{Cl - 30.0f, R.Y, 60.0f, R.H};
			const Rect Rh2{Cr - 30.0f, R.Y, 60.0f, R.H};
			if (Released(Lh) || Released(Rh2))
			{
				Field = Index;
				CreatorChange(Released(Rh2) ? 1 : -1, true);
			}
		}
		if (Interactive && Ptr.Wheel != 0.0f && Over)
		{
			// The wheel over a row steps it, like Left and Right.
			Field = Index;
			CreatorChange(Ptr.Wheel < 0.0f ? 1 : -1, S != hero::Slot::Height);
		}
		if (Released(R) && !Focus)
		{
			Field = Index;
			SelAt = Now;
		}
	}
	// The body tab: how tall, next to the apartment's door.
	if (LookTab == 2)
	{
		const float Base = 884.0f;
		const float Px = 1.3f;
		const float Cx = X + 120.0f;
		const float DoorH = 203.0f * Px;
		C->StrokeRoundRect({Cx + 170.0f, Base - DoorH, 92.0f * Px, DoorH}, 2.0f, F(MenuInk, 0.18f), 1.5f);
		C->FillCircle(Cx + 170.0f + 92.0f * Px - 14.0f, Base - DoorH * 0.48f, 3.0f, Paint(F(MenuInk, 0.3f)));
		TrackedText(*C, "THE DOOR \xC2\xB7 203 CM", Cx + 170.0f, Base - DoorH - 10.0f, 11.0f, 700, F(MenuMuted, 0.8f), 2.4f);
		for (int Cm = 150; Cm <= 200; Cm += 10)
		{
			const float Y = Base - Fc(Cm) * Px;
			C->FillRect({Cx - 96.0f, Y, Cm % 50 == 0 ? 16.0f : 9.0f, 1.0f}, Paint(F(MenuInk, 0.3f)));
			if (Cm % 50 == 0)
			{
				C->Text(std::to_string(Cm), Cx - 76.0f, Y + 4.0f, Ts(11.0f, 700, F(MenuMuted, 0.8f)));
			}
		}
		C->FillRect({X, Base, W * 0.5f, 1.0f}, Paint(F(MenuInk, 0.25f)));
		const float Top = Base - Fc(Draft.Appearance.Height) * Px;
		C->FillRect({Cx - 96.0f, Top, 300.0f, 1.0f}, Paint(F(MenuNeon, 0.6f)));
		DrawFigure(*C, Draft.Appearance, Cx, Base, Px, F(MenuNeon, 0.22f), F(MenuNeon, 0.9f));
		TrackedText(*C, HeightLabel(Draft.Appearance.Height), Cx + 60.0f, Top - 8.0f, 13.0f, 800, F(MenuNeon), 2.4f);
	}
}

void FrontEnd::PlayersCard(const Rect& R, double Now)
{
	const hero::BackgroundInfo& Bi = hero::InfoOf(Draft.Story);
	const float In = Ease((Now - StageAt) / 0.5);
	const Color Gold = Hex(EcGold);
	const Color Ash = Hex(EcAsh);
	const Color Ice = Hex(EcIce);
	// The Embercrest's card: midnight navy, the ember warming its corner, a gold edge and a foil sweep.
	C->GlowRoundRect({R.X, R.Y + 10.0f, R.W, R.H}, 22.0f, F(Hex(0x000000), 0.6f * In), 36.0f);
	C->FillRoundRect(R, 22.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, F(Hex(EcSlate)), F(Hex(EcMidnight))));
	C->PushClip({R.X + 12.0f, R.Y + 12.0f, R.W - 24.0f, R.H - 24.0f});
	C->FillCircle(R.X + 46.0f, R.Y + 48.0f, 220.0f,
		Paint::Radial({R.X + 46.0f, R.Y + 48.0f}, 0.0f, {R.X + 46.0f, R.Y + 48.0f}, 220.0f, F(Hex(EcEmber), 0.16f), 0.45f, F(Hex(EcEmber), 0.04f), F(Hex(EcEmber), 0.0f)));
	{
		// The crest again, large and faint, low on the right: a watermark.
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Fade * 0.05f);
		EmbercrestMark(*C, R.X + R.W - 96.0f, R.Y + R.H - 150.0f, 240.0f, Hex(EcMidnight));
		C->SetAlpha(A0);
	}
	const float Sweep = static_cast<float>(std::fmod(Now * 0.25, 1.6)) - 0.3f;
	const float Bx = R.X + Sweep * R.W;
	C->FillPolygon({{Bx, R.Y}, {Bx + 120.0f, R.Y}, {Bx - 60.0f, R.Y + R.H}, {Bx - 180.0f, R.Y + R.H}},
		Paint::Linear({Bx - 180.0f, 0.0f}, {Bx + 120.0f, 0.0f}, F(Hex(EcEmber), 0.0f), F(Gold, 0.1f)));
	C->PopClip();
	C->StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 22.0f, F(Gold, 0.4f), 1.5f);
	// Header: the mark, the wordmark and the club.
	{
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Fade);
		EmbercrestMark(*C, R.X + 48.0f, R.Y + 50.0f, 52.0f, Hex(EcSlate));
		C->SetAlpha(A0);
	}
	TrackedText(*C, "EMBERCREST", R.X + 86.0f, R.Y + 46.0f, 20.0f, 900, F(Ash), 5.0f);
	TrackedText(*C, "CASINO \xC2\xB7 PLAYERS CLUB", R.X + 86.0f, R.Y + 64.0f, 11.0f, 700, F(Gold, 0.9f), 3.0f);
	const float Pw = TrackedWidth(*C, "NEW MEMBER", 11.0f, 800, 2.4f) + 28.0f;
	C->FillRoundRect({R.X + R.W - 28.0f - Pw, R.Y + 30.0f, Pw, 26.0f}, 13.0f, Paint(F(Gold, 0.16f)));
	C->StrokeRoundRect({R.X + R.W - 28.0f - Pw, R.Y + 30.0f, Pw, 26.0f}, 13.0f, F(Gold, 0.45f), 1.0f);
	TrackedText(*C, "NEW MEMBER", R.X + R.W - 28.0f - Pw * 0.5f, R.Y + 48.0f, 11.0f, 800, F(Gold), 2.4f, Align::Center);
	C->FillRect({R.X + 28.0f, R.Y + 76.0f, R.W - 56.0f, 1.0f}, Paint::Linear({R.X + 28.0f, 0.0f}, {R.X + R.W - 28.0f, 0.0f}, F(Hex(EcEmber), 0.5f), F(Gold, 0.0f)));
	// Photo, with a flash as the card is printed.
	const Rect Ph{R.X + 28.0f, R.Y + 90.0f, 196.0f, 218.0f};
	C->FillRoundRect(Ph, 12.0f, Paint::Linear({0.0f, Ph.Y}, {0.0f, Ph.Y + Ph.H}, F(Hex(0x24304d)), F(Hex(0x0c1222))));
	C->PushClip(Ph);
	PortraitLight Light;
	Light.Rim = Hex(Bi.Color);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * Fade);
	DrawPortrait(*C, Draft, Ph.X + Ph.W * 0.5f, Ph.Y + 100.0f, 0.6f, 0.0, Light);
	C->SetAlpha(A0);
	const double Flash = Now - StageAt;
	if (Flash < 0.35)
	{
		C->FillRect(Ph, Paint(F(Hex(0xffffff), 0.6f * static_cast<float>(1.0 - Flash / 0.35))));
	}
	C->PopClip();
	C->StrokeRoundRect(Ph, 12.0f, F(Ash, 0.16f), 1.0f);
	// Fields.
	const float Fx = R.X + 252.0f;
	auto FieldText = [&](const std::string& Label, const std::string& Value, float Fx0, float Fy, float Size) {
		TrackedText(*C, Label, Fx0, Fy, 10.0f, 800, F(Ice), 2.6f);
		C->Text(Value, Fx0, Fy + Size + 6.0f, Ts(Size, 800, F(Ash), Align::Left, Baseline::Alphabetic, false, R.X + R.W - 28.0f - Fx0));
	};
	FieldText("NAME", Draft.FullName(), Fx, R.Y + 106.0f, 26.0f);
	FieldText("AGE", std::to_string(Draft.Age), Fx, R.Y + 172.0f, 20.0f);
	FieldText("HEIGHT", HeightLabel(Draft.Appearance.Height) + " \xC2\xB7 " + std::to_string(Draft.Appearance.Height) + " cm", Fx + 110.0f, R.Y + 172.0f, 20.0f);
	const hero::CountryInfo* Ci = hero::FindCountry(Draft.Country);
	TrackedText(*C, "FROM", Fx, R.Y + 232.0f, 10.0f, 800, F(Ice), 2.6f);
	if (Ci)
	{
		const float Fa = C->GetAlpha();
		C->SetAlpha(Fa * Fade);
		rlnet_detail::NetFlag(*C, Ci->Code, Fx, R.Y + 244.0f, 27.0f, 18.0f);
		C->SetAlpha(Fa);
		C->Text(std::string(Ci->City) + ", " + Ci->Name, Fx + 38.0f, R.Y + 259.0f, Ts(18.0f, 800, F(Ash), Align::Left, Baseline::Alphabetic, false, R.X + R.W - Fx - 66.0f));
	}
	FieldText("PLAYS AS", "@" + ScreenName, Fx, R.Y + 290.0f, 18.0f);
	// Footer: the member number and a stripe of barcode.
	const uint32_t H = NameHash(Draft.FullName() + ScreenName);
	char Num[32];
	std::snprintf(Num, sizeof(Num), "No. %04u %04u %04u", (H >> 20) % 10000u, (H >> 8) % 10000u, H % 10000u);
	C->Text(Num, R.X + 28.0f, R.Y + R.H - 30.0f, Ts(15.0f, 700, F(Ash, 0.8f), Align::Left, Baseline::Alphabetic, true));
	TrackedText(*C, "MEMBER SINCE OCT 2026", R.X + 28.0f, R.Y + R.H - 52.0f, 10.0f, 800, F(Ice), 2.6f);
	float Bx2 = R.X + R.W - 200.0f;
	for (int I = 0; I < 40 && Bx2 < R.X + R.W - 28.0f; ++I)
	{
		const float Bw = 1.0f + Fc(static_cast<int>((H >> (I % 29)) & 3u));
		C->FillRect({Bx2, R.Y + R.H - 58.0f, Bw, 34.0f}, Paint(F(Ash, 0.75f)));
		Bx2 += Bw + 2.0f;
	}
}

void FrontEnd::ReviewStep(double Now)
{
	const float X = MenuMargin;
	const float W = CreatorFormWidth(ViewW);
	// The card, and the perk beside it when there's room (16:10 narrows the card a little to keep it); on 4:3 and 5:4
	// the perk comes as a strip under the card.
	float Cw = std::min(620.0f, W);
	bool Beside = W - Cw >= 280.0f;
	if (!Beside && W - 308.0f >= 540.0f)
	{
		Cw = W - 308.0f;
		Beside = true;
	}
	PlayersCard({X, 336.0f, Cw, 392.0f}, Now);
	const hero::BackgroundInfo& B = hero::InfoOf(Draft.Story);
	const Color Bc = Hex(B.Color);
	float BioTop = 752.0f;
	if (Beside)
	{
		const Rect Perk{X + Cw + 28.0f, 336.0f, W - Cw - 28.0f, 392.0f};
		C->FillRect(Perk, Paint(F(Bc, 0.07f)));
		C->FillRect({Perk.X, Perk.Y, 3.0f, Perk.H}, Paint(F(Bc)));
		C->FillCircle(Perk.X + 56.0f, Perk.Y + 64.0f, 30.0f, Paint(F(Bc, 0.14f)));
		BackgroundIcon(*C, B.Id, Perk.X + 56.0f, Perk.Y + 64.0f, 44.0f, F(Bc));
		TrackedText(*C, std::string(B.Name), Perk.X + 24.0f, Perk.Y + 134.0f, 13.0f, 800, F(Bc), 3.0f);
		TrackedText(*C, "PERK", Perk.X + 24.0f, Perk.Y + 182.0f, 11.0f, 800, F(MenuMuted), 3.0f);
		C->Text(B.PerkName, Perk.X + 24.0f, Perk.Y + 214.0f, Ts(24.0f, 900, F(MenuInk), Align::Left, Baseline::Alphabetic, false, Perk.W - 48.0f));
		FitLines(*C, B.PerkText, Perk.X + 24.0f, Perk.Y + 218.0f, Perk.W - 48.0f, Perk.Y + Perk.H - 84.0f, 15.0f, 13.0f, 400, F(MenuMuted));
		C->FillRect({Perk.X + 24.0f, Perk.Y + Perk.H - 74.0f, 32.0f, 2.0f}, Paint(F(Bc, 0.6f)));
		FitLines(*C, "\xE2\x80\x9C" + std::string(B.Tagline) + "\xE2\x80\x9D", Perk.X + 24.0f, Perk.Y + Perk.H - 74.0f, Perk.W - 48.0f, Perk.Y + Perk.H - 10.0f, 16.0f, 13.0f, 300,
			F(MenuInk, 0.75f));
	}
	else
	{
		// A strip under the card: the icon, the perk's name and what it does. The perk is what the background changes, so
		// when one line won't hold it the strip grows a second rather than cut it off.
		const float TextW = W - 80.0f;
		const bool TwoLines = C->Measure(B.PerkText, 14.0f, 400) > TextW;
		const Rect Strip{X, TwoLines ? 740.0f : 744.0f, W, TwoLines ? 76.0f : 58.0f};
		C->FillRect(Strip, Paint(F(Bc, 0.08f)));
		C->FillRect({Strip.X, Strip.Y, 3.0f, Strip.H}, Paint(F(Bc)));
		BackgroundIcon(*C, B.Id, Strip.X + 34.0f, Strip.Y + Strip.H * 0.5f, 30.0f, F(Bc));
		const float Tx = Strip.X + 64.0f;
		const float Lw = TrackedText(*C, "PERK", Tx, Strip.Y + 24.0f, 10.0f, 800, F(Bc), 2.6f);
		C->Text(B.PerkName, Tx + Lw + 12.0f, Strip.Y + 25.0f, Ts(17.0f, 900, F(MenuInk), Align::Left, Baseline::Alphabetic, false, Strip.W - (Tx - Strip.X) - Lw - 28.0f));
		if (TwoLines)
		{
			FitLines(*C, B.PerkText, Tx, Strip.Y + 30.0f, TextW, Strip.Y + Strip.H - 6.0f, 14.0f, 12.0f, 400, F(MenuMuted));
		}
		else
		{
			C->Text(B.PerkText, Tx, Strip.Y + 46.0f, Ts(14.0f, 400, F(MenuMuted), Align::Left, Baseline::Alphabetic, false, TextW));
		}
		BioTop = Strip.Y + Strip.H + 10.0f;
	}
	// The bio, then (with room) a reminder that a career is waiting; BEGIN asks before replacing it either way.
	const bool Remind = Info.HasSave && Beside;
	FitLines(*C, hero::Bio(Draft), X, BioTop, W, Remind ? 868.0f : 892.0f, 19.0f, 15.0f, 300, F(MenuInk, 0.9f));
	if (Remind)
	{
		C->Text("Starting over replaces your current career.", X, 892.0f, Ts(16.0f, 600, F(MenuWarn)));
	}
}

void FrontEnd::NewGamePage(double Now)
{
	const float Card = CreatorCardWidth(ViewW);
	CreatorPortrait({ViewW - MenuMargin - Card, 112.0f, Card, 836.0f}, Now);
	CreatorHeader(Now);
	switch (Stage)
	{
	case CreatorStage::Identity: IdentityStep(Now); break;
	case CreatorStage::Background: BackgroundStep(Now); break;
	case CreatorStage::Look: LookStep(Now); break;
	default: ReviewStep(Now); break;
	}
	if (Cur != Page::NewGame)
	{
		return;
	}
	// Buttons: BACK, NEXT and the dice. On narrow screens they tighten, then the dice's label shortens to a die, so
	// none ever sits on another.
	const float X = MenuMargin;
	const float By = 908.0f;
	const float W = CreatorFormWidth(ViewW);
	const bool Last = Stage == CreatorStage::Review;
	const bool Dice = Stage == CreatorStage::Identity || Stage == CreatorStage::Look;
	auto LabelW = [&](const std::string& S) { return TrackedWidth(*C, S, 20.0f, 900, 3.0f) + 40.0f; };
	float BackW = 180.0f;
	float NextW = Last ? 340.0f : 250.0f;
	std::string DiceLabel = Stage == CreatorStage::Look ? "RANDOM LOOK" : "RANDOMIZE";
	float DiceW = 236.0f;
	if (BackW + NextW + 32.0f + (Dice ? DiceW : 0.0f) > W)
	{
		BackW = 150.0f;
		NextW = Last ? 300.0f : 210.0f;
		DiceW = W - (BackW + NextW + 32.0f);
		if (DiceW < LabelW(DiceLabel))
		{
			DiceLabel = "RANDOM";
		}
		if (DiceW < LabelW(DiceLabel))
		{
			DiceLabel.clear();
			DiceW = 60.0f;
		}
	}
	if (Button({X, By, BackW, 60.0f}, "BACK", false, false))
	{
		CreatorBack(Now);
		return;
	}
	if (Button({X + BackW + 16.0f, By, NextW, 60.0f}, Last ? "BEGIN NIGHT ONE" : "NEXT", true, true))
	{
		CreatorNext(Now);
		return;
	}
	if (Dice)
	{
		const Rect Dr{X + W - DiceW, By, DiceW, 60.0f};
		const bool Hit = Button(Dr, DiceLabel, false, false);
		if (DiceLabel.empty())
		{
			CreatorDie(*C, Dr.X + Dr.W * 0.5f, Dr.Y + Dr.H * 0.5f, F(MenuInk, Interactive && Inside(Dr, Ptr.X, Ptr.Y) ? 1.0f : 0.8f));
		}
		if (Hit)
		{
			CreatorRandomize(Stage == CreatorStage::Look, Now);
		}
		// The identity's refusal shares the line above the buttons: it wins while it's up.
		const bool Refused = Stage == CreatorStage::Identity && !Refusal.empty() && Now - RefusalAt < 3.0;
		if (Now - UndoAt < 2.0 && !Refused)
		{
			const float Ua = 1.0f - Ease((Now - UndoAt - 1.4) / 0.6);
			const std::string Note = Draft.FirstName.empty() ? std::string("Undone.") : "Undone: back to " + Draft.FullName() + ".";
			C->Text(Note, X + W, By - 16.0f, Ts(15.0f, 600, F(MenuTeal, Ua), Align::Right, Baseline::Alphabetic, false, W * 0.6f));
		}
	}
	// The hints, least needed first: on a narrow screen those go, then the footer, before anything overlaps.
	const bool Typing = FocusedText() != nullptr;
	std::vector<std::pair<std::string, std::string>> Pairs;
	switch (Stage)
	{
	case CreatorStage::Identity:
		Pairs.push_back({"TAB/DPAD", "NEXT FIELD"});
		Pairs.push_back({"LEFT|RIGHT/DPAD", Gamepad && Field <= 2 ? "SUGGEST" : "AGE \xC2\xB7 COUNTRY"});
		if (!Typing || Gamepad)
		{
			Pairs.push_back({"R/Y", "RANDOMIZE"});
		}
		break;
	case CreatorStage::Background: Pairs.push_back({"LEFT|RIGHT/DPAD", "CHOOSE"}); break;
	case CreatorStage::Look:
		Pairs.push_back({"Q|E/LB|RB", "SECTION"});
		Pairs.push_back({"UP|DOWN/DPAD", "SELECT"});
		Pairs.push_back({"LEFT|RIGHT/DPAD", "CHANGE"});
		Pairs.push_back({"R/Y", "RANDOM"});
		break;
	default: break;
	}
	if (CreatorCanUndo())
	{
		Pairs.push_back({"CTRL|Z/X", "UNDO"});
	}
	Pairs.push_back({"ENTER/A", Last ? "BEGIN" : "CONTINUE"});
	Pairs.push_back({"ESC/B", "BACK"});
	const std::string FooterText = "SHORT STACK " + Info.Version + " \xC2\xB7 PROTOTYPE BUILD";
	const float FooterW = TrackedWidth(*C, FooterText, 12.0f, 700, 2.4f) + 40.0f;
	while (Pairs.size() > 2 && HintsWidth(Pairs) > ViewW - MenuMargin * 2.0f)
	{
		Pairs.erase(Pairs.begin());
	}
	const float End = Hints(Pairs);
	if (End + FooterW <= ViewW - MenuMargin)
	{
		Footer();
	}
}
} // namespace ui
} // namespace ss
