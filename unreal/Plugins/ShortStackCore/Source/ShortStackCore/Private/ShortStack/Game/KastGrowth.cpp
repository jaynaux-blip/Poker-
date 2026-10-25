// The creator update (SPEC_CREATORS §2, §8.1): Kast's growth model as pure functions (KastGrowth.h), core-kastmath's file.
// Every term of §2 with the constants of probes/synth/synth_kast.py, the keyed draws, Heat's closed form, the forecast, the
// payout split, the sponsor read rule and the save lines over a SaveView. Phase A-K, day 1: neutral stubs (every factor 1,
// every chance and count 0, Heat never moves, nothing written or read), and nothing calls them yet.
#include "ShortStack/Game/KastGrowth.h"

#include "ShortStack/Game/Kast.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace kast
{
namespace growth
{
// ------------------------------------------------------------------ keyed draws

double Unit(uint64_t /*Seed*/, uint64_t /*StreamNo*/, Tag /*T*/, uint64_t /*A*/, uint64_t /*B*/)
{
	return 0.0;
}

uint64_t NameKey(const std::string& /*Name*/)
{
	return 0;
}

double ExpAt(double /*Mean*/, double /*U*/)
{
	return 0.0;
}

int PoissonAt(double /*Mean*/, double /*U*/)
{
	return 0;
}

double NormalAt(double /*U*/)
{
	return 0.0;
}

// ------------------------------------------------------------------ the Ping and drop-in presence

double PingW(double /*DaysSince*/, double /*L*/)
{
	return 1.0;
}

double PingWHot(double W, double /*H*/)
{
	return W;
}

double Honey(double /*DaysSinceFollow*/)
{
	return 0.0;
}

double Appetite(double /*DaysSince*/)
{
	return 1.0;
}

double Budget(double /*Hw*/)
{
	return 1.0;
}

double HwToday(double Hw, int /*HwDay*/, int /*Today*/)
{
	return Hw;
}

double Pa(const PingTerms& /*T*/)
{
	return 0.0;
}

double FriendPa(int /*Streams*/, double /*Sched*/, double /*ToD*/)
{
	return 0.0;
}

double FriendArrive(int /*Streams*/, bool /*Second*/, double /*DeeAt*/, double /*U*/)
{
	return 0.0;
}

double ArriveDelay(bool /*OnSchedule*/, double /*U*/)
{
	return 0.0;
}

double StayMean(double /*L*/, bool /*Friend*/, int /*Streams*/, bool /*Mod*/)
{
	return 0.0;
}

double GapMean(double /*L*/, bool /*Friend*/, bool /*Mod*/, bool /*Results*/, double /*DeepPull*/)
{
	return 0.0;
}

double Pst2(double /*P*/, double /*RunDepth*/)
{
	return 0.0;
}

bool SlotNext(bool Here, double /*Sm*/, double /*G*/, double /*U*/)
{
	return Here;
}

double Presence(double /*Tau*/, double /*Sm*/, double /*G*/)
{
	return 0.0;
}

// ------------------------------------------------------------------ the deepest run, Run Alerts and Rail Call

double Prestige(int /*Entrants*/, Chips /*BuyIn*/, double /*Series*/)
{
	return 0.0;
}

double Depth(int /*Entrants*/, int /*Remaining*/)
{
	return 0.0;
}

bool Armed(int /*Entrants*/)
{
	return false;
}

bool EventMode(int /*Entrants*/, int /*Remaining*/, int /*TableSize*/)
{
	return false;
}

double SweatNov(int /*Ft7*/)
{
	return 1.0;
}

double Sweat(double /*P*/, double /*RunDepth*/, double /*Nov*/)
{
	return 1.0;
}

double Railbirds(double /*Norm*/, double /*P*/, double /*RunDepth*/, double /*Nov*/)
{
	return 0.0;
}

double LurkDeep(double /*P*/, double /*RunDepth*/)
{
	return 1.0;
}

double DeepFollow(double /*P*/, double /*RunDepth*/)
{
	return 1.0;
}

int BroadcastSize(double /*P*/)
{
	return 0;
}

double RunValue(RunStage /*S*/)
{
	return 0.0;
}

double Pull(RunStage /*S*/)
{
	return 0.0;
}

double AlertBack(double /*Share*/, double /*L*/)
{
	return 0.0;
}

double AlertFollower(double /*Share*/, double /*W*/, double /*L*/, double /*P*/)
{
	return 0.0;
}

bool RailCallOpen(int /*Entrants*/, double /*RunDepth*/)
{
	return false;
}

double RailCallChance(double /*W*/, double /*L*/, double /*Sched*/, double /*ToD*/, double /*AppetiteNow*/, double /*BudgetNow*/)
{
	return 0.0;
}

// ------------------------------------------------------------------ Heat

double HeatAt(double Heat, double /*At*/, double /*Now*/)
{
	return Heat;
}

double HeatShown(double Heat, double /*At*/, double /*Now*/, int /*LastStreamDay*/)
{
	return Heat;
}

double HeatH(double /*Shown*/)
{
	return 0.0;
}

double HeatS(double /*H*/)
{
	return 1.0;
}

double HeatA(double /*H*/)
{
	return 1.0;
}

NightGain Gain(double /*Lr*/, double /*Bias*/, double /*Run*/, double /*RunNorm*/, double /*WeekUsed*/, double /*HeatShownNow*/)
{
	return NightGain();
}

double ShowUp(int /*ShowWeek*/, int /*ShowDays*/, int /*Today*/)
{
	return 0.0;
}

int Tier(double /*Shown*/)
{
	return 0;
}

bool FrontRow(double /*Shown*/, double /*U*/)
{
	return false;
}

// ------------------------------------------------------------------ discovery, stay and follow

double Disc(int /*Level*/, int /*Streak*/)
{
	return 1.0;
}

double Lane(int /*Streams*/)
{
	return 1.0;
}

double Rising(int /*Streams*/, double /*Norm*/)
{
	return 1.0;
}

double Pond(double /*V*/)
{
	return 1.0;
}

double Novelty(double /*Hours7*/)
{
	return 1.0;
}

double Taper(double /*Uptime*/, double /*RunDepth*/)
{
	return 1.0;
}

double Tide(double /*U*/, bool /*SeriesWindow*/)
{
	return 1.0;
}

double Buzz(double /*Views48*/, double /*Norm*/)
{
	return 1.0;
}

double HD(double /*E*/)
{
	return 1.0;
}

double Sat(int /*Followers*/)
{
	return 1.0;
}

double Stamina(double /*Hours7*/)
{
	return 1.0;
}

// ------------------------------------------------------------------ raids, clips, the community

double RaidRate(int /*Tokens*/, double /*DiscNow*/, double /*H*/)
{
	return 0.0;
}

double RaidSize(double /*U1*/, double /*U2*/)
{
	return 0.0;
}

bool EarnsToken(int /*Today*/, int /*LastRaidDay*/, double /*StreamMinutes*/)
{
	return false;
}

double MomentRaidChance(RunStage /*S*/)
{
	return 0.0;
}

double BigNameChance(double /*H*/, double /*P*/)
{
	return 0.0;
}

double RaiderStay(double /*U1*/, double /*U2*/, bool /*Welcomed*/)
{
	return 0.0;
}

double ClipReach(double /*V*/, double /*N*/, double /*H*/, double /*HypeHit*/)
{
	return 0.0;
}

double AffinityBump(double Affinity, double /*ShareOfGap*/)
{
	return Affinity;
}

double LurkerMean(int /*Untracked*/, double /*Sched*/, double /*ToD*/, double /*SumPingWAway*/, double /*Deep*/)
{
	return 0.0;
}

int Lurkers(double /*Mean*/, double /*U*/)
{
	return 0;
}

// ------------------------------------------------------------------ engagement

double EngageFade(double E, double /*DWorld*/)
{
	return E;
}

double ThankGain(int /*K*/)
{
	return 0.0;
}

double QuestionRate(double /*Strangers*/, double /*Present*/, double /*Raiders*/, bool /*Teaching*/)
{
	return 0.0;
}

// ------------------------------------------------------------------ money

double EngT(double /*E*/)
{
	return 1.0;
}

double EngS(double /*E*/)
{
	return 1.0;
}

double Dil(double /*AvgPresent*/, double /*DaysSinceGave*/)
{
	return 1.0;
}

double TipChance(double /*L*/, double /*Hype*/, double /*E*/, double /*Dilution*/)
{
	return 0.0;
}

double CheerChance(double /*L*/, double /*Hype*/, double /*E*/, double /*Dilution*/)
{
	return 0.0;
}

double SubChance(double /*L*/, double /*Hype*/, double /*E*/)
{
	return 0.0;
}

double RenewChance(double /*L*/, bool /*Gift*/)
{
	return 0.0;
}

double GiveHazard(double /*L*/, double /*Hype*/, double /*E*/, double /*Dilution*/, double /*Minutes*/, bool /*NearBigMoment*/)
{
	return 0.0;
}

Payout PayoutOf(Chips Unpaid, Chips Pending, int /*RentsPaid*/)
{
	// Neutral until the body lands: today's payout (the balance pays, nothing pending moves), so Session::PayOut's U3Kast path
	// pays as it always has.
	Payout Today;
	Today.Pay = Unpaid;
	Today.Pending = Pending;
	return Today;
}

bool SponsorReadPays(int /*TodayDay*/, double /*ReadAt*/, double /*LiveMinutes*/, int /*Count*/, int /*ReadsPaid*/, int /*ReadsAsk*/)
{
	return false;
}

// ------------------------------------------------------------------ the week's rings

void LiveAdd(std::array<int, 7>& /*Ring*/, int& /*RingDay*/, int /*Today*/, int /*Minutes*/)
{
}

double H7(const std::array<int, 7>& /*Ring*/, int /*RingDay*/, int /*Today*/)
{
	return 0.0;
}

bool Granted(int /*GrantDay*/, uint32_t /*GrantMask*/, int /*Today*/, uint32_t /*Bit*/)
{
	return false;
}

void UseGrant(int& /*GrantDay*/, uint32_t& /*GrantMask*/, int /*Today*/, uint32_t /*Bit*/)
{
}

// ------------------------------------------------------------------ the forecast and the directory rank

Band Forecast(const std::vector<double>& /*Attend*/, const std::vector<double>& /*S*/, double /*A0*/, double /*Q*/, double /*Bias*/)
{
	return Band();
}

Standing RankAt(double /*World*/, int /*Viewers*/)
{
	return Standing();
}

// ------------------------------------------------------------------ saves (nothing is written or read in Phase A-K)

void WriteLines(std::ostream& /*Out*/, const SaveView& /*V*/)
{
}

bool ReadLine(const std::vector<std::string>& /*P*/, SaveView& /*V*/)
{
	return false;
}

std::string MemberTail(const Member& /*M*/, int /*Today*/)
{
	return std::string();
}

bool ReadMemberTail(const std::vector<std::string>& /*P*/, Member& /*M*/)
{
	return false;
}

std::string LogTail(const StreamLog& /*L*/)
{
	return std::string();
}

bool ReadLogTail(const std::vector<std::string>& /*P*/, StreamLog& /*L*/)
{
	return false;
}

std::string GoodwillTail(int /*LastRaidDay*/)
{
	return std::string();
}

int ReadGoodwillTail(const std::vector<std::string>& /*P*/)
{
	return -1;
}
} // namespace growth
} // namespace kast
} // namespace ss
