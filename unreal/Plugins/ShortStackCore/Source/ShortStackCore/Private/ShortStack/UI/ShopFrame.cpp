// Northside's shop screens: the shared frame and MakeShopScreen (the realism update's spec, §7.4). The header, the
// keeper's bubble, the tabs, the grid of cards, the receipt and its stamps, the tip row, the main button and the
// footer; each page fills in what it sells (ShopFrame.h).
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace shopframe
{
// ------------------------------------------------------------------ small things

std::string Money2(Chips Cents)
{
	return Money(Cents);
}

std::string Price(Chips Cents)
{
	std::string M = Money(Cents);
	if (Cents % 100 == 0 && M.size() > 3 && M.compare(M.size() - 3, 3, ".00") == 0)
	{
		M.resize(M.size() - 3);
	}
	return M;
}

std::string Upper(std::string Text)
{
	for (char& Ch : Text)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return Text;
}

std::string MinutesText(int Minutes)
{
	if (Minutes < 60)
	{
		return std::to_string(std::max(0, Minutes)) + " MIN";
	}
	const int Hours = Minutes / 60;
	const int Rest = Minutes % 60;
	return std::to_string(Hours) + " HR" + (Rest > 0 ? " " + std::to_string(Rest) + " MIN" : std::string());
}

std::string Stamped(double World)
{
	const int Day = net::DayOf(World);
	return std::string(net::WeekdayName(Day)) + " " + net::DateLabel(Day) + "  " + net::TimeLabel(World);
}

uint64_t Hash(const std::string& Text)
{
	uint64_t H = 1469598103934665603ull;
	for (const char Ch : Text)
	{
		H ^= static_cast<uint8_t>(Ch);
		H *= 1099511628211ull;
	}
	return H;
}

namespace
{
struct VenueFace
{
	const char* Id;
	const char* Name;
	const char* Address;
	uint32_t Color;
};

/** The enterable shops as the spec names them (§2.3, §5): used when the venue table doesn't say. */
const VenueFace Faces[] = {
	{"barber", "Fifth St. Barbers", "1818 Fifth St.", 0xd7263d},
	{"thrift", "Second Chance Thrift", "220 Market St.", 0x5fae6b},
	{"gym", "IronHouse Gym", "246 Market St.", 0xf28a3a},
	{"diner", "Ruby's", "201 Market St.", 0xff3b5c},
	{"pawn", "Northside Pawn & Loan", "232 Market St.", 0xf2c14e},
	{"wardrobe", "Your Closet", "", 0x27d3c3},
};

const VenueFace* FaceOf(const std::string& Venue)
{
	for (const VenueFace& F : Faces)
	{
		if (Venue == F.Id)
		{
			return &F;
		}
	}
	return nullptr;
}

/** The keepers, as the spec casts them (§5.0, §6.1), for when the town has nobody on its books. */
struct KeeperFace
{
	const char* Id;
	const char* Name;
	const char* Role;
};
const KeeperFace Keepers[] = {
	{"lonnie", "Lonnie", "Barber"},
	{"bea", "Bea", "Volunteer"},
	{"rosa", "Rosa", "Trainer"},
	{"ruby", "Ruby", "Owner"},
	{"marlene", "Marlene", "Night waitress"},
	{"augie", "Augie", "Pawnbroker"},
	{"benny", "Benny", "Night clerk"},
	{"ray", "Ray", "Owner"},
};

const KeeperFace* KeeperOf(const std::string& Id)
{
	for (const KeeperFace& K : Keepers)
	{
		if (Id == K.Id)
		{
			return &K;
		}
	}
	return nullptr;
}

const char* RoleWord(town::Job J)
{
	switch (J)
	{
	case town::Job::Clerk: return "Clerk";
	case town::Job::Barber: return "Barber";
	case town::Job::Thrift: return "Volunteer";
	case town::Job::Trainer: return "Trainer";
	case town::Job::Waitress: return "Waitress";
	case town::Job::Cook: return "Cook";
	case town::Job::Pawnbroker: return "Pawnbroker";
	case town::Job::Host: return "Host";
	case town::Job::Attendant: return "Attendant";
	default: return "";
	}
}

/**
 * The keepers' own words, for any moment the town's KeeperLine leaves empty. The lines the spec gives each of them
 * (§5.1-§5.8) come first; "{weekday}" is today's name, "{lot114}" what Unit 114 really brought him (shop::AuctionWords).
 */
struct Words
{
	const char* Keeper;
	const char* Moment;
	std::vector<const char*> Lines;
};

const std::vector<Words>& AllWords()
{
	static const std::vector<Words> W = {
		{"lonnie", "greet", {"Sit down, sit down. Same as last time, or you finally gonna let me fix that?", "Dee was in Thursday. Says you fold too much. Says it like it's a compliment.", "Chair's warm. Sit."}},
		{"lonnie", "browse", {"Take your time. The hair's not going anywhere. Well. Some of it.", "Thirty-one years on Fifth. Show me a picture, I'll tell you if it's a lie."}},
		{"lonnie", "pick:color", {"Grey's coming in. Distinguished. Or forty-five bucks, your call."}},
		{"lonnie", "pick", {"That I can do. Hold still when I get to the ears.", "Good. That suits the face."}},
		{"lonnie", "grow", {"Can't put hair back on, kid. Give it three weeks."}},
		{"lonnie", "pay", {"Lean back. Chin down.", "Head still. I've only got the one good pair of scissors."}},
		{"lonnie", "declined", {"Card's no good, kid. Cash works. So does next week."}},
		{"lonnie", "tip", {"Appreciate it. Tell your friends. Not the loud one."}},
		{"lonnie", "stiff", {"Mm. Next time, then."}},
		{"lonnie", "leave", {"Don't let anybody else touch that."}},
		{"lonnie", "closing", {"Broom's out. That means you."}},
		{"bea", "greet", {"Yellow tags are half off Saturday. Today's {weekday}. So, no."}},
		{"bea", "saturday", {"Yellow tags are half off today. Don't tell the register. That's me."}},
		{"bea", "browse", {"That jacket came in a box from a man who moved to Florida. You wear it better.", "New racks Thursday. The good stuff's gone by Friday."}},
		{"bea", "try", {"Booth's on the left. Curtain sticks. Pull, don't yank."}},
		{"bea", "pick", {"Good eye.", "That one's got some life left in it."}},
		{"bea", "pay", {"Wear it in good health, sweetheart."}},
		{"bea", "declined", {"Machine says no, sweetheart. Put something back and we'll try again."}},
		{"bea", "donate", {"God bless. And you're writing that off, sweetheart."}},
		{"bea", "leave", {"Come back Thursday. New racks."}},
		{"bea", "closing", {"Lights go off in ten minutes, decided or not."}},
		{"rosa", "greet", {"Towel's free. The shake isn't."}},
		{"rosa", "door", {"Fob works. Lights work. I don't, till six."}},
		{"rosa", "browse", {"You look like someone who sleeps in a chair."}},
		{"rosa", "train", {"Four in, hold four, four out. Works in the ring. Works when some guy shoves on the river."}},
		{"rosa", "pick", {"Good. Don't bounce it."}},
		{"rosa", "pay", {"Go. Clock's running."}},
		{"rosa", "declined", {"Card bounced. The bag doesn't care. I do."}},
		{"rosa", "leave", {"Drink water. Real water."}},
		{"rosa", "closing", {"Desk closes in ten. The fob works after."}},
		{"ruby", "coffee", {"Fresh pot. Holler when you're low."}},
		{"ruby", "sit", {"Stay as long as you like. The stool doesn't charge rent."}},
		{"ruby", "greet", {"Sit anywhere that isn't Sal's."}},
		{"ruby", "browse", {"Thirty-one years and the grill's the only thing in here that never needed a day off."}},
		{"ruby", "pick", {"Good choice. Comes with toast whether you want it or not."}},
		{"ruby", "pay", {"Eat it while it's hot. That's the whole secret."}},
		{"ruby", "declined", {"Card says no. Water's free, and so's the advice."}},
		{"ruby", "tip", {"That's kind of you."}},
		{"ruby", "stiff", {"Mm-hm. Come again."}},
		{"ruby", "leave", {"Come back hungry."}},
		{"marlene", "coffee", {"Coffee's on. I'll keep it coming, hon."}},
		{"marlene", "sit", {"Take the booth as long as you like. Nobody's waiting on it."}},
		{"marlene", "greet", {"Coffee, hon? You look like you lost a flip."}},
		{"marlene", "browse", {"Pie's cherry. It's always cherry. Don't ask about the apple."}},
		{"marlene", "pick", {"Coming right up, hon."}},
		{"marlene", "pay", {"Coming right up, hon."}},
		{"marlene", "declined", {"Card's being shy tonight, hon. Want to try something smaller?"}},
		{"marlene", "tip", {"You're a doll."}},
		{"marlene", "stiff", {"Mm-hm. Come again."}},
		{"marlene", "leave", {"Mind the step, it's wet."}},
		{"augie", "greet", {"Set it on the glass. Let me look."}},
		{"augie", "browse", {"Guitars I don't sell to card players. You'd hock it back to me inside a month."}},
		{"augie", "appraise", {"Set it on the glass. Let me look."}},
		{"augie", "guitars", {"Every one of those has a ticket on its neck. Somebody's coming back for each of them. Most won't."}},
		{"augie", "appraise:award", {"Forty years, six of these have come over this glass. Two came back for 'em."}},
		{"augie", "loan", {"Thirty days. The ticket's your receipt. Don't lose it."}},
		{"augie", "redeem", {"Look at that. It came home."}},
		{"augie", "renew", {"Another thirty. Same ticket, new date."}},
		{"augie", "forfeit", {"It was yours. Thirty days and a week. That's the deal."}},
		{"augie", "pay", {"All sales final. It's on the sign."}},
		{"augie", "sell", {"Count it. Everybody counts it."}},
		{"augie", "declined", {"Ticket says one thing, your card says another. Come back when they agree."}},
		{"augie", "leave", {"Buzzer's on your way out."}},
		{"augie", "closing", {"Buzzer's off in ten. Ticket or cash, make it quick."}},
		{"augie", "appraise:lot114", {"Guy bought a unit on Ninth. Brought me {lot114}. One-fourteen. Mean anything to you?"}},
	};
	return W;
}
} // namespace

Color Accent(const std::string& Venue)
{
	const shop::Venue* V = shop::FindVenue(Venue);
	if (V && V->Color != 0)
	{
		return Hex(V->Color);
	}
	const VenueFace* F = FaceOf(Venue);
	return F ? Hex(F->Color) : frontend_detail::MenuTeal;
}

void VenueMark(Canvas& C, const std::string& Venue, float X, float Y, float R, double Time)
{
	const Color Acc = Accent(Venue);
	const float A = C.GetAlpha();
	// A soft halo, as the counter's coin has.
	C.FillCircle(X, Y, R * 1.8f, Paint::Radial({X, Y}, R * 0.8f, {X, Y}, R * 1.8f, Fade(Acc, 0.22f * A), 0.4f, Fade(Acc, 0.07f * A), Fade(Acc, 0.0f)));
	if (Venue == "barber")
	{
		// The pole: a glass tube with its stripes turning, chrome caps.
		const Rect Tube{X - R * 0.32f, Y - R * 0.86f, R * 0.64f, R * 1.72f};
		C.FillRoundRect(Tube, R * 0.3f, Paint(Hex(0xf4f1ea)));
		C.PushClip(Tube);
		const float Turn = static_cast<float>(std::fmod(Time * 0.6, 1.0)) * R * 0.9f;
		for (int K = -3; K < 5; ++K)
		{
			const float Y0 = Tube.Y + Cf(K) * R * 0.45f + Turn;
			const Color Stripe = K % 2 == 0 ? Hex(0xd7263d) : Hex(0x1f4fa8);
			C.FillPolygon({{Tube.X, Y0}, {Tube.X + Tube.W, Y0 - R * 0.36f}, {Tube.X + Tube.W, Y0 - R * 0.2f}, {Tube.X, Y0 + R * 0.16f}}, Paint(Stripe));
		}
		C.FillRect({Tube.X, Tube.Y, Tube.W * 0.32f, Tube.H}, Paint(Rgba(255, 255, 255, 0.28f)));
		C.PopClip();
		C.FillRoundRect({X - R * 0.42f, Y - R * 1.02f, R * 0.84f, R * 0.2f}, R * 0.08f, Paint::Linear({X - R * 0.42f, 0.0f}, {X + R * 0.42f, 0.0f}, Hex(0xe9edf2), Hex(0x7d848e)));
		C.FillRoundRect({X - R * 0.42f, Y + R * 0.82f, R * 0.84f, R * 0.2f}, R * 0.08f, Paint::Linear({X - R * 0.42f, 0.0f}, {X + R * 0.42f, 0.0f}, Hex(0xe9edf2), Hex(0x7d848e)));
		C.FillCircle(X, Y - R * 1.14f, R * 0.16f, Paint(Hex(0xf6f1e6)));
	}
	else if (Venue == "thrift" || Venue == "wardrobe")
	{
		// A wire hanger, a tag on its hook.
		const Color Wire = Venue == "thrift" ? Hex(0xd9dde3) : Acc;
		C.StrokePolyline({{X, Y - R * 0.5f}, {X, Y - R * 0.66f}}, false, Wire, R * 0.09f, true);
		C.StrokeArc(X + R * 0.12f, Y - R * 0.72f, R * 0.14f, 3.14159f, 6.28318f, Wire, R * 0.09f, true);
		C.StrokePolyline({{X, Y - R * 0.5f}, {X - R * 0.95f, Y + R * 0.3f}, {X + R * 0.95f, Y + R * 0.3f}, {X, Y - R * 0.5f}}, false, Wire, R * 0.1f, true);
		if (Venue == "thrift")
		{
			C.FillRoundRect({X + R * 0.2f, Y - R * 0.38f, R * 0.46f, R * 0.32f}, R * 0.06f, Paint(Hex(0xffd23f)));
			C.FillCircle(X + R * 0.28f, Y - R * 0.22f, R * 0.05f, Paint(Hex(0x2b2723)));
		}
		else
		{
			C.FillPolygon({{X - R * 0.62f, Y + R * 0.34f}, {X + R * 0.62f, Y + R * 0.34f}, {X + R * 0.5f, Y + R * 0.98f}, {X - R * 0.5f, Y + R * 0.98f}}, Paint(Fade(Acc, 0.5f)));
		}
	}
	else if (Venue == "gym")
	{
		// A dumbbell, plates and the knurled bar.
		C.FillRoundRect({X - R * 0.62f, Y - R * 0.09f, R * 1.24f, R * 0.18f}, R * 0.05f, Paint(Hex(0xb9c0c9)));
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Cf(Side);
			C.FillRoundRect({X + S * R * 0.62f - R * 0.17f, Y - R * 0.62f, R * 0.34f, R * 1.24f}, R * 0.08f, Paint::Linear({0.0f, Y - R}, {0.0f, Y + R}, Hex(0x3a3d44), Hex(0x15161a)));
			C.FillRoundRect({X + S * R * 0.92f - R * 0.12f, Y - R * 0.42f, R * 0.24f, R * 0.84f}, R * 0.06f, Paint(Hex(0x2a2c31)));
			C.FillRect({X + S * R * 0.62f - R * 0.17f, Y - R * 0.62f, R * 0.06f, R * 1.24f}, Paint(Fade(Acc, 0.8f)));
		}
	}
	else if (Venue == "diner")
	{
		// A diner mug, thick-walled, and its steam.
		C.FillRoundRect({X - R * 0.55f, Y - R * 0.4f, R * 0.95f, R * 0.95f}, R * 0.16f, Paint::Linear({X - R * 0.55f, 0.0f}, {X + R * 0.4f, 0.0f}, Hex(0xfbf7ee), Hex(0xc9c2b4)));
		C.StrokeArc(X + R * 0.42f, Y + R * 0.06f, R * 0.24f, -1.57f, 1.57f, Hex(0xd9d3c6), R * 0.12f, true);
		C.FillRect({X - R * 0.55f, Y - R * 0.18f, R * 0.95f, R * 0.12f}, Paint(Acc));
		for (int K = -1; K <= 1; ++K)
		{
			const float Wob = static_cast<float>(std::sin(Time * 2.0 + Cf(K))) * R * 0.06f;
			C.StrokePolyline({{X - R * 0.08f + Cf(K) * R * 0.24f, Y - R * 0.52f}, {X - R * 0.16f + Cf(K) * R * 0.24f + Wob, Y - R * 0.74f}, {X - R * 0.06f + Cf(K) * R * 0.24f, Y - R * 0.96f}}, false,
				Rgba(255, 255, 255, 0.4f), R * 0.06f, true);
		}
	}
	else if (Venue == "pawn")
	{
		// Three gold balls hung from a bracket.
		C.StrokePolyline({{X - R * 0.7f, Y - R * 0.86f}, {X + R * 0.7f, Y - R * 0.86f}}, false, Hex(0x8a6d2a), R * 0.08f, true);
		const Vec2 Balls[3] = {{X - R * 0.45f, Y - R * 0.12f}, {X + R * 0.45f, Y - R * 0.12f}, {X, Y + R * 0.58f}};
		for (const Vec2& B : Balls)
		{
			C.StrokePolyline({{B.X, Y - R * 0.86f}, {B.X, B.Y - R * 0.3f}}, false, Hex(0x8a6d2a), R * 0.04f);
			C.FillCircle(B.X, B.Y, R * 0.34f, Paint::Radial({B.X - R * 0.1f, B.Y - R * 0.12f}, 0.0f, {B.X, B.Y}, R * 0.34f, Hex(0xfff1b8), 0.45f, Hex(0xe0b23c), Hex(0x8a6514)));
		}
	}
	else
	{
		C.FillCircle(X, Y, R * 0.6f, Paint(Acc));
	}
}

void NeedIcon(Canvas& C, int Which, float X, float Y, float S, const Color& Col)
{
	if (Which == 0)
	{
		C.StrokePolyline({{X - S * 0.18f, Y - S * 0.42f}, {X - S * 0.18f, Y - S * 0.05f}}, false, Col, S * 0.1f, true);
		C.StrokePolyline({{X, Y - S * 0.42f}, {X, Y + S * 0.42f}}, false, Col, S * 0.12f, true);
		C.StrokePolyline({{X + S * 0.18f, Y - S * 0.42f}, {X + S * 0.18f, Y - S * 0.05f}}, false, Col, S * 0.1f, true);
		C.StrokePolyline({{X - S * 0.18f, Y - S * 0.05f}, {X, Y + S * 0.05f}, {X + S * 0.18f, Y - S * 0.05f}}, false, Col, S * 0.1f, true);
	}
	else if (Which == 1)
	{
		C.FillPolygon({{X, Y - S * 0.46f}, {X + S * 0.28f, Y + S * 0.06f}, {X, Y + S * 0.18f}, {X - S * 0.28f, Y + S * 0.06f}}, Paint(Col));
		C.FillCircle(X, Y + S * 0.14f, S * 0.29f, Paint(Col));
	}
	else
	{
		C.FillPolygon({{X + S * 0.1f, Y - S * 0.48f}, {X - S * 0.26f, Y + S * 0.06f}, {X - S * 0.02f, Y + S * 0.06f}, {X - S * 0.1f, Y + S * 0.48f}, {X + S * 0.26f, Y - S * 0.06f}, {X + S * 0.02f, Y - S * 0.06f}},
			Paint(Col));
	}
}

float NeedPills(Canvas& C, double Hunger, double Thirst, double Energy, float X, float Y, float MaxW)
{
	using frontend_detail::MenuWarn;
	const std::pair<int, double> Effects[3] = {{0, Hunger}, {1, Thirst}, {2, Energy}};
	float Ex = X;
	for (const std::pair<int, double>& E : Effects)
	{
		if (std::fabs(E.second) < 0.5)
		{
			continue;
		}
		const bool Good = E.second > 0.0;
		const uint32_t Hue = E.first == 0 ? 0xff9f43u : E.first == 1 ? 0x4fb8ffu : 0xf2c14eu;
		const std::string Txt = (Good ? "+" : "\xE2\x88\x92") + std::to_string(static_cast<int>(std::lround(std::fabs(E.second))));
		const float Pw = C.Measure(Txt, 13.0f, 800) + 34.0f;
		if (Ex + Pw > X + MaxW)
		{
			break;
		}
		C.FillRoundRect({Ex, Y, Pw, 24.0f}, 12.0f, Paint(Fade(Good ? Hex(Hue) : MenuWarn, 0.14f)));
		NeedIcon(C, E.first, Ex + 13.0f, Y + 12.0f, 13.0f, Good ? Hex(Hue) : MenuWarn);
		C.Text(Txt, Ex + 24.0f, Y + 17.0f, Ts(13.0f, 800, Good ? Hex(Hue) : MenuWarn));
		Ex += Pw + 6.0f;
	}
	return Ex - X;
}

float BadgeWidth(Canvas& C, const std::string& Text)
{
	return frontend_detail::TrackedWidth(C, Text, 10.0f, 800, 1.6f) + 16.0f;
}

std::string BadgeText(Canvas& C, const std::string& Why, float MaxW, const char* Fallback)
{
	std::string Text = Upper(Why);
	while (!Text.empty() && (Text.back() == '.' || Text.back() == ' '))
	{
		Text.pop_back();
	}
	return !Text.empty() && BadgeWidth(C, Text) <= MaxW ? Text : std::string(Fallback);
}

float Badge(Canvas& C, const std::string& Text, float X, float Y, const Color& Col, bool Solid)
{
	const float Bw = BadgeWidth(C, Text);
	C.FillRoundRect({X, Y, Bw, 20.0f}, 10.0f, Paint(Fade(Col, Solid ? 0.9f : 0.16f)));
	if (!Solid)
	{
		C.StrokeRoundRect({X + 0.5f, Y + 0.5f, Bw - 1.0f, 19.0f}, 10.0f, Fade(Col, 0.55f), 1.0f);
	}
	frontend_detail::TrackedText(C, Text, X + 8.0f, Y + 14.0f, 10.0f, 800, Solid ? Hex(0x0b0d12) : Col, 1.6f);
	return Bw;
}

hero::Look Dressed(const hero::Look& L, const wardrobe::Closet& Closet)
{
	hero::Look Out = L;
	if (Closet.Seeded)
	{
		wardrobe::ApplyTo(Closet, Out);
	}
	return Out;
}

Session::ShopOrder Order(const std::string& Venue, const std::vector<std::string>& Offers, int TipPercent)
{
	Session::ShopOrder O;
	O.Venue = Venue;
	O.Offers = Offers;
	O.TipPercent = TipPercent;
	return O;
}

// ------------------------------------------------------------------ the page

using namespace frontend_detail;

Page::Page(Session& InSession, std::string InVenue) : S(InSession), Venue(std::move(InVenue))
{
}

void Page::Open(double Now, const std::string& PageId)
{
	Shown = true;
	Left = false;
	OpenedAt = Now;
	OpenedOn = PageId;
	Tab = 0;
	TabAt = Now;
	Sel = 0;
	SelAt = Now;
	FirstRow = 0;
	Tipping = false;
	// Every visit's tip row starts at the house default: a stiff last time isn't remembered as this time's choice.
	TipIndex = TipChoices.empty() ? 0 : std::clamp(TipDefault, 0, static_cast<int>(TipChoices.size()) - 1);
	Previewing = false;
	StampWas = StampKind::None;
	StampAt = -10.0;
	LineAt.clear();
	PressX = PressY = -1.0f;
	ClosingSaid = false;
	Note.clear();
	Opened(Now, PageId);
	if (Note.empty())
	{
		Say(Greeting(), Now);
	}
}

void Page::Close(double Now)
{
	if (!Shown)
	{
		return;
	}
	if (Previewing)
	{
		EndPreview();
	}
	Closing(Now);
	Shown = false;
	Tipping = false;
}

bool Page::TakeLeave()
{
	const bool Was = Left;
	Left = false;
	return Was;
}

void Page::SetCaps(const HostCaps& InCaps)
{
	Caps = InCaps;
	CapsChanged();
}

std::vector<HostRequest> Page::TakeRequests()
{
	std::vector<HostRequest> Out;
	Out.swap(Requests);
	return Out;
}

void Page::WalkAway(double Now)
{
	if (!Shown)
	{
		return;
	}
	if (Seated)
	{
		HostRequest R;
		R.K = HostRequest::Stand;
		Ask(R);
	}
	Close(Now);
	Left = true;
}

void Page::Camera(const std::string& Shot)
{
	HostRequest R;
	R.K = HostRequest::Camera;
	R.Name = Shot;
	Ask(R);
}

void Page::Preview(const hero::Look& L, const wardrobe::Closet& Closet)
{
	HostRequest R;
	R.K = HostRequest::PreviewLook;
	R.Look = L;
	R.Closet = Closet;
	Ask(R);
	Previewing = true;
}

void Page::EndPreview()
{
	HostRequest R;
	R.K = HostRequest::EndPreview;
	Ask(R);
	Previewing = false;
}

void Page::Redress()
{
	HostRequest R;
	R.K = HostRequest::Redress;
	R.Closet = S.Life.Closet;
	R.Look = Dressed(S.Person.Appearance, S.Life.Closet);
	Ask(R);
}

bool Page::Started(double Now)
{
	if (!S.TimeSkip.Active)
	{
		return false;
	}
	std::string Id = S.TimeSkip.Result.ActivityId;
	if (Id.compare(0, 5, "shop:") == 0)
	{
		const size_t Colon = Id.find(':', 5);
		Id = Colon == std::string::npos ? Id.substr(5) : Id.substr(Colon + 1);
	}
	if (Id.empty())
	{
		Id = "wait";
	}
	if (Previewing)
	{
		EndPreview();
	}
	HostRequest R;
	R.K = HostRequest::BeginService;
	R.Name = Id;
	Ask(R);
	Closing(Now);
	Shown = false;
	Tipping = false;
	Left = true;
	return true;
}

void Page::Stamp(StampKind K, double Now)
{
	StampWas = K;
	StampAt = Now;
}

Page::Keeper Page::Who() const
{
	Keeper K;
	const double World = S.WorldMinutes();
	// The town's roster (whoever is on shift), or the shop's own (§5.0's keepers by their hours): nobody between the
	// gym's shifts.
	std::string Id = town::OnShift(Venue, World);
	if (Id.empty())
	{
		Id = shop::KeeperOf(Venue, World);
	}
	if (Id.empty())
	{
		return K;
	}
	K.Id = Id;
	const town::Person* P = town::FindPerson(Id);
	const KeeperFace* F = KeeperOf(Id);
	if (F)
	{
		K.Name = F->Name;
		K.Role = F->Role;
	}
	else if (P)
	{
		const size_t Space = P->Name.find(' ');
		K.Name = Space == std::string::npos ? P->Name : P->Name.substr(0, Space);
		K.Role = RoleWord(P->Role);
	}
	else
	{
		K.Name = Id;
		K.Name[0] = static_cast<char>(K.Name[0] >= 'a' && K.Name[0] <= 'z' ? K.Name[0] - 32 : K.Name[0]);
	}
	return K;
}

std::string Page::KeeperSays(const std::string& Moment) const
{
	const Keeper K = Who();
	if (K.Id.empty())
	{
		return std::string();
	}
	if (const std::shared_ptr<const town::TalkContext> Ctx = town::ContextFor(S, K.Id, Venue))
	{
		std::string Said = town::KeeperLine(*Ctx, Moment);
		if (!Said.empty())
		{
			return Said;
		}
	}
	// Their own words: the same line all visit, a different one another day or another visit.
	const double World = S.WorldMinutes();
	const auto Vt = S.Life.Visits.find(Venue);
	const int Count = Vt == S.Life.Visits.end() ? 0 : Vt->second.Count;
	const std::string Head = Moment.substr(0, Moment.find(':'));
	const Words* Best = nullptr;
	for (const Words& W : AllWords())
	{
		if (K.Id == W.Keeper && !W.Lines.empty() && (Moment == W.Moment || (!Best && Head == W.Moment)))
		{
			Best = &W;
		}
	}
	if (!Best)
	{
		return std::string();
	}
	const uint64_t Pick = Hash(K.Id + Moment + std::to_string(net::DayOf(World)) + ":" + std::to_string(Count)) % Best->Lines.size();
	std::string Out = Best->Lines[static_cast<size_t>(Pick)];
	auto Fill = [&Out](const std::string& Key, const std::string& Value) {
		const size_t At = Out.find(Key);
		if (At != std::string::npos)
		{
			Out.replace(At, Key.size(), Value);
		}
	};
	Fill("{weekday}", net::WeekdayName(net::DayOf(World), true));
	if (Out.find("{lot114}") != std::string::npos)
	{
		Fill("{lot114}", shop::AuctionWords(S.Life.Pawn, World));
	}
	return Out;
}

void Page::Say(const std::string& Moment, double Now, bool Bad)
{
	const std::string Words = KeeperSays(Moment);
	if (!Words.empty())
	{
		SayText(Words, Now, Bad);
	}
}

void Page::SayText(const std::string& Words, double Now, bool Bad)
{
	Note = Words;
	NoteAt = Now;
	NoteBad = Bad;
}

std::string Page::Under() const
{
	const shop::Venue* V = shop::FindVenue(Venue);
	const VenueFace* F = FaceOf(Venue);
	std::string Address = V && !V->Address.empty() ? V->Address : F ? F->Address : "";
	std::string Out = Upper(Address);
	if (V)
	{
		const std::string Hours = shop::HoursLine(*V, S.WorldMinutes());
		if (!Hours.empty())
		{
			Out += (Out.empty() ? "" : " \xC2\xB7 ") + Upper(Hours);
		}
	}
	return Out;
}

std::vector<Hint> Page::Hints() const
{
	return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "PICK"}, {"TAB", "Y", "PAY"}};
}

bool Page::Released(const Rect& R) const
{
	return Ptr.Released && Inside(R, Ptr.X, Ptr.Y) && Inside(R, PressX, PressY);
}

void Page::SwitchTab(int To, double Now)
{
	const int N = static_cast<int>(TabNames().size());
	if (N <= 0)
	{
		return;
	}
	Tab = ((To % N) + N) % N;
	TabAt = Now;
	Sel = 0;
	SelAt = Now;
	FirstRow = 0;
	// A stamp belongs to the receipt it landed on: another tab draws another bill (the gym's floor quotes a workout,
	// not the pass just paid for). The pawn ticket's punch stays with its ticket.
	if (StampWas != StampKind::Punched)
	{
		StampWas = StampKind::None;
	}
	TabChanged(Now);
}

void Page::Key(const std::string& Name, double Now)
{
	if (!Shown)
	{
		return;
	}
	Tick(Now);
	// Just opened: the button that opened it (E, or the pad's A) may still be down. Leaving always works.
	if (Now - OpenedAt < OpenGrace && Name != "Escape" && Name != "P")
	{
		return;
	}
	if (Name == "Escape" || Name == "P")
	{
		if (Tipping)
		{
			Tipping = false;
			return;
		}
		if (StepBack(Now))
		{
			return;
		}
		WalkAway(Now);
		return;
	}
	if (Tipping)
	{
		const int N = static_cast<int>(TipChoices.size());
		if ((Name == "Left" || Name == "Right") && N > 0)
		{
			TipIndex = std::clamp(TipIndex + (Name == "Left" ? -1 : 1), 0, N - 1);
		}
		else if (Name == "Enter" || Name == "Space" || Name == "Tab" || Name == "Reset")
		{
			DoMain(Now);
		}
		else if (Name == "Backspace" || Name == "Delete" || Name == "PutBack")
		{
			Tipping = false;
		}
		return;
	}
	if ((Name == "Q" || Name == "E" || Name == "TabPrev" || Name == "TabNext") && !TabsHeld())
	{
		if (TabNames().size() > 1)
		{
			SwitchTab(Tab + (Name == "Q" || Name == "TabPrev" ? -1 : 1), Now);
		}
		return;
	}
	if (PageKey(Name, Now))
	{
		return;
	}
	if (Name == "Tab" || Name == "Reset")
	{
		DoMain(Now);
	}
}

bool Page::GridKey(const std::string& Name, int N, int Cols, double Now)
{
	if (N <= 0)
	{
		return false;
	}
	int Next = Sel;
	if (Name == "Left")
	{
		Next = std::max(0, Sel - 1);
	}
	else if (Name == "Right")
	{
		Next = std::min(N - 1, Sel + 1);
	}
	else if (Name == "Up")
	{
		Next = Sel - Cols >= 0 ? Sel - Cols : Sel;
	}
	else if (Name == "Down")
	{
		Next = Sel + Cols < N ? Sel + Cols : (Sel / Cols < (N - 1) / Cols ? N - 1 : Sel);
	}
	else
	{
		return false;
	}
	if (Next != Sel)
	{
		Sel = Next;
		SelAt = Now;
	}
	return true;
}

void Page::CardBack(Canvas& C, const Rect& R, bool Focus, bool Over, bool Dim, double Now)
{
	const Color Acc = Accent(Venue);
	if (Focus)
	{
		C.GlowRoundRect(R, 6.0f, Fade(Acc, 0.25f), 22.0f);
	}
	C.FillRoundRect(R, 6.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Focus ? CardTop : Rgba(24, 28, 38, Dim ? 0.8f : 0.95f), CardLow));
	C.StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 6.0f, Focus ? Acc : Fade(MenuInk, Over ? 0.22f : 0.08f), Focus ? 2.0f : 1.0f);
	(void)Now;
}

float Page::Paragraph(Canvas& C, const std::string& Text, float X, float Y, float Width, float Size, int Weight, const Color& Col, float LineH, int MaxLines)
{
	std::vector<std::string> Lines = WrapText(C, Text, Width, Size, Weight);
	if (static_cast<int>(Lines.size()) > MaxLines && MaxLines > 0)
	{
		Lines.resize(static_cast<size_t>(MaxLines));
		Lines.back() += "\xE2\x80\xA6";
	}
	float Ly = Y;
	for (const std::string& Ln : Lines)
	{
		C.Text(Ln, X, Ly, Ts(Size, Weight, Col, Align::Left, Baseline::Alphabetic, false, Width));
		Ly += LineH;
	}
	return Ly - LineH;
}

void Page::Grid(Canvas& C, const Rect& R, int N, int Cols, float CardH, const std::function<void(int, const Rect&, bool, bool)>& Paint_, const std::function<void(int)>& Picked,
	double Now)
{
	if (N <= 0 || Cols <= 0)
	{
		return;
	}
	const float Gap = 16.0f;
	const float CardW = (R.W - Gap * Cf(Cols - 1)) / Cf(Cols);
	const int Fit = std::max(1, static_cast<int>((R.H + Gap) / (CardH + Gap)));
	const int Rows = (N + Cols - 1) / Cols;
	const int SelRow = std::clamp(Sel, 0, N - 1) / Cols;
	// The choice in view: when it moves past an edge the rows follow it.
	if (SelRow < FirstRow)
	{
		FirstRow = SelRow;
	}
	if (SelRow >= FirstRow + Fit)
	{
		FirstRow = SelRow - Fit + 1;
	}
	// The wheel over the grid scrolls it, and the choice comes along to stay in view.
	if (Ptr.Wheel != 0.0f && Inside(R, Ptr.X, Ptr.Y) && Rows > Fit)
	{
		FirstRow = std::clamp(FirstRow + (Ptr.Wheel > 0.0f ? 1 : -1), 0, Rows - Fit);
		const int Row = std::clamp(SelRow, FirstRow, FirstRow + Fit - 1);
		Sel = std::min(N - 1, Row * Cols + Sel % Cols);
	}
	FirstRow = std::clamp(FirstRow, 0, std::max(0, Rows - Fit));
	int Clicked = -1;
	const int First = FirstRow * Cols;
	const int Last = std::min(N, (FirstRow + Fit) * Cols);
	for (int I = First; I < Last; ++I)
	{
		const int K = I - First;
		const float In = Ease((Now - TabAt - 0.035 * static_cast<double>(K)) / 0.3);
		const Rect Cell{R.X + Cf(K % Cols) * (CardW + Gap), R.Y + Cf(K / Cols) * (CardH + Gap) + (1.0f - In) * 14.0f, CardW, CardH};
		const bool Focus = I == Sel;
		const bool Over = Hover(Cell);
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * In);
		Paint_(I, Cell, Focus, Over);
		C.SetAlpha(A0);
		if (Released(Cell))
		{
			Clicked = I;
		}
	}
	// More above or below: a quiet count at the edge.
	if (FirstRow > 0)
	{
		Arrow(C, R.X + R.W - 14.0f, R.Y - 14.0f, 6.0f, 0, Fade(MenuInk, 0.6f));
	}
	if (Last < N)
	{
		const std::string More = std::to_string(N - Last) + " MORE";
		const float Mw = TrackedWidth(C, More, 11.0f, 800, 2.4f);
		const float My = R.Y + Cf(Fit) * (CardH + Gap) - Gap + 22.0f;
		if (My < R.Y + R.H + 30.0f)
		{
			TrackedText(C, More, R.X + R.W - Mw - 22.0f, My, 11.0f, 800, MenuMuted, 2.4f);
			Arrow(C, R.X + R.W - 10.0f, My - 4.0f, 6.0f, 1, Fade(MenuInk, 0.6f));
		}
	}
	if (Clicked >= 0)
	{
		if (Sel != Clicked)
		{
			SelAt = Now;
		}
		Sel = Clicked;
		Picked(Clicked);
	}
}

// ------------------------------------------------------------------ the receipt

float Page::DrawBill(Canvas& C, const Receipt& R, float X0, float Y, double Now)
{
	// A declined card shakes the paper: a quick wobble that dies away.
	const double Dd = Now - StampAt;
	const float Shake = StampWas == StampKind::Declined && Dd >= 0.0 && Dd < 0.4 ? static_cast<float>(std::sin(Dd * 70.0) * 9.0 * (1.0 - Dd / 0.4)) : 0.0f;
	const float X = X0 + Shake;
	Color Sheet = Paper;
	Color Ink = PaperInk;
	Color Print = PaperInk; // the printed parts (the guest check's green)
	bool MonoBody = true;
	switch (R.Paper)
	{
	case PaperKind::Slip:
		Sheet = Hex(0xfbfaf6);
		Ink = Hex(0x23262b);
		Print = Hex(0x23262b);
		break;
	case PaperKind::GuestCheck:
		Sheet = Hex(0xe3eedb);
		Ink = Hex(0x1f3c88);
		Print = Hex(0x2f6b3d);
		MonoBody = false;
		break;
	case PaperKind::Ticket:
		Sheet = Hex(0xf5c8d3);
		Ink = Hex(0x4a1c2a);
		Print = Hex(0x7a2a42);
		break;
	case PaperKind::Donation:
		Sheet = Hex(0xfdfcf8);
		Ink = Hex(0x262a36);
		Print = Hex(0x1f4f8f);
		MonoBody = false;
		break;
	default: break;
	}
	const float RowH = 26.0f;
	// How tall: the header, the lines (as many as the column holds), the sums and the foot.
	const float Room = FooterY - 170.0f - Y - (Tipping ? 120.0f : 0.0f);
	const float Fixed = 120.0f + Cf(static_cast<int>(R.Head.size())) * 20.0f + 30.0f + Cf(static_cast<int>(R.Sums.size())) * 24.0f + 40.0f + Cf(static_cast<int>(R.Foot.size())) * 22.0f + 30.0f;
	const int MaxLines = std::max(2, static_cast<int>((Room - Fixed) / RowH));
	std::vector<Line> Rows = R.Lines;
	int Hidden = 0;
	if (static_cast<int>(Rows.size()) > MaxLines)
	{
		Hidden = static_cast<int>(Rows.size()) - (MaxLines - 1);
		Rows.resize(static_cast<size_t>(MaxLines - 1));
	}
	const int Shown_ = std::max(1, static_cast<int>(Rows.size()) + (Hidden > 0 ? 1 : 0));
	const float PaperH = Fixed + Cf(Shown_) * RowH + (R.Total.Left.empty() ? -40.0f : 0.0f);
	// The paper: a torn edge on the register's roll, straight on the rest.
	std::vector<Vec2> Shape = {{X, Y}, {X + ReceiptW, Y}};
	if (R.Paper == PaperKind::Thermal)
	{
		for (int K = 0; K <= 22; ++K)
		{
			Shape.push_back({X + ReceiptW - Cf(K) * ReceiptW / 22.0f, Y + PaperH + (K % 2 == 0 ? 0.0f : 9.0f)});
		}
	}
	else
	{
		Shape.push_back({X + ReceiptW, Y + PaperH});
		Shape.push_back({X, Y + PaperH});
	}
	C.GlowRoundRect({X, Y + 8.0f, ReceiptW, PaperH}, 4.0f, Rgba(0, 0, 0, 0.5f), 26.0f);
	C.FillPolygon(Shape, Paint::Linear({0.0f, Y}, {0.0f, Y + PaperH}, Sheet, Shade(Sheet, -0.05f)));
	auto Write = [&](const std::string& Txt, float Xx, float Yy, Align Al, float Size, int Weight, const Color& Col, float A = 1.0f, bool Mono = true, float MaxW = 0.0f) {
		C.Text(Txt, Xx, Yy, Ts(Size, Weight, Fade(Col, A), Al, Baseline::Alphabetic, Mono, MaxW));
	};
	// The paper's own marks.
	float Ry = Y;
	if (R.Paper == PaperKind::Slip)
	{
		// The pole's stripes across the top of the barber's ticket.
		C.PushClip({X, Y, ReceiptW, 14.0f});
		for (float Sx = -20.0f; Sx < ReceiptW + 20.0f; Sx += 28.0f)
		{
			C.FillPolygon({{X + Sx, Y + 14.0f}, {X + Sx + 10.0f, Y}, {X + Sx + 20.0f, Y}, {X + Sx + 10.0f, Y + 14.0f}}, Paint(Hex(0xd7263d)));
			C.FillPolygon({{X + Sx + 14.0f, Y + 14.0f}, {X + Sx + 24.0f, Y}, {X + Sx + 28.0f, Y}, {X + Sx + 18.0f, Y + 14.0f}}, Paint(Hex(0x1f4fa8)));
		}
		C.PopClip();
	}
	else if (R.Paper == PaperKind::GuestCheck)
	{
		// The printed rules: one under each line the pen writes on (the items, the sums, the total), and the price
		// column's rule from the header down to the total, clear of the header and the foot. The layout below is the
		// text pass's: the header's divider, then a row each RowH, the second divider, the sums each 24, the total 32.
		const float HeadDiv = Y + 80.0f + 20.0f * Cf(static_cast<int>(R.Head.size()));
		const float RowsTop = HeadDiv + 6.0f;
		std::vector<float> Under;
		for (int K = 1; K <= Shown_; ++K)
		{
			Under.push_back(RowsTop + RowH * Cf(K) + 8.0f);
		}
		float Last = RowsTop + RowH * Cf(Shown_) + 18.0f;
		for (size_t J = 0; J < R.Sums.size(); ++J)
		{
			Last += 24.0f;
			Under.push_back(Last + 8.0f);
		}
		if (!R.Total.Left.empty())
		{
			Last += 32.0f;
			Under.push_back(Last + 10.0f);
		}
		for (const float U : Under)
		{
			C.FillRect({X + 16.0f, U, ReceiptW - 32.0f, 1.0f}, Paint(Fade(Print, 0.25f)));
		}
		C.FillRect({X + ReceiptW - 118.0f, HeadDiv, 1.0f, Last + 10.0f - HeadDiv}, Paint(Fade(Print, 0.3f)));
	}
	else if (R.Paper == PaperKind::Ticket)
	{
		// The stub: the perforation, CUSTOMER COPY and the number (the punch goes through it when the loan is made).
		for (float Px = X + 8.0f; Px < X + ReceiptW - 8.0f; Px += 12.0f)
		{
			C.FillRect({Px, Y + PaperH - 66.0f, 6.0f, 1.5f}, Paint(Fade(Print, 0.45f)));
		}
		C.Text("CUSTOMER COPY" + (R.Number.empty() ? std::string() : " \xC2\xB7 " + R.Number), X + 28.0f, Y + PaperH - 27.0f, Ts(13.0f, 700, Fade(Print, 0.8f), Align::Left, Baseline::Alphabetic, true, ReceiptW - 110.0f));
	}
	else if (R.Paper == PaperKind::Donation)
	{
		C.FillRect({X, Y, ReceiptW, 8.0f}, Paint(Print));
	}
	if (!R.Number.empty())
	{
		Write(R.Number, X + ReceiptW - 22.0f, Y + 34.0f, Align::Right, 15.0f, 700, R.Paper == PaperKind::Ticket ? Hex(0xb01e3c) : Print);
	}
	Ry += 52.0f;
	Write(R.Title, X + ReceiptW * 0.5f, Ry, Align::Center, 20.0f, R.Paper == PaperKind::GuestCheck || R.Paper == PaperKind::Donation ? 900 : 700, Print, 1.0f, MonoBody, ReceiptW - 140.0f);
	Ry += 6.0f;
	for (const std::string& H : R.Head)
	{
		Ry += 20.0f;
		Write(H, X + ReceiptW * 0.5f, Ry, Align::Center, 13.0f, 500, Print, 0.85f, MonoBody, ReceiptW - 48.0f);
	}
	Ry += 22.0f;
	C.StrokePolyline({{X + 24.0f, Ry}, {X + ReceiptW - 24.0f, Ry}}, false, Fade(Print, 0.35f), 1.0f);
	Ry += 6.0f;
	if (Rows.empty())
	{
		Ry += RowH;
		Write(R.Empty, X + ReceiptW * 0.5f, Ry, Align::Center, 15.0f, 500, Ink, 0.8f, MonoBody, ReceiptW - 48.0f);
	}
	std::string Back;
	const float Fs = MonoBody ? 15.0f : 16.0f;
	for (const Line& Ln : Rows)
	{
		Ry += RowH;
		const auto At = Ln.Id.empty() ? LineAt.end() : LineAt.find(Ln.Id);
		const float P = At == LineAt.end() ? 1.0f : Ease((Now - At->second) / 0.16);
		const float Ly = Ry - (1.0f - P) * 10.0f;
		if (Ln.Removable && !Ln.Id.empty())
		{
			// The mouse's way to take one off: a minus beside the paper on the line under the pointer.
			const Rect Band{X - 40.0f, Ry - RowH + 6.0f, ReceiptW + 40.0f, RowH};
			if (Inside(Band, Ptr.X, Ptr.Y))
			{
				C.FillRect({X + 16.0f, Band.Y, ReceiptW - 32.0f, RowH}, Paint(Fade(Ink, 0.06f)));
				const Rect Minus{X - 33.0f, Band.Y + 2.0f, 22.0f, 22.0f};
				const bool MinusOver = Inside(Minus, Ptr.X, Ptr.Y);
				C.FillCircle(Minus.X + 11.0f, Minus.Y + 11.0f, 11.0f, Paint(MinusOver ? Fade(MenuWarn, 0.9f) : Fade(MenuInk, 0.22f)));
				C.StrokePolyline({{Minus.X + 6.5f, Minus.Y + 11.0f}, {Minus.X + 15.5f, Minus.Y + 11.0f}}, false, MenuInk, 2.0f, true);
				if (Released(Minus))
				{
					Back = Ln.Id;
				}
			}
		}
		const Color Col = Ln.Red ? Hex(0xc0262f) : Ink;
		const float A = P * (Ln.Dim ? 0.6f : 1.0f);
		const float RightW = Ln.Right.empty() ? 0.0f : C.Measure(Ln.Right, Fs, Ln.Bold ? 700 : 500, MonoBody) + 14.0f;
		const float MidW = Ln.Mid.empty() ? 0.0f : C.Measure(Ln.Mid, 13.0f, 500, MonoBody) + 14.0f;
		// On the guest check the pen keeps left of the price column's rule.
		const float LeftMax = R.Paper == PaperKind::GuestCheck ? std::min(ReceiptW - 56.0f - RightW - MidW, ReceiptW - 156.0f) : ReceiptW - 56.0f - RightW - MidW;
		Write(Ln.Left, X + 28.0f, Ly, Align::Left, Fs, Ln.Bold ? 700 : 500, Col, A, MonoBody, LeftMax);
		if (!Ln.Mid.empty())
		{
			Write(Ln.Mid, X + ReceiptW - 28.0f - RightW, Ly, Align::Right, 13.0f, 500, Col, A * 0.75f, MonoBody);
		}
		Write(Ln.Right, X + ReceiptW - 28.0f, Ly, Align::Right, Fs, Ln.Bold ? 700 : 500, Col, A, MonoBody);
	}
	if (Hidden > 0)
	{
		Ry += RowH;
		Write("+ " + std::to_string(Hidden) + " MORE", X + 28.0f, Ry, Align::Left, 14.0f, 500, Ink, 0.6f, MonoBody);
	}
	Ry += 18.0f;
	C.StrokePolyline({{X + 24.0f, Ry}, {X + ReceiptW - 24.0f, Ry}}, false, Fade(Print, 0.35f), 1.0f);
	for (const Line& Ln : R.Sums)
	{
		Ry += 24.0f;
		const Color Col = Ln.Red ? Hex(0xc0262f) : Ink;
		Write(Ln.Left, X + 28.0f, Ry, Align::Left, 15.0f, 500, Col, Ln.Dim ? 0.6f : 1.0f, MonoBody, ReceiptW - 180.0f);
		Write(Ln.Right, X + ReceiptW - 28.0f, Ry, Align::Right, 15.0f, 500, Col, Ln.Dim ? 0.6f : 1.0f, MonoBody);
	}
	if (!R.Total.Left.empty())
	{
		Ry += 32.0f;
		Write(R.Total.Left, X + 28.0f, Ry, Align::Left, 20.0f, 700, R.Total.Red ? Hex(0xc0262f) : Ink, 1.0f, MonoBody, ReceiptW - 200.0f);
		Write(R.Total.Right, X + ReceiptW - 28.0f, Ry, Align::Right, 20.0f, 700, R.Total.Red ? Hex(0xc0262f) : Ink, 1.0f, MonoBody);
	}
	Ry += 6.0f;
	for (const std::string& F : R.Foot)
	{
		Ry += 22.0f;
		Write(F, X + ReceiptW * 0.5f, Ry, Align::Center, 13.0f, 500, Print, 0.85f, MonoBody, ReceiptW - 40.0f);
	}
	// The stamp, or the ticket's punch.
	const bool Fresh = StampWas != StampKind::None && Now - StampAt < (StampWas == StampKind::Punched ? 1.0e9 : 4.0);
	if (Fresh && StampWas == StampKind::Punched && R.Paper == PaperKind::Ticket)
	{
		// Through the stub, clear of the numbers.
		const float Sa = Ease((Now - StampAt) / 0.12);
		C.FillCircle(X + ReceiptW - 54.0f, Y + PaperH - 33.0f, 15.0f * Sa, Paint(Rgba(6, 8, 12, 0.92f)));
		C.StrokeEllipse(X + ReceiptW - 54.0f, Y + PaperH - 33.0f, 16.0f * Sa, 16.0f * Sa, Fade(Print, 0.5f), 1.5f);
	}
	else if (Fresh && StampWas != StampKind::Punched)
	{
		const float Sa = Ease((Now - StampAt) / 0.15);
		const bool Good = StampWas != StampKind::Declined;
		const Color StampInk = StampWas == StampKind::Thanks ? Hex(0x1f4fa8) : Good ? Hex(0x1f9d55) : Hex(0xd7263d);
		const char* Word = StampWas == StampKind::Paid ? "PAID" : StampWas == StampKind::Declined ? "DECLINED" : StampWas == StampKind::Redeemed ? "REDEEMED" : StampWas == StampKind::Thanks ? "THANK YOU" : "PAID OUT";
		C.Save();
		C.Translate(X + ReceiptW * 0.5f, Y + PaperH * 0.55f);
		C.Rotate(-0.22f);
		C.Scale(1.6f - 0.6f * Sa, 1.6f - 0.6f * Sa);
		const float Sw = TrackedWidth(C, Word, 40.0f, 900, 6.0f) + 40.0f;
		// Slammed down in 0.15 s; the ink lifts over the last half second of its four.
		const float StampA = 0.85f * Sa * std::clamp(static_cast<float>((4.0 - (Now - StampAt)) / 0.5), 0.0f, 1.0f);
		C.StrokeRoundRect({-Sw * 0.5f, -36.0f, Sw, 64.0f}, 8.0f, Fade(StampInk, StampA), 4.0f);
		TrackedText(C, Word, 0.0f, 12.0f, 40.0f, 900, Fade(StampInk, StampA), 6.0f, Align::Center);
		C.Restore();
	}
	if (!Back.empty())
	{
		RemoveLine(Back, Now);
	}
	return Y + PaperH + (R.Paper == PaperKind::Thermal ? 9.0f : 0.0f);
}

float Page::DrawTips(Canvas& C, float X, float Y, double /*Now*/)
{
	const Keeper K = Who();
	const std::string Label = K.Name.empty() ? std::string("TIP") : "TIP " + Upper(K.Name);
	TrackedText(C, Label, X, Y + 14.0f, 11.0f, 800, MenuMuted, 2.6f);
	Glyph(C, Gamepad ? "DPAD" : "LEFT", X + ReceiptW - 70.0f, Y + 20.0f, 0.8f);
	Glyph(C, Gamepad ? "DPAD" : "RIGHT", X + ReceiptW - 34.0f, Y + 20.0f, Gamepad ? 0.0f : 0.8f);
	const int N = static_cast<int>(TipChoices.size());
	const float Gap = 8.0f;
	const float Cw = (ReceiptW - Gap * Cf(N - 1)) / Cf(std::max(1, N));
	// Each chip says what it comes to: a share of the bill before tax (TipBase).
	for (int I = 0; I < N; ++I)
	{
		const Rect R{X + Cf(I) * (Cw + Gap), Y + 30.0f, Cw, 58.0f};
		const bool On = I == TipIndex;
		const bool Over = Hover(R);
		C.FillRoundRect(R, 8.0f, Paint(On ? Fade(Mint, 0.9f) : Fade(MenuInk, Over ? 0.12f : 0.06f)));
		const std::string Pct = TipChoices[static_cast<size_t>(I)] == 0 ? std::string("NONE") : std::to_string(TipChoices[static_cast<size_t>(I)]) + "%";
		C.Text(Pct, R.X + R.W * 0.5f, R.Y + 26.0f, Ts(18.0f, 900, On ? Hex(0x05140c) : MenuInk, Align::Center));
		C.Text(TipChoices[static_cast<size_t>(I)] == 0 ? std::string("stiff") : Money(TipAmount(TipChoices[static_cast<size_t>(I)])), R.X + R.W * 0.5f, R.Y + 46.0f,
			Ts(12.0f, 600, On ? Hex(0x0b2a18) : MenuMuted, Align::Center));
		if (Released(R))
		{
			TipIndex = I;
		}
	}
	return Y + 88.0f;
}

float Page::DrawButton(Canvas& C, const Button& B, float X, float Y, double Now)
{
	const Rect R{X, Y, ReceiptW, 66.0f};
	const bool Over = Hover(R);
	C.FillRect(R, Paint(B.Enabled ? (B.Warn ? Fade(MenuWarn, 0.3f) : Mix(Mint, Hex(0xffffff), Over ? 0.15f : 0.0f)) : Fade(MenuInk, 0.06f)));
	if (B.Enabled && !B.Warn)
	{
		C.GlowRoundRect(R, 1.0f, Fade(Mint, 0.3f), 18.0f);
	}
	const float Lw = std::min(TrackedWidth(C, B.Label, 18.0f, 900, 2.6f), R.W - 100.0f);
	const float Size = Lw < TrackedWidth(C, B.Label, 18.0f, 900, 2.6f) ? 18.0f * Lw / TrackedWidth(C, B.Label, 18.0f, 900, 2.6f) : 18.0f;
	TrackedText(C, B.Label, R.X + (R.W - 70.0f) * 0.5f, R.Y + 40.0f, Size, 900, B.Enabled && !B.Warn ? Hex(0x05140c) : B.Enabled ? MenuInk : MenuMuted, 2.6f * Size / 18.0f, Align::Center);
	Glyph(C, Gamepad ? "Y" : "TAB", R.X + R.W - 64.0f, R.Y + 43.0f, B.Enabled ? 0.9f : 0.4f);
	if (Released(R) && B.Enabled)
	{
		DoMain(Now);
	}
	return Y + 66.0f;
}

void Page::Side(Canvas& C, const Rect& R, double Now)
{
	const Receipt Bl = Bill(Now);
	float Y = DrawBill(C, Bl, R.X, R.Y, Now);
	if (Tipping && !TipChoices.empty())
	{
		Y = DrawTips(C, R.X, Y + 26.0f, Now);
	}
	DrawButton(C, Main(Now), R.X, Y + 30.0f, Now);
}

// ------------------------------------------------------------------ drawing the frame

void Page::DrawHeader(Canvas& C, float LeftW, double Now)
{
	const shop::Venue* V = shop::FindVenue(Venue);
	const VenueFace* F = FaceOf(Venue);
	const std::string Name = Upper(Title());
	const bool Narrow = LeftW < 1000.0f;
	VenueMark(C, Venue, Margin + 34.0f, 108.0f, 30.0f, Now);
	const float NameSize = Narrow ? 28.0f : 34.0f;
	float Track = Narrow ? 3.0f : 4.0f;
	float Nw = TrackedWidth(C, Name, NameSize, 900, Track);
	const float NameMax = std::max(260.0f, LeftW * 0.5f - 96.0f);
	float Ns = NameSize;
	if (Nw > NameMax)
	{
		Ns = NameSize * NameMax / Nw;
		Track = Track * NameMax / Nw;
		Nw = NameMax;
	}
	TrackedText(C, Name, Margin + 86.0f, 116.0f, Ns, 900, MenuInk, Track);
	// Under the name: the address and the hours, and whether it's open.
	float Ux = Margin + 88.0f;
	if (V && Venue != "wardrobe")
	{
		const bool OpenNow = S.ShopOpenFor(Venue);
		const std::string Pill = OpenNow ? "OPEN" : "CLOSED";
		const float Pw = C.Measure(Pill, 11.0f, 800) + 18.0f;
		C.FillRoundRect({Ux, 128.0f, Pw, 20.0f}, 10.0f, Paint(OpenNow ? Hex(0x3ecf6e, 0.18f) : Hex(0xff5a5f, 0.18f)));
		C.Text(Pill, Ux + Pw * 0.5f, 138.0f, Ts(11.0f, 800, OpenNow ? Hex(0x3ecf6e) : MenuWarn, Align::Center, Baseline::Middle));
		Ux += Pw + 10.0f;
	}
	(void)F;
	const std::string Sub = Under();
	const float SubMax = std::max(160.0f, Margin + 88.0f + NameMax + 40.0f - Ux);
	float SubSize = 12.0f;
	float SubTrack = 2.4f;
	const float SubW = TrackedWidth(C, Sub, SubSize, 700, SubTrack);
	if (SubW > SubMax)
	{
		SubSize = std::max(9.0f, SubSize * SubMax / SubW);
		SubTrack = SubTrack * SubSize / 12.0f;
	}
	if (TrackedWidth(C, Sub, SubSize, 700, SubTrack) <= SubMax + 0.5f)
	{
		TrackedText(C, Sub, Ux, 143.0f, SubSize, 700, Accent(Venue), SubTrack);
	}
	else
	{
		C.Text(Sub, Ux, 143.0f, Ts(SubSize, 700, Accent(Venue), Align::Left, Baseline::Alphabetic, false, SubMax));
	}

	// The keeper: the rest of the row past the name (never over it), as tall as what they say.
	const Keeper K = Who();
	const std::string Label = K.Id.empty() ? KeeperAway() : Upper(K.Name) + (K.Role.empty() ? "" : " \xC2\xB7 " + Upper(K.Role));
	const std::string Said = K.Id.empty() ? (Note.empty() ? std::string() : Note) : Note;
	if (Label.empty() && Said.empty())
	{
		return;
	}
	// Past the name and past the line under it, which starts after the OPEN pill.
	const float SubEnd = Ux + std::min(SubMax, TrackedWidth(C, Sub, SubSize, 700, SubTrack));
	const float BubX = std::max(Margin + LeftW * 0.48f, std::max(Margin + 86.0f + Nw, SubEnd) + 40.0f);
	const float BubW = std::max(260.0f, Margin + LeftW - BubX);
	const std::string Quote = Said.empty() ? std::string() : "\xE2\x80\x9C" + Said + "\xE2\x80\x9D";
	float Fs = 17.0f;
	float Lh = 24.0f;
	std::vector<std::string> Lines = Quote.empty() ? std::vector<std::string>() : WrapText(C, Quote, BubW - 44.0f, Fs, 400);
	if (Lines.size() > 2)
	{
		Fs = 15.0f;
		Lh = 21.0f;
		Lines = WrapText(C, Quote, BubW - 44.0f, Fs, 400);
	}
	if (Lines.size() > 3)
	{
		Lines.resize(3);
		Lines[2] += "\xE2\x80\xA6";
	}
	const Rect Bubble{BubX, 64.0f, BubW, std::max(96.0f, 46.0f + Cf(static_cast<int>(Lines.size())) * Lh)};
	const float Na = Ease((Now - NoteAt) / 0.3);
	C.FillRoundRect(Bubble, 14.0f, Paint(Rgba(255, 255, 255, 0.06f * Na)));
	C.FillPolygon({{Bubble.X + 30.0f, Bubble.Y + Bubble.H}, {Bubble.X + 54.0f, Bubble.Y + Bubble.H}, {Bubble.X + 26.0f, Bubble.Y + Bubble.H + 16.0f}}, Paint(Rgba(255, 255, 255, 0.06f * Na)));
	TrackedText(C, Label, Bubble.X + 22.0f, Bubble.Y + 28.0f, 11.0f, 800, Fade(NoteBad ? MenuWarn : Accent(Venue), Na), 2.6f);
	float Ny = Bubble.Y + 34.0f;
	for (const std::string& Ln : Lines)
	{
		Ny += Lh;
		C.Text(Ln, Bubble.X + 22.0f, Ny, Ts(Fs, 400, Fade(MenuInk, 0.92f * Na), Align::Left, Baseline::Alphabetic, false, Bubble.W - 40.0f));
	}
}

void Page::DrawTabs(Canvas& C, float LeftW, double Now)
{
	const std::vector<std::string> Names = TabNames();
	if (Names.size() <= 1)
	{
		return;
	}
	const bool Held = TabsHeld();
	float Tx = Margin;
	const float Qw = Glyph(C, Gamepad ? "LB" : "Q", Tx, TabY + 3.0f, Held ? 0.35f : 1.0f);
	Tx += Qw + 24.0f;
	const Color Acc = Accent(Venue);
	// The row fits the left column (six tabs at 4:3 would run under the receipt): a size down, and tighter, when it must.
	float Natural = 0.0f;
	for (const std::string& Name : Names)
	{
		Natural += TrackedWidth(C, Name, 16.0f, 800, 4.0f) + 36.0f;
	}
	const float Room = LeftW - (Tx - Margin) - 60.0f;
	const float K = Natural > Room && Natural > 0.0f ? std::max(0.62f, Room / Natural) : 1.0f;
	const float Size = 16.0f * K;
	const float Track = 4.0f * K;
	const float Gap = 36.0f * K;
	for (int I = 0; I < static_cast<int>(Names.size()); ++I)
	{
		const std::string& Name = Names[static_cast<size_t>(I)];
		const bool On = I == Tab;
		const float Lw = TrackedWidth(C, Name, Size, 800, Track);
		TrackedText(C, Name, Tx, TabY, Size, 800, On ? MenuInk : Fade(MenuMuted, Held ? 0.5f : 1.0f), Track);
		if (On)
		{
			C.FillRect({Tx, TabY + 12.0f, Lw, 3.0f}, Paint(Acc));
		}
		if (!Held && Released({Tx - 10.0f, TabY - 26.0f, Lw + 20.0f, 40.0f}) && !On)
		{
			SwitchTab(I, Now);
		}
		Tx += Lw + Gap;
	}
	Glyph(C, Gamepad ? "RB" : "E", Tx - Gap + 24.0f, TabY + 3.0f, Held ? 0.35f : 1.0f);
}

void Page::DrawFooter(Canvas& C, double Now)
{
	float Hx = Margin;
	const float Hy = FooterY;
	std::vector<Hint> List = Tipping ? std::vector<Hint>{{"LEFT", "DPAD", "TIP"}, {"TAB", "Y", "PAY"}} : Hints();
	const bool Step = Tipping || TabsHeld();
	List.push_back({"ESC", "B", Step ? "BACK" : "WALK AWAY"});
	for (size_t I = 0; I < List.size(); ++I)
	{
		const Hint& Hp = List[I];
		const float X0 = Hx;
		Hx += Glyph(C, Gamepad ? Hp.Pad : Hp.Glyph, Hx, Hy, 1.0f) + 10.0f;
		Hx += TrackedText(C, Hp.Label, Hx, Hy - 2.0f, 13.0f, 700, MenuMuted, 2.6f) + 32.0f;
		if (I + 1 == List.size() && Released({X0 - 8.0f, Hy - 29.0f, Hx - X0 - 16.0f, 40.0f}))
		{
			if (Tipping)
			{
				Tipping = false;
			}
			else if (!StepBack(Now))
			{
				WalkAway(Now);
			}
		}
	}
}

void Page::Draw(Canvas& C, double Now)
{
	if (!Shown)
	{
		Ptr.EndFrame();
		return;
	}
	Tick(Now);
	if (Ptr.Pressed)
	{
		PressX = Ptr.X;
		PressY = Ptr.Y;
	}
	const float W = C.Width();
	const float H = Height;
	const float In = Ease((Now - OpenedAt) / 0.4);
	const float Al = C.GetAlpha();
	C.SetAlpha(Al * In);
	const float LeftW = W - Margin * 2.0f - ReceiptW - 48.0f;
	// The room behind: dimmed, or clear in the middle where the hero sits in the mirror.
	switch (Backing())
	{
	case Backdrop::Full:
		C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W, 0.0f}, Rgba(4, 6, 10, 0.9f), Rgba(4, 6, 10, 0.55f)));
		C.FillRect({0.0f, H - 240.0f, W, 240.0f}, Paint::Linear({0.0f, H - 240.0f}, {0.0f, H}, Rgba(4, 6, 10, 0.0f), Rgba(4, 6, 10, 0.8f)));
		break;
	case Backdrop::Split:
	{
		const float Lw = Margin + BodyWidth(LeftW) + 80.0f;
		C.FillRect({0.0f, 0.0f, Lw, H}, Paint::Linear({0.0f, 0.0f}, {Lw, 0.0f}, Rgba(4, 6, 10, 0.92f), Rgba(4, 6, 10, 0.0f)));
		const float Rx = W - Margin - ReceiptW - 90.0f;
		C.FillRect({Rx, 0.0f, W - Rx, H}, Paint::Linear({Rx, 0.0f}, {W, 0.0f}, Rgba(4, 6, 10, 0.0f), Rgba(4, 6, 10, 0.85f)));
		C.FillRect({0.0f, 0.0f, W, 200.0f}, Paint::Linear({0.0f, 0.0f}, {0.0f, 200.0f}, Rgba(4, 6, 10, 0.7f), Rgba(4, 6, 10, 0.0f)));
		C.FillRect({0.0f, H - 160.0f, W, 160.0f}, Paint::Linear({0.0f, H - 160.0f}, {0.0f, H}, Rgba(4, 6, 10, 0.0f), Rgba(4, 6, 10, 0.75f)));
		break;
	}
	case Backdrop::Low:
		C.FillRect({0.0f, H - 330.0f, W, 330.0f}, Paint::Linear({0.0f, H - 330.0f}, {0.0f, H}, Rgba(4, 6, 10, 0.0f), Rgba(4, 6, 10, 0.88f)));
		C.FillRect({0.0f, 0.0f, W, 180.0f}, Paint::Linear({0.0f, 0.0f}, {0.0f, 180.0f}, Rgba(4, 6, 10, 0.6f), Rgba(4, 6, 10, 0.0f)));
		break;
	}

	// Ten minutes from closing, the keeper says so (once).
	if (const shop::Venue* V = shop::FindVenue(Venue))
	{
		const double World = S.WorldMinutes();
		if (!ClosingSaid && shop::IsOpen(*V, World))
		{
			const double Shuts = shop::ClosesAt(*V, World);
			if (Shuts > World && Shuts - World <= 10.0 && !shop::IsOpen(*V, Shuts + 0.5))
			{
				ClosingSaid = true;
				Say("closing", Now);
			}
		}
	}

	if (Backing() != Backdrop::Low)
	{
		DrawHeader(C, LeftW, Now);
		DrawTabs(C, LeftW, Now);
	}
	Body(C, {Margin, BodyTop, LeftW, FooterY - 56.0f - BodyTop}, Now);
	if (Shown)
	{
		Side(C, {W - Margin - ReceiptW, 86.0f, ReceiptW, FooterY - 40.0f - 86.0f}, Now);
	}
	if (Shown)
	{
		DrawFooter(C, Now);
	}
	C.SetAlpha(Al);
	Ptr.EndFrame();
}

std::string Page::Title() const
{
	const shop::Venue* V = shop::FindVenue(Venue);
	if (V && !V->Name.empty())
	{
		return V->Name;
	}
	const VenueFace* F = FaceOf(Venue);
	return F ? F->Name : Venue;
}

std::string Page::KeeperAway() const
{
	return std::string();
}

float Page::BodyWidth(float LeftW) const
{
	return LeftW;
}

Chips Page::TipAmount(int Percent)
{
	return static_cast<Chips>(std::llround(static_cast<double>(TipBase()) * static_cast<double>(Percent) / 100.0));
}
} // namespace shopframe

std::unique_ptr<ShopScreen> MakeShopScreen(const std::string& Venue, Session& S)
{
	if (Venue == "barber")
	{
		return shopframe::MakeBarber(S);
	}
	if (Venue == "thrift")
	{
		return shopframe::MakeThrift(S);
	}
	if (Venue == "gym")
	{
		return shopframe::MakeGym(S);
	}
	if (Venue == "diner")
	{
		return shopframe::MakeDiner(S);
	}
	if (Venue == "pawn")
	{
		return shopframe::MakePawn(S);
	}
	if (Venue == "wardrobe")
	{
		return shopframe::MakeWardrobe(S);
	}
	return nullptr;
}
} // namespace ui
} // namespace ss
