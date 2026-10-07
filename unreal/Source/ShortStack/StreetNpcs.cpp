#include "StreetNpcs.h"

#include "BackRoomPlayer.h"
#include "Camera/PlayerCameraManager.h"
#include "CastLooks.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
// Before the street's headers: StreetNs.h's inline helpers hold light components by weak pointer.
#include "Components/LightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FrameBudget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "ShopStage.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"
#include "ShortStackCharacter.h"
#include "StreetAnim.h"
#include "StreetCrowd.h"
#include "StreetStage.h"
#include "StreetWorld.h"
#include "TalkFace.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetNpcs, Log, All);

SHORTSTACK_STREET_PRELOAD(AStreetNpcs)

// Picks recorded here (spec 6.6, 6.7's ranges): tiers by camera distance at 8 / 25 / 45 m with 1.5 m of room either way;
// the budget re-run five times a second; the frame budget "at its floor" after two seconds at r.ScreenPercentage 60 or less,
// which cuts two pedestrians; the load's fade held black up to 10 s for the pool; one build a frame while the screen fades,
// two while it's black; the schedule read every world minute; someone due on comes on where nobody sees, looked for three
// times a second; the clerk's beats 8-22 s each (idle 45%, the phone 30%, the rag 25%); the waitress along the counter at
// 1.15 m/s, up to 3.8 m either way of her station; Sal behind the paper 65% of his morning; the name tag after 0.4 s of a
// look within 4 m, fading in over 0.25 s and out over 0.4. The keepers' closing lines are the game mode's (it says them
// through Speak). Dee's regulars on the fallback schedule (no career) at 20:40 (Sal, a cigarette by the Wash & Fold until
// 20:58), 20:50 (Mei), 20:55 (Big Lou), 21:10 (Twitch, running), out again one by one from 03:20 to 04:40.

namespace StreetNpcsDetail
{
TAutoConsoleVariable<int32> CVarBudget(TEXT("ss.Npc.Budget"), -1, TEXT("The street's live NPC MetaHumans at most (-1: spec 6.7's 10; the crowd takes what the staff leave)."));
TAutoConsoleVariable<float> CVarHour(TEXT("ss.Npc.Hour"), -1.0f, TEXT("The people's schedule read at this hour of today (-1: the clock's). Only who is where; the clock itself runs on."));
TAutoConsoleVariable<int32> CVarDebug(TEXT("ss.Npc.Debug"), 0, TEXT("1: draws the people's tiers and states over their heads."));
FStreetOverrides::FRegister BudgetOverride(TEXT("ss.Npc.Budget"), TEXT("-1"));
FStreetOverrides::FRegister HourOverride(TEXT("ss.Npc.Hour"), TEXT("-1"));

constexpr int32 MaxLive = 10;
constexpr int32 MaxVisible = 8;
constexpr int32 MaxNear = 4;
constexpr double NearRange = 800.0;
constexpr double MidRange = 2500.0;
constexpr double FarRange = 4500.0;
constexpr double TierRoom = 150.0;
constexpr int32 MaxPooled = 14;

FString Utf8(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

std::string Std(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

double TimeOfDay(double Minutes)
{
	return Minutes - FMath::FloorToDouble(Minutes / ss::net::MinutesPerDay) * ss::net::MinutesPerDay;
}

/** The block's people (spec 6.1, 6.3): what the director knows of them before (and beside) core's town::People(). */
struct FRosterRow
{
	/** town::People()'s id. */
	const TCHAR* Id;
	const TCHAR* Name;
	const TCHAR* Label;
	const TCHAR* Venue;
	/** The station they work (staff), or none. */
	const TCHAR* Station;
	/** A world person's table MetaHuman (MHC_<Cast>) and the name the world and the table know them by (their clothes, CastLooks::PersonaFor); staff take their face from the face registry and their clothes from CastLooks::WearFor. */
	const TCHAR* Cast;
	const TCHAR* WorldName;
	bool bWoman;
	bool bStaff;
};

const FRosterRow Roster[] = {
	{TEXT("benny"), TEXT("Benny"), TEXT("Night clerk"), TEXT("store"), TEXT("store-counter"), nullptr, nullptr, false, true},
	{TEXT("ray"), TEXT("Ray"), TEXT("Owner"), TEXT("store"), TEXT("store-counter"), nullptr, nullptr, false, true},
	{TEXT("lonnie"), TEXT("Lonnie"), TEXT("Barber"), TEXT("barber"), TEXT("barber-chair1"), nullptr, nullptr, false, true},
	{TEXT("bea"), TEXT("Bea"), TEXT("Volunteer"), TEXT("thrift"), TEXT("thrift-register"), nullptr, nullptr, true, true},
	{TEXT("rosa"), TEXT("Rosa"), TEXT("Trainer"), TEXT("gym"), TEXT("gym-desk"), nullptr, nullptr, true, true},
	{TEXT("ruby"), TEXT("Ruby"), TEXT("Owner"), TEXT("diner"), TEXT("diner-counter"), nullptr, nullptr, true, true},
	{TEXT("marlene"), TEXT("Marlene"), TEXT("Waitress"), TEXT("diner"), TEXT("diner-counter"), nullptr, nullptr, true, true},
	{TEXT("augie"), TEXT("Augie"), TEXT("Pawnbroker"), TEXT("pawn"), TEXT("pawn-counter"), nullptr, nullptr, false, true},
	{TEXT("inez"), TEXT("Inez"), TEXT("Attendant"), TEXT("washfold"), TEXT("washfold-counter"), nullptr, nullptr, true, true},
	{TEXT("sal"), TEXT("Sal"), TEXT("Regular"), TEXT(""), nullptr, TEXT("Sal"), TEXT("Sal"), false, false},
	{TEXT("mei"), TEXT("Mei"), TEXT("Regular"), TEXT(""), nullptr, TEXT("Mei"), TEXT("Mei"), true, false},
	{TEXT("big-lou"), TEXT("Big Lou"), TEXT("Regular"), TEXT(""), nullptr, TEXT("BigLou"), TEXT("Big Lou"), false, false},
	{TEXT("twitch"), TEXT("Twitch"), TEXT("Regular"), TEXT(""), nullptr, TEXT("Twitch"), TEXT("Twitch"), false, false},
	{TEXT("mrs-park"), TEXT("Mrs. Park"), TEXT("Regular"), TEXT(""), nullptr, TEXT("MrsPark"), TEXT("Mrs. Park"), true, false},
};

// Core's town (Town.h): Dee's back room is the station "dee-table" (off the street: whoever is there is inside), the
// sidewalk by the Wash & Fold's door is "washfold-door" (Dee's regulars walking up to it, Sal's cigarette, walking off from
// it), Ruby's way out is "diner-door".
const FName DeeTable(TEXT("dee-table"));
const FName WashDoorStation(TEXT("washfold-door"));
const FName DinerDoor(TEXT("diner-door"));

/** Dee's regulars on her nights (spec 6.3): when each walks up (minutes into the game day, the fallback's), how. */
struct FArrival
{
	const TCHAR* Id;
	int32 Arrive;
	int32 Leave; // minutes after midnight, the morning after
	bool bMarket;
	bool bSmoke;
	bool bUmbrella;
	float Pace;
};
const FArrival Arrivals[] = {
	{TEXT("sal"), 20 * 60 + 40, 3 * 60 + 20, true, true, false, 1.0f},
	{TEXT("mei"), 20 * 60 + 50, 3 * 60 + 50, false, false, true, 1.0f},
	{TEXT("big-lou"), 20 * 60 + 55, 4 * 60 + 20, false, false, false, 0.9f},
	{TEXT("twitch"), 21 * 60 + 10, 4 * 60 + 40, false, false, false, 1.7f},
};

/** A beat in a keeper's idle cycle and its share of the time (null: nothing in the hands). */
struct FBeatOdds
{
	const TCHAR* Beat;
	float Odds;
};

/** What a hand prop is and how a hand holds it (spec 4.5's SM_NS_Prop_*: origin at the grip, +X along it, Z up as held). */
enum class EPropAxis : uint8
{
	Across,
	Fingers,
	Palm,
	NegAcross,
	NegFingers,
	NegPalm,
};
struct FPropSpec
{
	const TCHAR* Mesh;
	int32 Side;
	EPropAxis X;
	EPropAxis Z;
};

/** The props a beat puts in the hands (the cut's by its phase, as StreetAnim's "cut" cycles). */
TArray<FPropSpec> PropsFor(FName Beat, float BeatTime)
{
	const FString B = Beat.ToString();
	if (B == TEXT("cut"))
	{
		const float C = FMath::Fmod(BeatTime, 16.0f);
		if (C >= 6.0f && C < 10.0f)
		{
			return {{TEXT("SM_NS_Prop_Clippers"), 1, EPropAxis::Across, EPropAxis::Palm}};
		}
		if (C >= 14.0f)
		{
			return {{TEXT("SM_NS_Prop_NeckDuster"), 1, EPropAxis::Across, EPropAxis::Palm}};
		}
		return {{TEXT("SM_NS_Prop_Comb"), 0, EPropAxis::Across, EPropAxis::NegFingers}, {TEXT("SM_NS_Prop_Scissors"), 1, EPropAxis::Fingers, EPropAxis::Palm}};
	}
	if (B == TEXT("comb"))
	{
		return {{TEXT("SM_NS_Prop_Comb"), 0, EPropAxis::Across, EPropAxis::NegFingers}, {TEXT("SM_NS_Prop_Scissors"), 1, EPropAxis::Fingers, EPropAxis::Palm}};
	}
	if (B == TEXT("clippers"))
	{
		return {{TEXT("SM_NS_Prop_Clippers"), 1, EPropAxis::Across, EPropAxis::Palm}};
	}
	if (B == TEXT("brush"))
	{
		return {{TEXT("SM_NS_Prop_NeckDuster"), 1, EPropAxis::Across, EPropAxis::Palm}};
	}
	if (B == TEXT("pour"))
	{
		return {{TEXT("SM_NS_Prop_Carafe"), 1, EPropAxis::Palm, EPropAxis::Across}};
	}
	if (B == TEXT("plate"))
	{
		return {{TEXT("SM_NS_Prop_Plate"), 1, EPropAxis::Fingers, EPropAxis::NegPalm}};
	}
	if (B == TEXT("pad"))
	{
		return {{TEXT("SM_NS_Prop_Notepad"), 0, EPropAxis::Fingers, EPropAxis::Palm}, {TEXT("SM_NS_Prop_Pen"), 1, EPropAxis::Across, EPropAxis::Palm}};
	}
	if (B == TEXT("wipe"))
	{
		return {{TEXT("SM_NS_Prop_Rag"), 1, EPropAxis::Fingers, EPropAxis::NegPalm}};
	}
	if (B == TEXT("phone") || B == TEXT("phonestand") || B == TEXT("phonesit"))
	{
		return {{TEXT("SM_NS_Prop_Phone"), 1, EPropAxis::Fingers, EPropAxis::Palm}};
	}
	if (B == TEXT("paper"))
	{
		// The Herald folded in quarters, held up by its fold, its face to the reader.
		return {{TEXT("SM_NS_Prop_Newspaper"), 1, EPropAxis::Fingers, EPropAxis::Palm}};
	}
	if (B == TEXT("eat"))
	{
		return {{TEXT("SM_NS_Prop_Fork"), 1, EPropAxis::Across, EPropAxis::Palm}};
	}
	if (B == TEXT("coffee"))
	{
		return {{TEXT("SM_NS_Prop_Mug"), 1, EPropAxis::Palm, EPropAxis::Across}};
	}
	return {};
}

/** Where a beat's eyes rest in the station's frame (F ahead, R right, U up; cm), or false for none (the hero, idly). */
bool BeatFocus(FName B, float SeatHeight, FVector& Out)
{
	// Compared as names (this runs every frame for everyone placed: no strings made).
	static const FName Cut(TEXT("cut")), Comb(TEXT("comb")), Clippers(TEXT("clippers")), Brush(TEXT("brush")), Pour(TEXT("pour")), Pad(TEXT("pad")), Wipe(TEXT("wipe")),
		Ring(TEXT("ring")), Phone(TEXT("phone")), PhoneStand(TEXT("phonestand")), Eat(TEXT("eat")), Coffee(TEXT("coffee")), Paper(TEXT("paper")), PhoneSit(TEXT("phonesit"));
	if (B == Cut || B == Comb || B == Clippers || B == Brush)
	{
		Out = FVector(50.0, 0.0, 122.0);
	}
	else if (B == Pour)
	{
		Out = FVector(52.0, 6.0, 115.0);
	}
	else if (B == Pad)
	{
		Out = FVector(26.0, 0.0, 114.0);
	}
	else if (B == Wipe)
	{
		Out = FVector(46.0, 8.0, 97.0);
	}
	else if (B == Ring)
	{
		Out = FVector(42.0, 18.0, 100.0);
	}
	else if (B == Phone)
	{
		Out = FVector(26.0, 6.0, 113.0);
	}
	else if (B == PhoneStand)
	{
		Out = FVector(24.0, 4.0, 112.0);
	}
	else if (B == Eat || B == Coffee)
	{
		Out = FVector(32.0, 0.0, SeatHeight + 30.0);
	}
	else if (B == Paper)
	{
		Out = FVector(34.0, 2.0, SeatHeight + 50.0);
	}
	else if (B == PhoneSit)
	{
		Out = FVector(26.0, 4.0, SeatHeight + 34.0);
	}
	else
	{
		return false;
	}
	return true;
}

/** Glass and the like (every material on it translucent): it hides nobody standing behind it. */
bool SeeThrough(const UPrimitiveComponent* C)
{
	const int32 N = C ? C->GetNumMaterials() : 0;
	for (int32 I = 0; I < N; ++I)
	{
		const UMaterialInterface* M = C->GetMaterial(I);
		const EBlendMode Mode = M ? M->GetBlendMode() : BLEND_Opaque;
		if (Mode == BLEND_Opaque || Mode == BLEND_Masked)
		{
			return false;
		}
	}
	return N > 0;
}

/** The hand props as loaded (the street's preload list has them in memory already), and those not imported yet (asked once a street). */
TMap<FName, TWeakObjectPtr<UStaticMesh>> PropsLoaded;
TSet<FName> PropsMissing;

UStaticMesh* PropMesh(const TCHAR* Name)
{
	const FName Key(Name);
	if (const TWeakObjectPtr<UStaticMesh>* Found = PropsLoaded.Find(Key); Found && Found->IsValid())
	{
		return Found->Get();
	}
	if (PropsMissing.Contains(Key))
	{
		return nullptr;
	}
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FStreetPreload::MeshPath(Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Mesh)
	{
		PropsLoaded.Add(Key, Mesh);
	}
	else
	{
		PropsMissing.Add(Key);
	}
	return Mesh;
}

/** A bone's transform in its mesh's component space, in the reference pose. */
FTransform RefPose(const FReferenceSkeleton& Ref, int32 Bone)
{
	FTransform T = Ref.GetRefBonePose()[Bone];
	for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
	{
		T = T * Ref.GetRefBonePose()[Parent];
	}
	return T;
}

/** A prop in a hand (in the hand bone's space, from the reference pose), or null when the mesh or the hand isn't there. */
UStaticMeshComponent* PutInHand(AShortStackCharacter* Body, const FPropSpec& Spec)
{
	USkeletalMeshComponent* Skin = Body ? Body->GetMesh() : nullptr;
	const USkeletalMesh* Asset = Skin ? Skin->GetSkeletalMeshAsset() : nullptr;
	UStaticMesh* Mesh = PropMesh(Spec.Mesh);
	if (!Asset || !Mesh)
	{
		return nullptr;
	}
	const TCHAR* S = Spec.Side == 0 ? TEXT("l") : TEXT("r");
	const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
	const FName HandName(*FString::Printf(TEXT("hand_%s"), S));
	const int32 Hand = Ref.FindBoneIndex(HandName);
	if (Hand == INDEX_NONE)
	{
		return nullptr;
	}
	const FTransform HandRef = RefPose(Ref, Hand);
	auto InHand = [&](const TCHAR* Bone, const FVector& Fallback) {
		const int32 I = Ref.FindBoneIndex(FName(*FString::Printf(TEXT("%s_%s"), Bone, S)));
		return I == INDEX_NONE ? Fallback : HandRef.InverseTransformPosition(RefPose(Ref, I).GetLocation());
	};
	const FVector Middle = InHand(TEXT("middle_01"), FVector(9.0, 0.0, 0.0));
	const FVector Fingers = Middle.GetSafeNormal();
	const FVector Across = (InHand(TEXT("index_01"), FVector(9.0, 2.5, 0.0)) - InHand(TEXT("pinky_01"), FVector(8.0, -3.5, 0.0))).GetSafeNormal();
	FVector Palm = FVector::CrossProduct(Fingers, Across).GetSafeNormal();
	Palm = Spec.Side == 0 ? -Palm : Palm;
	auto Axis = [&](EPropAxis A) {
		switch (A)
		{
		case EPropAxis::Across: return Across;
		case EPropAxis::Fingers: return Fingers;
		case EPropAxis::Palm: return Palm;
		case EPropAxis::NegAcross: return -Across;
		case EPropAxis::NegFingers: return -Fingers;
		case EPropAxis::NegPalm: return -Palm;
		}
		return Across;
	};
	const FQuat Turn = FRotationMatrix::MakeFromXZ(Axis(Spec.X), Axis(Spec.Z)).ToQuat();
	// The grip in the palm's middle, a little out of it (the fingers close round it there).
	const FVector Grip = Middle * 0.55 + Palm * 2.5;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Body, MakeUniqueObjectName(Body, UStaticMeshComponent::StaticClass(), TEXT("HandProp")), RF_Transient);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCanEverAffectNavigation(false);
	C->SetGenerateOverlapEvents(false);
	C->SetupAttachment(Skin, HandName);
	C->SetRelativeTransform(FTransform(Turn, Grip));
	C->RegisterComponent();
	return C;
}

/** Spec 5's voices, until core's talk says more (the fallback's lines by person and moment). */
FString VoiceLine(FName Id, const FString& Moment, int32 Pick)
{
	struct FVoice
	{
		const TCHAR* Id;
		const TCHAR* Moment;
		const TCHAR* Lines[3];
	};
	static const FVoice Voices[] = {
		{TEXT("lonnie"), TEXT("greet"), {TEXT("Sit down, sit down. Same as last time, or you finally gonna let me fix that?"), TEXT("Can't put hair back on, kid. Give it three weeks."), TEXT("Dee was in Thursday. Says you fold too much. Says it like it's a compliment.")}},
		{TEXT("lonnie"), TEXT("closing"), {TEXT("Broom's out. That means you."), nullptr, nullptr}},
		{TEXT("bea"), TEXT("greet"), {TEXT("Yellow tags are half off Saturday."), TEXT("Booth's on the left. Curtain sticks. Pull, don't yank."), TEXT("That jacket came in a box from a man who moved to Florida. You wear it better.")}},
		{TEXT("bea"), TEXT("closing"), {TEXT("We're closing up, sweetheart. Bring it to the register."), nullptr, nullptr}},
		{TEXT("rosa"), TEXT("greet"), {TEXT("Towel's free. The shake isn't."), TEXT("You look like a man who sleeps in a chair."), TEXT("Four in, hold four, four out. Works in the ring.")}},
		{TEXT("ruby"), TEXT("greet"), {TEXT("Sit anywhere that isn't Sal's."), TEXT("Thirty-one years and the grill's the only thing in here that never needed a day off."), nullptr}},
		{TEXT("marlene"), TEXT("greet"), {TEXT("Coffee, hon? You look like you lost a flip."), TEXT("Pie's cherry. It's always cherry. Don't ask about the apple."), nullptr}},
		{TEXT("augie"), TEXT("greet"), {TEXT("Set it on the glass. Let me look."), TEXT("Guitars I don't sell to card players. You'd hock it back to me inside a month."), nullptr}},
		{TEXT("augie"), TEXT("closing"), {TEXT("Buzzer's off in ten. Ticket or cash, make it quick."), nullptr, nullptr}},
		{TEXT("benny"), TEXT("greet"), {TEXT("Hey. The usual?"), TEXT("Coffee's fresh. Ish."), nullptr}},
		{TEXT("ray"), TEXT("greet"), {TEXT("Benny says you're all right. Don't make a liar of him."), TEXT("Ray. I own the place. The coffee's my fault."), nullptr}},
		{TEXT("sal"), TEXT("greet"), {TEXT("Dee's got a seat open."), TEXT("Kid. You playing tonight?"), nullptr}},
		{TEXT("mei"), TEXT("greet"), {TEXT("Night Shift board: I'm fourth. You're... not on it. Yet."), nullptr, nullptr}},
		{TEXT("big-lou"), TEXT("greet"), {TEXT("Hey hey! You want a wing? You don't get a wing."), nullptr, nullptr}},
		{TEXT("twitch"), TEXT("greet"), {TEXT("Can't talk, can't talk, I'm late, she hates late."), nullptr, nullptr}},
		{TEXT("mrs-park"), TEXT("greet"), {TEXT("Good morning. The noon game. You should come and lose to me."), nullptr, nullptr}},
	};
	for (const FVoice& V : Voices)
	{
		if (Id == FName(V.Id) && Moment == V.Moment)
		{
			int32 N = 0;
			while (N < 3 && V.Lines[N])
			{
				++N;
			}
			return N > 0 ? FString(V.Lines[Pick % N]) : FString();
		}
	}
	return FString();
}

FAutoConsoleCommandWithWorldAndArgs CmdNpcList(TEXT("ss.Npc.List"), TEXT("Logs the street's people: who is where, their tier and motion, the budget (spec 7.7)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		const AStreetNpcs* Npcs = AStreetNpcs::Find(World);
		UE_LOG(LogStreetNpcs, Display, TEXT("%s"), Npcs ? *Npcs->TestList() : TEXT("ss.Npc.List: no people director on this map"));
	}));
FAutoConsoleCommandWithWorldAndArgs CmdNpcSpawn(TEXT("ss.Npc.Spawn"), TEXT("ss.Npc.Spawn <id>: builds someone now and puts them at their station (or before the hero)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		AStreetNpcs* Npcs = AStreetNpcs::Find(World);
		UE_LOG(LogStreetNpcs, Display, TEXT("%s"), Npcs ? *Npcs->TestSpawn(Args.Num() > 0 ? Args[0] : FString()) : TEXT("ss.Npc.Spawn: no people director on this map"));
	}));
FAutoConsoleCommandWithWorldAndArgs CmdNpcTalk(TEXT("ss.Npc.Talk"), TEXT("ss.Npc.Talk <id> [choice]: their opening line and choices, or their answer to a choice."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		AStreetNpcs* Npcs = AStreetNpcs::Find(World);
		UE_LOG(LogStreetNpcs, Display, TEXT("%s"),
			Npcs ? *Npcs->TestTalk(Args.Num() > 0 ? Args[0] : FString(), Args.Num() > 1 ? Args[1] : FString()) : TEXT("ss.Npc.Talk: no people director on this map"));
	}));
FAutoConsoleCommandWithWorldAndArgs CmdNpcBudget(TEXT("ss.Npc.BudgetSet"), TEXT("ss.Npc.BudgetSet <n>: the live NPC cap (-1: spec 6.7's 10). The same as setting ss.Npc.Budget."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		AStreetNpcs* Npcs = AStreetNpcs::Find(World);
		UE_LOG(LogStreetNpcs, Display, TEXT("%s"), Npcs ? *Npcs->TestBudget(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1) : TEXT("no people director on this map"));
	}));
FAutoConsoleCommandWithWorldAndArgs CmdNpcBeat(TEXT("ss.Npc.Beat"), TEXT("ss.Npc.Beat <id|hero> <beat>: a work beat on someone for 20 s (spec 6.6's: ring, phone, wipe, cut, pour, pad, chair, eat...)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		AStreetNpcs* Npcs = AStreetNpcs::Find(World);
		UE_LOG(LogStreetNpcs, Display, TEXT("%s"),
			Npcs ? *Npcs->TestBeat(Args.Num() > 0 ? Args[0] : FString(), Args.Num() > 1 ? Args[1] : FString()) : TEXT("no people director on this map"));
	}));
FAutoConsoleCommandWithWorldAndArgs CmdAnimDescribe(TEXT("ss.Anim.Describe"), TEXT("Logs the mocap library and what the hero's and each person's body is playing."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (!World)
		{
			return;
		}
		const UStreetMocap* Lib = UStreetMocap::Get(World);
		UE_LOG(LogStreetNpcs, Display, TEXT("%s"), Lib ? *Lib->Describe() : TEXT("no mocap library (no game instance)"));
		for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
		{
			if (const UStreetBodyAnim* Anim = It->StreetBody(); Anim && !It->IsHidden())
			{
				UE_LOG(LogStreetNpcs, Display, TEXT("  %s (%s): %s"), *It->GetName(), *It->GetCastFace().ToString(), *Anim->DescribeMotion());
			}
		}
	}));
} // namespace StreetNpcsDetail

using namespace StreetNpcsDetail;

AStreetNpcs::AStreetNpcs()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AStreetNpcs* AStreetNpcs::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AStreetNpcs> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

AStreetNpcs* AStreetNpcs::FindOrSpawn(UWorld* World)
{
	if (AStreetNpcs* Found = Find(World))
	{
		return Found;
	}
	return World ? World->SpawnActor<AStreetNpcs>() : nullptr;
}

void AStreetNpcs::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : {TEXT("SM_NS_Prop_Comb"), TEXT("SM_NS_Prop_Scissors"), TEXT("SM_NS_Prop_Clippers"), TEXT("SM_NS_Prop_NeckDuster"), TEXT("SM_NS_Prop_Carafe"),
			 TEXT("SM_NS_Prop_Plate"), TEXT("SM_NS_Prop_Notepad"), TEXT("SM_NS_Prop_Pen"), TEXT("SM_NS_Prop_Rag"), TEXT("SM_NS_Prop_Phone"), TEXT("SM_NS_Prop_Fork"),
			 TEXT("SM_NS_Prop_Mug"), TEXT("SM_NS_Prop_Newspaper")})
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
}

// ------------------------------------------------------------------ the session's side

void AStreetNpcs::Begin(AStreetStage* InStage, AShortStackCharacter* InHero, AStreetCrowd* InCrowd)
{
	if (!InStage || !GetWorld())
	{
		UE_LOG(LogStreetNpcs, Warning, TEXT("No stage: the block's people stay home"));
		return;
	}
	if (Stage && Stage != InStage)
	{
		// Another street: everyone built for the last one goes (adopted bodies stay: they were never ours), and their talk uses
		// with them.
		UStreetWorld* Registry = UStreetWorld::Get(this);
		for (FStreetPerson& P : People)
		{
			ClearProps(P);
			if (Registry && P.TalkUse != 0)
			{
				Registry->RemoveUse(P.TalkUse);
			}
			if (P.Body && !P.bAdopted)
			{
				P.Body->Destroy();
			}
		}
		People.Reset();
		Talks.Reset();
		AheadIds.Reset();
		AheadSlot = -1;
	}
	Stage = InStage;
	Hero = InHero;
	Crowd = InCrowd;
	bBegun = true;
	PropsMissing.Reset(); // art imported since the last street is looked for again
	Dice.Initialize(static_cast<int32>(GetUniqueID()) * 7919 + 17);
	if (Crowd)
	{
		Crowd->SetDirector(this);
	}
	// The faces: whose is whose on this map (the player's is nobody else's; the staff's are kept for them).
	if (UCastFaceRegistry* Faces = UCastFaceRegistry::Get(this))
	{
		const ss::Session* S = CareerSession();
		Faces->Configure(InHero ? InHero->GetCastFace() : FName(), S ? CastLooks::CareerKey(S->Person) : FString());
	}
	if (People.Num() == 0)
	{
		BuildRoster();
	}
	Adopt();
	ReadSlot = -1; // read the schedule at the next hour
	UE_LOG(LogStreetNpcs, Log, TEXT("The block's people: %d known%s%s"), People.Num(), bAutoSpawned ? TEXT(" (the crowd started the director)") : TEXT(""),
		CareerSession() ? TEXT(", on the career's schedule") : TEXT(", no career yet (the fallback schedule until SetSession or CastLooks::BindSession)"));
}

void AStreetNpcs::AttachCrowd(AStreetCrowd* InCrowd)
{
	if (InCrowd && Crowd != InCrowd)
	{
		Crowd = InCrowd;
		Crowd->SetDirector(this);
	}
}

void AStreetNpcs::SetSession(ss::Session* InSession)
{
	Session = InSession;
	if (Session && bBegun)
	{
		if (UCastFaceRegistry* Faces = UCastFaceRegistry::Get(this))
		{
			const AShortStackCharacter* H = Hero.IsValid() ? Hero.Get() : GetHero();
			Faces->Configure(H ? H->GetCastFace() : FName(), CastLooks::CareerKey(Session->Person));
			// The spare heroes the staff borrow are picked per career: anyone not built yet takes the career's.
			for (FStreetPerson& P : People)
			{
				const FName Face = Faces->FaceFor(P.Id);
				if (!P.Body && !Face.IsNone())
				{
					P.Face = Face;
				}
			}
		}
		ReadSlot = -1;
	}
}

ss::Session* AStreetNpcs::CareerSession() const
{
	return Session ? Session : CastLooks::BoundSession(this);
}

void AStreetNpcs::SetHour(double InWorldMinutes, float InDaylight, float InRain, bool bInPaused)
{
	WorldMinutes = InWorldMinutes;
	Daylight = FMath::Clamp(InDaylight, 0.0f, 1.0f);
	Rain = FMath::Max(0.0f, InRain);
	bPaused = bInPaused;
	bHourKnown = true;
}

// ------------------------------------------------------------------ the roster and the schedule

void AStreetNpcs::BuildRoster()
{
	People.Reset();
	for (const FRosterRow& R : Roster)
	{
		FStreetPerson& P = People.AddDefaulted_GetRef();
		P.Id = FName(R.Id);
		P.Name = R.Name;
		P.Label = R.Label;
		P.Venue = R.Venue && *R.Venue ? FName(R.Venue) : NAME_None;
		P.bWoman = R.bWoman;
		P.bStaff = R.bStaff;
		P.bFixed = P.Id == FName(TEXT("inez"));
		P.WorldName = R.WorldName ? FString(R.WorldName) : FString();
		if (R.Cast)
		{
			P.Face = CastLooks::IsBuilt(R.Cast) ? FName(R.Cast) : NAME_None;
		}
	}
	// Core's people where they're known there (people-core's town): their names, venues and faces win. Dee stays at her
	// table (her own room is another stage's), and the cook behind Ruby's pass is a baked figure (spec 6.1), not a MetaHuman.
	for (const ss::town::Person& C : ss::town::People())
	{
		const FName Id(*Utf8(C.Id));
		FStreetPerson* P = Person(Id);
		if (!P)
		{
			P = &People.AddDefaulted_GetRef();
			P->Id = Id;
			P->bStaff = C.Role != ss::town::Job::Regular && C.Role != ss::town::Job::Local && C.Role != ss::town::Job::Fan && C.Role != ss::town::Job::Pedestrian;
			P->Label = P->bStaff ? FString() : FString(TEXT("Regular"));
			P->WorldName = C.InWorld ? Utf8(C.Name) : FString();
		}
		P->Name = Utf8(C.Name);
		P->Venue = C.Venue.empty() ? P->Venue : FName(*Utf8(C.Venue));
		P->bWoman = C.Woman;
		P->bNoBody = C.Role == ss::town::Job::Cook || C.Role == ss::town::Job::Host || (C.Cast.empty() && !P->bStaff && P->Face.IsNone());
		if (!C.Cast.empty() && CastLooks::IsBuilt(Utf8(C.Cast)))
		{
			P->Face = FName(*Utf8(C.Cast));
		}
	}
	// Their heights (the body is fitted to it as it's built): the staff's as CastLooks dresses them, the world's people by a
	// seed of their id (155-175 cm for women, 165-195 for men).
	for (FStreetPerson& P : People)
	{
		P.Height = P.bStaff ? CastLooks::HeightFor(P.Id) : CastLooks::PedestrianHeight(FCrc::StrCrc32(*P.Id.ToString()), P.bWoman);
	}
	// The block's kept and borrowed faces (spec 6.1) for the staff, from the face registry.
	if (const UCastFaceRegistry* Faces = UCastFaceRegistry::Get(this))
	{
		for (FStreetPerson& P : People)
		{
			const FName Face = Faces->FaceFor(P.Id);
			if (!Face.IsNone())
			{
				P.Face = Face;
			}
		}
	}
}

FStreetPerson* AStreetNpcs::Person(FName Id)
{
	return People.FindByPredicate([Id](const FStreetPerson& P) { return P.Id == Id; });
}

const FStreetPerson* AStreetNpcs::FindPerson(FName PersonId) const
{
	return People.FindByPredicate([PersonId](const FStreetPerson& P) { return P.Id == PersonId; });
}

double AStreetNpcs::ScheduleMinutes() const
{
	const float Hour = CVarHour.GetValueOnGameThread();
	if (Hour >= 0.0f)
	{
		return FMath::FloorToDouble(WorldMinutes / ss::net::MinutesPerDay) * ss::net::MinutesPerDay + FMath::Clamp(static_cast<double>(Hour), 0.0, 23.99) * 60.0;
	}
	return WorldMinutes;
}

std::vector<ss::town::Placement> AStreetNpcs::FallbackWho(double World) const
{
	// Spec 5.0's hours, 6.1's shifts and 6.3's regulars as core's town has them (Town.h: the same ids and stations), for a
	// street with no career to read (the map opened on its own in the editor). Each window by the day it opens (bit 0
	// Monday), as life::Activity has them.
	std::vector<ss::town::Placement> Out;
	const int32 Day = ss::net::DayOf(World);
	const double Midnight = static_cast<double>(Day) * ss::net::MinutesPerDay;
	using ss::town::Doing;
	auto Add = [&Out](const TCHAR* Id, const TCHAR* Venue, const TCHAR* Station, Doing What, double Since, double Until) {
		ss::town::Placement P;
		P.Id = Std(Id);
		P.Venue = Std(Venue);
		P.Station = Std(Station);
		P.What = What;
		P.Since = Since;
		P.Until = Until;
		Out.push_back(P);
	};
	auto Window = [&](const TCHAR* Id, const TCHAR* Venue, const TCHAR* Station, int32 Opens, int32 Closes, int32 Days, Doing What) {
		// Today's window, or yesterday's still running past midnight.
		for (int32 Back = 0; Back <= 1; ++Back)
		{
			const int32 D = Day - Back;
			if (((Days >> ss::net::Weekday(D)) & 1) == 0)
			{
				continue;
			}
			const double From = Midnight - Back * ss::net::MinutesPerDay + Opens;
			const double To = From + ((Closes > Opens ? Closes : Closes + 1440) - Opens);
			if (World >= From && World < To)
			{
				Add(Id, Venue, Station, What, From, To);
				return;
			}
		}
	};
	const int32 Every = 0x7f;
	const int32 TueToSun = Every & ~1;
	Window(TEXT("benny"), TEXT("store"), TEXT("store-counter"), 22 * 60, 10 * 60, Every, Doing::Work);
	Window(TEXT("ray"), TEXT("store"), TEXT("store-counter"), 10 * 60, 22 * 60, Every, Doing::Work);
	Window(TEXT("lonnie"), TEXT("barber"), TEXT("barber-chair1"), 10 * 60, 22 * 60, TueToSun & ~((1 << 4) | (1 << 5)), Doing::Work);
	Window(TEXT("lonnie"), TEXT("barber"), TEXT("barber-chair1"), 10 * 60, 24 * 60, (1 << 4) | (1 << 5), Doing::Work);
	Window(TEXT("bea"), TEXT("thrift"), TEXT("thrift-register"), 10 * 60, 21 * 60, Every & ~(1 << 6), Doing::Work);
	Window(TEXT("bea"), TEXT("thrift"), TEXT("thrift-register"), 12 * 60, 18 * 60, 1 << 6, Doing::Work);
	Window(TEXT("rosa"), TEXT("gym"), TEXT("gym-desk"), 6 * 60, 12 * 60, Every, Doing::Work);
	Window(TEXT("rosa"), TEXT("gym"), TEXT("gym-desk"), 16 * 60, 22 * 60, Every, Doing::Work);
	Window(TEXT("ruby"), TEXT("diner"), TEXT("diner-counter"), 6 * 60, 18 * 60, Every, Doing::Work);
	Window(TEXT("marlene"), TEXT("diner"), TEXT("diner-counter"), 18 * 60, 6 * 60, Every, Doing::Work);
	Window(TEXT("augie"), TEXT("pawn"), TEXT("pawn-counter"), 9 * 60, 20 * 60, 0x0f, Doing::Work);
	Window(TEXT("augie"), TEXT("pawn"), TEXT("pawn-counter"), 9 * 60, 21 * 60, (1 << 4) | (1 << 5), Doing::Work);
	Window(TEXT("augie"), TEXT("pawn"), TEXT("pawn-counter"), 11 * 60, 17 * 60, 1 << 6, Doing::Work);
	// Ruby's regulars (spec 6.3), at most two at once: Sal with the paper on weekday mornings, Mrs. Park on Sunday before the
	// noon game, Mei on her online nights (not Dee's), Big Lou on Friday nights.
	Window(TEXT("sal"), TEXT("diner"), TEXT("diner-stool1"), 7 * 60, 9 * 60 + 30, 0x1f, Doing::Sit);
	Window(TEXT("mrs-park"), TEXT("diner"), TEXT("diner-booth1a"), 10 * 60 + 30, 11 * 60 + 30, 1 << 6, Doing::Sit);
	Window(TEXT("mei"), TEXT("diner"), TEXT("diner-booth2a"), 2 * 60, 5 * 60, (1 << 0) | (1 << 1) | (1 << 3), Doing::Sit);
	Window(TEXT("big-lou"), TEXT("diner"), TEXT("diner-booth4a"), 23 * 60, 60, 1 << 4, Doing::Sit);
	// Dee's regulars on her nights (Life's "dee-game": Tue, Thu, Sat): up from the stop to the Wash & Fold's door (Sal's
	// cigarette by it until a couple of minutes to nine), at her table, out near dawn one by one.
	static const ss::life::Activity* const Game = ss::life::Find("dee-game");
	const int32 GameDays = Game ? Game->Days : ((1 << 1) | (1 << 3) | (1 << 5));
	for (const FArrival& A : Arrivals)
	{
		for (int32 Back = 0; Back <= 1; ++Back)
		{
			const int32 D = Day - Back;
			if (((GameDays >> ss::net::Weekday(D)) & 1) == 0)
			{
				continue;
			}
			const double Night = static_cast<double>(D) * ss::net::MinutesPerDay;
			// Each night a little different (seeded by the night), the order kept.
			const double Jitter = static_cast<double>(static_cast<int32>((static_cast<uint32>(D) * 2654435761u + static_cast<uint32>(A.Arrive)) >> 24) % 7) - 3.0;
			const double At = Night + A.Arrive + Jitter;
			const double Sits = A.bSmoke ? Night + 20 * 60 + 58 : At;
			const double Leaves = Night + ss::net::MinutesPerDay + A.Leave + Jitter * 2.0;
			const TCHAR* const Id = A.Id;
			// Not two places at once: Dee's night wins over a booth at Ruby's.
			const bool bTonight = World >= At - 4.0 && World < Leaves + 5.0;
			if (!bTonight)
			{
				continue;
			}
			for (auto It = Out.begin(); It != Out.end();)
			{
				It = It->Id == Std(Id) ? Out.erase(It) : It + 1;
			}
			if (World < At)
			{
				Add(Id, TEXT("washfold"), TEXT("washfold-door"), Doing::Arrive, At - 4.0, At);
			}
			else if (World < Sits)
			{
				Add(Id, TEXT("washfold"), TEXT("washfold-door"), Doing::Smoke, At, Sits);
			}
			else if (World < Leaves)
			{
				Add(Id, TEXT("washfold"), TEXT("dee-table"), Doing::Sit, Sits, Leaves);
			}
			else
			{
				Add(Id, TEXT("washfold"), TEXT("washfold-door"), Doing::Leave, Leaves, Leaves + 5.0);
			}
			break;
		}
	}
	return Out;
}

std::vector<ss::town::Placement> AStreetNpcs::WhoAt(double World) const
{
	const ss::Session* S = CareerSession();
	return S && !ss::town::People().empty() ? ss::town::Who(World, *S) : FallbackWho(World);
}

void AStreetNpcs::ReadSchedule()
{
	const double Now = ScheduleMinutes();
	// Every world minute: core's placements are minutes long (a walk up from the stop, a cigarette, a hand-off).
	ReadSlot = static_cast<int64>(FMath::FloorToDouble(Now));
	ReadAt = Now;
	ReadOverride = CVarHour.GetValueOnGameThread();
	const bool bCore = CareerSession() && !ss::town::People().empty();
	if (!bCore && !bLoggedFallback)
	{
		bLoggedFallback = true;
		UE_LOG(LogStreetNpcs, Log, TEXT("The people's schedule: %s"), CareerSession() ? TEXT("core's town has nobody (its stubs): spec 5.0 and 6.1-6.3's fallback")
																			  : TEXT("no career on this map (SetSession, CastLooks::BindSession): spec 5.0 and 6.1-6.3's fallback"));
	}
	const std::vector<ss::town::Placement> Who = WhoAt(Now);
	for (FStreetPerson& P : People)
	{
		P.bWanted = false;
	}
	for (const ss::town::Placement& Pl : Who)
	{
		FStreetPerson* P = Person(FName(*Utf8(Pl.Id)));
		if (!P)
		{
			continue; // the Embercrest's locals at the shelter ("npc:<Name>", spec 6.3: P1), or someone the roster doesn't know
		}
		const FName Station = Pl.Station.empty() ? NAME_None : FName(*Utf8(Pl.Station));
		const uint8 Doing = static_cast<uint8>(Pl.What);
		if (P->WantStation != Station || P->WantDoing != Doing || !P->bWanted)
		{
			P->StuckFor = 0.0f;
			P->bWhyLogged = false;
		}
		P->bWanted = true;
		P->WantStation = Station;
		P->WantDoing = Doing;
		P->WantSince = Pl.Since;
		P->WantUntil = Pl.Until;
		P->WantVenue = Pl.Venue.empty() ? NAME_None : FName(*Utf8(Pl.Venue));
		if (P->bStaff && !P->WantVenue.IsNone() && P->WantVenue != FName(TEXT("market-stop")))
		{
			P->Venue = P->WantVenue;
		}
	}
	for (FStreetPerson& P : People)
	{
		if (!P.bWanted)
		{
			P.WantStation = NAME_None;
			P.WantVenue = NAME_None;
		}
	}
}

AStreetNpcs::EWant AStreetNpcs::WantOf(const FStreetPerson& P, FTransform* OutStation, float* OutSeat) const
{
	using ss::town::Doing;
	if (!P.bWanted || P.WantStation.IsNone())
	{
		return EWant::Off;
	}
	const Doing What = static_cast<Doing>(P.WantDoing);
	if (P.WantStation == DeeTable)
	{
		return EWant::Inside;
	}
	if (P.WantStation == WashDoorStation)
	{
		return What == Doing::Leave ? EWant::WalkOut : EWant::WalkIn;
	}
	if (What == Doing::Leave)
	{
		return EWant::Off; // out of Ruby's door, home from the shop: gone once nobody sees
	}
	const UStreetWorld* Registry = UStreetWorld::Get(this);
	FTransform T;
	FName Pose;
	float Seat = 0.0f;
	if (!Registry || !Registry->Station(P.WantStation, T, Pose, Seat))
	{
		return EWant::Pending;
	}
	if (OutStation)
	{
		*OutStation = T;
	}
	if (OutSeat)
	{
		*OutSeat = Seat;
	}
	return EWant::Station;
}

float AStreetNpcs::SmokeSecondsFor(const FStreetPerson& P) const
{
	// The world's clock runs at the wall's (a world minute is a real one, spec 11.1): the cigarette lasts until the placement
	// after the walk up (town's Smoke at the Wash & Fold's door) ends, or what's left of it.
	using ss::town::Doing;
	const ss::Session* S = CareerSession();
	const bool bCore = S && !ss::town::People().empty();
	const double Now = ScheduleMinutes();
	double Ends = -1.0;
	if (static_cast<Doing>(P.WantDoing) == Doing::Smoke)
	{
		Ends = P.WantUntil;
	}
	else if (static_cast<Doing>(P.WantDoing) == Doing::Arrive)
	{
		const std::string Id = Std(P.Id.ToString());
		const double After = P.WantUntil + 0.05;
		if (bCore)
		{
			ss::town::Placement Next;
			if (ss::town::Where(Id, After, *S, Next) && Next.What == Doing::Smoke)
			{
				Ends = Next.Until;
			}
		}
		else
		{
			for (const ss::town::Placement& Next : FallbackWho(After))
			{
				if (Next.Id == Id && Next.What == Doing::Smoke)
				{
					Ends = Next.Until;
				}
			}
		}
	}
	return Ends > Now ? static_cast<float>(FMath::Clamp((Ends - Now) * 60.0, 20.0, 1500.0)) : 0.0f;
}

// ------------------------------------------------------------------ bodies

void AStreetNpcs::Adopt()
{
	// The game mode's own people from before there was a director (Benny at the counter, Inez at hers): theirs to keep, ours to
	// tier and (Benny) to stand down when Ray is on.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FStreetPerson& P : People)
	{
		if (P.Body || P.Face.IsNone() || (P.Id != FName(TEXT("benny")) && P.Id != FName(TEXT("inez"))))
		{
			continue;
		}
		for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
		{
			AShortStackCharacter* C = *It;
			if (C && C != Hero.Get() && !C->IsPlayerControlled() && C->GetOwner() == nullptr && C->GetCastFace() == P.Face)
			{
				P.Body = C;
				P.bAdopted = true;
				P.State = EStreetPersonState::Placed;
				P.Station = P.Id == FName(TEXT("benny")) ? FName(TEXT("store-counter")) : FName(TEXT("washfold-counter"));
				// A station is where the feet are, upright (the beats' hands and eyes are measured up from it), not the capsule's middle.
				const float Half = C->GetCapsuleComponent() ? C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;
				P.StationAt = FTransform(FRotator(0.0f, static_cast<float>(C->GetActorRotation().Yaw), 0.0f), C->GetActorLocation() - FVector(0.0, 0.0, Half));
				P.SeatHeight = 0.0f;
				if (UCastFaceRegistry* Faces = UCastFaceRegistry::Get(this))
				{
					Faces->Claim(P.Face, P.Id, C);
				}
				UE_LOG(LogStreetNpcs, Log, TEXT("%s: the game mode's own (%s), adopted"), *P.Name, *P.Face.ToString());
				break;
			}
		}
	}
}

bool AStreetNpcs::BuildBody(FStreetPerson& P)
{
	UWorld* World = GetWorld();
	if (P.Body || !World || P.Face.IsNone())
	{
		return P.Body != nullptr;
	}
	int32 Pooled = 0;
	for (const FStreetPerson& O : People)
	{
		Pooled += O.Body && !O.bAdopted ? 1 : 0;
	}
	if (Pooled >= MaxPooled)
	{
		return false;
	}
	const double Started = FPlatformTime::Seconds();
	const FTransform Hidden(FRotator::ZeroRotator, Offstage());
	AShortStackCharacter* B = World->SpawnActorDeferred<AShortStackCharacter>(AShortStackCharacter::StaticClass(), Hidden, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!B)
	{
		return false;
	}
	B->AutoPossessAI = EAutoPossessAI::Disabled;
	// Dressed as themselves (CastLooks, spec 6.1): the staff in their wardrobes (Lonnie's smock, Marlene's pink uniform,
	// Augie's white shirt and readers), the world's people in what they wear at Dee's table, so the player knows Sal outside;
	// fitted to their height (the actor stays at scale 1: the gait and the clips read the body's own size).
	const FCastWear Wear = P.WorldName.IsEmpty() ? CastLooks::WearFor(P.Id) : CastLooks::WearOf(CastLooks::PersonaFor(P.WorldName));
	B->ApplyCast(*P.Face.ToString(), Wear, P.Height);
	B->FinishSpawning(Hidden);
	if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		Move->SetComponentTickEnabled(false);
	}
	UCapsuleComponent* Capsule = B->GetCapsuleComponent();
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsule->SetGenerateOverlapEvents(false);
	TInlineComponentArray<UActorComponent*> Parts(B);
	for (UActorComponent* C : Parts)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(C))
		{
			Prim->SetCastHiddenShadow(false);
		}
		if (USkeletalMeshComponent* Skin = Cast<USkeletalMeshComponent>(C))
		{
			Skin->bDisableClothSimulation = true;
			Skin->SetGenerateOverlapEvents(false);
		}
		else if (UGroomComponent* Hair = Cast<UGroomComponent>(C))
		{
			Hair->SetEnableSimulation(false);
		}
	}
	P.Body = B;
	B->SetDetailTier(3);
	if (UCastFaceRegistry* Faces = UCastFaceRegistry::Get(this))
	{
		if (!Faces->Claim(P.Face, P.Id, B))
		{
			UE_LOG(LogStreetNpcs, Warning, TEXT("%s's face %s is kept for %s"), *P.Name, *P.Face.ToString(), *Faces->ReservedFor(P.Face).ToString());
		}
	}
	UE_LOG(LogStreetNpcs, Log, TEXT("%s built as %s (%.0f cm) in %.0f ms%s"), *P.Name, *P.Face.ToString(), P.Height, (FPlatformTime::Seconds() - Started) * 1000.0,
		bDark ? TEXT(" under black") : (bFadingIn ? TEXT(" as the street fades in") : TEXT(" IN VIEW (ss.Npc.Spawn)")));
	return true;
}

void AStreetNpcs::ClearProps(FStreetPerson& P)
{
	for (UStaticMeshComponent* C : P.Props)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	P.Props.Reset();
	P.PropsFor = NAME_None;
}

void AStreetNpcs::DressProps(FStreetPerson& P, FName Beat)
{
	// The cut's props change with its phase: keyed by beat and phase (names made once: this runs every frame for everyone placed).
	static const FName CutBeat(TEXT("cut"));
	static const FName CutKeys[3] = {FName(TEXT("cut0")), FName(TEXT("cut1")), FName(TEXT("cut2"))};
	const float T = P.Body ? P.Body->GetWorkTime() : 0.0f;
	FName Want = Beat;
	if (Beat == CutBeat)
	{
		const float C = FMath::Fmod(T, 16.0f);
		Want = CutKeys[C < 6.0f ? 0 : (C < 10.0f ? 1 : (C < 14.0f ? 0 : 2))];
	}
	if (Want == P.PropsFor)
	{
		return;
	}
	ClearProps(P);
	P.PropsFor = Want;
	if (!P.Body || Beat.IsNone())
	{
		return;
	}
	for (const FPropSpec& Spec : PropsFor(Beat, T))
	{
		if (UStaticMeshComponent* C = PutInHand(P.Body, Spec))
		{
			P.Props.Add(C);
		}
	}
}

void AStreetNpcs::HeroProps()
{
	AShortStackCharacter* H = Hero.Get();
	const FName Beat = H ? H->GetWorkBeat() : NAME_None;
	if (Beat == HeroPropsFor)
	{
		return;
	}
	for (UStaticMeshComponent* C : HeroPropParts)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	HeroPropParts.Reset();
	HeroPropsFor = Beat;
	if (!H || Beat.IsNone())
	{
		return;
	}
	for (const FPropSpec& Spec : PropsFor(Beat, 0.0f))
	{
		if (UStaticMeshComponent* C = PutInHand(H, Spec))
		{
			HeroPropParts.Add(C);
		}
	}
}

void AStreetNpcs::HeroServiceBeat()
{
	// The hero sat in a barber's chair outside a service (the Style Book open, waiting on the cut): still, the head held for
	// him, the eyes to the mirror. The services' own beats are the game's (ServiceBeatFor: the chair, the gym's machines, a
	// meal or a coffee at Ruby's) and are never touched here; the one set here is taken off when the hero gets up.
	AShortStackCharacter* H = Hero.Get();
	if (!H)
	{
		return;
	}
	static const FName Barber(TEXT("barber")), Chair(TEXT("chair"));
	FName Want = NAME_None;
	if (H->IsSeated() && H->GetSeatBlend() > 0.95f && HeroZone == Barber && H->GetSeatHeight() >= 50.0f)
	{
		Want = Chair; // a barber's chair (the waiting chairs are lower)
	}
	const FName Now = H->GetWorkBeat();
	if (!Want.IsNone() && (Now.IsNone() || Now == HeroBeatSet))
	{
		if (Now != Want)
		{
			H->SetWork(Want, FTransform(FRotator(0.0f, static_cast<float>(H->GetActorRotation().Yaw), 0.0f), H->GetActorLocation()));
		}
		HeroBeatSet = Want;
	}
	else if (Want.IsNone() && !HeroBeatSet.IsNone())
	{
		if (Now == HeroBeatSet)
		{
			H->SetWork(NAME_None, FTransform::Identity);
		}
		HeroBeatSet = NAME_None;
	}
}

// ------------------------------------------------------------------ placing

FVector AStreetNpcs::Offstage() const
{
	return Stage ? Stage->GetActorTransform().TransformPosition(FVector(260.0, -6050.0, -1500.0)) : FVector(0.0, 0.0, -100000.0);
}

void AStreetNpcs::Show(FStreetPerson& P, bool bShow)
{
	AShortStackCharacter* B = P.Body;
	if (!B)
	{
		return;
	}
	if (bShow)
	{
		// Out of the pool: visible (the budget sets the real tier this same frame: it runs after the people). Shown outright as
		// well: the crowd may have hidden them itself when it handed them back, which the tier's Gone doesn't know to undo.
		ArbitrateIn = 0.0f;
		B->SetDetailTier(2);
		B->SetActorHiddenInGame(false);
		B->SetActorEnableCollision(true);
		B->SetActorTickEnabled(true);
		B->CustomTimeDilation = 1.0f;
		TInlineComponentArray<UActorComponent*> Parts(B);
		for (UActorComponent* C : Parts)
		{
			if (C->IsA<USkinnedMeshComponent>() || C->IsA<UGroomComponent>())
			{
				C->SetComponentTickEnabled(true);
			}
		}
		return;
	}
	B->SetSeated(false, 46.0f);
	B->SetWork(NAME_None, FTransform::Identity);
	B->SetLookTarget(FVector::ZeroVector, 0.0f);
	B->SetTalking(0.0f, 0.6f);
	ClearProps(P);
	if (!P.bAdopted)
	{
		B->SetActorLocation(Offstage(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
	{
		Move->Velocity = FVector::ZeroVector;
	}
	B->SetDetailTier(3);
	P.Beat = NAME_None;
	P.bEngaged = false;
	P.ServeFor = 0.0f;
	P.Slide = 0.0f;
	P.SlideWant = 0.0f;
}

void AStreetNpcs::Pool(FStreetPerson& P)
{
	if (P.State == EStreetPersonState::Walking && Crowd)
	{
		return; // the crowd has them until they're off the set or through a door
	}
	Show(P, false);
	P.State = EStreetPersonState::Pooled;
	P.Station = NAME_None;
	P.bLateSaid = false;
	// The seconds they were watched at their station aren't a wait for whatever's next (a walk up to Dee's, another station).
	P.StuckFor = 0.0f;
	P.LookIn = 0.0f;
	UpdateTalkUse(P);
}

bool AStreetNpcs::Place(FStreetPerson& P, FName StationId)
{
	const UStreetWorld* Registry = UStreetWorld::Get(this);
	AShortStackCharacter* B = P.Body;
	FTransform At;
	FName Pose;
	float Seat = 0.0f;
	if (!Registry || !B || !Registry->Station(StationId, At, Pose, Seat))
	{
		return false;
	}
	// Whoever had the station goes (a shift change: in the same unseen moment).
	for (FStreetPerson& O : People)
	{
		if (&O != &P && O.State == EStreetPersonState::Placed && O.Station == StationId && !O.bFixed)
		{
			Pool(O);
		}
	}
	Show(P, true);
	const float Half = B->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	B->SetActorLocationAndRotation(At.GetLocation() + FVector(0.0, 0.0, Half + 0.5), FRotator(0.0f, static_cast<float>(At.Rotator().Yaw), 0.0f), false, nullptr,
		ETeleportType::TeleportPhysics);
	B->SetSeated(Seat > 1.0f, Seat > 1.0f ? Seat : 46.0f);
	P.State = EStreetPersonState::Placed;
	P.Station = StationId;
	P.StationAt = FTransform(FRotator(0.0f, static_cast<float>(At.Rotator().Yaw), 0.0f), At.GetLocation());
	P.SeatHeight = Seat;
	P.Beat = NAME_None;
	P.BeatLeft = 0.0f;
	P.StuckFor = 0.0f;
	P.ServeFor = 0.0f;
	P.Slide = 0.0f;
	P.SlideWant = 0.0f;
	UpdateTalkUse(P);
	UE_LOG(LogStreetNpcs, Log, TEXT("%s at %s%s"), *P.Name, *StationId.ToString(), Seat > 1.0f ? TEXT(" (seated)") : TEXT(""));
	return true;
}

void AStreetNpcs::LendToCrowd(FStreetPerson& P, uint8 Doing)
{
	if (!Crowd || !P.Body)
	{
		return;
	}
	FCrowdNamedPlan Plan;
	// Their size, for the crowd's spacing (the body is fitted to its height already: the crowd leaves its scale be).
	Plan.Scale = FMath::Clamp(P.Height / 175.0f, 0.85f, 1.12f);
	Plan.bPlaceNow = bDark;
	Plan.From = FName(TEXT("Market")); // off the 14 at Market St (spec 6.3)
	for (const FArrival& A : Arrivals)
	{
		if (P.Id == FName(A.Id))
		{
			Plan.Pace = A.Pace;
			Plan.bUmbrella = A.bUmbrella;
			Plan.From = A.bMarket ? FName(TEXT("Market")) : NAME_None;
		}
	}
	const bool bOut = Doing == static_cast<uint8>(ss::town::Doing::Leave);
	// Sal's cigarette by the Wash & Fold's door until Dee opens up (town's Smoke after his walk up, or what's left of it).
	Plan.SmokeFor = bOut ? 0.0f : SmokeSecondsFor(P);
	Plan.Role = bOut ? ECrowdRole::PlayerOut : ECrowdRole::Player;
	// Shown only once the crowd has somewhere to bring them on unseen (it's asked again every couple of seconds till then).
	if (Crowd->AddNamed(P.Body, P.Id, Plan))
	{
		Show(P, true);
		P.State = EStreetPersonState::Walking;
		P.WalkedFor = Doing;
		P.WalkedSince = P.WantSince;
		UpdateTalkUse(P);
		UE_LOG(LogStreetNpcs, Log, TEXT("%s %s%s"), *P.Name, bOut ? TEXT("out of Dee's game, on the way home") : TEXT("on the way to Dee's game"),
			Plan.SmokeFor > 0.0f ? *FString::Printf(TEXT(", a cigarette by the Wash & Fold's door first (%.0f s)"), Plan.SmokeFor) : TEXT(""));
	}
}

void AStreetNpcs::UpdateTalkUse(FStreetPerson& P)
{
	UStreetWorld* Registry = UStreetWorld::Get(this);
	if (!Registry)
	{
		return;
	}
	// E reaches whoever is out where the hero can talk to them, but the Lucky Penny's clerk (the counter is how he's talked to)
	// and the Wash & Fold's attendant (the stage's own use).
	static const FName Store(TEXT("store"));
	const bool bWant = P.Body && !P.bFixed && P.Venue != Store && (P.State == EStreetPersonState::Placed || P.State == EStreetPersonState::Walking);
	if (bWant && P.TalkUse == 0)
	{
		FStreetUse U;
		U.Id = FName(*FString::Printf(TEXT("talk-%s"), *P.Id.ToString()));
		U.Verb = EStreetVerb::Talk;
		U.Venue = P.Venue;
		U.Param = P.Id.ToString();
		U.Prompt = FString::Printf(TEXT("Talk to %s"), *P.Name);
		U.Radius = P.State == EStreetPersonState::Walking ? 150.0f : (P.bStaff ? 110.0f : 130.0f);
		U.Anchor = P.Body->GetCapsuleComponent();
		U.Offset = FVector(0.0, 0.0, 30.0);
		P.TalkUse = Registry->AddUse(U);
	}
	else if (!bWant && P.TalkUse != 0)
	{
		Registry->RemoveUse(P.TalkUse);
		P.TalkUse = 0;
	}
}

void AStreetNpcs::UpdateEngaged(FStreetPerson& P, float Dt)
{
	AShortStackCharacter* B = P.Body;
	const AShortStackCharacter* H = Hero.Get();
	if (!B)
	{
		return;
	}
	if (P.bReleaseAfterLine && !B->IsTalking())
	{
		Release(P.Id);
		return;
	}
	if (P.State != EStreetPersonState::Placed)
	{
		return; // a walker is held by the crowd (EngageNamed)
	}
	// Turned to the hero (the body steps round with a turn clip past 50 degrees) and holding their eyes; seated, only the head.
	// The waitress on her way along the counter faces the way she's going (UpdateBeat walks her once she does), and turns back
	// to the counter when she's there.
	const FVector To = H ? H->GetActorLocation() - B->GetActorLocation() : FVector::ZeroVector;
	float Want = static_cast<float>(P.StationAt.Rotator().Yaw);
	if (P.bEngaged && H && !B->IsSeated() && To.Size2D() > 20.0)
	{
		Want = static_cast<float>(To.Rotation().Yaw);
	}
	else if (!P.bEngaged && FMath::Abs(P.SlideWant - P.Slide) > 15.0f)
	{
		const FVector Along = P.StationAt.GetRotation().GetRightVector() * (P.SlideWant > P.Slide ? 1.0 : -1.0);
		Want = static_cast<float>(Along.Rotation().Yaw);
	}
	const float Now = static_cast<float>(B->GetActorRotation().Yaw);
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(Now, Want)) > 2.0f)
	{
		B->SetActorRotation(FRotator(0.0f, FMath::FixedTurn(Now, Want, 160.0f * Dt), 0.0f));
	}
	if (P.bEngaged && H)
	{
		B->SetLookTarget(H->EyeLocation(), 1.0f);
	}
}

void AStreetNpcs::UpdateBeat(FStreetPerson& P, float Dt)
{
	AShortStackCharacter* B = P.Body;
	if (!B || P.bFixed)
	{
		return;
	}
	// Names made once (this runs every frame for everyone placed).
	static const FName Store(TEXT("store")), Barber(TEXT("barber")), Diner(TEXT("diner")), MarketStop(TEXT("market-stop"));
	static const FName Sal(TEXT("sal")), BigLou(TEXT("big-lou")), Mei(TEXT("mei"));
	static const FName Coffee(TEXT("coffee")), Attend(TEXT("attend")), Cut(TEXT("cut")), Pad(TEXT("pad")), Pour(TEXT("pour")), Lean(TEXT("lean")), Refill(TEXT("refill"));
	const AShortStackCharacter* H = Hero.Get();
	const FVector Fwd = P.StationAt.GetRotation().GetForwardVector();
	const FVector Right = P.StationAt.GetRotation().GetRightVector();
	const FVector HeroAt = H ? H->GetActorLocation() : FVector(0.0, 0.0, -1.0e6);
	const FVector Rel = HeroAt - P.StationAt.GetLocation();
	// The next beat of a cycle once this one has run its time: one of Odds (a beat, or none), for MinS..MaxS seconds.
	auto Cycle = [&](std::initializer_list<FBeatOdds> Odds, float MinS, float MaxS) {
		P.BeatLeft -= Dt;
		if (P.BeatLeft > 0.0f)
		{
			return P.Beat;
		}
		P.BeatLeft = Dice.FRandRange(MinS, MaxS);
		float Roll = Dice.FRand();
		for (const FBeatOdds& O : Odds)
		{
			Roll -= O.Odds;
			if (Roll <= 0.0f)
			{
				return O.Beat ? FName(O.Beat) : FName();
			}
		}
		return FName();
	};
	FName Want = NAME_None;
	// Back at her station unless serving (below); held where she is while she talks with the hero.
	P.SlideWant = P.bEngaged ? P.Slide : 0.0f;
	if (P.bEngaged)
	{
		Want = NAME_None;
	}
	else if (P.AskedLeft > 0.0f)
	{
		P.AskedLeft -= Dt;
		Want = P.AskedBeat;
	}
	else if (P.SeatHeight > 1.0f)
	{
		// Ruby's regulars at the counter or in a booth: Sal behind the paper on his mornings, Big Lou at his plate, Mei on her
		// phone, everyone at their coffee between.
		if (P.Id == Sal)
		{
			Want = Cycle({{TEXT("paper"), 0.65f}, {TEXT("coffee"), 0.35f}}, 14.0f, 32.0f);
		}
		else if (P.Id == BigLou)
		{
			Want = Cycle({{TEXT("eat"), 0.7f}, {TEXT("coffee"), 0.3f}}, 15.0f, 30.0f);
		}
		else if (P.Id == Mei)
		{
			Want = Cycle({{TEXT("phonesit"), 0.6f}, {TEXT("coffee"), 0.4f}}, 12.0f, 28.0f);
		}
		else
		{
			Want = Coffee;
		}
	}
	else if (P.Venue == Store && P.bStaff)
	{
		// The clerk: the hands on the counter for someone at it; otherwise the phone, the rag, or nothing much.
		const bool bCustomer = Rel.Size2D() < 260.0 && FVector::DotProduct(Rel.GetSafeNormal2D(), Fwd) > 0.3;
		Want = bCustomer ? Attend : Cycle({{nullptr, 0.45f}, {TEXT("phone"), 0.30f}, {TEXT("wipe"), 0.25f}}, 8.0f, 22.0f);
	}
	else if (P.Venue == Barber && P.bStaff)
	{
		// The barber cuts while the hero is in his chair (its head mark before him).
		const FVector Mark = P.StationAt.GetLocation() + Fwd * 50.0;
		Want = H && H->IsSeated() && FVector::Dist2D(HeroAt, Mark) < 110.0 ? Cut : FName();
	}
	else if (P.Venue == Diner && P.bStaff)
	{
		// The waitress: along the counter to whoever sits at it (the hero; now and then a regular on a stool), the pad, the
		// coffee poured; between, the rag on the counter and a lean on the pass.
		FVector Served = FVector::ZeroVector;
		if (H && H->IsSeated() && FVector::DotProduct(Rel, Fwd) > 0.0 && FVector::DotProduct(Rel, Fwd) < 260.0 && FMath::Abs(FVector::DotProduct(Rel, Right)) < 420.0)
		{
			Served = HeroAt;
		}
		else if (P.ServeFor > 0.0f && P.ServeFor < 9.0f)
		{
			// Finishing a refill for a regular.
			for (const FStreetPerson& O : People)
			{
				const FVector At = O.Body ? O.Body->GetActorLocation() - P.StationAt.GetLocation() : FVector::ZeroVector;
				if (&O != &P && O.State == EStreetPersonState::Placed && O.SeatHeight > 60.0f && O.Body && FMath::Abs(FVector::DotProduct(At, Right)) < 420.0 &&
					FVector::DotProduct(At, Fwd) > 0.0)
				{
					Served = O.Body->GetActorLocation();
					break;
				}
			}
		}
		if (!Served.IsZero())
		{
			P.ServeFor += Dt;
			P.SlideWant = FMath::Clamp(static_cast<float>(FVector::DotProduct(Served - P.StationAt.GetLocation(), Right)), -380.0f, 380.0f);
			const bool bThere = FMath::Abs(P.Slide - P.SlideWant) < 15.0f;
			Want = !bThere ? FName() : (P.ServeFor < 6.0f && Served == HeroAt ? Pad : (P.ServeFor < 12.0f ? Pour : Lean));
		}
		else
		{
			P.ServeFor = 0.0f;
			Want = Cycle({{TEXT("wipe"), 0.3f}, {TEXT("lean"), 0.25f}, {TEXT("refill"), 0.15f}, {nullptr, 0.3f}}, 10.0f, 25.0f);
			if (Want == Refill)
			{
				// A refill for whoever's at the counter (the next pass of the serving branch finds them), or nothing.
				P.ServeFor = 0.01f;
				P.BeatLeft = 0.0f;
				Want = NAME_None;
			}
		}
	}
	else if (P.WantVenue == MarketStop && !P.bStaff)
	{
		// Waiting for the 14: the phone, a look up the street for it.
		Want = Cycle({{TEXT("phonestand"), 0.55f}, {nullptr, 0.45f}}, 10.0f, 24.0f);
	}
	// Along the counter to the one being served, and back again after (at a walk, the gait stepping with it): turned the way
	// she's going first (UpdateEngaged turns her), so she walks it rather than sidestepping; the last few cm as she turns back.
	const float Gap = P.SlideWant - P.Slide;
	const float AlongYaw = static_cast<float>((Right * (Gap > 0.0f ? 1.0 : -1.0)).Rotation().Yaw);
	const bool bFacingAlong = FMath::Abs(FMath::FindDeltaAngleDegrees(static_cast<float>(B->GetActorRotation().Yaw), AlongYaw)) < 35.0f;
	if (!FMath::IsNearlyEqual(P.Slide, P.SlideWant, 0.5f) && (bFacingAlong || FMath::Abs(Gap) <= 15.0f))
	{
		const float Was = P.Slide;
		P.Slide = FMath::FInterpConstantTo(P.Slide, P.SlideWant, Dt, 115.0f);
		const float Half = B->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		B->SetActorLocation(P.StationAt.GetLocation() + Right * P.Slide + FVector(0.0, 0.0, Half + 0.5), false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
		{
			Move->Velocity = Right * ((P.Slide - Was) / FMath::Max(Dt, 0.001f));
		}
	}
	else if (UCharacterMovementComponent* Move = B->GetCharacterMovement(); Move && !Move->Velocity.IsNearlyZero())
	{
		Move->Velocity = FVector::ZeroVector;
	}
	const FTransform Work(P.StationAt.GetRotation(), P.StationAt.GetLocation() + Right * P.Slide);
	if (Want != P.Beat || B->GetWorkBeat() != Want)
	{
		P.Beat = Want;
		B->SetWork(Want, Work);
	}
	DressProps(P, Want);
	// The eyes on the work (the attention stack's point of interest); idle, the character's own (the hero when near).
	FVector Focus;
	if (!P.bEngaged)
	{
		if (!Want.IsNone() && BeatFocus(Want, P.SeatHeight, Focus))
		{
			B->SetLookTarget(Work.TransformPosition(Focus), 0.8f);
		}
		else
		{
			B->SetLookTarget(FVector::ZeroVector, 0.0f);
		}
	}
}

void AStreetNpcs::UpdatePeople(float Dt)
{
	using ss::town::Doing;
	const UStreetWorld* Registry = UStreetWorld::Get(this);
	const AShortStackCharacter* H = Hero.Get();
	const uint8 Arrive = static_cast<uint8>(Doing::Arrive);
	const uint8 Leave = static_cast<uint8>(Doing::Leave);
	// Back from the crowd: in at Dee's steel door (at her game now), or off the set (home for the night).
	if (Crowd)
	{
		for (const FName Back : Crowd->TakeReturned())
		{
			if (FStreetPerson* P = Person(Back))
			{
				P->State = EStreetPersonState::Pooled;
				Show(*P, false);
				P->State = P->WalkedFor == Arrive ? EStreetPersonState::Away : EStreetPersonState::Pooled;
				UpdateTalkUse(*P);
			}
		}
	}
	const double Now = ScheduleMinutes();
	int32 Builds = bDark ? 2 : (bFadingIn ? 1 : 0);
	for (FStreetPerson& P : People)
	{
		if (P.bFixed || P.bNoBody)
		{
			continue;
		}
		FTransform StationT;
		float Seat = 0.0f;
		const EWant Want = WantOf(P, &StationT, &Seat);
		if (Want == EWant::Pending && !P.bWhyLogged)
		{
			P.bWhyLogged = true;
			UE_LOG(LogStreetNpcs, Log, TEXT("%s is due at %s, which isn't registered (its shop stage isn't up): kept in the pool"), *P.Name, *P.WantStation.ToString());
		}
		if (P.Face.IsNone())
		{
			if (Want != EWant::Off && Want != EWant::Inside && Want != EWant::Pending && !P.bWhyLogged)
			{
				P.bWhyLogged = true;
				UE_LOG(LogStreetNpcs, Warning, TEXT("%s has no face to wear (not built, nothing to borrow): stays home"), *P.Name);
			}
			continue;
		}
		const bool bWalk = Want == EWant::WalkIn || Want == EWant::WalkOut;
		const uint8 WalkDoing = Want == EWant::WalkOut ? Leave : Arrive;
		const bool bWalked = P.WalkedFor == WalkDoing && P.WalkedSince == P.WantSince;
		// A body for whoever needs one now (only while the screen is dark or fading in: every build is a hitch).
		const bool bNeedBody = Want == EWant::Station || (bWalk && !bWalked && P.State != EStreetPersonState::Walking);
		if (bNeedBody && !P.Body && Builds > 0)
		{
			--Builds;
			BuildBody(P);
		}
		switch (P.State)
		{
		case EStreetPersonState::Pooled:
		case EStreetPersonState::Away:
			if (Want == EWant::Station && P.Body)
			{
				// Onto their station (a shift starting, a regular at Ruby's, someone at the 14's shelter) only where nobody
				// sees them appear, and never as a second of a face already in view (spec 6.7's face registry).
				// Looked at a few times a second (each look is a few traces), at once under black.
				P.StuckFor -= Dt;
				if (bDark || P.StuckFor <= 0.0f)
				{
					P.StuckFor = 0.35f;
					const UCastFaceRegistry* Faces = UCastFaceRegistry::Get(this);
					const bool bDoubled = Faces && bHasView && !bDark && Faces->InViewElsewhere(P.Face, P.Body, ViewAt, ViewAxes[0]);
					if (!bDoubled && CanAppear(StationT.GetLocation()))
					{
						Place(P, P.WantStation);
					}
				}
			}
			else if (bWalk && P.Body && !bWalked)
			{
				// Each walk once: up to Dee's from the pool (not back out of her game), out of her game (or, the street opened as
				// they were leaving, out of her door all the same). Every couple of seconds until there's somewhere to come on unseen.
				if (Want == EWant::WalkOut || P.State == EStreetPersonState::Pooled)
				{
					P.StuckFor -= Dt;
					if (P.StuckFor <= 0.0f)
					{
						LendToCrowd(P, WalkDoing);
						P.StuckFor = P.State == EStreetPersonState::Walking ? 0.0f : 2.0f;
					}
				}
			}
			else if (Want == EWant::Inside)
			{
				P.State = EStreetPersonState::Away; // at her table already (the street opened after they went in)
			}
			else if (Want == EWant::Off && P.State == EStreetPersonState::Away)
			{
				P.State = EStreetPersonState::Pooled; // home from Dee's, unseen
			}
			break;
		case EStreetPersonState::Placed:
		{
			if (Want == EWant::Station && P.Station == P.WantStation)
			{
				break;
			}
			if (P.bEngaged)
			{
				break; // not mid-sentence
			}
			// Off shift (or on to somewhere else): gone once nobody's looking (or under black, or while the budget has them hidden:
			// an asleep shop's keeper, someone far off). Looked at a few times a second, not every frame (each look is a few
			// traces). A clerk whose relief can't come yet stays on, and says so to a hero at the counter.
			bool bGo = bDark || !P.Body || P.Body->IsHidden();
			P.LookIn -= Dt;
			if (!bGo && P.LookIn <= 0.0f)
			{
				P.LookIn = 0.3f;
				bGo = !Seen(P.Body->GetActorLocation() - FVector(0.0, 0.0, P.Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), 170.0f);
			}
			if (bGo)
			{
				Pool(P);
			}
			else
			{
				P.StuckFor += Dt;
				if (!P.bLateSaid && H && P.Body && FVector::Dist2D(H->GetActorLocation(), P.Body->GetActorLocation()) < 300.0)
				{
					for (const FStreetPerson& O : People)
					{
						if (&O != &P && O.bWanted && O.WantStation == P.Station && !O.Name.IsEmpty())
						{
							P.bLateSaid = true;
							Speak(P.Id, FString::Printf(TEXT("%s's late."), *O.Name));
							break;
						}
					}
				}
			}
			break;
		}
		case EStreetPersonState::Walking:
			if (Crowd && !Crowd->IsNamedOut(P.Id))
			{
				// The crowd let them go without a word (another street began): back in the pool.
				P.State = EStreetPersonState::Pooled;
				Show(P, false);
				UpdateTalkUse(P);
			}
			break;
		}
		if (P.State == EStreetPersonState::Placed)
		{
			UpdateEngaged(P, Dt);
			UpdateBeat(P, Dt);
		}
		else if (P.bReleaseAfterLine && P.Body && !P.Body->IsTalking())
		{
			Release(P.Id);
		}
	}
	// Whoever the next hours want, built ahead while the screen is black (the next clerk, tonight's regulars, Ruby's
	// morning), so the arrivals and the shift change don't wait for a black that may not come. Who that is is read once per
	// schedule read (a world minute), not on every dark frame: the schedule's reads aren't free.
	if (Builds > 0 && bHourKnown)
	{
		if (AheadSlot != ReadSlot)
		{
			AheadSlot = ReadSlot;
			AheadIds.Reset();
			for (const double Ahead : {20.0, 90.0, 240.0})
			{
				for (const ss::town::Placement& Pl : WhoAt(Now + Ahead))
				{
					const FName Station = Pl.Station.empty() ? NAME_None : FName(*Utf8(Pl.Station));
					FTransform T;
					FName Pose;
					float Seat = 0.0f;
					const bool bOnStreet = Station == WashDoorStation || (!Station.IsNone() && Station != DeeTable && Registry && Registry->Station(Station, T, Pose, Seat));
					if (bOnStreet)
					{
						AheadIds.AddUnique(FName(*Utf8(Pl.Id)));
					}
				}
			}
		}
		for (const FName Id : AheadIds)
		{
			FStreetPerson* P = Builds > 0 ? Person(Id) : nullptr;
			if (P && !P->Body && !P->Face.IsNone() && !P->bFixed && !P->bNoBody)
			{
				--Builds;
				BuildBody(*P);
			}
		}
	}
}

// ------------------------------------------------------------------ talk

void AStreetNpcs::Engage(FName PersonId)
{
	FStreetPerson* P = Person(PersonId);
	if (!P || !P->Body)
	{
		return;
	}
	P->bEngaged = true;
	P->bReleaseAfterLine = false;
	P->Body->SetWork(NAME_None, P->StationAt);
	P->Beat = NAME_None;
	ClearProps(*P);
	if (P->State == EStreetPersonState::Walking && Crowd)
	{
		Crowd->EngageNamed(PersonId, true);
	}
	if (const AShortStackCharacter* H = Hero.Get())
	{
		P->Body->SetLookTarget(H->EyeLocation(), 1.0f);
	}
}

void AStreetNpcs::Release(FName PersonId)
{
	FStreetPerson* P = Person(PersonId);
	if (!P)
	{
		return;
	}
	if (P->bEngaged && P->Body && P->Body->IsTalking() && !P->bReleaseAfterLine)
	{
		// Mid-line (the goodbye said as the card closes): they hold the hero's eyes until it's said, then go back to what they
		// were doing (UpdatePeople lets them go).
		P->bReleaseAfterLine = true;
		return;
	}
	P->bEngaged = false;
	P->bReleaseAfterLine = false;
	Talks.Remove(PersonId);
	if (P->Body)
	{
		P->Body->SetLookTarget(FVector::ZeroVector, 0.0f);
	}
	if (P->State == EStreetPersonState::Walking && Crowd)
	{
		Crowd->EngageNamed(PersonId, false);
	}
}

void AStreetNpcs::Speak(FName PersonId, const FString& Line, float Seconds)
{
	const FStreetPerson* P = FindPerson(PersonId);
	if (Line.IsEmpty())
	{
		return;
	}
	const FString Speaker = P ? P->Name : PersonId.ToString();
	if (P && P->Body)
	{
		P->Body->SetTalking(Seconds > 0.0f ? Seconds : TalkFace::SecondsFor(Line), 0.6f);
	}
	if (Subtitles.Num() >= 4)
	{
		Subtitles.RemoveAt(0);
	}
	Subtitles.Add({Speaker, Line});
	UE_LOG(LogStreetNpcs, Log, TEXT("%s: \"%s\""), *Speaker, *Line);
}

bool AStreetNpcs::TakeSubtitle(FString& OutSpeaker, FString& OutLine)
{
	if (Subtitles.Num() == 0)
	{
		return false;
	}
	OutSpeaker = Subtitles[0].Speaker;
	OutLine = Subtitles[0].Line;
	Subtitles.RemoveAt(0);
	return true;
}

void AStreetNpcs::UpdateNameTag(float Dt)
{
	// Whom the hero's eyes are on: one of the block's people within 4 m, their head within a few degrees of the look.
	const AShortStackCharacter* H = Hero.Get();
	FName Now = NAME_None;
	if (H)
	{
		const FVector Eye = H->EyeLocation();
		const FVector Dir = H->LookDirection().GetSafeNormal();
		double Best = 0.0;
		for (const FStreetPerson& P : People)
		{
			if (!P.Body || P.bFixed || (P.State != EStreetPersonState::Placed && P.State != EStreetPersonState::Walking) || P.Body->GetDetailTier() >= 3)
			{
				continue;
			}
			const FVector To = P.Body->EyeLocation() - Eye;
			const double Dist = To.Size();
			const double Cos = Dist > 1.0 ? FVector::DotProduct(To / Dist, Dir) : 0.0;
			// Nearer, the cone is wider (a head is a bigger mark at a meter than at four).
			const double Need = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(14.0 - Dist / 50.0, 6.0, 12.0)));
			if (Dist < 400.0 && Cos > Need && Cos > Best)
			{
				Best = Cos;
				Now = P.Id;
			}
		}
	}
	LookedFor = Now == LookedAt && !Now.IsNone() ? LookedFor + Dt : 0.0f;
	LookedAt = Now;
	if (!Now.IsNone() && LookedFor >= 0.4f)
	{
		if (TagWho != Now)
		{
			TagWho = Now;
			TagAlpha = 0.0f;
		}
		TagAlpha = FMath::Min(1.0f, TagAlpha + Dt / 0.25f);
	}
	else if (!TagWho.IsNone())
	{
		// Holds a moment once the eyes move on, then fades.
		TagAlpha = FMath::Max(0.0f, TagAlpha - Dt / 0.4f);
		if (TagAlpha <= 0.0f)
		{
			TagWho = NAME_None;
		}
	}
}

bool AStreetNpcs::NameTag(FString& OutName, FString& OutSub, FVector& OutAt, float& OutAlpha) const
{
	const FStreetPerson* P = TagWho.IsNone() ? nullptr : FindPerson(TagWho);
	if (!P || !P->Body || TagAlpha <= 0.0f)
	{
		return false;
	}
	// Someone the hero has no reason to know is just "Stranger" (no name to give them yet).
	const ss::Session* S = CareerSession();
	const std::string Tag = S ? ss::town::Tag(Std(P->Id.ToString()), *S) : std::string();
	const bool bKnown = !S || !Tag.empty();
	OutName = bKnown ? P->Name : FString(TEXT("Stranger"));
	OutSub = !Tag.empty() ? Utf8(Tag) : (S ? FString() : P->Label);
	OutAt = P->Body->EyeLocation() + FVector(0.0, 0.0, 32.0);
	OutAlpha = TagAlpha;
	return true;
}

std::shared_ptr<const ss::town::TalkContext> AStreetNpcs::ContextFor(const FStreetPerson& P) const
{
	const ss::Session* S = CareerSession();
	if (!S)
	{
		return nullptr;
	}
	// Where they're met: their shop, or where the schedule has them (one of Ruby's regulars at the counter, Sal by the Wash
	// & Fold's door).
	const FName Venue = !P.WantVenue.IsNone() ? P.WantVenue : P.Venue;
	return ss::town::ContextFor(*S, Std(P.Id.ToString()), Std(Venue.IsNone() ? FString() : Venue.ToString()));
}

FString AStreetNpcs::FallbackLine(const FStreetPerson& P, const FString& Moment) const
{
	const int32 Pick = static_cast<int32>(FMath::FloorToDouble(WorldMinutes / 37.0)) + static_cast<int32>(GetTypeHash(P.Id) & 0xff);
	const FString Line = VoiceLine(P.Id, Moment, Pick);
	if (!Line.IsEmpty())
	{
		return Line;
	}
	if (Moment == TEXT("greet"))
	{
		return P.bStaff ? TEXT("Help you?") : TEXT("Hey.");
	}
	if (Moment == TEXT("bump"))
	{
		return P.bStaff ? TEXT("Easy.") : TEXT("Easy, champ.");
	}
	return FString();
}

FString AStreetNpcs::BarkFor(FName PersonId, const FString& Trigger) const
{
	const FStreetPerson* P = FindPerson(PersonId);
	if (!P)
	{
		return FString();
	}
	if (const std::shared_ptr<const ss::town::TalkContext> C = ContextFor(*P))
	{
		const std::string Said = ss::town::Bark(*C, Std(Trigger));
		if (!Said.empty())
		{
			return Utf8(Said);
		}
		if (Trigger == TEXT("closing"))
		{
			const std::string Keeper = ss::town::KeeperLine(*C, "closing");
			if (!Keeper.empty())
			{
				return Utf8(Keeper);
			}
		}
	}
	return FallbackLine(*P, Trigger);
}

bool AStreetNpcs::OpenTalk(FName PersonId, ss::town::Talk& Out)
{
	FStreetPerson* P = Person(PersonId);
	if (!P || !P->Body || (P->State != EStreetPersonState::Placed && P->State != EStreetPersonState::Walking))
	{
		return false;
	}
	Engage(PersonId);
	Out = ss::town::Talk();
	if (const std::shared_ptr<const ss::town::TalkContext> C = ContextFor(*P))
	{
		Out = ss::town::Open(*C);
	}
	if (Out.Name.empty())
	{
		Out.Name = Std(P->Name);
	}
	if (Out.Label.empty())
	{
		Out.Label = Std(P->Label);
	}
	if (Out.Line.empty())
	{
		Out.Line = Std(FallbackLine(*P, TEXT("greet")));
	}
	if (Out.Choices.empty())
	{
		// Spec 6.4's card when core's talk has nothing (no career on this map): the news, the games, the person's service, goodbye.
		Out.Choices.push_back({"news", "What's new?"});
		Out.Choices.push_back({"games", "Any games around?"});
		const FString Venue = P->Venue.ToString();
		const char* Service = !P->bStaff ? nullptr
							  : Venue == TEXT("barber") ? "Get a cut"
							  : Venue == TEXT("diner")	? "Menu"
							  : Venue == TEXT("gym")	? "Day pass"
							  : Venue == TEXT("pawn")	? "Sell or pawn"
							  : Venue == TEXT("thrift") ? "Browse the racks"
														: nullptr;
		if (Service)
		{
			Out.Choices.push_back({"service", Service});
		}
		Out.Choices.push_back({"bye", "See you."});
	}
	if (ss::Session* S = CareerSession())
	{
		S->Heard(Std(PersonId.ToString()), Out);
	}
	Talks.Add(PersonId, nullptr); // talking: the answers build their context afresh (it reads what's been heard)
	Speak(PersonId, Utf8(Out.Line));
	return true;
}

bool AStreetNpcs::AnswerTalk(FName PersonId, const FString& ChoiceId, ss::town::Talk& Out)
{
	FStreetPerson* P = Person(PersonId);
	if (!P || !P->Body)
	{
		return false;
	}
	Out = ss::town::Talk();
	ss::Session* S = CareerSession();
	// A context built after what was last heard (Talk.h: build a new context after Heard).
	if (const std::shared_ptr<const ss::town::TalkContext> C = ContextFor(*P))
	{
		Out = ss::town::Answer(*C, Std(ChoiceId));
	}
	// Twitch's twenty (Talk.h): lent through the session, or why not said instead.
	if (S && ChoiceId == TEXT("lend-yes"))
	{
		const std::string Why = S->Lend(Std(PersonId.ToString()), 2000);
		if (!Why.empty())
		{
			Out.Line = Why;
			Out.SaidKey.clear();
		}
	}
	if (Out.Name.empty())
	{
		Out.Name = Std(P->Name);
	}
	if (Out.Line.empty())
	{
		if (ChoiceId == TEXT("news"))
		{
			Out.Line = Rain > 0.5f ? "Rain's keeping everybody in. Quiet." : "Quiet. Same faces.";
		}
		else if (ChoiceId == TEXT("games"))
		{
			Out.Line = "Dee runs a game Tuesdays, Thursdays and Saturdays. In through the Wash & Fold, the steel door at the back.";
		}
		else if (ChoiceId == TEXT("service"))
		{
			Out.Line = "Come on, then.";
		}
		else
		{
			Out.Line = "See you.";
		}
	}
	const bool bEnds = ChoiceId == TEXT("bye") || ChoiceId == TEXT("service") || (!S && ChoiceId != TEXT("news") && ChoiceId != TEXT("games")) || (S && Out.Choices.empty());
	if (!bEnds && Out.Choices.empty())
	{
		Out.Choices.push_back({"news", "What's new?"});
		Out.Choices.push_back({"games", "Any games around?"});
		Out.Choices.push_back({"bye", "See you."});
	}
	if (bEnds)
	{
		Out.Choices.clear();
	}
	if (S)
	{
		S->Heard(Std(PersonId.ToString()), Out);
	}
	Speak(PersonId, Utf8(Out.Line));
	// Held for as long as the card is up, even past their last word (it closes with "See you."): the card's close releases
	// them (Release, after the line).
	return true;
}

void AStreetNpcs::PlayBeat(FName PersonId, FName Beat, float Seconds)
{
	if (FStreetPerson* P = Person(PersonId))
	{
		P->AskedBeat = Beat;
		P->AskedLeft = FMath::Max(0.1f, Seconds);
	}
}

FName AStreetNpcs::ServiceBeatFor(FName Venue, const FString& OfferId)
{
	const FString V = Venue.ToString();
	if (V == TEXT("barber"))
	{
		return TEXT("chair");
	}
	if (V == TEXT("gym"))
	{
		return OfferId.Contains(TEXT("treadmill")) ? FName(TEXT("treadmill")) : OfferId.Contains(TEXT("bag")) ? FName(TEXT("bag")) : FName(TEXT("weights"));
	}
	if (V == TEXT("diner"))
	{
		// A drink or a while sat (diner/coffee, /oj, /milkshake; "sit a while"): the mug; a plate (diner/short-stack, /pie...): the fork.
		const bool bDrink = OfferId.Contains(TEXT("coffee")) || OfferId.Contains(TEXT("juice")) || OfferId.EndsWith(TEXT("/oj")) || OfferId.Contains(TEXT("shake")) ||
							OfferId.Contains(TEXT("sit")) || OfferId.Contains(TEXT("wait"));
		return bDrink ? FName(TEXT("coffee")) : FName(TEXT("eat"));
	}
	return NAME_None;
}

// ------------------------------------------------------------------ the budget

int32 AStreetNpcs::NumLive() const
{
	int32 N = 0;
	for (const FStreetPerson& P : People)
	{
		N += P.Body && !P.Body->IsHidden() && P.Body->GetDetailTier() < 3 ? 1 : 0;
	}
	return N;
}

int32 AStreetNpcs::CrowdAllowance() const
{
	const int32 Cap = CVarBudget.GetValueOnGameThread() >= 0 ? CVarBudget.GetValueOnGameThread() : MaxLive;
	// The live cap less the director's people out of the pool, and the visible cap less those of them drawn now: kept to
	// both, the crowd sends someone off before the budget would have to hide a pedestrian in plain view to keep 8 visible.
	int32 Mine = 0;
	int32 MineDrawn = 0;
	for (const FStreetPerson& P : People)
	{
		if (P.Body && P.State != EStreetPersonState::Pooled && P.State != EStreetPersonState::Away)
		{
			++Mine;
			MineDrawn += P.Body->GetDetailTier() < 3 ? 1 : 0;
		}
	}
	return FMath::Clamp(FMath::Min(Cap - Mine, MaxVisible - MineDrawn) - (bAtFloor ? 2 : 0), 0, 6);
}

void AStreetNpcs::Arbitrate()
{
	struct FEntry
	{
		AShortStackCharacter* Body = nullptr;
		int32 Priority = 4;
		double Dist = 0.0;
		int32 Want = 0;
	};
	TArray<FEntry, TInlineAllocator<24>> All;
	TMap<FName, TWeakObjectPtr<AShopStage>> Shops;
	for (TActorIterator<AShopStage> It(GetWorld()); It; ++It)
	{
		Shops.Add(It->Venue, *It);
	}
	auto Want = [this](const AShortStackCharacter* B, double& OutDist) {
		OutDist = bHasView ? FVector::Dist(ViewAt, B->GetActorLocation()) : 0.0;
		const int32 Was = B->GetDetailTier();
		const double D = OutDist;
		if (D < NearRange + (Was == 0 ? TierRoom : -TierRoom))
		{
			return 0;
		}
		if (D < MidRange + (Was <= 1 ? TierRoom : -TierRoom))
		{
			return 1;
		}
		if (D < FarRange + (Was <= 2 ? TierRoom : -TierRoom))
		{
			return 2;
		}
		return 3;
	};
	for (FStreetPerson& P : People)
	{
		if (!P.Body || P.State == EStreetPersonState::Pooled || P.State == EStreetPersonState::Away)
		{
			continue;
		}
		FEntry E;
		E.Body = P.Body;
		E.Want = Want(P.Body, E.Dist);
		// Whoever is talking to the hero first, then the staff in the hero's zone, the world's people, the other staff.
		E.Priority = P.bEngaged || P.Body->IsTalking() ? 0 : (P.bStaff && !P.Venue.IsNone() && P.Venue == HeroZone ? 1 : (!P.bStaff ? 2 : 3));
		if (P.bEngaged)
		{
			E.Want = 0;
		}
		// Inside a shop that's asleep (spec 3.8): nobody is drawn.
		if (const TWeakObjectPtr<AShopStage>* Shop = Shops.Find(!P.WantVenue.IsNone() ? P.WantVenue : P.Venue); Shop && Shop->IsValid() && !(*Shop)->IsAwake() && P.State == EStreetPersonState::Placed)
		{
			E.Want = 3;
		}
		P.Priority = E.Priority;
		All.Add(E);
	}
	if (Crowd)
	{
		Crowd->ForEachPedestrian([&](AShortStackCharacter* B) {
			FEntry E;
			E.Body = B;
			E.Want = Want(B, E.Dist);
			E.Priority = 4;
			All.Add(E);
		});
	}
	All.Sort([](const FEntry& A, const FEntry& B) { return A.Priority != B.Priority ? A.Priority < B.Priority : A.Dist < B.Dist; });
	int32 Visible = 0;
	int32 Near = 0;
	for (FEntry& E : All)
	{
		if (E.Want < 3)
		{
			if (Visible < MaxVisible)
			{
				++Visible;
			}
			else
			{
				// Over the visible cap: nobody new is shown, and nobody already drawn vanishes in plain view (kept at Far, the
				// cheapest drawn tier, until the crowd's allowance, which counts the director's drawn people, walks someone off).
				E.Want = E.Body->GetDetailTier() < 3 ? 2 : 3;
			}
		}
		if (E.Want == 0)
		{
			if (Near < MaxNear)
			{
				++Near;
			}
			else
			{
				E.Want = 1;
			}
		}
		E.Body->SetDetailTier(E.Want);
	}
	// The crowd's share of the live cap (and the first cut when the frame budget is at its floor).
	if (Crowd)
	{
		Crowd->SetAllowance(CrowdAllowance());
	}
}

// ------------------------------------------------------------------ the camera

void AStreetNpcs::UpdateView()
{
	bHasView = false;
	bDark = false;
	bFadingIn = false;
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return;
	}
	bHasView = true;
	ViewAt = Cam->GetCameraLocation();
	const FRotationMatrix R(Cam->GetCameraRotation());
	ViewAxes[0] = R.GetUnitAxis(EAxis::X);
	ViewAxes[1] = R.GetUnitAxis(EAxis::Y);
	ViewAxes[2] = R.GetUnitAxis(EAxis::Z);
	double Aspect = 16.0 / 9.0;
	if (UGameViewportClient* Viewport = World->GetGameViewport())
	{
		FVector2D Size;
		Viewport->GetViewportSize(Size);
		if (Size.X > 0.0 && Size.Y > 0.0)
		{
			Aspect = Size.X / Size.Y;
		}
	}
	const double Half = FMath::DegreesToRadians(FMath::Clamp(static_cast<double>(Cam->GetFOVAngle()), 20.0, 170.0) * 0.5);
	ViewTanX = FMath::Tan(Half) * 1.15 + 0.08;
	ViewTanY = FMath::Tan(Half) / Aspect * 1.15 + 0.08;
	bFadingIn = Cam->bEnableFading && Cam->FadeAlpha.X > Cam->FadeAlpha.Y && Cam->FadeAmount > 0.05f;
	bDark = Cam->bEnableFading && Cam->FadeAmount > 0.85f;
}

bool AStreetNpcs::Seen(const FVector& At, float Height) const
{
	if (!bHasView || bDark)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	for (const float Up : {15.0f, Height * 0.55f, Height})
	{
		const FVector P = At + FVector(0.0, 0.0, Up);
		const FVector To = P - ViewAt;
		const double Dist = To.Size();
		if (Dist < 250.0)
		{
			return true;
		}
		if (Dist > 9000.0)
		{
			continue;
		}
		const double Ahead = FVector::DotProduct(To, ViewAxes[0]);
		if (Ahead <= 0.0 || FMath::Abs(FVector::DotProduct(To, ViewAxes[1])) > Ahead * ViewTanX || FMath::Abs(FVector::DotProduct(To, ViewAxes[2])) > Ahead * ViewTanY)
		{
			continue;
		}
		// In the frame: seen, unless something solid and drawn stands between (people don't hide anyone, nor does glass: the
		// staff behind a shop's window are seen from the street).
		FCollisionQueryParams Query(SCENE_QUERY_STAT(StreetNpcsSight), false);
		bool bBlocked = false;
		for (int32 Try = 0; Try < 4; ++Try)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, ViewAt, P, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
			{
				break;
			}
			const UPrimitiveComponent* C = Hit.GetComponent();
			if (C && C->IsVisible() && !SeeThrough(C))
			{
				bBlocked = true;
				break;
			}
			Query.AddIgnoredComponent(C);
		}
		if (!bBlocked)
		{
			return true;
		}
	}
	return false;
}

bool AStreetNpcs::CanAppear(const FVector& At) const
{
	return bDark || !Seen(At, 175.0f);
}

void AStreetNpcs::HoldFade(float Dt)
{
	// The load's fade holds black until everyone wanted now is built (spec 6.7): every build is a hitch nobody should see.
	// Each fade waits 10 s at most (the count starts again with the next one).
	if (!bFadingIn)
	{
		FadeHeld = 0.0f;
		bFadeGaveUp = false;
		return;
	}
	if (IsReady())
	{
		return;
	}
	if (FadeHeld > 10.0f)
	{
		if (!bFadeGaveUp)
		{
			bFadeGaveUp = true;
			UE_LOG(LogStreetNpcs, Warning, TEXT("The fade stopped waiting for the people after 10 s (ss.Npc.List says who isn't built)"));
		}
		return;
	}
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	if (APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr; Cam && Cam->FadeTime > 0.0f)
	{
		Cam->FadeTimeRemaining = Cam->FadeTime;
		FadeHeld += Dt;
	}
}

bool AStreetNpcs::IsReady() const
{
	if (!bBegun)
	{
		return true; // nobody to wait for
	}
	if (!bHourKnown || ReadSlot < 0)
	{
		return false; // who's wanted isn't known yet
	}
	for (const FStreetPerson& P : People)
	{
		if (!P.bWanted || P.bFixed || P.bNoBody || P.Face.IsNone() || P.Body)
		{
			continue;
		}
		const EWant Want = WantOf(P);
		if (Want == EWant::Station || Want == EWant::WalkIn || Want == EWant::WalkOut)
		{
			return false;
		}
	}
	return true;
}

AShortStackCharacter* AStreetNpcs::GetHero() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}

// ------------------------------------------------------------------ each frame

void AStreetNpcs::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBegun || !Stage || !GetWorld())
	{
		return;
	}
	if (!Hero.IsValid())
	{
		Hero = GetHero();
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Clock += Dt;
	UpdateView();
	if (const UStreetWorld* Registry = UStreetWorld::Get(this); Registry && Hero.IsValid())
	{
		const FStreetZone* Zone = Registry->ZoneAt(Hero->GetActorLocation());
		HeroZone = Zone ? Zone->Id : NAME_None;
	}
	// The schedule: every world minute (core's placements are minutes long: a walk up from the stop, a cigarette, a
	// hand-off), on a jump of the clock (a skip, sleep: more than five minutes in a frame), or when the test hour changes.
	if (bHourKnown)
	{
		const double Now = ScheduleMinutes();
		const int64 Slot = static_cast<int64>(FMath::FloorToDouble(Now));
		const bool bJump = LastSeenMinutes >= 0.0 && FMath::Abs(Now - LastSeenMinutes) > 5.0;
		LastSeenMinutes = Now;
		if (Slot != ReadSlot || bJump || ReadOverride != CVarHour.GetValueOnGameThread())
		{
			ReadSchedule();
		}
	}
	if (!bPaused && bHourKnown)
	{
		UpdatePeople(Dt);
		HeroServiceBeat();
	}
	UpdateNameTag(bPaused ? 0.0f : Dt);
	HeroProps();
	// The budget, five times a second (and the floor of the frame budget, which cuts the pedestrians first: the render scale
	// held at FrameBudget's floor for two seconds).
	static const IConsoleVariable* const ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	if (const IConsoleVariable* Scale = ScreenPercentage)
	{
		FloorFor = Scale->GetFloat() <= FFrameBudget::MinPercent + 0.5f ? FloorFor + Dt : 0.0f;
		bAtFloor = FloorFor > 2.0f;
	}
	ArbitrateIn -= Dt;
	if (ArbitrateIn <= 0.0f)
	{
		ArbitrateIn = 0.2f;
		Arbitrate();
	}
	HoldFade(Dt);
#if ENABLE_DRAW_DEBUG
	if (CVarDebug.GetValueOnGameThread() > 0)
	{
		static const TCHAR* const States[] = {TEXT("pooled"), TEXT("placed"), TEXT("walking"), TEXT("away")};
		for (const FStreetPerson& P : People)
		{
			if (P.Body && P.State != EStreetPersonState::Pooled && P.State != EStreetPersonState::Away)
			{
				DrawDebugString(GetWorld(), P.Body->GetActorLocation() + FVector(0.0, 0.0, 120.0),
					FString::Printf(TEXT("%s %s t%d p%d %s"), *P.Name, States[static_cast<int32>(P.State)], P.Body->GetDetailTier(), P.Priority, *P.Beat.ToString()), nullptr,
					FColor::Cyan, 0.0f, true, 1.0f);
			}
		}
	}
#endif
}

void AStreetNpcs::EndPlay(const EEndPlayReason::Type Reason)
{
	UStreetWorld* Registry = UStreetWorld::Get(this);
	for (FStreetPerson& P : People)
	{
		if (Registry && P.TalkUse != 0)
		{
			Registry->RemoveUse(P.TalkUse);
			P.TalkUse = 0;
		}
		if (Reason == EEndPlayReason::Destroyed && P.Body && !P.bAdopted)
		{
			P.Body->Destroy();
		}
	}
	People.Reset();
	Session = nullptr;
	Super::EndPlay(Reason);
}

// ------------------------------------------------------------------ checks

FString AStreetNpcs::Describe() const
{
	static const TCHAR* const States[] = {TEXT("in the pool"), TEXT("at"), TEXT("walking"), TEXT("away")};
	static const TCHAR* const Doings[] = {TEXT("work"), TEXT("arrive"), TEXT("leave"), TEXT("sit"), TEXT("wait"), TEXT("smoke"), TEXT("walk")};
	int32 Built = 0;
	for (const FStreetPerson& P : People)
	{
		Built += P.Body ? 1 : 0;
	}
	FString Out = FString::Printf(TEXT("People: %s%s, %d known, %d built, %d live of %d (the crowd's allowance %d%s)%s%s"), UTF8_TO_TCHAR(ss::net::TimeLabel(ScheduleMinutes()).c_str()),
		CVarHour.GetValueOnGameThread() >= 0.0f ? TEXT(" (ss.Npc.Hour)") : TEXT(""), People.Num(), Built, NumLive(),
		CVarBudget.GetValueOnGameThread() >= 0 ? CVarBudget.GetValueOnGameThread() : MaxLive, CrowdAllowance(), bAtFloor ? TEXT(", frame budget at its floor") : TEXT(""),
		CareerSession() && !ss::town::People().empty() ? TEXT(", core's schedule") : TEXT(", the fallback schedule"), IsReady() ? TEXT("") : TEXT(", NOT READY"));
	for (const FStreetPerson& P : People)
	{
		if (P.bNoBody && !P.bWanted)
		{
			continue;
		}
		const UStreetBodyAnim* Anim = P.Body ? P.Body->StreetBody() : nullptr;
		const FString Due = P.bWanted ? FString::Printf(TEXT(", due: %s at %s"), Doings[FMath::Clamp(static_cast<int32>(P.WantDoing), 0, 6)], P.WantStation.IsNone() ? TEXT("-") : *P.WantStation.ToString())
									  : FString();
		const FString Tier = P.Body ? FString::Printf(TEXT(", tier %d"), P.Body->GetDetailTier()) : (P.bNoBody ? FString(TEXT(", no body on the street")) : FString(TEXT(", not built")));
		const FString Motion = Anim && P.State != EStreetPersonState::Pooled && P.State != EStreetPersonState::Away ? FString::Printf(TEXT(", %s"), *Anim->DescribeMotion()) : FString();
		Out += FString::Printf(TEXT("\n  %s (%s, %s, %.0f cm): %s %s%s%s%s"), *P.Name, *P.Id.ToString(), P.Face.IsNone() ? TEXT("no face") : *P.Face.ToString(), P.Height,
			States[static_cast<int32>(P.State)], P.State == EStreetPersonState::Placed ? *P.Station.ToString() : TEXT(""), *Due, *Tier, *Motion);
	}
	return Out;
}

FString AStreetNpcs::TestList() const
{
	return Describe();
}

FString AStreetNpcs::TestSpawn(const FString& Id)
{
	FStreetPerson* P = Person(FName(*Id.ToLower()));
	if (!P)
	{
		return FString::Printf(TEXT("ss.Npc.Spawn: nobody called '%s' (ss.Npc.List)"), *Id);
	}
	if (!P->Body && !BuildBody(*P))
	{
		return FString::Printf(TEXT("ss.Npc.Spawn: %s can't be built (face %s)"), *P->Name, *P->Face.ToString());
	}
	const FName Station = !P->WantStation.IsNone() ? P->WantStation : P->Station;
	for (const FRosterRow& R : Roster)
	{
		if (P->Id == FName(R.Id) && R.Station && Place(*P, FName(R.Station)))
		{
			return FString::Printf(TEXT("%s at %s"), *P->Name, R.Station);
		}
	}
	if (!Station.IsNone() && Place(*P, Station))
	{
		return FString::Printf(TEXT("%s at %s"), *P->Name, *Station.ToString());
	}
	// No station: before the hero, facing them.
	AShortStackCharacter* H = Hero.Get();
	if (!H)
	{
		return TEXT("ss.Npc.Spawn: no hero to stand them before");
	}
	Show(*P, true);
	const FVector At = H->GetActorLocation() + H->GetActorForwardVector() * 200.0;
	const FRotator Facing(0.0f, static_cast<float>((H->GetActorLocation() - At).Rotation().Yaw), 0.0f);
	P->Body->SetActorLocationAndRotation(At, Facing.Quaternion(), false, nullptr, ETeleportType::TeleportPhysics);
	P->State = EStreetPersonState::Placed;
	P->Station = NAME_None;
	// Their feet (the hero's capsule is the same height as theirs, near enough): where a station's frame stands.
	P->StationAt = FTransform(Facing, At - FVector(0.0, 0.0, P->Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	P->SeatHeight = 0.0f;
	UpdateTalkUse(*P);
	return FString::Printf(TEXT("%s before the hero (no station registered)"), *P->Name);
}

FString AStreetNpcs::TestTalk(const FString& Id, const FString& Choice)
{
	ss::town::Talk T;
	const FName Who(*Id.ToLower());
	const bool bOk = Choice.IsEmpty() ? OpenTalk(Who, T) : AnswerTalk(Who, Choice, T);
	if (!bOk)
	{
		return FString::Printf(TEXT("ss.Npc.Talk: %s isn't out to talk to (ss.Npc.Spawn %s first)"), *Id, *Id);
	}
	FString Out = FString::Printf(TEXT("%s (%s): \"%s\""), *Utf8(T.Name), *Utf8(T.Label), *Utf8(T.Line));
	for (const ss::town::Choice& C : T.Choices)
	{
		Out += FString::Printf(TEXT("\n  [%s] %s"), *Utf8(C.Id), *Utf8(C.Label));
	}
	if (T.Choices.empty())
	{
		// No card here to close: the talk is over once this line is said.
		Release(Who);
	}
	return Out;
}

FString AStreetNpcs::TestBudget(int32 Live)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("ss.Npc.Budget")))
	{
		Var->Set(Live, ECVF_SetByConsole);
	}
	Arbitrate();
	return Describe();
}

FString AStreetNpcs::TestHour(float Hour)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("ss.Npc.Hour")))
	{
		Var->Set(Hour, ECVF_SetByConsole);
	}
	ReadSchedule();
	return Describe();
}

FString AStreetNpcs::TestBeat(const FString& Id, const FString& Beat)
{
	const FName B = Beat.IsEmpty() || Beat.Equals(TEXT("none"), ESearchCase::IgnoreCase) ? NAME_None : FName(*Beat.ToLower());
	if (Id.Equals(TEXT("hero"), ESearchCase::IgnoreCase))
	{
		AShortStackCharacter* H = Hero.Get();
		if (!H)
		{
			return TEXT("ss.Npc.Beat: no hero");
		}
		H->SetWork(B, H->GetActorTransform());
		return FString::Printf(TEXT("the hero: %s"), *B.ToString());
	}
	FStreetPerson* P = Person(FName(*Id.ToLower()));
	if (!P || !P->Body)
	{
		return FString::Printf(TEXT("ss.Npc.Beat: %s isn't built"), *Id);
	}
	PlayBeat(P->Id, B, 20.0f);
	return FString::Printf(TEXT("%s: %s for 20 s"), *P->Name, *B.ToString());
}
