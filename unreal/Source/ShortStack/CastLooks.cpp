#include "CastLooks.h"

#include "BackRoomPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HeroGrooms.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"
#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Town.h"
#include "ShortStackCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogCastLooks, Log, All);

namespace CastLooksDetail
{
/** A person of the block whose face is kept (spec 6.1): their own build once made, a face kept for good, or a borrowed one. */
struct FPerson
{
	const TCHAR* Person;
	/** Their own MetaHuman (MHC_<Own>), worn once it's built. */
	const TCHAR* Own;
	/** A face kept for them for good, worn while their own isn't built (or always, when they have none). */
	const TCHAR* Kept;
	bool bWoman;
	/** The extra they borrow first while their own is pending (see CastLooks.h). */
	const TCHAR* Prefer;
};

const FPerson People[] = {
	{TEXT("benny"), nullptr, TEXT("ExtraB"), false, nullptr},
	{TEXT("inez"), nullptr, TEXT("ExtraK"), true, nullptr},
	{TEXT("augie"), TEXT("Augie"), TEXT("ExtraH"), false, nullptr},
	{TEXT("ray"), TEXT("Ray"), nullptr, false, nullptr},
	{TEXT("lonnie"), TEXT("Lonnie"), nullptr, false, nullptr},
	{TEXT("rosa"), TEXT("Rosa"), nullptr, true, TEXT("ExtraI")},
	{TEXT("ruby"), TEXT("Ruby"), nullptr, true, TEXT("ExtraC")},
	{TEXT("marlene"), TEXT("Marlene"), nullptr, true, TEXT("ExtraG")},
	{TEXT("bea"), TEXT("Bea"), nullptr, true, TEXT("ExtraA")},
};

const FName HeroWho(TEXT("hero"));

FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

/** The Back Room's colors: an sRGB hex as the persona stores it (FColor straight, as PersonaFor always has). */
FLinearColor Persona8(uint32 Hex)
{
	return FLinearColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

FBackRoomTell Tell(EBackRoomTell T, EBackRoomTellMeaning Means, float Reliability, float FalseRate)
{
	FBackRoomTell Out;
	Out.Tell = T;
	Out.Means = Means;
	Out.Reliability = Reliability;
	Out.FalseRate = FalseRate;
	return Out;
}

/** A persona with glasses (1), shades (2) or aviators (3) in a frame color. */
FBackRoomPersona Eyed(FBackRoomPersona P, int32 Eyewear, uint32 FrameSrgb)
{
	P.Eyewear = Eyewear;
	P.FrameColor = Persona8(FrameSrgb);
	return P;
}

/** The room's bodies by whom they suit, in BackRoomLive.cpp's order (ExtraA, C, E, G, I, K are women's). */
const TCHAR* WomenBodies[6] = {TEXT("ExtraA"), TEXT("ExtraC"), TEXT("ExtraE"), TEXT("ExtraG"), TEXT("ExtraI"), TEXT("ExtraK")};
const TCHAR* MenBodies[6] = {TEXT("ExtraB"), TEXT("ExtraD"), TEXT("ExtraF"), TEXT("ExtraH"), TEXT("ExtraJ"), TEXT("ExtraL")};
const TCHAR* ExtraBodies[12] = {TEXT("ExtraA"), TEXT("ExtraB"), TEXT("ExtraC"), TEXT("ExtraD"), TEXT("ExtraE"), TEXT("ExtraF"), TEXT("ExtraG"), TEXT("ExtraH"),
	TEXT("ExtraI"), TEXT("ExtraJ"), TEXT("ExtraK"), TEXT("ExtraL")};

// The shirts' prints and chest graphics (art/blender/prints.py's masks), by the persona's numbering (0: none).
const TCHAR* const Prints[] = {nullptr, TEXT("T_Print_Stripes"), TEXT("T_Print_Breton"), TEXT("T_Print_Ringer"), TEXT("T_Print_Tartan"), TEXT("T_Print_Gingham"),
	TEXT("T_Print_Dots"), TEXT("T_Print_Camo")};
// How many repeats across the shirt's layout each print reads best at (as the Back Room's).
const float PrintRepeats[] = {1.0f, 7.0f, 9.0f, 5.0f, 3.0f, 7.0f, 9.0f, 2.0f};
// 7: the thrift's rare "RiverLine Sunday Major 2019" tee (prints.py's T_Graphic_SundayMajor).
const TCHAR* const Graphics[] = {nullptr, TEXT("T_Graphic_Embercrest"), TEXT("T_Graphic_AllIn"), TEXT("T_Graphic_Chip"), TEXT("T_Graphic_BadBeat"),
	TEXT("T_Graphic_Sunset"), TEXT("T_Graphic_Number"), TEXT("T_Graphic_SundayMajor")};

/** The block's staff at work (spec 6.1) and the block's others, as the street dresses them. */
struct FStaffWear
{
	const TCHAR* Person;
	uint32 Shirt;
	int32 Print;
	uint32 InkA;
	uint32 InkB;
	uint32 Pants;
	ss::wardrobe::Cut Eyes;
	uint32 Frame;
	bool bWorkShoes;
	uint32 Shoes;
	float Height;
	/** A jacket over the shirt (Count: none) in its color, and trousers (Jeans, Chinos; Count: the shorts in Pants' tone). */
	ss::wardrobe::Cut Outer;
	uint32 OuterColor;
	ss::wardrobe::Cut Bottom;
	/** The body's breadth (FCastWear::Breadth): 1 as built. */
	float Breadth;
};
using ss::wardrobe::Cut;
const FStaffWear Staff[] = {
	// The Lucky Penny's red, nights and days: Benny's over jeans, Ray's polo over khakis.
	{TEXT("benny"), 0xd7263d, 0, 0, 0, 0x2a2b30, Cut::Count, 0, true, 0, 183.0f, Cut::Count, 0, Cut::Jeans, 1.0f},
	{TEXT("ray"), 0xb0202f, 0, 0, 0, 0x8b7d5b, Cut::Count, 0, true, 0, 178.0f, Cut::Count, 0, Cut::Chinos, 1.04f},
	// A white barber's smock (the tie's under it), charcoal trousers.
	{TEXT("lonnie"), 0xeceae4, 0, 0, 0, 0x2b2c31, Cut::Count, 0, true, 0, 175.0f, Cut::Count, 0, Cut::Chinos, 1.0f},
	// A heather cardigan, round reading glasses on, grey slacks; slight.
	{TEXT("bea"), 0x8a6f7d, 0, 0, 0, 0x4a4a52, Cut::Round, 0x3b2314, true, 0, 158.0f, Cut::Count, 0, Cut::Chinos, 0.95f},
	// The trainer: a teal track jacket over a ringer with white bands, black trousers, sneakers.
	{TEXT("rosa"), 0x1f6f6a, 3, 0xf2efe6, 0x14161c, 0x161618, Cut::Count, 0, false, 0xece7db, 168.0f, Cut::Track, 0x1f6f6a, Cut::Chinos, 1.0f},
	// Ruby: a burgundy cardigan, dark slacks. Her build is on Lani's preset body, as Inez's face is (staff_cast.py): taller and
	// fuller than Inez, so the two never read as one woman.
	{TEXT("ruby"), 0x6a1f2e, 0, 0, 0, 0x2a2b30, Cut::Count, 0, true, 0, 170.0f, Cut::Count, 0, Cut::Chinos, 1.07f},
	// Marlene, nights: the pink uniform, the top and the trousers in one pink (the apron's the pale band of it), white shoes.
	{TEXT("marlene"), 0xe7a1b0, 0, 0, 0, 0xd99aa9, Cut::Count, 0, false, 0xece7db, 163.0f, Cut::Count, 0, Cut::Chinos, 1.0f},
	// Augie: a short-sleeved white shirt, gold wire readers, grey trousers.
	{TEXT("augie"), 0xe8e6df, 0, 0, 0, 0x3b3d43, Cut::Wire, 0xc9a24d, true, 0, 172.0f, Cut::Count, 0, Cut::Chinos, 1.03f},
	// Inez at the Wash & Fold: her smock (The Block's), slacks; small and slight beside Ruby.
	{TEXT("inez"), 0x5f8fa6, 0, 0, 0, 0x2a2b30, Cut::Count, 0, true, 0, 157.0f, Cut::Count, 0, Cut::Chinos, 0.95f},
};

const FStaffWear* StaffFor(FName Person)
{
	for (const FStaffWear& S : Staff)
	{
		if (Person == FName(S.Person))
		{
			return &S;
		}
	}
	return nullptr;
}

/** The block's people by the names the world gives them (an entrant at the Embercrest's tables, a name tag): their id. */
FName BlockPersonByName(const FString& Name)
{
	static const TPair<const TCHAR*, const TCHAR*> Names[] = {{TEXT("Lonnie"), TEXT("lonnie")}, {TEXT("Lonnie Pruitt"), TEXT("lonnie")}, {TEXT("Augie"), TEXT("augie")},
		{TEXT("Augie Szabo"), TEXT("augie")}, {TEXT("August Szabo"), TEXT("augie")}, {TEXT("Ray Molina"), TEXT("ray")}, {TEXT("Rosa Delgado"), TEXT("rosa")},
		{TEXT("Ruby Okafor"), TEXT("ruby")}, {TEXT("Bea Kowalski"), TEXT("bea")}, {TEXT("Marlene"), TEXT("marlene")}, {TEXT("Benny"), TEXT("benny")}};
	for (const TPair<const TCHAR*, const TCHAR*>& N : Names)
	{
		if (Name.Equals(N.Key, ESearchCase::IgnoreCase))
		{
			return FName(N.Value);
		}
	}
	return NAME_None;
}

/** The sessions the maps play, by world (BindSession). */
TMap<TWeakObjectPtr<UWorld>, ss::Session*>& Sessions()
{
	static TMap<TWeakObjectPtr<UWorld>, ss::Session*> Map;
	return Map;
}

/** The hero's look on maps that have only a save (BindHeroLook). */
TMap<TWeakObjectPtr<UWorld>, ss::hero::Look>& HeroLooks()
{
	static TMap<TWeakObjectPtr<UWorld>, ss::hero::Look> Map;
	return Map;
}

UWorld* WorldOf(const UObject* Context)
{
	return Context && GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
}

AShortStackCharacter* HeroIn(UWorld* World)
{
	APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}
} // namespace CastLooksDetail

using namespace CastLooksDetail;

namespace CastLooks
{
bool IsBuilt(const FString& Name)
{
	return !Name.IsEmpty() && FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/BP_MHC_%s"), *Name, *Name));
}

bool IsWomansFace(FName Face)
{
	static const TSet<FName> Women = {FName(TEXT("ExtraA")), FName(TEXT("ExtraC")), FName(TEXT("ExtraE")), FName(TEXT("ExtraG")), FName(TEXT("ExtraI")), FName(TEXT("ExtraK")),
		FName(TEXT("HeroB0")), FName(TEXT("HeroB1")), FName(TEXT("HeroB2")), FName(TEXT("Bea")), FName(TEXT("Rosa")), FName(TEXT("Ruby")), FName(TEXT("Marlene")),
		FName(TEXT("Dee")), FName(TEXT("Mei")), FName(TEXT("MrsPark"))};
	return Women.Contains(Face);
}

const TArray<FName>& Extras()
{
	static const TArray<FName> List = {FName(TEXT("ExtraA")), FName(TEXT("ExtraB")), FName(TEXT("ExtraC")), FName(TEXT("ExtraD")), FName(TEXT("ExtraE")), FName(TEXT("ExtraF")),
		FName(TEXT("ExtraG")), FName(TEXT("ExtraH")), FName(TEXT("ExtraI")), FName(TEXT("ExtraJ")), FName(TEXT("ExtraK")), FName(TEXT("ExtraL"))};
	return List;
}

const TArray<FName>& EmbercrestFaces()
{
	static const TArray<FName> List = {FName(TEXT("ExtraJ")), FName(TEXT("ExtraD")), FName(TEXT("ExtraL")), FName(TEXT("ExtraE")), FName(TEXT("ExtraF"))};
	return List;
}

FName HeroFaceFor(const ss::hero::Look& Look)
{
	const int32 Band = Look.Skin <= 2 ? 0 : (Look.Skin <= 5 ? 1 : 2);
	return FName(*FString::Printf(TEXT("Hero%c%d"), Look.Body == 1 ? TEXT('B') : TEXT('A'), Band));
}

FString CareerKey(const ss::hero::Character& Person)
{
	return FString(UTF8_TO_TCHAR(Person.FullName().c_str()));
}

TArray<FBlockFace> BlockFaces(FName PlayerHero, const FString& CareerKey)
{
	TArray<FBlockFace> Out;
	TSet<FName> Taken;
	if (!PlayerHero.IsNone())
	{
		Taken.Add(PlayerHero);
	}
	// Own builds and kept faces first, so the borrowers can't take them.
	TArray<int32> Pending;
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(People)); ++I)
	{
		const FPerson& P = People[I];
		FBlockFace& F = Out.AddDefaulted_GetRef();
		F.Person = FName(P.Person);
		F.bWoman = P.bWoman;
		if (P.Own && IsBuilt(P.Own))
		{
			F.Face = FName(P.Own);
			F.bOwnBuild = true;
		}
		else if (P.Kept)
		{
			F.Face = FName(P.Kept);
		}
		else
		{
			Pending.Add(I);
			continue;
		}
		Taken.Add(F.Face);
	}
	auto Free = [&Taken](FName Face) { return !Taken.Contains(Face) && IsBuilt(Face.ToString()); };
	for (const int32 I : Pending)
	{
		const FPerson& P = People[I];
		FBlockFace& F = Out[I];
		F.bBorrowed = true;
		// 1. Their preferred extra, then any extra of their body the Embercrest's doors don't use.
		TArray<FName> Try;
		if (P.Prefer)
		{
			Try.Add(FName(P.Prefer));
		}
		for (const FName Extra : Extras())
		{
			if (IsWomansFace(Extra) == P.bWoman && !EmbercrestFaces().Contains(Extra))
			{
				Try.AddUnique(Extra);
			}
		}
		for (const FName Face : Try)
		{
			if (Free(Face))
			{
				F.Face = Face;
				break;
			}
		}
		// 2. The extras run short: a spare hero of their body, the same one all career.
		if (F.Face.IsNone())
		{
			const uint32 Hash = ss::live::FaceHash(std::string(TCHAR_TO_UTF8(*(CareerKey + TEXT("/") + P.Person))));
			for (int32 K = 0; K < 3 && F.Face.IsNone(); ++K)
			{
				const FName Hero(*FString::Printf(TEXT("Hero%c%d"), P.bWoman ? TEXT('B') : TEXT('A'), static_cast<int32>((Hash + K) % 3)));
				if (Free(Hero))
				{
					F.Face = Hero;
				}
			}
		}
		// 3. Failing that, one of the Embercrest's five.
		if (F.Face.IsNone())
		{
			for (const FName Extra : EmbercrestFaces())
			{
				if (IsWomansFace(Extra) == P.bWoman && Free(Extra))
				{
					F.Face = Extra;
					break;
				}
			}
		}
		if (!F.Face.IsNone())
		{
			Taken.Add(F.Face);
		}
	}
	return Out;
}

TArray<FName> ReservedFaces(FName PlayerHero, const FString& CareerKey)
{
	TArray<FName> Out;
	if (!PlayerHero.IsNone())
	{
		Out.Add(PlayerHero);
	}
	for (const FBlockFace& F : BlockFaces(PlayerHero, CareerKey))
	{
		if (!F.Face.IsNone())
		{
			Out.AddUnique(F.Face);
		}
	}
	return Out;
}

FName FreeFaceLike(FName Face, const TArray<FName>& Reserved, const TArray<FName>& Avoid)
{
	if (!Reserved.Contains(Face))
	{
		return Face;
	}
	const bool bWoman = IsWomansFace(Face);
	for (const FName Extra : Extras())
	{
		if (IsWomansFace(Extra) == bWoman && !Reserved.Contains(Extra) && !Avoid.Contains(Extra) && IsBuilt(Extra.ToString()))
		{
			return Extra;
		}
	}
	return NAME_None;
}

// ------------------------------------------------------------------ the Back Room's people

FBackRoomPersona PersonaFor(const FString& Name)
{
	using T = EBackRoomTell;
	using M = EBackRoomTellMeaning;
	auto Persona = [](const TCHAR* InName, float Nerves, float Expressive, float Restless, float Fidget, float Posture, float Chatter, int32 Seed,
		TArray<FBackRoomTell> Tells, uint32 ShirtSrgb, float HeroRead) {
		FBackRoomPersona P;
		P.Shirt = Persona8(ShirtSrgb);
		P.Name = InName;
		P.Nervousness = Nerves;
		P.Expressiveness = Expressive;
		P.Restlessness = Restless;
		P.ChipFidget = Fidget;
		P.Posture = Posture;
		P.Chatter = Chatter;
		P.Seed = Seed;
		P.Tells = MoveTemp(Tells);
		P.HeroRead = HeroRead;
		return P;
	};
	// The regulars, what gives them away, and how they read you (HeroRead: above 0 they read your shaking
	// hands right, below 0 they think nerves mean a bluff).
	if (Name == TEXT("Dee"))
	{
		return Persona(TEXT("Dee"), 0.1f, 0.3f, 0.3f, 0.0f, 0.6f, 0.2f, 37, {}, 0x1f2a3d, 0.0f);
	}
	if (Name == TEXT("You"))
	{
		return Persona(TEXT("You"), 0.3f, 0.5f, 0.5f, 0.3f, 0.5f, 0.0f, 7, {}, 0x3c4a44, 0.0f);
	}
	if (Name == TEXT("Sal"))
	{
		// Old-timer, plays tight. Honest tells: he glances at his chips when he connects, and a bluff
		// makes him rub his neck. Forty years of watching hands: he knows a shake.
		// Reading glasses he pushes up to look at the board.
		return Eyed(Persona(TEXT("Sal"), 0.2f, 0.35f, 0.4f, 0.6f, 0.3f, 0.35f, 11,
			{Tell(T::ChipGlance, M::Strong, 0.85f, 0.05f), Tell(T::Recheck, M::Weak, 0.55f, 0.08f), Tell(T::NeckTouch, M::Bluff, 0.75f, 0.08f)}, 0x6b6a4e, 0.6f), 1, 0x3b2314);
	}
	if (Name == TEXT("Big Lou"))
	{
		// Loud, calls everything. His smile is real when he has it; when he bluffs it's mouth only.
		// Sees you shake and calls, every time.
		return Persona(TEXT("Big Lou"), 0.25f, 0.75f, 0.35f, 0.5f, 0.2f, 0.8f, 53,
			{Tell(T::RealSmile, M::Strong, 0.8f, 0.1f), Tell(T::FalseSmile, M::Bluff, 0.75f, 0.08f), Tell(T::ChipReach, M::Weak, 0.6f, 0.12f)}, 0x1c1c1e, -0.6f);
	}
	if (Name == TEXT("Twitch"))
	{
		// Wired, bets too much. Shakes when he has it; bluffing, he stares you down and swallows.
		// Shades indoors, like the streamers he watches.
		return Eyed(Persona(TEXT("Twitch"), 0.85f, 0.7f, 0.9f, 0.9f, 0.7f, 0.5f, 41,
			{Tell(T::Tremble, M::Strong, 0.85f, 0.12f), Tell(T::StareDown, M::Bluff, 0.75f, 0.12f), Tell(T::Swallow, M::Bluff, 0.6f, 0.1f),
				Tell(T::BlinkBurst, M::Bluff, 0.45f, 0.15f)}, 0x8c2a24, -0.5f), 2, 0x101010);
	}
	if (Name == TEXT("Mei"))
	{
		// Quiet and sharp. She acts: a sigh and a look away mean she's strong. Her pupils don't act. She
		// reads yours best of all.
		return Persona(TEXT("Mei"), 0.15f, 0.25f, 0.3f, 0.3f, 0.55f, 0.15f, 23,
			{Tell(T::Sigh, M::Strong, 0.65f, 0.08f), Tell(T::LookAway, M::Strong, 0.5f, 0.15f), Tell(T::PupilFlare, M::Strong, 0.6f, 0.05f),
				Tell(T::LipPress, M::Bluff, 0.45f, 0.12f)}, 0x34383d, 0.9f);
	}
	// The Embercrest's Sunday faces.
	if (Name == TEXT("Mrs. Park"))
	{
		// Thirty years of the Sunday tournament. Folds and folds; when she finally bluffs she goes
		// rigid, and a real hand pulls her eyes down to her chips. Nothing gets past her.
		return Eyed(Persona(TEXT("Mrs. Park"), 0.15f, 0.2f, 0.25f, 0.4f, 0.75f, 0.25f, 61,
			{Tell(T::Freeze, M::Bluff, 0.85f, 0.05f), Tell(T::ChipGlance, M::Strong, 0.7f, 0.05f), Tell(T::Sigh, M::Weak, 0.5f, 0.1f)}, 0x5a3e57, 0.7f), 1, 0x5a1f2a);
	}
	if (Name == TEXT("Rick"))
	{
		// Car-dealership money, loud and loose. Bluffing he stares you down and rubs his neck; with a
		// hand he goes quiet and looks off at the room.
		// Aviators, gold, from the car showroom's floor.
		return Eyed(Persona(TEXT("Rick"), 0.35f, 0.7f, 0.6f, 0.7f, 0.25f, 0.75f, 67,
			{Tell(T::StareDown, M::Bluff, 0.8f, 0.15f), Tell(T::NeckTouch, M::Bluff, 0.55f, 0.1f), Tell(T::LookAway, M::Strong, 0.6f, 0.1f)}, 0x2b4d7a, -0.3f), 3, 0xc9a24a);
	}
	if (Name == TEXT("Dre"))
	{
		// Talks the whole time. Can't hide a smile with a hand; swallows when he's pushing air, and
		// reaches for chips when he wants you to check.
		return Persona(TEXT("Dre"), 0.3f, 0.85f, 0.55f, 0.6f, 0.45f, 0.9f, 71,
			{Tell(T::RealSmile, M::Strong, 0.75f, 0.15f), Tell(T::Swallow, M::Bluff, 0.6f, 0.1f), Tell(T::ChipReach, M::Weak, 0.55f, 0.1f)}, 0xc28a2e, -0.2f);
	}
	if (Name == TEXT("gh0stfold"))
	{
		// The rival from RiverLine, in a hoodie. Almost nothing leaks: the pupils, close up, when he has
		// it, and a burst of blinks after a bluff. Reads you better than anyone.
		return Persona(TEXT("gh0stfold"), 0.1f, 0.12f, 0.3f, 0.5f, 0.6f, 0.3f, 83,
			{Tell(T::PupilFlare, M::Strong, 0.45f, 0.02f), Tell(T::BlinkBurst, M::Bluff, 0.35f, 0.05f)}, 0x232326, 1.0f);
	}
	// Anyone else: a stranger with a couple of the common tells.
	const int32 Seed = static_cast<int32>(GetTypeHash(Name) & 0x7fffffff);
	FRandomStream R(Seed);
	const T Pool[] = {T::ChipGlance, T::BrowFlash, T::Swallow, T::LipPress, T::LookAway, T::NeckTouch, T::Recheck, T::Sigh, T::ChipReach, T::RealSmile};
	const M Means[] = {M::Strong, M::Weak, M::Bluff};
	TArray<FBackRoomTell> Tells;
	const T A = Pool[R.RandRange(0, 9)];
	T B = Pool[R.RandRange(0, 9)];
	B = B == A ? Pool[(static_cast<int32>(A) + 3) % 10] : B;
	Tells.Add(Tell(A, Means[R.RandRange(0, 2)], 0.6f, 0.12f));
	Tells.Add(Tell(B, Means[R.RandRange(0, 2)], 0.5f, 0.12f));
	static const uint32 Shirts[] = {0x2f3b52, 0x6e2b2b, 0x3a5a40, 0x8a7a5a, 0x222222, 0x5a4a6e, 0x9a9a9a, 0x7a4a2a};
	FBackRoomPersona P = Persona(*Name, R.FRandRange(0.15f, 0.6f), R.FRandRange(0.3f, 0.7f), R.FRandRange(0.3f, 0.7f), R.FRandRange(0.2f, 0.7f), R.FRandRange(0.3f, 0.7f),
		0.0f, Seed % 997 + 101, MoveTemp(Tells), Shirts[R.RandRange(0, 7)], 0.0f);
	// What they wear, the same every night: a print or a graphic on about half the shirts, trousers, now and then a
	// hat (over short hair), glasses or shades.
	static const uint32 Tones[] = {0x1f2a44, 0x2f3b52, 0x6e2b2b, 0x3a5a40, 0x8a7a5a, 0x222222, 0x5a4a6e, 0x9a9a9a, 0x7a4a2a, 0xd8d4c8, 0x7b1e1e,
		0x1e4d6b, 0xc9a227, 0x4a5d23, 0x30343c, 0xb85c38, 0xe8e6df, 0x2b2b30};
	constexpr int32 NumTones = UE_ARRAY_COUNT(Tones);
	P.Shirt = Persona8(Tones[R.RandRange(0, NumTones - 1)]);
	const float Look = R.FRand();
	if (Look < 0.36f)
	{
		P.ShirtPrint = R.RandRange(1, 7);
	}
	else if (Look < 0.56f)
	{
		P.ShirtGraphic = R.RandRange(1, 6);
	}
	// The print's colors against the shirt: light on dark, dark on light.
	const bool bDark = P.Shirt.GetLuminance() < 0.25f;
	static const uint32 Lights[] = {0xf2efe6, 0xe8d9a8, 0xd9e2ea, 0xf0c9a0, 0xc8d8b8};
	static const uint32 Darks[] = {0x14161c, 0x2a1f1a, 0x1c2a40, 0x3a1414, 0x223322};
	P.ShirtB = Persona8(bDark ? Lights[R.RandRange(0, 4)] : Darks[R.RandRange(0, 4)]);
	P.ShirtC = Persona8(Tones[R.RandRange(0, NumTones - 1)]);
	static const uint32 Trousers[] = {0x2a3a5c, 0x243150, 0x161616, 0x8b7d5b, 0x4a4a4e, 0x1c2236, 0x5a4632};
	P.Pants = Persona8(Trousers[R.RandRange(0, 6)]);
	const float Head = R.FRand();
	P.Headwear = Head < 0.16f ? 1 : Head < 0.23f ? 2 : Head < 0.27f ? 3 : 0;
	const float Eye = R.FRand();
	P.Eyewear = Eye < 0.13f ? 1 : Eye < 0.20f ? 2 : Eye < 0.24f ? 3 : 0;
	// Muted: under the table lamps a hat's crown takes the most light of anything in the room.
	static const uint32 Hats[] = {0x1c2236, 0x141414, 0x5e1a1a, 0x6b5f45, 0x26392a, 0x2f3338, 0x4a3426, 0x1f3a3d};
	P.WearColor = Persona8(Hats[R.RandRange(0, 7)]);
	static const uint32 Frames[] = {0x111111, 0x3b2314, 0x2a2d33, 0x1c2a4a, 0x4a1a22};
	P.FrameColor = Persona8(Frames[R.RandRange(0, 4)]);
	return P;
}

FString CastAssetFor(const FString& Name)
{
	if (Name == TEXT("Big Lou"))
	{
		return TEXT("BigLou");
	}
	if (Name == TEXT("Mrs. Park"))
	{
		return TEXT("MrsPark");
	}
	if (Name == TEXT("gh0stfold"))
	{
		return TEXT("Ghost");
	}
	if (Name == TEXT("You"))
	{
		return TEXT("Hero");
	}
	for (const TCHAR* Known : {TEXT("Dee"), TEXT("Sal"), TEXT("Twitch"), TEXT("Mei"), TEXT("Rick"), TEXT("Dre")})
	{
		if (Name == Known)
		{
			return Known;
		}
	}
	return FString();
}

FString DefaultBody(const FString& Id, const FString& Name)
{
	const FString Own = CastAssetFor(Name);
	if (!Own.IsEmpty())
	{
		return Own;
	}
	// The block's people at the room's tables (Lonnie plays the Monday Nightly): their own build, or the face kept for them.
	const FName Person = BlockPersonByName(Name);
	if (!Person.IsNone())
	{
		for (const FPerson& P : People)
		{
			if (Person == FName(P.Person))
			{
				if (P.Own && IsBuilt(P.Own))
				{
					return P.Own;
				}
				if (P.Kept)
				{
					return P.Kept;
				}
			}
		}
	}
	const uint32 Hash = FCrc::StrCrc32(*Id);
	const ss::live::RoomLocal* Local = ss::live::FindRoomLocal(std::string(TCHAR_TO_UTF8(*Name)));
	return Local ? (Local->Woman ? WomenBodies : MenBodies)[Hash % 6] : ExtraBodies[Hash % 12];
}

// ------------------------------------------------------------------ wardrobes

const TCHAR* PrintTexture(int32 Index)
{
	return Index > 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(Prints)) ? Prints[Index] : nullptr;
}

float PrintTiling(int32 Index)
{
	return Index > 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(PrintRepeats)) ? PrintRepeats[Index] : 1.0f;
}

const TCHAR* GraphicTexture(int32 Index)
{
	return Index > 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(Graphics)) ? Graphics[Index] : nullptr;
}

int32 PrintCount()
{
	return UE_ARRAY_COUNT(Prints) - 1;
}

int32 GraphicCount()
{
	return UE_ARRAY_COUNT(Graphics) - 1;
}

FCastWear WearOf(const FBackRoomPersona& P)
{
	FCastWear W;
	W.Shirt = P.Shirt;
	W.Print = P.ShirtPrint;
	W.Graphic = P.ShirtGraphic;
	W.InkA = P.ShirtB;
	W.InkB = P.ShirtC;
	W.InkC = P.ShirtD;
	W.Pants = P.Pants;
	// The table's pieces, as the street's one set has them: a cap, a beanie, a trilby; glasses, shades, aviators.
	W.Hat = P.Headwear == 1 ? Cut::BallCap : P.Headwear == 2 ? Cut::Beanie : P.Headwear == 3 ? Cut::Trilby : Cut::Count;
	W.Eyes = P.Eyewear == 1 ? Cut::Square : P.Eyewear == 2 ? Cut::Shades : P.Eyewear == 3 ? Cut::Aviators : Cut::Count;
	W.HatColor = P.WearColor;
	W.FrameColor = P.FrameColor;
	// Canvas sneakers off the street, in white or black by who they are (the table never sees their feet).
	const uint32 Hash = FCrc::StrCrc32(*P.Name.ToLower());
	W.bWorkShoes = false;
	W.ShoeColor = Srgb((Hash & 1) ? 0xd6d0c0 : 0x1d1e22);
	// Below the table: trousers, the same every day (jeans keep their indigo, chinos take the persona's trousers' tone), worn
	// wherever a pair is fitted to their body (garments.py: SK_Garment_<Kind>_<Face>); the shorts in that tone until then.
	W.Bottom = ((Hash >> 1) & 1) ? Cut::Jeans : Cut::Chinos;
	const bool bKnown = !CastAssetFor(P.Name).IsEmpty() || !BlockPersonByName(P.Name).IsNone();
	if (P.Name == TEXT("gh0stfold"))
	{
		// The rival from RiverLine, in his hoodie (as the table describes him).
		W.Outer = Cut::Hoodie;
		W.OuterColor = P.Shirt;
	}
	else if (!bKnown)
	{
		// A stranger: about two in five in a jacket over the shirt (a hoodie or a bomber, in one of the street's tones). The known
		// faces keep their table look outside, so the player knows Sal on the street.
		static const uint32 Jackets[] = {0x1f2a44, 0x2b2c31, 0x3a3128, 0x4a5d23, 0x5a1f22, 0x1d1d21, 0x6b5f45, 0x30343c, 0x1e4d6b, 0x7a4a2a};
		const uint32 Pick = (Hash >> 2) % 100;
		if (Pick < 40)
		{
			W.Outer = Pick < 20 ? Cut::Hoodie : Cut::Bomber;
			W.OuterColor = Srgb(Jackets[(Hash >> 9) % UE_ARRAY_COUNT(Jackets)]);
		}
	}
	return W;
}

FCastWear WearFor(FName Person)
{
	const FStaffWear* S = StaffFor(Person);
	if (!S)
	{
		// Anyone else: as a stranger by that id dresses at the table.
		return WearOf(PersonaFor(Person.ToString()));
	}
	FCastWear W;
	W.Shirt = Srgb(S->Shirt);
	W.Print = S->Print;
	W.InkA = Srgb(S->InkA);
	W.InkB = Srgb(S->InkB);
	W.Pants = Srgb(S->Pants);
	W.Outer = S->Outer;
	W.OuterColor = Srgb(S->OuterColor);
	W.Bottom = S->Bottom;
	W.Eyes = S->Eyes;
	W.FrameColor = Srgb(S->Frame);
	W.bWorkShoes = S->bWorkShoes;
	W.ShoeColor = Srgb(S->Shoes);
	W.Breadth = S->Breadth;
	return W;
}

float PedestrianHeight(uint32 Seed, bool bWoman)
{
	// Two uniform draws averaged (a triangle round the middle): women 155-175 cm, men 165-195.
	const float A = static_cast<float>(Seed & 0xffff) / 65535.0f;
	const float B = static_cast<float>((Seed >> 16) & 0xffff) / 65535.0f;
	const float U = 0.5f * (A + B);
	return bWoman ? FMath::Lerp(155.0f, 175.0f, U) : FMath::Lerp(165.0f, 195.0f, U);
}

float HeightFor(FName Person)
{
	if (const FStaffWear* S = StaffFor(Person))
	{
		return S->Height;
	}
	// Anyone else (Dee, the world's people): a height seeded by their id, for their sex as the town lists them ("mei", "mrs-park":
	// town::People's ids are lower case, and a cooked game's FName may print another case), by the face only for an id the town
	// doesn't know.
	const FString Key = Person.ToString().ToLower();
	const ss::town::Person* Known = ss::town::FindPerson(std::string(TCHAR_TO_UTF8(*Key)));
	return PedestrianHeight(FCrc::StrCrc32(*Key), Known ? Known->Woman : IsWomansFace(Person));
}

// ------------------------------------------------------------------ what the 3D can show

uint64 GearKindsDrawable()
{
	// The apartment's gear meshes (NightOneStage draws GearDrop's items with them), by each item's picture; services and
	// subscriptions (the overlay, the mod bot, the meal kit, the gym) have nothing to stand on a shelf.
	auto MeshFor = [](ss::gear::Art A) -> const TCHAR* {
		switch (A)
		{
		case ss::gear::Art::Monitor:
			return TEXT("SM_Monitor");
		case ss::gear::Art::MonitorWide:
			return TEXT("SM_MonitorWide");
		case ss::gear::Art::Tower:
		case ss::gear::Art::TowerDual:
			return TEXT("SM_Tower");
		case ss::gear::Art::MacroPad:
			return TEXT("SM_MacroPad");
		case ss::gear::Art::Webcam:
			return TEXT("SM_Webcam");
		case ss::gear::Art::WebcamPro:
			return TEXT("SM_WebcamPro");
		case ss::gear::Art::Mirrorless:
			return TEXT("SM_Mirrorless");
		case ss::gear::Art::MicUsb:
			return TEXT("SM_MicUsb");
		case ss::gear::Art::MicXlr:
			return TEXT("SM_MicArm");
		case ss::gear::Art::RingLight:
			return TEXT("SM_RingLight");
		case ss::gear::Art::KeyLights:
			return TEXT("SM_KeyLight_L");
		case ss::gear::Art::Router:
			return TEXT("SM_Router");
		case ss::gear::Art::Chair:
			return TEXT("SM_Chair");
		case ss::gear::Art::Headphones:
			return TEXT("SM_Headphones");
		case ss::gear::Art::Plant:
			return TEXT("SM_Plant");
		default:
			return nullptr;
		}
	};
	uint64 Bits = 0;
	const std::vector<ss::gear::Item>& Catalog = ss::gear::Catalog();
	for (int32 I = 0; I < static_cast<int32>(Catalog.size()) && I < 64; ++I)
	{
		const TCHAR* Mesh = MeshFor(Catalog[static_cast<size_t>(I)].Pic);
		if (Mesh && FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s"), Mesh, Mesh)))
		{
			Bits |= uint64(1) << I;
		}
	}
	return Bits;
}

ss::ui::HostCaps HostCapsFor(const AShortStackCharacter* Hero)
{
	ss::ui::HostCaps Caps;
	if (!Hero)
	{
		return Caps;
	}
	HeroGrooms::FillCaps(Hero->GetCastFace(), Caps.HairStyles, Caps.FacialHair, Caps.Brows);
	Caps.GarmentKinds = Hero->GarmentKindsDrawable();
	// The gear meshes don't change while a map plays (imports land between sessions), and the shops ask every couple of
	// seconds: looked for once per world (a package lookup per catalog item).
	static TWeakObjectPtr<UWorld> GearWorld;
	static uint64 GearBits = 0;
	UWorld* World = Hero->GetWorld();
	if (!World || GearWorld.Get() != World)
	{
		GearBits = GearKindsDrawable();
		GearWorld = World;
	}
	Caps.GearKinds = GearBits;
	// The pawn's curios that render on the hero (1 the Lucky Seven chip, 2 the wristwatch): both P1, none yet.
	Caps.Curios = 0;
	Caps.Preview3D = !Hero->GetCastFace().IsNone();
	return Caps;
}

// ------------------------------------------------------------------ the console's session

void BindSession(const UObject* WorldContext, ss::Session* Session)
{
	UWorld* World = WorldOf(WorldContext);
	TMap<TWeakObjectPtr<UWorld>, ss::Session*>& Map = Sessions();
	for (auto It = Map.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	if (!World)
	{
		return;
	}
	if (Session)
	{
		Map.Add(World, Session);
	}
	else
	{
		Map.Remove(World);
	}
}

ss::Session* BoundSession(const UObject* WorldContext)
{
	UWorld* World = WorldOf(WorldContext);
	ss::Session* const* Found = World ? Sessions().Find(World) : nullptr;
	return Found ? *Found : nullptr;
}

void BindHeroLook(const UObject* WorldContext, const ss::hero::Look& Look)
{
	UWorld* World = WorldOf(WorldContext);
	TMap<TWeakObjectPtr<UWorld>, ss::hero::Look>& Map = HeroLooks();
	for (auto It = Map.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	if (World)
	{
		Map.Add(World, Look);
	}
}

bool HeroLookOn(const UObject* WorldContext, ss::hero::Look& OutLook)
{
	UWorld* World = WorldOf(WorldContext);
	const ss::hero::Look* Found = World ? HeroLooks().Find(World) : nullptr;
	if (Found)
	{
		OutLook = *Found;
	}
	return Found != nullptr;
}
} // namespace CastLooks

// ------------------------------------------------------------------ the face registry

UCastFaceRegistry* UCastFaceRegistry::Get(const UObject* Context)
{
	UWorld* World = WorldOf(Context);
	return World ? World->GetSubsystem<UCastFaceRegistry>() : nullptr;
}

bool UCastFaceRegistry::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCastFaceRegistry::Configure(FName InPlayerHero, const FString& InCareerKey)
{
	PlayerHero = InPlayerHero;
	CareerKey = InCareerKey;
	BlockNow = CastLooks::BlockFaces(PlayerHero, CareerKey);
	bConfigured = true;
	// Claims of bodies gone, or of faces now kept for someone else, lapse; their wearers claim again.
	for (auto It = Claims.CreateIterator(); It; ++It)
	{
		const FName Face = It.Key();
		It.Value().RemoveAll([this, Face](const FClaim& C) { return !C.Body.IsValid() || !IsAllowed(Face, C.Who); });
		if (It.Value().Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

FName UCastFaceRegistry::FaceFor(FName Person) const
{
	for (const FBlockFace& F : BlockNow)
	{
		if (F.Person == Person)
		{
			return F.Face;
		}
	}
	return Person == HeroWho ? PlayerHero : NAME_None;
}

FName UCastFaceRegistry::ReservedFor(FName Face) const
{
	if (Face.IsNone())
	{
		return NAME_None;
	}
	if (Face == PlayerHero)
	{
		return HeroWho;
	}
	for (const FBlockFace& F : BlockNow)
	{
		if (F.Face == Face)
		{
			return F.Person;
		}
	}
	return NAME_None;
}

bool UCastFaceRegistry::IsAllowed(FName Face, FName Who) const
{
	const FName Keeper = ReservedFor(Face);
	return Keeper.IsNone() || Keeper == Who;
}

bool UCastFaceRegistry::Claim(FName Face, FName Who, AShortStackCharacter* Body)
{
	if (Face.IsNone() || !Body || !IsAllowed(Face, Who))
	{
		return false;
	}
	TArray<FClaim>& Worn = Claims.FindOrAdd(Face);
	Worn.RemoveAll([Body](const FClaim& C) { return !C.Body.IsValid() || C.Body.Get() == Body; });
	Worn.Add(FClaim{Who, Body});
	return true;
}

void UCastFaceRegistry::Release(FName Face, const AShortStackCharacter* Body)
{
	if (TArray<FClaim>* Worn = Claims.Find(Face))
	{
		Worn->RemoveAll([Body](const FClaim& C) { return !C.Body.IsValid() || C.Body.Get() == Body; });
		if (Worn->Num() == 0)
		{
			Claims.Remove(Face);
		}
	}
}

TArray<AShortStackCharacter*> UCastFaceRegistry::Wearers(FName Face) const
{
	TArray<AShortStackCharacter*> Out;
	if (const TArray<FClaim>* Worn = Claims.Find(Face))
	{
		for (const FClaim& C : *Worn)
		{
			if (AShortStackCharacter* Body = C.Body.Get())
			{
				Out.Add(Body);
			}
		}
	}
	return Out;
}

bool UCastFaceRegistry::InViewElsewhere(FName Face, const AShortStackCharacter* Body, const FVector& Eye, const FVector& Dir, float CosHalfFov, double MaxDistance) const
{
	const FVector Ahead = Dir.GetSafeNormal();
	for (const AShortStackCharacter* Other : Wearers(Face))
	{
		if (Other == Body || Other->IsHidden())
		{
			continue;
		}
		const FVector To = Other->GetActorLocation() - Eye;
		const double Dist = To.Size();
		if (Dist <= MaxDistance && (Dist < 1.0 || FVector::DotProduct(To / Dist, Ahead) >= CosHalfFov))
		{
			return true;
		}
	}
	return false;
}

TArray<FName> UCastFaceRegistry::Reserved() const
{
	TArray<FName> Out;
	if (!PlayerHero.IsNone())
	{
		Out.Add(PlayerHero);
	}
	for (const FBlockFace& F : BlockNow)
	{
		if (!F.Face.IsNone())
		{
			Out.AddUnique(F.Face);
		}
	}
	return Out;
}

FString UCastFaceRegistry::Describe() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Faces on this map: the player is %s%s"), *PlayerHero.ToString(), bConfigured ? TEXT("") : TEXT(" (not configured)")));
	for (const FBlockFace& F : BlockNow)
	{
		const int32 Worn = F.Face.IsNone() ? 0 : Wearers(F.Face).Num();
		Lines.Add(FString::Printf(TEXT("  %s: %s%s%s, %.0f cm"), *F.Person.ToString(), F.Face.IsNone() ? TEXT("(no face to be had)") : *F.Face.ToString(),
			F.bOwnBuild ? TEXT(" (own build)") : (F.bBorrowed ? TEXT(" (borrowed until their build lands)") : TEXT(" (kept)")), Worn > 0 ? TEXT(", worn now") : TEXT(""),
			CastLooks::HeightFor(F.Person)));
	}
	TArray<FName> Faces;
	Claims.GetKeys(Faces);
	Faces.Sort(FNameLexicalLess());
	for (const FName Face : Faces)
	{
		const int32 Worn = Wearers(Face).Num();
		if (Worn > 0 && ReservedFor(Face).IsNone())
		{
			Lines.Add(FString::Printf(TEXT("  %s: worn by %d pedestrian%s"), *Face.ToString(), Worn, Worn == 1 ? TEXT("") : TEXT("s")));
		}
	}
	return FString::Join(Lines, TEXT("\n"));
}

FString UCastFaceRegistry::Show(FName InPlayerHero, const FString& InCareerKey)
{
	if (!bConfigured || !InCareerKey.IsEmpty())
	{
		Configure(InPlayerHero, InCareerKey);
	}
	return Describe();
}

FString UCastFaceRegistry::TestCloset(const UObject* WorldContext, const FString& Command)
{
	ss::Session* S = CastLooks::BoundSession(WorldContext);
	if (!S)
	{
		return TEXT("ss.Closet: no session on this map (the game mode binds it: CastLooks::BindSession)");
	}
	TArray<FString> A;
	Command.ParseIntoArrayWS(A);
	const FString Verb = A.Num() > 0 ? A[0].ToLower() : FString(TEXT("list"));
	ss::wardrobe::Closet& C = S->Life.Closet;
	FString Said;
	if (!C.Seeded && Verb != TEXT("list"))
	{
		// A career from before the closet: seeded from the creator's look first (as the migration does), so giving or wearing
		// one piece doesn't leave the rest of the hero bare.
		const std::set<std::string> Had = C.Owned;
		C = ss::wardrobe::SeedFrom(S->Person.Appearance);
		C.Owned.insert(Had.begin(), Had.end());
	}
	if (Verb == TEXT("give") && A.Num() > 1)
	{
		int32 Given = 0;
		if (A[1].Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			for (const ss::wardrobe::Garment& G : ss::wardrobe::Catalog())
			{
				Given += C.Owned.insert(G.Id).second ? 1 : 0;
			}
			if (ss::wardrobe::Catalog().empty())
			{
				Said = TEXT(" (the wardrobe catalog is empty: core-shops' Phase B fills it)");
			}
		}
		else
		{
			Given += C.Owned.insert(std::string(TCHAR_TO_UTF8(*A[1]))).second ? 1 : 0;
		}
		C.Seeded = true;
		Said = FString::Printf(TEXT("gave %d%s"), Given, *Said);
	}
	else if (Verb == TEXT("wear") && A.Num() > 1)
	{
		const std::string Why = S->Wear(std::string(TCHAR_TO_UTF8(*A[1])));
		Said = Why.empty() ? FString(TEXT("worn")) : FString::Printf(TEXT("not worn: %s"), UTF8_TO_TCHAR(Why.c_str()));
	}
	else if (Verb == TEXT("off") && A.Num() > 1)
	{
		const std::string Why = S->TakeOff(FCString::Atoi(*A[1]));
		Said = Why.empty() ? FString(TEXT("taken off")) : FString::Printf(TEXT("not taken off: %s"), UTF8_TO_TCHAR(Why.c_str()));
	}
	if (AShortStackCharacter* Hero = HeroIn(WorldOf(WorldContext)))
	{
		Hero->RefreshLook(S->Person.Appearance, C);
	}
	TArray<FString> Worn;
	for (int32 Slot = 0; Slot < ss::wardrobe::SlotCount; ++Slot)
	{
		Worn.Add(C.Worn[static_cast<size_t>(Slot)].empty() ? FString(TEXT("-")) : FString(UTF8_TO_TCHAR(C.Worn[static_cast<size_t>(Slot)].c_str())));
	}
	return FString::Printf(TEXT("Closet%s: %d owned, wearing [%s]%s%s"), C.Seeded ? TEXT("") : TEXT(" (not seeded)"), static_cast<int32>(C.Owned.size()),
		*FString::Join(Worn, TEXT(", ")), Said.IsEmpty() ? TEXT("") : TEXT("; "), *Said);
}

FString UCastFaceRegistry::TestFit(const UObject* WorldContext, float Level)
{
	ss::Session* S = CastLooks::BoundSession(WorldContext);
	if (!S)
	{
		return TEXT("ss.Fit.Set: no session on this map (the game mode binds it: CastLooks::BindSession)");
	}
	if (Level >= 0.0f)
	{
		S->Life.Fit.Level = FMath::Clamp(static_cast<double>(Level), 0.0, 100.0);
		// Counted now, so the fade starts from here.
		S->Life.Fit.LastCounted = S->WorldMinutes();
		S->Life.Fit.LastDecay = S->WorldMinutes();
	}
	const ss::body::TableEffects T = ss::body::TableOf(S->Life.Fit, S->WorldMinutes());
	return FString::Printf(TEXT("Fitness %.0f: at the table rest %.0f bpm, settle x%.2f, steady -%.0f bpm, Focus refill %.3f/s, Focus cost x%.2f, tilt x%.2f"), S->Life.Fit.Level,
		T.RestBpm, T.SettleScale, T.SteadyBpm, T.FocusRefill, T.FocusCostScale, T.TiltGainScale);
}

namespace CastLooksDetail
{
FAutoConsoleCommandWithWorldAndArgs FacesCmd(TEXT("ss.Look.Faces"),
	TEXT("ss.Look.Faces [career key]: whose face is whose on this map (spec 6.1) and who wears each now; configures the registry from the hero if nobody has "
		 "(AShortStackCharacter::TestFaces is its twin)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		UCastFaceRegistry* Registry = UCastFaceRegistry::Get(World);
		if (!Registry)
		{
			UE_LOG(LogCastLooks, Display, TEXT("ss.Look.Faces: no face registry here (it runs in game and PIE worlds)"));
			return;
		}
		const AShortStackCharacter* Hero = HeroIn(World);
		UE_LOG(LogCastLooks, Display, TEXT("%s"), *Registry->Show(Hero ? Hero->GetCastFace() : FName(), Args.Num() > 0 ? Args[0] : FString()));
	}));

FAutoConsoleCommandWithWorldAndArgs ClosetCmd(TEXT("ss.Closet"),
	TEXT("ss.Closet [list | give <garment id|all> | wear <garment id> | off <slot 0-5>]: the career's closet on this map's session, and the hero dressed in it "
		 "(UCastFaceRegistry::TestCloset is its twin)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		UE_LOG(LogCastLooks, Display, TEXT("%s"), *UCastFaceRegistry::TestCloset(World, FString::Join(Args, TEXT(" "))));
	}));

FAutoConsoleCommandWithWorldAndArgs ClosetGiveCmd(TEXT("ss.Closet.Give"), TEXT("ss.Closet.Give <garment id|all>: into the closet (spec 7.7)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		UE_LOG(LogCastLooks, Display, TEXT("%s"), *UCastFaceRegistry::TestCloset(World, TEXT("give ") + (Args.Num() > 0 ? Args[0] : FString(TEXT("all")))));
	}));

FAutoConsoleCommandWithWorldAndArgs FitCmd(TEXT("ss.Fit.Set"),
	TEXT("ss.Fit.Set <0-100>: the career's fitness on this map's session (spec 7.7); with no number, what it does at the table (UCastFaceRegistry::TestFit is its twin)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		UE_LOG(LogCastLooks, Display, TEXT("%s"), *UCastFaceRegistry::TestFit(World, Args.Num() > 0 ? FCString::Atof(*Args[0]) : -1.0f));
	}));
} // namespace CastLooksDetail
