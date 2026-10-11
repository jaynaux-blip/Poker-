#include "ShortStack/Audio/Voice.h"
#include "VoiceEngine.h"
#include "../StrictFloat.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// Voices for the card room (next_segment_spec §4.10). audio-synth.
//
// PlanSpeech reads a line the way a person would say it: its spelling gives the syllables, their vowels and consonants,
// numbers are spoken out, punctuation gives the pauses and the tune. The words themselves never come out: the engine
// speaks the plan as a murmur, and the captions carry the words. Everything is a pure function of its inputs.

namespace ss
{
namespace audio
{
namespace voice_detail
{
using vox::SylSpec;

bool IsLetter(char C)
{
	return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z');
}

bool IsDigit(char C)
{
	return C >= '0' && C <= '9';
}

char Lower(char C)
{
	return C >= 'A' && C <= 'Z' ? static_cast<char>(C - 'A' + 'a') : C;
}

bool VowelLetter(char C)
{
	return C == 'a' || C == 'e' || C == 'i' || C == 'o' || C == 'u';
}

// ------------------------------------------------------------------ numbers, spoken

void Under1000(int N, std::vector<std::string>& Out)
{
	static const char* Ones[20] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
		"eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
	static const char* Tens[10] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
	if (N >= 100)
	{
		Out.push_back(Ones[N / 100]);
		Out.push_back("hundred");
		N %= 100;
		if (N == 0)
		{
			return;
		}
	}
	if (N >= 20)
	{
		Out.push_back(Tens[N / 10]);
		if (N % 10 != 0)
		{
			Out.push_back(Ones[N % 10]);
		}
		return;
	}
	Out.push_back(Ones[N]);
}

void NumberWords(const std::string& Digits, std::vector<std::string>& Out)
{
	// Long strings (a phone number, a hand id) are read digit by digit; a leading zero is "oh" (seven-oh-six).
	if (Digits.empty())
	{
		return;
	}
	if (Digits.size() > 12 || (Digits.size() > 1 && Digits[0] == '0'))
	{
		for (const char C : Digits)
		{
			if (C == '0')
			{
				Out.push_back("oh");
			}
			else
			{
				Under1000(C - '0', Out);
			}
		}
		return;
	}
	unsigned long long N = 0;
	for (const char C : Digits)
	{
		N = N * 10ull + static_cast<unsigned long long>(C - '0');
	}
	if (N == 0)
	{
		Out.push_back("zero");
		return;
	}
	static const char* Scales[4] = {"", "thousand", "million", "billion"};
	int Groups[4] = {0, 0, 0, 0};
	for (int G = 0; G < 4; ++G)
	{
		Groups[G] = static_cast<int>(N % 1000ull);
		N /= 1000ull;
	}
	for (int G = 3; G >= 0; --G)
	{
		if (Groups[G] == 0)
		{
			continue;
		}
		Under1000(Groups[G], Out);
		if (G > 0)
		{
			Out.push_back(Scales[G]);
		}
	}
}

// ------------------------------------------------------------------ the line, split into words and phrases

struct Phrase
{
	std::vector<std::string> Words;
	vox::PhraseEnd End = vox::PhraseEnd::Stop;
};

void Tokenize(const std::string& Line, std::vector<Phrase>& Out)
{
	Out.clear();
	Out.emplace_back();
	auto Close = [&](vox::PhraseEnd End) {
		if (!Out.back().Words.empty())
		{
			Out.back().End = End;
			Out.emplace_back();
		}
		else if (Out.size() > 1)
		{
			// "?!" or "..." after a phrase already closed: the stronger mark wins.
			Phrase& Prev = Out[Out.size() - 2];
			if (End == vox::PhraseEnd::Question || End == vox::PhraseEnd::Ellipsis)
			{
				Prev.End = End;
			}
		}
	};
	const size_t N = Line.size();
	std::string Word;
	int Capitals = 0;
	auto Flush = [&]() {
		if (!Word.empty())
		{
			const bool Acronym = Capitals == static_cast<int>(Word.size()) && Word.size() >= 2 && Word.size() <= 4 && Word != "ok";
			Out.back().Words.push_back(Acronym ? "#" + Word : Word);
			Word.clear();
		}
		Capitals = 0;
	};
	for (size_t I = 0; I < N; ++I)
	{
		const char C = Line[I];
		const unsigned char U = static_cast<unsigned char>(C);
		if (IsLetter(C))
		{
			Capitals += (C >= 'A' && C <= 'Z') ? 1 : 0;
			Word.push_back(Lower(C));
			continue;
		}
		if (C == '\'' )
		{
			continue; // you're -> youre
		}
		if (U >= 0x80)
		{
			// UTF-8: a curly apostrophe stays inside the word; a dash or an ellipsis is a pause; anything else breaks it.
			if (U == 0xE2 && I + 2 < N && static_cast<unsigned char>(Line[I + 1]) == 0x80)
			{
				const unsigned char Third = static_cast<unsigned char>(Line[I + 2]);
				I += 2;
				if (Third == 0x99 || Third == 0x98)
				{
					continue;
				}
				Flush();
				if (Third == 0xA6)
				{
					Close(vox::PhraseEnd::Ellipsis);
				}
				else if (Third == 0x93 || Third == 0x94)
				{
					Close(vox::PhraseEnd::Comma);
				}
				continue;
			}
			Flush();
			continue;
		}
		if (IsDigit(C))
		{
			Flush();
			std::string Digits;
			std::string Fraction;
			size_t J = I;
			while (J < N && (IsDigit(Line[J]) || (Line[J] == ',' && J + 1 < N && IsDigit(Line[J + 1]) && !Digits.empty())))
			{
				if (IsDigit(Line[J]))
				{
					Digits.push_back(Line[J]);
				}
				++J;
			}
			if (J + 1 < N && Line[J] == '.' && IsDigit(Line[J + 1]))
			{
				++J;
				while (J < N && IsDigit(Line[J]))
				{
					Fraction.push_back(Line[J]);
					++J;
				}
			}
			std::vector<std::string> Spoken;
			// A clock's minutes: "7:06" is seven-oh-six, but "7:00" is just seven ("cards at seven P M").
			const bool Minutes = I >= 2 && Line[I - 1] == ':' && IsDigit(Line[I - 2]);
			if (!(Minutes && Digits.find_first_not_of('0') == std::string::npos))
			{
				NumberWords(Digits, Spoken);
			}
			if (!Fraction.empty())
			{
				Spoken.push_back("point");
				for (const char D : Fraction)
				{
					NumberWords(std::string(1, D), Spoken);
				}
			}
			if (J < N && Line[J] == '%')
			{
				Spoken.push_back("percent");
				++J;
			}
			for (const std::string& S : Spoken)
			{
				Out.back().Words.push_back(S);
			}
			I = J - 1;
			continue;
		}
		Flush();
		switch (C)
		{
		case ',':
		case ';':
			Close(vox::PhraseEnd::Comma);
			break;
		case ':':
			if (!(I > 0 && IsDigit(Line[I - 1]) && I + 1 < N && IsDigit(Line[I + 1])))
			{
				Close(vox::PhraseEnd::Comma);
			}
			break;
		case '.':
			if (I + 2 < N && Line[I + 1] == '.' && Line[I + 2] == '.')
			{
				Close(vox::PhraseEnd::Ellipsis);
				I += 2;
			}
			else
			{
				Close(vox::PhraseEnd::Stop);
			}
			break;
		case '!':
			Close(vox::PhraseEnd::Exclaim);
			break;
		case '?':
			Close(vox::PhraseEnd::Question);
			break;
		case '-':
			// A dash between spaces is a pause; a hyphen inside "seven-four" only breaks the word.
			if (I > 0 && I + 1 < N && Line[I - 1] == ' ' && Line[I + 1] == ' ')
			{
				Close(vox::PhraseEnd::Comma);
			}
			break;
		case '&':
			Out.back().Words.push_back("and");
			break;
		case '+':
			Out.back().Words.push_back("plus");
			break;
		default:
			break;
		}
	}
	Flush();
	if (Out.back().Words.empty())
	{
		Out.pop_back();
	}
	else
	{
		Out.back().End = vox::PhraseEnd::Stop;
	}
}

// ------------------------------------------------------------------ a word, split into syllables

struct Unit
{
	uint8_t Manner = vox::MNone;
	uint8_t Place = 1;
	int Letters = 1;
	bool Silent = false;
};

/** The consonant unit at W[I] (a digraph counts as one). */
Unit ConsonantAt(const std::string& W, size_t I)
{
	const char C = W[I];
	const char Nx = I + 1 < W.size() ? W[I + 1] : '\0';
	Unit U;
	auto Two = [&](uint8_t M, uint8_t P) {
		U.Manner = M;
		U.Place = P;
		U.Letters = 2;
		return U;
	};
	if (C == 't' && Nx == 'h')
	{
		return Two(vox::MWeakFricative, 1);
	}
	if (C == 's' && Nx == 'h')
	{
		return Two(vox::MShibilant, 2);
	}
	if (C == 'c' && Nx == 'h')
	{
		return Two(vox::MShibilant, 2);
	}
	if (C == 'p' && Nx == 'h')
	{
		return Two(vox::MWeakFricative, 0);
	}
	if (C == 'n' && Nx == 'g')
	{
		return Two(vox::MNasal, 3);
	}
	if (C == 'c' && Nx == 'k')
	{
		return Two(vox::MVoicelessStop, 3);
	}
	if (C == 'w' && Nx == 'h')
	{
		return Two(vox::MGlide, 0);
	}
	if (C == 'q' && Nx == 'u')
	{
		return Two(vox::MVoicelessStop, 3);
	}
	if (C == 'g' && Nx == 'h')
	{
		Unit S = Two(vox::MNone, 1);
		S.Silent = I > 0;
		if (I == 0)
		{
			S.Manner = vox::MVoicedStop;
			S.Place = 3;
		}
		return S;
	}
	const bool Soft = Nx == 'e' || Nx == 'i' || Nx == 'y';
	switch (C)
	{
	case 'b': U.Manner = vox::MVoicedStop; U.Place = 0; break;
	case 'c': U.Manner = static_cast<uint8_t>(Soft ? vox::MSibilant : vox::MVoicelessStop); U.Place = Soft ? 1 : 3; break;
	case 'd': U.Manner = vox::MVoicedStop; U.Place = 1; break;
	case 'f': U.Manner = vox::MWeakFricative; U.Place = 0; break;
	case 'g': U.Manner = static_cast<uint8_t>(Soft ? vox::MVoicedFricative : vox::MVoicedStop); U.Place = Soft ? 2 : 3; break;
	case 'h': U.Manner = vox::MAspirate; U.Place = 4; break;
	case 'j': U.Manner = vox::MVoicedFricative; U.Place = 2; break;
	case 'k': U.Manner = vox::MVoicelessStop; U.Place = 3; break;
	case 'l': U.Manner = vox::MLateral; U.Place = 1; break;
	case 'm': U.Manner = vox::MNasal; U.Place = 0; break;
	case 'n': U.Manner = vox::MNasal; U.Place = 1; break;
	case 'p': U.Manner = vox::MVoicelessStop; U.Place = 0; break;
	case 'q': U.Manner = vox::MVoicelessStop; U.Place = 3; break;
	case 'r': U.Manner = vox::MRhotic; U.Place = 2; break;
	case 's': U.Manner = vox::MSibilant; U.Place = 1; break;
	case 't': U.Manner = vox::MVoicelessStop; U.Place = 1; break;
	case 'v': U.Manner = vox::MVoicedFricative; U.Place = 0; break;
	case 'w': U.Manner = vox::MGlide; U.Place = 0; break;
	case 'x': U.Manner = vox::MVoicelessStop; U.Place = 3; break;
	case 'y': U.Manner = vox::MGlide; U.Place = 2; break;
	case 'z': U.Manner = vox::MVoicedFricative; U.Place = 1; break;
	default: U.Manner = vox::MNone; break;
	}
	return U;
}

bool FunctionWord(const std::string& W)
{
	static const char* List[] = {"the", "a", "an", "and", "or", "but", "of", "to", "in", "on", "at", "for", "from", "by", "with",
		"as", "is", "it", "its", "was", "are", "be", "been", "am", "i", "im", "ive", "you", "youre", "he", "hes", "she", "shes", "we",
		"they", "me", "him", "her", "us", "them", "my", "your", "his", "our", "their", "that", "thats", "this", "if", "so", "than",
		"then", "do", "does", "did", "has", "have", "had", "can", "will", "would", "could", "should", "just", "theres", "were",
		"what", "whats", "who", "how", "when", "up", "out", "all", "some", "any", "there", "here", "about"};
	for (const char* F : List)
	{
		if (W == F)
		{
			return true;
		}
	}
	return false;
}

struct Nucleus
{
	size_t From = 0;
	size_t To = 0; // exclusive
};

/** The vowel a nucleus makes, read from its letters and what follows them. */
int VowelFor(const std::string& W, const Nucleus& Nu, bool Last, bool MagicE, int Syllables)
{
	const std::string G = W.substr(Nu.From, Nu.To - Nu.From);
	const char After = Nu.To < W.size() ? W[Nu.To] : '\0';
	const char After2 = Nu.To + 1 < W.size() ? W[Nu.To + 1] : '\0';
	const bool RControl = After == 'r' && !(After2 != '\0' && (VowelLetter(After2) || After2 == 'y'));
	const bool WordFinal = Nu.To >= W.size();
	using namespace vox;
	if (W == "you" || W == "youre")
	{
		return VUW;
	}
	if (W == "your" || W == "four" || W == "pour")
	{
		return VAO;
	}
	if (G == "a")
	{
		if (W == "a")
		{
			return VAX;
		}
		if (RControl)
		{
			return VAA;
		}
		if (MagicE)
		{
			return VEY;
		}
		if (After == 'l' && (After2 == 'l' || After2 == 'k' || After2 == 't'))
		{
			return VAO;
		}
		return VAE;
	}
	if (G == "ai" || G == "ay" || G == "ei" || G == "ey" || G == "eigh")
	{
		return VEY;
	}
	if (G == "au" || G == "aw")
	{
		return VAO;
	}
	if (G == "e")
	{
		if (RControl)
		{
			return VER;
		}
		if (MagicE)
		{
			return VIY;
		}
		if (Last && WordFinal && W.size() <= 3)
		{
			return VIY; // me, be, we, she
		}
		return VEH;
	}
	if (G == "ee" || G == "ea")
	{
		return VIY;
	}
	if (G == "ie")
	{
		return WordFinal ? VAY : VIY;
	}
	if (G == "ye")
	{
		return VAY;
	}
	if (G == "i")
	{
		if (RControl)
		{
			return VER;
		}
		if (MagicE || (After == 'g' && After2 == 'h') || (After == 'n' && After2 == 'd') || (After == 'l' && After2 == 'd'))
		{
			return VAY;
		}
		if (W == "i")
		{
			return VAY;
		}
		return VIH;
	}
	if (G == "o")
	{
		if (RControl)
		{
			return VAO;
		}
		if (MagicE || (Last && WordFinal) || (After == 'l' && (After2 == 'd' || After2 == 'l' || After2 == 't')))
		{
			return VOW;
		}
		return VAA;
	}
	if (G == "oa")
	{
		return VOW;
	}
	if (G == "ow")
	{
		return Nu.From >= 2 || After == 'n' ? VOW : VAW;
	}
	if (G == "oo")
	{
		return After == 'k' || After == 'd' ? VUH : VUW;
	}
	if (G == "ou")
	{
		if (After == 'l' && After2 == 'd')
		{
			return VUH; // could, would, should
		}
		if (After == 'g' && After2 == 'h')
		{
			return VOW;
		}
		return VAW;
	}
	if (G == "oi" || G == "oy")
	{
		return VOY;
	}
	if (G == "u")
	{
		if (RControl)
		{
			return VER;
		}
		if (MagicE)
		{
			return VUW;
		}
		if ((After == 'l' && After2 == 'l') || (After == 's' && After2 == 'h'))
		{
			return VUH;
		}
		return VAH;
	}
	if (G == "ue" || G == "ui" || G == "ew")
	{
		return VUW;
	}
	if (G == "y")
	{
		if (Last && WordFinal)
		{
			return Syllables == 1 ? VAY : VIY;
		}
		return VIH;
	}
	switch (G.empty() ? 'a' : G[0])
	{
	case 'a': return VAE;
	case 'e': return VEH;
	case 'i': return VIH;
	case 'o': return VAA;
	case 'u': return VAH;
	default: return VAX;
	}
}

/** The word's syllables, appended to Out. */
/** An acronym, letter by letter (bee-bee, eye-tee-em): each letter's name is a stressed syllable, the last most. */
void SpellOut(const std::string& Letters, std::vector<SylSpec>& Out)
{
	for (size_t I = 0; I < Letters.size(); ++I)
	{
		const char L = Letters[I];
		SylSpec S;
		S.Stressed = I + 1 == Letters.size();
		S.WordEnd = I + 1 == Letters.size();
		switch (L)
		{
		case 'a': case 'h': case 'j': case 'k': S.V = vox::VEY; break;
		case 'i': case 'y': S.V = vox::VAY; break;
		case 'o': S.V = vox::VOW; break;
		case 'q': case 'u': case 'w': S.V = vox::VUW; break;
		case 'r': S.V = vox::VAA; break;
		case 'f': case 'l': case 'm': case 'n': case 's': case 'x': S.V = vox::VEH; break;
		default: S.V = vox::VIY; break; // b c d e g p t v z
		}
		// "Bee", "tee": the letter is the onset; "eff", "ell", "em": it's the coda.
		const Unit U = ConsonantAt(std::string(1, L) + ((L == 'c' || L == 'g') ? "e" : ""), 0); // see, gee: the soft ones
		const bool Coda = L == 'f' || L == 'l' || L == 'm' || L == 'n' || L == 's' || L == 'x' || L == 'r';
		if (U.Manner != vox::MNone && L != 'a' && L != 'e' && L != 'i' && L != 'o' && L != 'u')
		{
			(Coda ? S.Coda : S.Onset) = U.Manner;
			(Coda ? S.CodaPlace : S.OnPlace) = U.Place;
		}
		if (L == 'h')
		{
			S.Coda = vox::MShibilant; // aitch
			S.CodaPlace = 2;
		}
		Out.push_back(S);
	}
}

void Syllabify(const std::string& Word, std::vector<SylSpec>& Out)
{
	if (Word.empty())
	{
		return;
	}
	if (Word[0] == '#')
	{
		SpellOut(Word.substr(1), Out);
		return;
	}
	const std::string& W = Word;
	// Nuclei: runs of vowel letters; y is a vowel unless it starts the word or a vowel follows it; w and y after a
	// vowel belong to it when no vowel follows (show, day).
	std::vector<Nucleus> Nu;
	const size_t N = W.size();
	auto IsV = [&](size_t I) {
		const char C = W[I];
		if (VowelLetter(C))
		{
			return !(C == 'u' && I > 0 && W[I - 1] == 'q');
		}
		if (C == 'y')
		{
			return I > 0 && !(I + 1 < N && VowelLetter(W[I + 1]));
		}
		return false;
	};
	for (size_t I = 0; I < N;)
	{
		if (!IsV(I))
		{
			++I;
			continue;
		}
		Nucleus X;
		X.From = I;
		while (I < N && IsV(I))
		{
			++I;
		}
		if (I < N && (W[I] == 'w' || W[I] == 'y') && !(I + 1 < N && VowelLetter(W[I + 1])))
		{
			++I;
		}
		X.To = I;
		Nu.push_back(X);
	}
	// Silent endings: a final e after a consonant (but "-le" is its own syllable), "-es" and "-ed" after most consonants.
	bool MagicE = false;
	if (Nu.size() >= 2)
	{
		const Nucleus& L = Nu.back();
		const std::string Tail = W.substr(L.From);
		const char Before = L.From > 0 ? W[L.From - 1] : '\0';
		const char Before2 = L.From > 1 ? W[L.From - 2] : '\0';
		bool Drop = false;
		if (Tail == "e" && Before != '\0' && !VowelLetter(Before))
		{
			Drop = !(Before == 'l' && Before2 != '\0' && !VowelLetter(Before2));
			MagicE = Drop && L.From >= 2 && Before2 != '\0' && (VowelLetter(Before2) || Before2 == 'y');
			if (MagicE && L.From >= 3 && VowelLetter(W[L.From - 3]))
			{
				MagicE = false; // "ea" + consonant + e: leave it
			}
		}
		else if (Tail == "es" && Before != 's' && Before != 'x' && Before != 'z' && !(Before == 'h' && (Before2 == 'c' || Before2 == 's')))
		{
			Drop = true;
		}
		else if (Tail == "ed" && Before != 't' && Before != 'd')
		{
			Drop = true;
		}
		if (Drop)
		{
			Nu.pop_back();
		}
	}
	if (Nu.empty())
	{
		// No vowel at all ("hmm", "shh"): a hum if it's a hum, else nothing to say.
		if (W.find('m') != std::string::npos)
		{
			SylSpec S;
			S.V = vox::VHum;
			S.WordEnd = true;
			S.Stressed = true;
			Out.push_back(S);
		}
		return;
	}
	const bool Function = FunctionWord(W);
	const int Count = static_cast<int>(Nu.size());
	// Stress: the first syllable, the second after an unstressed prefix, the one before -tion/-sion/-ic.
	int Stress = 0;
	if (Count >= 2)
	{
		static const char* Prefixes[] = {"a", "be", "de", "re", "con", "com", "pre", "ex"};
		for (const char* P : Prefixes)
		{
			const size_t L = std::strlen(P);
			if (W.compare(0, L, P) == 0 && Nu[0].From < L && Nu[1].From >= L)
			{
				Stress = 1;
				break;
			}
		}
		if (W.size() > 4)
		{
			const std::string End = W.substr(W.size() - 4);
			if (End == "tion" || End == "sion" || W.compare(W.size() - 2, 2, "ic") == 0)
			{
				Stress = std::max(0, Count - 2);
			}
		}
	}
	// Consonants: a word's first run is the first syllable's onset; a run between two nuclei gives its first unit to the
	// coda before and its last to the onset after; the last run is the last coda.
	auto Units = [&](size_t From, size_t To, std::vector<Unit>& Into) {
		Into.clear();
		for (size_t I = From; I < To;)
		{
			Unit U = ConsonantAt(W, I);
			U.Letters = std::max(1, std::min(U.Letters, static_cast<int>(To - I)));
			if (!U.Silent && U.Manner != vox::MNone)
			{
				Into.push_back(U);
			}
			I += static_cast<size_t>(U.Letters);
		}
	};
	std::vector<Unit> Run;
	const size_t Base = Out.size();
	for (int K = 0; K < Count; ++K)
	{
		SylSpec S;
		S.V = static_cast<uint8_t>(VowelFor(W, Nu[static_cast<size_t>(K)], K == Count - 1, MagicE && K == Count - 1, Count));
		S.Stressed = !Function && K == Stress;
		S.Function = Function;
		S.WordEnd = K == Count - 1;
		const size_t RunFrom = K == 0 ? 0 : Nu[static_cast<size_t>(K - 1)].To;
		Units(RunFrom, Nu[static_cast<size_t>(K)].From, Run);
		if (!Run.empty())
		{
			S.Onset = Run.back().Manner;
			S.OnPlace = Run.back().Place;
			if (K > 0 && Run.size() >= 2)
			{
				SylSpec& Prev = Out[Base + static_cast<size_t>(K - 1)];
				Prev.Coda = Run.front().Manner;
				Prev.CodaPlace = Run.front().Place;
			}
		}
		Out.push_back(S);
	}
	Units(Nu.back().To, N, Run);
	if (!Run.empty())
	{
		SylSpec& LastS = Out.back();
		LastS.Coda = Run.front().Manner;
		LastS.CodaPlace = Run.front().Place;
		if (Run.front().Manner == vox::MRhotic && Run.size() >= 2)
		{
			LastS.Coda = Run[1].Manner; // "car" is in the vowel; "cart" ends in t
			LastS.CodaPlace = Run[1].Place;
		}
	}
	// A syllabic -le: the l is its onset and its vowel is a schwa.
	if (Count >= 2 && W.size() >= 3 && W.compare(W.size() - 2, 2, "le") == 0 && !VowelLetter(W[W.size() - 3]))
	{
		Out.back().V = vox::VAX;
	}
}

uint32_t HashText(const std::string& S)
{
	uint32_t H = 2166136261u;
	for (const char C : S)
	{
		H ^= static_cast<unsigned char>(C);
		H *= 16777619u;
	}
	return H;
}
} // namespace voice_detail

Timbre TimbreFor(bool Woman, int Age, uint32_t Seed)
{
	vox::Rng R;
	R.Seed(Seed ^ 0x7153u);
	const float A = static_cast<float>(std::min(std::max(Age, 16), 90));
	Timbre T;
	if (Woman)
	{
		// Women's voices settle lower with age.
		const float Base = 214.0f - 0.55f * std::max(0.0f, A - 30.0f);
		T.F0 = std::min(255.0f, std::max(165.0f, Base * R.Range(0.9f, 1.1f)));
		T.Tract = 1.17f * R.Range(0.96f, 1.04f);
		T.Breath = R.Range(0.18f, 0.3f);
	}
	else
	{
		// Men's dip through middle age and rise a little after sixty.
		const float Base = 118.0f - 0.25f * std::max(0.0f, A - 25.0f) + 0.45f * std::max(0.0f, A - 60.0f);
		T.F0 = std::min(155.0f, std::max(85.0f, Base * R.Range(0.88f, 1.14f)));
		T.Tract = R.Range(0.95f, 1.05f);
		T.Breath = R.Range(0.08f, 0.2f);
	}
	T.Rate = std::min(5.0f, std::max(4.0f, R.Range(4.2f, 5.0f) - (A > 60.0f ? 0.25f : 0.0f)));
	T.Rasp = std::min(0.9f, std::max(0.0f, 0.06f + 0.012f * std::max(0.0f, A - 35.0f) + R.Range(-0.05f, 0.12f)));
	return T;
}

Timbre PaTimbre(uint32_t Seed)
{
	vox::Rng R;
	R.Seed(Seed ^ 0x9a11u);
	Timbre T;
	if (R.Chance(0.4f))
	{
		T.F0 = R.Range(180.0f, 205.0f);
		T.Tract = 1.16f * R.Range(0.98f, 1.02f);
	}
	else
	{
		T.F0 = R.Range(105.0f, 128.0f);
		T.Tract = R.Range(0.97f, 1.03f);
	}
	T.Rate = R.Range(3.3f, 3.8f);
	T.Breath = 0.1f;
	T.Rasp = 0.05f;
	return T;
}

SpeechPlan PlanSpeech(const std::string& Line, const Timbre& T, float Effort, uint32_t Seed)
{
	using namespace voice_detail;
	SpeechPlan Plan;
	std::vector<Phrase> Phrases;
	Tokenize(Line, Phrases);
	vox::Rng R;
	R.Seed(vox::Mix32(Seed, HashText(Line)));
	const float E = std::min(1.2f, std::max(0.0f, Effort));
	std::vector<SylSpec> Specs;
	float At = 0.0f;
	for (size_t P = 0; P < Phrases.size(); ++P)
	{
		Specs.clear();
		for (const std::string& W : Phrases[P].Words)
		{
			Syllabify(W, Specs);
		}
		if (Specs.empty())
		{
			continue;
		}
		const size_t From = Plan.Syllables.size();
		Plan.Syllables.resize(From + Specs.size());
		At = vox::ProsodyPhrase(Specs.data(), static_cast<int>(Specs.size()), Phrases[P].End, T, E, R, At, Plan.Syllables.data() + From);
		if (P + 1 < Phrases.size())
		{
			At += vox::PauseAfter(Phrases[P].End, T, R);
		}
	}
	Plan.Seconds = Plan.Syllables.empty() ? 0.0f : Plan.Syllables.back().Start + Plan.Syllables.back().Length + 0.05f;
	return Plan;
}

float MouthAt(const SpeechPlan& P, float T)
{
	const std::vector<Syllable>& S = P.Syllables;
	if (S.empty() || T < S.front().Start)
	{
		return 0.0f;
	}
	// The last syllable starting at or before T.
	size_t Lo = 0;
	size_t Hi = S.size();
	while (Hi - Lo > 1)
	{
		const size_t Mid = (Lo + Hi) / 2;
		if (S[Mid].Start <= T)
		{
			Lo = Mid;
		}
		else
		{
			Hi = Mid;
		}
	}
	const Syllable& Y = S[Lo];
	if (Y.Length <= 0.0f || T >= Y.Start + Y.Length)
	{
		return 0.0f;
	}
	// The jaw opens through the onset, holds on the vowel and closes into the next consonant.
	const float U = (T - Y.Start) / Y.Length;
	auto Smooth = [](float X) { return X * X * (3.0f - 2.0f * X); };
	const float Rise = Y.Onset != 0 ? 0.35f : 0.18f;
	const float Fall = Y.Coda != 0 ? 0.3f : 0.2f;
	float Env = 1.0f;
	if (U < Rise)
	{
		Env = 0.12f + 0.88f * Smooth(U / Rise);
	}
	else if (U > 1.0f - Fall)
	{
		Env = 0.12f + 0.88f * Smooth((1.0f - U) / Fall);
	}
	return std::min(1.0f, std::max(0.0f, Y.Open * Env));
}

namespace voice_detail
{
void Limit(std::vector<float>& Samples, float Ceiling)
{
	float Peak = 0.0f;
	for (const float V : Samples)
	{
		Peak = std::max(Peak, std::fabs(V));
	}
	if (Peak > Ceiling)
	{
		const float Scale = Ceiling / Peak;
		for (float& V : Samples)
		{
			V *= Scale;
		}
	}
}

/** The effort a plan was made at, read back from its gains (PlanSpeech folds effort into them). */
float EffortOf(const SpeechPlan& P)
{
	if (P.Syllables.empty())
	{
		return 0.6f;
	}
	float Sum = 0.0f;
	for (const Syllable& S : P.Syllables)
	{
		Sum += S.Gain;
	}
	const float Mean = Sum / static_cast<float>(P.Syllables.size());
	return std::min(1.1f, std::max(0.2f, (Mean / 0.82f - 0.55f) / 0.75f));
}
} // namespace voice_detail

std::vector<float> RenderSpeech(const SpeechPlan& P, const Timbre& T, bool Murmur, uint32_t Seed)
{
	if (P.Syllables.empty())
	{
		return {};
	}
	vox::DenormalGuard Guard;
	const float Rate = vox::OutRate;
	float End = P.Seconds;
	for (const Syllable& S : P.Syllables)
	{
		End = std::max(End, S.Start + S.Length);
	}
	std::vector<float> Out(static_cast<size_t>((End + 0.15f) * Rate), 0.0f);
	vox::Talker Voice;
	Voice.Start(P.Syllables.data(), static_cast<int>(P.Syllables.size()), T, Murmur, voice_detail::EffortOf(P), Seed, Rate);
	Voice.Render(Out.data(), static_cast<int>(Out.size()));
	voice_detail::Limit(Out, 0.95f);
	return Out;
}

std::vector<float> RenderVocal(Vocal V, const Timbre& T, int Voices, uint32_t Seed)
{
	if (V == Vocal::None)
	{
		return {};
	}
	vox::DenormalGuard Guard;
	const float Rate = vox::OutRate;
	const int People = std::max(1, std::min(Voices, 400));
	vox::Rng R;
	R.Seed(vox::Mix32(Seed, static_cast<uint32_t>(V) * 7919u + static_cast<uint32_t>(People)));
	std::vector<float> Out;
	if (V == Vocal::Clap || V == Vocal::Applause)
	{
		std::vector<vox::Clapper> Hands(static_cast<size_t>(std::min(People, 160)));
		const int N = vox::SeedClappers(V, People, R, 0.02f, Hands.data(), static_cast<int>(Hands.size()));
		float Length = 0.0f;
		for (int I = 0; I < N; ++I)
		{
			Length = std::max(Length, Hands[static_cast<size_t>(I)].Until);
		}
		Out.assign(static_cast<size_t>((Length + 0.2f) * Rate), 0.0f);
		const float Norm = 0.32f / std::sqrt(static_cast<float>(std::max(1, N)));
		vox::ClapVoice C;
		for (int I = 0; I < N; ++I)
		{
			const vox::Clapper& H = Hands[static_cast<size_t>(I)];
			for (float At = H.Start; At < H.Until; At += H.Period * R.Range(0.85f, 1.15f))
			{
				const float G = vox::ClapGain(H, At);
				if (G <= 0.0f)
				{
					continue;
				}
				C.Strike(H.Hz * R.Range(0.95f, 1.05f), H.Q, G * Norm, R.Range(0.004f, 0.008f), Rate);
				const size_t From = static_cast<size_t>(At * Rate);
				const size_t Len = static_cast<size_t>(0.04f * Rate);
				for (size_t K = 0; K < Len && From + K < Out.size(); ++K)
				{
					const float Noise = R.Signed();
					Out[From + K] += (C.Band.Run(Noise) * C.Env * 2.2f + Noise * C.Click * 0.25f) * C.Gain;
					C.Env *= C.Decay;
					C.Click *= 0.6f;
				}
				C.Band.Clear();
			}
		}
		voice_detail::Limit(Out, 0.95f);
		return Out;
	}
	// Each person their own voice, a little apart; past 24 the crowd is louder, not longer.
	const int Rendered = std::min(People, 24);
	const float Spread = V == Vocal::Laugh ? 0.6f : V == Vocal::Cheer ? 0.4f : V == Vocal::Ooh ? 0.25f : 0.3f;
	const float Effort = V == Vocal::Cheer ? 1.0f : V == Vocal::Gasp ? 0.8f : V == Vocal::Laugh ? 0.65f : V == Vocal::Sigh ? 0.4f : 0.55f;
	const float Norm = (Rendered > 1 ? 1.2f / std::sqrt(static_cast<float>(Rendered)) : 1.0f) * std::sqrt(static_cast<float>(People) / static_cast<float>(Rendered));
	Syllable Plan[16];
	std::vector<float> One;
	for (int P = 0; P < Rendered; ++P)
	{
		const Timbre Who = P == 0 ? T : vox::CrowdMember(T, R);
		const float Onset = P == 0 ? 0.0f : R.Range(0.0f, Spread);
		const int N = vox::VocalPlan(V, Who, R, 0.0f, Plan, 16);
		if (N <= 0)
		{
			continue;
		}
		float End = 0.0f;
		for (int K = 0; K < N; ++K)
		{
			End = std::max(End, Plan[K].Start + Plan[K].Length);
		}
		One.assign(static_cast<size_t>((End + 0.15f) * Rate), 0.0f);
		vox::Talker Voice;
		Voice.Start(Plan, N, Who, false, Effort, vox::Mix32(Seed, static_cast<uint32_t>(P)), Rate);
		Voice.Render(One.data(), static_cast<int>(One.size()));
		const size_t From = static_cast<size_t>(Onset * Rate);
		if (Out.size() < From + One.size())
		{
			Out.resize(From + One.size(), 0.0f);
		}
		for (size_t K = 0; K < One.size(); ++K)
		{
			Out[From + K] += One[K] * Norm;
		}
	}
	voice_detail::Limit(Out, 0.95f);
	return Out;
}

void PaChain(std::vector<float>& Samples, uint32_t Seed)
{
	if (Samples.empty())
	{
		return;
	}
	vox::DenormalGuard Guard;
	vox::PaStage Pa;
	Pa.Init(vox::OutRate, Seed);
	Samples.resize(Samples.size() + static_cast<size_t>(Pa.TailSeconds() * vox::OutRate), 0.0f);
	Pa.Process(Samples.data(), static_cast<int>(Samples.size()));
	voice_detail::Limit(Samples, 0.98f);
}
} // namespace audio
} // namespace ss
