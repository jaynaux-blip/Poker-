#include "ShortStack/UI/VideoArt.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/StreamArt.h"
#include "ShortStack/UI/ThumbArt.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>

// Rerun's art (update 2 §4.2, §4.5): the logo, the five thumbnail frames, a video's tile, Studio's kind glyphs. All
// vector, drawn live from a video's Thumb (about six numbers and words): no image is kept. Marigold on ink, cream text,
// clear of Kast's violet and lime, RiverLine's teal and any platform's red. Every number on a frame is the hand's own
// (U7); an opponent's cards show only when they were turned up (Thumb::Cards has them only then). core-video.

namespace ss
{
namespace ui
{
namespace videoart
{
namespace videoart_detail
{
Color Fade(const Color& C, float A)
{
	return Color{C.R, C.G, C.B, C.A * A};
}

/** Big words with a dark edge, so they read over anything (a thumbnail's text at 640 x 360 is read at 200 px wide). */
void Loud(Canvas& C, const std::string& Text, float X, float Y, float Size, const Color& Fill, Align A, float MaxW)
{
	if (Text.empty())
	{
		return;
	}
	TextStyle S;
	S.Size = Size;
	S.Weight = 900;
	S.HAlign = A;
	S.VAlign = Baseline::Middle;
	S.MaxWidth = MaxW;
	// Shrink to fit rather than cut a word off.
	const float W = C.Measure(Text, Size, 900);
	if (MaxW > 0.0f && W > MaxW)
	{
		// A hair under the width: the size scaled to exactly MaxW measures MaxW give or take a float's rounding, and a hair
		// over it Canvas::Text cuts the word with an ellipsis ("BIG P..." on a Channel tile's Reaction frame).
		S.Size = std::max(10.0f, Size * MaxW / W * 0.98f);
	}
	const float Edge = std::max(1.5f, S.Size * 0.06f);
	S.Col = Hex(0x000000, 0.75f);
	for (const Vec2 D : {Vec2{-Edge, 0.0f}, Vec2{Edge, 0.0f}, Vec2{0.0f, -Edge}, Vec2{0.0f, Edge}, Vec2{Edge, Edge}})
	{
		C.Text(Text, X + D.X, Y + D.Y, S);
	}
	S.Col = Fill;
	C.Text(Text, X, Y, S);
}

/** "AsAd/7c7h/Kd8s2c9h7d": the player's cards, the opponent's (only if shown), the board. */
void SplitCards(const std::string& Field, std::vector<Card>& Mine, std::vector<Card>& Theirs, std::vector<Card>& Board)
{
	std::vector<std::string> Parts(1);
	for (const char Ch : Field)
	{
		if (Ch == '/')
		{
			Parts.emplace_back();
		}
		else
		{
			Parts.back().push_back(Ch);
		}
	}
	Mine = ParseCards(Parts.size() > 0 ? Parts[0] : std::string());
	Theirs = ParseCards(Parts.size() > 1 ? Parts[1] : std::string());
	Board = ParseCards(Parts.size() > 2 ? Parts[2] : std::string());
}

void Ground(Canvas& C, const Rect& R, const Color& Accent)
{
	C.FillRect(R, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, Hex(Ink), Mix(Hex(Ink), Hex(0x2a2116), 0.9f)));
	C.FillEllipse(R.X + R.W * 0.72f, R.Y + R.H * 0.4f, R.W * 0.55f, R.H * 0.75f,
		Paint::Radial({R.X + R.W * 0.72f, R.Y + R.H * 0.4f}, 0.0f, {R.X + R.W * 0.72f, R.Y + R.H * 0.4f}, R.W * 0.55f, Fade(Accent, 0.32f), 0.5f, Fade(Accent, 0.1f),
			Fade(Accent, 0.0f)));
}

/** A row of cards, left to right from X; Lift raises one (the river, when it's the story). */
void CardRow(Canvas& C, const std::vector<Card>& Cards, float X, float Y, float W, float H, float Gap, int Lift)
{
	for (size_t I = 0; I < Cards.size(); ++I)
	{
		const float Up = static_cast<int>(I) == Lift ? H * 0.18f : 0.0f;
		if (static_cast<int>(I) == Lift)
		{
			C.GlowRoundRect({X + static_cast<float>(I) * (W + Gap), Y - Up, W, H}, W * 0.08f, Hex(Marigold, 0.8f), W * 0.25f);
		}
		DrawCard(C, Cards[I], X + static_cast<float>(I) * (W + Gap), Y - Up, W, H);
	}
}

/** The player with no camera on them (live, storytime) or with no look to draw: a plain figure in cream. */
void Figure(Canvas& C, const Rect& R, const hero::Character* Who)
{
	C.PushClip(R);
	C.FillRect(R, Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Hex(0x2b2418), Hex(0x14110c)));
	if (Who)
	{
		const float Scale = R.H / 520.0f;
		DrawPortrait(C, *Who, R.X + R.W * 0.5f, R.Y + R.H * 0.38f, Scale, 0.0);
	}
	else
	{
		const float Cx = R.X + R.W * 0.5f;
		C.FillEllipse(Cx, R.Y + R.H * 0.36f, R.H * 0.16f, R.H * 0.2f, Hex(Cream, 0.9f));
		C.FillEllipse(Cx, R.Y + R.H * 0.98f, R.H * 0.36f, R.H * 0.34f, Hex(Cream, 0.9f));
	}
	C.PopClip();
}

/** An opponent nobody may see (storytime): a silhouette with a question mark. */
void Nobody(Canvas& C, float Cx, float Cy, float Rad)
{
	C.FillCircle(Cx, Cy, Rad, Paint::Linear({Cx, Cy - Rad}, {Cx, Cy + Rad}, Hex(0x3a3a44), Hex(0x1c1c22)));
	C.FillEllipse(Cx, Cy - Rad * 0.18f, Rad * 0.34f, Rad * 0.4f, Hex(0x0c0c10));
	C.FillEllipse(Cx, Cy + Rad * 0.62f, Rad * 0.62f, Rad * 0.42f, Hex(0x0c0c10));
	C.StrokeEllipse(Cx, Cy, Rad, Rad, Hex(Cream, 0.5f), std::max(1.5f, Rad * 0.05f));
	TextStyle Q;
	Q.Size = Rad * 0.8f;
	Q.Weight = 900;
	Q.Col = Hex(Cream);
	Q.HAlign = Align::Center;
	Q.VAlign = Baseline::Middle;
	C.Text("?", Cx, Cy - Rad * 0.12f, Q);
}

void Trophy(Canvas& C, float Cx, float Cy, float S, const Color& Gold)
{
	const Paint Metal = Paint::Linear({Cx - S, Cy - S}, {Cx + S, Cy + S}, Mix(Gold, Hex(0xffffff), 0.35f), Mix(Gold, Hex(0x5a3a00), 0.35f));
	// The cup, its handles, the stem and the base.
	C.FillPolygon({{Cx - S * 0.62f, Cy - S * 0.9f}, {Cx + S * 0.62f, Cy - S * 0.9f}, {Cx + S * 0.5f, Cy - S * 0.2f}, {Cx + S * 0.18f, Cy + S * 0.08f},
					  {Cx - S * 0.18f, Cy + S * 0.08f}, {Cx - S * 0.5f, Cy - S * 0.2f}},
		Metal);
	C.StrokeArc(Cx - S * 0.62f, Cy - S * 0.58f, S * 0.26f, Pi * 0.5f, Pi * 1.5f, Gold, S * 0.09f);
	C.StrokeArc(Cx + S * 0.62f, Cy - S * 0.58f, S * 0.26f, -Pi * 0.5f, Pi * 0.5f, Gold, S * 0.09f);
	C.FillRect({Cx - S * 0.08f, Cy + S * 0.06f, S * 0.16f, S * 0.42f}, Metal);
	C.FillRoundRect({Cx - S * 0.46f, Cy + S * 0.46f, S * 0.92f, S * 0.26f}, S * 0.05f, Metal);
	C.FillRoundRect({Cx - S * 0.3f, Cy + S * 0.52f, S * 0.6f, S * 0.12f}, S * 0.03f, Hex(Ink, 0.55f));
	C.FillEllipse(Cx - S * 0.2f, Cy - S * 0.62f, S * 0.08f, S * 0.18f, Hex(0xffffff, 0.45f));
}

void Play(Canvas& C, float Cx, float Cy, float S, const Color& Col)
{
	C.FillPolygon({{Cx - S * 0.38f, Cy - S * 0.5f}, {Cx + S * 0.5f, Cy}, {Cx - S * 0.38f, Cy + S * 0.5f}}, Col);
}

/** Word-wrapped text in at most Lines lines (the last cut with an ellipsis). */
void Wrapped(Canvas& C, const std::string& Text, float X, float Y, float W, float Size, int Weight, const Color& Col, int Lines)
{
	std::vector<std::string> Words;
	std::string Cur;
	for (const char Ch : Text)
	{
		if (Ch == ' ')
		{
			if (!Cur.empty())
			{
				Words.push_back(Cur);
			}
			Cur.clear();
		}
		else
		{
			Cur.push_back(Ch);
		}
	}
	if (!Cur.empty())
	{
		Words.push_back(Cur);
	}
	TextStyle S;
	S.Size = Size;
	S.Weight = Weight;
	S.Col = Col;
	S.VAlign = Baseline::Top;
	int Line = 0;
	std::string Row;
	for (size_t I = 0; I < Words.size(); ++I)
	{
		const std::string Try = Row.empty() ? Words[I] : Row + " " + Words[I];
		if (!Row.empty() && C.Measure(Try, Size, Weight) > W)
		{
			if (Line == Lines - 1)
			{
				break;
			}
			C.Text(Row, X, Y + static_cast<float>(Line) * Size * 1.22f, S);
			++Line;
			Row = Words[I];
		}
		else
		{
			Row = Try;
		}
	}
	S.MaxWidth = W;
	C.Text(Row, X, Y + static_cast<float>(Line) * Size * 1.22f, S);
}

std::string Age(double Minutes)
{
	const double Days = Minutes / net::MinutesPerDay;
	if (Minutes < 60.0)
	{
		return "just now";
	}
	if (Days < 1.0)
	{
		const int H = static_cast<int>(Minutes / 60.0);
		return std::to_string(H) + (H == 1 ? " hour ago" : " hours ago");
	}
	if (Days < 30.0)
	{
		const int D = static_cast<int>(Days);
		return std::to_string(D) + (D == 1 ? " day ago" : " days ago");
	}
	if (Days < 365.0)
	{
		const int M = static_cast<int>(Days / 30.0);
		return std::to_string(M) + (M == 1 ? " month ago" : " months ago");
	}
	const int Y = static_cast<int>(Days / 365.0);
	return std::to_string(Y) + (Y == 1 ? " year ago" : " years ago");
}

std::string ViewCount(double Views)
{
	const long long N = static_cast<long long>(std::floor(Views));
	if (N >= 1000000)
	{
		return Fixed(static_cast<double>(N) / 1000000.0, N >= 10000000 ? 0 : 1) + "M views";
	}
	if (N >= 10000)
	{
		return std::to_string(N / 1000) + "K views";
	}
	return Grouped(N) + (N == 1 ? " view" : " views");
}
} // namespace videoart_detail

void Logo(Canvas& C, Rect R)
{
	using namespace videoart_detail;
	const float S = std::min(R.W, R.H);
	const float Cx = R.X + R.W * 0.5f, Cy = R.Y + R.H * 0.5f;
	const Rect Sq{Cx - S * 0.5f, Cy - S * 0.5f, S, S};
	C.FillRoundRect(Sq, S * 0.24f, Paint::Linear({Sq.X, Sq.Y}, {Sq.X, Sq.Y + S}, Mix(Hex(Marigold), Hex(0xffffff), 0.12f), Mix(Hex(Marigold), Hex(0x8a4a00), 0.18f)));
	// A play triangle inside a counter-clockwise arrow: the rerun.
	const float Rad = S * 0.31f;
	const float Line = std::max(1.5f, S * 0.075f);
	const float From = -Pi * 0.35f, To = Pi * 1.45f;
	C.StrokeArc(Cx, Cy, Rad, From, To, Hex(Ink), Line, true);
	// The arrowhead at the arc's start, pointing back the way it turns (counter-clockwise on screen).
	const Vec2 Tip{Cx + Rad * std::cos(From), Cy + Rad * std::sin(From)};
	const Vec2 Along{std::sin(From), -std::cos(From)}; // the tangent, against the arc's direction
	const Vec2 Out{std::cos(From), std::sin(From)};
	const float Head = S * 0.13f;
	C.FillPolygon({{Tip.X + Along.X * Head, Tip.Y + Along.Y * Head}, {Tip.X + Out.X * Head * 0.8f, Tip.Y + Out.Y * Head * 0.8f}, {Tip.X - Out.X * Head * 0.8f, Tip.Y - Out.Y * Head * 0.8f}},
		Hex(Ink));
	Play(C, Cx + S * 0.02f, Cy, S * 0.3f, Hex(Ink));
}

void Thumbnail(Canvas& C, Rect R, const video::Thumb& T)
{
	Thumbnail(C, R, T, nullptr);
}

void Thumbnail(Canvas& C, Rect R, const video::Thumb& T, const hero::Character* Who)
{
	using namespace videoart_detail;
	const Color Accent = Hex(T.Accent != 0 ? T.Accent : Marigold);
	C.PushClip(R);
	Ground(C, R, Accent);
	std::vector<Card> Mine, Theirs, Board;
	SplitCards(T.Cards, Mine, Theirs, Board);
	const float U = R.H / 360.0f; // the frame's unit: 1 at 640 x 360
	const bool Portrait = T.Mood >= 10;
	const int Mood = std::clamp(Portrait ? T.Mood - 10 : T.Mood, 0, 5);
	switch (T.Fr)
	{
	case video::Frame::Reaction:
	{
		// The face, big, on the left; the cards in a corner; the words over the right.
		const Rect Face{R.X, R.Y, R.W * 0.58f, R.H};
		if (Portrait)
		{
			Figure(C, Face, Who);
		}
		else
		{
			streamart::Cam Look;
			Look.Gear.Quality = 0.6;
			Look.Gear.Lights = 1;
			Look.Face = static_cast<kast::Mood>(Mood);
			Look.Live = false;
			Look.FaceAge = 0.4;
			streamart::Facecam(C, Face, Look);
		}
		C.FillRect({Face.X + Face.W - 60.0f * U, R.Y, 60.0f * U, R.H}, Paint::Linear({Face.X + Face.W - 60.0f * U, 0.0f}, {Face.X + Face.W, 0.0f}, Fade(Hex(Ink), 0.0f), Hex(Ink)));
		Loud(C, T.BigText, R.X + R.W * 0.79f, R.Y + R.H * 0.3f, 74.0f * U, Accent, Align::Center, R.W * 0.4f);
		if (Mine.size() >= 2)
		{
			// With the opponent's shown cards beside them, both pairs shrink so the two fit inside the frame's right edge.
			const bool Both = Theirs.size() >= 2;
			CardRow(C, Mine, R.X + R.W * 0.62f, R.Y + R.H * 0.52f, (Both ? 60.0f : 70.0f) * U, (Both ? 84.0f : 98.0f) * U, (Both ? 6.0f : 8.0f) * U, -1);
		}
		if (Theirs.size() >= 2)
		{
			CardRow(C, Theirs, R.X + R.W - 110.0f * U, R.Y + R.H * 0.6f, 46.0f * U, 64.0f * U, 4.0f * U, -1);
		}
		break;
	}
	case video::Frame::Cards:
	{
		// The hole cards and the board, the river lifted.
		if (Mine.size() >= 2)
		{
			CardRow(C, Mine, R.X + 40.0f * U, R.Y + 70.0f * U, 104.0f * U, 146.0f * U, 12.0f * U, -1);
		}
		if (Theirs.size() >= 2)
		{
			CardRow(C, Theirs, R.X + R.W - 40.0f * U - 2.0f * 84.0f * U - 10.0f * U, R.Y + 82.0f * U, 84.0f * U, 118.0f * U, 10.0f * U, -1);
			Loud(C, "VS", R.X + R.W * 0.6f, R.Y + 140.0f * U, 40.0f * U, Hex(Cream), Align::Center, 0.0f);
		}
		if (!Board.empty())
		{
			const float W = 64.0f * U, Gap = 8.0f * U;
			const float Total = static_cast<float>(Board.size()) * (W + Gap) - Gap;
			CardRow(C, Board, R.X + (R.W - Total) * 0.5f, R.Y + R.H - 112.0f * U, W, 90.0f * U, Gap, Board.size() >= 5 ? 4 : -1);
		}
		Loud(C, T.BigText, R.X + R.W * 0.5f, R.Y + 34.0f * U, 54.0f * U, Accent, Align::Center, R.W * 0.9f);
		break;
	}
	case video::Frame::Number:
	{
		// The pot or the prize, huge.
		Loud(C, T.BigText, R.X + R.W * 0.5f, R.Y + R.H * 0.44f, 150.0f * U, Accent, Align::Center, R.W * 0.9f);
		if (Mine.size() >= 2)
		{
			CardRow(C, Mine, R.X + R.W * 0.5f - 64.0f * U, R.Y + R.H - 104.0f * U, 60.0f * U, 84.0f * U, 8.0f * U, -1);
		}
		break;
	}
	case video::Frame::Versus:
	{
		// The player's mark against theirs (gh0stfold's ghost; nobody's face from a private game).
		const float Rad = 92.0f * U;
		AvatarSpec Me;
		Me.Icon = AvatarIcon::AceCard;
		Me.Bg = Marigold;
		Me.Bg2 = 0x8a4a00;
		Me.Ink = Ink;
		Me.Frame = AvatarFrame::Neon;
		Me.Rim = Cream;
		Me.Halo = true;
		DrawAvatar(C, R.X + R.W * 0.25f, R.Y + R.H * 0.46f, Rad, Me);
		if (T.Opponent.empty())
		{
			Nobody(C, R.X + R.W * 0.75f, R.Y + R.H * 0.46f, Rad);
		}
		else
		{
			AvatarSpec Them = AvatarFor(T.Opponent);
			if (T.Opponent == RivalName)
			{
				Them.Icon = AvatarIcon::Ghost;
				Them.Frame = AvatarFrame::Neon;
				Them.Halo = true;
			}
			DrawAvatar(C, R.X + R.W * 0.75f, R.Y + R.H * 0.46f, Rad, Them);
			TextStyle N;
			N.Size = 26.0f * U;
			N.Weight = 700;
			N.Col = Hex(Cream);
			N.HAlign = Align::Center;
			N.VAlign = Baseline::Middle;
			N.MaxWidth = R.W * 0.42f;
			C.Text(T.Opponent, R.X + R.W * 0.75f, R.Y + R.H * 0.46f + Rad + 26.0f * U, N);
		}
		Loud(C, T.BigText, R.X + R.W * 0.5f, R.Y + R.H * 0.46f, 84.0f * U, Accent, Align::Center, 0.0f);
		break;
	}
	case video::Frame::Trophy:
	{
		Trophy(C, R.X + R.W * 0.3f, R.Y + R.H * 0.46f, 120.0f * U, Hex(0xf2c14e));
		Loud(C, T.BigText, R.X + R.W * 0.7f, R.Y + R.H * 0.46f, 70.0f * U, Accent, Align::Center, R.W * 0.5f);
		break;
	}
	}
	if (T.Site != 0)
	{
		// A rival site's hand wears its strip (G3).
		siteart::Strip(C, {R.X, R.Y + R.H - 14.0f * U, R.W, 14.0f * U}, static_cast<net::SiteId>(T.Site));
	}
	C.PopClip();
}

void Tile(Canvas& C, Rect R, const video::Video& V, double World)
{
	using namespace videoart_detail;
	// The thumbnail at 16:9 on top, a duration badge, then the title in two lines and "views · age".
	const float ThumbH = R.W * 9.0f / 16.0f;
	const Rect Th{R.X, R.Y, R.W, std::min(ThumbH, R.H * 0.72f)};
	C.PushClip(R);
	C.FillRoundRect(Th, 8.0f, Hex(Ink));
	C.PushClip(Th);
	if (V.X.On && V.X.Spec[static_cast<size_t>(std::clamp(static_cast<int>(V.X.DuelWin), 0, 2))].Arch >= 0)
	{
		// Update 3 (C3, ui-thumbs): a Rerun 2.0 video's shown recipe through the compositor (Tile has no Look: the default
		// person; the pages draw with the player's, thumbart::VideoThumb). Update 2's videos draw their frame below, unchanged.
		thumbart::Budget All;
		All.Max = 1 << 20;
		thumbart::Draw(C, Th, V.X.Spec[static_cast<size_t>(std::clamp(static_cast<int>(V.X.DuelWin), 0, 2))], hero::Character(), V.Th, All);
	}
	else
	{
		Thumbnail(C, Th, V.Th);
	}
	C.PopClip();
	const int Seconds = static_cast<int>(std::lround(V.LengthMin * 60.0));
	const std::string Length = std::to_string(Seconds / 60) + ":" + (Seconds % 60 < 10 ? "0" : "") + std::to_string(Seconds % 60);
	TextStyle B;
	B.Size = std::max(10.0f, Th.H * 0.075f);
	B.Weight = 700;
	B.Col = Hex(Cream);
	B.HAlign = Align::Right;
	B.VAlign = Baseline::Middle;
	const float Bw = C.Measure(Length, B.Size, 700) + B.Size * 0.8f;
	C.FillRoundRect({Th.X + Th.W - Bw - 6.0f, Th.Y + Th.H - B.Size * 1.6f - 6.0f, Bw, B.Size * 1.6f}, 4.0f, Hex(0x000000, 0.78f));
	C.Text(Length, Th.X + Th.W - 6.0f - B.Size * 0.4f, Th.Y + Th.H - B.Size * 0.8f - 6.0f, B);
	const float Size = std::max(11.0f, R.W * 0.052f);
	Wrapped(C, V.Title, R.X, Th.Y + Th.H + Size * 0.5f, R.W, Size, 700, Hex(Cream), 2);
	TextStyle M;
	M.Size = Size * 0.82f;
	M.Col = Hex(0xa89f8e);
	M.VAlign = Baseline::Top;
	M.MaxWidth = R.W;
	C.Text(ViewCount(video::Views(V, World)) + " \xC2\xB7 " + Age(World - V.At), R.X, Th.Y + Th.H + Size * 3.1f, M);
	C.PopClip();
}

void KindGlyph(Canvas& C, float Cx, float Cy, float Size, video::Kind K)
{
	using namespace videoart_detail;
	const float S = Size * 0.5f;
	const Color Col = Hex(Marigold);
	switch (K)
	{
	case video::Kind::BadBeat:
	case video::Kind::Suckout:
	case video::Kind::Cooler:
		// A lightning bolt.
		C.FillPolygon({{Cx + S * 0.15f, Cy - S}, {Cx - S * 0.55f, Cy + S * 0.12f}, {Cx - S * 0.02f, Cy + S * 0.12f}, {Cx - S * 0.2f, Cy + S}, {Cx + S * 0.6f, Cy - S * 0.18f},
						  {Cx + S * 0.05f, Cy - S * 0.18f}},
			Col);
		break;
	case video::Kind::Bluff:
	case video::Kind::BluffCaught:
	case video::Kind::HeroCall:
	case video::Kind::HeroFold:
		// An eye: a read.
		C.FillEllipse(Cx, Cy, S, S * 0.55f, Col);
		C.FillCircle(Cx, Cy, S * 0.34f, Hex(Ink));
		C.FillCircle(Cx + S * 0.1f, Cy - S * 0.1f, S * 0.1f, Hex(Cream));
		break;
	case video::Kind::BigPot:
	case video::Kind::Knockout:
	case video::Kind::Bubble:
		// A stack of chips.
		for (int I = 0; I < 3; ++I)
		{
			const float Y = Cy + S * 0.55f - static_cast<float>(I) * S * 0.42f;
			C.FillEllipse(Cx, Y, S * 0.85f, S * 0.32f, Mix(Col, Hex(Ink), 0.25f * static_cast<float>(I % 2)));
			C.StrokeEllipse(Cx, Y, S * 0.85f, S * 0.32f, Hex(Ink, 0.6f), std::max(1.0f, S * 0.08f));
		}
		break;
	case video::Kind::DeepRun:
	case video::Kind::FinalTable:
	case video::Kind::Win: Trophy(C, Cx, Cy, S * 0.95f, Col); break;
	case video::Kind::Rival:
		// A ghost.
		C.FillCircle(Cx, Cy - S * 0.2f, S * 0.7f, Hex(Cream));
		C.FillPolygon({{Cx - S * 0.7f, Cy - S * 0.2f}, {Cx + S * 0.7f, Cy - S * 0.2f}, {Cx + S * 0.7f, Cy + S * 0.9f}, {Cx + S * 0.35f, Cy + S * 0.6f}, {Cx, Cy + S * 0.9f},
						  {Cx - S * 0.35f, Cy + S * 0.6f}, {Cx - S * 0.7f, Cy + S * 0.9f}},
			Hex(Cream));
		C.FillCircle(Cx - S * 0.25f, Cy - S * 0.25f, S * 0.12f, Hex(Ink));
		C.FillCircle(Cx + S * 0.25f, Cy - S * 0.25f, S * 0.12f, Hex(Ink));
		break;
	case video::Kind::Story:
	case video::Kind::StreamMoment:
	case video::Kind::Count:
		// A speech bubble.
		C.FillRoundRect({Cx - S, Cy - S * 0.75f, S * 2.0f, S * 1.25f}, S * 0.35f, Col);
		C.FillPolygon({{Cx - S * 0.4f, Cy + S * 0.45f}, {Cx - S * 0.55f, Cy + S}, {Cx, Cy + S * 0.45f}}, Col);
		break;
	}
}
} // namespace videoart
} // namespace ui
} // namespace ss
