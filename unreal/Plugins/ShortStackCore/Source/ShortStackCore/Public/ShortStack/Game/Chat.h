#pragma once

#include "ShortStack/Common.h"

namespace ss
{
class Rng;

/** Table chat: dealer lines plus the things online players type at 3 a.m. Port of web/src/game/chat.ts. */
enum class ChatKind : int
{
	Dealer,
	Player,
	System,
	Rival,
	Hero,
};

struct ChatLine
{
	std::string Who; // empty for dealer and system lines
	std::string Text;
	ChatKind Kind = ChatKind::System;
	double Time = 0.0;
};

/** The rival's screen name. */
extern SHORTSTACKCORE_API const char* const RivalName; // "gh0stfold"
/** The rival's tournament player id ("npc:gh0stfold"). */
std::string RivalId();

std::string Brag(Rng& R);
std::string Salt(Rng& R);
std::string Nice(Rng& R);
std::string BustLine(Rng& R);
std::string IdleLine(Rng& R);

enum class RivalMoment : int
{
	Arrive,
	WinVsHero,
	LoseVsHero,
	HeroBust,
	BustsSelf,
};
std::string RivalLine(RivalMoment Moment, Rng& R);
} // namespace ss
