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
//   - Gate2Fixtures (update 2 C2, SPEC §6.5): update2_creator (fixture_tool write-g2) parses, writes back byte for byte,
//     and holds what §6.5 and PIE §8.2 read: the videos, the tails and stubs, the open candidates (AUTO, the player's own,
//     the Embercrest's, Dee's storytime), Partner, a payout and a balance; it plays on through the calendar and takes
//     §8.2's four fakes.
//   - Gate3Fixtures (update 2 C3, SPEC §6.5): update2_multisite (fixture_tool write-g3) parses, writes back byte for byte, and
//     holds what PIE §8.3 reads: accounts on both rivals (deposits, fees, wallets that add up), a withdrawal in transit that
//     lands once through the calendar (across a save and a load too), RunItTwice's tickets, rival results, Rewards' boost,
//     and Team RiverLine under the clause with one strike.
//   - StreamLinks (update 2 C2, the g2 audit's S3): a hand's stream moments (SkipMoments' on the skip path, the player's
//     knockouts on both) fire before its Rerun candidate is cut, so the candidate links them; a streamed bounty event's
//     knockouts are each on stream once, linked to their own hands.
//   - RivalSites (update 2 B3, SPEC §5.5, §5.8, §5.9): the session's seams for the rival sites: the networks on the game
//     thread and never left on a gone world; accounts, wallets and RiverLine-only restrictions; no bank line for a rival's
//     ticket or seat; the sitting's net and Dee's "broke" counting the sites' money; a rival's field (members only, the
//     buy-in's stakes, the rival only with an account); with core-sites' and core-deals' bodies, the wallet's buy-in and
//     prize and the clause's strike when another site's table comes to the front on stream.
// Phase C's MigrationAll (§7.5) builds on SaveFixtures. A feature whose migration writes lines on load (§7.5 steps 2 to 4)
// keeps SaveFixtures green: only the segment's own lines (and Kast's legacy deal lines) may change, and that's reported.
#include "CopyLint.h"
#include "SessionSeg.h"

#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/FrontEnd.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <set>
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
	// (Every update2_* fixture, update 2 §6.5: gate fixtures written by this build's code, so byte for byte with every line kept.
	// C1's two, C2's creator, C3's multi-site career.)
	const char* Names[] = {"pre_segment_fresh.ssave", "pre_segment_mid_nightly.ssave", "pre_segment_team_riverline.ssave", "pre_segment_evicted.ssave", "update2_blank_world.ssave",
		"update2_auto_mid_nightly.ssave", "update2_creator.ssave", "update2_multisite.ssave"};
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

// ------------------------------------------------------------------ update 2 C2: the gate's fixture

/**
 * Gate2Fixtures (update 2 C2, SPEC §6.5, for PIE §8.2): the gate's committed save (fixture_tool write-g2) parses, writes back
 * byte for byte, loads a second time the same, and holds what §6.5 asks of it and §8.2 step 2 opens: an Established career
 * with a Kast channel; the 60 newest videos whole, of several kinds, formats, title styles and frames; the older ones folded
 * into the tails and the evergreen ones under a year old as stubs; open candidates the plan played (AUTO), the player's own,
 * the Embercrest's and Dee's (storytime); the partner gate crossed (the review and the welcome in Mail), a payout paid (its
 * key and its bank line, kind 13) and a balance accruing. In a session: the Channel's view adds up (its 28 days never more
 * than the views to date), the calendar plays it on a week with its views and the save round-trips; §8.2 step 2's four
 * fakes go into Studio with their chips.
 */
void Gate2Fixtures()
{
	namespace video = ss::video;
	const char* Name = "update2_creator.ssave";
	const std::string Saved = Fixture(Name);
	ss::SaveData D;
	const bool Parsed = Saved.rfind("shortstack.nightone.v2\n", 0) == 0 && ss::SaveData::Parse(Saved, D);
	Expect(Parsed, "gate 2 fixtures: update2_creator.ssave is there and parses");
	if (!Parsed)
	{
		return;
	}
	const std::string Again = D.Serialize();
	ss::SaveData Second;
	Expect(Again == Saved && ss::SaveData::Parse(Again, Second) && Second.Serialize() == Again,
		"gate 2 fixtures: update2_creator.ssave writes back byte for byte and loads a second time the same");
	std::printf("  gate 2 fixtures: %s: %s (%zu bytes)\n", Name, Again == Saved ? "written back byte for byte" : "NOT written back byte for byte", Again.size());
	const video::Channel& C = D.Life.Video;
	std::set<int> Kinds, Formats, Styles, Frames;
	for (const video::Video& V : C.Videos)
	{
		Kinds.insert(static_cast<int>(V.K));
		Formats.insert(static_cast<int>(V.F));
		Styles.insert(static_cast<int>(V.T));
		Frames.insert(static_cast<int>(V.Fr));
	}
	bool Tail = false;
	for (const video::Tail& T : C.Tails)
	{
		Tail = Tail || T.Total[0] > 0.0;
	}
	const double World = D.ClockMinutes + ss::net::MinutesPerDay * ss::net::NightOneDay;
	bool Young = !C.Stubs.empty();
	for (const video::Stub& S : C.Stubs)
	{
		Young = Young && World - S.At < 365.0 * ss::net::MinutesPerDay && S.E > 0.0;
	}
	bool Auto = false, Own = false, Live = false, Story = false;
	for (const video::Candidate& X : C.Open)
	{
		Auto = Auto || (X.Auto && X.Source != 2);
		Own = Own || (X.Source == 0 && !X.Auto);
		Live = Live || (X.Source == 1 && X.Id.rfind("lv:", 0) == 0);
		Story = Story || (X.Source == 2 && X.Storytime && X.Id.rfind("dee:", 0) == 0);
	}
	std::string Key;
	for (const std::string& K : C.Paid)
	{
		Key = K.rfind("pay:", 0) == 0 ? K : Key;
	}
	const ss::life::LedgerEntry* Paid = nullptr;
	for (const ss::life::LedgerEntry& E : D.Life.Ledger)
	{
		Paid = E.Kind == ss::life::LedgerVideo && E.Amount >= 10000 && E.Label.rfind("Rerun payout: ", 0) == 0 ? &E : Paid;
	}
	int Review = 0, Welcome = 0;
	for (const ss::life::Letter& M : D.Life.Inbox)
	{
		Review += M.Source == "video" && M.Kind == video::LetterReview ? 1 : 0;
		Welcome += M.Source == "video" && M.Kind == video::LetterWelcome ? 1 : 0;
	}
	std::printf("  gate 2 fixtures: the creator on %s: %zu videos whole (%zu kinds, %zu formats, %zu title styles, %zu frames), %zu stubs, %zu open candidates, %.0f "
				"subscribers, Partner since %s, paid %s (%s), balance %s\n",
		ss::net::DateLabel(ss::net::DayOf(World)).c_str(), C.Videos.size(), Kinds.size(), Formats.size(), Styles.size(), Frames.size(), C.Stubs.size(), C.Open.size(), C.Subs,
		C.PartnerAt >= 0.0 ? ss::net::DateLabel(ss::net::DayOf(C.PartnerAt)).c_str() : "never", ss::Money(C.PaidCents).c_str(), Key.c_str(), ss::Money(C.BalanceCents).c_str());
	Expect((D.Life.Update2LinesRead & ss::life::U2Video) != 0 && D.WorldText.empty() && C.Videos.size() == 60 && Kinds.size() >= 6 && Formats.size() >= 3 && Styles.size() >= 4 &&
			   Frames.size() >= 4,
		"gate 2 fixtures: the 60 newest videos whole, of six kinds or more, three formats and four title styles and frames (no world lines: an Established world)");
	Expect(Tail && Young && C.Stubs.size() >= 5, "gate 2 fixtures: the older videos folded into the tails, the evergreen ones under a year old kept as stubs");
	Expect(Auto && Own && Live && Story && C.Open.size() <= 30, "gate 2 fixtures: open candidates the plan played (AUTO), the player's own, the Embercrest's and Dee's (storytime)");
	Expect(C.ReviewAt >= 0.0 && C.PartnerAt > C.ReviewAt && Review == 1 && Welcome == 1, "gate 2 fixtures: the partner gate crossed: the review, then the welcome, in Mail");
	Expect(!Key.empty() && Paid && C.PaidCents >= Paid->Amount && C.BalanceCents > 0, "gate 2 fixtures: a payout paid (its key, its bank line, kind 13) and a balance accruing");
	// In a session: the Kast channel, the Channel page's view, a week on through the calendar, the save.
	Hooks H;
	ss::Session S(H, "gate2-creator", &D);
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	const video::ChannelView V = S.VideoView();
	const double Total = video::TotalViews(S.Life.Video, S.WorldMinutes());
	double Chart = 0.0;
	for (const double X : V.ViewsByDay)
	{
		Chart += X;
	}
	std::printf("  gate 2 fixtures: in a session: %d Kast followers over %d streams; the Channel page: %.0f subscribers, %.0f views in 28 days (the chart's days %.0f) of "
				"%.0f to date\n",
		S.Channel.Followers, S.Channel.Streams, V.Subs, V.Views28, Chart, Total);
	Expect(S.WorldStart() == ss::world::Start::Established && S.Channel.Followers > 0 && S.Channel.Streams > 0, "gate 2 fixtures: an Established career with a Kast channel");
	Expect(V.Partner && V.Subs >= 1000.0 && V.Views28 > 0.0 && V.Views28 <= Total && Chart <= Total + 0.5,
		"gate 2 fixtures: the Channel page reads Partner, and its 28 days never add up to more than the views to date");
	const double Subs0 = video::SubsAt(S.Life.Video, S.WorldMinutes());
	const double Views0 = Total;
	Now = Walk(S, Now, S.WorldMinutes() + 7.0 * Day);
	S.Save();
	ss::SaveData Back;
	const bool Round = ss::SaveData::Parse(H.Last.Serialize(), Back) && Back.Serialize() == H.Last.Serialize();
	Expect(Round && video::TotalViews(S.Life.Video, S.WorldMinutes()) > Views0 && video::SubsAt(S.Life.Video, S.WorldMinutes()) >= Subs0 && S.Life.Video.SteppedTo > World,
		"gate 2 fixtures: a week on, the channel's views and subscribers grew through the calendar, and the save round-trips");
	// PIE §8.2 step 2: the four fakes in Studio, with their chips.
	Hooks Hf;
	ss::SaveData F;
	ss::SaveData::Parse(Saved, F);
	ss::Session P(Hf, "gate2-creator-fakes", &F);
	P.CurrentScreen = ss::Screen::Lobby;
	Wait(P, 0.0, 0.2);
	const size_t Before = P.Life.Video.Open.size();
	bool Faked = true;
	for (const char* Args : {"fake badbeat online", "fake bluff embercrest", "fake bigpot skip", "fake cooler dees"})
	{
		Faked = P.DebugVideo(Args).rfind("Faked ", 0) == 0 && Faked;
	}
	// (The dev command's ids: "rl:fake@<minute>:<kind>" online and skipped, "lv:fake@..." at the Embercrest, Dee's numbered 500 on.)
	int Fakes = 0;
	bool Chips = true;
	for (const video::Candidate& X : P.VideoCandidates())
	{
		const size_t Cut = X.Id.rfind(':');
		const bool Fake = X.Id.find("fake@") != std::string::npos || (X.Id.rfind("dee:", 0) == 0 && Cut != std::string::npos && std::atoi(X.Id.c_str() + Cut + 1) >= 500);
		Fakes += Fake ? 1 : 0;
		Chips = Chips && (!Fake || ((X.Source != 3 || X.Auto) && (X.Source != 2 || X.Storytime) && (X.Source != 1 || X.Id.rfind("lv:", 0) == 0)));
	}
	std::printf("  gate 2 fixtures: Studio after the four fakes: %zu candidates (%zu before)\n", P.Life.Video.Open.size(), Before);
	Expect(Faked && Fakes >= 4 && Chips && P.Life.Video.Open.size() <= 30, "gate 2 fixtures: PIE's four fakes go into Studio (the skipped hand AUTO, Dee's storytime)");
}

// ------------------------------------------------------------------ update 2 C3: the gate's fixture

/**
 * The copy pass on letters as the game wrote them (update 2 §1.8, §6.7, C3's trademark pass): every body passes the copy lint
 * (a letter's 600 characters); no subject or body names a real site, wallet or video platform, one of the ten sites that
 * closed in the world's own story, a real format's name ("Flip & Go"), or the rivals' old names; and none says the game
 * played for anyone ("AI", "bot", "autoplay": U1). Returns the problems (printed).
 */
int ScreenLetters(const char* Suite, const std::vector<life::Letter>& Letters)
{
	std::vector<std::pair<std::string, std::string>> Bodies;
	int Problems = 0;
	auto Word = [](const std::string& Text, const std::string& W, bool Blind) {
		std::string T = Text;
		std::string Find = W;
		if (Blind)
		{
			T = copylint::Lower(T);
			Find = copylint::Lower(Find);
		}
		for (size_t P = T.find(Find); P != std::string::npos; P = T.find(Find, P + 1))
		{
			const bool Left = P == 0 || !std::isalnum(static_cast<unsigned char>(T[P - 1]));
			const bool Right = P + Find.size() >= T.size() || !std::isalnum(static_cast<unsigned char>(T[P + Find.size()]));
			if (Left && Right)
			{
				return true;
			}
		}
		return false;
	};
	for (const life::Letter& L : Letters)
	{
		Bodies.emplace_back(L.From + " / " + L.Subject, L.Body);
		for (const std::string& Text : {L.Subject, L.Body, L.From})
		{
			for (const char* Name : {"PokerStars", "GGPoker", "partypoker", "888poker", "ACR", "Ignition", "BetOnline", "WPT Global", "CoinPoker", "Natural8", "Winamax", "Unibet",
					 "Run It Once", "Skrill", "Neteller", "PayPal", "Venmo", "ecoPayz", "MuchBetter", "YouTube", "TikTok", "Rumble", "Vimeo", "Dailymotion", "NorthPot", "BluffHarbor",
					 "AceLine", "ChipCity", "Felt Republic", "PocketRocket", "RiverBend Poker", "Saltwater Poker", "CardRoom 88", "Tiltproof", "Flip & Go", "Flopjoy", "Turnstone"})
			{
				if (Word(Text, Name, false))
				{
					++Problems;
					std::printf("COPY (%s) %s: names \"%s\": %s\n", Suite, L.Subject.c_str(), Name, Text.c_str());
				}
			}
			for (const char* W : {"AI", "bot", "bots", "autoplay"})
			{
				if (Word(Text, W, std::string(W) != "AI"))
				{
					++Problems;
					std::printf("COPY (%s) %s: says \"%s\": %s\n", Suite, L.Subject.c_str(), W, Text.c_str());
				}
			}
		}
	}
	return Problems + copylint::Run(Suite, Bodies, copylint::LetterBody);
}

/** The save a session writes now, its world's text in it (a session may hand the host a snapshot to write instead). */
std::string SavedNowSeams(ss::Session& S, Hooks& H)
{
	S.Save();
	ss::SaveData D = H.Last;
	if (D.WorldText.empty() && D.WorldSnapshot)
	{
		D.WorldSnapshot->Write(D.WorldText);
	}
	return D.Serialize();
}

/**
 * Gate3Fixtures (update 2 C3, SPEC §6.5, for PIE §8.3): update2_multisite (fixture_tool write-g3) parses, writes back byte for
 * byte, loads a second time the same, and holds what §6.5 asks of it: accounts on both rivals (Ignite's from its own gate,
 * after Act I and 150 paid events; RunItTwice's as the console sends it), a card deposit on each with its fee in the bank's
 * ledger, each wallet its lines' sum, a withdrawal in transit, RunItTwice's tickets, rival results, Rewards with the "We
 * noticed" boost, and a Team RiverLine deal under the clause with one strike. In a session, through the calendar: the
 * withdrawal lands once, as one bank line net of its fee, on its minute; a save and a load across it never lands it twice.
 */
void Gate3Fixtures()
{
	namespace net = ss::net;
	namespace site = ss::site;
	const char* Name = "update2_multisite.ssave";
	const std::string Saved = Fixture(Name);
	ss::SaveData D;
	const bool Parsed = Saved.rfind("shortstack.nightone.v2\n", 0) == 0 && ss::SaveData::Parse(Saved, D);
	Expect(Parsed, "gate 3 fixtures: update2_multisite.ssave is there and parses");
	if (!Parsed)
	{
		return;
	}
	const std::string Again = D.Serialize();
	ss::SaveData Second;
	Expect(Again == Saved && ss::SaveData::Parse(Again, Second) && Second.Serialize() == Again,
		"gate 3 fixtures: update2_multisite.ssave writes back byte for byte and loads a second time the same");
	std::printf("  gate 3 fixtures: %s: %s (%zu bytes)\n", Name, Again == Saved ? "written back byte for byte" : "NOT written back byte for byte", Again.size());
	const site::Book& B = D.Life.Sites;
	const site::Account* Ign = nullptr;
	const site::Account* Rit = nullptr;
	for (const site::Account& A : B.Accounts)
	{
		Ign = A.Site == net::SiteId::Ignite ? &A : Ign;
		Rit = A.Site == net::SiteId::RunItTwice ? &A : Rit;
	}
	auto Lines = [](const site::Account* A) {
		ss::Chips Sum = 0;
		for (const site::Line& L : A ? A->Lines : std::vector<site::Line>())
		{
			Sum += L.Cents;
		}
		return Sum;
	};
	const site::Transfer* Out = nullptr;
	for (const site::Transfer& X : B.Transfers)
	{
		Out = X.State == 0 && X.Out && X.Site == net::SiteId::Ignite ? &X : Out;
	}
	int Deposits = 0;
	int CardFees = 0;
	for (const life::LedgerEntry& E : D.Life.Ledger)
	{
		Deposits += E.Kind == 7 && E.Amount == -5000 && E.Label.rfind("Deposit: ", 0) == 0 ? 1 : 0;
		CardFees += E.Kind == life::LedgerSites && E.Amount < 0 ? 1 : 0;
	}
	const double World = D.ClockMinutes + ss::net::MinutesPerDay * ss::net::NightOneDay;
	double IgniteAsked = -1.0;
	for (const site::Invitation& I : B.Invites)
	{
		IgniteAsked = I.Site == net::SiteId::Ignite && I.State == 1 ? I.At : IgniteAsked;
	}
	const ss::sponsor::Contract* Team = nullptr;
	for (const ss::sponsor::Contract& C : D.Life.Sponsors.Contracts)
	{
		Team = C.Brand == "riverline" && C.St == ss::sponsor::State::Active ? &C : Team;
	}
	int OnIgnite = 0, OnRit = 0;
	for (const ss::HistoryEntry& E : D.History)
	{
		OnIgnite += net::SiteOf(E.EventId) == net::SiteId::Ignite ? 1 : 0;
		OnRit += net::SiteOf(E.EventId) == net::SiteId::RunItTwice ? 1 : 0;
	}
	std::printf("  gate 3 fixtures: the multi-site career on %s: bank %s, Ignite %s, RunItTwice %s, %s in transit (arriving %s), %d rents paid, Team RiverLine %s (%d strike), "
				"%d Ignite and %d RunItTwice results\n",
		ss::net::DateLabel(ss::net::DayOf(World)).c_str(), ss::Money(D.BankrollCents).c_str(), Ign ? ss::Money(Ign->Cents).c_str() : "none", Rit ? ss::Money(Rit->Cents).c_str() : "none",
		Out ? ss::Money(Out->Cents - Out->FeeCents).c_str() : "nothing", Out ? ss::net::DateLabel(ss::net::DayOf(Out->ArrivesAt)).c_str() : "-", D.Life.RentsPaid,
		Team ? Team->T.Only.c_str() : "none", Team ? Team->Strikes : 0, OnIgnite, OnRit);
	Expect((D.Life.Update2LinesRead & (life::U2Sites | life::U2Rewards)) == (life::U2Sites | life::U2Rewards) && D.WorldText.empty() && D.Life.RentsPaid >= 2,
		"gate 3 fixtures: the sites' and Rewards' lines kept, two rents paid (Act I over), no world lines (an Established world)");
	Expect(Ign && Rit && Ign->Cents == Lines(Ign) && Rit->Cents == Lines(Rit) && Ign->Deposited == 5000 && Rit->Deposited == 5000 && Deposits == 2 && CardFees == 2,
		"gate 3 fixtures: $50 by card on each rival (the bank's kind-7 line and its fee), each wallet its own lines' sum");
	Expect(Out && Out->Cents == 4000 && Out->FeeCents == 300 && Out->ArrivesAt > World && Ign && Ign->Withdrawn == 4000,
		"gate 3 fixtures: $40 out of Ignite, in transit (in neither balance) until it lands");
	Expect(IgniteAsked >= static_cast<double>(ss::net::Def(net::SiteId::Ignite).LaunchDay) * ss::net::MinutesPerDay && Ign && Ign->BonusPending > 0 &&
			   D.Life.Sites.RiverLine.BoostUntil > World && D.Life.TicketsFor("runittwice.sunday-grind") == 1 && D.Life.TicketsFor("runittwice.supper-stack") == 1,
		"gate 3 fixtures: Ignite's letter after its launch, its bonus pending, RiverLine's boost, RunItTwice's tickets left");
	Expect(Team && Team->T.Only == "riverline" && Team->Strikes == 1 && OnIgnite >= 1 && OnRit >= 1,
		"gate 3 fixtures: Team RiverLine under the clause with one strike, and results on both rivals");
	// The copy pass on the letters as the game wrote them (the invitations, the welcomes, the tickets, "We noticed", the strike).
	const int Copy = ScreenLetters("gate 3 fixtures: Mail", D.Life.Inbox);
	Failures += Copy;
	std::printf("  gate 3 fixtures: the copy pass over %zu letters: %d problems\n", D.Life.Inbox.size(), Copy);
	// In a session: the calendar lands the withdrawal once, net of the fee, as one bank line; across a save and a load too.
	if (!Out)
	{
		return;
	}
	const ss::Chips Net = Out->Cents - Out->FeeCents;
	const double Arrives = Out->ArrivesAt;
	const std::string Label = "Withdrawal: Ignite";
	auto BankLines = [&Label](const ss::Session& S) {
		int N = 0;
		for (const life::LedgerEntry& E : S.Life.Ledger)
		{
			N += E.Label == Label && E.Kind == 7 ? 1 : 0;
		}
		return N;
	};
	Hooks H;
	ss::Session S(H, "gate3-multisite", &D);
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.1);
	const ss::Chips Bank = S.BankrollCents;
	const ss::Chips Wallet = Ign ? Ign->Cents : 0;
	// To an hour before it lands: nothing yet; saved there and loaded again; then past it.
	S.LobbyMinutes += Arrives - 60.0 - S.WorldMinutes();
	Now = Wait(S, Now, 0.1);
	const bool Early = BankLines(S) == 0;
	ss::SaveData Mid;
	ss::SaveData::Parse(SavedNowSeams(S, H), Mid);
	Hooks H2;
	ss::Session L(H2, "gate3-multisite", &Mid);
	L.CurrentScreen = ss::Screen::Lobby;
	Now = Wait(L, Now, 0.1);
	L.LobbyMinutes += Arrives + 2.0 * Day - L.WorldMinutes();
	Now = Wait(L, Now, 0.1);
	const site::Transfer* Landed = nullptr;
	for (const site::Transfer& X : L.Life.Sites.Transfers)
	{
		Landed = X.Id == Out->Id ? &X : Landed;
	}
	const life::LedgerEntry* Line = nullptr;
	for (const life::LedgerEntry& E : L.Life.Ledger)
	{
		Line = E.Label == Label && E.Kind == 7 ? &E : Line;
	}
	ss::Chips SpentAfter = 0;
	for (const life::LedgerEntry& E : L.Life.Ledger)
	{
		SpentAfter += E.At > World && E.Label != Label ? E.Amount : 0;
	}
	ss::Chips WalletNow = 0;
	for (const site::Account& A : L.Life.Sites.Accounts)
	{
		WalletNow = A.Site == net::SiteId::Ignite ? A.Cents : WalletNow;
	}
	std::printf("  gate 3 fixtures: the withdrawal lands %s: %s, the bank %s -> %s\n", Line ? (ss::net::DateLabel(ss::net::DayOf(Line->At)) + " " + ss::net::TimeLabel(Line->At)).c_str() : "never",
		Line ? ss::Money(Line->Amount).c_str() : "-", ss::Money(Bank).c_str(), ss::Money(L.BankrollCents).c_str());
	Expect(Early && BankLines(L) == 1 && Line && Line->Amount == Net && std::fabs(Line->At - Arrives) < 1e-6 && Landed && Landed->State == 1 && WalletNow == Wallet &&
			   L.BankrollCents == Bank + Net + SpentAfter,
		"gate 3 fixtures: the withdrawal lands once on its minute, one bank line net of its $3 fee, across a save and a load; the wallet doesn't move");
}

// ------------------------------------------------------------------ update 2 C2: the stream's moments and their hands

/** Aces all in preflop against sevens, the player on the button and Villain in the other seat, 64 big blinds each (the
 *  deal session_seg_video.cpp's AcesAllIn uses): "5d" on the river and the aces hold, the villain out of chips; "7d" and
 *  the sevens get there. */
std::unique_ptr<ss::Hand> LinkAces(const char* River, const std::string& Villain)
{
	std::vector<ss::Card> Deck = ss::ParseCards(std::string("7cAs7hAd2hKd8s2c3h9h4h") + River);
	for (int C = 0; C < 52; ++C)
	{
		if (std::find(Deck.begin(), Deck.end(), C) == Deck.end())
		{
			Deck.push_back(C);
		}
	}
	ss::HandConfig Cfg;
	Cfg.Players = {{0, ss::HeroId, 6400}, {1, Villain, 6400}};
	Cfg.ButtonSeat = 0;
	Cfg.SmallBlind = 100;
	Cfg.BigBlind = 200;
	Cfg.Deck = Deck;
	ss::Rng R("seams-link-aces");
	auto Hd = std::make_unique<ss::Hand>(Cfg, R);
	for (const ss::PlayerAction& A : {ss::PlayerAction::RaiseTo(600), ss::PlayerAction::RaiseTo(6400), ss::PlayerAction::Call()})
	{
		Hd->Act(A);
	}
	return Hd;
}

/** The notes the stream logged since it held Before (the log keeps the newest 60; these checks stay far under). */
std::vector<ss::kast::MomentNote> LinkNotesSince(const ss::Session& S, size_t Before)
{
	return std::vector<ss::kast::MomentNote>(S.Stream.Moments.begin() + static_cast<std::ptrdiff_t>(std::min(Before, S.Stream.Moments.size())), S.Stream.Moments.end());
}

/** Whether two moment notes are the same note (HypeAfter climbs with every moment, so two notes never match; the link is
 *  left out: a hand's note is linked as the hand ends, after it lands). */
bool LinkSameNote(const ss::kast::MomentNote& A, const ss::kast::MomentNote& B)
{
	return A.Kind == B.Kind && A.World == B.World && A.Real == B.Real && A.Viewers == B.Viewers && A.HypeAfter == B.HypeAfter;
}

/**
 * S3 (update 2 C2, g2 audit): a hand's stream moments fire before its Rerun candidate is cut (HeroHandDone), so the
 * candidate carries that hand's face, chat and clip time (§4.4's stream add-on):
 *   - the skip path: the moments a skip used to drop (SkipMoments: a won all-in here) and the player's knockout link to the
 *     skipped hand's candidate (they fired after it, unlinked, before C2), and a clipped one's candidate holds the clip's
 *     time (so its upload freezes the Kast clip: the audit's double count);
 *   - the watch path: the knockout links to its own hand (it fired at the tick's end before C2, on the next hand's clock,
 *     and the next hand's candidate took it), and the next hand's candidate takes only its own moment, never the tick's (a level, a new tournament: 2d);
 *   - a streamed bounty event, skipped: every knockout the player made is on stream exactly once (the bounty count), each
 *     linked to its own hand's candidate, and the skipped hands' won all-ins link too.
 */
void StreamLinks()
{
	namespace kast = ss::kast;
	namespace video = ss::video;
	auto CandidateOf = [](const ss::Session& S, const std::string& Id) -> const video::Candidate* {
		for (const video::Candidate& C : S.Life.Video.Open)
		{
			if (C.Id == Id)
			{
				return &C;
			}
		}
		return nullptr;
	};
	Hooks H;
	ss::Session S(H, "seams-stream-links");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	S.Register(1); // $0.25 Hyper Sprint, 180 players
	const std::string Live = Rig && S.T ? S.GoLive() : std::string("no table");
	Expect(Live.empty() && S.Stream.Live, "stream links: live on Kast at a table");
	if (!S.Stream.Live || !S.T || S.T->Players.size() < 2)
	{
		return;
	}
	const std::string Villain = S.T->Players[1].Id; // somebody in the field (a name the stream can say)
	const ss::TPlayer* Who = S.PlayerById(Villain);
	const std::string Prefix = "rl:" + S.T->Spec.Id + ":";
	// 1. The skip path: the aces hold and send the villain home. The won all-in and the knockout go on stream first.
	{
		S.T->Tick += 1;
		S.Stream.Viewers = 40.0;
		const auto Hd = LinkAces("5d", Villain);
		ss::Session::HandFacts F;
		F.ByPlan = true;
		F.Skipped = true;
		F.EquityAtAllIn = 0.81;
		for (const ss::PotResult& P : Hd->PotResults)
		{
			F.PotBefore += P.Amount;
		}
		const size_t Before = S.Stream.Moments.size();
		S.HeroHandDone(*Hd, F);
		const std::vector<kast::MomentNote> Notes = LinkNotesSince(S, Before);
		const std::string Id = Prefix + std::to_string(S.T->Tick);
		const video::Candidate* C = CandidateOf(S, Id);
		bool Won = false, Out = false, Linked = !Notes.empty();
		std::vector<std::string> Chat;
		for (const kast::MomentNote& N : Notes)
		{
			Won = Won || N.Kind == kast::Moment::WonAllIn;
			Out = Out || N.Kind == kast::Moment::Knockout;
			Linked = Linked && N.Candidate == Id;
			for (const std::string& Line : N.Chat)
			{
				if (Chat.size() < 3)
				{
					Chat.push_back(Line);
				}
			}
		}
		std::printf("  stream links: a skipped hand fired %zu moments (won all-in %d, knockout %d), its candidate %s, face %d, %zu chat lines\n", Notes.size(), Won ? 1 : 0, Out ? 1 : 0,
			C ? C->Id.c_str() : "none", C ? C->Stream.Face : -1, C ? C->Stream.Chat.size() : 0);
		Expect(Won && Out && Linked && C && C->Source == 3 && C->Auto, "stream links: a skipped hand's won all-in and knockout go to its own candidate (SkipMoments before the candidate)");
		Expect(C && C->Stream.Face == static_cast<int>(Notes.back().Face) && C->Stream.Chat == Chat && C->Stream.Viewers >= 40,
			"stream links: the skipped hand's candidate carries the moments' face, chat and viewers");
	}
	// 1b. The clip's time (the audit's double count): the same skipped hand, a hand at a time, until somebody in chat clips
	// one of its moments (a won all-in 18%, a knockout 10% with the room full). Its candidate holds the clip's time, so the
	// upload stops that Kast clip bringing its own follows (§4.7). (The log is cleared first so its newest 60 hold them all.)
	{
		S.Stream.Moments.clear();
		double Clip = -1.0;
		std::string Id;
		int Hands = 0;
		for (; Hands < 30 && Clip < 0.0; ++Hands)
		{
			S.T->Tick += 1;
			S.Stream.Viewers = 40.0;
			const auto Hd = LinkAces("5d", Villain);
			ss::Session::HandFacts F;
			F.ByPlan = true;
			F.Skipped = true;
			F.EquityAtAllIn = 0.81;
			for (const ss::PotResult& P : Hd->PotResults)
			{
				F.PotBefore += P.Amount;
			}
			const size_t Before = S.Stream.Moments.size();
			S.HeroHandDone(*Hd, F);
			for (const kast::MomentNote& N : LinkNotesSince(S, Before))
			{
				Clip = Clip >= 0.0 ? Clip : N.ClipAt;
			}
			Id = Prefix + std::to_string(S.T->Tick);
		}
		const video::Candidate* C = CandidateOf(S, Id);
		std::printf("  stream links: a skipped hand clipped on the %s try, its candidate %s, clip at %.1f (the candidate's %.1f)\n", ss::Ordinal(Hands).c_str(), C ? C->Id.c_str() : "none",
			Clip, C ? C->Stream.ClipAt : -1.0);
		Expect(Clip >= 0.0 && C && C->Stream.ClipAt == Clip, "stream links: a clipped skipped hand's candidate holds the clip's time (its upload freezes that clip)");
	}
	// 2. The watch path: the same deal watched (AfterAward has fired its own all-in moment; here the hook alone). One
	// knockout note, this hand's; then the next hand (the clock a hand on) loses an all-in, and its candidate takes only that.
	{
		S.T->Tick += 1;
		const auto Hd = LinkAces("5d", Villain);
		ss::Session::HandFacts F;
		F.Skipped = false;
		for (const ss::PotResult& P : Hd->PotResults)
		{
			F.PotBefore += P.Amount;
		}
		const size_t Before = S.Stream.Moments.size();
		S.HeroHandDone(*Hd, F);
		const std::vector<kast::MomentNote> Notes = LinkNotesSince(S, Before);
		const std::string Id = Prefix + std::to_string(S.T->Tick + 1);
		const bool One = Notes.size() == 1 && Notes[0].Kind == kast::Moment::Knockout && Notes[0].Candidate == Id && CandidateOf(S, Id) != nullptr;
		bool Named = false;
		for (const kast::ChatMsg& M : S.Stream.Chat)
		{
			Named = Named || (Who && M.Text == "bye " + Who->Name + " kastGG");
		}
		Expect(One && (Named || Notes[0].Chat.empty()), "stream links: a watched hand's knockout is on stream once, by name, and goes to that hand's candidate");
		S.T->Tick += 1;
		const auto Beat = LinkAces("7d", Villain);
		ss::Session::HandFacts G;
		G.Skipped = false;
		const size_t Then = S.Stream.Moments.size();
		S.Stream.OnMoment(S.Channel, S.StreamInputs(), kast::Moment::BadBeat); // what AfterAward fires for it
		S.HeroHandDone(*Beat, G);
		const std::vector<kast::MomentNote> Next = LinkNotesSince(S, Then);
		const std::string NextId = Prefix + std::to_string(S.T->Tick + 1);
		int Knockouts = 0;
		for (const kast::MomentNote& N : S.Stream.Moments)
		{
			Knockouts += N.Kind == kast::Moment::Knockout && N.Candidate == Id ? 1 : 0;
		}
		Expect(Next.size() == 1 && Next[0].Kind == kast::Moment::BadBeat && Next[0].Candidate == NextId && Knockouts == 1,
			"stream links: the next hand's candidate takes its own moment, never the last hand's knockout");
	}
	// 2b. Who busted whom as the tournament reckons it (ApplyHand: whoever won the last pot the busted player was in). Three
	// all in preflop, the player's aces the shortest stack against queens and kings: the aces take the main pot, the kings
	// the side pot over the queens. The queens are out of chips, sent home by the kings: no knockout on the player's stream.
	{
		S.T->Tick += 1;
		const std::string Queens = S.T->Players[2].Id;
		const std::string Kings = S.T->Players[3].Id;
		std::vector<ss::Card> Deck = ss::ParseCards("AsQsKdAhQhKc8s2c7d9hJh3sTs4c");
		for (int C = 0; C < 52; ++C)
		{
			if (std::find(Deck.begin(), Deck.end(), C) == Deck.end())
			{
				Deck.push_back(C);
			}
		}
		ss::HandConfig Cfg;
		Cfg.Players = {{0, ss::HeroId, 2000}, {1, Queens, 4000}, {2, Kings, 8000}};
		Cfg.ButtonSeat = 2;
		Cfg.SmallBlind = 100;
		Cfg.BigBlind = 200;
		Cfg.Deck = Deck;
		ss::Rng R("seams-link-side-pot");
		ss::Hand Hd(Cfg, R);
		for (const ss::PlayerAction& A : {ss::PlayerAction::RaiseTo(8000), ss::PlayerAction::Call(), ss::PlayerAction::Call()})
		{
			if (!Hd.bComplete)
			{
				Hd.Act(A);
			}
		}
		const ss::HandSeat* Mine = Hd.SeatByNumber(0);
		const ss::HandSeat* Out = Hd.SeatByNumber(1);
		const bool Dealt = Hd.bComplete && Mine && Mine->Stack > 0 && Out && Out->Stack == 0 && Hd.PotResults.size() >= 2 && Hd.PotResults[0].Winners == std::vector<int>{0} &&
			Hd.PotResults[1].Winners == std::vector<int>{2};
		ss::Session::HandFacts F;
		F.Skipped = false;
		const size_t Before = S.Stream.Moments.size();
		S.HeroHandDone(Hd, F);
		int Outs = 0;
		for (const kast::MomentNote& N : LinkNotesSince(S, Before))
		{
			Outs += N.Kind == kast::Moment::Knockout ? 1 : 0;
		}
		Expect(Dealt && Outs == 0, "stream links: a player busted in a side pot the kings won is the kings' knockout, not the player's (the main pot's)");
	}
	// 2c. A chop, as the tournament reckons it (C2's verify): the pot that sent the queens home split between the player on the
	// button and the kings' seat, so its first winner left of the button (the kings) has the knockout and the bounty. Not the
	// player's stream's, or "bye" and the bounty count would part ways.
	{
		S.T->Tick += 1;
		const std::string Queens = S.T->Players[2].Id;
		const std::string Kings = S.T->Players[3].Id;
		std::vector<ss::Card> Deck = ss::ParseCards("AhQsAcKdQcKs2cQhJdTc3c4s5c2h");
		for (int C = 0; C < 52; ++C)
		{
			if (std::find(Deck.begin(), Deck.end(), C) == Deck.end())
			{
				Deck.push_back(C);
			}
		}
		ss::HandConfig Cfg;
		Cfg.Players = {{0, Kings, 8000}, {1, Queens, 2000}, {2, ss::HeroId, 8000}};
		Cfg.ButtonSeat = 2;
		Cfg.SmallBlind = 100;
		Cfg.BigBlind = 200;
		Cfg.Deck = Deck;
		ss::Rng R("seams-link-chop");
		ss::Hand Hd(Cfg, R);
		for (const ss::PlayerAction& A : {ss::PlayerAction::RaiseTo(8000), ss::PlayerAction::Call(), ss::PlayerAction::Call()})
		{
			if (!Hd.bComplete)
			{
				Hd.Act(A);
			}
		}
		const ss::HandSeat* Out = Hd.SeatByNumber(1);
		const bool Dealt = Hd.bComplete && Out && Out->Stack == 0 && !Hd.PotResults.empty() && Hd.PotResults[0].Winners == std::vector<int>{0, 2};
		ss::Session::HandFacts F;
		F.Skipped = false;
		const size_t Before = S.Stream.Moments.size();
		S.HeroHandDone(Hd, F);
		int Outs = 0;
		for (const kast::MomentNote& N : LinkNotesSince(S, Before))
		{
			Outs += N.Kind == kast::Moment::Knockout ? 1 : 0;
		}
		Expect(Dealt && Outs == 0, "stream links: a chopped pot's knockout is its first winner's left of the button (the kings'), not the player's");
	}
	// 2d. The tick's own moments (C2's verify): a level and a new tournament fire between hands, on the next hand's clock (the
	// watch path's HandleTourneyEvents, a registration). That hand's candidate takes its own moment and leaves them: their chat
	// ("new tourney lets go") isn't the hand's.
	{
		S.T->Tick += 1;
		const auto Beat = LinkAces("7d", Villain);
		ss::Session::HandFacts G;
		G.Skipped = false;
		const size_t Then = S.Stream.Moments.size();
		S.Stream.OnMoment(S.Channel, S.StreamInputs(), kast::Moment::Register, S.T->Spec.Name);
		S.Stream.OnMoment(S.Channel, S.StreamInputs(), kast::Moment::LevelUp);
		S.Stream.OnMoment(S.Channel, S.StreamInputs(), kast::Moment::BadBeat); // what AfterAward fires for it
		S.HeroHandDone(*Beat, G);
		const std::string Id = Prefix + std::to_string(S.T->Tick + 1);
		int Own = 0, Ticks = 0, TicksLinked = 0;
		for (const kast::MomentNote& N : LinkNotesSince(S, Then))
		{
			const bool Tick = N.Kind == kast::Moment::Register || N.Kind == kast::Moment::LevelUp;
			Own += N.Kind == kast::Moment::BadBeat && N.Candidate == Id ? 1 : 0;
			Ticks += Tick ? 1 : 0;
			TicksLinked += Tick && !N.Candidate.empty() ? 1 : 0;
		}
		Expect(Own == 1 && Ticks == 2 && TicksLinked == 0 && CandidateOf(S, Id) != nullptr,
			"stream links: a hand's candidate takes its own moment and leaves the tick's (a level, a new tournament) unlinked");
	}
	// 3. A streamed bounty event, skipped (BountyTournament's Headhunter, a millisecond of skip a frame so the log never
	// turns over inside one): every note as it lands. The first of a few careers whose player sends somebody home.
	for (int Try = 0; Try < 6; ++Try)
	{
		Hooks Hp;
		ss::Session P(Hp, "seams-stream-pko-" + std::to_string(Try));
		P.CurrentScreen = ss::Screen::Lobby;
		P.Life.Unlocks.insert("bounty");
		P.BankrollCents = 300000;
		const bool PkoRig = P.Buy(ss::gear::FirstPcUpgrade().Id).empty();
		ss::net::EventInstance Pko;
		const bool Found = ss::net::Shared().FindInstance("hh-110@1500", Pko);
		if (Found)
		{
			P.RegisterEvent(ss::net::Shared().Listing(Pko, nullptr, P.Unlocks()));
		}
		const std::string OnAir = PkoRig && P.T ? P.GoLive() : std::string("no table");
		Expect(Found && OnAir.empty() && P.Stream.Live, "stream links: live on Kast at the Headhunter");
		if (!P.Stream.Live || !P.T)
		{
			return;
		}
		P.CurrentPace = ss::Pace::Sprint;
		P.SprintBudgetMs = 1.0;
		std::vector<kast::MomentNote> All;
		bool Lost = false;
		double Now = 0.0;
		ss::Rng Choice("seams-stream-pko-choices");
		for (int Frames = 0; P.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
		{
			Now += 1.0 / 30.0;
			P.Update(Now);
			const std::vector<kast::MomentNote>& M = P.Stream.Moments;
			size_t From = 0;
			if (!All.empty())
			{
				bool Seen = false;
				for (size_t K = M.size(); K-- > 0 && !Seen;)
				{
					Seen = LinkSameNote(M[K], All.back());
					From = Seen ? K + 1 : From;
				}
				From = Seen ? From : M.size();
				Lost = Lost || !Seen;
			}
			All.insert(All.end(), M.begin() + static_cast<std::ptrdiff_t>(From), M.end());
			if (P.HasPrompt && Now - P.Prompt.OpenedAt > 0.5)
			{
				// A hand Sprint handed back (the bubble's approach, the final table): call or check, the watch path's hands.
				P.HeroAct(P.Prompt.CanCheck ? ss::PlayerAction::Check() : Choice.Next() < 0.7 ? ss::PlayerAction::Call() : ss::PlayerAction::Fold());
			}
		}
		int Outs = 0, OutsLinked = 0, Swings = 0, SwingsLinked = 0;
		for (const kast::MomentNote& N : All)
		{
			const bool Out = N.Kind == kast::Moment::Knockout;
			const bool Swing = N.Kind == kast::Moment::WonAllIn || N.Kind == kast::Moment::BadBeat || N.Kind == kast::Moment::BigPot;
			Outs += Out ? 1 : 0;
			OutsLinked += Out && !N.Candidate.empty() ? 1 : 0;
			Swings += Swing ? 1 : 0;
			SwingsLinked += Swing && !N.Candidate.empty() ? 1 : 0;
		}
		const ss::Results& R = P.LastResults;
		std::printf("  stream links: a streamed Headhunter (career %d), skipped: %s of %d, %d knockouts (the bounty count), %d knockout notes (%d linked); %d swings on "
					"stream, %d linked to their hands\n",
			Try, ss::Ordinal(R.Place).c_str(), R.Entrants, R.Knockouts, Outs, OutsLinked, Swings, SwingsLinked);
		Expect(!Lost && P.CurrentScreen == ss::Screen::Results, "stream links: the Headhunter reaches the results, every note seen");
		Expect(Outs == R.Knockouts && OutsLinked == Outs, "stream links: every knockout is on stream exactly once, linked to its own hand's candidate");
		if (R.Knockouts == 0 && Try < 5)
		{
			continue;
		}
		Expect(R.Knockouts > 0 && SwingsLinked > 0, "stream links: a career with knockouts; the skipped hands' all-ins and big pots link to their candidates");
		break;
	}
}
// ------------------------------------------------------------------ G3: the rival sites' session seams (update 2 B3)

/**
 * A listing on a site of the test's own: tonight's $0.25 Hyper Sprint (the lobby's second row) under the site's prefix
 * ("runittwice.seams-<name>@<start>"), with the buy-in, field and table population asked for. No network schedules it, so
 * the world registers nobody for it: the session seats the field itself (NameField's fallback).
 */
ss::LobbyEvent SitesListing(ss::net::SiteId Site, const std::string& Name, ss::Chips BuyIn, int Entrants, const char* Population)
{
	ss::LobbyEvent L = ss::Lobby()[1];
	// (The hand-tuned listing's id has no start; a scheduled event's is "<template>@<world minute it starts>".)
	const long long Start = static_cast<long long>(std::llround(ss::net::MinutesPerDay * static_cast<double>(ss::net::NightOneDay) + L.Spec.StartClock));
	L.Spec.Id = (Site == ss::net::SiteId::RiverLine ? std::string() : std::string(ss::net::KeyOf(Site)) + ".") + "seams-" + Name + "@" + std::to_string(Start);
	L.Spec.Name = "Seams " + Name;
	L.Name = L.Spec.Name;
	L.Spec.BuyInCents = BuyIn;
	L.BuyInCents = BuyIn;
	L.Spec.FeeCents = BuyIn / 10;
	L.Spec.GuaranteeCents = 0;
	L.Spec.BountyCents = 0;
	L.Spec.MysteryBounty = false;
	L.Spec.SeatValueCents = 0;
	L.Spec.SeatTicket.clear();
	L.Spec.Entrants = Entrants;
	L.Spec.Population = Population;
	L.Joinable = true;
	return L;
}

/** A satellite that seats everyone (its pool buys a seat each), so it ends at the first look: a seat, no money. */
ss::LobbyEvent SitesAllSeats(ss::net::SiteId Site, const std::string& Name, const std::string& SeatTicket)
{
	ss::LobbyEvent L = SitesListing(Site, Name, 550, 18, Site == ss::net::SiteId::RunItTwice ? "low" : "micro");
	L.Spec.SeatValueCents = L.Spec.BuyInCents - L.Spec.FeeCents;
	L.Spec.SeatTicket = SeatTicket;
	return L;
}

/** An account on a rival, as the Cashier would have it (the test's own: no deposit, no letter). */
void SitesOpen(ss::Session& S, ss::net::SiteId Site, ss::Chips Cents)
{
	ss::site::Account A;
	A.Site = Site;
	A.JoinedAt = S.WorldMinutes();
	A.Cents = Cents;
	S.Life.Sites.Accounts.push_back(A);
}

ss::site::Account* SitesAccount(ss::Session& S, ss::net::SiteId Site)
{
	for (ss::site::Account& A : S.Life.Sites.Accounts)
	{
		if (A.Site == Site)
		{
			return &A;
		}
	}
	return nullptr;
}

/** The bank's lines with this label (the ledger keeps the newest 60). */
int SitesBankLines(const ss::Session& S, const std::string& Label)
{
	int N = 0;
	for (const life::LedgerEntry& E : S.Life.Ledger)
	{
		N += E.Label == Label ? 1 : 0;
	}
	return N;
}

/** Sprints the table in front to its results (the hero checks or folds when Sprint hands a decision back). */
double SitesPlayOut(ss::Session& S, double Now)
{
	S.CurrentPace = ss::Pace::Sprint;
	ss::Rng Choice("seams-sites-choices");
	int Decisions = 0;
	return PlayOut(S, Choice, Now, Decisions);
}

/** Whether core-sites' wallets are in (site::WalletOf answers for an account): the money checks that need its bodies. */
bool SitesWalletsLive()
{
	ss::site::Book B;
	ss::site::Account A;
	A.Site = ss::net::SiteId::Ignite;
	B.Accounts.push_back(A);
	return ss::site::WalletOf(B, ss::net::SiteId::Ignite) == &B.Accounts.front().Cents;
}

/**
 * The rival sites' seams in Session.cpp (update 2 B3, §5.5, §5.8, §5.9, §6.2; review_ready.md's traps in the lead's file):
 *   0. every site's network shows the session's world from its constructor, and a session going away moves every network
 *      that showed its world on (a rival attached on its own too);
 *   1. a rival's tables need an account there (freerolls too) and its wallet's money, never the bank's; RiverLine's
 *      restriction is RiverLine's alone;
 *   2. a ticket into a rival's event writes no bank line (its line is the site's); RiverLine's still writes its $0 line;
 *   3. a seat won on a rival writes no bank line either, the seat's ticket keeps its site's prefix, and a rival's results
 *      don't open RiverLine's account features (bounty, satellites, six-max) or bring Sam;
 *   4. the sitting's net counts the sites' money (wallets and transfers on their way), and Dee's "broke" text doesn't fire
 *      on a player with money sitting on a site;
 *   5. a rival's field: the session seats only the site's members, at the stakes its buy-in says (not RunItTwice's tougher
 *      population's), and the player's rival only with an account there; RiverLine's fields as ever, its fallback never
 *      seating someone who plays only on Ignite;
 *   6. (with core-sites' wallets) a rival's buy-in and prize move its wallet and never the bank or its ledger;
 *   7. (with core-deals' clause) registering on another site, or bringing its table to the front on stream, is a strike;
 *   8. Dee's text on a rival win; 9. on a rival's series title (Ignite keeps no series board: no leaderboard in it).
 */
void RivalSites()
{
	namespace net = ss::net;
	namespace site = ss::site;
	const net::SiteId Home = net::SiteId::RiverLine;
	const net::SiteId Ignite = net::SiteId::Ignite;
	const net::SiteId Rit = net::SiteId::RunItTwice;
	auto AllShow = [](const ss::world::World* W) {
		bool All = net::Shared().Attached() == W;
		for (int K = 1; K < static_cast<int>(net::SiteId::Count); ++K)
		{
			All = All && net::Site(static_cast<net::SiteId>(K)).Attached() == W;
		}
		return All;
	};
	// 0. The registry on the game thread, and nothing left pointing at a world that's gone.
	{
		Hooks Ha;
		auto Old = std::make_unique<ss::Session>(Ha, "seams-sites-old");
		Expect(AllShow(&Old->Living()), "sites: every site's network shows the session's world from its constructor (built and attached on the game thread)");
		Hooks Hb;
		ss::Session New(Hb, "seams-sites-new");
		net::Site(Ignite).Attach(&Old->Living()); // a rival attached on its own (not through AttachAll)
		Old.reset();
		Expect(AllShow(&New.Living()), "sites: a session going away moves every network that showed its world on to the newest (a rival's too)");
	}
	// 1. Accounts, the wallet, restrictions.
	{
		Hooks H;
		ss::Session S(H, "seams-sites-afford");
		S.CurrentScreen = ss::Screen::Lobby;
		S.BankrollCents = 100000;
		const ss::LobbyEvent Free = SitesListing(Ignite, "free", 0, 18, "soft");
		const ss::LobbyEvent Paid = SitesListing(Ignite, "paid", 550, 18, "soft");
		const ss::LobbyEvent Ours = SitesListing(Home, "home", 550, 18, "micro");
		Expect(net::SiteOf(Free.Spec.Id) == Ignite && net::SiteOf(Ours.Spec.Id) == Home, "sites: the test's listings are Ignite's and RiverLine's");
		Expect(!S.CanAfford(Free) && !S.CanAfford(Paid) && S.CanAfford(Ours), "sites: no account, no rival table (not even a freeroll); RiverLine's as ever");
		SitesOpen(S, Ignite, 549);
		SitesOpen(S, Rit, 100000); // (review-lead-g3) another site's money never pays for this one's seat
		Expect(S.CanAfford(Free) && !S.CanAfford(Paid),
			"sites: an account opens a rival's freerolls; a buy-in needs that wallet's money ($1,000 in the bank or on RunItTwice doesn't count)");
		SitesAccount(S, Ignite)->Cents = 550;
		Expect(S.CanAfford(Paid), "sites: $5.50 on Ignite buys an Ignite $5.50");
		S.Life.BannedUntil = S.WorldMinutes() + 24.0 * 60.0;
		Expect(!S.CanAfford(Ours) && S.CanAfford(Paid) && S.Restricted(Home) && !S.Restricted(Ignite) && !S.Restricted(Rit),
			"sites: RiverLine's restriction closes RiverLine's tables only (the rivals don't know or care)");
		// The record the invitation gates read (review-lead-g3; core-sites' rep::FromHistory, applied in Session::Record): the
		// last hundred results' average paid buy-in, a freeroll left out.
		S.History.clear();
		for (const ss::Chips BuyIn : {ss::Chips(1100), ss::Chips(2200), ss::Chips(0)})
		{
			ss::HistoryEntry E;
			E.Name = "Seams record";
			E.Place = 5;
			E.Entrants = 50;
			E.BuyInCents = BuyIn;
			S.History.push_back(E);
		}
		Expect(S.Record().Abi100 == 1650, "sites: Record() carries the last hundred's average paid buy-in (a freeroll left out) for the gates");
	}
	// 2. A ticket into a rival's event: no bank line, no money anywhere.
	{
		Hooks H;
		ss::Session S(H, "seams-sites-ticket");
		S.CurrentScreen = ss::Screen::Lobby;
		SitesOpen(S, Rit, 0);
		const ss::LobbyEvent Grind = SitesListing(Rit, "grind", 2200, 18, "low");
		const std::string Tid = "runittwice.seams-grind";
		S.Life.Tickets[Tid] = 1;
		const ss::Chips Bank = S.BankrollCents;
		const size_t Lines = S.Life.Ledger.size();
		const bool Could = S.CanAfford(Grind);
		S.RegisterEvent(Grind);
		const site::Account* A = SitesAccount(S, Rit);
		Expect(Could && S.T && S.T->Spec.Id == Grind.Spec.Id && S.Life.TicketsFor(Tid) == 0 && S.BankrollCents == Bank && A && A->Cents == 0,
			"sites: a RunItTwice ticket enters its event (an account with $0 on it), and no money moves");
		Expect(S.Life.Ledger.size() == Lines && SitesBankLines(S, "Ticket: Seams grind") == 0, "sites: a rival's ticket writes no bank line ($0 lines for a rival never reach the bank)");
		bool SiteLine = false;
		for (const site::Line& L : A ? A->Lines : std::vector<site::Line>())
		{
			SiteLine = SiteLine || (L.Label == "Ticket: Seams grind" && L.Cents == 0);
		}
		std::printf("  sites: a rival ticket's line in the site's own ledger: %s\n", SiteLine ? "yes" : "not kept (core-sites' AddLine not in yet)");
		Expect(SiteLine || !SitesWalletsLive(), "sites: a rival ticket's line is in the site's own ledger (once core-sites keeps lines)");
		Hooks Hh;
		ss::Session Ours(Hh, "seams-sites-ticket-home");
		Ours.CurrentScreen = ss::Screen::Lobby;
		Ours.Life.Tickets["seams-grind"] = 1;
		Ours.RegisterEvent(SitesListing(Home, "grind", 2200, 18, "low"));
		Expect(Ours.T && SitesBankLines(Ours, "Ticket: Seams grind") == 1 && Ours.Life.Ledger.front().Amount == 0, "sites: RiverLine's ticket still writes its $0 bank line");
	}
	// 3. A seat won on a rival.
	{
		Hooks H;
		ss::Session S(H, "seams-sites-seat");
		S.CurrentScreen = ss::Screen::Lobby;
		SitesOpen(S, Rit, 0);
		S.Life.Tickets["runittwice.seams-sat"] = 1;
		const ss::Chips Bank = S.BankrollCents;
		const size_t Inbox = H.Inbox.size();
		S.RegisterEvent(SitesAllSeats(Rit, "sat", "runittwice.seams-grind"));
		const bool Opened = S.T != nullptr;
		SitesPlayOut(S, 0.0);
		Expect(Opened && S.CurrentScreen == ss::Screen::Results && S.LastResults.SeatWon == "runittwice.seams-grind" && S.Life.TicketsFor("runittwice.seams-grind") == 1,
			"sites: a RunItTwice satellite's seat is a RunItTwice ticket (its prefix kept)");
		Expect(S.BankrollCents == Bank && SitesBankLines(S, "Seat won: Seams sat") == 0 && SitesBankLines(S, "Ticket: Seams sat") == 0,
			"sites: a seat won on a rival writes no bank line");
		bool SeatLine = false; // (review-lead-g3) the seat's own line is the site's
		for (const site::Line& L : SitesAccount(S, Rit) ? SitesAccount(S, Rit)->Lines : std::vector<site::Line>())
		{
			SeatLine = SeatLine || (L.Label == "Seat won: Seams sat" && L.Cents == 0);
		}
		Expect(SeatLine || !SitesWalletsLive(), "sites: a seat won on a rival is a $0 line in the site's own ledger");
		Expect(S.Life.Unlocks.empty() && !H.Got(Inbox, "RiverLine", "New on your account") && !H.Got(Inbox, "Sam", "riverline board"),
			"sites: a rival's result opens nothing of RiverLine's account, and Sam didn't see it on RiverLine's board");
		Hooks Hh;
		ss::Session Ours(Hh, "seams-sites-seat-home");
		Ours.CurrentScreen = ss::Screen::Lobby;
		Ours.Life.Tickets["seams-sat"] = 1;
		Ours.RegisterEvent(SitesAllSeats(Home, "sat", "seams-grind"));
		SitesPlayOut(Ours, 0.0);
		Expect(Ours.LastResults.SeatWon == "seams-grind" && SitesBankLines(Ours, "Seat won: Seams sat") == 1 && Ours.Life.Unlocks.count("bounty") == 1,
			"sites: RiverLine's seat still writes its bank line, and its result opens bounty events");
	}
	// 4. The sitting's net and Dee's "broke".
	{
		Hooks H;
		ss::Session S(H, "seams-sites-net");
		S.CurrentScreen = ss::Screen::Lobby;
		SitesOpen(S, Ignite, 5000);
		S.Life.Tickets["seams-net"] = 1;
		S.RegisterEvent(SitesAllSeats(Home, "net", "seams-grind"));
		// Mid-sitting, money moves on the site: $12.34 lands there, and $20 leaves it for the bank (on its way for days).
		site::Account* A = SitesAccount(S, Ignite);
		if (A)
		{
			A->Cents += 1234 - 2000;
		}
		site::Transfer X;
		X.Id = "seams-net-out";
		X.Site = Ignite;
		X.Out = true;
		X.Cents = 2000;
		X.FeeCents = 300;
		X.RequestedAt = S.WorldMinutes();
		X.ArrivesAt = S.WorldMinutes() + 5.0 * 24.0 * 60.0;
		S.Life.Sites.Transfers.push_back(X);
		SitesPlayOut(S, 0.0);
		std::printf("  sites: the sitting's net with $12.34 landing on Ignite and $20 on its way back: %s\n", ss::Money(S.LastResults.SessionNetCents).c_str());
		Expect(S.CurrentScreen == ss::Screen::Results && S.LastResults.SessionNetCents == 1234,
			"sites: the sitting's net counts the sites' money (the wallets and what's on its way), not the bank's alone");
		auto Broke = [Ignite](ss::Chips SiteCents, const char* Seed) {
			Hooks Hk;
			ss::Session B(Hk, Seed);
			B.CurrentScreen = ss::Screen::Lobby;
			B.BankrollCents = 10;
			if (SiteCents > 0)
			{
				SitesOpen(B, Ignite, SiteCents);
			}
			B.Life.Tickets["seams-broke"] = 1;
			const size_t Since = Hk.Inbox.size();
			B.RegisterEvent(SitesAllSeats(ss::net::SiteId::RiverLine, "broke", "seams-grind"));
			SitesPlayOut(B, 0.0);
			return B.CurrentScreen == ss::Screen::Results && Hk.Got(Since, "Dee", "Broke?");
		};
		Expect(Broke(0, "seams-sites-broke"), "sites: Dee's \"Broke?\" for a player with 10 cents and nothing on a site");
		Expect(!Broke(5000, "seams-sites-not-broke"), "sites: no \"Broke?\" for a player with $50 sitting on Ignite");
		// (review-lead-g3) Money on its way when the sitting starts lands during it, in the bank net of its fee: the sitting
		// nets that fee alone (a landed transfer is the bank's now, never counted twice).
		Hooks Hl;
		ss::Session L(Hl, "seams-sites-landed");
		L.CurrentScreen = ss::Screen::Lobby;
		SitesOpen(L, Ignite, 0);
		site::Transfer Y = X;
		Y.Id = "seams-net-landing";
		Y.RequestedAt = L.WorldMinutes() - 60.0;
		Y.ArrivesAt = L.WorldMinutes() + 60.0;
		L.Life.Sites.Transfers.push_back(Y);
		L.Life.Tickets["seams-landed"] = 1;
		L.RegisterEvent(SitesAllSeats(Home, "landed", "seams-grind"));
		site::Transfer& Landed = L.Life.Sites.Transfers.back();
		Landed.State = 1;
		L.BankrollCents += Landed.Cents - Landed.FeeCents;
		SitesPlayOut(L, 0.0);
		Expect(L.CurrentScreen == ss::Screen::Results && L.LastResults.SessionNetCents == -300,
			"sites: a withdrawal landing mid-sitting nets only its fee (a landed transfer is the bank's, never counted twice)");
	}
	// 5. Who sits at a rival's tables.
	{
		Hooks H;
		ss::Session S(H, "seams-sites-field");
		S.CurrentScreen = ss::Screen::Lobby;
		SitesOpen(S, Rit, 0);
		// RunItTwice accounts for every third weak micro regular (the ones NameField's fallback seats), and for none at Low.
		const unsigned Bit = 1u << static_cast<unsigned>(Rit);
		std::set<int> Members;
		int Pool = 0;
		for (const ss::world::Npc& N : S.Living().People())
		{
			if (N.Playing() && N.Tier == 1 && !N.Rival && N.Overall() < 0.5f && Pool++ % 3 == 0)
			{
				const_cast<ss::world::Npc&>(N).Sites = static_cast<uint8_t>(N.Sites | Bit);
				Members.insert(N.Id);
			}
		}
		// A $3.30 RunItTwice event seats RunItTwice's tougher population ("low"), but its people are micro players.
		S.Life.Tickets["runittwice.seams-field"] = 1;
		S.RegisterEvent(SitesListing(Rit, "field", 330, 180, "low"));
		// (The world may have given RunItTwice accounts of its own, core-world's memberships: a member is anyone with the bit.)
		int Seated = 0, Strangers = 0, WrongTier = 0;
		for (const auto& It : S.FieldNpc)
		{
			const ss::world::Npc* N = S.Living().Get(It.second);
			Seated += 1;
			Strangers += !N || (N->Sites & Bit) == 0 ? 1 : 0;
			WrongTier += N && N->Tier != 1 ? 1 : 0;
		}
		std::printf("  sites: a RunItTwice $3.30 seats %d people from the world (%zu micro regulars given an account; %d without one, %d not micro), rival at the table: %s\n",
			Seated, Members.size(), Strangers, WrongTier, S.PlayerById(ss::RivalId()) ? "yes" : "no");
		Expect(S.T && Members.size() >= 4 && Seated >= 4 && Strangers == 0 && WrongTier == 0,
			"sites: a RunItTwice table seats only RunItTwice's members, at the stakes its buy-in says (micro, not its population's low)");
		Expect(S.T && !S.PlayerById(ss::RivalId()) && S.FieldNpc.count(ss::RivalId()) == 0, "sites: the player's rival, with no RunItTwice account, isn't at a RunItTwice table");
		// With an account there, the rival plays it like anybody.
		Hooks Hr;
		ss::Session R(Hr, "seams-sites-field-rival");
		R.CurrentScreen = ss::Screen::Lobby;
		SitesOpen(R, Rit, 0);
		const int Rival = R.Living().Find(ss::RivalName);
		if (const ss::world::Npc* N = R.Living().Get(Rival))
		{
			const_cast<ss::world::Npc&>(*N).Sites = static_cast<uint8_t>(N->Sites | Bit);
		}
		R.Life.Tickets["runittwice.seams-field"] = 1;
		R.RegisterEvent(SitesListing(Rit, "field", 330, 180, "low"));
		Expect(Rival >= 0 && R.T && R.PlayerById(ss::RivalId()) && R.FieldNpc.count(ss::RivalId()) == 1, "sites: the rival with a RunItTwice account sits in its events");
		// RiverLine's own field still has the rival, and its regulars without any rival account.
		Hooks Hh;
		ss::Session Ours(Hh, "seams-sites-field-home");
		Ours.CurrentScreen = ss::Screen::Lobby;
		Ours.Life.Tickets["seams-field"] = 1;
		Ours.RegisterEvent(SitesListing(Home, "field", 330, 180, "micro"));
		Expect(Ours.T && Ours.PlayerById(ss::RivalId()) && Ours.FieldNpc.count(ss::RivalId()) == 1 && Ours.FieldNpc.size() >= 5,
			"sites: RiverLine's fields as ever: the rival, and the regulars who play it");
		// RiverLine's fallback seats only RiverLine's people: someone who plays only on Ignite (a few of its own arrivals) or
		// only for a rival's team never takes a RiverLine seat the world didn't give them (review-lead-g3).
		{
			Hooks Ho;
			ss::Session Only(Ho, "seams-sites-field-only");
			Only.CurrentScreen = ss::Screen::Lobby;
			std::set<int> Elsewhere;
			int Weak = 0;
			for (const ss::world::Npc& N : Only.Living().People())
			{
				if (N.Playing() && N.Tier == 1 && !N.Rival && N.Overall() < 0.5f && Weak++ % 2 == 0)
				{
					const_cast<ss::world::Npc&>(N).Sites = static_cast<uint8_t>(1u << static_cast<unsigned>(Ignite));
					Elsewhere.insert(N.Id);
				}
			}
			Only.Life.Tickets["seams-only"] = 1;
			Only.RegisterEvent(SitesListing(Home, "only", 330, 180, "micro"));
			const int RivalNpc = Only.Living().Find(ss::RivalName);
			int FromWorld = 0, IgniteOnly = 0;
			for (const auto& It : Only.FieldNpc)
			{
				FromWorld += It.second != RivalNpc ? 1 : 0;
				IgniteOnly += static_cast<int>(Elsewhere.count(It.second));
			}
			std::printf("  sites: a RiverLine $3.30 with %zu weak micro regulars on Ignite alone seats %d people from the world, %d of them Ignite-only\n",
				Elsewhere.size(), FromWorld, IgniteOnly);
			Expect(Only.T && Elsewhere.size() >= 4 && FromWorld >= 4 && IgniteOnly == 0,
				"sites: RiverLine's own fallback seats only people with a RiverLine account (never someone who plays on Ignite alone)");
		}
		// A real rival event from its site's schedule (once core-sites has one): the world's own registrations, members only.
		ss::net::EventInstance E;
		bool Found = false;
		for (const ss::net::EventInstance& C : net::Site(Rit).Window(S.WorldMinutes(), S.WorldMinutes() + 3.0 * Day))
		{
			Found = Found || (E = C, C.Entries > 30);
			if (Found)
			{
				break;
			}
		}
		if (!Found)
		{
			std::printf("  sites: RunItTwice has no schedule yet (core-sites): the world's own registrations unchecked here\n");
		}
		else
		{
			Hooks Hw;
			ss::Session W(Hw, "seams-sites-field-world");
			W.CurrentScreen = ss::Screen::Lobby;
			SitesOpen(W, Rit, 0);
			ss::LobbyEvent L = net::Site(Rit).Listing(E, nullptr, W.Unlocks());
			L.Joinable = true;
			W.Life.Tickets[net::Site(Rit).TicketOf(E.Id.substr(0, E.Id.find('@')))] = 1;
			// (review-lead-g3) The session's own filter, not only the world's: people with no RunItTwice account put on the
			// world's list for it never take a seat.
			std::set<int> Injected;
			for (const ss::world::Npc& N : W.Living().People())
			{
				if (Injected.size() < 3 && N.Playing() && !N.Rival && (N.Sites & Bit) == 0)
				{
					W.Living().Join(E.Id, N.Id);
					Injected.insert(N.Id);
				}
			}
			const std::vector<int> Before = W.Living().Registered(E.Id);
			int Listed = 0;
			for (const int Id : Before)
			{
				Listed += static_cast<int>(Injected.count(Id));
			}
			Expect(Listed == 3, "sites: (the test) three people without a RunItTwice account are on the world's list for its event");
			W.RegisterEvent(L);
			int Outsiders = 0;
			for (const auto& It : W.FieldNpc)
			{
				const ss::world::Npc* N = W.Living().Get(It.second);
				Outsiders += N && (N->Sites & Bit) == 0 ? 1 : 0;
			}
			std::printf("  sites: %s (%d registered by the world) seats %zu people from the world\n", E.Id.c_str(), static_cast<int>(Before.size()), W.FieldNpc.size());
			Expect(W.T && Outsiders == 0, "sites: a real RunItTwice event seats nobody without a RunItTwice account");
		}
	}
	// 6. The money on a rival's tables (core-sites' wallets: PotFor, OnBuyIn, SiteSettle).
	if (!SitesWalletsLive())
	{
		std::printf("  sites: core-sites' wallets aren't in yet: the rival buy-in and prize checks wait for them\n");
	}
	else
	{
		// Ignite $5.50s to the end (the first with 18 runners, then heads-up), a career at a time, until one cashes: each
		// one's buy-in and prize move the wallet alone, and the wallet's own lines say so.
		bool Cashed = false;
		for (int Try = 0; Try < 12 && !Cashed; ++Try)
		{
			Hooks H;
			ss::Session S(H, "seams-sites-pot-" + std::to_string(Try));
			S.CurrentScreen = ss::Screen::Lobby;
			S.BankrollCents = 100000;
			SitesOpen(S, Ignite, 10000);
			const size_t Lines = S.Life.Ledger.size();
			S.RegisterEvent(SitesListing(Ignite, "pot", 550, Try == 0 ? 18 : 2, "soft"));
			const site::Account* A = SitesAccount(S, Ignite);
			Expect(S.T && S.BankrollCents == 100000 && A && A->Cents == 10000 - 550 && S.Life.Ledger.size() == Lines,
				"sites: an Ignite buy-in comes off the Ignite wallet: the bank and its ledger don't move");
			SitesPlayOut(S, 0.0);
			const ss::Chips Won = S.LastResults.PrizeCents + S.LastResults.BountyCents;
			A = SitesAccount(S, Ignite);
			Cashed = Won > 0;
			std::printf("  sites: an Ignite $5.50 to the end: %s of %d, %s won; the wallet %s\n", ss::Ordinal(S.LastResults.Place).c_str(), S.LastResults.Entrants,
				ss::Money(Won).c_str(), A ? ss::Money(A->Cents).c_str() : "gone");
			Expect(S.CurrentScreen == ss::Screen::Results && S.BankrollCents == 100000 && A && A->Cents == 10000 - 550 + Won && S.Life.Ledger.size() == Lines &&
					   S.LastResults.SessionNetCents == Won - 550,
				"sites: its prize lands in the wallet, the bank's ledger untouched, and the sitting's net is the wallet's");
			bool BuyLine = false, PrizeLine = !Cashed;
			for (const site::Line& L : A ? A->Lines : std::vector<site::Line>())
			{
				BuyLine = BuyLine || (L.Label == "Buy-in: Seams pot" && L.Cents == -550);
				PrizeLine = PrizeLine || (L.Label == "Prize: Seams pot" && L.Cents == S.LastResults.PrizeCents);
			}
			Expect(BuyLine && PrizeLine, "sites: the wallet's own lines hold the buy-in and the prize (never the bank's)");
		}
		Expect(Cashed, "sites: one of the Ignite $5.50s cashed (the prize side was checked)");
	}
	// 7. The clause (core-deals' WouldStrike and NoteSitePlay): playing on another site, and its table in front on stream.
	{
		Hooks H;
		ss::Session S(H, "seams-sites-clause");
		S.CurrentScreen = ss::Screen::Lobby;
		S.BankrollCents = 300000;
		const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
		SitesOpen(S, Ignite, 0);
		ss::sponsor::Contract C;
		C.Id = "seams-clause";
		C.Brand = "riverline";
		C.St = ss::sponsor::State::Active;
		C.T.Only = "riverline";
		C.Since = S.WorldMinutes() - 60.0;
		C.Until = S.WorldMinutes() + 90.0 * Day;
		ss::sponsor::Book Book;
		Book.Contracts.push_back(C);
		if (!ss::sponsor::WouldStrike(Book, static_cast<int>(Ignite), false, false))
		{
			std::printf("  sites: core-deals' clause isn't in yet: the strike checks wait for it\n");
		}
		else
		{
			// An Ignite table opened before the deal (no strike), then the deal, a RiverLine table in front, live.
			S.Life.Tickets["ignite.seams-clause"] = 1;
			S.RegisterEvent(SitesListing(Ignite, "clause", 0, 18, "soft"));
			S.Life.Sponsors.Contracts.push_back(C);
			S.Life.Tickets["seams-clause"] = 1;
			S.RegisterEvent(SitesListing(Home, "clause", 0, 18, "micro"));
			// (review-lead-g3) Off stream, the table in front is nobody's business: the Ignite tile to the front and back again
			// is no strike.
			for (const net::SiteId Front : {Ignite, Home})
			{
				for (int K = 0; K < S.TableCount(); ++K)
				{
					if (net::SiteOf(S.Glance(K).EventId) == Front)
					{
						S.FocusTable(K);
					}
				}
			}
			const std::string Live = Rig && S.TableCount() == 2 ? S.GoLive() : std::string("no rig or no second table");
			const ss::sponsor::Contract* Mine = nullptr;
			for (const ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
			{
				Mine = K.Id == "seams-clause" ? &K : Mine;
			}
			const int Before = Mine ? Mine->Strikes : -1;
			int Ig = -1;
			for (int K = 0; K < S.TableCount(); ++K)
			{
				Ig = net::SiteOf(S.Glance(K).EventId) == Ignite ? K : Ig;
			}
			const bool RiverLineInFront = S.T && net::SiteOf(S.T->Spec.Id) == Home;
			S.FocusTable(Ig);
			Mine = nullptr;
			for (const ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
			{
				Mine = K.Id == "seams-clause" ? &K : Mine;
			}
			std::printf("  sites: Team RiverLine's clause, an Ignite table brought to the front on stream: strikes %d -> %d\n", Before, Mine ? Mine->Strikes : -1);
			Expect(Live.empty() && RiverLineInFront && Ig >= 0 && Before == 0 && Mine && Mine->Strikes == 1,
				"sites: on stream, bringing another site's table to the front is a strike under \"plays only on RiverLine\"");
			// (review-lead-g3) Live, a RiverLine table that finishes hands the front to the table left, an Ignite one: it's on
			// stream now, and that's a strike too (CloseFinishedTables; the tile wasn't clicked).
			Hooks Hc;
			ss::Session Two(Hc, "seams-sites-clause-close");
			Two.CurrentScreen = ss::Screen::Lobby;
			Two.BankrollCents = 300000;
			Two.AutoFocus = false; // only the finished table moves the front
			const bool Rig2 = Two.Buy(ss::gear::FirstPcUpgrade().Id).empty();
			SitesOpen(Two, Ignite, 0);
			Two.Life.Tickets["ignite.seams-close"] = 1;
			Two.RegisterEvent(SitesListing(Ignite, "close", 0, 18, "soft"));
			Two.Life.Sponsors.Contracts.push_back(C);
			Two.Life.Tickets["seams-close"] = 1;
			Two.RegisterEvent(SitesAllSeats(Home, "close", "seams-grind"));
			const std::string Live2 = Rig2 && Two.TableCount() == 2 ? Two.GoLive() : std::string("no rig or no second table");
			auto Strikes = [](const ss::Session& Of) {
				int N = -1;
				for (const ss::sponsor::Contract& K : Of.Life.Sponsors.Contracts)
				{
					N = K.Id == "seams-clause" ? K.Strikes : N;
				}
				return N;
			};
			const int Before2 = Strikes(Two);
			Two.CurrentPace = ss::Pace::Sprint; // the RiverLine satellite in front, to its seat
			double Clock = 0.0;
			for (int Frames = 0; Two.TableCount() == 2 && Frames < 400000; ++Frames)
			{
				Clock += 1.0 / 30.0;
				Two.Update(Clock);
				if (Two.HasPrompt && Clock - Two.Prompt.OpenedAt > 0.5)
				{
					Two.HeroAct(Two.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				}
			}
			const bool IgniteInFront = Two.TableCount() == 1 && Two.T && net::SiteOf(Two.T->Spec.Id) == Ignite;
			std::printf("  sites: live, a RiverLine table finishing hands the front to an Ignite one: strikes %d -> %d\n", Before2, Strikes(Two));
			Expect(Live2.empty() && Before2 == 0 && IgniteInFront && Strikes(Two) == 1,
				"sites: on stream, a finished table handing the front to another site's is a strike under \"plays only on RiverLine\"");
		}
	}
	// 8. Dee hears about a win on a rival: the prize is on that site, and site money never pays rent. Heads-up Ignite
	// freerolls, played by hand on the watch path (the one that texts her), until the player takes one; her text, linted.
	{
		std::vector<std::pair<std::string, std::string>> Copy;
		int Tries = 0;
		for (; Tries < 20 && Copy.empty(); ++Tries)
		{
			Hooks H;
			ss::Session S(H, "seams-sites-win-" + std::to_string(Tries));
			S.CurrentScreen = ss::Screen::Lobby;
			SitesOpen(S, Ignite, 0);
			const size_t Since = H.Inbox.size();
			ss::LobbyEvent Win = SitesListing(Ignite, "win", 0, 2, "soft");
			Win.Spec.GuaranteeCents = 200; // $2 to the winner
			S.RegisterEvent(Win);
			ss::Rng Choice("seams-sites-win-choices");
			int Decisions = 0;
			PlayOut(S, Choice, 0.0, Decisions);
			for (size_t K = Since; K < H.Inbox.size(); ++K)
			{
				if (H.Inbox[K].first == "Dee" && H.Inbox[K].second.rfind("You did WHAT?!", 0) == 0)
				{
					Copy.push_back(H.Inbox[K]);
				}
			}
		}
		const bool Honest = !Copy.empty() && Copy[0].second.find("Get it off that site and pay the rent.") != std::string::npos;
		std::printf("  sites: a heads-up Ignite win on the %s try: \"%s\"\n", ss::Ordinal(Tries).c_str(), Copy.empty() ? "no win" : Copy[0].second.c_str());
		Expect(Honest, "sites: Dee's text on a rival site's win tells the player to get it off the site first (it can't pay rent there)");
		Copy.push_back({"Dee", "You did WHAT?! Get it off that site and put it toward your key. Then come see me. We need to talk about your future."});
		Failures += copylint::Run("sites (session)", Copy, copylint::Pa);
		for (const auto& Line : Copy)
		{
			const std::string Low = copylint::Lower(Line.second);
			bool Clean = true;
			for (const char* Word : {" ai ", " bot", "autoplay", "flopjoy", "turnstone"})
			{
				Clean = Clean && Low.find(Word) == std::string::npos;
			}
			Expect(Clean, "sites: Dee's lines say nothing of autoplay or the old names");
		}
	}
	// 9. A series title won on a rival (review-lead-g3): Ignite keeps no series board (its boards are the month's and the
	// season's), so Dee names the title and no leaderboard. Heads-up Ignite freerolls dressed as Wildfire's first event (its
	// template, tonight), played by hand, until the player takes one.
	{
		const net::EventTemplate* Wild = net::Site(Ignite).FindTemplate("ignite.wildfeb27-1");
		const net::SeriesInfo* Series = Wild ? net::Site(Ignite).FindSeries(Wild->Series) : nullptr;
		if (!Series)
		{
			std::printf("  sites: Ignite has no Wildfire yet (core-sites): the rival series title's text unchecked here\n");
		}
		else
		{
			std::vector<std::pair<std::string, std::string>> Said;
			bool Won = false;
			int Tries = 0;
			for (; Tries < 20 && !Won; ++Tries)
			{
				Hooks H;
				ss::Session S(H, "seams-sites-series-" + std::to_string(Tries));
				S.CurrentScreen = ss::Screen::Lobby;
				SitesOpen(S, Ignite, 0);
				const size_t Since = H.Inbox.size();
				ss::LobbyEvent L = SitesListing(Ignite, "series", 0, 2, "soft");
				L.Spec.Id = Wild->Id + L.Spec.Id.substr(L.Spec.Id.find('@'));
				L.Spec.GuaranteeCents = 200;
				S.RegisterEvent(L);
				ss::Rng Choice("seams-sites-series-choices");
				int Decisions = 0;
				PlayOut(S, Choice, 0.0, Decisions);
				Won = S.CurrentScreen == ss::Screen::Results && S.LastResults.Place == 1;
				for (size_t K = Since; K < H.Inbox.size(); ++K)
				{
					if (H.Inbox[K].first == "Dee")
					{
						Said.push_back(H.Inbox[K]);
					}
				}
			}
			const std::string Want = "A " + Series->Name + " title. Different site, same cards. Nobody can take that away from you.";
			bool Title = false, Board = false;
			for (const auto& Line : Said)
			{
				Title = Title || Line.second == Want;
				Board = Board || Line.second.find("leaderboard") != std::string::npos;
			}
			std::printf("  sites: a %s title on the %s try: Dee %s\n", Series->Name.c_str(), ss::Ordinal(Tries).c_str(),
				Title ? "names the title" : Board ? "points at a leaderboard Ignite doesn't keep" : "says nothing of it");
			Expect(Won && Title && !Board, "sites: a rival's series title brings Dee's text about the title, never a leaderboard the site doesn't keep");
			Failures += copylint::Run("sites (session, series)", Said, copylint::Pa);
		}
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
	seams::Gate2Fixtures();
	seams::Gate3Fixtures();
	seams::StreamLinks();
	seams::RivalSites();
}
} // namespace session_test
