#include "ShortStack/Game/Wardrobe.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

// The wardrobe (the realism update's spec, §5.2 and §7.1). Picks recorded here, as the spec asks where it leaves room:
//  - Prices are the thrift's list prices inside §5.2's ranges: tees $3 plain, $4-5 print, $5-6 graphic; jeans $10-14 by
//    wash; chinos $9; work pants $11; shorts $4; sneakers $12; work shoes $15 (the table has none: priced a little over
//    the sneakers); hoodie and flannel $8; track jacket $12; denim $16; bomber $18; leather $38 ($45 for the rare
//    rider); hats $3-6; frames $5-8. The pawn's aviators ($18) and trilby ($14) are §5.8's.
//  - Condition: a thrift piece also comes Worn and Rough (15% and 30% off, §5.2), as its own id; Style falls with it
//    (x0.85, x0.7).
//  - Tags: one of five colors per piece, fixed by its id (a fifth of the racks are yellow); rare finds are white.
//  - The racks: 24-30 pieces a week (about a third tops, a quarter jackets, a fifth trousers, the rest shoes, hats and
//    frames), one piece per garment, plus 1-2 rare finds in each four-week block.

namespace ss
{
namespace wardrobe
{
namespace wardrobe_detail
{
const char* const ColorKeys[10] = {"charcoal", "navy", "olive", "burgundy", "sand", "black", "teal", "mustard", "white", "heather"};
const char* const ColorWords[10] = {"Charcoal", "Navy", "Olive", "Burgundy", "Sand", "Black", "Teal", "Mustard", "White", "Heather grey"};

Chips Cents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}

std::vector<Garment> Build()
{
	std::vector<Garment> L;
	auto Add = [&L](const std::string& Id, Cut K, const std::string& Name, int Color, double Price, double Style, int Warmth, bool Rain) -> Garment& {
		Garment G;
		G.Id = Id;
		G.Kind = K;
		G.Where = SlotOf(K);
		G.Name = Name;
		G.Color = Color;
		G.PriceCents = Cents(Price);
		G.Style = Style;
		G.Warmth = Warmth;
		G.RainProof = Rain;
		L.push_back(G);
		return L.back();
	};

	// Jackets: each of the creator's six in each of its eight colors, so any created look is a closet (hero::Look's
	// Outfit k is Cut k + 1).
	struct Jacket
	{
		Cut K;
		const char* Key;
		const char* Word;
		double Price;
		double Style;
		int Warmth;
		bool Rain;
	};
	static const Jacket Jackets[] = {
		{Cut::Hoodie, "hoodie", "hoodie", 8.0, 0.40, 2, false},
		{Cut::Bomber, "bomber", "bomber", 18.0, 0.55, 2, false},
		{Cut::Flannel, "flannel", "flannel", 8.0, 0.45, 1, false},
		{Cut::Denim, "denim", "denim jacket", 16.0, 0.55, 1, false},
		{Cut::Leather, "leather", "leather jacket", 38.0, 0.70, 2, true},
		{Cut::Track, "track", "track jacket", 12.0, 0.45, 1, true},
	};
	for (const Jacket& J : Jackets)
	{
		for (int C = 0; C < 8; ++C)
		{
			Garment& G = Add(std::string("outer.") + J.Key + "." + ColorKeys[C], J.K, std::string(ColorWords[C]) + " " + J.Word, C, J.Price, J.Style, J.Warmth, J.Rain);
			if (J.K == Cut::Flannel)
			{
				G.Print = C == 4 || C == 6 ? 4 : 3; // gingham in the light colors, tartan otherwise
			}
		}
	}
	// The thrift's rare finds (one or two a month).
	{
		Garment& G = Add("outer.leather.rider", Cut::Leather, "Vintage leather rider", 5, 45.0, 0.90, 2, true);
		G.Rare = true;
	}
	{
		Garment& G = Add("outer.track.embercrest", Cut::Track, "Embercrest staff fleece", 3, 22.0, 0.70, 2, false);
		G.Graphic = 0; // the crest on the chest
		G.Rare = true;
	}

	// Tees: plain in ten colors, five prints, six graphics, and the rare Sunday Major tee.
	for (int C = 0; C < 10; ++C)
	{
		const int Color = (C + 8) % 10; // white and heather first, then the creator's palette
		Add(std::string("top.tee.") + ColorKeys[Color], Cut::Tee, std::string(ColorWords[Color]) + " tee", Color, 3.0, 0.30, 0, false);
	}
	struct Printed
	{
		const char* Key;
		const char* Name;
		int Color;
		int Art;
		double Price;
	};
	static const Printed Prints[] = {
		{"stripes", "Striped tee", 1, 0, 4.0},
		{"breton", "Breton tee", 8, 1, 4.0},
		{"ringer", "Ringer tee", 8, 2, 4.0},
		{"dots", "Polka-dot tee", 5, 5, 4.0},
		{"camo", "Camo tee", 2, 6, 5.0},
	};
	for (const Printed& P : Prints)
	{
		Garment& G = Add(std::string("top.print.") + P.Key, Cut::Tee, P.Name, P.Color, P.Price, 0.38, 0, false);
		G.Print = P.Art;
	}
	static const Printed Graphics[] = {
		{"embercrest", "Embercrest tee", 5, 0, 6.0},
		{"allin", "All In tee", 0, 1, 5.0},
		{"chip", "Chip Stack tee", 1, 2, 5.0},
		{"badbeat", "Bad Beat tee", 8, 3, 6.0},
		{"sunset", "Sunset tee", 4, 4, 5.0},
		{"number", "Jersey-number tee", 6, 5, 5.0},
	};
	for (const Printed& P : Graphics)
	{
		Garment& G = Add(std::string("top.graphic.") + P.Key, Cut::Tee, P.Name, P.Color, P.Price, 0.42, 0, false);
		G.Graphic = P.Art;
	}
	{
		Garment& G = Add("top.graphic.sundaymajor", Cut::Tee, "RiverLine Sunday Major 2019 tee", 5, 9.0, 0.65, 0, false);
		G.Graphic = 6;
		G.Rare = true;
	}

	// Trousers.
	Add("bottom.jeans.dark", Cut::Jeans, "Dark-wash jeans", 0, 14.0, 0.50, 1, false);
	Add("bottom.jeans.mid", Cut::Jeans, "Mid-wash jeans", 1, 12.0, 0.45, 1, false);
	Add("bottom.jeans.light", Cut::Jeans, "Light-wash jeans", 2, 10.0, 0.42, 1, false);
	Add("bottom.jeans.black", Cut::Jeans, "Black jeans", 3, 12.0, 0.48, 1, false);
	Add("bottom.chinos.khaki", Cut::Chinos, "Khaki chinos", 0, 9.0, 0.50, 1, false);
	Add("bottom.chinos.navy", Cut::Chinos, "Navy chinos", 1, 9.0, 0.50, 1, false);
	Add("bottom.chinos.olive", Cut::Chinos, "Olive chinos", 2, 9.0, 0.50, 1, false);
	Add("bottom.chinos.black", Cut::Chinos, "Black chinos", 3, 9.0, 0.50, 1, false);
	Add("bottom.workpants.duck", Cut::WorkPants, "Brown duck work pants", 0, 11.0, 0.38, 2, false);
	Add("bottom.workpants.black", Cut::WorkPants, "Black work pants", 1, 11.0, 0.38, 2, false);
	Add("bottom.shorts.khaki", Cut::Shorts, "Khaki shorts", 0, 4.0, 0.20, 0, false);
	Add("bottom.shorts.grey", Cut::Shorts, "Grey shorts", 1, 4.0, 0.20, 0, false);
	Add("bottom.shorts.navy", Cut::Shorts, "Navy shorts", 2, 4.0, 0.20, 0, false);

	// Shoes.
	Add("shoes.sneakers.white", Cut::Sneakers, "White sneakers", 0, 12.0, 0.40, 0, false);
	Add("shoes.sneakers.black", Cut::Sneakers, "Black sneakers", 1, 12.0, 0.40, 0, false);
	Add("shoes.sneakers.grey", Cut::Sneakers, "Grey sneakers", 2, 12.0, 0.40, 0, false);
	Add("shoes.work.brown", Cut::WorkShoes, "Brown work shoes", 0, 15.0, 0.45, 1, true);
	Add("shoes.work.black", Cut::WorkShoes, "Black work shoes", 1, 15.0, 0.45, 1, true);

	// Hats (hero::Look's Hat 1..4) and the pawn's trilby.
	Add("hat.beanie", Cut::Beanie, "Knit beanie", 0, 3.0, 0.30, 1, false);
	Add("hat.ballcap", Cut::BallCap, "Ball cap", 0, 5.0, 0.30, 0, false);
	Add("hat.capback", Cut::CapBack, "Snapback", 0, 6.0, 0.32, 0, false);
	Add("hat.bucket", Cut::Bucket, "Bucket hat", 0, 4.0, 0.35, 0, true);
	{
		Garment& G = Add("hat.trilby", Cut::Trilby, "Felt trilby", 0, 14.0, 0.75, 1, true);
		G.Tier = 1;
	}

	// Frames (hero::Look's Glasses 1..4) and the pawn's aviators.
	Add("eyes.round", Cut::Round, "Round frames", 0, 5.0, 0.40, 0, false);
	Add("eyes.square", Cut::Square, "Square frames", 0, 6.0, 0.40, 0, false);
	Add("eyes.wire", Cut::Wire, "Wire frames", 0, 8.0, 0.45, 0, false);
	Add("eyes.shades", Cut::Shades, "Shades", 0, 7.0, 0.45, 0, false);
	{
		Garment& G = Add("eyes.aviators", Cut::Aviators, "Gold-rim aviators", 0, 18.0, 0.75, 0, false);
		G.Tier = 1;
	}

	// The same pieces as the thrift has them a second time around: worn, and rough.
	const size_t Base = L.size();
	for (size_t I = 0; I < Base; ++I)
	{
		if (L[I].Tier != 0 || L[I].Rare)
		{
			continue;
		}
		for (int Cond = 1; Cond <= 2; ++Cond)
		{
			Garment G = L[I];
			G.Id += Cond == 1 ? "~worn" : "~rough";
			G.Condition = Cond;
			G.Style = G.Style * (Cond == 1 ? 0.85 : 0.7);
			L.push_back(G);
		}
	}
	return L;
}

const std::map<std::string, size_t>& Index()
{
	static const std::map<std::string, size_t> M = [] {
		std::map<std::string, size_t> Out;
		const std::vector<Garment>& L = Catalog();
		for (size_t I = 0; I < L.size(); ++I)
		{
			Out[L[I].Id] = I;
		}
		return Out;
	}();
	return M;
}

/** FNV-1a: a piece's own number (its tag), the same in every save. */
uint32_t Hash(const std::string& S)
{
	uint32_t H = 2166136261u;
	for (const char Ch : S)
	{
		H ^= static_cast<uint8_t>(Ch);
		H *= 16777619u;
	}
	return H;
}

std::string BaseId(const std::string& Id)
{
	const size_t At = Id.find('~');
	return At == std::string::npos ? Id : Id.substr(0, At);
}

int FloorDiv(int A, int B)
{
	return A >= 0 ? A / B : -((-A + B - 1) / B);
}

const Garment* Worn(const Closet& C, Slot S)
{
	const std::string& Id = C.Worn[static_cast<size_t>(S)];
	return Id.empty() ? nullptr : Find(Id);
}
} // namespace wardrobe_detail

using namespace wardrobe_detail;

const std::vector<Garment>& Catalog()
{
	static const std::vector<Garment> L = Build();
	return L;
}

const Garment* Find(const std::string& Id)
{
	const std::map<std::string, size_t>& M = Index();
	const auto It = M.find(Id);
	return It == M.end() ? nullptr : &Catalog()[It->second];
}

Slot SlotOf(Cut K)
{
	switch (K)
	{
	case Cut::Tee: return Slot::Top;
	case Cut::Hoodie:
	case Cut::Bomber:
	case Cut::Flannel:
	case Cut::Denim:
	case Cut::Leather:
	case Cut::Track: return Slot::Outer;
	case Cut::Shorts:
	case Cut::Jeans:
	case Cut::Chinos:
	case Cut::WorkPants: return Slot::Bottom;
	case Cut::Sneakers:
	case Cut::WorkShoes: return Slot::Shoes;
	case Cut::Beanie:
	case Cut::BallCap:
	case Cut::CapBack:
	case Cut::Bucket:
	case Cut::Trilby: return Slot::Hat;
	case Cut::Round:
	case Cut::Square:
	case Cut::Wire:
	case Cut::Shades:
	case Cut::Aviators: return Slot::Eyes;
	case Cut::Count: break;
	}
	return Slot::Top;
}

uint32_t Tone(const Garment& G)
{
	auto Pick = [](const uint32_t* Colors, int Count, int I) { return Colors[std::clamp(I, 0, Count - 1)]; };
	if (G.Where == Slot::Outer || G.Where == Slot::Top)
	{
		if (G.Color == 8)
		{
			return 0xe9e7e1;
		}
		if (G.Color == 9)
		{
			return 0x8f9092;
		}
		return hero::OutfitTone(std::clamp(G.Color, 0, 7));
	}
	static const uint32_t Jeans[] = {0x1f2b45, 0x3d5c86, 0x7d9bbf, 0x1c1d20};
	static const uint32_t Chinos[] = {0xb59e78, 0x1f2d4d, 0x4b5638, 0x1c1d20};
	static const uint32_t Work[] = {0x7e5f36, 0x1c1d20};
	static const uint32_t Shorts[] = {0xb59e78, 0x6e6f72, 0x1f2d4d};
	static const uint32_t Sneakers[] = {0xf0eee8, 0x18191b, 0x9a9c9f};
	static const uint32_t Boots[] = {0x5a3a22, 0x1b1b1b};
	switch (G.Kind)
	{
	case Cut::Jeans: return Pick(Jeans, 4, G.Color);
	case Cut::Chinos: return Pick(Chinos, 4, G.Color);
	case Cut::WorkPants: return Pick(Work, 2, G.Color);
	case Cut::Shorts: return Pick(Shorts, 3, G.Color);
	case Cut::Sneakers: return Pick(Sneakers, 3, G.Color);
	case Cut::WorkShoes: return Pick(Boots, 2, G.Color);
	case Cut::Beanie: return 0x34383f;
	case Cut::BallCap: return 0x1f2d4d;
	case Cut::CapBack: return 0x15161a;
	case Cut::Bucket: return 0xbfa883;
	case Cut::Trilby: return 0x4a3a2c;
	case Cut::Round: return 0x2a2522;
	case Cut::Square: return 0x15161a;
	case Cut::Wire: return 0xb8a46a;
	case Cut::Shades: return 0x101112;
	case Cut::Aviators: return 0xd4b062;
	default: return 0x808080;
	}
}

const char* PrintName(int Print)
{
	static const char* const Names[] = {"T_Print_Stripes", "T_Print_Breton", "T_Print_Ringer", "T_Print_Tartan", "T_Print_Gingham", "T_Print_Dots", "T_Print_Camo"};
	return Print >= 0 && Print < 7 ? Names[Print] : nullptr;
}

const char* GraphicName(int Graphic)
{
	static const char* const Names[] = {"T_Graphic_Embercrest", "T_Graphic_AllIn", "T_Graphic_Chip", "T_Graphic_BadBeat", "T_Graphic_Sunset", "T_Graphic_Number",
		"T_Graphic_SundayMajor"};
	return Graphic >= 0 && Graphic < 7 ? Names[Graphic] : nullptr;
}

const char* ConditionName(int Condition)
{
	return Condition <= 0 ? "Good" : Condition == 1 ? "Worn" : "Rough";
}

int TagOf(const Garment& G)
{
	if (G.Rare || G.Tier != 0)
	{
		return 0;
	}
	return static_cast<int>(Hash(G.Id) % 5u);
}

Closet SeedFrom(const hero::Look& L)
{
	Closet C;
	C.Seeded = true;
	auto Own = [&C](const std::string& Id, Slot Wear) {
		if (!Find(Id))
		{
			return;
		}
		C.Owned.insert(Id);
		if (Wear != Slot::Count)
		{
			C.Worn[static_cast<size_t>(Wear)] = Id;
		}
	};
	// What they had on in the creator: the jacket (none for "None"), the headwear and the frames.
	static const char* const JacketKeys[] = {"hoodie", "bomber", "flannel", "denim", "leather", "track"};
	if (L.Outfit >= 0 && L.Outfit < hero::OutfitNone)
	{
		Own(std::string("outer.") + JacketKeys[L.Outfit] + "." + ColorKeys[std::clamp(L.OutfitColor, 0, 7)], Slot::Outer);
	}
	static const char* const Hats[] = {"", "hat.beanie", "hat.ballcap", "hat.capback", "hat.bucket"};
	if (L.Hat >= 1 && L.Hat <= 4)
	{
		Own(Hats[L.Hat], Slot::Hat);
	}
	static const char* const Frames[] = {"", "eyes.round", "eyes.square", "eyes.wire", "eyes.shades"};
	if (L.Glasses >= 1 && L.Glasses <= 4)
	{
		Own(Frames[L.Glasses], Slot::Eyes);
	}
	// And what came in the suitcase. The shirt under the jacket stays the tee in the outfit color (Top ""), as the creator
	// showed it; the white tee is a spare.
	Own("top.tee.white", Slot::Count);
	Own("bottom.jeans.mid", Slot::Bottom);
	Own("shoes.sneakers.white", Slot::Shoes);
	Own("bottom.shorts.khaki", Slot::Count);
	return C;
}

void ApplyTo(const Closet& C, hero::Look& L)
{
	if (!C.Seeded && C.Owned.empty())
	{
		return;
	}
	if (const Garment* Outer = wardrobe_detail::Worn(C, Slot::Outer))
	{
		L.Outfit = static_cast<int>(Outer->Kind) - static_cast<int>(Cut::Hoodie);
		L.OutfitColor = std::clamp(Outer->Color, 0, 7);
	}
	else
	{
		L.Outfit = hero::OutfitNone;
		if (const Garment* Top = wardrobe_detail::Worn(C, Slot::Top))
		{
			L.OutfitColor = Top->Color == 8 ? 4 : Top->Color == 9 ? 0 : std::clamp(Top->Color, 0, 7);
		}
	}
	L.Hat = 0;
	if (const Garment* Hat = wardrobe_detail::Worn(C, Slot::Hat))
	{
		switch (Hat->Kind)
		{
		case Cut::Beanie: L.Hat = 1; break;
		case Cut::BallCap: L.Hat = 2; break;
		case Cut::CapBack: L.Hat = 3; break;
		default: L.Hat = 4; break; // the bucket hat (and the trilby, on the portrait)
		}
	}
	L.Glasses = 0;
	if (const Garment* Eyes = wardrobe_detail::Worn(C, Slot::Eyes))
	{
		switch (Eyes->Kind)
		{
		case Cut::Round: L.Glasses = 1; break;
		case Cut::Square: L.Glasses = 2; break;
		case Cut::Wire: L.Glasses = 3; break;
		default: L.Glasses = 4; break; // shades (and the aviators, on the portrait)
		}
	}
}

double StyleOf(const Closet& C)
{
	if (!C.Seeded && C.Owned.empty())
	{
		return 0.0;
	}
	// The jacket counts most, then the shirt, the trousers and the shoes; with no jacket the rest share its weight. An
	// empty Top is the plain tee in the outfit color and an empty Bottom the shorts; an empty Shoes slot is barefoot (the
	// closet's word for it) and adds nothing, so kicking the shoes off never dresses an outfit up.
	const Garment* Outer = wardrobe_detail::Worn(C, Slot::Outer);
	const Garment* Top = wardrobe_detail::Worn(C, Slot::Top);
	const Garment* Bottom = wardrobe_detail::Worn(C, Slot::Bottom);
	const Garment* Shoes = wardrobe_detail::Worn(C, Slot::Shoes);
	double Sum = 0.0;
	double Weight = 0.0;
	if (Outer)
	{
		Sum += 0.35 * Outer->Style;
		Weight += 0.35;
	}
	Sum += 0.25 * (Top ? Top->Style : 0.30);
	Sum += 0.20 * (Bottom ? Bottom->Style : 0.20);
	Sum += 0.20 * (Shoes ? Shoes->Style : 0.0);
	Weight += 0.65;
	double Style = Sum / Weight;
	// A vintage hat or frames finish it.
	for (const Slot S : {Slot::Hat, Slot::Eyes})
	{
		const Garment* G = wardrobe_detail::Worn(C, S);
		if (G && G->Tier >= 1)
		{
			Style += 0.05;
		}
	}
	return std::clamp(Style, 0.0, 1.0);
}

int ThriftWeek(double World)
{
	// Day 0 is a Monday; the first Thursday is day 3. Bea puts the new racks out before the doors open.
	return FloorDiv(net::DayOf(World) - 3, 7);
}

std::vector<std::string> ThriftStock(int Week, uint64_t Seed, uint64_t KindsAvailable)
{
	const std::vector<Garment>& All = Catalog();
	auto Showable = [KindsAvailable](const Garment& G) {
		const int K = static_cast<int>(G.Kind);
		return K >= 0 && K < 64 && ((KindsAvailable >> K) & 1ull) != 0;
	};
	// The pool, slot by slot: every thrift piece the host can draw (tier 0, not a rare find), in catalog order.
	std::array<std::vector<size_t>, SlotCount> Pool;
	for (size_t I = 0; I < All.size(); ++I)
	{
		const Garment& G = All[I];
		if (G.Tier == 0 && !G.Rare && Showable(G))
		{
			Pool[static_cast<size_t>(G.Where)].push_back(I);
		}
	}
	Rng R("thrift:" + std::to_string(Seed) + ":" + std::to_string(Week));
	const int Want = 24 + R.Int(7);
	for (std::vector<size_t>& P : Pool)
	{
		R.Shuffle(P);
	}
	// Roughly a third tops, a quarter jackets, a fifth trousers, and the shoes, hats and frames; a slot with too little
	// on offer gives its share to the rest.
	static const double Share[SlotCount] = {0.32, 0.26, 0.20, 0.08, 0.07, 0.07};
	std::array<int, SlotCount> Quota{};
	int Given = 0;
	for (int S = 0; S < SlotCount; ++S)
	{
		Quota[static_cast<size_t>(S)] = static_cast<int>(std::lround(Share[S] * static_cast<double>(Want)));
		Given += Quota[static_cast<size_t>(S)];
	}
	Quota[0] += Want - Given;
	std::vector<size_t> Picked;
	std::set<std::string> Bases;
	std::array<size_t, SlotCount> Cursor{};
	auto Take = [&](int S) {
		std::vector<size_t>& P = Pool[static_cast<size_t>(S)];
		size_t& At = Cursor[static_cast<size_t>(S)];
		while (At < P.size())
		{
			const size_t I = P[At++];
			// One of a garment on the racks (not the same navy hoodie twice in different states).
			if (Bases.insert(BaseId(All[I].Id)).second)
			{
				Picked.push_back(I);
				return true;
			}
		}
		return false;
	};
	int Short = 0;
	for (int S = 0; S < SlotCount; ++S)
	{
		for (int K = 0; K < Quota[static_cast<size_t>(S)]; ++K)
		{
			Short += Take(S) ? 0 : 1;
		}
	}
	for (int S = 0; Short > 0 && S < SlotCount; ++S)
	{
		while (Short > 0 && Take(S))
		{
			--Short;
		}
	}
	// One or two rare finds in each four weeks, in weeks of their own.
	const int Block = FloorDiv(Week, 4);
	Rng Rare("thrift-rare:" + std::to_string(Seed) + ":" + std::to_string(Block));
	std::vector<int> Weeks = {0, 1, 2, 3};
	Rare.Shuffle(Weeks);
	const int Finds = 1 + Rare.Int(2);
	const int InBlock = Week - Block * 4;
	if (std::find(Weeks.begin(), Weeks.begin() + Finds, InBlock) != Weeks.begin() + Finds)
	{
		std::vector<size_t> Rares;
		for (size_t I = 0; I < All.size(); ++I)
		{
			if (All[I].Rare && Showable(All[I]))
			{
				Rares.push_back(I);
			}
		}
		if (!Rares.empty())
		{
			Picked.push_back(Rares[static_cast<size_t>(Rare.Int(static_cast<int>(Rares.size())))]);
		}
	}
	// On the racks by slot (tops, jackets, trousers, shoes, hats, frames), each rack in catalog order.
	std::stable_sort(Picked.begin(), Picked.end(), [&All](size_t A, size_t B) {
		const int Sa = static_cast<int>(All[A].Where);
		const int Sb = static_cast<int>(All[B].Where);
		return Sa != Sb ? Sa < Sb : A < B;
	});
	std::vector<std::string> Out;
	Out.reserve(Picked.size());
	for (const size_t I : Picked)
	{
		Out.push_back(All[I].Id);
	}
	return Out;
}

Chips ThriftPrice(const Garment& G, double World)
{
	Chips Price = G.PriceCents;
	if (G.Tier == 0 && !G.Rare)
	{
		// Condition takes 0/15/30% off (the list prices are whole dollars: the cents come out exact).
		static const Chips Off[3] = {0, 15, 30};
		Price = Price * (100 - Off[std::clamp(G.Condition, 0, 2)]) / 100;
		// Yellow tags are half off on Saturdays (half a cent up).
		if (TagOf(G) == 1 && net::Weekday(net::DayOf(World)) == 5)
		{
			Price = (Price + 1) / 2;
		}
	}
	return Price;
}
} // namespace wardrobe
} // namespace ss
