#include "ShortStack/Game/Talk.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

// What people on the block say (the realism update's spec, §6.4; Talk.h has the rules). people-core.
//
// The lines are tables by speaker and topic ("lonnie|look:hungry"), falling back to their role ("@waitress|...") and then
// to anyone ("*|..."); a topic with several lines picks one by the speaker, the topic and the quarter hour, so a person
// says the same thing for a few minutes and something else later. Placeholders in braces are filled from the context:
// {first} {screen} {weekday} {Next} {closes} {queue} {who} {what} {money} {place} {event} {when} {names} {time} {bus}
// {leader} {due} {Sell} {Loan} {meirank} {herorank} {lot114}.

namespace ss
{
namespace town
{
/** Everything a talk needs to know, read once from the session (Talk.h: rebuild it after Heard). */
struct TalkContext
{
	struct Item
	{
		std::string Key; // a one-time key ("" when it can be said again)
		std::string Topic;
		std::string Line;
	};

	std::string Speaker;
	std::string Venue;
	std::string Name;
	std::string Label;
	const Person* Who = nullptr;
	Job Role = Job::Pedestrian;
	bool InWorld = false;
	int Npc = -1;
	// The clock and the sky.
	double World = 0.0;
	int Day = 0;
	int Weekday = 0;
	double Hour = 0.0;
	weather::Sky Sky = weather::Sky::Clear;
	bool Raining = false;
	bool Soaked = false;
	// The player.
	std::string Screen;
	std::string First;
	hero::Background Story = hero::Background::Newcomer;
	bool Created = false;
	double Hunger = 0.0;
	double Thirst = 0.0;
	double Energy = 100.0;
	double Fitness = 0.0;
	bool Evicted = false;
	int EvictedDay = -1;
	bool FreshCut = false;
	int CutDay = -1;
	bool NewLook = false;
	bool Famous = false;
	bool Member = false;
	// The bond.
	Bond Town;
	bool Met = false;
	int DaysAway = 0;
	bool HasMemory = false;
	std::string Remembered; // the world's own greeting from their memories of the player (World::SpokenGreeting)
	// The venue.
	bool Open = true;
	bool Staffed = true;
	bool AtWork = false; // talking to a keeper at their own shop while it's staffed
	int Queue = 0;
	int Tickets = 0;
	// Twitch's twenty.
	bool Owes = false;
	bool Repays = false;
	bool CanAsk = false;
	bool PaysCheck = false;
	// Big Lou's standing offer to put the player in an event (core-stake, next_segment_spec §2.5), said over the check at
	// Ruby's: "" for none.
	std::string LouStake;
	// What they could tell.
	std::vector<Item> News;
	std::vector<Item> Games;
	// Sponsors (next_segment_spec §3): the offer this keeper makes for their shop, its deal, an appearance window open
	// with them now, the patches the player wears where they show, and a deal signed lately (the word on the block).
	std::string SponsorBrand;  // the brand this person makes offers for ("luckypenny" for Ray), "" for none
	std::string SponsorOffer;  // its open offer (a contract id) while they're at work, "" for none
	bool SponsorSignable = false;
	bool SponsorActive = false;
	std::string SponsorAppear; // the venue of an appearance window open with them now ("store"), "" for none
	std::vector<std::string> Patches;
	std::string NewSponsor;    // a deal signed in the last three weeks: the brand's short name
	std::string NewSponsorKey;
	std::set<std::string> Told;
	std::map<std::string, std::string> Vars;
	uint32_t Salt = 0;
};

namespace talk_detail
{
using Table = std::map<std::string, std::vector<std::string>>;

/** speaker|topic -> what they might say. */
const Table& Lines()
{
	static const Table T = {
		// ---------------------------------------------------------------- anyone
		{"*|greet:morning", {"Morning.", "Morning. Early one, huh?"}},
		{"*|greet:day", {"Afternoon.", "Hey there."}},
		{"*|greet:evening", {"Evening.", "Hey. Nice night for it."}},
		{"*|greet:late", {"Late one, huh?", "You're up late."}},
		{"*|weather:rain", {"Coming down out there.", "Some weather, huh?"}},
		{"*|weather:storm", {"Hear that thunder? Stay in a while.", "Wild out there tonight."}},
		{"*|look:hungry", {"You look like you could eat. Ruby's is open. Ruby's is always open."}},
		{"*|look:tired", {"You look beat. Go home and sleep.", "When did you last sleep? Honestly."}},
		{"*|look:thirsty", {"You look parched. Get some water in you."}},
		{"*|look:soaked", {"You're soaked through. Stand by the heater a minute."}},
		{"*|look:cut", {"Fresh cut? Lonnie's work. I'd know those corners anywhere."}},
		{"*|look:new", {"New jacket? Looks good on you."}},
		{"*|look:famous", {"Hey, I've seen you. On the stream, right? My cousin watches you."}},
		{"*|bond:evicted", {"Heard about your place. Sorry. Dee's good people, you'll be all right."}},
		{"*|bond:away", {"Look who it is. Haven't seen you around in a while."}},
		{"*|bond:regular", {"Hey, there's a familiar face."}},
		{"*|bond:tipper", {"Good to see you. Really."}},
		{"*|bye", {"See you.", "Take it easy.", "Go on, then."}},
		{"*|news:none", {"Nothing new. Same street, same people.", "Quiet. Quiet's good."}},
		{"*|games:own", {"Dee's game, Tuesdays, Thursdays, Saturdays. Back of the Wash & Fold. You didn't hear it from me."}},
		{"*|games:embercrest", {"The {event} is at {time} up at the Embercrest. The 14 at {bus} gets you there."}},
		{"*|rumour:dees", {"Dee's got a game {when}. {names}.", "Dee's game is {when}. Doors at nine. {names}."}},
		{"*|rumour:dees-now", {"Dee's got a game going right now, back of the Wash & Fold. {names}."}},
		{"*|rumour:winner", {"{who} won the {event} {when}. {money}.", "You hear? {who} took the {event} {when}. {money}."}},
		{"*|rumour:winner-you", {"Hey, you won the {event}! Everybody's talking about it."}},
		{"*|rumour:nightly", {"{names} in the Nightly tonight. Bus at {bus}."}},
		{"*|rumour:nightshift", {"{leader}'s on top of the Night Shift board."}},
		{"*|rumour:nightshift-you", {"You're on top of the Night Shift board. Don't let it go to your head."}},
		{"*|result:win", {"You won the {event}? Get out of here. Congratulations."}},
		{"*|result:cash", {"Saw you cashed the {event}. {place}. Not bad at all."}},
		{"*|result:bust", {"Tough one at the {event}. Shake it off."}},
		{"*|news:won", {"{who} won {what}. {money}, I heard.", "You hear {who} took down {what}? {money}."}},
		{"*|news:final", {"{who} made the final table of {what}."}},
		{"*|news:break", {"{who}'s taking a break from the tables. Smart, maybe."}},
		{"*|news:returned", {"{who}'s back at it. Couldn't stay away."}},
		{"*|news:breakout", {"{who}'s running hot. Everybody's talking about it."}},
		{"*|news:champion", {"{who} won {what}. That's a real title. {money}."}},
		{"*|news:sponsor", {"{who} got a sponsor. Patch and everything."}},
		{"*|news:streaming", {"{who} started streaming. On Kast. With a webcam and everything."}},
		{"*|news:milestone", {"{who} hit a milestone. {what}."}},
		{"*|service", {"Sure thing."}},
		{"*|b:bump", {"Watch it.", "Hey!", "Easy."}},
		{"*|b:bump-known", {"Easy, champ.", "Whoa. Slow down."}},
		{"*|b:recognize", {"Hey\xE2\x80\x94" "aren't you {screen}?", "Wait. {screen}? From the stream?"}},
		{"*|b:rain", {"Here it comes.", "Coming down now.", "Should've brought the umbrella."}},
		{"*|b:closing", {"Closed. {Next}."}},
		{"*|b:excuse", {"Excuse me.", "Pardon.", "'Scuse me."}},

		// ---------------------------------------------------------------- the regulars, by role
		{"@regular|greet", {"Hey.", "Evening."}},
		{"@regular|b:bump", {"Hey, watch it.", "Easy, easy."}},
		{"@local|greet@market-stop", {"Fourteen up to the Embercrest? Should be along.", "You playing tonight?"}},
		{"@local|greet", {"Hey there.", "Evening."}},
		{"@waitress|k:greet", {"Sit anywhere, hon."}},
		{"@waitress|k:pay", {"Thanks, hon."}},

		// ---------------------------------------------------------------- Lonnie Pruitt, the barber
		{"lonnie|greet", {"Look who's here. Chair's warm.", "Come in, come in. Don't mind the radio."}},
		{"lonnie|greet:first", {"New face. Sit anywhere, I'll get to you. Lonnie."}},
		{"lonnie|look:hungry", {"You eat today? There's a patty melt at Ruby's with your name on it. Go. The chair'll keep."}},
		{"lonnie|look:tired", {"You look like you slept in a booth at Ruby's. Sit. I won't tell."}},
		{"lonnie|look:soaked", {"Shake off by the door, kid. Not on my floor."}},
		{"lonnie|look:cut", {"There it is. Holding up nice. You're welcome."}},
		{"lonnie|look:new", {"New jacket? Look at you. Somebody's winning."}},
		{"lonnie|look:famous", {"My nephew watches you on that Kast. Says you fold too much. I told him that's how you get old."}},
		{"lonnie|bond:away", {"Look who it is. I was about to put your picture on a milk carton."}},
		{"lonnie|bond:evicted", {"Heard about the apartment. Dee's couch is lumpy, but she means well."}},
		{"lonnie|bond:tipper", {"My best tipper walks in. Sit, sit."}},
		{"lonnie|bond:stiffed", {"Last time you forgot something at the register. I remember. I always remember."}},
		{"lonnie|bond:regular", {"There's my regular. Same as last time?"}},
		{"lonnie|bg:dealerskid", {"Your mother dealt downtown, didn't she? Dee says you've got her hands. Don't waste 'em."}},
		{"lonnie|bg:hustler", {"You still run with Marcus? Don't answer that. I cut hair, I don't hear things."}},
		{"lonnie|bg:kitchen", {"Line cook? I can tell by the forearms. Burns heal, kid."}},
		{"lonnie|bg:bouncer", {"You worked a door somewhere. You stand like you're waiting for somebody to start something."}},
		{"lonnie|bg:dropout", {"College kid. Numbers. Lot of you on Fifth lately."}},
		{"lonnie|bg:newcomer", {"New in town? Fifth's all right once it knows your name. Takes about three haircuts."}},
		{"lonnie|news:none", {"Dee was in Thursday. Says you fold too much. Says it like it's a compliment."}},
		{"lonnie|result:cash", {"Saw you cashed the {event}. {place}? Buy yourself a real haircut."}},
		{"lonnie|result:win", {"You won the {event}? Kid. Sit down, this one's on the house. The next one isn't."}},
		{"lonnie|result:bust", {"Rough one at the {event}. Thirty-one years I've cut hair on Fifth. Everybody busts."}},
		{"lonnie|games:own", {"Monday Nightly up the hill, that's me. Shop's shut Mondays so I can lose in peace."}},
		{"lonnie|rumour:garage", {"There's a game in a garage across the river. Thursdays at ten, five-dollar big blind. Rick plays. I don't, and neither should you."}},
		{"lonnie|service", {"Sit down, sit down."}},
		{"lonnie|service:wait", {"Grab the bench. About {queue} minutes. There's a paper."}},
		{"lonnie|bye", {"Go on. Keep your head down and your collar clean.", "See you in three weeks. Not four."}},
		{"lonnie|k:greet", {"Sit down, sit down. Same as last time, or you finally gonna let me fix that?", "Chair's yours. What are we doing?"}},
		{"lonnie|k:browse", {"Take your time. The hair's not going anywhere. Well. Some of it."}},
		{"lonnie|k:pick", {"Good choice.", "That'll suit you."}},
		{"lonnie|k:pick:color", {"Grey's coming in. Distinguished. Or forty-five bucks, your call."}},
		{"lonnie|k:grow", {"Can't put hair back on, kid. Give it three weeks."}},
		{"lonnie|k:wait", {"Grab the bench. About {queue} minutes. There's a paper.", "Grab the bench. Won't be long."}},
		{"lonnie|k:pay", {"Hold still. Let's see what we've got."}},
		{"lonnie|k:declined", {"Card says no. I don't cut on credit, kid. Thirty-one years, I never have."}},
		{"lonnie|k:tip", {"Appreciated. Go show somebody."}},
		{"lonnie|k:stiff", {"Mm. All right."}},
		{"lonnie|k:leave", {"Go on. Show it off."}},
		{"lonnie|k:closing", {"Broom's out. That means you."}},
		{"lonnie|b:closing", {"Closed, kid. {Next}."}},
		{"lonnie|b:bump", {"Easy, kid."}},
		{"lonnie|b:rain", {"Rain's good for business. Everybody needs a place to stand."}},

		// ---------------------------------------------------------------- Bea Kowalski, Second Chance Thrift
		{"bea|greet", {"Welcome to Second Chance. Everything here's been loved once already.", "Hello, sweetheart. New stock's on the left."}},
		{"bea|look:hungry", {"You're too thin. Soup at St. Agnes Wednesdays. Or Ruby's, if you're proud."}},
		{"bea|look:soaked", {"Don't drip on the cashmere. We have one cashmere."}},
		{"bea|look:new", {"Is that one of ours? It is! I tagged that."}},
		{"bea|bond:donor", {"My favorite donor. The Lord sees. So does the IRS."}},
		{"bea|bond:away", {"There you are. I thought you'd found a nicer thrift. There isn't one."}},
		{"bea|news:none", {"Father Mike's knees are bad again. Pray for him, he won't."}},
		{"bea|games:own", {"Bingo's Thursday at St. Agnes. That's the only game I know, and I win it."}},
		{"bea|service", {"Racks are by size. Mostly. People put things back wherever."}},
		{"bea|bye", {"Go with God. And bring the hangers back."}},
		{"bea|k:greet", {"Yellow tags are half off Saturday. Today's {weekday}. So, no.", "Racks are by size. Mostly."}},
		{"bea|k:greet:saturday", {"Yellow tags are half off today, sweetheart. Go wild. Responsibly."}},
		{"bea|k:saturday", {"Yellow tags are half off today, sweetheart. Go wild. Responsibly."}},
		{"bea|k:browse", {"That jacket came in a box from a man who moved to Florida. You wear it better.", "New stock Thursdays. The good stuff goes by Friday."}},
		{"bea|k:pick", {"Good eye. That one's barely worn.", "Mm. That one's got character."}},
		{"bea|k:try", {"Booth's on the left. Curtain sticks. Pull, don't yank."}},
		{"bea|k:pay", {"There you go. God bless."}},
		{"bea|k:declined", {"Card says no, sweetheart. Happens to the best of us. I'll hold it till Thursday."}},
		{"bea|k:donate", {"God bless. And you're writing that off, sweetheart."}},
		{"bea|k:leave", {"Come back Thursday. New boxes."}},
		{"bea|k:closing", {"We close at {closes}, sweetheart. Bring it to the register or bring it back {next}."}},
		{"bea|b:closing", {"We're closed, sweetheart. {Next}."}},
		{"bea|b:bump", {"Mind the hip, sweetheart. It's new."}},

		// ---------------------------------------------------------------- Rosa Delgado, IronHouse Gym
		{"rosa|greet", {"You here to work or to look at the equipment?", "Hey. Sign in, then sweat."}},
		{"rosa|look:tired", {"You look like you sleep in a chair."}},
		{"rosa|look:hungry", {"Eat first. I don't train empty tanks."}},
		{"rosa|look:fit", {"You're carrying yourself different. Good. Keep showing up."}},
		{"rosa|bond:regular", {"Look who keeps showing up. That's the whole secret, by the way."}},
		{"rosa|bg:bouncer", {"Door work? Then you know: the one who stays calm wins. Same in here."}},
		{"rosa|news:none", {"Somebody keeps putting the twenty-fives on the wrong rack. When I find out who."}},
		{"rosa|games:own", {"Lonnie plays up the hill Mondays. Me, I box. Cards make you soft in the middle."}},
		{"rosa|service", {"Day pass is ten. Towel's free."}},
		{"rosa|service:member", {"Your fob's good. Want a session? Forty, and I make you hate me."}},
		{"rosa|bye", {"Hydrate.", "Go. Sleep in a bed."}},
		{"rosa|k:greet", {"Towel's free. The shake isn't.", "Pick a machine. Don't just stand there."}},
		{"rosa|k:browse", {"Treadmill if you want to think. Bag if you don't."}},
		{"rosa|k:pick", {"Good. That one'll hurt in the right places.", "Fine choice. Don't baby it."}},
		{"rosa|k:train", {"Four in, hold four, four out. Works in the ring. Works when some guy shoves on the river."}},
		{"rosa|k:pay", {"Good. Now earn it."}},
		{"rosa|k:declined", {"Card's flat. Come back when it isn't. The bag'll wait."}},
		{"rosa|k:leave", {"Same time next week. I'll know if you don't."}},
		{"rosa|k:closing", {"Desk closes in ten. Fob works after. I don't."}},
		{"rosa|k:door", {"Fob works. Lights work. I don't, till {opens}."}},
		{"rosa|b:closing", {"Fob works. Lights work. I don't, till {opens}."}},
		{"rosa|b:bump", {"Footwork. Work on it."}},

		// ---------------------------------------------------------------- Ruby Okafor, days at Ruby's
		{"ruby|greet", {"Sit anywhere that isn't Sal's.", "Thirty-one years and the grill's the only thing in here that never needed a day off."}},
		{"ruby|greet:morning", {"Morning, baby. Coffee's on.", "Sit anywhere that isn't Sal's."}},
		{"ruby|look:hungry", {"You're eating. Don't argue with me, I've got a spatula."}},
		{"ruby|look:soaked", {"Hang that by the radiator. Sit, sit."}},
		{"ruby|bg:kitchen", {"Line cook? I can tell by how you look at my grill. Don't get ideas."}},
		{"ruby|bond:stiffed", {"I hear you forgot something last time. Marlene doesn't forget. Neither do I."}},
		{"ruby|news:none", {"Sal's been reading the same paper since Tuesday. Says the news doesn't change, only the names."}},
		{"ruby|games:own", {"Big Lou comes in Fridays after his game. Ask him. He'll tell you more than you want."}},
		{"ruby|service", {"Menu. Pancakes are good. Everything's good."}},
		{"ruby|bye", {"Go on. Don't be a stranger.", "Take care, baby."}},
		{"ruby|k:greet", {"Sit anywhere that isn't Sal's.", "What'll it be, baby?"}},
		{"ruby|k:browse", {"The Short Stack's what you came for. Everybody does, eventually."}},
		{"ruby|k:pick", {"Good choice.", "Coming right up."}},
		{"ruby|k:pay", {"Thank you, baby."}},
		{"ruby|k:declined", {"Card's being shy. Pay me next time. Don't make me come find you."}},
		{"ruby|k:tip", {"Bless you, baby."}},
		{"ruby|k:stiff", {"Mm."}},
		{"ruby|k:coffee", {"Fresh pot."}},
		{"ruby|k:sit", {"Stay as long as you like. That's what the booths are for."}},
		{"ruby|k:leave", {"Go on. Don't be a stranger."}},

		// ---------------------------------------------------------------- Marlene, nights at Ruby's
		{"marlene|greet", {"Coffee, hon? You look like you lost a flip.", "Sit wherever. It's just you and the pie case."}},
		{"marlene|look:tired", {"Third coffee? I'll bring the pot."}},
		{"marlene|look:hungry", {"Hon, you're eating. I'm not asking."}},
		{"marlene|bond:stiffed", {"Oh, it's you. We take cash up front at this hour, hon. Kidding. Mostly."}},
		{"marlene|bond:tipper", {"There's my favorite. Your usual booth's open."}},
		{"marlene|news:none", {"Cook burned his hand again. Says it's character. Says that every time."}},
		{"marlene|games:own", {"Ask Mei. She's in the corner booth most nights, playing on that laptop like it owes her money."}},
		{"marlene|rumour:basement", {"There's a game in a basement downtown, Saturdays at eleven. Twenty-dollar blinds. The guys who play it tip like they've got something to prove."}},
		{"marlene|service", {"Menu's on the table, hon."}},
		{"marlene|bye", {"Night, hon. Watch the step."}},
		{"marlene|k:greet", {"Coffee, hon? You look like you lost a flip.", "What can I get you, hon?"}},
		{"marlene|k:browse", {"Pie's cherry. It's always cherry. Don't ask about the apple."}},
		{"marlene|k:pick", {"You got it, hon."}},
		{"marlene|k:pay", {"Thanks, hon."}},
		{"marlene|k:declined", {"Card says no, hon. Happens at four in the morning. Happens to everybody."}},
		{"marlene|k:tip", {"You're a doll."}},
		{"marlene|k:stiff", {"Mm-hm. Come again."}},
		{"marlene|k:coffee", {"Top you off?"}},
		{"marlene|k:sit", {"Stay. Nobody's waiting on that booth till the breakfast crowd."}},
		{"marlene|k:leave", {"Night, hon. Watch the step."}},

		// ---------------------------------------------------------------- the cook
		{"cook|greet", {"Can't talk, I got eggs going.", "Order up in a sec."}},
		{"cook|bye", {"Yep."}},
		{"cook|b:hello", {"Order up!"}},
		{"cook|b:bump", {"Behind!"}},

		// ---------------------------------------------------------------- August "Augie" Szabo, Northside Pawn & Loan
		{"augie|greet", {"Come in, the buzzer's slow. So am I.", "Afternoon. Set whatever it is on the glass."}},
		{"augie|look:hungry", {"You look hungry. People who come in hungry sell cheap. Eat first."}},
		{"augie|look:new", {"Nice jacket. I'd give you nine for it. Don't sell it to me."}},
		{"augie|bond:owed", {"Your ticket's due {due}. Don't make me sell your things. I hate that part."}},
		{"augie|bond:evicted", {"Heard you're on Dee's couch. Your things in the unit on Ninth? Keep the unit paid. I've seen what happens."}},
		{"augie|bg:dealerskid", {"Your mother dealt downtown. I knew her face before I knew yours. Good hands, that woman."}},
		{"augie|bg:bouncer", {"Embercrest door, right? I know the look. You count the room when you walk in."}},
		{"augie|bg:hustler", {"Friend of Marcus? Then you know my rule: I don't buy anything with a story attached."}},
		{"augie|news:twitch", {"Your friend's watch has been in that cage more than I have."}},
		{"augie|news:none", {"Forty years I've watched card players walk in with watches and out without 'em. Nothing new under the sun."}},
		{"augie|games:own", {"Games? Ask Lonnie. I've got enough card players' jewelry as it is."}},
		{"augie|rumour:garage", {"Fellas from the garage game across the river come in Fridays with rings. Thursday nights, ten o'clock, five-dollar blind. You didn't hear it from me."}},
		{"augie|service", {"Set it on the glass. Let me look."}},
		{"augie|bye", {"Go on. Mind the door, it sticks."}},
		{"augie|k:greet", {"Come in, the buzzer's slow. So am I.", "Set whatever it is on the glass."}},
		{"augie|k:browse", {"Everything in the case is tagged. Everything on the wall is somebody's. Don't touch the wall."}},
		{"augie|k:pick", {"That one's clean. I checked.", "Good piece. Works like the day it was bought."}},
		{"augie|k:appraise", {"Set it on the glass. Let me look."}},
		{"augie|k:appraise:gear", {"Works? Don't tell me. I'll find out."}},
		{"augie|k:appraise:cup", {"Pot metal, gold wash. It's a nice cup."}},
		{"augie|k:appraise:award", {"Forty years, six of these have come over this glass. Two came back for 'em."}},
		{"augie|k:appraise:keepsake", {"Somebody gave you this to keep. I'll hold it. I won't judge."}},
		{"augie|k:appraise:garment", {"Leather I'll take. The rest, try Bea."}},
		{"augie|k:appraise:lot114", {"Guy bought a unit on Ninth. Brought me {lot114}. One-fourteen. Mean anything to you?"}},
		{"augie|k:offer", {"Two numbers. One to own it, one to hold it thirty days. The ticket's your receipt. Don't lose it."}},
		{"augie|k:offer:numbers", {"{Sell} to own it. {Loan} to hold it, thirty days. The ticket's your receipt. Don't lose it."}},
		{"augie|k:refuse", {"I'm not a furniture store."}},
		{"augie|k:refuse:dee-chip", {"Dee's? Put that back in your pocket. She'd have my license."}},
		{"augie|k:refuse:furniture", {"I'm not a furniture store."}},
		{"augie|k:loan", {"Pink copy's yours. Thirty days, then seven of grace. Then it's mine."}},
		{"augie|k:sell", {"Count it. Everybody counts it.", "There. It's mine now. Don't come back crying, come back with a better one."}},
		{"augie|k:redeem", {"Look at that. It came home."}},
		{"augie|k:renew", {"Twenty percent buys you thirty more days. Same ticket. Don't make a habit of it."}},
		{"augie|k:forfeit", {"It was yours. Thirty days and a week. That's the deal."}},
		{"augie|k:guitars", {"Guitars I don't sell to card players. You'd hock it back to me inside a month."}},
		{"augie|k:pay", {"Pleasure."}},
		{"augie|k:declined", {"Short? Come back with the rest. It's not going anywhere. For thirty days."}},
		{"augie|k:leave", {"Mind the door. It sticks."}},
		{"augie|k:closing", {"Buzzer's off in ten. Ticket or cash, make it quick."}},
		{"augie|b:closing", {"Closed. {Next}. Buzzer's off."}},
		{"augie|b:bump", {"Careful."}},

		// ---------------------------------------------------------------- the Lucky Penny: Ray Molina (days), Benny (nights)
		{"ray|greet", {"Welcome in. Lotto's up to forty million.", "Afternoon. Need anything, holler."}},
		{"ray|greet:morning", {"Morning. Benny leave the coffee on? Course he didn't."}},
		{"ray|look:hungry", {"Roller grill's on. Fresh, not Benny-fresh."}},
		{"ray|bond:regular", {"There's my regular. Benny says you keep him company nights."}},
		{"ray|games:own", {"I sell the paper, I don't read it. Sal does. Ask Sal."}},
		{"ray|service", {"What can I get you?"}},
		{"ray|bye", {"Take care now."}},
		{"ray|k:greet", {"Welcome in.", "What can I get you?"}},
		{"ray|k:browse", {"Take your time. Holler if you can't find it.", "Twenty-two years I've stocked these shelves. Ask me where anything is."}},
		{"ray|k:pick", {"Good choice.", "That's a good one. Sells out by Friday."}},
		{"ray|k:pay", {"There you go. Bag?"}},
		{"ray|k:declined", {"Card's not going through. Try it again, or come back. No rush."}},
		{"ray|k:leave", {"Have a good one.", "Take it easy out there."}},
		{"benny|greet", {"Rough night? Everybody in here at this hour is having one.", "Hey. Coffee's fresh at midnight. It's not midnight. Still hot, though."}},
		{"benny|look:hungry", {"You look like you need a roller dog. They're fresh. Ish."}},
		{"benny|look:tired", {"You look like you need the whole pot. I'll start another."}},
		{"benny|bond:evicted", {"Dee says you're on her couch for a while. She's good people. Don't eat all her cereal."}},
		{"benny|bond:away", {"Look who it is. I thought you moved."}},
		{"benny|games:own", {"Dee's game, Tuesdays, Thursdays, Saturdays. I'd go, but somebody's gotta sell scratchers."}},
		{"benny|rumour:garage", {"Guy came in for a Volt Rush at four, said there's a game in a garage across the river. Thursdays at ten. Five-dollar blind. Rick plays. Rick scares me."}},
		{"benny|rumour:nightshift", {"{leader}'s on top of the Night Shift board again. I watch it on my phone. Don't tell Ray."}},
		{"benny|service", {"Grab what you want, I'll ring you up."}},
		{"benny|bye", {"Stay out of trouble. Or, you know. Less trouble."}},
		{"benny|k:greet", {"Hey. Grab what you want."}},
		{"benny|k:browse", {"Take your time. I'm not going anywhere. Literally. Till ten."}},
		{"benny|k:pick", {"Solid pick.", "Good call. The other one's a lie."}},
		{"benny|k:pay", {"That it? Bag?"}},
		{"benny|k:declined", {"Card says no, man. Want to put something back?"}},
		{"benny|k:leave", {"Stay dry out there."}},

		// ---------------------------------------------------------------- the Wash & Fold
		{"inez|greet", {"Dryer six eats quarters. Use seven.", "Change machine's working tonight. Miracles happen."}},
		{"inez|games:own", {"Dee's back room? Tuesdays, Thursdays, Saturdays. I don't hear anything. I fold."}},
		{"inez|news:none", {"Somebody left a whole load in four since Sunday. If it's yours, it smells like it."}},
		{"inez|bye", {"Take your lint with you."}},
		{"dee|greet", {"You coming in, or do you just like the smell of fabric softener?", "Evening. Table's warm."}},
		{"dee|bg:dealerskid", {"You've got your mother's hands. She'd want you to use them better than she did."}},
		{"dee|bond:evicted", {"Couch is yours as long as you need it. Don't make me regret it."}},
		{"dee|games:own", {"My game's Tuesday, Thursday, Saturday. Doors at nine. Don't be late and don't be loud."}},
		{"dee|rumour:dees", {"My game's {when}. Doors at nine. {names}."}},
		{"dee|rumour:dees-now", {"We're dealing right now. {names}. Pull up a chair or get out of the doorway."}},
		{"dee|bye", {"Go on. Play tight."}},
		{"marcus|greet", {"Not here. Not now."}},
		{"marcus|bye", {"Burner. Like always."}},

		// ---------------------------------------------------------------- Sal
		{"sal|greet@washfold", {"Dee's got a seat open.", "Evening. You in tonight?"}},
		{"sal|greet@diner", {"Paper says rain. Paper's been wrong since seventy-four.", "Sit. Ruby'll come. Ruby always comes."}},
		{"sal|greet@market-stop", {"Fourteen's late. Fourteen's always late."}},
		{"sal|greet", {"Evening.", "Hey, kid."}},
		{"sal|look:hungry", {"You eat today? Eat. Card players who don't eat make bad calls at three in the morning."}},
		{"sal|look:tired", {"You look like I feel. Go to bed. Cards'll be there."}},
		{"sal|bg:dealerskid", {"Your mother dealt downtown. I lost a lot of money to that woman's deck. Never her fault."}},
		{"sal|news:none", {"Nothing in the paper but the paper."}},
		{"sal|games:own", {"Dee's. Tuesdays, Thursdays, Saturdays. Doors at nine. I'm in seat two, don't take it."}},
		{"sal|rumour:garage", {"There's the Garage, across the river. Thursdays at ten, five-dollar big blind. Rick plays. I quit going when my knees did."}},
		{"sal|rumour:basement", {"Basement downtown, Saturdays at eleven. Twenty-dollar blind. Not for you. Not yet."}},
		{"sal|bye", {"Go on. Don't fold the sevens.", "See you at Dee's."}},
		{"sal|b:hello", {"Kid.", "Evening."}},

		// ---------------------------------------------------------------- Mei
		{"mei|greet@diner", {"Night Shift board: I'm {meirank}. You're... {herorank}", "Hey. Don't look at my screen, it's not going well."}},
		{"mei|greet:nb@diner", {"Hey. Don't look at my screen, it's not going well.", "Coffee's terrible here. I love it."}},
		{"mei|greet@washfold", {"Hey. Dee's? Dee's."}},
		{"mei|greet", {"Hey.", "Oh. Hi."}},
		{"mei|look:tired", {"You look like you've been four-tabling since Tuesday."}},
		{"mei|bg:dropout", {"Stats dropout? Same. Different school. Don't tell my mom."}},
		{"mei|games:own", {"The Nightly's soft if you can get up there. Or the Night Shift. I'm on the Night Shift."}},
		{"mei|rumour:nightshift", {"{leader}'s leading the Night Shift board. Again. I don't want to talk about it."}},
		{"mei|bye", {"gl. I mean. Good luck."}},

		// ---------------------------------------------------------------- Big Lou
		{"big-lou|greet@washfold", {"Wings! Don't touch the hot ones, they're Dee's.", "You hungry? I brought too many. I always bring too many."}},
		{"big-lou|greet@diner", {"Sit down, sit down. Marlene! Another coffee."}},
		{"big-lou|greet", {"Hey! Look at this guy.", "There you are."}},
		{"big-lou|check", {"Put it on mine. You cashed, I eat. That's the rule."}},
		{"big-lou|k:check", {"Put it on mine. You cashed, I eat. That's the rule."}},
		{"big-lou|look:hungry", {"Eat something. On me. Don't argue, I'm bigger."}},
		{"big-lou|games:own", {"Saturday Big Stack up the hill. And Dee's, of course. Never miss Dee's."}},
		{"big-lou|rumour:basement", {"The Basement downtown, Saturdays at eleven. Twenty-dollar blind. I go when I'm feeling rich. I'm never that rich."}},
		{"big-lou|rumour:garage", {"Rick runs the Garage across the river. Thursdays at ten. Five-dollar blind. I lost a car there once. Long story."}},
		{"big-lou|bye", {"Go easy. And take a wing."}},

		// ---------------------------------------------------------------- Twitch
		{"twitch|greet", {"Yo. You got a second? Like, a financial second?", "Hey. Hey. What's up. I'm good. Everything's good."}},
		{"twitch|greet@washfold", {"Yo! Seat next to me's open. Probably bad luck, but it's open."}},
		{"twitch|bond:repaid", {"Here. Twenty. Told you. Told you I'm good for it."}},
		{"twitch|bond:owes", {"I know, I know. Twenty. I'm getting it. I'm close. I'm so close."}},
		{"twitch|games:own", {"Midnight Turbo, Fridays, Saturdays. Ten-minute levels. It's basically a slot machine, but you get to look smart."}},
		{"twitch|rumour:garage", {"The Garage is where Rick goes to take people's money. Thursdays, ten o'clock, across the river. Five-dollar blind. I'm banned. Long story."}},
		{"twitch|rumour:nightshift", {"{leader}'s winning the Night Shift. I was winning it. For like an hour."}},
		{"twitch|lend", {"Twenty. Just twenty. I'm good for it. I'm literally always good for it."}},
		{"twitch|lend-yes", {"You're a saint. Saint of Fifth Street. I'll get you back, swear."}},
		{"twitch|lend-no", {"Yeah. No. That's fair. That's honestly fair."}},
		{"twitch|bye", {"Later. Wish me luck. I'm gonna need it."}},

		// ---------------------------------------------------------------- Mrs. Park
		{"mrs-park|greet@diner", {"Good morning. You're up early for a card player.", "Sit, sit. The Warm-Up starts at noon. I'm never late."}},
		{"mrs-park|greet@market-stop", {"The 11:35 is never late. People are."}},
		{"mrs-park|greet", {"Hello, dear."}},
		{"mrs-park|look:hungry", {"Eat something, dear. You can't think on an empty stomach."}},
		{"mrs-park|games:own", {"The Sunday Warm-Up. Noon, eighty dollars. Thirty years I've played it. Thirty-one, next month."}},
		{"mrs-park|bye", {"Bus is at eleven thirty-five. I'm going to go stand there for ten minutes, like a sensible person."}},

		// ---------------------------------------------------------------- sponsors (next_segment_spec §3): the keepers'
		// offers in person, the deals running, the patches people notice, and the word on the block
		{"ray|sponsor:pitch:luckypenny", {"Owner's been watching you come in at all hours for a month. He wants your picture on the corkboard. Got a minute?"}},
		{"ray|sponsor:terms:luckypenny", {"Twenty-five a week on the book, one Noon Deepstack on us a month, and you wear the penny when you play. I've got the slip right here."}},
		{"ray|sponsor:signed:luckypenny", {"Hold still. Say penny. There. That's going up right by the lotto board."}},
		{"ray|sponsor:appear:store", {"Stand by the lotto board. Don't blink. There. Owner's going to love that one."}},
		{"ray|sponsor:luckypenny", {"Your picture's getting looks. Guy asked me if you won the lotto.", "Owner says the corkboard never looked better. His words, not mine."}},
		{"lonnie|sponsor:pitch:barber", {"You sit in my chair more than my own nephew. How about you wear my name at those tournaments, and I keep you sharp for free?"}},
		{"lonnie|sponsor:terms:barber", {"A cut or a line-up every two weeks, on me. You wear the patch at two tournaments a month, and one Saturday you sit in the window chair."}},
		{"lonnie|sponsor:signed:barber", {"Good. First one's on me whenever you want it. Sit in the window some Saturday, let the street see the work."}},
		{"lonnie|sponsor:appear:barber", {"Window chair's yours. Sit up straight. People slow down when there's somebody in it."}},
		{"lonnie|sponsor:barber", {"Two guys came in asking who does your hair. That's the patch working.", "Keep that patch where people can see it. That's the whole deal."}},
		{"rosa|sponsor:pitch:ironhouse", {"Owner asked me who works hardest in here. I said you. He wants our patch on you at a tournament. We cover the membership."}},
		{"rosa|sponsor:terms:ironhouse", {"Membership's on us. Eight workouts a month and the patch at one tournament. I count, so don't make me chase you."}},
		{"rosa|sponsor:signed:ironhouse", {"Good. Now go put in the work. I'll be counting."}},
		{"rosa|sponsor:ironhouse", {"Eight a month. I'm counting.", "You've been coming in. Good. Keep that patch on at the tables."}},
		{"*|sponsor:later", {"Offer's good for the week."}},
		{"*|look:patch:luckypenny", {"Is that a penny patch? The corner store sponsors you now?"}},
		{"ray|look:patch:luckypenny", {"There's the penny. Owner's going to want a picture of you wearing it."}},
		{"*|look:patch:barber", {"Fifth St. Barbers on your sleeve. Lonnie's got you advertising."}},
		{"lonnie|look:patch:barber", {"My name on your sleeve. Don't get it dirty."}},
		{"*|look:patch:ironhouse", {"IronHouse patch, huh? Rosa's got you on the team."}},
		{"rosa|look:patch:ironhouse", {"Looks good on you. Now earn it."}},
		{"*|look:patch:voltrush", {"Volt Rush on the cap. They at least give you the drinks free?"}},
		{"*|look:patch:embercrest", {"That's the Embercrest's crest. You're on their team now?"}},
		{"*|look:patch:stacked", {"Stacked? Those hoodies cost more than my rent."}},
		{"*|look:patch:rangewise", {"Rangewise patch. So you're a teacher now."}},
		{"*|look:patch:riverline", {"Team RiverLine on your chest. Look at you."}},
		{"*|look:patch:turnstone", {"Turnstone Poker. Never heard of them. They pay?"}},
		{"*|rumour:sponsor-you", {"Heard {brand} is sponsoring you now. Look at you.", "Word is {brand} signed you. Don't forget the little people."}},
		{"big-lou|rumour:sponsor-you", {"{brand} sponsors you? Huh. Respect."}},
		{"sal|rumour:sponsor-you", {"{brand}, huh. Back in my day you paid for your own hat."}},
	};
	return T;
}

uint32_t Fnv(const std::string& S, uint32_t H = 2166136261u)
{
	for (const char Ch : S)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
	}
	return H;
}

const char* RoleKey(Job J)
{
	switch (J)
	{
	case Job::Clerk: return "@clerk";
	case Job::Barber: return "@barber";
	case Job::Thrift: return "@thrift";
	case Job::Trainer: return "@trainer";
	case Job::Waitress: return "@waitress";
	case Job::Cook: return "@cook";
	case Job::Pawnbroker: return "@pawnbroker";
	case Job::Host: return "@host";
	case Job::Attendant: return "@attendant";
	case Job::Regular: return "@regular";
	case Job::Local: return "@local";
	case Job::Fan: return "@fan";
	case Job::Pedestrian: return "@pedestrian";
	}
	return "@pedestrian";
}

/** Fills {name} placeholders from the context's variables (and Vars, for one line's own). */
std::string Fill(const TalkContext& C, const std::string& Line, const std::map<std::string, std::string>& Extra)
{
	std::string Out;
	for (size_t I = 0; I < Line.size(); ++I)
	{
		if (Line[I] == '{')
		{
			const size_t End = Line.find('}', I);
			if (End != std::string::npos)
			{
				const std::string Key = Line.substr(I + 1, End - I - 1);
				const std::string* Value = nullptr;
				const auto Given = Extra.find(Key);
				if (Given != Extra.end())
				{
					Value = &Given->second;
				}
				else
				{
					const auto Known = C.Vars.find(Key);
					Value = Known != C.Vars.end() ? &Known->second : nullptr;
				}
				if (!Value || Value->empty())
				{
					return ""; // a line that needs what we don't know isn't said
				}
				Out += *Value;
				I = End;
				continue;
			}
		}
		Out.push_back(Line[I]);
	}
	return Out;
}

/** What Speaker says about Topic: their own line, else their role's, else anyone's; "" when none (or it can't be filled). */
std::string Say(const TalkContext& C, const std::string& Topic, const std::map<std::string, std::string>& Extra = {})
{
	const Table& T = Lines();
	for (const std::string& Who : {C.Speaker, std::string(RoleKey(C.Role)), std::string("*")})
	{
		const auto It = T.find(Who + "|" + Topic);
		if (It == T.end() || It->second.empty())
		{
			continue;
		}
		const std::vector<std::string>& Options = It->second;
		const size_t Pick = static_cast<size_t>(Fnv(Topic, C.Salt) % static_cast<uint32_t>(Options.size()));
		for (size_t K = 0; K < Options.size(); ++K)
		{
			const std::string Line = Fill(C, Options[(Pick + K) % Options.size()], Extra);
			if (!Line.empty())
			{
				return Line;
			}
		}
	}
	return "";
}

/** Only the speaker's own line (no role or anyone's), for topics that should stay theirs. */
std::string Own(const TalkContext& C, const std::string& Topic)
{
	const Table& T = Lines();
	return T.count(C.Speaker + "|" + Topic) > 0 ? Say(C, Topic) : std::string();
}

const char* Band(double Hour)
{
	return Hour >= 5.0 && Hour < 11.0 ? "morning" : Hour >= 11.0 && Hour < 17.0 ? "day" : Hour >= 17.0 && Hour < 22.0 ? "evening" : "late";
}

std::string Capital(std::string S)
{
	if (!S.empty() && S[0] >= 'a' && S[0] <= 'z')
	{
		S[0] = static_cast<char>(S[0] - 'a' + 'A');
	}
	return S;
}

const char* const Ones[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
	"seventeen", "eighteen", "nineteen"};
const char* const Tens[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};

std::string Under100(long long N)
{
	if (N < 20)
	{
		return Ones[N];
	}
	return std::string(Tens[N / 10]) + (N % 10 != 0 ? std::string("-") + Ones[N % 10] : std::string());
}

/** The last two digits after a leading part: "-forty-seven", "-oh-five", "" for 00. */
std::string Tail(long long N)
{
	if (N == 0)
	{
		return "";
	}
	return N < 10 ? std::string("-oh-") + Ones[N] : "-" + Under100(N);
}

/** "fourth", "fourteenth", "twenty-first"; past 99, "number 140". */
std::string OrdinalWord(int N)
{
	static const char* const Th[] = {"zeroth", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth", "eleventh", "twelfth",
		"thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth"};
	static const char* const TensTh[] = {"", "", "twentieth", "thirtieth", "fortieth", "fiftieth", "sixtieth", "seventieth", "eightieth", "ninetieth"};
	if (N <= 0 || N > 99)
	{
		return "number " + std::to_string(N);
	}
	if (N < 20)
	{
		return Th[N];
	}
	return N % 10 == 0 ? std::string(TensTh[N / 10]) : std::string(Tens[N / 10]) + "-" + Th[N % 10];
}

/** "nine", "ten-thirty", "six-thirty-five", "noon", "midnight". */
std::string SpokenTime(double World)
{
	const long long M = static_cast<long long>(std::llround(std::fmod(std::fmod(World, 1440.0) + 1440.0, 1440.0))) % 1440;
	const long long H = M / 60;
	const long long Min = M % 60;
	if (Min == 0 && H == 12)
	{
		return "noon";
	}
	if (Min == 0 && H == 0)
	{
		return "midnight";
	}
	const long long H12 = H % 12 == 0 ? 12 : H % 12;
	return Under100(H12) + (Min == 0 ? std::string() : Min == 30 ? std::string("-thirty") : Tail(Min));
}

/** "twelve hundred bucks", "forty-two grand", "a million two". */
std::string SpokenMoney(Chips Cents)
{
	const long long D = static_cast<long long>(Cents / 100);
	if (D < 1000)
	{
		return SpokenDollars(D) + " bucks";
	}
	if (D < 10000)
	{
		return SpokenDollars(D / 10 * 10) + " bucks";
	}
	if (D < 1000000)
	{
		return SpokenDollars((D + 500) / 1000) + " grand";
	}
	const long long Tenths = (D + 50000) / 100000;
	return Tenths % 10 == 0 ? SpokenDollars(Tenths / 10) + " million" : SpokenDollars(Tenths / 10) + " point " + Ones[Tenths % 10] + " million";
}

std::string Names(const std::vector<std::string>& L)
{
	if (L.empty())
	{
		return "";
	}
	std::string Out = L[0];
	for (size_t I = 1; I < L.size(); ++I)
	{
		Out += (I + 1 == L.size() ? " and " : ", ") + L[I];
	}
	return Out;
}

/** The 14 that gets a rider up to the Embercrest for Start (town::BusFor), as it's said: "six-thirty-five", or "". */
std::string BusWord(double Start)
{
	const double Bus = BusFor(Start);
	return Bus >= 0.0 ? SpokenTime(Bus) : std::string();
}

/** "the Nightly" for an Embercrest occurrence, else the event's own name. */
std::string EventWord(const std::string& Id, const std::string& Name)
{
	const live::Occurrence O = live::FindOccurrence(Id);
	return O.Valid() ? std::string("the ") + O.T->Short : Name;
}

/** Who knows about the world's poker: everyone but Bea, Rosa, Inez and the cook. */
bool Follows(const TalkContext& C)
{
	return !(C.Speaker == "bea" || C.Speaker == "rosa" || C.Speaker == "inez" || C.Speaker == "cook" || C.Speaker == "marcus");
}

/** Who has heard of the underground games, and which. */
bool KnowsGarage(const std::string& S)
{
	return S == "lonnie" || S == "sal" || S == "big-lou" || S == "augie" || S == "twitch" || S == "benny";
}

bool KnowsBasement(const std::string& S)
{
	return S == "sal" || S == "big-lou" || S == "marlene";
}

void Item(std::vector<TalkContext::Item>& Out, const std::string& Key, const std::string& Topic, const std::string& Line)
{
	if (!Line.empty())
	{
		Out.push_back({Key, Topic, Line});
	}
}

/** The world's story about someone the speaker knows, said their way. */
std::string EventLine(const TalkContext& C, const world::World& W, const world::WorldEvent& E, std::string& Topic)
{
	const world::Npc* N = W.Get(E.Npc);
	if (!N)
	{
		return "";
	}
	switch (E.Kind)
	{
	case world::EventKind::Won: Topic = "news:won"; break;
	case world::EventKind::FinalTable: Topic = "news:final"; break;
	case world::EventKind::Break: Topic = "news:break"; break;
	case world::EventKind::Returned: Topic = "news:returned"; break;
	case world::EventKind::Breakout: Topic = "news:breakout"; break;
	case world::EventKind::Champion: Topic = "news:champion"; break;
	case world::EventKind::Sponsored: Topic = "news:sponsor"; break;
	case world::EventKind::StartedStreaming: Topic = "news:streaming"; break;
	case world::EventKind::Milestone: Topic = "news:milestone"; break;
	default: return "";
	}
	std::map<std::string, std::string> V;
	V["who"] = N->Name;
	V["what"] = E.What.empty() ? std::string("a tournament") : E.What;
	V["money"] = E.Amount > 0 ? Capital(SpokenMoney(E.Amount)) : std::string("Real money");
	return Say(C, Topic, V);
}

/** Builds what this speaker could tell: news (the world and the player), rumours, and games. */
void Gather(const Session& S, TalkContext& C)
{
	const world::World& W = S.Living();
	const bool Knows = Follows(C);
	const double Now = C.World;
	// The player's last result, if it was in the last three days (the latest of online and live).
	if (Knows)
	{
		std::string Id;
		std::string Event;
		int Place = 0;
		Chips Prize = 0;
		double At = -1.0e18;
		if (!S.History.empty())
		{
			const HistoryEntry& H = S.History.back();
			At = net::EntryStart(H);
			Id = H.EventId.empty() ? H.Name : H.EventId;
			Event = H.Name;
			Place = H.Place;
			Prize = H.Prize;
		}
		for (const life::LiveEntry& E : S.Life.LiveEntries)
		{
			if (E.State == life::LiveEntry::Finished && E.FinishedAt >= At)
			{
				At = E.FinishedAt;
				Id = E.Id;
				Event = EventWord(E.Id, E.Name);
				Place = E.Place;
				Prize = E.PrizeCents;
			}
		}
		if (!Id.empty() && At >= Now - 3.0 * 1440.0 && At <= Now && Place > 0)
		{
			std::map<std::string, std::string> V;
			V["event"] = Event;
			V["place"] = Capital(OrdinalWord(Place));
			const std::string Topic = Place == 1 ? "result:win" : Prize > 0 ? "result:cash" : "result:bust";
			Item(C.News, "result:" + Id, "news", Say(C, Topic, V));
		}
	}
	// A deal signed lately: the word on the block (once, by whoever tells it first).
	if (Knows && !C.NewSponsor.empty())
	{
		Item(C.News, C.NewSponsorKey, "rumour:sponsor-you", Say(C, "rumour:sponsor-you", {{"brand", C.NewSponsor}}));
	}
	if (!W.Ready())
	{
		return;
	}
	// What happened to the people they know, the last three days, newest first.
	if (Knows)
	{
		std::set<int> Known;
		for (const char* Name : {"Sal", "Mei", "Big Lou", "Twitch", "Mrs. Park", "Rick", "Dre"})
		{
			const int Id = W.Find(Name);
			if (Id >= 0 && Id != C.Npc)
			{
				Known.insert(Id);
			}
		}
		const std::vector<world::WorldEvent>& Log = W.Events();
		int Count = 0;
		for (auto It = Log.rbegin(); It != Log.rend() && It->At >= Now - 3.0 * 1440.0 && Count < 3; ++It)
		{
			if (It->At > Now || Known.count(It->Npc) == 0)
			{
				continue;
			}
			std::string Topic;
			const std::string Line = EventLine(C, W, *It, Topic);
			if (!Line.empty())
			{
				Item(C.News, "news:" + std::to_string(static_cast<int>(It->Kind)) + ":" + std::to_string(It->Npc) + ":" + std::to_string(net::DayOf(It->At)), "news", Line);
				++Count;
			}
		}
		// Augie on Twitch's watch, when the world has him broke.
		const world::Npc* Twitch = W.Get(W.Find("Twitch"));
		if (C.Speaker == "augie" && Twitch && (Twitch->St == world::Status::Broke || Twitch->Mood == world::Momentum::MajorDownswing))
		{
			Item(C.News, "", "news", Own(C, "news:twitch"));
		}
	}
	// Rumours. Last night's winner at the Embercrest.
	if (Knows)
	{
		for (int D = C.Day - 1; D <= C.Day; ++D)
		{
			for (const live::Occurrence& O : live::Occurrences(D))
			{
				const net::EventResult* R = W.ResultOf(O.Id);
				if (!R || R->FinalTable.empty() || O.Start + O.T->Hours * 60.0 > Now)
				{
					continue;
				}
				const net::Placing& P = R->FinalTable.front();
				std::map<std::string, std::string> V;
				V["event"] = std::string(O.T->Short);
				// When it was played: the Warm-Up isn't "last night".
				const bool Evening = O.T->StartMinute >= 17 * 60;
				V["when"] = O.Day == C.Day ? (Evening ? "tonight" : "today") : (Evening ? "last night" : "yesterday");
				if (P.Player == -1)
				{
					Item(C.News, "rumour:winner:" + O.Id, "rumour", Say(C, "rumour:winner-you", V));
					continue;
				}
				const world::Npc* N = W.Get(P.Player);
				V["who"] = N ? N->Name : P.Name;
				V["money"] = P.Prize > 0 ? Capital(SpokenMoney(P.Prize)) : std::string("Good money");
				Item(C.News, "rumour:winner:" + O.Id, "rumour", Say(C, "rumour:winner", V));
			}
		}
		// Tonight's Nightly: the faces the player would know.
		for (const live::Occurrence& O : live::Occurrences(C.Day))
		{
			if (std::string(O.T->Key) != "nightly" || O.Start <= Now)
			{
				continue;
			}
			std::vector<std::string> Faces;
			for (const int Id : W.RegisteredNow(O.Id))
			{
				const world::Npc* N = W.Get(Id);
				const world::Bond* B = W.BondWith(Id);
				if (N && N->Name != C.Name && (N->From == world::Origin::Cast || (B && !B->Label().empty())) && Faces.size() < 3)
				{
					Faces.push_back(N->Name);
				}
			}
			if (!Faces.empty())
			{
				std::map<std::string, std::string> V;
				V["names"] = Names(Faces) + (Faces.size() == 1 ? "'s" : " are");
				V["bus"] = BusWord(O.Start);
				Item(C.News, "rumour:nightly:" + std::to_string(C.Day), "rumour", Say(C, "rumour:nightly", V));
			}
		}
	}
	// The Night Shift board's leader (the ones who watch it).
	if (C.Speaker == "mei" || C.Speaker == "twitch" || C.Speaker == "benny")
	{
		const net::HeroStats Hero = net::StatsFrom(S.HeroName, S.History, Now);
		net::BoardRow Mine;
		const std::vector<net::BoardRow> Board = net::Shared().Leaderboard(net::Board::NightShift, Now, Hero, 20, &Mine);
		if (!Board.empty() && Board.front().Value > 0.0)
		{
			const net::BoardRow& Top = Board.front();
			const std::vector<net::Player>& Rows = net::Shared().Players();
			if (Top.Player == -1)
			{
				Item(C.News, "rumour:nightshift:" + std::to_string(C.Day), "rumour", Say(C, "rumour:nightshift-you"));
			}
			else if (Top.Player >= 0 && Top.Player < static_cast<int>(Rows.size()) && Rows[static_cast<size_t>(Top.Player)].Name != C.Name)
			{
				std::map<std::string, std::string> V;
				V["leader"] = Rows[static_cast<size_t>(Top.Player)].Name;
				Item(C.News, "rumour:nightshift:" + std::to_string(C.Day), "rumour", Say(C, "rumour:nightshift", V));
			}
			if (C.Speaker == "mei")
			{
				for (const net::BoardRow& R : Board)
				{
					if (R.Player == C.Npc && R.Player >= 0)
					{
						C.Vars["meirank"] = OrdinalWord(R.Rank);
						C.Vars["herorank"] = Mine.Rank > 0 && Mine.Rank <= 20 && Mine.Value > 0.0 ? OrdinalWord(Mine.Rank) + ". Don't get comfortable." : std::string("not on it. Yet.");
					}
				}
			}
		}
	}
	// Games: Dee's next night (or the one being dealt now, past midnight too) and who's at it, the underground games
	// (once), the Embercrest's next event.
	for (int D = C.Day - 1; D <= C.Day + 6; ++D)
	{
		const int Wd = net::Weekday(D);
		const bool Night = Wd == 1 || Wd == 3 || Wd == 5;
		const double Ends = static_cast<double>(D + 1) * 1440.0 + 5.0 * 60.0;
		if (!Night || Ends <= Now)
		{
			continue;
		}
		const double Doors = static_cast<double>(D) * 1440.0 + 21.0 * 60.0;
		const bool Dealing = Now >= Doors;
		// Who's at it: the faces the player would know by name (the cast, anyone with a bond), three at most, and a word
		// for the rest of the table.
		std::vector<std::string> Coming;
		int Others = 0;
		for (const int Id : W.Entered("dees@" + std::to_string(D)))
		{
			const world::Npc* N = W.Get(Id);
			const world::Bond* B = W.BondWith(Id);
			if (!N || N->Name == C.Name)
			{
				continue;
			}
			if ((N->From == world::Origin::Cast || (B && !B->Label().empty())) && Coming.size() < 3)
			{
				Coming.push_back(N->Name);
			}
			else
			{
				++Others;
			}
		}
		std::map<std::string, std::string> V;
		V["when"] = D == C.Day ? std::string("tonight") : D == C.Day + 1 ? std::string("tomorrow night") : std::string(net::WeekdayName(D, true));
		const char* const One = Dealing ? " is in" : " is coming";
		const char* const Many = Dealing ? " are in" : " are coming";
		const std::string More = Others == 0 ? std::string() : Others == 1 ? std::string(", and one more") : std::string(", and a few more");
		V["names"] = Coming.empty() ? std::string(C.Speaker == "dee" ? "The usual crowd" : "The usual crowd, probably") : Names(Coming) + (Coming.size() == 1 ? One : Many) + More;
		if (Knows || C.Speaker == "inez")
		{
			Item(C.Games, "rumour:dees:" + std::to_string(D), "rumour", Say(C, Dealing ? "rumour:dees-now" : "rumour:dees", V));
		}
		break;
	}
	if (KnowsGarage(C.Speaker))
	{
		Item(C.Games, "rumour:garage", "rumour", Own(C, "rumour:garage"));
	}
	if (KnowsBasement(C.Speaker))
	{
		Item(C.Games, "rumour:basement", "rumour", Own(C, "rumour:basement"));
	}
	if (Knows)
	{
		for (int D = C.Day; D <= C.Day + 1; ++D)
		{
			bool Found = false;
			for (const live::Occurrence& O : live::Occurrences(D))
			{
				if (O.Start <= Now + 25.0)
				{
					continue;
				}
				std::map<std::string, std::string> V;
				V["event"] = std::string(O.T->Short);
				V["time"] = SpokenTime(O.Start) + (D == C.Day ? std::string() : std::string(" tomorrow"));
				V["bus"] = BusWord(O.Start);
				Item(C.Games, "", "games", Say(C, "games:embercrest", V));
				Found = true;
				break;
			}
			if (Found)
			{
				break;
			}
		}
	}
	Item(C.Games, "", "games", Say(C, "games:own"));
}

/** The service a person offers where they stand now: its choice label, or "". */
std::string ServiceLabel(const TalkContext& C)
{
	if (!C.AtWork)
	{
		return "";
	}
	if (C.Venue == "barber")
	{
		return "Get a cut";
	}
	if (C.Venue == "diner" && C.Role == Job::Waitress)
	{
		return "Menu";
	}
	if (C.Venue == "gym")
	{
		return C.Member ? "Book a session" : "Day pass";
	}
	if (C.Venue == "pawn")
	{
		return "Sell or pawn";
	}
	if (C.Venue == "thrift")
	{
		return "Show me the racks";
	}
	if (C.Venue == "store")
	{
		return "Ring me up";
	}
	return "";
}

std::vector<Choice> Choices(const TalkContext& C)
{
	std::vector<Choice> Out;
	if (C.Role == Job::Cook || C.Speaker == "marcus")
	{
		Out.push_back({"bye", "See you."});
		return Out;
	}
	Out.push_back({"news", "What's new?"});
	Out.push_back({"games", "Any games around?"});
	const std::string Service = ServiceLabel(C);
	if (!Service.empty())
	{
		Out.push_back({"service", Service});
	}
	if (C.CanAsk)
	{
		Out.push_back({"lend", "What do you need?"});
	}
	// A keeper with an offer for their shop (§3.1: Ray's "Hear him out"), and an appearance window open with them now.
	if (!C.SponsorOffer.empty())
	{
		Out.push_back({"sponsor", C.Who && C.Who->Woman ? "Hear her out" : "Hear him out"});
	}
	if (!C.SponsorAppear.empty())
	{
		Out.push_back({"sponsor-appear", C.SponsorAppear == "store" ? "Take the picture" : "Sit in the window"});
	}
	Out.push_back({"bye", "See you."});
	return Out;
}

bool Told(const TalkContext& C, const std::string& Key)
{
	return !Key.empty() && C.Told.count(Key) > 0;
}

const char* BackgroundKey(hero::Background B)
{
	switch (B)
	{
	case hero::Background::Kitchen: return "bg:kitchen";
	case hero::Background::DealersKid: return "bg:dealerskid";
	case hero::Background::Dropout: return "bg:dropout";
	case hero::Background::Bouncer: return "bg:bouncer";
	case hero::Background::Hustler: return "bg:hustler";
	case hero::Background::Newcomer: return "bg:newcomer";
	case hero::Background::Count: break;
	}
	return "bg:newcomer";
}

/** The base greeting: the venue's and the hour's, the weather's when it's coming down. */
std::string Greeting(const TalkContext& C, std::string& Topic)
{
	Topic = "hour";
	if (C.Sky == weather::Sky::Storm || C.Raining)
	{
		const std::string Wet = Own(C, C.Sky == weather::Sky::Storm ? "weather:storm" : "weather:rain");
		if (!Wet.empty())
		{
			Topic = "weather";
			return Wet;
		}
	}
	const std::string B = Band(C.Hour);
	if (C.Speaker == "mei" && C.Venue == "diner" && C.Vars.count("meirank") == 0)
	{
		const std::string Line = Say(C, "greet:nb@diner");
		if (!Line.empty())
		{
			return Line;
		}
	}
	if (!C.Met)
	{
		const std::string First = Own(C, "greet:first");
		if (!First.empty())
		{
			return First;
		}
	}
	for (const std::string& T : {"greet@" + C.Venue + ":" + B, "greet@" + C.Venue, "greet:" + B})
	{
		const std::string Line = Own(C, T);
		if (!Line.empty())
		{
			return Line;
		}
	}
	const std::string Own_ = Own(C, "greet");
	if (!Own_.empty())
	{
		return Own_;
	}
	const std::string Role = Say(C, "greet@" + C.Venue);
	if (!Role.empty() && (C.Role == Job::Local || C.Role == Job::Regular))
	{
		return Role;
	}
	if (C.Raining)
	{
		Topic = "weather";
		return Say(C, C.Sky == weather::Sky::Storm ? "weather:storm" : "weather:rain");
	}
	return Say(C, std::string("greet:") + B);
}

/** The first thing they'd say, in ClerkLine's order. Sets the line, its topic and any one-time key. */
void Opener(const TalkContext& C, Talk& T, bool Keeper)
{
	auto Try = [&](bool When, const std::string& Topic, const std::string& Key = std::string(), const char* Shown = nullptr) {
		if (!When || !T.Line.empty() || Told(C, Key))
		{
			return;
		}
		const std::string Line = Say(C, Topic);
		if (!Line.empty())
		{
			T.Line = Line;
			T.Topic = Shown ? Shown : Topic;
			T.SaidKey = Key;
		}
	};
	const std::string Sp = C.Speaker;
	const bool Inside = C.AtWork || C.Venue == "diner"; // come in out of it
	// 0. A keeper with an offer for their shop says it first, once (§3.1); the talk card then has "Hear him out".
	const std::string SponsorTopic = "sponsor:" + C.SponsorBrand;
	Try(!Keeper && !C.SponsorOffer.empty(), "sponsor:pitch:" + C.SponsorBrand, "sponsor:pitch:" + C.SponsorOffer, SponsorTopic.c_str());
	// 1. How the player looks.
	Try(C.Hunger >= 70.0, "look:hungry");
	Try(C.Energy < 25.0, "look:tired");
	Try(C.Thirst >= 75.0, "look:thirsty");
	Try(C.Soaked && Inside, "look:soaked");
	if (!Keeper)
	{
		Try(C.FreshCut && C.Met, "look:cut", "cut:" + Sp + ":" + std::to_string(C.CutDay));
		Try(C.NewLook, "look:new");
		// A sponsor's patch where it shows: each person remarks on each brand once.
		for (const std::string& Patch : C.Patches)
		{
			Try(true, "look:patch:" + Patch, "patch:" + Sp + ":" + Patch);
		}
		Try(C.Famous && C.Role != Job::Cook, "look:famous", "famous:" + Sp);
	}
	Try(Sp == "rosa" && C.Fitness >= 50.0 && C.Met, "look:fit", Keeper ? std::string() : "fit:rosa:" + std::to_string(C.Day / 30));
	// 2. What this person knows.
	Try(C.Repays, "bond:repaid", "repaid:twitch");
	Try(C.Owes && !C.Repays, "bond:owes");
	Try((C.Town.Flags & Stiffed) != 0, "bond:stiffed");
	Try(Sp == "augie" && C.Tickets > 0 && C.Vars.count("due") > 0, "bond:owed");
	if (!Keeper)
	{
		Try(C.Evicted, "bond:evicted", "evicted:" + Sp + ":" + std::to_string(C.EvictedDay));
		Try(C.Met && C.DaysAway >= 10, "bond:away", "away:" + Sp + ":" + std::to_string(C.Town.LastDay));
		Try(C.Met && C.Created, BackgroundKey(C.Story), "bg:" + Sp, "bond:background");
		if (T.Line.empty() && C.HasMemory && !C.Remembered.empty())
		{
			T.Line = C.Remembered;
			T.Topic = "bond:memory";
		}
	}
	Try(Sp == "bea" && (C.Town.Flags & Donor) != 0, "bond:donor");
	// The keeper of a shop with a deal running: a word about it now and then (a week apart).
	Try(!Keeper && C.SponsorActive && C.AtWork, SponsorTopic, SponsorTopic + ":" + std::to_string(C.Day / 7));
	Try((C.Town.Flags & Tipper) != 0, "bond:tipper");
	Try(C.Town.Visits >= 5, "bond:regular");
	if (Keeper)
	{
		return;
	}
	// 3 and 4. News, then rumours: the first not told yet.
	for (const TalkContext::Item& I : C.News)
	{
		if (T.Line.empty() && !Told(C, I.Key) && !I.Key.empty())
		{
			T.Line = I.Line;
			T.Topic = I.Topic;
			T.SaidKey = I.Key;
		}
	}
	// 5. The hour and the weather.
	if (T.Line.empty())
	{
		std::string Topic;
		T.Line = Greeting(C, Topic);
		T.Topic = Topic;
	}
}
} // namespace talk_detail

using namespace talk_detail;

std::string SpokenDollars(long long Dollars)
{
	const long long D = Dollars < 0 ? -Dollars : Dollars;
	std::string Out;
	if (D < 100)
	{
		Out = Under100(D);
	}
	else if (D < 1000)
	{
		Out = D % 100 == 0 ? std::string(Ones[D / 100]) + " hundred" : std::string(Ones[D / 100]) + Tail(D % 100);
	}
	else if (D < 10000)
	{
		if (D % 1000 == 0)
		{
			Out = std::string(Ones[D / 1000]) + " thousand";
		}
		else if (D % 100 == 0)
		{
			Out = Under100(D / 100) + " hundred";
		}
		else if (D / 100 % 10 == 0)
		{
			Out = std::string(Ones[D / 1000]) + " thousand " + Under100(D % 100); // 2,047: two thousand forty-seven
		}
		else
		{
			Out = Under100(D / 100) + Tail(D % 100);
		}
	}
	else
	{
		const long long K = D / 1000;
		const long long R = D % 1000;
		Out = (K < 100 ? Under100(K) : SpokenDollars(K)) + " thousand" + (R == 0 ? std::string() : " " + SpokenDollars(R));
	}
	return Dollars < 0 ? "minus " + Out : Out;
}

std::shared_ptr<const TalkContext> ContextFor(const Session& S, const std::string& Speaker, const std::string& Venue)
{
	auto C = std::make_shared<TalkContext>();
	const Person* P = FindPerson(Speaker);
	const bool Local = Speaker.rfind("npc:", 0) == 0;
	C->Speaker = Speaker;
	C->Who = P;
	C->Name = P ? P->Name : Local ? Speaker.substr(4) : Speaker;
	C->Role = P ? P->Role : Local ? Job::Local : Job::Pedestrian;
	C->Venue = Venue.empty() && P ? P->Venue : Venue;
	C->InWorld = (P && P->InWorld) || Local;
	const world::World& W = S.Living();
	if (C->InWorld && W.Ready())
	{
		C->Npc = W.Find(C->Name);
	}
	// The clock and the sky.
	C->World = S.WorldMinutes();
	C->Day = net::DayOf(C->World);
	C->Weekday = net::Weekday(C->Day);
	C->Hour = (C->World - static_cast<double>(C->Day) * 1440.0) / 60.0;
	const uint64_t Seed = S.WorldSeed();
	const weather::Now Sky = weather::At(C->World, Seed);
	C->Sky = Sky.State;
	C->Raining = Sky.Rain >= 0.25;
	C->Soaked = C->Raining || weather::At(C->World - 15.0, Seed).Rain >= 0.25;
	// The player.
	C->Screen = S.HeroName;
	C->First = S.Person.FirstName;
	C->Story = S.Person.Story;
	C->Created = S.Person.Created;
	C->Hunger = S.Life.Hunger;
	C->Thirst = S.Life.Thirst;
	C->Energy = S.Life.Energy;
	C->Fitness = S.Life.Fit.Level;
	C->Evicted = S.Evicted();
	C->EvictedDay = net::DayOf(S.Life.EvictedAt);
	// A cut of Lonnie's in the last three days (body::Sharp's rule: the cut a career starts with, from before the city,
	// never reads as fresh).
	C->FreshCut = S.Life.Groom.Cuts > 0 && S.Life.Groom.CutAt > C->World - 3.0 * 1440.0 && S.Life.Groom.CutAt <= C->World;
	C->CutDay = net::DayOf(std::max(0.0, S.Life.Groom.CutAt));
	C->Famous = Renown(S) >= 0.55;
	C->Member = S.Owns("gym");
	// The bond.
	const auto Bonded = S.Life.Town.find(Speaker);
	if (Bonded != S.Life.Town.end())
	{
		C->Town = Bonded->second;
	}
	C->Met = C->Town.Familiarity > 0;
	const uint32_t Look = LookHash(S.Person.Appearance, S.Life.Closet);
	C->NewLook = C->Met && C->Town.LookSeen != 0 && C->Town.LookSeen != Look && !C->FreshCut;
	C->DaysAway = C->Met && C->Town.LastDay >= 0 ? C->Day - C->Town.LastDay : 0;
	C->Salt = talk_detail::Fnv(Speaker) ^ static_cast<uint32_t>(static_cast<int64_t>(std::floor(C->World / 15.0)) & 0x7fffffff);
	if (C->Npc >= 0)
	{
		const world::Bond* B = W.BondWith(C->Npc);
		C->HasMemory = B && !B->Memories.empty();
		C->Met = C->Met || C->HasMemory;
		C->Remembered = C->HasMemory ? W.SpokenGreeting(C->Npc, C->Salt) : std::string();
	}
	C->Label = Tag(Speaker, S);
	if (C->Label.empty() && C->InWorld)
	{
		C->Label = Local ? "Embercrest regular" : C->Role == Job::Regular ? "Dee's regular" : "Regular at Ruby's";
	}
	// The venue.
	const shop::Venue* V = shop::FindVenue(C->Venue);
	if (V)
	{
		C->Open = shop::IsOpen(*V, C->World);
		C->Staffed = shop::IsStaffed(*V, C->World);
		// When it opens next: now, if it's shut; once this stretch is over, if it's open (the closing line's "bring it back
		// tomorrow", Rosa's "till four"). Round the clock, there's no next.
		const double Closes = C->Open ? shop::ClosesAt(*V, C->World) : C->World;
		const bool AllHours = C->Open && Closes >= C->World + 7.0 * 1440.0;
		const double Opens = C->Open ? (AllHours ? C->World : shop::NextOpen(*V, Closes + 0.5)) : shop::NextOpen(*V, C->World);
		const int OpenDay = net::DayOf(Opens);
		C->Vars["Next"] = OpenDay == C->Day ? "Back at " + SpokenTime(Opens) : OpenDay == C->Day + 1 ? std::string("Tomorrow") : std::string(net::WeekdayName(OpenDay, true));
		C->Vars["next"] = OpenDay == C->Day ? "at " + SpokenTime(Opens) : OpenDay == C->Day + 1 ? std::string("tomorrow") : std::string(net::WeekdayName(OpenDay, true));
		C->Vars["opens"] = SpokenTime(Opens);
		if (C->Open && !AllHours)
		{
			C->Vars["closes"] = SpokenTime(Closes);
		}
		C->AtWork = P && !P->InWorld && P->Venue == C->Venue && C->Staffed && OnShift(C->Venue, C->World) == Speaker;
	}
	if (C->Venue == "barber")
	{
		C->Queue = BarberQueue(C->World);
		if (C->Queue > 0)
		{
			C->Vars["queue"] = Under100(C->Queue);
		}
	}
	C->Tickets = static_cast<int>(S.Life.Pawn.Tickets.size());
	double Due = 1.0e18;
	for (const shop::Ticket& Tk : S.Life.Pawn.Tickets)
	{
		Due = std::min(Due, Tk.DueAt);
	}
	if (C->Tickets > 0 && Due > C->World)
	{
		const int DueDay = net::DayOf(Due);
		C->Vars["due"] = DueDay == C->Day ? std::string("today") : DueDay == C->Day + 1 ? std::string("tomorrow") : std::string(net::WeekdayName(DueDay, true));
	}
	// Unit 114's pieces as they really came in (the pawn book's auction lots on his shelf), not a stock pair.
	C->Vars["lot114"] = shop::AuctionWords(S.Life.Pawn, C->World);
	C->Vars["screen"] = S.HeroName;
	C->Vars["first"] = S.Person.FirstName;
	C->Vars["weekday"] = net::WeekdayName(C->Day, true);
	// Twitch's twenty: lent and not back; he pays it from his next cash. Session::Lend keeps what he'd won in his career
	// when he borrowed it ("loan:twitch:won:<cents>"): anything he's won since (a tournament cash, a good night at Dee's)
	// and he has it.
	if (Speaker == "twitch")
	{
		const auto Loan = S.Life.Said.find("loan:twitch");
		const world::Npc* Twitch = W.Get(C->Npc);
		C->Owes = Loan != S.Life.Said.end() && SaidFresh(Loan->second, C->Day); // three months on, it's never coming
		if (C->Owes && Twitch)
		{
			const std::string Mark = "loan:twitch:won:";
			const auto Then = S.Life.Said.lower_bound(Mark);
			if (Then != S.Life.Said.end() && Then->first.compare(0, Mark.size(), Mark) == 0)
			{
				C->Repays = Twitch->CareerWon() > static_cast<Chips>(std::strtoll(Then->first.c_str() + Mark.size(), nullptr, 10));
			}
			else
			{
				const int Since = std::min(Loan->second, C->Day);
				for (const world::Finish& F : Twitch->Recent)
				{
					C->Repays = C->Repays || (F.Day > Since && F.Prize > 0);
				}
			}
		}
		const bool Broke = Twitch && (Twitch->St == world::Status::Broke || Twitch->Mood == world::Momentum::Downswing || Twitch->Mood == world::Momentum::MajorDownswing);
		C->CanAsk = !C->Owes && Broke;
	}
	C->PaysCheck = PaysYourCheck(S, C->World) == Speaker;
	if (C->PaysCheck && Speaker == "big-lou")
	{
		// His offer, in person: the same words as his text, the day said as it is tonight ("tomorrow night", not the
		// "on Friday" of when he texted).
		for (const stake::Offer& O : S.Life.Stakes.Offers)
		{
			if (O.Backer != "Big Lou" || O.K != stake::Kind::Event || O.State != stake::OfferStanding || O.Expires <= C->World)
			{
				continue;
			}
			const live::Occurrence Ev = live::FindOccurrence(O.EntryId);
			if (Ev.Valid())
			{
				C->LouStake = stake::Speak("Big Lou", "offer", Fnv1a(O.Id),
					{{"Short", Ev.T->Short}, {"Event", Ev.T->Name}, {"Amount", Money(Ev.T->BuyInCents)}, {"Day", live::DayWord(Ev, C->World)}});
				break;
			}
		}
	}
	// Sponsors (next_segment_spec §3): the offer and the deal of the brand this person keeps shop for, an appearance window
	// open with them, the patches that show on the player, and the newest deal (three weeks of word on the block).
	{
		const sponsor::Book& Book = S.Life.Sponsors;
		for (const sponsor::Brand& Br : sponsor::Brands())
		{
			if (Br.Keeper.empty() || Br.Keeper != Speaker)
			{
				continue;
			}
			C->SponsorBrand = Br.Id;
			C->SponsorOffer = C->AtWork ? sponsor::OpenOfferFor(Book, Br.Id, C->World) : std::string();
			C->SponsorSignable = !C->SponsorOffer.empty() && sponsor::CanSign(Book, S.Life, C->SponsorOffer, C->World).empty();
			C->SponsorActive = sponsor::Active(Book, Br.Id, C->World);
			if (C->AtWork && C->SponsorActive)
			{
				for (const sponsor::Window& W2 : sponsor::Appearances(Book, C->World, C->World + 1.0))
				{
					if (W2.Venue == Br.Venue && !W2.Done && C->World >= W2.From && C->World < W2.To)
					{
						C->SponsorAppear = W2.Venue;
					}
				}
			}
		}
		for (int I = 0; I < sponsor::PatchSpots; ++I)
		{
			const std::string& Patch = S.Life.Closet.Patches[static_cast<size_t>(I)];
			if (!Patch.empty() && sponsor::Active(Book, Patch, C->World) && wardrobe::PatchFits(S.Life.Closet, I) &&
				std::find(C->Patches.begin(), C->Patches.end(), Patch) == C->Patches.end())
			{
				C->Patches.push_back(Patch);
			}
		}
		double Newest = -1.0;
		for (const sponsor::Contract& K : Book.Contracts)
		{
			const sponsor::Brand* Br = sponsor::FindBrand(K.Brand);
			if (Br && sponsor::Active(Book, K.Brand, C->World) && K.Renews.empty() && !K.Legacy && K.Since <= C->World && C->World - K.Since < 21.0 * 1440.0 && K.Since > Newest)
			{
				Newest = K.Since;
				C->NewSponsor = Br->Short;
				C->NewSponsorKey = "rumour:sponsor-you:" + K.Id;
			}
		}
	}
	for (const auto& It : S.Life.Said)
	{
		if (SaidFresh(It.second, C->Day))
		{
			C->Told.insert(It.first); // more than 90 days ago is forgotten, as the save forgets it
		}
	}
	Gather(S, *C);
	return C;
}

Talk Open(const TalkContext& C)
{
	Talk T;
	T.Name = C.Name;
	T.Label = C.Label;
	T.Familiarity = C.Town.LastDay == C.Day ? 1 : 2;
	if (C.Role == Job::Cook)
	{
		// Behind the pass with eggs going: a word, not a conversation.
		T.Line = Say(C, "greet");
		T.Topic = "hour";
		T.Choices = Choices(C);
		return T;
	}
	if (C.PaysCheck)
	{
		T.Line = Say(C, "check");
		T.Topic = "bond:check";
		if (!C.LouStake.empty())
		{
			T.Line += " " + C.LouStake;
		}
	}
	if (T.Line.empty())
	{
		Opener(C, T, false);
	}
	T.Choices = Choices(C);
	return T;
}

Talk Answer(const TalkContext& C, const std::string& ChoiceId)
{
	Talk T;
	T.Name = C.Name;
	T.Label = C.Label;
	T.Topic = ChoiceId;
	auto First = [&](const std::vector<TalkContext::Item>& From) {
		for (const TalkContext::Item& I : From)
		{
			if (!I.Key.empty() && !Told(C, I.Key))
			{
				T.Line = I.Line;
				T.SaidKey = I.Key;
				T.Topic = I.Topic;
				return;
			}
		}
		for (const TalkContext::Item& I : From)
		{
			if (I.Key.empty())
			{
				T.Line = I.Line;
				T.Topic = I.Topic;
				return;
			}
		}
	};
	if (ChoiceId == "news")
	{
		First(C.News);
		if (T.Line.empty())
		{
			T.Line = Say(C, "news:none");
			T.Topic = "news";
		}
		T.Choices = Choices(C);
	}
	else if (ChoiceId == "games")
	{
		First(C.Games);
		T.Choices = Choices(C);
	}
	else if (ChoiceId == "service")
	{
		if (!C.AtWork)
		{
			return Talk();
		}
		T.Line = C.Queue > 0 ? Say(C, "service:wait") : std::string();
		if (T.Line.empty() && C.Member && C.Venue == "gym")
		{
			T.Line = Say(C, "service:member");
		}
		if (T.Line.empty())
		{
			T.Line = Say(C, "service");
		}
	}
	else if (ChoiceId == "lend" && C.CanAsk)
	{
		T.Line = Say(C, "lend");
		T.Choices = {{"lend-yes", "Here's twenty."}, {"lend-no", "Not tonight."}};
	}
	else if ((ChoiceId == "lend-yes" || ChoiceId == "lend-no") && C.CanAsk)
	{
		T.Line = Say(C, ChoiceId);
		T.Familiarity = ChoiceId == "lend-yes" ? 2 : 0;
		T.Choices = {{"news", "What's new?"}, {"games", "Any games around?"}, {"bye", "See you."}};
	}
	else if (ChoiceId == "sponsor" && !C.SponsorOffer.empty())
	{
		// The terms across the counter; Heard puts them in writing in Mail ("sponsor:heard:<contract>").
		T.Line = Say(C, "sponsor:terms:" + C.SponsorBrand);
		T.Topic = "sponsor:" + C.SponsorBrand;
		T.SaidKey = "sponsor:heard:" + C.SponsorOffer;
		if (C.SponsorSignable)
		{
			T.Choices.push_back({"sponsor-sign", "Deal."});
		}
		T.Choices.push_back({"sponsor-later", "Let me think on it."});
	}
	else if (ChoiceId == "sponsor-sign" && C.SponsorSignable)
	{
		// Signed on the counter slip: Heard signs it ("sponsor:sign:<contract>").
		T.Line = Say(C, "sponsor:signed:" + C.SponsorBrand);
		T.Topic = "sponsor:" + C.SponsorBrand;
		T.SaidKey = "sponsor:sign:" + C.SponsorOffer;
		T.Familiarity = 2;
		T.Choices = {{"news", "What's new?"}, {"bye", "See you."}};
	}
	else if (ChoiceId == "sponsor-later" && !C.SponsorOffer.empty())
	{
		T.Line = Say(C, "sponsor:later");
		T.Topic = "sponsor:" + C.SponsorBrand;
		T.Choices = Choices(C);
	}
	else if (ChoiceId == "sponsor-appear" && !C.SponsorAppear.empty())
	{
		// The photo with Ray, Lonnie's window chair: Heard counts the appearance ("sponsor:appear:<venue>:<day>").
		T.Line = Say(C, "sponsor:appear:" + C.SponsorAppear);
		T.Topic = "sponsor:" + C.SponsorBrand;
		T.SaidKey = "sponsor:appear:" + C.SponsorAppear + ":" + std::to_string(C.Day);
		T.Choices = {{"news", "What's new?"}, {"bye", "See you."}};
	}
	else if (ChoiceId == "bye")
	{
		T.Line = Say(C, "bye");
	}
	else
	{
		return Talk();
	}
	return T;
}

std::string KeeperLine(const TalkContext& C, const std::string& Moment)
{
	const size_t Colon = Moment.find(':');
	const std::string Head = Moment.substr(0, Colon);
	if (Head == "greet")
	{
		// Their bubble as the screen opens: how the player looks and what they know first (nothing one-time: a bubble is
		// said every time the page opens), then the keeper's own greeting.
		Talk T;
		Opener(C, T, true);
		if (!T.Line.empty())
		{
			return T.Line;
		}
		if (C.Speaker == "bea" && C.Weekday == 5)
		{
			const std::string Sat = Own(C, "k:greet:saturday");
			if (!Sat.empty())
			{
				return Sat;
			}
		}
		const std::string Line = Say(C, "k:greet");
		return Line.empty() ? Say(C, std::string("greet:") + Band(C.Hour)) : Line;
	}
	if (Head == "offer" && Colon != std::string::npos)
	{
		// offer:<sell cents>:<loan cents>: the numbers as he says them.
		const std::string Rest = Moment.substr(Colon + 1);
		const size_t Split = Rest.find(':');
		if (Split != std::string::npos)
		{
			const long long Sell = std::strtoll(Rest.substr(0, Split).c_str(), nullptr, 10) / 100;
			const long long Loan = std::strtoll(Rest.substr(Split + 1).c_str(), nullptr, 10) / 100;
			const std::string Line = Say(C, "k:offer:numbers", {{"Sell", Capital(SpokenDollars(Sell))}, {"Loan", Capital(SpokenDollars(Loan))}});
			if (!Line.empty())
			{
				return Line;
			}
		}
		return Say(C, "k:offer");
	}
	if (Colon != std::string::npos)
	{
		const std::string Exact = Say(C, "k:" + Moment);
		if (!Exact.empty())
		{
			return Exact;
		}
	}
	return Say(C, "k:" + Head);
}

std::string Bark(const TalkContext& C, const std::string& Trigger)
{
	if (Trigger == "bump")
	{
		const bool Knows = C.Met || !C.Label.empty();
		const std::string Own_ = Own(C, "b:bump");
		if (!Own_.empty())
		{
			return Own_;
		}
		return Say(C, Knows && C.InWorld ? "b:bump-known" : "b:bump");
	}
	if (Trigger == "recognize")
	{
		return C.Met ? std::string() : Say(C, "b:recognize");
	}
	if (Trigger == "rain")
	{
		return Say(C, "b:rain");
	}
	if (Trigger == "closing" || Trigger == "door")
	{
		return C.Open && C.Staffed ? std::string() : Say(C, "b:closing");
	}
	if (Trigger == "hello")
	{
		const std::string Own_ = Own(C, "b:hello");
		return Own_.empty() ? Say(C, std::string("greet:") + Band(C.Hour)) : Own_;
	}
	if (Trigger == "excuse")
	{
		return Say(C, "b:excuse");
	}
	return "";
}
} // namespace town
} // namespace ss
