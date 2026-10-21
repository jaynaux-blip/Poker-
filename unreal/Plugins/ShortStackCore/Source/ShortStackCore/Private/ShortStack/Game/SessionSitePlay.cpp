// Update 3 (SPEC §7.2): the Session's site-play methods, core-siteplay's file (the SessionStake.cpp precedent): the pages'
// view, the claims (each "" or why not, through site::play's keyed and idempotent claims), the post-hand hook, the
// calendar's step and ss.Site.Play. Phase A, day 1: stubs that keep today's behaviour (nothing claimed, stepped or noted).
#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/SitePlay.h"

#include <string>

namespace ss
{
site::play::View Session::SitePlayView(net::SiteId /*S*/) const
{
	return site::play::View();
}

std::string Session::SpinWheel()
{
	return "Not yet.";
}

std::string Session::OpenCracker(const std::string& /*BoxId*/)
{
	return "Not yet.";
}

std::string Session::ClaimDare(int /*Slot*/)
{
	return "Not yet.";
}

std::string Session::ClaimTier(int /*Tier*/)
{
	return "Not yet.";
}

std::string Session::BuyPrize(int /*Item*/)
{
	return "Not yet.";
}

std::string Session::Equip(int /*Slot*/, int /*Look*/)
{
	return "Not yet.";
}

std::string Session::StarEvent(const std::string& /*TemplateId*/, bool /*On*/)
{
	return "Not yet.";
}

std::string Session::CallOut(int /*Npc*/)
{
	return "Not yet.";
}

void Session::SitePlayHand(const Hand& /*H*/, const HandFacts& /*F*/)
{
}

void Session::SitePlayStep(double From, double To)
{
	// Phase A's seam (the lead): site::play::Step steps only a book a paid entry or a claim already made (never seeding one,
	// S10), and a League group it draws goes to the world, which records the group's own results (§3.6 R2). The letters it
	// hands back are core-siteplay's to post when its Step writes any.
	const site::play::StepOut Out = site::play::Step(Life.SitePlay, Life, Life.Sites, &LivingWorld, Life.Sponsors, WorldSeed(), From, To);
	if (Out.Roster)
	{
		LivingWorld.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
	}
}

std::string Session::DebugSitePlay(const std::string& /*Args*/)
{
	return "site play: not in this build yet";
}
} // namespace ss
