// The copy lint (next_segment_spec §7.6, N8): every suite runs it over its inventories (talk::Inventory(), the
// sponsors' letters and pitches, the backers' texts, the badges' labels and honorifics, the PA). No placeholder ships.
//
// A line passes when, with its {Vars} filled from sample values:
//   - no '{' or '}' is left over (every variable it uses has a sample);
//   - it is at most MaxChars characters (UTF-8 code points): 84 for table talk, 200 for the PA and mail subjects,
//     600 for a letter's body;
//   - it ends in '.', '!', '?' or a closing quote (" ' and the curly closing quotes);
//   - it has no doubled spaces;
//   - it holds none of the banned strings: "Lorem", "TODO", "TBD", "placeholder", "XXX".
//
// Use: copylint::Run("talk", talk::Inventory(), copylint::TableTalk, {{"Table", "12"}}) prints each problem and returns
// how many there were; add it to the suite's failures. Samples() holds the common variables; pass the suite's own on
// top of them. Header-only; every test executable may include it.
#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace copylint
{
/** The longest a filled line may be, in characters. */
constexpr size_t TableTalk = 84; // table talk, dealer calls, the rail
constexpr size_t Pa = 200;       // the floor on the PA
constexpr size_t MailSubject = 200;
constexpr size_t LetterBody = 600;

/** The common template variables and a sample of realistic length for each. */
inline std::map<std::string, std::string> Samples()
{
	return {
		{"Hero", "Jesse"},
		{"HeroFull", "Jesse Cole"},
		{"Name", "Mrs. Park"},
		{"Who", "Big Lou"},
		{"Event", "Embercrest Nightly"},
		{"Short", "Nightly"},
		{"Table", "12"},
		{"Seat", "4"},
		{"Place", "3rd"},
		{"Left", "41"},
		{"Paid", "14"},
		{"Amount", "$1,280"},
		{"Chips", "18,400"},
		{"Bb", "11.5"},
		{"Brand", "Volt Rush"},
		{"Rep", "Dana Kell"},
		{"Day", "Saturday"},
		{"Time", "8:00 PM"},
		{"Level", "9"},
		{"Blinds", "800/1,600"},
	};
}

/** Characters in UTF-8 text (code points: continuation bytes don't count). */
inline size_t Characters(const std::string& Utf8)
{
	size_t N = 0;
	for (const char Ch : Utf8)
	{
		N += (static_cast<unsigned char>(Ch) & 0xC0u) != 0x80u ? 1u : 0u;
	}
	return N;
}

/** Template with every {Key} that Vars knows replaced; unknown keys stay as written. */
inline std::string Fill(const std::string& Template, const std::map<std::string, std::string>& Vars)
{
	std::string Out;
	size_t I = 0;
	while (I < Template.size())
	{
		const size_t Open = Template.find('{', I);
		if (Open == std::string::npos)
		{
			Out.append(Template, I, std::string::npos);
			break;
		}
		const size_t Close = Template.find('}', Open + 1);
		if (Close == std::string::npos)
		{
			Out.append(Template, I, std::string::npos);
			break;
		}
		Out.append(Template, I, Open - I);
		const auto It = Vars.find(Template.substr(Open + 1, Close - Open - 1));
		Out += It != Vars.end() ? It->second : Template.substr(Open, Close - Open + 1);
		I = Close + 1;
	}
	return Out;
}

inline bool EndsWith(const std::string& S, const char* Tail)
{
	const std::string T(Tail);
	return S.size() >= T.size() && S.compare(S.size() - T.size(), T.size(), T) == 0;
}

inline std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return S;
}

/** What's wrong with one template once filled from Vars ("" when nothing). */
inline std::string Check(const std::string& Template, size_t MaxChars, const std::map<std::string, std::string>& Vars)
{
	const std::string Line = Fill(Template, Vars);
	if (Line.empty())
	{
		return "empty";
	}
	if (Line.find('{') != std::string::npos || Line.find('}') != std::string::npos)
	{
		return "a variable with no sample: " + Line;
	}
	if (Characters(Line) > MaxChars)
	{
		return "longer than " + std::to_string(MaxChars) + " characters (" + std::to_string(Characters(Line)) + ")";
	}
	if (!(EndsWith(Line, ".") || EndsWith(Line, "!") || EndsWith(Line, "?") || EndsWith(Line, "\"") || EndsWith(Line, "'") ||
		  EndsWith(Line, "\xE2\x80\x9D") || EndsWith(Line, "\xE2\x80\x99")))
	{
		return "doesn't end in . ! ? or a closing quote";
	}
	if (Template.find("  ") != std::string::npos || Line.find("  ") != std::string::npos)
	{
		return "a doubled space";
	}
	for (const char* Banned : {"TODO", "TBD", "XXX"})
	{
		if (Line.find(Banned) != std::string::npos)
		{
			return std::string("holds \"") + Banned + "\"";
		}
	}
	const std::string Low = Lower(Line);
	for (const char* Banned : {"lorem", "placeholder"})
	{
		if (Low.find(Banned) != std::string::npos)
		{
			return std::string("holds \"") + Banned + "\"";
		}
	}
	return std::string();
}

/** Every (speaker, template) of an inventory, with Extra's variables on top of Samples(): prints each problem, returns how many. */
inline int Run(const char* Suite, const std::vector<std::pair<std::string, std::string>>& Inventory, size_t MaxChars,
	const std::map<std::string, std::string>& Extra = {})
{
	std::map<std::string, std::string> Vars = Samples();
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	int Problems = 0;
	for (const auto& Entry : Inventory)
	{
		const std::string Why = Check(Entry.second, MaxChars, Vars);
		if (!Why.empty())
		{
			++Problems;
			std::printf("COPY (%s) %s: \"%s\": %s\n", Suite, Entry.first.c_str(), Entry.second.c_str(), Why.c_str());
		}
	}
	return Problems;
}

/** The lint on lines it must pass and lines it must catch: 0 when it works (prints what it got wrong). */
inline int SelfCheck()
{
	const std::map<std::string, std::string> Vars = Samples();
	int Wrong = 0;
	const auto Expect = [&](const std::string& Template, size_t Max, bool Passes) {
		const std::string Why = Check(Template, Max, Vars);
		if (Why.empty() != Passes)
		{
			++Wrong;
			std::printf("FAILED: copy lint on \"%s\": %s\n", Template.c_str(), Why.empty() ? "passed" : Why.c_str());
		}
	};
	Expect("{Who} is putting you in. Don't make him regret it.", TableTalk, true);
	Expect("Seat {Seat}, it's on you.", TableTalk, true);
	Expect("You good for rent?", TableTalk, true);
	Expect("All in and a call, table {Table}!", TableTalk, true);
	Expect("He said \"fold.\"", TableTalk, true);
	Expect("Mei leaned over: \xE2\x80\x9CSwap ten?\xE2\x80\x9D", TableTalk, true);
	Expect("Forty-something. You\xE2\x80\x99re fine.", TableTalk, true);
	Expect(std::string(83, 'a') + ".", TableTalk, true); // 84 characters
	Expect(std::string(84, 'a') + ".", TableTalk, false);
	Expect(std::string(80, 'a') + "\xE2\x80\x99s.", TableTalk, true); // 83 characters in 85 bytes
	Expect("", TableTalk, false);
	Expect("{Nobody} took it down.", TableTalk, false);
	Expect("Two  spaces here.", TableTalk, false);
	Expect("No ending here", TableTalk, false);
	Expect("A trailing space. ", TableTalk, false);
	Expect("TODO: write Lou's line.", TableTalk, false);
	Expect("TBD.", TableTalk, false);
	Expect("Call XXX back.", TableTalk, false);
	Expect("Lorem ipsum dolor.", Pa, false);
	Expect("A Placeholder line.", Pa, false);
	Expect("This is a table talk line that runs on and on, well past the eighty-four character limit.", TableTalk, false);
	Expect("This is a table talk line that runs on and on, well past the eighty-four character limit.", Pa, true);
	return Wrong;
}
} // namespace copylint
