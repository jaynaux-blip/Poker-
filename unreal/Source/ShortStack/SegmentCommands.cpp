// The next segment's debug commands (next_segment_spec 2.13, 3.10, 5.7): backing, sponsors and badges, forwarded to the
// session's Debug* (core-stake's SessionStake.cpp, core-sponsor's SessionSponsor.cpp, core-badges' SessionBadges.cpp).
// They run where the session lives, at home (Night One): offers, contracts and badges are the career's, and the card
// room has a save, not a session.
//
// Each command passes its verb and its arguments to the session as one line, the verb in lower case and the arguments as
// typed: "ss.Stake.Offer lou" is DebugStake("offer lou"), "ss.Badge.Grant KastPartner" is DebugBadge("grant KastPartner"),
// "ss.Sponsor.Describe" is DebugSponsor("describe"). The session's answer is logged a line at a time.
//
//   ss.Stake.Describe | .Offer <lou|dee|mei|rick|lanternrow|sam> | .Advance <hours> | .Grant <lanternrow|sam|finch>
//            | .List <event id> <percent> [markup] | .Pay
//   ss.Sponsor.Describe | .Standing <0-5> | .Offer <brand> | .Sign <brand> | .Period | .Breach <pickedup|restricted>
//              | .Patch <chest|sleeve|cap> [brand] | .Appear <venue>
//   ss.Badge.Grant <kind> [tier|count] | .Describe <name|me> | .Atlas | .Review

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NightOneGame.h"
#include "NightOneGameMode.h"
#include "ShortStack/Game/Session.h"

#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogSegment, Log, All);

namespace SegmentDetail
{
/** The career's session in this world: Night One's (the apartment), or none. */
ss::Session* SessionIn(UWorld* World)
{
	ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	FNightOneGame* Game = Mode ? Mode->GetGame() : nullptr;
	return Game ? &Game->Session : nullptr;
}

enum class EFeature : uint8
{
	Stake,
	Sponsor,
	Badge,
};

void Forward(EFeature Feature, const TCHAR* Verb, const TArray<FString>& Args, UWorld* World)
{
	const TCHAR* Name = Feature == EFeature::Stake ? TEXT("ss.Stake") : Feature == EFeature::Sponsor ? TEXT("ss.Sponsor") : TEXT("ss.Badge");
	ss::Session* S = SessionIn(World);
	if (!S)
	{
		UE_LOG(LogSegment, Warning, TEXT("%s.%s: only at home (Night One), where the career's session lives"), Name, Verb);
		return;
	}
	FString Line = FString(Verb).ToLower();
	for (const FString& A : Args)
	{
		Line += TEXT(" ") + A;
	}
	const std::string In(TCHAR_TO_UTF8(*Line));
	const std::string Out = Feature == EFeature::Stake ? S->DebugStake(In) : Feature == EFeature::Sponsor ? S->DebugSponsor(In) : S->DebugBadge(In);
	TArray<FString> Lines;
	FString(UTF8_TO_TCHAR(Out.c_str())).ParseIntoArrayLines(Lines, false);
	UE_LOG(LogSegment, Display, TEXT("%s %s"), Name, *Line);
	for (const FString& L : Lines)
	{
		UE_LOG(LogSegment, Display, TEXT("  %s"), *L);
	}
}

/** One console command, "ss.<Feature>.<Verb>", forwarding to the session. */
struct FSegmentCommand
{
	FSegmentCommand(EFeature Feature, const TCHAR* Command, const TCHAR* Verb, const TCHAR* Help)
		: Cmd(Command, Help, FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([Feature, Verb](const TArray<FString>& Args, UWorld* World) { Forward(Feature, Verb, Args, World); }))
	{
	}
	FAutoConsoleCommandWithWorldAndArgs Cmd;
};

// Backing (next_segment_spec 2.13).
FSegmentCommand StakeDescribe(EFeature::Stake, TEXT("ss.Stake.Describe"), TEXT("Describe"), TEXT("Backing: the stakes, offers, pieces, swaps, packages, owed and the record (testing)."));
FSegmentCommand StakeOffer(EFeature::Stake, TEXT("ss.Stake.Offer"), TEXT("Offer"), TEXT("ss.Stake.Offer <lou|dee|mei|rick|lanternrow|sam>: that backer offers now (testing)."));
FSegmentCommand StakeAdvance(EFeature::Stake, TEXT("ss.Stake.Advance"), TEXT("Advance"), TEXT("ss.Stake.Advance <hours>: the backing book moves on (listings fill, due dates come) (testing)."));
FSegmentCommand StakeGrant(EFeature::Stake, TEXT("ss.Stake.Grant"), TEXT("Grant"), TEXT("ss.Stake.Grant <lanternrow|sam|finch>: a package offered and signed now (testing)."));
FSegmentCommand StakeList(EFeature::Stake, TEXT("ss.Stake.List"), TEXT("List"),
	TEXT("ss.Stake.List <event id> <percent> [markup]: pieces of an event put up for sale at a markup (1.05; par when left out), event ids as ss.Live.Describe lists them (testing)."));
FSegmentCommand StakePay(EFeature::Stake, TEXT("ss.Stake.Pay"), TEXT("Pay"), TEXT("Pays the first debt the player owes a backer or a buyer now (testing)."));
// Sponsors (3.10).
FSegmentCommand SponsorDescribe(EFeature::Sponsor, TEXT("ss.Sponsor.Describe"), TEXT("Describe"), TEXT("Sponsors: standing, the contract book, asks, strikes, patches (testing)."));
FSegmentCommand SponsorStanding(EFeature::Sponsor, TEXT("ss.Sponsor.Standing"), TEXT("Standing"), TEXT("ss.Sponsor.Standing <0-5>: the player's standing set (testing)."));
FSegmentCommand SponsorOffer(EFeature::Sponsor, TEXT("ss.Sponsor.Offer"), TEXT("Offer"), TEXT("ss.Sponsor.Offer <brand>: that brand's offer arrives now (testing)."));
FSegmentCommand SponsorSign(EFeature::Sponsor, TEXT("ss.Sponsor.Sign"), TEXT("Sign"), TEXT("ss.Sponsor.Sign <brand>: signs that brand's contract (testing)."));
FSegmentCommand SponsorPeriod(EFeature::Sponsor, TEXT("ss.Sponsor.Period"), TEXT("Period"), TEXT("The sponsors' review comes now: stipends, asks met or missed, strikes (testing)."));
FSegmentCommand SponsorBreach(EFeature::Sponsor, TEXT("ss.Sponsor.Breach"), TEXT("Breach"), TEXT("ss.Sponsor.Breach <pickedup|restricted>: a conduct breach now (testing)."));
FSegmentCommand SponsorPatch(EFeature::Sponsor, TEXT("ss.Sponsor.Patch"), TEXT("Patch"),
	TEXT("ss.Sponsor.Patch <chest|sleeve|cap> [brand]: a running deal's patch sewn on that spot; no brand takes it off (testing)."));
FSegmentCommand SponsorAppear(EFeature::Sponsor, TEXT("ss.Sponsor.Appear"), TEXT("Appear"),
	TEXT("ss.Sponsor.Appear <venue>: an appearance made there now (store, barber, embercrest), inside its window (testing)."));
// Badges (5.7).
FSegmentCommand BadgeGrant(EFeature::Badge, TEXT("ss.Badge.Grant"), TEXT("Grant"), TEXT("ss.Badge.Grant <kind> [tier|count]: the player holds that badge now (testing)."));
FSegmentCommand BadgeDescribe(EFeature::Badge, TEXT("ss.Badge.Describe"), TEXT("Describe"), TEXT("ss.Badge.Describe <name|me>: someone's badges and how they were earned."));
FSegmentCommand BadgeAtlas(EFeature::Badge, TEXT("ss.Badge.Atlas"), TEXT("Atlas"), TEXT("The badge atlas: its cells and glyphs."));
FSegmentCommand BadgeReview(EFeature::Badge, TEXT("ss.Badge.Review"), TEXT("Review"), TEXT("The month's badge review now: Verified, the club, the room's leaders (testing)."));
} // namespace SegmentDetail
