#include "ShortStack/Game/Stake.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// Backing (next_segment_spec §2). core-stake.
//
// Every movement is keyed and recorded in Book::Done, so asking twice moves nothing twice (N2). The player's ledger gets
// one line per counterparty paid (kind 10, Backing), never a line per fill; a package's money moves in its own ledger
// (Deal::Lines), and its roll never touches the bankroll except at a chop. The copy is the backers' own voices: Lou
// is sentimental, Mrs. Park pays to the cent, Sal is a nit, Mei is quick, Rick is good for it Friday, Dee doesn't back
// people she likes, Lantern Row goes by the spreadsheet, and Sam texts at three in the morning.

namespace ss
{
namespace stake
{
namespace stake_detail
{
constexpr double Day = 1440.0;
constexpr double Hour = 60.0;
using Vars = std::map<std::string, std::string>;

const char* const Lou = "Big Lou";
const char* const Park = "Mrs. Park";
const char* const Sal = "Sal";
const char* const Dre = "Dre";
const char* const Mei = "Mei";
const char* const Rick = "Rick";
const char* const Dee = "Dee";
const char* const LanternRow = "Lantern Row Staking";
const char* const SamName = "Sam";
const char* const Finch = "Finch School";
const char* const MarketName = "Online buyers";

// ------------------------------------------------------------------ text and numbers

std::string Esc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
	for (const char Ch : In)
	{
		if (Ch == '\\')
		{
			Out += "\\\\";
		}
		else if (Ch == '\n')
		{
			Out += "\\n";
		}
		else if (Ch == '\t')
		{
			Out += "\\t";
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::string Unesc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

std::string F2(double V)
{
	return Fixed(V, 2);
}

double Num(const std::string& S)
{
	return std::strtod(S.c_str(), nullptr);
}

Chips Cents(const std::string& S)
{
	return static_cast<Chips>(std::strtoll(S.c_str(), nullptr, 10));
}

int Int(const std::string& S)
{
	return std::atoi(S.c_str());
}

std::string I2(long long V)
{
	return std::to_string(V);
}

/** "10%", "2.5%". */
std::string PctText(int Permille)
{
	return Permille % 10 == 0 ? std::to_string(Permille / 10) + "%" : std::to_string(Permille / 10) + "." + std::to_string(Permille % 10) + "%";
}

/** "1.05". */
std::string MarkupText(int Permille)
{
	const int Hundredths = (Permille + 5) / 10;
	const std::string Frac = std::to_string(Hundredths % 100);
	return std::to_string(Hundredths / 100) + "." + (Frac.size() < 2 ? "0" + Frac : Frac);
}

/** "+44%", "-4.8%": whole percents from 5% up, one decimal under it. */
std::string RoiText(double Roi)
{
	const double P = Roi * 100.0;
	const double A = P < 0.0 ? -P : P;
	std::string Body;
	if (A >= 5.0)
	{
		Body = std::to_string(static_cast<long long>(A + 0.5));
	}
	else
	{
		const long long Tenths = static_cast<long long>(A * 10.0 + 0.5);
		Body = std::to_string(Tenths / 10) + "." + std::to_string(Tenths % 10);
	}
	return (P < 0.0 && Body != "0.0" ? "-" : "+") + Body + "%";
}

/** An event's name without its price: "Embercrest Nightly $120" -> "Embercrest Nightly". */
std::string Title(const std::string& Name)
{
	const size_t At = Name.rfind(" $");
	return At == std::string::npos ? Name : Name.substr(0, At);
}

bool Online(const std::string& EventId, net::EventInstance& Out)
{
	return !live::FindOccurrence(EventId).Valid() && net::Shared().FindInstance(EventId, Out);
}

/** The event's name, its short name, its buy-in and when late registration closes (live or online). */
struct EventFacts
{
	bool Valid = false;
	bool Live = false;
	std::string Name;
	std::string Short;
	Chips BuyIn = 0;
	double Start = 0.0;
	double Closes = 0.0;
	std::string Key; // the live template's key ("nightly")
};

EventFacts FactsOf(const std::string& EventId)
{
	EventFacts F;
	const live::Occurrence O = live::FindOccurrence(EventId);
	if (O.Valid())
	{
		F.Valid = true;
		F.Live = true;
		F.Name = O.T->Name;
		F.Short = O.T->Short;
		F.BuyIn = O.T->BuyInCents;
		F.Start = O.Start;
		F.Closes = O.LateRegEnds;
		F.Key = O.T->Key;
		return F;
	}
	net::EventInstance I;
	if (Online(EventId, I))
	{
		const net::EventTemplate& T = net::Shared().TemplateOf(I);
		F.Valid = true;
		F.Name = T.Name;
		F.Short = T.Name;
		F.BuyIn = T.BuyInCents;
		F.Start = I.Start;
		F.Closes = I.Start + I.LateReg;
	}
	return F;
}

uint32_t Hash(const std::string& S)
{
	return Fnv1a(S);
}

Rng SeedRng(const std::string& Id, int OnDay)
{
	return Rng("stake:" + Id + ":" + std::to_string(OnDay));
}

double HourOf(double World)
{
	const double InDay = World - static_cast<double>(net::DayOf(World)) * Day;
	return InDay / Hour;
}

/** People text in the daytime (Sam at three in the morning). */
bool Waking(double World)
{
	const double H = HourOf(World);
	return H >= 10.0 && H < 22.0;
}

bool SmallHours(double World)
{
	const double H = HourOf(World);
	return H >= 0.5 && H < 4.5;
}

/** The people in the world (World::HeroMoney finds them by name); Dee is the town's, the outfits and the market aren't anyone. */
bool WorldPerson(const std::string& Who)
{
	return !Who.empty() && Who != Dee && Who != LanternRow && Who != SamName && Who != Finch && Who != MarketName;
}

std::string Display(const std::string& Who)
{
	return Who.empty() ? std::string(MarketName) : Who;
}

std::string Fill(const std::string& Template, const Vars& V)
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
		const auto It = V.find(Template.substr(Open + 1, Close - Open - 1));
		Out += It != V.end() ? It->second : Template.substr(Open, Close - Open + 1);
		I = Close + 1;
	}
	return Out;
}

// ------------------------------------------------------------------ the copy

struct Copy
{
	const char* Who;
	const char* Kind;
	const char* Text;
};

// Texts (at most 200 characters), rail lines and break texts (at most 84), in each backer's own voice. Never a number
// of chips or a card at the rail: a railbird sees the stack the way the room does, and the director says that part.
const Copy Lines[] = {
	// Big Lou: sentimental, sweats from the rail, covers your check at Ruby's.
	{"Big Lou", "offer", "You're playing the {Short} {Day}. I'm putting you in. My buy-in comes back first, then we chop it down the middle."},
	{"Big Lou", "offer", "The {Short}, {Day}. I'm putting you in, kid. My {Amount} comes back first, then it's fifty-fifty. Nothing carried over."},
	{"Big Lou", "taken", "That's my horse. Get some sleep, and eat something with a vegetable in it."},
	{"Big Lou", "taken", "Good. Don't make me regret it. You won't."},
	{"Big Lou", "weekly", "One a week, kid. I've got a mortgage too."},
	{"Big Lou", "paused", "Let's give it a month, kid. Nothing personal. The numbers are the numbers."},
	{"Big Lou", "unpaused", "Saw the cash. Forget the month. We're back in business, kid."},
	{"Big Lou", "lapse", "Missed you at the {Short}. No harm done. The offer comes back around."},
	{"Big Lou", "result.cash", "{Amount} says you can play. Same time next week?"},
	{"Big Lou", "result.cash", "Got my buy-in back with change, and {Amount} for you. That's how it's supposed to go."},
	{"Big Lou", "result.big", "You see that? I told Sal. I told the whole counter. {Amount} is yours, kid."},
	{"Big Lou", "result.bust", "That's poker. My money, my problem. Go eat something."},
	{"Big Lou", "result.bust", "Bricked. Happens to everybody. Ruby's tomorrow, I'm buying."},
	{"Big Lou", "piece.buy", "Put me down for {Pct} of your {Short}. Don't make me sorry."},
	{"Big Lou", "piece.owed", "Saw the {Place}. My piece comes to {Amount}. Pay me when you pay me."},
	{"Big Lou", "piece.bust", "My piece went down with you. That's what a piece is for."},
	{"Big Lou", "reminder", "Not chasing you, kid. Just the {Amount} from the {Short}, when you're flush."},
	{"Big Lou", "late", "Fourteen days was the deal, kid. {Amount}. I'm not mad yet."},
	{"Big Lou", "welch", "I covered your check at Ruby's. Remember that. {Amount}, and we're square."},
	{"Big Lou", "paid", "Got it. You're good people."},
	{"Big Lou", "paid.early", "Already? Look at you. Next coffee's on me."},
	{"Big Lou", "counter.yes", "Fine. You drive a hard bargain for a kid without a car."},
	{"Big Lou", "counter.stands", "Take it or leave it, kid."},
	{"Big Lou", "counter.no", "Forget it. Ask me next week."},
	{"Big Lou", "rail.bubble", "Fold your way in, kid. I didn't come up here for the view."},
	{"Big Lou", "rail.money", "That's the money. Now go get the rest of it."},
	{"Big Lou", "rail.doubleup", "There it is! Stack 'em up, kid!"},
	{"Big Lou", "rail.doubleup", "Look at that! Don't give it back, kid."},
	{"Big Lou", "rail.doubleup", "Now that's a stack. Keep it, kid."},
	{"Big Lou", "rail.final", "Final table! I'm calling Sal. I'm calling everybody."},
	{"Big Lou", "rail.bust", "Hey. Head up. Ruby's opens at six."},
	{"Big Lou", "rail.win", "That's my horse! That's my horse!"},
	{"Big Lou", "rail.beat", "Unbelievable. Shake it off, kid. Shake it off."},
	{"Big Lou", "rail.beat", "That river's a crook. Shake it off, kid."},
	{"Big Lou", "rail.beat", "Nothing you could do about that one, kid."},
	{"Big Lou", "rail.idle", "Don't look at me. Look at the cards."},
	{"Big Lou", "rail.idle", "Easy, kid. One hand at a time."},
	{"Big Lou", "break", "how many chips. dont round up."},
	{"Big Lou", "break", "break. eat something. you look like a ghost."},
	// Mrs. Park: to the cent, never 100%, one late payment and she stops for 90 days.
	{"Mrs. Park", "piece.buy", "I'll take {Pct} of your {Short} at your price. Play it patiently."},
	{"Mrs. Park", "piece.pass", "Your price is a little rich for me this time. Perhaps when the sample's bigger."},
	{"Mrs. Park", "piece.owed", "Congratulations on {Place}. My {Pct} is {Amount}. {Day} is fine."},
	{"Mrs. Park", "piece.bust", "Not our night. I'll be at the noon game Tuesday if you'd like to talk about it."},
	{"Mrs. Park", "reminder", "A reminder that {Amount} is due tomorrow. I keep my own books."},
	{"Mrs. Park", "late", "The {Amount} was due yesterday. I don't ask twice."},
	{"Mrs. Park", "welch", "I won't be buying your action again. {Amount} is still owed."},
	{"Mrs. Park", "paid", "Received. Thank you."},
	{"Mrs. Park", "paid.early", "Received, to the cent. Thank you. That's how it's done."},
	{"Mrs. Park", "paused", "Not this month. You know why."},
	{"Mrs. Park", "rail.bubble", "Patience. Nobody is paying you to be brave."},
	{"Mrs. Park", "rail.money", "Good. Now the real tournament starts."},
	{"Mrs. Park", "rail.doubleup", "Well played. Now breathe."},
	{"Mrs. Park", "rail.doubleup", "Good. Now protect it."},
	{"Mrs. Park", "rail.doubleup", "Nicely played. Don't celebrate yet."},
	{"Mrs. Park", "rail.final", "Sit up straight. They read slouching."},
	{"Mrs. Park", "rail.bust", "It happens. Write the hand down tonight."},
	{"Mrs. Park", "rail.win", "Champion. I'll expect you at the noon game."},
	{"Mrs. Park", "rail.beat", "That's the game. Next hand."},
	{"Mrs. Park", "rail.beat", "Unlucky. Play the next one the same way."},
	{"Mrs. Park", "rail.beat", "No mistake there. Keep your focus."},
	{"Mrs. Park", "rail.idle", "I'm watching. That's all."},
	{"Mrs. Park", "rail.idle", "Steady. No hurry."},
	{"Mrs. Park", "break", "How many chips? Exactly, please."},
	{"Mrs. Park", "break", "Your count, please. To the chip."},
	// Sal: a nit. Five percent at par, or nothing.
	{"Sal", "piece.buy", "Five at par. Not a penny of markup, kid."},
	{"Sal", "piece.pass", "Markup? I'll buy at par or I'll buy nothing, kid."},
	{"Sal", "piece.owed", "My five percent. {Amount}. Cash is fine."},
	{"Sal", "piece.bust", "Five percent of nothing. I've lost more on coffee."},
	{"Sal", "reminder", "Still got my {Amount}? Just asking."},
	{"Sal", "late", "You're late, kid. I noticed. I notice everything."},
	{"Sal", "welch", "That's the last nickel you get from me."},
	{"Sal", "paid", "Square."},
	{"Sal", "paid.early", "Early. Huh. Square, kid."},
	{"Sal", "rail.bubble", "Fold. Fold. Then fold some more."},
	{"Sal", "rail.money", "A min-cash is a cash. Now tighten up."},
	{"Sal", "rail.doubleup", "Don't get cute with it."},
	{"Sal", "rail.doubleup", "Good. Now fold for an hour."},
	{"Sal", "rail.doubleup", "Okay. Don't give it back, kid."},
	{"Sal", "rail.final", "Nine-high is not a hand, kid. Remember that."},
	{"Sal", "rail.bust", "Told you. Didn't I tell you?"},
	{"Sal", "rail.win", "Huh. Okay. Okay, kid."},
	{"Sal", "rail.beat", "That's why I fold."},
	{"Sal", "rail.beat", "Cards don't care. Fold more."},
	{"Sal", "rail.beat", "See? Every time."},
	{"Sal", "rail.idle", "Fold more."},
	{"Sal", "rail.idle", "Tighter, kid."},
	{"Sal", "break", "chips?"},
	{"Sal", "break", "how many. exact."},
	// Dre: small roll, big heart.
	{"Dre", "piece.buy", "Got five on you for the {Short}. Run it up."},
	{"Dre", "piece.owed", "My five came in! {Amount}. Whenever, bro."},
	{"Dre", "piece.bust", "Five percent of a brick. All good, next one."},
	{"Dre", "reminder", "Yo, the {Amount} from the {Short}. No stress, just saying."},
	{"Dre", "late", "Bro. {Amount}. I got bills too."},
	{"Dre", "welch", "Thought we were cool. {Amount}. Guess not."},
	{"Dre", "paid", "Appreciate you."},
	{"Dre", "paid.early", "Already?! You're a legend."},
	{"Dre", "rail.bubble", "Don't do nothing crazy. Just get in."},
	{"Dre", "rail.money", "We in the money! Let's go!"},
	{"Dre", "rail.doubleup", "Let's go! Let's go!"},
	{"Dre", "rail.doubleup", "That's what I'm talking about!"},
	{"Dre", "rail.doubleup", "Double up! Run it up, bro!"},
	{"Dre", "rail.final", "Final table, baby! I'm filming this."},
	{"Dre", "rail.bust", "Unlucky, bro. You played it right."},
	{"Dre", "rail.win", "No way. No way! Champion!"},
	{"Dre", "rail.beat", "That river is a crime."},
	{"Dre", "rail.beat", "Nah, that's cold. Stay up, bro."},
	{"Dre", "rail.beat", "Robbed. Straight robbed. Next hand."},
	{"Dre", "rail.idle", "I'm right here, bro."},
	{"Dre", "rail.idle", "Let's get it, bro."},
	{"Dre", "break", "you still in?? how deep."},
	{"Dre", "break", "u good? how many chips."},
	// Mei: ambitious, quick, pays inside a day.
	{"Mei", "offer", "Swap {Pct} in the {Short}? I'm playing it either way."},
	{"Mei", "swap.yes", "Deal. {Pct} each. Don't make me regret it."},
	{"Mei", "swap.no", "Not this one. Make a final table with me first."},
	{"Mei", "swap.notplaying", "I'm not in that one."},
	{"Mei", "swap.owe", "You owe me {Amount} on the swap. No rush. Tomorrow's a rush."},
	{"Mei", "swap.owed", "My run beat yours. I mean yours beat mine. {Amount} coming tomorrow."},
	{"Mei", "swap.pays", "Sent {Amount}. We're square. Next time I win."},
	{"Mei", "swap.even", "Swap came out even. Boring. Next time."},
	{"Mei", "reminder", "Swap money. {Amount}. Today, please."},
	{"Mei", "late", "You're late on the swap. People talk, you know."},
	{"Mei", "welch", "I'm done swapping with you. {Amount}. You know where I sit."},
	{"Mei", "paid", "Got it. Even."},
	{"Mei", "paid.early", "Already? Okay, you're alright."},
	{"Mei", "rail.bubble", "Breathe. You're not as short as you feel."},
	{"Mei", "rail.money", "Money! Now play like it's yours."},
	{"Mei", "rail.doubleup", "Yes! That's how you do it."},
	{"Mei", "rail.doubleup", "Nice. Don't get greedy."},
	{"Mei", "rail.doubleup", "There it is. Keep the pressure on."},
	{"Mei", "rail.final", "Final table. I'll hate you a little if you win this."},
	{"Mei", "rail.bust", "Out on that? Okay. Next week."},
	{"Mei", "rail.win", "Fine. You win. Drinks are on you."},
	{"Mei", "rail.beat", "Disgusting river. Reset."},
	{"Mei", "rail.beat", "Gross. Forget it. Next hand."},
	{"Mei", "rail.beat", "Ugh. Shake it off. You played it fine."},
	{"Mei", "rail.idle", "Go. Go."},
	{"Mei", "rail.idle", "Focus."},
	{"Mei", "break", "how deep are you. real number."},
	{"Mei", "break", "chip count. no rounding up."},
	// Rick: turbos, risk, good for it Friday.
	{"Rick", "offer", "{Pct} swap in the {Short} tonight? I'm good for it, swear on my mother's car."},
	{"Rick", "swap.yes", "Done. This is the one, I can feel it."},
	{"Rick", "swap.no", "Nah, I'm feeling it tonight. Keeping all of me."},
	{"Rick", "swap.notplaying", "Not playing that one. Rent's due."},
	{"Rick", "swap.owe", "Your swap. {Amount}. I need it more than you know."},
	{"Rick", "swap.owed", "Good for it {Day}, swear on my mother's car."},
	{"Rick", "swap.pays", "Here. {Amount}. Told you. The car's safe."},
	{"Rick", "swap.even", "Swap's a wash. Story of my life."},
	{"Rick", "reminder", "Swap money, chief. {Amount}. Rent's got me by the throat."},
	{"Rick", "late", "Dude. {Amount}. You're making me look like the responsible one."},
	{"Rick", "welch", "Never thought you'd be the one who stiffed me. {Amount}."},
	{"Rick", "paid", "You're a real one."},
	{"Rick", "paid.early", "Already? Who raised you?"},
	{"Rick", "rail.bubble", "Shove it! No. Don't listen to me."},
	{"Rick", "rail.money", "Money! I had a parlay on you!"},
	{"Rick", "rail.doubleup", "Ship it! Ship it!"},
	{"Rick", "rail.doubleup", "Double up! The parlay lives!"},
	{"Rick", "rail.doubleup", "Yes! That's the spot! That's the spot!"},
	{"Rick", "rail.final", "Final table! I'm buying a lottery ticket."},
	{"Rick", "rail.bust", "Variance, man. Pure variance."},
	{"Rick", "rail.win", "My swap partner! My swap partner!"},
	{"Rick", "rail.beat", "Rigged. Totally rigged. Keep going."},
	{"Rick", "rail.beat", "Unreal. The deck hates us both."},
	{"Rick", "rail.beat", "Of course it's the river. Keep firing."},
	{"Rick", "rail.idle", "Come on, come on."},
	{"Rick", "rail.idle", "Let's go, let's go."},
	{"Rick", "break", "chips? i got a parlay riding on you lol."},
	{"Rick", "break", "still alive?? how many chips."},
	// Dee: deals the Sunday, doesn't back people she likes, buys the piece anyway.
	{"Dee", "offer", "Put me down for ten percent of your first Sunday. At par. For the lessons. I don't back people I like. Ruins both."},
	{"Dee", "taken", "Good. Now I have to watch."},
	{"Dee", "lapse", "You skipped the Sunday. My ten percent stays in my pocket."},
	{"Dee", "piece.owed", "My ten percent. {Amount}. Frame the first dollar."},
	{"Dee", "piece.bust", "Well. I bought a lesson. So did you."},
	{"Dee", "reminder", "The {Amount}. I'm not your landlord, so I'll ask nicely once."},
	{"Dee", "late", "I don't chase money. I'm asking anyway. {Amount}."},
	{"Dee", "welch", "Keep it. I meant what I said about backing people I like."},
	{"Dee", "paid", "Thank you. Now forget I'm in it."},
	{"Dee", "paid.early", "Paid before I asked. I'll pretend I'm not proud."},
	{"Dee", "rail.idle", "Don't look at me. I'm working."},
	{"Dee", "rail.idle", "Eyes on your own cards, kid."},
	{"Dee", "break", "Saw your table from the box. Sit still. You're fine."},
	{"Dee", "break", "Drink some water. Sit still. Back in five."},
	// Sam: the Burner. The fine print is spelled out before you sign.
	{"Sam", "offer", "lantern row got u on a leash? i do 100%. u keep 35. makeup's makeup. read the fine print lol."},
	{"Sam", "offer", "rent's coming. i do 100%. u keep 35. makeup's makeup. read the fine print lol."},
	{"Sam", "offer.terms", "fine print: quit while ur in makeup and its a debt. 25% of every cash and every shift til its paid. two sessions on my account a week or ur cut drops to 25."},
	{"Sam", "taken", "welcome to the family lol."},
	{"Sam", "chop", "periods up. ur cut's in. dont spend it all on rent."},
	{"Sam", "down", "makeup's {Amount}. keep firing. i dont do chops on request."},
	{"Sam", "nochop", "i dont do chops on request."},
	{"Sam", "walk", "u walked with {Amount} in makeup. thats a debt now. 25% of everything til its paid."},
	{"Sam", "garnish", "took my 25%. {Amount} left on the debt."},
	{"Sam", "clear", "debt's clear. pleasure doing business."},
	{"Sam", "ghosts", "need my two sessions this week. or ur cut drops."},
	{"Sam", "counter.yes", "fine. dont tell anybody i went soft."},
	{"Sam", "counter.stands", "take it or leave it."},
	{"Sam", "counter.no", "forget it. u know where to find me."},
	// Lantern Row's short notes (its letters are in the Mail app).
	{"Lantern Row Staking", "nochop", "We chop on the period. Up twenty buy-ins, we'll do it early."},
	{"Lantern Row Staking", "one", "One package at a time. Finish this one first."},
	{"Lantern Row Staking", "counter.yes", "Agreed. The sheet says you've earned it."},
	{"Lantern Row Staking", "counter.stands", "The offer stands as written for another day."},
	{"Lantern Row Staking", "counter.no", "We'll pass for now. Our door stays open in a week."},
	// Anyone else: the world's regulars who back the player, and a generic backer at the rail.
	{"*", "offer.package", "I've watched you play. I'll stake you online: a {Amount} roll, events up to {Cap}, fifty-fifty after makeup, sixty days. Makeup's forgiven at the end."},
	{"*", "offer.package.walked", "Heard you walked on somebody's makeup. I'll still stake you online: a {Amount} roll, events up to {Cap}, sixty-forty my way, sixty days."},
	{"*", "taken", "Good. Fire away, and play your game."},
	{"*", "chop", "Sixty days are up. Your half of the profit is in. Pleasure."},
	{"*", "end", "Sixty days are up. The makeup's mine to eat. No hard feelings."},
	{"*", "piece.owed", "My {Pct} comes to {Amount}. Whenever you're ready."},
	{"*", "swap.yes", "Sure. {Pct} each."},
	{"*", "swap.no", "I'll keep my own action tonight."},
	{"*", "swap.notplaying", "I'm not playing that one."},
	{"*", "swap.owe", "Swap came to {Amount} my way."},
	{"*", "swap.owed", "I owe you {Amount} on the swap. You'll have it in a few days."},
	{"*", "swap.pays", "Sent {Amount} for the swap. Good luck out there."},
	{"*", "swap.even", "Our swap washed out. Next time."},
	{"*", "reminder", "Just the {Amount}, when you can."},
	{"*", "late", "The {Amount} was due. I'd like it."},
	{"*", "welch", "I won't back you again. {Amount} is still owed."},
	{"*", "paid", "Thanks. Good doing business."},
	{"*", "paid.early", "Early. I'll remember that."},
	{"*", "counter.yes", "Alright. You've got a deal."},
	{"*", "counter.stands", "That's my offer."},
	{"*", "counter.no", "Never mind, then."},
	{"*", "rail.bubble", "Easy now. Just get in."},
	{"*", "rail.money", "That's a cash. Keep going."},
	{"*", "rail.doubleup", "Yes! Stack it."},
	{"*", "rail.doubleup", "Nice! Keep it going."},
	{"*", "rail.doubleup", "There you go! Stack it up."},
	{"*", "rail.final", "Final table. Take a breath."},
	{"*", "rail.bust", "Tough one. You'll get it next time."},
	{"*", "rail.win", "Champion! Unbelievable!"},
	{"*", "rail.beat", "Brutal. Keep your head."},
	{"*", "rail.beat", "Ouch. Shake it off."},
	{"*", "rail.beat", "Rough one. Next hand."},
	{"*", "rail.idle", "Come on."},
	{"*", "rail.idle", "You've got this."},
	{"*", "break", "how are you doing in there."},
	{"*", "break", "still in? how many chips."},
};

struct Letter
{
	const char* Kind;
	const char* Subject;
	const char* Body;
};

// Lantern Row Staking's letters (the Mail app's inbox, Source "stake"): by the spreadsheet, never unkind.
const Letter Letters[] = {
	{"offer", "A package from Lantern Row",
		"We track everyone with five hundred events and an edge, and you're one of them. We'd like to stake you: a {Amount} roll for events up to {Cap}, "
		"{Pct} of the profit to you after makeup, a chop every thirty days, and Fred's leak review with each chop. Makeup isn't a debt. Walk and you owe "
		"nothing, but we don't take back people who walk."},
	{"welcome", "Your roll is live",
		"The roll is funded. Every event up to {Cap} plays on it, the entry and two re-entries; a fourth bullet is your own money. No cherry-picking: "
		"everything in range goes on the roll. We chop on {Day}. Fred's review comes with it."},
	{"chop", "Chop: {Amount} to you",
		"The period's done and the roll finished up after makeup. Your {Pct} is in your account. The roll resets to its size. Good month."},
	{"nochop", "No chop this month",
		"The period's done. Makeup carries: {Amount}. That's the deal working the way it's built. Keep the volume up and keep firing."},
	{"volume", "Volume",
		"{Count} events this period against the {Volume} we agreed. Short twice and we end it."},
	{"tierup", "Your numbers moved",
		"Your sample's earned a step. Next period you play events up to {Cap} at {Pct} of the profit."},
	{"tierdown", "Stepping you down",
		"Two losing periods. We're moving you to events up to {Cap} at {Pct}. It isn't personal. It's the sheet."},
	{"cut.makeup", "Ending the package",
		"We're ending the package. The makeup is ours to eat. Good luck out there."},
	{"cut.roi", "Ending the package",
		"The numbers don't support it right now. Come back with a sample. The makeup is ours, and you owe us nothing."},
	{"cut.volume", "Ending the package",
		"Two periods short of the volume. We're ending it. The makeup is ours, and you owe us nothing."},
	{"closed", "Package closed",
		"You asked to close the package with the roll up. Your {Pct} of the profit, {Amount}, is in your account, and we're square. "
		"If you want back in, write us in a month."},
	{"walked", "You walked",
		"You walked away from {Amount} of makeup. You owe us nothing. We won't stake you for a year, and the people who stake will hear about it."},
	{"reminder", "Chop day is {Day}",
		"{Count} events so far against the {Volume}. The roll is at {Amount}."},
	{"counter", "Your counter",
		"{Line} Fred reads this inbox himself, so ask if anything's unclear."},
	{"result.big", "Saw the run",
		"Fred had your final table up on the second monitor. That's the roll working. Don't change a thing."},
};

std::vector<const Copy*> Variants(const std::string& Who, const std::string& Kind)
{
	std::vector<const Copy*> Out;
	for (const Copy& C : Lines)
	{
		if (Kind == C.Kind && Who == C.Who)
		{
			Out.push_back(&C);
		}
	}
	return Out;
}

std::string Say(const std::string& Who, const std::string& Kind, uint32_t Salt, const Vars& V = Vars())
{
	std::vector<const Copy*> Found = Variants(Who, Kind);
	if (Found.empty())
	{
		Found = Variants("*", Kind);
	}
	if (Found.empty())
	{
		return std::string();
	}
	return Fill(Found[Salt % Found.size()]->Text, V);
}

const Letter* FindLetter(const std::string& Kind)
{
	for (const Letter& L : Letters)
	{
		if (Kind == L.Kind)
		{
			return &L;
		}
	}
	return nullptr;
}

void Mail(life::State& L, double At, const std::string& Thread, const std::string& Kind, const Vars& V, int Sort)
{
	const Letter* Found = FindLetter(Kind);
	if (!Found)
	{
		return;
	}
	life::Letter Out;
	Out.At = At;
	Out.Thread = Thread;
	Out.Source = "stake";
	Out.From = LanternRow;
	Out.Subject = Fill(Found->Subject, V);
	Out.Body = Fill(Found->Body, V);
	Out.Kind = Sort;
	life::Post(L, Out);
}

// The letter kinds the book posts (LetterOffer to LetterNote) are stake::LetterKind in Stake.h, where the Mail app reads them.

// ------------------------------------------------------------------ the book

bool Once(Book& B, const std::string& Key, double At)
{
	if (B.Done.count(Key) > 0)
	{
		return false;
	}
	B.Done[Key] = At;
	return true;
}

bool Did(const Book& B, const std::string& Key)
{
	return B.Done.count(Key) > 0;
}

/** Money between the player and someone else: one ledger line (kind 10). */
void Move(Chips& Bankroll, life::State& L, double At, const std::string& Label, Chips Amount)
{
	if (Amount == 0)
	{
		return;
	}
	Bankroll += Amount;
	L.Record(At, Label, Amount, life::LedgerBacking);
}

/**
 * The same on a site's pot (update 2 §1.2): RiverLine's is the bankroll and the bank's line, as above; a rival's wallet
 * moves with no bank line (its own site line, through site::AddLine, is core-deals' at G3).
 */
void Move(const Pot& P, life::State& L, double At, const std::string& Label, Chips Amount)
{
	if (!P.Cents || Amount == 0)
	{
		return;
	}
	if (P.Site == 0)
	{
		Move(*P.Cents, L, At, Label, Amount);
		return;
	}
	*P.Cents += Amount;
}

void FlowTo(Book& B, const std::string& Id, const std::string& Who, const std::string& Where, Chips Cents, Memory M, double At)
{
	if (!WorldPerson(Who) || !Once(B, "flow:" + Id, At))
	{
		return;
	}
	Flow F;
	F.Id = Id;
	F.Npc = Who;
	F.Where = Where;
	F.Cents = Cents;
	F.Memory = static_cast<int>(M);
	F.At = At;
	B.ToWorld.push_back(F);
}

void DealLine(Deal& D, double At, Chips Amount, const std::string& Label)
{
	Line Ln;
	Ln.At = At;
	Ln.Amount = Amount;
	Ln.Label = Label;
	D.Lines.push_back(Ln);
	if (D.Lines.size() > 40)
	{
		D.Lines.erase(D.Lines.begin(), D.Lines.begin() + static_cast<std::ptrdiff_t>(D.Lines.size() - 40));
	}
}

Deal* FindDeal(Book& B, const std::string& Id)
{
	for (Deal& D : B.Deals)
	{
		if (D.Id == Id)
		{
			return &D;
		}
	}
	return nullptr;
}

const Deal* FindDeal(const Book& B, const std::string& Id)
{
	for (const Deal& D : B.Deals)
	{
		if (D.Id == Id)
		{
			return &D;
		}
	}
	return nullptr;
}

Deal* Package(Book& B)
{
	for (Deal& D : B.Deals)
	{
		if (D.K == Kind::Package && D.State == DealOpen)
		{
			return &D;
		}
	}
	return nullptr;
}

/**
 * The event stake an entry's funding names: the deal itself, or the deal its standing offer became when the player
 * registered on it (the desk's "Lou'll put you in": the entry keeps the offer's id, the deal is "deal:" + that id).
 */
Deal* BackerDeal(Book& B, const life::LiveEntry& E)
{
	for (const std::string& Id : {E.Fund.Ref, "deal:" + E.Fund.Ref})
	{
		Deal* D = FindDeal(B, Id);
		if (D && D->K == Kind::Event && D->EventId == E.Id)
		{
			return D;
		}
	}
	return nullptr;
}

/** An event stake for this occurrence, waiting or registered. */
const Deal* EventDeal(const Book& B, const std::string& EventId)
{
	for (const Deal& D : B.Deals)
	{
		if (D.K == Kind::Event && D.EventId == EventId && (D.State == DealOpen || D.State == DealPlaying))
		{
			return &D;
		}
	}
	return nullptr;
}

Offer* FindOffer(Book& B, const std::string& Id)
{
	for (Offer& O : B.Offers)
	{
		if (O.Id == Id)
		{
			return &O;
		}
	}
	return nullptr;
}

/** Lou's (or anyone's) event stakes since From that counted: accepted, not refunded or gone by. */
int StakesSince(const Book& B, const std::string& Backer, double From)
{
	int N = 0;
	for (const Deal& D : B.Deals)
	{
		N += D.K == Kind::Event && D.Backer == Backer && D.Since > From && D.State != DealRefunded && D.State != DealLapsed ? 1 : 0;
	}
	return N;
}

bool Paused(const Book& B, const std::string& Key, double World)
{
	const auto It = B.PausedUntil.find(Key);
	return It != B.PausedUntil.end() && It->second > World;
}

bool WelchedOn(const Book& B, const std::string& Who, double Since)
{
	for (const Owed& O : B.Owes)
	{
		if (O.Who == Who && !O.ToYou && O.State == OwedWelched && O.Due >= Since)
		{
			return true;
		}
	}
	return false;
}

int MarksSince(const Conduct& C, int K, double Since)
{
	int N = 0;
	for (const std::pair<double, int>& M : C.Marks)
	{
		N += M.second == K && M.first >= Since ? 1 : 0;
	}
	return N;
}

void Mark(Book& B, double At, int K)
{
	std::vector<std::pair<double, int>>& M = B.Record.Marks;
	M.push_back({At, K});
	if (K == MarkShot || K == MarkTilt)
	{
		// Shots and tilted registrations count over 90 days (§2.9), and the most any reader needs is a few: the last 30 of
		// each are kept, so a grinder's month of registrations stays a short line in the save.
		int Kept = 0;
		for (auto It = M.rbegin(); It != M.rend(); ++It)
		{
			if (It->second == K && (++Kept > 30 || It->first < At - 90.0 * Day))
			{
				It->second = -1;
			}
		}
		M.erase(std::remove_if(M.begin(), M.end(), [](const std::pair<double, int>& X) { return X.second == -1; }), M.end());
	}
}

/** The pieces' money for one listing's fills by World: (price per percent) x percent, each fill. */
Chips PricePerPct(Chips BuyIn, int Markup)
{
	return (BuyIn * static_cast<Chips>(Markup) + 50000) / 100000;
}

Chips PieceCost(Chips BuyIn, int Markup, int Permille)
{
	return PricePerPct(BuyIn, Markup) * static_cast<Chips>(Permille / 10);
}

int FilledBy(const Listing& Ls, double World)
{
	int N = 0;
	for (const std::tuple<double, std::string, int>& P : Ls.Plan)
	{
		N += std::get<0>(P) <= World ? std::get<2>(P) : 0;
	}
	return std::min(N, Ls.Ask);
}

/** The fills by World, buyer by buyer (named buyers summed; each fill off the market its own buyer). */
std::vector<std::pair<std::string, int>> FillsBy(const Listing& Ls, double World)
{
	std::vector<std::pair<std::string, int>> Out;
	int Left = Ls.Ask;
	for (const std::tuple<double, std::string, int>& P : Ls.Plan)
	{
		if (std::get<0>(P) > World || std::get<2>(P) <= 0 || Left <= 0)
		{
			continue;
		}
		const int Take = std::min(Left, std::get<2>(P));
		Left -= Take;
		const std::string& Who = std::get<1>(P);
		auto It = std::find_if(Out.begin(), Out.end(), [&Who](const std::pair<std::string, int>& X) { return !Who.empty() && X.first == Who; });
		if (It == Out.end())
		{
			Out.push_back({Who, Take});
		}
		else
		{
			It->second += Take;
		}
	}
	return Out;
}

/** The money the pieces sold on an event bring in at World, the share, and how many buyers. */
struct PiecesNow
{
	Chips Money = 0;
	int Permille = 0;
	int Buyers = 0;
	int Markup = 0; // the one markup, or 0 when listings differ
};

PiecesNow PiecesOn(const Book& B, const std::string& EventId, double World)
{
	PiecesNow P;
	bool First = true;
	for (const Listing& Ls : B.Listings)
	{
		if (Ls.EventId != EventId || Ls.Collected)
		{
			continue;
		}
		for (const std::pair<std::string, int>& F : FillsBy(Ls, World))
		{
			P.Money += PieceCost(Ls.BuyIn, Ls.Markup, F.second);
			P.Permille += F.second;
			P.Buyers += 1;
		}
		if (FilledBy(Ls, World) > 0)
		{
			P.Markup = First || P.Markup == Ls.Markup ? Ls.Markup : 0;
			First = false;
		}
	}
	return P;
}

std::string BackersText(int N)
{
	return std::to_string(N) + (N == 1 ? " backer" : " backers");
}

/** Rent due within the reserve's days of At (S6): what auto-pay keeps back. */
Chips Reserve(const life::State& L, double At)
{
	const bool Due = L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice;
	return Due && L.RentDeadline - At <= RentReserveDays * Day ? L.RentDueCents : 0;
}

/** "Wednesday" for a time. */
std::string WeekdayOf(double At)
{
	return net::WeekdayName(net::DayOf(At), true);
}

Vars EventVars(const std::string& EventId)
{
	Vars V;
	const EventFacts F = FactsOf(EventId);
	V["Short"] = F.Valid ? F.Short : std::string("event");
	V["Event"] = F.Valid ? Title(F.Name) : std::string("the event");
	if (F.Valid)
	{
		V["Day"] = WeekdayOf(F.Start);
	}
	return V;
}

/** The next occurrence of a template starting MinLead to MaxLead minutes after World, its desk still taking entries. */
live::Occurrence NextOf(const char* Key, double World, double MinLead, double MaxLead)
{
	const int Today = net::DayOf(World);
	for (int D = Today; D <= Today + 8; ++D)
	{
		for (const live::Occurrence& O : live::Occurrences(D))
		{
			if (std::string(O.T->Key) == Key && O.Start >= World + MinLead && O.Start <= World + MaxLead && O.LateRegEnds > World + live::TravelMinutes)
			{
				return O;
			}
		}
	}
	return live::Occurrence();
}

bool Plays(const View& V, const std::string& EventId, const std::string& Who)
{
	const auto It = V.Playing.find(EventId);
	return It != V.Playing.end() && std::find(It->second.begin(), It->second.end(), Who) != It->second.end();
}

Regard RegardOf(const View& V, const std::string& Who)
{
	const auto It = V.Regards.find(Who);
	return It == V.Regards.end() ? Regard() : It->second;
}

Chips RollOf(const View& V, const std::string& Who)
{
	const auto It = V.Rolls.find(Who);
	return It == V.Rolls.end() ? 0 : It->second;
}

// Lantern Row's tiers (§2.7): the floor, the player's cut, the cap, the roll, the volume.
struct TierSpec
{
	double Floor;
	int Cut;
	Chips Cap;
	Chips Roll;
	int Volume;
};
const TierSpec LanternTiers[3] = {
	{0.05, 350, 1100, 110000, 120},
	{0.08, 450, 2200, 220000, 120},
	{0.15, 500, 5500, 550000, 100},
};

int TierFor(double ExpRoi)
{
	return ExpRoi >= LanternTiers[2].Floor ? 3 : ExpRoi >= LanternTiers[1].Floor ? 2 : ExpRoi >= LanternTiers[0].Floor ? 1 : 0;
}

Terms LanternTerms(int Tier)
{
	const TierSpec& S = LanternTiers[std::min(2, std::max(0, Tier - 1))];
	Terms T;
	T.CutPermille = S.Cut;
	T.CapOnline = S.Cap;
	T.CapLive = 0;
	T.Roll = S.Roll;
	T.PeriodDays = 30;
	T.Volume = S.Volume;
	T.DueDays = 0;
	T.BuyInBack = false;
	T.MakeupIsDebt = false;
	return T;
}

Terms SamTerms()
{
	Terms T;
	T.CutPermille = 350;
	T.CapOnline = 5500;
	T.CapLive = 12000;
	T.Roll = 150000;
	T.PeriodDays = 30;
	T.Volume = 0;
	T.GhostsPerWeek = 2;
	T.BuyInBack = false;
	T.MakeupIsDebt = true;
	return T;
}

/** The walk-away point's jitter: people are a little unpredictable, seeded once per offer (-25..+25 permille). */
int Jitter(const Offer& O)
{
	return static_cast<int>(Hash("walk:" + O.Id) % 51u) - 25;
}

std::string Lowered(const std::string& In)
{
	std::string Out = In;
	for (char& Ch : Out)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return Out;
}
} // namespace stake_detail

using namespace stake_detail;

// ------------------------------------------------------------------ prices (§2.6)

double ShrunkRoi(int Events, double Observed, bool Live, double Prior)
{
	const double K = Live ? 60.0 : 600.0;
	const double N = static_cast<double>(std::max(0, Events));
	return Prior + (Observed - Prior) * N / (N + K);
}

double OnlinePrior(Chips Abi)
{
	if (Abi <= 0)
	{
		return -100.0 / 1100.0;
	}
	return -static_cast<double>(net::FeeOf(Abi)) / static_cast<double>(Abi);
}

double LivePrior(Chips BuyIn)
{
	for (const live::EventTemplate& T : live::Schedule())
	{
		if (T.BuyInCents == BuyIn && BuyIn > 0)
		{
			return -static_cast<double>(T.FeeCents) / static_cast<double>(T.BuyInCents);
		}
	}
	return -0.125;
}

double ExpRoiOf(const View& V, bool Live, Chips BuyIn)
{
	if (Live)
	{
		const double Prior = LivePrior(BuyIn);
		return V.Rec ? ShrunkRoi(V.Rec->LiveSample, V.Rec->LiveRoi, true, Prior) : Prior;
	}
	const Chips Abi = V.Rec && V.Rec->Abi > 0 ? V.Rec->Abi : BuyIn;
	const double Prior = OnlinePrior(Abi);
	return V.Rec ? ShrunkRoi(V.Rec->OnlineEvents, V.Rec->OnlineRoi, false, Prior) : Prior;
}

int MarkupCap(bool Live, Chips BuyIn)
{
	if (!Live)
	{
		return 1350;
	}
	return BuyIn >= 100000 ? 1500 : 1200;
}

Terms LanternRowTerms(int Tier)
{
	return LanternTerms(Tier);
}

int LanternRowTier(double ExpRoi)
{
	return TierFor(ExpRoi);
}

int Ceiling(const View& V, const Book& B, const std::string& Buyer, bool Live, Chips BuyIn)
{
	(void)B;
	const Regard R = RegardOf(V, Buyer);
	double C = 1.0 + ExpRoiOf(V, Live, BuyIn);
	if (V.Rec)
	{
		C += 0.10 * V.Rec->Renown;
		// Conduct: word of mouth (a welch, a late payment), the paper (a pickup) for the room, the site's restriction online.
		C -= V.Rec->Welched180 ? 0.10 : 0.0;
		C -= V.Rec->Late90 ? 0.05 : 0.0;
		C -= Live && V.Rec->PickedUp90 ? 0.05 : 0.0;
		C -= !Live && V.Rec->Restricted180 ? 0.05 : 0.0;
	}
	C += 0.05 * static_cast<double>(R.Trust) - 0.10 * static_cast<double>(R.Resentment);
	const double Cap = static_cast<double>(MarkupCap(Live, BuyIn)) / 1000.0;
	C = C < 1.0 ? 1.0 : C > Cap ? Cap : C;
	const long long Hundredths = static_cast<long long>(C * 100.0 + 0.5 + 1e-9);
	return static_cast<int>(Hundredths * 10);
}

std::string CeilingWhy(const View& V, bool Live, Chips BuyIn)
{
	const int N = V.Rec ? (Live ? V.Rec->LiveSample : V.Rec->OnlineEvents) : 0;
	const double Observed = V.Rec ? (Live ? V.Rec->LiveRoi : V.Rec->OnlineRoi) : 0.0;
	const double Believe = ExpRoiOf(V, Live, BuyIn);
	if (N <= 0)
	{
		return Live ? "No live record yet: buyers pay par, if they buy." : "No RiverLine record yet: buyers pay par, if they buy.";
	}
	const std::string Sample = std::to_string(N) + (Live ? (N == 1 ? " live event" : " live events") : (N == 1 ? " RiverLine event" : " RiverLine events"));
	return Sample + " at " + RoiText(Observed) + ": they believe " + RoiText(Believe) + (Believe >= 0.0 ? " of it." : ".");
}

// ------------------------------------------------------------------ offers (§2.5)

std::vector<Offer> OffersFor(const View& V, const Book& B, double World)
{
	std::vector<Offer> Out;
	const int Today = net::DayOf(World);
	const rep::Public* Rec = V.Rec;
	auto Standing = [&B](const std::string& Backer) {
		return std::any_of(B.Offers.begin(), B.Offers.end(), [&Backer](const Offer& O) { return O.Backer == Backer && O.State == OfferStanding; });
	};
	auto Fresh = [&B, &Out](const std::string& Id) {
		return !Did(B, "offer:" + Id) && std::none_of(B.Offers.begin(), B.Offers.end(), [&Id](const Offer& O) { return O.Id == Id; }) &&
			std::none_of(Out.begin(), Out.end(), [&Id](const Offer& O) { return O.Id == Id; });
	};
	auto Clean = [&B](const std::string& EventId, int More) {
		return !EventDeal(B, EventId) && AskedOn(B, EventId) + SwappedOn(B, EventId) + More <= MaxSoldPermille;
	};

	// Big Lou: the Nightly, at most once a week, after the player's first live cash, while he's not pausing.
	if (Waking(World) && Rec && Rec->LiveCashes >= 1 && !Standing(Lou) && !Paused(B, Lou, World) && !Paused(B, std::string("offers:") + Lou, World) &&
		StakesSince(B, Lou, World - LouWeekDays * Day) == 0 && StakesSince(B, Lou, World - 30.0 * Day) < LouMonthCap && !WelchedOn(B, Lou, World - 365.0 * Day))
	{
		const Regard R = RegardOf(V, Lou);
		const bool Open = std::any_of(B.Deals.begin(), B.Deals.end(), [](const Deal& D) { return D.K == Kind::Event && D.Backer == Lou && D.State == DealOpen; });
		const live::Occurrence O = NextOf("nightly", World, 6.0 * Hour, 3.0 * Day);
		if (R.Known && !Open && O.Valid() && O.T->BuyInCents <= LouMaxStakeCents && RollOf(V, Lou) >= 20 * O.T->BuyInCents && Fresh("lou:" + O.Id) &&
			Clean(O.Id, 0))
		{
			Offer Of;
			Of.Id = "lou:" + O.Id;
			Of.K = Kind::Event;
			Of.Backer = Lou;
			Of.EntryId = O.Id;
			Of.T.CutPermille = 500;
			Of.T.BuyInBack = true;
			Of.T.DueDays = 0;
			Of.Expires = std::min(O.LateRegEnds, World + OfferDays * Day);
			Of.At = World;
			Of.Permille = 1000;
			Of.BuyIn = O.T->BuyInCents;
			Vars Vs = EventVars(O.Id);
			Vs["Amount"] = Money(O.T->BuyInCents);
			Vs["Day"] = live::DayWord(O, World);
			Of.Line = Say(Lou, "offer", Hash(Of.Id), Vs);
			Out.push_back(Of);
		}
	}

	// Dee: ten percent of the player's first Sunday, at par, once.
	if (Waking(World) && Rec && Rec->LiveEvents >= 1 && !V.SundayPlayed && !Did(B, "dee-piece") && !Standing(Dee))
	{
		const live::Occurrence O = NextOf("sunday", World, 12.0 * Hour, 3.0 * Day);
		if (O.Valid() && Fresh("dee:" + O.Id) && Clean(O.Id, 100))
		{
			Offer Of;
			Of.Id = "dee:" + O.Id;
			Of.K = Kind::Pieces;
			Of.Backer = Dee;
			Of.EntryId = O.Id;
			Of.T.MarkupPermille = 1000;
			Of.Expires = std::min(O.LateRegEnds, World + OfferDays * Day);
			Of.At = World;
			Of.Permille = 100;
			Of.BuyIn = O.T->BuyInCents;
			Of.Line = Say(Dee, "offer", Hash(Of.Id), EventVars(O.Id));
			Out.push_back(Of);
		}
	}

	// Mei: a swap in the Sunday or the Big Stack she's playing, once she respects the player or they've final-tabled together.
	if (Waking(World) && !Standing(Mei) && !Paused(B, std::string("offers:") + Mei, World))
	{
		const Regard R = RegardOf(V, Mei);
		if (R.Respect >= 0.3f || R.FinalTable)
		{
			for (const char* Key : {"sunday", "bigstack"})
			{
				const live::Occurrence O = NextOf(Key, World, 6.0 * Hour, 2.0 * Day);
				const int Share = R.Respect >= 0.5f ? 100 : 50;
				const bool Already = std::any_of(B.Swaps.begin(), B.Swaps.end(), [&O](const Swap& S) { return S.EntryId == O.Id && S.Partner == Mei; });
				if (O.Valid() && Plays(V, O.Id, Mei) && !Already && Fresh("mei:" + O.Id) && Clean(O.Id, Share))
				{
					Offer Of;
					Of.Id = "mei:" + O.Id;
					Of.K = Kind::Swap;
					Of.Backer = Mei;
					Of.EntryId = O.Id;
					Of.Expires = std::min(O.LateRegEnds, World + 2.0 * Day);
					Of.At = World;
					Of.Permille = Share;
					Of.BuyIn = O.T->BuyInCents;
					Vars Vs = EventVars(O.Id);
					Vs["Pct"] = PctText(Share);
					Of.Line = Say(Mei, "offer", Hash(Of.Id), Vs);
					Out.push_back(Of);
					break;
				}
			}
		}
	}

	// Rick: a swap in the Midnight Turbo, the nights he's in it.
	if (HourOf(World) >= 12.0 && HourOf(World) < 22.0 && !Standing(Rick) && !Paused(B, std::string("offers:") + Rick, World))
	{
		const Regard R = RegardOf(V, Rick);
		const live::Occurrence O = NextOf("turbo", World, 1.0 * Hour, 0.5 * Day);
		const bool Already = O.Valid() && std::any_of(B.Swaps.begin(), B.Swaps.end(), [&O](const Swap& S) { return S.EntryId == O.Id && S.Partner == Rick; });
		if (R.Known && R.Resentment < 0.4f && O.Valid() && Plays(V, O.Id, Rick) && !Already && Fresh("rick:" + O.Id) && Clean(O.Id, 100))
		{
			Offer Of;
			Of.Id = "rick:" + O.Id;
			Of.K = Kind::Swap;
			Of.Backer = Rick;
			Of.EntryId = O.Id;
			Of.Expires = O.LateRegEnds;
			Of.At = World;
			Of.Permille = 100;
			Of.BuyIn = O.T->BuyInCents;
			Vars Vs = EventVars(O.Id);
			Vs["Pct"] = PctText(100);
			Of.Line = Say(Rick, "offer", Hash(Of.Id), Vs);
			Out.push_back(Of);
		}
	}

	// Lantern Row Staking: 500 tracked events at +5% shrunk ROI (freerolls excluded), no welch in a year, never someone
	// who walked; one tier down per three tilted registrations in 90 days; ten cut points off for 180 days after a walk.
	if (Rec && !ActivePackage(B) && !Standing(LanternRow) && !Paused(B, "lanternrow", World) && !V.Restricted && Rec->OnlineEvents >= LanternRowEvents &&
		!Rec->Welched365 && !Rec->Walked365)
	{
		const double Exp = ExpRoiOf(V, false, Rec->Abi);
		int Tier = TierFor(Exp) - MarksSince(B.Record, MarkTilt, World - 90.0 * Day) / 3;
		if (Tier >= 1 && Fresh("lanternrow:" + std::to_string(Today)))
		{
			Offer Of;
			Of.Id = "lanternrow:" + std::to_string(Today);
			Of.K = Kind::Package;
			Of.By = Outfit::LanternRow;
			Of.Backer = LanternRow;
			Of.T = LanternTerms(Tier);
			if (MarksSince(B.Record, MarkWalked, World - 180.0 * Day) > 0)
			{
				Of.T.CutPermille -= 100;
			}
			Of.Tier = Tier;
			Of.Expires = World + 7.0 * Day;
			Of.At = World;
			Of.BuyIn = Of.T.CapOnline;
			Out.push_back(Of);
		}
	}

	// Sam: broke, rent due inside ten days, and the player has cashed. Three in the morning, on the Burner.
	if (SmallHours(World) && V.Cashes >= 1 && V.Bankroll < 15000 && V.RentDue > 0 && V.RentDeadline - World <= 10.0 * Day && !ActivePackage(B) &&
		!Standing(SamName) && !Paused(B, "sam", World) && std::none_of(B.Deals.begin(), B.Deals.end(), [](const Deal& D) { return D.Debt > 0; }))
	{
		if (Fresh("sam:" + std::to_string(Today)))
		{
			Offer Of;
			Of.Id = "sam:" + std::to_string(Today);
			Of.K = Kind::Package;
			Of.By = Outfit::Sam;
			Of.Backer = SamName;
			Of.T = SamTerms();
			Of.Expires = World + OfferDays * Day;
			Of.At = World;
			Of.BuyIn = Of.T.CapOnline;
			Of.Line = Say(SamName, "offer", Did(B, "lanternrow-ever") ? 0u : 1u); // (the leash line for someone Lantern Row has staked)
			Out.push_back(Of);
		}
	}

	// The world's regulars who respect or trust the player, with a roll 400 times the player's ABI and fewer than three
	// horses: an online package, fifty-fifty, sixty days, makeup forgiven at the end. At most one a month.
	if (Waking(World) && Rec && Rec->Abi > 0 && Rec->OnlineEvents >= 100 && !ActivePackage(B) && !Paused(B, "regulars", World))
	{
		for (const Patron& P : V.Patrons)
		{
			if (!P.Respects || P.Roll < 400 * Rec->Abi || P.Horses >= 3 || Standing(P.Name) || Paused(B, "offers:" + P.Name, World) || WelchedOn(B, P.Name, World - 365.0 * Day))
			{
				continue;
			}
			const std::string Id = "regular:" + P.Name + ":" + std::to_string(Today);
			if (!Fresh(Id))
			{
				continue;
			}
			Offer Of;
			Of.Id = Id;
			Of.K = Kind::Package;
			Of.Backer = P.Name;
			// Word of a walk away from makeup: ten points off the cut for 180 days (§2.9).
			Of.T.CutPermille = MarksSince(B.Record, MarkWalked, World - 180.0 * Day) > 0 ? 400 : 500;
			Of.T.CapOnline = 2 * Rec->Abi;
			Of.T.Roll = 50 * Of.T.CapOnline;
			Of.T.PeriodDays = 60;
			Of.T.BuyInBack = false;
			Of.Expires = World + OfferDays * Day;
			Of.At = World;
			Of.BuyIn = Of.T.CapOnline;
			Vars Vs;
			Vs["Amount"] = Money(Of.T.Roll);
			Vs["Cap"] = net::BuyIn(Of.T.CapOnline);
			Of.Line = Say(P.Name, Of.T.CutPermille < 500 ? "offer.package.walked" : "offer.package", Hash(Id), Vs);
			Out.push_back(Of);
			break;
		}
	}
	return Out;
}

double Margin(const Offer& O, const Terms& Ask)
{
	// The walk-away point (§2.10), offset by who's offering: Lou 10 points better for the player than his offer; Mrs.
	// Park none; Lantern Row one tier step; Sam 10 cut points (he lowballs); the world's regulars 5; jittered per offer.
	int CutWalk = O.T.CutPermille + Jitter(O);
	Chips CapWalk = O.T.CapOnline;
	int MarkupWalk = O.T.MarkupPermille + Jitter(O) / 5;
	if (O.Backer == Lou || O.By == Outfit::Sam)
	{
		CutWalk += 100;
	}
	else if (O.By == Outfit::LanternRow)
	{
		const Terms Next = LanternTerms(std::min(3, O.Tier + 1));
		CutWalk += O.Tier >= 3 ? 25 : Next.CutPermille - LanternTerms(O.Tier).CutPermille;
		CapWalk = O.Tier >= 3 ? O.T.CapOnline : Next.CapOnline;
	}
	else if (O.Backer != Park && O.Backer != Dee)
	{
		CutWalk += 50;
	}
	// Only what the ask changes is measured: a counter moves one lever.
	double M = 1.0;
	if (Ask.CutPermille != O.T.CutPermille)
	{
		M = std::min(M, static_cast<double>(CutWalk - Ask.CutPermille) / 100.0);
	}
	if (Ask.MarkupPermille != O.T.MarkupPermille)
	{
		M = std::min(M, static_cast<double>(MarkupWalk - Ask.MarkupPermille) / 100.0);
	}
	if (Ask.CapOnline != O.T.CapOnline)
	{
		const double Step = static_cast<double>(std::max<Chips>(1, CapWalk > O.T.CapOnline ? CapWalk - O.T.CapOnline : O.T.CapOnline));
		M = std::min(M, static_cast<double>(CapWalk - Ask.CapOnline) / Step);
	}
	return M;
}

int Answer(const Offer& O, const Terms& Ask, int OnDay)
{
	const double M = Margin(O, Ask);
	const double A = M < 0.0 ? -M : M;
	const double P = 0.5 + 0.5 * M / (0.2 + A);
	Rng R = SeedRng(O.Id, OnDay);
	if (R.Next() < P)
	{
		return 1;
	}
	return O.Counters >= 1 ? -1 : 0;
}

Terms CounterTerms(const Offer& O, int Lever)
{
	Terms T = O.T;
	if (Lever == 0)
	{
		T.CutPermille = std::min(1000, T.CutPermille + 50);
	}
	else if (Lever == 1)
	{
		// For the player: a piece bought at a little more, a backer's markup a little less.
		T.MarkupPermille = O.K == Kind::Pieces ? T.MarkupPermille + 50 : std::max(1000, T.MarkupPermille - 50);
	}
	else if (Lever == 2 && O.K == Kind::Package)
	{
		T.CapOnline = O.By == Outfit::LanternRow && O.Tier < 3 ? LanternTerms(O.Tier + 1).CapOnline : T.CapOnline + T.CapOnline / 2;
	}
	return T;
}

std::string Counter(Book& B, Chips& Bankroll, life::State& L, const std::string& OfferId, int Lever, double World)
{
	Offer* O = FindOffer(B, OfferId);
	if (!O || O->State != OfferStanding)
	{
		return "That offer's gone.";
	}
	const bool Fits = O->K == Kind::Event ? Lever == 0 : O->K == Kind::Pieces ? Lever == 1 : O->K == Kind::Package ? Lever == 0 || Lever == 2 : false;
	if (!Fits)
	{
		return "There's nothing to counter there.";
	}
	const int Today = net::DayOf(World);
	if (O->CounteredDay == Today)
	{
		return "One counter a day. Give them till tomorrow.";
	}
	const Terms Ask = CounterTerms(*O, Lever);
	const int Said = Answer(*O, Ask, Today);
	const std::string Who = O->Backer;
	const Outfit By = O->By;
	const uint32_t Salt = Hash(O->Id) + static_cast<uint32_t>(Today);
	// Lantern Row answers by letter, like everything it says (the Mail app); people answer by text (the session sends it).
	const std::string Thread = OfferId; // (a copy: the offer may leave the book before the answer goes out)
	auto Reply = [&L, By, World, Thread](const std::string& Line) {
		if (By == Outfit::LanternRow && !Line.empty())
		{
			Vars V;
			V["Line"] = Line;
			Mail(L, World, Thread, "counter", V, LetterNote);
		}
		return Line;
	};
	if (Said == 1)
	{
		Offer Agreed = *O;
		Agreed.T = Ask;
		const std::string Not = Accept(B, Bankroll, L, Agreed, World);
		return Not.empty() ? Reply(Say(Who, "counter.yes", Salt)) : Not;
	}
	if (Said == 0)
	{
		O->Counters += 1;
		O->CounteredDay = Today;
		O->Lever = Lever;
		O->Expires = std::min(O->Expires, World + 24.0 * Hour);
		return Reply(Say(Who, "counter.stands", Salt));
	}
	// A second refusal withdraws it, and they won't offer for a week (an outfit's door is its own key).
	O->State = OfferWithdrawn;
	B.PausedUntil["offers:" + Who] = World + 7.0 * Day;
	if (By == Outfit::LanternRow || By == Outfit::Sam)
	{
		const std::string Door = By == Outfit::LanternRow ? "lanternrow" : "sam";
		B.PausedUntil[Door] = std::max(B.PausedUntil[Door], World + 7.0 * Day);
	}
	else if (O->K == Kind::Package)
	{
		B.PausedUntil["regulars"] = std::max(B.PausedUntil["regulars"], World + 7.0 * Day);
	}
	Once(B, "offer:" + O->Id, World);
	B.Offers.erase(std::remove_if(B.Offers.begin(), B.Offers.end(), [&OfferId](const Offer& X) { return X.Id == OfferId; }), B.Offers.end());
	return Reply(Say(Who, "counter.no", Salt));
}

namespace stake_detail
{
/** Takes an offer; Registering: the desk is registering on it right now (the entry already exists). */
std::string Take(Book& B, life::State& L, const Offer& O, double World, bool Registering)
{
	if (Did(B, "offer:" + O.Id))
	{
		return "That offer's gone.";
	}
	if (O.Expires > 0.0 && O.Expires <= World)
	{
		return "That offer ran out.";
	}
	const EventFacts F = FactsOf(O.EntryId);
	if (O.K != Kind::Package)
	{
		if (!F.Valid || F.Closes <= World)
		{
			return "That one's already under way.";
		}
		if (!Registering && O.K != Kind::Swap && live::EntryFor(L, O.EntryId) != nullptr)
		{
			return "You're already registered for that one.";
		}
	}
	switch (O.K)
	{
	case Kind::Event:
	{
		if (StakesSince(B, O.Backer, World - LouWeekDays * Day) > 0 || StakesSince(B, O.Backer, World - 30.0 * Day) >= LouMonthCap)
		{
			return Say(O.Backer, "weekly", Hash(O.Id));
		}
		if (Paused(B, O.Backer, World))
		{
			return Say(O.Backer, "paused", Hash(O.Id));
		}
		if (EventDeal(B, O.EntryId) || AskedOn(B, O.EntryId) > 0 || SwappedOn(B, O.EntryId) > 0)
		{
			return "You've sold action in that one already. It's yours or nobody's.";
		}
		Deal D;
		D.Id = "deal:" + O.Id;
		D.K = Kind::Event;
		D.Backer = O.Backer;
		D.T = O.T;
		D.Since = World;
		D.EventId = O.EntryId;
		D.Until = F.Closes;
		D.Stake = F.BuyIn;
		D.State = DealOpen;
		B.Deals.push_back(D);
		break;
	}
	case Kind::Pieces:
	{
		if (EventDeal(B, O.EntryId))
		{
			return "Someone's already putting you in that one.";
		}
		if (AskedOn(B, O.EntryId) + SwappedOn(B, O.EntryId) + O.Permille > MaxSoldPermille)
		{
			return "That's more than 70% of you sold. Keep some of yourself.";
		}
		Listing Ls;
		Ls.EventId = O.EntryId;
		Ls.Ask = O.Permille;
		Ls.Markup = O.T.MarkupPermille;
		Ls.Filled = O.Permille;
		Ls.Opens = World;
		Ls.Closes = F.Closes;
		Ls.Plan.push_back(std::make_tuple(World, O.Backer, O.Permille));
		Ls.BuyIn = F.BuyIn;
		Ls.Live = F.Live;
		Ls.Told = 1; // the offer was the text
		B.Listings.push_back(Ls);
		if (O.Backer == Dee)
		{
			Once(B, "dee-piece", World);
		}
		break;
	}
	case Kind::Swap:
	{
		if (EventDeal(B, O.EntryId))
		{
			return "Someone's already putting you in that one.";
		}
		if (AskedOn(B, O.EntryId) + SwappedOn(B, O.EntryId) + O.Permille > MaxSoldPermille)
		{
			return "That's more than 70% of you gone. Keep some of yourself.";
		}
		const life::LiveEntry* Mine = live::EntryFor(L, O.EntryId);
		if (Mine && (Mine->State != life::LiveEntry::Registered || Mine->Fund.From != life::Paid::Own))
		{
			return Mine->State != life::LiveEntry::Registered ? "That one's over." : "That entry isn't yours to swap.";
		}
		Swap S;
		S.EntryId = O.EntryId;
		S.Partner = O.Backer;
		S.Permille = O.Permille;
		S.State = Mine ? SwapLive : SwapAgreed;
		S.Closes = F.Closes;
		S.Since = World;
		B.Swaps.push_back(S);
		FlowTo(B, "swap:" + O.EntryId + ":" + O.Backer, O.Backer, Title(F.Name), 0, Memory::Swapped, World);
		break;
	}
	case Kind::Package:
	{
		if (ActivePackage(B))
		{
			return Say(LanternRow, "one", 0);
		}
		if (O.By == Outfit::LanternRow && (Paused(B, "lanternrow", World) || MarksSince(B.Record, MarkWalked, World - 365.0 * Day) > 0))
		{
			return "Lantern Row doesn't take back people who walk.";
		}
		Deal D;
		D.Id = "deal:" + O.Id;
		D.K = Kind::Package;
		D.By = O.By;
		D.Backer = O.Backer;
		D.T = O.T;
		D.Since = World;
		D.NextChop = World + static_cast<double>(O.T.PeriodDays) * Day;
		D.RollNow = O.T.Roll;
		D.Funded = O.T.Roll;
		D.Tier = O.Tier;
		D.GhostsAtStart = L.Ghosts;
		D.State = DealOpen;
		DealLine(D, World, O.T.Roll, "The roll, funded");
		B.Deals.push_back(D);
		if (O.By == Outfit::LanternRow)
		{
			Once(B, "lanternrow-ever", World);
			Vars Vs;
			Vs["Cap"] = net::BuyIn(O.T.CapOnline);
			Vs["Day"] = net::DateLabel(net::DayOf(D.NextChop));
			Mail(L, World, D.Id, "welcome", Vs, LetterWelcome);
		}
		FlowTo(B, "package:" + D.Id, O.Backer, "a package", 0, Memory::Staked, World);
		break;
	}
	}
	Once(B, "offer:" + O.Id, World);
	const std::string Id = O.Id; // (O may be one of B.Offers)
	B.Offers.erase(std::remove_if(B.Offers.begin(), B.Offers.end(), [&Id](const Offer& X) { return X.Id == Id; }), B.Offers.end());
	return "";
}
} // namespace stake_detail

std::string Accept(Book& B, Chips& Bankroll, life::State& L, const Offer& O, double World)
{
	(void)Bankroll;
	const Offer Held = O; // the book's own offer may be the one passed in, and it goes
	return Take(B, L, Held, World, false);
}

std::string Decline(Book& B, const std::string& OfferId, double World)
{
	Offer* O = FindOffer(B, OfferId);
	if (!O)
	{
		return "That offer's gone.";
	}
	const Offer Was = *O;
	Once(B, "offer:" + OfferId, World);
	B.Offers.erase(std::remove_if(B.Offers.begin(), B.Offers.end(), [&OfferId](const Offer& X) { return X.Id == OfferId; }), B.Offers.end());
	if (Was.By == Outfit::LanternRow)
	{
		B.PausedUntil["lanternrow"] = World + 30.0 * Day;
	}
	else if (Was.By == Outfit::Sam)
	{
		B.PausedUntil["sam"] = World + 14.0 * Day;
	}
	else if (Was.K == Kind::Package)
	{
		B.PausedUntil["regulars"] = World + 30.0 * Day;
	}
	if (Was.Backer == Dee)
	{
		Once(B, "dee-piece", World);
	}
	return "";
}

void ToBurner(Book& B, double World, const std::string& Said)
{
	if (Said.empty())
	{
		return;
	}
	B.Burner.push_back({World, Said});
	if (B.Burner.size() > 20)
	{
		B.Burner.erase(B.Burner.begin(), B.Burner.begin() + static_cast<std::ptrdiff_t>(B.Burner.size() - 20));
	}
}

void Announce(Book& B, life::State& L, const Offer& O, double World, std::vector<std::pair<std::string, std::string>>& Texts)
{
	if (!Once(B, "said:" + O.Id, World))
	{
		return;
	}
	if (O.By == Outfit::LanternRow)
	{
		Vars Vs;
		Vs["Amount"] = Money(O.T.Roll);
		Vs["Cap"] = net::BuyIn(O.T.CapOnline);
		Vs["Pct"] = PctText(O.T.CutPermille);
		Mail(L, World, O.Id, "offer", Vs, LetterOffer);
		return;
	}
	if (O.By == Outfit::Sam)
	{
		// The fine print, spelled out before the player signs (§2.7, Q4).
		ToBurner(B, World, O.Line);
		ToBurner(B, World, Say(SamName, "offer.terms", 0));
		return;
	}
	if (!O.Line.empty())
	{
		Texts.push_back({O.Backer, O.Line});
	}
}

// ------------------------------------------------------------------ who pays (§1.2)

life::Funding FundFor(const Book& B, const std::string& EntryId, Chips BuyIn, bool Live, double World)
{
	life::Funding F;
	if (Live)
	{
		if (const Deal* D = EventDeal(B, EntryId))
		{
			F.From = life::Paid::Backer;
			F.Ref = D->Id;
			F.Who = D->Backer;
			F.Permille = 1000;
			F.Cents = BuyIn;
			return F;
		}
	}
	if (const Deal* P = ActivePackage(B))
	{
		const Chips Cap = Live ? P->T.CapLive : P->T.CapOnline;
		int Bullets = 0;
		for (const Fund& Fd : B.Funds)
		{
			Bullets += Fd.EventId == EntryId && Fd.DealId == P->Id ? 1 : 0;
		}
		if (BuyIn > 0 && BuyIn <= Cap && Bullets < (Live ? 1 : PackageBullets) && AskedOn(B, EntryId) == 0 && SwappedOn(B, EntryId) == 0)
		{
			F.From = life::Paid::Package;
			F.Ref = P->Id;
			F.Who = BackerName(*P);
			F.Permille = 1000 - P->T.CutPermille;
			F.Cents = BuyIn;
			return F;
		}
	}
	const PiecesNow Pc = PiecesOn(B, EntryId, World);
	if (Pc.Permille > 0)
	{
		F.Ref = EntryId;
		F.Who = BackersText(Pc.Buyers);
		F.Permille = Pc.Permille;
		F.Cents = Pc.Money;
	}
	return F;
}

std::vector<std::pair<life::Funding, std::string>> FundingOptions(const life::State& L, const live::Occurrence& O, double World)
{
	std::vector<std::pair<life::Funding, std::string>> Out;
	if (!O.Valid())
	{
		Out.push_back({life::Funding(), std::string()});
		return Out;
	}
	const Book& B = L.Stakes;
	const Chips BuyIn = O.T->BuyInCents;
	const bool Sold = AskedOn(B, O.Id) > 0 || SwappedOn(B, O.Id) > 0;
	// A backer's stake: an accepted one for this event, or Lou's standing offer for it (registering takes it).
	if (const Deal* D = EventDeal(B, O.Id))
	{
		life::Funding F;
		F.From = life::Paid::Backer;
		F.Ref = D->Id;
		F.Who = D->Backer;
		F.Permille = 1000;
		F.Cents = BuyIn;
		Out.push_back({F, std::string()});
	}
	else
	{
		for (const Offer& Of : B.Offers)
		{
			if (Of.K == Kind::Event && Of.EntryId == O.Id && Of.State == OfferStanding && Of.Expires > World)
			{
				life::Funding F;
				F.From = life::Paid::Backer;
				F.Ref = Of.Id;
				F.Who = Of.Backer;
				F.Permille = 1000;
				F.Cents = BuyIn;
				const bool Weekly = StakesSince(B, Of.Backer, World - LouWeekDays * Day) > 0 || StakesSince(B, Of.Backer, World - 30.0 * Day) >= LouMonthCap;
				Out.push_back({F, Weekly ? Say(Of.Backer, "weekly", Hash(Of.Id)) : Paused(B, Of.Backer, World) ? Say(Of.Backer, "paused", Hash(Of.Id)) : Sold ? std::string("You've sold pieces of this one. It's yours or nobody's.") : std::string()});
				break;
			}
		}
	}
	// The sponsors' covered entries, each deal that covers entries with why it won't cover this one (used up this month,
	// over its cap, not an event it names), and the brand's short name for the ledger ("Lucky Penny: covered the Noon
	// Deepstack", §9.2). Covered entries go in clean: no pieces, no swaps, this segment.
	for (const std::pair<life::Funding, std::string>& Cover : sponsor::CoverOptions(L.Sponsors, O, World))
	{
		Out.push_back({Cover.first, !Cover.second.empty() ? Cover.second : Sold ? std::string("Covered entries go in clean. You've sold pieces of this one.") : std::string()});
	}
	// A package's roll (S5: every event in its range plays on it).
	if (const Deal* P = ActivePackage(B))
	{
		if (P->T.CapLive > 0)
		{
			life::Funding F;
			F.From = life::Paid::Package;
			F.Ref = P->Id;
			F.Who = BackerName(*P);
			F.Permille = 1000 - P->T.CutPermille;
			F.Cents = BuyIn;
			const std::string Why = BuyIn > P->T.CapLive ? BackerName(*P) + "'s roll stops at " + net::BuyIn(P->T.CapLive) + " live."
				: Sold ? std::string("You've sold pieces of this one. It's your own money.") : std::string();
			Out.push_back({F, Why});
		}
	}
	// The player's own money, with what the pieces sold on it bring in.
	Out.push_back({FundFor(B, O.Id, BuyIn, true, World), std::string()});
	if (Out.back().first.From != life::Paid::Own)
	{
		// (FundFor found the backer or the package above: the player's own money is the plain one.)
		life::Funding Own;
		const PiecesNow Pc = PiecesOn(B, O.Id, World);
		if (Pc.Permille > 0)
		{
			Own.Ref = O.Id;
			Own.Who = BackersText(Pc.Buyers);
			Own.Permille = Pc.Permille;
			Own.Cents = Pc.Money;
		}
		Out.back().first = Own;
	}
	return Out;
}

std::vector<std::pair<life::Funding, std::string>> OnlineFundingOptions(const life::State& L, const std::string& EventId, Chips BuyIn, double World)
{
	std::vector<std::pair<life::Funding, std::string>> Out;
	const Book& B = L.Stakes;
	if (const Deal* P = ActivePackage(B))
	{
		if (P->T.CapOnline > 0)
		{
			int Bullets = 0;
			for (const Fund& Fd : B.Funds)
			{
				Bullets += Fd.EventId == EventId && Fd.DealId == P->Id ? 1 : 0;
			}
			life::Funding F;
			F.From = life::Paid::Package;
			F.Ref = P->Id;
			F.Who = BackerName(*P);
			F.Permille = 1000 - P->T.CutPermille;
			F.Cents = BuyIn;
			std::string Why;
			if (BuyIn <= 0)
			{
				Why = "Freerolls are on you. No roll needed.";
			}
			else if (BuyIn > P->T.CapOnline)
			{
				Why = BackerName(*P) + "'s roll stops at " + net::BuyIn(P->T.CapOnline) + ".";
			}
			else if (Bullets >= PackageBullets)
			{
				Why = BackerName(*P) + " covers two re-entries. This bullet's on you.";
			}
			else if (AskedOn(B, EventId) > 0)
			{
				Why = "You've sold pieces of this one. It's your own money.";
			}
			Out.push_back({F, Why});
		}
	}
	life::Funding Own;
	const PiecesNow Pc = PiecesOn(B, EventId, World);
	if (Pc.Permille > 0)
	{
		Own.Ref = EventId;
		Own.Who = BackersText(Pc.Buyers);
		Own.Permille = Pc.Permille;
		Own.Cents = Pc.Money;
	}
	Out.push_back({Own, std::string()});
	return Out;
}

bool Accepted(const life::Funding& F)
{
	// FundingOptions names an accepted stake by its deal ("deal:" + the offer it came from), a standing offer by itself.
	return F.From == life::Paid::Backer && F.Ref.rfind("deal:", 0) == 0;
}

// ------------------------------------------------------------------ money (§1.2, §2.4)

namespace stake_detail
{
bool InRoster(const life::LiveEntry& E, const std::string& Who)
{
	return std::any_of(E.Roster.begin(), E.Roster.end(), [&Who](const std::pair<std::string, int>& R) { return R.first == Who; });
}

/** The package's roll falls under 40 buy-ins at its cap: the backer tops it back to size (the makeup grows). */
void TopUp(Deal& D, double At)
{
	// Under 40 buy-ins at the online cap (or half the roll, for a short one like Sam's), or short of the next buy-in.
	const Chips Cap = std::max(D.T.CapOnline, D.T.CapLive);
	const Chips Floor = std::max(std::min(40 * std::max<Chips>(1, D.T.CapOnline > 0 ? D.T.CapOnline : D.T.CapLive), D.T.Roll / 2), Cap);
	if (Cap > 0 && D.RollNow < Floor && D.RollNow < D.T.Roll)
	{
		const Chips Add = D.T.Roll - D.RollNow;
		D.RollNow += Add;
		D.Funded += Add;
		DealLine(D, At, Add, "Topped up by " + BackerName(D));
	}
	D.Makeup = std::max<Chips>(0, D.Funded - D.RollNow);
}

/** A listing's fills become pieces of the entry, and the buyers' money moves in one line (§1.2). */
void Collect(Book& B, Chips& Bankroll, life::State& L, const std::string& EventId, const std::string& EntryName, const life::LiveEntry* E, double World)
{
	Chips Total = 0;
	int Permille = 0;
	int Markup = -1;
	for (Listing& Ls : B.Listings)
	{
		if (Ls.EventId != EventId || Ls.Collected)
		{
			continue;
		}
		Ls.Collected = true;
		Ls.Filled = FilledBy(Ls, World);
		for (const std::pair<std::string, int>& F : FillsBy(Ls, World))
		{
			Piece P;
			P.EntryId = EventId;
			P.Buyer = F.first;
			P.Permille = F.second;
			P.Paid = PieceCost(Ls.BuyIn, Ls.Markup, F.second);
			P.InField = E && !F.first.empty() && InRoster(*E, F.first);
			B.Pieces.push_back(P);
			Total += P.Paid;
			Permille += P.Permille;
			Markup = Markup < 0 || Markup == Ls.Markup ? Ls.Markup : 0;
			FlowTo(B, "piece:" + EventId + ":" + F.first + ":" + std::to_string(B.Pieces.size()), F.first, Title(EntryName), -P.Paid, Memory::BoughtPiece, World);
			if (F.first == Lou)
			{
				B.NetOnYou[Lou] -= P.Paid;
			}
		}
	}
	if (Total > 0)
	{
		Move(Bankroll, L, World, "Pieces sold: " + Title(EntryName) + ", " + PctText(Permille) + (Markup > 0 ? " at " + MarkupText(Markup) : std::string()), Total);
	}
	// What was listed and not filled by now is off the table.
}

/** The listings and swaps on an event someone else paid for lapse: covered and staked entries go in clean. */
void Clear(Book& B, const std::string& EventId)
{
	B.Listings.erase(std::remove_if(B.Listings.begin(), B.Listings.end(), [&EventId](const Listing& Ls) { return Ls.EventId == EventId && !Ls.Collected; }), B.Listings.end());
	for (Swap& S : B.Swaps)
	{
		if (S.EntryId == EventId && (S.State == SwapAgreed || S.State == SwapLive))
		{
			S.State = SwapLapsed;
		}
	}
}

/** What the pieces on an entry paid: owed to each buyer (one record per counterparty), Lou's net kept. */
void PiecesOwed(Book& B, const std::string& EntryId, const std::string& Name, int Place, Chips Prize, double World)
{
	std::map<std::string, std::pair<int, Chips>> ByBuyer; // display name -> permille, share
	for (Piece& P : B.Pieces)
	{
		if (P.EntryId != EntryId || P.Settled)
		{
			continue;
		}
		P.Settled = true;
		P.Share = Prize * static_cast<Chips>(P.Permille) / 1000;
		std::pair<int, Chips>& Sum = ByBuyer[Display(P.Buyer)];
		Sum.first += P.Permille;
		Sum.second += P.Share;
		if (P.Buyer == Lou)
		{
			B.NetOnYou[Lou] += P.Share;
		}
	}
	for (const auto& It : ByBuyer)
	{
		if (It.second.second <= 0)
		{
			// A brick: nothing owed, but the people who bought a piece remember the night (told by Step).
			Once(B, "brick:" + EntryId + ":" + It.first, World);
			continue;
		}
		Owed O;
		O.Id = "owe:" + EntryId + ":" + It.first;
		O.Who = It.first;
		O.Label = PctText(It.second.first) + " of your " + Title(Name) + (Place > 0 ? ", " + Ordinal(Place) : std::string());
		O.Cents = It.second.second;
		O.Since = World;
		O.Due = World + (It.first == Lou ? LouGraceDays * Day : DueHours * Hour);
		O.EntryId = EntryId;
		if (std::none_of(B.Owes.begin(), B.Owes.end(), [&O](const Owed& X) { return X.Id == O.Id; }))
		{
			B.Owes.push_back(O);
		}
	}
}

/** Lou's running net: at -$360 or worse he pauses 30 days; back above it, the pause ends (told by Step). */
void LouNet(Book& B, double World)
{
	const auto It = B.NetOnYou.find(Lou);
	if (It == B.NetOnYou.end())
	{
		return;
	}
	if (It->second <= LouPauseAtCents)
	{
		if (!Paused(B, Lou, World))
		{
			B.PausedUntil[Lou] = World + LouPauseDays * Day;
			Once(B, "lou-pause:" + F2(World), World);
		}
	}
	else if (Paused(B, Lou, World))
	{
		B.PausedUntil[Lou] = World;
		Once(B, "lou-unpause:" + F2(World), World);
	}
}

/** Sam's debt: a quarter of every cash and shift until it's paid. */
void Garnish(Book& B, Chips& Bankroll, life::State& L, Chips Income, const std::string& What, double World)
{
	for (Deal& D : B.Deals)
	{
		if (D.Debt <= 0 || Income <= 0)
		{
			continue;
		}
		const Chips Take = std::min(D.Debt, Income / 4);
		if (Take <= 0)
		{
			continue;
		}
		Move(Bankroll, L, World, "Sam: 25% of " + What, -Take);
		D.Debt -= Take;
		DealLine(D, World, -Take, "Garnished: " + What);
		Once(B, "garnish:" + D.Id + ":" + F2(World) + ":" + What, World);
		Vars Vg;
		Vg["Amount"] = Money(D.Debt);
		ToBurner(B, World, D.Debt > 0 ? Say(SamName, "garnish", Hash(What), Vg) : Say(SamName, "clear", 0));
		return;
	}
}

/** Sam's quarter on a site's pot (update 2 §1.2): RiverLine's forwards to the bankroll's; a rival's is core-deals' (G3). */
void Garnish(Book& B, const Pot& P, life::State& L, Chips Income, const std::string& What, double World)
{
	if (P.Cents && P.Site == 0)
	{
		Garnish(B, *P.Cents, L, Income, What, World);
	}
}

/** Swaps settle once both results are in: each side's share of the other's prize, and the difference owed once. */
void FinishSwap(Book& B, Swap& S, double World)
{
	if (S.State == SwapSettled || S.State == SwapLapsed || S.Mine < 0 || S.Theirs < 0)
	{
		return;
	}
	S.State = SwapSettled;
	const Chips IOwe = S.Mine * static_cast<Chips>(S.Permille) / 1000;
	const Chips TheyOwe = S.Theirs * static_cast<Chips>(S.Permille) / 1000;
	S.Net = TheyOwe - IOwe;
	const EventFacts F = FactsOf(S.EntryId);
	if (S.Net == 0)
	{
		Once(B, "even:" + S.EntryId + ":" + S.Partner, World);
		return;
	}
	Owed O;
	O.Id = "owe:swap:" + S.EntryId + ":" + S.Partner;
	O.Who = S.Partner;
	O.Label = PctText(S.Permille) + " swap, " + (F.Valid ? Title(F.Name) : std::string("the event"));
	O.Cents = S.Net < 0 ? -S.Net : S.Net;
	O.ToYou = S.Net > 0;
	O.Since = World;
	O.EntryId = S.EntryId;
	if (O.ToYou)
	{
		// Paid by personality (§2.5): Mei inside a day, Rick in two to nine days, anyone else in three.
		Rng R = SeedRng("swap:" + S.EntryId + ":" + S.Partner, net::DayOf(World));
		O.Due = World + (S.Partner == Mei ? 20.0 * Hour : S.Partner == Rick ? static_cast<double>(2 + R.Int(8)) * Day : DueHours * Hour);
	}
	else
	{
		O.Due = World + DueHours * Hour;
	}
	if (std::none_of(B.Owes.begin(), B.Owes.end(), [&O](const Owed& X) { return X.Id == O.Id; }))
	{
		B.Owes.push_back(O);
	}
}
} // namespace stake_detail

void OnRegister(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, double World)
{
	if (!Once(B, "reg:" + E.Id, World))
	{
		return;
	}
	const life::Funding& F = E.Fund;
	const std::string Name = Title(E.Name);
	if (F.From != life::Paid::Backer)
	{
		// Someone's stake on this event, and the player went in another way (their own money at the desk, a cover, the
		// roll): the stake is off, quietly. Nobody's money moved, and nobody texts that they missed you.
		for (Deal& D : B.Deals)
		{
			if (D.K == Kind::Event && D.EventId == E.Id && D.State == DealOpen)
			{
				D.State = DealLapsed;
				Once(B, "told:lapse:" + D.Id, World);
			}
		}
	}
	switch (F.From)
	{
	case life::Paid::Backer:
	{
		Deal* D = BackerDeal(B, E);
		if (!D)
		{
			// Registered on a standing offer (the desk's "Lou'll put you in"): taking the offer is the handshake.
			if (Offer* Of = FindOffer(B, F.Ref))
			{
				const Offer Took = *Of;
				if (Take(B, L, Took, World, true).empty())
				{
					D = BackerDeal(B, E);
				}
			}
		}
		if (!D || D->State != DealOpen)
		{
			break;
		}
		D->State = DealPlaying;
		D->Stake = F.Cents > 0 ? F.Cents : E.PaidCents;
		D->InField = InRoster(E, D->Backer);
		Move(Bankroll, L, World, D->Backer + ": stake, " + Name, D->Stake);
		FlowTo(B, "stake:" + E.Id, D->Backer, Name, -D->Stake, Memory::Staked, World);
		B.NetOnYou[D->Backer] -= D->Stake;
		Clear(B, E.Id);
		break;
	}
	case life::Paid::Package:
	{
		Deal* D = FindDeal(B, F.Ref);
		if (!D)
		{
			break;
		}
		if (D->RollNow < E.PaidCents)
		{
			TopUp(*D, World);
		}
		D->RollNow -= E.PaidCents;
		DealLine(*D, World, -E.PaidCents, Name + ": buy-in");
		Fund Fd;
		Fd.EventId = E.Id;
		Fd.DealId = D->Id;
		Fd.Bullet = 1;
		Fd.Cents = E.PaidCents;
		Fd.Name = Name;
		Fd.At = World;
		B.Funds.push_back(Fd);
		D->Events += 1;
		D->PeriodEvents += 1;
		TopUp(*D, World);
		Clear(B, E.Id);
		break;
	}
	case life::Paid::Own:
	{
		// The record against the player: a shot is more than 5% of the bankroll on one entry of their own money.
		const Chips Own = E.PaidCents - PiecesOn(B, E.Id, World).Money;
		if (Own > Bankroll / 20 && Own > 0)
		{
			Mark(B, World, MarkShot);
			B.Record.Shots += 1;
		}
		Collect(B, Bankroll, L, E.Id, E.Name, &E, World);
		for (Swap& S : B.Swaps)
		{
			if (S.EntryId == E.Id && S.State == SwapAgreed)
			{
				S.State = SwapLive;
			}
		}
		break;
	}
	case life::Paid::Sponsor:
	case life::Paid::Ticket:
		Clear(B, E.Id);
		break;
	}
}

void OnSettle(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, Chips Prize, double World)
{
	if (!Once(B, "settle:" + E.Id, World))
	{
		return;
	}
	const std::string Name = Title(E.Name);
	const life::Funding& F = E.Fund;
	if (F.From == life::Paid::Backer)
	{
		Deal* D = BackerDeal(B, E);
		if (D && D->State == DealPlaying)
		{
			// The buy-in back to the backer first, then the profit split (the player's cut; their share rounds down).
			const Chips Back = std::min(Prize, D->Stake);
			const Chips Rest = Prize - Back;
			const Chips Theirs = Back + Rest * static_cast<Chips>(1000 - D->T.CutPermille) / 1000;
			D->Prize = Prize;
			D->TheirShare = Theirs;
			D->State = DealSettled;
			Move(Bankroll, L, World, D->Backer + ": his share, " + Name, -Theirs);
			if (Theirs > 0)
			{
				FlowTo(B, "share:" + E.Id, D->Backer, Name, Theirs, Memory::PaidUp, World);
			}
			B.NetOnYou[D->Backer] += Theirs;
			if (D->Backer == Lou)
			{
				LouNet(B, World);
			}
		}
	}
	else if (F.From == life::Paid::Package)
	{
		if (Deal* D = FindDeal(B, F.Ref))
		{
			for (Fund& Fd : B.Funds)
			{
				if (Fd.EventId == E.Id && Fd.DealId == D->Id && !Fd.Settled)
				{
					Fd.Settled = true;
				}
			}
			if (Prize > 0)
			{
				Move(Bankroll, L, World, BackerName(*D) + "'s roll: " + Name, -Prize);
				D->RollNow += Prize;
				DealLine(*D, World, Prize, Name + ": " + Ordinal(E.Place));
			}
			D->Makeup = std::max<Chips>(0, D->Funded - D->RollNow);
		}
	}
	else
	{
		PiecesOwed(B, E.Id, E.Name, E.Place, Prize, World);
		const bool HadLou = std::any_of(B.Pieces.begin(), B.Pieces.end(), [&E](const Piece& P) { return P.EntryId == E.Id && P.Buyer == Lou; });
		if (HadLou)
		{
			LouNet(B, World);
		}
		for (Swap& S : B.Swaps)
		{
			if (S.EntryId == E.Id && (S.State == SwapLive || S.State == SwapAgreed))
			{
				S.Mine = Prize;
				FinishSwap(B, S, World);
			}
		}
		if (Prize > 0)
		{
			Garnish(B, Bankroll, L, Prize, "the " + Name + " cash", World);
		}
	}
}

void OnRefund(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, double World)
{
	if (!Once(B, "refund:" + E.Id, World))
	{
		return;
	}
	const std::string Name = Title(E.Name);
	const life::Funding& F = E.Fund;
	if (F.From == life::Paid::Backer)
	{
		if (Deal* D = BackerDeal(B, E))
		{
			if (D->State == DealPlaying)
			{
				Move(Bankroll, L, World, D->Backer + ": stake back, " + Name + " cancelled", -D->Stake);
				FlowTo(B, "stakeback:" + E.Id, D->Backer, Name, D->Stake, Memory::PaidUp, World);
				B.NetOnYou[D->Backer] += D->Stake;
			}
			D->State = DealRefunded;
		}
	}
	else if (F.From == life::Paid::Package)
	{
		if (Deal* D = FindDeal(B, F.Ref))
		{
			const auto It = std::find_if(B.Funds.begin(), B.Funds.end(), [&E, D](const Fund& Fd) { return Fd.EventId == E.Id && Fd.DealId == D->Id; });
			if (It != B.Funds.end())
			{
				D->RollNow += It->Cents;
				DealLine(*D, World, It->Cents, Name + ": cancelled, buy-in back");
				D->Events = std::max(0, D->Events - 1);
				D->PeriodEvents = std::max(0, D->PeriodEvents - 1);
				B.Funds.erase(It);
			}
			D->Makeup = std::max<Chips>(0, D->Funded - D->RollNow);
		}
	}
	else
	{
		// The buyers repaid, markup included, in one line; the pieces are gone. A swap is called off.
		Chips Total = 0;
		for (const Piece& P : B.Pieces)
		{
			if (P.EntryId == E.Id)
			{
				Total += P.Paid;
				FlowTo(B, "pieceback:" + E.Id + ":" + P.Buyer + ":" + I2(P.Paid), P.Buyer, Name, P.Paid, Memory::PaidUp, World);
				if (P.Buyer == Lou)
				{
					B.NetOnYou[Lou] += P.Paid;
				}
			}
		}
		Move(Bankroll, L, World, "Pieces bought back: " + Name, -Total);
		B.Pieces.erase(std::remove_if(B.Pieces.begin(), B.Pieces.end(), [&E](const Piece& P) { return P.EntryId == E.Id; }), B.Pieces.end());
	}
	Clear(B, E.Id);
}

void OnlineEntered(Book& B, Chips& Bankroll, life::State& L, const std::string& EventId, const std::string& Name, Chips BuyIn, const life::Funding& F,
	Chips BankrollBefore, double Tilt, double World)
{
	// The record against the player (§2.9): a shot (more than 5% of the bankroll, the player's own money), a tilted
	// registration (tilt at 0.7 or more). Nothing else is kept for an entry of the player's own money with nothing sold:
	// a grinder's hundreds of registrations a month write no keys.
	if (F.From != life::Paid::Package && BuyIn > 0 && BuyIn > BankrollBefore / 20)
	{
		Mark(B, World, MarkShot);
		B.Record.Shots += 1;
	}
	if (Tilt >= 0.7)
	{
		Mark(B, World, MarkTilt);
		B.Record.Tilts += 1;
	}
	if (F.From == life::Paid::Package)
	{
		Deal* D = FindDeal(B, F.Ref);
		if (!D)
		{
			return;
		}
		int Bullet = 1;
		for (const Fund& Fd : B.Funds)
		{
			Bullet += Fd.EventId == EventId && Fd.DealId == D->Id ? 1 : 0;
		}
		if (D->RollNow < BuyIn)
		{
			TopUp(*D, World);
		}
		D->RollNow -= BuyIn;
		DealLine(*D, World, -BuyIn, Name + (Bullet > 1 ? ": re-entry" : ": buy-in"));
		Fund Fd;
		Fd.EventId = EventId;
		Fd.DealId = D->Id;
		Fd.Bullet = Bullet;
		Fd.Cents = BuyIn;
		Fd.Name = Name;
		Fd.At = World;
		B.Funds.push_back(Fd);
		D->Events += 1;
		D->PeriodEvents += 1;
		TopUp(*D, World);
		return;
	}
	// The first bullet collects what the listing sold (a listing collects once: Listing::Collected).
	Collect(B, Bankroll, L, EventId, Name, nullptr, World);
}

void OnlineSettle(Book& B, Chips& Bankroll, life::State& L, const std::string& EventId, Chips Prize, double World)
{
	// The bullet this result belongs to: the package's oldest unsettled fund (Fund::Settled guards it, saved with it), or
	// the player's own entry.
	for (Fund& Fd : B.Funds)
	{
		if (Fd.EventId != EventId || Fd.Settled)
		{
			continue;
		}
		Fd.Settled = true;
		Deal* D = FindDeal(B, Fd.DealId);
		if (D && Prize > 0)
		{
			Move(Bankroll, L, World, BackerName(*D) + "'s roll: " + Fd.Name, -Prize);
			D->RollNow += Prize;
			DealLine(*D, World, Prize, Fd.Name + ": prize");
			if (D->By == Outfit::LanternRow && Prize >= 100 * D->T.CapOnline)
			{
				Mail(L, World, D->Id, "result.big", Vars(), LetterNote);
			}
		}
		if (D)
		{
			D->Makeup = std::max<Chips>(0, D->Funded - D->RollNow);
		}
		return;
	}
	// The player's own: the pieces' shares owed (each piece settles once: Piece::Settled), Sam's quarter of a cash.
	if (std::any_of(B.Pieces.begin(), B.Pieces.end(), [&EventId](const Piece& P) { return P.EntryId == EventId && !P.Settled; }))
	{
		const EventFacts Fc = FactsOf(EventId);
		PiecesOwed(B, EventId, Fc.Valid ? Fc.Name : EventId, 0, Prize, World);
	}
	if (Prize > 0 && std::any_of(B.Deals.begin(), B.Deals.end(), [](const Deal& D) { return D.Debt > 0; }) && Once(B, "osettle:" + EventId + ":" + F2(World), World))
	{
		const EventFacts Fc = FactsOf(EventId);
		Garnish(B, Bankroll, L, Prize, "the " + (Fc.Valid ? Title(Fc.Name) : std::string("tournament")) + " cash", World);
	}
}

void OnlineSettle(Book& B, const Pot& P, life::State& L, const std::string& EventId, Chips Prize, double World)
{
	// RiverLine's pot is the bankroll: today's path. A rival's (core-deals, G3) settles on its wallet with site lines only;
	// until then every event routes to RiverLine, so nothing reaches it.
	if (P.Cents && P.Site == 0)
	{
		OnlineSettle(B, *P.Cents, L, EventId, Prize, World);
	}
}

int AskedOn(const Book& B, const std::string& EventId)
{
	int N = 0;
	for (const Listing& Ls : B.Listings)
	{
		N += Ls.EventId == EventId ? (Ls.Collected ? Ls.Filled : Ls.Ask) : 0;
	}
	return N;
}

int FilledOn(const Book& B, const std::string& EventId, double World)
{
	int N = 0;
	for (const Listing& Ls : B.Listings)
	{
		N += Ls.EventId == EventId ? (Ls.Collected ? Ls.Filled : FilledBy(Ls, World)) : 0;
	}
	return N;
}

int SwappedOn(const Book& B, const std::string& EventId)
{
	int N = 0;
	for (const Swap& S : B.Swaps)
	{
		N += S.EntryId == EventId && S.State != SwapLapsed ? S.Permille : 0;
	}
	return N;
}

std::string List(Book& B, const View& V, const std::string& EventId, int Ask, int Markup, double Opens, double Closes)
{
	const EventFacts F = FactsOf(EventId);
	const double World = std::max(Opens, V.World);
	if (!F.Valid)
	{
		return "There's no such event.";
	}
	if (F.Closes <= World + (F.Live ? live::TravelMinutes : 0.0))
	{
		return "That one's already under way.";
	}
	if (F.BuyIn <= 0)
	{
		return "Nobody buys a piece of a freeroll.";
	}
	const int Step = F.Live ? 50 : 10;
	if (Ask < Step || Ask % Step != 0)
	{
		return F.Live ? "Pieces at the Embercrest go in fives: 5%, 10%, 15%." : "Pieces online go in whole percents.";
	}
	if (AskedOn(B, EventId) + SwappedOn(B, EventId) + Ask > MaxSoldPermille)
	{
		return "Keep at least 30% of yourself. Pieces and swaps stop at 70%.";
	}
	const int Cap = MarkupCap(F.Live, F.BuyIn);
	if (Markup < 1000 || Markup > Cap || Markup % 10 != 0)
	{
		return "Markup runs from 1.00 to " + MarkupText(Cap) + (F.Live ? " for a local event." : " online.");
	}
	if (const Deal* D = EventDeal(B, EventId))
	{
		return D->Backer + "'s putting you in that one. It's his action.";
	}
	if (const Deal* P = ActivePackage(B))
	{
		if ((F.Live ? P->T.CapLive : P->T.CapOnline) >= F.BuyIn)
		{
			return "That one's on " + BackerName(*P) + "'s roll. It's their action.";
		}
	}
	const int Today = net::DayOf(World);
	int Count = 0;
	for (const Listing& Ls : B.Listings)
	{
		Count += Ls.EventId == EventId ? 1 : 0;
	}
	Rng R = SeedRng("list:" + EventId + ":" + std::to_string(Count), Today);
	Listing Ls;
	Ls.EventId = EventId;
	Ls.Ask = Ask;
	Ls.Markup = Markup;
	Ls.Opens = World;
	Ls.Closes = std::min(Closes > World ? Closes : F.Closes, F.Closes);
	Ls.BuyIn = F.BuyIn;
	Ls.Live = F.Live;
	// The fills drawn once (S7): who buys, how much and when, between now and two days on (or registration, sooner).
	const double Window = std::max(30.0, std::min(Ls.Closes - World - 30.0, 2.0 * Day));
	const double Exp = ExpRoiOf(V, F.Live, F.BuyIn);
	int Left = Ask;
	auto At = [&R, World, Window]() { return World + std::floor(Window * R.Range(0.08, 0.92)); };
	if (F.Live)
	{
		struct Buyer
		{
			const char* Name;
			int Max;
			int Offset;
			bool ParOnly;
		};
		const Buyer Buyers[] = {{Lou, 100 + 50 * R.Int(3), 50, false}, {Park, 100, 0, false}, {Dre, 50, 0, false}, {Sal, 50, 0, true}};
		for (const Buyer& By : Buyers)
		{
			const Regard Rg = RegardOf(V, By.Name);
			const std::string Who = By.Name;
			if (!Rg.Known || Rg.Resentment >= 0.4f || WelchedOn(B, Who, World - 365.0 * Day))
			{
				continue;
			}
			if (Who == Park && ((!V.Rec || V.Rec->LiveEvents < 20) && V.RoiRank < 0.5))
			{
				continue;
			}
			if (Who == Park && Paused(B, Park, World))
			{
				continue;
			}
			const bool Friend = Who == Lou || Rg.Trust >= 0.4f;
			const int Limit = std::min(Cap, Ceiling(V, B, Who, true, F.BuyIn) + By.Offset);
			const bool Buys = By.ParOnly ? Markup == 1000 && (Exp >= 0.0 || Friend) : (Markup == 1000 && Friend) || (Markup <= Limit && (Exp >= 0.0 || Friend));
			const int Take = std::min(By.Max, Left) / Step * Step;
			const Chips Cost = PieceCost(F.BuyIn, Markup, Take);
			// Nobody puts more than a tenth of their roll into a piece. A world person's roll is known (V.Rolls, broke
			// included: World::HeroMoney never takes them below $0, so a piece they can't pay for would be money from
			// nowhere); without the world (the card room's desk) the cast buy as they always have.
			const auto Known = V.Rolls.find(Who);
			const bool CantPay = Known != V.Rolls.end() && Cost * 10 > Known->second;
			if (!Buys || Take <= 0 || CantPay)
			{
				// A pass, said out loud (Sal at a markup; Mrs. Park when it's rich for her).
				if ((By.ParOnly && Markup > 1000) || (Who == Park && Take > 0 && Markup > Limit))
				{
					Ls.Plan.push_back(std::make_tuple(At(), Who, 0));
				}
				continue;
			}
			Ls.Plan.push_back(std::make_tuple(At(), Who, Take));
			Left -= Take;
		}
	}
	// Off the market: strangers buy only when they believe the player wins (ExpRoi >= 0) and the price is under what they'd pay.
	const int Market = Ceiling(V, B, std::string(), F.Live, F.BuyIn);
	if (Exp >= 0.0 && Markup <= Market && Left > 0)
	{
		const double Appetite = std::min(1.0, std::max(0.0, static_cast<double>(Market - Markup) / 80.0 + 0.25)) * R.Range(0.75, 1.25);
		const double Renown = V.Rec ? V.Rec->Renown : 0.0;
		int Room = static_cast<int>(std::floor((F.Live ? 100.0 + 100.0 * Renown : 250.0 + 200.0 * Renown) * Appetite));
		Room = std::min(Left, Room / Step * Step);
		while (Room > 0)
		{
			const int Chunk = F.Live ? Step : std::min(Room, (1 + R.Int(5)) * Step);
			Ls.Plan.push_back(std::make_tuple(At(), std::string(), Chunk));
			Room -= Chunk;
			Left -= Chunk;
		}
	}
	std::stable_sort(Ls.Plan.begin(), Ls.Plan.end(), [](const std::tuple<double, std::string, int>& A, const std::tuple<double, std::string, int>& Bx) { return std::get<0>(A) < std::get<0>(Bx); });
	if (std::none_of(Ls.Plan.begin(), Ls.Plan.end(), [](const std::tuple<double, std::string, int>& P) { return std::get<2>(P) > 0; }))
	{
		// Nobody would buy: say why rather than list into silence.
		if (Exp < 0.0 && Markup > 1000)
		{
			return "Buyers read results, and yours don't sell at a markup yet. Par to the people who know you, maybe.";
		}
		if (Markup > Market)
		{
			return "Nobody's paying " + MarkupText(Markup) + ". Buyers expect up to " + MarkupText(Market) + ".";
		}
		return "Nobody's buying a piece of you yet. Backers read results. Go get some.";
	}
	Ls.Filled = FilledBy(Ls, World);
	B.Listings.push_back(Ls);
	return "";
}

std::string CancelListing(Book& B, const std::string& EventId, double World)
{
	(void)World;
	const size_t Before = B.Listings.size();
	B.Listings.erase(std::remove_if(B.Listings.begin(), B.Listings.end(), [&EventId](const Listing& Ls) { return Ls.EventId == EventId && !Ls.Collected; }), B.Listings.end());
	return B.Listings.size() == Before ? "You haven't sold any pieces of that one." : "";
}

std::string Projection(const Book& B, const std::string& EventId, double World)
{
	int Total = 0;
	int Now = 0;
	double Last = World;
	std::vector<std::string> Named;
	int Others = 0;
	for (const Listing& Ls : B.Listings)
	{
		if (Ls.EventId != EventId || Ls.Collected)
		{
			continue;
		}
		for (const std::tuple<double, std::string, int>& P : Ls.Plan)
		{
			if (std::get<2>(P) <= 0)
			{
				continue;
			}
			Total += std::get<2>(P);
			Now += std::get<0>(P) <= World ? std::get<2>(P) : 0;
			Last = std::max(Last, std::get<0>(P));
			if (std::get<1>(P).empty())
			{
				++Others;
			}
			else if (std::find(Named.begin(), Named.end(), std::get<1>(P)) == Named.end())
			{
				Named.push_back(std::get<1>(P));
			}
		}
	}
	if (Total <= 0)
	{
		return "Nobody's biting at that price. Nothing moves unless you play it.";
	}
	std::string Who;
	for (size_t I = 0; I < Named.size(); ++I)
	{
		Who += (I == 0 ? "" : I + 1 == Named.size() && Others == 0 ? " and " : ", ") + Named[I];
	}
	if (Others > 0)
	{
		Who += Named.empty() ? std::to_string(Others) + (Others == 1 ? " buyer off the board" : " buyers off the board") : " and " + std::to_string(Others) + (Others == 1 ? " other" : " others");
	}
	const std::string When = net::DayOf(Last) == net::DayOf(World) ? "tonight" : std::string("by ") + net::WeekdayName(net::DayOf(Last), true);
	return "Projected: " + PctText(Total) + " " + When + " (" + Who + "). " + (Now > 0 ? PctText(Now) + " spoken for so far." : "Collected when you register.");
}

std::string ProposeSwap(Book& B, const View& V, const std::string& EntryId, const std::string& Partner, int Permille)
{
	const EventFacts F = FactsOf(EntryId);
	if (!F.Valid || !F.Live)
	{
		return "Swaps are for the Embercrest's events.";
	}
	if (F.Closes <= V.World)
	{
		return "That one's already under way.";
	}
	const uint32_t Salt = Hash(EntryId + Partner);
	Vars Vs = EventVars(EntryId);
	Vs["Pct"] = PctText(Permille);
	if (!Plays(V, EntryId, Partner))
	{
		return Say(Partner, "swap.notplaying", Salt, Vs);
	}
	if (Permille != 50 && Permille != 100)
	{
		return "Swaps at the Embercrest are 5% or 10%.";
	}
	if (EventDeal(B, EntryId))
	{
		return "Someone's already putting you in that one.";
	}
	if (std::any_of(B.Swaps.begin(), B.Swaps.end(), [&](const Swap& S) { return S.EntryId == EntryId && S.Partner == Partner && S.State != SwapLapsed; }))
	{
		return "You're already swapped with " + Partner + " in that one.";
	}
	if (AskedOn(B, EntryId) + SwappedOn(B, EntryId) + Permille > MaxSoldPermille)
	{
		return "Keep at least 30% of yourself. Pieces and swaps stop at 70%.";
	}
	const Regard R = RegardOf(V, Partner);
	bool Yes = false;
	if (Partner == Mei)
	{
		Yes = (F.Key == "sunday" || F.Key == "bigstack") && (R.Respect >= 0.3f || R.FinalTable);
	}
	else if (Partner == Rick)
	{
		Yes = F.Key == "turbo" && R.Known && R.Resentment < 0.4f;
	}
	else
	{
		Yes = R.Known && R.Respect >= 0.3f && R.Resentment < 0.4f && Permille == 50;
	}
	if (!Yes || WelchedOn(B, Partner, V.World - 365.0 * Day))
	{
		return Say(Partner, "swap.no", Salt, Vs);
	}
	Swap S;
	S.EntryId = EntryId;
	S.Partner = Partner;
	S.Permille = Permille;
	S.State = SwapAgreed;
	S.Closes = F.Closes;
	S.Since = V.World;
	B.Swaps.push_back(S);
	FlowTo(B, "swap:" + EntryId + ":" + Partner, Partner, Title(F.Name), 0, Memory::Swapped, V.World);
	return "";
}

void SettleSwap(Book& B, Chips& Bankroll, life::State& L, const std::string& EntryId, const std::string& Partner, Chips TheirPrize, double World)
{
	(void)Bankroll;
	(void)L;
	for (Swap& S : B.Swaps)
	{
		if (S.EntryId != EntryId || S.Partner != Partner || S.State == SwapSettled || S.State == SwapLapsed)
		{
			continue;
		}
		if (TheirPrize < 0)
		{
			S.State = SwapLapsed;
			return;
		}
		S.Theirs = TheirPrize;
		FinishSwap(B, S, World);
		return;
	}
}

// ------------------------------------------------------------------ the calendar (§1.3)

namespace stake_detail
{
/** The player pays an owed record: one line, the counterparty's flow, the record kept. */
void PayOwed(Book& B, Chips& Bankroll, life::State& L, Owed& O, double At, std::vector<std::pair<std::string, std::string>>* Texts)
{
	const bool WasLate = O.State == OwedLate || O.State == OwedWelched;
	const bool Early = !WasLate && At <= O.Since + 24.0 * Hour;
	Move(Bankroll, L, At, O.Who + ": " + O.Label, -O.Cents);
	O.State = OwedPaid;
	O.PaidAt = At;
	B.Record.Honored += WasLate ? 0 : 1;
	FlowTo(B, "paid:" + O.Id, O.Who, O.Label, O.Cents, WasLate ? Memory::PaidLate : Memory::PaidUp, At);
	if (Early && !B.ToWorld.empty() && B.ToWorld.back().Id == "paid:" + O.Id)
	{
		B.ToWorld.back().Early = true;
	}
	if (Texts && O.Who != MarketName)
	{
		const std::string Said = Say(O.Who, Early ? "paid.early" : "paid", Hash(O.Id));
		if (!Said.empty())
		{
			Texts->push_back({O.Who, Said});
		}
	}
}

Vars OwedVars(const Owed& O)
{
	Vars V = EventVars(O.EntryId);
	V["Amount"] = Money(O.Cents);
	V["Day"] = WeekdayOf(O.Due);
	return V;
}

/** A package's period ends (§2.7): chop when up, makeup carried when not, the volume counted, tiers, the 90-day review. */
void PeriodEnd(Book& B, Chips& Bankroll, life::State& L, Deal& D, double At, const View* V, std::vector<std::pair<std::string, std::string>>& Texts)
{
	const Chips Profit = D.RollNow - D.Funded;
	D.Periods += 1;
	const bool Short = D.T.Volume > 0 && D.PeriodEvents < D.T.Volume;
	D.Missed += Short ? 1 : 0;
	const bool Lantern = D.By == Outfit::LanternRow;
	const bool SamDeal = D.By == Outfit::Sam;
	const bool Regular = D.By == Outfit::None;
	Vars Vs;
	if (Profit > 0)
	{
		int Cut = D.T.CutPermille;
		if (SamDeal && D.T.GhostsPerWeek > 0)
		{
			// Two sessions on his account a week, or the cut drops to 25%.
			const int Weeks = std::max(1, D.T.PeriodDays / 7);
			Cut = L.Ghosts - D.GhostsAtStart >= D.T.GhostsPerWeek * Weeks ? D.T.CutPermille : 250;
		}
		const Chips Theirs = Profit * static_cast<Chips>(1000 - Cut) / 1000;
		const Chips Yours = Profit - Theirs;
		Move(Bankroll, L, At, BackerName(D) + ": the chop, your " + PctText(Cut), Yours);
		DealLine(D, At, -Yours, "The chop: " + PctText(Cut) + " to you");
		DealLine(D, At, -Theirs, "The chop: the rest to " + BackerName(D));
		B.Record.Chops += 1;
		FlowTo(B, "chop:" + D.Id + ":" + std::to_string(D.Periods), D.Backer, "a package", Theirs, Memory::PaidUp, At);
		D.Funded = D.T.Roll;
		D.RollNow = D.T.Roll;
		D.Losing = 0;
		Vs["Amount"] = Money(Yours);
		Vs["Pct"] = PctText(Cut);
		if (Lantern)
		{
			Mail(L, At, D.Id, "chop", Vs, LetterChop);
		}
		else if (SamDeal)
		{
			Texts.push_back({SamName, Say(SamName, "chop", Hash(D.Id) + static_cast<uint32_t>(D.Periods))});
		}
		else
		{
			Texts.push_back({D.Backer, Say(D.Backer, "chop", Hash(D.Id))});
		}
	}
	else
	{
		D.Losing += 1;
		D.Makeup = std::max<Chips>(0, D.Funded - D.RollNow);
		Vs["Amount"] = Money(D.Makeup);
		if (Lantern)
		{
			Mail(L, At, D.Id, "nochop", Vs, LetterChop);
		}
		else if (SamDeal)
		{
			Texts.push_back({SamName, Say(SamName, "down", Hash(D.Id) + static_cast<uint32_t>(D.Periods), Vs)});
		}
	}
	if (Short && Lantern)
	{
		Vars Vv;
		Vv["Count"] = std::to_string(D.PeriodEvents);
		Vv["Volume"] = std::to_string(D.T.Volume);
		Mail(L, At, D.Id, "volume", Vv, LetterNote);
	}
	D.PeriodEvents = 0;
	D.GhostsAtStart = L.Ghosts;
	D.NextChop += static_cast<double>(D.T.PeriodDays) * Day;
	// A regular's package is one sixty-day term: what's left of the makeup is theirs to eat.
	if (Regular)
	{
		if (Profit <= 0)
		{
			Texts.push_back({D.Backer, Say(D.Backer, "end", Hash(D.Id))});
		}
		D.State = DealEnded;
		D.EndedAt = At;
		D.Makeup = 0;
		B.PausedUntil["regulars"] = std::max(B.PausedUntil["regulars"], At + 30.0 * Day); // the next regular's offer: a month on
		return;
	}
	if (!Lantern)
	{
		return;
	}
	// Fred's review with the chop (P1): the two worst spots since the last one, a real hand, the homework.
	{
		const coach::Review Rv = coach::ReviewOf(L.Study, "FoldEquityFred");
		life::Letter Ltr;
		Ltr.At = At;
		Ltr.Thread = D.Id;
		Ltr.Source = "stake";
		Ltr.From = "FoldEquityFred";
		Ltr.Subject = Rv.Subject;
		Ltr.Body = Rv.Body;
		Ltr.Kind = LetterReview;
		life::Post(L, Ltr);
		coach::Reviewed(L.Study, Rv, At);
	}
	// Tier moves: up after a profitable period whose shrunk ROI qualifies, down after two losing periods.
	const double Exp = V && V->Rec ? ExpRoiOf(*V, false, V->Rec->Abi) : -1.0;
	const int Earned = V && V->Rec ? TierFor(Exp) : 0;
	const int WasTier = D.Tier;
	if (Profit > 0 && Earned > D.Tier && D.Tier < 3)
	{
		D.Tier += 1;
	}
	else if (D.Losing >= 2 && D.Tier > 1)
	{
		D.Tier -= 1;
		D.Losing = 0;
	}
	// Only a move between tiers rewrites the terms: a cap or a cut won in a counter holds until the tier itself moves.
	const Terms Next = LanternTerms(D.Tier);
	if (D.Tier != WasTier)
	{
		const bool Up = D.Tier > WasTier;
		D.T.CutPermille = Next.CutPermille;
		D.T.CapOnline = Next.CapOnline;
		D.T.Volume = Next.Volume;
		// The roll resizes, the makeup carried: up, the backer adds the difference; down, the backer takes back what's
		// over the new size (nothing when the roll is already under it: the top-ups refill it to the new size).
		const Chips Delta = Up ? std::max<Chips>(0, Next.Roll - D.T.Roll) : -std::max<Chips>(0, D.RollNow - Next.Roll);
		D.T.Roll = Next.Roll;
		D.RollNow += Delta;
		D.Funded += Delta;
		if (Delta != 0)
		{
			DealLine(D, At, Delta, Up ? "The roll, sized up" : "The roll, sized down");
		}
		Vars Vt;
		Vt["Cap"] = net::BuyIn(Next.CapOnline);
		Vt["Pct"] = PctText(Next.CutPermille);
		Mail(L, At, D.Id, Up ? "tierup" : "tierdown", Vt, LetterNote);
	}
	// The 90-day review: cut loose over 150 buy-ins of makeup (forgiven), short of the volume twice, or a shrunk ROI under +5%.
	if (D.Periods % 3 == 0)
	{
		D.Makeup = std::max<Chips>(0, D.Funded - D.RollNow);
		const char* Why = D.Makeup > 150 * D.T.CapOnline ? "cut.makeup" : D.Missed >= 2 ? "cut.volume" : (V && V->Rec && Exp < LanternRowFloor) ? "cut.roi" : nullptr;
		if (Why)
		{
			D.State = DealEnded;
			D.EndedAt = At;
			DealLine(D, At, -D.Makeup, "Makeup forgiven");
			Mail(L, At, D.Id, Why, Vars(), LetterEnded);
			Mark(B, At, MarkCutLoose);
			B.PausedUntil["lanternrow"] = At + 180.0 * Day;
		}
	}
}
} // namespace stake_detail

void Step(Book& B, Chips& Bankroll, life::State& L, double From, double To, std::vector<std::pair<std::string, std::string>>& Texts, const View* V)
{
	if (To <= From)
	{
		return;
	}
	// Pieces filling: a text from each person who buys (or passes), when their fill comes due.
	for (Listing& Ls : B.Listings)
	{
		if (Ls.Collected)
		{
			continue;
		}
		for (size_t I = static_cast<size_t>(std::max(0, Ls.Told)); I < Ls.Plan.size(); ++I)
		{
			const std::tuple<double, std::string, int>& P = Ls.Plan[I];
			if (std::get<0>(P) > To)
			{
				break;
			}
			Ls.Told = static_cast<int>(I) + 1;
			const std::string& Who = std::get<1>(P);
			if (Who.empty() || Who == Dee || std::get<0>(P) > Ls.Closes)
			{
				continue;
			}
			Vars Vs = EventVars(Ls.EventId);
			Vs["Pct"] = PctText(std::get<2>(P));
			const std::string Said = Say(Who, std::get<2>(P) > 0 ? "piece.buy" : "piece.pass", Hash(Ls.EventId + Who), Vs);
			if (!Said.empty())
			{
				Texts.push_back({Who, Said});
			}
		}
		Ls.Filled = FilledBy(Ls, To);
	}
	// Listings never collected lapse when late registration closes: nobody paid, nothing moves.
	B.Listings.erase(std::remove_if(B.Listings.begin(), B.Listings.end(), [To](const Listing& Ls) { return !Ls.Collected && Ls.Closes <= To; }), B.Listings.end());
	// Event stakes and swaps for an event that went by without the player.
	for (Deal& D : B.Deals)
	{
		if (D.K == Kind::Event && D.State == DealOpen && D.Until > 0.0 && D.Until <= To)
		{
			D.State = DealLapsed;
			if (Once(B, "told:lapse:" + D.Id, To))
			{
				const std::string Said = Say(D.Backer, "lapse", Hash(D.Id), EventVars(D.EventId));
				if (!Said.empty())
				{
					Texts.push_back({D.Backer, Said});
				}
			}
		}
	}
	for (Swap& S : B.Swaps)
	{
		if (S.State == SwapAgreed && S.Closes > 0.0 && S.Closes <= To)
		{
			S.State = SwapLapsed;
		}
	}
	// What's owed, in due order.
	std::vector<size_t> Order;
	for (size_t I = 0; I < B.Owes.size(); ++I)
	{
		Order.push_back(I);
	}
	std::stable_sort(Order.begin(), Order.end(), [&B](size_t A, size_t Bx) { return B.Owes[A].Due < B.Owes[Bx].Due; });
	for (const size_t I : Order)
	{
		Owed& O = B.Owes[I];
		if (O.State == OwedPaid || O.State == OwedVoid || (O.State == OwedOpen && O.Due - 24.0 * Hour > To))
		{
			continue; // (most steps: nothing near due)
		}
		const Vars Vs = OwedVars(O);
		if (O.ToYou)
		{
			if (O.State == OwedOpen && O.Due <= To)
			{
				// They pay, by personality, at the time they said.
				const double At = std::max(From, O.Due);
				Move(Bankroll, L, At, O.Who + ": " + O.Label, O.Cents);
				O.State = OwedPaid;
				O.PaidAt = At;
				FlowTo(B, "paidyou:" + O.Id, O.Who, O.Label, -O.Cents, Memory::Swapped, At); // a swap settling their way
				const std::string Said = Say(O.Who, "swap.pays", Hash(O.Id), Vs);
				if (!Said.empty())
				{
					Texts.push_back({O.Who, Said});
				}
			}
			continue;
		}
		const double Welch = O.Due + (O.Who == Lou ? LouGraceDays : WelchDays) * Day;
		if (O.State == OwedOpen)
		{
			if (!O.Reminded && O.Due - 24.0 * Hour <= To && O.Due > From)
			{
				O.Reminded = true;
				const double At = std::max(From, O.Due - 24.0 * Hour);
				if (Bankroll < O.Cents + Reserve(L, At) && O.Who != MarketName)
				{
					const std::string Said = Say(O.Who, "reminder", Hash(O.Id), Vs);
					if (!Said.empty())
					{
						Texts.push_back({O.Who, Said});
					}
				}
			}
			if (O.Due <= To)
			{
				const double At = std::max(From, O.Due);
				if (Bankroll >= O.Cents + Reserve(L, At))
				{
					PayOwed(B, Bankroll, L, O, At, &Texts);
					continue;
				}
				// Late at the due time (§2.9): a text, a mark for 90 days, and they remember.
				O.State = OwedLate;
				B.Record.Late += 1;
				Mark(B, O.Due, MarkLate);
				FlowTo(B, "late:" + O.Id, O.Who, O.Label, 0, Memory::PaidLate, O.Due);
				if (O.Who == Park)
				{
					B.PausedUntil[Park] = O.Due + 90.0 * Day;
				}
				if (O.Who != MarketName)
				{
					const std::string Said = Say(O.Who, "late", Hash(O.Id), Vs);
					if (!Said.empty())
					{
						Texts.push_back({O.Who, Said});
					}
				}
			}
		}
		if (O.State == OwedLate || O.State == OwedWelched)
		{
			// Paid as soon as the money covers it, keeping the rent reserve.
			if (Bankroll >= O.Cents + Reserve(L, To))
			{
				PayOwed(B, Bankroll, L, O, To, &Texts);
				continue;
			}
			if (O.State == OwedLate && Welch <= To)
			{
				O.State = OwedWelched;
				B.Record.Welched += 1;
				Mark(B, Welch, MarkWelched);
				FlowTo(B, "welch:" + O.Id, O.Who, O.Label, 0, Memory::Welched, Welch);
				if (O.Who != MarketName)
				{
					const std::string Said = Say(O.Who, "welch", Hash(O.Id), Vs);
					if (!Said.empty())
					{
						Texts.push_back({O.Who, Said});
					}
				}
			}
		}
	}
	// Packages: each period's end that falls in the step.
	for (Deal& D : B.Deals)
	{
		for (int Guard = 0; Guard < 24 && D.K == Kind::Package && D.State == DealOpen && D.NextChop > 0.0 && D.NextChop <= To; ++Guard)
		{
			const double At = D.NextChop;
			if (!Once(B, "chop:" + D.Id + ":" + std::to_string(D.Periods + 1), At))
			{
				D.NextChop += static_cast<double>(D.T.PeriodDays) * Day;
				continue;
			}
			PeriodEnd(B, Bankroll, L, D, At, V, Texts);
		}
		// Lantern Row's reminder, three days before chop day.
		if (D.K == Kind::Package && D.State == DealOpen && D.By == Outfit::LanternRow && D.NextChop - 3.0 * Day > From && D.NextChop - 3.0 * Day <= To &&
			Once(B, "remind:" + D.Id + ":" + std::to_string(D.Periods + 1), To))
		{
			Vars Vs;
			Vs["Day"] = WeekdayOf(D.NextChop);
			Vs["Count"] = std::to_string(D.PeriodEvents);
			Vs["Volume"] = std::to_string(D.T.Volume);
			Vs["Amount"] = Money(D.RollNow);
			Mail(L, std::max(From, D.NextChop - 3.0 * Day), D.Id, "reminder", Vs, LetterNote);
		}
	}
	// Sam's debt: a quarter of every shift's pay (ShiftLink, ledger kind 1) since the last step.
	if (std::any_of(B.Deals.begin(), B.Deals.end(), [](const Deal& D) { return D.Debt > 0; }))
	{
		for (const life::LedgerEntry& X : std::vector<life::LedgerEntry>(L.Ledger))
		{
			if (X.Kind == 1 && X.Amount > 0 && X.At > From - 1.0 && X.At <= To && Once(B, "shift:" + F2(X.At) + ":" + X.Label, To))
			{
				Garnish(B, Bankroll, L, X.Amount, "the shift", To);
			}
		}
	}
	// Results, told once (looked at once a minute of world time): the backer's text after the cage, the pieces' owed, a brick,
	// Lou's pause.
	if (static_cast<long long>(std::floor(From)) != static_cast<long long>(std::floor(To)))
	{
		for (const Deal& D : B.Deals)
		{
			if (D.K == Kind::Event && D.State == DealSettled && Once(B, "told:" + D.Id, To))
			{
				Vars Vs = EventVars(D.EventId);
				Vs["Amount"] = Money(D.Prize - D.TheirShare);
				const char* K = D.Prize <= 0 ? "result.bust" : D.Prize >= 4 * D.Stake ? "result.big" : "result.cash";
				const std::string Said = Say(D.Backer, K, Hash(D.Id), Vs);
				if (!Said.empty())
				{
					Texts.push_back({D.Backer, Said});
				}
			}
		}
		for (const Owed& O : B.Owes)
		{
			if (O.Since <= To && O.Who != MarketName && Once(B, "told:" + O.Id, To))
			{
				Vars Vs = OwedVars(O);
				int Permille = 0;
				for (const Piece& P : B.Pieces)
				{
					Permille += P.EntryId == O.EntryId && Display(P.Buyer) == O.Who ? P.Permille : 0;
				}
				Vs["Pct"] = PctText(Permille);
				const size_t Comma = O.Label.rfind(", ");
				Vs["Place"] = Comma == std::string::npos ? std::string("finish") : O.Label.substr(Comma + 2);
				const bool Swapped = O.Id.rfind("owe:swap:", 0) == 0;
				const char* K = Swapped ? (O.ToYou ? "swap.owed" : "swap.owe") : "piece.owed";
				const std::string Said = Say(O.Who, K, Hash(O.Id), Vs);
				if (!Said.empty() && O.State != OwedPaid)
				{
					Texts.push_back({O.Who, Said});
				}
			}
		}
		for (auto& Kv : B.Done)
		{
			const std::string& Key = Kv.first;
			if (Key.rfind("brick:", 0) == 0 && !Did(B, "told:" + Key))
			{
				const std::string Who = Key.substr(Key.rfind(':') + 1);
				if (Who != MarketName)
				{
					const std::string Said = Say(Who, "piece.bust", Hash(Key));
					if (!Said.empty())
					{
						Texts.push_back({Who, Said});
					}
				}
			}
			else if (Key.rfind("even:", 0) == 0 && !Did(B, "told:" + Key))
			{
				const std::string Who = Key.substr(Key.rfind(':') + 1);
				const std::string Said = Say(Who, "swap.even", Hash(Key));
				if (!Said.empty())
				{
					Texts.push_back({Who, Said});
				}
			}
			else if ((Key.rfind("lou-pause:", 0) == 0 || Key.rfind("lou-unpause:", 0) == 0) && !Did(B, "told:" + Key))
			{
				Texts.push_back({Lou, Say(Lou, Key.rfind("lou-pause:", 0) == 0 ? "paused" : "unpaused", Hash(Key))});
			}
		}
		std::vector<std::string> Told;
		for (const auto& Kv : B.Done)
		{
			const std::string& Key = Kv.first;
			if ((Key.rfind("brick:", 0) == 0 || Key.rfind("even:", 0) == 0 || Key.rfind("lou-pause:", 0) == 0 || Key.rfind("lou-unpause:", 0) == 0) && !Did(B, "told:" + Key))
			{
				Told.push_back("told:" + Key);
			}
		}
		for (const std::string& T : Told)
		{
			B.Done[T] = To;
		}
	}
	// Offers that ran out; an outfit's door stays shut a while after one is let go.
	for (const Offer& O : B.Offers)
	{
		if (O.Expires > 0.0 && O.Expires <= To)
		{
			Once(B, "offer:" + O.Id, To);
			if (O.By == Outfit::LanternRow)
			{
				B.PausedUntil["lanternrow"] = std::max(B.PausedUntil["lanternrow"], To + 30.0 * Day);
			}
			else if (O.By == Outfit::Sam)
			{
				B.PausedUntil["sam"] = std::max(B.PausedUntil["sam"], To + 14.0 * Day);
			}
			else if (O.K == Kind::Package)
			{
				B.PausedUntil["regulars"] = std::max(B.PausedUntil["regulars"], To + 30.0 * Day);
			}
			else if (O.Backer == Dee)
			{
				Once(B, "dee-piece", To);
			}
		}
	}
	B.Offers.erase(std::remove_if(B.Offers.begin(), B.Offers.end(), [To](const Offer& O) { return O.Expires > 0.0 && O.Expires <= To; }), B.Offers.end());
	if (static_cast<long long>(std::floor(From / 60.0)) == static_cast<long long>(std::floor(To / 60.0)))
	{
		B.SteppedTo = To;
		return; // the pruning runs once an hour of world time (the calendar steps every frame)
	}
	// Pruning (§7.4): paid or settled records after 60 days, done keys after 120 once their record is closed, finished
	// deals after 120 days (a package keeps its last 40 lines), flows once applied, marks after 400 days.
	const double Old = To - 120.0 * Day;
	B.Owes.erase(std::remove_if(B.Owes.begin(), B.Owes.end(), [To](const Owed& O) { return (O.State == OwedPaid || O.State == OwedVoid) && O.PaidAt < To - 60.0 * Day; }), B.Owes.end());
	B.Swaps.erase(std::remove_if(B.Swaps.begin(), B.Swaps.end(), [To](const Swap& S) { return (S.State == SwapSettled || S.State == SwapLapsed) && S.Since < To - 60.0 * Day; }), B.Swaps.end());
	B.Pieces.erase(std::remove_if(B.Pieces.begin(), B.Pieces.end(),
					   [&B, To](const Piece& P) {
						   return P.Settled && std::none_of(B.Owes.begin(), B.Owes.end(), [&P](const Owed& O) { return O.EntryId == P.EntryId && O.State != OwedPaid; }) &&
							   B.Done.count("settle:" + P.EntryId) > 0 && B.Done.at("settle:" + P.EntryId) < To - 60.0 * Day;
					   }),
		B.Pieces.end());
	B.Funds.erase(std::remove_if(B.Funds.begin(), B.Funds.end(), [To](const Fund& F) { return F.Settled && F.At < To - 2.0 * Day; }), B.Funds.end()); // (bullets count within the event)
	B.Deals.erase(std::remove_if(B.Deals.begin(), B.Deals.end(),
					  [Old](const Deal& D) {
						  const bool Closed = D.State == DealSettled || D.State == DealRefunded || D.State == DealLapsed || ((D.State == DealEnded || D.State == DealWalked) && D.Debt <= 0);
						  const double When = D.K == Kind::Package ? D.EndedAt : std::max(D.Since, D.Until);
						  return Closed && When < Old;
					  }),
		B.Deals.end());
	B.ToWorld.erase(std::remove_if(B.ToWorld.begin(), B.ToWorld.end(), [](const Flow& F) { return F.Applied; }), B.ToWorld.end());
	B.Record.Marks.erase(std::remove_if(B.Record.Marks.begin(), B.Record.Marks.end(), [To](const std::pair<double, int>& M) { return M.first < To - 400.0 * Day; }), B.Record.Marks.end());
	for (auto It = B.Done.begin(); It != B.Done.end();)
	{
		const std::string& Key = It->first;
		bool Open = It->second >= Old || Key == "dee-piece" || Key == "lanternrow-ever";
		if (!Open && (Key.rfind("reg:", 0) == 0 || Key.rfind("settle:", 0) == 0 || Key.rfind("refund:", 0) == 0))
		{
			const std::string Entry = Key.substr(Key.find(':') + 1);
			const life::LiveEntry* E = live::EntryFor(L, Entry);
			Open = (E && E->State == life::LiveEntry::Registered) || std::any_of(B.Owes.begin(), B.Owes.end(), [&Entry](const Owed& O) { return O.EntryId == Entry; });
		}
		else if (!Open && Key.rfind("told:owe:", 0) == 0)
		{
			// A debt that's still on the books (a welch can sit for months) keeps its "told": nobody texts it twice.
			const std::string Id = Key.substr(5);
			Open = std::any_of(B.Owes.begin(), B.Owes.end(), [&Id](const Owed& O) { return O.Id == Id; });
		}
		It = Open ? std::next(It) : B.Done.erase(It);
	}
	B.SteppedTo = To;
}

std::string Chop(Book& B, Chips& Bankroll, life::State& L, const std::string& DealId, double World)
{
	Deal* D = FindDeal(B, DealId);
	if (!D || D->K != Kind::Package || D->State != DealOpen)
	{
		return "There's no deal to chop.";
	}
	if (D->By == Outfit::Sam)
	{
		return Say(SamName, "nochop", 0);
	}
	const Chips Profit = D->RollNow - D->Funded;
	const Chips Cap = std::max<Chips>(1, D->T.CapOnline);
	if (Profit < 20 * Cap)
	{
		return D->By == Outfit::LanternRow ? Say(LanternRow, "nochop", 0) : "Up twenty buy-ins, they'll chop early. You're not there.";
	}
	if (!Once(B, "chop:" + D->Id + ":early:" + F2(World), World))
	{
		return "";
	}
	// An early chop: the period's split now, the period itself runs on to its date.
	const Chips Theirs = Profit * static_cast<Chips>(1000 - D->T.CutPermille) / 1000;
	const Chips Yours = Profit - Theirs;
	Move(Bankroll, L, World, BackerName(*D) + ": the chop, your " + PctText(D->T.CutPermille), Yours);
	DealLine(*D, World, -Yours, "Early chop: " + PctText(D->T.CutPermille) + " to you");
	DealLine(*D, World, -Theirs, "Early chop: the rest to " + BackerName(*D));
	FlowTo(B, "chop:" + D->Id + ":early:" + F2(World), D->Backer, "a package", Theirs, Memory::PaidUp, World);
	B.Record.Chops += 1;
	D->Funded = D->T.Roll;
	D->RollNow = D->T.Roll;
	D->Makeup = 0;
	D->Losing = 0;
	return "";
}

std::string WalkAway(Book& B, Chips& Bankroll, life::State& L, const std::string& DealId, double World)
{
	Deal* D = FindDeal(B, DealId);
	if (!D || D->K != Kind::Package || D->State != DealOpen)
	{
		return "There's no deal to walk away from.";
	}
	const Chips Profit = D->RollNow - D->Funded;
	// Leaving a stable, even square, closes its door for a month: nobody quits on every up-swing and signs again the
	// next morning (the chop at each peak would be a free option on the backer's money).
	auto Door = [&B, D, World]() {
		const std::string Key = D->By == Outfit::LanternRow ? "lanternrow" : D->By == Outfit::Sam ? "sam" : "regulars";
		B.PausedUntil[Key] = std::max(B.PausedUntil[Key], World + 30.0 * Day);
	};
	if (Profit > 0)
	{
		// Up: the chop first (the profit is the player's to split), then the deal ends clean.
		const Chips Theirs = Profit * static_cast<Chips>(1000 - D->T.CutPermille) / 1000;
		Move(Bankroll, L, World, BackerName(*D) + ": the chop, your " + PctText(D->T.CutPermille), Profit - Theirs);
		DealLine(*D, World, -(Profit - Theirs), "Final chop: " + PctText(D->T.CutPermille) + " to you");
		DealLine(*D, World, -Theirs, "Final chop: the rest to " + BackerName(*D));
		FlowTo(B, "chop:" + D->Id + ":final", D->Backer, "a package", Theirs, Memory::PaidUp, World);
		B.Record.Chops += 1;
		D->State = DealEnded;
		D->EndedAt = World;
		D->Makeup = 0;
		if (D->By == Outfit::LanternRow)
		{
			Vars Vu;
			Vu["Amount"] = Money(Profit - Theirs);
			Vu["Pct"] = PctText(D->T.CutPermille);
			Mail(L, World, D->Id, "closed", Vu, LetterEnded);
		}
		Door();
		return "";
	}
	D->Makeup = std::max<Chips>(0, D->Funded - D->RollNow);
	D->State = DealWalked;
	D->EndedAt = World;
	if (D->Makeup <= 0)
	{
		D->State = DealEnded;
		Door();
		return "";
	}
	// Walked from makeup (§2.9): nothing owed (except Sam's), the mark for a year, the backer remembers.
	B.Record.Walked += 1;
	Mark(B, World, MarkWalked);
	FlowTo(B, "walk:" + D->Id, D->Backer, "a package", 0, Memory::WalkedOut, World);
	Vars Vs;
	Vs["Amount"] = Money(D->Makeup);
	if (D->T.MakeupIsDebt)
	{
		D->Debt = D->Makeup;
		DealLine(*D, World, 0, "Walked: " + Money(D->Makeup) + " of makeup is a debt");
		ToBurner(B, World, Say(SamName, "walk", Hash(D->Id), Vs));
	}
	else
	{
		DealLine(*D, World, -D->Makeup, "Walked: makeup forgiven");
	}
	if (D->By == Outfit::LanternRow)
	{
		B.PausedUntil["lanternrow"] = World + 365.0 * Day;
		Mail(L, World, D->Id, "walked", Vs, LetterEnded);
	}
	else
	{
		Door();
	}
	return "";
}

std::string Pay(Book& B, Chips& Bankroll, life::State& L, const std::string& OwedId, double World)
{
	for (Owed& O : B.Owes)
	{
		if (O.Id != OwedId)
		{
			continue;
		}
		if (O.ToYou)
		{
			return O.Who + " owes you this one.";
		}
		if (O.State == OwedPaid || O.State == OwedVoid)
		{
			return "That's paid.";
		}
		if (Bankroll < O.Cents)
		{
			return "You don't have " + Money(O.Cents) + ".";
		}
		PayOwed(B, Bankroll, L, O, World, nullptr);
		return "";
	}
	return "You don't owe anyone that.";
}

void NoteConduct(Book& B, int K, double World)
{
	Mark(B, World, K);
	B.Record.Seeded = true;
}

// ------------------------------------------------------------------ the room (§1.5)

std::vector<std::pair<std::string, int>> RailFor(const Book& B, const std::string& EntryId, const std::set<std::string>& OutTonight)
{
	std::map<std::string, int> Who;
	std::set<std::string> Field;
	for (const Deal& D : B.Deals)
	{
		if (D.K == Kind::Event && D.EventId == EntryId && (D.State == DealPlaying || D.State == DealSettled))
		{
			Who[D.Backer] += 1000;
			if (D.InField)
			{
				Field.insert(D.Backer);
			}
		}
	}
	for (const Piece& P : B.Pieces)
	{
		if (P.EntryId == EntryId && !P.Buyer.empty())
		{
			Who[P.Buyer] += P.Permille;
			if (P.InField)
			{
				Field.insert(P.Buyer);
			}
		}
	}
	for (const Swap& S : B.Swaps)
	{
		if (S.EntryId == EntryId && S.State != SwapLapsed)
		{
			Who[S.Partner] += S.Permille;
			Field.insert(S.Partner); // a swap partner is in the field by definition
		}
	}
	std::vector<std::pair<std::string, int>> Out;
	for (const auto& It : Who)
	{
		// Dee deals the Sunday from the box; the outfits and the market aren't at the rail.
		if (It.first == Dee || !WorldPerson(It.first))
		{
			continue;
		}
		if (Field.count(It.first) > 0 && OutTonight.count(It.first) == 0)
		{
			continue; // still in tonight's field: they play straight at their own table (R7)
		}
		Out.push_back(It);
	}
	std::stable_sort(Out.begin(), Out.end(), [](const std::pair<std::string, int>& A, const std::pair<std::string, int>& Bx) {
		return A.second != Bx.second ? A.second > Bx.second : A.first < Bx.first;
	});
	if (Out.size() > 2)
	{
		Out.resize(2);
	}
	return Out;
}

std::string RailLine(const Book& B, const std::string& EntryId, const std::string& Who, int Moment, uint32_t Salt)
{
	(void)B;
	(void)EntryId;
	std::string Kind;
	switch (static_cast<talk::Moment>(Moment))
	{
	case talk::Moment::Bubble: Kind = "rail.bubble"; break;
	case talk::Moment::InTheMoney: Kind = "rail.money"; break;
	case talk::Moment::DoubleUp: Kind = "rail.doubleup"; break;
	case talk::Moment::FinalTable: Kind = "rail.final"; break;
	case talk::Moment::Lose: Kind = "rail.bust"; break;
	case talk::Moment::Win: Kind = "rail.win"; break;
	case talk::Moment::BadBeat:
	case talk::Moment::Suckout:
	case talk::Moment::Cooler: Kind = "rail.beat"; break;
	case talk::Moment::Rail: Kind = "rail.idle"; break;
	case talk::Moment::Break: Kind = "break"; break;
	default: return std::string();
	}
	return Say(Who, Kind, Salt);
}

std::vector<life::Mark> Marks(const Book& B, double From, double To)
{
	std::vector<life::Mark> Out;
	auto Add = [&Out, From, To](double At, int Date, const std::string& Label, const std::string& Where) {
		if (At >= From && At <= To)
		{
			life::Mark M;
			M.At = At;
			M.Kind = Date;
			M.Label = Label;
			M.Where = Where;
			Out.push_back(M);
		}
	};
	for (const Owed& O : B.Owes)
	{
		if (O.State == OwedOpen || O.State == OwedLate)
		{
			Add(O.Due, O.ToYou ? DateOwed : DatePay, O.ToYou ? O.Who + " owes you " + Money(O.Cents) : Money(O.Cents) + " to " + O.Who, "Backing");
		}
	}
	for (const Deal& D : B.Deals)
	{
		if (D.K == Kind::Package && D.State == DealOpen)
		{
			Add(D.NextChop, DateChop, "Chop day: " + BackerName(D), "Backing");
		}
		else if (D.K == Kind::Event && D.State == DealOpen)
		{
			const live::Occurrence O = live::FindOccurrence(D.EventId);
			if (O.Valid())
			{
				Add(O.Start, DateBacked, D.Backer + "'s putting you in: " + O.T->Short, "The Embercrest");
			}
		}
	}
	for (const Listing& Ls : B.Listings)
	{
		if (!Ls.Collected)
		{
			const EventFacts F = FactsOf(Ls.EventId);
			Add(F.Valid ? F.Start : Ls.Closes, DateSold, "Pieces sold: " + (F.Valid ? F.Short : std::string("an event")), F.Live ? "The Embercrest" : "RiverLine");
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const life::Mark& A, const life::Mark& Bx) { return A.At < Bx.At; });
	return Out;
}

// ------------------------------------------------------------------ what the pages show

Chips StakedRoll(const Book& B)
{
	const Deal* P = ActivePackage(B);
	return P ? P->RollNow : 0;
}

Chips MakeupOf(const Book& B)
{
	const Deal* P = ActivePackage(B);
	return P ? std::max<Chips>(0, P->Funded - P->RollNow) : 0;
}

Chips YouOwe(const Book& B)
{
	Chips N = 0;
	for (const Owed& O : B.Owes)
	{
		N += !O.ToYou && (O.State == OwedOpen || O.State == OwedLate || O.State == OwedWelched) ? O.Cents : 0;
	}
	for (const Deal& D : B.Deals)
	{
		N += D.Debt;
	}
	return N;
}

Chips OwedToYou(const Book& B)
{
	Chips N = 0;
	for (const Owed& O : B.Owes)
	{
		N += O.ToYou && O.State == OwedOpen ? O.Cents : 0;
	}
	return N;
}

const Deal* ActivePackage(const Book& B)
{
	for (const Deal& D : B.Deals)
	{
		if (D.K == Kind::Package && D.State == DealOpen)
		{
			return &D;
		}
	}
	return nullptr;
}

std::string BackerName(const Deal& D)
{
	switch (D.By)
	{
	case Outfit::LanternRow: return LanternRow;
	case Outfit::Sam: return SamName;
	case Outfit::FinchSchool: return Finch;
	case Outfit::None: break;
	}
	return D.Backer;
}

std::string OfferFrom(const Offer& O)
{
	return O.By == Outfit::LanternRow ? std::string(LanternRow) : O.By == Outfit::Sam ? std::string(SamName) : O.Backer;
}

std::string TermsLine(const Offer& O)
{
	const std::string Dot = " \xC2\xB7 ";
	const EventFacts F = FactsOf(O.EntryId);
	switch (O.K)
	{
	case Kind::Event:
		return PctText(O.Permille) + " of the " + (F.Valid ? F.Name : std::string("event")) + Dot + "buy-in back first, then " +
			(O.T.CutPermille == 500 ? std::string("50/50") : PctText(O.T.CutPermille) + " of the rest to you") + Dot + "nothing carried over" + Dot + "one a week";
	case Kind::Pieces:
		return PctText(O.Permille) + " of your " + (F.Valid ? F.Short : std::string("event")) + " at " + MarkupText(O.T.MarkupPermille) + Dot +
			Money(PieceCost(O.BuyIn, O.T.MarkupPermille, O.Permille)) + " when you register" + Dot + "they get " + PctText(O.Permille) + " of the prize";
	case Kind::Swap:
		return PctText(O.Permille) + " each in the " + (F.Valid ? F.Short : std::string("event")) + Dot + "if you both play" + Dot + "the difference settles after";
	case Kind::Package:
	{
		std::string Out = Money(O.T.Roll) + " roll" + Dot + "events up to " + net::BuyIn(O.T.CapOnline) + (O.T.CapLive > 0 ? " online, " + net::BuyIn(O.T.CapLive) + " live" : std::string()) +
			Dot + PctText(O.T.CutPermille) + " of the profit to you after makeup" + Dot + std::to_string(O.T.PeriodDays) + "-day periods";
		if (O.T.Volume > 0)
		{
			Out += Dot + std::to_string(O.T.Volume) + " events a period";
		}
		if (O.T.MakeupIsDebt)
		{
			Out += Dot + "makeup is a debt if you quit: 25% of every cash and shift";
		}
		else
		{
			Out += Dot + "makeup isn't a debt";
		}
		if (O.T.GhostsPerWeek > 0)
		{
			Out += Dot + std::to_string(O.T.GhostsPerWeek) + " sessions on his account a week, or 25%";
		}
		return Out;
	}
	}
	return std::string();
}

std::string DealLine(const Deal& D)
{
	const std::string Dot = " \xC2\xB7 ";
	if (D.K == Kind::Event)
	{
		const EventFacts F = FactsOf(D.EventId);
		return std::string("100% of the ") + (F.Valid ? F.Name : std::string("event")) + Dot + "buy-in back first, then " +
			(D.T.CutPermille == 500 ? std::string("50/50") : PctText(D.T.CutPermille) + " of the rest to you");
	}
	std::string Out = Money(D.T.Roll) + " roll" + Dot + "up to " + net::BuyIn(D.T.CapOnline) + Dot + PctText(D.T.CutPermille) + " to you after makeup";
	if (D.T.Volume > 0)
	{
		Out += Dot + std::to_string(D.PeriodEvents) + " of " + std::to_string(D.T.Volume) + " events";
	}
	return Out;
}

bool Anything(const Book& B)
{
	return !B.Offers.empty() || !B.Deals.empty() || !B.Listings.empty() || !B.Swaps.empty() || !B.Owes.empty() || !B.Pieces.empty();
}

std::string Speak(const std::string& Who, const std::string& Kind, uint32_t Salt, const std::map<std::string, std::string>& Fills)
{
	return Say(Who, Kind, Salt, Fills);
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (const Copy& C : Lines)
	{
		Out.push_back({std::string(C.Who) + "/" + C.Kind, C.Text});
	}
	return Out;
}

std::vector<std::pair<std::string, std::string>> LetterInventory()
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (const stake_detail::Letter& L : Letters)
	{
		Out.push_back({std::string("subject/") + L.Kind, L.Subject});
		Out.push_back({std::string("body/") + L.Kind, L.Body});
	}
	return Out;
}

// ------------------------------------------------------------------ the save (§7.4, §7.5)

namespace stake_detail
{
// LinesRead bits.
constexpr uint32_t ReadAny = 1u << 0;
constexpr uint32_t ReadConduct = 1u << 1;

void Terms6(const Terms& T, std::string& Out)
{
	Out += "\t" + std::to_string(T.CutPermille) + "\t" + std::to_string(T.MarkupPermille) + "\t" + I2(T.CapOnline) + "\t" + I2(T.CapLive) + "\t" + I2(T.Roll) + "\t" +
		std::to_string(T.PeriodDays) + "\t" + std::to_string(T.Volume) + "\t" + std::to_string(T.DueDays) + "\t" + std::to_string(T.GhostsPerWeek) + "\t" + (T.BuyInBack ? "1" : "0") +
		"\t" + (T.MakeupIsDebt ? "1" : "0");
}

Terms ReadTerms(const std::vector<std::string>& P, size_t At)
{
	Terms T;
	if (P.size() < At + 11)
	{
		return T;
	}
	T.CutPermille = Int(P[At]);
	T.MarkupPermille = Int(P[At + 1]);
	T.CapOnline = Cents(P[At + 2]);
	T.CapLive = Cents(P[At + 3]);
	T.Roll = Cents(P[At + 4]);
	T.PeriodDays = Int(P[At + 5]);
	T.Volume = Int(P[At + 6]);
	T.DueDays = Int(P[At + 7]);
	T.GhostsPerWeek = Int(P[At + 8]);
	T.BuyInBack = P[At + 9] == "1";
	T.MakeupIsDebt = P[At + 10] == "1";
	return T;
}

std::string PlanText(const std::vector<std::tuple<double, std::string, int>>& Plan)
{
	std::string Out;
	for (const std::tuple<double, std::string, int>& P : Plan)
	{
		if (!Out.empty())
		{
			Out.push_back(';');
		}
		Out += F2(std::get<0>(P)) + ":" + std::to_string(std::get<2>(P)) + ":";
		for (const char Ch : std::get<1>(P))
		{
			if (Ch == ';' || Ch == '\\')
			{
				Out.push_back('\\');
			}
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::vector<std::tuple<double, std::string, int>> ReadPlan(const std::string& Text)
{
	std::vector<std::tuple<double, std::string, int>> Out;
	size_t I = 0;
	while (I < Text.size())
	{
		std::string Item;
		while (I < Text.size() && Text[I] != ';')
		{
			if (Text[I] == '\\' && I + 1 < Text.size())
			{
				Item.push_back(Text[++I]);
			}
			else
			{
				Item.push_back(Text[I]);
			}
			++I;
		}
		++I;
		const size_t A = Item.find(':');
		const size_t Bx = A == std::string::npos ? std::string::npos : Item.find(':', A + 1);
		if (Bx == std::string::npos)
		{
			continue;
		}
		Out.push_back(std::make_tuple(Num(Item.substr(0, A)), Item.substr(Bx + 1), Int(Item.substr(A + 1, Bx - A - 1))));
	}
	return Out;
}

std::string MarksText(const std::vector<std::pair<double, int>>& Marks)
{
	std::string Out;
	for (const std::pair<double, int>& M : Marks)
	{
		Out += (Out.empty() ? "" : ";") + F2(M.first) + ":" + std::to_string(M.second);
	}
	return Out;
}

std::vector<std::pair<double, int>> ReadMarks(const std::string& Text)
{
	std::vector<std::pair<double, int>> Out;
	size_t I = 0;
	while (I < Text.size())
	{
		size_t End = Text.find(';', I);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Item = Text.substr(I, End - I);
		const size_t C = Item.find(':');
		if (C != std::string::npos)
		{
			Out.push_back({Num(Item.substr(0, C)), Int(Item.substr(C + 1))});
		}
		I = End + 1;
	}
	return Out;
}

bool ConductWorthKeeping(const Conduct& C)
{
	return C.Late || C.Welched || C.Walked || C.Shots || C.Tilts || C.Chops || C.Honored || !C.Marks.empty();
}
} // namespace stake_detail

void WriteLines(const life::State& L, std::string& Out)
{
	const Book& B = L.Stakes;
	for (const Deal& D : B.Deals)
	{
		Out += "stake\tdeal\t" + Esc(D.Id) + "\t" + std::to_string(static_cast<int>(D.K)) + "\t" + std::to_string(static_cast<int>(D.By)) + "\t" + Esc(D.Backer) + "\t" +
			Esc(D.EventId) + "\t" + F2(D.Since) + "\t" + F2(D.NextChop) + "\t" + F2(D.Until) + "\t" + I2(D.RollNow) + "\t" + I2(D.Funded) + "\t" + I2(D.Makeup) + "\t" +
			I2(D.Debt) + "\t" + std::to_string(D.Events) + "\t" + std::to_string(D.Periods) + "\t" + std::to_string(D.State) + "\t" + I2(D.Stake) + "\t" + I2(D.Prize) +
			"\t" + I2(D.TheirShare) + "\t" + (D.InField ? "1" : "0") + "\t" + std::to_string(D.Tier) + "\t" + std::to_string(D.Losing) + "\t" + std::to_string(D.Missed) +
			"\t" + std::to_string(D.PeriodEvents) + "\t" + std::to_string(D.GhostsAtStart) + "\t" + F2(D.EndedAt);
		Terms6(D.T, Out);
		Out += "\n";
		for (const Line& Ln : D.Lines)
		{
			Out += "stake\tdline\t" + Esc(D.Id) + "\t" + F2(Ln.At) + "\t" + I2(Ln.Amount) + "\t" + Esc(Ln.Label) + "\n";
		}
	}
	for (const Fund& F : B.Funds)
	{
		Out += "stake\tfund\t" + Esc(F.EventId) + "\t" + Esc(F.DealId) + "\t" + std::to_string(F.Bullet) + "\t" + I2(F.Cents) + "\t" + (F.Settled ? "1" : "0") + "\t" + Esc(F.Name) +
			"\t" + F2(F.At) + "\n";
	}
	for (const Listing& Ls : B.Listings)
	{
		Out += "stake\tlist\t" + Esc(Ls.EventId) + "\t" + std::to_string(Ls.Ask) + "\t" + std::to_string(Ls.Markup) + "\t" + std::to_string(Ls.Filled) + "\t" + F2(Ls.Opens) +
			"\t" + F2(Ls.Closes) + "\t" + (Ls.Collected ? "1" : "0") + "\t" + I2(Ls.BuyIn) + "\t" + (Ls.Live ? "1" : "0") + "\t" + std::to_string(Ls.Told) + "\t" +
			Esc(PlanText(Ls.Plan)) + "\n";
	}
	for (const Piece& P : B.Pieces)
	{
		Out += "stake\tpiece\t" + Esc(P.EntryId) + "\t" + Esc(P.Buyer) + "\t" + std::to_string(P.Permille) + "\t" + I2(P.Paid) + "\t" + (P.InField ? "1" : "0") + "\t" +
			I2(P.Share) + "\t" + (P.Settled ? "1" : "0") + "\n";
	}
	for (const Swap& S : B.Swaps)
	{
		Out += "stake\tswap\t" + Esc(S.EntryId) + "\t" + Esc(S.Partner) + "\t" + std::to_string(S.Permille) + "\t" + std::to_string(S.State) + "\t" + I2(S.Net) + "\t" + I2(S.Mine) +
			"\t" + I2(S.Theirs) + "\t" + F2(S.Closes) + "\t" + F2(S.Since) + "\n";
	}
	for (const Owed& O : B.Owes)
	{
		Out += "stake\towe\t" + Esc(O.Id) + "\t" + Esc(O.Who) + "\t" + Esc(O.Label) + "\t" + I2(O.Cents) + "\t" + F2(O.Due) + "\t" + std::to_string(O.State) + "\t" +
			(O.ToYou ? "1" : "0") + "\t" + F2(O.Since) + "\t" + Esc(O.EntryId) + "\t" + F2(O.PaidAt) + "\t" + (O.Reminded ? "1" : "0") + "\n";
	}
	for (const Offer& O : B.Offers)
	{
		Out += "stake\toffer\t" + Esc(O.Id) + "\t" + std::to_string(static_cast<int>(O.K)) + "\t" + std::to_string(static_cast<int>(O.By)) + "\t" + Esc(O.Backer) + "\t" +
			Esc(O.EntryId) + "\t" + F2(O.Expires) + "\t" + std::to_string(O.Counters) + "\t" + F2(O.At) + "\t" + std::to_string(O.Permille) + "\t" + I2(O.BuyIn) + "\t" +
			std::to_string(O.State) + "\t" + std::to_string(O.Tier) + "\t" + std::to_string(O.CounteredDay) + "\t" + std::to_string(O.Lever) + "\t" + Esc(O.Line);
		Terms6(O.T, Out);
		Out += "\n";
	}
	if (ConductWorthKeeping(B.Record))
	{
		const Conduct& C = B.Record;
		Out += "stake\tconduct\t" + std::to_string(C.Late) + "\t" + std::to_string(C.Welched) + "\t" + std::to_string(C.Walked) + "\t" + std::to_string(C.Shots) + "\t" +
			std::to_string(C.Tilts) + "\t" + std::to_string(C.Chops) + "\t" + std::to_string(C.Honored) + "\t1\t" + MarksText(C.Marks) + "\n";
	}
	for (const Flow& F : B.ToWorld)
	{
		if (!F.Applied)
		{
			Out += "stake\tflow\t" + Esc(F.Id) + "\t" + Esc(F.Npc) + "\t" + Esc(F.Where) + "\t" + I2(F.Cents) + "\t" + std::to_string(F.Memory) + "\t" + F2(F.At) + "\t" + (F.Early ? "1" : "0") + "\n";
		}
	}
	std::set<std::string> Who;
	for (const auto& It : B.NetOnYou)
	{
		Who.insert(It.first);
	}
	for (const auto& It : B.PausedUntil)
	{
		Who.insert(It.first);
	}
	for (const std::string& W : Who)
	{
		const auto N = B.NetOnYou.find(W);
		const auto P = B.PausedUntil.find(W);
		Out += "stake\tnet\t" + Esc(W) + "\t" + I2(N == B.NetOnYou.end() ? 0 : N->second) + "\t" + F2(P == B.PausedUntil.end() ? 0.0 : P->second) + "\t" +
			(N == B.NetOnYou.end() ? "0" : "1") + "\t" + (P == B.PausedUntil.end() ? "0" : "1") + "\n";
	}
	for (const std::pair<double, std::string>& M : B.Burner)
	{
		Out += "stake\tburner\t" + F2(M.first) + "\t" + Esc(M.second) + "\n";
	}
	for (const auto& It : B.Done)
	{
		Out += "stake\tdone\t" + Esc(It.first) + "\t" + F2(It.second) + "\n";
	}
	coach::WriteLines(L.Study, Out);
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	if (P.empty())
	{
		return false;
	}
	if (P[0] == "coach")
	{
		return coach::ReadLine(P, L.Study);
	}
	if (P[0] != "stake" || P.size() < 2)
	{
		return false;
	}
	Book& B = L.Stakes;
	const std::string& K = P[1];
	if (K == "deal" && P.size() >= 38)
	{
		Deal D;
		D.Id = Unesc(P[2]);
		D.K = static_cast<Kind>(Int(P[3]));
		D.By = static_cast<Outfit>(Int(P[4]));
		D.Backer = Unesc(P[5]);
		D.EventId = Unesc(P[6]);
		D.Since = Num(P[7]);
		D.NextChop = Num(P[8]);
		D.Until = Num(P[9]);
		D.RollNow = Cents(P[10]);
		D.Funded = Cents(P[11]);
		D.Makeup = Cents(P[12]);
		D.Debt = Cents(P[13]);
		D.Events = Int(P[14]);
		D.Periods = Int(P[15]);
		D.State = Int(P[16]);
		D.Stake = Cents(P[17]);
		D.Prize = Cents(P[18]);
		D.TheirShare = Cents(P[19]);
		D.InField = P[20] == "1";
		D.Tier = Int(P[21]);
		D.Losing = Int(P[22]);
		D.Missed = Int(P[23]);
		D.PeriodEvents = Int(P[24]);
		D.GhostsAtStart = Int(P[25]);
		D.EndedAt = Num(P[26]);
		D.T = ReadTerms(P, 27);
		B.Deals.push_back(D);
	}
	else if (K == "dline" && P.size() >= 6)
	{
		const std::string Id = Unesc(P[2]);
		if (Deal* D = FindDeal(B, Id))
		{
			Line Ln;
			Ln.At = Num(P[3]);
			Ln.Amount = Cents(P[4]);
			Ln.Label = Unesc(P[5]);
			D->Lines.push_back(Ln);
		}
	}
	else if (K == "fund" && P.size() >= 9)
	{
		Fund F;
		F.EventId = Unesc(P[2]);
		F.DealId = Unesc(P[3]);
		F.Bullet = Int(P[4]);
		F.Cents = Cents(P[5]);
		F.Settled = P[6] == "1";
		F.Name = Unesc(P[7]);
		F.At = Num(P[8]);
		B.Funds.push_back(F);
	}
	else if (K == "list" && P.size() >= 13)
	{
		Listing Ls;
		Ls.EventId = Unesc(P[2]);
		Ls.Ask = Int(P[3]);
		Ls.Markup = Int(P[4]);
		Ls.Filled = Int(P[5]);
		Ls.Opens = Num(P[6]);
		Ls.Closes = Num(P[7]);
		Ls.Collected = P[8] == "1";
		Ls.BuyIn = Cents(P[9]);
		Ls.Live = P[10] == "1";
		Ls.Told = Int(P[11]);
		Ls.Plan = ReadPlan(Unesc(P[12]));
		B.Listings.push_back(Ls);
	}
	else if (K == "piece" && P.size() >= 9)
	{
		Piece Pc;
		Pc.EntryId = Unesc(P[2]);
		Pc.Buyer = Unesc(P[3]);
		Pc.Permille = Int(P[4]);
		Pc.Paid = Cents(P[5]);
		Pc.InField = P[6] == "1";
		Pc.Share = Cents(P[7]);
		Pc.Settled = P[8] == "1";
		B.Pieces.push_back(Pc);
	}
	else if (K == "swap" && P.size() >= 11)
	{
		Swap S;
		S.EntryId = Unesc(P[2]);
		S.Partner = Unesc(P[3]);
		S.Permille = Int(P[4]);
		S.State = Int(P[5]);
		S.Net = Cents(P[6]);
		S.Mine = Cents(P[7]);
		S.Theirs = Cents(P[8]);
		S.Closes = Num(P[9]);
		S.Since = Num(P[10]);
		B.Swaps.push_back(S);
	}
	else if (K == "owe" && P.size() >= 13)
	{
		Owed O;
		O.Id = Unesc(P[2]);
		O.Who = Unesc(P[3]);
		O.Label = Unesc(P[4]);
		O.Cents = Cents(P[5]);
		O.Due = Num(P[6]);
		O.State = Int(P[7]);
		O.ToYou = P[8] == "1";
		O.Since = Num(P[9]);
		O.EntryId = Unesc(P[10]);
		O.PaidAt = Num(P[11]);
		O.Reminded = P[12] == "1";
		B.Owes.push_back(O);
	}
	else if (K == "offer" && P.size() >= 28)
	{
		Offer O;
		O.Id = Unesc(P[2]);
		O.K = static_cast<Kind>(Int(P[3]));
		O.By = static_cast<Outfit>(Int(P[4]));
		O.Backer = Unesc(P[5]);
		O.EntryId = Unesc(P[6]);
		O.Expires = Num(P[7]);
		O.Counters = Int(P[8]);
		O.At = Num(P[9]);
		O.Permille = Int(P[10]);
		O.BuyIn = Cents(P[11]);
		O.State = Int(P[12]);
		O.Tier = Int(P[13]);
		O.CounteredDay = Int(P[14]);
		O.Lever = Int(P[15]);
		O.Line = Unesc(P[16]);
		O.T = ReadTerms(P, 17);
		B.Offers.push_back(O);
	}
	else if (K == "conduct" && P.size() >= 10)
	{
		Conduct& C = B.Record;
		C.Late = Int(P[2]);
		C.Welched = Int(P[3]);
		C.Walked = Int(P[4]);
		C.Shots = Int(P[5]);
		C.Tilts = Int(P[6]);
		C.Chops = Int(P[7]);
		C.Honored = Int(P[8]);
		C.Seeded = P[9] == "1";
		C.Marks = ReadMarks(P.size() > 10 ? P[10] : std::string());
		B.LinesRead |= ReadConduct;
	}
	else if (K == "flow" && P.size() >= 8)
	{
		Flow F;
		F.Id = Unesc(P[2]);
		F.Npc = Unesc(P[3]);
		F.Where = Unesc(P[4]);
		F.Cents = Cents(P[5]);
		F.Memory = Int(P[6]);
		F.At = Num(P[7]);
		F.Early = P.size() > 8 && P[8] == "1";
		B.ToWorld.push_back(F);
	}
	else if (K == "net" && P.size() >= 7)
	{
		const std::string W = Unesc(P[2]);
		if (P[5] == "1")
		{
			B.NetOnYou[W] = Cents(P[3]);
		}
		if (P[6] == "1")
		{
			B.PausedUntil[W] = Num(P[4]);
		}
	}
	else if (K == "done" && P.size() >= 4)
	{
		B.Done[Unesc(P[2])] = Num(P[3]);
	}
	else if (K == "burner" && P.size() >= 4)
	{
		B.Burner.push_back({Num(P[2]), Unesc(P[3])});
	}
	else
	{
		return K == "deal" || K == "dline" || K == "fund" || K == "list" || K == "piece" || K == "swap" || K == "owe" || K == "offer" || K == "conduct" || K == "flow" ||
			K == "net" || K == "done" || K == "burner";
	}
	B.LinesRead |= ReadAny;
	return true;
}

void AfterParse(SaveData& Data)
{
	// An older save (§7.5): the book starts empty. The record against the player keeps what happens from here on (a
	// pickup or a restriction through Session::StakeConduct, shots, tilted registrations, late payments); what came
	// before is the life's own record (Busts, Bans and BannedUntil, Evictions), which the public record and the backers
	// read from there. Nothing is written on load: a save, played or not, writes back exactly as it was read, and a
	// session that never reloaded keeps the same book as one that did (the monkey's round trip).
	Data.Life.Stakes.Record.Seeded = true;
}
} // namespace stake
} // namespace ss
