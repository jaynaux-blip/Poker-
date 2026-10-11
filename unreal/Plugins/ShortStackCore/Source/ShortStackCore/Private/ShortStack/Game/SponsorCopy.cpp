#include "SponsorCopy.h"
#include "../StrictFloat.h"

// The sponsor book's words (next_segment_spec §3, N8). core-sponsor.
//
// Letters are short prose: the contract card in the Mail app is drawn from the contract's terms, so a body never holds
// numbers a counter could change. Bodies end on a sentence (the Mail app signs them with the From line). Every template
// here is linted by sponsor_test (CopyLint.h) with Samples() on top of the lint's own.

namespace ss
{
namespace sponsor
{
namespace sponsor_copy
{
namespace sponsor_copy_detail
{
using Table = std::vector<std::pair<std::string, std::string>>;

const Table& Bodies()
{
	static const Table T = {
		// ---------------------------------------------------------------- offers, by mail (two wordings each)
		{"offer:voltrush:0",
			"{Why} The clerk at the Lucky Penny says you go through our cans like water, so this shouldn't be a stretch. We'd like the Volt Rush cap on your head when "
			"you play live. A check every month, a case at your door every Monday, and a little extra when you make a final table in it. The numbers are on the card "
			"below. Sign, counter or pass. No hard feelings either way."},
		{"offer:voltrush:1",
			"{Why} We keep an eye on who's grinding at three in the morning, and you keep showing up. Volt Rush wants its cap on you at the live tables: a monthly "
			"check, a case every Monday, and a bonus for every final table you make in it. Details are below. Take a look and tell me what you think."},
		{"offer:overclock:0",
			"{Why} Overclock pays streamers to keep a can in frame and say our name once a night. We pay by the live hour and by the read, through Kast, same as "
			"always. Thirty days to start, longer if it works for both of us. The card below has it all."},
		{"offer:overclock:1",
			"{Why} You stream the late shift, and the late shift is our whole audience. Put Overclock on your overlay, read for us once a stream, and we pay for every "
			"hour you're live. Thirty days to see how it goes. Terms are below."},
		{"offer:embercrest:0",
			"{Why} I'd like you on Team Embercrest. That means the crest on your chest in my room, your face at our meet-the-regulars before a Sunday, and playing our "
			"schedule like you already do. In return, some of your entries are on the house, and so is the bus. The terms are below. Come see me at the desk if "
			"you'd rather talk in person."},
		{"offer:embercrest:1",
			"{Why} The room has noticed, and so have I. Team Embercrest is a handful of regulars who wear the crest and make the place feel like home. We cover some "
			"entries and the bus up the ridge. You show up, play, and say hello to the new faces. Terms below."},
		{"offer:halo:0",
			"{Why} Halo builds cameras for people who stream from a desk at two in the morning. We'd like to send you the next one up, put our mark on your overlay, "
			"and pay a little for every hour you're live with it. Sixty days. Details below."},
		{"offer:halo:1",
			"{Why} Your facecam deserves better, and we make better. Join the Halo creator program: a new camera on us, our mark on your overlay, and a small rate for "
			"every live hour. The terms are on the card below."},
		{"offer:tunnelrat:0",
			"{Why} Your chat trusts you, and that matters to a VPN. We pay by the live hour and by the read, through Kast. One read a stream, our mark on the overlay, "
			"thirty days. The card below has the numbers."},
		{"offer:tunnelrat:1",
			"{Why} TunnelRat sponsors a handful of poker channels we actually watch. Yours is one of them now, if you want it. Hourly pay, a read a night, our mark on "
			"your overlay. Thirty days to start."},
		{"offer:stacked:0",
			"{Why} Stacked makes hoodies for twelve-hour sessions, and we want them on somebody who plays twelve-hour sessions. Wear Stacked when you play live, keep "
			"our name on stream, and we pay by the hour and by the read. Thirty days to start. The rest is below."},
		{"offer:stacked:1",
			"{Why} We'd like to dress you. Stacked at your live events, a read on stream now and then, hourly pay through Kast. If the first month works, we go "
			"longer. Terms below."},
		{"offer:rangewise:0",
			"{Why} Rangewise is a training site, and the best advertising we have is a player who explains their spots out loud. Four teaching streams a month, our "
			"patch on your sleeve, a monthly fee and a bonus for every lesson. Six months. It's all below."},
		{"offer:rangewise:1",
			"{Why} You think out loud well, and that's rarer than winning. We'd like four teaching streams a month under the Rangewise name, with a monthly fee, a "
			"bonus per stream and a free membership. The details are below."},
		{"offer:riverline:0",
			"{Why} RiverLine would like you on Team RiverLine. The patch on your chest, our tournaments on your stream, a monthly retainer, tickets to the Daily Main, "
			"and the gold frame on your name. Six months. The full terms are below."},
		{"offer:riverline:1",
			"{Why} Team RiverLine is ten players and a few streamers, and we have a seat open. You'd wear the patch at every live event, stream our tournaments, and get "
			"a retainer, Daily Main tickets and the gold frame. Terms below."},
		{"offer:sitwell:0",
			"{Why} Ten thousand people sit with you every night. Sitwell would like to be the chair. One comes with the deal, along with hourly pay and a read a "
			"stream. Thirty days to start."},
		{"offer:sitwell:1",
			"{Why} Long sessions are our whole business. Put Sitwell on your overlay, read for us once a stream, and the chair under you is on us. Terms below."},
		{"offer:turnstone:0",
			"{Why} Turnstone Poker is new in town, and we want our name on the chest of somebody people already watch. A monthly retainer, a bonus for every final "
			"table on the stage, and the big one covered with travel. One year. It's all below."},
		{"offer:turnstone:1",
			"{Why} We're building a team before we open the doors, and we'd like you on it. Our patch at every live event, appearances when we ask, and a retainer "
			"that makes that easy. One year. Terms below."},
		// ---------------------------------------------------------------- offers made in person, then put in writing
		{"heard:luckypenny",
			"Ray told you the deal at the counter. Here it is in writing, in case you'd rather sign here than on the back of a receipt. Store credit every week, one "
			"Noon Deepstack on us, and the penny on you when you play. Owner says thanks for the business."},
		{"heard:barber",
			"Lonnie asked us to put it in writing. A cut or a line-up every two weeks on the house, our patch on your sleeve at live events, and one Saturday afternoon "
			"in the window chair so the street sees who sits there. Sign whenever you're ready."},
		{"heard:ironhouse",
			"Rosa says you're in here more than some of the staff. IronHouse would like its patch on your sleeve at a live event a month. We'll cover the membership "
			"while you keep putting in the work. Terms are below."},
		// ---------------------------------------------------------------- welcome
		{"welcome:luckypenny",
			"Welcome to the corkboard. Your credit goes on the book at the counter and on Penny Drop every week. Sew the penny on, and come by for the photo when "
			"Ray's on. Ten to ten."},
		{"welcome:barber",
			"Welcome aboard. Your first cut on the house is whenever you want it. Sew the patch on your sleeve, and pick a Saturday afternoon for the window chair."},
		{"welcome:ironhouse",
			"Welcome. Your membership's on us from today. Eight workouts a month and the patch at one live event. Rosa will keep count, believe me."},
		{"welcome:voltrush",
			"Welcome to Volt Rush. Your first case lands Monday. Sew the patch on your cap before your next live event. We pay at the end of each month, for the "
			"month you just did."},
		{"welcome:overclock",
			"Welcome aboard. Our mark is on your overlay from your next stream. One read a stream, hourly pay through Kast. Go make some noise."},
		{"welcome:embercrest",
			"Welcome to Team Embercrest. Your covered entries are waiting at the desk whenever you are. The crest goes on your chest in the room. I'll see you at the "
			"next meet-the-regulars."},
		{"welcome:halo",
			"Welcome to Halo creators. Your camera ships today. It's ours while the term runs and yours to keep if we finish clean. Our mark is on your overlay from "
			"your next stream."},
		{"welcome:tunnelrat",
			"Welcome. You're on the overlay from your next stream. Read once a stream when it feels natural. Chat can smell a forced one."},
		{"welcome:stacked",
			"Welcome to Stacked. Wear us when you play live, and keep a read going on stream. We'll talk about a longer run at the end of the month."},
		{"welcome:rangewise",
			"Welcome. Your Rangewise membership is live. Title your lessons as teaching streams and keep them past two hours, and the bonus follows."},
		{"welcome:riverline",
			"Welcome to Team RiverLine. The patch, the gold frame and your first Daily Main tickets are on your account today. The first-Saturday community stream is "
			"the one we'll ask about."},
		{"welcome:sitwell",
			"Welcome. The chair ships today. Sit in it, stream in it, read for us once a stream. It's ours until the term ends clean, then it's yours."},
		{"welcome:turnstone", "Welcome to Turnstone. Our patch at every live event, and the stage is where we want to see it. We'll be in touch about the Championship."},
		{"welcome", "Welcome aboard. Everything we agreed is on the card below, and the first review is a month from today."},
		// ---------------------------------------------------------------- reviews
		{"review:paid", "Month {Period} is in the books, and you did everything we asked. {Amount} is in your account. Same again this month."},
		{"review:paid:voltrush", "Month {Period} is in the books, and the cap made it to every event we asked for. {Amount} is in your account. Keep the cans in frame."},
		{"review:paid:rangewise", "Month {Period} is in the books. The lessons were good, and people signed up off them. {Amount} is in your account."},
		{"review:paid:riverline", "Month {Period} is in the books. Hours, reads and the patch, all there. {Amount} is in your account. Welcome to month {Next}."},
		{"review:kind", "Month {Period} is in the books, and you held up your end. Same again this month."},
		{"review:kind:luckypenny", "Month {Period} is done. Owner's happy, Ray's happy. Same again this month."},
		{"review:kind:barber", "Month {Period} is done. Lonnie says two guys asked who cuts your hair. Same again this month."},
		{"review:kind:ironhouse", "Month {Period} is done. Rosa counted every workout, and you made the number. Same again this month."},
		{"review:kind:embercrest", "Month {Period} is in the books. You played our schedule and the room saw the crest. Same again this month."},
		// ---------------------------------------------------------------- strikes and cures
		{"strike:money",
			"We went over month {Period} and came up short: {Missed}. That's a strike. Half this month's money is held, and half of next month's extras, until you "
			"make it up. You have until {Cure}. Do that and we're square."},
		{"strike:kind",
			"We went over month {Period} and came up short: {Missed}. That's a strike, and half of next month's extras are on hold until you make it up. You have "
			"until {Cure}. Do that and we're square."},
		{"strike:kind:embercrest",
			"We went over the month and came up short: {Missed}. That's a strike, and half of next month's covered entries are on hold. Make it up by {Cure} and we're "
			"square. The next meet-the-regulars is Sunday, before cards."},
		{"strike:kind:luckypenny", "Owner went over the month with Ray. We came up short: {Missed}. Half the credit's on hold until you make it up. You have until {Cure}."},
		{"cured:money", "You made it up. The {Amount} we held is in your account. Clean slate."},
		{"cured:kind", "You made it up. We're square, and next month's extras are back to normal."},
		// ---------------------------------------------------------------- endings
		{"ended:cause", "Two strikes in one term. We're ending it here, effective today. No hard feelings. It just didn't fit."},
		{"ended:cure", "The two weeks are up and the shortfall is still there. We're ending it here, effective today."},
		{"ended:conduct", "We saw the paper this morning. Our name can't sit next to that, so the deal is over, effective today."},
		{"ended:conduct:embercrest", "I saw the paper. Team Embercrest is over, effective today."},
		{"ended:conduct:luckypenny", "Owner saw the paper again. He can't keep the picture up this time. The deal's over."},
		{"ended:restricted", "RiverLine restricted your account. We can't carry a player the site won't, so the deal is over, effective today."},
		{"pass:luckypenny", "Owner saw the paper. He's letting it slide. Once."},
		{"clawback", "We're taking back {Amount} of the signing money."},
		{"writeoff", "Consider the rest a lesson."},
		{"returned", "We'll take back what we sent you."},
		{"patch:luckypenny", "Cut the penny off the jacket."},
		{"patch:barber", "Take our patch off your sleeve when you get a minute."},
		{"patch:embercrest", "The crest comes off your chest."},
		{"patch", "Take our patch off when you get a minute."},
		{"notice:ack", "Understood. The deal runs to the end of this month, {Until}, and we'll pay the month if its asks are met. No hard feelings."},
		{"notice:end", "That was the last month. Thanks for the run, and good luck out there."},
		{"term:end", "That's the full term, start to finish. Thank you. Anything we sent you is yours to keep."},
		{"term:end:legacy", "That's the end of the old deal. Thanks for carrying us on the stream."},
		{"term:notrenewed", "The term's up. We're going in a different direction, and that's on our numbers, not on you. Anything we sent you is yours to keep."},
		{"term:short", "The term's up, and the last month came up short: {Missed}. We won't be renewing."},
		// ---------------------------------------------------------------- renewals, counters, raises
		{"renewal", "The term ends {Until}, and we'd like to keep going. New numbers are on the card below. They're built on what you did, not what you promised."},
		{"renewal:match", "The term ends {Until}, and we'd like to keep going. We saw what {Rival} put in front of you, so we matched it. The card below has the numbers."},
		{"renewal:legacy", "The old deal ends {Until}. If you want to keep going, here's what a new one looks like. The terms are below."},
		{"reply:full", "Can do. The updated terms are on the card below."},
		{"reply:money", "Can do {Amount} a month. That's the ceiling on my end."},
		{"reply:half", "We can meet you halfway. The updated terms are on the card below."},
		{"reply:pass", "We'll pass for now. Maybe down the road."},
		{"reply:raise", "Fair ask. You've grown since we signed. The new numbers start this month."},
		{"migrated", "Picking this back up. The terms have changed since we last wrote; they're below."},
		{"migrated:active", "Same deal as before, now on the books. Hourly and reads through Kast, nothing else asked, and it runs to {Until}."},
	};
	return T;
}

const Table& Subjects()
{
	static const Table T = {
		{"offer:voltrush", "Volt Rush wants your cap."},
		{"offer:overclock", "Overclock on your overlay?"},
		{"offer:embercrest", "Team Embercrest wants you."},
		{"offer:halo", "A camera, on us."},
		{"offer:tunnelrat", "TunnelRat would like a word."},
		{"offer:stacked", "Stacked wants to dress you."},
		{"offer:rangewise", "Teach on Rangewise?"},
		{"offer:riverline", "An invitation to Team RiverLine."},
		{"offer:sitwell", "The chair under you."},
		{"offer:turnstone", "Turnstone Poker would like a word."},
		{"heard:luckypenny", "Penny's Pick, in writing."},
		{"heard:barber", "Fifth St. Barbers, in writing."},
		{"heard:ironhouse", "IronHouse, in writing."},
		{"welcome", "{Short}: welcome aboard."},
		{"welcome:embercrest", "Welcome to Team Embercrest."},
		{"review", "{Short}: month {Period} is in."},
		{"strike", "{Short}: we came up short."},
		{"cured", "{Short}: we're square."},
		{"ended", "{Short}: we're done."},
		{"ended:conduct", "{Short}: about the paper."},
		{"ended:restricted", "{Short}: about your account."},
		{"pass", "{Short}: about the paper."},
		{"notice:ack", "{Short}: notice received."},
		{"notice:end", "{Short}: that's a wrap."},
		{"term:end", "{Short}: the term's up."},
		{"term:notrenewed", "{Short}: thanks for the run."},
		{"term:short", "{Short}: the term's up."},
		{"renewal", "{Short}: another term?"},
		{"reply", "{Short}: our answer."},
		{"migrated", "{Short}: picking this back up."},
		{"migrated:active", "{Short}: on the books."},
	};
	return T;
}

const Table& Spoken()
{
	static const Table T = {
		// Why a brand is calling now (the first sentence of an offer).
		{"why:title", "You won the {Event}, and people noticed."},
		{"why:final", "You made a final table at the Embercrest."},
		{"why:room", "You've played {Count} Embercrest events in two months and cashed {Cashes} of them."},
		{"why:cashes", "{Cashes} cashes at the Embercrest in three months gets noticed."},
		{"why:partner", "Kast made you a Partner."},
		{"why:channel", "Your channel's up to {Followers} followers, and they stay."},
		{"why:cans", "You've bought {Count} of our cans at the Lucky Penny."},
		{"why:tier", "People in the room know your name now."},
		{"why:webcam", "Your stream's getting bigger and your camera isn't."},
		{"why:migrated", "We wrote you through Kast a while back."},
		// Texts from the brands (the phone).
		{"product:voltrush", "Case is at your door. Twelve cans. Keep them in frame."},
		{"product:voltrush:dee", "Case left with Dee at her door. Twelve cans. Keep them in frame."},
	};
	return T;
}
} // namespace sponsor_copy_detail

const std::vector<std::pair<std::string, std::string>>& All(int Kind)
{
	return Kind == Body ? sponsor_copy_detail::Bodies() : Kind == Subject ? sponsor_copy_detail::Subjects() : sponsor_copy_detail::Spoken();
}

std::string Text(int Kind, const std::string& Key)
{
	const std::vector<std::pair<std::string, std::string>>& T = All(Kind);
	std::string Want = Key;
	while (!Want.empty())
	{
		for (const std::pair<std::string, std::string>& Kv : T)
		{
			if (Kv.first == Want)
			{
				return Kv.second;
			}
		}
		const size_t Colon = Want.rfind(':');
		if (Colon == std::string::npos)
		{
			break;
		}
		Want.resize(Colon);
	}
	return std::string();
}

std::string Fill(const std::string& Template, const std::map<std::string, std::string>& Vars)
{
	std::string Out;
	size_t I = 0;
	while (I < Template.size())
	{
		const size_t Open = Template.find('{', I);
		if (Open == std::string::npos)
		{
			Out.append(Template, I, std::string::npos);
			break;
		}
		const size_t Close = Template.find('}', Open + 1);
		if (Close == std::string::npos)
		{
			Out.append(Template, I, std::string::npos);
			break;
		}
		Out.append(Template, I, Open - I);
		const auto It = Vars.find(Template.substr(Open + 1, Close - Open - 1));
		Out += It != Vars.end() ? It->second : Template.substr(Open, Close - Open + 1);
		I = Close + 1;
	}
	return Out;
}

std::map<std::string, std::string> Samples()
{
	return {
		{"Brand", "Fifth St. Barbers"},
		{"Short", "Team Embercrest"},
		{"Rep", "RiverLine Player Relations"},
		{"Why", "You've played 9 Embercrest events in two months and cashed 3 of them."},
		{"Amount", "$1,500"},
		{"Until", "Wednesday, Dec 30"},
		{"Cure", "Wednesday, Dec 30"},
		{"Missed", "6 of 8 Embercrest events, and the meet-the-regulars"},
		{"Period", "11"},
		{"Next", "12"},
		{"Rival", "Team RiverLine"},
		{"Item", "Mirrorless camera + capture card"},
		{"Event", "Embercrest Sunday $150"},
		{"Count", "12"},
		{"Cashes", "5"},
		{"Followers", "25,000"},
	};
}
} // namespace sponsor_copy
} // namespace sponsor
} // namespace ss
