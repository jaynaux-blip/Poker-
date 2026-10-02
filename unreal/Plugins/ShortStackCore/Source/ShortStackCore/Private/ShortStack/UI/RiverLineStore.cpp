// GearDrop: the online store on the laptop. Product cards for everything that changes the game (screens for more
// tables, the rig and the stream gear for Kast, the apartment for energy and tilt), a "your setup" panel that shows
// the desk filling up and what it all adds up to, and an order card before anything is bought.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/StreamArt.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlstore_detail
{
const float StoreH = 962.0f; // above the taskbar
const Color StoreInk = Hex(0x111827);
const Color StoreGray = Hex(0x6b7280);
const Color StoreLine = Hex(0xe5e7eb);
const Color StoreOrange = Hex(0xff6b2c);
const Color StoreOrange2 = Hex(0xf43f5e);
const Color StoreGreen = Hex(0x16a34a);

std::string StorePrice(const gear::Item& I)
{
	return Money(I.PriceCents) + (I.Monthly ? "/mo" : "");
}

/** Five stars, filled to the rating. */
void StoreStars(Canvas& Cv, float X, float Cy, float Sz, double Rating)
{
	for (int K = 0; K < 5; ++K)
	{
		const float Fill = Nf(std::max(0.0, std::min(1.0, Rating - static_cast<double>(K))));
		const float Cx = X + Sz * 0.5f + Nf(K) * (Sz + 2.0f);
		NetStar(Cv, Cx, Cy, Sz * 0.5f, Hex(0xd1d5db));
		if (Fill > 0.0f)
		{
			Cv.PushClip({Cx - Sz * 0.5f, Cy - Sz * 0.5f, Sz * Fill, Sz});
			NetStar(Cv, Cx, Cy, Sz * 0.5f, Hex(0xf59e0b));
			Cv.PopClip();
		}
	}
}

/** The store's logo: a parcel with a drop arrow. */
void StoreLogo(Canvas& Cv, float X, float Y, float Sz)
{
	Cv.FillRoundRect({X, Y, Sz, Sz}, Sz * 0.26f, Hex(0xffffff));
	Cv.FillPolygon({{X + Sz * 0.2f, Y + Sz * 0.38f}, {X + Sz * 0.5f, Y + Sz * 0.24f}, {X + Sz * 0.8f, Y + Sz * 0.38f}, {X + Sz * 0.8f, Y + Sz * 0.74f}, {X + Sz * 0.5f, Y + Sz * 0.86f}, {X + Sz * 0.2f, Y + Sz * 0.74f}},
		Paint::Linear({X, Y}, {X + Sz, Y + Sz}, StoreOrange, StoreOrange2));
	Cv.StrokePolyline({{X + Sz * 0.5f, Y + Sz * 0.3f}, {X + Sz * 0.5f, Y + Sz * 0.66f}}, false, Hex(0xffffff), Sz * 0.08f, true);
	Cv.StrokePolyline({{X + Sz * 0.38f, Y + Sz * 0.54f}, {X + Sz * 0.5f, Y + Sz * 0.67f}, {X + Sz * 0.62f, Y + Sz * 0.54f}}, false, Hex(0xffffff), Sz * 0.08f, true);
}

/** What the next good buy would be: the cheapest item that adds the most to the stream or the tables, affordable or close. */
const gear::Item* StoreSuggestion(const Session& S)
{
	// The laptop can't stream: the next PC upgrade comes first.
	if (!S.GearFx().CanStream())
	{
		return &gear::FirstPcUpgrade();
	}
	const gear::Item* Best = nullptr;
	double BestScore = 0.0;
	for (const gear::Item& I : gear::Catalog())
	{
		if (!S.CanBuy(I.Id).empty() && S.CanBuy(I.Id) != "Not enough in the bank.")
		{
			continue;
		}
		const double Value = I.Quality * 3.0 + I.Tables * 0.5 + (I.Resolution > S.GearFx().Resolution ? 0.2 : 0.0) + I.Drain + I.Rest + I.ModBot * 0.3;
		const double Score = Value / (static_cast<double>(I.PriceCents) / 10000.0 + 0.3);
		if (Value > 0.0 && Score > BestScore)
		{
			Best = &I;
			BestScore = Score;
		}
	}
	return Best;
}
} // namespace rlstore_detail

using namespace rlstore_detail;

void RiverLine::GearDropApp(double Now)
{
	const float In = NetEase((Now - AppAt) / 0.5);
	C->FillRect({0.0f, 0.0f, NetW, StoreH}, Hex(0xf4f5f7));
	// Header.
	C->FillRect({0.0f, 0.0f, NetW, 76.0f}, Paint::Linear({0.0f, 0.0f}, {NetW, 0.0f}, StoreOrange, StoreOrange2));
	StoreLogo(*C, 26.0f, 16.0f, 44.0f);
	const float Lw = UI.Text("GearDrop", 82.0f, 50.0f, Ts(28.0f, 900, Hex(0xffffff)));
	UI.Text("delivered tonight", 92.0f + Lw, 50.0f, Ts(14.0f, 600, Hex(0xffe4d6)));
	const Rect Search{420.0f, 18.0f, 520.0f, 40.0f};
	C->FillRoundRect(Search, 20.0f, Hex(0xffffff));
	C->StrokeEllipse(Search.X + 24.0f, Search.Y + 18.0f, 7.0f, 7.0f, StoreGray, 2.0f);
	C->StrokePolyline({{Search.X + 29.0f, Search.Y + 23.0f}, {Search.X + 34.0f, Search.Y + 28.0f}}, false, StoreGray, 2.0f, true);
	UI.Text("Search monitors, mics, lights, chairs\xE2\x80\xA6", Search.X + 44.0f, Search.Y + 26.0f, Ts(15.0f, 500, Hex(0x9ca3af)));
	UI.Text("Deliver to " + S.HeroName + " \xC2\xB7 Apt 4B", NetW - 30.0f, 32.0f, Ts(13.0f, 600, Hex(0xffe4d6), Align::Right));
	UI.Text("Bank " + Money(S.BankrollCents), NetW - 30.0f, 56.0f, Ts(18.0f, 900, Hex(0xffffff), Align::Right, Baseline::Alphabetic, true));

	// Categories.
	const std::pair<int, const char*> Cats[5] = {{-1, "All"}, {0, "Rig"}, {1, "Stream"}, {2, "Home"}, {3, "Subscriptions"}};
	float Cx = 24.0f;
	for (const auto& Cat : Cats)
	{
		int Count = 0;
		for (const gear::Item& I : gear::Catalog())
		{
			Count += Cat.first < 0 || static_cast<int>(I.Cat) == Cat.first ? 1 : 0;
		}
		const std::string Label = std::string(Cat.second) + "  " + std::to_string(Count);
		const float W = UI.Measure(Label, 15.0f, 700) + 32.0f;
		const Rect Chip{Cx, 92.0f, W, 36.0f};
		const Ui::ClickState St = UI.Clickable(std::string("storecat") + Cat.second, Chip);
		if (St.Clicked)
		{
			ShowStoreCategory(Cat.first);
		}
		const bool On = StoreCat == Cat.first;
		C->FillRoundRect(Chip, 18.0f, On ? StoreInk : St.Hover ? Hex(0xe5e7eb) : Hex(0xffffff));
		if (!On)
		{
			C->StrokeRoundRect(Chip, 18.0f, StoreLine, 1.0f);
		}
		UI.Text(Cat.second, Chip.X + 16.0f, Chip.Y + 23.0f, Ts(15.0f, 700, On ? Hex(0xffffff) : StoreInk));
		UI.Text(std::to_string(Count), Chip.X + W - 16.0f, Chip.Y + 23.0f, Ts(13.0f, 700, On ? Hex(0xfdba74) : StoreGray, Align::Right));
		Cx += W + 10.0f;
	}
	UI.Text("Free same-night delivery on everything \xC2\xB7 cancel subscriptions any time", 1084.0f, 116.0f, Ts(13.0f, 600, StoreGray, Align::Right));

	// The grid.
	std::vector<const gear::Item*> Items;
	for (const gear::Item& I : gear::Catalog())
	{
		if (StoreCat < 0 || static_cast<int>(I.Cat) == StoreCat)
		{
			Items.push_back(&I);
		}
	}
	const Rect Area{16.0f, 140.0f, 1076.0f, StoreH - 148.0f};
	const float CardW = 253.0f;
	const float CardH = 338.0f;
	const float Gap = 16.0f;
	const int Cols = 4;
	const int Rows = (static_cast<int>(Items.size()) + Cols - 1) / Cols;
	const float Content = Nf(Rows) * (CardH + Gap) + 8.0f;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (UI.Hover(Area) && UI.Ptr.Wheel != 0.0f && OrderId.empty())
	{
		StoreScrollGoal -= UI.Ptr.Wheel * 120.0f;
	}
	StoreScrollGoal = std::max(0.0f, std::min(MaxScroll, StoreScrollGoal));
	StoreScroll += (StoreScrollGoal - StoreScroll) * NetFollow(Dt, 14.0);
	C->PushClip(Area);
	for (size_t K = 0; K < Items.size(); ++K)
	{
		const int Col = static_cast<int>(K) % Cols;
		const int Row = static_cast<int>(K) / Cols;
		const float Rise = (1.0f - NetEase((Now - AppAt - 0.025 * static_cast<double>(K)) / 0.45)) * 24.0f;
		const Rect R{Area.X + 8.0f + Nf(Col) * (CardW + Gap), Area.Y + 4.0f + Nf(Row) * (CardH + Gap) - StoreScroll + Rise, CardW, CardH};
		if (R.Y > Area.Y + Area.H || R.Y + R.H < Area.Y)
		{
			continue;
		}
		StoreCard(*Items[K], R, Now, static_cast<int>(K));
	}
	C->PopClip();
	if (MaxScroll > 0.0f)
	{
		const float Track = Area.H - 16.0f;
		const float Thumb = std::max(40.0f, Track * Area.H / Content);
		C->FillRoundRect({Area.X + Area.W - 4.0f, Area.Y + 8.0f + (Track - Thumb) * (StoreScroll / MaxScroll), 4.0f, Thumb}, 2.0f, Rgba(0, 0, 0, 0.18f));
	}

	// Your setup.
	SetupPanel({1104.0f, 92.0f, 472.0f, StoreH - 108.0f}, Now);

	// Delivered: the toast.
	const double Since = Now - DeliveredAt;
	if (Since < 3.5 && !Delivered.empty())
	{
		const float A = NetEase(Since / 0.3) * (1.0f - NetEase((Since - 3.0) / 0.5));
		const float Tw = UI.Measure(Delivered, 16.0f, 700) + 80.0f;
		const Rect T{NetW / 2.0f - Tw / 2.0f, 90.0f + (1.0f - A) * -20.0f, Tw, 50.0f};
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * A);
		C->GlowRoundRect(T, 25.0f, Rgba(0, 0, 0, 0.25f), 18.0f);
		C->FillRoundRect(T, 25.0f, StoreInk);
		C->FillCircle(T.X + 28.0f, T.Y + 25.0f, 13.0f, StoreGreen);
		NetCheck(*C, T.X + 28.0f, T.Y + 25.0f, 14.0f, Hex(0xffffff));
		UI.Text(Delivered, T.X + 52.0f, T.Y + 30.0f, Ts(16.0f, 700, Hex(0xffffff)));
		C->SetAlpha(A0);
	}
	if (!OrderId.empty())
	{
		OrderCard(Now);
	}
	(void)In;
}

void RiverLine::StoreCard(const gear::Item& I, const Rect& R0, double Now, int Index)
{
	const bool Owned = S.Owns(I.Id);
	const std::string Why = S.CanBuy(I.Id);
	const Ui::ClickState Card = UI.Clickable("storecard" + I.Id, R0, OrderId.empty());
	const float Lift = Card.Hover && OrderId.empty() ? 4.0f : 0.0f;
	const Rect R{R0.X, R0.Y - Lift, R0.W, R0.H};
	C->GlowRoundRect({R.X, R.Y + 4.0f, R.W, R.H}, 16.0f, Rgba(17, 24, 39, Card.Hover ? 0.16f : 0.07f), Card.Hover ? 18.0f : 10.0f);
	C->FillRoundRect(R, 16.0f, Hex(0xffffff));
	// The picture.
	const Rect Pic{R.X + 8.0f, R.Y + 8.0f, R.W - 16.0f, 160.0f};
	const Color Tint = Hex(I.Color);
	C->FillRoundRect(Pic, 12.0f, Paint::Linear({Pic.X, Pic.Y}, {Pic.X + Pic.W, Pic.Y + Pic.H}, Mix(Tint, Hex(0xffffff), 0.85f), Mix(Tint, Hex(0xffffff), 0.62f)));
	streamart::Product(*C, I.Pic, {Pic.X + 20.0f, Pic.Y + 6.0f, Pic.W - 40.0f, Pic.H - 12.0f}, I.Color, Now);
	float Px = Pic.X + 10.0f;
	if (Owned)
	{
		Px += NetPill(*C, I.Monthly ? "SUBSCRIBED" : "OWNED", Px, Pic.Y + 10.0f, StoreGreen, true, 10.5f) + 6.0f;
	}
	if (I.Monthly)
	{
		NetPill(*C, "MONTHLY", Pic.X + Pic.W - 10.0f - (UI.Measure("MONTHLY", 10.5f, 700) + 13.65f), Pic.Y + 10.0f, Hex(0x7c3aed), false, 10.5f);
	}
	else if (I.Reviews > 15000 && !Owned)
	{
		NetPill(*C, "BESTSELLER", Px, Pic.Y + 10.0f, Hex(0xea580c), true, 10.5f);
	}
	// The words.
	float Y = Pic.Y + Pic.H + 24.0f;
	NetSpaced(*C, NetUpper(I.Brand), R.X + 16.0f, Y, 10.5f, 800, StoreGray, 1.2f);
	Y += 22.0f;
	UI.Text(I.Name, R.X + 16.0f, Y, Ts(16.5f, 800, StoreInk, Align::Left, Baseline::Alphabetic, false, R.W - 32.0f));
	Y += 20.0f;
	StoreStars(*C, R.X + 16.0f, Y - 4.0f, 11.0f, I.Rating);
	UI.Text(Grouped(I.Reviews), R.X + 84.0f, Y, Ts(12.0f, 600, StoreGray));
	Y += 14.0f;
	const float Ew = std::min(R.W - 32.0f, UI.Measure(I.Effect, 12.5f, 800) + 20.0f);
	C->FillRoundRect({R.X + 16.0f, Y, Ew, 24.0f}, 8.0f, Mix(Tint, Hex(0xffffff), 0.86f));
	UI.Text(I.Effect, R.X + 26.0f, Y + 16.5f, Ts(12.5f, 800, Mix(Tint, Hex(0x000000), 0.45f), Align::Left, Baseline::Alphabetic, false, Ew - 20.0f));
	Y += 52.0f;
	const std::string Price = I.PriceCents >= 100000 ? NetMoney(I.PriceCents) : Money(I.PriceCents);
	UI.Text(Price, R.X + 16.0f, Y, Ts(24.0f, 900, StoreInk, Align::Left, Baseline::Alphabetic, true));
	if (I.Monthly)
	{
		UI.Text("/month", R.X + 20.0f + UI.Measure(Price, 24.0f, 900, true), Y, Ts(13.0f, 600, StoreGray));
	}
	// The button.
	const Rect Btn{R.X + R.W - 120.0f, Y - 26.0f, 104.0f, 36.0f};
	std::string Label = I.Monthly ? "Subscribe" : "Buy";
	Color Fill = StoreOrange;
	bool Enabled = OrderId.empty();
	if (Owned)
	{
		Label = I.Monthly ? "Manage" : "Owned";
		Fill = StoreGreen;
		Enabled = Enabled && I.Monthly;
	}
	else if (!Why.empty())
	{
		Label = Why == "Not enough in the bank." ? "Short " + NetMoney(I.PriceCents - S.BankrollCents) : Why == "You have better." ? "Have better" : "Locked";
		Enabled = false;
	}
	if (AppButton("storebuy" + I.Id, Btn, Label, Fill, Hex(0xffffff), Enabled))
	{
		OrderId = I.Id;
		AppAt = Now - 1.0; // keep the grid settled behind the card
	}
	if (!Owned && Why.rfind("Needs", 0) == 0)
	{
		UI.Text(Why, R.X + 16.0f, R.Y + R.H - 12.0f, Ts(11.5f, 600, Hex(0xb45309), Align::Left, Baseline::Alphabetic, false, R.W - 32.0f));
	}
	if (Card.Clicked && OrderId.empty())
	{
		OrderId = I.Id; // the card itself opens the details
	}
	(void)Index;
}

void RiverLine::SetupPanel(const Rect& R, double Now)
{
	const gear::Effects& Fx = S.GearFx();
	C->GlowRoundRect({R.X, R.Y + 4.0f, R.W, R.H}, 18.0f, Rgba(17, 24, 39, 0.07f), 12.0f);
	C->FillRoundRect(R, 18.0f, Hex(0xffffff));
	UI.Text("Your setup", R.X + 24.0f, R.Y + 40.0f, Ts(21.0f, 900, StoreInk));
	int Count = 0;
	for (const auto& G : S.Gear)
	{
		Count += gear::Find(G.first) ? 1 : 0;
	}
	UI.Text(Count == 0 ? std::string("Just the laptop") : std::to_string(Count) + (Count == 1 ? " item" : " items"), R.X + R.W - 24.0f, R.Y + 40.0f, Ts(13.0f, 700, StoreGray, Align::Right));
	const Rect Pic{R.X + 16.0f, R.Y + 58.0f, R.W - 32.0f, 250.0f};
	C->PushClip(Pic);
	streamart::Desk(*C, Pic, S.Gear, S.Streaming(), Now);
	C->PopClip();
	C->StrokeRoundRect(Pic, 2.0f, StoreLine, 1.0f);

	// What it adds up to.
	struct Stat
	{
		std::string Label;
		std::string Value;
		Color Col;
	};
	const auto Pct = [](double V) { return std::to_string(static_cast<int>(std::round(V * 100.0))) + "%"; };
	std::vector<Stat> Stats;
	Stats.push_back({"Tables at once", std::to_string(std::min(Fx.Tables, Session::TableLimit)) + " of " + std::to_string(Session::TableLimit), Fx.Tables >= Session::TableLimit ? StoreGreen : StoreInk});
	Stats.push_back({"Stream resolution", gear::ResolutionLabel(Fx.Resolution), Fx.Resolution >= 1080 ? StoreGreen : Fx.Resolution >= 720 ? StoreInk : Hex(0xdc2626)});
	Stats.push_back({"Tables on stream", std::to_string(Fx.StreamTables) + (Fx.StreamTables == 1 ? " table smooth" : " tables smooth"), StoreInk});
	Stats.push_back({"Fatigue", Fx.Drain > 0.0 ? "\xE2\x88\x92" + Pct(Fx.Drain) : std::string("normal"), Fx.Drain > 0.0 ? StoreGreen : StoreGray});
	Stats.push_back({"Sleep", Fx.Rest > 0.0 ? "+" + Pct(Fx.Rest) + " energy" : std::string("normal"), Fx.Rest > 0.0 ? StoreGreen : StoreGray});
	Stats.push_back({"Tilt recovery", Fx.Calm > 0.0 ? "+" + Pct(Fx.Calm) + " faster" : std::string("normal"), Fx.Calm > 0.0 ? StoreGreen : StoreGray});
	Stats.push_back({"Chat filter", Fx.ModBot > 0.0 ? "catches " + Pct(Fx.ModBot) : std::string("none"), Fx.ModBot > 0.0 ? StoreGreen : StoreGray});
	Stats.push_back({"Subscriptions", Fx.MonthlyCents > 0 ? Money(Fx.MonthlyCents) + "/mo" : std::string("none"), StoreInk});
	float Y = Pic.Y + Pic.H + 26.0f;
	// Production value: the number Kast cares about.
	UI.Text("Stream production value", R.X + 24.0f, Y, Ts(14.0f, 700, StoreInk));
	const double Q = Fx.Quality;
	UI.Text(Pct(Q), R.X + R.W - 24.0f, Y, Ts(16.0f, 900, Q >= 0.6 ? StoreGreen : Q >= 0.25 ? Hex(0xea580c) : Hex(0xdc2626), Align::Right, Baseline::Alphabetic, true));
	Y += 12.0f;
	C->FillRoundRect({R.X + 24.0f, Y, R.W - 48.0f, 10.0f}, 5.0f, Hex(0xf3f4f6));
	C->FillRoundRect({R.X + 24.0f, Y, std::max(10.0f, (R.W - 48.0f) * Nf(Q)), 10.0f}, 5.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(kast::Violet), Hex(kast::Lime)));
	Y += 34.0f;
	for (size_t K = 0; K < Stats.size(); ++K)
	{
		const float Sx = R.X + 24.0f + Nf(K % 2) * (R.W - 48.0f) / 2.0f;
		const float Sy = Y + Nf(K / 2) * 46.0f;
		UI.Text(Stats[K].Label, Sx, Sy, Ts(12.0f, 600, StoreGray));
		UI.Text(Stats[K].Value, Sx, Sy + 20.0f, Ts(15.5f, 800, Stats[K].Col, Align::Left, Baseline::Alphabetic, false, (R.W - 64.0f) / 2.0f));
	}
	Y += 4.0f * 46.0f + 8.0f;
	C->FillRect({R.X + 24.0f, Y, R.W - 48.0f, 1.0f}, StoreLine);
	Y += 26.0f;
	// The next thing worth buying.
	if (const gear::Item* Next = StoreSuggestion(S))
	{
		NetSpaced(*C, "RECOMMENDED NEXT", R.X + 24.0f, Y, 10.5f, 900, StoreOrange, 1.4f);
		const Rect Row{R.X + 16.0f, Y + 10.0f, R.W - 32.0f, 70.0f};
		const Ui::ClickState St = UI.Clickable("storenext", Row, OrderId.empty());
		if (St.Hover)
		{
			C->FillRoundRect(Row, 12.0f, Hex(0xfff7ed));
		}
		C->FillRoundRect({Row.X + 8.0f, Row.Y + 8.0f, 54.0f, 54.0f}, 10.0f, Mix(Hex(Next->Color), Hex(0xffffff), 0.75f));
		streamart::Product(*C, Next->Pic, {Row.X + 10.0f, Row.Y + 10.0f, 50.0f, 50.0f}, Next->Color, Now);
		UI.Text(Next->Name, Row.X + 76.0f, Row.Y + 30.0f, Ts(15.0f, 800, StoreInk, Align::Left, Baseline::Alphabetic, false, Row.W - 180.0f));
		UI.Text(Next->Effect, Row.X + 76.0f, Row.Y + 50.0f, Ts(12.5f, 600, StoreGray, Align::Left, Baseline::Alphabetic, false, Row.W - 180.0f));
		UI.Text(StorePrice(*Next), Row.X + Row.W - 12.0f, Row.Y + 40.0f, Ts(16.0f, 900, StoreInk, Align::Right, Baseline::Alphabetic, true));
		if (St.Clicked)
		{
			OrderId = Next->Id;
		}
		Y += 96.0f;
	}
	else
	{
		UI.Text("Everything worth having is on the desk.", R.X + 24.0f, Y + 6.0f, Ts(14.0f, 600, StoreGray));
		Y += 40.0f;
	}
	// Subscriptions running.
	for (const auto& G : S.Gear)
	{
		const gear::Item* I = gear::Find(G.first);
		if (!I || !I->Monthly || Y > R.Y + R.H - 30.0f)
		{
			continue;
		}
		UI.Text(I->Name, R.X + 24.0f, Y, Ts(13.5f, 700, StoreInk, Align::Left, Baseline::Alphabetic, false, 220.0f));
		UI.Text("renews " + std::string(net::WeekdayName(net::DayOf(G.second), true)) + " " + net::DateLabel(net::DayOf(G.second)), R.X + 250.0f, Y, Ts(12.0f, 600, StoreGray));
		const Rect Cancel{R.X + R.W - 92.0f, Y - 17.0f, 68.0f, 24.0f};
		const Ui::ClickState St = UI.Clickable("storecancel" + I->Id, Cancel, OrderId.empty());
		C->FillRoundRect(Cancel, 12.0f, St.Hover ? Hex(0xfee2e2) : Hex(0xf3f4f6));
		UI.Text("Cancel", Cancel.X + Cancel.W / 2.0f, Cancel.Y + 16.5f, Ts(12.0f, 700, St.Hover ? Hex(0xdc2626) : StoreGray, Align::Center));
		if (St.Clicked)
		{
			S.Cancel(I->Id);
		}
		Y += 30.0f;
	}
}

void RiverLine::OrderCard(double Now)
{
	const gear::Item* I = gear::Find(OrderId);
	if (!I)
	{
		OrderId.clear();
		return;
	}
	C->FillRect({0.0f, 0.0f, NetW, StoreH}, Rgba(17, 24, 39, 0.55f));
	const Rect R{NetW / 2.0f - 380.0f, 190.0f, 760.0f, 520.0f};
	C->GlowRoundRect(R, 22.0f, Rgba(0, 0, 0, 0.35f), 30.0f);
	C->FillRoundRect(R, 22.0f, Hex(0xffffff));
	const Rect Pic{R.X + 24.0f, R.Y + 24.0f, 300.0f, R.H - 48.0f};
	C->FillRoundRect(Pic, 16.0f, Paint::Linear({Pic.X, Pic.Y}, {Pic.X + Pic.W, Pic.Y + Pic.H}, Mix(Hex(I->Color), Hex(0xffffff), 0.85f), Mix(Hex(I->Color), Hex(0xffffff), 0.55f)));
	streamart::Product(*C, I->Pic, {Pic.X + 20.0f, Pic.Y + 80.0f, Pic.W - 40.0f, Pic.W - 40.0f}, I->Color, Now);
	const float X = Pic.X + Pic.W + 32.0f;
	const float W = R.X + R.W - 32.0f - X;
	float Y = R.Y + 56.0f;
	NetSpaced(*C, NetUpper(I->Brand) + " \xC2\xB7 " + NetUpper(gear::CategoryName(I->Cat)), X, Y, 11.0f, 800, StoreGray, 1.4f);
	Y += 38.0f;
	UI.Text(I->Name, X, Y, Ts(26.0f, 900, StoreInk, Align::Left, Baseline::Alphabetic, false, W));
	Y += 26.0f;
	StoreStars(*C, X, Y - 5.0f, 13.0f, I->Rating);
	UI.Text(Fixed(I->Rating, 1) + " \xC2\xB7 " + Grouped(I->Reviews) + " reviews", X + 80.0f, Y, Ts(13.0f, 600, StoreGray));
	Y += 34.0f;
	Y = NetParagraph(*C, I->Blurb, X, Y, W, 15.5f, 500, Hex(0x374151), 22.0f, 4) + 14.0f;
	const float Ew = UI.Measure(I->Effect, 14.0f, 800) + 24.0f;
	C->FillRoundRect({X, Y, Ew, 30.0f}, 9.0f, Mix(Hex(I->Color), Hex(0xffffff), 0.85f));
	UI.Text(I->Effect, X + 12.0f, Y + 20.0f, Ts(14.0f, 800, Mix(Hex(I->Color), Hex(0x000000), 0.45f)));
	Y += 64.0f;
	UI.Text(Money(I->PriceCents) + (I->Monthly ? " a month" : ""), X, Y, Ts(32.0f, 900, StoreInk, Align::Left, Baseline::Alphabetic, true));
	const Chips After = S.BankrollCents - I->PriceCents;
	UI.Text("Bank after: " + Money(std::max<Chips>(0, After)) + (I->Monthly ? " \xC2\xB7 renews every 30 days" : " \xC2\xB7 arrives tonight"), X, Y + 26.0f, Ts(13.0f, 600, After < 0 ? Hex(0xdc2626) : StoreGray));
	const bool Owned = S.Owns(I->Id);
	const std::string Why = S.CanBuy(I->Id);
	const Rect Place{X, R.Y + R.H - 82.0f, W * 0.6f, 52.0f};
	if (Owned && I->Monthly)
	{
		if (AppButton("storeorder", Place, "End subscription", Hex(0xdc2626), Hex(0xffffff), true))
		{
			S.Cancel(I->Id);
			OrderId.clear();
		}
	}
	else if (AppButton("storeorder", Place, Why.empty() ? (I->Monthly ? "Subscribe" : "Place order") : Why, StoreOrange, Hex(0xffffff), Why.empty(), Why.empty() ? Money(I->PriceCents) : std::string()))
	{
		const std::string Result = S.Buy(I->Id);
		if (Result.empty())
		{
			Delivered = I->Monthly ? I->Name + " is active." : "Delivered: " + I->Name + " is on your desk.";
			DeliveredAt = Now;
		}
		OrderId.clear();
	}
	const Rect Back{Place.X + Place.W + 12.0f, Place.Y, W - Place.W - 12.0f, Place.H};
	const Ui::ClickState St = UI.Clickable("storeback", Back);
	C->FillRoundRect(Back, 12.0f, St.Hover ? Hex(0xe5e7eb) : Hex(0xf3f4f6));
	UI.Text("Keep shopping", Back.X + Back.W / 2.0f, Back.Y + 31.0f, Ts(15.0f, 700, StoreInk, Align::Center));
	if (St.Clicked)
	{
		OrderId.clear();
	}
}
} // namespace ui
} // namespace ss
