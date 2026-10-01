#include "ShortStack/Game/Handles.h"
#include "../StrictFloat.h"

#include "ShortStack/Rng.h"

#include <cctype>

namespace ss
{
namespace handles
{
namespace handles_detail
{
struct Country
{
	const char* Code;
	double Weight;
	std::vector<const char*> First;
	std::vector<const char*> Last;
	std::vector<const char*> Native; // poker words, animals and local color, in the local language
};

const std::vector<Country>& Countries()
{
	static const std::vector<Country> C = {
		{"BR", 12, {"Lucas", "Gabriel", "Rafael", "Thiago", "Bruno", "Felipe", "Mateus", "Joao", "Ana", "Julia", "Camila", "Bia"},
			{"Silva", "Santos", "Oliveira", "Souza", "Costa", "Ferreira", "Almeida", "Rocha"}, {"Blefe", "ReiDoRio", "Tubarao", "Peixe", "Sorte", "Ficha", "Coringa", "Jacare"}},
		{"DE", 7, {"Lukas", "Jonas", "Felix", "Max", "Leon", "Paul", "Tim", "Niklas", "Lena", "Anna", "Sophie", "Jule"},
			{"Mueller", "Schmidt", "Fischer", "Weber", "Wagner", "Becker", "Hoffmann", "Koch"}, {"Kartenhai", "Bluffkoenig", "Glueckspilz", "Zocker", "Nachtfalke", "Flussratte", "Kartenhexe", "Pokerface"}},
		{"CA", 6, {"Liam", "Noah", "Ethan", "Owen", "Logan", "Emma", "Chloe", "Maya", "Jack", "Ryan"},
			{"Tremblay", "Roy", "Gagnon", "Martin", "Leblanc", "Campbell", "Morin", "Bouchard"}, {"Maple", "Moose", "Loonie", "Toque", "Canuck", "Poutine", "Timbit", "Hoser"}},
		{"GB", 7, {"Oliver", "Harry", "Jack", "George", "Charlie", "Alfie", "Freddie", "Olivia", "Amelia", "Isla", "Tom", "Ollie"},
			{"Smith", "Jones", "Taylor", "Brown", "Walker", "Wright", "Hughes", "Clarke"}, {"Cheeky", "Proper", "Skint", "Brolly", "Gaffer", "Punter", "Quid", "Cuppa"}},
		{"RU", 6, {"Dmitri", "Ivan", "Sergei", "Alexei", "Nikita", "Pavel", "Artem", "Olga", "Irina", "Katya", "Maxim", "Yuri"},
			{"Volkov", "Ivanov", "Smirnov", "Petrov", "Sokolov", "Morozov", "Orlov", "Kuznetsov"}, {"Medved", "Akula", "Volk", "Udacha", "Tigr", "Zver", "Bystro", "Karty"}},
		{"UA", 4, {"Taras", "Andriy", "Oleksii", "Bohdan", "Yaroslav", "Oksana", "Sofiia", "Iryna", "Mykola", "Dmytro"},
			{"Shevchenko", "Kovalenko", "Bondarenko", "Tkachenko", "Kravchenko", "Melnyk", "Boyko", "Lysenko"}, {"Kozak", "Sokil", "Vovk", "Dnipro", "Borshch", "Hetman"}},
		{"US", 5, {"Mike", "Chris", "Tyler", "Jake", "Kevin", "Brandon", "Josh", "Ashley", "Megan", "Kayla", "Derek", "Cody"},
			{"Johnson", "Miller", "Davis", "Garcia", "Wilson", "Anderson", "Moore", "Jackson"}, {"Vegas", "Tex", "Bayou", "Brooklyn", "Rodeo", "Jersey", "Gator", "Dixie"}},
		{"NL", 3, {"Daan", "Sem", "Lucas", "Milan", "Thijs", "Bram", "Femke", "Lotte", "Sanne", "Jesse"},
			{"deJong", "Jansen", "deVries", "vdBerg", "Bakker", "Visser", "Smit", "Meijer"}, {"KaasHaai", "Stroopwafel", "Tulp", "Molen", "Fietser", "Gezellig", "Oranje", "Klompen"}},
		{"FR", 4, {"Julien", "Hugo", "Louis", "Lucas", "Theo", "Nathan", "Camille", "Chloe", "Manon", "Leo", "Antoine", "Baptiste"},
			{"Martin", "Bernard", "Dubois", "Moreau", "Laurent", "Lefevre", "Roux", "Girard"}, {"LeRequin", "Bluffeur", "LaChance", "Tapis", "Flambeur", "Croissant", "Jeton", "LeRenard"}},
		{"IT", 3, {"Luca", "Marco", "Matteo", "Alessandro", "Lorenzo", "Giulia", "Chiara", "Francesca", "Davide", "Andrea"},
			{"Rossi", "Russo", "Ferrari", "Esposito", "Bianchi", "Romano", "Colombo", "Ricci"}, {"LoSqualo", "Fortuna", "Asso", "Bluffatore", "Gnocchi", "Vesuvio", "Lupo", "Pizzaiolo"}},
		{"ES", 3, {"Pablo", "Javi", "Alejandro", "Sergio", "Carlos", "Lucia", "Marta", "Paula", "Alvaro", "Dani"},
			{"Garcia", "Fernandez", "Lopez", "Martinez", "Sanchez", "Romero", "Navarro", "Torres"}, {"ElTiburon", "Farol", "Suerte", "Fichas", "Toro", "Tapas", "Siesta", "Cartas"}},
		{"SE", 3, {"Erik", "Oscar", "William", "Hugo", "Lucas", "Linnea", "Elsa", "Maja", "Axel", "Viktor"},
			{"Johansson", "Andersson", "Karlsson", "Nilsson", "Eriksson", "Larsson", "Olsson", "Lindberg"}, {"Hajen", "Vargen", "Isbjorn", "Fika", "Norrsken", "Lycka", "Kortspel"}},
		{"FI", 2, {"Juho", "Aleksi", "Eetu", "Ville", "Mikko", "Aino", "Emma", "Veera", "Lauri", "Joni"},
			{"Virtanen", "Korhonen", "Nieminen", "Makinen", "Laine", "Heikkinen", "Koskinen", "Salonen"}, {"Sisu", "Sauna", "Susi", "Kettu", "Revontuli", "Karhu", "Pakkanen"}},
		{"PL", 3, {"Kuba", "Piotr", "Michal", "Kacper", "Tomek", "Ola", "Zosia", "Kasia", "Bartek", "Wojtek"},
			{"Nowak", "Kowalski", "Wisniewski", "Wojcik", "Kaminski", "Lewandowski", "Zielinski", "Szymanski"}, {"Rekin", "Blef", "Pierogi", "Zubr", "Sokol", "Wilk", "Szczescie"}},
		{"MX", 3, {"Diego", "Chuy", "Luis", "Carlos", "Mateo", "Sofia", "Valeria", "Ximena", "Emiliano", "Pepe"},
			{"Hernandez", "Gonzalez", "Rodriguez", "Ramirez", "Flores", "Cruz", "Morales", "Reyes"}, {"ElPescado", "Tiburon", "Taquero", "Lucha", "Charro", "Calavera", "Jaguar", "Picante"}},
		{"AR", 3, {"Nico", "Santi", "Mateo", "Tomas", "Facu", "Agus", "Valen", "Juli", "Lauti", "Mica"},
			{"Gonzalez", "Rodriguez", "Gomez", "Fernandez", "Diaz", "Alvarez", "Romero", "Benitez"}, {"Che", "Mate", "Tango", "ElPibe", "Gaucho", "Asado", "Pampa", "Mufa"}},
		{"CN", 5, {"Wei", "Jun", "Hao", "Lei", "Ming", "Yan", "Li", "Xin", "Chen", "Bo"},
			{"Wang", "Zhang", "Liu", "Chen", "Yang", "Huang", "Zhao", "Wu"}, {"LongWei", "LaoHu", "JinLong", "Lucky8", "ShaYu", "Mahjong", "Fortune88", "XiongMao"}},
		{"JP", 3, {"Kenji", "Haru", "Ren", "Sota", "Yuto", "Yui", "Hina", "Aoi", "Daiki", "Kaito"},
			{"Sato", "Suzuki", "Takahashi", "Tanaka", "Watanabe", "Ito", "Yamamoto", "Nakamura"}, {"Kaiju", "Samurai", "Ronin", "Neko", "Sakura", "Kitsune", "Tengu", "Shogun"}},
		{"KR", 2, {"Minho", "Jisoo", "Hyun", "Joon", "Seo", "Jiwoo", "Minji", "Yuna", "Dong", "Taeyang"},
			{"Kim", "Lee", "Park", "Choi", "Jung", "Kang", "Cho", "Yoon"}, {"Horangi", "Kimchi", "Daebak", "Jjang", "Hwarang", "Bibimbap"}},
		{"AT", 2, {"Lukas", "Florian", "Tobias", "David", "Elias", "Lena", "Hannah", "Leonie", "Fabian", "Jakob"},
			{"Gruber", "Huber", "Bauer", "Wagner", "Pichler", "Steiner", "Moser", "Mayer"}, {"Schmaeh", "Alpen", "Strudel", "Gams", "Donau", "Kaiserschmarrn"}},
		{"BE", 2, {"Lucas", "Arthur", "Louis", "Noah", "Jules", "Emma", "Olivia", "Lina", "Lars", "Wout"},
			{"Peeters", "Janssens", "Maes", "Jacobs", "Mertens", "Willems", "Claes", "Goossens"}, {"Frietkot", "Praline", "Gaufre", "Atomium", "Frites", "Manneken"}},
		{"PT", 2, {"Joao", "Rodrigo", "Martim", "Tiago", "Diogo", "Ines", "Beatriz", "Mariana", "Tomas", "Duarte"},
			{"Silva", "Santos", "Ferreira", "Pereira", "Costa", "Oliveira", "Martins", "Sousa"}, {"Bacalhau", "Tubarao", "Saudade", "Fado", "Galo", "Pastel"}},
		{"CZ", 2, {"Jakub", "Jan", "Tomas", "Adam", "Ondra", "Tereza", "Eliska", "Anna", "Matej", "Vojta"},
			{"Novak", "Svoboda", "Novotny", "Dvorak", "Cerny", "Prochazka", "Kucera", "Vesely"}, {"Krtek", "Knedlik", "Zralok", "Stesti", "Vlk", "Medvidek"}},
		{"IE", 2, {"Sean", "Conor", "Cian", "Oisin", "Darragh", "Aoife", "Niamh", "Ciara", "Padraig", "Liam"},
			{"Murphy", "Kelly", "OSullivan", "Walsh", "Byrne", "Ryan", "OBrien", "Doyle"}, {"Craic", "Shamrock", "Banshee", "Clover", "Gaff", "Celtic"}},
		{"NO", 2, {"Jakob", "Emil", "Noah", "Filip", "Aksel", "Nora", "Emma", "Ingrid", "Sondre", "Magnus"},
			{"Hansen", "Johansen", "Olsen", "Larsen", "Andersen", "Pedersen", "Nilsen", "Berg"}, {"Fjord", "Troll", "Ulv", "Bjorn", "Nordlys", "Isbre"}},
		{"DK", 1, {"William", "Oscar", "Noah", "Lucas", "Victor", "Freja", "Ida", "Clara", "Mads", "Rasmus"},
			{"Nielsen", "Jensen", "Hansen", "Pedersen", "Andersen", "Christensen", "Larsen", "Sorensen"}, {"Hygge", "Kongen", "Daner", "Isbjorn", "Smorrebrod"}},
		{"AU", 3, {"Jack", "Oliver", "William", "Noah", "Lachie", "Charlotte", "Mia", "Ruby", "Kev", "Bazza"},
			{"Smith", "Jones", "Williams", "Brown", "Wilson", "Taylor", "Martin", "White"}, {"Roo", "Dingo", "Wombat", "Koala", "Croc", "Fairdinkum", "Outback", "Stralya"}},
		{"IN", 3, {"Arjun", "Rohan", "Aarav", "Vikram", "Rahul", "Priya", "Ananya", "Diya", "Kabir", "Ishaan"},
			{"Sharma", "Patel", "Singh", "Kumar", "Gupta", "Mehta", "Reddy", "Iyer"}, {"TeenPatti", "Chai", "Bhai", "Masala", "Sher", "Desi", "Jugaad"}},
	};
	return C;
}

const std::vector<const char*>& Adjectives()
{
	static const std::vector<const char*> V = {"Lucky", "Silent", "Cold", "Royal", "Lazy", "Midnight", "Sneaky", "Iron", "Velvet", "Golden", "Wild", "Quiet", "Frosty", "Sly",
		"Rusty", "Stone", "Neon", "Polar", "Salty", "Sleepy", "Steady", "Shady", "Crispy", "Mellow", "Rogue", "Cosmic", "Electric", "Hollow", "Ruthless", "Patient", "Reckless",
		"Smooth", "Dusty", "Lunar", "Crimson", "Copper", "Thirsty", "Humble", "Stoic", "Restless"};
	return V;
}

const std::vector<const char*>& Nouns()
{
	static const std::vector<const char*> V = {"River", "Ace", "Kicker", "Nuts", "Flush", "Shark", "Fish", "Whale", "Rounder", "Grinder", "Bluff", "Turn", "Flop", "Button",
		"Blinds", "Stack", "Chip", "Felt", "Deuce", "Cowboy", "Queen", "Jack", "Joker", "Shove", "Rail", "Muck", "Dealer", "Pot", "Raise", "Bubble", "Snowman", "Rocket", "Hammer",
		"Cooler", "Monster", "Wheel", "Boat", "Quads", "Gutshot", "Owl"};
	return V;
}

/** Handles people really pick: hand names, jokes about the grind, late nights and bad beats. */
const std::vector<const char*>& Curated()
{
	static const std::vector<const char*> V = {"PocketRockets", "SnowmenSam", "QuadsOrBust", "OneMoreTable", "NoSleepTil6", "LandlordHatesMe", "BankrollRIP", "RamenBudget",
		"CoolerKing", "BubbleBoy", "MinCashMike", "FoldEquity", "SolverSays", "ICMhero", "DeepStackDan", "ShortStackSue", "AceRag", "SuitedAnything", "ThreeBetBob", "CheckRaiseCarl",
		"FloatTheFlop", "RiverRat", "SetMiner", "BigSlickRick", "HammerTime72", "SevenDeuceKing", "GutshotGary", "OvercardOscar", "DonkBetDonna", "LimpKing", "MuckYourHand",
		"CallStationCarl", "TankTillDawn", "TimeBankTina", "SunriseGrinder", "GraveyardShift", "CoffeeAndCards", "EnergyDrinkEd", "WifeThinksImAsleep", "DadAtWork", "MomSaidOneMore",
		"JustOneMoreHand", "FishyBusiness", "TheNitFather", "NittyGritty", "LuckBox", "RunGoodRandy", "SickHeater", "ColdDeckCasey", "BadBeatBarry", "TiltedTom", "SpewQueen",
		"VarianceVictim", "SmallSample", "PlusEV", "MinRaiseMaria", "AllInAnnie", "ShipItSteve", "GoodGameGreg", "NiceHandSir", "SoGoodSoFast", "WhyAlwaysRiver", "RiverMeAgain",
		"TurnAndBurn", "RagsToRiver", "CardDeadCarl", "BlindStealBen", "ButtonMasher", "SmallBlindSally", "BigBlindBill", "HijackHank", "CutoffCurt", "LateRegLarry", "ReEntryRuth",
		"MonsterUnderBed", "PocketQueens", "KingsOrBust", "AceKingNever", "Jacks4Life", "TensFullAgain", "BoatBuilder", "SteelWheel", "NutFlushDraw", "BackdoorBetty",
		"RunnerRunner", "OneOuter", "TwoPairTony", "TripsTracy", "StraightEdge", "FullHouseFred", "ChipAndAChair", "TheProfessor", "Mr_Fold", "MrRiver", "LadyLuck",
		"CountessOfCards", "DukeOfDeuces", "BaronBluff", "SultanOfSuited", "TheGrinder", "NightCrawler", "Insomniac", "3amShove", "MoonlightShover", "CoffeeOverdose",
		"TacoTuesday", "PizzaAndPoker", "SushiShover", "ChipsAndSalsa", "CatLadyCalls", "DogDadDeals", "SlowRollSam", "TableCaptain", "ChatBanned", "SilentRaiser", "TheQuietOne",
		"HUDwatcher", "NoteTaker", "StatsGeek", "FoldPreRepeat", "BluffCatcher", "ThinValueTim", "OverbetOllie", "MinBetMatt", "SqueezePlay", "ColdCall4bet", "PuntMaster",
		"HeroCallHarry", "SnapCallSandy", "ClickBackKing", "TheRailbird", "DealMeIn", "SitAndGoGo", "GhostTown"};
	return V;
}

std::string Lower(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		Ch = static_cast<char>(std::tolower(static_cast<unsigned char>(Ch)));
	}
	return Out;
}

template <typename T>
const T& Pick(const std::vector<T>& V, Rng& R)
{
	return V[static_cast<size_t>(R.Int(static_cast<int>(V.size())))];
}

const Country& Find(const std::string& Code)
{
	for (const Country& C : Countries())
	{
		if (Code == C.Code)
		{
			return C;
		}
	}
	return Countries().front();
}

std::string TwoDigits(Rng& R)
{
	// Birth years and lucky numbers, not random noise.
	static const int Favorites[] = {7, 8, 13, 21, 22, 23, 33, 44, 77, 88, 99, 101};
	if (R.Chance(0.55))
	{
		const int Year = 78 + R.Int(28);
		return Year >= 100 ? "0" + std::to_string(Year - 100) : std::to_string(Year);
	}
	return std::to_string(Favorites[R.Int(static_cast<int>(sizeof(Favorites) / sizeof(Favorites[0])))]);
}

/** "R1v3rR4t": light leetspeak (every vowel has a fair chance). */
std::string Leet(const std::string& S, Rng& R)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		const char L = static_cast<char>(std::tolower(static_cast<unsigned char>(Ch)));
		if (!R.Chance(0.6))
		{
			continue;
		}
		Ch = L == 'o' ? '0' : L == 'e' ? '3' : L == 'i' ? '1' : L == 'a' ? '4' : Ch;
	}
	return Out;
}

std::string Slang(Rng& R)
{
	std::string Base = std::string(Pick(Adjectives(), R)) + Pick(Nouns(), R);
	const double Style = R.Next();
	if (Style < 0.25)
	{
		const std::string A = Pick(Adjectives(), R);
		Base = Lower(A) + "_" + Lower(Pick(Nouns(), R));
	}
	if (R.Chance(0.22))
	{
		Base += R.Chance(0.5) ? TwoDigits(R) : std::to_string(1 + R.Int(9));
	}
	return Base;
}

std::string RealName(const Country& C, Rng& R)
{
	const std::string F = Pick(C.First, R);
	const std::string L = Pick(C.Last, R);
	switch (R.Int(7))
	{
	case 0: return Lower(F) + Lower(L.substr(0, 1));
	case 1: return Lower(F) + "_" + Lower(L);
	case 2: return F + L;
	case 3: return Lower(F) + TwoDigits(R);
	case 4: return Lower(F) + "." + Lower(L.substr(0, 1));
	case 5: return Lower(L) + TwoDigits(R);
	default: return F + "_" + Pick(C.Native, R);
	}
}

std::string Native(const Country& C, Rng& R)
{
	const std::string W = Pick(C.Native, R);
	switch (R.Int(4))
	{
	case 0: return W;
	case 1: return W + TwoDigits(R);
	case 2: return W + std::string(Pick(Nouns(), R));
	default: return std::string(Pick(Adjectives(), R)) + W;
	}
}

std::string Gamer(Rng& R)
{
	const std::string N = Pick(Nouns(), R);
	switch (R.Int(7))
	{
	case 0: return "xX" + N + "Xx";
	case 1: return Leet(std::string(Pick(Adjectives(), R)) + N, R);
	case 2: return "n00b" + Lower(N);
	case 3: return N + "TTV";
	case 4: return "AFK_at_BB";
	case 5: return "brb_tilt";
	default: return "GG_" + Lower(N);
	}
}

std::string ProName(const Country& C, Rng& R)
{
	const std::string F = Pick(C.First, R);
	const std::string L = Pick(C.Last, R);
	switch (R.Int(4))
	{
	case 0: return F.substr(0, 1) + L;
	case 1: return Lower(F.substr(0, 1) + L);
	case 2: return L + std::to_string(1 + R.Int(9));
	default: return Lower(F) + Lower(L);
	}
}
} // namespace handles_detail

using namespace handles_detail;

std::string PickCountry(Rng& R)
{
	double Total = 0.0;
	for (const Country& C : Countries())
	{
		Total += C.Weight;
	}
	double Roll = R.Next() * Total;
	for (const Country& C : Countries())
	{
		if ((Roll -= C.Weight) < 0.0)
		{
			return C.Code;
		}
	}
	return Countries().front().Code;
}

std::string Make(Rng& R, const std::string& Code, bool Pro)
{
	const Country& C = Find(Code);
	std::string Name;
	// Five to seventeen characters: long enough to be someone, short enough for a seat plate.
	for (int Try = 0; Try < 8 && (Name.size() < 5 || Name.size() > 17); ++Try)
	{
		const double Roll = R.Next();
		if (Pro)
		{
			Name = Roll < 0.5 ? ProName(C, R) : Roll < 0.8 ? RealName(C, R) : Roll < 0.9 ? Slang(R) : std::string(Pick(Curated(), R));
		}
		else
		{
			Name = Roll < 0.24 ? Slang(R)
				: Roll < 0.54 ? RealName(C, R)
				: Roll < 0.66 ? Native(C, R)
				: Roll < 0.84 ? std::string(Pick(Curated(), R))
				: Roll < 0.92 ? Gamer(R)
				: ProName(C, R);
		}
	}
	return Name;
}

std::vector<Identity> Field(int Count, Rng& R, const std::set<std::string>& Taken)
{
	std::vector<Identity> Out;
	std::set<std::string> Used;
	for (const std::string& T : Taken)
	{
		Used.insert(Lower(T));
	}
	for (int I = 0; I < Count; ++I)
	{
		Identity Id;
		Id.Country = PickCountry(R);
		std::string Name = Make(R, Id.Country);
		// A taken handle gets a number, the way a sign-up form would ask for one.
		for (int Try = 0; Used.count(Lower(Name)) > 0; ++Try)
		{
			Name = Try < 6 ? Make(R, Id.Country) : Name + std::to_string(R.Int(10));
		}
		Used.insert(Lower(Name));
		Id.Name = Name;
		Out.push_back(Id);
	}
	return Out;
}
} // namespace handles
} // namespace ss
