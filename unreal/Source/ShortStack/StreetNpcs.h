#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include <memory>
#include <string>
#include <vector>

#include "StreetNpcs.generated.h"

class AShortStackCharacter;
class AStreetCrowd;
class AStreetStage;
class UStaticMeshComponent;

namespace ss
{
class Session;
namespace town
{
struct Talk;
struct TalkContext;
struct Placement;
} // namespace town
} // namespace ss

/** Where one of the block's people is now (AStreetNpcs). */
enum class EStreetPersonState : uint8
{
	/** In the pool: built or not, hidden under the set (the detail tier's Gone). */
	Pooled,
	/** At a station (spec 2.10), working, sitting or waiting. */
	Placed,
	/** Lent to the crowd, walking somewhere (spec 6.8's Named role: Dee's regulars coming and going). */
	Walking,
	/** Somewhere off the street the schedule says they are (in Dee's back room at her game): hidden. */
	Away,
};

/** One of the block's people the director pools (spec 6.1, 6.3): the staff, Dee's and Ruby's regulars, the attendant. */
USTRUCT()
struct FStreetPerson
{
	GENERATED_BODY()

	/** Their MetaHuman, built once (under black) and kept: hidden and moved, never rebuilt. */
	UPROPERTY()
	TObjectPtr<AShortStackCharacter> Body;
	/** Hand props in their hands now (spec 4.5's SM_NS_Prop_*), by the beat. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Props;

	/** town::People()'s id ("lonnie", "big-lou"), their name and label ("Lonnie", "Barber"), their venue ("barber"), the face they wear. */
	FName Id;
	FString Name;
	FString Label;
	FName Venue;
	FName Face;
	/** The name the world and the table know a world person by ("Big Lou": their clothes, CastLooks::PersonaFor); empty for staff. */
	FString WorldName;
	bool bWoman = false;
	/** Staff (a keeper, the clerk) rather than one of the world's people. */
	bool bStaff = false;
	/** The body was the game mode's (Benny, Inez, spawned before there was a director): the director only tiers and places it. */
	bool bAdopted = false;
	/** The director leaves where they stand alone (the Wash & Fold's attendant: the stage's spot and her own lines). */
	bool bFixed = false;
	/** Never a MetaHuman on the street (the cook behind the pass: a baked figure, spec 6.1; Dee, who stays at her table). */
	bool bNoBody = false;
	/** Their height (cm, 155-195; the body is fitted to it as it's built: CastLooks::HeightFor, PedestrianHeight). */
	float Height = 175.0f;

	EStreetPersonState State = EStreetPersonState::Pooled;
	/** What the schedule wants now (town::Who's placement): the venue and station (None: not on the street), what they do there (town::Doing), until when. */
	FName WantVenue;
	FName WantStation;
	uint8 WantDoing = 0;
	bool bWanted = false;
	double WantSince = 0.0;
	double WantUntil = 0.0;
	/** The station they're at, its transform (world, their feet) and seat height (0 standing). */
	FName Station;
	FTransform StationAt = FTransform::Identity;
	float SeatHeight = 0.0f;
	/** Held talking with the hero (Engage): turned to them, the hands still; let go after their last line if asked. */
	bool bEngaged = false;
	bool bReleaseAfterLine = false;
	/** The beat (spec 6.6) now and how long it holds, and one the game asked for (a sale rung up). */
	FName Beat;
	float BeatLeft = 0.0f;
	FName AskedBeat;
	float AskedLeft = 0.0f;
	FName PropsFor;
	/** The waitress serving someone at the counter: how long she's been at it (s), and how far along the counter from her
	 *  station she is (cm along its right; eased toward SlideWant at a walk). */
	float ServeFor = 0.0f;
	float Slide = 0.0f;
	float SlideWant = 0.0f;
	/** The Talk use on them (the registry's handle; 0 none). */
	int32 TalkUse = 0;
	/** Said once a shift: "Ray's late." (the relief kept from coming on by the hero watching the counter). */
	bool bLateSaid = false;
	/** Seen at their station since (real seconds; a shift change waits until nobody's looking), or till the next try at coming on. */
	float StuckFor = 0.0f;
	/** Placed but due off: till the next look at whether anyone sees them go (each look is a few traces: a few times a second). */
	float LookIn = 0.0f;
	/** The walk the crowd was last given for them (town::Doing Arrive or Leave) and the placement it was for (its Since): each walk once. */
	uint8 WalkedFor = 0xff;
	double WalkedSince = -1.0;
	/** Logged once: why they can't be placed (no station, no face). */
	bool bWhyLogged = false;
	int32 Priority = 4;
};

/** A line on its way to the HUD's subtitles (spec 7.4: 2 at most, the newest at the bottom). */
struct FStreetSubtitle
{
	FString Speaker;
	FString Line;
};

/**
 * The street's people director (spec 7.5.7): the block's cast at their stations by the hour, talking, working, sitting.
 *
 * It pools everyone by face (spec 6.1: the six staff on their own MetaHumans once built, borrowed faces until then, Augie
 * on ExtraH, Benny on ExtraB, Dee's regulars on their table MetaHumans) and builds them only while the screen is black (the
 * load's fade, which it holds until they're ready; a service's fade; sleep): every MetaHuman build hitches. Built, a person
 * is never rebuilt: off shift they're hidden under the set (the detail tier's Gone) and moved where they're next wanted.
 *
 * Who is where comes from core (town::Who on the career, people-core's schedule: spec 6.2's shifts, 6.3's regulars), re-read
 * every world minute (its placements are minutes long) or on a clock jump; with no career on the map (none given through
 * SetSession or bound with CastLooks::BindSession) a fallback with the same ids and stations stands in. Staff stand (or sit) at
 * their stations from the registry (UStreetWorld::Station: the street's store-counter, each shop's), appearing and leaving
 * only unseen; a station that isn't registered yet keeps its person in the pool. Dee's regulars walk up on her nights as the
 * crowd's Named walkers (town's Arrive at "washfold-door": Sal off the 14 from the Market St stop to a cigarette by the Wash &
 * Fold's door, Mei under an umbrella, Twitch running), are inside at "dee-table", and come out of her steel door near dawn
 * (Leave). Ruby's regulars take their stools and booths (Sal behind the paper, Big Lou at his plate, Mei on her phone).
 *
 * One budget for every MetaHuman on the map (spec 6.7): at most 10 live, 8 visible and 4 at Near, by priority (whoever is
 * talking to the hero, the staff in the hero's zone, the world's people, then the pedestrians, who are cut first when the
 * frame budget sits at its floor), through each character's one SetDetailTier; the crowd's share is its allowance
 * (AStreetCrowd::SetAllowance). The face registry (UCastFaceRegistry) is configured here at the load.
 *
 * Talk (spec 6.4): a Talk use on each person E can reach (the registry puts people first), the talk's context from core
 * (town::ContextFor, Open, Answer; Session::Heard), the person stopping, turning to the hero and holding their eyes while
 * they speak, subtitled (TakeSubtitle) and flapping (SetTalking). Beats (spec 6.6): the clerk rings up, leans with the
 * phone, wipes the counter; the barber cuts while the hero is in his chair; the waitress walks along the counter to whoever
 * sits at it, writes the order, pours, wipes, leans; their hand props on their hands. The hero's beats at the services are the
 * game mode's (ServiceBeatFor names them); the director holds the hero still in a barber's chair outside one, and puts the
 * props of whatever beat the hero is at (the fork, the mug) in their hands.
 *
 * The street's game mode spawns it (FindOrSpawn), calls Begin with the stage, the hero and the crowd, SetSession with the
 * career, and SetHour each frame; the crowd spawns one itself (bAutoSpawned) when the game mode hasn't by its first hour.
 */
UCLASS()
class SHORTSTACK_API AStreetNpcs : public AActor
{
	GENERATED_BODY()

public:
	AStreetNpcs();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** The director already in World, or null. */
	static AStreetNpcs* Find(const UWorld* World);
	/** The director in World, spawned if there's none yet (the game mode and the crowd both come here: one director). */
	static AStreetNpcs* FindOrSpawn(UWorld* World);

	/** The street's people on Stage, around Hero, alongside Crowd (its allowance and its Named walkers). Again starts over. */
	void Begin(AStreetStage* InStage, AShortStackCharacter* InHero, AStreetCrowd* InCrowd);
	/** The career (the schedule, the talk's context and what was heard). Null: the fallback schedule and lines. */
	void SetSession(ss::Session* InSession);
	/** The crowd, when it comes after Begin (the crowd finds the director and attaches itself). */
	void AttachCrowd(AStreetCrowd* InCrowd);
	/** The hour (world minutes), the daylight and the rain (as AStreetStage::RainStrength), and whether the game is paused. */
	void SetHour(double InWorldMinutes, float InDaylight, float InRain, bool bInPaused);

	/** Holds someone for talk: they stop, turn to the hero and hold their eyes (the talk card is up). */
	void Engage(FName PersonId);
	void Release(FName PersonId);
	/** Says Line (subtitled, the mouth flapping for Seconds; 0: as long as the line takes). */
	void Speak(FName PersonId, const FString& Line, float Seconds = 0.0f);
	/** The next subtitle for the HUD (the oldest first): who and what. False when there's none. */
	bool TakeSubtitle(FString& OutSpeaker, FString& OutLine);
	/**
	 * The name tag (spec 6.4) for the HUD's NameTag: the one of the block's people the hero has looked at, within 4 m, for
	 * 0.4 s: their name ("Sal"), the tag's second half (town::Tag: their bond's label, "Knows you", or their role, "Barber";
	 * "Stranger" for one the hero has no reason to know), over their head (world: project it for the HUD), and its fade 0..1.
	 * False when there's none (pedestrians never have one).
	 */
	bool NameTag(FString& OutName, FString& OutSub, FVector& OutAt, float& OutAlpha) const;

	/**
	 * E on someone (the game mode's Talk verb, spec 7.5.3): they're engaged and say their opening line (core's town::Open, heard
	 * by the session); Out is what the talk card shows. False when nobody by that id can talk now.
	 */
	bool OpenTalk(FName PersonId, ss::town::Talk& Out);
	/** A choice on the card: their answer, said and heard; an empty Out.Choices ends the talk (they're released after the line). */
	bool AnswerTalk(FName PersonId, const FString& ChoiceId, ss::town::Talk& Out);
	/** A beat the game asks of someone for Seconds ("ring" when a sale goes through at the counter). */
	void PlayBeat(FName PersonId, FName Beat, float Seconds);
	/** The hero's beat for a service (spec 6.6): "chair" at the barber, "treadmill"/"bag"/"weights" at the gym, "eat"/"coffee" at Ruby's. */
	static FName ServiceBeatFor(FName Venue, const FString& OfferId);
	/** A bark for one of the block's people (core's town::Bark: "bump", "recognize", "rain", "closing"); empty with nothing to say. */
	FString BarkFor(FName PersonId, const FString& Trigger) const;

	/** Everyone wanted on the street now is built (the load's fade may lift). */
	bool IsReady() const;
	/** The MetaHumans the director has out now (placed, walking, and the adopted ones shown): its share of the live cap. */
	int32 NumLive() const;
	/** The person by id, or null. */
	const FStreetPerson* FindPerson(FName PersonId) const;
	/** One paragraph: the hour, the budget, each person and where they are, their tier and motion (ss.Npc.List). */
	FString Describe() const;

	/** Every asset the director loads (the hand props), for the street's preload list. */
	static void AppendPreload(TArray<FString>& Paths);

	/** Spawned by the crowd because the game mode hadn't spawned one (it forwards the hour to it). */
	bool bAutoSpawned = false;

	// ------------------------------------------------------------ tests (the console's ss.Npc.*, and their twins for scripts)
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestList() const;
	/** ss.Npc.Spawn <id>: built now (black or not) and placed at their station, or in front of the hero when it has none. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestSpawn(const FString& Id);
	/** ss.Npc.Talk <id> [choice]: their opening line and choices, or their answer to a choice. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestTalk(const FString& Id, const FString& Choice);
	/** ss.Npc.Budget <n>: the live cap (-1: spec 6.7's 10). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestBudget(int32 Live);
	/** ss.Npc.Hour <h>: the schedule read at that hour of today (-1: the clock's). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestHour(float Hour);
	/** ss.Npc.Beat <id> <beat>: a beat on someone (or "hero") for 20 s. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestBeat(const FString& Id, const FString& Beat);

private:
	/** What a person's placement asks of the street now. */
	enum class EWant : uint8
	{
		/** Not on the street (off shift, gone home, not due). */
		Off,
		/** At a registered station: working, sitting, waiting (placed, and taken away, only unseen). */
		Station,
		/** Due at a station no stage has registered yet (its shop stage hasn't spawned): kept in the pool. */
		Pending,
		/** Inside Dee's back room at her game (town's "dee-table"): hidden. */
		Inside,
		/** Walking up to the Wash & Fold's door and on through it to Dee's (town's Arrive and Smoke at "washfold-door"). */
		WalkIn,
		/** Out of Dee's steel door and off the set (town's Leave from "washfold-door"). */
		WalkOut,
	};

	// The roster and the schedule.
	void BuildRoster();
	FStreetPerson* Person(FName Id);
	void ReadSchedule();
	std::vector<ss::town::Placement> FallbackWho(double World) const;
	/** Everyone's placement at World: core's town::Who on the career, else the fallback. */
	std::vector<ss::town::Placement> WhoAt(double World) const;
	double ScheduleMinutes() const;
	/** The career: the one SetSession gave, else the one the street's game mode bound (CastLooks::BindSession). */
	ss::Session* CareerSession() const;
	EWant WantOf(const FStreetPerson& P, FTransform* OutStation = nullptr, float* OutSeat = nullptr) const;
	/** How long Sal's cigarette by the Wash & Fold's door lasts from now (real seconds; 0: no cigarette first). */
	float SmokeSecondsFor(const FStreetPerson& P) const;
	/** The hero held still in a barber's chair outside a service (the game mode sets the services' beats). */
	void HeroServiceBeat();
	/** Who the hero is looking at (for the name tag), and how long. */
	void UpdateNameTag(float Dt);

	// Bodies.
	bool BuildBody(FStreetPerson& P);
	void Adopt();
	void DressProps(FStreetPerson& P, FName Beat);
	void ClearProps(FStreetPerson& P);

	// Placing.
	void UpdatePeople(float Dt);
	bool Place(FStreetPerson& P, FName StationId);
	void Pool(FStreetPerson& P);
	void LendToCrowd(FStreetPerson& P, uint8 Doing);
	void Show(FStreetPerson& P, bool bShow);
	void UpdateTalkUse(FStreetPerson& P);
	void UpdateBeat(FStreetPerson& P, float Dt);
	void UpdateEngaged(FStreetPerson& P, float Dt);
	void HeroProps();

	// The budget.
	void Arbitrate();
	int32 CrowdAllowance() const;

	// The camera.
	void UpdateView();
	bool CanAppear(const FVector& At) const;
	bool Seen(const FVector& At, float Height) const;
	void HoldFade(float Dt);

	// Helpers.
	FVector Offstage() const;
	AShortStackCharacter* GetHero() const;
	std::shared_ptr<const ss::town::TalkContext> ContextFor(const FStreetPerson& P) const;
	FString FallbackLine(const FStreetPerson& P, const FString& Moment) const;

	UPROPERTY(Transient)
	TObjectPtr<AStreetStage> Stage;
	UPROPERTY(Transient)
	TObjectPtr<AStreetCrowd> Crowd;
	UPROPERTY(Transient)
	TArray<FStreetPerson> People;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> HeroPropParts;
	TWeakObjectPtr<AShortStackCharacter> Hero;
	ss::Session* Session = nullptr;

	// The hour.
	bool bBegun = false;
	bool bHourKnown = false;
	double WorldMinutes = 0.0;
	float Daylight = 0.0f;
	float Rain = 0.0f;
	bool bPaused = false;
	/** The schedule's slot last read (30 world minutes each) and the clock then (a jump re-reads it). */
	int64 ReadSlot = -1;
	double ReadAt = -1.0;
	double LastSeenMinutes = -1.0;
	float ReadOverride = -2.0f;
	bool bLoggedFallback = false;
	/** Who the next hours want on the street (built ahead under black), read once per schedule read (ReadSlot), not every dark frame. */
	TArray<FName> AheadIds;
	int64 AheadSlot = -1;
	/** Said once a fade: the pool wasn't ready when the load's fade stopped waiting for it. */
	bool bFadeGaveUp = false;

	// The camera, as the crowd sees it.
	FVector ViewAt = FVector::ZeroVector;
	FVector ViewAxes[3] = {FVector::ForwardVector, FVector::RightVector, FVector::UpVector};
	double ViewTanX = 1.0;
	double ViewTanY = 0.6;
	bool bHasView = false;
	bool bDark = false;
	bool bFadingIn = false;
	float FadeHeld = 0.0f;

	// The budget.
	float ArbitrateIn = 0.0f;
	float FloorFor = 0.0f;
	bool bAtFloor = false;
	FName HeroZone;

	TArray<FStreetSubtitle> Subtitles;
	/** Talk contexts kept for the talk's answers, by person (core's opaque context). */
	TMap<FName, std::shared_ptr<const ss::town::TalkContext>> Talks;
	FName HeroPropsFor;
	/** The beat the director put the hero at (HeroServiceBeat), taken off again when the service ends. */
	FName HeroBeatSet;
	/** The name tag: who's looked at now and for how long, who has the tag (fading in, or out after the look moves on). */
	FName LookedAt;
	float LookedFor = 0.0f;
	FName TagWho;
	float TagAlpha = 0.0f;
	float Clock = 0.0f;
	FRandomStream Dice;
};
