#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Sites.h"

// The rival sites' side of the session (update 2 §5, §6.2): invitations, the Cashier, restrictions by site, the pots the
// money lands in, the event's settle by site, the calendar's step and the debug command. Phase A, day 1 (the lead): stubs
// that keep today's behaviour, every id routing to RiverLine. core-sites owns this file from here; dormant until G3.

namespace ss
{
void Session::SiteSettle(const Results& Res)
{
	// Phase A's stub: every event is RiverLine's, and RiverLine's money is the bank's (ShowResults paid it).
	(void)Res;
}

std::string Session::AnswerInvite(net::SiteId S, bool Accept)
{
	(void)S;
	(void)Accept;
	return "Only RiverLine's open to you.";
}

std::string Session::Deposit(net::SiteId S, Chips Cents, const std::string& Method)
{
	(void)S;
	(void)Cents;
	(void)Method;
	return "Only RiverLine's open to you.";
}

std::string Session::Withdraw(net::SiteId S, Chips Cents, const std::string& Method)
{
	(void)S;
	(void)Cents;
	(void)Method;
	return "Only RiverLine's open to you.";
}

std::string Session::ReverseWithdrawal(const std::string& TransferId)
{
	(void)TransferId;
	return "Only RiverLine's open to you.";
}

bool Session::Restricted(net::SiteId S) const
{
	// A RiverLine restriction is RiverLine's alone (§1.9 #12): the rivals don't know or care.
	return S == net::SiteId::RiverLine && Restricted();
}

stake::Pot Session::PotFor(net::SiteId S)
{
	// RiverLine's money is the bank's (U8). Phase A: every site is RiverLine.
	(void)S;
	stake::Pot P;
	P.Cents = &BankrollCents;
	P.Site = 0;
	return P;
}

void Session::SiteStep(double From, double To)
{
	// CheckCalendar's step after Rerun's (§1.6). Phase A's stub: nothing to step (a no-op until G3).
	(void)From;
	(void)To;
}

std::string Session::DebugSite(const std::string& Args)
{
	// Phase A's stub (ss.Site.*: game/Update2Commands.cpp).
	(void)Args;
	return "Sites: RiverLine only.";
}
} // namespace ss
