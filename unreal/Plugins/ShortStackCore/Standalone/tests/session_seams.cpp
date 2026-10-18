// night_one_session: the lead's Phase A checks (next_segment_spec §7.5, §8.2), compiled into session_test and called
// from main() in session_main.cpp before the tracks' files. The seams changed nothing a save or a session can see:
//   - SaveFixtures: the saves Tests/fixtures/ holds, written by c09bc31's code (tests/fixture_tool.cpp), parse and write
//     back byte for byte, load a second time the same, and keep what each one must (the mid-month entry resumes with
//     its checkpoint and the player's own money; Kast's riverline deal is Team RiverLine; the v1 save reads as the
//     Embercrest's); the world's own text reads and writes back as it was;
//   - LiveRecord: the record the kept entries can't hold (LiveWins, LiveSpentCents, LiveDays, LastShiftAt): kept by
//     Register, Settle and a shift, worked out for an older save, written only once kept;
//   - EntryLines: livefund, livevoice, liveroom, livebadge, liveleader, livepatch and mail survive a save, escapes and all;
//     settle and refund clear the night's snapshot and keep the player's own patches;
//   - Funding: who pays changes what registration needs and writes (CanRegister, PayFare's covered bus), FundingLine;
//     anyone else's money only as the books offer it now (a stale or made-up funding is refused, nothing moves);
//   - Rounds: Tournament::Rounds reads the tick and draws nothing;
//   - Settings: the ten new settings default, clamp and round-trip; today's string reads to the defaults;
//   - SessionSeams: the Session.h block's stubs keep today's behavior (Record's renown is town::Renown's);
//   - Wave1a: the lead's fixes after wave 1a (the world knows the player's birth year; a package's roll affords online).
//   - Gate1Fixtures (update 2 C1, SPEC §6.5): the gate's two committed saves (fixture_tool write-g1) parse, write back byte
//     for byte, load a second time the same, and keep what PIE §8.1 reads from them: the Blank world and its boards, the
//     played lines and Career's tags; the checkpointed Nightly's host text (auto, autoseen, autoread), the engine
//     checkpoint restoring, and the plan carrying on from it.
// Phase C's MigrationAll (§7.5) builds on SaveFixtures. A feature whose migration writes lines on load (§7.5 steps 2 to 4)
// keeps SaveFixtures green: only the segment's own lines (and Kast's legacy deal lines) may change, and that's reported.
#include "CopyLint.h"
#include "SessionSeg.h"

#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/UI/FrontEnd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace session_test
{
namespace seams
{
namespace life = ss::life;
namespace live = ss::live;
const double Day = 1440.0;

std::string ReadFile(const std::string& Path)
{
	std::string Out;
	FILE* F = std::fopen(Path.c_str(), "rb");
	if (!F)
	{
		return Out;
	}
	char Buf[65536];
	size_t N = 0;
	while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
	{
		Out.append(Buf, N);
	}
	std::fclose(F);
	// A checkout that converted the line endings (the fixtures' .gitattributes says not to) still reads as written.
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

std::string Fixture(const char* Name)
{
	return ReadFile(std::string(SS_TESTS_DIR) + "/fixtures/" + Name);
}

/** The save without the segment's own lines (and Kast's legacy deal lines, which core-sponsor's migration moves into the
 *  book), life\tlive back to its six fields: what must survive any build of the segment byte for byte. Update 2's lines
 *  (autoplay, video, site; update 2 SPEC §6.5) are allowed the same way, and kast\tclip goes back to its eight fields (the
 *  9th is the Rerun candidate's id). */
std::string Segmentless(const std::string& Text, int* Dropped = nullptr)
{
	static const char* Ours[] = {"livefund\t", "livevoice\t", "liveroom\t", "livebadge\t", "liveleader\t", "livepatch\t", "mail\t", "stake\t", "coach\t",
		"sponsor\t", "badge\t", "life\tshift\t", "life\tliveday\t", "kast\tdeal\t", "kast\toffer\t", "kast\tdeclined\t", "autoplay\t", "video\t", "site\t"};
	std::string Out;
	int Count = 0;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		std::string Line = Text.substr(Start, End - Start);
		Start = End;
		bool Drop = false;
		for (const char* Tag : Ours)
		{
			Drop = Drop || Line.rfind(Tag, 0) == 0;
		}
		if (Drop)
		{
			++Count;
			continue;
		}
		const bool LiveLine = Line.rfind("life\tlive\t", 0) == 0;
		if (LiveLine || Line.rfind("kast\tclip\t", 0) == 0)
		{
			const size_t Keep = LiveLine ? 6 : 8;
			size_t Tabs = 0;
			for (size_t K = 0; K < Line.size(); ++K)
			{
				if (Line[K] == '\t' && ++Tabs == Keep)
				{
					Line = Line.substr(0, K) + "\n";
					break;
				}
			}
		}
		Out += Line;
	}
	if (Dropped)
	{
		*Dropped = Count;
	}
	return Out;
}

/** Written back byte for byte; or, once a feature migrates on load, the same but for the segment's own lines (reported). */
void RoundTrip(const char* Name, const std::string& Saved, const std::string& Expected)
{
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(Saved, D);
	Expect(Parsed, (std::string("fixtures: ") + Name + " parses").c_str());
	if (!Parsed)
	{
		return;
	}
	const std::string Again = D.Serialize();
	if (Again == Expected)
	{
		std::printf("  fixtures: %s: written back byte for byte (%zu bytes)\n", Name, Again.size());
	}
	else
	{
		int Added = 0;
		const std::string Ours = Segmentless(Again, &Added);
		Expect(Ours == Segmentless(Expected), (std::string("fixtures: ") + Name + " writes back byte for byte but for the segment's own lines").c_str());
		std::printf("  fixtures: %s: written back with %d of the segment's lines\n", Name, Added);
	}
	ss::SaveData Second;
	Expect(ss::SaveData::Parse(Again, Second) && Second.Serialize() == Again, (std::string("fixtures: ") + Name + " loads a second time the same").c_str());
}

void SaveFixtures()
{
	const char* Names[] = {"pre_segment_fresh.ssave", "pre_segment_mid_nightly.ssave", "pre_segment_team_riverline.ssave", "pre_segment_evicted.ssave"};
	for (const char* Name : Names)
	{
		const std::string Saved = Fixture(Name);
		Expect(Saved.rfind("shortstack.nightone.v2\n", 0) == 0, (std::string("fixtures: ") + Name + " is there").c_str());
		RoundTrip(Name, Saved, Saved);
	}
	// The v1 ("riverside") save reads as the Embercrest's and writes what c09bc31 wrote for it.
	{
		const std::string V1 = Fixture("pre_segment_v1.ssave");
		RoundTrip("pre_segment_v1.ssave", V1, Fixture("pre_segment_v1.expected.ssave"));
		ss::SaveData D;
		bool Renamed = ss::SaveData::Parse(V1, D) && !D.Life.LiveEntries.empty();
		for (const life::LiveEntry& E : D.Life.LiveEntries)
		{
			Renamed = Renamed && E.Id.rfind("embercrest", 0) == 0 && E.Name.find("Riverside") == std::string::npos;
		}
		Expect(Renamed, "fixtures: the v1 save's entries are the Embercrest's");
	}
	// The fresh career keeps its bankroll and rent to the cent.
	{
		ss::SaveData D;
		Expect(ss::SaveData::Parse(Fixture("pre_segment_fresh.ssave"), D) && D.BankrollCents == 237 + ss::hero::PerksOf(D.Person).StartCents &&
				   D.Life.RentStage == life::Rent::Due && D.Life.RentDueCents == 122500 && D.Life.RentDeadline == 5.0 * Day,
			"fixtures: the fresh career's bankroll and first rent");
		Expect(D.Life.LiveWins == 0 && D.Life.LiveSpentCents == 0 && D.Life.LiveDays.empty() && D.Life.LastShiftAt == -1.0e9 && D.Life.SegmentLinesRead == 0,
			"fixtures: a fresh career's record is empty, and nothing of the segment's was read");
	}
	// The middle of a month: the live record worked out from the kept entries; tonight's entry resumes as it was.
	{
		const std::string Saved = Fixture("pre_segment_mid_nightly.ssave");
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(Saved, D);
		std::string World = Fixture("world_v1.txt");
		D.WorldText = World;
		const life::LiveEntry* Tonight = live::EntryFor(D.Life, "embercrest-nightly@9");
		// Its voices are the ones its roster gives (live::AfterParse fills an entry kept before voices were with
		// room::VoicesFor, core-room's: empty while that was a stub), and the room's state starts empty.
		bool RosterVoices = Tonight != nullptr;
		if (Tonight)
		{
			const std::vector<life::LiveVoice> Given = ss::room::VoicesFor(*Tonight);
			RosterVoices = Given.size() == Tonight->Voices.size();
			for (size_t K = 0; RosterVoices && K < Given.size(); ++K)
			{
				RosterVoices = Given[K].Name == Tonight->Voices[K].Name && Given[K].Style == Tonight->Voices[K].Style;
			}
		}
		Expect(Parsed && Tonight && Tonight->State == life::LiveEntry::Registered && !Tonight->Checkpoint.empty() && Tonight->ArrivedAt > 0.0 && life::IsOwnMoney(Tonight->Fund) &&
				   RosterVoices && Tonight->Room.empty(),
			"fixtures: tonight's entry, checkpointed, the player's own money, the voices its roster gives, no room");
		int Wins = 0;
		ss::Chips Spent = 0;
		int Finished = 0;
		for (const life::LiveEntry& E : D.Life.LiveEntries)
		{
			Wins += E.State == life::LiveEntry::Finished && E.Place == 1 ? 1 : 0;
			Spent += E.State == life::LiveEntry::Finished ? E.PaidCents : 0;
			Finished += E.State == life::LiveEntry::Finished ? 1 : 0;
		}
		Expect(Wins == 1 && D.Life.LiveWins == 1 && Finished == D.Life.LiveEvents && D.Life.LiveSpentCents == Spent && D.Life.LiveDays.size() == 2 &&
				   D.Life.LiveDays.front().Day == 3 && D.Life.LiveDays.back().Day == 5 && D.Life.LiveDays.back().WonCents == D.Life.LiveWonCents,
			"fixtures: the record from the kept entries: a title, what they cost, two days of play");
		Expect(std::fabs(D.Life.LastShiftAt - 3660.0) < 0.01 && (D.Life.SegmentLinesRead & (life::SegLiveWins | life::SegLiveRecord | life::SegShift)) == 0,
			"fixtures: the last shift from the ledger; none of it was in the save");
		Hooks H;
		ss::Session S(H, "fixture-resume", &D);
		Expect(S.LiveInProgress() == "embercrest-nightly@9", "fixtures: the night is there to go back to");
		const life::LiveEntry* Kept = live::EntryFor(S.Life, "embercrest-nightly@9");
		Expect(Kept && Kept->Checkpoint == Tonight->Checkpoint && life::IsOwnMoney(Kept->Fund), "fixtures: the session keeps the checkpoint and the funding");
		const ss::rep::Public R = S.Record();
		Expect(R.Titles == 1 && R.LiveCashes == D.Life.LiveCashes && R.LiveEvents == 2 && std::fabs(ss::town::Renown(S) - R.Renown) < 1e-12,
			"fixtures: the public record has the title; the town's renown is the record's");
		// Today's formula on the kept entries gives the same (a career of fewer than sixty live events).
		int Wall = 0;
		for (const life::LiveEntry& E : S.Life.LiveEntries)
		{
			Wall += E.State == life::LiveEntry::Finished && E.Place == 1 ? 1 : 0;
		}
		const double Followers = std::min(1.0, std::log10(1.0 + static_cast<double>(std::max(0, S.Channel.Followers))) / 5.0);
		const double Fans = std::min(1.0, static_cast<double>(S.Channel.Superfans()) / 40.0);
		const double Cashes = std::min(1.0, static_cast<double>(S.Life.LiveCashes) / 15.0);
		const double Wins2 = std::min(1.0, static_cast<double>(Wall) / 4.0);
		const double Awards = std::min(1.0, static_cast<double>(S.Living().HeroAwards().size()) / 2.0);
		const double Old = std::min(1.0, 0.35 * Followers + 0.15 * Fans + 0.15 * Cashes + 0.15 * Wins2 + 0.2 * Awards);
		Expect(ss::town::Renown(S) == Old, "fixtures: town::Renown is c09bc31's on this career, to the bit");
	}
	// Kast's deals moved into the sponsor book (core-sponsor, next_segment_spec §3.8): Team RiverLine kept, TunnelRat's offer
	// in the book (and in Mail), Sitwell's no a 60-day cooldown, and Kast's own lists cleared.
	{
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(Fixture("pre_segment_team_riverline.ssave"), D);
		Hooks H;
		ss::Session S(H, "fixture-kast", &D);
		Expect(Parsed && S.TeamRiverLine() && !ss::sponsor::OpenOfferFor(S.Life.Sponsors, "tunnelrat", S.WorldMinutes()).empty() && S.Life.Sponsors.Cooldown.count("sitwell") == 1 &&
				   S.Channel.Offers.empty() && S.Channel.Declined.empty() && S.Channel.Deals.empty(),
			"fixtures: Team RiverLine kept, TunnelRat's offer in the book, Sitwell's no a 60-day cooldown");
	}
	// Evicted: the locks changed, the gear in storage.
	{
		ss::SaveData D;
		Expect(ss::SaveData::Parse(Fixture("pre_segment_evicted.ssave"), D) && D.Life.RentStage == life::Rent::Evicted && D.Life.StorageDue > 0.0 && D.Life.Evictions == 1,
			"fixtures: the evicted career is still evicted, its gear in storage");
	}
	// The world's own lines: a version 1 world reads, migrates once to the current version (core-world: 2 at next_segment_spec §7.5 step 5, 3 at update 2 G1),
	// and the migrated world writes back byte for byte (a second load migrates nothing).
	auto ReadWorld = [](const std::string& Text, ss::world::World& W) {
		size_t Start = 0;
		while (Start < Text.size())
		{
			size_t End = Text.find('\n', Start);
			End = End == std::string::npos ? Text.size() : End;
			std::vector<std::string> Fields(1);
			for (size_t K = Start; K < End; ++K)
			{
				if (Text[K] == '\t')
				{
					Fields.emplace_back();
				}
				else
				{
					Fields.back().push_back(Text[K]);
				}
			}
			W.Read(Fields);
			Start = End + 1;
		}
		W.Finish();
	};
	{
		const std::string Text = Fixture("world_v1.txt");
		ss::world::World W;
		ReadWorld(Text, W);
		std::string Again;
		W.Write(Again);
		ss::world::World Second;
		ReadWorld(Again, Second);
		std::string Third;
		Second.Write(Third);
		// (Pinned to world::Version: 2 until update 2's G1, 3 from it, update 2 §6.5.)
		Expect(W.Ready() && !Text.empty() && Text.rfind("world\thead\t1\t", 0) == 0 && Again.rfind("world\thead\t" + std::to_string(ss::world::Version) + "\t", 0) == 0 &&
				   Third == Again && W.Began() == ss::world::Start::Established,
			("fixtures: world_v1.txt reads, migrates once to version " + std::to_string(ss::world::Version) + ", and writes back byte for byte").c_str());
	}
}

void LiveRecord()
{
	// Kept by Settle: a title, the buy-ins, the day, merged by day and kept 400 days.
	life::State L;
	ss::Chips Bank = 100000;
	const live::Occurrence A = live::FindOccurrence("embercrest-nightly@3");
	const live::Occurrence B = live::FindOccurrence("embercrest-noon@3");
	Expect(live::Register(Bank, L, B, B.DeskOpens + 10.0, 70, {}).empty() && live::Settle(Bank, L, B.Id, 1, 70, 90000, B.Start + 300.0), "record: a noon game won");
	Expect(live::Register(Bank, L, A, A.DeskOpens + 10.0, 80, {}).empty() && live::Settle(Bank, L, A.Id, 50, 80, 0, A.Start + 200.0), "record: the Nightly the same day");
	Expect(L.LiveWins == 1 && L.LiveSpentCents == B.T->BuyInCents + A.T->BuyInCents && L.LiveDays.size() == 1 && L.LiveDays[0].Day == 3 && L.LiveDays[0].Events == 2 &&
			   L.LiveDays[0].Cashes == 1 && L.LiveDays[0].Embercrest == 2 && L.LiveDays[0].EmbercrestCashes == 1 && L.LiveDays[0].WonCents == 90000 &&
			   L.LiveDays[0].SpentCents == L.LiveSpentCents && (L.SegmentLinesRead & life::SegLiveRecord) != 0,
		"record: one day, two events, a cash, the title and what they cost");
	// A refund was never played: no record.
	const live::Occurrence C = live::FindOccurrence("embercrest-nightly@4");
	Expect(live::Register(Bank, L, C, C.DeskOpens + 10.0, 80, {}).empty() && live::Refund(Bank, L, C.Id, C.Start, "not enough players") && L.LiveDays.size() == 1 &&
			   L.LiveSpentCents == B.T->BuyInCents + A.T->BuyInCents,
		"record: a refund isn't played");
	// 400 days on, the first day goes.
	const live::Occurrence Late = live::FindOccurrence("embercrest-nightly@" + std::to_string(3 + life::LiveDaysKept));
	Expect(live::Register(Bank, L, Late, Late.DeskOpens + 10.0, 80, {}).empty() && live::Settle(Bank, L, Late.Id, 9, 80, 0, Late.Start + 300.0) && L.LiveDays.size() == 1 &&
			   L.LiveDays[0].Day == Late.Day,
		"record: the last 400 days");
	// Written once kept, and read back the same.
	ss::SaveData D;
	D.Life = L;
	const std::string Text = D.Serialize();
	ss::SaveData Back;
	Expect(Text.find("life\tlive\t3\t1\t1\t90000\t1\t" + std::to_string(L.LiveSpentCents) + "\n") != std::string::npos && Text.find("life\tliveday\t") != std::string::npos && ss::SaveData::Parse(Text, Back) &&
			   Back.Life.LiveWins == L.LiveWins && Back.Life.LiveSpentCents == L.LiveSpentCents && Back.Life.LiveDays.size() == L.LiveDays.size() && Back.Serialize() == Text,
		"record: life\\tlive's 7th and 8th fields and the days, round trip");
	// An older save: the record from the kept entries; a win older than them counts once from the best place.
	{
		ss::SaveData Old;
		Old.Life.LiveEvents = 70;
		Old.Life.LiveBestPlace = 1;
		life::LiveEntry E;
		E.Id = "embercrest-nightly@40";
		E.State = life::LiveEntry::Finished;
		E.PaidCents = 12000;
		E.Place = 12;
		E.FinishedAt = 40.0 * Day + 22.0 * 60.0;
		for (int K = 0; K < 60; ++K)
		{
			Old.Life.LiveEntries.push_back(E);
		}
		ss::SaveData Read;
		Expect(ss::SaveData::Parse(Old.Serialize(), Read) && Read.Life.LiveWins == 1 && Read.Life.LiveSpentCents == 70 * 12000 && Read.Life.LiveDays.size() == 1,
			"record: an older save's: a title from the best place, the buy-ins estimated for the 70 events");
		Expect(Read.Serialize() == Old.Serialize(), "record: and none of it is written until kept");
	}
	// The last shift: kept by a shift from now on; an older save's from the newest shift line, else the oldest line.
	{
		ss::SaveData Old;
		Old.Life.Shifts = 3;
		Old.Life.Record(500.0, "Rent", -1000, 3);
		Old.Life.Record(900.0, "Lucky Penny #212 shift", 4000, 1);
		Old.Life.Record(1300.0, "Penny Drop", -900, 8);
		ss::SaveData Read;
		Expect(ss::SaveData::Parse(Old.Serialize(), Read) && Read.Life.LastShiftAt == 900.0, "shift: the newest shift line");
		Old.Life.Ledger.erase(Old.Life.Ledger.begin() + 1);
		Expect(ss::SaveData::Parse(Old.Serialize(), Read) && Read.Life.LastShiftAt == 500.0, "shift: no shift line left: the oldest kept line");
		Old.Life.Shifts = 0;
		Expect(ss::SaveData::Parse(Old.Serialize(), Read) && Read.Life.LastShiftAt == -1.0e9, "shift: no shifts: none");
		Read.Life.LastShiftAt = 2222.5;
		Read.Life.SegmentLinesRead |= life::SegShift;
		const std::string Kept = Read.Serialize();
		ss::SaveData Again;
		Expect(Kept.find("life\tshift\t2222.50\n") != std::string::npos && ss::SaveData::Parse(Kept, Again) && Again.Life.LastShiftAt == 2222.5 && Again.Serialize() == Kept,
			"shift: life\\tshift once kept, round trip");
	}
	// A shift through the session keeps it.
	{
		Hooks H;
		ss::Session S(H, "record-shift");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.LobbyMinutes += 2.0 * Day + 7.0 * 60.0 - S.WorldMinutes();
		Now = Wait(S, Now, 0.1);
		S.Life.Energy = 90.0;
		S.Life.Hunger = S.Life.Thirst = 10.0;
		Expect(S.StartActivity("washfold").empty(), "shift: on at the laundromat");
		Now = Wait(S, Now, 6.0);
		const auto Shift = std::find_if(S.Life.Ledger.begin(), S.Life.Ledger.end(), [](const life::LedgerEntry& X) { return X.Kind == 1; });
		Expect(S.Life.Shifts == 1 && Shift != S.Life.Ledger.end() && S.Life.LastShiftAt == Shift->At && (S.Life.SegmentLinesRead & life::SegShift) != 0 &&
				   H.Last.Serialize().find("life\tshift\t") != std::string::npos,
			"shift: the session keeps when it ended");
	}
}

void EntryLines()
{
	life::State L;
	ss::Chips Bank = 100000;
	const live::Occurrence O = live::FindOccurrence("embercrest@6"); // the Sunday keeps its old id
	Expect(O.Valid() && live::Register(Bank, L, O, O.DeskOpens + 5.0, 100, {{"Mei", 4}, {"Big Lou", 3}}).empty(), "lines: registered for the Sunday");
	life::LiveEntry* E = live::EntryFor(L, O.Id);
	if (!E)
	{
		return;
	}
	E->Fund.From = life::Paid::Backer;
	E->Fund.Ref = "lou:6";
	E->Fund.Who = "Big Lou";
	E->Fund.Permille = 1000;
	E->Fund.Cents = 15000;
	life::LiveVoice V;
	V.Name = "Mei";
	V.Style = 4;
	V.Social = 0.35f;
	V.Aggro = 0.62f;
	V.Grit = 0.7f;
	V.Mood = 2;
	V.Is = 1;
	V.Age = 31;
	V.Pro = true;
	V.Rivalry = 0.25f;
	V.Familiarity = 0.5f;
	V.Respect = 0.75f;
	V.Story = "Won the Sunday\tin July.\nTwice.";
	V.Ties = {{"Big Lou", 2}, {"A:B;C\\D", 5}};
	E->Voices.push_back(V);
	E->Room = "dir1 7 3\tfinal=0\nmoney=1";
	E->Badges = {{"Mei", "b:4:1"}, {"Big Lou", "c:2"}};
	life::RoomLeader Ld;
	Ld.Rank = 1;
	Ld.Name = "Mrs. Park";
	Ld.Points = 812.25;
	Ld.Badge = "c:3";
	E->Leaders.push_back(Ld);
	E->Patches = {{"Mei", "voltrush"}, {"grinder_3c", "luckypenny"}};
	life::Letter M;
	M.At = 8000.5;
	M.Thread = "c-luckypenny-1";
	M.Source = "sponsor";
	M.From = "Ray Okafor";
	M.Subject = "The corkboard";
	M.Body = "Line one.\n\tLine two, with a tab and a back\\slash.";
	M.Kind = 2;
	life::Post(L, M);
	ss::SaveData D;
	D.Life = L;
	const std::string Text = D.Serialize();
	ss::SaveData Back;
	const bool Parsed = ss::SaveData::Parse(Text, Back);
	const life::LiveEntry* R = live::EntryFor(Back.Life, O.Id);
	Expect(Parsed && R && R->Fund.From == life::Paid::Backer && R->Fund.Ref == "lou:6" && R->Fund.Who == "Big Lou" && R->Fund.Permille == 1000 && R->Fund.Cents == 15000,
		"lines: livefund");
	Expect(R && R->Voices.size() == 1 && R->Voices[0].Name == "Mei" && R->Voices[0].Story == V.Story && R->Voices[0].Ties == V.Ties && R->Voices[0].Social == 0.35f &&
			   R->Voices[0].Pro && !R->Voices[0].Staked && R->Voices[0].Age == 31,
		"lines: livevoice, its story and ties escaped");
	Expect(R && R->Room == E->Room && R->Badges == E->Badges && R->Leaders.size() == 1 && R->Leaders[0].Name == "Mrs. Park" && R->Leaders[0].Points == 812.25 &&
			   R->Patches == E->Patches,
		"lines: liveroom, livebadge, liveleader, livepatch");
	Expect(Back.Life.Inbox.size() == 1 && Back.Life.Inbox[0].Body == M.Body && Back.Life.Inbox[0].Subject == M.Subject && Back.Life.Inbox[0].Kind == 2 && !Back.Life.Inbox[0].Read,
		"lines: mail");
	Expect(Back.Serialize() == Text, "lines: the save writes back byte for byte");
	// Settle clears the night's snapshot, keeps the player's own patch and the funding.
	Expect(live::Settle(Bank, L, O.Id, 3, 100, 120000, O.Start + 500.0), "lines: settled");
	E = live::EntryFor(L, O.Id);
	Expect(E && E->Voices.empty() && E->Room.empty() && E->Badges.empty() && E->Leaders.empty() && E->Patches.size() == 1 && E->Patches[0].first == "grinder_3c" &&
			   E->Fund.Who == "Big Lou",
		"lines: settle clears the voices, the room, badges, leaders and the world's patches");
	// Mail keeps the last 80, bodies of 600 characters.
	life::State Many;
	for (int K = 0; K < 90; ++K)
	{
		life::Letter X;
		X.Subject = std::to_string(K);
		X.Body = std::string(700, 'a');
		life::Post(Many, X);
	}
	Expect(Many.Inbox.size() == life::InboxKeep && Many.Inbox.front().Subject == "10" && Many.Inbox.back().Body.size() == life::LetterBodyMax, "mail: the last 80, 600 characters a body");
}

void Funding()
{
	life::State L;
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
	const double At = O.DeskOpens + 10.0;
	// Someone else's money is what the books offer now (stake::FundingOptions): Big Lou's standing offer for the Nightly,
	// Team Embercrest's covered entries with the bus, and a package whose roll takes a $120 event.
	ss::stake::Offer Standing;
	Standing.Id = "lou:" + O.Id;
	Standing.K = ss::stake::Kind::Event;
	Standing.Backer = "Big Lou";
	Standing.EntryId = O.Id;
	Standing.Expires = O.LateRegEnds;
	Standing.At = O.DeskOpens;
	Standing.BuyIn = O.T->BuyInCents;
	L.Stakes.Offers.push_back(Standing);
	ss::sponsor::Contract Team;
	Team.Id = "c:embercrest:seams";
	Team.Brand = "embercrest";
	Team.St = ss::sponsor::State::Active;
	Team.Since = At - Day;
	Team.Until = At + 30.0 * Day;
	Team.T.CoveredEntries = 2;
	Team.T.CoveredCapCents = O.T->BuyInCents;
	Team.T.CoversFares = true;
	L.Sponsors.Contracts.push_back(Team);
	ss::stake::Deal Sam;
	Sam.Id = "deal:sam:seams";
	Sam.K = ss::stake::Kind::Package;
	Sam.By = ss::stake::Outfit::Sam;
	Sam.Backer = "Sam";
	Sam.T.CutPermille = 350;
	Sam.T.CapLive = O.T->BuyInCents;
	Sam.T.Roll = 150000;
	Sam.RollNow = Sam.T.Roll;
	Sam.Funded = Sam.T.Roll;
	Sam.Since = At - Day;
	Sam.NextChop = At + 29.0 * Day;
	L.Stakes.Deals.push_back(Sam);
	life::Funding Lou;
	life::Funding Cover;
	life::Funding Roll;
	for (const std::pair<life::Funding, std::string>& Option : ss::stake::FundingOptions(L, O, At))
	{
		if (!Option.second.empty())
		{
			continue;
		}
		switch (Option.first.From)
		{
		case life::Paid::Backer: Lou = Option.first; break;
		case life::Paid::Sponsor: Cover = Option.first; break;
		case life::Paid::Package: Roll = Option.first; break;
		default: break;
		}
	}
	Expect(Lou.Ref == Standing.Id && Lou.Who == "Big Lou" && Cover.Ref == Team.Id && Cover.Fares && Roll.Ref == Sam.Id, "funding: the books offer Lou's stake, the cover and the roll");
	Expect(live::CanRegister(1000, L, O, At) == "The Nightly is $120.00 and the bus." && live::CanRegister(1000, L, O, At, Lou).empty() &&
			   live::CanRegister(400, L, O, At, Lou) == "The bus up the ridge and back is $5.80.",
		"funding: backed, the player needs only the bus");
	Expect(live::CanRegister(0, L, O, At, Cover).empty(), "funding: covered with the fares, nothing");
	// A funding the books don't offer as it stands (a stale screen's, or made up) is refused with the clerk's line, and
	// registering on it moves nothing: a stake that's off, a cover used up or with different terms, a roll that isn't one.
	{
		life::Funding Gone = Lou;
		Gone.Ref = "lou:somewhere-else";
		life::Funding Richer = Lou;
		Richer.Cents = 10 * O.T->BuyInCents;
		life::State Used = L;
		Used.Sponsors.Contracts[0].Now.CoveredUsed = 2;
		life::Funding NoCut = Cover;
		NoCut.Fares = false;
		life::Funding Made = Roll;
		Made.Ref = "deal:nobody";
		life::Funding Ticket;
		Ticket.From = life::Paid::Ticket;
		ss::Chips Bank = 1000000;
		Expect(live::CanRegister(Bank, L, O, At, Gone) == "That stake's off. It's your own money tonight." && live::CanRegister(Bank, L, O, At, Richer) == live::CanRegister(Bank, L, O, At, Gone) &&
				   live::CanRegister(Bank, Used, O, At, Cover) == "Team Embercrest's covered entries are used this month." &&
				   live::CanRegister(Bank, L, O, At, NoCut) == "That cover's off. It's your own money tonight." &&
				   live::CanRegister(Bank, L, O, At, Made) == "The roll isn't paying for this one. It's your own money tonight." && !live::CanRegister(Bank, L, O, At, Ticket).empty(),
			"funding: what the books don't offer now is refused at the desk");
		life::State Stale = L;
		Expect(!live::Register(Bank, Stale, O, At, 80, {}, Gone).empty() && Bank == 1000000 && Stale.LiveEntries.empty() && Stale.Ledger.empty(), "funding: and registering on it moves nothing");
		life::Funding Fares;
		Fares.Fares = true; // the player's own money never covers the bus
		Expect(live::CanRegister(O.T->BuyInCents, L, O, At, Fares) == "The Nightly is $120.00 and the bus.", "funding: only a sponsor's cover pays the bus");
	}
	life::Funding Pieces;
	Pieces.Permille = 350;
	Pieces.Cents = 4400;
	{
		// The pieces' money counts as the book has it: a listing on the Nightly filled for $44.
		life::State Sold;
		Expect(live::CanRegister(O.T->BuyInCents + 2 * live::BusFareCents - 1, Sold, O, At, Pieces) == "The Nightly is $120.00 and the bus.", "funding: pieces nobody bought pay for nothing");
	}
	// A package's roll pays: nothing in the player's ledger; a covered bus: two lines that net to nothing.
	ss::Chips Bank = 1000;
	Expect(live::Register(Bank, L, O, At, 80, {}, Roll).empty() && Bank == 1000 && L.Ledger.empty(), "funding: the package's roll pays, the player's ledger untouched");
	Expect(live::Refund(Bank, L, O.Id, O.Start, "not enough players") && Bank == 1000 && L.Ledger.empty(), "funding: and its refund too");
	life::State L2;
	L2.Sponsors.Contracts.push_back(Team);
	ss::Chips Bank2 = 0;
	Expect(live::Register(Bank2, L2, O, At, 80, {}, Cover).empty(), "funding: covered");
	const size_t Lines = L2.Ledger.size();
	Expect(live::PayFare(Bank2, L2, O.Id, false, At + 1.0) && L2.Ledger.size() == Lines + 2 && L2.Ledger[0].Amount == live::BusFareCents &&
			   L2.Ledger[0].Kind == life::LedgerSponsors && L2.Ledger[1].Amount == -live::BusFareCents,
		"funding: a covered bus: the fare and its cover");
	// FundingLine's copy.
	std::vector<std::pair<std::string, std::string>> Copy;
	life::Funding Own;
	Expect(live::FundingLine(Own, "Nightly").empty(), "funding: nothing to say about the player's own money");
	Copy.push_back({"backer", live::FundingLine(Lou, "Nightly")});
	Copy.push_back({"package", live::FundingLine(Roll, "Sunday")});
	Copy.push_back({"sponsor", live::FundingLine(Cover, "Nightly")});
	Copy.push_back({"pieces", live::FundingLine(Pieces, "Sunday")});
	life::Funding Ticket;
	Ticket.From = life::Paid::Ticket;
	Copy.push_back({"ticket", live::FundingLine(Ticket, "Big Stack")});
	Expect(Copy[0].second == "Big Lou is putting you in the Nightly." && Copy[3].second == "You've sold 35% of your Sunday.", "funding: the lines");
	// The clerk's refusals of someone else's money the books don't offer now: linted with the rest.
	{
		life::State Nothing;
		size_t Refusals = 0;
		for (const life::Paid From : {life::Paid::Backer, life::Paid::Sponsor, life::Paid::Package, life::Paid::Ticket})
		{
			life::Funding Off;
			Off.From = From;
			Off.Ref = "nobody";
			const std::string Said = live::CanRegister(1000000, Nothing, O, At, Off);
			Refusals += Said.empty() ? 0u : 1u;
			Copy.push_back({"refused", Said});
		}
		Expect(Refusals == 4, "funding: each kind of money not on offer is refused in words");
	}
	Failures += copylint::Run("funding", Copy, copylint::Pa);
}

void Rounds()
{
	// Reading Rounds draws nothing: the same night with and without reading it, checkpoint for checkpoint.
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
	std::unique_ptr<ss::Tournament> Read = live::MakeField(O, 90, "Hero", "rounds", {});
	std::unique_ptr<ss::Tournament> Blind = live::MakeField(O, 90, "Hero", "rounds", {});
	ss::Profile AutoA = ss::MakeProfile(ss::Archetype::Tag, Read->R);
	ss::Profile AutoB = ss::MakeProfile(ss::Archetype::Tag, Blind->R);
	bool Same = true;
	bool Shape = true;
	int AllIns = 0;
	int Calls = 0;
	int Outs = 0;
	for (int Tick = 0; Tick < 400 && !Read->bFinished; ++Tick)
	{
		const size_t Tables = Read->Tables.size();
		const std::vector<ss::TEvent> Events = Read->SimulateTick(&AutoA);
		Blind->SimulateTick(&AutoB);
		int Busts = 0;
		for (const ss::TEvent& Ev : Events)
		{
			Busts += Ev.Type == ss::TEventType::Bust ? 1 : 0;
		}
		int Out = 0;
		for (const ss::TableRound& R : Read->Rounds())
		{
			Shape = Shape && R.BigBlind > 0 && R.Pot > 0 && (!R.Called || (R.AllIn && R.Showdown));
			AllIns += R.AllIn ? 1 : 0;
			Calls += R.Called ? 1 : 0;
			Out += static_cast<int>(R.Out.size());
		}
		Shape = Shape && Read->Rounds().size() <= Tables && Out == Busts;
		Outs += Out;
		Same = Same && Read->Checkpoint() == Blind->Checkpoint();
	}
	Expect(Same, "rounds: reading them changes nothing");
	Expect(Shape && AllIns > 0 && Calls > 0 && Outs > 0, "rounds: a round per table played, all-ins, calls and the busts each tick");
	std::printf("  rounds: %d all-ins, %d called, %d out\n", AllIns, Calls, Outs);
}

void Settings()
{
	using ss::ui::GameSettings;
	const GameSettings Defaults;
	GameSettings Today;
	// What c09bc31 wrote: no key of the segment's.
	const std::string Old = "shortstack.settings.v1\nquality=2\nmaster=75\ntablepace=1\nresolution=\n";
	Expect(GameSettings::Parse(Old, Today) && Today.VoicesVolume == 100 && Today.Captions == 1 && Today.CaptionSize == 100 && Today.CaptionBackdrop == 72 && Today.SpeakerNames &&
			   Today.TableTalk == 1 && Today.StackReadout == 2 && Today.StackUnits == 0 && Today.TurnCue == 2 && Today.HudScale == 100 && Today.Quality == 2,
		"settings: today's string reads to the defaults");
	GameSettings Mine = Defaults;
	Mine.VoicesVolume = 40;
	Mine.Captions = 2;
	Mine.CaptionSize = 150;
	Mine.CaptionBackdrop = 30;
	Mine.SpeakerNames = false;
	Mine.TableTalk = 0;
	Mine.StackReadout = 1;
	Mine.StackUnits = 1;
	Mine.TurnCue = 0;
	Mine.HudScale = 120;
	GameSettings Back;
	Expect(GameSettings::Parse(Mine.Serialize(), Back) && Back.Serialize() == Mine.Serialize() && Back.VoicesVolume == 40 && Back.HudScale == 120 && !Back.SpeakerNames,
		"settings: a full string round-trips");
	GameSettings Wild;
	Expect(GameSettings::Parse("shortstack.settings.v1\nvoices=400\ncaptions=9\ncaptionsize=10\nhudscale=500\nstackunits=-3\nnewkey=1\n", Wild) && Wild.VoicesVolume == 100 &&
			   Wild.Captions == 2 && Wild.CaptionSize == 100 && Wild.HudScale == 130 && Wild.StackUnits == 0,
		"settings: clamped, and an unknown key ignored");
}

void SessionSeams()
{
	Hooks H;
	ss::Session S(H, "seams");
	S.CurrentScreen = ss::Screen::Lobby;
	Wait(S, 0.0, 0.2);
	Expect(std::string(ss::SoundName(ss::SoundId::Mail)) == "mail", "seams: the Mail sound has a name");
	const std::vector<std::pair<life::Funding, std::string>> Options = S.FundingOptions("embercrest-nightly@9");
	Expect(Options.size() == 1 && life::IsOwnMoney(Options[0].first) && Options[0].second.empty(), "seams: until backing lands, the player's own money");
	Expect(S.Marks(0.0, 60.0 * Day).empty() && S.HeroBadges().Held.empty() && S.Unseen().empty() && S.SponsorTier() == 0 && !S.TeamRiverLine(), "seams: no marks, badges or sponsors yet");
	Expect(!S.AcceptStake("x").empty() && !S.ListAction("embercrest-nightly@9", 300, 1050).empty() && !S.SignDeal("nope").empty(), "seams: the actions are refused for now");
	Expect(std::fabs(ss::town::Renown(S) - S.Record().Renown) < 1e-12 && S.Record().Renown < 0.1, "seams: renown is the public record's");
	// The Burner's Embercrest row asks who'd pay the way GoToLive does; until backing lands, the player's own money, so
	// the row reads as it did, broke or not.
	{
		const life::Activity* Room = life::Find("embercrest");
		life::Context Ctx = S.LifeContext();
		Expect(Room && Ctx.LiveFunding && life::IsOwnMoney(Ctx.LiveFunding("embercrest-nightly@9")), "seams: the life context carries GoToLive's funding");
		bool Same = Room != nullptr;
		for (const ss::Chips Bank : {ss::Chips(0), ss::Chips(500), ss::Chips(13000), ss::Chips(100000)})
		{
			for (const double At : {2.0 * Day + 18.0 * 60.0, 3.0 * Day + 11.0 * 60.0, 6.0 * Day + 19.5 * 60.0})
			{
				Ctx.Bankroll = Bank;
				Ctx.World = At;
				life::Context Plain = Ctx;
				Plain.LiveFunding = nullptr;
				Same = Same && life::Blocked(*Room, S.Life, Ctx) == life::Blocked(*Room, S.Life, Plain);
			}
		}
		Expect(Same, "seams: the Embercrest row's why-not is today's");
	}
	Expect(std::string(ss::rep::Mood(0.1)) == "Sounds open to it" && std::string(ss::rep::Mood(-0.1)) == "That's a stretch" && std::string(ss::rep::Mood(-0.5)) == "Don't push it",
		"seams: the three moods");
}

void RenownKept()
{
	// A career past sixty live entries whose title is older than the entries kept: the town keeps c09bc31's count (the
	// wall is the wins among the kept entries), while the public record keeps the title for life (§1.1).
	ss::SaveData Old;
	Old.Life.LiveEvents = 70;
	Old.Life.LiveCashes = 9;
	Old.Life.LiveBestPlace = 1;
	life::LiveEntry E;
	E.Id = "embercrest-nightly@40";
	E.State = life::LiveEntry::Finished;
	E.PaidCents = 12000;
	E.Place = 12;
	E.FinishedAt = 40.0 * Day + 22.0 * 60.0;
	for (int K = 0; K < 60; ++K)
	{
		Old.Life.LiveEntries.push_back(E);
	}
	ss::SaveData Read;
	const bool Parsed = ss::SaveData::Parse(Old.Serialize(), Read);
	Hooks H;
	ss::Session S(H, "renown-kept", &Read);
	int Wall = 0;
	for (const life::LiveEntry& X : S.Life.LiveEntries)
	{
		Wall += X.State == life::LiveEntry::Finished && X.Place == 1 ? 1 : 0;
	}
	const double Followers = std::min(1.0, std::log10(1.0 + static_cast<double>(std::max(0, S.Channel.Followers))) / 5.0);
	const double Fans = std::min(1.0, static_cast<double>(S.Channel.Superfans()) / 40.0);
	const double Cashes = std::min(1.0, static_cast<double>(S.Life.LiveCashes) / 15.0);
	const double Wins = std::min(1.0, static_cast<double>(Wall) / 4.0);
	const double Awards = std::min(1.0, static_cast<double>(S.Living().HeroAwards().size()) / 2.0);
	const double Before = std::min(1.0, 0.35 * Followers + 0.15 * Fans + 0.15 * Cashes + 0.15 * Wins + 0.2 * Awards);
	const ss::rep::Public R = S.Record();
	Expect(Parsed && Wall == 0 && R.Titles == 1 && ss::town::Renown(S) == Before && R.Renown > Before,
		"renown: past sixty entries the town's is c09bc31's (the wall on the kept entries); the public record keeps the title");
}
void Wave1a()
{
	// The player's birth year reaches the world (World::HeroBorn, not saved): the Hall of Fame's age rule needs it. Set when
	// the world is made, when the creator names a new career's age, and when a save is read.
	{
		Hooks H;
		ss::Session S(H, "born");
		const int Began = ss::world::YearOf(ss::net::DayOf(S.Living().StartedAt()));
		const bool Fresh = S.Living().HeroBorn == Began - S.Person.Age;
		ss::hero::Character Who;
		Who.FirstName = "Ada";
		Who.LastName = "Reyes";
		Who.Age = 45;
		S.NewCareer(Who);
		const int Born = S.Living().HeroBorn;
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(H.Last.Serialize(), D);
		Hooks H2;
		ss::Session Back(H2, "born", &D);
		Expect(Fresh && Born == ss::world::YearOf(ss::net::DayOf(S.Living().StartedAt())) - 45 && Parsed && Back.Living().HeroBorn == Born,
			"wave 1a: the world knows the player's birth year (new, created, loaded)");
	}
	// An online event the package's roll pays for can be joined on a short bankroll (Session::CanAfford): the roll pays it.
	{
		Hooks H;
		ss::Session S(H, "roll-affords");
		S.CurrentScreen = ss::Screen::Lobby;
		const ss::stake::Terms Tier1 = ss::stake::LanternRowTerms(1);
		const ss::LobbyEvent* Ev = nullptr;
		for (const ss::LobbyEvent& X : ss::Lobby())
		{
			Ev = !Ev && X.Joinable && X.BuyInCents > 0 && X.BuyInCents <= Tier1.CapOnline ? &X : Ev;
		}
		S.BankrollCents = 0;
		const bool Broke = Ev && !S.CanAfford(*Ev);
		ss::stake::Deal Roll;
		Roll.Id = "deal:lanternrow:seams";
		Roll.K = ss::stake::Kind::Package;
		Roll.By = ss::stake::Outfit::LanternRow;
		Roll.Backer = "Lantern Row Staking";
		Roll.T = Tier1;
		Roll.Tier = 1;
		Roll.Since = S.WorldMinutes();
		Roll.NextChop = S.WorldMinutes() + 30.0 * Day;
		Roll.RollNow = Tier1.Roll;
		Roll.Funded = Tier1.Roll;
		S.Life.Stakes.Deals.push_back(Roll);
		Expect(Ev && Broke && S.CanAfford(*Ev), "wave 1a: the package's roll lets a broke player into an event in its range");
	}
	// A freeroll is on the player's stats page and nowhere in the public record (S9): the count, the ROI and the ABI that
	// backers price pieces on are the paid events'. One network freeroll cashed for $50, one $11 event cashed for $33.
	{
		Hooks H;
		ss::Session S(H, "freeroll-record");
		const double Now = S.WorldMinutes();
		const ss::net::Network& Net = ss::net::Shared();
		std::string Free;
		std::string Paid;
		ss::Chips PaidBuyIn = 0;
		for (const ss::net::EventInstance& E : Net.Window(Now - 12.0 * 60.0, Now + 24.0 * 60.0))
		{
			const ss::Chips BuyIn = Net.TemplateOf(E).BuyInCents;
			Free = Free.empty() && BuyIn == 0 ? E.Id : Free;
			if (Paid.empty() && BuyIn >= 500 && BuyIn <= 1100)
			{
				Paid = E.Id;
				PaidBuyIn = BuyIn;
			}
		}
		ss::world::World& W = S.Living();
		for (const std::string& Id : {Free, Paid})
		{
			W.HeroEntered(Id, std::string());
			W.HeroFinished(Id, Id == Free ? 40 : 12, Id == Free ? 5000 : 3300, ss::world::World::TableReport());
		}
		const ss::world::Tracker& Page = W.HeroStats();
		const ss::rep::Public Rec = S.Record();
		Expect(!Free.empty() && !Paid.empty() && Page.Events == 2 && Page.Freerolls == 1 && Rec.OnlineEvents == 1 && Rec.Abi == PaidBuyIn && Page.PaidAbi() == PaidBuyIn &&
				   std::fabs(Rec.OnlineRoi - Page.PaidRoi()) < 1e-9 && std::fabs(Page.Roi() - Page.PaidRoi()) > 0.1,
			"wave 1a: a freeroll stays out of the public record's count, ROI and ABI");
	}
	// The CONTINUE card's marks are the career's (FrontEnd's DescribeSession, ui-laptop's patch, §5.6): nothing on a new
	// career, the club's tier once the room's visits earn it.
	{
		Hooks H;
		ss::Session S(H, "continue-marks");
		const ss::ui::FrontEndInfo Before = ss::ui::DescribeSession(S, true);
		S.DebugBadge("grant club 2");
		const ss::ui::FrontEndInfo After = ss::ui::DescribeSession(S, true);
		Expect(Before.ClubTier == 0 && Before.TopBadge.K == ss::badge::Kind::Count && After.ClubTier == 2, "wave 1a: the CONTINUE card's club tier is the career's");
	}
}

/** The checkpoint host text's lines (one per line, split on tabs). */
std::vector<std::vector<std::string>> HostLines(const std::string& Text)
{
	std::vector<std::vector<std::string>> Out;
	size_t At = 0;
	while (At < Text.size())
	{
		size_t End = Text.find('\n', At);
		End = End == std::string::npos ? Text.size() : End;
		std::vector<std::string> F(1);
		for (size_t K = At; K < End; ++K)
		{
			if (Text[K] == '\t')
			{
				F.emplace_back();
			}
			else
			{
				F.back().push_back(Text[K]);
			}
		}
		if (End > At)
		{
			Out.push_back(F);
		}
		At = End + 1;
	}
	return Out;
}

/** A hand the skip played, into the run (the card room host's sink, as fixture_tool's and G1Journey's). */
struct GateSink : ss::live::SkipSink
{
	ss::autoplay::Run* R = nullptr;
	ss::style::HeroBrain* Brain = nullptr;
	const ss::autoplay::TableAuto* Plan = nullptr;
	const ss::style::Learned* Book = nullptr;
	int Hands = 0;
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		ss::autoplay::NoteSkippedHand(*R, H, HeroSeat, 1.0);
		*Brain = ss::autoplay::BrainFor(*Plan, *R, 1.0, 1, nullptr, Book);
		++Hands;
	}
	void Events(const std::vector<ss::TEvent>&) override {}
	void Checkpoint(int) override {}
};

void Gate1Fixtures()
{
	namespace autoplay = ss::autoplay;
	const char* Names[] = {"update2_blank_world.ssave", "update2_auto_mid_nightly.ssave"};
	for (const char* Name : Names)
	{
		const std::string Saved = Fixture(Name);
		ss::SaveData D;
		const bool Parsed = Saved.rfind("shortstack.nightone.v2\n", 0) == 0 && ss::SaveData::Parse(Saved, D);
		Expect(Parsed, (std::string("gate 1 fixtures: ") + Name + " is there and parses").c_str());
		const std::string Again = D.Serialize();
		ss::SaveData Second;
		Expect(Again == Saved && ss::SaveData::Parse(Again, Second) && Second.Serialize() == Again,
			(std::string("gate 1 fixtures: ") + Name + " writes back byte for byte and loads a second time the same").c_str());
		std::printf("  gate 1 fixtures: %s: %s (%zu bytes)\n", Name, Again == Saved ? "written back byte for byte" : "NOT written back byte for byte", Again.size());
	}
	// The fresh start, four days in: the world it began with comes back Blank (its world lines are in the save), its boards
	// have filled from nothing, and Career reads the plan's events as PIE §8.1 step 3 expects.
	{
		ss::SaveData D;
		ss::SaveData::Parse(Fixture("update2_blank_world.ssave"), D);
		Hooks H;
		ss::Session S(H, "gate1-blank", &D);
		const std::string Line = S.DebugWorld("");
		int Team = 0;
		for (const ss::world::Npc& N : S.Living().People())
		{
			Team += N.Pro ? 1 : 0;
		}
		Expect(S.WorldStart() == ss::world::Start::Blank && S.Living().Began() == ss::world::Start::Blank && Team == 0 &&
				   Line.find("World: Fresh start, version " + std::to_string(ss::world::Version)) == 0,
			("gate 1 fixtures: the blank world loads Blank, version " + std::to_string(ss::world::Version) + ", no team yet (" + Line.substr(0, Line.find('\n')) + ")").c_str());
		ss::net::HeroStats Hero;
		Hero.Name = S.HeroName;
		const std::vector<ss::net::BoardRow> Season = ss::net::Shared().Leaderboard(ss::net::Board::Season, S.WorldMinutes(), Hero, 60, nullptr);
		const std::vector<ss::net::BoardRow> Earnings = ss::net::Shared().Leaderboard(ss::net::Board::Earnings, S.WorldMinutes(), Hero, 60, nullptr);
		bool Positive = !Season.empty() && !Earnings.empty();
		for (const ss::net::BoardRow& R : Season)
		{
			Positive = Positive && R.Value > 0.0;
		}
		for (const ss::net::BoardRow& R : Earnings)
		{
			Positive = Positive && R.Value > 0.0;
		}
		Expect(ss::net::Shared().Attached() == &S.Living() && Positive, "gate 1 fixtures: four days in, the Season and Earnings boards have filled, with no $0 rows");
		// The played lines: an event the plan played whole (Career's AUTO tag, the accuracy "—"), one shared with the player
		// (the results line), and the Nightly it skipped.
		const autoplay::Played* Whole = nullptr;
		const autoplay::Played* Shared = nullptr;
		const autoplay::Played* Night = nullptr;
		for (const autoplay::Played& P : S.Life.Auto.Results)
		{
			Whole = !Whole && P.Venue == 0 && P.OwnDecisions == 0 && P.S != ss::style::Style::Autopilot ? &P : Whole;
			Shared = !Shared && P.Venue == 0 && P.OwnDecisions > 0 && P.AiDecisions > 0 ? &P : Shared;
			Night = !Night && P.Venue == 1 ? &P : Night;
		}
		bool WholeDash = false;
		for (const ss::HistoryEntry& E : S.History)
		{
			WholeDash = WholeDash || (Whole && E.EventId == Whole->Id && E.AccuracyPct == 0.0);
		}
		Expect(Whole && WholeDash && autoplay::CareerTag(*Whole).rfind("AUTO \xC2\xB7 ", 0) == 0 && autoplay::CareerTag(*Whole).find("100%") != std::string::npos,
			"gate 1 fixtures: an event the plan played whole reads AUTO at 100%, with no accuracy of the player's (the \xE2\x80\x94)");
		Expect(Shared && autoplay::PlayedLine(*Shared).rfind("Played by your plan: " + std::to_string(Shared->AiDecisions) + " of " +
														   std::to_string(Shared->AiDecisions + Shared->OwnDecisions) + " decisions",
							 0) == 0,
			("gate 1 fixtures: the shared event's results line (" + (Shared ? autoplay::PlayedLine(*Shared) : std::string("none")) + ")").c_str());
		const ss::life::LiveEntry* E = Night ? live::EntryFor(S.Life, Night->Id) : nullptr;
		Expect(Night && E && E->State == ss::life::LiveEntry::Finished && Night->AiHands > 0 && Night->OwnDecisions == 0, "gate 1 fixtures: the Nightly the plan skipped, settled and on the record");
		Expect((S.Life.Update2LinesRead & (ss::life::U2AutoPref | ss::life::U2AutoPlayed | ss::life::U2Notes)) == (ss::life::U2AutoPref | ss::life::U2AutoPlayed | ss::life::U2Notes) &&
				   S.Life.Auto.Pref.Form > autoplay::FormStart && !S.Life.Auto.Notes.empty(),
			"gate 1 fixtures: the player's own decisions moved Form; the notes are kept");
	}
	// The checkpointed Nightly: the game goes back to it, the host text carries the plan, and the plan carries on from it.
	{
		ss::SaveData D;
		ss::SaveData::Parse(Fixture("update2_auto_mid_nightly.ssave"), D);
		Hooks H;
		ss::Session S(H, "gate1-auto", &D);
		const std::string Id = "embercrest-nightly@9";
		const ss::life::LiveEntry* E = live::EntryFor(S.Life, Id);
		Expect(S.LiveInProgress() == Id && E && E->State == ss::life::LiveEntry::Registered && E->Checkpoint.rfind("ck1 ", 0) == 0 && !E->Room.empty(),
			"gate 1 fixtures: tonight's Nightly is there to go back to, checkpointed, the room's state kept");
		if (!E)
		{
			return;
		}
		autoplay::TableAuto Plan;
		autoplay::Run Run;
		ss::style::Learned Book;
		int Autos = 0;
		int Seen = 0;
		int Reads = 0;
		int Hands = -1;
		bool Parsed = true;
		for (const std::vector<std::string>& F : HostLines(E->CheckpointHost))
		{
			std::string Joined;
			for (size_t K = 0; K < F.size(); ++K)
			{
				Joined += (K ? "\t" : "") + F[K];
			}
			if (F[0] == "auto")
			{
				++Autos;
				Parsed = autoplay::ReadRun(Joined, Plan, Run) && Parsed;
			}
			else if (F[0] == "autoseen" && F.size() >= 17)
			{
				++Seen;
				ss::reads::Stats St;
				Parsed = ss::reads::Parse(Joined.substr(Joined.find('\t', 9) + 1), St) && Parsed;
				Run.Seen[F[1]] = St;
			}
			else if (F[0] == "autoread" && F.size() >= 3)
			{
				++Reads;
				Book.Tells[F[1]] = std::atoi(F[2].c_str());
			}
			else if (F[0] == "hands" && F.size() == 2)
			{
				Hands = std::atoi(F[1].c_str());
			}
		}
		// The read book the night seated with: the learned tells (seen twice or more), by name.
		std::map<std::string, int> Learned;
		for (const auto& Kv : S.Life.Reads)
		{
			const size_t Slash = Kv.first.find('/');
			if (Kv.second >= 2 && Slash != std::string::npos)
			{
				++Learned[Kv.first.substr(0, Slash)];
			}
		}
		Expect(Parsed && Autos == 1 && Seen > 0 && Reads > 0 && Book.Tells == Learned && Hands == Run.AiHands + Run.OwnHands,
			"gate 1 fixtures: the host text's auto, autoseen and autoread lines read back (the book as seated: the tells seen twice)");
		Expect(Plan.S == ss::style::Style::Gto && Plan.M == autoplay::Mode::Skip && Plan.To == autoplay::Target::Bubble && Run.AiHands > 0 && Run.AiDecisions >= Run.AiHands &&
				   Run.OwnDecisions == 0 && Run.Sharp > 0.80 && Run.Sharp <= 1.0,
			"gate 1 fixtures: the plan was GTO, skipping to the bubble, with its Sharpness and its counts");
		Expect(autoplay::WasPlayingLine(Plan.S) == "Your plan was playing: GTO. [K] carry on.", "gate 1 fixtures: the line the room shows when the night comes back");
		// The engine's checkpoint restores, and the plan skips on from it to the bubble (or the night's end) with the run as kept.
		const live::Occurrence O = live::FindOccurrence(Id);
		std::vector<ss::ReservedPlayer> Known;
		for (const std::pair<std::string, int>& R : E->Roster)
		{
			ss::ReservedPlayer P;
			P.Name = R.first;
			P.Type = static_cast<ss::Archetype>(R.second);
			Known.push_back(P);
		}
		bool Restored = false;
		std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, S.HeroName, Known, true, Restored);
		const bool Same = Restored && T && T->Checkpoint() == E->Checkpoint;
		ss::style::HeroBrain Brain = autoplay::BrainFor(Plan, Run, 1.0, 1, nullptr, &Book);
		GateSink Sink;
		Sink.R = &Run;
		Sink.Brain = &Brain;
		Sink.Plan = &Plan;
		Sink.Book = &Book;
		const int Before = Run.AiHands;
		ss::live::SkipWhy Why = ss::live::SkipWhy::Budget;
		for (int Guard = 0; Same && Guard < 2000 && Why == ss::live::SkipWhy::Budget; ++Guard)
		{
			Why = ss::live::SkipSlice(*T, Brain, Plan, 0.0, Sink).Why;
		}
		std::printf("  gate 1 fixtures: tonight resumed at hand %d, the plan skipped %d more hands (why %d)\n", Same ? T->Tick - Sink.Hands : -1, Sink.Hands, static_cast<int>(Why));
		Expect(Same && Why != ss::live::SkipWhy::Budget && Sink.Hands > 0 && Run.AiHands == Before + Sink.Hands,
			"gate 1 fixtures: the engine checkpoint restores and the plan skips on from it, its counts carried");
	}
}
} // namespace seams

void SegLead()
{
	seams::SaveFixtures();
	seams::LiveRecord();
	seams::EntryLines();
	seams::Funding();
	seams::Rounds();
	seams::Settings();
	seams::SessionSeams();
	seams::RenownKept();
	seams::Wave1a();
	seams::Gate1Fixtures();
}
} // namespace session_test
