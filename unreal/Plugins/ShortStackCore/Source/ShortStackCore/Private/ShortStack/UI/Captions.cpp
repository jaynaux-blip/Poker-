#include "ShortStack/UI/Captions.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Captions (next_segment_spec §4.9). core-tableui.

namespace ss
{
namespace ui
{
namespace captions_detail
{
using namespace frontend_detail;

const Color Teal = Hex(0x27d3c3);
const Color Warm = Hex(0xf2832e);  // said to the player: the Embercrest's ember
const Color Amber = Hex(0xf6b04a); // the floor
const Color Ink = Hex(0xeef1f6);

Color A(Color C, float Alpha)
{
	C.A *= Alpha;
	return C;
}

size_t Characters(const std::string& S)
{
	size_t N = 0;
	for (const char Ch : S)
	{
		N += (static_cast<unsigned char>(Ch) & 0xC0u) != 0x80u ? 1u : 0u;
	}
	return N;
}

/** Up to Max lines of Text in Width, the last one cut short with an ellipsis if it ran on. */
std::vector<std::string> WrapRows(Canvas& C, const std::string& Text, float Width, float Size, int Weight, size_t Max)
{
	std::vector<std::string> Out = WrapText(C, Text, Width, Size, Weight);
	if (Out.size() > Max)
	{
		Out.resize(Max);
		std::string& Last = Out.back();
		const std::string Ellipsis = "\xE2\x80\xA6";
		while (!Last.empty() && C.Measure(Last + Ellipsis, Size, Weight) > Width)
		{
			// Back off a whole UTF-8 character.
			size_t Cut = Last.size() - 1;
			while (Cut > 0 && (static_cast<unsigned char>(Last[Cut]) & 0xC0u) == 0x80u)
			{
				--Cut;
			}
			Last.resize(Cut);
		}
		while (!Last.empty() && Last.back() == ' ')
		{
			Last.pop_back();
		}
		Last += Ellipsis;
	}
	// A single word wider than the box: cut it (a name, a long number).
	for (std::string& L : Out)
	{
		if (C.Measure(L, Size, Weight) > Width)
		{
			const std::string Ellipsis = "\xE2\x80\xA6";
			while (L.size() > 1 && C.Measure(L + Ellipsis, Size, Weight) > Width)
			{
				size_t Cut = L.size() - 1;
				while (Cut > 0 && (static_cast<unsigned char>(L[Cut]) & 0xC0u) == 0x80u)
				{
					--Cut;
				}
				L.resize(Cut);
			}
			L += Ellipsis;
		}
	}
	return Out;
}

/** A small loudspeaker, its centre at (X, Y), Size tall. */
void Speaker(Canvas& C, float X, float Y, float Size, const Color& Col)
{
	const float S = Size;
	C.FillRect({X - S * 0.42f, Y - S * 0.16f, S * 0.22f, S * 0.32f}, Paint(Col));
	C.FillPolygon({{X - S * 0.22f, Y - S * 0.16f}, {X + S * 0.04f, Y - S * 0.4f}, {X + S * 0.04f, Y + S * 0.4f}, {X - S * 0.22f, Y + S * 0.16f}}, Paint(Col));
	C.StrokeArc(X + S * 0.06f, Y, S * 0.2f, -0.9f, 0.9f, Col, std::max(1.2f, S * 0.08f));
	C.StrokeArc(X + S * 0.06f, Y, S * 0.36f, -0.9f, 0.9f, A(Col, 0.7f), std::max(1.2f, S * 0.08f));
}

/** Where the speaker is, before the name (Everything mode): a chevron left or right, or down for behind. */
float SideMark(Canvas& C, int Side, float X, float Cy, float K, const Color& Col)
{
	if (Side == 0)
	{
		return 0.0f;
	}
	const float S = 5.5f * K;
	if (Side == 2)
	{
		C.StrokePolyline({{X, Cy - S * 0.5f}, {X + S, Cy + S * 0.5f}, {X + S * 2.0f, Cy - S * 0.5f}}, false, Col, 2.0f * K, true);
	}
	else
	{
		Chevron(C, X + S, Cy, S, Side > 0, Col);
	}
	return S * 2.0f + 8.0f * K;
}

struct Shown
{
	const Caption* Cap = nullptr;
	float Alpha = 1.0f;
};

std::string Upper(std::string S)
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

/**
 * The PA's tag (style 1): FLOOR for the floor (no speaker, or "Floor"), otherwise whoever has the mic, as the host names
 * them: on a Sunday Dee's "Dee · PA" reads DEE · PA (spec 4.7). Shown whether or not speaker names are on: it
 * says where the voice comes from, as FLOOR always has.
 */
std::string PaTag(const Caption& Cp)
{
	const std::string Up = Upper(Cp.Speaker);
	return Up.empty() || Up == "FLOOR" ? std::string("FLOOR") : Up;
}
} // namespace captions_detail

float CaptionHold(const std::string& Line, float SpeechSeconds)
{
	const float Read = 1.5f + static_cast<float>(captions_detail::Characters(Line)) / 15.0f;
	return std::max(SpeechSeconds, std::clamp(Read, 2.6f, 8.0f));
}

bool CaptionShown(const Caption& Line, const CaptionStyle& S)
{
	if (S.Mode <= 0)
	{
		return false;
	}
	if (S.Mode == 1)
	{
		return Line.Style != 2 && !Line.Room;
	}
	return true;
}

void DrawCaptions(Canvas& C, const std::vector<Caption>& Lines, const CaptionStyle& S, float BottomY, float MaxWidth, double Now)
{
	using namespace captions_detail;
	if (S.Mode <= 0 || Lines.empty())
	{
		return;
	}
	std::vector<Shown> List;
	for (const Caption& Cp : Lines)
	{
		const double Age = Now - Cp.At;
		const double Hold = Cp.Hold > 0.0f ? static_cast<double>(Cp.Hold) : static_cast<double>(CaptionHold(Cp.Line, 0.0f));
		if (Age < 0.0 || Age >= Hold || !CaptionShown(Cp, S) || Cp.Line.empty())
		{
			continue;
		}
		List.push_back({&Cp, std::min(Ease(Age / 0.2), Ease((Hold - Age) / 0.4))});
	}
	std::stable_sort(List.begin(), List.end(), [](const Shown& L, const Shown& R) { return L.Cap->At < R.Cap->At; });
	// Over the cap: the lowest priority goes first, then the oldest; the floor stays.
	const size_t Cap = static_cast<size_t>(std::max(1, S.MaxShown));
	while (List.size() > Cap)
	{
		size_t Drop = List.size();
		for (size_t I = 0; I < List.size(); ++I)
		{
			const Caption& Cp = *List[I].Cap;
			if (Cp.Style == 1)
			{
				continue;
			}
			if (Drop == List.size() || Cp.Priority < List[Drop].Cap->Priority)
			{
				Drop = I;
			}
		}
		List.erase(List.begin() + static_cast<std::ptrdiff_t>(Drop == List.size() ? 0 : Drop));
	}
	const float K = std::clamp(static_cast<float>(S.Size), 80.0f, 200.0f) / 100.0f;
	const float Back = std::clamp(static_cast<float>(S.Backdrop), 0.0f, 100.0f) / 100.0f;
	const float Size = 21.0f * K;
	const float TagSize = 13.0f * K;
	const float LineH = 28.0f * K;
	const float Pad = 20.0f * K;
	const float Cx = C.Width() * 0.5f;
	float By = BottomY;
	for (auto It = List.rbegin(); It != List.rend(); ++It)
	{
		const Caption& Cp = *It->Cap;
		const float Al = It->Alpha;
		const bool Floor = Cp.Style == 1;
		const bool Sound = Cp.Style == 2;
		const bool ToYou = Cp.Style == 3;
		const Color TagCol = Floor ? Amber : ToYou ? Warm : Teal;
		const std::string Tag = Floor ? PaTag(Cp) : Sound || !S.Names ? std::string() : Cp.Speaker;
		const bool Mark = S.Mode >= 2 && Cp.Side != 0 && !Floor;
		// The tag's width: the side mark, the speaker glyph (the floor), the name.
		float TagW = 0.0f;
		if (Mark)
		{
			TagW += 5.5f * K * 2.0f + 8.0f * K;
		}
		if (Floor)
		{
			TagW += 22.0f * K;
		}
		if (!Tag.empty())
		{
			TagW += TrackedWidth(C, Tag, TagSize, 800, 2.4f * K);
		}
		const float TagGap = TagW > 0.0f ? 14.0f * K : 0.0f;
		// About 42 characters a line; the floor gets the box's whole width (its announcements run long, and nobody cuts the PA).
		const float Room = MaxWidth - TagW - TagGap - Pad * 2.0f;
		const float TextMax = std::max(120.0f, Floor ? Room : std::min(Room, 520.0f * K));
		const std::string Body = Cp.Line;
		const int Weight = Sound ? 300 : 500;
		const std::vector<std::string> Rows = WrapRows(C, Body, TextMax, Size, Weight, 2);
		float Widest = 0.0f;
		for (const std::string& Ln : Rows)
		{
			Widest = std::max(Widest, C.Measure(Ln, Size, Weight));
		}
		const float Bw = Pad * 2.0f + TagW + TagGap + Widest;
		const float Bh = 16.0f * K + static_cast<float>(Rows.size()) * LineH;
		const float Bx = Cx - Bw * 0.5f;
		By -= Bh;
		C.FillRoundRect({Bx, By, Bw, Bh}, 10.0f * K, Paint(Rgba(4, 6, 10, Back * Al)));
		if (Floor)
		{
			C.FillRect({Bx + 10.0f * K, By, Bw - 20.0f * K, 2.0f}, Paint(A(Amber, 0.9f * Al)));
		}
		float Tx = Bx + Pad;
		const float FirstBase = By + 8.0f * K + LineH - 1.0f * K;
		const float TagMid = FirstBase - TagSize * 0.36f;
		if (Mark)
		{
			Tx += SideMark(C, Cp.Side, Tx, TagMid, K, A(TagCol, 0.85f * Al));
		}
		if (Floor)
		{
			Speaker(C, Tx + 8.0f * K, TagMid, 16.0f * K, A(Amber, Al));
			Tx += 22.0f * K;
		}
		if (!Tag.empty())
		{
			Tx += TrackedText(C, Tag, Tx, FirstBase - 1.0f * K, TagSize, 800, A(TagCol, Al), 2.4f * K);
		}
		Tx += TagGap;
		float Ly = By + 8.0f * K;
		for (const std::string& Ln : Rows)
		{
			Ly += LineH;
			C.Text(Ln, Tx, Ly - 1.0f * K, Ts(Size, Weight, A(Ink, (Sound ? 0.7f : 0.95f) * Al)));
		}
		By -= 8.0f * K;
	}
}

void DrawNightLog(Canvas& C, const std::vector<Caption>& Lines, const std::string& Title, const Rect& R, double Now)
{
	using namespace captions_detail;
	(void)Now;
	TrackedText(C, Upper(Title), R.X, R.Y + 18.0f, 15.0f, 800, Amber, 3.0f);
	C.FillRect({R.X, R.Y + 32.0f, R.W, 1.0f}, Paint(A(Ink, 0.12f)));
	if (Lines.empty())
	{
		C.Text("Nothing said yet. The room's still finding its seats.", R.X, R.Y + 72.0f, Ts(16.0f, 400, A(Ink, 0.6f)));
		return;
	}
	const float Top = R.Y + 48.0f;
	const float ClockW = 92.0f;
	const float TagW = 150.0f;
	const float TextX = R.X + ClockW + TagW;
	const float TextW = std::max(160.0f, R.W - ClockW - TagW);
	const float Row = 24.0f;
	// The newest that fit, measured from the bottom up (a line wraps to two rows at most).
	struct Laid
	{
		const Caption* Cap;
		std::vector<std::string> Rows;
	};
	std::vector<Laid> Out;
	float Used = 0.0f;
	const size_t First = Lines.size() > 40 ? Lines.size() - 40 : 0;
	for (size_t I = Lines.size(); I-- > First;)
	{
		const Caption& Cp = Lines[I];
		Laid L{&Cp, captions_detail::WrapRows(C, Cp.Line, TextW, 15.0f, Cp.Style == 2 ? 300 : 400, 2)};
		const float H = static_cast<float>(L.Rows.size()) * Row + 6.0f;
		if (Used + H > R.H - 48.0f)
		{
			break;
		}
		Used += H;
		Out.push_back(L);
	}
	float Y = Top;
	for (auto It = Out.rbegin(); It != Out.rend(); ++It)
	{
		const Caption& Cp = *It->Cap;
		const bool Floor = Cp.Style == 1;
		const bool Sound = Cp.Style == 2;
		const Color TagCol = Floor ? Amber : Cp.Style == 3 ? Warm : Teal;
		const float Base = Y + 17.0f;
		C.Text(Cp.Clock, R.X, Base, Ts(13.0f, 500, A(Ink, 0.5f), Align::Left, Baseline::Alphabetic, true));
		const std::string Tag = Floor ? PaTag(Cp) : Sound ? std::string() : Upper(Cp.Speaker);
		if (!Tag.empty())
		{
			C.Text(Tag, R.X + ClockW, Base, Ts(13.0f, 800, TagCol, Align::Left, Baseline::Alphabetic, false, TagW - 12.0f));
		}
		float Ly = Base;
		for (const std::string& Ln : It->Rows)
		{
			C.Text(Ln, TextX, Ly, Ts(15.0f, Sound ? 300 : 400, A(Ink, Sound ? 0.6f : 0.9f)));
			Ly += Row;
		}
		Y += static_cast<float>(It->Rows.size()) * Row + 6.0f;
	}
}
} // namespace ui
} // namespace ss
