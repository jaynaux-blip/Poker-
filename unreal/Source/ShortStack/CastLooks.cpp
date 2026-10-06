#include "CastLooks.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "ShortStack/Game/Live.h"
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
		FName(TEXT("HeroB0")), FName(TEXT("HeroB1")), FName(TEXT("HeroB2")), FName(TEXT("Bea")), FName(TEXT("Rosa")), FName(TEXT("Ruby")), FName(TEXT("Marlene"))};
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
} // namespace CastLooks

UCastFaceRegistry* UCastFaceRegistry::Get(const UObject* Context)
{
	UWorld* World = Context && GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
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
		Lines.Add(FString::Printf(TEXT("  %s: %s%s%s"), *F.Person.ToString(), F.Face.IsNone() ? TEXT("(no face to be had)") : *F.Face.ToString(),
			F.bOwnBuild ? TEXT(" (own build)") : (F.bBorrowed ? TEXT(" (borrowed until their build lands)") : TEXT(" (kept)")), Worn > 0 ? TEXT(", worn now") : TEXT("")));
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
		const APlayerController* Pc = World->GetFirstPlayerController();
		const AShortStackCharacter* Hero = Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
		UE_LOG(LogCastLooks, Display, TEXT("%s"), *Registry->Show(Hero ? Hero->GetCastFace() : FName(), Args.Num() > 0 ? Args[0] : FString()));
	}));
} // namespace CastLooksDetail
