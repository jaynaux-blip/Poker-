#include "ShortStack/Game/Hero.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace ss
{
namespace hero
{
namespace hero_detail
{
const BackgroundInfo Backgrounds[BackgroundCount] = {
	{Background::Kitchen, "kitchen", "Line Cook", "Twelve-hour shifts behind a flat-top.",
		"You've worked every station in every kitchen that would have you, and the late shift at Rosa's diner pays the minimum. Poker started on the lid of the "
		"walk-in cooler with the dishwashers after close. You still smell like the fryer.",
		"Double Shift", "ShiftLink jobs pay 20% more, meals go further, and food and drink restore 25% more energy.", 0xff8a3d},
	{Background::DealersKid, "dealerskid", "Dealer's Kid", "Grew up under a card room table.",
		"Your mother dealt the graveyard shift at the Embercrest for twenty years, and you did your homework in the break room. You learned to watch hands "
		"before you could shuffle. Dee dealt next to her once and still asks how she's doing.",
		"Born Reader", "A tell is learned the first time the cards confirm it, not the second.", 0xf2c14e},
	{Background::Dropout, "dropout", "Stats Dropout", "Two years of probability, then the online games.",
		"You were good at statistics and better at the games the statistics were about. The scholarship didn't survive the second year. The student loans "
		"did. You know exactly how bad your odds are, which is not the same as doing something about them.",
		"Expected Value", "Bounty tournaments are open on RiverLine from the first night.", 0x60a5fa},
	{Background::Bouncer, "bouncer", "Casino Bouncer", "Six years on the door at the Embercrest.",
		"You've thrown out drunks, card counters and a man who tried to eat his own chips. You've watched every kind of loser walk out at four in the "
		"morning, and you know what tilt looks like from the outside. Then the Embercrest cut the night staff.",
		"Thick Skin", "Bad beats and coolers put you on tilt 30% less.", 0xff5a5f},
	{Background::Hustler, "hustler", "Corner Hustler", "Knows Marcus from the old block.",
		"You ran errands for people like Marcus before you could drive, and you've never been picked up. You know which corners have cameras and which "
		"cops work nights. You'd rather win it at the table, but the table doesn't always pay the rent.",
		"Low Profile", "Burner runs are 5% less likely to go wrong, and police heat cools 50% faster.", 0xb36bff},
	{Background::Newcomer, "newcomer", "Fresh Start", "New in town with one suitcase.",
		"You sold everything that didn't fit in one suitcase and took the night bus to the city. The deposit on the apartment ate most of what you saved, "
		"and nobody here knows your name yet. That might be the best thing about it.",
		"Rainy-Day Fund", "You start with $40 more in the bank.", 0x27d3c3},
};

struct NamePool
{
	const char* Code;
	const char* First; // "|"-separated
	const char* Last;
};

const NamePool Pools[] = {
	{"AR", "Mateo|Santiago|Lucia|Valentina|Tomas|Camila", "Gonzalez|Fernandez|Acosta|Romero|Benitez"},
	{"AU", "Jack|Lachlan|Mia|Chloe|Riley|Sam", "Mitchell|Kelly|Nguyen|Walsh|Murray"},
	{"AT", "Lukas|Florian|Lena|Hannah|Felix|Anna", "Gruber|Huber|Bauer|Wagner|Pichler"},
	{"BE", "Arthur|Louis|Emma|Elise|Noah|Lotte", "Peeters|Janssens|Maes|Dubois|Claes"},
	{"BR", "Lucas|Gabriel|Ana|Beatriz|Rafael|Julia", "Silva|Santos|Oliveira|Costa|Rocha"},
	{"CA", "Liam|Owen|Emma|Maya|Logan|Chloe", "Tremblay|Roy|Gagnon|Campbell|Bouchard"},
	{"CN", "Wei|Hao|Li|Yan|Jun|Xin", "Wang|Zhang|Liu|Chen|Zhao"},
	{"CZ", "Jakub|Tomas|Tereza|Eliska|Ondrej|Klara", "Novak|Svoboda|Dvorak|Cerny|Prochazka"},
	{"DK", "Mads|Frederik|Freja|Ida|Magnus|Clara", "Jensen|Nielsen|Hansen|Larsen|Poulsen"},
	{"FI", "Juho|Aleksi|Aino|Veera|Eetu|Emma", "Virtanen|Korhonen|Nieminen|Laine|Salonen"},
	{"FR", "Hugo|Louis|Camille|Manon|Theo|Chloe", "Martin|Bernard|Moreau|Laurent|Girard"},
	{"DE", "Lukas|Jonas|Lena|Sophie|Felix|Mia", "Mueller|Schmidt|Fischer|Weber|Becker"},
	{"IN", "Arjun|Rohan|Priya|Ananya|Vikram|Isha", "Sharma|Patel|Iyer|Reddy|Mehta"},
	{"IE", "Cian|Conor|Aoife|Niamh|Sean|Ciara", "Murphy|Kelly|Byrne|Ryan|Doyle"},
	{"IT", "Luca|Matteo|Giulia|Chiara|Marco|Sofia", "Rossi|Russo|Esposito|Romano|Ricci"},
	{"JP", "Haruto|Ren|Yui|Hina|Sota|Aoi", "Sato|Suzuki|Takahashi|Tanaka|Ito"},
	{"MX", "Diego|Emiliano|Sofia|Valeria|Luis|Ximena", "Hernandez|Ramirez|Flores|Cruz|Morales"},
	{"NL", "Daan|Sem|Femke|Lotte|Bram|Sanne", "de Jong|Jansen|de Vries|Bakker|Visser"},
	{"NO", "Jakob|Emil|Nora|Ingrid|Henrik|Sofie", "Hansen|Johansen|Olsen|Larsen|Berg"},
	{"PL", "Jakub|Kacper|Zofia|Maja|Piotr|Ola", "Nowak|Kowalski|Wojcik|Kaminski|Zielinski"},
	{"PT", "Joao|Rodrigo|Beatriz|Mariana|Tiago|Ines", "Ferreira|Pereira|Carvalho|Gomes|Lopes"},
	{"RU", "Dmitri|Ivan|Anna|Olga|Nikita|Daria", "Ivanov|Smirnov|Volkov|Sokolov|Orlov"},
	{"KR", "Minjun|Jiho|Seoyeon|Jiwoo|Hyun|Yuna", "Kim|Lee|Park|Choi|Jung"},
	{"ES", "Pablo|Alvaro|Lucia|Marta|Sergio|Paula", "Garcia|Lopez|Martinez|Navarro|Torres"},
	{"SE", "Erik|Oscar|Elsa|Maja|Axel|Linnea", "Johansson|Andersson|Karlsson|Nilsson|Lindberg"},
	{"UA", "Taras|Bohdan|Oksana|Sofiia|Andriy|Iryna", "Shevchenko|Kovalenko|Bondarenko|Melnyk|Boyko"},
	{"GB", "Oliver|Harry|Amelia|Isla|George|Freya", "Smith|Taylor|Walker|Wright|Hughes"},
	{"US", "Jesse|Marcus|Dana|Kayla|Tyler|Riley", "Cole|Reyes|Brooks|Hayes|Carter"},
};

std::vector<std::string> SplitBars(const char* S)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char* P = S; *P; ++P)
	{
		if (*P == '|')
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur += *P;
		}
	}
	Out.push_back(Cur);
	return Out;
}

const char* const SlotKeys[SlotCount] = {"body", "face", "skin", "eyes", "brows", "hair", "haircolor", "facialhair", "build", "height", "outfit", "outfitcolor", "glasses", "hat"};
const char* const SlotNames[SlotCount] = {"BODY TYPE", "FACE SHAPE", "SKIN TONE", "EYE COLOR", "BROWS", "HAIRSTYLE", "HAIR COLOR", "FACIAL HAIR", "BUILD", "HEIGHT",
	"JACKET", "JACKET COLOR", "GLASSES", "HEADWEAR"};

const char* const BodyNames[] = {"Type A", "Type B"};
const char* const FaceNames[] = {"Oval", "Square", "Round", "Long", "Heart", "Angular"};
const char* const SkinNames[] = {"Porcelain", "Fair", "Light", "Warm Beige", "Olive", "Tan", "Golden Brown", "Brown", "Deep Brown", "Deep"};
const char* const EyeNames[] = {"Brown", "Dark Brown", "Hazel", "Green", "Blue", "Grey"};
const char* const BrowNames[] = {"Thin", "Natural", "Thick", "Arched"};
const char* const HairNames[] = {"Shaved", "Buzz Cut", "Crew Cut", "Textured Crop", "Side Part", "Undercut", "Curls", "Afro", "Shoulder Length", "Ponytail", "Braids", "Bob"};
const char* const HairColorNames[] = {"Black", "Dark Brown", "Brown", "Auburn", "Copper", "Blonde", "Platinum", "Grey"};
const char* const FacialHairNames[] = {"None", "Stubble", "Mustache", "Goatee", "Short Beard", "Full Beard"};
const char* const BuildNames[] = {"Slim", "Average", "Athletic", "Heavy"};
const char* const OutfitNames[] = {"Hoodie", "Bomber", "Flannel", "Denim Jacket", "Leather Jacket", "Track Jacket"};
const char* const OutfitColorNames[] = {"Charcoal", "Navy", "Olive", "Burgundy", "Sand", "Black", "Teal", "Mustard"};
const char* const GlassesNames[] = {"None", "Round", "Square", "Wire Frames", "Shades"};
const char* const HatNames[] = {"None", "Beanie", "Ball Cap", "Cap, Backwards", "Bucket Hat"};

template <size_t N>
constexpr int CountOf(const char* const (&)[N])
{
	return static_cast<int>(N);
}

const uint32_t Skins[] = {0xf6dccb, 0xeac0a2, 0xdfab87, 0xd29b74, 0xc08a5b, 0xa9734b, 0x8d5b38, 0x70452a, 0x55331f, 0x3d2416};
const uint32_t Hairs[] = {0x161312, 0x3a2519, 0x5f3d26, 0x7a3522, 0xa4532b, 0xc9a368, 0xe2d6bd, 0x9c9a97};
const uint32_t Eyes[] = {0x6b4226, 0x3b2416, 0x8a6a3a, 0x4f7a4a, 0x4f7fae, 0x7c8a94};
const uint32_t Outfits[] = {0x34383f, 0x1f2d4d, 0x4b5638, 0x6a1f2e, 0xbfa883, 0x15161a, 0x1f6f6a, 0xc79a2e};

bool IsLetterByte(unsigned char Ch)
{
	return (Ch >= 'a' && Ch <= 'z') || (Ch >= 'A' && Ch <= 'Z') || Ch >= 0x80;
}
} // namespace hero_detail

// ------------------------------------------------------------------ backgrounds

const BackgroundInfo& InfoOf(Background B)
{
	const int I = std::clamp(static_cast<int>(B), 0, BackgroundCount - 1);
	return hero_detail::Backgrounds[I];
}

const BackgroundInfo* FindBackground(const std::string& Key)
{
	for (const BackgroundInfo& Bi : hero_detail::Backgrounds)
	{
		if (Key == Bi.Key)
		{
			return &Bi;
		}
	}
	return nullptr;
}

Perks PerksOf(Background B)
{
	Perks P;
	switch (B)
	{
	case Background::Kitchen:
		P.JobPay = 1.2;
		P.MealBoost = 1.25;
		break;
	case Background::DealersKid: P.ReadWeight = 2; break;
	case Background::Dropout: P.StartUnlocks.push_back("bounty"); break;
	case Background::Bouncer: P.TiltGain = 0.7; break;
	case Background::Hustler:
		P.HustleRisk = -0.05;
		P.HeatCool = 1.5;
		break;
	case Background::Newcomer: P.StartCents = 4000; break;
	default: break;
	}
	return P;
}

Perks PerksOf(const Character& C)
{
	return C.Created ? PerksOf(C.Story) : Perks();
}

// ------------------------------------------------------------------ countries

const std::vector<CountryInfo>& Countries()
{
	static const std::vector<CountryInfo> L = {
		{"AR", "Argentina", "Rosario", "Argentine"},
		{"AU", "Australia", "Melbourne", "Australian"},
		{"AT", "Austria", "Graz", "Austrian"},
		{"BE", "Belgium", "Antwerp", "Belgian"},
		{"BR", "Brazil", "S\xC3\xA3o Paulo", "Brazilian"},
		{"CA", "Canada", "Hamilton", "Canadian"},
		{"CN", "China", "Chengdu", "Chinese"},
		{"CZ", "Czechia", "Brno", "Czech"},
		{"DK", "Denmark", "Aarhus", "Danish"},
		{"FI", "Finland", "Tampere", "Finnish"},
		{"FR", "France", "Lyon", "French"},
		{"DE", "Germany", "Hamburg", "German"},
		{"IN", "India", "Pune", "Indian"},
		{"IE", "Ireland", "Cork", "Irish"},
		{"IT", "Italy", "Naples", "Italian"},
		{"JP", "Japan", "Osaka", "Japanese"},
		{"MX", "Mexico", "Guadalajara", "Mexican"},
		{"NL", "Netherlands", "Rotterdam", "Dutch"},
		{"NO", "Norway", "Bergen", "Norwegian"},
		{"PL", "Poland", "Krak\xC3\xB3w", "Polish"},
		{"PT", "Portugal", "Porto", "Portuguese"},
		{"RU", "Russia", "Kazan", "Russian"},
		{"KR", "South Korea", "Busan", "Korean"},
		{"ES", "Spain", "Valencia", "Spanish"},
		{"SE", "Sweden", "Gothenburg", "Swedish"},
		{"UA", "Ukraine", "Kharkiv", "Ukrainian"},
		{"GB", "United Kingdom", "Manchester", "British"},
		{"US", "United States", "Cleveland", "American"},
	};
	return L;
}

const CountryInfo* FindCountry(const std::string& Code)
{
	for (const CountryInfo& Ci : Countries())
	{
		if (Code == Ci.Code)
		{
			return &Ci;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------ look

int Look::Get(Slot S) const
{
	switch (S)
	{
	case Slot::Body: return Body;
	case Slot::Face: return Face;
	case Slot::Skin: return Skin;
	case Slot::Eyes: return Eyes;
	case Slot::Brows: return Brows;
	case Slot::Hair: return Hair;
	case Slot::HairColor: return HairColor;
	case Slot::FacialHair: return FacialHair;
	case Slot::Build: return Build;
	case Slot::Height: return Height;
	case Slot::Outfit: return Outfit;
	case Slot::OutfitColor: return OutfitColor;
	case Slot::Glasses: return Glasses;
	case Slot::Hat: return Hat;
	default: return 0;
	}
}

void Look::Set(Slot S, int Value, bool Wrap)
{
	int V = Value;
	if (S == Slot::Height)
	{
		V = std::clamp(V, MinHeight, MaxHeight);
	}
	else
	{
		const int N = OptionCount(S);
		V = Wrap ? ((V % N) + N) % N : std::clamp(V, 0, N - 1);
	}
	switch (S)
	{
	case Slot::Body: Body = V; break;
	case Slot::Face: Face = V; break;
	case Slot::Skin: Skin = V; break;
	case Slot::Eyes: Eyes = V; break;
	case Slot::Brows: Brows = V; break;
	case Slot::Hair: Hair = V; break;
	case Slot::HairColor: HairColor = V; break;
	case Slot::FacialHair: FacialHair = V; break;
	case Slot::Build: Build = V; break;
	case Slot::Height: Height = V; break;
	case Slot::Outfit: Outfit = V; break;
	case Slot::OutfitColor: OutfitColor = V; break;
	case Slot::Glasses: Glasses = V; break;
	case Slot::Hat: Hat = V; break;
	default: break;
	}
}

bool Look::operator==(const Look& O) const
{
	for (int I = 0; I < SlotCount; ++I)
	{
		if (Get(static_cast<Slot>(I)) != O.Get(static_cast<Slot>(I)))
		{
			return false;
		}
	}
	return true;
}

const char* SlotName(Slot S)
{
	const int I = static_cast<int>(S);
	return I >= 0 && I < SlotCount ? hero_detail::SlotNames[I] : "";
}

int OptionCount(Slot S)
{
	using namespace hero_detail;
	switch (S)
	{
	case Slot::Body: return CountOf(BodyNames);
	case Slot::Face: return CountOf(FaceNames);
	case Slot::Skin: return CountOf(SkinNames);
	case Slot::Eyes: return CountOf(EyeNames);
	case Slot::Brows: return CountOf(BrowNames);
	case Slot::Hair: return CountOf(HairNames);
	case Slot::HairColor: return CountOf(HairColorNames);
	case Slot::FacialHair: return CountOf(FacialHairNames);
	case Slot::Build: return CountOf(BuildNames);
	case Slot::Height: return MaxHeight - MinHeight + 1;
	case Slot::Outfit: return CountOf(OutfitNames);
	case Slot::OutfitColor: return CountOf(OutfitColorNames);
	case Slot::Glasses: return CountOf(GlassesNames);
	case Slot::Hat: return CountOf(HatNames);
	default: return 1;
	}
}

std::string OptionName(Slot S, int Value)
{
	using namespace hero_detail;
	if (S == Slot::Height)
	{
		const int Cm = std::clamp(Value, MinHeight, MaxHeight);
		const int Inches = static_cast<int>(static_cast<double>(Cm) / 2.54 + 0.5);
		return std::to_string(Inches / 12) + "'" + std::to_string(Inches % 12) + "\" \xC2\xB7 " + std::to_string(Cm) + " cm";
	}
	const int V = std::clamp(Value, 0, OptionCount(S) - 1);
	const size_t I = static_cast<size_t>(V);
	switch (S)
	{
	case Slot::Body: return BodyNames[I];
	case Slot::Face: return FaceNames[I];
	case Slot::Skin: return SkinNames[I];
	case Slot::Eyes: return EyeNames[I];
	case Slot::Brows: return BrowNames[I];
	case Slot::Hair: return HairNames[I];
	case Slot::HairColor: return HairColorNames[I];
	case Slot::FacialHair: return FacialHairNames[I];
	case Slot::Build: return BuildNames[I];
	case Slot::Outfit: return OutfitNames[I];
	case Slot::OutfitColor: return OutfitColorNames[I];
	case Slot::Glasses: return GlassesNames[I];
	case Slot::Hat: return HatNames[I];
	default: return "";
	}
}

int SlotGroup(Slot S)
{
	switch (S)
	{
	case Slot::Body:
	case Slot::Face:
	case Slot::Skin:
	case Slot::Eyes:
	case Slot::Brows: return 0;
	case Slot::Hair:
	case Slot::HairColor:
	case Slot::FacialHair: return 1;
	case Slot::Build:
	case Slot::Height: return 2;
	default: return 3;
	}
}

uint32_t SkinTone(int Index)
{
	return hero_detail::Skins[std::clamp(Index, 0, 9)];
}

uint32_t HairTone(int Index)
{
	return hero_detail::Hairs[std::clamp(Index, 0, 7)];
}

uint32_t EyeTone(int Index)
{
	return hero_detail::Eyes[std::clamp(Index, 0, 5)];
}

uint32_t OutfitTone(int Index)
{
	return hero_detail::Outfits[std::clamp(Index, 0, 7)];
}

uint32_t HairToneAt(const Look& L, int Age)
{
	const uint32_t Base = HairTone(L.HairColor);
	// Platinum and grey are already there; everyone else greys from 44, at most two thirds of the way by 64.
	const double Grey = L.HairColor >= 6 ? 0.0 : std::clamp((static_cast<double>(Age) - 44.0) / 30.0, 0.0, 0.65);
	if (Grey <= 0.0)
	{
		return Base;
	}
	const double R = static_cast<double>((Base >> 16) & 0xffu);
	const double G = static_cast<double>((Base >> 8) & 0xffu);
	const double B = static_cast<double>(Base & 0xffu);
	// The color drains first (a dark brown at 57 reads grey, not beige), then the silver comes in.
	const double Lum = 0.299 * R + 0.587 * G + 0.114 * B;
	const double Drain = std::min(1.0, Grey * 1.6);
	const double Silver = Grey * 0.75;
	auto Channel = [&](double V, double Ash, double Shine) {
		const double Drained = V + (Ash - V) * Drain;
		return static_cast<uint32_t>(std::clamp(Drained + (Shine - Drained) * Silver + 0.5, 0.0, 255.0));
	};
	return (Channel(R, Lum, 169.0) << 16) | (Channel(G, Lum, 171.0) << 8) | Channel(B, Lum * 1.02, 174.0);
}

uint32_t HatTone(const Look& L, int Age)
{
	// The jacket picks a partner (navy with sand, charcoal with mustard). When that would vanish into the hair under
	// it (a black cap on black hair, a sand cap on greying brown), the next tone round the palette that doesn't, and
	// never the jacket's own.
	static const int Partner[8] = {7, 4, 5, 0, 1, 3, 0, 5};
	const int Jacket = std::clamp(L.OutfitColor, 0, 7);
	const uint32_t Hair = HairToneAt(L, Age);
	auto Apart = [Hair](uint32_t Tone) {
		int Sum = 0;
		for (int Shift = 0; Shift <= 16; Shift += 8)
		{
			Sum += std::abs(static_cast<int>((Tone >> Shift) & 0xffu) - static_cast<int>((Hair >> Shift) & 0xffu));
		}
		return Sum >= 80; // of 765
	};
	for (int K = 0; K < 8; ++K)
	{
		// Steps of three visit all eight tones.
		const int I = (Partner[Jacket] + K * 3) % 8;
		if (I != Jacket && Apart(OutfitTone(I)))
		{
			return OutfitTone(I);
		}
	}
	return OutfitTone(Partner[Jacket]);
}

// ------------------------------------------------------------------ character

int NameLength(const std::string& Part)
{
	int N = 0;
	for (const char Ch : Part)
	{
		// UTF-8: every byte but a continuation starts a letter.
		N += (static_cast<unsigned char>(Ch) & 0xC0) != 0x80 ? 1 : 0;
	}
	return N;
}

bool NameValid(const std::string& Part)
{
	// Sixteen letters, accented or not (two bytes each at most in the creator's range).
	if (Part.empty() || NameLength(Part) > MaxNameLength || Part.size() > 2 * MaxNameLength || !hero_detail::IsLetterByte(static_cast<unsigned char>(Part[0])))
	{
		return false;
	}
	for (const char Ch : Part)
	{
		const unsigned char U = static_cast<unsigned char>(Ch);
		if (!hero_detail::IsLetterByte(U) && Ch != ' ' && Ch != '\'' && Ch != '-')
		{
			return false;
		}
	}
	return Part.back() != ' ';
}

std::string Character::FullName() const
{
	return LastName.empty() ? FirstName : FirstName + " " + LastName;
}

std::string Character::Problem() const
{
	if (!NameValid(FirstName))
	{
		return FirstName.empty() ? "Give them a first name." : "First names use letters, spaces, apostrophes and hyphens.";
	}
	if (!LastName.empty() && !NameValid(LastName))
	{
		return "Last names use letters, spaces, apostrophes and hyphens.";
	}
	if (Age < MinAge || Age > MaxAge)
	{
		return "Age is " + std::to_string(MinAge) + " to " + std::to_string(MaxAge) + ".";
	}
	if (!FindCountry(Country))
	{
		return "Pick where they're from.";
	}
	return "";
}

std::string Character::Serialize() const
{
	std::ostringstream Out;
	Out << "hero\tname\t" << FirstName << "\t" << LastName << "\n";
	Out << "hero\tage\t" << Age << "\n";
	Out << "hero\tcountry\t" << Country << "\n";
	Out << "hero\tbackground\t" << InfoOf(Story).Key << "\n";
	Out << "hero\tlook";
	for (int I = 0; I < SlotCount; ++I)
	{
		Out << "\t" << hero_detail::SlotKeys[I] << "\t" << Appearance.Get(static_cast<Slot>(I));
	}
	Out << "\n";
	Out << "hero\tcreated\t" << (Created ? 1 : 0) << "\n";
	return Out.str();
}

bool Character::Read(const std::vector<std::string>& F)
{
	if (F.size() < 3 || F[0] != "hero")
	{
		return false;
	}
	const std::string& Key = F[1];
	if (Key == "name")
	{
		if (NameValid(F[2]))
		{
			FirstName = F[2];
		}
		LastName = F.size() >= 4 && NameValid(F[3]) ? F[3] : std::string();
	}
	else if (Key == "age")
	{
		Age = std::clamp(std::atoi(F[2].c_str()), MinAge, MaxAge);
	}
	else if (Key == "country")
	{
		if (FindCountry(F[2]))
		{
			Country = F[2];
		}
	}
	else if (Key == "background")
	{
		if (const BackgroundInfo* Bi = FindBackground(F[2]))
		{
			Story = Bi->Id;
		}
	}
	else if (Key == "look")
	{
		// Key and value pairs, so a slot added later reads its default from an older save.
		for (size_t K = 2; K + 1 < F.size(); K += 2)
		{
			for (int I = 0; I < SlotCount; ++I)
			{
				if (F[K] == hero_detail::SlotKeys[I])
				{
					Appearance.Set(static_cast<Slot>(I), std::atoi(F[K + 1].c_str()));
				}
			}
		}
	}
	else if (Key == "created")
	{
		Created = F[2] == "1";
	}
	return true;
}

Character Random(Rng& R)
{
	Character C;
	const std::vector<CountryInfo>& All = Countries();
	C.Country = All[static_cast<size_t>(R.Int(static_cast<int>(All.size())))].Code;
	int Body = 0;
	for (const hero_detail::NamePool& P : hero_detail::Pools)
	{
		if (C.Country == P.Code)
		{
			const std::vector<std::string> Firsts = hero_detail::SplitBars(P.First);
			const std::vector<std::string> Lasts = hero_detail::SplitBars(P.Last);
			const int Fi = R.Int(static_cast<int>(Firsts.size()));
			C.FirstName = Firsts[static_cast<size_t>(Fi)];
			C.LastName = Lasts[static_cast<size_t>(R.Int(static_cast<int>(Lasts.size())))];
			// Each pool's first names run A, A, B, B, A, B by body type (a nudge for the dice, never a rule).
			Body = Fi == 2 || Fi == 3 || Fi == 5 ? 1 : 0;
		}
	}
	// Mostly twenties and thirties, the occasional older grinder.
	C.Age = R.Next() < 0.8 ? 19 + R.Int(18) : 37 + R.Int(MaxAge - 37 + 1);
	C.Story = static_cast<Background>(R.Int(BackgroundCount));
	C.Appearance = RandomLook(R, C.Age, Body);
	C.Created = true;
	return C;
}

Look RandomLook(Rng& R, int Age, int Body)
{
	Look L;
	L.Body = std::clamp(Body, 0, OptionCount(Slot::Body) - 1);
	L.Face = R.Int(OptionCount(Slot::Face));
	L.Skin = R.Int(OptionCount(Slot::Skin));
	L.Eyes = L.Skin >= 6 ? R.Int(2) : R.Int(OptionCount(Slot::Eyes));
	L.Brows = R.Int(OptionCount(Slot::Brows));
	L.Hair = R.Int(OptionCount(Slot::Hair));
	L.HairColor = Age >= 50 && R.Next() < 0.6 ? 7 : (L.Skin >= 5 ? R.Int(3) : R.Int(7));
	L.FacialHair = L.Body == 0 ? R.Int(OptionCount(Slot::FacialHair)) : 0;
	L.Build = R.Int(OptionCount(Slot::Build));
	L.Height = L.Body == 0 ? 168 + R.Int(25) : 157 + R.Int(22);
	L.Outfit = R.Int(OptionCount(Slot::Outfit));
	L.OutfitColor = R.Int(OptionCount(Slot::OutfitColor));
	L.Glasses = R.Next() < 0.3 ? 1 + R.Int(OptionCount(Slot::Glasses) - 1) : 0;
	L.Hat = R.Next() < 0.35 ? 1 + R.Int(OptionCount(Slot::Hat) - 1) : 0;
	return L;
}

std::vector<std::string> NameSuggestions(const std::string& Country, bool Last)
{
	const hero_detail::NamePool* Pick = nullptr;
	for (const hero_detail::NamePool& P : hero_detail::Pools)
	{
		// Unknown codes fall back to the last pool (the United States), as a new career's default does.
		Pick = &P;
		if (Country == P.Code)
		{
			break;
		}
	}
	return Pick ? hero_detail::SplitBars(Last ? Pick->Last : Pick->First) : std::vector<std::string>();
}

std::string Bio(const Character& C)
{
	const CountryInfo* Ci = FindCountry(C.Country);
	const std::string Home = Ci ? std::string(Ci->City) + ", " + Ci->Name : std::string("somewhere else");
	std::string Out = C.FullName() + ", " + std::to_string(C.Age) + ". From " + Home + ". ";
	switch (C.Story)
	{
	case Background::Kitchen: Out += "Cooked on the line until the hours stopped adding up. "; break;
	case Background::DealersKid: Out += "Raised in the Embercrest's break room by a graveyard-shift dealer. "; break;
	case Background::Dropout: Out += "Left a statistics degree for the online games. "; break;
	case Background::Bouncer: Out += "Worked the door at the Embercrest until the night staff was cut. "; break;
	case Background::Hustler: Out += "Grew up on the same block as Marcus and never got picked up. "; break;
	default: Out += "Came to the city with one suitcase and a little saved. "; break;
	}
	// What the account holds on night one: $2.37, and any savings go straight into it (Session::NewCareer).
	const Chips Saved = PerksOf(C.Story).StartCents;
	const std::string SavedText = Saved % 100 == 0 ? "$" + std::to_string(Saved / 100) : Money(Saved);
	Out += "Now: a rented room, a laptop, rent due Friday, and " + Money(237 + Saved) + " on RiverLine";
	Out += Saved > 0 ? " (" + SavedText + " of it saved from home)." : ".";
	return Out;
}
} // namespace hero
} // namespace ss
