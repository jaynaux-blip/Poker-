// riverline_ui: core-badges's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// the badges' own art goldens: badges_atlas (every variant at 12, 16, 24 and 64), badges_room_leaders, badges_club_card
// (the desk's and Career's mini card), badges_rail (the card's rail, C7); every glyph's 12 px draw list differing from
// every other's; the atlas's cells; and the words fitting where they're drawn (the tooltip, Career's progress cards,
// the rail's labels, the room's leaders).
//
// The laptop's pages that carry badges (badges_board_live, badges_board_honors, badges_board_hover, badges_card_rail,
// badges_card_tab, badges_career_progress, badges_embercrest_page, badges_table_seat, badges_kast_overlay) are drawn by
// ui-laptop's and core-sponsor's files; their goldens live with those pages (tests/ui_seg_laptop.cpp, ui_seg_sponsor.cpp).
#include "UiSeg.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/BadgeArt.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace ui_test
{
namespace badges_seg
{
namespace badge = ss::badge;
namespace badgeart = ss::ui::badgeart;

void Check(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

/** One glyph alone on a canvas: its draw list. */
ss::ui::DrawList Glyph(const badge::Badge& B, float Size)
{
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 128.0f, 128.0f, 1.0f);
	badgeart::Draw(C, B, 64.0f, 64.0f, Size, 0.0);
	return L;
}

/** A draw list's geometry and text as one string (to tell two apart). */
std::string Print(const ss::ui::DrawList& L)
{
	std::string Out;
	char Buf[96];
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		std::snprintf(Buf, sizeof(Buf), "%.2f,%.2f,%.3f,%.3f,%.3f,%.3f;", V.X, V.Y, V.Col.R, V.Col.G, V.Col.B, V.Col.A);
		Out += Buf;
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Out += "|" + Cmd.Text.Text;
		}
	}
	return Out;
}

/** Every text run in order, joined (letter-spaced words are drawn a glyph a run). */
std::string Runs(const ss::ui::DrawList& L)
{
	std::string Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Out += Cmd.Text.Text;
		}
	}
	return Out;
}

/** The lines a sentence wraps into at a width (the tooltip's paragraph). */
int Lines(const ss::ui::TextMeasurer& M, const std::string& Text, float Size, float Width)
{
	int N = 1;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Trial = Line.empty() ? Word : Line + " " + Word;
		if (!Line.empty() && M.Width(Trial, ss::ui::Font::Regular, Size) > Width)
		{
			++N;
			Line = Word;
		}
		else
		{
			Line = Trial;
		}
		Start = End + 1;
	}
	return N;
}

badge::Badge Make(badge::Kind K, int Tier = 0, int Count = 0, bool Main = false, const std::string& Brand = std::string())
{
	badge::Badge B;
	B.K = K;
	B.Tier = Tier;
	B.Count = Count;
	B.Main = Main;
	B.Brand = Brand;
	B.Year = 2027;
	return B;
}

void Atlas()
{
	const std::vector<badge::Badge>& All = badgeart::AtlasBadges();
	// The atlas: one cell a variant, 8 a row, into the 1,024 x 1,024 target at 128 (64 cells at most).
	{
		TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1024.0f, 1024.0f, 1.0f);
		const int Cells = badgeart::Atlas(C, 128.0f);
		Expect(Cells == static_cast<int>(All.size()) && Cells <= 64 && Cells >= 50 && !L.Vertices.empty(), "badges_atlas: every variant in a cell, 64 at most");
		bool Inside = true;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Inside = Inside && V.X >= -1.0f && V.Y >= -1.0f && V.X <= 1025.0f && V.Y <= 128.0f * static_cast<float>((Cells + 7) / 8) + 1.0f;
		}
		Expect(Inside, "badges_atlas: the art stays inside its cells' rows");
		Expect(Drew(L, "5+") && Drew(L, "9+") && Drew(L, "2"), "badges_atlas: the count medals (2 to 5+, the wall's to 9+)");
		WriteList("badges_atlas_target", L);
	}
	// AtlasCell: each variant is its own cell; counts past the atlas's share the last; an unknown brand, the generic patch.
	{
		bool Own = true;
		for (size_t K = 0; K < All.size(); ++K)
		{
			Own = Own && badgeart::AtlasCell(All[K]) == static_cast<int>(K);
		}
		Expect(Own, "atlas cells: every variant finds its own cell");
		const int Five = badgeart::AtlasCell(Make(badge::Kind::Bracelet, 0, 5));
		Expect(badgeart::AtlasCell(Make(badge::Kind::Bracelet, 0, 12)) == Five && badgeart::AtlasCell(Make(badge::Kind::Bracelet, 0, 12, true)) != Five,
			"atlas cells: 12 bracelets share the 5+ cell; a Main Event's has its own");
		Expect(badgeart::AtlasCell(Make(badge::Kind::OnTheWall, 0, 30)) == badgeart::AtlasCell(Make(badge::Kind::OnTheWall, 0, 9)), "atlas cells: the wall past 9");
		const int Generic = badgeart::AtlasCell(Make(badge::Kind::Sponsored, 0, 0, false, "nobody-knows"));
		Expect(Generic >= 0 && Generic == badgeart::AtlasCell(Make(badge::Kind::Sponsored)) && badgeart::AtlasCell(Make(badge::Kind::Sponsored, 0, 0, false, "voltrush")) != Generic,
			"atlas cells: an unknown brand gets the plain patch");
		Expect(badgeart::AtlasCell(badge::Badge()) == -1, "atlas cells: no badge, no cell");
	}
	// Every variant at 12, 16, 24 and 64; and at 12 every glyph's draw list differs from every other's.
	{
		TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 900.0f, 1.0f);
		const float Sizes[4] = {12.0f, 16.0f, 24.0f, 64.0f};
		for (int Row = 0; Row < 4; ++Row)
		{
			float X = 20.0f;
			float Y = 20.0f + static_cast<float>(Row) * 200.0f;
			for (const badge::Badge& B : All)
			{
				badge::Badge Shown = B;
				Shown.Count = B.Count > 0 ? B.Count : 0;
				badgeart::Draw(C, Shown, X + Sizes[Row] * 0.5f, Y + Sizes[Row] * 0.5f, Sizes[Row], 0.0);
				X += Sizes[Row] + 8.0f;
				if (X > 1560.0f)
				{
					X = 20.0f;
					Y += Sizes[Row] + 8.0f;
				}
			}
		}
		Expect(!L.Vertices.empty(), "badges_atlas: drawn at 12, 16, 24 and 64");
		WriteList("badges_atlas", L);
		std::set<std::string> Seen;
		int Same = 0;
		int Empty = 0;
		for (const badge::Badge& B : All)
		{
			const ss::ui::DrawList One = Glyph(B, 12.0f);
			Empty += One.Vertices.empty() ? 1 : 0;
			if (!Seen.insert(Print(One)).second)
			{
				++Same;
				std::printf("  the same 12 px art twice: %s\n", badge::Label(B).c_str());
			}
		}
		Expect(Same == 0 && Empty == 0, "badges_atlas: at 12 px every glyph's draw list differs from every other's");
		// Tiers change the mark, not only the colour (§1.7): the club's and the card's tiers differ in shape (vertex counts).
		const size_t C1 = Glyph(Make(badge::Kind::Club, 1), 16.0f).Vertices.size();
		const size_t C2 = Glyph(Make(badge::Kind::Club, 2), 16.0f).Vertices.size();
		const size_t C3 = Glyph(Make(badge::Kind::Club, 3), 16.0f).Vertices.size();
		const size_t K1 = Glyph(Make(badge::Kind::CircuitCard, 1), 16.0f).Vertices.size();
		const size_t K2 = Glyph(Make(badge::Kind::CircuitCard, 2), 16.0f).Vertices.size();
		const size_t K3 = Glyph(Make(badge::Kind::CircuitCard, 3), 16.0f).Vertices.size();
		Expect(C1 != C2 && C2 != C3 && C1 != C3 && K1 != K2 && K2 != K3 && K1 != K3, "glyphs: a tier changes the mark, not just the colour");
		// The year's numerals come in at 24 (§5.5).
		badge::Badge Poy = Make(badge::Kind::PlayerOfYear, 1, 1);
		Expect(!Drew(Glyph(Poy, 16.0f), "'27") && Drew(Glyph(Poy, 24.0f), "'27"), "glyphs: a Player of the Year's numerals at 24 and up");
		Expect(Drew(Glyph(Make(badge::Kind::Ring, 0, 3), 16.0f), "3") && !Drew(Glyph(Make(badge::Kind::Ring, 0, 1), 16.0f), "1"), "glyphs: the count medal from two");
	}
}

void Leaders()
{
	// Ten rows on the 1,024 x 576 page (the room clock's LEADERS rect), the player's own in ember, one glyph a line (C8).
	std::vector<ss::life::RoomLeader> Rows;
	const char* Names[10] = {"Mei", "Big Lou", "Mrs. Park", "Sal", "kenji.k", "grinder_3c", "Rick", "VikingVolta", "Dre", "Corinne Vasquez"};
	for (int K = 0; K < 10; ++K)
	{
		ss::life::RoomLeader L;
		L.Rank = K + 1;
		L.Name = Names[K];
		L.Points = 1240.6 - 81.0 * K;
		badge::Set S;
		if (K == 0)
		{
			S.Held = {Make(badge::Kind::OnTheWall, 0, 2), Make(badge::Kind::Club, 3, 40)};
		}
		else if (K % 3 == 1)
		{
			S.Held = {Make(badge::Kind::Club, 2, 20), Make(badge::Kind::VerifiedPro), Make(badge::Kind::Ring, 0, 1)};
		}
		L.Badge = badge::Pack(S);
		Rows.push_back(L);
	}
	auto Page = [](const std::vector<ss::life::RoomLeader>& R) {
		TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1024.0f, 576.0f, 1.0f);
		badgeart::RoomLeaders(C, {30.0f, 92.0f, 964.0f, 446.0f}, R, "NOVEMBER", "grinder_3c", 1.0);
		return L;
	};
	const ss::ui::DrawList L = Page(Rows);
	WriteList("badges_room_leaders", L);
	Expect(Runs(L).find("NOVEMBER'S LEADERS") != std::string::npos && Runs(L).find("POINTS") != std::string::npos, "badges_room_leaders: the month's header");
	Expect(DrewRun(L, "grinder_3c") && DrewRun(L, "Corinne Vasquez") && DrewRun(L, "1,241") && DrewRun(L, "10"), "badges_room_leaders: ten rows, rank, name and points");
	// C8: a row with three badges draws exactly what its one top glyph draws.
	std::vector<ss::life::RoomLeader> Tops = Rows;
	for (ss::life::RoomLeader& R : Tops)
	{
		const badge::Set Held = badge::Unpack(R.Badge);
		badge::Set One;
		if (const badge::Badge* B = badge::Top(Held, badge::Room, badge::FrameNone))
		{
			One.Held.push_back(*B);
		}
		R.Badge = badge::Pack(One);
	}
	Expect(Print(Page(Tops)) == Print(L), "badges_room_leaders: one glyph a line (C8)");
	bool Fits = true;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		Fits = Fits && V.X >= 28.0f && V.X <= 996.0f && V.Y >= 60.0f && V.Y <= 540.0f;
	}
	Expect(Fits, "badges_room_leaders: inside its rect on the page");
}

void Cards()
{
	// The desk's card (the panel's 410 wide at 1080p) and Career's mini card (220 x 138), every tier.
	const char* Words[4] = {"NEW MEMBER", "REGULAR", "EMBER", "CREST"};
	for (int Tier = 0; Tier <= 3; ++Tier)
	{
		badgeart::ClubCardInfo I;
		I.Name = "Jesse Cole";
		I.ScreenName = "grinder_3c";
		I.Country = "US";
		I.Tier = Tier;
		I.Visits = Tier == 0 ? 4 : Tier == 1 ? 6 : Tier == 2 ? 21 : 38;
		I.Wins = Tier >= 2 ? 2 : 0;
		I.SinceDay = 1;
		I.New = Tier == 1;
		for (int Mini = 0; Mini < 2; ++Mini)
		{
			TableMeasurer M;
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, M, 600.0f, 400.0f, 1.0f);
			const ss::ui::Rect R = Mini ? ss::ui::Rect{20.0f, 20.0f, 220.0f, 138.0f} : ss::ui::Rect{20.0f, 20.0f, 410.0f, 410.0f / 1.586f};
			badgeart::ClubCard(C, R, I, 3.0);
			const std::string Name = std::string(Mini ? "badges_club_card_mini_" : "badges_club_card_") + std::to_string(Tier);
			WriteList(Name, L);
			const std::string Text = Runs(L);
			const bool Word = Text.find(Words[Tier]) != std::string::npos;
			Expect(Word && DrewRun(L, "Jesse Cole"), (Name + ": the tier and the name").c_str());
			if (!Mini)
			{
				const char* Strip = Tier == 0 ? "4 of 6 to Regular" : Tier == 1 ? "6 of 18 to Ember" : Tier == 2 ? "21 of 36 to Crest" : "38 in 90 days";
				Expect(Text.find(Strip) != std::string::npos, (Name + ": the strip to the next card").c_str());
				Expect(Tier < 2 || Text.find("ON THE WALL \xC3\x97" "2") != std::string::npos, (Name + ": the wall").c_str());
			}
			// What shows stays on the card: the geometry outside any clip, inside the rect but for the shadow's blur.
			bool Inside = true;
			int Clips = 0;
			float Far = 0.0f;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				Clips += Cmd.Type == ss::ui::DrawCmd::Kind::PushClip ? 1 : Cmd.Type == ss::ui::DrawCmd::Kind::PopClip ? -1 : 0;
				if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles || Clips > 0)
				{
					continue;
				}
				for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
				{
					const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
					const float Out = std::max({R.X - V.X, V.X - (R.X + R.W), R.Y - V.Y, V.Y - (R.Y + R.H)});
					if (V.Col.A > 0.02f && Out > 0.0f)
					{
						Far = std::max(Far, Out);
					}
				}
			}
			Inside = Far <= 34.0f && Clips == 0;
			Check(Inside, Name + ": the card stays in its rect, its shadow aside (" + std::to_string(Far) + " out)");
		}
	}
}

void CardFromSave()
{
	// The desk's card from the save alone: the tier from the live record's Embercrest nights, printed once (DeskKey).
	ss::SaveData D;
	D.HeroName = "grinder_3c";
	const int Day = 60; // December 4, 2026: the 1st is day 57, January 1 day 88
	for (int K = 0; K < 7; ++K)
	{
		ss::life::LiveDay Night;
		Night.Day = Day - 80 + K * 11;
		Night.Events = 1;
		Night.Embercrest = 1;
		D.Life.LiveDays.push_back(Night);
	}
	ss::life::LiveDay Online;
	Online.Day = Day - 1;
	Online.Events = 1;
	D.Life.LiveDays.push_back(Online);
	D.Life.LiveWins = 1;
	const double World = Day * 1440.0 + 600.0;
	const badgeart::ClubCardInfo I = badgeart::ClubCardOf(D, World);
	Expect(I.Tier == 1 && I.Visits == 7 && I.Wins == 1 && I.ScreenName == "grinder_3c" && I.New, "club card: from the save, Regular on seven nights, not printed yet");
	D.TextsSeen.push_back(badge::DeskKey(1));
	Expect(!badgeart::ClubCardOf(D, World).New && badge::DeskLine(1, false, ", grinder") == "There's my regular, grinder.", "club card: printed once, then the greeting");
	Expect(badgeart::ClubCardOf(D, World + 27.0 * 1440.0).Visits == 5 && badgeart::ClubCardOf(D, World + 27.0 * 1440.0).Tier == 1 && badgeart::ClubCardOf(D, World + 28.0 * 1440.0).Tier == 0, "club card: holds to the 1st, then the nights age out");
	// A drop on the 1st (Ember printed, Regular now): the desk greets a member, it never prints a lower card with an upgrade line.
	ss::SaveData Dropped = D;
	Dropped.TextsSeen = {badge::DeskKey(2)};
	const badgeart::ClubCardInfo Down = badgeart::ClubCardOf(Dropped, World);
	Expect(Down.Tier == 1 && !Down.New, "club card: after a drop to Regular from a printed Ember card, no new print");
}

void Rail()
{
	// C7: the card's rail shows six, then "+N".
	badge::Set S;
	S.Held = {Make(badge::Kind::HallOfFame), Make(badge::Kind::Bracelet, 0, 2, true), Make(badge::Kind::Ring, 0, 3), Make(badge::Kind::PlayerOfYear, 1, 1),
		Make(badge::Kind::TeamRiverLine), Make(badge::Kind::CircuitCard, 2), Make(badge::Kind::VerifiedPro), Make(badge::Kind::KastPartner), Make(badge::Kind::OnTheWall, 0, 1)};
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1060.0f, 200.0f, 1.0f);
	const float W = badgeart::Rail(C, S, badge::Card, 40.0f, 60.0f, 22.0f, 6, true);
	WriteList("badges_rail", L);
	Expect(DrewRun(L, "+3") && DrewRun(L, "HALL OF FAME") && DrewRun(L, "2\xC3\x97 BRACELET") && DrewRun(L, "3\xC3\x97 RING") && !DrewRun(L, "VERIFIED PRO"),
		"badges_rail: six glyphs, labels on the first three, then +3 (C7)");
	Expect(W > 6.0f * 22.0f && W < 1000.0f, "badges_rail: its width comes back");
	ss::ui::DrawList Bare;
	ss::ui::Canvas Cb(Bare, M, 1060.0f, 200.0f, 1.0f);
	Expect(badgeart::Rail(Cb, badge::Set(), badge::Card, 40.0f, 60.0f, 22.0f, 6, true) == 0.0f && Bare.Vertices.empty(), "badges_rail: nothing held, nothing drawn");
}

void Fits()
{
	// The tooltip (300 x 88 on the laptop): the title at 15 bold beside the 32 glyph, the hover line in four lines at 12 (13 apart).
	TableMeasurer M;
	int Over = 0;
	ss::badge::Holder Rich;
	for (const badge::Badge& B : badgeart::AtlasBadges())
	{
		badge::Badge Shown = B;
		Shown.Year = 2027;
		const std::string Title = badge::Title(Shown);
		const std::string How = badge::HowEarned(Shown, Rich);
		if (M.Width(Title, ss::ui::Font::Bold, 15.0f) > 232.0f || Lines(M, How, 12.0f, 232.0f) > 4)
		{
			++Over;
			std::printf("  tooltip: \"%s\" / \"%s\" (%d lines)\n", Title.c_str(), How.c_str(), Lines(M, How, 12.0f, 232.0f));
		}
	}
	// The longest hover lines, filled with long but real values.
	const std::vector<std::string> Long = {
		"Plays for a living, and the numbers hold: 4,814 tournaments, +48% ROI, 2,410 buy-ins up. Checked on the 1st of every month.",
		"3\xC3\x97 bracelet winner. Latest: $1,500 Bounty, The Championship 2027, Jun 14, 2027.",
		"A Main Event bracelet in 2027 put them on the circuit's card. Good through 2028.",
		"Hall of Fame, class of 2036. Forty or older, ten years on the scene, and a record that held up against the best.",
		"38 events at the Embercrest in the last 90 days. The card holds until the 1st.",
	};
	for (const std::string& How : Long)
	{
		if (Lines(M, How, 12.0f, 232.0f) > 4)
		{
			++Over;
			std::printf("  tooltip: \"%s\" (%d lines)\n", How.c_str(), Lines(M, How, 12.0f, 232.0f));
		}
	}
	Expect(Over == 0, "fits: every tooltip's title and hover line fit 300 x 88 (four lines at 12)");
	// Career's progress cards (300 x 84): the line at 14 bold and the detail at 12.5, 268 wide.
	const std::vector<std::string> Progress = {"Verified Pro \xC2\xB7 1,000 of 1,000 tournaments", "Ember \xC2\xB7 12 of 18 events in 90 days", "Crest \xC2\xB7 35 of 36 events in 90 days",
		"Kast Partner \xC2\xB7 41 of 75 average viewers", "Verified Pro \xC2\xB7 29 of 30 days clear", "Circuit Card \xC2\xB7 standings from week 13",
		"Gold Card \xC2\xB7 1,512 of 2,240 points", "Verified Pro \xC2\xB7 ROI +14% of +15%"};
	const std::vector<std::string> Details = {"A night played counts once. Cards drop only on the 1st.", "ROI +123% \xC2\xB7 149 of 150 buy-ins up \xC2\xB7 29 of 30 days clear",
		"Last ShiftLink shift: Nov 22. It takes 30 days clear of the day job.", "24 of 25 hours \xC2\xB7 11 of 12 days \xC2\xB7 last 30 days", "Rank 1,412 \xC2\xB7 the top 200 hold a card. Regional events and up."};
	int Wide = 0;
	for (const std::string& P : Progress)
	{
		if (M.Width(P, ss::ui::Font::Bold, 14.0f) > 268.0f)
		{
			++Wide;
			std::printf("  progress: \"%s\" is %.0f wide\n", P.c_str(), M.Width(P, ss::ui::Font::Bold, 15.0f));
		}
	}
	for (const std::string& D : Details)
	{
		if (Lines(M, D, 12.5f, 268.0f) > 2)
		{
			++Wide;
			std::printf("  progress detail: \"%s\" wraps to %d lines\n", D.c_str(), Lines(M, D, 12.5f, 268.0f));
		}
	}
	Expect(Wide == 0, "fits: Career's progress lines fit 300 x 84");
	// The rail's labels (22 px glyphs, labels at 11) and the room's honorific line (15, the HUD's studying caption).
	int Labels = 0;
	for (const badge::Badge& B : badgeart::AtlasBadges())
	{
		badge::Badge Shown = B;
		Shown.Year = 2027;
		Labels += M.Width(badge::Label(Shown), ss::ui::Font::Bold, 11.0f) > 140.0f ? 1 : 0;
	}
	Expect(Labels == 0, "fits: every label fits the rail at 11 px");
	ss::badge::Holder H;
	H.WallWins = 12;
	H.ClubVisitsNow = 40;
	H.TeamRiverLine = true;
	const badge::Set S = badge::Earned(H, 900);
	Expect(M.Width(badge::Introduce(S, badge::Room, "Corinne Vasquez"), ss::ui::Font::Regular, 15.0f) < 600.0f &&
			   M.Width(badge::Caption(S, badge::Room), ss::ui::Font::Regular, 15.0f) < 600.0f,
		"fits: the studying caption's honorific and line");
}
} // namespace badges_seg

void SegBadges()
{
	badges_seg::Atlas();
	badges_seg::Leaders();
	badges_seg::Cards();
	badges_seg::CardFromSave();
	badges_seg::Rail();
	badges_seg::Fits();
}
} // namespace ui_test
