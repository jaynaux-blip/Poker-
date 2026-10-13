#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Video.h"

// Rerun's side of the session (update 2 §4, §6.2): the candidates the hands make, publishing (the edit's TimeSkip and its
// settle), the channel's view, the calendar's step and the debug command. Phase A, day 1 (the lead): stubs that keep
// today's behaviour (no candidates, no videos, no payouts). core-video owns this file from here.

namespace ss
{
void Session::VideoNoteHand(const Hand& H, const HandFacts& F)
{
	// Phase A's stub: the highlight candidate (§4.4) is core-video's, G2.
	(void)H;
	(void)F;
}

void Session::VideoRecap(const Results& Res)
{
	// Phase A's stub: the event's recap candidates are core-video's, G2.
	(void)Res;
}

std::vector<video::Candidate> Session::VideoCandidates() const
{
	return Life.Video.Open;
}

std::string Session::Publish(const video::Choice& C)
{
	// Phase A's stub.
	(void)C;
	return "Rerun isn't open yet.";
}

std::string Session::SettleEdit(const std::string& ActivityId)
{
	// Phase A's stub: FinishSkip's "video:" branch (nothing starts an edit yet).
	(void)ActivityId;
	return "";
}

std::string Session::DeleteVideo(const std::string& VideoId)
{
	(void)VideoId;
	return "Rerun isn't open yet.";
}

video::ChannelView Session::VideoView() const
{
	return video::View(Life.Video, WorldMinutes());
}

void Session::VideoStep(double From, double To)
{
	// CheckCalendar's step after the sponsors' (§1.6): views, subscribers, the review and the monthly payout.
	video::Step(Life.Video, Life, BankrollCents, Channel, From, To);
}

std::string Session::DebugVideo(const std::string& Args)
{
	// Phase A's stub (ss.Video.*: game/Update2Commands.cpp).
	(void)Args;
	const video::ChannelView V = VideoView();
	return "Rerun: " + std::to_string(static_cast<long long>(V.Subs)) + " subscribers, " + std::to_string(Life.Video.Videos.size()) + " videos, " +
		std::to_string(Life.Video.Open.size()) + " candidates.";
}
} // namespace ss
