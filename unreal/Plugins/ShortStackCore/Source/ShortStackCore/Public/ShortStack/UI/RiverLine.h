#pragma once

#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Ui.h"

namespace ss
{
namespace ui
{
/**
 * RiverLine: the fictional poker site running on the laptop. Draws the boot
 * screen, lobby, table and results into a canvas each frame and turns pointer
 * and key input into session actions. Port of web/src/client/riverline.ts.
 */
class RiverLine
{
public:
	static constexpr float Width = 1600.0f;
	static constexpr float Height = 1000.0f;

	explicit RiverLine(Session& InSession) : S(InSession) {}

	Ui UI;
	/** Set when the player clicks "Lean back"; the host clears it after handling. */
	bool LeanBackRequested = false;

	/** Draws one frame (and handles the pointer input gathered since the last one). */
	void Draw(Canvas& C, double Now);
	/** Keyboard shortcuts: "f", "c", "x", "r", "b", "a", "ArrowUp", "ArrowDown". */
	void Key(const std::string& Key);

private:
	void Logo(float X, float Y, float Scale);
	void TopBar(double Now);
	void DrawCursor();
	void Boot(double Now);
	void LobbyScreen(double Now);
	float Wrap(const std::string& Text, float X, float Y, float MaxW, float Size, const Color& Col, float LineHeight);
	std::vector<std::string> WrapLines(const std::string& Text, float MaxW, float Size);
	int HeroSeatNo() const;
	Vec2 SlotPos(int Seat) const;
	Vec2 BetPos(int Seat) const;
	void Table(double Now);
	std::vector<Card> WinningCards() const;
	void DrawSeat(const SeatVis& Seat, double Now);
	Vec2 FlightPoint(const FlightEnd& End) const;
	void DrawFlight(const Flight& F, double Now);
	void GradeBadges(double Now);
	void Controls(double Now);
	double PotFrac(const HeroPrompt& P, double F) const;
	double PotRaise(const HeroPrompt& P) const;
	void SidePanel(double Now);
	void Overlays(double Now);
	void ResultsScreen(double Now);

	float GradeLabel(Grade G, const std::string& Rest, float X, float Y, const TextStyle& Style, bool Draw = true);
	float ArrowText(const std::string& Left, const std::string& Right, float X, float Y, const TextStyle& Style);
	void Ghost(float CX, float CY, float Size);

	Session& S;
	Canvas* C = nullptr;
};
} // namespace ui
} // namespace ss
