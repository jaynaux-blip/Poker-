#include "HeroGrooms.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "GroomInstance.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "ShortStackCharacter.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogHeroGrooms, Log, All);

namespace HeroGroomsDetail
{
TAutoConsoleVariable<int32> CVarMidCardsStep(TEXT("ss.Look.MidCardsStep"), 1,
	TEXT("Mid tier (8-25 m): how many cards LODs past its first a groom is held on (1..2, spec 6.7)."));

/** The spike's budget: a hitch past half a 60 Hz frame shows. */
const float HitchBudgetMs = 8.0f;
const int32 BaseFrames = 30;
const int32 AfterFrames = 60;
const int32 SettleFrames = 10;
const double LoadTimeoutSeconds = 60.0;

FStreamableManager& Streamable()
{
	static FStreamableManager Manager;
	return Manager;
}

/** One frame as stat unit sees it (ms): the frame, the game thread, the render thread, the GPU. */
struct FFrame
{
	float Ms = 0.0f;
	float Gt = 0.0f;
	float Rt = 0.0f;
	float Gpu = 0.0f;
};

FFrame Sample(float DeltaSeconds)
{
	FFrame F;
	F.Ms = DeltaSeconds * 1000.0f;
	F.Gt = FPlatformTime::ToMilliseconds(GGameThreadTime);
	F.Rt = FPlatformTime::ToMilliseconds(GRenderThreadTime);
	F.Gpu = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
	return F;
}

float MedianOf(TArray<float> Values)
{
	if (Values.Num() == 0)
	{
		return 0.0f;
	}
	Values.Sort();
	return Values[Values.Num() / 2];
}

/** A label as part of a file name. */
FString FileSafe(const FString& In)
{
	FString Out = In;
	for (TCHAR& C : Out)
	{
		if (!FChar::IsAlnum(C) && C != TEXT('_') && C != TEXT('-'))
		{
			C = TEXT('_');
		}
	}
	return Out;
}

const TCHAR* TypeName(EGroomGeometryType Type)
{
	return Type == EGroomGeometryType::Cards ? TEXT("cards") : (Type == EGroomGeometryType::Meshes ? TEXT("helmet") : TEXT("strands"));
}

/**
 * A cast build's scalp groom and its binding (/Game/ShortStack/Cast/Built/MHC_<Name>/Grooms), from the asset registry: the
 * binding whose name says hair (the assembly sometimes names the groom GroomAsset_0 and gives its binding the style's name,
 * as HeroA0's and Twitch's), and the groom its searchable Groom tag points at.
 */
bool FindScalp(const FString& CastName, FSoftObjectPath& OutGroom, FSoftObjectPath& OutBinding, FString& OutWhy)
{
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (!Registry)
	{
		OutWhy = TEXT("no asset registry");
		return false;
	}
	const FString Folder = FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/Grooms"), *CastName);
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*Folder));
	Filter.ClassPaths.Add(UGroomBindingAsset::StaticClass()->GetClassPathName());
	TArray<FAssetData> Assets;
	Registry->GetAssets(Filter, Assets);
	for (const FAssetData& Asset : Assets)
	{
		FString Name = Asset.AssetName.ToString();
		Name.RemoveFromEnd(TEXT("_Binding"));
		if (HeroGrooms::KindOf(FString(), Name) != HeroGrooms::EGroomKind::Scalp)
		{
			continue;
		}
		FString Tag;
		if (Asset.GetTagValue(FName(TEXT("Groom")), Tag) && !Tag.IsEmpty() && Tag != TEXT("None"))
		{
			OutGroom = FSoftObjectPath(FPackageName::ExportTextPathToObjectPath(Tag));
		}
		else if (const UGroomBindingAsset* Loaded = Cast<UGroomBindingAsset>(Asset.GetAsset()))
		{
			// No tag (an asset saved before the property was searchable): the binding itself says, loaded here, untimed.
			OutGroom = FSoftObjectPath(Loaded->GetGroom());
		}
		OutBinding = Asset.GetSoftObjectPath();
		if (OutGroom.IsValid())
		{
			return true;
		}
	}
	OutWhy = FString::Printf(TEXT("no scalp groom binding in %s (%d bindings there)"), *Folder, Assets.Num());
	return false;
}

/** One swap to measure: to a candidate, or back to the hero's own groom. */
struct FStep
{
	FString Label;
	FSoftObjectPath GroomPath;
	FSoftObjectPath BindingPath;
	bool bNoBinding = false;
	/** The hero's own groom and binding (no load). */
	bool bOriginal = false;
	/** Take the two head-turn shots after this step. */
	bool bShots = false;
};

struct FResult
{
	FString Label;
	FString GroomName;
	FString BindingName;
	double LoadMs = 0.0;
	double CallMs = 0.0;
	float BaseMs = 0.0f;
	/** The worst frame before the swap: how noisy the baseline was (a hitch of the editor's own, not the swap's). */
	float BaseWorstMs = 0.0f;
	float MaxMs = 0.0f;
	/** The frame once the new hair has settled (the median of the last half after the swap): its running cost against BaseMs. */
	float SettledMs = 0.0f;
	/** The worst frame after the swap less the higher of the frame before it and the settled frame. */
	float HitchMs = 0.0f;
	int32 WorstAt = INDEX_NONE;
	float BaseGt = 0.0f;
	float MaxGt = 0.0f;
	float BaseRt = 0.0f;
	float MaxRt = 0.0f;
	float BaseGpu = 0.0f;
	float MaxGpu = 0.0f;
	bool bNoBinding = false;
	bool bMeshOk = false;
	bool bGroomOk = false;
	bool bBindingValid = false;
	bool bBound = false;
	bool bSkinning = false;
	int32 Lod = INDEX_NONE;
	EGroomGeometryType LodType = EGroomGeometryType::Strands;
	/** On a cards LOD: its type says cards, or the assembly's cards descriptions put cards there. */
	bool bOnCards = false;
	HeroGrooms::FGroomLods Lods;
	bool bSimulates = false;
	int32 Niagara = 0;
	/** PASS, HITCH or FAIL, and why when not PASS. */
	FString Verdict;
	FString Why;
};

class FSpike
{
public:
	TWeakObjectPtr<AShortStackCharacter> Hero;
	TWeakObjectPtr<UGroomComponent> Groom;
	TStrongObjectPtr<UGroomAsset> OrigGroom;
	TStrongObjectPtr<UGroomBindingAsset> OrigBinding;
	/** The hair's own materials as it was dressed (its shading, over whatever the Blueprint overrode), put back at the end. */
	TArray<TStrongObjectPtr<UMaterialInterface>> OrigMaterials;
	int32 OrigLod = INDEX_NONE;
	/** Every candidate loaded, kept resident for the whole run: a repeat is a warm swap, not a reload. */
	TArray<TStrongObjectPtr<UObject>> Keep;
	/** The scalp groom was added for the spike (the look had none: bald, or a hat over full hair): taken off again after. */
	bool bCreated = false;
	bool bSync = false;
	bool bWasFirstPerson = false;
	/** Dressed as another hero face for the spike (face=HeroA0), and the look to put back after; the first wait is longer. */
	bool bRedressed = false;
	ss::hero::Look OrigLook;
	int32 OrigAge = 0;
	int32 FirstSettle = 0;
	FString HeroFace;
	TArray<FStep> Steps;
	TArray<FResult> Results;
	TArray<FString> Lines;

	bool Tick(float DeltaSeconds);
	void Finish(bool bAborted);
	void BeginStep();

private:
	enum class EPhase : uint8
	{
		Load,
		Settle,
		Base,
		After,
		Shots,
		Done,
	};
	EPhase Phase = EPhase::Done;
	int32 StepAt = -1;
	int32 Wait = 0;
	double LoadStart = 0.0;
	TSharedPtr<FStreamableHandle> Handle;
	TStrongObjectPtr<UGroomAsset> StepGroom;
	TStrongObjectPtr<UGroomBindingAsset> StepBinding;
	TArray<FFrame> Before;
	TArray<FFrame> After;
	FResult Current;
	int32 ShotStage = 0;
	TWeakObjectPtr<ACameraActor> ShotCamera;

	void Swap();
	void Evaluate();
	bool ShotsTick();
	void EndShots();
	void Say(const FString& Line);
	FString Summary() const;
	APlayerController* Controller() const;
};

TUniquePtr<FSpike> GSpike;
FTSTicker::FDelegateHandle GSpikeTicker;
FString GLastReport;

void FSpike::Say(const FString& Line)
{
	Lines.Add(Line);
	UE_LOG(LogHeroGrooms, Display, TEXT("%s"), *Line);
}

APlayerController* FSpike::Controller() const
{
	return Hero.IsValid() ? Cast<APlayerController>(Hero->GetController()) : nullptr;
}

void FSpike::BeginStep()
{
	++StepAt;
	Before.Reset();
	After.Reset();
	StepGroom.Reset();
	StepBinding.Reset();
	Handle.Reset();
	if (!Steps.IsValidIndex(StepAt))
	{
		Phase = EPhase::Done;
		return;
	}
	const FStep& S = Steps[StepAt];
	Current = FResult();
	Current.Label = S.Label;
	LoadStart = FPlatformTime::Seconds();
	// A garbage collection now, inside the settle frames, so the editor's own (every minute or so in PIE) can't land in a
	// measured window and read as the swap's hitch. The candidates are kept resident (Keep), so it frees none of them.
	if (GEngine)
	{
		GEngine->ForceGarbageCollection(true);
	}
	if (S.bOriginal || bSync)
	{
		// Already resident (the hero's own), or loaded inside the timed call (sync).
		Phase = EPhase::Settle;
		Wait = FMath::Max(SettleFrames, FirstSettle);
		FirstSettle = 0;
		return;
	}
	TArray<FSoftObjectPath> Paths;
	Paths.Add(S.GroomPath);
	if (!S.bNoBinding && S.BindingPath.IsValid())
	{
		Paths.Add(S.BindingPath);
	}
	Handle = Streamable().RequestAsyncLoad(Paths, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	Phase = EPhase::Load;
}

bool FSpike::Tick(float DeltaSeconds)
{
	AShortStackCharacter* H = Hero.Get();
	if (!H || !Groom.IsValid() || !H->GetWorld())
	{
		Say(TEXT("GroomSpike: the hero or their groom went away (PIE ended, or the hero was dressed again): stopped"));
		GLastReport = FString::Join(Lines, TEXT("\n"));
		return false;
	}
	switch (Phase)
	{
	case EPhase::Load:
	{
		const FStep& S = Steps[StepAt];
		const double Waited = FPlatformTime::Seconds() - LoadStart;
		if (Handle.IsValid() && !Handle->HasLoadCompleted())
		{
			if (Waited > LoadTimeoutSeconds)
			{
				Say(FString::Printf(TEXT("GroomSpike %d/%d %s: FAIL, not loaded after %.0f s (%s)"), StepAt + 1, Steps.Num(), *S.Label, Waited, *S.GroomPath.ToString()));
				BeginStep();
			}
			break;
		}
		if (!StepGroom.IsValid())
		{
			StepGroom.Reset(Cast<UGroomAsset>(S.GroomPath.ResolveObject()));
			if (!S.bNoBinding && S.BindingPath.IsValid())
			{
				StepBinding.Reset(Cast<UGroomBindingAsset>(S.BindingPath.ResolveObject()));
			}
		}
		if (!StepGroom.IsValid())
		{
			Say(FString::Printf(TEXT("GroomSpike %d/%d %s: FAIL, %s didn't load"), StepAt + 1, Steps.Num(), *S.Label, *S.GroomPath.ToString()));
			BeginStep();
			break;
		}
		Keep.Emplace(StepGroom.Get());
		if (StepBinding.IsValid())
		{
			Keep.Emplace(StepBinding.Get());
		}
		// Loaded isn't ready: in the editor the groom and the binding build their data from the DDC after loading (a cooked
		// game's are in the package). The Style Book's preload waits for this too.
		const UGroomBindingAsset* B = StepBinding.Get();
		const bool bReady = StepGroom->IsValid() && (!B || (!B->IsCompiling() && B->IsValid()));
		if (!bReady)
		{
			if (Waited > LoadTimeoutSeconds)
			{
				Say(FString::Printf(TEXT("GroomSpike %d/%d %s: FAIL, loaded but not ready after %.0f s (groom valid %d, binding compiling %d valid %d)"), StepAt + 1,
					Steps.Num(), *S.Label, Waited, StepGroom->IsValid() ? 1 : 0, B && B->IsCompiling() ? 1 : 0, B && B->IsValid() ? 1 : 0));
				BeginStep();
			}
			break;
		}
		Current.LoadMs = Waited * 1000.0;
		Phase = EPhase::Settle;
		Wait = FMath::Max(SettleFrames, FirstSettle);
		FirstSettle = 0;
		break;
	}
	case EPhase::Settle:
		if (--Wait <= 0)
		{
			Phase = EPhase::Base;
		}
		break;
	case EPhase::Base:
		Before.Add(Sample(DeltaSeconds));
		if (Before.Num() >= BaseFrames)
		{
			Swap();
			Phase = EPhase::After;
		}
		break;
	case EPhase::After:
		After.Add(Sample(DeltaSeconds));
		if (After.Num() >= AfterFrames)
		{
			Evaluate();
			if (Steps[StepAt].bShots && Controller())
			{
				Phase = EPhase::Shots;
				ShotStage = 0;
				Wait = 0;
			}
			else
			{
				BeginStep();
			}
		}
		break;
	case EPhase::Shots:
		if (!ShotsTick())
		{
			EndShots();
			BeginStep();
		}
		break;
	case EPhase::Done:
	default:
		Finish(false);
		return false;
	}
	return true;
}

void FSpike::Swap()
{
	AShortStackCharacter* H = Hero.Get();
	UGroomComponent* G = Groom.Get();
	const FStep& S = Steps[StepAt];
	const double Start = FPlatformTime::Seconds();
	UGroomAsset* Asset = S.bOriginal ? OrigGroom.Get() : StepGroom.Get();
	UGroomBindingAsset* Binding = S.bOriginal ? OrigBinding.Get() : (S.bNoBinding ? nullptr : StepBinding.Get());
	if (!S.bOriginal && bSync)
	{
		// Without a preload: the load is on the game thread, inside the frame.
		Asset = LoadObject<UGroomAsset>(nullptr, *S.GroomPath.ToString());
		Binding = !S.bNoBinding && S.BindingPath.IsValid() ? LoadObject<UGroomBindingAsset>(nullptr, *S.BindingPath.ToString()) : nullptr;
		StepGroom.Reset(Asset);
		StepBinding.Reset(Binding);
		if (Asset)
		{
			Keep.Emplace(Asset);
		}
		if (Binding)
		{
			Keep.Emplace(Binding);
		}
	}
	if (Asset)
	{
		H->SwapGroom(G, Asset, Binding);
	}
	Current.CallMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	if (bSync && !S.bOriginal)
	{
		Current.LoadMs = Current.CallMs;
	}
}

void FSpike::Evaluate()
{
	AShortStackCharacter* H = Hero.Get();
	UGroomComponent* G = Groom.Get();
	FResult& R = Current;
	TArray<float> Ms;
	TArray<float> Gt;
	TArray<float> Rt;
	TArray<float> Gpu;
	for (const FFrame& F : Before)
	{
		Ms.Add(F.Ms);
		Gt.Add(F.Gt);
		Rt.Add(F.Rt);
		Gpu.Add(F.Gpu);
	}
	R.BaseMs = MedianOf(Ms);
	R.BaseWorstMs = Ms.Num() ? FMath::Max(Ms) : 0.0f;
	R.BaseGt = MedianOf(Gt);
	R.BaseRt = MedianOf(Rt);
	R.BaseGpu = MedianOf(Gpu);
	for (int32 I = 0; I < After.Num(); ++I)
	{
		const FFrame& F = After[I];
		if (F.Ms > R.MaxMs)
		{
			R.MaxMs = F.Ms;
			R.WorstAt = I + 1;
		}
		R.MaxGt = FMath::Max(R.MaxGt, F.Gt);
		R.MaxRt = FMath::Max(R.MaxRt, F.Rt);
		R.MaxGpu = FMath::Max(R.MaxGpu, F.Gpu);
	}
	// Settled: the median of the last half after the swap. A groom that simply costs more to draw (strands, no cards) raises
	// every frame after it; that's its running cost, not the swap's hitch, so the hitch is measured over the higher of the two.
	TArray<float> Tail;
	for (int32 I = After.Num() / 2; I < After.Num(); ++I)
	{
		Tail.Add(After[I].Ms);
	}
	R.SettledMs = MedianOf(Tail);
	R.HitchMs = FMath::Max(0.0f, R.MaxMs - FMath::Max(R.BaseMs, R.SettledMs));

	UGroomAsset* Asset = G->GroomAsset;
	UGroomBindingAsset* Binding = G->BindingAsset;
	const USkeletalMesh* FaceMesh = H->Face ? H->Face->GetSkeletalMeshAsset() : nullptr;
	R.GroomName = GetNameSafe(Asset);
	R.BindingName = Binding ? Binding->GetName() : FString(TEXT("none"));
	R.bNoBinding = Binding == nullptr;
	if (Binding)
	{
		R.bMeshOk = UGroomBindingAsset::IsCompatible(FaceMesh, Binding, false);
		R.bGroomOk = UGroomBindingAsset::IsCompatible(static_cast<const UGroomAsset*>(Asset), Binding, false);
		R.bBindingValid = UGroomBindingAsset::IsBindingAssetValid(Binding, false, false);
	}
	// Bound: every group's instance registered the face as the mesh it follows (otherwise it's rigid on the component, and
	// stays put while the head turns).
	R.bBound = G->GetGroupCount() > 0;
	for (int32 I = 0; I < static_cast<int32>(G->GetGroupCount()); ++I)
	{
		const FHairGroupInstance* Instance = G->GetGroupInstance(I);
		R.bBound = R.bBound && Instance && Instance->Debug.MeshComponentForDebug == static_cast<const UMeshComponent*>(H->Face.Get());
	}
	R.Lod = G->GetForcedLOD();
	R.Lods = HeroGrooms::LodsOf(Asset);
	// Not forced (a strands-only groom left on its own LODs): what it draws up close, its first.
	const int32 Drawn = FMath::Max(R.Lod, 0);
	R.LodType = Asset ? Asset->GetGeometryType(0, Drawn) : EGroomGeometryType::Strands;
	R.bOnCards = R.LodType == EGroomGeometryType::Cards || (R.Lods.FirstCards != INDEX_NONE && R.Lod >= R.Lods.FirstCards && R.Lod <= R.Lods.LastCards);
	// A rigid LOD registers the face too, but rides the component, not the skin: following the head needs skinning.
	R.bSkinning = Asset && Asset->GetBindingType(0, Drawn) == EGroomBindingType::Skinning;
	R.bBound = R.bBound && R.bSkinning;
	R.bSimulates = Asset && Asset->IsSimulationEnable();
	for (const TObjectPtr<UNiagaraComponent>& N : G->NiagaraComponents)
	{
		R.Niagara += N ? 1 : 0;
	}
	if (!Asset)
	{
		R.Verdict = TEXT("FAIL");
		R.Why = TEXT("no groom on the component");
	}
	else if (!R.bBound)
	{
		R.Verdict = TEXT("FAIL");
		R.Why = R.bNoBinding ? TEXT("no binding: rigid, won't follow the head")
							 : (R.bSkinning ? TEXT("the binding was refused: rigid, won't follow the head") : TEXT("this LOD binds rigidly: won't follow the head"));
	}
	else if (!R.bOnCards && R.Lods.FirstCards != INDEX_NONE)
	{
		R.Verdict = TEXT("FAIL");
		R.Why = FString::Printf(TEXT("held on %s, not cards"), TypeName(R.LodType));
	}
	else
	{
		R.Verdict = R.HitchMs <= HitchBudgetMs ? TEXT("PASS") : TEXT("HITCH");
		if (R.Lods.FirstCards == INDEX_NONE)
		{
			// The buzz cuts and the stubble come as strands only (no cards, no helmet): drawn as strands whatever the tier.
			R.Why = TEXT("a strands-only groom: no cards to hold");
		}
	}
	Say(FString::Printf(TEXT("GroomSpike %d/%d %s: groom %s, binding %s | preload %.0f ms | swap call %.2f ms | frame %.1f ms before (worst %.1f), worst %.1f ms after (hitch +%.1f ms, frame %d), settled %.1f ms | GT %.1f/%.1f RT %.1f/%.1f GPU %.1f/%.1f ms (before/worst) | binding: mesh %s groom %s valid %s, bound to the face %s (%s) | LOD %d (%s; cards %d..%d, helmet %d, %d LODs) | simulation %s, %d Niagara | %s%s%s"),
		StepAt + 1, Steps.Num(), *R.Label, *R.GroomName, *R.BindingName, R.LoadMs, R.CallMs, R.BaseMs, R.BaseWorstMs, R.MaxMs, R.HitchMs, R.WorstAt, R.SettledMs, R.BaseGt, R.MaxGt, R.BaseRt, R.MaxRt,
		R.BaseGpu, R.MaxGpu, R.bNoBinding ? TEXT("-") : (R.bMeshOk ? TEXT("ok") : TEXT("NO")), R.bNoBinding ? TEXT("-") : (R.bGroomOk ? TEXT("ok") : TEXT("NO")),
		R.bNoBinding ? TEXT("-") : (R.bBindingValid ? TEXT("ok") : TEXT("NO")), R.bBound ? TEXT("yes") : TEXT("NO"), R.bSkinning ? TEXT("skinning") : TEXT("rigid LOD"), R.Lod, TypeName(R.LodType), R.Lods.FirstCards,
		R.Lods.LastCards, R.Lods.Helmet, R.Lods.Count, R.bSimulates ? TEXT("on") : TEXT("off"), R.Niagara, *R.Verdict, R.Why.IsEmpty() ? TEXT("") : TEXT(": "), *R.Why));
	Results.Add(R);
}

bool FSpike::ShotsTick()
{
	if (Wait > 0)
	{
		--Wait;
		return true;
	}
	AShortStackCharacter* H = Hero.Get();
	APlayerController* Pc = Controller();
	UWorld* World = H->GetWorld();
	if (!Pc || !World)
	{
		return false;
	}
	const FVector Eye = H->EyeLocation();
	const FVector Up(0.0, 0.0, 1.0);
	const FVector Ahead = H->GetActorForwardVector();
	auto TurnTo = [&](float Degrees) { H->SetLookTarget(Eye + Ahead.RotateAngleAxis(Degrees, Up) * 300.0, 1.0f); };
	auto Shoot = [&](const TCHAR* Side) {
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir() / FString::Printf(TEXT("GroomSpike_%s_%s.png"), *FileSafe(Steps[StepAt].Label), Side));
		Pc->ConsoleCommand(FString::Printf(TEXT("HighResShot 1280x720 filename=%s"), *Path), false);
		Say(FString::Printf(TEXT("GroomSpike shot: %s"), *Path));
	};
	switch (ShotStage++)
	{
	case 0:
	{
		// A portrait from the front and a little to the side, the head turned one way and then the other: the hair must turn
		// with it (bound), on cards, with no seam at the hairline.
		const FVector At = Eye + Ahead * 150.0 + H->GetActorRightVector() * 35.0 + Up * 3.0;
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform((Eye - Up * 6.0 - At).Rotation(), At));
		if (!Camera)
		{
			return false;
		}
		Camera->GetCameraComponent()->SetFieldOfView(30.0f);
		Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		ShotCamera = Camera;
		Pc->SetViewTargetWithBlend(Camera, 0.0f);
		TurnTo(40.0f);
		Wait = 45;
		return true;
	}
	case 1:
		Shoot(TEXT("L"));
		Wait = 20;
		return true;
	case 2:
		TurnTo(-40.0f);
		Wait = 45;
		return true;
	case 3:
		Shoot(TEXT("R"));
		Wait = 20;
		return true;
	default:
		return false;
	}
}

void FSpike::EndShots()
{
	if (AShortStackCharacter* H = Hero.Get())
	{
		H->SetLookTarget(FVector::ZeroVector, 0.0f);
		if (APlayerController* Pc = Controller())
		{
			Pc->SetViewTargetWithBlend(H, 0.0f);
		}
	}
	if (ACameraActor* Camera = ShotCamera.Get())
	{
		Camera->Destroy();
	}
	ShotCamera.Reset();
}

FString FSpike::Summary() const
{
	// Per candidate: the first swap to it (cold: the groom's GPU resources are allocated the first time it's drawn, which a
	// player meets on the Style Book's first preview of it) and the rest (warm), the swap call and the hitch.
	TArray<FString> Out;
	TArray<FString> Labels;
	for (const FResult& R : Results)
	{
		Labels.AddUnique(R.Label);
	}
	bool bAnyFail = false;
	bool bAnyHitch = false;
	bool bAnyCold = false;
	bool bOwnFail = false;
	float WorstMedian = 0.0f;
	float WorstFirst = 0.0f;
	for (const FString& Label : Labels)
	{
		TArray<float> Calls;
		TArray<float> Hitches;
		TArray<float> Settles;
		bool bFail = false;
		bool bBound = true;
		bool bCards = true;
		FString Why;
		FString GroomName;
		for (const FResult& R : Results)
		{
			if (R.Label == Label)
			{
				Calls.Add(static_cast<float>(R.CallMs));
				Hitches.Add(R.HitchMs);
				Settles.Add(R.SettledMs - R.BaseMs);
				bFail = bFail || R.Verdict == TEXT("FAIL");
				bBound = bBound && R.bBound;
				bCards = bCards && R.bOnCards;
				Why = R.Why.IsEmpty() ? Why : R.Why;
				GroomName = R.GroomName;
			}
		}
		const float MedianHitch = MedianOf(Hitches);
		const float FirstHitch = Hitches.Num() ? Hitches[0] : 0.0f;
		const bool bHitch = !bFail && MedianHitch > HitchBudgetMs;
		const bool bCold = !bFail && !bHitch && FirstHitch > HitchBudgetMs;
		const FString Verdict = bFail ? TEXT("FAIL") : (bHitch ? TEXT("HITCH") : (bCold ? TEXT("PASS warm, HITCH cold") : TEXT("PASS")));
		const bool bOwn = Label == TEXT("self") || Label == TEXT("back");
		bAnyFail = bAnyFail || bFail;
		bOwnFail = bOwnFail || (bOwn && bFail);
		bAnyHitch = bAnyHitch || bHitch;
		bAnyCold = bAnyCold || bCold;
		if (!bFail)
		{
			WorstMedian = FMath::Max(WorstMedian, MedianHitch);
			WorstFirst = FMath::Max(WorstFirst, FirstHitch);
		}
		const FString Because = Why.IsEmpty() ? FString() : FString::Printf(TEXT(" (%s)"), *Why);
		Out.Add(FString::Printf(TEXT("  %s (%s): swap call %.2f ms median, %.2f ms worst; hitch %.1f ms first, %.1f ms median, %.1f ms worst, over %d; settled %+.1f ms a frame against before; %s, %s: %s%s"),
			*Label, *GroomName, MedianOf(Calls), Calls.Num() ? FMath::Max(Calls) : 0.0f, FirstHitch, MedianHitch, Hitches.Num() ? FMath::Max(Hitches) : 0.0f, Hitches.Num(),
			MedianOf(Settles), bBound ? TEXT("bound to the face") : TEXT("NOT bound"), bCards ? TEXT("on cards") : TEXT("NOT on cards"), *Verdict, *Because));
	}
	FString Verdict;
	if (Results.Num() == 0)
	{
		Verdict = TEXT("VERDICT: no results (nothing was measured).");
	}
	else if (bOwnFail)
	{
		Verdict = TEXT("VERDICT: FAIL. The hero's own card groom doesn't stay bound to the face or on cards when set at run time: the barber sells what renders (shave, beard on/off, color, buzz, shaved) and marks the rest PORTRAIT ONLY (spec 10).");
	}
	else if (bAnyHitch)
	{
		Verdict = FString::Printf(TEXT("VERDICT: HITCH. A median hitch of %.1f ms > %.0f ms: swap the hair under black (the barber's cinematic fades before the reveal; the Style Book previews in the portrait), spec 7.5.4 and 10."),
			WorstMedian, HitchBudgetMs);
	}
	else if (bAnyCold)
	{
		Verdict = FString::Printf(TEXT("VERDICT: PASS WARM. Warm swaps hitch at most %.1f ms (median) but a groom's first swap hitched %.1f ms: change hair in view only after each groom has been drawn once (the Style Book's preload draws the face's set hidden under the barber stage's wake), else under black."),
			WorstMedian, WorstFirst);
	}
	else
	{
		Verdict = FString::Printf(TEXT("VERDICT: PASS. Hitches at most %.1f ms (first swaps %.1f ms) <= %.0f ms, bound to the face and on cards: the hair can change in view (the Style Book's preview and the reveal)%s."), WorstMedian,
			WorstFirst, HitchBudgetMs, bAnyFail ? TEXT("; a cross-face candidate failed, which says nothing about same-face copies") : TEXT(""));
	}
	// How far to trust it: a baseline window with a hitch of its own (the editor's, not the swap's), and an editor that ran
	// slow throughout (PIE without focus is throttled).
	int32 Noisy = 0;
	TArray<float> Bases;
	for (const FResult& R : Results)
	{
		Noisy += R.BaseWorstMs - R.BaseMs > HitchBudgetMs ? 1 : 0;
		Bases.Add(R.BaseMs);
	}
	TArray<FString> Notes;
	if (Noisy > 0)
	{
		Notes.Add(FString::Printf(TEXT("NOTE: %d of %d baselines had a hitch of their own (worst before > median + %.0f ms): the editor's noise is that size; run it again if a verdict sits near the budget."),
			Noisy, Results.Num(), HitchBudgetMs));
	}
	if (MedianOf(Bases) > 50.0f)
	{
		Notes.Add(FString::Printf(TEXT("NOTE: frames ran %.0f ms (median): was PIE in the background (the editor throttles it)? Run it again with the viewport focused."), MedianOf(Bases)));
	}
	return FString::Printf(TEXT("GroomSpike summary, hero %s (%s):\n%s\n%s%s%s"), *HeroFace, bSync ? TEXT("loaded inside the call") : TEXT("preloaded"), *FString::Join(Out, TEXT("\n")), *Verdict,
		Notes.Num() ? TEXT("\n") : TEXT(""), *FString::Join(Notes, TEXT("\n")));
}

void FSpike::Finish(bool bAborted)
{
	EndShots();
	AShortStackCharacter* H = Hero.Get();
	UGroomComponent* G = Groom.Get();
	if (H && bRedressed)
	{
		// Dressed as another hero face for the spike (face=): the player's own look again.
		ss::hero::Character As;
		As.Appearance = OrigLook;
		As.Age = OrigAge;
		H->ApplyLook(As);
	}
	else if (H && G)
	{
		if (bCreated)
		{
			H->DropGroom(G);
		}
		else
		{
			if (G->GroomAsset != OrigGroom.Get() || G->BindingAsset != OrigBinding.Get())
			{
				H->SwapGroom(G, OrigGroom.Get(), OrigBinding.Get());
			}
			// SwapGroom shades from the groom's own materials; the hair as dressed had its shading over the Blueprint's.
			for (int32 Slot = 0; Slot < OrigMaterials.Num() && Slot < G->GetNumMaterials(); ++Slot)
			{
				if (OrigMaterials[Slot].IsValid() && G->GetMaterial(Slot) != OrigMaterials[Slot].Get())
				{
					G->SetMaterial(Slot, OrigMaterials[Slot].Get());
				}
			}
			if (G->GetForcedLOD() != OrigLod)
			{
				G->SetForcedLOD(OrigLod);
			}
		}
	}
	if (H)
	{
		H->SetLookTarget(FVector::ZeroVector, 0.0f);
		H->SetFirstPerson(bWasFirstPerson);
	}
	if (bAborted)
	{
		Say(TEXT("GroomSpike: stopped, the hero's own hair put back"));
	}
	Say(Summary());
	GLastReport = FString::Join(Lines, TEXT("\n"));
}

/** An asset's object path from a package path or an object path ("/Game/X/Y" is "/Game/X/Y.Y"). */
FSoftObjectPath ObjectPathOf(const FString& Path)
{
	return FSoftObjectPath(Path.Contains(TEXT(".")) ? Path : Path + TEXT(".") + FPackageName::GetShortName(Path));
}

/** "repeat=3 binding=/Game/... sync shots" */
FString OptionValue(const TArray<FString>& Tokens, const TCHAR* Key)
{
	const FString Prefix = FString(Key) + TEXT("=");
	for (const FString& T : Tokens)
	{
		if (T.StartsWith(Prefix, ESearchCase::IgnoreCase))
		{
			return T.Mid(Prefix.Len());
		}
	}
	return FString();
}

bool HasOption(const TArray<FString>& Tokens, const TCHAR* Key)
{
	for (const FString& T : Tokens)
	{
		if (T.Equals(Key, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

AShortStackCharacter* HeroIn(UWorld* World)
{
	APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs SpikeCmd(TEXT("ss.Look.GroomSpike"),
	TEXT("ss.Look.GroomSpike [all|self|back|<HeroA1|cast name>|<groom path>|stop|report] [repeat=N] [binding=<path|none>] [sync] [shots] [face=HeroA0]: the day-one groom check "
		 "(spec 7.5.4): swaps the hero's hair at run time and measures the hitch (HeroGrooms.h)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		const FString What = Args.Num() > 0 ? Args[0] : FString(TEXT("all"));
		if (What.Equals(TEXT("stop"), ESearchCase::IgnoreCase))
		{
			HeroGrooms::StopSpike();
			return;
		}
		if (What.Equals(TEXT("report"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogHeroGrooms, Display, TEXT("%s"), *HeroGrooms::SpikeReport());
			return;
		}
		TArray<FString> Options;
		for (int32 I = 1; I < Args.Num(); ++I)
		{
			Options.Add(Args[I]);
		}
		UE_LOG(LogHeroGrooms, Display, TEXT("%s"), *HeroGrooms::StartSpike(HeroIn(World), What, FString::Join(Options, TEXT(" "))));
	}));
} // namespace HeroGroomsDetail

using namespace HeroGroomsDetail;

namespace HeroGrooms
{
EGroomKind KindOf(const FString& Variable, const FString& Asset)
{
	auto Is = [&](const TCHAR* Word) { return Variable.Contains(Word) || Asset.StartsWith(Word); };
	if (Is(TEXT("Eyelash")))
	{
		return EGroomKind::Lashes;
	}
	if (Is(TEXT("Eyebrow")))
	{
		return EGroomKind::Brows;
	}
	if (Is(TEXT("Mustache")))
	{
		return EGroomKind::Mustache;
	}
	if (Is(TEXT("Beard")) || Is(TEXT("Goatee")))
	{
		return EGroomKind::Beard;
	}
	if (Is(TEXT("Fuzz")) || Is(TEXT("Peachfuzz")))
	{
		return EGroomKind::Fuzz;
	}
	return EGroomKind::Scalp;
}

FGroomLods LodsOf(const UGroomAsset* Asset)
{
	FGroomLods Out;
	if (!Asset)
	{
		return Out;
	}
	Out.Count = FMath::Max(0, Asset->GetLODCount());
	// As the assembly describes them (the street and the Back Room have held grooms on these since the cast was built).
	for (const FHairGroupsCardsSourceDescription& Cards : Asset->GetHairGroupsCards())
	{
		if (Cards.ImportedMesh && Cards.LODIndex >= 0)
		{
			Out.FirstCards = Out.FirstCards == INDEX_NONE ? Cards.LODIndex : FMath::Min(Out.FirstCards, Cards.LODIndex);
			Out.LastCards = FMath::Max(Out.LastCards, Cards.LODIndex);
		}
	}
	for (const FHairGroupsMeshesSourceDescription& Meshes : Asset->GetHairGroupsMeshes())
	{
		if (Meshes.ImportedMesh && Meshes.LODIndex >= 0)
		{
			Out.Helmet = Out.Helmet == INDEX_NONE ? Meshes.LODIndex : FMath::Min(Out.Helmet, Meshes.LODIndex);
		}
	}
	if (Out.Helmet != INDEX_NONE && Out.LastCards >= Out.Helmet)
	{
		Out.LastCards = FMath::Max(Out.FirstCards, Out.Helmet - 1);
	}
	return Out;
}

int32 LodForTier(const UGroomAsset* Asset, int32 Tier)
{
	const FGroomLods L = LodsOf(Asset);
	switch (FMath::Clamp(Tier, 0, 2))
	{
	case 0:
		return L.FirstCards;
	case 1:
		return L.FirstCards == INDEX_NONE ? INDEX_NONE : FMath::Min(L.FirstCards + FMath::Clamp(CVarMidCardsStep.GetValueOnGameThread(), 1, 2), L.LastCards);
	default:
		return L.Helmet != INDEX_NONE ? L.Helmet : L.LastCards;
	}
}

bool HoldForTier(UGroomComponent* Groom, int32 Tier)
{
	if (!Groom || !Groom->GroomAsset)
	{
		return false;
	}
	const int32 Lod = LodForTier(Groom->GroomAsset, Tier);
	if (Lod == INDEX_NONE)
	{
		return false;
	}
	if (Groom->GetForcedLOD() != Lod)
	{
		Groom->SetForcedLOD(Lod);
	}
	return true;
}

void HoldOnCards(UGroomComponent* Groom)
{
	HoldForTier(Groom, 0);
}

FString StartSpike(AShortStackCharacter* Hero, const FString& Candidate, const FString& Options)
{
	if (GSpike)
	{
		return TEXT("GroomSpike: one is running (ss.Look.GroomSpike stop)");
	}
	UWorld* World = Hero ? Hero->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld())
	{
		return TEXT("GroomSpike: no hero in a game world (run it in PIE, on the street or anywhere the hero walks)");
	}
	TArray<FString> Tokens;
	Options.ParseIntoArrayWS(Tokens);
	const FString RepeatText = OptionValue(Tokens, TEXT("repeat"));
	const int32 Repeat = FMath::Clamp(RepeatText.IsEmpty() ? 3 : FCString::Atoi(*RepeatText), 1, 10);
	const FString BindingOption = OptionValue(Tokens, TEXT("binding"));
	const bool bShots = HasOption(Tokens, TEXT("shots"));

	TUniquePtr<FSpike> S = MakeUnique<FSpike>();
	S->Hero = Hero;
	S->bSync = HasOption(Tokens, TEXT("sync"));
	// face=HeroA0: the spec's check is on HeroA0, and the career may wear another; dressed as it (the look's body type and
	// skin band) for the spike and put back after. The rebuild is before the first measurement, which waits for it to settle.
	const FString FaceOption = OptionValue(Tokens, TEXT("face"));
	const ss::hero::Look& Worn = Hero->GetLook();
	if (!FaceOption.IsEmpty() && (FName(*FaceOption) != Hero->GetCastFace() || Worn.Hair < 2 || Worn.Hat != 0))
	{
		const bool bHeroFace = FaceOption.Len() == 6 && FaceOption.StartsWith(TEXT("Hero")) && (FaceOption[4] == TEXT('A') || FaceOption[4] == TEXT('B'))
			&& FaceOption[5] >= TEXT('0') && FaceOption[5] <= TEXT('2');
		if (!bHeroFace)
		{
			return FString::Printf(TEXT("GroomSpike: face= takes a hero face, HeroA0..HeroB2 (not '%s')"), *FaceOption);
		}
		ss::hero::Character As;
		As.Appearance = Hero->GetLook();
		As.Age = Hero->GetLookAge();
		S->OrigLook = As.Appearance;
		S->OrigAge = As.Age;
		As.Appearance.Body = FaceOption[4] == TEXT('B') ? 1 : 0;
		As.Appearance.Skin = FaceOption[5] == TEXT('0') ? 1 : (FaceOption[5] == TEXT('1') ? 4 : 7);
		// Wearing that face's own hair, bare-headed (the creator's buzz or shaved would take it off, a hat would hide it and
		// keep only hair that lies close): the swap is measured on the scalp groom the face was assembled with.
		As.Appearance.Hair = FMath::Max(As.Appearance.Hair, 2);
		As.Appearance.Hat = 0;
		Hero->ApplyLook(As);
		S->bRedressed = true;
		S->FirstSettle = 90;
		if (FName(*FaceOption) != Hero->GetCastFace())
		{
			As.Appearance = S->OrigLook;
			Hero->ApplyLook(As);
			return FString::Printf(TEXT("GroomSpike: %s isn't built (hero_cast.py)"), *FaceOption);
		}
	}
	S->HeroFace = Hero->GetCastFace().ToString();
	// Giving up before the first step: the hero as they were.
	auto Abandon = [&S, Hero](const FString& Why) {
		if (S->bCreated && S->Groom.IsValid())
		{
			Hero->DropGroom(S->Groom.Get());
		}
		if (S->bRedressed)
		{
			ss::hero::Character As;
			As.Appearance = S->OrigLook;
			As.Age = S->OrigAge;
			Hero->ApplyLook(As);
		}
		return Why;
	};
	UGroomComponent* Groom = Hero->ScalpGroom();
	if (!Groom || !Groom->GroomAsset)
	{
		// The look wears no scalp hair (bald, or full hair under a hat): the face's own, from its assembly, on a groom added
		// for the spike and taken off after.
		FSoftObjectPath OwnGroom;
		FSoftObjectPath OwnBinding;
		FString Why;
		if (!FindScalp(S->HeroFace, OwnGroom, OwnBinding, Why))
		{
			return Abandon(FString::Printf(TEXT("GroomSpike: the hero wears no hair and their face's own can't be found: %s"), *Why));
		}
		S->OrigGroom.Reset(Cast<UGroomAsset>(OwnGroom.TryLoad()));
		S->OrigBinding.Reset(Cast<UGroomBindingAsset>(OwnBinding.TryLoad()));
		if (!S->OrigGroom.IsValid())
		{
			return Abandon(FString::Printf(TEXT("GroomSpike: %s didn't load"), *OwnGroom.ToString()));
		}
		if (!Groom)
		{
			Groom = Hero->AddScalpGroom();
			S->bCreated = true;
		}
	}
	else
	{
		S->OrigGroom.Reset(Groom->GroomAsset.Get());
		S->OrigBinding.Reset(Groom->BindingAsset.Get());
		for (int32 Slot = 0; Slot < Groom->GetNumMaterials(); ++Slot)
		{
			S->OrigMaterials.Emplace(Groom->GetMaterial(Slot));
		}
	}
	if (!Groom)
	{
		return Abandon(TEXT("GroomSpike: no groom component to swap on"));
	}
	S->Groom = Groom;
	S->OrigLod = Groom->GetForcedLOD();

	auto Own = [](const TCHAR* Label) {
		FStep Step;
		Step.Label = Label;
		Step.bOriginal = true;
		return Step;
	};
	TArray<FString> Skipped;
	auto AddCandidate = [&](const FString& Label, const FSoftObjectPath& GroomPath, const FSoftObjectPath& BindingPath, bool bNoBinding, bool bShotsHere) {
		for (int32 I = 0; I < Repeat; ++I)
		{
			FStep Step;
			Step.Label = Label;
			Step.GroomPath = GroomPath;
			Step.BindingPath = BindingPath;
			Step.bNoBinding = bNoBinding;
			Step.bShots = bShotsHere && I == 0;
			S->Steps.Add(Step);
			FStep Back = Own(TEXT("back"));
			Back.bShots = bShotsHere && I == 0;
			S->Steps.Add(Back);
		}
	};
	auto AddCast = [&](const FString& Name, bool bShotsHere) {
		FSoftObjectPath GroomPath;
		FSoftObjectPath BindingPath;
		FString Why;
		if (!FindScalp(Name, GroomPath, BindingPath, Why))
		{
			Skipped.Add(FString::Printf(TEXT("%s (%s)"), *Name, *Why));
			return false;
		}
		const bool bNoBinding = BindingOption.Equals(TEXT("none"), ESearchCase::IgnoreCase);
		AddCandidate(Name, GroomPath, BindingOption.IsEmpty() || bNoBinding ? BindingPath : ObjectPathOf(BindingOption), bNoBinding, bShotsHere);
		return true;
	};

	if (Candidate.Equals(TEXT("self"), ESearchCase::IgnoreCase) || Candidate.Equals(TEXT("back"), ESearchCase::IgnoreCase))
	{
		for (int32 I = 0; I < Repeat; ++I)
		{
			FStep Step = Own(*Candidate.ToLower());
			Step.bShots = bShots && I == 0;
			S->Steps.Add(Step);
		}
	}
	else if (Candidate.Equals(TEXT("all"), ESearchCase::IgnoreCase))
	{
		for (int32 I = 0; I < Repeat; ++I)
		{
			FStep Step = Own(TEXT("self"));
			Step.bShots = bShots && I == 0;
			S->Steps.Add(Step);
		}
		// The other heroes of the hero's body type: the same topology and LODs, a different assembly.
		const TCHAR Body = S->HeroFace.StartsWith(TEXT("HeroB")) ? TEXT('B') : TEXT('A');
		for (int32 Band = 0; Band < 3; ++Band)
		{
			const FString Name = FString::Printf(TEXT("Hero%c%d"), Body, Band);
			if (Name != S->HeroFace)
			{
				AddCast(Name, bShots);
			}
		}
	}
	else if (Candidate.StartsWith(TEXT("/")))
	{
		const FSoftObjectPath GroomPath = ObjectPathOf(Candidate);
		const FString Package = GroomPath.GetLongPackageName();
		FSoftObjectPath BindingPath;
		const bool bNoBinding = BindingOption.Equals(TEXT("none"), ESearchCase::IgnoreCase);
		if (!BindingOption.IsEmpty() && !bNoBinding)
		{
			BindingPath = ObjectPathOf(BindingOption);
		}
		else if (!bNoBinding && FPackageName::DoesPackageExist(Package + TEXT("_Binding")))
		{
			BindingPath = ObjectPathOf(Package + TEXT("_Binding"));
		}
		AddCandidate(FPackageName::GetShortName(Package), GroomPath, BindingPath, bNoBinding || !BindingPath.IsValid(), bShots);
	}
	else
	{
		AddCast(Candidate, bShots);
	}
	if (S->Steps.Num() == 0)
	{
		return Abandon(FString::Printf(TEXT("GroomSpike: nothing to measure for '%s'%s%s"), *Candidate, Skipped.Num() ? TEXT(": ") : TEXT(""), *FString::Join(Skipped, TEXT("; "))));
	}

	// In third person, so the hair is drawn (first person keeps the head from the player's own view).
	S->bWasFirstPerson = Hero->IsFirstPerson();
	Hero->SetFirstPerson(false);
	const FString Plan = FString::Printf(TEXT("GroomSpike: hero %s, own groom %s (binding %s, LOD %d)%s; %d swaps, about %d s%s%s%s"), *S->HeroFace, *GetNameSafe(S->OrigGroom.Get()),
		*GetNameSafe(S->OrigBinding.Get()), S->OrigLod, S->bCreated ? TEXT(", added for the spike") : TEXT(""), S->Steps.Num(),
		FMath::CeilToInt(S->Steps.Num() * (BaseFrames + AfterFrames + SettleFrames) / 40.0f), S->bSync ? TEXT(", loading inside the call") : TEXT(""),
		Skipped.Num() ? TEXT("; skipped: ") : TEXT(""), *FString::Join(Skipped, TEXT("; ")));
	S->Lines.Add(Plan);
	S->BeginStep();
	GSpike = MoveTemp(S);
	GSpikeTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DeltaSeconds) {
		if (!GSpike)
		{
			return false;
		}
		const bool bMore = GSpike->Tick(DeltaSeconds);
		if (!bMore)
		{
			GSpike.Reset();
		}
		return bMore;
	}));
	return Plan;
}

void StopSpike()
{
	if (GSpike)
	{
		GSpike->Finish(true);
		GSpike.Reset();
	}
	FTSTicker::RemoveTicker(GSpikeTicker);
	GSpikeTicker.Reset();
}

bool IsSpikeRunning()
{
	return GSpike.IsValid();
}

FString SpikeReport()
{
	if (GSpike)
	{
		return FString::Join(GSpike->Lines, TEXT("\n")) + TEXT("\n(running)");
	}
	return GLastReport.IsEmpty() ? FString(TEXT("GroomSpike: not run yet (ss.Look.GroomSpike)")) : GLastReport;
}
} // namespace HeroGrooms
