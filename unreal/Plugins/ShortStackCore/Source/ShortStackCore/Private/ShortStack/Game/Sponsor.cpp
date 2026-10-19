#include "ShortStack/Game/Sponsor.h"
#include "../StrictFloat.h"
#include "SponsorCopy.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Sponsors (next_segment_spec §3). core-sponsor.
//
// The catalog (§3.4), standing (§3.3), the contract book: offers on each brand's weekly look, signing with exclusivity,
// the overlay's two slots and the patch spots, 30-day reviews paying in arrears, strikes with a 14-day cure, notice,
// the morals clause, counters answered the next business morning, renewals 14 days out. Live money runs inside
// live::Register/Settle/Refund (N2): the cover line, the cut, the bonuses. Kast works on a view of the book (§3.8).
//
// Every movement of money is keyed in Book::PaidKeys ("stipend|<contract>|<period>", "cover|<entry>|<contract>|<period>"),
// so stepping a span twice, or after a save in the middle of it, pays once. Keys of contracts and entries the save no
// longer keeps are pruned with them.

namespace ss
{
namespace sponsor
{
namespace sponsor_detail
{
constexpr double DayMinutes = 1440.0;
constexpr double PeriodDays = 30.0;
constexpr double OfferDays = 7.0;
constexpr size_t EndedKept = 24;
constexpr double DeclineCooldownDays = 60.0;
constexpr double ExpireCooldownDays = 21.0;
constexpr double CauseCooldownDays = 120.0;
constexpr double NoticeBrandDays = 180.0;
constexpr double NoticeCategoryDays = 30.0;
constexpr double ConductDays = 180.0;
constexpr double RestrictedDays = 365.0;
constexpr double RenewalLeadDays = 14.0;
constexpr int BlockDays = 12;
constexpr int BlockWindow = 60;

using sponsor_copy::Fill;
using sponsor_copy::Text;

std::string Esc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
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

std::string Unesc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
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

/** A time or an amount with two decimals, as the save writes them ("-1000000000.00"). */
std::string Num2(double V)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.2f", V);
	return Buf;
}

/** A double with at most four decimals and no trailing zeros ("20", "15.5", "0.6667"): stable once read back. */
std::string NumShort(double V)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.4f", V);
	std::string S = Buf;
	while (!S.empty() && S.back() == '0')
	{
		S.pop_back();
	}
	if (!S.empty() && S.back() == '.')
	{
		S.pop_back();
	}
	return S == "-0" ? std::string("0") : S;
}

/** "$150", "$12.50", "$1,500": whole dollars without the cents. */
std::string Dollars(Chips Cents)
{
	std::string M = Money(Cents);
	if (M.size() > 3 && M.compare(M.size() - 3, 3, ".00") == 0)
	{
		M.resize(M.size() - 3);
	}
	return M;
}

/** "Wednesday, Dec 30". */
std::string DateWords(double World)
{
	const int Day = net::DayOf(World);
	return std::string(net::WeekdayName(Day, true)) + ", " + net::DateLabel(Day);
}

uint32_t Hash(const std::string& S)
{
	return Fnv1a(S);
}

std::vector<std::string> Split(const std::string& S, char Sep)
{
	std::vector<std::string> Out(1);
	for (const char Ch : S)
	{
		if (Ch == Sep)
		{
			Out.emplace_back();
		}
		else
		{
			Out.back().push_back(Ch);
		}
	}
	return Out;
}

bool HasSpot(const Terms& T, Spot S)
{
	return std::find(T.Wants.begin(), T.Wants.end(), S) != T.Wants.end();
}

/** The patch spots a brand takes (Wants without the overlay): one patch, on any one of them. */
std::vector<Spot> SpotsOf(const Terms& T)
{
	std::vector<Spot> Out;
	for (const Spot S : T.Wants)
	{
		if (S != Spot::Overlay && std::find(Out.begin(), Out.end(), S) == Out.end())
		{
			Out.push_back(S);
		}
	}
	return Out;
}

bool HasOverlay(const Contract& C)
{
	return C.Legacy || HasSpot(C.T, Spot::Overlay);
}

bool PaysOnStream(const Terms& T)
{
	return T.PerLiveHourCents > 0 || T.PerReadCents > 0;
}

/** Signed and not over (a renewal signed ahead of its start is live too). */
bool Live(const Contract& C)
{
	return C.St == State::Active || C.St == State::Notice;
}

bool RunningAt(const Contract& C, double World)
{
	return Live(C) && C.Since <= World && World < C.Until;
}

bool OpenOffer(const Contract& C)
{
	return C.St == State::Offered || C.St == State::Countered;
}

const Brand* BrandOf(const Contract& C)
{
	return FindBrand(C.Brand);
}

/** "IronHouse's", "Fifth St. Barbers'". */
std::string Possessive(const std::string& Name)
{
	return !Name.empty() && Name.back() == 's' ? Name + "'" : Name + "'s";
}

std::string ShortOf(const std::string& BrandId)
{
	const Brand* B = FindBrand(BrandId);
	return B ? B->Short : BrandId;
}

bool Mails(const Brand& B)
{
	return B.Keeper.empty();
}

std::string FromOf(const Brand& B)
{
	return Mails(B) ? B.Rep : B.Name;
}

/** The rent the bankroll has to keep: what's due within seven days (a clawback stops there, §3.6). */
Chips RentReserve(const life::State& L, double World)
{
	const bool Due = L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice;
	return Due && L.RentDeadline - World <= 7.0 * DayMinutes ? L.RentDueCents : 0;
}

/** The template's keys: "<kind>|<contract>|..." or, for an entry's money, "<kind>|<entry>|...". */
bool EntryKey(const std::string& Kind)
{
	return Kind == "cover" || Kind == "uncover" || Kind == "bonus" || Kind == "cut";
}

/** Every Ask in order. */
const Ask AllAsks[] = {Ask::LiveHours, Ask::Reads, Ask::TeachingStreams, Ask::LiveEvents, Ask::EmbercrestEvents, Ask::Workouts, Ask::Appearances, Ask::CommunityStreams};

/** How much of an ask makes it met: hours and reads at 80%, the rest in full (§3.7). */
double Threshold(Ask A)
{
	return A == Ask::LiveHours || A == Ask::Reads ? 0.8 : 1.0;
}

bool Counted(Ask A)
{
	return A != Ask::LiveHours;
}

/** The appearance's words in a letter ("the photo with Ray"). */
std::string AppearanceWords(const std::string& BrandId)
{
	if (BrandId == "luckypenny")
	{
		return "the photo with Ray";
	}
	if (BrandId == "barber")
	{
		return "the afternoon in the window chair";
	}
	if (BrandId == "embercrest")
	{
		return "the meet-the-regulars";
	}
	return "the appearance";
}

std::string Plural(int N, const char* One, const char* Many)
{
	return std::to_string(N) + " " + (N == 1 ? One : Many);
}

/** One missed ask, as a letter says it ("1 of 2 live events in the patch"). */
std::string MissedWords(const std::string& BrandId, Ask A, double Have, double Need)
{
	const int H = static_cast<int>(std::floor(Have + 1e-9));
	const int N = static_cast<int>(std::ceil(Need - 1e-9));
	switch (A)
	{
	case Ask::LiveHours: return std::to_string(static_cast<int>(std::floor(Have / 60.0 + 1e-9))) + " of " + Plural(static_cast<int>(std::lround(Need)), "live hour", "live hours");
	case Ask::Reads: return std::to_string(H) + " of " + Plural(N, "read", "reads");
	case Ask::TeachingStreams: return std::to_string(H) + " of " + Plural(N, "teaching stream", "teaching streams");
	case Ask::LiveEvents: return std::to_string(H) + " of " + Plural(N, "live event", "live events") + " in the patch";
	case Ask::EmbercrestEvents: return std::to_string(H) + " of " + Plural(N, "Embercrest event", "Embercrest events");
	case Ask::Workouts: return std::to_string(H) + " of " + Plural(N, "workout", "workouts");
	case Ask::Appearances: return AppearanceWords(BrandId);
	case Ask::CommunityStreams: return "the community stream";
	case Ask::Count: break;
	}
	return std::string();
}

std::string JoinAnd(const std::vector<std::string>& Parts)
{
	std::string Out;
	for (size_t I = 0; I < Parts.size(); ++I)
	{
		Out += (I == 0 ? "" : I + 1 == Parts.size() ? ", and " : ", ") + Parts[I];
	}
	if (Parts.size() == 2)
	{
		Out = Parts[0] + " and " + Parts[1];
	}
	return Out;
}

std::map<std::string, std::string> VarsOf(const Contract& C)
{
	std::map<std::string, std::string> V;
	const Brand* B = BrandOf(C);
	V["Brand"] = B ? B->Name : C.Brand;
	V["Short"] = B ? B->Short : C.Brand;
	V["Rep"] = B ? B->Rep : C.Brand;
	return V;
}

/** Posts a letter on C's thread: the subject by key (falling back to its generic form), the body as written. */
void Post(life::State& L, double At, const Contract& C, int Kind, const std::string& SubjectKey, const std::string& Body, const std::map<std::string, std::string>& Extra = {})
{
	const Brand* B = BrandOf(C);
	std::map<std::string, std::string> V = VarsOf(C);
	for (const auto& Kv : Extra)
	{
		V[Kv.first] = Kv.second;
	}
	life::Letter M;
	M.At = At;
	M.Thread = C.Id;
	M.Source = "sponsor";
	M.From = B ? FromOf(*B) : C.Brand;
	M.Subject = Fill(Text(sponsor_copy::Subject, SubjectKey + ":" + C.Brand), V);
	M.Body = Body;
	M.Kind = Kind;
	life::Post(L, M);
}

/** A body by key for C's brand ("strike:kind" tries "strike:kind:embercrest" first), filled. */
std::string BodyFor(const Contract& C, const std::string& Key, const std::map<std::string, std::string>& Extra = {})
{
	std::map<std::string, std::string> V = VarsOf(C);
	for (const auto& Kv : Extra)
	{
		V[Kv.first] = Kv.second;
	}
	return Fill(Text(sponsor_copy::Body, Key + ":" + C.Brand), V);
}

std::string Joined(std::initializer_list<std::string> Parts)
{
	std::string Out;
	for (const std::string& P : Parts)
	{
		if (!P.empty())
		{
			Out += (Out.empty() ? "" : " ") + P;
		}
	}
	return Out;
}

Chips Round5Dollars(double Cents)
{
	return static_cast<Chips>(std::llround(Cents / 500.0)) * 500;
}

void Scale(Terms& T, double Factor)
{
	T.MonthlyCents = T.MonthlyCents > 0 ? std::max<Chips>(500, Round5Dollars(static_cast<double>(T.MonthlyCents) * Factor)) : 0;
	T.PerLiveHourCents = static_cast<Chips>(std::llround(static_cast<double>(T.PerLiveHourCents) * Factor));
	T.PerReadCents = static_cast<Chips>(std::llround(static_cast<double>(T.PerReadCents) * Factor));
	T.FinalTableBonusCents = T.FinalTableBonusCents > 0 ? std::max<Chips>(500, Round5Dollars(static_cast<double>(T.FinalTableBonusCents) * Factor)) : 0;
	T.TitleBonusCents = T.TitleBonusCents > 0 ? std::max<Chips>(500, Round5Dollars(static_cast<double>(T.TitleBonusCents) * Factor)) : 0;
	T.PerTeachingCents = T.PerTeachingCents > 0 ? std::max<Chips>(500, Round5Dollars(static_cast<double>(T.PerTeachingCents) * Factor)) : 0;
}

bool HasMoney(const Terms& T)
{
	return T.MonthlyCents > 0 || T.PerLiveHourCents > 0 || T.PerReadCents > 0 || T.FinalTableBonusCents > 0 || T.TitleBonusCents > 0 || T.PerTeachingCents > 0;
}

bool HasAsks(const Terms& T)
{
	for (const double A : T.Asks)
	{
		if (A > 0.0)
		{
			return true;
		}
	}
	return false;
}

/** The next weekday 9:00 to 10:29 after World (the brand's seeded minute): a counter's answer, a raise's. */
double NextBusinessMorning(double World, const std::string& Salt)
{
	int Day = net::DayOf(World) + 1;
	while (net::Weekday(Day) >= 5)
	{
		++Day;
	}
	return static_cast<double>(Day) * DayMinutes + 9.0 * 60.0 + static_cast<double>(Hash(Salt) % 90u);
}

/** The world minute World's save clock reads (a SaveData has no session). */
double SaveWorld(const SaveData& D)
{
	return net::MinutesPerDay * static_cast<double>(net::NightOneDay) + D.ClockMinutes;
}
} // namespace sponsor_detail

using namespace sponsor_detail;

// ------------------------------------------------------------------ the catalog (§3.4)

namespace sponsor_catalog_detail
{
Brand Make(const char* Id, const char* Name, const char* Short, Category Cat, int Tier, uint32_t Color, uint32_t Ink, int Mark)
{
	Brand B;
	B.Id = Id;
	B.Name = Name;
	B.Short = Short;
	B.Cat = Cat;
	B.Tier = Tier;
	B.Color = Color;
	B.Ink = Ink;
	B.Mark = Mark;
	B.Needs.Tier = Tier;
	B.Offer.Exclusive = ExclusiveCategory(Cat);
	return B;
}

void Asks(Brand& B, std::initializer_list<std::pair<Ask, double>> List)
{
	for (const auto& A : List)
	{
		B.Offer.Asks[static_cast<size_t>(A.first)] = A.second;
	}
}

std::vector<Brand> Build()
{
	std::vector<Brand> V;
	{
		// The corner store: a Polaroid on the corkboard, PENNY'S PICK in Sharpie.
		Brand B = Make("luckypenny", "Lucky Penny #212", "Lucky Penny", Category::Local, 1, 0x1f5c3a, 0xc8743a, 0);
		B.Rep = "Ray";
		B.RepTitle = "Days at the register";
		B.Keeper = "ray";
		B.Venue = "store";
		B.Needs.BlockVenue = "store";
		B.Needs.BlockVisits = BlockDays;
		B.Offer.Days = 90;
		B.Offer.Perks = {{Gift::StoreCredit, "store", 1, 2500, 7}};
		B.Offer.CoveredEntries = 1;
		B.Offer.CoveredCapCents = 8000;
		B.Offer.CoveredEvents = {"noon"};
		Asks(B, {{Ask::LiveEvents, 2.0}, {Ask::Appearances, 1.0}});
		B.Offer.Wants = {Spot::Chest, Spot::Cap};
		V.push_back(B);
	}
	{
		Brand B = Make("barber", "Fifth St. Barbers", "Fifth St. Barbers", Category::Local, 1, 0x1d3557, 0xe63946, 1);
		B.Rep = "Lonnie";
		B.RepTitle = "Barber";
		B.Keeper = "lonnie";
		B.Venue = "barber";
		B.Offer.Days = 60;
		B.Offer.Perks = {{Gift::Service, "barber", 1, 3000, 14}};
		Asks(B, {{Ask::LiveEvents, 2.0}, {Ask::Appearances, 1.0}});
		B.Offer.Wants = {Spot::Sleeve};
		V.push_back(B);
	}
	{
		Brand B = Make("ironhouse", "IronHouse Gym", "IronHouse", Category::Local, 1, 0x2a2a2e, 0xef4444, 2);
		B.Rep = "Rosa";
		B.RepTitle = "Trainer";
		B.Keeper = "rosa";
		B.Venue = "gym";
		B.Offer.Days = 90;
		B.Offer.Perks = {{Gift::Gear, "gym", 1, 3499, 30}};
		Asks(B, {{Ask::Workouts, 8.0}, {Ask::LiveEvents, 1.0}});
		B.Offer.Wants = {Spot::Sleeve};
		V.push_back(B);
	}
	{
		Brand B = Make("voltrush", "Volt Rush", "Volt Rush", Category::Energy, 2, 0x16181d, 0xb6ff2e, 3);
		B.Rep = "Dana Kell";
		B.RepTitle = "Partnerships";
		B.Offer.Days = 90;
		B.Offer.MonthlyCents = 15000;
		B.Offer.FinalTableBonusCents = 5000;
		B.Offer.Perks = {{Gift::Product, "volt-rush", 12, 0, 7}};
		Asks(B, {{Ask::LiveEvents, 3.0}});
		B.Offer.Wants = {Spot::Cap};
		V.push_back(B);
	}
	{
		Brand B = Make("overclock", "Overclock", "Overclock", Category::Energy, 2, 0x84cc16, 0x0b0716, 4);
		B.Kast = "Overclock";
		B.Product = "Overclock Energy, zero sugar";
		B.ReadLine = "This hand is brought to you by Overclock Energy. Zero sugar, all-night focus. Code GRIND in the panels.";
		B.Rep = "Jules Okafor";
		B.RepTitle = "Creator partnerships";
		B.Needs.Followers = 300;
		B.Needs.AvgViewers = 15;
		B.Offer.Days = 30;
		B.Offer.PerLiveHourCents = 300;
		B.Offer.PerReadCents = 1000;
		Asks(B, {{Ask::LiveHours, 20.0}, {Ask::Reads, 8.0}});
		B.Offer.Wants = {Spot::Overlay};
		B.Legacy = true;
		V.push_back(B);
	}
	{
		Brand B = Make("embercrest", "Team Embercrest", "Team Embercrest", Category::CardRoom, 2, 0x1b1512, 0xe8742c, 5);
		B.Rep = "Denise Ochoa";
		B.RepTitle = "Poker room manager";
		B.Venue = "embercrest";
		B.Offer.Days = 90;
		B.Offer.CoveredEntries = 2;
		B.Offer.CoveredCapCents = 12000;
		B.Offer.CoveredEvents = {"noon", "warmup", "nightly", "turbo"};
		B.Offer.CoversFares = true;
		B.Offer.TitleBonusCents = 25000;
		Asks(B, {{Ask::EmbercrestEvents, 8.0}, {Ask::Appearances, 1.0}});
		B.Offer.Wants = {Spot::Chest};
		V.push_back(B);
	}
	{
		Brand B = Make("halo", "Halo", "Halo", Category::Gear, 2, 0xa3e635, 0x0f1a05, 6);
		B.Kast = "Halo";
		B.Product = "Halo cameras";
		B.ReadLine = "That's the Halo on my face tonight. Sharp in the dark, kind to a bad night's sleep. Link's in the panels.";
		B.Rep = "Halo Creators";
		B.RepTitle = "Creator program";
		B.Needs.Followers = 500;
		B.Offer.Days = 60;
		B.Offer.PerLiveHourCents = 200;
		B.Offer.Perks = {{Gift::Gear, "camera", 1, 0, 0}};
		Asks(B, {{Ask::LiveHours, 15.0}});
		B.Offer.Wants = {Spot::Overlay};
		V.push_back(B);
	}
	{
		Brand B = Make("tunnelrat", "TunnelRat VPN", "TunnelRat", Category::Privacy, 3, 0x38bdf8, 0x04121c, 7);
		B.Kast = "TunnelRat VPN";
		B.Product = "TunnelRat VPN";
		B.ReadLine = "Quick one: TunnelRat VPN keeps your connection private wherever you play. Link below, first month free.";
		B.Rep = "Sol Brenner";
		B.RepTitle = "Partner team";
		B.Needs.Followers = 1000;
		B.Needs.AvgViewers = 30;
		B.Offer.Days = 30;
		B.Offer.PerLiveHourCents = 600;
		B.Offer.PerReadCents = 2500;
		Asks(B, {{Ask::LiveHours, 25.0}, {Ask::Reads, 8.0}});
		B.Offer.Wants = {Spot::Overlay};
		B.Legacy = true;
		V.push_back(B);
	}
	{
		Brand B = Make("stacked", "Stacked Apparel", "Stacked", Category::Apparel, 3, 0xf59e0b, 0x1a1204, 8);
		B.Kast = "Stacked Apparel";
		B.Product = "Stacked hoodies and caps";
		B.ReadLine = "Hoodie's from Stacked Apparel. Built for twelve-hour sessions. Code in the panels for 20% off.";
		B.Rep = "Theo Marsh";
		B.RepTitle = "Brand partnerships";
		B.Needs.Followers = 2500;
		B.Needs.AvgViewers = 60;
		B.Offer.Days = 30;
		B.Offer.PerLiveHourCents = 1000;
		B.Offer.PerReadCents = 4000;
		Asks(B, {{Ask::LiveEvents, 4.0}, {Ask::LiveHours, 25.0}, {Ask::Reads, 8.0}});
		B.Offer.Wants = {Spot::Chest, Spot::Cap};
		B.Legacy = true;
		V.push_back(B);
	}
	{
		Brand B = Make("rangewise", "Rangewise", "Rangewise", Category::Training, 3, 0x6366f1, 0xeef2ff, 9);
		B.Rep = "Wes Abara";
		B.RepTitle = "Head of content";
		B.Needs.Partner = true;
		B.Offer.Days = 180;
		B.Offer.MonthlyCents = 40000;
		B.Offer.PerTeachingCents = 15000;
		B.Offer.TeachingCap = 4;
		Asks(B, {{Ask::TeachingStreams, 4.0}});
		B.Offer.Wants = {Spot::Sleeve};
		V.push_back(B);
	}
	{
		Brand B = Make("riverline", "Team RiverLine", "Team RiverLine", Category::Site, 4, 0x27d3c3, 0x04201d, 10);
		B.Kast = "RiverLine";
		B.Product = "Team RiverLine streamer contract";
		B.ReadLine = "Everything you see tonight is on RiverLine. New players: the welcome freeroll is in my panels. Team RiverLine!";
		B.Rep = "RiverLine Player Relations";
		B.RepTitle = "Team RiverLine";
		B.Needs.Followers = 2500;
		B.Needs.AvgViewers = 75;
		B.Needs.Partner = true;
		B.Offer.Days = 180;
		B.Offer.MonthlyCents = 150000;
		B.Offer.PerLiveHourCents = 2500;
		B.Offer.PerReadCents = 6000;
		B.Offer.Perks = {{Gift::Ticket, "daily-main", 2, 10900, 30}};
		Asks(B, {{Ask::LiveHours, 60.0}, {Ask::Reads, 12.0}, {Ask::CommunityStreams, 1.0}});
		B.Offer.Wants = {Spot::Chest};
		B.Legacy = true;
		V.push_back(B);
	}
	{
		Brand B = Make("sitwell", "Sitwell", "Sitwell", Category::Gear, 4, 0xfb923c, 0x1f0d02, 11);
		B.Kast = "Sitwell";
		B.Product = "Sitwell Pro gaming chair";
		B.ReadLine = "Twelve hours in and my back is fine. That's the Sitwell Pro. Link's below.";
		B.Rep = "Nadia Ferro";
		B.RepTitle = "Creator partnerships";
		B.Needs.Followers = 10000;
		B.Needs.AvgViewers = 300;
		B.Needs.Partner = true;
		B.Offer.Days = 30;
		B.Offer.PerLiveHourCents = 3500;
		B.Offer.PerReadCents = 9000;
		B.Offer.Perks = {{Gift::Gear, "chair", 1, 32900, 0}};
		Asks(B, {{Ask::LiveHours, 40.0}, {Ask::Reads, 8.0}});
		B.Offer.Wants = {Spot::Overlay};
		B.Legacy = true;
		V.push_back(B);
	}
	{
		// The poker site that isn't RiverLine (§1.9 #15): named for the street before the river. Tier 5 needs venues that
		// aren't playable yet, so it never looks on its own (P2); ss.Sponsor.Offer turnstone brings it.
		Brand B = Make("turnstone", "Turnstone Poker", "Turnstone", Category::Site, 5, 0x334155, 0xd4a843, 12);
		B.Rep = "Ingrid Vale";
		B.RepTitle = "Head of sponsored pros";
		B.Needs.Open = false;
		B.Offer.Days = 365;
		B.Offer.MonthlyCents = 400000;
		B.Offer.FinalTableBonusCents = 250000;
		Asks(B, {{Ask::LiveEvents, 4.0}});
		B.Offer.Wants = {Spot::Chest};
		V.push_back(B);
	}
	for (Brand& B : V)
	{
		// Every brand's offer letter has its wordings in the copy table (local brands make theirs in person).
		B.Pitch.clear();
		for (int K = 0; K < 2; ++K)
		{
			const std::string T = Text(sponsor_copy::Body, "offer:" + B.Id + ":" + std::to_string(K));
			if (!T.empty() && T.find("offer:") == std::string::npos)
			{
				B.Pitch.push_back("offer:" + B.Id + ":" + std::to_string(K));
			}
		}
	}
	return V;
}
} // namespace sponsor_catalog_detail

const std::vector<Brand>& Brands()
{
	static const std::vector<Brand> All = sponsor_catalog_detail::Build();
	return All;
}

const Brand* FindBrand(const std::string& Id)
{
	for (const Brand& B : Brands())
	{
		if (B.Id == Id)
		{
			return &B;
		}
	}
	return nullptr;
}

const Brand* FindBrandByName(const std::string& Name)
{
	if (Name.empty())
	{
		return nullptr;
	}
	for (const Brand& B : Brands())
	{
		if (B.Name == Name || B.Short == Name || (!B.Kast.empty() && B.Kast == Name))
		{
			return &B;
		}
	}
	return nullptr;
}

bool ExclusiveCategory(Category C)
{
	return C == Category::Energy || C == Category::Apparel || C == Category::Training || C == Category::CardRoom || C == Category::Site;
}

const char* CategoryWord(Category C)
{
	switch (C)
	{
	case Category::Local: return "local";
	case Category::Energy: return "energy-drink";
	case Category::Apparel: return "apparel";
	case Category::Gear: return "gear";
	case Category::Privacy: return "privacy";
	case Category::Training: return "training";
	case Category::CardRoom: return "card-room";
	case Category::Site: return "poker-site";
	case Category::Count: break;
	}
	return "sponsor";
}

const char* SpotWord(Spot S)
{
	switch (S)
	{
	case Spot::Chest: return "chest";
	case Spot::Sleeve: return "sleeve";
	case Spot::Cap: return "cap";
	case Spot::Overlay: return "overlay";
	case Spot::Count: break;
	}
	return "patch";
}

// ------------------------------------------------------------------ standing (§3.3)

namespace sponsor_standing_detail
{
/** The stream road: Kast's followers and nightly average, and Partner. */
int StreamRoad(const rep::Public& P)
{
	int T = 0;
	T = P.Followers >= 150 ? 1 : T;
	T = P.Followers >= 500 && P.Avg30 >= 20.0 ? 2 : T;
	T = P.Partner ? std::max(T, 3) : T;
	T = P.Followers >= 5000 && P.Avg30 >= 150.0 ? std::max(T, 4) : T;
	T = P.Followers >= 25000 && P.Avg30 >= 600.0 ? 5 : T;
	return T;
}

/** The live road (the Embercrest's; tiers 4 and 5 open when their venues are playable). */
int LiveRoad(const rep::Public& P)
{
	int Live = P.LiveEvents >= 3 ? 1 : 0;
	Live = (P.Embercrest60 >= 8 && P.Cashes60 >= 2) || (P.BestEmbercrestPlace > 0 && P.BestEmbercrestPlace <= 6) ? 2 : Live;
	Live = P.Titles >= 1 || P.Cashes90 >= 5 ? 3 : Live;
	return Live;
}

/**
 * The creator road (update 2 SPEC §4.7 item 5): Rerun's subscribers, its views in the last 30 days and Partner, from
 * rep::Public (rep::Of fills them from life::State::Video; an old save has no channel, so the road is empty and every
 * old book behaves as it did). The real platforms' partner gate is 1,000 subscribers, so the first tier starts there;
 * the poker niche saturates past about 20,000 (§4.6), so tiers 4 and 5 are years of steady work or a viral run.
 */
int CreatorRoad(const rep::Public& P)
{
	int T = P.VideoSubs >= 1000 ? 1 : 0;
	T = P.VideoSubs >= 5000 && P.Views30 >= 20000 ? std::max(T, 2) : T;
	T = P.VideoPartner && P.VideoSubs >= 10000 ? std::max(T, 3) : T;
	T = P.VideoSubs >= 50000 && P.Views30 >= 150000 ? std::max(T, 4) : T;
	T = P.VideoSubs >= 250000 ? 5 : T;
	return T;
}

/** The stream road, the live road and the creator road, before the conduct limits: standing takes the best of them. */
int Roads(const rep::Public& P)
{
	return std::max({StreamRoad(P), LiveRoad(P), CreatorRoad(P)});
}

/** The creator road is what lifted standing (above both the others): the offer letter says so (why:rerun). */
bool CreatorLeads(const rep::Public& P)
{
	const int Creator = CreatorRoad(P);
	return Creator > 0 && Creator > std::max(StreamRoad(P), LiveRoad(P));
}

/** The why:rerun line for what the creator road's tier stands on: Partner (3), the month's views (2, 4), or subscribers. */
std::string CreatorWhyKey(const rep::Public& P)
{
	const int Creator = CreatorRoad(P);
	return Creator == 3 ? "why:rerun:partner" : Creator == 2 || Creator == 4 ? "why:rerun:views" : "why:rerun";
}

int Limited(const rep::Public& P, const Brand* B, int T)
{
	if (P.PickedUp90)
	{
		T = std::min(T, 1);
	}
	if (P.Restricted180 && B && (B->Cat == Category::Site || B->Cat == Category::Training))
	{
		T = 0;
	}
	return T;
}
} // namespace sponsor_standing_detail

int Tier(const rep::Public& P, const std::string& BrandId)
{
	return sponsor_standing_detail::Limited(P, FindBrand(BrandId), sponsor_standing_detail::Roads(P));
}

int TierOf(const rep::Public& P, const Book& B, const life::State& L, const std::string& BrandId, double World)
{
	const Brand* Br = FindBrand(BrandId);
	int T = sponsor_standing_detail::Roads(P);
	T = std::max(T, B.DebugTier);
	// Dee's road: her game up $200, lifetime.
	T = L.BackRoomNetCents >= 20000 ? std::max(T, 1) : T;
	// The block road, at that shop only.
	if (Br && !Br->Venue.empty() && Br->Cat == Category::Local && VisitDays(B, Br->Venue, World) >= BlockDays)
	{
		T = std::max(T, 1);
	}
	return sponsor_standing_detail::Limited(P, Br, T);
}

const char* StandingWords(int Tier)
{
	switch (Tier)
	{
	case 1: return "Known on the block";
	case 2: return "A face in the room";
	case 3: return "A name in the city";
	case 4: return "On the circuit";
	case 5: return "A poker personality";
	default: return "Nobody's calling yet";
	}
}

int VisitDays(const Book& B, const std::string& Venue, double World)
{
	const auto It = B.Days.find(Venue);
	if (It == B.Days.end())
	{
		return 0;
	}
	const int Today = net::DayOf(World);
	int N = 0;
	for (const int D : It->second)
	{
		N += D <= Today && Today - D < BlockWindow ? 1 : 0;
	}
	return N;
}

void NoteVisit(Book& B, const std::string& Venue, double World)
{
	std::vector<int>& Days = B.Days[Venue];
	const int Today = net::DayOf(World);
	if (Days.empty() || Days.back() != Today)
	{
		Days.push_back(Today);
	}
	Days.erase(std::remove_if(Days.begin(), Days.end(), [Today](int D) { return Today - D >= BlockWindow; }), Days.end());
}

// ------------------------------------------------------------------ contracts: lookups and periods

const Contract* Find(const Book& B, const std::string& ContractId)
{
	for (const Contract& C : B.Contracts)
	{
		if (C.Id == ContractId)
		{
			return &C;
		}
	}
	return nullptr;
}

Contract* Find(Book& B, const std::string& ContractId)
{
	for (Contract& C : B.Contracts)
	{
		if (C.Id == ContractId)
		{
			return &C;
		}
	}
	return nullptr;
}

const Contract* Running(const Book& B, const std::string& BrandId, double World)
{
	for (const Contract& C : B.Contracts)
	{
		if (C.Brand == BrandId && RunningAt(C, World))
		{
			return &C;
		}
	}
	return nullptr;
}

bool WouldStrike(const Book& B, int Site, bool Streaming, bool Video)
{
	// Phase A's stub (core-deals, G3): no contract carries a "plays only on" clause yet.
	(void)B;
	(void)Site;
	(void)Streaming;
	(void)Video;
	return false;
}

void NoteSitePlay(Book& B, life::State& L, int Site, double World, bool Streaming, bool Video)
{
	// Phase A's stub (core-deals, G3): nothing to strike yet.
	(void)B;
	(void)L;
	(void)Site;
	(void)World;
	(void)Streaming;
	(void)Video;
}

bool Active(const Book& B, const std::string& BrandId, double World)
{
	return Running(B, BrandId, World) != nullptr;
}

int Periods(const Contract& C)
{
	const double Len = std::max(0.0, C.Until - C.Since);
	return std::max(1, static_cast<int>(std::ceil(Len / (PeriodDays * DayMinutes) - 1e-9)));
}

double PeriodStart(const Contract& C, int Period)
{
	return C.Since + static_cast<double>(std::max(0, Period - 1)) * PeriodDays * DayMinutes;
}

double PeriodEnd(const Contract& C, int Period)
{
	return std::min(C.Until, C.Since + static_cast<double>(std::max(1, Period)) * PeriodDays * DayMinutes);
}

int PeriodAt(const Contract& C, double World)
{
	if (World < C.Since)
	{
		return 0;
	}
	const int P = static_cast<int>(std::floor((World - C.Since) / (PeriodDays * DayMinutes))) + 1;
	return std::min(P, Periods(C));
}

double Need(const Contract& C, Ask A, int Period)
{
	const double Base = C.T.Asks[static_cast<size_t>(A)];
	if (Base <= 0.0)
	{
		return 0.0;
	}
	const double Len = PeriodEnd(C, Period) - PeriodStart(C, Period);
	const double Scale = std::min(1.0, Len / (PeriodDays * DayMinutes));
	const double Want = Base * Scale;
	if (A == Ask::LiveHours)
	{
		return Want;
	}
	return std::max(1.0, std::round(Want));
}

double Have(const Tally& T, Ask A)
{
	switch (A)
	{
	case Ask::LiveHours: return T.LiveMinutes / 60.0;
	case Ask::Reads: return static_cast<double>(T.Reads);
	case Ask::TeachingStreams: return static_cast<double>(T.Teaching);
	case Ask::LiveEvents: return static_cast<double>(T.LiveEvents);
	case Ask::EmbercrestEvents: return static_cast<double>(T.Embercrest);
	case Ask::Workouts: return static_cast<double>(T.Workouts);
	case Ask::Appearances: return static_cast<double>(T.Appearances);
	case Ask::CommunityStreams: return static_cast<double>(T.Community);
	case Ask::Count: break;
	}
	return 0.0;
}

const char* AskLabel(Ask A)
{
	switch (A)
	{
	case Ask::LiveHours: return "Live hours";
	case Ask::Reads: return "Reads";
	case Ask::TeachingStreams: return "Teaching streams";
	case Ask::LiveEvents: return "Live events in the patch";
	case Ask::EmbercrestEvents: return "Embercrest events";
	case Ask::Workouts: return "Workouts";
	case Ask::Appearances: return "Appearances";
	case Ask::CommunityStreams: return "Community stream";
	case Ask::Count: break;
	}
	return "";
}

namespace sponsor_measure_detail
{
/** The teaching title's text (kast::Titles()' Teaching entry: the log keeps a title's text, Kast.cpp). */
const std::string& TeachingTitle()
{
	static const std::string T = [] {
		for (const kast::TitleSpec& S : kast::Titles())
		{
			if (S.Tag == "Teaching")
			{
				return S.Text;
			}
		}
		return std::string();
	}();
	return T;
}

/** The first Saturday of its month (the community stream's day). */
bool FirstSaturday(int Day)
{
	if (net::Weekday(Day) != 5)
	{
		return false;
	}
	int Y = 0;
	int M = 0;
	int D = 0;
	world::CivilDate(Day, Y, M, D);
	return D <= 7;
}

/** The player's own patch rows on an entry (anyone not in the roster): did they wear Brand? */
bool Wore(const life::LiveEntry& E, const std::string& BrandId)
{
	for (const std::pair<std::string, std::string>& P : E.Patches)
	{
		if (P.second != BrandId)
		{
			continue;
		}
		const bool Field = std::any_of(E.Roster.begin(), E.Roster.end(), [&P](const std::pair<std::string, int>& R) { return R.first == P.first; });
		if (!Field)
		{
			return true;
		}
	}
	return false;
}

/** The record-based asks over From..To: streams started then, entries finished then. */
Tally Window(const Contract& C, const life::State& L, const kast::Channel& Ch, double From, double To, double StreamStart)
{
	Tally T;
	for (const kast::StreamLog& S : Ch.Log)
	{
		if (S.Start < From || S.Start >= To)
		{
			continue;
		}
		T.LiveMinutes += S.Minutes;
		T.Teaching += S.Title == TeachingTitle() && !S.Title.empty() && S.Minutes >= 120.0 ? 1 : 0;
		T.Community += FirstSaturday(net::DayOf(S.Start)) ? 1 : 0;
	}
	if (StreamStart >= From && StreamStart < To)
	{
		T.LiveMinutes += std::max(0.0, To - StreamStart);
		T.Community += FirstSaturday(net::DayOf(StreamStart)) ? 1 : 0;
	}
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State != life::LiveEntry::Finished || E.FinishedAt < From || E.FinishedAt >= To)
		{
			continue;
		}
		T.Embercrest += live::IsEmbercrest(E.Id) ? 1 : 0;
		T.LiveEvents += Wore(E, C.Brand) ? 1 : 0;
	}
	return T;
}
} // namespace sponsor_measure_detail

Tally Measure(const Contract& C, const life::State& L, const kast::Channel& Ch, double World, double StreamStart)
{
	const int P = std::max(1, std::min(C.Period + 1, Periods(C)));
	const double From = PeriodStart(C, P);
	const double To = std::min(World, PeriodEnd(C, P));
	Tally T = sponsor_measure_detail::Window(C, L, Ch, From, std::max(From, To), StreamStart);
	T.Reads = C.Now.Reads;
	T.Appearances = C.Now.Appearances;
	T.CoveredUsed = C.Now.CoveredUsed;
	T.Workouts = C.WorkoutBase < 0 ? 0 : std::max(0, L.Fit.Workouts - C.WorkoutBase);
	return T;
}

// ------------------------------------------------------------------ gates and offers

namespace sponsor_offer_detail
{
bool OwnsWebcam(const gear::Owned* Gear)
{
	return Gear && (Gear->count("webcam-720") > 0 || Gear->count("webcam-1080") > 0 || Gear->count("mirrorless") > 0);
}

/** The brand's own gate (§3.4), "" when it's met. */
std::string Gate(const Brand& Br, const Book& B, const life::State& L, const rep::Public& P, double World, const gear::Owned* Gear)
{
	const int T = TierOf(P, B, L, Br.Id, World);
	const auto Bought = L.Bought.find("volt-rush");
	const int Cans = Bought == L.Bought.end() ? 0 : Bought->second;
	const bool Title = P.Titles >= 1;
	if (Br.Id == "luckypenny")
	{
		return VisitDays(B, "store", World) >= BlockDays ? std::string() : std::string("Twelve days at the counter in two months.");
	}
	if (Br.Id == "barber")
	{
		return T >= 1 && L.Groom.Cuts >= 5 ? std::string() : std::string("Five cuts in Lonnie's chair, and known on the block.");
	}
	if (Br.Id == "ironhouse")
	{
		return T >= 1 && L.Fit.Workouts >= 10 ? std::string() : std::string("Ten workouts, and known on the block.");
	}
	if (Br.Id == "voltrush")
	{
		return T >= 2 && Cans >= 6 ? std::string() : std::string("A face in the room, and six cans bought.");
	}
	if (Br.Id == "embercrest")
	{
		const bool Room = P.Embercrest60 >= 8 && P.Cashes60 >= 2;
		const bool Final = P.BestEmbercrestPlace > 0 && P.BestEmbercrestPlace <= 6;
		return Room || Final ? std::string() : std::string("Eight Embercrest events and two cashes in two months, or a final table.");
	}
	if (Br.Id == "halo")
	{
		return P.Followers >= 500 || (T >= 2 && OwnsWebcam(Gear)) ? std::string() : std::string("500 followers, or a face in the room with a webcam.");
	}
	if (Br.Id == "stacked")
	{
		return (P.Followers >= 2500 && P.Avg30 >= 60.0) || Title ? std::string() : std::string("2,500 followers and 60 a night, or an Embercrest title.");
	}
	if (Br.Id == "rangewise")
	{
		return P.Partner || (Title && P.LiveCashes >= 5) ? std::string() : std::string("Partner, or a title and five cashes.");
	}
	if (Br.Id == "riverline")
	{
		const bool Stream = P.Partner && P.Followers >= 2500 && P.Avg30 >= 75.0;
		return Stream || T >= 4 ? std::string() : std::string("Partner with 2,500 followers and 75 a night.");
	}
	if (Br.Needs.Followers > 0 || Br.Needs.AvgViewers > 0 || Br.Needs.Partner)
	{
		const bool Ok = P.Followers >= Br.Needs.Followers && P.Avg30 >= static_cast<double>(Br.Needs.AvgViewers) && (!Br.Needs.Partner || P.Partner);
		return Ok ? std::string() : Grouped(Br.Needs.Followers) + " followers and " + std::to_string(Br.Needs.AvgViewers) + " a night" + (Br.Needs.Partner ? ", and Partner." : ".");
	}
	return T >= Br.Tier ? std::string() : std::string(StandingWords(Br.Tier)) + " first.";
}

/** Why the brand is calling now: the first sentence of its offer. */
std::string WhyLine(const Brand& Br, const life::State& L, const rep::Public& P, const gear::Owned* Gear)
{
	std::map<std::string, std::string> V;
	V["Count"] = std::to_string(P.Embercrest60);
	V["Cashes"] = std::to_string(P.Cashes60);
	V["Followers"] = Grouped(P.Followers);
	V["Subs"] = Grouped(P.VideoSubs);
	V["Views"] = Grouped(P.Views30);
	// The creator road (update 2 SPEC §4.7): where standing is what opened the door and Rerun is what lifted it, the
	// letter says Rerun, not the room or a Kast count the record doesn't have.
	const bool Creator = sponsor_standing_detail::CreatorLeads(P);
	std::string Key = Creator ? sponsor_standing_detail::CreatorWhyKey(P) : std::string("why:tier");
	std::string LastTitle;
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State == life::LiveEntry::Finished && E.Place == 1)
		{
			LastTitle = E.Name;
		}
	}
	V["Event"] = LastTitle.empty() ? std::string("Embercrest Nightly") : LastTitle;
	if (Br.Id == "voltrush")
	{
		const auto It = L.Bought.find("volt-rush");
		V["Count"] = std::to_string(It == L.Bought.end() ? 0 : It->second);
		Key = "why:cans";
	}
	else if (Br.Id == "embercrest")
	{
		Key = P.BestEmbercrestPlace > 0 && P.BestEmbercrestPlace <= 6 && !(P.Embercrest60 >= 8 && P.Cashes60 >= 2) ? "why:final" : "why:room";
	}
	else if (Br.Id == "halo")
	{
		// Below 500 followers Halo's gate is the webcam, which MakeOffer can't see, so that letter's reason is MakeOffer's
		// own (the channel's numbers when Rerun made the face).
		Key = P.Followers >= 500 || !OwnsWebcam(Gear) ? "why:channel" : "why:webcam";
	}
	else if (Br.Id == "stacked")
	{
		Key = !LastTitle.empty() && P.Followers < 2500 ? "why:title" : "why:channel";
	}
	else if (Br.Id == "rangewise" || Br.Id == "riverline")
	{
		// Team RiverLine short of its stream gate opened on standing (on the circuit): when Rerun lifted it, Rerun says why.
		const bool ByRerun = Br.Id == "riverline" && Creator && !(P.Partner && P.Followers >= 2500 && P.Avg30 >= 75.0);
		Key = ByRerun ? Key : P.Partner ? "why:partner" : !LastTitle.empty() ? "why:title" : "why:tier";
	}
	else if (!Br.Kast.empty())
	{
		Key = "why:channel";
	}
	return Fill(Text(sponsor_copy::Spoken, Key), V);
}

/** A new contract id: the brand and the day ("voltrush@42"), with a letter on the end if that's taken. */
std::string NewId(const Book& B, const std::string& BrandId, double World, const char* Tail = "")
{
	const std::string Base = BrandId + "@" + std::to_string(net::DayOf(World)) + Tail;
	std::string Id = Base;
	for (char Ch = 'b'; Find(B, Id) && Ch <= 'z'; ++Ch)
	{
		Id = Base + Ch;
	}
	return Id;
}

/** Brand's weekly look: its weekday (mail on weekdays) and the minute it looks that day. */
int LookWeekday(const Brand& Br)
{
	return static_cast<int>(Hash("look:" + Br.Id) % (Mails(Br) ? 5u : 6u));
}

double LookAt(const Brand& Br, int Day, const std::string& Seed)
{
	const double Minute = Mails(Br) ? 9.0 * 60.0 + static_cast<double>(Hash(Seed + ":sponsor:" + Br.Id + ":" + std::to_string(Day)) % 540u) : 10.0 * 60.0 + 30.0;
	return static_cast<double>(Day) * DayMinutes + Minute;
}

/** The brand's next look strictly after T. */
double NextLook(const Brand& Br, double T, const std::string& Seed)
{
	const int Want = LookWeekday(Br);
	for (int Day = net::DayOf(T); Day <= net::DayOf(T) + 8; ++Day)
	{
		if (net::Weekday(Day) == Want)
		{
			const double At = LookAt(Br, Day, Seed);
			if (At > T)
			{
				return At;
			}
		}
	}
	return 1.0e18;
}
} // namespace sponsor_offer_detail

std::string Eligible(const Book& B, const life::State& L, const rep::Public& P, const std::string& BrandId, double World, const gear::Owned* Gear)
{
	const Brand* Br = FindBrand(BrandId);
	if (!Br)
	{
		return "No such brand.";
	}
	if (!Br->Needs.Open)
	{
		return Br->Short + " isn't signing players here yet.";
	}
	for (const Contract& C : B.Contracts)
	{
		if (C.Brand == BrandId && (OpenOffer(C) || (Live(C) && C.Until > World)))
		{
			return OpenOffer(C) ? "There's already an offer on the table." : "You're already signed.";
		}
	}
	for (const std::string& Key : {BrandId, std::string("cat:") + std::to_string(static_cast<int>(Br->Cat)), std::string("*")})
	{
		const auto It = B.Cooldown.find(Key);
		if (It != B.Cooldown.end() && It->second > World)
		{
			return Br->Short + " isn't calling until " + net::DateLabel(net::DayOf(It->second)) + ".";
		}
	}
	// A rival in an exclusive category stays quiet while another brand holds it, until that deal's last month (when a
	// rival's offer is what gives the incumbent its right to match, §3.7).
	for (const Contract& C : B.Contracts)
	{
		const Brand* Cb = BrandOf(C);
		if (Cb && C.Brand != BrandId && Cb->Cat == Br->Cat && ExclusiveCategory(Br->Cat) && Live(C) && (C.T.Exclusive || Br->Offer.Exclusive) &&
			C.Until - World > PeriodDays * DayMinutes)
		{
			return Cb->Short + " has the " + CategoryWord(Br->Cat) + " category.";
		}
	}
	if (P.PickedUp90 && Br->Tier > 1)
	{
		return "Not with your name in the paper.";
	}
	if (P.Restricted180 && (Br->Cat == Category::Site || Br->Cat == Category::Training))
	{
		return "Not with your RiverLine account restricted.";
	}
	return sponsor_offer_detail::Gate(*Br, B, L, P, World, Gear);
}

std::string MakeOffer(Book& B, life::State& L, const rep::Public& P, const std::string& BrandId, double World, const std::string& Seed, bool Force)
{
	using namespace sponsor_offer_detail;
	const Brand* Br = FindBrand(BrandId);
	if (!Br)
	{
		return std::string();
	}
	const std::string Why = Eligible(B, L, P, BrandId, World);
	if (!Why.empty() && !Force)
	{
		return std::string();
	}
	for (const Contract& C : B.Contracts)
	{
		if (C.Brand == BrandId && (OpenOffer(C) || (Live(C) && C.Until > World)))
		{
			return std::string();
		}
	}
	Contract C;
	C.Id = NewId(B, BrandId, World);
	C.Brand = BrandId;
	C.St = State::Offered;
	C.T = Br->Offer;
	C.OfferedAt = World;
	C.ExpiresAt = World + OfferDays * DayMinutes;
	C.TierAtSigning = TierOf(P, B, L, BrandId, World);
	B.Contracts.push_back(C);
	if (Mails(*Br) && !Br->Pitch.empty())
	{
		const size_t Variant = Hash(Seed + ":sponsor:" + BrandId + ":" + std::to_string(net::DayOf(World))) % Br->Pitch.size();
		// Why they're calling now; an offer put on the table past its gate (ss.Sponsor.Offer, or a gate this call can't see,
		// like Halo's webcam) says only that the room knows the name, or, when Rerun is what lifted standing, the channel's
		// own numbers: never a number the record doesn't have.
		std::string Reason = Fill(Text(sponsor_copy::Spoken, "why:tier"), {});
		if (Why.empty())
		{
			Reason = WhyLine(*Br, L, P, nullptr);
		}
		else if (sponsor_standing_detail::CreatorLeads(P))
		{
			Reason = Fill(Text(sponsor_copy::Spoken, sponsor_standing_detail::CreatorWhyKey(P)), {{"Subs", Grouped(P.VideoSubs)}, {"Views", Grouped(P.Views30)}});
		}
		const std::string Body = Fill(Text(sponsor_copy::Body, Br->Pitch[Variant]), {{"Why", Reason}});
		Post(L, World, B.Contracts.back(), LetterOffer, "offer", Body);
	}
	return C.Id;
}

void HeardOffer(Book& B, life::State& L, const std::string& ContractId, double World)
{
	const Contract* C = Find(B, ContractId);
	if (!C || !OpenOffer(*C) || !BrandOf(*C) || Mails(*BrandOf(*C)))
	{
		return;
	}
	for (const life::Letter& M : L.Inbox)
	{
		if (M.Thread == ContractId && M.Kind == LetterHeard)
		{
			return;
		}
	}
	Post(L, World, *C, LetterHeard, "heard", BodyFor(*C, "heard"));
}

std::string OpenOfferFor(const Book& B, const std::string& BrandId, double World)
{
	for (const Contract& C : B.Contracts)
	{
		if (C.Brand == BrandId && OpenOffer(C) && C.ExpiresAt > World)
		{
			return C.Id;
		}
	}
	return std::string();
}

// ------------------------------------------------------------------ signing: exclusivity, the overlay, the patch spots

namespace sponsor_sign_detail
{
/** Each brand one patch spot of those it takes, no two on one: can they all be placed? */
bool Assignable(const std::vector<std::vector<Spot>>& Needs, size_t I, std::array<bool, PatchSpots>& Used)
{
	if (I == Needs.size())
	{
		return true;
	}
	for (const Spot S : Needs[I])
	{
		const size_t K = static_cast<size_t>(S);
		if (K < Used.size() && !Used[K])
		{
			Used[K] = true;
			if (Assignable(Needs, I + 1, Used))
			{
				return true;
			}
			Used[K] = false;
		}
	}
	return false;
}

std::string SpotsWords(const std::vector<Spot>& Spots)
{
	std::vector<std::string> W;
	for (const Spot S : Spots)
	{
		W.push_back(SpotWord(S));
	}
	std::string Out;
	for (size_t I = 0; I < W.size(); ++I)
	{
		Out += (I == 0 ? "" : I + 1 == W.size() ? " or " : ", ") + W[I];
	}
	return Out;
}

/** What signing C collides with ("" when nothing): an exclusive category, the overlay's two slots, the patch spots. */
std::string Collides(const Book& B, const Contract& C, double World)
{
	const Brand* Br = BrandOf(C);
	if (!Br)
	{
		return "No such brand.";
	}
	const double Starts = std::max(World, C.Since);
	int Overlays = 0;
	std::vector<std::vector<Spot>> Needs;
	for (const Contract& X : B.Contracts)
	{
		if (&X == &C || !Live(X) || X.Until <= Starts || X.Brand == C.Brand)
		{
			continue;
		}
		const Brand* Xb = BrandOf(X);
		if (Xb && Xb->Cat == Br->Cat && ExclusiveCategory(Br->Cat) && (X.T.Exclusive || C.T.Exclusive))
		{
			return Xb->Short + " has your " + CategoryWord(Br->Cat) + " category until " + net::DateLabel(net::DayOf(X.Until)) + ".";
		}
		Overlays += HasOverlay(X) ? 1 : 0;
		const std::vector<Spot> S = SpotsOf(X.T);
		if (!S.empty())
		{
			Needs.push_back(S);
		}
	}
	if (HasOverlay(C) && Overlays >= kast::MaxDeals)
	{
		return "Both overlay slots are spoken for.";
	}
	const std::vector<Spot> Mine = SpotsOf(C.T);
	if (!Mine.empty())
	{
		Needs.push_back(Mine);
		std::array<bool, PatchSpots> Used{};
		if (!Assignable(Needs, 0, Used))
		{
			return "No room for " + Possessive(Br->Short) + " patch: your " + SpotsWords(Mine) + " is spoken for.";
		}
	}
	return std::string();
}
} // namespace sponsor_sign_detail

std::string CanSign(const Book& B, const life::State& L, const std::string& ContractId, double World)
{
	(void)L;
	const Contract* C = Find(B, ContractId);
	if (!C)
	{
		return "That offer's gone.";
	}
	if (C->St == State::Countered)
	{
		return "Waiting on their answer.";
	}
	if (Live(*C))
	{
		return "You've already signed it.";
	}
	if (C->St != State::Offered || C->ExpiresAt <= World)
	{
		return "That offer's gone.";
	}
	return sponsor_sign_detail::Collides(B, *C, World);
}

std::vector<std::string> Conflicts(const Book& B, const life::State& L, const std::string& ContractId, double World)
{
	std::vector<std::string> Out;
	const Contract* C = Find(B, ContractId);
	if (!C)
	{
		return Out;
	}
	const std::string Why = CanSign(B, L, ContractId, World);
	if (!Why.empty() && OpenOffer(*C))
	{
		Out.push_back(Why);
	}
	const Brand* Br = BrandOf(*C);
	if (!Br || !ExclusiveCategory(Br->Cat))
	{
		return Out;
	}
	for (const Contract& X : B.Contracts)
	{
		const Brand* Xb = BrandOf(X);
		if (&X != C && OpenOffer(X) && X.ExpiresAt > World && Xb && Xb->Cat == Br->Cat && X.Brand != C->Brand && (X.T.Exclusive || C->T.Exclusive))
		{
			Out.push_back("Only one can be signed: signing passes on " + Xb->Short + ".");
		}
	}
	return Out;
}

std::string Sign(Book& B, life::State& L, Chips& Bankroll, const std::string& ContractId, double World)
{
	const std::string Why = CanSign(B, L, ContractId, World);
	if (!Why.empty())
	{
		return Why;
	}
	Contract* C = Find(B, ContractId);
	double Since = World;
	if (!C->Renews.empty())
	{
		if (const Contract* Prior = Find(B, C->Renews))
		{
			Since = Live(*Prior) ? std::max(World, Prior->Until) : World;
		}
	}
	C->St = State::Active;
	C->Since = Since;
	C->Until = Since + static_cast<double>(C->T.Days) * DayMinutes;
	C->Period = 0;
	C->Now = Tally();
	C->WorkoutBase = L.Fit.Workouts;
	C->MetSum = 0.0;
	C->Pending = -1;
	const std::string Id = C->Id;
	const std::string BrandId = C->Brand;
	const Brand* Br = FindBrand(BrandId);
	// A signing bonus is paid on signing (and comes back, in proportion, under the morals clause).
	if (C->T.SigningCents > 0 && B.PaidKeys.insert("signing|" + Id).second)
	{
		Bankroll += C->T.SigningCents;
		L.Record(World, ShortOf(BrandId) + ": signing", C->T.SigningCents, life::LedgerSponsors);
		C->PaidCents += C->T.SigningCents;
	}
	// The month's tickets (Team RiverLine's Daily Mains), the first at signing. A renewal signed ahead starts later: Step
	// grants its first month's when it starts, and counts its workouts from then (WorkoutBase -1 until it does).
	if (Since > World)
	{
		C->WorkoutBase = -1;
	}
	for (const Perk& Pk : C->T.Perks)
	{
		if (Pk.Kind == Gift::Ticket && Since <= World && B.PaidKeys.insert("ticket|" + Id + "|1").second)
		{
			L.Tickets[Pk.Id] += Pk.Count;
		}
	}
	// Signing an exclusive category passes on the other offers in it.
	if (Br && ExclusiveCategory(Br->Cat))
	{
		for (Contract& X : B.Contracts)
		{
			const Brand* Xb = BrandOf(X);
			if (X.Id != Id && OpenOffer(X) && Xb && Xb->Cat == Br->Cat && X.Brand != BrandId)
			{
				X.St = State::Ended;
				X.Why = End::Declined;
				X.Pending = -1;
				B.Cooldown[X.Brand] = std::max(B.Cooldown[X.Brand], World + DeclineCooldownDays * DayMinutes);
			}
		}
	}
	C = Find(B, Id);
	Post(L, World, *C, LetterWelcome, "welcome", BodyFor(*C, "welcome"));
	return std::string();
}

void Decline(Book& B, const std::string& ContractId, double World)
{
	Contract* C = Find(B, ContractId);
	if (!C || !OpenOffer(*C))
	{
		return;
	}
	C->St = State::Ended;
	C->Why = End::Declined;
	C->Pending = -1;
	B.Cooldown[C->Brand] = std::max(B.Cooldown[C->Brand], World + DeclineCooldownDays * DayMinutes);
}


std::string GiveNotice(Book& B, life::State& L, const std::string& ContractId, double World)
{
	Contract* C = Find(B, ContractId);
	if (!C || !Live(*C) || C->Until <= World)
	{
		return "There's no deal to end.";
	}
	if (C->St == State::Notice)
	{
		return "You've already given notice.";
	}
	if (C->Since > World)
	{
		return "It hasn't started yet.";
	}
	C->St = State::Notice;
	B.NextAt = 0.0;
	const int P = std::max(1, std::min(C->Period + 1, Periods(*C)));
	Post(L, World, *C, LetterNotice, "notice:ack", BodyFor(*C, "notice:ack", {{"Until", DateWords(PeriodEnd(*C, P))}}));
	return std::string();
}

// ------------------------------------------------------------------ leverage and counters (§3.7)

namespace sponsor_counter_detail
{
/** What each lever needs (§3.7's table). */
double Needs(Lever L)
{
	switch (L)
	{
	case Lever::Money: return 0.35;
	case Lever::Lighter: return 0.30;
	case Lever::Shorter: return 0.15;
	case Lever::OneMoreEntry: return 0.40;
	case Lever::NoExclusivity: return 0.80;
	}
	return 1.0;
}

bool Halves(Lever L)
{
	return L == Lever::Money || L == Lever::Lighter;
}

/** 0 to 0.6: how far past the brand's gate the player stands. */
double PastGate(const Brand& Br, const rep::Public& P)
{
	double Past = std::max(0.0, 0.3 * static_cast<double>(Tier(P, Br.Id) - Br.Tier));
	if (Br.Needs.Followers > 0)
	{
		double Ratio = static_cast<double>(P.Followers) / static_cast<double>(Br.Needs.Followers);
		if (Br.Needs.AvgViewers > 0)
		{
			Ratio = std::min(Ratio, P.Avg30 / static_cast<double>(Br.Needs.AvgViewers));
		}
		Past = std::max(Past, (Ratio - 1.0) * 0.6);
	}
	return std::clamp(Past, 0.0, 0.6);
}

void Apply(Contract& C, Lever L, int Outcome)
{
	switch (L)
	{
	case Lever::Money: Scale(C.T, Outcome >= 2 ? 1.25 : 1.12); break;
	case Lever::Lighter:
		for (double& A : C.T.Asks)
		{
			A *= Outcome >= 2 ? 0.75 : 0.88;
		}
		break;
	case Lever::Shorter: C.T.Days = std::min(C.T.Days, 60); break;
	case Lever::OneMoreEntry: C.T.CoveredEntries += 1; break;
	case Lever::NoExclusivity: C.T.Exclusive = false; break;
	}
}
} // namespace sponsor_counter_detail

double Leverage(const Book& B, const rep::Public& P, const std::string& BrandId)
{
	const Brand* Br = FindBrand(BrandId);
	if (!Br)
	{
		return 0.0;
	}
	double L = sponsor_counter_detail::PastGate(*Br, P);
	// A rival's offer in the category on the table.
	for (const Contract& C : B.Contracts)
	{
		const Brand* Cb = BrandOf(C);
		L += OpenOffer(C) && Cb && C.Brand != BrandId && Cb->Cat == Br->Cat && Br->Cat != Category::Local ? 0.15 : 0.0;
	}
	// A clean previous term with them.
	for (const Contract& C : B.Contracts)
	{
		if (C.Brand == BrandId && C.St == State::Ended && (C.Why == End::Lapsed || C.Why == End::Walked) && C.Strikes == 0)
		{
			L += 0.1;
			break;
		}
	}
	// Strikes and welches in the last 180 days. Now is where the calendar has stepped to (it runs every frame); a book that
	// was never stepped reads from its latest strike.
	double Now = B.SteppedTo;
	for (const double S : B.Strikes)
	{
		Now = std::max(Now, S);
	}
	for (const double S : B.Strikes)
	{
		L -= Now - S < 180.0 * DayMinutes ? 0.2 : 0.0;
	}
	L -= P.Welched180 ? 0.2 : 0.0;
	return L;
}

int CounterOutcome(Lever L, double Leverage)
{
	const double Need = sponsor_counter_detail::Needs(L);
	if (Leverage + 1e-9 >= Need)
	{
		return 2;
	}
	return sponsor_counter_detail::Halves(L) && Leverage + 1e-9 >= Need - 0.2 ? 1 : 0;
}

double Margin(const Book& B, const rep::Public& P, const std::string& ContractId, Lever L)
{
	const Contract* C = Find(B, ContractId);
	return C ? Leverage(B, P, C->Brand) - sponsor_counter_detail::Needs(L) : -1.0;
}

std::string LeverLabel(Lever L)
{
	switch (L)
	{
	case Lever::Money: return "More money: +25% on the stipend and the rates.";
	case Lever::Lighter: return "Lighter: a quarter off what they ask.";
	case Lever::Shorter: return "Shorter: a sixty-day term.";
	case Lever::OneMoreEntry: return "One more covered entry a month.";
	case Lever::NoExclusivity: return "No exclusivity.";
	}
	return std::string();
}

std::string LeverBlocked(const Contract& C, Lever L)
{
	switch (L)
	{
	case Lever::Money: return HasMoney(C.T) ? std::string() : std::string("There's no money in it to raise.");
	case Lever::Lighter: return HasAsks(C.T) ? std::string() : std::string("They aren't asking for anything.");
	case Lever::Shorter: return C.T.Days > 60 ? std::string() : std::string("It's as short as they go.");
	case Lever::OneMoreEntry: return C.T.CoveredEntries > 0 ? std::string() : std::string("They don't cover entries.");
	case Lever::NoExclusivity: return C.T.Exclusive ? std::string() : std::string("It isn't exclusive.");
	}
	return std::string();
}

std::string Counter(Book& B, const rep::Public& P, const std::string& ContractId, Lever L, double World)
{
	Contract* C = Find(B, ContractId);
	if (!C || C->St != State::Offered || C->ExpiresAt <= World)
	{
		return C && C->St == State::Countered ? "Waiting on their answer." : "That offer's gone.";
	}
	if (C->Counters > 0)
	{
		return "One counter per offer.";
	}
	const std::string Blocked = LeverBlocked(*C, L);
	if (!Blocked.empty())
	{
		return Blocked;
	}
	const int Out = CounterOutcome(L, Leverage(B, P, C->Brand));
	C->St = State::Countered;
	C->Counters = 1;
	C->Pending = static_cast<int>(L) * 10 + Out;
	C->ReplyAt = NextBusinessMorning(World, C->Id + ":reply");
	B.NextAt = 0.0;
	return std::string();
}

std::string AskForRaise(Book& B, const rep::Public& P, const std::string& ContractId, double World)
{
	Contract* C = Find(B, ContractId);
	if (!C || !RunningAt(*C, World))
	{
		return "There's no deal to talk about.";
	}
	if (C->Raised)
	{
		return "You've asked once this term.";
	}
	if (C->Pending >= 0)
	{
		return "Waiting on their answer.";
	}
	if (!HasMoney(C->T))
	{
		return "There's no money in it to raise.";
	}
	const int Gained = Tier(P, C->Brand) - C->TierAtSigning;
	if (Gained < 1)
	{
		return "Your numbers haven't moved enough to ask.";
	}
	C->Raised = true;
	C->Pending = PendingRaise * 10 + std::min(4, Gained);
	C->ReplyAt = NextBusinessMorning(World, C->Id + ":raise");
	B.NextAt = 0.0;
	return std::string();
}

// ------------------------------------------------------------------ the calendar's step

namespace sponsor_step_detail
{
struct Ctx
{
	Book& B;
	life::State& L;
	Chips& Bankroll;
	const kast::Channel& Ch;
	const rep::Public& P;
	const std::string& Seed;
	const StepContext& X;
};

void Pay(Ctx& K, Contract& C, const std::string& Key, Chips Cents, const std::string& Label, double At)
{
	if (Cents <= 0 || !K.B.PaidKeys.insert(Key).second)
	{
		return;
	}
	K.Bankroll += Cents;
	K.L.Record(At, Label, Cents, life::LedgerSponsors);
	C.PaidCents += Cents;
}

bool Sewn(const life::State& L, const std::string& BrandId)
{
	return std::find(L.Closet.Patches.begin(), L.Closet.Patches.end(), BrandId) != L.Closet.Patches.end();
}

/** The patch line for an ending letter ("Cut the penny off the jacket."), when one was sewn. */
std::string PatchLine(const life::State& L, const Contract& C)
{
	return Sewn(L, C.Brand) ? BodyFor(C, "patch") : std::string();
}

/** The line for an ending letter when the brand's gear goes back (§3.6: a term that didn't finish clean). Ended for cause,
 *  it always does; walked away from or not renewed, a next term of theirs running on keeps it the brand's (EndContract). */
std::string ReturnedLine(const Book& B, const Contract& C, double At, bool ForCause)
{
	if (C.Property.empty())
	{
		return std::string();
	}
	for (const Contract& X : B.Contracts)
	{
		if (!ForCause && X.Id != C.Id && X.Brand == C.Brand && Live(X) && X.Until > At)
		{
			return std::string();
		}
	}
	return BodyFor(C, "returned");
}

/** Ends a contract: its state, the cooldowns, the patches off (unless a renewal runs on), a renewal signed ahead cancelled
 *  when it ends badly. The letter is the caller's. */
void EndContract(Book& B, life::State& L, size_t I, End Why, double At)
{
	Contract& C = B.Contracts[I];
	const std::string BrandId = C.Brand;
	C.St = State::Ended;
	C.Why = Why;
	C.Pending = -1;
	C.CureUntil = 0.0;
	C.Until = std::min(C.Until, std::max(C.Since, At));
	// The gear is the player's after a term that finished clean (the callers take it back when the last month was short).
	C.Kept = Why == End::Lapsed || Why == End::NotRenewed || Why == End::Walked;
	const Brand* Br = FindBrand(BrandId);
	const std::string Cat = Br ? "cat:" + std::to_string(static_cast<int>(Br->Cat)) : std::string("cat:x");
	auto Cool = [&B](const std::string& Key, double Until) { B.Cooldown[Key] = std::max(B.Cooldown[Key], Until); };
	switch (Why)
	{
	case End::ForCause: Cool(BrandId, At + CauseCooldownDays * DayMinutes); break;
	case End::Walked:
		Cool(BrandId, At + NoticeBrandDays * DayMinutes);
		if (Br && ExclusiveCategory(Br->Cat))
		{
			Cool(Cat, At + NoticeCategoryDays * DayMinutes);
		}
		break;
	case End::NotRenewed: Cool(BrandId, At + 90.0 * DayMinutes); break;
	case End::Lapsed: Cool(BrandId, At + 30.0 * DayMinutes); break;
	default: break;
	}
	if (Why == End::ForCause || Why == End::Conduct)
	{
		// Ended for cause or conduct, the unused comps lapse with it: the store credit the deal put on the book (§3.7).
		const bool Credit = std::any_of(C.T.Perks.begin(), C.T.Perks.end(), [](const Perk& P) { return P.Kind == Gift::StoreCredit; });
		B.StoreCreditCents = Credit ? 0 : B.StoreCreditCents;
		for (Contract& X : B.Contracts)
		{
			if (X.Brand == BrandId && X.Id != C.Id && (OpenOffer(X) || (Live(X) && X.Since >= At)))
			{
				X.St = State::Ended;
				X.Why = End::Withdrawn;
				X.Pending = -1;
			}
		}
	}
	// The patch comes off with the deal (Q17), unless the next term runs on; then the gear the brand sent stays the brand's,
	// on the term that runs on.
	bool RunsOn = false;
	for (Contract& X : B.Contracts)
	{
		if (X.Id != C.Id && X.Brand == BrandId && Live(X) && X.Until > At)
		{
			RunsOn = true;
			X.Property.insert(X.Property.end(), C.Property.begin(), C.Property.end());
			C.Property.clear();
		}
	}
	if (!RunsOn)
	{
		for (std::string& P : L.Closet.Patches)
		{
			P = P == BrandId ? std::string() : P;
		}
	}
}

/** The record-based asks over From..To, plus what the book counted. */
Tally Counts(const Ctx& K, const Contract& C, double From, double To, const Tally& Counted, int WorkoutBase)
{
	const double Stream = K.X.StreamStart >= 0.0 && K.X.StreamStart < To ? K.X.StreamStart : -1.0;
	Tally T = sponsor_measure_detail::Window(C, K.L, K.Ch, From, To, Stream);
	T.Reads = Counted.Reads;
	T.Appearances = Counted.Appearances;
	T.CoveredUsed = Counted.CoveredUsed;
	T.Workouts = WorkoutBase < 0 ? 0 : std::max(0, K.L.Fit.Workouts - WorkoutBase);
	return T;
}

void SetShort(Tally& S, Ask A, double Amount)
{
	const int N = static_cast<int>(std::ceil(Amount - 1e-9));
	switch (A)
	{
	case Ask::LiveHours: S.LiveMinutes = Amount * 60.0; break;
	case Ask::Reads: S.Reads = N; break;
	case Ask::TeachingStreams: S.Teaching = N; break;
	case Ask::LiveEvents: S.LiveEvents = N; break;
	case Ask::EmbercrestEvents: S.Embercrest = N; break;
	case Ask::Workouts: S.Workouts = N; break;
	case Ask::Appearances: S.Appearances = N; break;
	case Ask::CommunityStreams: S.Community = N; break;
	case Ask::Count: break;
	}
}

Chips Prorated(const Contract& C, Chips Monthly, int Period)
{
	const double Len = PeriodEnd(C, Period) - PeriodStart(C, Period);
	const double Share = std::min(1.0, Len / (PeriodDays * DayMinutes));
	return Share >= 1.0 ? Monthly : static_cast<Chips>(std::llround(static_cast<double>(Monthly) * Share));
}

/** A strike's cure: made up yet? Pays the held half and says so when it is. */
bool TryCure(Ctx& K, size_t I, double At)
{
	Contract& C = K.B.Contracts[I];
	if (C.CureUntil <= 0.0)
	{
		return false;
	}
	const Tally W = Counts(K, C, C.CureFrom, std::max(C.CureFrom, At), C.Cure, C.CureWorkoutBase);
	for (const Ask A : AllAsks)
	{
		const double Short = Have(C.Short, A);
		if (Short > 0.0 && Have(W, A) + 1e-9 < Short)
		{
			return false;
		}
	}
	const Chips Held = C.HeldCents;
	Pay(K, C, "held|" + C.Id + "|" + std::to_string(C.Period), Held, ShortOf(C.Brand) + ": held money", At);
	C.HeldCents = 0;
	C.CureUntil = 0.0;
	C.Short = Tally();
	C.Cure = Tally();
	// The held comps come back with the cure (§3.1, scene 3): from here the covers, the credit and the case are whole again.
	C.CompsHalved = false;
	Post(K.L, At, C, LetterCured, "cured", BodyFor(C, Held > 0 ? "cured:money" : "cured:kind", {{"Amount", Dollars(Held)}}));
	return true;
}

/** Period Pn's review at At (§3.6, §3.7). */
void Review(Ctx& K, size_t I, double At)
{
	Contract& C = K.B.Contracts[I];
	const int Pn = C.Period + 1;
	const Tally T = Counts(K, C, PeriodStart(C, Pn), PeriodEnd(C, Pn), C.Now, C.WorkoutBase);
	std::vector<std::string> Missed;
	Tally Short;
	double Share = 0.0;
	int Asked = 0;
	for (const Ask A : AllAsks)
	{
		const double N = Need(C, A, Pn);
		if (N <= 0.0)
		{
			continue;
		}
		const double H = Have(T, A);
		++Asked;
		Share += std::min(1.0, H / N);
		const double Bar = N * Threshold(A);
		if (H + 1e-9 < Bar)
		{
			Missed.push_back(MissedWords(C.Brand, A, H, N));
			SetShort(Short, A, Bar - H);
		}
	}
	const bool Met = Missed.empty();
	C.MetSum += Asked > 0 ? Share / static_cast<double>(Asked) : 1.0;
	const Chips Stipend = Prorated(C, C.T.MonthlyCents, Pn);
	const std::string Short_ = ShortOf(C.Brand);
	const std::string Key = "stipend|" + C.Id + "|" + std::to_string(Pn);
	// The teaching bonus: earned per lesson, paid in arrears with the review.
	if (C.T.PerTeachingCents > 0 && T.Teaching > 0)
	{
		Pay(K, C, "teach|" + C.Id + "|" + std::to_string(Pn), C.T.PerTeachingCents * std::min(T.Teaching, std::max(1, C.T.TeachingCap)), Short_ + ": lessons", At);
	}
	C.CompsHalved = false;
	const bool Last = Pn >= Periods(C);
	if (C.St == State::Notice)
	{
		// Notice: the period is paid if its asks were met, and the deal ends here.
		if (Met)
		{
			Pay(K, C, Key, Stipend, Short_ + ": monthly", At);
		}
		C.Period = Pn;
		// Walking away keeps the gear only if the last month was worked (sign, take the camera, give notice: it goes back).
		Post(K.L, At, C, LetterEnded, "notice:end", Joined({BodyFor(C, "notice:end"), Met ? std::string() : ReturnedLine(K.B, C, At, false), PatchLine(K.L, C)}));
		EndContract(K.B, K.L, I, End::Walked, At);
		K.B.Contracts[I].Kept = K.B.Contracts[I].Kept && Met;
		return;
	}
	if (Met)
	{
		Pay(K, C, Key, Stipend, Short_ + ": monthly", At);
		if (HasAsks(C.T) || Stipend > 0)
		{
			Post(K.L, At, C, LetterReview, "review", BodyFor(C, Stipend > 0 ? "review:paid" : "review:kind", {{"Period", std::to_string(Pn)}, {"Next", std::to_string(Pn + 1)}, {"Amount", Dollars(Stipend)}}));
		}
	}
	else
	{
		++C.Strikes;
		K.B.Strikes.push_back(At);
		if (C.Strikes >= 2)
		{
			C.Period = Pn;
			Post(K.L, At, C, LetterEnded, "ended", Joined({BodyFor(C, "ended:cause"), ReturnedLine(K.B, C, At, true), PatchLine(K.L, C)}));
			EndContract(K.B, K.L, I, End::ForCause, At);
			return;
		}
		// A strike pays half and holds half for the cure; half of next period's comps are held.
		const Chips Half = Stipend / 2;
		Pay(K, C, Key, Half, Short_ + ": monthly (half)", At);
		C.Period = Pn;
		if (Last)
		{
			// No term left to make it up in: the held half goes with the term. A renewal already signed runs on (the brand
			// renewed on the term's first months); otherwise they don't renew.
			bool Renewed = false;
			for (const Contract& X : K.B.Contracts)
			{
				Renewed = Renewed || (X.Renews == C.Id && Live(X));
			}
			if (Renewed)
			{
				EndContract(K.B, K.L, I, End::Lapsed, At);
				return;
			}
			// A term that ends on a short month didn't finish clean: they don't renew, and what they sent goes back (a
			// thirty-day deal signed for the chair and never worked keeps no chair).
			Post(K.L, At, C, LetterEnded, "term:short", Joined({BodyFor(C, "term:short", {{"Missed", JoinAnd(Missed)}}), ReturnedLine(K.B, C, At, false), PatchLine(K.L, C)}));
			EndContract(K.B, K.L, I, End::NotRenewed, At);
			K.B.Contracts[I].Kept = false;
			return;
		}
		C.HeldCents += Stipend - Half;
		C.CureFrom = At;
		C.CureUntil = At + static_cast<double>(std::max(1, C.T.CureDays)) * DayMinutes;
		C.Short = Short;
		C.Cure = Tally();
		C.CureWorkoutBase = K.L.Fit.Workouts;
		C.CompsHalved = true;
		Post(K.L, At, C, LetterStrike, "strike",
			BodyFor(C, Stipend > 0 ? "strike:money" : "strike:kind", {{"Period", std::to_string(Pn)}, {"Missed", JoinAnd(Missed)}, {"Cure", DateWords(C.CureUntil)}}));
	}
	C.Period = Pn;
	C.Now = Tally();
	C.WorkoutBase = K.L.Fit.Workouts;
	if (!Last)
	{
		// The next period's tickets (Team RiverLine's Daily Mains), halved under a strike.
		for (const Perk& Pk : C.T.Perks)
		{
			if (Pk.Kind == Gift::Ticket && K.B.PaidKeys.insert("ticket|" + C.Id + "|" + std::to_string(Pn + 1)).second)
			{
				K.L.Tickets[Pk.Id] += C.CompsHalved ? Pk.Count / 2 : Pk.Count;
			}
		}
		return;
	}
	// The end of the term.
	bool Renewed = false;
	for (const Contract& X : K.B.Contracts)
	{
		Renewed = Renewed || (X.Renews == C.Id && Live(X));
	}
	const double TermShare = C.Period > 0 ? C.MetSum / static_cast<double>(C.Period) : 1.0;
	if (Renewed)
	{
		EndContract(K.B, K.L, I, End::Lapsed, At);
	}
	else if (!HasAsks(C.T))
	{
		Post(K.L, At, C, LetterEnded, "term:end", Joined({BodyFor(C, C.Legacy ? "term:end:legacy" : "term:end"), PatchLine(K.L, C)}));
		EndContract(K.B, K.L, I, End::Lapsed, At);
	}
	else if (TermShare < 0.6)
	{
		Post(K.L, At, C, LetterEnded, "term:notrenewed", Joined({BodyFor(C, "term:notrenewed"), PatchLine(K.L, C)}));
		EndContract(K.B, K.L, I, End::NotRenewed, At);
	}
	else
	{
		Post(K.L, At, C, LetterEnded, "term:end", Joined({BodyFor(C, "term:end"), PatchLine(K.L, C)}));
		EndContract(K.B, K.L, I, End::Lapsed, At);
	}
}

/** Fourteen days out: the brand mails new terms (§3.7). */
void Renewal(Ctx& K, size_t I, double At)
{
	const Contract Old = K.B.Contracts[I];
	const Brand* Br = FindBrand(Old.Brand);
	if (!Br || !K.B.PaidKeys.insert("renew|" + Old.Id).second)
	{
		return;
	}
	const double Share = Old.Period > 0 ? Old.MetSum / static_cast<double>(Old.Period) : 1.0;
	if (Old.Strikes >= 2 || (HasAsks(Old.T) && Share < 0.6) || Old.CureUntil > 0.0)
	{
		return; // the term's review says so
	}
	Terms NT = Old.Legacy ? Br->Offer : Old.T;
	const int Now = TierOf(K.P, K.B, K.L, Old.Brand, At);
	if (!Old.Legacy)
	{
		const double Gained = static_cast<double>(std::max(0, Now - Old.TierAtSigning));
		Scale(NT, std::clamp((0.85 + 0.3 * Share) * (1.0 + 0.1 * Gained), 0.7, 1.4));
		if (Old.Brand == "embercrest" && Old.Strikes == 0)
		{
			NT.CoveredEntries = std::max(NT.CoveredEntries, 3); // three after a clean term (Q33)
		}
		if (Old.Brand == "overclock")
		{
			NT.MonthlyCents += 10000;
			NT.Days = 90;
			if (!HasSpot(NT, Spot::Sleeve))
			{
				NT.Wants.push_back(Spot::Sleeve);
			}
		}
		if (Old.Brand == "stacked")
		{
			NT.Days = 90;
		}
	}
	// The right to match: a rival's offer in the category on the table, and the incumbent matches its money.
	std::string Rival;
	for (const Contract& X : K.B.Contracts)
	{
		const Brand* Xb = BrandOf(X);
		if (OpenOffer(X) && Xb && Xb->Cat == Br->Cat && X.Brand != Old.Brand && ExclusiveCategory(Br->Cat) && X.T.MonthlyCents > NT.MonthlyCents)
		{
			NT.MonthlyCents = std::min(X.T.MonthlyCents, static_cast<Chips>(std::llround(static_cast<double>(std::max<Chips>(Old.T.MonthlyCents, 1)) * 1.4)));
			Rival = Xb->Short;
		}
	}
	Contract R;
	R.Id = sponsor_offer_detail::NewId(K.B, Old.Brand, At, "r");
	R.Brand = Old.Brand;
	R.St = State::Offered;
	R.T = NT;
	R.OfferedAt = At;
	R.ExpiresAt = Old.Until;
	R.Renews = Old.Id;
	R.TierAtSigning = Now;
	K.B.Contracts.push_back(R);
	const Contract& Made = K.B.Contracts.back();
	const std::string Body = Old.Legacy ? BodyFor(Made, "renewal:legacy", {{"Until", DateWords(Old.Until)}})
		: !Rival.empty()               ? BodyFor(Made, "renewal:match", {{"Until", DateWords(Old.Until)}, {"Rival", Rival}})
									   : BodyFor(Made, "renewal", {{"Until", DateWords(Old.Until)}});
	Post(K.L, At, Made, LetterRenewal, "renewal", Body);
}

/** A counter's or a raise's answer. */
void Reply(Ctx& K, size_t I, double At)
{
	Contract& C = K.B.Contracts[I];
	if (C.Pending < 0)
	{
		return;
	}
	const int Lev = C.Pending / 10;
	const int Out = C.Pending % 10;
	C.Pending = -1;
	if (Lev == PendingRaise)
	{
		Scale(C.T, std::min(1.4, 1.0 + 0.1 * static_cast<double>(Out)));
		Post(K.L, At, C, LetterReply, "reply", BodyFor(C, "reply:raise"));
		return;
	}
	if (Out <= 0)
	{
		C.St = State::Ended;
		C.Why = End::Withdrawn;
		K.B.Cooldown[C.Brand] = std::max(K.B.Cooldown[C.Brand], At + DeclineCooldownDays * DayMinutes);
		Post(K.L, At, C, LetterReply, "reply", BodyFor(C, "reply:pass"));
		return;
	}
	const Lever L = static_cast<Lever>(std::clamp(Lev, 0, 4));
	sponsor_counter_detail::Apply(C, L, Out);
	C.St = State::Offered;
	C.ExpiresAt = std::max(C.ExpiresAt, At + 3.0 * DayMinutes);
	const bool Money = L == Lever::Money && Out == 1 && C.T.MonthlyCents > 0;
	Post(K.L, At, C, LetterReply, "reply", BodyFor(C, Money ? "reply:money" : Out >= 2 ? "reply:full" : "reply:half", {{"Amount", Dollars(C.T.MonthlyCents)}}));
}

/** The weekly perk's next time strictly after T (store credit every EveryDays from the start; product on Mondays at 11:20). */
double NextPerk(const Contract& C, const Perk& Pk, double T)
{
	if (!Live(C) || Pk.EveryDays <= 0)
	{
		return 1.0e18;
	}
	if (Pk.Kind == Gift::StoreCredit)
	{
		const double Step = static_cast<double>(Pk.EveryDays) * DayMinutes;
		double K = std::floor((std::max(T, C.Since) - C.Since) / Step) + 1.0;
		double At = C.Since + K * Step;
		while (At <= T)
		{
			K += 1.0;
			At = C.Since + K * Step;
		}
		return At < C.Until ? At : 1.0e18;
	}
	if (Pk.Kind == Gift::Product)
	{
		const double From = std::max(T, C.Since);
		for (int Day = net::DayOf(From); Day <= net::DayOf(From) + 8; ++Day)
		{
			const double At = static_cast<double>(Day) * DayMinutes + 11.0 * 60.0 + 20.0;
			if (net::Weekday(Day) == 0 && At > T && At > C.Since)
			{
				return At < C.Until ? At : 1.0e18;
			}
		}
	}
	return 1.0e18;
}

void GrantPerk(Ctx& K, size_t I, const Perk& Pk, double At)
{
	Contract& C = K.B.Contracts[I];
	if (!RunningAt(C, At))
	{
		return;
	}
	if (Pk.Kind == Gift::StoreCredit)
	{
		const long long Week = std::llround((At - C.Since) / (static_cast<double>(Pk.EveryDays) * DayMinutes));
		if (K.B.PaidKeys.insert("credit|" + C.Id + "|" + std::to_string(Week)).second)
		{
			K.B.StoreCreditCents += C.CompsHalved ? Pk.Cents / 2 : Pk.Cents;
		}
	}
	else if (Pk.Kind == Gift::Product)
	{
		if (K.B.PaidKeys.insert("product|" + C.Id + "|" + std::to_string(net::DayOf(At))).second)
		{
			const int Count = C.CompsHalved ? Pk.Count / 2 : Pk.Count;
			K.L.Pantry[Pk.Id] += Count;
			const Brand* Br = BrandOf(C);
			if (K.X.Texts && Br)
			{
				const bool Dee = K.L.RentStage == life::Rent::Evicted;
				K.X.Texts->push_back({Br->Short, Fill(Text(sponsor_copy::Spoken, "product:" + C.Brand + (Dee ? ":dee" : "")), {})});
			}
		}
	}
}

double RenewalAt(const Contract& C)
{
	return C.Until - RenewalLeadDays * DayMinutes;
}

bool RenewalDue(const Book& B, const Contract& C)
{
	return C.St == State::Active && FindBrand(C.Brand) != nullptr && C.Until - C.Since >= PeriodDays * DayMinutes && B.PaidKeys.count("renew|" + C.Id) == 0;
}

/** The earliest thing to do after Prev (an overdue one is due at Prev). */
double Earliest(const Ctx& K, double Prev)
{
	double Next = 1.0e18;
	auto Due = [&Next, Prev](double At) { Next = std::min(Next, std::max(At, Prev)); };
	for (const Contract& C : K.B.Contracts)
	{
		if (C.Pending >= 0)
		{
			Due(C.ReplyAt);
		}
		if (OpenOffer(C) && C.Pending < 0)
		{
			Due(C.ExpiresAt);
		}
		if (Live(C))
		{
			if (C.Since > Prev)
			{
				Due(C.Since); // a term signed ahead starts
			}
			if (C.Period < Periods(C))
			{
				Due(PeriodEnd(C, C.Period + 1));
			}
			if (C.CureUntil > 0.0)
			{
				Due(C.CureUntil);
			}
			if (RenewalDue(K.B, C))
			{
				Due(RenewalAt(C));
			}
			for (const Perk& Pk : C.T.Perks)
			{
				Next = std::min(Next, NextPerk(C, Pk, Prev));
			}
		}
	}
	for (const Brand& Br : Brands())
	{
		if (Br.Needs.Open)
		{
			Next = std::min(Next, sponsor_offer_detail::NextLook(Br, Prev, K.Seed));
		}
	}
	return Next;
}

void ProcessAt(Ctx& K, double Prev, double At)
{
	const size_t Count = K.B.Contracts.size();
	for (size_t I = 0; I < Count; ++I)
	{
		if (K.B.Contracts[I].Pending >= 0 && K.B.Contracts[I].ReplyAt <= At)
		{
			Reply(K, I, At);
		}
		Contract& C = K.B.Contracts[I];
		if (OpenOffer(C) && C.ExpiresAt <= At && C.Pending < 0)
		{
			// An offer left unanswered lapses; a brand ignored again waits longer each time (three weeks, then six, up to
			// twelve), so Mail isn't the same letter every month.
			C.St = State::Ended;
			C.Why = End::Expired;
			int Before = 0;
			for (const Contract& X : K.B.Contracts)
			{
				Before += X.Brand == C.Brand && X.Id != C.Id && X.St == State::Ended && X.Why == End::Expired ? 1 : 0;
			}
			const double Wait = ExpireCooldownDays * static_cast<double>(std::min(4, 1 + Before));
			K.B.Cooldown[C.Brand] = std::max(K.B.Cooldown[C.Brand], At + Wait * DayMinutes);
			continue;
		}
		if (!Live(C))
		{
			continue;
		}
		if (C.Since <= At)
		{
			// A term signed ahead (a renewal) starts: its first month's tickets, and its workouts count from here.
			C.WorkoutBase = C.WorkoutBase < 0 ? K.L.Fit.Workouts : C.WorkoutBase;
			for (const Perk& Pk : C.T.Perks)
			{
				if (Pk.Kind == Gift::Ticket && K.B.PaidKeys.insert("ticket|" + C.Id + "|1").second)
				{
					K.L.Tickets[Pk.Id] += Pk.Count;
				}
			}
		}
		const std::vector<Perk> Perks = C.T.Perks;
		for (const Perk& Pk : Perks)
		{
			const double When = NextPerk(K.B.Contracts[I], Pk, Prev);
			if (When <= At)
			{
				GrantPerk(K, I, Pk, When);
			}
		}
		if (Live(K.B.Contracts[I]) && K.B.Contracts[I].CureUntil > 0.0 && K.B.Contracts[I].CureUntil <= At && !TryCure(K, I, K.B.Contracts[I].CureUntil))
		{
			Contract& Lapsed = K.B.Contracts[I];
			Post(K.L, At, Lapsed, LetterEnded, "ended", Joined({BodyFor(Lapsed, "ended:cure"), ReturnedLine(K.B, Lapsed, At, true), PatchLine(K.L, Lapsed)}));
			EndContract(K.B, K.L, I, End::ForCause, At);
			continue;
		}
		if (Live(K.B.Contracts[I]) && RenewalDue(K.B, K.B.Contracts[I]) && RenewalAt(K.B.Contracts[I]) <= At && K.B.Contracts[I].Since <= At)
		{
			Renewal(K, I, At);
		}
		if (Live(K.B.Contracts[I]) && K.B.Contracts[I].Period < Periods(K.B.Contracts[I]) && PeriodEnd(K.B.Contracts[I], K.B.Contracts[I].Period + 1) <= At)
		{
			Review(K, I, At);
		}
	}
	for (const Brand& Br : Brands())
	{
		if (!Br.Needs.Open)
		{
			continue;
		}
		const double Look = sponsor_offer_detail::NextLook(Br, Prev, K.Seed);
		if (Look <= At && Eligible(K.B, K.L, K.P, Br.Id, Look, K.X.Gear).empty())
		{
			MakeOffer(K.B, K.L, K.P, Br.Id, Look, K.Seed, true);
		}
	}
}

/** Keeps the book bounded: the last 24 ended contracts, live cooldowns, keys for what's still kept, the last year's strikes. */
void Prune(Book& B, const life::State& L, double World)
{
	std::vector<size_t> Ended;
	for (size_t I = 0; I < B.Contracts.size(); ++I)
	{
		if (B.Contracts[I].St == State::Ended)
		{
			Ended.push_back(I);
		}
	}
	if (Ended.size() > EndedKept)
	{
		std::stable_sort(Ended.begin(), Ended.end(), [&B](size_t A, size_t C) {
			const double Ta = std::max(B.Contracts[A].Until, B.Contracts[A].OfferedAt);
			const double Tc = std::max(B.Contracts[C].Until, B.Contracts[C].OfferedAt);
			return Ta < Tc;
		});
		std::set<std::string> Drop;
		for (size_t K = 0; K + EndedKept < Ended.size(); ++K)
		{
			Drop.insert(B.Contracts[Ended[K]].Id);
		}
		B.Contracts.erase(std::remove_if(B.Contracts.begin(), B.Contracts.end(), [&Drop](const Contract& C) { return Drop.count(C.Id) > 0; }), B.Contracts.end());
	}
	for (auto It = B.Cooldown.begin(); It != B.Cooldown.end();)
	{
		It = It->second <= World ? B.Cooldown.erase(It) : std::next(It);
	}
	for (auto It = B.PaidKeys.begin(); It != B.PaidKeys.end();)
	{
		const std::vector<std::string> F = Split(*It, '|');
		bool Keep = F.size() >= 2;
		if (Keep && EntryKey(F[0]))
		{
			Keep = std::any_of(L.LiveEntries.begin(), L.LiveEntries.end(), [&F](const life::LiveEntry& E) { return E.Id == F[1]; });
		}
		else if (Keep)
		{
			// A contract's keys while it can still pay (an ended one never does again, and SteppedTo keeps a span from being
			// stepped twice): the save carries the running deals' keys, not a career's.
			const Contract* C = Find(B, F[1]);
			Keep = C != nullptr && C->St != State::Ended;
		}
		It = Keep ? std::next(It) : B.PaidKeys.erase(It);
	}
	B.Strikes.erase(std::remove_if(B.Strikes.begin(), B.Strikes.end(), [World](double S) { return World - S > 365.0 * DayMinutes; }), B.Strikes.end());
	const int Today = net::DayOf(World);
	for (auto It = B.Days.begin(); It != B.Days.end();)
	{
		std::vector<int>& D = It->second;
		D.erase(std::remove_if(D.begin(), D.end(), [Today](int X) { return Today - X >= BlockWindow; }), D.end());
		It = D.empty() ? B.Days.erase(It) : std::next(It);
	}
}
} // namespace sponsor_step_detail

void Step(Book& B, life::State& L, Chips& Bankroll, const kast::Channel& C, const rep::Public& P, double From, double To, const std::string& Seed, const StepContext& Ctx)
{
	using namespace sponsor_step_detail;
	sponsor_step_detail::Ctx K{B, L, Bankroll, C, P, Seed, Ctx};
	double Prev = std::max(From, B.SteppedTo);
	if (To < Prev)
	{
		return;
	}
	for (int Guard = 0; Guard < 20000; ++Guard)
	{
		const double Next = Earliest(K, Prev);
		if (Next > To)
		{
			break;
		}
		ProcessAt(K, Prev, Next);
		if (Next <= Prev)
		{
			// Something overdue was done at Prev; look again from there (each one changes its own state, so it ends).
			continue;
		}
		Prev = Next;
	}
	// A cure made up since: the held money comes back now, not at the deadline.
	for (size_t I = 0; I < B.Contracts.size(); ++I)
	{
		if (Live(B.Contracts[I]) && B.Contracts[I].CureUntil > To)
		{
			TryCure(K, I, To);
		}
	}
	Prune(B, L, To);
	B.SteppedTo = std::max(B.SteppedTo, To);
	// The next thing to do (the session skips the step until then); an open cure is checked every hour.
	double Next = Earliest(K, To);
	for (const Contract& X : B.Contracts)
	{
		Next = Live(X) && X.CureUntil > 0.0 ? std::min(Next, To + 60.0) : Next;
	}
	B.NextAt = Next;
}

// ------------------------------------------------------------------ live money (§1.2): covers, cuts, bonuses

namespace sponsor_live_detail
{
/** Whether C's cover takes O (the cap, the events it names, inside the term). */
std::string CoverWhyNot(const Contract& C, const live::Occurrence& O, double World)
{
	const Brand* Br = BrandOf(C);
	const std::string Short = Br ? Br->Short : C.Brand;
	if (!O.Valid() || C.T.CoveredEntries <= 0)
	{
		return Short + " doesn't cover entries.";
	}
	const bool Named = C.T.CoveredEvents.empty() || std::find(C.T.CoveredEvents.begin(), C.T.CoveredEvents.end(), std::string(O.T->Key)) != C.T.CoveredEvents.end();
	if (!Named || (C.T.CoveredCapCents > 0 && O.T->BuyInCents > C.T.CoveredCapCents))
	{
		return Short + " doesn't cover the " + O.T->Short + ".";
	}
	if (O.Start >= C.Until)
	{
		return "The deal ends before the " + std::string(O.T->Short) + ".";
	}
	const int Allowed = C.CompsHalved ? C.T.CoveredEntries / 2 : C.T.CoveredEntries;
	if (C.Now.CoveredUsed >= Allowed)
	{
		return Allowed <= 0 ? Possessive(Short) + " covers are on hold this month." : Possessive(Short) + " covered " + (Allowed == 1 ? "entry is" : "entries are") + " used this month.";
	}
	(void)World;
	return std::string();
}

std::string CoverKeyPrefix(const std::string& EntryId)
{
	return "cover|" + EntryId + "|";
}
} // namespace sponsor_live_detail

const Contract* Covering(const Book& B, const live::Occurrence& O, double World)
{
	for (const Contract& C : B.Contracts)
	{
		if (RunningAt(C, World) && C.T.CoveredEntries > 0 && sponsor_live_detail::CoverWhyNot(C, O, World).empty())
		{
			return &C;
		}
	}
	return nullptr;
}

life::Funding CoverFunding(const Contract& C, const live::Occurrence& O)
{
	life::Funding F;
	F.From = life::Paid::Sponsor;
	F.Ref = C.Id;
	F.Who = ShortOf(C.Brand);
	F.Cents = O.Valid() ? O.T->BuyInCents : 0;
	F.CutPct = C.T.PrizeCutPct;
	F.Fares = C.T.CoversFares;
	return F;
}

std::vector<std::pair<life::Funding, std::string>> CoverOptions(const Book& B, const live::Occurrence& O, double World)
{
	std::vector<std::pair<life::Funding, std::string>> Out;
	for (const Contract& C : B.Contracts)
	{
		if (RunningAt(C, World) && C.T.CoveredEntries > 0)
		{
			Out.push_back({CoverFunding(C, O), sponsor_live_detail::CoverWhyNot(C, O, World)});
		}
	}
	return Out;
}

void OnRegister(Book& B, life::State& L, Chips& Bankroll, const life::LiveEntry& E, double World)
{
	if (E.Fund.From != life::Paid::Sponsor || E.Fund.Ref.empty())
	{
		return;
	}
	Contract* C = Find(B, E.Fund.Ref);
	if (!C)
	{
		return;
	}
	for (const std::string& K : B.PaidKeys)
	{
		if (K.rfind(sponsor_live_detail::CoverKeyPrefix(E.Id), 0) == 0)
		{
			return; // covered once
		}
	}
	B.PaidKeys.insert(sponsor_live_detail::CoverKeyPrefix(E.Id) + C->Id + "|" + std::to_string(C->Period + 1));
	const live::Occurrence O = live::FindOccurrence(E.Id);
	const std::string Event = O.Valid() ? std::string(O.T->Short) : E.Name;
	Bankroll += E.PaidCents;
	L.Record(World, (E.Fund.Who.empty() ? ShortOf(C->Brand) : E.Fund.Who) + ": covered the " + Event, E.PaidCents, life::LedgerSponsors);
	C->Now.CoveredUsed += 1;
	B.NextAt = 0.0;
}

void Settled(Book& B, life::State& L, Chips& Bankroll, const life::LiveEntry& E)
{
	const double At = E.FinishedAt;
	const live::Occurrence O = live::FindOccurrence(E.Id);
	const std::string Event = O.Valid() ? std::string(O.T->Short) : E.Name;
	// A negotiated cut of the profit on a covered entry (the contract is senior to a backer, §1.2).
	if (E.Fund.From == life::Paid::Sponsor && E.Fund.CutPct > 0)
	{
		const Chips Profit = std::max<Chips>(0, E.PrizeCents - E.PaidCents);
		const Chips Cut = Profit * std::min(100, E.Fund.CutPct) / 100;
		if (Cut > 0 && B.PaidKeys.insert("cut|" + E.Id).second)
		{
			Bankroll -= Cut;
			L.Record(At, E.Fund.Who + ": cut of the " + Event, -Cut, life::LedgerSponsors);
			if (Contract* C = Find(B, E.Fund.Ref))
			{
				C->PaidCents -= Cut;
			}
		}
	}
	// Bonuses, for the brands whose patch was worn that night.
	for (const std::pair<std::string, std::string>& P : E.Patches)
	{
		if (!sponsor_measure_detail::Wore(E, P.second))
		{
			continue;
		}
		for (Contract& C : B.Contracts)
		{
			if (C.Brand != P.second || !Live(C) || E.RegisteredAt < C.Since || E.RegisteredAt >= C.Until)
			{
				continue;
			}
			const std::string Short = ShortOf(C.Brand);
			if (C.T.FinalTableBonusCents > 0 && E.Place >= 1 && E.Place <= 6 && B.PaidKeys.insert("bonus|" + E.Id + "|" + C.Brand + "|ft").second)
			{
				Bankroll += C.T.FinalTableBonusCents;
				L.Record(At, Short + ": final table bonus", C.T.FinalTableBonusCents, life::LedgerSponsors);
				C.PaidCents += C.T.FinalTableBonusCents;
			}
			if (C.T.TitleBonusCents > 0 && E.Place == 1 && B.PaidKeys.insert("bonus|" + E.Id + "|" + C.Brand + "|title").second)
			{
				Bankroll += C.T.TitleBonusCents;
				L.Record(At, Short + ": title bonus", C.T.TitleBonusCents, life::LedgerSponsors);
				C.PaidCents += C.T.TitleBonusCents;
			}
		}
	}
	B.NextAt = 0.0;
}

void OnRefund(Book& B, life::State& L, Chips& Bankroll, const life::LiveEntry& E, double World)
{
	if (E.Fund.From != life::Paid::Sponsor)
	{
		return;
	}
	std::string Key;
	for (const std::string& K : B.PaidKeys)
	{
		if (K.rfind(sponsor_live_detail::CoverKeyPrefix(E.Id), 0) == 0)
		{
			Key = K;
		}
	}
	if (Key.empty() || !B.PaidKeys.insert("uncover|" + E.Id).second)
	{
		return;
	}
	// The cover goes back with the refund, and doesn't count as used.
	Bankroll -= E.PaidCents;
	L.Record(World, (E.Fund.Who.empty() ? std::string("Your sponsor") : E.Fund.Who) + ": cover returned", -E.PaidCents, life::LedgerSponsors);
	const std::vector<std::string> F = Split(Key, '|');
	if (F.size() >= 4)
	{
		if (Contract* C = Find(B, F[2]))
		{
			if (std::atoi(F[3].c_str()) == C->Period + 1 && C->Now.CoveredUsed > 0)
			{
				C->Now.CoveredUsed -= 1;
			}
		}
	}
	B.NextAt = 0.0;
}

// ------------------------------------------------------------------ patches (§3.6, §3.9)

std::string Sew(wardrobe::Closet& C, const Book& B, int SpotIndex, const std::string& BrandId, double World)
{
	if (SpotIndex < 0 || SpotIndex >= PatchSpots)
	{
		return "There's no patch spot there.";
	}
	if (BrandId.empty())
	{
		C.Patches[static_cast<size_t>(SpotIndex)].clear();
		return std::string();
	}
	const Contract* K = Running(B, BrandId, World);
	if (!K)
	{
		return "Nobody's given you that patch to wear.";
	}
	const std::vector<Spot> Spots = SpotsOf(K->T);
	const std::string Short = ShortOf(BrandId);
	if (Spots.empty())
	{
		return Short + " doesn't make a patch.";
	}
	if (std::find(Spots.begin(), Spots.end(), static_cast<Spot>(SpotIndex)) == Spots.end())
	{
		return Possessive(Short) + " patch goes on the " + sponsor_sign_detail::SpotsWords(Spots) + ".";
	}
	for (std::string& P : C.Patches)
	{
		P = P == BrandId ? std::string() : P;
	}
	C.Patches[static_cast<size_t>(SpotIndex)] = BrandId;
	return std::string();
}

std::vector<std::string> PatchesFor(const Book& B, int SpotIndex, double World)
{
	std::vector<std::string> Out;
	for (const Contract& C : B.Contracts)
	{
		const std::vector<Spot> Spots = SpotsOf(C.T);
		if (RunningAt(C, World) && std::find(Spots.begin(), Spots.end(), static_cast<Spot>(SpotIndex)) != Spots.end() &&
			std::find(Out.begin(), Out.end(), C.Brand) == Out.end())
		{
			Out.push_back(C.Brand);
		}
	}
	return Out;
}

namespace sponsor_patch_detail
{
/** Brand's patch is sewn on a spot it takes, and that spot shows on what's worn. */
bool Shows(const wardrobe::Closet& C, const Contract& K)
{
	const std::vector<Spot> Spots = SpotsOf(K.T);
	for (int I = 0; I < PatchSpots; ++I)
	{
		if (C.Patches[static_cast<size_t>(I)] == K.Brand && std::find(Spots.begin(), Spots.end(), static_cast<Spot>(I)) != Spots.end() && wardrobe::PatchFits(C, I))
		{
			return true;
		}
	}
	return false;
}
} // namespace sponsor_patch_detail

std::string Fits(const Book& B, const wardrobe::Closet& C, bool AtEmbercrest, double World)
{
	std::string Out;
	for (const Contract& K : B.Contracts)
	{
		const std::vector<Spot> Spots = SpotsOf(K.T);
		if (!RunningAt(K, World) || Spots.empty())
		{
			continue;
		}
		const bool AtEvents = K.T.Asks[static_cast<size_t>(Ask::LiveEvents)] > 0.0;
		const bool InTheRoom = K.Brand == "embercrest";
		if ((!AtEvents && !InTheRoom) || (InTheRoom && !AtEmbercrest) || sponsor_patch_detail::Shows(C, K))
		{
			continue;
		}
		const std::string Line = ShortOf(K.Brand) + " on the " + sponsor_sign_detail::SpotsWords(Spots) + (InTheRoom ? " in the room" : " at live events");
		Out += (Out.empty() ? "" : " \xC2\xB7 ") + Line;
	}
	return Out;
}

void NoteWorn(life::LiveEntry& E, const std::string& Hero, const wardrobe::Closet& C, const Book& B, double World)
{
	for (int I = 0; I < PatchSpots; ++I)
	{
		const std::string& BrandId = C.Patches[static_cast<size_t>(I)];
		const Contract* K = BrandId.empty() ? nullptr : Running(B, BrandId, World);
		if (!K || !sponsor_patch_detail::Shows(C, *K))
		{
			continue;
		}
		const bool Have = std::any_of(E.Patches.begin(), E.Patches.end(), [&](const std::pair<std::string, std::string>& P) { return P.first == Hero && P.second == BrandId; });
		if (!Have)
		{
			E.Patches.push_back({Hero, BrandId});
		}
	}
}

std::string WearingLine(const wardrobe::Closet& C, const Book& B, double World)
{
	std::string Out;
	for (int I = 0; I < PatchSpots; ++I)
	{
		const std::string& BrandId = C.Patches[static_cast<size_t>(I)];
		const Contract* K = BrandId.empty() ? nullptr : Running(B, BrandId, World);
		if (K && wardrobe::PatchFits(C, I))
		{
			Out += (Out.empty() ? "Wearing " : " \xC2\xB7 ") + ShortOf(BrandId) + " (" + SpotWord(static_cast<Spot>(I)) + ")";
		}
	}
	return Out;
}

// ------------------------------------------------------------------ appearances (§3.5)

namespace sponsor_appear_detail
{
/** The brand's appearance window on Day, if it has one that day. */
bool WindowOn(const std::string& BrandId, int Day, Window& W)
{
	const double Base = static_cast<double>(Day) * DayMinutes;
	const int Wd = net::Weekday(Day);
	if (BrandId == "luckypenny")
	{
		W.Venue = "store";
		W.Label = "Lucky Penny photo";
		W.From = Base + 10.0 * 60.0; // Ray's shift
		W.To = Base + 22.0 * 60.0;
		return true;
	}
	if (BrandId == "barber" && Wd == 5)
	{
		W.Venue = "barber";
		W.Label = "Lonnie's window chair";
		W.From = Base + 13.0 * 60.0;
		W.To = Base + 17.0 * 60.0;
		return true;
	}
	if (BrandId == "embercrest" && Wd == 6)
	{
		// From a quarter to six, through the Sunday's first level (cards at seven, 20-minute levels): someone in the room
		// since the Warm-Up does it before cards; someone off the bus (the room seats them at ten to seven) stands up from
		// the chair for a lap with Denise before the blinds go up.
		W.Venue = "embercrest";
		W.Label = "Meet the regulars";
		W.From = Base + 17.0 * 60.0 + 45.0;
		W.To = Base + 19.0 * 60.0 + 20.0;
		return true;
	}
	return false;
}

/** Still wanted in the period At falls in (or by an open cure). */
bool Wanted(const Contract& C, double At)
{
	const int P = PeriodAt(C, At);
	const bool Cure = C.CureUntil > At && C.Short.Appearances > C.Cure.Appearances;
	if (P == C.Period + 1)
	{
		return Cure || C.Now.Appearances < static_cast<int>(Need(C, Ask::Appearances, P));
	}
	return Cure || P > C.Period + 1;
}
} // namespace sponsor_appear_detail

std::vector<Window> Appearances(const Book& B, double From, double To)
{
	std::vector<Window> Out;
	for (const Contract& C : B.Contracts)
	{
		if (!Live(C) || C.T.Asks[static_cast<size_t>(Ask::Appearances)] <= 0.0)
		{
			continue;
		}
		const double Lo = std::max(From, C.Since);
		const double Hi = std::min(To, C.Until);
		for (int Day = net::DayOf(Lo); Lo < Hi && Day <= net::DayOf(Hi); ++Day)
		{
			Window W;
			if (!sponsor_appear_detail::WindowOn(C.Brand, Day, W) || W.To <= Lo || W.From >= Hi)
			{
				continue;
			}
			W.Contract = C.Id;
			W.Done = !sponsor_appear_detail::Wanted(C, std::max(W.From, C.Since));
			Out.push_back(W);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const Window& A, const Window& C) { return A.From < C.From; });
	return Out;
}

std::string Appear(Book& B, const std::string& Venue, double World)
{
	bool Later = false;
	for (Contract& C : B.Contracts)
	{
		if (!RunningAt(C, World) || C.T.Asks[static_cast<size_t>(Ask::Appearances)] <= 0.0)
		{
			continue;
		}
		Window W;
		if (!sponsor_appear_detail::WindowOn(C.Brand, net::DayOf(World), W) || W.Venue != Venue)
		{
			continue;
		}
		if (World < W.From || World > W.To)
		{
			Later = true;
			continue;
		}
		const bool Cure = C.CureUntil > World && C.Short.Appearances > C.Cure.Appearances;
		const bool Period = C.Now.Appearances < static_cast<int>(Need(C, Ask::Appearances, C.Period + 1));
		if (!Cure && !Period)
		{
			return "You've done that one this month.";
		}
		C.Now.Appearances += Period ? 1 : 0;
		C.Cure.Appearances += Cure ? 1 : 0;
		B.NextAt = 0.0;
		return std::string();
	}
	return Later ? "Not now. Check the window on your calendar." : "Nobody's expecting you.";
}

// ------------------------------------------------------------------ Kast's view of the book (§3.8)

std::vector<kast::Deal> StreamView(const Book& B, double World)
{
	std::vector<kast::Deal> Out;
	for (const Contract& C : B.Contracts)
	{
		if (!RunningAt(C, World) || !PaysOnStream(C.T))
		{
			continue;
		}
		kast::Deal D;
		D.Id = C.Brand;
		D.Since = C.Since;
		D.Until = C.Until;
		D.ReadAt = C.ReadAt;
		D.EarnedCents = C.PaidCents;
		D.PerHourCents = C.T.PerLiveHourCents;
		D.ReadCents = C.T.PerReadCents;
		D.Overlay = HasOverlay(C);
		Out.push_back(D);
	}
	return Out;
}

void AbsorbStream(Book& B, const std::vector<kast::Deal>& Deals)
{
	for (const kast::Deal& D : Deals)
	{
		for (Contract& C : B.Contracts)
		{
			if (C.Brand != D.Id || !Live(C) || C.Since != D.Since)
			{
				continue;
			}
			if (D.ReadAt > C.ReadAt)
			{
				C.ReadAt = D.ReadAt;
				C.Now.Reads += D.ReadAt >= PeriodStart(C, C.Period + 1) ? 1 : 0;
				C.Cure.Reads += C.CureUntil > 0.0 && D.ReadAt >= C.CureFrom ? 1 : 0;
				B.NextAt = 0.0;
			}
			C.PaidCents = std::max(C.PaidCents, D.EarnedCents);
		}
	}
}

// ------------------------------------------------------------------ the morals clause (§3.7)

void Conduct(Book& B, life::State& L, Chips& Bankroll, Breach Why, double World)
{
	using namespace sponsor_step_detail;
	B.NextAt = 0.0;
	for (size_t I = 0; I < B.Contracts.size(); ++I)
	{
		Contract& C = B.Contracts[I];
		const Brand* Br = BrandOf(C);
		if (!Br)
		{
			continue;
		}
		const bool Site = Br->Cat == Category::Site || Br->Cat == Category::Training;
		const bool Hit = Why == Breach::PickedUp ? C.T.ConductClause : Site;
		if (!Hit)
		{
			continue;
		}
		if (OpenOffer(C))
		{
			C.St = State::Ended;
			C.Why = End::Withdrawn;
			C.Pending = -1;
			continue;
		}
		if (!Live(C) || C.Until <= World)
		{
			continue;
		}
		if (Why == Breach::PickedUp && C.Brand == "luckypenny" && !B.PennyPass)
		{
			// The owner lets it slide, once (Q13).
			B.PennyPass = true;
			Post(L, World, C, LetterConduct, "pass", BodyFor(C, "pass"));
			continue;
		}
		// Signing money back in proportion to the time left, never past the rent reserve; the rest is written off.
		std::string Money;
		if (C.T.SigningCents > 0 && B.PaidKeys.count("signing|" + C.Id) > 0 && C.Until > C.Since)
		{
			const double Left = std::clamp((C.Until - World) / (C.Until - C.Since), 0.0, 1.0);
			const Chips Owed = static_cast<Chips>(std::llround(static_cast<double>(C.T.SigningCents) * Left));
			const Chips Take = std::min(Owed, std::max<Chips>(0, Bankroll - RentReserve(L, World)));
			if (Take > 0 && B.PaidKeys.insert("clawback|" + C.Id).second)
			{
				Bankroll -= Take;
				L.Record(World, Br->Short + ": signing money back", -Take, life::LedgerSponsors);
				C.PaidCents -= Take;
			}
			Money = Joined({Take > 0 ? BodyFor(C, "clawback", {{"Amount", Dollars(Take)}}) : std::string(), Take < Owed ? BodyFor(C, "writeoff") : std::string()});
		}
		const std::string Returned = !C.Property.empty() ? BodyFor(C, "returned") : std::string();
		const std::string Body = Joined({BodyFor(C, Why == Breach::PickedUp ? "ended:conduct" : "ended:restricted"), Money, Returned, PatchLine(L, C)});
		Post(L, World, C, LetterConduct, Why == Breach::PickedUp ? "ended:conduct" : "ended:restricted", Body);
		EndContract(B, L, I, End::Conduct, World);
	}
	if (Why == Breach::PickedUp)
	{
		B.Cooldown["*"] = std::max(B.Cooldown["*"], World + ConductDays * DayMinutes);
	}
	else
	{
		for (const Category Cat : {Category::Site, Category::Training})
		{
			const std::string Key = "cat:" + std::to_string(static_cast<int>(Cat));
			B.Cooldown[Key] = std::max(B.Cooldown[Key], World + RestrictedDays * DayMinutes);
		}
	}
}

// ------------------------------------------------------------------ perks in kind, property

Chips SpendCredit(Book& B, Chips Due)
{
	const Chips Take = std::min(B.StoreCreditCents, std::max<Chips>(0, Due));
	B.StoreCreditCents -= Take;
	return Due - Take;
}

bool ServiceCovered(const Book& B, const std::string& Venue, double World)
{
	for (const Contract& C : B.Contracts)
	{
		if (!RunningAt(C, World))
		{
			continue;
		}
		for (const Perk& Pk : C.T.Perks)
		{
			if (Pk.Kind == Gift::Service && Pk.Id == Venue && Pk.EveryDays > 0)
			{
				const long long N = static_cast<long long>(std::floor((World - C.Since) / (static_cast<double>(Pk.EveryDays) * DayMinutes)));
				if (B.PaidKeys.count("service|" + C.Id + "|" + std::to_string(N)) == 0)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void UseService(Book& B, const std::string& Venue, double World)
{
	for (const Contract& C : B.Contracts)
	{
		if (!RunningAt(C, World))
		{
			continue;
		}
		for (const Perk& Pk : C.T.Perks)
		{
			if (Pk.Kind == Gift::Service && Pk.Id == Venue && Pk.EveryDays > 0)
			{
				const long long N = static_cast<long long>(std::floor((World - C.Since) / (static_cast<double>(Pk.EveryDays) * DayMinutes)));
				if (B.PaidKeys.insert("service|" + C.Id + "|" + std::to_string(N)).second)
				{
					return;
				}
			}
		}
	}
}

bool CoversGear(const Book& B, const std::string& GearId, double World)
{
	for (const Contract& C : B.Contracts)
	{
		if (!RunningAt(C, World))
		{
			continue;
		}
		for (const Perk& Pk : C.T.Perks)
		{
			if (Pk.Kind == Gift::Gear && Pk.Id == GearId)
			{
				return true;
			}
		}
	}
	return false;
}

bool IsProperty(const Book& B, const std::string& GearOrGarmentId, double World)
{
	const bool Tagged = GearOrGarmentId.rfind("gear:", 0) == 0 || GearOrGarmentId.rfind("garment:", 0) == 0;
	const std::string Id = Tagged ? GearOrGarmentId : "gear:" + GearOrGarmentId;
	for (const Contract& C : B.Contracts)
	{
		if (C.Kept || !Live(C) || World >= C.Until)
		{
			continue;
		}
		if (std::find(C.Property.begin(), C.Property.end(), Id) != C.Property.end())
		{
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------ what other features read

std::vector<std::pair<std::string, uint32_t>> Badges(const Book& B, double World)
{
	std::vector<std::pair<std::string, uint32_t>> Out;
	for (const Contract& C : B.Contracts)
	{
		const Brand* Br = BrandOf(C);
		if (Br && RunningAt(C, World) && std::none_of(Out.begin(), Out.end(), [&C](const std::pair<std::string, uint32_t>& P) { return P.first == C.Brand; }))
		{
			Out.push_back({C.Brand, Br->Color});
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const std::pair<std::string, uint32_t>& A, const std::pair<std::string, uint32_t>& C) {
		const Brand* Ba = FindBrand(A.first);
		const Brand* Bc = FindBrand(C.first);
		return (Ba ? Ba->Tier : 0) > (Bc ? Bc->Tier : 0);
	});
	return Out;
}

std::vector<life::Mark> Marks(const Book& B, double From, double To)
{
	std::vector<life::Mark> Out;
	auto Add = [&Out, From, To](double At, int Kind, const std::string& Label, const std::string& Where) {
		if (At >= From && At < To)
		{
			life::Mark M;
			M.At = At;
			M.Kind = Kind;
			M.Label = Label;
			M.Where = Where;
			Out.push_back(M);
		}
	};
	for (const Contract& C : B.Contracts)
	{
		const std::string Short = ShortOf(C.Brand);
		if (OpenOffer(C))
		{
			Add(C.ExpiresAt, MarkOfferEnds, Short + " offer ends", "Mail");
			continue;
		}
		if (!Live(C))
		{
			continue;
		}
		for (int P = C.Period + 1; P <= Periods(C); ++P)
		{
			Add(PeriodEnd(C, P), MarkReview, P == Periods(C) ? Short + " term ends" : Short + " review", "Mail");
		}
		if (sponsor_step_detail::RenewalDue(B, C))
		{
			Add(sponsor_step_detail::RenewalAt(C), MarkRenewal, Short + " renewal", "Mail");
		}
		if (C.CureUntil > 0.0)
		{
			Add(C.CureUntil, MarkCure, Short + " cure deadline", "Mail");
		}
	}
	// The first open window of each contract's appearance in the span.
	std::set<std::string> Seen;
	for (const Window& W : Appearances(B, From, To))
	{
		if (!W.Done && Seen.insert(W.Contract).second)
		{
			const shop::Venue* V = shop::FindVenue(W.Venue);
			Add(std::max(W.From, From), MarkAppearance, W.Label, V ? V->Name : std::string("The Embercrest"));
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const life::Mark& A, const life::Mark& C) { return A.At < C.At; });
	return Out;
}

Chips NpcWeekly(const std::string& BrandId)
{
	const Brand* Br = FindBrand(BrandId);
	if (!Br)
	{
		return 0;
	}
	const Terms& T = Br->Offer;
	const double Month = static_cast<double>(T.MonthlyCents) + T.Asks[static_cast<size_t>(Ask::LiveHours)] * static_cast<double>(T.PerLiveHourCents) +
		T.Asks[static_cast<size_t>(Ask::Reads)] * static_cast<double>(T.PerReadCents);
	return static_cast<Chips>(std::llround(Month / 4.3 / 100.0)) * 100;
}

// ------------------------------------------------------------------ the contract card (Mail)

const char* Pill(const Contract& C, double World)
{
	switch (C.St)
	{
	case State::Offered:
	case State::Countered: return C.ExpiresAt > World ? "OFFER" : "ENDED";
	case State::Active: return C.CureUntil > World ? "STRIKE" : C.Until <= World ? "ENDED" : "ACTIVE";
	case State::Notice: return "NOTICE";
	case State::Ended: return "ENDED";
	}
	return "ENDED";
}

Card CardOf(const Contract& C, const life::State& L, const kast::Channel& Ch, double World)
{
	Card K;
	const Terms& T = C.T;
	const Brand* Br = BrandOf(C);
	// What you get.
	if (T.MonthlyCents > 0)
	{
		K.Get.push_back(Dollars(T.MonthlyCents) + " a month, paid at each review");
	}
	if (T.PerLiveHourCents > 0)
	{
		K.Get.push_back(Dollars(T.PerLiveHourCents) + " for every hour live");
	}
	if (T.PerReadCents > 0)
	{
		K.Get.push_back(Dollars(T.PerReadCents) + " a read, one a stream");
	}
	for (const Perk& Pk : T.Perks)
	{
		switch (Pk.Kind)
		{
		case Gift::StoreCredit: K.Get.push_back(Dollars(Pk.Cents) + " a week in store credit"); break;
		case Gift::Product: K.Get.push_back(std::to_string(Pk.Count) + "-pack at the door every Monday"); break;
		case Gift::Service: K.Get.push_back("A cut or line-up every " + std::to_string(Pk.EveryDays) + " days, on the house"); break;
		case Gift::Gear: K.Get.push_back(Pk.Id == "gym" ? std::string("Your gym membership, paid") : Pk.Id == "camera" ? std::string("The next Halo camera up") : std::string("A Sitwell chair")); break;
		case Gift::Garment: K.Get.push_back("A kit wearing the mark"); break;
		case Gift::Ticket: K.Get.push_back(std::to_string(Pk.Count) + " Daily Main tickets a month"); break;
		}
	}
	if (T.CoveredEntries > 0)
	{
		// "1 covered Noon Deepstack a month", "2 covered entries of $120 or less a month".
		const bool One = T.CoveredEntries == 1;
		const std::string Which = T.CoveredEvents.size() == 1 ? std::string(One ? "Noon Deepstack" : "Noon Deepstacks")
															   : std::string(One ? "entry" : "entries") + " of " + Dollars(T.CoveredCapCents) + " or less";
		K.Get.push_back(std::to_string(T.CoveredEntries) + " covered " + Which + " a month");
	}
	if (T.CoversFares)
	{
		K.Get.push_back("The bus both ways on covered nights");
	}
	if (T.FinalTableBonusCents > 0)
	{
		K.Get.push_back(Dollars(T.FinalTableBonusCents) + " per final table in the patch");
	}
	if (T.TitleBonusCents > 0)
	{
		K.Get.push_back(Dollars(T.TitleBonusCents) + " per title in the patch");
	}
	if (T.PerTeachingCents > 0)
	{
		K.Get.push_back(Dollars(T.PerTeachingCents) + " per teaching stream (" + std::to_string(T.TeachingCap) + " at most)");
	}
	if (T.SigningCents > 0)
	{
		K.Get.push_back(Dollars(T.SigningCents) + " on signing");
	}
	// What they ask, with this period's progress.
	const int P = Live(C) && C.Since <= World ? std::max(1, std::min(C.Period + 1, Periods(C))) : 1;
	const Tally Now = Live(C) && C.Since <= World ? Measure(C, L, Ch, World) : Tally();
	Contract Shape = C;
	if (!Live(C))
	{
		Shape.Since = World;
		Shape.Until = World + static_cast<double>(T.Days) * DayMinutes;
	}
	for (const Ask A : AllAsks)
	{
		const double N = Need(Shape, A, P);
		if (N > 0.0)
		{
			K.Asks.push_back({A == Ask::Appearances ? "Appearance: " + AppearanceWords(C.Brand) : std::string(AskLabel(A)), Have(Now, A), N});
		}
	}
	if (Live(C) && C.Since <= World)
	{
		const double Ends = PeriodEnd(C, P);
		K.Deadline = "Period ends " + net::DateLabel(net::DayOf(Ends)) + " \xC2\xB7 " + std::to_string(std::max(0, static_cast<int>(std::ceil((Ends - World) / DayMinutes)))) + " days";
	}
	// The fine print.
	const std::string Cat = Br ? CategoryWord(Br->Cat) : "sponsor";
	K.Fine.push_back(std::to_string(T.Days) + " days" + (T.Exclusive ? " \xC2\xB7 " + Cat + ", exclusive" : std::string()));
	const std::vector<Spot> Spots = SpotsOf(T);
	if (!Spots.empty())
	{
		K.Fine.push_back("Patch: " + sponsor_sign_detail::SpotsWords(Spots));
	}
	if (HasOverlay(C))
	{
		K.Fine.push_back("Overlay: one of your two slots");
	}
	if (HasAsks(T))
	{
		K.Fine.push_back("A month short is a strike: half held, " + std::to_string(T.CureDays) + " days to make it up");
		K.Fine.push_back("Two strikes in a term end it");
	}
	if (T.ConductClause)
	{
		K.Fine.push_back(C.Brand == "luckypenny" ? "Conduct: the owner forgives one pickup" : "Conduct: a pickup ends it");
	}
	if (C.Legacy)
	{
		K.Fine.push_back("Kast's old terms: hourly and reads, nothing asked");
	}
	return K;
}

// ------------------------------------------------------------------ copy

std::vector<std::pair<std::string, std::string>> Inventory(int Kind)
{
	return sponsor_copy::All(Kind);
}

std::map<std::string, std::string> InventorySamples()
{
	return sponsor_copy::Samples();
}

// ------------------------------------------------------------------ the save (§7.4) and the migration (§3.8)

namespace sponsor_save_detail
{
std::string TallyFields(const Tally& T)
{
	return Num2(T.LiveMinutes) + "\t" + std::to_string(T.Reads) + "\t" + std::to_string(T.Teaching) + "\t" + std::to_string(T.LiveEvents) + "\t" + std::to_string(T.Embercrest) + "\t" +
		std::to_string(T.Workouts) + "\t" + std::to_string(T.Appearances) + "\t" + std::to_string(T.CoveredUsed) + "\t" + std::to_string(T.Community);
}

Tally ReadTally(const std::vector<std::string>& F, size_t At)
{
	Tally T;
	T.LiveMinutes = std::atof(F[At].c_str());
	T.Reads = std::atoi(F[At + 1].c_str());
	T.Teaching = std::atoi(F[At + 2].c_str());
	T.LiveEvents = std::atoi(F[At + 3].c_str());
	T.Embercrest = std::atoi(F[At + 4].c_str());
	T.Workouts = std::atoi(F[At + 5].c_str());
	T.Appearances = std::atoi(F[At + 6].c_str());
	T.CoveredUsed = std::atoi(F[At + 7].c_str());
	T.Community = std::atoi(F[At + 8].c_str());
	return T;
}

bool Empty(const Tally& T)
{
	return T.LiveMinutes == 0.0 && T.Reads == 0 && T.Teaching == 0 && T.LiveEvents == 0 && T.Embercrest == 0 && T.Workouts == 0 && T.Appearances == 0 && T.CoveredUsed == 0 &&
		T.Community == 0;
}

std::string TermsFields(const Terms& T)
{
	std::string Out = "days=" + std::to_string(T.Days) + "\tsign=" + std::to_string(T.SigningCents) + "\tmo=" + std::to_string(T.MonthlyCents) + "\thr=" + std::to_string(T.PerLiveHourCents) +
		"\trd=" + std::to_string(T.PerReadCents) + "\tft=" + std::to_string(T.FinalTableBonusCents) + "\tti=" + std::to_string(T.TitleBonusCents) + "\tce=" + std::to_string(T.CoveredEntries) +
		"\tcc=" + std::to_string(T.CoveredCapCents) + "\tfa=" + (T.CoversFares ? "1" : "0") + "\tcut=" + std::to_string(T.PrizeCutPct) + "\tex=" + (T.Exclusive ? "1" : "0") +
		"\tcond=" + (T.ConductClause ? "1" : "0") + "\tcure=" + std::to_string(T.CureDays) + "\tpt=" + std::to_string(T.PerTeachingCents) + "\ttc=" + std::to_string(T.TeachingCap);
	std::string Asks;
	for (size_t I = 0; I < T.Asks.size(); ++I)
	{
		Asks += (I == 0 ? "" : ",") + NumShort(T.Asks[I]);
	}
	Out += "\task=" + Asks;
	std::string Wants;
	for (const Spot S : T.Wants)
	{
		Wants += (Wants.empty() ? "" : ".") + std::to_string(static_cast<int>(S));
	}
	Out += "\twants=" + Wants;
	std::string Events;
	for (const std::string& E : T.CoveredEvents)
	{
		Events += (Events.empty() ? "" : ".") + E;
	}
	Out += "\tev=" + Events;
	for (const Perk& Pk : T.Perks)
	{
		Out += "\tperk=" + std::to_string(static_cast<int>(Pk.Kind)) + "." + Pk.Id + "." + std::to_string(Pk.Count) + "." + std::to_string(Pk.Cents) + "." + std::to_string(Pk.EveryDays);
	}
	return Out;
}

void ReadTerms(const std::vector<std::string>& F, size_t From, Terms& T)
{
	T = Terms();
	for (size_t I = From; I < F.size(); ++I)
	{
		const size_t Eq = F[I].find('=');
		if (Eq == std::string::npos)
		{
			continue;
		}
		const std::string K = F[I].substr(0, Eq);
		const std::string V = F[I].substr(Eq + 1);
		const long long N = std::strtoll(V.c_str(), nullptr, 10);
		if (K == "days") T.Days = static_cast<int>(N);
		else if (K == "sign") T.SigningCents = N;
		else if (K == "mo") T.MonthlyCents = N;
		else if (K == "hr") T.PerLiveHourCents = N;
		else if (K == "rd") T.PerReadCents = N;
		else if (K == "ft") T.FinalTableBonusCents = N;
		else if (K == "ti") T.TitleBonusCents = N;
		else if (K == "ce") T.CoveredEntries = static_cast<int>(N);
		else if (K == "cc") T.CoveredCapCents = N;
		else if (K == "fa") T.CoversFares = N != 0;
		else if (K == "cut") T.PrizeCutPct = static_cast<int>(N);
		else if (K == "ex") T.Exclusive = N != 0;
		else if (K == "cond") T.ConductClause = N != 0;
		else if (K == "cure") T.CureDays = static_cast<int>(N);
		else if (K == "pt") T.PerTeachingCents = N;
		else if (K == "tc") T.TeachingCap = static_cast<int>(N);
		else if (K == "ask")
		{
			const std::vector<std::string> A = Split(V, ',');
			for (size_t J = 0; J < A.size() && J < T.Asks.size(); ++J)
			{
				T.Asks[J] = std::atof(A[J].c_str());
			}
		}
		else if (K == "wants")
		{
			for (const std::string& S : Split(V, '.'))
			{
				if (!S.empty())
				{
					T.Wants.push_back(static_cast<Spot>(std::clamp(std::atoi(S.c_str()), 0, static_cast<int>(Spot::Count) - 1)));
				}
			}
		}
		else if (K == "ev")
		{
			for (const std::string& S : Split(V, '.'))
			{
				if (!S.empty())
				{
					T.CoveredEvents.push_back(S);
				}
			}
		}
		else if (K == "perk")
		{
			const std::vector<std::string> P = Split(V, '.');
			if (P.size() >= 5)
			{
				Perk Pk;
				Pk.Kind = static_cast<Gift>(std::clamp(std::atoi(P[0].c_str()), 0, static_cast<int>(Gift::Ticket)));
				Pk.Id = P[1];
				Pk.Count = std::atoi(P[2].c_str());
				Pk.Cents = std::strtoll(P[3].c_str(), nullptr, 10);
				Pk.EveryDays = std::atoi(P[4].c_str());
				T.Perks.push_back(Pk);
			}
		}
	}
}

std::string JoinStrings(const std::vector<std::string>& V, char Sep)
{
	std::string Out;
	for (size_t I = 0; I < V.size(); ++I)
	{
		Out += (I == 0 ? std::string() : std::string(1, Sep)) + V[I];
	}
	return Out;
}
} // namespace sponsor_save_detail

void WriteLines(const life::State& L, std::string& Out)
{
	using namespace sponsor_save_detail;
	const Book& B = L.Sponsors;
	const bool Any = (B.LinesRead & LineBook) != 0 || !B.Contracts.empty() || !B.Cooldown.empty() || !B.PaidKeys.empty() || B.StoreCreditCents != 0 || !B.Days.empty() ||
		!B.Strikes.empty() || B.PennyPass;
	if (Any)
	{
		Out += "sponsor\tbook\t" + Num2(B.SteppedTo) + "\t" + std::to_string(B.StoreCreditCents) + "\t" + (B.PennyPass ? "1" : "0") + "\n";
		for (const auto& D : B.Days)
		{
			std::vector<std::string> Days;
			for (const int X : D.second)
			{
				Days.push_back(std::to_string(X));
			}
			Out += "sponsor\tdays\t" + Esc(D.first) + "\t" + JoinStrings(Days, '|') + "\n";
		}
		if (!B.Strikes.empty())
		{
			std::vector<std::string> S;
			for (const double X : B.Strikes)
			{
				S.push_back(Num2(X));
			}
			Out += "sponsor\tstrikes\t" + JoinStrings(S, '|') + "\n";
		}
		for (const Contract& C : B.Contracts)
		{
			Out += "sponsor\tdeal\t" + Esc(C.Id) + "\t" + Esc(C.Brand) + "\t" + std::to_string(static_cast<int>(C.St)) + "\t" + std::to_string(static_cast<int>(C.Why)) + "\t" + Num2(C.OfferedAt) +
				"\t" + Num2(C.ExpiresAt) + "\t" + Num2(C.Since) + "\t" + Num2(C.Until) + "\t" + std::to_string(C.Period) + "\t" + std::to_string(C.Strikes) + "\t" +
				std::to_string(C.Counters) + "\t" + Num2(C.CureUntil) + "\t" + std::to_string(C.PaidCents) + "\t" + (C.Legacy ? "1" : "0") + "\t" + std::to_string(C.TierAtSigning) + "\t" +
				Num2(C.ReadAt) + "\t" + std::to_string(C.WorkoutBase) + "\t" + std::to_string(C.HeldCents) + "\t" + Num2(C.CureFrom) + "\t" + std::to_string(C.CureWorkoutBase) + "\t" +
				(C.CompsHalved ? "1" : "0") + "\t" + Num2(C.ReplyAt) + "\t" + std::to_string(C.Pending) + "\t" + (C.Raised ? "1" : "0") + "\t" + Esc(C.Renews) + "\t" + NumShort(C.MetSum) +
				"\t" + (C.Kept ? "1" : "0") + "\t" + Esc(JoinStrings(C.Property, '|')) + "\n";
			Out += "sponsor\tterms\t" + Esc(C.Id) + "\t" + TermsFields(C.T) + "\n";
			if (!Empty(C.Now))
			{
				Out += "sponsor\ttally\t" + Esc(C.Id) + "\tnow\t" + TallyFields(C.Now) + "\n";
			}
			if (!Empty(C.Short))
			{
				Out += "sponsor\ttally\t" + Esc(C.Id) + "\tshort\t" + TallyFields(C.Short) + "\n";
			}
			if (!Empty(C.Cure))
			{
				Out += "sponsor\ttally\t" + Esc(C.Id) + "\tcure\t" + TallyFields(C.Cure) + "\n";
			}
		}
		for (const std::string& K : B.PaidKeys)
		{
			Out += "sponsor\tpaid\t" + Esc(K) + "\n";
		}
		for (const auto& Cd : B.Cooldown)
		{
			Out += "sponsor\tcool\t" + Esc(Cd.first) + "\t" + Num2(Cd.second) + "\n";
		}
	}
	for (int I = 0; I < PatchSpots; ++I)
	{
		const std::string& P = L.Closet.Patches[static_cast<size_t>(I)];
		if (!P.empty())
		{
			Out += "sponsor\tpatch\t" + std::to_string(I) + "\t" + Esc(P) + "\n";
		}
	}
}

bool ReadLine(const std::vector<std::string>& F, life::State& L)
{
	using namespace sponsor_save_detail;
	if (F.size() < 3 || F[0] != "sponsor")
	{
		return false;
	}
	Book& B = L.Sponsors;
	const std::string& K = F[1];
	if (K == "book" && F.size() >= 5)
	{
		B.SteppedTo = std::atof(F[2].c_str());
		B.StoreCreditCents = std::strtoll(F[3].c_str(), nullptr, 10);
		B.PennyPass = F[4] == "1";
		B.LinesRead |= LineBook;
		return true;
	}
	if (K == "days" && F.size() >= 4)
	{
		std::vector<int>& D = B.Days[Unesc(F[2])];
		for (const std::string& X : Split(F[3], '|'))
		{
			if (!X.empty())
			{
				D.push_back(std::atoi(X.c_str()));
			}
		}
		return true;
	}
	if (K == "strikes")
	{
		for (const std::string& X : Split(F[2], '|'))
		{
			if (!X.empty())
			{
				B.Strikes.push_back(std::atof(X.c_str()));
			}
		}
		return true;
	}
	if (K == "deal" && F.size() >= 30)
	{
		Contract C;
		C.Id = Unesc(F[2]);
		C.Brand = Unesc(F[3]);
		C.St = static_cast<State>(std::clamp(std::atoi(F[4].c_str()), 0, static_cast<int>(State::Ended)));
		C.Why = static_cast<End>(std::clamp(std::atoi(F[5].c_str()), 0, static_cast<int>(End::NotRenewed)));
		C.OfferedAt = std::atof(F[6].c_str());
		C.ExpiresAt = std::atof(F[7].c_str());
		C.Since = std::atof(F[8].c_str());
		C.Until = std::atof(F[9].c_str());
		C.Period = std::atoi(F[10].c_str());
		C.Strikes = std::atoi(F[11].c_str());
		C.Counters = std::atoi(F[12].c_str());
		C.CureUntil = std::atof(F[13].c_str());
		C.PaidCents = std::strtoll(F[14].c_str(), nullptr, 10);
		C.Legacy = F[15] == "1";
		C.TierAtSigning = std::atoi(F[16].c_str());
		C.ReadAt = std::atof(F[17].c_str());
		C.WorkoutBase = std::atoi(F[18].c_str());
		C.HeldCents = std::strtoll(F[19].c_str(), nullptr, 10);
		C.CureFrom = std::atof(F[20].c_str());
		C.CureWorkoutBase = std::atoi(F[21].c_str());
		C.CompsHalved = F[22] == "1";
		C.ReplyAt = std::atof(F[23].c_str());
		C.Pending = std::atoi(F[24].c_str());
		C.Raised = F[25] == "1";
		C.Renews = Unesc(F[26]);
		C.MetSum = std::atof(F[27].c_str());
		C.Kept = F[28] == "1";
		const std::string Prop = Unesc(F[29]);
		for (const std::string& P : Split(Prop, '|'))
		{
			if (!P.empty())
			{
				C.Property.push_back(P);
			}
		}
		B.Contracts.push_back(C);
		B.LinesRead |= LineDeal;
		return true;
	}
	if (K == "terms" && F.size() >= 4)
	{
		if (Contract* C = Find(B, Unesc(F[2])))
		{
			ReadTerms(F, 3, C->T);
		}
		return true;
	}
	if (K == "tally" && F.size() >= 13)
	{
		if (Contract* C = Find(B, Unesc(F[2])))
		{
			const Tally T = ReadTally(F, 4);
			(F[3] == "now" ? C->Now : F[3] == "short" ? C->Short : C->Cure) = T;
		}
		return true;
	}
	if (K == "paid")
	{
		B.PaidKeys.insert(Unesc(F[2]));
		return true;
	}
	if (K == "cool" && F.size() >= 4)
	{
		B.Cooldown[Unesc(F[2])] = std::atof(F[3].c_str());
		return true;
	}
	if (K == "patch" && F.size() >= 4)
	{
		const int I = std::atoi(F[2].c_str());
		if (I >= 0 && I < PatchSpots)
		{
			L.Closet.Patches[static_cast<size_t>(I)] = Unesc(F[3]);
		}
		B.LinesRead |= LinePatch;
		return true;
	}
	return false;
}

void AfterParse(SaveData& D)
{
	Book& B = D.Life.Sponsors;
	if ((B.LinesRead & LineBook) != 0)
	{
		return;
	}
	kast::Channel& Ch = D.Channel;
	if (Ch.Deals.empty() && Ch.Offers.empty() && Ch.Declined.empty())
	{
		return;
	}
	const double World = SaveWorld(D);
	// 1. A deal still running: an active, legacy contract on its old terms (hourly and reads, nothing asked).
	for (const kast::Deal& Old : Ch.Deals)
	{
		const Brand* Br = FindBrand(Old.Id);
		if (!Br || Old.Until <= World)
		{
			continue;
		}
		Contract C;
		C.Id = sponsor_offer_detail::NewId(B, Old.Id, Old.Since);
		C.Brand = Old.Id;
		C.St = State::Active;
		C.Legacy = true;
		C.OfferedAt = Old.Since;
		C.Since = Old.Since;
		C.Until = Old.Until;
		C.ReadAt = Old.ReadAt;
		C.PaidCents = Old.EarnedCents;
		C.T = Terms();
		C.T.Days = static_cast<int>(std::lround((Old.Until - Old.Since) / DayMinutes));
		C.T.PerLiveHourCents = Br->Offer.PerLiveHourCents;
		C.T.PerReadCents = Br->Offer.PerReadCents;
		C.T.Wants = {Spot::Overlay};
		C.T.Exclusive = false;
		C.T.ConductClause = false;
		C.WorkoutBase = D.Life.Fit.Workouts;
		B.Contracts.push_back(C);
		// On the books, and in Mail so the player can find it there.
		Post(D.Life, World, B.Contracts.back(), LetterWelcome, "migrated:active", BodyFor(B.Contracts.back(), "migrated:active", {{"Until", DateWords(Old.Until)}}));
	}
	// 2. An offer waiting: today's terms, a week to answer, and a letter so Mail shows it.
	for (const std::string& Id : Ch.Offers)
	{
		const Brand* Br = FindBrand(Id);
		if (!Br || Running(B, Id, World))
		{
			continue;
		}
		Contract C;
		C.Id = sponsor_offer_detail::NewId(B, Id, World);
		C.Brand = Id;
		C.St = State::Offered;
		C.T = Br->Offer;
		C.OfferedAt = World;
		C.ExpiresAt = World + OfferDays * DayMinutes;
		B.Contracts.push_back(C);
		Post(D.Life, World, B.Contracts.back(), LetterOffer, "migrated", BodyFor(B.Contracts.back(), "migrated"));
	}
	// 3. A declined one: sixty days, not never.
	for (const std::string& Id : Ch.Declined)
	{
		B.Cooldown[Id] = std::max(B.Cooldown[Id], World + DeclineCooldownDays * DayMinutes);
	}
	// 4. Kast's own lists are history now: only sponsor lines are written from here on.
	Ch.Deals.clear();
	Ch.Offers.clear();
	Ch.Declined.clear();
	B.SteppedTo = World;
	B.NextAt = 0.0;
}
} // namespace sponsor
} // namespace ss
