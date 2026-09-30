#include "ShortStack/Game/Chat.h"
#include "../StrictFloat.h"

#include "ShortStack/Rng.h"

namespace ss
{
const char* const RivalName = "gh0stfold";

std::string RivalId()
{
	return std::string("npc:") + RivalName;
}

namespace chat_detail
{
const std::vector<std::string>& WinBrag()
{
	static const std::vector<std::string> V = {"ship it", "ty", "ty ty", "thx for the chips", "lets gooo", "knew it", "EZ", "gg wp"};
	return V;
}
const std::vector<std::string>& LoseSalt()
{
	static const std::vector<std::string> V = {"unreal", "every. single. time.", "rigged site lol", "how do you call that", "wow", "nice river", "sick", "of course", "this site man"};
	return V;
}
const std::vector<std::string>& NiceLines()
{
	static const std::vector<std::string> V = {"nh", "nh sir", "gh", "wp"};
	return V;
}
const std::vector<std::string>& BustLines()
{
	static const std::vector<std::string> V = {"gg", "gg all", "gl everyone", "gg, bed time", "and im out. gl"};
	return V;
}
const std::vector<std::string>& IdleLines()
{
	static const std::vector<std::string> V = {
		"anyone else up at 3am lol", "gl all", "blinds are flying", "coffee #4", "this turbo is a lottery", "ante up",
		"who else has work tomorrow", "fold fold fold fold", "card dead for an hour", "my cat just stepped on my keyboard",
		"ICM says fold. my heart says jam", "min cash here i come",
	};
	return V;
}
const std::vector<std::string>& Rival(RivalMoment M)
{
	static const std::vector<std::string> Arrive = {"evening, table.", "moved again. fine.", "hi fish. just kidding. mostly."};
	static const std::vector<std::string> WinVsHero = {"thanks for the chips", "you had it. i had more.", "predictable.", "called it before the flop"};
	static const std::vector<std::string> LoseVsHero = {"nice hand. won't happen twice", "enjoy that one", "ok. noted."};
	static const std::vector<std::string> HeroBust = {"gg. see you tomorrow night", "gg. go get some sleep", "back to the freerolls"};
	static const std::vector<std::string> BustsSelf = {"gg. you got lucky tonight", "well played. this time."};
	switch (M)
	{
	case RivalMoment::Arrive: return Arrive;
	case RivalMoment::WinVsHero: return WinVsHero;
	case RivalMoment::LoseVsHero: return LoseVsHero;
	case RivalMoment::HeroBust: return HeroBust;
	default: return BustsSelf;
	}
}
} // namespace chat_detail

std::string Brag(Rng& R) { return R.Pick(chat_detail::WinBrag()); }
std::string Salt(Rng& R) { return R.Pick(chat_detail::LoseSalt()); }
std::string Nice(Rng& R) { return R.Pick(chat_detail::NiceLines()); }
std::string BustLine(Rng& R) { return R.Pick(chat_detail::BustLines()); }
std::string IdleLine(Rng& R) { return R.Pick(chat_detail::IdleLines()); }
std::string RivalLine(RivalMoment Moment, Rng& R) { return R.Pick(chat_detail::Rival(Moment)); }
} // namespace ss
