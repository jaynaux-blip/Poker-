// The creator update (SPEC_CREATORS §5.3, §5.5): every new Kast string, core-kastmath's file. One table holds every template
// (its key, its kind, its text with {Vars}); the accessors (KastCopy.h) fill them with the facts they're given, and
// kast::Inventory() and copy::Entries() hand the whole table to the copy lint and tests/Trademarks.h's screens (kast_growth's
// SegPure), like video::InventoryU3(). U1's ban covers it: "heads-down" is the only pace word in the world.
#include "KastCopy.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace kast
{
// The file's private helpers (Unreal's unity builds paste files together: every name here is prefixed).
namespace kastcopy_detail
{
struct KcRow
{
	const char* Key;
	copy::Kind Type;
	const char* Text;
};

// Keys starting "pool." are chat pools: every row with the key is one variant. The rest are unique.
const std::vector<KcRow>& KcRows()
{
	using copy::Kind;
	static const std::vector<KcRow> Rows = {
		// ---- K1 the Room
		{"k1.watching_chatting", Kind::Label, "{Watching} watching \xC2\xB7 {Chatting} chatting"},
		{"k1.watching", Kind::Label, "{N} watching"},
		{"k1.chatting", Kind::Label, "{N} chatting"},
		{"k1.mods", Kind::System, "{N} moderators are watching chat."},
		{"k1.mods.one", Kind::System, "1 moderator is watching chat."},
		{"k1.mods_here", Kind::Label, "{Here} of {Of} here"},
		{"k1.heads_down", Kind::Label, "heads-down tonight: chat is quieter"},
		{"k1.chatted", Kind::Label, "{N} people chatted tonight"},
		{"k1.chatted.one", Kind::Label, "1 person chatted tonight"},
		{"k1.nobody", Kind::Label, "nobody here right now"},
		// ---- K2 First Faces, First Follower, Doors open, Set the scene
		{"k2.golive", Kind::System, "You're live! Your ping reached {Reached} of {Of} followers."},
		{"k2.golive.one", Kind::System, "You're live! Your ping reached {Reached} of 1 follower."},
		{"k2.golive.none", Kind::System, "You're live! Share the link and say hi to chat."},
		{"k2.golive.afterglow", Kind::System, "You're live! Your ping reached {Reached} of {Of} \xC2\xB7 {Followed} followed you last night."},
		{"k2.golive.again", Kind::System, "You're live again. Kast pings your followers once every 4 hours."},
		{"k2.ping_toast", Kind::Toast, "Your ping reached {Reached} of {Of} followers ({Pct}%)"},
		{"k2.ping_toast.one", Kind::Toast, "Your ping reached {Reached} of 1 follower ({Pct}%)"},
		{"k2.dee.hi", Kind::Chat, "LIVE!! the laundromat is watching"},
		{"k2.mei.hi", Kind::Chat, "mei is here"},
		{"k2.dee.embercrest", Kind::Chat, "how was the Embercrest??"},
		{"k2.first_follower", Kind::Toast, "{Who} followed: your first follower who isn't family"},
		{"k2.first_follower.sub", Kind::Label, "your first follower who isn't family"},
		{"pool.k2.first_follower", Kind::Chat, "FIRST FOLLOWER kastPog"},
		{"pool.k2.first_follower", Kind::Chat, "first follower!!"},
		{"pool.k2.first_follower", Kind::Chat, "welcome to the rail kastLove"},
		{"pool.k2.first_follower", Kind::Chat, "the first of many"},
		{"k2.empty.rank", Kind::System, "Your stream is #{Rank} of {Of} live in Poker."},
		{"k2.empty.seen", Kind::System, "{Saw} people saw your stream in the directory in the last 30 minutes; {Came} came in."},
		{"k2.empty.seen.one", Kind::System, "1 person saw your stream in the directory in the last 30 minutes; {Came} came in."},
		{"k2.empty.seen.none_in", Kind::System, "{Saw} people saw your stream in the directory in the last 30 minutes; none came in yet."},
		{"k2.empty.seen.one_none", Kind::System, "1 person saw your stream in the directory in the last 30 minutes; nobody came in yet."},
		{"k2.empty.seen.zero", Kind::System, "Nobody saw your stream in the directory in the last 30 minutes."},
		{"k2.empty.saw", Kind::Label, "{Saw} saw you in the directory"},
		{"k2.empty.fresh", Kind::System, "New on Kast: you're on the Fresh shelf tonight."},
		{"k2.empty.rising", Kind::System, "Rising in Poker: Kast shows small channels to new viewers."},
		{"k2.scene.button", Kind::Label, "Set the scene"},
		{"k2.scene", Kind::System, "You talk to the camera about tonight's tournament."},
		// ---- K3 the Ping, Run Alerts, Rail Call
		{"k3.ping.reach", Kind::Label, "Ping reach last stream {Pct}%"},
		{"k3.ping.title", Kind::Label, "THE PING"},
		{"k3.ping.part.0", Kind::Label, "this week"},
		{"k3.ping.part.1", Kind::Label, "this month"},
		{"k3.ping.part.2", Kind::Label, "lapsed"},
		{"k3.alert.sent", Kind::Label, "RUN ALERT sent to {N} followers"},
		{"k3.alert.sent.one", Kind::Label, "RUN ALERT sent to 1 follower"},
		{"k3.alert.back", Kind::Label, "{N} back so far"},
		{"k3.alert.strip", Kind::Label, "RUN ALERT \xC2\xB7 {N} back so far"},
		{"pool.k3.return.bubble", Kind::Chat, "back for the bubble!"},
		{"pool.k3.return.bubble", Kind::Chat, "made it back for the bubble"},
		{"pool.k3.return.bubble", Kind::Chat, "bubble?? I'm here"},
		{"pool.k3.return.bubble", Kind::Chat, "hand for hand, I'm back"},
		{"pool.k3.return.itm", Kind::Chat, "back! are we in the money?"},
		{"pool.k3.return.itm", Kind::Chat, "came back for the cash"},
		{"pool.k3.return.itm", Kind::Chat, "in the money?? I'm here"},
		{"pool.k3.return.itm", Kind::Chat, "back for the run, gl"},
		{"pool.k3.return.ft", Kind::Chat, "back for the FT!"},
		{"pool.k3.return.ft", Kind::Chat, "you're at the final table??"},
		{"pool.k3.return.ft", Kind::Chat, "FINAL TABLE?! I'm here"},
		{"pool.k3.return.ft", Kind::Chat, "left dinner for this kastPog"},
		{"pool.k3.return.top3", Kind::Chat, "back for the top 3!"},
		{"pool.k3.return.top3", Kind::Chat, "three left?? I'm here"},
		{"pool.k3.return.top3", Kind::Chat, "top 3!! made it back"},
		{"pool.k3.return.top3", Kind::Chat, "I had to see this"},
		{"k3.rail.button", Kind::Label, "RAIL CALL"},
		{"k3.rail.reached", Kind::System, "Your rail call reached {N} followers."},
		{"k3.rail.reached.one", Kind::System, "Your rail call reached 1 follower."},
		{"pool.k3.rail.arrive", Kind::Chat, "rail call got me"},
		{"pool.k3.rail.arrive", Kind::Chat, "got the rail call, gl"},
		{"pool.k3.rail.arrive", Kind::Chat, "here for the rail"},
		{"pool.k3.rail.arrive", Kind::Chat, "rail call! made it"},
		{"pool.k3.rail.arrive", Kind::Chat, "saw the rail call, lets go"},
		{"k3.rail.dee", Kind::Chat, "omw!!"},
		{"k3.rail.mei", Kind::Chat, "mei on the rail"},
		{"k3.rail.whole", Kind::Label, "Your whole rail is here"},
		// ---- K4 Heat, Front Row, Stamina
		{"k4.tier.0", Kind::Label, "Cold"},
		{"k4.tier.1", Kind::Label, "Warming up"},
		{"k4.tier.2", Kind::Label, "Running hot"},
		{"k4.tier.3", Kind::Label, "On a heater"},
		{"k4.tier.4", Kind::Label, "Sun run"},
		{"k4.title", Kind::Label, "HEAT"},
		{"k4.ghost", Kind::Label, "tonight {Gain} so far"},
		{"k4.ghost.settling", Kind::Label, "tonight: settling in"},
		{"k4.watching_tier", Kind::Label, "{N} watching \xC2\xB7 {Tier}"},
		{"k4.effects", Kind::Label, "Heat {Heat} \xC2\xB7 lifts the directory, turnout and your ping"},
		{"k4.cold_means", Kind::System, "Cold means you went dark 7+ days: while you stream every week it never reads Cold."},
		{"k4.why.run", Kind::Label, "{When} {Stage} ({Runners} runners)"},
		{"k4.why.when.tonight", Kind::Label, "Tonight's"},
		{"k4.why.when.last", Kind::Label, "Last night's"},
		{"k4.why.when.day", Kind::Label, "{Day}'s"},
		{"k4.why.stage.0", Kind::Label, "deep run"},
		{"k4.why.stage.1", Kind::Label, "bubble run"},
		{"k4.why.stage.2", Kind::Label, "cash"},
		{"k4.why.stage.3", Kind::Label, "final table"},
		{"k4.why.stage.4", Kind::Label, "top 3"},
		{"k4.why.stage.5", Kind::Label, "win"},
		{"k4.why.over", Kind::Label, "{Pct}% over your forecast"},
		{"k4.why.under", Kind::Label, "{Pct}% under your forecast"},
		{"k4.why.show", Kind::Label, "Showing up: {N} nights this week"},
		{"k4.why.show.one", Kind::Label, "Showing up: 1 night this week"},
		{"k4.why.cooling", Kind::Label, "Cooling: {N} days dark"},
		{"k4.why.cooling.one", Kind::Label, "Cooling: 1 day dark"},
		{"k4.why.clip", Kind::Label, "Your clip hit the top of the Kast feed"},
		{"k4.why.video.pushed", Kind::Label, "Your video {Title} got pushed hard"},
		{"k4.why.video.breakout", Kind::Label, "Your video {Title} broke out"},
		{"k4.why.hotweek", Kind::Label, "Hot week: Heat can't climb any faster this week"},
		{"k4.why.record", Kind::Label, "Your biggest room yet ({Peak} at the peak)"},
		{"k4.why.raid", Kind::Label, "{Who}'s raid of {Size}"},
		{"k4.why.milestone", Kind::Label, "{N} followers"},
		{"k4.why.steady", Kind::Label, "Steady: nights near your forecast"},
		{"k4.why.new", Kind::Label, "New channel: Kast is getting to know you"},
		// The C-K PIE fix (F4): a final table off air, from the channel's own days (no field size kept), and a night under 60 world
		// minutes that moved no Heat says why.
		{"k4.why.run.plain", Kind::Label, "{When} {Stage}"},
		{"k4.why.short", Kind::Label, "Under an hour: only a deep run counts"},
		{"k4.why.short.counted", Kind::Label, "This run's Heat already counted"},
		{"k4.why.short.usual", Kind::Label, "Runs this deep are your usual now"},
		{"k4.why.short.before", Kind::Label, "Stages before going live don't count"}, // review-pie-room: live at a formed final table
		{"k4.why.ceiling", Kind::Label, "Heat is at its ceiling"},
		{"pool.k4.tierup", Kind::Chat, "chat is cooking"},
		{"pool.k4.tierup", Kind::Chat, "the room is up tonight"},
		{"pool.k4.tierup", Kind::Chat, "look at this chat go"},
		{"pool.k4.tierup", Kind::Chat, "we're cooking now kastHype"},
		{"k4.front", Kind::System, "Kast put you in Front Row in Poker."},
		{"k4.stamina", Kind::System, "Stamina: {Hours} hours live this week. Chat can tell."},
		{"k4.stamina.title", Kind::Label, "STAMINA"},
		{"k4.stamina.note", Kind::System, "Shown from 24 hours live in 7 days. A day off brings it back."},
		// ---- K5 the Sweat, Event Mode
		{"k5.bar.ft", Kind::Label, "FINAL TABLE \xC2\xB7 {Event} \xC2\xB7 {Left} left of {Of}"},
		{"k5.bar.two", Kind::Label, "LAST TWO TABLES \xC2\xB7 {Event} \xC2\xB7 {Left} left of {Of}"},
		{"k5.chip", Kind::Label, "FINAL TABLE"},
		{"k5.directory", Kind::Label, "FINAL TABLE \xC2\xB7 {Event} \xC2\xB7 {Left} left"},
		{"pool.k5.sweat", Kind::Chat, "ICM spot here"},
		{"pool.k5.sweat", Kind::Chat, "fold fold fold"},
		{"pool.k5.sweat", Kind::Chat, "THIS IS IT"},
		{"pool.k5.sweat", Kind::Chat, "hand for hand"},
		{"pool.k5.sweat", Kind::Chat, "who is the short stack?"},
		{"pool.k5.sweat", Kind::Chat, "every fold is money now"},
		{"pool.k5.sweat", Kind::Chat, "ladder ladder ladder"},
		{"pool.k5.sweat", Kind::Chat, "my heart can't take this"},
		{"k5.shelf", Kind::Label, "Sweating now"},
		{"k5.shelf.sub", Kind::Label, "streams at a deep run, live"},
		// ---- K6 the Broadcast, big names
		{"k6.broadcast.riverline", Kind::System, "RiverLine Live sent {N} viewers: one of our final tablists is streaming."},
		{"k6.broadcast.ignite", Kind::System, "Ignite's Bandstand sent {N} viewers."},
		{"k6.broadcast.runittwice", Kind::System, "RunItTwice's Newsdesk sent {N} viewers."},
		{"k6.broadcast.other", Kind::System, "The site's broadcast sent {N} viewers."},
		// The tease's price is a fact where it's named: RiverLine's and Ignite's cheapest nightly events the Broadcast features
		// (P 1 at their usual fields) are $5.40 and $5.50; RunItTwice's start at $33 (its $5.50 nightlies never reach P 1), so its
		// tease names no price (kast_growth's SegPure checks the schedules).
		{"k6.tease.riverline", Kind::Label, "RiverLine Live features final tables from $5"},
		{"k6.tease.ignite", Kind::Label, "Ignite's Bandstand features final tables from $5"},
		{"k6.tease.runittwice", Kind::Label, "RunItTwice's Newsdesk features its biggest final tables"},
		{"k6.tease.other", Kind::Label, "The sites feature their biggest final tables"},
		{"pool.k6.chat.riverline", Kind::Chat, "RiverLine Live brought me, gl"},
		{"pool.k6.chat.riverline", Kind::Chat, "from the RiverLine broadcast o7"},
		{"pool.k6.chat.riverline", Kind::Chat, "the broadcast said final table, so here I am"},
		{"pool.k6.chat.ignite", Kind::Chat, "the Bandstand sent me over"},
		{"pool.k6.chat.ignite", Kind::Chat, "from the Bandstand, gl at the FT"},
		{"pool.k6.chat.ignite", Kind::Chat, "Ignite's broadcast brought me o7"},
		{"pool.k6.chat.runittwice", Kind::Chat, "the Newsdesk sent me, gl"},
		{"pool.k6.chat.runittwice", Kind::Chat, "from the RunItTwice broadcast o7"},
		{"pool.k6.chat.runittwice", Kind::Chat, "Newsdesk said final table, I'm here"},
		{"pool.k6.chat.other", Kind::Chat, "the broadcast sent me, gl"},
		{"pool.k6.chat.other", Kind::Chat, "here from the broadcast o7"},
		{"k6.raided", Kind::Toast, "{Who} raided with {Size}"},
		// ---- K7 Fresh on Kast, Rising in Poker, Poker Rank
		{"k7.chip.new", Kind::Label, "NEW ON KAST"},
		{"k7.chip.rising", Kind::Label, "RISING IN POKER"},
		{"k7.shelf.fresh", Kind::Label, "Fresh on Kast"},
		{"k7.shelf.fresh.sub", Kind::Label, "new channels: Kast shows them to new viewers"},
		{"k7.shelf.rising", Kind::Label, "Rising in Poker"},
		{"k7.shelf.rising.sub", Kind::Label, "small channels: Kast shows them to new viewers"},
		{"k7.rank", Kind::Label, "#{Rank} of {Of} in Poker"},
		{"k7.rank.now", Kind::Label, "You're #{Rank} in Poker right now"},
		{"k7.rank.climb", Kind::Label, "You climbed from #{From} to #{To} in Poker tonight"},
		{"k7.rank.best", Kind::Label, "Best tonight: #{Rank} in Poker"},
		// ---- K8 Where They Came From (View::Source's order)
		{"k8.tag.0", Kind::Label, "returning"},
		{"k8.tag.1", Kind::Label, "directory"},
		{"k8.tag.2", Kind::Label, "fresh"},
		{"k8.tag.3", Kind::Label, "rising"},
		{"k8.tag.4", Kind::Label, "railbirds"},
		{"k8.tag.5", Kind::Label, "raid"},
		{"k8.tag.6", Kind::Label, "broadcast"},
		{"k8.tag.7", Kind::Label, "word of mouth"},
		{"k8.tag.8", Kind::Label, "video"},
		{"k8.tag.9", Kind::Label, "clip"},
		{"k8.tag.10", Kind::Label, "lurking"},
		{"k8.cause.0", Kind::Label, "the ping"},
		{"k8.cause.1", Kind::Label, "the directory"},
		{"k8.cause.2", Kind::Label, "Fresh on Kast"},
		{"k8.cause.3", Kind::Label, "Rising in Poker"},
		{"k8.cause.4", Kind::Label, "railbirds"},
		{"k8.cause.5", Kind::Label, "raids"},
		{"k8.cause.6", Kind::Label, "the Broadcast"},
		{"k8.cause.7", Kind::Label, "word of mouth"},
		{"k8.cause.8", Kind::Label, "your video"},
		{"k8.cause.9", Kind::Label, "your clip"},
		{"k8.cause.10", Kind::Label, "lurkers"},
		{"k8.count", Kind::Label, "{N} {Tag}"},
		{"k8.came", Kind::Label, "Came because of"},
		{"k8.followed", Kind::Label, "Followed:"},
		{"k8.from_video", Kind::Chat, "came from the {Title} video"},
		{"k8.from_clip", Kind::Chat, "came from your clip"},
		// ---- K9 the Forecast
		{"k9.band", Kind::Label, "Tonight: likely {Lo}-{Hi}"},
		{"k9.band.one", Kind::Label, "Tonight: likely {Lo}"},
		{"k9.context", Kind::Label, "{Day} \xC2\xB7 {Schedule} \xC2\xB7 {Tier}"},
		{"k9.sched.0", Kind::Label, "no schedule posted"},
		{"k9.sched.1", Kind::Label, "on schedule"},
		{"k9.sched.2", Kind::Label, "off schedule"},
		{"k9.short", Kind::Label, "forecast {Lo}-{Hi}"},
		{"k9.verdict.above", Kind::Label, "above your forecast"},
		{"k9.verdict.inside", Kind::Label, "inside your forecast"},
		{"k9.verdict.under", Kind::Label, "under your forecast"},
		// ---- K10 raids
		{"k10.welcome.button", Kind::Label, "WELCOME THE RAID"},
		{"k10.welcome_raid", Kind::System, "{Who} raided with {Size}: welcome to Kast!"},
		{"k10.welcome.streamer", Kind::Chat, "Welcome in, raiders! Grab a seat on the rail."},
		{"pool.k10.welcomed", Kind::Chat, "ty for the welcome!"},
		{"pool.k10.welcomed", Kind::Chat, "thanks for having us"},
		{"pool.k10.welcomed", Kind::Chat, "o7 from the raid"},
		{"pool.k10.welcomed", Kind::Chat, "love the welcome kastLove"},
		{"k10.owed", Kind::Label, "{N} channels owe you a raid"},
		{"k10.owed.one", Kind::Label, "1 channel owes you a raid"},
		{"k10.owed.none", Kind::Label, "No channel owes you a raid yet"},
		{"k10.owed.short", Kind::Label, "{N} raids owed"},
		{"k10.owed.short.one", Kind::Label, "1 raid owed"},
		{"k10.owed.short.none", Kind::Label, "no raids owed"},
		{"k10.raided_ago", Kind::Label, "You raided {Who} {N} days ago (no new token until {Day})"},
		{"k10.raided_ago.today", Kind::Label, "You raided {Who} today (no new token until {Day})"},
		{"k10.raided_ago.yesterday", Kind::Label, "You raided {Who} yesterday (no new token until {Day})"},
		{"k10.token_rule", Kind::System, "A raid only earns a token after an hour live, and once a week per channel. They come back when they come back."},
		{"k10.retention", Kind::Label, "{Who} raided with {Size} \xC2\xB7 still here at 15 min: {At15} \xC2\xB7 followed: {Followed}"},
		{"k10.retention.broadcast", Kind::Label, "{Who} sent {Size} \xC2\xB7 still here at 15 min: {At15} \xC2\xB7 followed: {Followed}"},
		// ---- K11 Thank, Answer, Nod
		{"k11.nod", Kind::Label, "Nod"},
		{"k11.nods_left", Kind::Label, "{N} Nods left"},
		{"k11.nods_left.one", Kind::Label, "1 Nod left"},
		{"k11.nods_left.none", Kind::Label, "No Nods left"},
		{"pool.k11.nod_reply", Kind::Chat, "he noticed me kastLove"},
		{"pool.k11.nod_reply", Kind::Chat, "got noticed!!"},
		{"pool.k11.nod_reply", Kind::Chat, "the nod!! kastPog"},
		{"pool.k11.nod_reply", Kind::Chat, "omg hi"},
		{"k11.thank_heard", Kind::Label, "(chat heard you)"},
		{"pool.k11.answer_followed", Kind::Chat, "thanks for answering! followed"},
		{"pool.k11.answer_followed", Kind::Chat, "that helped, followed"},
		{"pool.k11.answer_followed", Kind::Chat, "great answer, hit follow"},
		// ---- K12 Hold or Crack
		{"k12.title", Kind::Label, "HOLD OR CRACK?"},
		{"k12.counts", Kind::Label, "{Holds} HOLDS \xC2\xB7 {Cracks} CRACKS"},
		{"k12.shares", Kind::Label, "HOLDS {Holds}% \xC2\xB7 CRACKS {Cracks}%"},
		{"k12.equity", Kind::Label, "your equity {Pct}%"},
		{"k12.callit.holds", Kind::Label, "Call it: HOLDS"},
		{"k12.callit.cracks", Kind::Label, "Call it: CRACKS"},
		{"k12.streamer.holds", Kind::Chat, "HOLDS. calling it"},
		{"k12.streamer.cracks", Kind::Chat, "CRACKS. calling it"},
		{"pool.k12.chat.holds", Kind::Chat, "HOLDS"},
		{"pool.k12.chat.holds", Kind::Chat, "holds holds holds"},
		{"pool.k12.chat.holds", Kind::Chat, "it holds"},
		{"pool.k12.chat.holds", Kind::Chat, "HOLD kastPog"},
		{"pool.k12.chat.holds", Kind::Chat, "this one holds"},
		{"pool.k12.chat.cracks", Kind::Chat, "CRACKS"},
		{"pool.k12.chat.cracks", Kind::Chat, "it cracks, sorry"},
		{"pool.k12.chat.cracks", Kind::Chat, "cracks kastRIP"},
		{"pool.k12.chat.cracks", Kind::Chat, "feeling a crack"},
		{"pool.k12.chat.cracks", Kind::Chat, "the river gets him"},
		{"k12.reveal.right", Kind::System, "{Pct}% of chat called it."},
		{"k12.reveal.line", Kind::System, "{Call}! {Pct}% of chat called it: {Points} points."},
		{"k12.reveal.line.few", Kind::System, "{Call}! {Right} of {Of} called it: {Points} points."},
		{"k12.reveal.line.none", Kind::System, "{Call}! Nobody in chat called it."},
		{"k12.reveal.holds", Kind::Label, "HOLDS"},
		{"k12.reveal.cracks", Kind::Label, "CRACKS"},
		{"pool.k12.reveal.wrong", Kind::Chat, "the rail is in shambles"},
		{"pool.k12.reveal.wrong", Kind::Chat, "nobody saw that coming"},
		{"pool.k12.reveal.wrong", Kind::Chat, "chat is in shambles"},
		{"pool.k12.called", Kind::Chat, "called it!"},
		{"pool.k12.called", Kind::Chat, "never in doubt"},
		{"pool.k12.called", Kind::Chat, "told you"},
		{"pool.k12.called", Kind::Chat, "easy call kastHype"},
		{"pool.k12.missed", Kind::Chat, "pain"},
		{"pool.k12.missed", Kind::Chat, "I believed"},
		{"pool.k12.missed", Kind::Chat, "noooo"},
		{"pool.k12.missed", Kind::Chat, "should've called it"},
		{"k12.best_caller", Kind::Label, "{Who} called {Calls} of {Of}"},
		// ---- K15 Clip Lane, Clip of the Night
		{"k15.clip.feed", Kind::Label, "Clip of the night: \xE2\x80\x9C{Title}\xE2\x80\x9D \xC2\xB7 in the Kast feed"},
		{"k15.clip.channel", Kind::Label, "Clip of the night: \xE2\x80\x9C{Title}\xE2\x80\x9D \xC2\xB7 on your channel"},
		{"k15.clip_it", Kind::Label, "Clip it"},
		{"k15.make_cut", Kind::Label, "Make it a Cut"},
		{"k15.top", Kind::Toast, "Your clip is at the top of the Kast feed"},
		// ---- K16 Welcome back, the streak
		{"k16.welcome_back", Kind::System, "Welcome back: {N} familiar faces checked in."},
		{"k16.welcome_back.one", Kind::System, "Welcome back: 1 familiar face checked in."},
		{"k16.streak", Kind::Label, "Streak {N} weeks"},
		{"k16.streak.one", Kind::Label, "Streak 1 week"},
		// ---- K17 the end card, stings, the letter
		{"k17.peak", Kind::Label, "Peak {Peak} {At}, {Time}"},
		{"k17.peak.plain", Kind::Label, "Peak {Peak} at {Time}"},
		{"k17.at.0", Kind::Label, "at a new tournament"},
		{"k17.at.1", Kind::Label, "on an all-in"},
		{"k17.at.2", Kind::Label, "on an all-in that held"},
		{"k17.at.3", Kind::Label, "on an all-in"},
		{"k17.at.4", Kind::Label, "on a bad beat"},
		{"k17.at.5", Kind::Label, "on a big pot"},
		{"k17.at.6", Kind::Label, "on a knockout"},
		{"k17.at.7", Kind::Label, "at the bubble"},
		{"k17.at.8", Kind::Label, "in the money"},
		{"k17.at.9", Kind::Label, "at the final table"},
		{"k17.at.10", Kind::Label, "at the win"},
		{"k17.at.11", Kind::Label, "at the bust"},
		{"k17.at.12", Kind::Label, "at the cash"},
		{"k17.at.13", Kind::Label, "on a big play"},
		{"k17.at.14", Kind::Label, "on a blunder"},
		{"k17.at.15", Kind::Label, "with the rival at the table"},
		{"k17.at.16", Kind::Label, "at a level change"},
		{"k17.at.17", Kind::Label, "at three left"},
		{"k17.at.raid", Kind::Label, "on a raid"}, // the C-K PIE fix (F1): a peak a network raid brought (PeakRaid)
		{"k17.followers", Kind::Label, "+{N}"},
		{"k17.followers.best", Kind::Label, "+{N} \xC2\xB7 best night ever"},
		{"k17.rollcall.one", Kind::Label, "{A} sweated the whole run"},
		{"k17.rollcall.two", Kind::Label, "{A} and {B} sweated the whole run"},
		{"k17.rollcall.more", Kind::Label, "{A}, {B} and {More} more sweated the whole run"},
		{"k17.heat.move", Kind::Label, "{From} \xE2\x86\x92 {To} \xC2\xB7 {Gain}"},
		{"k17.heat.same", Kind::Label, "{To} \xC2\xB7 {Gain}"},
		{"k17.tonight", Kind::Label, "Tonight: tips {Tips} \xC2\xB7 cheers {Cheers}"},
		{"k17.tonight.tips", Kind::Label, "Tonight: tips {Tips}"},
		{"k17.renewals", Kind::Label, "While you were away: renewals {Amount}"},
		{"k17.tipped", Kind::Label, "{Who} tipped {Amount}"},
		{"k17.pending", Kind::Label, "pending: {Amount}, paid after your next rent"},
		{"k17.pending.note", Kind::Label, "pending: paid after your next rent"},
		{"k17.goal", Kind::Label, "{ToGo} to {Goal} followers"},
		{"k17.first_payout", Kind::Toast, "Your first Kast payout: {Amount}"},
		{"k17.sponsor.read", Kind::Label, "{Brand}: stream read"},
		{"k17.sponsor.reads", Kind::Label, "{Brand}: {N} stream reads"},
		{"k17.sponsor.hours", Kind::Label, "{Brand}: {N} hours on stream"},
		{"k17.sponsor.hours.one", Kind::Label, "{Brand}: 1 hour on stream"},
		{"k17.sponsor.live_hours", Kind::Label, "{Brand}: live hours"},
		{"k17.sponsor.done", Kind::Label, "{Brand}: read done today"},
		{"k17.done", Kind::Label, "Done"},
		{"k17.milestone", Kind::System, "Milestone: {N} followers!"},
		{"k17.first_tip", Kind::System, "First tip from someone new: {Who} sent {Amount}!"},
		{"k17.first_raid", Kind::System, "Your first raid: {Who} brought {Size}!"},
		{"k17.sting.followers", Kind::Toast, "{N} followers!"},
		{"k17.sting.stranger_tip", Kind::Toast, "Your first tip from someone new!"},
		{"k17.sting.first_raid", Kind::Toast, "Your first raid!"},
		{"k17.sting.affiliate", Kind::Toast, "You're a Kast Affiliate!"},
		{"k17.sting.partner", Kind::Toast, "You're a Kast Partner!"},
		{"k17.letter.from", Kind::Label, "Kast"},
		{"k17.letter.subject", Kind::Label, "New on Kast"},
		{"k17.letter.body", Kind::Letter,
			"New on Kast: your ping, Heat and the Broadcast. Creator payouts now follow how people actually watch: per visit, not per hour."},
		// ---- K20 Kast money (the Community page)
		{"k20.sponsor_apart", Kind::System, "Sponsor pay is its own ledger line, never part of the Kast payout."},
		{"k20.partner_avg", Kind::System, "Partner's average counts your own viewers: raids and the Broadcast don't count."},
		{"k20.affiliate_bar", Kind::Label, "{Followers} followers \xC2\xB7 {Minutes} minutes \xC2\xB7 {Days} days live \xC2\xB7 an average of {Average}, over 30 days"},
		{"k20.affiliate_friends", Kind::System, "Dee and Mei subscribed the night it landed."},
		{"k20.affiliate_to_go", Kind::Label, "{ToGo} to go: Affiliate at {Followers}"},
	};
	return Rows;
}

/** A unique row's text by its key ("" for a key that isn't one: kast_growth's SegPure calls every accessor). */
std::string KcText(const char* Key)
{
	for (const KcRow& Row : KcRows())
	{
		if (std::strcmp(Row.Key, Key) == 0)
		{
			return Row.Text;
		}
	}
	return std::string();
}

/** A chat pool: every row with the key, in order. */
std::vector<std::string> KcPool(const char* Key)
{
	std::vector<std::string> Out;
	for (const KcRow& Row : KcRows())
	{
		if (std::strcmp(Row.Key, Key) == 0)
		{
			Out.push_back(Row.Text);
		}
	}
	return Out;
}

/** Template with each {Name} of Vars replaced (copylint's Fill, for the game). */
std::string KcFill(const std::string& Template, std::initializer_list<std::pair<const char*, std::string>> Vars)
{
	std::string Out = Template;
	for (const auto& Var : Vars)
	{
		const std::string Key = std::string("{") + Var.first + "}";
		for (size_t At = Out.find(Key); At != std::string::npos; At = Out.find(Key, At + Var.second.size()))
		{
			Out.replace(At, Key.size(), Var.second);
		}
	}
	return Out;
}

std::string KcSigned(double Gain)
{
	const long long G = std::llround(Gain);
	return (G < 0 ? "-" : "+") + Grouped(G < 0 ? -G : G);
}

int KcPct(double Part, double Whole)
{
	return Whole > 0.0 ? static_cast<int>(std::lround(100.0 * Part / Whole)) : 0;
}

/** Text cut to Keep UTF-8 characters (code points) and an ellipsis when it is longer than Max. */
std::string KcShort(const std::string& Text, size_t Max, size_t Keep)
{
	size_t Chars = 0;
	for (const char Ch : Text)
	{
		Chars += (static_cast<unsigned char>(Ch) & 0xC0u) != 0x80u ? 1u : 0u;
	}
	if (Chars <= Max)
	{
		return Text;
	}
	size_t Cut = 0;
	size_t Seen = 0;
	for (; Cut < Text.size(); ++Cut)
	{
		if ((static_cast<unsigned char>(Text[Cut]) & 0xC0u) != 0x80u)
		{
			if (Seen == Keep)
			{
				break;
			}
			++Seen;
		}
	}
	std::string Out = Text.substr(0, Cut);
	while (!Out.empty() && (Out.back() == ' ' || Out.back() == ',' || Out.back() == '.' || Out.back() == ':'))
	{
		Out.pop_back();
	}
	return Out + "\xE2\x80\xA6";
}

const char* KcSiteKey(int Site)
{
	switch (static_cast<net::SiteId>(Site))
	{
	case net::SiteId::RiverLine: return "riverline";
	case net::SiteId::Ignite: return "ignite";
	case net::SiteId::RunItTwice: return "runittwice";
	default: return "other";
	}
}
} // namespace kastcopy_detail

std::vector<std::string> Inventory()
{
	std::vector<std::string> Out;
	for (const kastcopy_detail::KcRow& Row : kastcopy_detail::KcRows())
	{
		Out.push_back(Row.Text);
	}
	return Out;
}

namespace copy
{
std::vector<Entry> Entries()
{
	std::vector<Entry> Out;
	for (const kastcopy_detail::KcRow& Row : kastcopy_detail::KcRows())
	{
		Out.push_back({Row.Key, Row.Type, Row.Text});
	}
	return Out;
}

// ------------------------------------------------------------------ numbers in copy

std::string Dollars(Chips Cents)
{
	if (Cents % 100 == 0)
	{
		const Chips Whole = Cents / 100;
		return (Whole < 0 ? "-$" : "$") + Grouped(Whole < 0 ? -Whole : Whole);
	}
	return Money(Cents);
}

std::string Number(int N)
{
	return Grouped(N);
}

// ------------------------------------------------------------------ K1

std::string WatchingChatting(int Watching, int Chatting)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k1.watching_chatting");
	return KcFill(T, {{"Watching", Grouped(Watching)}, {"Chatting", Grouped(Chatting)}});
}

std::string Watching(int N)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k1.watching");
	return KcFill(T, {{"N", Grouped(N)}});
}

std::string Chatting(int N)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k1.chatting");
	return KcFill(T, {{"N", Grouped(N)}});
}

std::string ModsWatching(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k1.mods");
	static const std::string One = KcText("k1.mods.one");
	return N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

std::string ModsHere(int Here, int Of)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k1.mods_here");
	return KcFill(T, {{"Here", Grouped(Here)}, {"Of", Grouped(Of)}});
}

std::string HeadsDown()
{
	static const std::string T = kastcopy_detail::KcText("k1.heads_down");
	return T;
}

std::string ChattedTonight(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k1.chatted");
	static const std::string One = KcText("k1.chatted.one");
	return N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

std::string NobodyHere()
{
	static const std::string T = kastcopy_detail::KcText("k1.nobody");
	return T;
}

// ------------------------------------------------------------------ K2

std::string GoLive(int Reached, int Of)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k2.golive");
	static const std::string One = KcText("k2.golive.one");
	static const std::string None = KcText("k2.golive.none");
	if (Of <= 0)
	{
		return None;
	}
	return KcFill(Of == 1 ? One : Many, {{"Reached", Grouped(Reached)}, {"Of", Grouped(Of)}});
}

std::string GoLiveAfterglow(int Reached, int Of, int FollowedLastNight)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k2.golive.afterglow");
	return KcFill(T, {{"Reached", Grouped(Reached)}, {"Of", Grouped(Of)}, {"Followed", Grouped(FollowedLastNight)}});
}

std::string GoLiveNoPing()
{
	static const std::string T = kastcopy_detail::KcText("k2.golive.again");
	return T;
}

std::string PingToast(int Reached, int Of)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k2.ping_toast");
	static const std::string One = KcText("k2.ping_toast.one");
	return KcFill(Of == 1 ? One : Many, {{"Reached", Grouped(Reached)}, {"Of", Grouped(Of)}, {"Pct", std::to_string(KcPct(Reached, Of))}});
}

std::string DeeHi()
{
	static const std::string T = kastcopy_detail::KcText("k2.dee.hi");
	return T;
}

std::string MeiHi()
{
	static const std::string T = kastcopy_detail::KcText("k2.mei.hi");
	return T;
}

std::string DeeEmbercrest()
{
	static const std::string T = kastcopy_detail::KcText("k2.dee.embercrest");
	return T;
}

std::string FirstFollower(const std::string& Who)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k2.first_follower");
	return KcFill(T, {{"Who", Who}});
}

std::string FirstFollowerSub()
{
	static const std::string T = kastcopy_detail::KcText("k2.first_follower.sub");
	return T;
}

const std::vector<std::string>& FirstFollowerChat()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k2.first_follower");
	return P;
}

std::string EmptyRank(int Rank, int Of)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k2.empty.rank");
	return KcFill(T, {{"Rank", Grouped(Rank)}, {"Of", Grouped(Of)}});
}

std::string EmptySeen(int Saw, int Came)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k2.empty.seen");
	static const std::string One = KcText("k2.empty.seen.one");
	static const std::string ManyNone = KcText("k2.empty.seen.none_in");
	static const std::string OneNone = KcText("k2.empty.seen.one_none");
	static const std::string Zero = KcText("k2.empty.seen.zero");
	if (Saw <= 0)
	{
		return Zero;
	}
	if (Came <= 0)
	{
		return Saw == 1 ? OneNone : KcFill(ManyNone, {{"Saw", Grouped(Saw)}});
	}
	return KcFill(Saw == 1 ? One : Many, {{"Saw", Grouped(Saw)}, {"Came", Grouped(Came)}});
}

std::string SawYou(int Saw)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k2.empty.saw");
	return KcFill(T, {{"Saw", Grouped(Saw)}});
}

std::string EmptyFresh()
{
	static const std::string T = kastcopy_detail::KcText("k2.empty.fresh");
	return T;
}

std::string EmptyRising()
{
	static const std::string T = kastcopy_detail::KcText("k2.empty.rising");
	return T;
}

std::string SetTheSceneButton()
{
	static const std::string T = kastcopy_detail::KcText("k2.scene.button");
	return T;
}

std::string SetTheScene()
{
	static const std::string T = kastcopy_detail::KcText("k2.scene");
	return T;
}

// ------------------------------------------------------------------ K3

std::string PingReach(int Pct)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k3.ping.reach");
	return KcFill(T, {{"Pct", std::to_string(Pct)}});
}

std::string PingTitle()
{
	static const std::string T = kastcopy_detail::KcText("k3.ping.title");
	return T;
}

std::string PingPart(int Part)
{
	using namespace kastcopy_detail;
	static const std::string Parts[3] = {KcText("k3.ping.part.0"), KcText("k3.ping.part.1"), KcText("k3.ping.part.2")};
	return Parts[static_cast<size_t>(std::max(0, std::min(2, Part)))];
}

std::string RunAlertSent(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k3.alert.sent");
	static const std::string One = KcText("k3.alert.sent.one");
	return N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

std::string RunAlertBack(int N)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k3.alert.back");
	return KcFill(T, {{"N", Grouped(N)}});
}

std::string RunAlertStrip(int Back)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k3.alert.strip");
	return KcFill(T, {{"N", Grouped(Back)}});
}

const std::vector<std::string>& RunAlertReturn(growth::RunStage S)
{
	using namespace kastcopy_detail;
	static const std::vector<std::string> Bubble = KcPool("pool.k3.return.bubble");
	static const std::vector<std::string> Itm = KcPool("pool.k3.return.itm");
	static const std::vector<std::string> Ft = KcPool("pool.k3.return.ft");
	static const std::vector<std::string> Top3 = KcPool("pool.k3.return.top3");
	switch (S)
	{
	case growth::RunStage::Bubble: return Bubble;
	case growth::RunStage::Itm: return Itm;
	case growth::RunStage::Top3: return Top3;
	default: return Ft;
	}
}

std::string RailCallButton()
{
	static const std::string T = kastcopy_detail::KcText("k3.rail.button");
	return T;
}

std::string RailCallReached(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k3.rail.reached");
	static const std::string One = KcText("k3.rail.reached.one");
	return N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

const std::vector<std::string>& RailCallArrive()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k3.rail.arrive");
	return P;
}

std::string DeeOmw()
{
	static const std::string T = kastcopy_detail::KcText("k3.rail.dee");
	return T;
}

std::string MeiRail()
{
	static const std::string T = kastcopy_detail::KcText("k3.rail.mei");
	return T;
}

std::string WholeRailHere()
{
	static const std::string T = kastcopy_detail::KcText("k3.rail.whole");
	return T;
}

// ------------------------------------------------------------------ K4

std::string TierName(int Tier)
{
	using namespace kastcopy_detail;
	static const std::string Names[5] = {KcText("k4.tier.0"), KcText("k4.tier.1"), KcText("k4.tier.2"), KcText("k4.tier.3"), KcText("k4.tier.4")};
	return Names[static_cast<size_t>(std::max(0, std::min(4, Tier)))];
}

std::string HeatTitle()
{
	static const std::string T = kastcopy_detail::KcText("k4.title");
	return T;
}

std::string GhostTonight(double Gain)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.ghost");
	return KcFill(T, {{"Gain", KcSigned(Gain)}});
}

std::string GhostSettling()
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.ghost.settling");
	return T;
}

std::string WatchingTier(int Watching, int Tier)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.watching_tier");
	return KcFill(T, {{"N", Grouped(Watching)}, {"Tier", TierName(Tier)}});
}

std::string HeatEffects(int Heat)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.effects");
	return KcFill(T, {{"Heat", std::to_string(Heat)}});
}

std::string ColdMeans()
{
	static const std::string T = kastcopy_detail::KcText("k4.cold_means");
	return T;
}

std::string WhyRun(growth::RunStage S, int Runners, int DaysAgo, const std::string& DayName)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.run");
	static const std::string Tonight = KcText("k4.why.when.tonight");
	static const std::string Last = KcText("k4.why.when.last");
	static const std::string OnDay = KcText("k4.why.when.day");
	static const std::string Stages[6] = {KcText("k4.why.stage.0"), KcText("k4.why.stage.1"), KcText("k4.why.stage.2"), KcText("k4.why.stage.3"),
		KcText("k4.why.stage.4"), KcText("k4.why.stage.5")};
	const std::string When = DaysAgo <= 0 ? Tonight : DaysAgo == 1 ? Last : KcFill(OnDay, {{"Day", DayName}});
	const size_t Stage = static_cast<size_t>(std::max(0, std::min(5, static_cast<int>(S))));
	return KcFill(T, {{"When", When}, {"Stage", Stages[Stage]}, {"Runners", Grouped(Runners)}});
}

std::string WhyOverForecast(int Pct)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.over");
	return KcFill(T, {{"Pct", Grouped(Pct)}});
}

std::string WhyUnderForecast(int Pct)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.under");
	return KcFill(T, {{"Pct", Grouped(Pct)}});
}

std::string WhyShowingUp(int Nights)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k4.why.show");
	static const std::string One = KcText("k4.why.show.one");
	return Nights == 1 ? One : KcFill(Many, {{"N", Grouped(Nights)}});
}

std::string WhyCooling(int DaysDark)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k4.why.cooling");
	static const std::string One = KcText("k4.why.cooling.one");
	return DaysDark == 1 ? One : KcFill(Many, {{"N", Grouped(DaysDark)}});
}

std::string WhyClipTop()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.clip");
	return T;
}

std::string WhyVideo(const std::string& Title, bool Breakout)
{
	using namespace kastcopy_detail;
	static const std::string Pushed = KcText("k4.why.video.pushed");
	static const std::string Broke = KcText("k4.why.video.breakout");
	return KcFill(Breakout ? Broke : Pushed, {{"Title", KcShort(Title, 16, 10)}});
}

std::string WhyHotWeek()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.hotweek");
	return T;
}

std::string WhyRecordPeak(int Peak)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.record");
	return KcFill(T, {{"Peak", Grouped(Peak)}});
}

std::string WhyRaid(const std::string& From, int Size)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.raid");
	return KcFill(T, {{"Who", From}, {"Size", Grouped(Size)}});
}

std::string WhyMilestone(int Followers)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.milestone");
	return KcFill(T, {{"N", Grouped(Followers)}});
}

std::string WhySteady()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.steady");
	return T;
}

std::string WhyNew()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.new");
	return T;
}

std::string WhyRunPlain(growth::RunStage S, int DaysAgo, const std::string& DayName)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.why.run.plain");
	static const std::string Tonight = KcText("k4.why.when.tonight");
	static const std::string Last = KcText("k4.why.when.last");
	static const std::string OnDay = KcText("k4.why.when.day");
	static const std::string Stages[6] = {KcText("k4.why.stage.0"), KcText("k4.why.stage.1"), KcText("k4.why.stage.2"), KcText("k4.why.stage.3"),
		KcText("k4.why.stage.4"), KcText("k4.why.stage.5")};
	const std::string When = DaysAgo <= 0 ? Tonight : DaysAgo == 1 ? Last : KcFill(OnDay, {{"Day", DayName}});
	const size_t Stage = static_cast<size_t>(std::max(0, std::min(5, static_cast<int>(S))));
	return KcFill(T, {{"When", When}, {"Stage", Stages[Stage]}});
}

std::string WhyShort()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.short");
	return T;
}

std::string WhyShortCounted()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.short.counted");
	return T;
}

std::string WhyShortUsual()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.short.usual");
	return T;
}

std::string WhyShortBefore()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.short.before");
	return T;
}

std::string WhyCeiling()
{
	static const std::string T = kastcopy_detail::KcText("k4.why.ceiling");
	return T;
}

std::array<std::string, 2> WhyOffline(const Channel& Ch, int LastStreamDay, double World)
{
	// A world week is net::DayOf / 7 (day 0 a Monday), as GainWeek and ShowWeek count them; ShowDays holds the week's weekday bits.
	const int Today = net::DayOf(World);
	const int Week = static_cast<int>(std::floor(static_cast<double>(Today) / 7.0));
	int Nights = 0;
	for (int M = Ch.ShowDays; M != 0; M &= M - 1)
	{
		++Nights;
	}
	std::vector<std::string> Lines;
	// A final table on stream in the last 6 world days first, as the live lines put a deep run first (C-K PIE F4: a final table
	// night never reads "New channel"). FtDays holds the days (bit k is FtBase - k), not the field's size: no runners here.
	if (Ch.FtBase >= 0 && Ch.FtDays != 0)
	{
		const uint32_t Bits = static_cast<uint32_t>(Ch.FtDays);
		int K = 0;
		while (K < 31 && ((Bits >> K) & 1u) == 0u)
		{
			++K;
		}
		const int FtDay = Ch.FtBase - K;
		const int Ago = Today - FtDay;
		if (Ago >= 0 && Ago <= 6)
		{
			Lines.push_back(WhyRunPlain(growth::RunStage::FinalTable, Ago, net::WeekdayName(FtDay, true)));
		}
	}
	if (LastStreamDay >= 0 && Today - LastStreamDay >= 2)
	{
		Lines.push_back(WhyCooling(Today - LastStreamDay));
	}
	if (Ch.GainWeek == Week && Ch.GainUsed >= 35.0 - 1e-9)
	{
		Lines.push_back(WhyHotWeek());
	}
	if (Ch.ShowWeek == Week && Nights > 0)
	{
		Lines.push_back(WhyShowingUp(Nights));
	}
	if (Lines.empty() && Ch.Streams <= 3)
	{
		Lines.push_back(WhyNew());
	}
	std::array<std::string, 2> Why;
	for (size_t I = 0; I < Lines.size() && I < Why.size(); ++I)
	{
		Why[I] = Lines[I];
	}
	return Why;
}

const std::vector<std::string>& TierUpChat()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k4.tierup");
	return P;
}

std::string FrontRow()
{
	static const std::string T = kastcopy_detail::KcText("k4.front");
	return T;
}

std::string StaminaLine(int Hours)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k4.stamina");
	return KcFill(T, {{"Hours", Grouped(Hours)}});
}

std::string StaminaTitle()
{
	static const std::string T = kastcopy_detail::KcText("k4.stamina.title");
	return T;
}

std::string StaminaNote()
{
	static const std::string T = kastcopy_detail::KcText("k4.stamina.note");
	return T;
}

// ------------------------------------------------------------------ K5

std::string EventBar(bool FinalTable, const std::string& Event, int Left, int Of)
{
	using namespace kastcopy_detail;
	static const std::string Ft = KcText("k5.bar.ft");
	static const std::string Two = KcText("k5.bar.two");
	return KcFill(FinalTable ? Ft : Two, {{"Event", Event}, {"Left", Grouped(Left)}, {"Of", Grouped(Of)}});
}

std::string FinalTableChip()
{
	static const std::string T = kastcopy_detail::KcText("k5.chip");
	return T;
}

std::string DirectoryEvent(const std::string& Event, int Left)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k5.directory");
	return KcFill(T, {{"Event", Event}, {"Left", Grouped(Left)}});
}

const std::vector<std::string>& SweatChat()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k5.sweat");
	return P;
}

std::string SweatingNowShelf()
{
	static const std::string T = kastcopy_detail::KcText("k5.shelf");
	return T;
}

std::string SweatingNowSub()
{
	static const std::string T = kastcopy_detail::KcText("k5.shelf.sub");
	return T;
}

// ------------------------------------------------------------------ K6

std::string Broadcast(int Site, int Viewers)
{
	using namespace kastcopy_detail;
	return KcFill(KcText((std::string("k6.broadcast.") + KcSiteKey(Site)).c_str()), {{"N", Grouped(Viewers)}});
}

std::string BroadcastTease(int Site)
{
	using namespace kastcopy_detail;
	return KcText((std::string("k6.tease.") + KcSiteKey(Site)).c_str());
}

const std::vector<std::string>& BroadcastChat(int Site)
{
	using namespace kastcopy_detail;
	static const std::vector<std::string> FromRiverLine = KcPool("pool.k6.chat.riverline");
	static const std::vector<std::string> FromIgnite = KcPool("pool.k6.chat.ignite");
	static const std::vector<std::string> FromRunItTwice = KcPool("pool.k6.chat.runittwice");
	static const std::vector<std::string> FromElsewhere = KcPool("pool.k6.chat.other");
	switch (static_cast<net::SiteId>(Site))
	{
	case net::SiteId::RiverLine: return FromRiverLine;
	case net::SiteId::Ignite: return FromIgnite;
	case net::SiteId::RunItTwice: return FromRunItTwice;
	default: return FromElsewhere;
	}
}

std::string RaidedWith(const std::string& From, int Size)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k6.raided");
	return KcFill(T, {{"Who", From}, {"Size", Grouped(Size)}});
}

// ------------------------------------------------------------------ K7

std::string NewOnKastChip()
{
	static const std::string T = kastcopy_detail::KcText("k7.chip.new");
	return T;
}

std::string RisingChip()
{
	static const std::string T = kastcopy_detail::KcText("k7.chip.rising");
	return T;
}

std::string FreshShelf()
{
	static const std::string T = kastcopy_detail::KcText("k7.shelf.fresh");
	return T;
}

std::string FreshShelfSub()
{
	static const std::string T = kastcopy_detail::KcText("k7.shelf.fresh.sub");
	return T;
}

std::string RisingShelf()
{
	static const std::string T = kastcopy_detail::KcText("k7.shelf.rising");
	return T;
}

std::string RisingShelfSub()
{
	static const std::string T = kastcopy_detail::KcText("k7.shelf.rising.sub");
	return T;
}

std::string RankHeader(int Rank, int Of)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k7.rank");
	return KcFill(T, {{"Rank", Grouped(Rank)}, {"Of", Grouped(Of)}});
}

std::string RankNow(int Rank)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k7.rank.now");
	return KcFill(T, {{"Rank", Grouped(Rank)}});
}

std::string RankClimb(int From, int Best)
{
	using namespace kastcopy_detail;
	static const std::string Climb = KcText("k7.rank.climb");
	static const std::string Held = KcText("k7.rank.best");
	if (From > Best && Best > 0)
	{
		return KcFill(Climb, {{"From", Grouped(From)}, {"To", Grouped(Best)}});
	}
	return KcFill(Held, {{"Rank", Grouped(Best)}});
}

// ------------------------------------------------------------------ K8

std::string SourceTag(View::Source S)
{
	using namespace kastcopy_detail;
	const int I = static_cast<int>(S);
	if (I < 0 || I >= static_cast<int>(View::Source::Count))
	{
		return std::string();
	}
	return KcText(("k8.tag." + std::to_string(I)).c_str());
}

std::string SourceCause(View::Source S)
{
	using namespace kastcopy_detail;
	const int I = static_cast<int>(S);
	if (I < 0 || I >= static_cast<int>(View::Source::Count))
	{
		return std::string();
	}
	return KcText(("k8.cause." + std::to_string(I)).c_str());
}

std::string SourceCount(View::Source S, int N)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k8.count");
	return KcFill(T, {{"N", Grouped(N)}, {"Tag", SourceTag(S)}});
}

std::string CameBecauseOf()
{
	static const std::string T = kastcopy_detail::KcText("k8.came");
	return T;
}

std::string FollowedFrom()
{
	static const std::string T = kastcopy_detail::KcText("k8.followed");
	return T;
}

std::string CameFromVideo(const std::string& Title)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k8.from_video");
	return KcFill(T, {{"Title", KcShort(Title, 16, 10)}});
}

std::string CameFromClip()
{
	static const std::string T = kastcopy_detail::KcText("k8.from_clip");
	return T;
}

// ------------------------------------------------------------------ K9

std::string ForecastBand(double Lo, double Hi)
{
	using namespace kastcopy_detail;
	static const std::string Range = KcText("k9.band");
	static const std::string Point = KcText("k9.band.one");
	const long long L = std::max(0ll, std::llround(Lo));
	const long long H = std::max(L, std::llround(Hi));
	return L == H ? KcFill(Point, {{"Lo", Grouped(L)}}) : KcFill(Range, {{"Lo", Grouped(L)}, {"Hi", Grouped(H)}});
}

std::string ForecastContext(const std::string& DayName, int Schedule, int Tier)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k9.context");
	static const std::string Sched[3] = {KcText("k9.sched.0"), KcText("k9.sched.1"), KcText("k9.sched.2")};
	return KcFill(T, {{"Day", DayName}, {"Schedule", Sched[static_cast<size_t>(std::max(0, std::min(2, Schedule)))]}, {"Tier", TierName(Tier)}});
}

std::string ForecastShort(double Lo, double Hi)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k9.short");
	const long long L = std::max(0ll, std::llround(Lo));
	const long long H = std::max(L, std::llround(Hi));
	return KcFill(T, {{"Lo", Grouped(L)}, {"Hi", Grouped(H)}});
}

std::string ForecastVerdict(double Avg, const growth::Band& Fc)
{
	using namespace kastcopy_detail;
	static const std::string Above = KcText("k9.verdict.above");
	static const std::string Inside = KcText("k9.verdict.inside");
	static const std::string Under = KcText("k9.verdict.under");
	return Avg > Fc.Hi ? Above : Avg < Fc.Lo ? Under : Inside;
}

// ------------------------------------------------------------------ K10

std::string WelcomeTheRaidButton()
{
	static const std::string T = kastcopy_detail::KcText("k10.welcome.button");
	return T;
}

std::string WelcomeRaid(const std::string& From, int Size)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k10.welcome_raid");
	return KcFill(T, {{"Who", From}, {"Size", Grouped(Size)}});
}

const std::vector<std::string>& WelcomedChat()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k10.welcomed");
	return P;
}

std::string WelcomeRaidStreamer()
{
	static const std::string T = kastcopy_detail::KcText("k10.welcome.streamer");
	return T;
}

std::string RaidsOwed(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k10.owed");
	static const std::string One = KcText("k10.owed.one");
	static const std::string None = KcText("k10.owed.none");
	return N <= 0 ? None : N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

std::string RaidsOwedShort(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k10.owed.short");
	static const std::string One = KcText("k10.owed.short.one");
	static const std::string None = KcText("k10.owed.short.none");
	return N <= 0 ? None : N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

std::string RaidedAgo(const std::string& Who, int DaysAgo, const std::string& NextDay)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k10.raided_ago");
	static const std::string Today = KcText("k10.raided_ago.today");
	static const std::string Yesterday = KcText("k10.raided_ago.yesterday");
	const std::string& T = DaysAgo <= 0 ? Today : DaysAgo == 1 ? Yesterday : Many;
	return KcFill(T, {{"Who", Who}, {"N", Grouped(DaysAgo)}, {"Day", NextDay}});
}

std::string TokenRule()
{
	static const std::string T = kastcopy_detail::KcText("k10.token_rule");
	return T;
}

std::string RaidRetention(const std::string& From, int Size, int At15, int Followed)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k10.retention");
	return KcFill(T, {{"Who", From}, {"Size", Grouped(Size)}, {"At15", Grouped(At15)}, {"Followed", Grouped(Followed)}});
}

std::string BroadcastRetention(const std::string& From, int Size, int At15, int Followed)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k10.retention.broadcast");
	return KcFill(T, {{"Who", From}, {"Size", Grouped(Size)}, {"At15", Grouped(At15)}, {"Followed", Grouped(Followed)}});
}

// ------------------------------------------------------------------ K11

std::string NodButton()
{
	static const std::string T = kastcopy_detail::KcText("k11.nod");
	return T;
}

std::string NodsLeft(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k11.nods_left");
	static const std::string One = KcText("k11.nods_left.one");
	static const std::string None = KcText("k11.nods_left.none");
	return N <= 0 ? None : N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

const std::vector<std::string>& NodReply()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k11.nod_reply");
	return P;
}

std::string ThankHeard()
{
	static const std::string T = kastcopy_detail::KcText("k11.thank_heard");
	return T;
}

const std::vector<std::string>& AnswerFollowed()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k11.answer_followed");
	return P;
}

// ------------------------------------------------------------------ K12

std::string HoldOrCrackTitle()
{
	static const std::string T = kastcopy_detail::KcText("k12.title");
	return T;
}

std::string HoldOrCrackCounts(int Holds, int Cracks)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k12.counts");
	return KcFill(T, {{"Holds", Grouped(Holds)}, {"Cracks", Grouped(Cracks)}});
}

std::string HoldOrCrackShares(int Holds, int Cracks)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k12.shares");
	const int Total = std::max(0, Holds) + std::max(0, Cracks);
	if (Total <= 0)
	{
		return HoldOrCrackCounts(0, 0);
	}
	const int HoldPct = KcPct(std::max(0, Holds), Total);
	return KcFill(T, {{"Holds", std::to_string(HoldPct)}, {"Cracks", std::to_string(100 - HoldPct)}});
}

std::string YourEquity(double Equity)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k12.equity");
	if (Equity < 0.0)
	{
		return std::string(); // unknown: the marker isn't drawn
	}
	return KcFill(T, {{"Pct", std::to_string(KcPct(std::min(1.0, Equity), 1.0))}});
}

std::string CallIt(bool Holds)
{
	using namespace kastcopy_detail;
	static const std::string H = KcText("k12.callit.holds");
	static const std::string C = KcText("k12.callit.cracks");
	return Holds ? H : C;
}

std::string StreamerCall(bool Holds)
{
	using namespace kastcopy_detail;
	static const std::string H = KcText("k12.streamer.holds");
	static const std::string C = KcText("k12.streamer.cracks");
	return Holds ? H : C;
}

const std::vector<std::string>& ChatCalls(bool Holds)
{
	using namespace kastcopy_detail;
	static const std::vector<std::string> H = KcPool("pool.k12.chat.holds");
	static const std::vector<std::string> C = KcPool("pool.k12.chat.cracks");
	return Holds ? H : C;
}

std::string RevealRight(int Pct)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k12.reveal.right");
	return KcFill(T, {{"Pct", std::to_string(Pct)}});
}

const std::vector<std::string>& RevealWrong()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k12.reveal.wrong");
	return P;
}

std::string RevealLine(bool Held, int Right, int Of, int Points)
{
	using namespace kastcopy_detail;
	static const std::string Share = KcText("k12.reveal.line");
	static const std::string Few = KcText("k12.reveal.line.few");
	static const std::string None = KcText("k12.reveal.line.none");
	static const std::string HoldsWord = KcText("k12.reveal.holds");
	static const std::string CracksWord = KcText("k12.reveal.cracks");
	const std::string Call = Held ? HoldsWord : CracksWord;
	if (Right <= 0 || Of <= 0)
	{
		return KcFill(None, {{"Call", Call}});
	}
	if (Of >= 10)
	{
		return KcFill(Share, {{"Call", Call}, {"Pct", std::to_string(KcPct(Right, Of))}, {"Points", Grouped(Points)}});
	}
	return KcFill(Few, {{"Call", Call}, {"Right", Grouped(Right)}, {"Of", Grouped(Of)}, {"Points", Grouped(Points)}});
}

const std::vector<std::string>& CalledItChat()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k12.called");
	return P;
}

const std::vector<std::string>& MissedCallChat()
{
	static const std::vector<std::string> P = kastcopy_detail::KcPool("pool.k12.missed");
	return P;
}

std::string BestCaller(const std::string& Who, int Calls, int Of)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k12.best_caller");
	return KcFill(T, {{"Who", Who}, {"Calls", Grouped(Calls)}, {"Of", Grouped(Of)}});
}

// ------------------------------------------------------------------ K15

std::string ClipOfTheNight(const std::string& Title, bool InFeed)
{
	using namespace kastcopy_detail;
	static const std::string Feed = KcText("k15.clip.feed");
	static const std::string Unlisted = KcText("k15.clip.channel");
	return KcFill(InFeed ? Feed : Unlisted, {{"Title", Title}});
}

std::string ClipItButton()
{
	static const std::string T = kastcopy_detail::KcText("k15.clip_it");
	return T;
}

std::string MakeItACutButton()
{
	static const std::string T = kastcopy_detail::KcText("k15.make_cut");
	return T;
}

std::string ClipTopOfFeed()
{
	static const std::string T = kastcopy_detail::KcText("k15.top");
	return T;
}

// ------------------------------------------------------------------ K16

std::string WelcomeBack(int N)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k16.welcome_back");
	static const std::string One = KcText("k16.welcome_back.one");
	return N == 1 ? One : KcFill(Many, {{"N", Grouped(N)}});
}

std::string StreakRow(int Weeks)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k16.streak");
	static const std::string One = KcText("k16.streak.one");
	return Weeks == 1 ? One : KcFill(Many, {{"N", Grouped(Weeks)}});
}

// ------------------------------------------------------------------ K17

std::string PeakLine(int Peak, int MomentAtPeak, const std::string& Clock)
{
	using namespace kastcopy_detail;
	static const std::string AtMoment = KcText("k17.peak");
	static const std::string Plain = KcText("k17.peak.plain");
	static const std::string OnRaid = KcText("k17.at.raid");
	if (MomentAtPeak == PeakRaid)
	{
		return KcFill(AtMoment, {{"Peak", Grouped(Peak)}, {"At", OnRaid}, {"Time", Clock}});
	}
	if (MomentAtPeak < 0 || MomentAtPeak >= static_cast<int>(Moment::Count))
	{
		return KcFill(Plain, {{"Peak", Grouped(Peak)}, {"Time", Clock}});
	}
	return KcFill(AtMoment, {{"Peak", Grouped(Peak)}, {"At", KcText(("k17.at." + std::to_string(MomentAtPeak)).c_str())}, {"Time", Clock}});
}

std::string NewFollowersLine(int N, bool Best)
{
	using namespace kastcopy_detail;
	static const std::string Plain = KcText("k17.followers");
	static const std::string Record = KcText("k17.followers.best");
	return KcFill(Best ? Record : Plain, {{"N", Grouped(N)}});
}

std::string RollCall(const std::vector<std::string>& Names)
{
	using namespace kastcopy_detail;
	static const std::string One = KcText("k17.rollcall.one");
	static const std::string Two = KcText("k17.rollcall.two");
	static const std::string More = KcText("k17.rollcall.more");
	if (Names.empty())
	{
		return std::string();
	}
	if (Names.size() == 1)
	{
		return KcFill(One, {{"A", Names[0]}});
	}
	if (Names.size() == 2)
	{
		return KcFill(Two, {{"A", Names[0]}, {"B", Names[1]}});
	}
	return KcFill(More, {{"A", Names[0]}, {"B", Names[1]}, {"More", Grouped(static_cast<int64_t>(Names.size()) - 2)}});
}

std::string HeatMove(int FromTier, int ToTier, double Gain)
{
	using namespace kastcopy_detail;
	static const std::string Move = KcText("k17.heat.move");
	static const std::string Same = KcText("k17.heat.same");
	if (FromTier == ToTier)
	{
		return KcFill(Same, {{"To", TierName(ToTier)}, {"Gain", KcSigned(Gain)}});
	}
	return KcFill(Move, {{"From", TierName(FromTier)}, {"To", TierName(ToTier)}, {"Gain", KcSigned(Gain)}});
}

std::string TipsTonight(Chips Tips, Chips Cheers, bool Affiliate)
{
	using namespace kastcopy_detail;
	static const std::string Both = KcText("k17.tonight");
	static const std::string TipsOnly = KcText("k17.tonight.tips");
	return Affiliate ? KcFill(Both, {{"Tips", Dollars(Tips)}, {"Cheers", Dollars(Cheers)}}) : KcFill(TipsOnly, {{"Tips", Dollars(Tips)}});
}

std::string RenewalsAway(Chips Cents)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.renewals");
	return KcFill(T, {{"Amount", Dollars(Cents)}});
}

std::string Tipped(const std::string& Who, Chips Cents)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.tipped");
	return KcFill(T, {{"Who", Who}, {"Amount", Dollars(Cents)}});
}

std::string PendingLine(Chips Cents)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.pending");
	return KcFill(T, {{"Amount", Dollars(Cents)}});
}

std::string PendingNote()
{
	static const std::string T = kastcopy_detail::KcText("k17.pending.note");
	return T;
}

std::string NextGoal(int ToGo, int Goal)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.goal");
	return KcFill(T, {{"ToGo", Grouped(ToGo)}, {"Goal", Grouped(Goal)}});
}

std::string FirstPayout(Chips Cents)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.first_payout");
	return KcFill(T, {{"Amount", Dollars(Cents)}});
}

std::string SponsorReadLabel(const std::string& Brand)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.sponsor.read");
	return KcFill(T, {{"Brand", Brand}});
}

std::string SponsorHoursLabel(const std::string& Brand, int Hours)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k17.sponsor.hours");
	static const std::string One = KcText("k17.sponsor.hours.one");
	return KcFill(Hours == 1 ? One : Many, {{"Brand", Brand}, {"N", Grouped(Hours)}});
}

std::string SponsorReadDone(const std::string& Brand)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.sponsor.done");
	return KcFill(T, {{"Brand", Brand}});
}

std::string LedgerRead(const std::string& Brand, int Reads)
{
	using namespace kastcopy_detail;
	static const std::string Many = KcText("k17.sponsor.reads");
	return Reads > 1 ? KcFill(Many, {{"Brand", Brand}, {"N", Grouped(Reads)}}) : SponsorReadLabel(Brand);
}

std::string LedgerHours(const std::string& Brand)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.sponsor.live_hours");
	return KcFill(T, {{"Brand", Brand}});
}

std::string StingFollowers(int Step)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.sting.followers");
	return KcFill(T, {{"N", Grouped(Step)}});
}

std::string StingStrangerTip()
{
	static const std::string T = kastcopy_detail::KcText("k17.sting.stranger_tip");
	return T;
}

std::string StingFirstRaid()
{
	static const std::string T = kastcopy_detail::KcText("k17.sting.first_raid");
	return T;
}

std::string StingAffiliate()
{
	static const std::string T = kastcopy_detail::KcText("k17.sting.affiliate");
	return T;
}

std::string StingPartner()
{
	static const std::string T = kastcopy_detail::KcText("k17.sting.partner");
	return T;
}

std::string DoneButton()
{
	static const std::string T = kastcopy_detail::KcText("k17.done");
	return T;
}

std::string MilestoneFollowers(int N)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.milestone");
	return KcFill(T, {{"N", Grouped(N)}});
}

std::string FirstStrangerTip(const std::string& Who, Chips Cents)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.first_tip");
	return KcFill(T, {{"Who", Who}, {"Amount", Dollars(Cents)}});
}

std::string FirstRaidIn(const std::string& From, int Size)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k17.first_raid");
	return KcFill(T, {{"Who", From}, {"Size", Grouped(Size)}});
}

bool IsFirstStrangerTip(const std::string& Line)
{
	static const std::string T = kastcopy_detail::KcText("k17.first_tip");
	static const std::string Head = T.substr(0, T.find('{'));
	return !Head.empty() && Line.compare(0, Head.size(), Head) == 0;
}

bool IsFirstRaidIn(const std::string& Line)
{
	static const std::string T = kastcopy_detail::KcText("k17.first_raid");
	static const std::string Head = T.substr(0, T.find('{'));
	return !Head.empty() && Line.compare(0, Head.size(), Head) == 0;
}

std::string LetterFrom()
{
	static const std::string T = kastcopy_detail::KcText("k17.letter.from");
	return T;
}

std::string LetterSubject()
{
	static const std::string T = kastcopy_detail::KcText("k17.letter.subject");
	return T;
}

std::string LetterBody()
{
	static const std::string T = kastcopy_detail::KcText("k17.letter.body");
	return T;
}

// ------------------------------------------------------------------ K20

std::string SponsorApart()
{
	static const std::string T = kastcopy_detail::KcText("k20.sponsor_apart");
	return T;
}

std::string PartnerAverage()
{
	static const std::string T = kastcopy_detail::KcText("k20.partner_avg");
	return T;
}

std::string AffiliateBar(int Followers, int Minutes, int Days, int Average)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k20.affiliate_bar");
	return KcFill(T, {{"Followers", Grouped(Followers)}, {"Minutes", Grouped(Minutes)}, {"Days", Grouped(Days)}, {"Average", Grouped(Average)}});
}

std::string AffiliateFriends()
{
	static const std::string T = kastcopy_detail::KcText("k20.affiliate_friends");
	return T;
}

std::string AffiliateToGo(int ToGo, int At)
{
	using namespace kastcopy_detail;
	static const std::string T = KcText("k20.affiliate_to_go");
	return KcFill(T, {{"ToGo", Grouped(ToGo)}, {"Followers", Grouped(At)}});
}
} // namespace copy
} // namespace kast
} // namespace ss
