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

float Fc(int V)
{
	return static_cast<float>(V);
}

/** The portrait card's width, and the content column's beside it (capped on ultrawide screens). */
float CardWidth(float ViewW)
{
	return std::clamp(ViewW * 0.31f, 460.0f, 640.0f);
}

float ContentWidth(float ViewW)
{
	return std::min(1060.0f, ViewW - frontend_detail::MenuMargin * 2.0f - CardWidth(ViewW) - 64.0f);
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

/** A vector icon for each background, drawn in its color. */
void BackgroundIcon(Canvas& C, hero::Background B, float Cx, float Cy, float S, const Color& Col)
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
			const float X = Cx - S * 0.12f + Fc(I) * S * 0.13f;
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
			C.StrokeRoundRect(Rc, S * 0.05f, I == 0 ? WithAlpha(Col, 0.6f) : Col, W);
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
			const float T = Fc(I) / 24.0f * 2.0f - 1.0f;
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
			const float X = Cx + Fc(I) * S * 0.32f;
			C.StrokePolyline({{X, Cy - S * 0.2f}, {X, Cy + S * 0.3f}}, false, Col, W * 1.2f, true);
			C.FillCircle(X, Cy - S * 0.24f, S * 0.06f, Paint(Col));
			C.StrokePolyline({{X - S * 0.1f, Cy + S * 0.31f}, {X + S * 0.1f, Cy + S * 0.31f}}, false, Col, W, true);
		}
		std::vector<Vec2> Rope;
		for (int I = 0; I <= 12; ++I)
		{
			const float T = Fc(I) / 12.0f;
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
		C.FillRoundRect({Cx - S * 0.11f, Cy - S * 0.21f, S * 0.22f, S * 0.16f}, S * 0.02f, Paint(WithAlpha(Col, 0.45f)));
		for (int R = 0; R < 3; ++R)
		{
			for (int K = 0; K < 3; ++K)
			{
				C.FillCircle(Cx + Fc(K - 1) * S * 0.08f, Cy + S * 0.04f + Fc(R) * S * 0.08f, S * 0.022f, Paint(Col));
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
			C.StrokePolyline({{Cx + Fc(I) * S * 0.2f, Cy - S * 0.16f}, {Cx + Fc(I) * S * 0.2f, Cy + S * 0.3f}}, false, Col, W * 0.8f);
		}
		break;
	}
	}
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
} // namespace creator_detail

using namespace frontend_detail;
using namespace creator_detail;

// ------------------------------------------------------------------ flow

void FrontEnd::CreatorOpen(double Now)
{
	// A new face each time the creator opens: somewhere to start, not a template to fill in.
	Rng R("creator:" + std::to_string(Creations++));
	Draft = hero::Random(R);
	Stage = CreatorStage::Identity;
	Field = 0;
	LookTab = 0;
	StageAt = Now;
	Refusal.clear();
	RefusalAt = -10.0;
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

void FrontEnd::CreatorNext(double Now)
{
	if (Stage == CreatorStage::Identity)
	{
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
	const std::string Why = CreatorProblem();
	if (!Why.empty())
	{
		CreatorGo(CreatorStage::Identity, Now);
		CreatorRefuse(Why, Now);
		return;
	}
	Sound(SoundId::ChipStack, 0.8);
	Draft.Created = true;
	Close(Now);
	Hooks.NewCareer(ScreenName, Draft);
}

void FrontEnd::CreatorRandomize(bool LookOnly, double Now)
{
	Rng R("creator:roll:" + std::to_string(Rolls++) + ":" + std::to_string(Creations));
	const hero::Character Fresh = hero::Random(R);
	if (LookOnly)
	{
		Draft.Appearance = Fresh.Appearance;
	}
	else
	{
		const hero::Background Kept = Draft.Story;
		Draft = Fresh;
		Draft.Story = Kept;
	}
	SelAt = Now;
	Sound(SoundId::ChipStack, 0.5);
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
			int I = 0;
			for (size_t K = 0; K < All.size(); ++K)
			{
				I = Draft.Country == All[K].Code ? static_cast<int>(K) : I;
			}
			const int N = static_cast<int>(All.size());
			I = Wrap ? (I + Delta + N) % N : std::clamp(I + Delta, 0, N - 1);
			Draft.Country = All[static_cast<size_t>(I)].Code;
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
	if (Name == "Backspace")
	{
		if (std::string* Text = FocusedText())
		{
			if (!Text->empty())
			{
				PopGlyph(*Text);
				Sound(SoundId::Click, 0.2);
			}
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
			SelAt = Now;
			Sound(SoundId::Click, 0.3);
		}
		return;
	}
	if (Stage == CreatorStage::Identity && Field == 4 && (Name == "Up" || Name == "Down"))
	{
		// Up and down move through the country grid a row at a time, and out of it at the top.
		const std::vector<hero::CountryInfo>& All = hero::Countries();
		int I = 0;
		for (size_t K = 0; K < All.size(); ++K)
		{
			I = Draft.Country == All[K].Code ? static_cast<int>(K) : I;
		}
		if (Name == "Up" && I < 7)
		{
			Field = 3;
		}
		else
		{
			I = std::clamp(I + (Name == "Up" ? -7 : 7), 0, static_cast<int>(All.size()) - 1);
			Draft.Country = All[static_cast<size_t>(I)].Code;
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
		CreatorChange(Name == "Left" ? -1 : 1, Stage == CreatorStage::Look);
	}
	else if (Stage == CreatorStage::Look && (Name == "Q" || Name == "E" || Name == "TabPrev" || Name == "TabNext"))
	{
		LookTab = (LookTab + (Name == "Q" || Name == "TabPrev" ? 3 : 1)) % 4;
		Field = 0;
		SelAt = Now;
		Sound(SoundId::Click, 0.4);
	}
	else if ((Name == "R" && !Typing) || Name == "Reset")
	{
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
		// Names: letters (with accents), spaces, apostrophes and hyphens; no leading space.
		const bool Letter = (Codepoint >= 'a' && Codepoint <= 'z') || (Codepoint >= 'A' && Codepoint <= 'Z') || (Codepoint >= 0xC0 && Codepoint <= 0x24F && Codepoint != 0xD7 && Codepoint != 0xF7);
		Ok = Letter || (!Text->empty() && (Codepoint == ' ' || Codepoint == '\'' || Codepoint == '-'));
	}
	const std::string Add = Utf8(Codepoint);
	if (!Ok || Add.empty() || Text->size() + Add.size() > 16)
	{
		return;
	}
	*Text += Add;
	Sound(SoundId::Click, 0.2);
	(void)Now;
}

// ------------------------------------------------------------------ drawing

bool FrontEnd::TextBox(const Rect& R, const std::string& Label, const std::string& Value, int Index, double Now)
{
	const bool Focus = Field == Index;
	const bool Over = Interactive && Inside(R, Ptr.X, Ptr.Y);
	TrackedText(*C, Label, R.X, R.Y - 14.0f, 13.0f, 700, F(Focus ? MenuInk : MenuMuted), 3.2f);
	C->FillRect(R, Paint(F(MenuInk, Focus ? 0.08f : Over ? 0.06f : 0.04f)));
	const bool Bad = Focus && Now - RefusalAt < 1.6;
	C->FillRect({R.X, R.Y + R.H - 3.0f, R.W, 3.0f}, Paint(F(Bad ? MenuWarn : Focus ? MenuNeon : MenuInk, Focus ? 1.0f : 0.18f)));
	const float Tw = C->Text(Value.empty() ? std::string() : Value, R.X + 22.0f, R.Y + R.H * 0.5f + 11.0f, Ts(30.0f, 700, F(MenuInk), Align::Left, Baseline::Alphabetic, false, R.W - 90.0f));
	if (Value.empty() && !Focus)
	{
		C->Text("Type here", R.X + 22.0f, R.Y + R.H * 0.5f + 9.0f, Ts(24.0f, 400, F(MenuDim)));
	}
	if (Focus && std::fmod(Now, 1.0) < 0.55)
	{
		C->FillRect({R.X + 25.0f + Tw, R.Y + 17.0f, 3.0f, R.H - 34.0f}, Paint(F(MenuNeon)));
	}
	TrackedText(*C, std::to_string(Value.size()) + "/16", R.X + R.W - 18.0f, R.Y + R.H * 0.5f + 5.0f, 11.0f, 700, F(MenuDim), 2.0f, Align::Right);
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
	// The stepper: done steps teal, this one neon, the rest waiting.
	const float W = ContentWidth(ViewW);
	const float Seg = W / 4.0f;
	for (int I = 0; I < 4; ++I)
	{
		const float Sx = X + Fc(I) * Seg;
		const Rect Hit{Sx, 252.0f, Seg - 12.0f, 44.0f};
		const bool Done = I < S;
		const bool Here = I == S;
		const Color Col = Here ? MenuNeon : Done ? MenuTeal : MenuDim;
		TrackedText(*C, (I < 9 ? "0" : "") + std::to_string(I + 1), Sx, 276.0f, 13.0f, 900, F(Col), 2.0f);
		TrackedText(*C, StepNames[I], Sx + 34.0f, 276.0f, 13.0f, 700, F(Here ? MenuInk : Done ? MenuMuted : MenuDim), 3.0f);
		C->FillRect({Sx, 290.0f, Seg - 12.0f, 3.0f}, Paint(F(Col, Here ? 1.0f : Done ? 0.7f : 0.25f)));
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
	// The booth: a magenta wash from the street side, the rim light behind on the right, rain on the glass.
	const Color RimCol = Stage == CreatorStage::Identity ? MenuTeal : Hex(Bi.Color);
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
	const std::string Where = Ci ? std::string(Ci->City) + ", " + Ci->Name : std::string("Somewhere");
	Lx += TrackedText(*C, std::to_string(Draft.Age) + " \xC2\xB7 " + Where, Lx, R.Y + R.H - 74.0f, 14.0f, 700, F(MenuMuted), 2.0f);
	// Background pill and the screen name.
	const std::string Pill = Bi.Name;
	const float Pw = TrackedWidth(*C, Pill, 12.0f, 800, 2.4f) + 54.0f;
	const Rect Pr{R.X + 32.0f, R.Y + R.H - 54.0f, Pw, 30.0f};
	C->FillRoundRect(Pr, 15.0f, Paint(F(Hex(Bi.Color), 0.14f)));
	C->StrokeRoundRect(Pr, 15.0f, F(Hex(Bi.Color), 0.7f), 1.2f);
	BackgroundIcon(*C, Draft.Story, Pr.X + 17.0f, Pr.Y + 15.0f, 22.0f, F(Hex(Bi.Color)));
	TrackedText(*C, Pill, Pr.X + 34.0f, Pr.Y + 20.0f, 12.0f, 800, F(Hex(Bi.Color)), 2.4f);
	TrackedText(*C, "@" + (ScreenName.empty() ? std::string("...") : ScreenName), R.X + R.W - 32.0f, Pr.Y + 20.0f, 13.0f, 700, F(MenuMuted), 1.6f, Align::Right);
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
	const float W = ContentWidth(ViewW);
	const float Half = (W - 24.0f) * 0.5f;
	const double Err = Now - RefusalAt;
	const float Shake = Err < 0.4 ? static_cast<float>(std::sin(Err * 70.0) * 8.0 * (1.0 - Err / 0.4)) : 0.0f;
	auto Sh = [&](int Index) { return Field == Index ? Shake : 0.0f; };
	TextBox({X + Sh(0), 356.0f, Half, 70.0f}, "FIRST NAME", Draft.FirstName, 0, Now);
	TextBox({X + Half + 24.0f + Sh(1), 356.0f, Half, 70.0f}, "LAST NAME", Draft.LastName, 1, Now);
	TextBox({X + Sh(2), 484.0f, Half, 70.0f}, "SCREEN NAME", ScreenName, 2, Now);
	C->Text("What RiverLine and the tables call you.", X, 584.0f, Ts(15.0f, 400, F(MenuMuted)));

	// Age: minus, the number, plus, and where it sits between 18 and 65.
	const Rect Ar{X + Half + 24.0f, 484.0f, Half, 70.0f};
	const bool AgeFocus = Field == 3;
	TrackedText(*C, "AGE", Ar.X, Ar.Y - 14.0f, 13.0f, 700, F(AgeFocus ? MenuInk : MenuMuted), 3.2f);
	C->FillRect(Ar, Paint(F(MenuInk, AgeFocus ? 0.08f : 0.04f)));
	C->FillRect({Ar.X, Ar.Y + Ar.H - 3.0f, Ar.W, 3.0f}, Paint(F(AgeFocus ? MenuNeon : MenuInk, AgeFocus ? 1.0f : 0.18f)));
	const Rect Minus{Ar.X, Ar.Y, 70.0f, Ar.H};
	const Rect Plus{Ar.X + Ar.W - 70.0f, Ar.Y, 70.0f, Ar.H};
	Chevron(*C, Minus.X + 35.0f, Ar.Y + 35.0f, 9.0f, false, F(MenuInk, Draft.Age > hero::MinAge ? 0.9f : 0.25f));
	Chevron(*C, Plus.X + 35.0f, Ar.Y + 35.0f, 9.0f, true, F(MenuInk, Draft.Age < hero::MaxAge ? 0.9f : 0.25f));
	C->Text(std::to_string(Draft.Age), Ar.X + Ar.W * 0.5f, Ar.Y + 47.0f, Ts(34.0f, 900, F(MenuInk), Align::Center));
	const float Tk = Fc(Draft.Age - hero::MinAge) / Fc(hero::MaxAge - hero::MinAge);
	C->FillRect({Ar.X, Ar.Y + Ar.H + 12.0f, Ar.W, 2.0f}, Paint(F(MenuInk, 0.12f)));
	C->FillRect({Ar.X, Ar.Y + Ar.H + 12.0f, Ar.W * Tk, 2.0f}, Paint(F(MenuNeon, 0.8f)));
	C->FillCircle(Ar.X + Ar.W * Tk, Ar.Y + Ar.H + 13.0f, 5.0f, Paint(F(MenuInk)));
	TrackedText(*C, std::to_string(hero::MinAge), Ar.X, Ar.Y + Ar.H + 34.0f, 11.0f, 700, F(MenuDim), 2.0f);
	TrackedText(*C, std::to_string(hero::MaxAge), Ar.X + Ar.W, Ar.Y + Ar.H + 34.0f, 11.0f, 700, F(MenuDim), 2.0f, Align::Right);
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
		C->Text(std::string(Home->City) + ", " + Home->Name, X + Lw + 18.0f, Gy - 15.0f, Ts(16.0f, 600, F(MenuInk, 0.85f)));
	}
	const std::vector<hero::CountryInfo>& All = hero::Countries();
	const float Gap = 8.0f;
	const float Tw = (W - Gap * 6.0f) / 7.0f;
	const float Th = 52.0f;
	for (size_t I = 0; I < All.size(); ++I)
	{
		const hero::CountryInfo& Ci = All[I];
		const Rect T{X + Fc(static_cast<int>(I % 7)) * (Tw + Gap), Gy + Fc(static_cast<int>(I / 7)) * (Th + Gap), Tw, Th};
		const bool Picked = Draft.Country == Ci.Code;
		const bool Over = Interactive && Inside(T, Ptr.X, Ptr.Y);
		C->FillRect(T, Paint(F(Picked ? MenuNeon : MenuInk, Picked ? 0.14f : Over ? 0.08f : 0.035f)));
		if (Picked)
		{
			C->StrokeRoundRect({T.X + 0.75f, T.Y + 0.75f, T.W - 1.5f, T.H - 1.5f}, 1.0f, F(MenuNeon, Field == 4 ? 1.0f : 0.6f), 1.5f);
		}
		const float Fa = C->GetAlpha();
		C->SetAlpha(Fa * Fade);
		rlnet_detail::NetFlag(*C, Ci.Code, T.X + 12.0f, T.Y + 16.0f, 30.0f, 20.0f);
		C->SetAlpha(Fa);
		C->Text(Ci.Code, T.X + 52.0f, T.Y + 26.0f, Ts(15.0f, 900, F(Picked ? MenuInk : MenuMuted)));
		C->Text(Ci.Name, T.X + 52.0f, T.Y + 42.0f, Ts(11.0f, 500, F(MenuDim), Align::Left, Baseline::Alphabetic, false, T.W - 58.0f));
		if (Released(T))
		{
			Draft.Country = Ci.Code;
			Field = 4;
			SelAt = Now;
			Sound(SoundId::Click, 0.3);
		}
	}
	if (Err < 3.0 && !Refusal.empty())
	{
		C->Text(Refusal, X, 892.0f, Ts(16.0f, 600, F(MenuWarn)));
	}
}

void FrontEnd::BackgroundStep(double Now)
{
	const float X = MenuMargin;
	const float W = ContentWidth(ViewW);
	const float Cw = (W - 32.0f) / 3.0f;
	const float Ch = 172.0f;
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
		C->FillCircle(R.X + 52.0f, R.Y + 52.0f, 30.0f, Paint(F(Bc, 0.12f * Ci)));
		BackgroundIcon(*C, B.Id, R.X + 52.0f, R.Y + 52.0f, 44.0f, F(Bc, Ci));
		C->Text(B.Name, R.X + 22.0f, R.Y + 118.0f, Ts(23.0f, 800, F(MenuInk, Ci), Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
		float Ty = R.Y + 122.0f;
		for (const std::string& L : WrapText(*C, B.Tagline, R.W - 44.0f, 14.0f, 400))
		{
			Ty += 20.0f;
			C->Text(L, R.X + 22.0f, Ty, Ts(14.0f, 400, F(MenuMuted, Ci)));
		}
		if (Picked)
		{
			TrackedText(*C, "CHOSEN", R.X + R.W - 18.0f, R.Y + 30.0f, 11.0f, 800, F(Bc, Ci), 2.6f, Align::Right);
		}
		if (Released(R))
		{
			Draft.Story = B.Id;
			SelAt = Now;
			Sound(SoundId::Click, 0.35);
		}
	}
	// The story and what it gives.
	const hero::BackgroundInfo& B = hero::InfoOf(Draft.Story);
	const Color Bc = Hex(B.Color);
	const float A = Ease((Now - SelAt) / 0.3);
	const float Py = 724.0f + (1.0f - A) * 8.0f;
	const float StoryW = W * 0.58f;
	float Y = Py;
	for (const std::string& L : WrapText(*C, B.Story, StoryW, 18.0f, 300))
	{
		Y += 27.0f;
		C->Text(L, X, Y, Ts(18.0f, 300, F(MenuInk, 0.9f * A)));
	}
	const Rect Perk{X + StoryW + 32.0f, Py + 4.0f, W - StoryW - 32.0f, 156.0f};
	C->FillRect(Perk, Paint(F(Bc, 0.08f * A)));
	C->FillRect({Perk.X, Perk.Y, 3.0f, Perk.H}, Paint(F(Bc, A)));
	TrackedText(*C, "PERK", Perk.X + 22.0f, Perk.Y + 32.0f, 12.0f, 800, F(Bc, A), 3.0f);
	C->Text(B.PerkName, Perk.X + 22.0f, Perk.Y + 64.0f, Ts(24.0f, 900, F(MenuInk, A)));
	float Yp = Perk.Y + 68.0f;
	for (const std::string& L : WrapText(*C, B.PerkText, Perk.W - 44.0f, 15.0f, 400))
	{
		Yp += 22.0f;
		C->Text(L, Perk.X + 22.0f, Yp, Ts(15.0f, 400, F(MenuMuted, A)));
	}
}

void FrontEnd::LookStep(double Now)
{
	const float X = MenuMargin;
	const float W = ContentWidth(ViewW);
	// Tabs.
	float Tx = X;
	const float TabY = 352.0f;
	Tx += Glyph(*C, Gamepad ? "LB" : "Q", Tx, TabY + 3.0f, Fade) + 26.0f;
	for (int I = 0; I < 4; ++I)
	{
		const bool On = I == LookTab;
		const float Lw = TrackedWidth(*C, LookTabs[I], 17.0f, 800, 4.0f);
		const Rect Hit{Tx - 10.0f, TabY - 26.0f, Lw + 20.0f, 40.0f};
		TrackedText(*C, LookTabs[I], Tx, TabY, 17.0f, 800, F(On ? MenuInk : MenuMuted, On ? 1.0f : 0.8f), 4.0f);
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

	// Rows.
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
		C->FillRect(R, Paint(F(MenuInk, (Focus ? 0.07f : Over ? 0.05f : 0.03f) * Ri)));
		if (Focus)
		{
			C->FillRect({R.X, R.Y, 3.0f, R.H}, Paint(F(MenuNeon, Ri)));
		}
		const int V = Draft.Appearance.Get(S);
		TrackedText(*C, hero::SlotName(S), R.X + 24.0f, R.Y + (IsColorSlot(S) ? 32.0f : 43.0f), 13.0f, 800, F(Focus ? MenuInk : MenuMuted, Ri), 3.0f);
		if (IsColorSlot(S))
		{
			C->Text(hero::OptionName(S, V), R.X + 24.0f, R.Y + 56.0f, Ts(15.0f, 600, F(MenuInk, 0.8f * Ri)));
			const int N = hero::OptionCount(S);
			const float Sw = 40.0f;
			const float Right = R.X + R.W - 34.0f;
			for (int K = 0; K < N; ++K)
			{
				const float Cx = Right - Fc(N - 1 - K) * Sw;
				const float Cy = R.Y + R.H * 0.5f;
				if (K == V)
				{
					C->StrokeEllipse(Cx, Cy, 19.0f, 19.0f, F(Focus ? MenuNeon : MenuInk, Ri), 2.0f);
				}
				C->FillCircle(Cx, Cy, 14.0f, Paint::Linear({Cx - 14.0f, Cy - 14.0f}, {Cx + 14.0f, Cy + 14.0f}, F(Mix(Hex(SwatchOf(S, K)), Hex(0xffffff), 0.12f), Ri), F(Mix(Hex(SwatchOf(S, K)), Hex(0x000000), 0.2f), Ri)));
				C->StrokeEllipse(Cx, Cy, 14.0f, 14.0f, F(Hex(0xffffff), 0.12f * Ri), 1.0f);
				if (Released({Cx - 18.0f, Cy - 18.0f, 36.0f, 36.0f}))
				{
					Field = Index;
					Draft.Appearance.Set(S, K);
					SelAt = Now;
					Sound(SoundId::Click, 0.3);
				}
			}
		}
		else if (S == hero::Slot::Height)
		{
			const float Sx = R.X + R.W - 520.0f;
			const float Sw = 300.0f;
			const float T = Fc(V - hero::MinHeight) / Fc(hero::MaxHeight - hero::MinHeight);
			C->FillRect({Sx, R.Y + 35.0f, Sw, 3.0f}, Paint(F(MenuInk, 0.15f * Ri)));
			C->FillRect({Sx, R.Y + 35.0f, Sw * T, 3.0f}, Paint(F(MenuNeon, Ri)));
			C->FillCircle(Sx + Sw * T, R.Y + 36.5f, 8.0f, Paint(F(MenuInk, Ri)));
			C->Text(hero::OptionName(S, V), R.X + R.W - 24.0f, R.Y + 44.0f, Ts(18.0f, 700, F(MenuInk, Ri), Align::Right));
			const Rect Track{Sx - 10.0f, R.Y, Sw + 20.0f, R.H};
			if (Interactive && Ptr.Down && Inside(Track, PressX, PressY))
			{
				Field = Index;
				const float Pick = std::clamp((Ptr.X - Sx) / Sw, 0.0f, 1.0f);
				Draft.Appearance.Set(S, hero::MinHeight + static_cast<int>(std::lround(Pick * Fc(hero::MaxHeight - hero::MinHeight))));
			}
		}
		else
		{
			const float Cl = R.X + R.W - 500.0f;
			const float Cr = R.X + R.W - 40.0f;
			const float Mid = (Cl + Cr) * 0.5f;
			Chevron(*C, Cl, R.Y + 34.0f, 9.0f, false, F(MenuInk, (Focus ? 0.95f : 0.5f) * Ri));
			Chevron(*C, Cr, R.Y + 34.0f, 9.0f, true, F(MenuInk, (Focus ? 0.95f : 0.5f) * Ri));
			C->Text(hero::OptionName(S, V), Mid, R.Y + 41.0f, Ts(20.0f, 700, F(MenuInk, Ri), Align::Center));
			const int N = hero::OptionCount(S);
			const float Pip = 10.0f;
			const float P0 = Mid - Fc(N - 1) * Pip * 0.5f;
			for (int K = 0; K < N; ++K)
			{
				C->FillCircle(P0 + Fc(K) * Pip, R.Y + 58.0f, K == V ? 2.6f : 1.8f, Paint(F(K == V ? MenuNeon : MenuInk, (K == V ? 1.0f : 0.22f) * Ri)));
			}
			const Rect Lh{Cl - 30.0f, R.Y, 60.0f, R.H};
			const Rect Rh2{Cr - 30.0f, R.Y, 60.0f, R.H};
			if (Released(Lh) || Released(Rh2))
			{
				Field = Index;
				CreatorChange(Released(Rh2) ? 1 : -1, true);
			}
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
		TrackedText(*C, "THE DOOR \xC2\xB7 203 CM", Cx + 170.0f, Base - DoorH - 10.0f, 11.0f, 700, F(MenuDim), 2.4f);
		for (int Cm = 150; Cm <= 200; Cm += 10)
		{
			const float Y = Base - Fc(Cm) * Px;
			C->FillRect({Cx - 96.0f, Y, Cm % 50 == 0 ? 16.0f : 9.0f, 1.0f}, Paint(F(MenuInk, 0.3f)));
			if (Cm % 50 == 0)
			{
				C->Text(std::to_string(Cm), Cx - 76.0f, Y + 4.0f, Ts(11.0f, 700, F(MenuDim)));
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
	C->GlowRoundRect({R.X, R.Y + 10.0f, R.W, R.H}, 22.0f, F(Hex(0x000000), 0.6f * In), 36.0f);
	C->FillRoundRect(R, 22.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, F(Hex(0x241635)), F(Hex(0x0b1020))));
	// Foil: a band of light sweeping across.
	const float Sweep = static_cast<float>(std::fmod(Now * 0.25, 1.6)) - 0.3f;
	C->PushClip({R.X + 12.0f, R.Y + 12.0f, R.W - 24.0f, R.H - 24.0f});
	const float Bx = R.X + Sweep * R.W;
	C->FillPolygon({{Bx, R.Y}, {Bx + 120.0f, R.Y}, {Bx - 60.0f, R.Y + R.H}, {Bx - 180.0f, R.Y + R.H}},
		Paint::Linear({Bx - 180.0f, 0.0f}, {Bx + 120.0f, 0.0f}, F(MenuNeon, 0.0f), F(MenuTeal, 0.1f)));
	C->PopClip();
	C->StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 22.0f, F(Hex(0xf2c14e), 0.35f), 1.5f);
	// Header.
	C->FillCircle(R.X + 44.0f, R.Y + 44.0f, 17.0f, Paint::Linear({R.X + 27.0f, R.Y + 27.0f}, {R.X + 61.0f, R.Y + 61.0f}, F(Hex(0xf6d58a)), F(Hex(0xb8892f))));
	C->Text("R", R.X + 44.0f, R.Y + 51.0f, Ts(20.0f, 900, F(Hex(0x2a1a06)), Align::Center));
	TrackedText(*C, "RIVERSIDE", R.X + 72.0f, R.Y + 44.0f, 20.0f, 900, F(MenuInk), 5.0f);
	TrackedText(*C, "CASINO \xC2\xB7 PLAYERS CLUB", R.X + 72.0f, R.Y + 62.0f, 11.0f, 700, F(MenuGold, 0.85f), 3.0f);
	const float Pw = TrackedWidth(*C, "NEW MEMBER", 11.0f, 800, 2.4f) + 28.0f;
	C->FillRoundRect({R.X + R.W - 28.0f - Pw, R.Y + 30.0f, Pw, 26.0f}, 13.0f, Paint(F(MenuGold, 0.16f)));
	TrackedText(*C, "NEW MEMBER", R.X + R.W - 28.0f - Pw * 0.5f, R.Y + 48.0f, 11.0f, 800, F(MenuGold), 2.4f, Align::Center);
	// Photo.
	const Rect Ph{R.X + 28.0f, R.Y + 86.0f, 196.0f, 222.0f};
	C->FillRoundRect(Ph, 12.0f, Paint::Linear({0.0f, Ph.Y}, {0.0f, Ph.Y + Ph.H}, F(Hex(0x2a3550)), F(Hex(0x101626))));
	C->PushClip(Ph);
	PortraitLight Light;
	Light.Rim = Hex(Bi.Color);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * Fade);
	DrawPortrait(*C, Draft, Ph.X + Ph.W * 0.5f, Ph.Y + 100.0f, 0.6f, 0.0, Light);
	C->SetAlpha(A0);
	C->PopClip();
	C->StrokeRoundRect(Ph, 12.0f, F(MenuInk, 0.15f), 1.0f);
	// Fields.
	const float Fx = R.X + 252.0f;
	auto FieldText = [&](const std::string& Label, const std::string& Value, float Fx0, float Fy, float Size) {
		TrackedText(*C, Label, Fx0, Fy, 10.0f, 800, F(MenuMuted), 2.6f);
		C->Text(Value, Fx0, Fy + Size + 6.0f, Ts(Size, 800, F(MenuInk), Align::Left, Baseline::Alphabetic, false, R.X + R.W - 28.0f - Fx0));
	};
	FieldText("NAME", Draft.FullName(), Fx, R.Y + 102.0f, 26.0f);
	FieldText("AGE", std::to_string(Draft.Age), Fx, R.Y + 170.0f, 20.0f);
	FieldText("HEIGHT", HeightLabel(Draft.Appearance.Height) + " \xC2\xB7 " + std::to_string(Draft.Appearance.Height) + " cm", Fx + 110.0f, R.Y + 170.0f, 20.0f);
	const hero::CountryInfo* Ci = hero::FindCountry(Draft.Country);
	TrackedText(*C, "FROM", Fx, R.Y + 230.0f, 10.0f, 800, F(MenuMuted), 2.6f);
	if (Ci)
	{
		const float Fa = C->GetAlpha();
		C->SetAlpha(Fa * Fade);
		rlnet_detail::NetFlag(*C, Ci->Code, Fx, R.Y + 242.0f, 27.0f, 18.0f);
		C->SetAlpha(Fa);
		C->Text(std::string(Ci->City) + ", " + Ci->Name, Fx + 38.0f, R.Y + 257.0f, Ts(18.0f, 800, F(MenuInk), Align::Left, Baseline::Alphabetic, false, R.X + R.W - Fx - 66.0f));
	}
	FieldText("PLAYS AS", "@" + ScreenName, Fx, R.Y + 290.0f, 18.0f);
	// Footer: the member number and a stripe of barcode.
	const uint32_t H = NameHash(Draft.FullName() + ScreenName);
	char Num[32];
	std::snprintf(Num, sizeof(Num), "No. %04u %04u %04u", (H >> 20) % 10000u, (H >> 8) % 10000u, H % 10000u);
	C->Text(Num, R.X + 28.0f, R.Y + R.H - 30.0f, Ts(15.0f, 700, F(MenuInk, 0.75f), Align::Left, Baseline::Alphabetic, true));
	TrackedText(*C, "MEMBER SINCE OCT 2026", R.X + 28.0f, R.Y + R.H - 52.0f, 10.0f, 800, F(MenuMuted), 2.6f);
	float Bx2 = R.X + R.W - 200.0f;
	for (int I = 0; I < 40 && Bx2 < R.X + R.W - 28.0f; ++I)
	{
		const float Bw = 1.0f + Fc(static_cast<int>((H >> (I % 29)) & 3u));
		C->FillRect({Bx2, R.Y + R.H - 58.0f, Bw, 34.0f}, Paint(F(MenuInk, 0.7f)));
		Bx2 += Bw + 2.0f;
	}
}

void FrontEnd::ReviewStep(double Now)
{
	const float X = MenuMargin;
	const float W = ContentWidth(ViewW);
	const float Cw = std::min(620.0f, W);
	PlayersCard({X, 336.0f, Cw, 392.0f}, Now);
	const hero::BackgroundInfo& B = hero::InfoOf(Draft.Story);
	const Color Bc = Hex(B.Color);
	if (W - Cw >= 280.0f)
	{
		// The perk beside the card.
		const Rect Perk{X + Cw + 28.0f, 336.0f, W - Cw - 28.0f, 392.0f};
		C->FillRect(Perk, Paint(F(Bc, 0.07f)));
		C->FillRect({Perk.X, Perk.Y, 3.0f, Perk.H}, Paint(F(Bc)));
		C->FillCircle(Perk.X + 56.0f, Perk.Y + 64.0f, 30.0f, Paint(F(Bc, 0.14f)));
		BackgroundIcon(*C, B.Id, Perk.X + 56.0f, Perk.Y + 64.0f, 44.0f, F(Bc));
		TrackedText(*C, std::string(B.Name), Perk.X + 24.0f, Perk.Y + 134.0f, 13.0f, 800, F(Bc), 3.0f);
		TrackedText(*C, "PERK", Perk.X + 24.0f, Perk.Y + 182.0f, 11.0f, 800, F(MenuMuted), 3.0f);
		C->Text(B.PerkName, Perk.X + 24.0f, Perk.Y + 214.0f, Ts(24.0f, 900, F(MenuInk)));
		float Yp = Perk.Y + 218.0f;
		for (const std::string& L : WrapText(*C, B.PerkText, Perk.W - 48.0f, 15.0f, 400))
		{
			Yp += 22.0f;
			C->Text(L, Perk.X + 24.0f, Yp, Ts(15.0f, 400, F(MenuMuted)));
		}
		C->FillRect({Perk.X + 24.0f, Perk.Y + Perk.H - 74.0f, 32.0f, 2.0f}, Paint(F(Bc, 0.6f)));
		float Yt = Perk.Y + Perk.H - 74.0f;
		for (const std::string& L : WrapText(*C, "\xE2\x80\x9C" + std::string(B.Tagline) + "\xE2\x80\x9D", Perk.W - 48.0f, 16.0f, 300))
		{
			Yt += 24.0f;
			C->Text(L, Perk.X + 24.0f, Yt, Ts(16.0f, 300, F(MenuInk, 0.75f)));
		}
	}
	float Y = 752.0f;
	for (const std::string& L : WrapText(*C, hero::Bio(Draft), W, 19.0f, 300))
	{
		Y += 28.0f;
		C->Text(L, X, Y, Ts(19.0f, 300, F(MenuInk, 0.9f)));
	}
	if (Info.HasSave)
	{
		C->Text("Starting over replaces your current career.", X, 892.0f, Ts(16.0f, 600, F(MenuWarn)));
	}
}

void FrontEnd::NewGamePage(double Now)
{
	const float Card = CardWidth(ViewW);
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
	// Buttons.
	const float X = MenuMargin;
	const float By = 908.0f;
	const float W = ContentWidth(ViewW);
	const bool Last = Stage == CreatorStage::Review;
	if (Button({X, By, 180.0f, 60.0f}, "BACK", false, false))
	{
		CreatorBack(Now);
		return;
	}
	if (Button({X + 196.0f, By, Last ? 340.0f : 250.0f, 60.0f}, Last ? "BEGIN NIGHT ONE" : "NEXT", true, true))
	{
		CreatorNext(Now);
		return;
	}
	if (Stage == CreatorStage::Identity || Stage == CreatorStage::Look)
	{
		if (Button({X + W - 236.0f, By, 236.0f, 60.0f}, Stage == CreatorStage::Look ? "RANDOM LOOK" : "RANDOMIZE", false, false))
		{
			CreatorRandomize(Stage == CreatorStage::Look, Now);
		}
	}
	switch (Stage)
	{
	case CreatorStage::Identity: Hints({{"TAB", "NEXT FIELD"}, {"LEFT|RIGHT/DPAD", "AGE \xC2\xB7 COUNTRY"}, {"ENTER/A", "CONTINUE"}, {"ESC/B", "BACK"}}); break;
	case CreatorStage::Background: Hints({{"LEFT|RIGHT/DPAD", "CHOOSE"}, {"ENTER/A", "CONTINUE"}, {"ESC/B", "BACK"}}); break;
	case CreatorStage::Look: Hints({{"Q|E/LB|RB", "SECTION"}, {"UP|DOWN/DPAD", "SELECT"}, {"LEFT|RIGHT/DPAD", "CHANGE"}, {"R/Y", "RANDOM"}, {"ENTER/A", "CONTINUE"}}); break;
	default: Hints({{"ENTER/A", "BEGIN"}, {"ESC/B", "BACK"}}); break;
	}
	Footer();
}
} // namespace ui
} // namespace ss
