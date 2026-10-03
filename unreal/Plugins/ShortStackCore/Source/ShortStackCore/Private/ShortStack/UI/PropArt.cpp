#include "ShortStack/UI/PropArt.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/UI/StoreCounter.h"
#include "../StrictFloat.h"

#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace props
{
namespace propart_detail
{
void Paper(Canvas& C, float W, float H)
{
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W * 0.3f, H}, Rgba(240, 235, 222, 1.0f), Rgba(222, 216, 201, 1.0f)));
}
} // namespace propart_detail

using namespace propart_detail;

void EvictionNotice(Canvas& C)
{
	const float W = NoticeW;
	const float H = NoticeH;
	Paper(C, W, H);
	const Color Ink = Hex(0x1a1a1a);
	C.Text("NOTICE TO PAY RENT", W / 2.0f, 80.0f, Ts(40.0f, 700, Ink, Align::Center));
	C.Text("OR QUIT", W / 2.0f, 128.0f, Ts(40.0f, 700, Ink, Align::Center));
	const char* Lines[12] = {
		"TO THE TENANT IN POSSESSION OF UNIT 3C:", "", "You are hereby notified that rent is past due", "for the premises you occupy, in the amount of",
		"$1,150.00 plus a late fee of $75.00.", "", "Within FOURTEEN (14) days you must pay the", "amount due in full, or vacate and surrender",
		"possession of the premises.", "", "Failure to do so will result in legal", "proceedings to recover possession.",
	};
	for (int I = 0; I < 12; ++I)
	{
		C.Text(Lines[I], 60.0f, 200.0f + static_cast<float>(I) * 30.0f, Ts(20.0f, 400, Ink));
	}
	// FINAL NOTICE stamp.
	const Color Stamp = Rgba(190, 20, 20, 0.85f);
	const float Cx = W * 0.62f;
	const float Cy = H * 0.78f;
	// Text runs cannot rotate, so the stamp is square to the page.
	const float Tw = C.Measure("FINAL NOTICE", 44.0f, 900) + 48.0f;
	C.StrokeRoundRect({Cx - Tw / 2.0f, Cy - 44.0f, Tw, 88.0f}, 2.0f, Stamp, 7.0f);
	C.Text("FINAL NOTICE", Cx, Cy + 2.0f, Ts(44.0f, 900, Stamp, Align::Center, Baseline::Middle));
}

void PowerBill(Canvas& C)
{
	Paper(C, BillW, BillH);
	C.Text("CITY POWER & LIGHT", 40.0f, 70.0f, Ts(30.0f, 700, Hex(0x222222)));
	C.Text("PAST DUE \xE2\x80\x94 $186.42", 40.0f, 120.0f, Ts(20.0f, 400, Hex(0x222222)));
	C.FillRect({40.0f, 140.0f, BillW - 80.0f, 4.0f}, Hex(0xb01717));
	for (int I = 0; I < 9; ++I)
	{
		const float Y = 200.0f + static_cast<float>(I) * 34.0f;
		C.FillRect({40.0f, Y, 380.0f - static_cast<float>(I % 3) * 60.0f, 10.0f}, Rgba(60, 60, 60, 0.35f));
		C.FillRect({BillW - 160.0f, Y, 120.0f, 10.0f}, Rgba(60, 60, 60, 0.35f));
	}
}

void StickyNote(Canvas& C, const std::vector<std::string>& Lines, const Color& PaperColor)
{
	const float S = NoteSize;
	C.FillRect({0.0f, 0.0f, S, S}, PaperColor);
	C.FillRect({0.0f, 0.0f, S, S * 0.2f}, Paint::Linear({0.0f, 0.0f}, {0.0f, S * 0.2f}, Rgba(0, 0, 0, 0.08f), Rgba(0, 0, 0, 0.0f)));
	for (size_t I = 0; I < Lines.size(); ++I)
	{
		C.Text(Lines[I], 18.0f, 62.0f + static_cast<float>(I) * 46.0f, Ts(40.0f, 700, Hex(0x1d2a6b)));
	}
}

void Poster(Canvas& C)
{
	const float W = PosterW;
	const float H = PosterH;
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(0x0b0b10), Hex(0x1a130a)));
	// Spotlit table silhouette.
	C.FillEllipse(300.0f, 520.0f, 300.0f, 300.0f, Paint::Radial({300.0f, 520.0f}, 20.0f, {300.0f, 520.0f}, 300.0f, Rgba(255, 210, 120, 0.55f), -1.0f, Color(), Rgba(255, 210, 120, 0.0f)));
	C.FillEllipse(300.0f, 600.0f, 230.0f, 80.0f, Hex(0x0d3b2a));
	const Color Foil = Hex(0xc9a44c);
	C.Text("THE GRAND CIRCUIT", 300.0f, 110.0f, Ts(30.0f, 700, Foil, Align::Center));
	C.Text("CHAMPIONSHIP", 300.0f, 190.0f, Ts(62.0f, 900, Foil, Align::Center));
	C.Text("$10,000 MAIN EVENT", 300.0f, 250.0f, Ts(44.0f, 700, Foil, Align::Center));
	C.Text("ONE TABLE. NINE SEATS. ONE CHAMPION.", 300.0f, 800.0f, Ts(22.0f, 400, Hex(0xb8a47a), Align::Center));
	// Wear: creases.
	C.StrokePolyline({{0.0f, 450.0f}, {600.0f, 430.0f}}, false, Rgba(255, 255, 255, 0.08f), 2.0f);
	C.StrokePolyline({{300.0f, 0.0f}, {310.0f, 900.0f}}, false, Rgba(255, 255, 255, 0.08f), 2.0f);
}

void NeonSign(Canvas& C)
{
	auto Glow = [&](const std::string& Text, float X, float Y, float Size, const Color& Col) {
		// Soft halo from offset copies, then a hot white core.
		for (int R = 3; R >= 1; --R)
		{
			const float D = static_cast<float>(R) * Size * 0.018f;
			const Color Halo{Col.R, Col.G, Col.B, 0.18f};
			for (int K = 0; K < 8; ++K)
			{
				const float A = static_cast<float>(K) * 0.785398f;
				C.Text(Text, X + D * std::cos(A), Y + D * std::sin(A), Ts(Size, 700, Halo, Align::Center, Baseline::Middle));
			}
		}
		C.Text(Text, X, Y, Ts(Size, 700, Col, Align::Center, Baseline::Middle));
		C.Text(Text, X, Y, Ts(Size, 700, Rgba(255, 255, 255, 0.55f), Align::Center, Baseline::Middle));
	};
	Glow("WASH & FOLD", 512.0f, 120.0f, 130.0f, Hex(0xff2e88));
	Glow("OPEN 24 HRS", 512.0f, 250.0f, 70.0f, Hex(0x35d3ff));
}

void Keyboard(Canvas& C)
{
	const float W = KeyboardW;
	const float H = KeyboardH;
	C.FillRect({0.0f, 0.0f, W, H}, Hex(0x1b1c1f));
	const std::vector<std::vector<std::string>> Rows = {
		{"esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "del"},
		{"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "bksp"},
		{"tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "\\"},
		{"caps", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "enter"},
		{"shift", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", "shift"},
		{"ctrl", "fn", "alt", " ", "alt", "<", "^v", ">"},
	};
	auto Wide = [](const std::string& K) {
		if (K == "tab") return 1.5f;
		if (K == "caps") return 1.8f;
		if (K == "enter") return 2.2f;
		if (K == "shift") return 2.4f;
		if (K == " ") return 6.2f;
		if (K == "bksp" || K == "\\") return 1.5f;
		return 1.0f;
	};
	const float RowH[6] = {34.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f};
	float Y = 20.0f;
	for (size_t Ri = 0; Ri < Rows.size(); ++Ri)
	{
		float Units = 0.0f;
		for (const std::string& K : Rows[Ri])
		{
			Units += Wide(K);
		}
		const float UnitW = (W - 60.0f) / Units;
		float X = 30.0f;
		for (const std::string& K : Rows[Ri])
		{
			const float Kw = Wide(K) * UnitW;
			const float Kh = RowH[Ri] - 8.0f;
			C.FillRoundRect({X + 3.0f, Y + 3.0f, Kw - 6.0f, Kh}, 6.0f, Hex(0x0d0e10));
			C.FillRoundRect({X + 4.0f, Y + 2.0f, Kw - 8.0f, Kh - 4.0f}, 5.0f, Hex(0x26282c));
			if (K != " ")
			{
				C.Text(K, X + 11.0f, Y + 9.0f, Ts(K.size() > 2 ? 13.0f : 18.0f, 500, Hex(0x8d9096), Align::Left, Baseline::Top));
			}
			X += Kw;
		}
		Y += RowH[Ri];
	}
}
// ------------------------------------------------------------------ the street

namespace propart_street
{
/** A coin with a clover, the Lucky Penny's mark. */
void Penny(Canvas& C, float X, float Y, float R)
{
	C.FillCircle(X, Y, R, Paint::Linear({X - R, Y - R}, {X + R, Y + R}, Hex(0xf6c58f), Hex(0x8a4a1c)));
	C.StrokeEllipse(X, Y, R * 0.84f, R * 0.84f, Rgba(107, 52, 18, 0.6f), R * 0.06f);
	for (int I = 0; I < 4; ++I)
	{
		const float A = static_cast<float>(I) * 1.5707963f + 0.785398f;
		C.FillCircle(X + std::cos(A) * R * 0.22f, Y - R * 0.06f + std::sin(A) * R * 0.22f, R * 0.2f, Hex(0x5a2a0c));
	}
	C.StrokePolyline({{X, Y + R * 0.05f}, {X + R * 0.08f, Y + R * 0.46f}}, false, Hex(0x5a2a0c), R * 0.08f, true);
}

void NeonText(Canvas& C, const std::string& Text, float X, float Y, float Size, const Color& Col, int Weight = 700)
{
	for (int R = 3; R >= 1; --R)
	{
		const float D = static_cast<float>(R) * Size * 0.02f;
		const Color Halo{Col.R, Col.G, Col.B, 0.16f};
		for (int K = 0; K < 8; ++K)
		{
			const float A = static_cast<float>(K) * 0.785398f;
			C.Text(Text, X + D * std::cos(A), Y + D * std::sin(A), Ts(Size, Weight, Halo, Align::Center, Baseline::Middle));
		}
	}
	C.Text(Text, X, Y, Ts(Size, Weight, Col, Align::Center, Baseline::Middle));
	C.Text(Text, X, Y, Ts(Size, Weight, Rgba(255, 255, 255, 0.55f), Align::Center, Baseline::Middle));
}
} // namespace propart_street

void StoreSign(Canvas& C)
{
	const float W = StoreSignW;
	const float H = StoreSignH;
	// A lightbox: warm white face, a red band, the coin.
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(0xfffaf0), Hex(0xf1e6cf)));
	C.FillRect({0.0f, H - 64.0f, W, 64.0f}, Hex(0xd7263d));
	C.FillRect({0.0f, 0.0f, W, 10.0f}, Hex(0x2b2723));
	propart_street::Penny(C, 150.0f, 122.0f, 92.0f);
	C.Text("LUCKY PENNY", 280.0f, 168.0f, Ts(150.0f, 900, Hex(0xd7263d)));
	C.Text("#212", W - 70.0f, 120.0f, Ts(64.0f, 900, Hex(0x2b2723), Align::Right));
	C.Text("FOOD \xC2\xB7 DRINKS \xC2\xB7 COFFEE \xC2\xB7 LOTTO \xC2\xB7 OPEN 24 HOURS", W * 0.5f, H - 32.0f, Ts(34.0f, 800, Hex(0xfffaf0), Align::Center, Baseline::Middle));
	// Dust and a dead tube: it's been up a while.
	C.FillRect({W * 0.72f, 12.0f, 120.0f, H - 78.0f}, Rgba(60, 50, 30, 0.08f));
}

void OpenSign(Canvas& C)
{
	C.StrokeRoundRect({20.0f, 20.0f, OpenSignW - 40.0f, OpenSignH - 40.0f}, 40.0f, Rgba(53, 211, 255, 0.85f), 6.0f);
	propart_street::NeonText(C, "OPEN", OpenSignW * 0.5f, 120.0f, 150.0f, Hex(0xff3b3b), 900);
	propart_street::NeonText(C, "24 HOURS", OpenSignW * 0.5f, 220.0f, 52.0f, Hex(0x35d3ff));
}

void StreetSign(Canvas& C, const std::string& Name, const std::string& Block)
{
	const float W = StreetSignW;
	const float H = StreetSignH;
	C.FillRoundRect({0.0f, 0.0f, W, H}, 18.0f, Hex(0x0f6b3a));
	C.StrokeRoundRect({10.0f, 10.0f, W - 20.0f, H - 20.0f}, 12.0f, Hex(0xf2f5f0), 6.0f);
	// Long names shrink to clear the block number.
	const float Fit = std::min(136.0f, 136.0f * (W - 270.0f) / std::max(1.0f, C.Measure(Name, 136.0f, 800)));
	C.Text(Name, 60.0f, H * 0.5f + Fit * 0.34f, Ts(Fit, 800, Hex(0xf2f5f0)));
	C.Text(Block, W - 50.0f, H * 0.5f - 34.0f, Ts(46.0f, 800, Hex(0xf2f5f0), Align::Right));
}

void DoorDecal(Canvas& C)
{
	const float W = DoorDecalW;
	C.FillRoundRect({0.0f, 0.0f, W, DoorDecalH}, 16.0f, Rgba(255, 255, 255, 0.92f));
	propart_street::Penny(C, W * 0.5f, 92.0f, 58.0f);
	C.Text("LUCKY PENNY", W * 0.5f, 196.0f, Ts(46.0f, 900, Hex(0xd7263d), Align::Center));
	C.Text("#212", W * 0.5f, 240.0f, Ts(30.0f, 800, Hex(0x2b2723), Align::Center));
	C.FillRect({40.0f, 268.0f, W - 80.0f, 3.0f}, Hex(0x2b2723));
	C.Text("OPEN", W * 0.5f, 330.0f, Ts(52.0f, 900, Hex(0x2b2723), Align::Center));
	C.Text("24 HOURS", W * 0.5f, 382.0f, Ts(40.0f, 900, Hex(0x2b2723), Align::Center));
	C.Text("7 DAYS A WEEK", W * 0.5f, 424.0f, Ts(26.0f, 700, Hex(0x6b6560), Align::Center));
	C.Text("NO SHIRT \xC2\xB7 NO SHOES \xC2\xB7 NO SERVICE", W * 0.5f, 484.0f, Ts(17.0f, 700, Hex(0x6b6560), Align::Center));
}

void Promo(Canvas& C, const std::string& ItemId, const std::string& Deal)
{
	const store::Item* I = store::Find(ItemId);
	const float W = PromoW;
	const float H = PromoH;
	const Color Body = I ? Hex(I->Color) : Hex(0x222222);
	const Color Label = I ? Hex(I->Accent) : Hex(0xffffff);
	C.PushClip({0.0f, 0.0f, W, H});
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W, H}, Mix(Body, Hex(0x000000), 0.2f), Mix(Body, Hex(0x000000), 0.65f)));
	// Rays behind the product.
	for (int K = 0; K < 18; ++K)
	{
		const float A0 = static_cast<float>(K) * 0.349f;
		C.FillPolygon({{W * 0.5f, H * 0.42f}, {W * 0.5f + std::cos(A0) * 900.0f, H * 0.42f + std::sin(A0) * 900.0f}, {W * 0.5f + std::cos(A0 + 0.17f) * 900.0f, H * 0.42f + std::sin(A0 + 0.17f) * 900.0f}},
			Paint(Rgba(255, 255, 255, 0.05f)));
	}
	if (I)
	{
		DrawProduct(C, *I, W * 0.5f, H * 0.42f, 420.0f);
		C.Text(I->Name, W * 0.5f, 110.0f, Ts(76.0f, 900, Label, Align::Center));
	}
	C.FillRect({0.0f, H - 230.0f, W, 230.0f}, Hex(0xffd23f));
	C.FillRect({0.0f, H - 230.0f, W, 12.0f}, Hex(0xd7263d));
	C.Text(Deal, W * 0.5f, H - 110.0f, Ts(96.0f, 900, Hex(0x1a1408), Align::Center));
	C.Text("AT YOUR LUCKY PENNY", W * 0.5f, H - 46.0f, Ts(30.0f, 800, Hex(0xd7263d), Align::Center));
	C.PopClip();
}

} // namespace props
} // namespace ui
} // namespace ss
