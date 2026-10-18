#include "ShortStack/Game/TableTalk.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cstdlib>

// What the card room's people say (next_segment_spec §4.5 to §4.8). core-room.
//
// The inventory is data: every template sits in a pool ("field.small.trip.open", "dealer.cage", "pa.level.ante"), and a
// line is a pool's template with its {Vars} filled. The Back Room's eight keep their lines word for word (CastLines:
// moved here from BackRoomPlayerTells.cpp's FLines and BackRoomLive.cpp's Goodbye, frozen by room_test).

namespace ss
{
namespace talk
{
namespace talk_detail
{
struct Template
{
	const char* Pool;
	const char* Text;
};

// ------------------------------------------------------------------ the field

const std::vector<Template>& FieldTemplates()
{
	static const std::vector<Template> All = {
		// Sitting down.
		{"field.sitdown", "Evening, all."},
		{"field.sitdown", "How we doing, folks?"},
		{"field.sitdown", "Good luck, everybody."},
		{"field.sitdown", "Mind the elbow. Long night."},
		{"field.sitdown", "Alright. Let's see some cards."},
		{"field.sitdown", "Evening. Who's running good?"},

		// Small talk: the trip up the ridge.
		{"field.small.trip.open", "You take the 14 up?"},
		{"field.small.trip.open", "That bus up the ridge gets longer every week."},
		{"field.small.trip.open", "Anybody else drive up? I'm parked in another county."},
		{"field.small.trip.open", "Switchbacks had me green by the third turn."},
		{"field.small.trip.open", "Who's driving down the mountain later? Asking for me."},
		{"field.small.trip.open", "Every time I come up here my ears pop twice."},
		{"field.small.trip.reply", "Driver took the switchbacks like he owed somebody money."},
		{"field.small.trip.reply", "Got a ride with my cousin. Never again."},
		{"field.small.trip.reply", "Drove. Don't ask me about the parking."},
		{"field.small.trip.reply", "Bus. Fell asleep and woke up at the resort."},
		{"field.small.trip.reply", "Same. My ears are still popping."},
		{"field.small.trip.reply", "Long way up for a cold deck."},

		// Small talk: food and coffee.
		{"field.small.food.open", "Anybody eaten? I'm running on peanuts from the bar."},
		{"field.small.food.open", "Somebody order me a burger before I fold out of hunger."},
		{"field.small.food.open", "What's good at the grill here? Don't say the wings."},
		{"field.small.food.open", "Coffee in this place could strip paint."},
		{"field.small.food.open", "Came up here on an empty stomach. Rookie move."},
		{"field.small.food.open", "They still do that chili at the bar?"},
		{"field.small.food.reply", "The fries. Only the fries."},
		{"field.small.food.reply", "Ate in the car. Regret it."},
		{"field.small.food.reply", "Coffee's free. That's the review."},
		{"field.small.food.reply", "Grab me something on the break."},
		{"field.small.food.reply", "Wings are fine. Just don't look at them."},
		{"field.small.food.reply", "I'm saving my appetite for your chips."},

		// Small talk: the room.
		{"field.small.room.open", "They turn the heat up in here, or is it me?"},
		{"field.small.room.open", "Listen to that slot floor. Somebody's hitting."},
		{"field.small.room.open", "This chair's been here since the eighties."},
		{"field.small.room.open", "Same felt, same chairs, same coffee."},
		{"field.small.room.open", "You can hear the wind off the ridge in here."},
		{"field.small.room.open", "Somebody tell the slots to keep it down."},
		{"field.small.room.reply", "It's you. You're running hot."},
		{"field.small.room.reply", "Those slots never sleep."},
		{"field.small.room.reply", "Chair's older than me and holding up better."},
		{"field.small.room.reply", "I like it. Feels like a card room should."},
		{"field.small.room.reply", "Beats my living room."},
		{"field.small.room.reply", "Keeps you awake, at least."},

		// Small talk: the night ahead.
		{"field.small.night.open", "Long night ahead."},
		{"field.small.night.open", "Gonna be a late one."},
		{"field.small.night.open", "Whoever wins this better buy breakfast."},
		{"field.small.night.open", "Tell me we're not going to three in the morning again."},
		{"field.small.night.open", "Promised myself I'd play tight tonight. We'll see."},
		{"field.small.night.open", "First one out buys coffee for the table."},
		{"field.small.night.reply", "Speak for yourself. I'm here till the end."},
		{"field.small.night.reply", "Breakfast's on whoever's left."},
		{"field.small.night.reply", "Coffee's free, genius."},
		{"field.small.night.reply", "Long as I'm still here for it."},
		{"field.small.night.reply", "I'll hold you to that."},
		{"field.small.night.reply", "We'll see who's tired at two."},

		// Small talk: their week (one of their own results, LiveVoice::Story).
		{"field.small.week", "{PlaceWord} in the {EventSpoken} {When}. {Entries} runners, and I walk with {Prize}."},
		{"field.small.week", "Cashed the {EventSpoken} {When}. {PlaceWord} of {Entries}. I'll take it."},
		{"field.small.week", "Made the money in the {EventSpoken} {When}. {PlaceWord}, {Prize}."},
		{"field.small.week", "{WhenCap} I ran {PlaceLower} in the {EventSpoken}. Out of {Entries}."},
		{"field.small.week", "{PlaceWord} out of {Entries} in the {EventSpoken} {When}. Felt like more."},
		{"field.small.week", "I got {PlaceLower} in the {EventSpoken} {When}. {Prize}. Good enough."},
		{"field.small.week.win", "Won the {EventSpoken} {When}. Still buying my own coffee, though."},
		{"field.small.week.win", "Took down the {EventSpoken} {When}. {Entries} runners."},
		{"field.small.week.win", "Shipped the {EventSpoken} {When}. Yeah, that was me."},
		{"field.small.week.online", "{PlaceWord} out of {Entries} online {When}. Took home {Prize}."},
		{"field.small.week.online", "Cashed online {When}. {PlaceWord} of {Entries}. Live's slower."},
		{"field.small.week.online", "Ran deep online {When}. {PlaceWord}. Still sore about it."},
		{"field.small.bubble", "Bubbled the {EventSpoken} {When}. {Entries} runners and I'm the one."},
		{"field.small.bubble", "{WhenCap} I bubbled the {EventSpoken}. Still not over it."},
		{"field.small.bubble", "Bubble boy in the {EventSpoken} {When}. Don't bring it up."},
		{"field.small.week.reply", "Not bad. I'll take that."},
		{"field.small.week.reply", "Beats my week."},
		{"field.small.week.reply", "Good for you. Keep it rolling."},
		{"field.small.week.reply", "Nice. Buy the next coffee, then."},
		{"field.small.week.reply", "That's a good week."},
		{"field.small.week.reply", "Hey, a cash is a cash."},
		{"field.small.bubble.reply", "Ouch. That one stays with you."},
		{"field.small.bubble.reply", "Brutal. It evens out."},
		{"field.small.bubble.reply", "Been there. It never stops stinging."},
		{"field.small.bubble.reply", "Next time, fold your way in."},

		// Small talk: how they're running (their Mood).
		{"field.small.heater", "I'm running good this week. Nobody say anything."},
		{"field.small.heater", "Cards have been nice to me lately. Don't jinx it."},
		{"field.small.heater", "Best week I've had in a while. Knock on the felt."},
		{"field.small.heater", "I'm hot right now. Stay out of my way."},
		{"field.small.heater", "Everything's hitting this week. Everything."},
		{"field.small.heater", "Don't look at me. I'm on a run."},
		{"field.small.heater.reply", "Don't jinx it, then."},
		{"field.small.heater.reply", "Knock on the felt for you."},
		{"field.small.heater.reply", "Spread some of that this way."},
		{"field.small.heater.reply", "Enjoy it. It doesn't last."},
		{"field.small.heater.reply", "Say it louder, see what happens."},
		{"field.small.heater.reply", "Good. Spill some my way."},
		{"field.small.down", "Haven't cashed in a while. I'm due, right?"},
		{"field.small.down", "Rough month. Cards owe me one."},
		{"field.small.down", "I'm running bad. Like, historically bad."},
		{"field.small.down", "Can't win a flip to save my life lately."},
		{"field.small.down", "This is the night it turns. Has to be."},
		{"field.small.down", "Been a long few weeks. I just want a sweat."},
		{"field.small.down.reply", "It turns. It always turns."},
		{"field.small.down.reply", "Variance. It comes back around."},
		{"field.small.down.reply", "Hang in there. Tight is right."},
		{"field.small.down.reply", "We've all been there."},
		{"field.small.down.reply", "Tonight's the night, then."},
		{"field.small.down.reply", "Don't chase it. Just play."},

		// Small talk: who they are (their Identity), and makeup (only their own: Npc::Backer != -1).
		{"field.small.online", "Online I'd have seen forty hands by now."},
		{"field.small.online", "Live is so slow. I keep reaching for a mouse."},
		{"field.small.online", "Online you don't wait for the shuffle."},
		{"field.small.online", "Twelve tables at home, and here I am staring at one."},
		{"field.small.online", "Feels weird holding real cards again."},
		{"field.small.online", "No time bank in here, huh? Nice."},
		{"field.small.online.reply", "Live's where you learn faces."},
		{"field.small.online.reply", "Slow's good. Gives you time to think."},
		{"field.small.online.reply", "Online kids. Every one of you."},
		{"field.small.online.reply", "Enjoy the cards. Real ones."},
		{"field.small.online.reply", "Grab a mouse on the break, then."},
		{"field.small.online.reply", "Some of us like the shuffle."},
		{"field.small.makeup", "I'm in makeup to my guy. Every buy-in's his till I'm out."},
		{"field.small.makeup", "Playing on somebody else's money tonight. No pressure."},
		{"field.small.makeup", "My backer reads the results page. Gotta run deep."},
		{"field.small.makeup", "Staked again. I need a cash to get out from under."},
		{"field.small.makeup", "Every chip I win tonight, somebody else is counting it."},
		{"field.small.makeup", "Makeup's a heavy bag to carry up a mountain."},
		{"field.small.makeup.reply", "Hm."},
		{"field.small.makeup.reply", "Rough way to play."},
		{"field.small.makeup.reply", "Then let's get you a sweat."},
		{"field.small.makeup.reply", "Rather you than me."},
		{"field.small.makeup.reply", "Run good, then."},
		{"field.small.makeup.reply", "Better make it count."},
		{"field.small.regular", "Every night I tell myself just the Nightly. Every night."},
		{"field.small.regular", "Same room, same felt, same faces."},
		{"field.small.regular", "I've played this room more than I've seen my sister."},
		{"field.small.regular", "Bartender knows my order. That's a problem."},
		{"field.small.regular", "I think this is my chair. It knows me."},
		{"field.small.regular", "Years up this mountain and the coffee never changed."},
		{"field.small.regular.reply", "You and me both."},
		{"field.small.regular.reply", "This room gets in your blood."},
		{"field.small.regular.reply", "That's how they get you."},
		{"field.small.regular.reply", "Could be worse places to spend a night."},
		{"field.small.regular.reply", "Least the chairs are comfortable."},
		{"field.small.regular.reply", "Speak for yourself. I've got a life."},

		// Small talk between people who know each other (their Ties).
		{"field.small.friend", "{Name}, don't you dare sit behind me at the next table."},
		{"field.small.friend", "{Name}, you said you'd fold more tonight."},
		{"field.small.friend", "Look who it is. {Name}, how's the family?"},
		{"field.small.friend", "{Name}. Same as always?"},
		{"field.small.friend", "Of all the tables, {Name}. Of all the tables."},
		{"field.small.friend", "{Name}, if you bust me I'm walking home."},
		{"field.small.friend.reply", "Wouldn't dream of it."},
		{"field.small.friend.reply", "Fold more? Who said that?"},
		{"field.small.friend.reply", "Good. Everybody's good."},
		{"field.small.friend.reply", "Same as always."},
		{"field.small.friend.reply", "Small room, huh?"},
		{"field.small.friend.reply", "You're walking either way."},
		{"field.small.rival", "{Name}. Again. Somebody up there hates me."},
		{"field.small.rival", "{Name}, try not to splash around tonight."},
		{"field.small.rival", "Every time I sit down, there's {Name}."},
		{"field.small.rival", "Don't get comfortable, {Name}."},
		{"field.small.rival", "Look who it is. {Name}, I'm coming for your stack."},
		{"field.small.rival", "{Name}. Still playing every hand?"},
		{"field.small.rival.reply", "Missed you too."},
		{"field.small.rival.reply", "Try and take it."},
		{"field.small.rival.reply", "Keep talking. It's cute."},
		{"field.small.rival.reply", "Same to you."},
		{"field.small.rival.reply", "You say that every time."},
		{"field.small.rival.reply", "We'll see who's here at midnight."},

		// Asking the dealer.
		{"field.ask.average", "What's average?"},
		{"field.ask.average", "What's the average stack now?"},
		{"field.ask.average", "Dealer, what's average?"},
		{"field.ask.left", "How many left?"},
		{"field.ask.left", "How many we got left?"},
		{"field.ask.left", "Dealer, how many players left?"},
		{"field.ask.break", "How long till the break?"},
		{"field.ask.break", "When's the break?"},
		{"field.ask.break", "We breaking soon?"},

		// Needles at the player: after you fold to their bet, show a bluff, or tank.
		{"field.needle", "You can have the next one, champ."},
		{"field.needle", "Every time I raise, huh?"},
		{"field.needle", "Folding again? I'm starting to like you."},
		{"field.needle", "Keep folding. I'll keep raising."},
		{"field.needle", "Scared money don't make money."},
		{"field.needle", "I'll show you one of these days. Maybe."},
		{"field.needle.rival", "You again. Good. I owe you one."},
		{"field.needle.rival", "Of course it's you."},
		{"field.needle.rival", "Still folding to me, {Hero}?"},
		{"field.needle.rival", "I remember you. You remember me?"},
		{"field.needle.rival", "Sit tight, {Hero}. I'm coming."},
		{"field.needle.rival", "Some things don't change, huh, {Hero}?"},
		{"field.needle.bluff", "You'd show me that? Cold."},
		{"field.needle.bluff", "Okay. Okay. I'll remember that."},
		{"field.needle.bluff", "Showing it. Real classy."},
		{"field.needle.bluff", "Enjoy it. I've got a long memory."},
		{"field.needle.bluff", "That's the last one you get."},
		{"field.needle.bluff", "Put that back in your pocket next time."},
		{"field.needle.tank", "Any day now."},
		{"field.needle.tank", "It's not that hard. Fold."},
		{"field.needle.tank", "You writing a novel over there?"},
		{"field.tank", "Take your time. I'm only getting older."},
		{"field.tank", "No rush. I've got all night."},
		{"field.tank", "Whatever you do, do it with feeling."},
		{"field.tank", "Think it through. We'll wait."},
		{"field.tank", "Big decision. Take a breath."},
		{"field.tank", "Go with your gut. It's usually right."},

		// After a showdown: only people who were in the hand.
		{"field.nicehand", "Nice hand."},
		{"field.nicehand", "Played that right."},
		{"field.nicehand", "Well played."},
		{"field.nicehand", "You had it. Fair enough."},
		{"field.nicehand", "Couldn't get away from it."},
		{"field.nicehand", "Good hand. Ship it."},
		{"field.nicehand.called.made", "Good call. I had it."},
		{"field.nicehand.called.made", "Good call. Thought I was good."},
		{"field.nicehand.called.made", "Good call. Thought I had you."},
		{"field.nicehand.called.air", "Good call. I had nothing."},
		{"field.nicehand.called.air", "Good call. Had to try."},
		{"field.nicehand.called.air", "You caught me. Good call."},
		{"field.suckout", "Sorry. Really."},
		{"field.suckout", "I'll take it."},
		{"field.suckout", "Didn't deserve that one."},
		{"field.suckout", "Sorry, sorry. I know."},
		{"field.suckout", "Ugly. I'll take it anyway."},
		{"field.suckout", "That's poker. Sorry, though."},
		{"field.badbeat", "{Mine}. {Theirs}. On the {Street}."},
		{"field.badbeat", "{Mine} against {TheirsLower}, and the {Street} does that."},
		{"field.badbeat", "I get it in with {MineLower}. {Theirs} gets there on the {Street}."},
		{"field.badbeat", "{Theirs}. Against my {MineLower}. On the {Street}."},
		{"field.badbeat", "{Mine}, all in, ahead. Then the {Street}."},
		{"field.badbeat", "Had {MineLower}. Lost to {TheirsLower} on the {Street}. Sure."},
		{"field.break.beat", "{Mine}. {Theirs}. On the {Street}."},
		{"field.break.beat", "Still thinking about it. {Mine} against {TheirsLower}."},
		{"field.break.beat", "{Mine} against {TheirsLower}. I keep seeing that {Street}."},
		{"field.cooler", "Cooler. Nothing to be done."},
		{"field.cooler", "Can't fold that. Nobody folds that."},
		{"field.cooler", "Wrong place, wrong time."},
		{"field.cooler", "That's a cooler. Good hand."},
		{"field.cooler", "Couldn't get away. Nobody could."},
		{"field.cooler", "Set up. Happens."},
		{"field.doubleup", "There's the double."},
		{"field.doubleup", "Back in business."},
		{"field.doubleup", "Now we're playing."},
		{"field.doubleup", "Needed that one."},
		{"field.doubleup", "Breathing room. Finally."},
		{"field.doubleup", "Double up. Let's go."},
		{"field.win", "Thank you."},
		{"field.win", "I'll take it."},
		{"field.win", "Ship it."},
		{"field.win", "Come to me."},
		{"field.win", "There we go."},
		{"field.win", "I'll take those."},
		{"field.lose", "Nice hand."},
		{"field.lose", "Gone."},
		{"field.lose", "Of course."},
		{"field.lose", "Unreal."},
		{"field.lose", "Okay. Okay."},
		{"field.lose", "Next one."},

		// Leaving.
		{"field.goodbye", "That's me. Good luck, all."},
		{"field.goodbye", "I'll be at the bar if anyone needs a sad story."},
		{"field.goodbye", "Good luck, everybody."},
		{"field.goodbye", "Well. That's the night."},
		{"field.goodbye", "Gonna go stare at the mountain for a while."},
		{"field.goodbye", "Nice playing with you all."},
		{"field.goodbye.paid", "Took a little something. Good luck, all."},
		{"field.goodbye.paid", "A cash is a cash. Night, all."},
		{"field.goodbye.paid", "I'll take the money and the bus. Good luck."},
		{"field.goodbye.felted", "Nice playing."},
		{"field.goodbye.felted", "Tough one. Nice playing."},
		{"field.goodbye.felted", "Well played tonight."},

		// The room's milestones.
		{"field.bubble", "Min-cash, baby."},
		{"field.bubble", "My rent thanks you, {Bubble}."},
		{"field.bubble", "We're in! We're in."},
		{"field.bubble", "Sorry, {Bubble}. And thank you, {Bubble}."},
		{"field.bubble", "Now we can play some poker."},
		{"field.bubble", "Money! Somebody call my mother."},
		{"field.money", "In the money. Now we play."},
		{"field.money", "Money's in. Breathe."},
		{"field.money", "Okay. Now the real tournament."},
		{"field.final", "Final table. Somebody take my picture."},
		{"field.final", "Made it. Now the real money."},
		{"field.final", "Don't let me get comfortable up here."},
		{"field.final", "Final table at the Embercrest. Not bad."},
		{"field.final", "Look at us. Big stage."},
		{"field.final", "Lights are brighter up here."},
		{"field.break", "Need some air. Back in a bit."},
		{"field.break", "Coffee. Now."},
		{"field.break", "Bathroom line's going to be a nightmare."},
		{"field.break", "Gonna call home. Back before the cards."},
		{"field.break", "Stretch break. My back's done."},
		{"field.break", "Anybody want anything from the bar?"},

		// The rail: a friend out of the event, still watching.
		{"field.rail", "Come on, {Name}!"},
		{"field.rail", "Let's go, {Name}!"},
		{"field.rail", "That's it, {Name}!"},
		{"field.rail", "Yes, {Name}! Yes!"},
		{"field.rail", "There it is, {Name}!"},
		{"field.rail", "Atta way, {Name}!"},
		// ... and before it lands, the player all in at the final table.
		{"field.rail.allin", "Come on, {Name}!"},
		{"field.rail.allin", "Hold, {Name}! Hold!"},
		{"field.rail.allin", "Come on, come on, {Name}!"},
		// A backer at the rail reading your stack the way a railbird can (chipset::ReadStack, rounded like a person).
		{"field.rail.read", "{Rough}. You're fine."},
		{"field.rail.read", "{Rough}, give or take. Keep going."},
		{"field.rail.read", "Looks like {RoughLower} from here. Don't spew it."},
		{"field.rail.read.short", "{Rough}. Pick your spots."},
		{"field.rail.read.short", "{Rough}, maybe. Tighten up."},
		{"field.rail.read.short", "Looks like {RoughLower}. Find a good one."},

		// Noticed: a regular who knows you, and the patch on you (LiveEntry::Patches).
		{"field.noticed.luckypenny", "The corner store sponsors you?"},
		{"field.noticed.luckypenny", "The Lucky Penny? You get free gum or what?"},
		{"field.noticed.barber", "Fifth St. Barbers? That explains the haircut."},
		{"field.noticed.barber", "A barber shop patch. Now I've seen everything."},
		{"field.noticed.ironhouse", "IronHouse, huh? You lifting between levels?"},
		{"field.noticed.ironhouse", "A gym sponsors you? Don't make me feel bad."},
		{"field.noticed.embercrest", "Team Embercrest. The house's own, huh?"},
		{"field.noticed.embercrest", "They put you on the team? Don't let it go to your head."},
		{"field.noticed.local", "{Brand}, huh? Supporting the neighborhood."},
		{"field.noticed.local", "Is that a {Brand} patch? Look at you."},
		{"field.noticed.energy", "{Brand}? You actually drink that stuff?"},
		{"field.noticed.energy", "That's a lot of {Brand} for one person."},
		{"field.noticed.site", "{Brand}, huh? Big time."},
		{"field.noticed.site", "{Brand} on your shirt at a live table. Bold."},
		{"field.noticed.other", "Nice patch. {Brand}, huh?"},
		{"field.noticed.other", "{Brand}? Didn't know they sponsored poker players."},
	};
	return All;
}

// ------------------------------------------------------------------ the dealers

const std::vector<Template>& DealerTemplates()
{
	static const std::vector<Template> All = {
		{"dealer.allin", "All in."},
		{"dealer.allin.seat", "Seat {Seat} is all in."},
		{"dealer.allin.named", "{Name} is all in."},
		{"dealer.allin.count", "All in. {Spoken}"},
		{"dealer.turnup", "Turn them up, please."},
		{"dealer.turnup", "Players, turn them up."},
		{"dealer.table.allin", "All in."},
		{"dealer.table.called", "All in and a call, table {TableWord}!"},
		{"dealer.table.called", "All in and a call on {TableWord}!"},
		{"dealer.table.seatopen", "Seat open, {TableWord}!"},
		{"dealer.table.seatopen", "Seat open on {TableWord}!"},
		{"dealer.seatopen", "Seat open."},
		{"dealer.cage", "{Place} place. See the cage for {Amount}. Good game."},
		{"dealer.blinds", "Blinds are up."},
		{"dealer.break", "Break, folks. Stretch your legs."},
		{"dealer.h4h", "Hand for hand, folks. Take your time."},
		{"dealer.average", "Average is about {Average}."},
		{"dealer.left", "{Left} left. {Paid} paid."},
		{"dealer.left.itm", "{Left} left. Everybody's in the money."},
		{"dealer.break.next", "Break after this level."},
		{"dealer.break.levels", "{LevelsCap} more levels to the break."},
		{"dealer.turn", "Seat {Seat}, it's on you."},
		{"dealer.turn.dee", "Kid. It's on you."},
		// BackRoomLive.cpp's own lines at c09bc31 (:778, :1602, :1844), now with the director's gates.
		{"dealer.welcome", "Welcome. Seat {Seat}."},
		{"dealer.move", "You're moving. Table {Table}, seat {Seat}. Grab your chips."},
		{"dealer.move.dee", "You're moving, kid. Table {Table}, seat {Seat}. Grab your chips."},
		{"dealer.back", "Welcome back."},
	};
	return All;
}

// ------------------------------------------------------------------ the floor

const std::vector<Template>& FloorTemplates()
{
	static const std::vector<Template> All = {
		{"pa.desk.open", "The {Event} is now open for registration at the tournament desk. {Stack} chips, {Minutes}-minute levels, cards at {Time}."},
		{"pa.ten.minutes", "Players for the {Event}, ten minutes. Your table and seat are on your receipt."},
		{"pa.start", "Ladies and gentlemen, welcome to the Embercrest {Short}. {Players} players, a prize pool of {Pool}. Dealers, shuffle up and deal!"},
		{"pa.start.late", "Late registration closes at {Time}. {Players} players, {Pool} in the pool."},
		{"pa.start.closed", "Registration is closed. {Players} players, {Pool} in the pool, the top {Paid} paid."},
		{"pa.level", "Players, the blinds are now {Sb} and {Bb}."},
		{"pa.level.ante", "Players, the blinds are now {Sb} and {Bb}, with a big blind ante of {Ante}."},
		{"pa.last.call", "Ladies and gentlemen, this is the last call for registration for the Embercrest {Short}. Late registration closes at {Time}."},
		{"pa.reg.closed", "Registration for the {Short} is now closed. {Players} players, a prize pool of {Pool}, and the top {Paid} are paid. A min-cash is {MinCash}."},
		{"pa.break", "Players, we are on a {Minutes}-minute break. Please be back in your seats when the clock says so."},
		{"pa.break.next", "When we come back, the blinds will be {Sb} and {Bb}."},
		{"pa.break.next.ante", "When we come back, the blinds will be {Sb} and {Bb} with a {Ante} big blind ante."},
		{"pa.colorup", "We're coloring up the {Denoms} on this break. Your dealer will make the change at your seat."},
		{"pa.five.minutes", "Five minutes, players. Five minutes left on the break."},
		{"pa.break.over", "Players, the break is over. Please take your seats, the cards are in the air."},
		{"pa.away", "Players, we are {Away} away from the money. {Paid} places are paid, and a min-cash is {MinCash}."},
		{"pa.h4h", "Players, we are on the bubble. We're now hand for hand. Dealers, please hold your decks until every table has finished the hand."},
		{"pa.h4h.call", "We have an all-in and a call at table {Table}."},
		{"pa.h4h.done", "All tables complete. Dealers, next hand."},
		{"pa.bubble", "And that's the bubble! {Name}, thank you for playing. Congratulations, everyone remaining is in the money."},
		{"pa.bubble.seat", "And that's the bubble! Table {Table}, seat {Seat}, thank you for playing. Congratulations, everyone remaining is in the money."},
		{"pa.bust.paid", "Table {Table}, seat {Seat} is out in {Place} place, collecting {Amount}."},
		{"pa.bust", "Table {Table}, seat {Seat} is out in {Place} place."},
		{"pa.table.breaking", "Table {Table} is breaking. Players, take your chips and your seat cards from the floor."},
		{"pa.moved", "Player from table {From}, you've been moved: table {Table}, seat {Seat}. Your chips are there."},
		{"pa.two.tables", "We're down to our last two tables. {Left} players left."},
		{"pa.final", "Ladies and gentlemen, we are down to our final table! Players, please bring your chips to the feature table."},
		{"pa.final.honor.title", "Among them, {Honor}, {Name}."},
		{"pa.final.honor.own", "Among them, {Honor} {Name}."},
		{"pa.final.honor.article", "Among them, {Name}, {Honor}."},
		{"pa.intro.open", "Ladies and gentlemen, your final table."},
		{"pa.intro.seat", "In seat {SeatWord}, with {Chips}, {Name}."},
		{"pa.intro.seat.title", "In seat {SeatWord}, {Honor}, {Name}, with {Chips}."},
		{"pa.intro.seat.own", "In seat {SeatWord}, {Honor} {Name}, with {Chips}."},
		{"pa.intro.seat.article", "In seat {SeatWord}, with {Chips}, {Name}, {Honor}."},
		{"pa.intro.seat.nameless", "In seat {SeatWord}, with {Chips}."},
		{"pa.headsup", "We're heads-up for the {Event} title. {A}, {AChips}. {B}, {BChips}. First place pays {First}."},
		{"pa.champion", "Ladies and gentlemen, your Embercrest {Short} champion... {Name}!"},
		{"pa.champion.dee.hero", "Ladies and gentlemen, your Embercrest {Short} champion... {Name}! Told you, kid."},
		{"pa.champion.seat", "Ladies and gentlemen, we have a champion. Congratulations to table {Table}, seat {Seat}!"},
		{"pa.cancelled", "Ladies and gentlemen, with {Players} players tonight's {Short} is cancelled. Please see the desk for your refund."},
		{"pa.courtesy", "Would the player who left a black phone at table {Table} please see the tournament desk."},
	};
	return All;
}

// ------------------------------------------------------------------ a fresh start's first month (update 2 §3.3 F9)

const std::vector<Template>& FreshTemplates()
{
	// The regulars on the room's new books, the first thirty days of a fresh start. The director picks a topic only while
	// it's true (room::Books: which of the room's events have a winner yet, the first names on the board) and fills the
	// event, the day and the name; no line here claims a result by itself (U5). After the dealers and the floor in the
	// inventory, so every older template keeps its number (a night's Told and the room's golden).
	static const std::vector<Template> All = {
		// Tonight's event is the first of its kind since the books opened.
		{"field.fresh.tonight", "First {FreshEvent} in the new books, and we're all sitting in it."},
		{"field.fresh.tonight", "Nobody's won the {FreshEvent} in the new books. That changes before we go home."},
		{"field.fresh.tonight", "Whoever takes this one, first {FreshEvent} winner in the books. That sticks."},
		{"field.fresh.tonight", "Board's got no {FreshEvent} winner on it yet. One of us fixes that."},
		{"field.fresh.tonight", "First {FreshEvent} of the new books. Feels like it should pay more."},
		{"field.fresh.tonight", "Somebody in this room is the first {FreshEvent} winner in the books."},
		// The first of another of the room's events, still to come (or being dealt now).
		{"field.fresh.next", "Who takes the first {FreshEvent} in the books? It's {FreshWhen}."},
		{"field.fresh.next", "First {FreshEvent} in the books is {FreshWhen}. Somebody's name goes up."},
		{"field.fresh.next", "Nobody's won the {FreshEvent} since the books opened. It's {FreshWhen}."},
		{"field.fresh.next", "Been thinking about the first {FreshEvent} in the books. It's {FreshWhen}."},
		{"field.fresh.next", "Board's still waiting on its first {FreshEvent} winner. That's {FreshWhen}."},
		{"field.fresh.next.nightly", "Who takes the first Nightly? It's {FreshWhen}. Everybody wants to be first."},
		{"field.fresh.next.nightly", "First Nightly in the new books is {FreshWhen}. Somebody's name goes up first."},
		{"field.fresh.next.nightly", "Nobody's taken a Nightly since the books opened. It's {FreshWhen}."},
		// Nothing the room has played since the books opened has been won yet.
		{"field.fresh.wall", "Board by the desk is still blank. Somebody's name goes up first."},
		{"field.fresh.wall", "Nobody's won a thing in here since the books opened. That board's anybody's."},
		{"field.fresh.wall", "Whole board by the desk, and not one name on it yet."},
		// The first name on the board: the first of the room's events won in the books.
		{"field.fresh.wallfirst", "{FreshWho}'s the first name on that board. The {FreshEvent}, {FreshThen}."},
		{"field.fresh.wallfirst", "First name up on that board? {FreshWho}. The {FreshEvent}, {FreshThen}."},
		{"field.fresh.wallfirst", "{FreshWho} got the first name up on that board. People remember that."},
		{"field.fresh.wallfirst.self", "First name on that board is mine. Somebody had to be first."},
		{"field.fresh.wallfirst.self", "Took the {FreshEvent} {FreshThen}. First name up there. Still sinking in."},
		{"field.fresh.wallfirst.you", "First name on that board's yours, {Hero}. Don't let it go to your head."},
		{"field.fresh.wallfirst.you", "You're the first name on the board by the desk. They'll tell that one a while."},
		// The first of one of the room's events, won.
		{"field.fresh.first", "{FreshWho} took the first {FreshEvent} in the books {FreshThen}."},
		{"field.fresh.first", "First {FreshEvent} in the books went to {FreshWho}. {FreshThenCap}, that was."},
		{"field.fresh.first", "{FreshWho} won the first {FreshEvent} since the books opened. Good for them."},
		{"field.fresh.first.self", "Won the first {FreshEvent} in the books {FreshThen}. Look it up."},
		{"field.fresh.first.self", "First {FreshEvent} in the books went to me. Still buying my own coffee."},
		{"field.fresh.first.you", "You took the first {FreshEvent} in the books, {Hero}. They'll talk about that."},
		{"field.fresh.first.you", "First {FreshEvent} in the new books has your name on it, {Hero}."},
		// More than one win up the hill already.
		{"field.fresh.twice", "{FreshWho}'s won in here more than once already, and the books aren't a month old."},
		{"field.fresh.twice", "Books aren't a month old and {FreshWho}'s up on that board more than once."},
		{"field.fresh.twice.self", "Up on that board more than once already. Books aren't a month old."},
		{"field.fresh.twice.self", "More than one win in here already. Nobody's buying me a drink, though."},
		{"field.fresh.twice.you", "You're up on that board more than once already, {Hero}. Easy now."},
		{"field.fresh.twice.you", "More than one win in here already, {Hero}, and the books aren't a month old."},
		// The books' first week.
		{"field.fresh.books", "New books, same faces. Everybody's record starts from the same nothing."},
		{"field.fresh.books", "Books are a week old, if that. Everybody's even. Won't last."},
		{"field.fresh.books", "Fresh books this week. Whatever you did in here before, nobody wrote it down."},
		{"field.fresh.books", "Whole room's starting from zero. The new books don't care who you were."},
		// The answers: nothing in them is a claim.
		{"field.fresh.reply", "Somebody's got to be first."},
		{"field.fresh.reply", "Might as well be somebody at this table."},
		{"field.fresh.reply", "First is first. Nobody remembers second."},
		{"field.fresh.reply", "I'll take it. First, last, whatever pays."},
		{"field.fresh.reply", "Get in line."},
		{"field.fresh.reply", "We'll see who's first."},
		{"field.fresh.first.reply", "Good for them."},
		{"field.fresh.first.reply", "Name's up there for good now."},
		{"field.fresh.first.reply", "That's how you get remembered in here."},
		{"field.fresh.first.reply", "Lucky. Or good. Hard to tell yet."},
		{"field.fresh.first.reply", "Somebody had to be first."},
		{"field.fresh.first.reply", "Huh. Didn't see that one coming."},
		{"field.fresh.self.reply", "We heard."},
		{"field.fresh.self.reply", "Yeah, yeah. Buy a round, then."},
		{"field.fresh.self.reply", "You'll tell us again next week."},
		{"field.fresh.self.reply", "Enjoy it while it lasts."},
	};
	return All;
}

// ------------------------------------------------------------------ the Back Room's eight, word for word

struct CastSet
{
	const char* Name;
	float Chatter;
	std::vector<const char*> Chat;
	std::vector<const char*> Win;
	std::vector<const char*> Lose;
	std::vector<const char*> Noticed;
	const char* Goodbye;
};

const std::vector<CastSet>& CastSets()
{
	// BackRoomPlayerTells.cpp's FLines (chat, win, lose, noticed) and BackRoomLive.cpp's Goodbye at c09bc31, and each
	// persona's Chatter (CastLooks.cpp). Frozen: room_test holds every line.
	static const std::vector<CastSet> All = {
		{"Sal", 0.35f,
			{"Forty years I been playing this game.", "In my day we played for matchsticks.", "Dee, this coffee's older than me.", "Patience. That's the whole game, kid."},
			{"Thank you kindly.", "Slow and steady."},
			{"Eh. Cards are cards.", "Shoulda stayed home tonight."},
			{"Something on my face, kid?", "Take a picture, it lasts longer."},
			"That's me. Good luck, kid."},
		{"Big Lou", 0.8f,
			{"Who's ordering wings?", "Lou don't fold. Lou never folds!", "That dryer's louder than my ex.", "Deal 'em, Dee, I'm feeling it tonight!"},
			{"That's what I'm talking about!", "Ship it! Come to papa."},
			{"Unbelievable. Un-be-lievable.", "Every time. Every single time!"},
			{"You lookin' at me? You lookin' at my chips?", "Eyes on your own cards, kid."},
			"Lou's out! Lou is OUT! Unbelievable."},
		{"Twitch", 0.5f,
			{"Can we go faster? Come on.", "Bet you I hit this one.", "Anybody got a lighter?", "Come on come on come on."},
			{"Yeah! Yeah! Pay me!", "Too easy, man."},
			{"This deck is rigged.", "Are you kidding me right now?"},
			{"What? What're you looking at?", "Quit staring, man. Seriously."},
			"Whatever, man. This deck hates me."},
		{"Mei", 0.15f,
			{"Mm.", "Your deal, Dee.", "Interesting."},
			{"Thank you.", "Nice try."},
			{"Good call.", "Well played."},
			{"See something you like?", "Careful. I look back."},
			"Good game, everyone."},
		{"Mrs. Park", 0.25f,
			{"My grandson plays on the computer. He is not good.", "Fold, fold, fold. Then I win.", "Thirty years I play this tournament.", "Dealer, slower please. My eyes."},
			{"Thank you, dear.", "Patience."},
			{"Hm.", "Next time."},
			{"You want to marry me? Stop looking.", "Eyes on your cards, young man."},
			"Thirty years. Still nothing. Goodnight, everybody."},
		{"Rick", 0.75f,
			{"I sell more cars by noon than this whole table wins tonight.", "Raise it up, let's play some poker!", "Who's buying drinks? I'm buying drinks.",
				"Dee, you're my good luck charm. Don't go anywhere."},
			{"That's how it's done!", "Stack 'em up, sweetheart."},
			{"Rigged. Un-be-lievable.", "Whatever. I'm rebuying next week."},
			{"Like what you see, champ?", "Take a picture."},
			"Unbelievable! I'm going to the bar."},
		{"Dre", 0.9f,
			{"Y'all see the game today? Brutal.", "I'm just here for the vibes, man.", "Dee! My favorite dealer in the building!", "If I bust, somebody tell my wife I won."},
			{"Let's gooo!", "Chip up, chip up."},
			{"All good, all good.", "That's poker, baby."},
			{"You reading me? Ain't nothing to read.", "My face is an open book, man. A boring one."},
			"That's poker, baby. Good luck, y'all."},
		{"gh0stfold", 0.3f,
			{"Same rake, worse coffee.", "Live is so slow.", "You play online?"},
			{"gg.", "ty."},
			{"nh.", "variance."},
			{"Staring won't help. I've seen your stats.", "Look at the board, not me."},
			"gg. see you online."},
	};
	return All;
}

const CastSet* FindCast(const std::string& Name)
{
	for (const CastSet& C : CastSets())
	{
		if (Name == C.Name)
		{
			return &C;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------ pools

struct Indexed
{
	std::string Key;  // pool + "." + its place in the pool
	std::string Pool;
	std::string Text;
	int Number = 0; // its place in the whole inventory: told as "#<Number>"
};

const std::vector<Indexed>& Everything()
{
	static const std::vector<Indexed> All = [] {
		std::vector<Indexed> Out;
		std::map<std::string, int> Seen;
		for (const std::vector<Template>* Set : {&FieldTemplates(), &DealerTemplates(), &FloorTemplates(), &FreshTemplates()})
		{
			for (const Template& T : *Set)
			{
				Indexed I;
				I.Pool = T.Pool;
				I.Key = I.Pool + "." + std::to_string(Seen[I.Pool]++);
				I.Text = T.Text;
				I.Number = static_cast<int>(Out.size());
				Out.push_back(std::move(I));
			}
		}
		return Out;
	}();
	return All;
}

/** The templates of one pool, in order. */
const std::vector<const Indexed*>& PoolOf(const std::string& Pool)
{
	// Built once: every pool and its templates in order (a line looks up a pool or two a hand).
	static const std::map<std::string, std::vector<const Indexed*>> Pools = [] {
		std::map<std::string, std::vector<const Indexed*>> Out;
		for (const Indexed& I : Everything())
		{
			Out[I.Pool].push_back(&I);
		}
		return Out;
	}();
	static const std::vector<const Indexed*> None;
	const auto It = Pools.find(Pool);
	return It == Pools.end() ? None : It->second;
}

/** Every {Var} the template uses is in Vars. */
bool Fillable(const std::string& Text, const std::map<std::string, std::string>& Vars)
{
	size_t I = 0;
	while ((I = Text.find('{', I)) != std::string::npos)
	{
		const size_t Close = Text.find('}', I + 1);
		if (Close == std::string::npos)
		{
			return false;
		}
		const auto It = Vars.find(Text.substr(I + 1, Close - I - 1));
		if (It == Vars.end() || It->second.empty())
		{
			return false;
		}
		I = Close + 1;
	}
	return true;
}

std::string Var(const std::map<std::string, std::string>& Vars, const char* Key)
{
	const auto It = Vars.find(Key);
	return It == Vars.end() ? std::string() : It->second;
}

/** A pool's line: the fillable templates, the untold ones first, Salt's pick among them. */
Said Pick(const std::string& Pool, const std::map<std::string, std::string>& Vars, uint32_t Salt, const std::set<std::string>* Told)
{
	std::vector<const Indexed*> Fit;
	for (const Indexed* I : PoolOf(Pool))
	{
		if (Fillable(I->Text, Vars))
		{
			Fit.push_back(I);
		}
	}
	if (Fit.empty())
	{
		return Said();
	}
	bool Last = false;
	if (Told)
	{
		std::vector<const Indexed*> Fresh;
		for (const Indexed* I : Fit)
		{
			if (!Told->count("#" + std::to_string(I->Number)))
			{
				Fresh.push_back(I);
			}
		}
		// The last untold one (or none left, from a night kept before pools were dealt fresh): Remember reshuffles.
		Last = Fresh.size() <= 1 && Fit.size() > 1;
		if (!Fresh.empty())
		{
			Fit.swap(Fresh);
		}
	}
	const Indexed* Chosen = Fit[Salt % static_cast<uint32_t>(Fit.size())];
	Said S;
	S.Exhausts = Last;
	S.Text = Fill(Chosen->Text, Vars);
	// A name that ends in its own period ("Maria G.", "J.P.") before the line's: one period, not two (an ellipsis stays).
	std::string Clean;
	for (size_t I = 0; I < S.Text.size(); ++I)
	{
		const bool Pair = S.Text[I] == '.' && I + 1 < S.Text.size() && S.Text[I + 1] == '.';
		const bool Run = Pair && ((I + 2 < S.Text.size() && S.Text[I + 2] == '.') || (I > 0 && S.Text[I - 1] == '.'));
		if (Pair && !Run)
		{
			continue;
		}
		Clean += S.Text[I];
	}
	S.Text = Clean;
	S.Key = Chosen->Key;
	S.Told = "#" + std::to_string(Chosen->Number);
	return S;
}

// ------------------------------------------------------------------ names

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return S;
}

/** First names people pick their handles from (the engine's Names.cpp and the handles' countries), lower case. */
const std::vector<std::string>& FirstNames()
{
	static const std::vector<std::string> All = [] {
		std::vector<std::string> V = {
			// Names.cpp
			"mike", "jenny", "kostas", "dave", "luis", "priya", "tomas", "anna", "jay", "marco", "sven", "olga", "kev", "nina", "raj", "sam", "leo",
			"mia", "dmitri", "yuki", "hector", "ben", "chloe", "omar", "lars", "ivy", "gus", "fran",
			// Handles.cpp's countries
			"lucas", "gabriel", "rafael", "thiago", "bruno", "felipe", "mateus", "joao", "julia", "camila", "bia", "lukas", "jonas", "felix", "max",
			"leon", "paul", "tim", "niklas", "lena", "sophie", "jule", "liam", "noah", "ethan", "owen", "logan", "emma", "maya", "jack", "ryan",
			"oliver", "harry", "george", "charlie", "alfie", "freddie", "olivia", "amelia", "isla", "tom", "ollie", "ivan", "sergei", "alexei",
			"nikita", "pavel", "artem", "irina", "katya", "maxim", "yuri", "taras", "andriy", "oleksii", "bohdan", "yaroslav", "oksana", "sofiia",
			"iryna", "mykola", "dmytro", "chris", "tyler", "jake", "kevin", "brandon", "josh", "ashley", "megan", "kayla", "derek", "cody", "daan",
			"sem", "milan", "thijs", "bram", "femke", "lotte", "sanne", "jesse", "julien", "hugo", "louis", "theo", "nathan", "manon", "antoine",
			"baptiste", "luca", "matteo", "alessandro", "lorenzo", "giulia", "chiara", "francesca", "davide", "andrea", "pablo", "javi", "alejandro",
			"sergio", "carlos", "lucia", "marta", "paula", "alvaro", "dani", "erik", "oscar", "william", "linnea", "elsa", "maja", "axel", "viktor",
			"juho", "aleksi", "eetu", "ville", "mikko", "aino", "veera", "lauri", "joni", "kuba", "piotr", "michal", "kacper", "tomek", "ola",
			"zosia", "kasia", "bartek", "wojtek", "diego", "chuy", "mateo", "sofia", "valeria", "ximena", "emiliano", "pepe", "nico", "santi",
			"facu", "agus", "valen", "juli", "lauti", "mica", "wei", "jun", "hao", "lei", "ming", "yan", "li", "xin", "chen", "bo", "kenji", "haru",
			"ren", "sota", "yuto", "yui", "hina", "aoi", "daiki", "kaito", "elias",
		};
		// Longest first, so "maxim" wins over "max" and "julia" over "juli".
		std::stable_sort(V.begin(), V.end(), [](const std::string& A, const std::string& B) { return A.size() > B.size(); });
		return V;
	}();
	return All;
}

bool IsLowerAlpha(char Ch)
{
	return Ch >= 'a' && Ch <= 'z';
}
} // namespace talk_detail

using namespace talk_detail;

const char* MomentName(Moment M)
{
	switch (M)
	{
	case Moment::SitDown: return "SitDown";
	case Moment::Small: return "Small";
	case Moment::Ask: return "Ask";
	case Moment::Answer: return "Answer";
	case Moment::Needle: return "Needle";
	case Moment::NiceHand: return "NiceHand";
	case Moment::Suckout: return "Suckout";
	case Moment::BadBeat: return "BadBeat";
	case Moment::Cooler: return "Cooler";
	case Moment::ShowedBluff: return "ShowedBluff";
	case Moment::DoubleUp: return "DoubleUp";
	case Moment::Goodbye: return "Goodbye";
	case Moment::Tank: return "Tank";
	case Moment::Bubble: return "Bubble";
	case Moment::InTheMoney: return "InTheMoney";
	case Moment::FinalTable: return "FinalTable";
	case Moment::Break: return "Break";
	case Moment::Noticed: return "Noticed";
	case Moment::Win: return "Win";
	case Moment::Lose: return "Lose";
	case Moment::Rail: return "Rail";
	}
	return "";
}

bool MomentFrom(const std::string& Name, Moment& Out)
{
	const std::string Want = Lower(Name);
	for (int I = 0; I <= static_cast<int>(Moment::Rail); ++I)
	{
		if (Lower(MomentName(static_cast<Moment>(I))) == Want)
		{
			Out = static_cast<Moment>(I);
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------ words

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

std::string NumberWords(long long N)
{
	static const char* Ones[20] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
		"thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
	static const char* Tens[10] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
	if (N < 0)
	{
		return "minus " + NumberWords(-N);
	}
	if (N < 20)
	{
		return Ones[N];
	}
	if (N < 100)
	{
		return std::string(Tens[N / 10]) + (N % 10 != 0 ? std::string("-") + Ones[N % 10] : std::string());
	}
	if (N < 1000)
	{
		return std::string(Ones[N / 100]) + " hundred" + (N % 100 != 0 ? " " + NumberWords(N % 100) : std::string());
	}
	if (N < 1000000)
	{
		return NumberWords(N / 1000) + " thousand" + (N % 1000 != 0 ? " " + NumberWords(N % 1000) : std::string());
	}
	return NumberWords(N / 1000000) + " million" + (N % 1000000 != 0 ? " " + NumberWords(N % 1000000) : std::string());
}

std::string OrdinalWord(int N)
{
	static const char* Small[20] = {"zeroth", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth",
		"eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth"};
	static const char* Tens[10] = {"", "", "twentieth", "thirtieth", "fortieth", "fiftieth", "sixtieth", "seventieth", "eightieth", "ninetieth"};
	if (N >= 0 && N < 20)
	{
		return Small[N];
	}
	if (N > 0 && N < 100)
	{
		if (N % 10 == 0)
		{
			return Tens[N / 10];
		}
		const std::string Head = NumberWords(N - N % 10);
		return Head + "-" + Small[N % 10];
	}
	if (N > 0 && N < 1000 && N % 100 == 0)
	{
		return NumberWords(N) + "th";
	}
	if (N > 100 && N < 1000)
	{
		return NumberWords(N - N % 100) + " " + OrdinalWord(N % 100);
	}
	return Ordinal(N);
}

std::string Ordinal(int N)
{
	const int Mod100 = ((N % 100) + 100) % 100;
	const int Mod10 = ((N % 10) + 10) % 10;
	const char* Suffix = (Mod100 >= 11 && Mod100 <= 13) ? "th" : Mod10 == 1 ? "st" : Mod10 == 2 ? "nd" : Mod10 == 3 ? "rd" : "th";
	return std::to_string(N) + Suffix;
}

std::string Count(long long N)
{
	const bool Negative = N < 0;
	std::string Digits = std::to_string(Negative ? -N : N);
	std::string Out;
	const int Len = static_cast<int>(Digits.size());
	for (int I = 0; I < Len; ++I)
	{
		Out += Digits[static_cast<size_t>(I)];
		const int Left = Len - 1 - I;
		if (Left > 0 && Left % 3 == 0)
		{
			Out += ',';
		}
	}
	return Negative ? "-" + Out : Out;
}

std::string Dollars(Chips Cents)
{
	const Chips Whole = Cents / 100;
	const Chips Part = Cents % 100;
	std::string Out = "$" + Count(Whole);
	if (Part != 0)
	{
		Out += "." + std::string(Part < 10 ? "0" : "") + std::to_string(Part);
	}
	return Out;
}

std::string Capitalized(std::string S)
{
	if (!S.empty() && S[0] >= 'a' && S[0] <= 'z')
	{
		S[0] = static_cast<char>(S[0] - 'a' + 'A');
	}
	return S;
}

std::vector<Result> StoryOf(const std::string& Story)
{
	// "day^event^place^entries^prizeCents^online" records joined by '|', newest first.
	std::vector<Result> Out;
	size_t Start = 0;
	while (Start < Story.size())
	{
		size_t End = Story.find('|', Start);
		if (End == std::string::npos)
		{
			End = Story.size();
		}
		const std::string Rec = Story.substr(Start, End - Start);
		std::vector<std::string> F;
		size_t A = 0;
		while (true)
		{
			const size_t B = Rec.find('^', A);
			F.push_back(Rec.substr(A, B == std::string::npos ? std::string::npos : B - A));
			if (B == std::string::npos)
			{
				break;
			}
			A = B + 1;
		}
		if (F.size() >= 6)
		{
			Result R;
			R.Day = std::atoi(F[0].c_str());
			R.Event = F[1];
			R.Place = std::atoi(F[2].c_str());
			R.Entries = std::atoi(F[3].c_str());
			R.Prize = static_cast<Chips>(std::strtoll(F[4].c_str(), nullptr, 10));
			R.Online = F[5] == "1";
			Out.push_back(R);
		}
		Start = End + 1;
	}
	return Out;
}

std::string StoryText(const std::vector<Result>& Results)
{
	std::string Out;
	for (const Result& R : Results)
	{
		std::string Event = R.Event;
		for (char& Ch : Event)
		{
			if (Ch == '|' || Ch == '^' || Ch == '\t' || Ch == '\n')
			{
				Ch = ' ';
			}
		}
		if (!Out.empty())
		{
			Out += '|';
		}
		Out += std::to_string(R.Day) + "^" + Event + "^" + std::to_string(R.Place) + "^" + std::to_string(R.Entries) + "^" + std::to_string(R.Prize) + "^" +
			(R.Online ? "1" : "0");
	}
	return Out;
}

std::string SpokenEvent(const std::string& EventName)
{
	for (const live::EventTemplate& T : live::Schedule())
	{
		if (EventName == T.Name)
		{
			return T.Short;
		}
	}
	// "River City Main $1,100" is said "River City Main"; "Embercrest Nightly" without the room's name.
	std::string Out = EventName;
	const size_t Price = Out.find(" $");
	if (Price != std::string::npos)
	{
		Out = Out.substr(0, Price);
	}
	const std::string Room = "Embercrest ";
	if (Out.rfind(Room, 0) == 0)
	{
		Out = Out.substr(Room.size());
	}
	return Out;
}

std::string DayWord(int Then, int Day)
{
	const int Ago = Day - Then;
	if (Ago <= 0)
	{
		return "today";
	}
	if (Ago == 1)
	{
		return "yesterday";
	}
	if (Ago < 7)
	{
		return net::WeekdayName(Then, true);
	}
	return "last week";
}

// ------------------------------------------------------------------ the people

bool IsCast(const std::string& Name)
{
	return FindCast(Name) != nullptr;
}

float CastChatter(const std::string& Name)
{
	const CastSet* C = FindCast(Name);
	return C ? C->Chatter : 0.0f;
}

std::vector<std::string> CastLines(const std::string& Name, Moment M)
{
	std::vector<std::string> Out;
	// Any case, as FLines' LinesFor matched (ESearchCase::IgnoreCase).
	const CastSet* C = FindCast(Name);
	for (const CastSet& Each : CastSets())
	{
		C = C ? C : Lower(Each.Name) == Lower(Name) ? &Each : nullptr;
	}
	if (!C)
	{
		return Out;
	}
	const std::vector<const char*>* From = nullptr;
	switch (M)
	{
	case Moment::Small: From = &C->Chat; break;
	case Moment::Win: From = &C->Win; break;
	case Moment::Lose: From = &C->Lose; break;
	case Moment::Noticed: From = &C->Noticed; break;
	case Moment::Goodbye: Out.push_back(C->Goodbye); return Out;
	default: return Out;
	}
	for (const char* L : *From)
	{
		Out.push_back(L);
	}
	return Out;
}

Said Cast(const std::string& Name, Moment M, uint32_t Salt)
{
	const std::vector<std::string> Lines = CastLines(Name, M);
	if (Lines.empty())
	{
		return Said();
	}
	const uint32_t I = Salt % static_cast<uint32_t>(Lines.size());
	Said S;
	S.Text = Lines[I];
	S.Key = "cast." + Lower(Name) + "." + Lower(MomentName(M)) + "." + std::to_string(I);
	S.Topic = Name + ":cast." + Lower(MomentName(M)) + "." + std::to_string(I);
	return S;
}

std::string SpokenHand(const std::vector<Card>& Hole)
{
	if (Hole.size() < 2)
	{
		return std::string();
	}
	Card A = Hole[0];
	Card B = Hole[1];
	if (RankOf(B) > RankOf(A))
	{
		std::swap(A, B);
	}
	if (RankOf(A) == RankOf(B))
	{
		return Lower(RankPlurals[RankOf(A)]);
	}
	std::string Out = Lower(RankNames[RankOf(A)]) + "-" + Lower(RankNames[RankOf(B)]);
	if (SuitOf(A) == SuitOf(B))
	{
		Out += " suited";
	}
	return Out;
}

bool KnownByName(const std::string& Name)
{
	return IsCast(Name) || live::FindRoomLocal(Name) != nullptr;
}

std::string FirstNameIn(const std::string& Handle)
{
	const std::string Low = Lower(Handle);
	for (const std::string& F : FirstNames())
	{
		if (Low.size() < F.size() || Low.compare(0, F.size(), F) != 0)
		{
			continue;
		}
		const std::string Rest = Handle.substr(F.size());
		const std::string Head = Handle.substr(0, F.size());
		bool HeadUpper = Head.size() >= 2;
		for (char Ch : Head)
		{
			HeadUpper = HeadUpper && Ch >= 'A' && Ch <= 'Z';
		}
		// The name ends where the handle shows it: the end, a digit, a separator, a capital ("DmitriVolkov", "MIKE_Shark"),
		// an initial ("lucasm"), or a whole surname after a name long enough not to be its start ("olgapetrov").
		const bool Ends = Rest.empty() || !IsLowerAlpha(Rest[0]) || HeadUpper || Rest.size() == 1 || (F.size() >= 4 && Rest.size() >= 4);
		if (Ends)
		{
			return Capitalized(F);
		}
	}
	return std::string();
}

std::string CaptionName(const std::string& Id, const std::string& Name, int Seat)
{
	if (Id == HeroId)
	{
		return Name;
	}
	if (KnownByName(Name))
	{
		return Name;
	}
	const std::string First = FirstNameIn(Name);
	if (!First.empty())
	{
		return First;
	}
	return "Seat " + std::to_string(Seat >= 0 ? Seat + 1 : 1);
}

// ------------------------------------------------------------------ lines

Said Line(Moment M, const Context& C)
{
	const life::LiveVoice* Self = C.Self;
	const std::string Topic = Var(C.Vars, "Topic");
	const std::string Name = Self ? Self->Name : std::string();
	// The Back Room's eight say their own lines where they have them (at the Embercrest, not the ones that belong to
	// Dee's game: the dryer downstairs, and Dee herself unless she's dealing).
	if (Self && IsCast(Name) && Topic.empty() && (M == Moment::Small || M == Moment::Win || M == Moment::Lose || M == Moment::Goodbye))
	{
		const bool Embercrest = Var(C.Vars, "Venue") == "embercrest";
		const bool DeeDeals = Var(C.Vars, "Dealer") == "Dee";
		const std::vector<std::string> All = CastLines(Name, M);
		std::vector<int> Fit;
		for (size_t I = 0; I < All.size(); ++I)
		{
			const bool Dryer = All[I].find("dryer") != std::string::npos;
			const bool Dee = All[I].find("Dee") != std::string::npos;
			if (Embercrest && (Dryer || (Dee && !DeeDeals)))
			{
				continue;
			}
			const std::string Spent = Name + ":cast." + Lower(MomentName(M)) + "." + std::to_string(I);
			if (C.Told && C.Told->count(Spent) && M == Moment::Small)
			{
				continue; // their chat lines once a night each
			}
			Fit.push_back(static_cast<int>(I));
		}
		if (!Fit.empty())
		{
			return Cast(Name, M, static_cast<uint32_t>(Fit[C.Salt % static_cast<uint32_t>(Fit.size())]));
		}
		if (M != Moment::Small)
		{
			return Said();
		}
	}
	std::string Pool;
	std::map<std::string, std::string> Vars = C.Vars;
	std::string Spends;
	switch (M)
	{
	case Moment::SitDown:
	{
		const std::string Greeting = Var(C.Vars, "Greeting");
		if (!Greeting.empty())
		{
			Said S;
			S.Text = Greeting;
			S.Key = "face.greeting";
			S.Topic = Name + ":greeting";
			return S;
		}
		Pool = "field.sitdown";
		break;
	}
	case Moment::Small:
	case Moment::Answer:
	{
		const bool Reply = M == Moment::Answer;
		if (Topic.rfind("fresh.", 0) == 0)
		{
			// A fresh start's first month (update 2 §3.3 F9): only from the director, which reads the room's books and offers a
			// topic only while it's true ("Books"), each once a night ("FreshKey"). The answers claim nothing; a line to the
			// player ("...you") gets none.
			const bool Mine = Topic.size() > 5 && Topic.compare(Topic.size() - 5, 5, ".self") == 0;
			const bool Yours = Topic.size() > 4 && Topic.compare(Topic.size() - 4, 4, ".you") == 0;
			if (Reply)
			{
				if (Yours)
				{
					return Said();
				}
				const bool About = Topic.rfind("fresh.first", 0) == 0 || Topic.rfind("fresh.wallfirst", 0) == 0 || Topic.rfind("fresh.twice", 0) == 0;
				Pool = Mine ? "field.fresh.self.reply" : About ? "field.fresh.first.reply" : "field.fresh.reply";
				break;
			}
			if (Var(C.Vars, "Books") != "fresh" || Var(C.Vars, "FreshKey").empty())
			{
				return Said();
			}
			Spends = "fresh:" + Var(C.Vars, "FreshKey");
			if (C.Told && C.Told->count(Spends))
			{
				return Said();
			}
			Pool = "field." + Topic;
			break;
		}
		std::string T = Topic.empty() ? std::string("night") : Topic;
		if (!Reply && Self)
		{
			// Gates: only what's true of the speaker.
			const int Mood = Self->Mood;
			if (T == "heater" && !(Mood == static_cast<int>(world::Momentum::Heater) || Mood == static_cast<int>(world::Momentum::Breakout) ||
									 Mood == static_cast<int>(world::Momentum::Confident)))
			{
				return Said();
			}
			if (T == "down" && !(Mood == static_cast<int>(world::Momentum::Downswing) || Mood == static_cast<int>(world::Momentum::MajorDownswing) ||
								   Mood == static_cast<int>(world::Momentum::Rebuilding)))
			{
				return Said();
			}
			if (T == "online" && Self->Is != static_cast<int>(world::Identity::OnlineGrinder))
			{
				return Said();
			}
			if (T == "regular" && Self->Is != static_cast<int>(world::Identity::LiveRegular))
			{
				return Said();
			}
			if (T == "makeup" && !Self->Staked)
			{
				return Said();
			}
			if (T == "friend" || T == "rival")
			{
				// Only to someone they're tied to that way (TieKind: Friend, HomeGame, TravelPartner talk; Rival needles).
				bool Tied = false;
				for (const std::pair<std::string, int>& Tie : Self->Ties)
				{
					const bool Friendly = Tie.second == static_cast<int>(world::TieKind::Friend) || Tie.second == static_cast<int>(world::TieKind::HomeGame) ||
						Tie.second == static_cast<int>(world::TieKind::TravelPartner);
					Tied = Tied || (C.To && Tie.first == C.To->Name && (T == "friend" ? Friendly : Tie.second == static_cast<int>(world::TieKind::Rival)));
				}
				if (!Tied)
				{
					return Said();
				}
			}
			if (T == "week" || T == "bubble")
			{
				// One of their own results, as their Story has it.
				const std::vector<Result> Story = StoryOf(Self->Story);
				const int Which = std::atoi(Var(C.Vars, "Record").c_str());
				if (Which < 0 || Which >= static_cast<int>(Story.size()) || Var(C.Vars, "Record").empty())
				{
					return Said();
				}
				const Result& R = Story[static_cast<size_t>(Which)];
				const int Day = Var(C.Vars, "Day").empty() ? R.Day : std::atoi(Var(C.Vars, "Day").c_str());
				const std::string When = DayWord(R.Day, Day);
				Vars["PlaceWord"] = Capitalized(OrdinalWord(R.Place));
				Vars["PlaceLower"] = OrdinalWord(R.Place);
				Vars["EventSpoken"] = SpokenEvent(R.Event);
				Vars["When"] = When;
				Vars["WhenCap"] = Capitalized(When);
				Vars["Entries"] = std::to_string(R.Entries);
				Vars["Prize"] = R.Prize > 0 ? Dollars((R.Prize + 50) / 100 * 100) : std::string(); // said to the dollar
				if (T == "bubble")
				{
					// A bubble is a result with nothing paid (the record says it, or it isn't one).
					if (R.Prize > 0)
					{
						return Said();
					}
				}
				else if (R.Prize <= 0)
				{
					return Said();
				}
				else if (R.Place == 1)
				{
					T = "week.win";
				}
				else if (R.Online)
				{
					T = "week.online";
				}
				Spends = "story:" + Name + ":" + std::to_string(R.Day) + ":" + R.Event;
				if (C.Told && C.Told->count(Spends))
				{
					return Said();
				}
			}
		}
		if (Reply && (T == "week.win" || T == "week.online"))
		{
			T = "week";
		}
		Pool = "field.small." + T + (Reply ? ".reply" : "");
		if (Reply && T != "week" && T != "bubble" && PoolOf(Pool).empty())
		{
			Pool = "field.small.night.reply";
		}
		if (!Reply && PoolOf(Pool).empty() && PoolOf(Pool + ".open").size() > 0)
		{
			Pool += ".open";
		}
		if (Spends.empty() && !Reply && T != "trip" && T != "food" && T != "room" && T != "night")
		{
			Spends = Name + ":small." + T; // what's personal comes up once a night; the trip, the food and the room don't run out
		}
		break;
	}
	case Moment::Ask: Pool = "field.ask." + (Topic.empty() ? std::string("average") : Topic); break;
	case Moment::Needle:
	{
		if (!Self)
		{
			return Said();
		}
		const bool Rival = Self->Rivalry >= 0.4f;
		if (!Rival && Self->Aggro < 0.6f)
		{
			return Said(); // only the aggressive and the rivals needle
		}
		Pool = Topic == "tank" ? "field.needle.tank" : Rival ? "field.needle.rival" : "field.needle";
		break;
	}
	case Moment::ShowedBluff:
		if (!Self || (Self->Rivalry < 0.4f && Self->Aggro < 0.6f))
		{
			return Said();
		}
		Pool = "field.needle.bluff";
		break;
	case Moment::Tank: Pool = "field.tank"; break;
	case Moment::NiceHand: Pool = Topic.empty() ? "field.nicehand" : "field.nicehand." + Topic; break;
	case Moment::Suckout: Pool = "field.suckout"; break;
	case Moment::BadBeat: Pool = "field.badbeat"; break;
	case Moment::Cooler: Pool = "field.cooler"; break;
	case Moment::DoubleUp: Pool = "field.doubleup"; break;
	case Moment::Goodbye: Pool = Topic.empty() ? "field.goodbye" : "field.goodbye." + Topic; break;
	case Moment::Bubble: Pool = "field.bubble"; break;
	case Moment::InTheMoney: Pool = "field.money"; break;
	case Moment::FinalTable: Pool = "field.final"; break;
	case Moment::Break: Pool = Topic == "beat" ? "field.break.beat" : "field.break"; break;
	case Moment::Noticed:
	{
		const std::string Brand = Topic;
		if (Brand.empty())
		{
			return Said();
		}
		Pool = "field.noticed." + Brand;
		if (PoolOf(Pool).empty())
		{
			Pool = "field.noticed." + (Var(C.Vars, "Category").empty() ? std::string("other") : Var(C.Vars, "Category"));
		}
		if (PoolOf(Pool).empty())
		{
			Pool = "field.noticed.other";
		}
		Spends = "noticed:" + Brand;
		if (C.Told && C.Told->count(Spends))
		{
			return Said();
		}
		break;
	}
	case Moment::Win: Pool = "field.win"; break;
	case Moment::Lose: Pool = "field.lose"; break;
	case Moment::Rail: Pool = Topic.empty() ? "field.rail" : "field.rail." + Topic; break;
	}
	Said S = Pick(Pool, Vars, C.Salt, C.Told);
	if (!S.Text.empty())
	{
		S.Topic = Spends;
	}
	return S;
}

void Remember(std::set<std::string>& Told, const Said& S)
{
	if (!S.Topic.empty())
	{
		Told.insert(S.Topic);
	}
	if (S.Told.empty())
	{
		return;
	}
	Told.insert(S.Told);
	const int Number = std::atoi(S.Told.c_str() + 1);
	const std::vector<Indexed>& All = Everything();
	if (!S.Exhausts || Number < 0 || Number >= static_cast<int>(All.size()))
	{
		return;
	}
	// The pool's been through: deal it fresh, keeping only the line just said out of the next pick.
	for (const Indexed* I : PoolOf(All[static_cast<size_t>(Number)].Pool))
	{
		if (I->Number != Number)
		{
			Told.erase("#" + std::to_string(I->Number));
		}
	}
}

Said Dealer(const std::string& Call, const Context& C)
{
	if (Var(C.Vars, "Dealer") == "Dee" && !PoolOf("dealer." + Call + ".dee").empty())
	{
		return Pick("dealer." + Call + ".dee", C.Vars, C.Salt, nullptr);
	}
	return Pick("dealer." + Call, C.Vars, C.Salt, nullptr);
}

Said Floor(const std::string& Call, const std::map<std::string, std::string>& Vars, uint32_t Salt)
{
	return Pick("pa." + Call, Vars, Salt, nullptr);
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (const Indexed& I : Everything())
	{
		Out.push_back({I.Key, I.Text});
	}
	for (const CastSet& C : CastSets())
	{
		for (const Moment M : {Moment::Small, Moment::Win, Moment::Lose, Moment::Noticed, Moment::Goodbye})
		{
			const std::vector<std::string> Lines = CastLines(C.Name, M);
			for (size_t I = 0; I < Lines.size(); ++I)
			{
				Out.push_back({"cast." + Lower(C.Name) + "." + Lower(MomentName(M)) + "." + std::to_string(I), Lines[I]});
			}
		}
	}
	return Out;
}
} // namespace talk
} // namespace ss
