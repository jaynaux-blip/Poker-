// The street's look (world realism spec 3.1-3.3, 0.7, 3.7, ue-world): the sky (a SkyAtmosphere, the sun on its October
// path matched to Session::Daylight's curve, the moon, volumetric cloud as thick as the weather, the city's sodium glow on
// it at night), two height-fog layers, exposure clamps and the grade by the sun's elevation, MegaLights on every local
// light; the weather (core's weather::At from the career's seed, or forced: ss.Street.Weather, ss.Street.Rain) eased
// into the rain, the wet, the puddles, the lightning and its thunder, and MPC_NS, which every M_NS_ material reads; the
// shops lit by their hours (Ns::FLit), the corner's signals (town::Signal, held through a skip), the stand-in windows
// by the hour, the bank's clock; the street's sound in stereo with its positioned voices; and the street's test hooks
// (ss.Street.Hour, ss.Street.Weather, ss.Street.Rain, ss.Street.Budget, ss.Street.Look).
#include "StreetStage.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "MarketStage.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/App.h"
#include "NightOneAudio.h"
#include "RHI.h"
#include "RenderTimer.h"
#include "RenderingThread.h"
#include "ShopStage.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/ShopArt.h"
#include "ShortStackCharacter.h"
#include "Slate/WidgetRenderer.h"
#include "SlateDrawList.h"
#include "StreetAudio.h"
#include "GameFramework/GameModeBase.h"
#include "StreetTraffic.h"

namespace StreetLookDetail
{
TAutoConsoleVariable<float> CVarHour(TEXT("ss.Street.Hour"), -1.0f, TEXT("The look's hour (0..24: the sky, the lamps, the windows, the shops' lights); -1 (default): the clock's. The session's clock is untouched."));
TAutoConsoleVariable<FString> CVarWeather(TEXT("ss.Street.Weather"), TEXT("auto"), TEXT("Forces the weather: clear, overcast, drizzle, rain, storm; auto (default): the career's (spec 3.3)."));
TAutoConsoleVariable<float> CVarRain(TEXT("ss.Street.Rain"), -1.0f, TEXT("Forces the rain's strength (0 none .. 1 a downpour); -1 (default): the weather's."));
TAutoConsoleVariable<float> CVarNightEV(TEXT("ss.Street.NightEV"), 0.0f, TEXT("Stops added to the street's night exposure clamps (+ darker)."));
TAutoConsoleVariable<float> CVarDayEV(TEXT("ss.Street.DayEV"), 0.0f, TEXT("Stops added to the street's day exposure clamps (+ darker)."));
TAutoConsoleVariable<float> CVarGlow(TEXT("ss.Street.Glow"), 1.0f, TEXT("A scale on the street's glows (the signs, the lamps' lenses, the neon, the signals)."));
TAutoConsoleVariable<int32> CVarClouds(TEXT("ss.Street.Clouds"), 1, TEXT("The volumetric clouds (0 off, for the budget's A/B)."));
// The day-one baseline's A/B (spec 3.8, 10): the street measured with and without MegaLights and the trees (and the
// clouds, ss.Street.Clouds), in Standalone at W1, W2 and inside the store.
TAutoConsoleVariable<int32> CVarMegaLights(TEXT("ss.Street.MegaLights"), 1, TEXT("MegaLights on the street's lights (1, spec 3.1) or the deferred lights (0), for the budget's A/B."));
TAutoConsoleVariable<int32> CVarTrees(TEXT("ss.Street.Trees"), 1, TEXT("The street trees, Fifth's and Market's (0 hides them, for the budget's A/B)."));
TAutoConsoleVariable<int32> CVarCrowdOffMarket(TEXT("ss.Street.CrowdOffMarket"), 0,
	TEXT("1: the crowd's people are hidden while on Market's roadway (a guard for a crowd walking The Block's hard-coded Market lines); 0 (default): the crowd walks the registry's walkways."));
// Test overrides start each Play session at their resets (FStreetOverrides).
FStreetOverrides::FRegister HourOverride(TEXT("ss.Street.Hour"), TEXT("-1"));
FStreetOverrides::FRegister WeatherOverride(TEXT("ss.Street.Weather"), TEXT("auto"));
FStreetOverrides::FRegister RainOverride(TEXT("ss.Street.Rain"), TEXT("-1"));

// ------------------------------------------------------------------ the sun (spec 3.2)
// October at 41 N. Session::Daylight (life::Daylight) climbs from 0 at 4:36 to 1 at 6:18 and falls from 18:30 to 20:00:
// matched exactly with the sun up from 6:18 to 18:30 (Daylight 1 while it is) and the dark coming at -18 degrees (night's
// end of astronomical twilight, where Daylight reaches 0): solar noon 12:24, a half day of 6.1 h, the declination that
// gives it (+1.7 degrees).
const double Latitude = 41.0;
const double Declination = 1.725;
const double SolarNoon = 12.4;

void SunAt(double Hour, double& OutEl, double& OutAz)
{
	const double L = FMath::DegreesToRadians(Latitude);
	const double D = FMath::DegreesToRadians(Declination);
	const double Ha = FMath::DegreesToRadians(15.0 * (Hour - SolarNoon));
	const double SinE = FMath::Sin(L) * FMath::Sin(D) + FMath::Cos(L) * FMath::Cos(D) * FMath::Cos(Ha);
	const double E = FMath::Asin(FMath::Clamp(SinE, -1.0, 1.0));
	OutEl = FMath::RadiansToDegrees(E);
	const double CosA = (FMath::Sin(D) - SinE * FMath::Sin(L)) / FMath::Max(1e-6, FMath::Cos(E) * FMath::Cos(L));
	OutAz = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosA, -1.0, 1.0)));
	if (FMath::Sin(Ha) > 0.0)
	{
		OutAz = 360.0 - OutAz;
	}
}

/** Toward a body at elevation El and compass azimuth Az, in the stage's frame (+X west, +Y north, -X east). */
FVector Toward(double El, double Az)
{
	const double E = FMath::DegreesToRadians(El);
	const double A = FMath::DegreesToRadians(Az);
	return FVector(-FMath::Sin(A) * FMath::Cos(E), FMath::Cos(A) * FMath::Cos(E), FMath::Sin(E));
}

/** 0 above Hi .. 1 below Lo (degrees of sun elevation). */
float Below(double El, double Hi, double Lo)
{
	return static_cast<float>(FMath::Clamp((Hi - El) / FMath::Max(0.001, Hi - Lo), 0.0, 1.0));
}

float Luma(const FLinearColor& C)
{
	return FMath::Max(0.02f, 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B);
}

/** The glow families' levels (nits) by the sun: deep night, dusk, the low sun, day. */
struct FGlowRow
{
	double El;
	float Warm, Cool, Neon, Signal, Lamps, Chase;
};
const FGlowRow GlowRows[4] = {{12.0, 300.0f, 600.0f, 900.0f, 3000.0f, 2000.0f, 500.0f}, {4.0, 120.0f, 180.0f, 220.0f, 900.0f, 600.0f, 180.0f},
	{-2.0, 25.0f, 35.0f, 45.0f, 140.0f, 140.0f, 40.0f}, {-8.0, 8.0f, 10.0f, 14.0f, 45.0f, 45.0f, 12.0f}};

Ns::FGlowLevels GlowAt(double El)
{
	Ns::FGlowLevels G;
	const FGlowRow* A = &GlowRows[3];
	const FGlowRow* B = &GlowRows[3];
	float T = 0.0f;
	if (El >= GlowRows[0].El)
	{
		A = B = &GlowRows[0];
	}
	else
	{
		for (int32 I = 0; I < 3; ++I)
		{
			if (El >= GlowRows[I + 1].El)
			{
				A = &GlowRows[I];
				B = &GlowRows[I + 1];
				T = static_cast<float>((A->El - El) / (A->El - B->El));
				break;
			}
		}
	}
	G.Warm = FMath::Lerp(A->Warm, B->Warm, T);
	G.Cool = FMath::Lerp(A->Cool, B->Cool, T);
	G.Neon = FMath::Lerp(A->Neon, B->Neon, T);
	G.Signal = FMath::Lerp(A->Signal, B->Signal, T);
	G.Lamps = FMath::Lerp(A->Lamps, B->Lamps, T);
	G.Chase = FMath::Lerp(A->Chase, B->Chase, T);
	return G;
}

/** A forced weather state (ss.Street.Weather) as the model's outputs would settle on it (spec 3.3's table). */
bool ForcedWeather(const FString& Word, ss::weather::Now& Out)
{
	static const TCHAR* const Words[5] = {TEXT("clear"), TEXT("overcast"), TEXT("drizzle"), TEXT("rain"), TEXT("storm")};
	static const double Rain[5] = {0.0, 0.0, 0.3, 0.75, 1.0};
	static const double Cloud[5] = {0.2, 0.85, 0.9, 0.95, 1.0};
	static const double Wet[5] = {0.03, 0.15, 0.62, 0.95, 1.0};
	static const double Wind[5] = {0.15, 0.3, 0.35, 0.5, 0.6};
	static const double Fog[5] = {0.1, 0.25, 0.35, 0.45, 0.6};
	for (int32 I = 0; I < 5; ++I)
	{
		if (Word.Equals(Words[I], ESearchCase::IgnoreCase))
		{
			Out.State = static_cast<ss::weather::Sky>(I);
			Out.Rain = Rain[I];
			Out.Cloud = Cloud[I];
			Out.Wetness = Wet[I];
			Out.Puddle = FMath::SmoothStep(0.55, 1.0, Wet[I]);
			Out.Wind = Wind[I];
			// Radians in the stage frame, the way it blows (Weather.h): from the west, as the model's mostly is.
			Out.WindDir = UE_DOUBLE_PI;
			Out.Fog = Fog[I];
			Out.Flash = 0.0;
			Out.ThunderIn = -1.0;
			return true;
		}
	}
	return false;
}

/** An apartment's chance of a lamp on at an hour (spec 3.4), for the stand-in windows (the kit's do it in M_NS_WindowRoom). */
float Occupancy(double Hr)
{
	if (Hr >= 19.0 && Hr < 23.0)
	{
		return 0.55f;
	}
	if (Hr >= 23.0 || Hr < 1.0)
	{
		return 0.35f;
	}
	if (Hr < 2.0)
	{
		return 0.25f;
	}
	if (Hr < 5.0)
	{
		return 0.08f;
	}
	if (Hr < 6.0)
	{
		return 0.15f;
	}
	if (Hr < 8.0)
	{
		return 0.3f;
	}
	return Hr < 17.5 ? 0.15f : 0.35f;
}

/** The emissive parameters an imported material glows by (an emissive factor that isn't black, else a strength). */
bool EmissiveOf(const UMaterialInterface* M, TArray<TPair<FName, FLinearColor>>& Colors, TArray<TPair<FName, float>>& Strengths)
{
	Colors.Reset();
	Strengths.Reset();
	if (!M)
	{
		return false;
	}
	auto Key = [](const FName& Name) { return Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT("")); };
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	M->GetAllVectorParameterInfo(Infos, Ids);
	bool bFactor = false;
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString K = Key(Info.Name);
		FLinearColor C;
		if ((K.StartsWith(TEXT("emissivefactor")) || K == TEXT("emissivecolor")) && M->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), C))
		{
			bFactor = true;
			if (!C.IsAlmostBlack())
			{
				Colors.Emplace(Info.Name, C);
			}
		}
	}
	if (!bFactor)
	{
		Infos.Reset();
		Ids.Reset();
		M->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			const FString K = Key(Info.Name);
			float V = 0.0f;
			if ((K == TEXT("emissivestrength") || K == TEXT("emissiveintensity")) && M->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), V) && V > 0.0f)
			{
				Strengths.Emplace(Info.Name, V);
			}
		}
	}
	return Colors.Num() > 0 || Strengths.Num() > 0;
}

/** One slot of an imported mesh lit at Level times its bake (a material of its own, made fresh and never saved). */
void ScaleSlotGlow(UStaticMeshComponent* Mesh, int32 Slot, float Level)
{
	const UStaticMesh* Source = Mesh ? Mesh->GetStaticMesh().Get() : nullptr;
	if (!Source || !Source->GetStaticMaterials().IsValidIndex(Slot))
	{
		return;
	}
	UMaterialInterface* Baked = Source->GetMaterial(Slot);
	TArray<TPair<FName, FLinearColor>> Colors;
	TArray<TPair<FName, float>> Strengths;
	if (!EmissiveOf(Baked, Colors, Strengths))
	{
		return;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot));
	if (!Mid || Mid->Parent != Baked)
	{
		Mid = UMaterialInstanceDynamic::Create(Baked, Mesh);
		Mid->SetFlags(RF_Transient);
		Mesh->SetMaterial(Slot, Mid);
	}
	for (const TPair<FName, FLinearColor>& C : Colors)
	{
		Mid->SetVectorParameterValue(C.Key, FLinearColor(C.Value.R * Level, C.Value.G * Level, C.Value.B * Level, C.Value.A));
	}
	for (const TPair<FName, float>& S : Strengths)
	{
		Mid->SetScalarParameterValue(S.Key, S.Value * Level);
	}
}

/** The game mode's audio component (the street's sound plays through it; AStreetGameMode's default subobject), or null. */
UNightOneAudio* StreetAudioOf(const UWorld* World)
{
	const AGameModeBase* Mode = World ? World->GetAuthGameMode() : nullptr;
	return Mode ? Mode->FindComponentByClass<UNightOneAudio>() : nullptr;
}

/** The street stage in World (the console's hooks act on it). */
AStreetStage* StageIn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AStreetStage> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

FAutoConsoleCommandWithWorldAndArgs CmdBudget(TEXT("ss.Street.Budget"), TEXT("The frame's cost and what's drawing it on the street (lights in range, MetaHumans by tier, translucency)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (const AStreetStage* Stage = StageIn(World))
		{
			UE_LOG(LogTemp, Display, TEXT("%s"), *Stage->TestBudget());
		}
	}));
FAutoConsoleCommandWithWorldAndArgs CmdLook(TEXT("ss.Street.Look"), TEXT("The street's look now: the hour, the sun, the weather, the exposure's clamps, Market's width, the plots."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (const AStreetStage* Stage = StageIn(World))
		{
			UE_LOG(LogTemp, Display, TEXT("%s"), *Stage->TestLook());
		}
	}));
} // namespace StreetLookDetail

using namespace StreetLookDetail;

// ------------------------------------------------------------------ Ns::FLit

int32 Ns::FLit::AddGlow(UObject* Outer, UStaticMeshComponent* Mesh, UMaterialInterface* GlowMaterial, EGlow Family, const FLinearColor& Color, float Scale, int32 Shop, bool bDay)
{
	if (!Mesh || !Outer)
	{
		return INDEX_NONE;
	}
	FGlow G;
	G.Mesh = Mesh;
	G.Family = Family;
	G.Color = Color;
	G.Scale = Scale;
	G.Shop = Shop;
	G.bDay = bDay;
	if (GlowMaterial)
	{
		// The kit's glow mesh: one M_NS_Glow over all of it, its color set here and its level by the hour.
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(GlowMaterial, Outer);
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
		Mid->SetScalarParameterValue(TEXT("Mode"), Family == EGlow::Chase ? 3.0f : 0.0f);
		Mid->SetScalarParameterValue(TEXT("Level"), 0.0f);
		Mid->SetScalarParameterValue(TEXT("Flicker"), Family == EGlow::Neon ? 0.04f : 0.0f);
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			Mesh->SetMaterial(Slot, Mid);
		}
		G.Mid = Mid;
		G.bNsGlow = true;
		return Glows.Add(G);
	}
	// An M_Street part: its Emissive is the level.
	if (UMaterialInstanceDynamic* Own = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
	{
		float Emissive = 0.0f;
		if (Own->GetScalarParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(FName(TEXT("Emissive")))), Emissive) &&
			Own->GetScalarParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(FName(TEXT("Pattern")))), Emissive))
		{
			G.Mid = Own;
			G.bStreetEmissive = true;
			return Glows.Add(G);
		}
	}
	// An imported glow (a lamp's lens, a bank's floods): its first glowing slot's bake, scaled.
	const UStaticMesh* Source = Mesh->GetStaticMesh();
	for (int32 Slot = 0; Source && Slot < Source->GetStaticMaterials().Num(); ++Slot)
	{
		UMaterialInterface* Baked = Source->GetMaterial(Slot);
		if (!EmissiveOf(Baked, G.Colors, G.Strengths))
		{
			continue;
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Baked, Outer);
		Mid->SetFlags(RF_Transient);
		Mesh->SetMaterial(Slot, Mid);
		G.Mid = Mid;
		return Glows.Add(G);
	}
	return INDEX_NONE;
}

void Ns::FLit::AddLight(int32 Shop, ULightComponent* L, float Level)
{
	if (Shops.IsValidIndex(Shop) && L)
	{
		Shops[Shop].Lights.Add(L);
		Shops[Shop].Levels.Add(Level);
	}
}

void Ns::FLit::Update(double World, const FGlowLevels& Levels, float Dt, bool bForce)
{
	for (FShop& S : Shops)
	{
		const bool bOpen = S.OpenAt(World);
		const int8 Want = bOpen ? 1 : 0;
		if (bForce || Want != S.Open || FMath::Abs(S.NightApplied - Levels.Night) > 0.01f)
		{
			S.Open = Want;
			S.NightApplied = Levels.Night;
			// Its spill and sign lights: on while it's open, from dusk to dawn.
			const float K = bOpen ? Levels.Night : 0.0f;
			for (int32 I = 0; I < S.Lights.Num(); ++I)
			{
				if (ULightComponent* L = S.Lights[I].Get())
				{
					L->SetIntensity(S.Levels[I] * K);
					L->SetVisibility(K > 0.01f);
				}
			}
			// Its displays behind the glass: lit while it's open (a night light's glimmer while it's shut).
			if (UStaticMeshComponent* Trim = S.Trim.Get())
			{
				ScaleSlotGlow(Trim, S.TrimSlot, bOpen ? Levels.LitDisplays : 0.04f);
			}
		}
		// The grille rolls up into its hood while it's open (2 s), down while it's shut.
		if (USceneComponent* G = S.Grille.Get())
		{
			const float Target = bOpen ? 1.0f : 0.0f;
			if (S.GrilleOpen != Target || bForce)
			{
				S.GrilleOpen = (bForce || Dt <= 0.0f) ? Target : FMath::FInterpConstantTo(S.GrilleOpen, Target, Dt, 0.5f);
				G->SetRelativeScale3D(FVector(1.0, 1.0, FMath::Max(0.03f, 1.0f - S.GrilleOpen)));
				G->SetVisibility(S.GrilleOpen < 0.97f);
			}
		}
	}
	for (FGlow& G : Glows)
	{
		UMaterialInstanceDynamic* Mid = G.Mid.Get();
		if (!Mid)
		{
			continue;
		}
		float Nits = Levels.Warm;
		switch (G.Family)
		{
		case EGlow::Cool:
			Nits = Levels.Cool;
			break;
		case EGlow::Neon:
			Nits = Levels.Neon;
			break;
		case EGlow::Signal:
			Nits = Levels.Signal;
			break;
		case EGlow::Lamps:
			Nits = Levels.Lamps;
			break;
		case EGlow::Chase:
			Nits = Levels.Chase;
			break;
		default:
			break;
		}
		const bool bShopOpen = G.Shop < 0 || IsOpen(G.Shop, World);
		const float K = !bShopOpen ? 0.0f : (G.Shop >= 0 && G.bDay ? 1.0f : Levels.Night);
		const float Level = (G.bNsGlow || G.bStreetEmissive ? Nits / Luma(G.Color) : 1.0f) * G.Scale * K * Levels.Exposure;
		if (!bForce && FMath::Abs(Level - G.Applied) <= 0.01f * FMath::Max(Level, G.Applied) + 1e-4f)
		{
			continue;
		}
		G.Applied = Level;
		if (G.bNsGlow)
		{
			Mid->SetScalarParameterValue(TEXT("Level"), Level);
			if (G.Family == EGlow::Chase)
			{
				Mid->SetScalarParameterValue(TEXT("ChaseRate"), bShopOpen && G.Shop >= 0 ? 1.6f : 0.0f);
			}
		}
		else if (G.bStreetEmissive)
		{
			Mid->SetScalarParameterValue(TEXT("Emissive"), Level);
			Mid->SetScalarParameterValue(TEXT("Strength"), Level);
		}
		else
		{
			for (const TPair<FName, FLinearColor>& C : G.Colors)
			{
				Mid->SetVectorParameterValue(C.Key, FLinearColor(C.Value.R * Level, C.Value.G * Level, C.Value.B * Level, C.Value.A));
			}
			for (const TPair<FName, float>& S : G.Strengths)
			{
				Mid->SetScalarParameterValue(S.Key, S.Value * Level);
			}
		}
	}
}

// ------------------------------------------------------------------ Ns::FScreens

FString Ns::PlateFor(int32 Set, int32 Index)
{
	const uint64 Seed = 0x5eedull * 1000003ull + static_cast<uint64>(Set) * 7919ull + static_cast<uint64>(FMath::Max(0, Index));
	return UTF8_TO_TCHAR(ss::ui::shopart::PlateText(Seed).c_str());
}

bool Ns::FScreens::Paint(UObject* Outer, UStaticMeshComponent* Mesh, const TCHAR* Slot, const FString& Key, int32 Pixels, EScreen How, const FLinearColor& Back)
{
	if (!Outer || !Mesh || !FApp::CanEverRender() || !FSlateTextMeasurer::IsAvailable())
	{
		return false;
	}
	const int32 Index = AStreetStage::NsSlot(Mesh, Slot);
	UMaterialInterface* Base = Index == INDEX_NONE ? nullptr : Cast<UMaterialInterface>(AStreetStage::NsAsset(ScreenPath));
	float W = 0.0f;
	float H = 0.0f;
	if (!Base || !ss::ui::shopart::FaceSize(TCHAR_TO_UTF8(*Key), W, H) || W <= 1.0f || H <= 1.0f)
	{
		return false;
	}
	// Its own target at the face's proportions (as the widget component makes one: the canvas draws into it through Slate).
	const int32 Px = FMath::Clamp(Pixels, 64, 2048);
	const int32 Py = FMath::Clamp(FMath::RoundToInt32(static_cast<float>(Px) * H / W), 16, 2048);
	// Never saved, and the editor's preview's never copied into Play-In-Editor (its play copy draws its own, as the parts are built).
	const UWorld* InWorld = Outer->GetWorld();
	const EObjectFlags Flags = InWorld && InWorld->IsGameWorld() ? RF_Transient : EObjectFlags(RF_Transient | RF_DuplicateTransient);
	UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Outer, NAME_None, Flags);
	Target->ClearColor = FLinearColor::Transparent;
	Target->InitCustomFormat(Px, Py, PF_B8G8R8A8, false);
	Target->UpdateResourceImmediate(true);
	FFace Face;
	Face.Target = Target;
	Face.Key = Key;
	Face.How = How;
	// What moves with the clock: the timetable's next buses (by the minute), the marquee (by the day: Fridays).
	Face.bClock = Key.StartsWith(TEXT("timetable")) || Key.StartsWith(TEXT("marquee")) || Key.StartsWith(TEXT("clock"));
	if (!Draw(Face, LastWorld))
	{
		return false;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	Mid->SetFlags(Flags);
	Mid->SetTextureParameterValue(TEXT("Image"), Target);
	Mid->SetVectorParameterValue(TEXT("Back"), Back);
	Mid->SetScalarParameterValue(TEXT("Glow"), 0.0f);
	// Plates and printed faces a little glossy; the lit screens behind their glass glossier still.
	Mid->SetScalarParameterValue(TEXT("Rough"), How == EScreen::Printed ? 0.45f : 0.2f);
	Mesh->SetMaterial(Index, Mid);
	Face.Mid = Mid;
	Faces.Add(Face);
	return true;
}

bool Ns::FScreens::Draw(FFace& Face, double World)
{
	UTextureRenderTarget2D* Target = Face.Target.Get();
	if (!Target || !FSlateApplication::IsInitialized() || !FSlateTextMeasurer::IsAvailable())
	{
		return false;
	}
	namespace A = ss::ui::shopart;
	const std::string Key = TCHAR_TO_UTF8(*Face.Key);
	float W = 0.0f;
	float H = 0.0f;
	if (!A::FaceSize(Key, W, H))
	{
		return false;
	}
	const size_t Colon = Key.find(':');
	const std::string Pre = Key.substr(0, Colon);
	const std::string Arg = Colon == std::string::npos ? std::string() : Key.substr(Colon + 1);
	FSlateTextMeasurer Measurer;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas C(*List, Measurer, W, H, static_cast<float>(Target->SizeX) / W);
	if (Pre == "plate")
	{
		A::Plate(C, Arg);
	}
	else if (Pre == "timetable")
	{
		A::Timetable(C, Arg.empty() ? std::string("market-east") : Arg, World);
	}
	else if (Pre == "shelterad")
	{
		A::ShelterAd(C);
	}
	else if (Pre == "marquee")
	{
		A::Marquee(C, World);
	}
	else if (Pre == "valance")
	{
		A::Valance(C, Arg);
	}
	else if (Pre == "clock")
	{
		A::BankClock(C, World);
	}
	else if (Pre == "dest" || Pre == "destside")
	{
		A::DestinationSign(C, Arg, Pre == "destside");
	}
	else
	{
		return false;
	}
	if (!Renderer)
	{
		Renderer = new FWidgetRenderer(false, true);
	}
	const FVector2D Size(Target->SizeX, Target->SizeY);
	TSharedRef<SDrawListWidget> Widget = SNew(SDrawListWidget).DesiredSize(Size);
	Widget->SetDrawList(TSharedPtr<const ss::ui::DrawList>(List));
	Renderer->DrawWidget(Target, Widget, Size, 0.0f);
	return true;
}

void Ns::FScreens::Update(double World, const FGlowLevels& Levels)
{
	LastWorld = World;
	// The clock's faces again when the minute moves, at most twice a second (a skip races the minutes).
	const int64 Minute = static_cast<int64>(FMath::FloorToDouble(World));
	const double Now = FPlatformTime::Seconds();
	const bool bRedraw = Minute != DrawnMinute && Now - LastDraw >= 0.5;
	if (bRedraw)
	{
		DrawnMinute = Minute;
		LastDraw = Now;
	}
	for (int32 I = Faces.Num() - 1; I >= 0; --I)
	{
		FFace& F = Faces[I];
		UMaterialInstanceDynamic* Mid = F.Mid.Get();
		if (!Mid || !F.Target.IsValid())
		{
			Faces.RemoveAtSwap(I);
			continue;
		}
		if (bRedraw && F.bClock)
		{
			Draw(F, World);
		}
		// Backlit: the 14's screens by the cool family (lit at every hour); bulbs: the marquee's boards after dusk.
		const float Glow = F.How == EScreen::Backlit ? Levels.Cool * 0.5f : F.How == EScreen::Bulbs ? Levels.Warm * 0.6f * Levels.Night : 0.0f;
		if (FMath::Abs(Glow - F.Applied) > 0.01f * FMath::Max(Glow, F.Applied) + 1e-4f)
		{
			F.Applied = Glow;
			Mid->SetScalarParameterValue(TEXT("Glow"), Glow);
		}
	}
}

void Ns::FScreens::Reset()
{
	Faces.Reset();
	if (Renderer)
	{
		BeginCleanup(Renderer);
		Renderer = nullptr;
	}
}

// ------------------------------------------------------------------ the preload list's look

void AStreetStage::AppendLookPreload(TArray<FString>& Paths)
{
	Paths.Add(Ns::MpcPath);
	Paths.Add(Ns::SkyGlowPath);
	Paths.Add(Ns::RainFxPath);
	Paths.Add(Ns::ScreenPath);
	Paths.Add(Ns::GlowAddPath);
	for (const TCHAR* Mat : {TEXT("MI_NS_Ground_Sidewalk"), TEXT("MI_NS_Ground_Asphalt"), TEXT("MI_NS_Ground_Patch"), TEXT("MI_NS_Ground_Curb"), TEXT("MI_NS_Ground_Gutter"),
			 TEXT("MI_NS_Ground_Concrete"), TEXT("MI_NS_Ground_Tactile"), TEXT("MI_NS_Ground_Marking"), TEXT("MI_NS_Ground_BusBay"), TEXT("MI_NS_Masonry_BrickRed"),
			 TEXT("MI_NS_Masonry_BrickBuff"), TEXT("MI_NS_Masonry_BrickDark"), TEXT("MI_NS_Masonry_BrickPainted"), TEXT("MI_NS_Masonry_Limestone"),
			 TEXT("MI_NS_Masonry_CastStone"), TEXT("MI_NS_Masonry_Stucco"), TEXT("MI_NS_Masonry_Roof"), TEXT("MI_NS_Leaf")})
	{
		Paths.Add(FString::Printf(TEXT("/Game/ShortStack/Materials/NS/%s.%s"), Mat, Mat));
	}
}

void AStreetStage::NsAppendPreload(TArray<FString>& Paths)
{
	AppendGroundPreload(Paths);
	AppendFrontsPreload(Paths);
	AppendFurniturePreload(Paths);
	AppendLookPreload(Paths);
	AStreetTraffic::AppendPreload(Paths);
}

// ------------------------------------------------------------------ the sky (spec 3.2)

void AStreetStage::BuildSkyAndWeather()
{
	// The skyline beyond the blocks (today's cubes until the modelled sheets land, spec 2.2), none standing in a street.
	UInstancedStaticMeshComponent* Skyline = NewPart<UInstancedStaticMeshComponent>();
	Skyline->SetStaticMesh(CubeMesh);
	UMaterialInstanceDynamic* City = CityMaterial ? UMaterialInstanceDynamic::Create(CityMaterial, this) : nullptr;
	if (City)
	{
		City->SetScalarParameterValue(TEXT("Strength"), 0.55f);
		SkyMids.Add(City);
	}
	Skyline->SetMaterial(0, City ? static_cast<UMaterialInterface*>(City) : Mat(TEXT("CityFallback"), 0x0a0b0e, 0.9f));
	Skyline->SetCastShadow(false);
	Skyline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skyline->bAffectDistanceFieldLighting = false;
	const FBox2D Streets[] = {FBox2D(FVector2D(-2000.0, Ns::HollisS - 3000.0), FVector2D(4000.0, Ns::WexlerN + 3000.0)),
		FBox2D(FVector2D(-20000.0, Ns::HollisS - 2500.0), FVector2D(22000.0, Ns::HollisN + 2500.0)),
		FBox2D(FVector2D(-20000.0, Ns::WexlerS - 2500.0), FVector2D(22000.0, Ns::WexlerN + 2500.0)),
		FBox2D(FVector2D(-14000.0, Ns::StoreY1 - 3000.0), FVector2D(0.0, Ns::MarketLineN + 3000.0)),
		FBox2D(FVector2D(Ns::CardinalE - 3000.0, -8000.0), FVector2D(Ns::MarketEast + 2000.0, 20000.0))};
	uint32 Dice = 4242u;
	auto Next = [&Dice]() {
		Dice = Dice * 1664525u + 1013904223u;
		return static_cast<double>(Dice >> 8) / static_cast<double>(1u << 24);
	};
	for (int32 I = 0; I < 220; ++I)
	{
		const double A = Next() * 2.0 * PI;
		const double R = 9000.0 + Next() * 30000.0;
		const double Hgt = 1500.0 + Next() * Next() * 12000.0;
		const double Wd = 1200.0 + Next() * 2400.0;
		const float Yaw = static_cast<float>(Next() * 90.0);
		const FVector2D At(R * FMath::Cos(A), 2000.0 + R * FMath::Sin(A));
		const FBox2D Foot(At - FVector2D(Wd, Wd) * 0.75, At + FVector2D(Wd, Wd) * 0.75);
		bool bInStreet = false;
		for (const FBox2D& S : Streets)
		{
			bInStreet |= S.Intersect(Foot);
		}
		if (!bInStreet)
		{
			Skyline->AddInstance(FTransform(FRotator(0.0f, Yaw, 0.0f), FVector(At.X, At.Y, Hgt * 0.5), FVector(Wd / 100.0, Wd / 100.0, Hgt / 100.0)));
		}
	}

	// The rain sheets (StreetStage.cpp keeps their layers), and its hits on the ground and drips off the awnings.
	BuildRain();
	BuildRainFx();

	// The sun: up to 80,000 lux at the top of the atmosphere, which reddens and dims it low; virtual shadow maps; the
	// cloud's shadows. The moon: 0.25 lux, cool, shadowing only at night (the lightning's flash rides on it).
	Sun = NewPart<UDirectionalLightComponent>();
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(80000.0f);
	Sun->SetLightSourceAngle(0.5357f);
	Sun->SetCastShadows(true);
	Sun->bCastCloudShadows = true;
	Sun->SetVolumetricScatteringIntensity(1.0f);
	Sun->MarkRenderStateDirty();
	Moon = NewPart<UDirectionalLightComponent>();
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(0.25f);
	Moon->SetLightColor(Ns::Srgb(0xb8c8ff));
	Moon->SetLightSourceAngle(1.0f);
	Moon->SetCastShadows(true);
	Moon->SetAtmosphereSunDiskColorScale(FLinearColor::Black);
	Moon->SetVolumetricScatteringIntensity(0.5f);
	Moon->SetRelativeRotation(FRotationMatrix::MakeFromX(-Toward(32.0, 230.0)).Rotator());
	// The atmosphere, its planet's top at the sidewalk.
	Atmosphere = NewPart<USkyAtmosphereComponent>();
	Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetTopAtComponentTransform;
	Atmosphere->SetAerialPespectiveViewDistanceScale(1.0f);
	Atmosphere->MarkRenderStateDirty();
	// Volumetric cloud from the engine's simple material (resolved in the editor and saved: CloudMaterial), low and thick
	// in overcast and rain (its base 1.2..2 km), thin and high when it's clear.
	UMaterialInterface* CloudBase = CloudMaterial ? CloudMaterial.Get() : Cast<UMaterialInterface>(NsAsset(Ns::CloudPath));
	if (CloudBase)
	{
		Clouds = NewPart<UVolumetricCloudComponent>();
		CloudMid = UMaterialInstanceDynamic::Create(CloudBase, this);
		Clouds->SetMaterial(CloudMid);
		Clouds->SetLayerBottomAltitude(1.6f);
		Clouds->SetLayerHeight(3.0f);
		Clouds->SetViewSampleCountScale(0.5f);
		Clouds->SetShadowViewSampleCountScale(0.5f);
		Clouds->SetReflectionViewSampleCountScale(0.25f);
		Clouds->SetShadowReflectionViewSampleCountScale(0.25f);
	}
	// The sky's light, captured from the sky alone in real time (the hour moves it every frame).
	SkyLight = NewPart<USkyLightComponent>();
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLight->SetRealTimeCapture(true);
	SkyLight->bLowerHemisphereIsBlack = false;
	SkyLight->SetLowerHemisphereColor(Ns::Srgb(0x1a1612) * 0.3f);
	SkyLight->SetIntensity(1.0f);
	SkyLight->MarkRenderStateDirty();
	// The city's sodium glow on the low cloud at night: an additive sphere past everything (M_NS_SkyGlow).
	if (UMaterialInterface* GlowSky = Cast<UMaterialInterface>(NsAsset(Ns::SkyGlowPath)))
	{
		SkyGlowMid = UMaterialInstanceDynamic::Create(GlowSky, this);
		UStaticMeshComponent* Dome = NewPart<UStaticMeshComponent>();
		Dome->SetStaticMesh(SphereMesh);
		Dome->SetMaterial(0, SkyGlowMid);
		Dome->SetRelativeScale3D(FVector(100000.0));
		Dome->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dome->SetCastShadow(false);
		Dome->SetVisibleInRayTracing(false);
		Dome->bAffectDistanceFieldLighting = false;
		Dome->bAffectDynamicIndirectLighting = false;
	}
	// Two height fogs (spec 3.2): the base, density 0.02 with a falloff of 0.02 (never 0.2 or more: the Embercrest's lesson),
	// and the street haze in rain, low and dense (0.06, 0.3, 50 cm down), which thins the far ends of the streets.
	// Volumetric fog shows the lamps' cones, its extinction from the weather.
	Fog = NewPart<UExponentialHeightFogComponent>();
	Fog->SetFogDensity(0.02f);
	Fog->SetFogHeightFalloff(0.02f);
	Fog->SetFogMaxOpacity(0.92f);
	Fog->SetStartDistance(200.0f);
	Fog->SetFogCutoffDistance(4000000.0f);
	Fog->SetFogInscatteringColor(FLinearColor(0.012f, 0.014f, 0.022f));
	Fog->SetSkyAtmosphereAmbientContributionColorScale(FLinearColor::White);
	FExponentialHeightFogData Haze;
	Haze.FogDensity = 0.0f;
	Haze.FogHeightFalloff = 0.3f;
	Haze.FogHeightOffset = -50.0f;
	Fog->SetSecondFogData(Haze);
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.6f);
	Fog->SetVolumetricFogExtinctionScale(1.0f);
	Fog->SetVolumetricFogAlbedo(FColor(190, 196, 205));
	Fog->SetVolumetricFogDistance(4000.0f);

	// The lens: MegaLights on every local light (spec 3.1), Lumen's front-layer reflections on the shop glass and rough
	// reflections up to 0.4; auto exposure inside the hour's clamps (ApplyHour, ApplyLens); the grade by the sun and the rain.
	Lens = NewPart<UPostProcessComponent>();
	Lens->bUnbound = true;
	FPostProcessSettings& S = Lens->Settings;
	S.bOverride_bMegaLights = true;
	S.bMegaLights = 1;
	S.bOverride_LumenFrontLayerTranslucencyReflections = true;
	S.LumenFrontLayerTranslucencyReflections = 1;
	S.bOverride_LumenMaxRoughnessToTraceReflections = true;
	S.LumenMaxRoughnessToTraceReflections = 0.4f;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	S.bOverride_AutoExposureLowPercent = true;
	S.AutoExposureLowPercent = 10.0f;
	S.bOverride_AutoExposureHighPercent = true;
	S.AutoExposureHighPercent = 90.0f;
	S.bOverride_HistogramLogMin = true;
	S.HistogramLogMin = -4.0f;
	S.bOverride_HistogramLogMax = true;
	S.HistogramLogMax = 16.0f;
	S.bOverride_AutoExposureMinBrightness = true;
	S.bOverride_AutoExposureMaxBrightness = true;
	S.bOverride_AutoExposureBias = true;
	S.bOverride_AutoExposureSpeedUp = true;
	S.AutoExposureSpeedUp = 2.5f;
	S.bOverride_AutoExposureSpeedDown = true;
	S.AutoExposureSpeedDown = 1.4f;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = 0;
	S.bOverride_LocalExposureHighlightContrastScale = true;
	S.LocalExposureHighlightContrastScale = 0.75f;
	S.bOverride_LocalExposureShadowContrastScale = true;
	S.LocalExposureShadowContrastScale = 0.95f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.7f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.4f;
	S.bOverride_FilmGrainIntensity = true;
	S.bOverride_SceneFringeIntensity = true;
	S.bOverride_MotionBlurAmount = true;
	S.bOverride_WhiteTemp = true;
	S.bOverride_ColorSaturation = true;
	S.bOverride_ColorContrast = true;
	S.bOverride_ColorGainHighlights = true;
	S.bOverride_ColorOffsetShadows = true;
	S.bOverride_ColorSaturationShadows = true;
	ApplyLens();
}

// ------------------------------------------------------------------ play: the neighbors, the cvars

void AStreetStage::SpawnNeighbors()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	const FTransform& T = GetActorTransform();
	// The shops on this stage's plots (spec 7.5.6): the barber's in the walk-up's bay, Ruby's on her corner (Market at its new width).
	if (AShopStage::ClassFor(TEXT("barber")))
	{
		AShopStage::FindOrSpawnVenue(World, TEXT("barber"), T);
	}
	if (bWideMarket && AShopStage::ClassFor(TEXT("diner")))
	{
		AShopStage::FindOrSpawnVenue(World, TEXT("diner"), T);
	}
	// Market's stage builds Market and Cardinal (and spawns their three shops) at its new width.
	if (bWideMarket)
	{
		Market = AMarketStage::FindOrSpawn(World, T, this);
	}
	// The cross streets' cars (spec 3.6).
	Traffic = AStreetTraffic::FindOrSpawn(World, this);
	// What the crowd walks (spec 6.8): Market's sidewalks from the registry (AMarketStage registers them). The guard that
	// keeps its people off Market's roadway is for a crowd still on today's lines (ss.Street.CrowdOffMarket 1).
	bCrowdOnOldLines = bWideMarket && CVarCrowdOffMarket.GetValueOnGameThread() != 0;
	// Lumen's far field for the skyline and the long views down the cross streets (here only: put back as the level ends).
	SetCvar(TEXT("r.LumenScene.FarField"), TEXT("1"));
}

void AStreetStage::SetCvar(const TCHAR* Name, const TCHAR* Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		SavedCvars.Emplace(FString(Name), Var->GetString());
		Var->Set(Value, ECVF_SetByCode);
	}
}

void AStreetStage::EndPlay(const EEndPlayReason::Type Reason)
{
	for (int32 I = SavedCvars.Num() - 1; I >= 0; --I)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(*SavedCvars[I].Key))
		{
			Var->Set(*SavedCvars[I].Value, ECVF_SetByCode);
		}
	}
	SavedCvars.Reset();
	for (const TWeakObjectPtr<AActor>& A : CrowdHidden)
	{
		if (AActor* Body = A.Get())
		{
			Body->SetActorHiddenInGame(false);
		}
	}
	CrowdHidden.Reset();
	Tone.Reset();
	Screens.Reset();
	Super::EndPlay(Reason);
}

// ------------------------------------------------------------------ the hour and the weather

void AStreetStage::SetWorldSeed(uint64 InSeed)
{
	if (InSeed != WorldSeed)
	{
		WorldSeed = InSeed;
		UpdateWeather(-1.0f);
		AppliedHourKey = -1000.0f;
		WindowHourApplied = -1000.0f;
	}
}

double AStreetStage::LookHour() const
{
	return (ShownWorld - FMath::FloorToDouble(ShownWorld / 1440.0) * 1440.0) / 60.0;
}

void AStreetStage::UpdateClock(float Dt)
{
	// The clock (SetClock, each frame from the game mode); before it's told (the editor, a test map) a late night of Night One.
	const double Clock = ClockWorld >= 0.0 ? ClockWorld : 1440.0 + 22.0 * 60.0;
	const float Forced = CVarHour.GetValueOnGameThread();
	ShownWorld = Forced >= 0.0f ? FMath::FloorToDouble(Clock / 1440.0) * 1440.0 + FMath::Fmod(static_cast<double>(Forced), 24.0) * 60.0 : Clock;
	// A skip: the clock racing ahead of real time (the career's clock runs a world second a second; a service's 30
	// minutes go by in seconds). The corner holds while it runs, and a moment after.
	if (RateWorld < 0.0 || Clock < RateWorld)
	{
		RateWorld = Clock;
		RateReal = RealClock;
	}
	else if (RealClock - RateReal >= 0.25)
	{
		const double Rate = (Clock - RateWorld) * 60.0 / (RealClock - RateReal);
		RateWorld = Clock;
		RateReal = RealClock;
		if (Rate > 10.0)
		{
			bSkipSeen = true;
			SkipQuiet = 1.0f;
		}
	}
	if (bSkipSeen)
	{
		SkipQuiet -= Dt;
		bSkipSeen = SkipQuiet > 0.0f;
	}
}

bool AStreetStage::IsHolding() const
{
	if (bSkipHoldSet || bSkipSeen)
	{
		return true;
	}
	const UStreetWorld* Registry = UStreetWorld::Get(this);
	return Registry && Registry->IsHeld();
}

void AStreetStage::UpdateWeather(float Dt)
{
	// What the weather wants now: forced (ss.Street.Weather), or the career's (core's model; a dry clear stub until it lands).
	ss::weather::Now Want;
	const FString Word = CVarWeather.GetValueOnGameThread();
	const bool bForced = !Word.Equals(TEXT("auto"), ESearchCase::IgnoreCase) && ForcedWeather(Word, Want);
	if (!bForced)
	{
		Want = ss::weather::At(ShownWorld, WorldSeed, RealClock);
	}
	const float RainForced = CVarRain.GetValueOnGameThread();
	if (RainForced >= 0.0f)
	{
		Want.Rain = FMath::Clamp(static_cast<double>(RainForced), 0.0, 1.0);
		Want.Wetness = FMath::Max(Want.Wetness, FMath::Clamp(Want.Rain * 1.25, 0.0, 1.0));
		Want.Puddle = FMath::Max(Want.Puddle, FMath::SmoothStep(0.55, 1.0, Want.Wetness));
		Want.Cloud = FMath::Max(Want.Cloud, 0.8 + 0.2 * Want.Rain);
		if (Want.Rain > 0.05 && Want.State < ss::weather::Sky::Drizzle)
		{
			Want.State = Want.Rain > 0.6 ? ss::weather::Sky::Rain : ss::weather::Sky::Drizzle;
		}
	}
	// Eased toward it (core eases over ten world minutes; a forced change shows over a few seconds), snapped at a build.
	if (Dt < 0.0f)
	{
		Weather = Want;
	}
	else
	{
		const float K = FMath::Min(1.0f, Dt * 0.6f);
		auto Ease = [K](double& V, double To) { V += (To - V) * K; };
		Ease(Weather.Rain, Want.Rain);
		Ease(Weather.Cloud, Want.Cloud);
		Ease(Weather.Wetness, Want.Wetness);
		Ease(Weather.Puddle, Want.Puddle);
		Ease(Weather.Wind, Want.Wind);
		Ease(Weather.Fog, Want.Fog);
		Weather.WindDir = Want.WindDir;
		Weather.State = Want.State;
		Weather.ThunderIn = Want.ThunderIn;
		Weather.Flash = Want.Flash;
	}
	// The street's sound and the old materials read these.
	Wetness = static_cast<float>(Weather.Wetness);
	// Lightning (spec 3.3): core's flash and its thunder's delay; in a forced storm the stage's own, every 20..90 s.
	float Flash = static_cast<float>(Want.Flash);
	if (bForced && Want.State == ss::weather::Sky::Storm && Dt >= 0.0f)
	{
		if (NextFlashAt <= 0.0)
		{
			NextFlashAt = RealClock + 20.0 + 70.0 * Ns::Hash01(static_cast<uint32>(RealClock * 1000.0));
		}
		if (RealClock >= NextFlashAt)
		{
			const float Delay = 1.0f + 5.0f * Ns::Hash01(static_cast<uint32>(NextFlashAt * 77.0));
			NextFlashAt = RealClock + 20.0 + 70.0 * Ns::Hash01(static_cast<uint32>(NextFlashAt * 131.0));
			FlashEnv = 1.0f;
			if (UNightOneAudio* Audio = StreetAudioOf(GetWorld()))
			{
				Audio->Thunder(Delay, 0.6f + 0.4f / FMath::Max(1.0f, Delay * 0.6f));
			}
		}
		// Two quick pulses in a quarter second, then gone.
		FlashEnv = FMath::Max(0.0f, FlashEnv - Dt * 4.0f);
		Flash = FlashEnv * (0.6f + 0.4f * FMath::Abs(FMath::Sin(static_cast<float>(RealClock) * 40.0f)));
	}
	else if (Flash > 0.5f && LastCoreFlash <= 0.5f && Want.ThunderIn >= 0.0)
	{
		if (UNightOneAudio* Audio = StreetAudioOf(GetWorld()))
		{
			Audio->Thunder(static_cast<float>(Want.ThunderIn), 0.8f);
		}
	}
	LastCoreFlash = static_cast<float>(Want.Flash);
	if (Weather.Flash > 0.001 && Flash <= 0.001f)
	{
		// The flash is over: the moon as the hour has it.
		AppliedHourKey = -1000.0f;
	}
	Weather.Flash = Flash;
	if (Moon && Flash > 0.001f)
	{
		Moon->SetIntensity(0.25f + 3.0f * Flash);
		Moon->SetLightColor(Ns::Srgb(0xd8e4ff));
	}
	for (UMaterialInstanceDynamic* M : RainMids)
	{
		if (M)
		{
			M->SetScalarParameterValue(TEXT("Flash"), Flash);
		}
	}
}

void AStreetStage::ApplyHour()
{
	if (!bBuilt && !GetWorld())
	{
		return;
	}
	const double Hour = LookHour();
	const float Cloud = static_cast<float>(Weather.Cloud);
	const float Rain = static_cast<float>(Weather.Rain);
	// What moves the look: the half minute, the weather in steps of 1/50 (it eases every frame for minutes at a slot's turn,
	// and the sun, the clouds, the fogs, the grade and the old materials' wet needn't all be set again each frame for it), the
	// tunables.
	auto Step = [](double V) { return static_cast<float>(FMath::RoundToDouble(V * 50.0)); };
	const float Key = static_cast<float>(FMath::RoundToDouble(ShownWorld * 2.0) * 0.5) + Step(Weather.Cloud) * 0.17f + Step(Weather.Rain) * 0.31f + Step(Weather.Fog) * 0.43f +
		CVarNightEV.GetValueOnGameThread() * 3.1f + CVarDayEV.GetValueOnGameThread() * 5.3f + CVarGlow.GetValueOnGameThread() * 7.7f +
		static_cast<float>(CVarClouds.GetValueOnGameThread()) * 11.0f;
	const bool bMoved = FMath::Abs(Key - AppliedHourKey) > 0.001f;
	if (bMoved)
	{
		AppliedHourKey = Key;
		double El = 0.0;
		double Az = 0.0;
		SunAt(Hour, El, Az);
		SunEl = static_cast<float>(El);
		SunAz = static_cast<float>(Az);
		const float NightOn = Below(El, 4.0, -2.0);
		const float Dark = Below(El, -4.0, -10.0);
		const float Overcast = FMath::SmoothStep(0.5f, 0.7f, Cloud);

		// The exposure's clamps (spec 3.2), EV100, blended over the bands: night < -6, dusk and dawn -6..6, day > 6 (overcast
		// or clear by the cloud); the tunables move night's and day's.
		const float DayMin = FMath::Lerp(10.5f, 9.5f, Overcast);
		const float DayMax = FMath::Lerp(13.5f, 12.0f, Overcast);
		const float WNightToDusk = Below(El, -4.0, -8.0);
		const float WDuskToDay = 1.0f - Below(El, 8.0, 4.0);
		float Lo = FMath::Lerp(3.0f, 0.5f, WNightToDusk);
		float Hi = FMath::Lerp(7.5f, 3.0f, WNightToDusk);
		Lo = FMath::Lerp(Lo, DayMin, WDuskToDay);
		Hi = FMath::Lerp(Hi, DayMax, WDuskToDay);
		const float NightW = Below(El, -2.0, -10.0);
		const float DayW = 1.0f - Below(El, 10.0, 4.0);
		EvMin = Lo + CVarNightEV.GetValueOnGameThread() * NightW + CVarDayEV.GetValueOnGameThread() * DayW;
		EvMax = FMath::Max(EvMin + 0.5f, Hi + CVarNightEV.GetValueOnGameThread() * NightW + CVarDayEV.GetValueOnGameThread() * DayW);

		// The sun and the moon.
		if (Sun)
		{
			Sun->SetRelativeRotation(FRotationMatrix::MakeFromX(-Toward(El, Az)).Rotator());
			Sun->SetIntensity(80000.0f * FMath::Clamp(static_cast<float>((El + 2.0) / 12.0), 0.0f, 1.0f));
			Sun->SetVisibility(El > -4.0);
		}
		if (Moon)
		{
			Moon->SetIntensity(0.25f * Dark * (1.0f - 0.5f * Cloud));
			Moon->SetLightColor(Ns::Srgb(0xb8c8ff));
			Moon->SetVisibility(Dark > 0.001f);
			const bool bShadows = Dark > 0.5f;
			if (static_cast<bool>(Moon->CastShadows) != bShadows)
			{
				Moon->SetCastShadows(bShadows);
			}
		}
		// The clouds: thin and high in the clear, a low thick deck in overcast and rain.
		if (Clouds)
		{
			const bool bOn = CVarClouds.GetValueOnGameThread() != 0;
			Clouds->SetVisibility(bOn);
			Clouds->SetLayerBottomAltitude(FMath::Lerp(2.0f, 1.2f, Cloud));
			Clouds->SetLayerHeight(FMath::Lerp(2.5f, 4.0f, Cloud));
			if (CloudMid && CloudMid->Parent)
			{
				TArray<FMaterialParameterInfo> Infos;
				TArray<FGuid> Ids;
				CloudMid->Parent->GetAllScalarParameterInfo(Infos, Ids);
				for (const FMaterialParameterInfo& Info : Infos)
				{
					float Base = 0.0f;
					const FString Name = Info.Name.ToString();
					if (!CloudMid->Parent->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Base))
					{
						continue;
					}
					if (Name.Contains(TEXT("Coverage"), ESearchCase::IgnoreCase))
					{
						CloudMid->SetScalarParameterValue(Info.Name, Base * FMath::Lerp(0.35f, 1.7f, Cloud));
					}
					else if (Name.Contains(TEXT("GlobalDensity"), ESearchCase::IgnoreCase))
					{
						CloudMid->SetScalarParameterValue(Info.Name, Base * FMath::Lerp(0.8f, 1.6f, Cloud));
					}
				}
			}
		}
		// The city's glow on the cloud at night: more cloud, more of it (a 0xff9c52 tint, spec 3.2).
		if (SkyGlowMid)
		{
			SkyGlowMid->SetScalarParameterValue(TEXT("Strength"), 0.35f * Dark * (0.25f + Cloud));
		}
		if (SkyLight)
		{
			SkyLight->SetLightColor(FMath::Lerp(FLinearColor::White, Ns::Srgb(0xc8b8a8), Dark));
		}
		// The fog: the base by the weather's fog, the low haze in rain; the night's dark blue-orange to the day's grey.
		if (Fog)
		{
			const float FogW = static_cast<float>(Weather.Fog);
			Fog->SetFogDensity(0.02f * FMath::Lerp(0.6f, 1.6f, FogW));
			FExponentialHeightFogData Haze;
			Haze.FogDensity = 0.06f * FMath::SmoothStep(0.2f, 0.9f, Rain) * FMath::Lerp(0.7f, 1.2f, FogW);
			Haze.FogHeightFalloff = 0.3f;
			Haze.FogHeightOffset = -50.0f;
			Fog->SetSecondFogData(Haze);
			const float Day = 1.0f - Below(El, 6.0, -6.0);
			const FLinearColor NightCol = FLinearColor(0.016f, 0.014f, 0.018f);
			const FLinearColor DayCol = FMath::Lerp(FLinearColor(0.42f, 0.46f, 0.52f), FLinearColor(0.34f, 0.35f, 0.37f), Overcast);
			Fog->SetFogInscatteringColor(FMath::Lerp(NightCol, DayCol, Day) * FMath::Pow(2.0f, FMath::Max(0.0f, EvMin - 1.0f)) * 0.08f);
			Fog->SetVolumetricFogExtinctionScale(FMath::Lerp(1.0f, 3.0f, FogW));
		}
		// The grade (spec 3.2): overcast day, dusk, night, blended by the sun; rain at any hour flattens it.
		if (Lens)
		{
			const float WDay = static_cast<float>(FMath::Clamp((El - 4.0) / 8.0, 0.0, 1.0));
			const float WNight = static_cast<float>(FMath::Clamp((-2.0 - El) / 6.0, 0.0, 1.0));
			const float WDusk = FMath::Max(0.0f, 1.0f - WDay - WNight);
			FPostProcessSettings& S = Lens->Settings;
			S.WhiteTemp = 6200.0f * WDay + 5600.0f * WDusk + 5000.0f * WNight;
			const float Wet = FMath::SmoothStep(0.1f, 0.6f, Rain);
			S.ColorSaturation = FVector4(1.0, 1.0, 1.0, FMath::Lerp(1.0f, 0.94f, Wet));
			S.ColorContrast = FVector4(1.0, 1.0, 1.0, FMath::Lerp(1.03f * WDay + 1.04f * WDusk + 1.06f * WNight, 1.05f, Wet));
			S.ColorGainHighlights = FVector4(1.0 + 0.03 * WDusk, 1.0, 1.0 - 0.04 * WDusk, 1.0);
			S.ColorOffsetShadows = FVector4(0.0, 0.002 * (WNight + WDay * Overcast), 0.006 * WNight + 0.004 * WDay * Overcast, 0.0);
			S.ColorSaturationShadows = FVector4(1.0, 1.0, 1.0, 1.0 - 0.15 * WNight);
		}
		// The glows' levels by the sun (Lit, Market's, the traffic's), the photocells' night.
		GlowLevels = GlowAt(El);
		const float GlowK = FMath::Max(0.0f, CVarGlow.GetValueOnGameThread());
		GlowLevels.Warm *= GlowK;
		GlowLevels.Cool *= GlowK;
		GlowLevels.Neon *= GlowK;
		GlowLevels.Signal *= GlowK;
		GlowLevels.Lamps *= GlowK;
		GlowLevels.Chase *= GlowK;
		GlowLevels.Night = NightOn;
		GlowLevels.Exposure = 1.0f;
		GlowLevels.LitDisplays = FMath::Lerp(0.35f, 1.0f, NightOn);
		// The old look's parts follow the same daylight.
		AppliedDaylight = -1.0f;
		SignScale = -1.0f;
		ApplyDaylight();
	}
	// (The shops and glows take the new levels in UpdateShops, this tick: with its Dt, so a grille mid-roll isn't snapped.)
}

void AStreetStage::UpdateMpc()
{
	UWorld* World = GetWorld();
	if (!Mpc || !World)
	{
		return;
	}
	UMaterialParameterCollectionInstance* I = World->GetParameterCollectionInstance(Mpc);
	if (!I)
	{
		return;
	}
	const double Hour = LookHour();
	const int32 Day = ss::net::DayOf(ShownWorld);
	I->SetScalarParameterValue(TEXT("Hour"), static_cast<float>(Hour));
	I->SetScalarParameterValue(TEXT("Weekday"), static_cast<float>(ss::net::Weekday(Day)));
	I->SetScalarParameterValue(TEXT("Daylight"), static_cast<float>(ss::life::Daylight(ShownWorld)));
	I->SetScalarParameterValue(TEXT("SunElev"), SunEl);
	I->SetScalarParameterValue(TEXT("Rain"), static_cast<float>(Weather.Rain));
	I->SetScalarParameterValue(TEXT("Wetness"), static_cast<float>(Weather.Wetness));
	I->SetScalarParameterValue(TEXT("Puddle"), static_cast<float>(Weather.Puddle));
	I->SetScalarParameterValue(TEXT("Lightning"), static_cast<float>(Weather.Flash));
	// WindDir is radians in the stage frame, the way the wind blows (0 toward +X, pi/2 toward +Y: Weather.h).
	const double Wind = Weather.Wind;
	I->SetScalarParameterValue(TEXT("WindX"), static_cast<float>(FMath::Cos(Weather.WindDir) * Wind));
	I->SetScalarParameterValue(TEXT("WindY"), static_cast<float>(FMath::Sin(Weather.WindDir) * Wind));
	I->SetScalarParameterValue(TEXT("Seed"), Ns::Hash01(static_cast<uint32>(WorldSeed ^ (WorldSeed >> 32))));
	I->SetScalarParameterValue(TEXT("Night"), GlowLevels.Night);
	I->SetScalarParameterValue(TEXT("Cloud"), static_cast<float>(Weather.Cloud));
}

void AStreetStage::UpdateShops(float Dt)
{
	// The plots' shops by their hours (the grilles roll), the faces core draws (the marquee by the day, the backlit by the hour),
	// and Market's and the traffic's look from the same hour.
	Lit.Update(ShownWorld, GlowLevels, Dt, Dt <= 0.0f);
	Screens.Update(ShownWorld, GlowLevels);
	if (AMarketStage* M = Market.Get())
	{
		M->SetLook(ShownWorld, GlowLevels, Dt);
	}
	if (AStreetTraffic* T = Traffic.Get())
	{
		T->SetLook(GlowLevels, static_cast<float>(Weather.Rain), IsHolding());
	}
	if (bCrowdOnOldLines)
	{
		KeepCrowdOffMarket(Dt);
	}
}

void AStreetStage::UpdateSignals()
{
	// The corner's state: core's (town::Signal, the same the crowd's crossings and the traffic read), held through a skip.
	// Its phases run 1..6 (1 Fifth green, 2 Fifth amber, 3 all red, 4 Market green, 5 Market amber, 6 all red); overnight it
	// flashes (FlashOn the lit half of each second).
	if (!IsHolding())
	{
		SignalShown = ss::town::Signal(ShownWorld);
	}
	const ss::town::SignalState& S = SignalShown;
	// The hand flashes once a second while the countdown runs (its own beat, not the overnight flash's).
	const bool bBlink = FMath::Frac(RealClock) < 0.5;
	// Each role's lamps: red, amber, green, walk (cells 0..3), hand, the countdown (4, 5).
	auto Cells = [&](uint8 SignalRole, float* Out) {
		for (int32 K = 0; K < 8; ++K)
		{
			Out[K] = 0.0f;
		}
		if (SignalRole == 0 || SignalRole == 1)
		{
			const bool bFifth = SignalRole == 0;
			if (S.Flashing)
			{
				// Fifth flashes amber, Market flashes red (spec 2.5).
				Out[bFifth ? 1 : 0] = S.FlashOn ? 1.0f : 0.0f;
				return 0.0;
			}
			const bool bGo = bFifth ? S.FifthGo : S.MarketGo;
			const bool bAmber = S.Phase == (bFifth ? 2 : 5);
			Out[bGo ? 2 : (bAmber ? 1 : 0)] = 1.0f;
			return 0.0;
		}
		// A walk head: dark overnight; WALK while it's given, then the flashing hand with the seconds left, then the hand.
		if (S.Flashing)
		{
			return 0.0;
		}
		const bool bFifthCrossing = SignalRole == 2;
		const bool bWalk = bFifthCrossing ? S.WalkFifth : S.WalkMarket;
		// People cross Fifth while Market has the green (phase 4), and Market's mouth while Fifth has it (phase 1).
		const bool bCounting = !bWalk && S.Phase == (bFifthCrossing ? 4 : 1) && S.Countdown > 0;
		if (bWalk)
		{
			Out[3] = 1.0f;
			return 0.0;
		}
		Out[4] = bCounting ? (bBlink ? 1.0f : 0.0f) : 1.0f;
		if (bCounting)
		{
			Out[5] = 1.0f;
			return static_cast<double>(S.Countdown);
		}
		return 0.0;
	};
	for (const FNsSignal& Sig : NsSignals)
	{
		UMaterialInstanceDynamic* Mid = Sig.Mid.Get();
		if (!Mid)
		{
			continue;
		}
		const uint8 SignalRole = Sig.Role & 0x0f;
		const int32 Cell = (Sig.Role >> 4) - 1;
		float Lamps[8];
		const double Count = Cells(SignalRole, Lamps);
		if (Cell < 0)
		{
			Mid->SetScalarParameterValue(TEXT("Level"), GlowLevels.Signal / 0.4f);
			Mid->SetVectorParameterValue(TEXT("LitA"), FLinearColor(Lamps[0], Lamps[1], Lamps[2], Lamps[3]));
			Mid->SetVectorParameterValue(TEXT("LitB"), FLinearColor(Lamps[4], Lamps[5], Lamps[6], Lamps[7]));
			Mid->SetScalarParameterValue(TEXT("Count"), static_cast<float>(Count));
		}
		else
		{
			// A stand-in's lamp: its own glow.
			Mid->SetScalarParameterValue(TEXT("Emissive"), Lamps[FMath::Clamp(Cell, 0, 7)] * GlowLevels.Signal * 0.4f);
		}
	}
}

void AStreetStage::UpdateWindowsByHour()
{
	// The stand-in fronts' windows by the hour (spec 3.4): each its own minute, the share of rooms lit following the hour.
	const double Hour = LookHour();
	if (FMath::Abs(Hour - WindowHourApplied) < 0.1 && WindowHourApplied > -100.0f)
	{
		return;
	}
	WindowHourApplied = static_cast<float>(Hour);
	TSet<UInstancedStaticMeshComponent*> Dirty;
	for (const FNsWindow& W : NsWindows)
	{
		UInstancedStaticMeshComponent* Set = W.Set.Get();
		if (!Set)
		{
			continue;
		}
		const uint32 Hash = static_cast<uint32>(W.Seed * 16777215.0f) ^ static_cast<uint32>(WorldSeed);
		const double Hr = FMath::Fmod(Hour + (Ns::Hash01(Hash * 3u + 1u) - 0.5) * 0.8 + 24.0, 24.0);
		const bool bLit = Ns::Hash01(Hash * 7u + 5u) < Occupancy(Hr);
		Set->SetCustomDataValue(W.Index, 0, bLit ? static_cast<float>(W.Look) : 0.0f, false);
		Dirty.Add(Set);
	}
	for (UInstancedStaticMeshComponent* Set : Dirty)
	{
		Set->MarkRenderStateDirty();
	}
}

void AStreetStage::UpdateBankClock()
{
	if (!BankHour || !BankMinute)
	{
		return;
	}
	// Turned clockwise as seen from the north (about the stage's +Y: twelve, up, toward three, west).
	const double InDay = ShownWorld - FMath::FloorToDouble(ShownWorld / 1440.0) * 1440.0;
	const double Minute = FMath::Fmod(InDay, 60.0);
	const double Hour12 = FMath::Fmod(InDay / 60.0, 12.0);
	BankMinute->SetRelativeRotation(FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Minute * 6.0)));
	BankHour->SetRelativeRotation(FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Hour12 * 30.0)));
}

void AStreetStage::KeepCrowdOffMarket(float Dt)
{
	// Today's crowd walks its own hard-coded Market lines (StreetCrowd.cpp), which at Market's new width run down its
	// westbound lane: its people are hidden while they're on Market's roadway or its old north line, until the crowd reads
	// the registry's walkways (spec 6.8).
	CrowdCheckIn -= Dt;
	if (CrowdCheckIn > 0.0f)
	{
		return;
	}
	CrowdCheckIn = 0.25f;
	UWorld* World = GetWorld();
	const APawn* Hero = World && World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	const UClass* CrowdClass = FindObject<UClass>(nullptr, TEXT("/Script/ShortStack.StreetCrowd"));
	if (!World || !CrowdClass)
	{
		return;
	}
	const FTransform& T = GetActorTransform();
	for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
	{
		AShortStackCharacter* Body = *It;
		if (Body == Hero || !Body->GetOwner() || !Body->GetOwner()->IsA(CrowdClass))
		{
			continue;
		}
		const FVector L = T.InverseTransformPosition(Body->GetActorLocation());
		const bool bOnMarket = L.X < -60.0 && L.Y > Ns::MarketS - 20.0 && L.Y < Ns::MarketN + 20.0;
		const int32 Hidden = CrowdHidden.IndexOfByKey(Body);
		if (bOnMarket && Hidden == INDEX_NONE && !Body->IsHidden())
		{
			Body->SetActorHiddenInGame(true);
			CrowdHidden.Add(Body);
		}
		else if (!bOnMarket && Hidden != INDEX_NONE)
		{
			Body->SetActorHiddenInGame(false);
			CrowdHidden.RemoveAtSwap(Hidden);
		}
	}
}

// ------------------------------------------------------------------ the street's sound (spec 3.7)

void AStreetStage::UpdateSound(const FVector& Camera, const FRotator& Rotation, float Dt)
{
	// The game mode's street ambience, found once it's made (FStreetAmbience::Live), moved onto a stereo stream (the
	// apartment keeps its own mono one), told where the listener is and which positioned sounds are near.
	if (!Tone.IsValid())
	{
		Tone = FStreetAmbience::Live();
		bToneStereo = false;
	}
	FStreetAmbience* Amb = Tone.Get();
	if (!Amb)
	{
		return;
	}
	if (!bToneStereo)
	{
		if (UNightOneAudio* Audio = StreetAudioOf(GetWorld()))
		{
			TSharedPtr<FStreetAmbience> Keep = Tone;
			Audio->SetAmbienceSourceStereo([Keep](float* Out, int32 Frames) { Keep->RenderStereo(Out, Frames); }, 0.75f);
			bToneStereo = true;
		}
	}
	const FRotator Flat(0.0f, Rotation.Yaw, 0.0f);
	Amb->SetListener(Camera, Flat.Vector(), FRotationMatrix(Flat).GetUnitAxis(EAxis::Y));
	Amb->SetWind(static_cast<float>(Weather.Wind));
	// (The shop the camera is in, FStreetAmbience::SetRoom, is the game mode's to say each frame, with the store's SetInside
	// and the laundromat's SetWash.)
	SoundIn -= Dt;
	if (SoundIn > 0.0f)
	{
		return;
	}
	SoundIn = 0.2f;
	// Within 40 m: the stage's own (the signs' hum while lit, the AC units, the downspouts and the awnings in rain), Market's,
	// and the shops' open doors leaking their rooms.
	TArray<FStreetEmitter> Near;
	const FTransform& T = GetActorTransform();
	const float Rain = static_cast<float>(Weather.Rain);
	for (int32 I = 0; I < NsEmitters.Num(); ++I)
	{
		const FNsEmitterSpot& E = NsEmitters[I];
		const FVector At = T.TransformPosition(E.At);
		if (FVector::DistSquared(At, Camera) > 4000.0 * 4000.0)
		{
			continue;
		}
		const EStreetEmitter Kind = static_cast<EStreetEmitter>(E.Kind);
		float Level = E.Level;
		if (Kind == EStreetEmitter::Hum)
		{
			Level *= GlowLevels.Night;
		}
		else if (Kind == EStreetEmitter::Gutter || Kind == EStreetEmitter::RainRoof)
		{
			Level *= FMath::SmoothStep(0.05f, 0.6f, Rain);
		}
		if (Level <= 0.01f)
		{
			continue;
		}
		FStreetEmitter Out;
		Out.At = At;
		Out.Kind = Kind;
		Out.Level = Level;
		Out.Hz = E.Hz;
		Out.Id = 1000u + static_cast<uint32>(I);
		Near.Add(Out);
	}
	if (const AMarketStage* M = Market.Get())
	{
		M->AppendEmitters(Near, Camera, Rain, GlowLevels.Night);
	}
	if (const AStreetTraffic* Cars = Traffic.Get())
	{
		Cars->AppendEmitters(Near, Camera, Rain);
	}
	if (UWorld* World = GetWorld())
	{
		uint32 DoorId = 5000u;
		for (TActorIterator<AShopStage> It(World); It; ++It)
		{
			const AShopStage* Shop = *It;
			const float Open = Shop->DoorOpenAmount();
			++DoorId;
			if (Open < 0.05f || !Shop->IsOpen() || Shop->IsInside(Camera))
			{
				continue;
			}
			const UStreetWorld* Registry = UStreetWorld::Get(this);
			for (const FStreetDoor& D : Registry ? Registry->Doors() : TArray<FStreetDoor>())
			{
				if (D.Id == Shop->Venue && FVector::DistSquared(D.Threshold, Camera) < 2500.0 * 2500.0)
				{
					FStreetEmitter Out;
					Out.At = D.Threshold + FVector(0.0, 0.0, 150.0);
					Out.Kind = EStreetEmitter::RoomLeak;
					Out.Level = Open;
					Out.Hz = static_cast<float>(DoorId % 7);
					Out.Id = DoorId;
					Near.Add(Out);
				}
			}
		}
	}
	Amb->SetEmitters(Near);
}

// ------------------------------------------------------------------ the rain's hits and drips (spec 3.3)

double AStreetStage::GroundZAt(double X, double Y, bool bWide)
{
	// The roads (Fifth, Hollis and Wexler across its ends, Market off the corner, Cardinal across Market's end) on the modelled
	// ground's crown (ns_ground.py's road_crown_z: the through streets' crowns across the intersections); everything else is
	// a sidewalk's top.
	const double S = bWide ? Ns::MarketS : Ns::OldMarketS;
	const double N = bWide ? Ns::MarketN : Ns::OldMarketN;
	const double East = bWide ? Ns::CardinalRoad0 : Ns::OldMarketWest;
	const bool bFifth = X > Ns::RoadX0 && X < Ns::RoadX1 && Y > Ns::HollisRoad1 && Y < Ns::WexlerRoad1;
	const bool bCross = (Y > Ns::HollisRoad1 && Y < Ns::HollisRoad0) || (Y > Ns::WexlerRoad0 && Y < Ns::WexlerRoad1);
	const bool bMarket = X <= Ns::RoadX0 && X > East && Y > S && Y < N;
	const bool bCardinal = bWide && X <= Ns::CardinalRoad0 && X > Ns::CardinalRoad1;
	return bFifth || bCross || bMarket || bCardinal ? Ns::RoadCrownZ(X, Y, bWide) : 0.0;
}

void AStreetStage::AddDrips(UInstancedStaticMeshComponent* Set, const FTransform& Frame, const FVector& A, const FVector& B, double GroundZ, double Spacing)
{
	if (!Set)
	{
		return;
	}
	const double Drop = A.Z - GroundZ;
	const double Run = FVector::Dist2D(A, B);
	if (Drop <= 10.0 || Run <= 1.0)
	{
		return;
	}
	// One line every Spacing cm along the edge (a hair off even, so they don't read as a comb), each the engine's cylinder
	// (radius 50, 100 tall about its middle) drawn down to a line 0.7 cm across from the edge to the ground.
	const int32 N = FMath::Max(1, FMath::FloorToInt32(Run / FMath::Max(10.0, Spacing)));
	for (int32 I = 0; I < N; ++I)
	{
		const double T = (static_cast<double>(I) + 0.25 + 0.5 * Ns::Hash01(static_cast<uint32>(I * 7919) ^ static_cast<uint32>(static_cast<int32>(A.X * 3.0 + A.Y)))) / N;
		const FVector Top = FMath::Lerp(A, B, T);
		const int32 Index = Set->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Top.X, Top.Y, GroundZ + Drop * 0.5), FVector(0.007, 0.007, Drop / 100.0)));
		const float Data[2] = {static_cast<float>(Frame.TransformPosition(Top).Z), static_cast<float>(Drop)};
		Set->SetCustomData(Index, TArrayView<const float>(Data, 2), false);
	}
	Set->MarkRenderStateDirty();
}

namespace StreetLookDetail
{
/** How many splashes are kept round the camera, and how far out (cm): about one hit per 1.6 square meters in a downpour. */
constexpr int32 SplashCount = 240;
constexpr double SplashReach = 1100.0;

/** Each splash's place in the pool (stage cm from its middle): a sunflower disc, so none bunch up. */
FVector2D SplashOffset(int32 I)
{
	const double Golden = 2.399963229728653;
	const double R = SplashReach * FMath::Sqrt((static_cast<double>(I) + 0.5) / SplashCount);
	const double A = Golden * I;
	const double Jitter = 35.0 * (Ns::Hash01(static_cast<uint32>(I) * 2654435761u) - 0.5);
	return FVector2D((R + Jitter) * FMath::Cos(A), (R - Jitter) * FMath::Sin(A));
}

/** Open ground the rain lands on (stage cm): Fifth between its building lines, the cross streets, Market and Cardinal. */
bool OpenGround(double X, double Y, bool bWide)
{
	if (X >= Ns::HomeLine && X <= Ns::FarFront && Y >= Ns::HollisS && Y <= Ns::WexlerN)
	{
		return true;
	}
	if ((Y >= Ns::HollisS && Y <= Ns::HollisN) || (Y >= Ns::WexlerS && Y <= Ns::WexlerN))
	{
		return FMath::Abs(X - Ns::FifthMidX) < Ns::PassReach;
	}
	if (bWide)
	{
		if (X < Ns::HomeLine && X >= Ns::MarketEast && Y >= Ns::StoreY1 && Y <= Ns::MarketLineN)
		{
			return true;
		}
		return X < Ns::MarketEast && X >= Ns::CardinalE && FMath::Abs(Y - Ns::MarketMidY) < Ns::PassReach;
	}
	return X < Ns::HomeLine && X >= Ns::OldMarketWest && Y >= Ns::StoreY1 && Y <= Ns::OldMarketLineN;
}

/** Ground kept dry overhead (stage cm, X0 Y0 X1 Y1): Fifth's awnings, the 14's shelter, the shed along Cardinal Furniture. */
const FBox2D Sheltered[] = {
	FBox2D(FVector2D(0.0, 3110.0), FVector2D(170.0, 3620.0)),
	FBox2D(FVector2D(0.0, -3330.0), FVector2D(170.0, -2420.0)),
	FBox2D(FVector2D(1870.0, -5800.0), FVector2D(2040.0, -5200.0)),
	FBox2D(FVector2D(-2490.0, 5090.0), FVector2D(-2010.0, 5270.0)),
	FBox2D(FVector2D(-9600.0, 6680.0), FVector2D(-7590.0, 6950.0)),
};
} // namespace StreetLookDetail

void AStreetStage::BuildRainFx()
{
	UMaterialInterface* Fx = Cast<UMaterialInterface>(NsAsset(Ns::RainFxPath));
	if (!Fx)
	{
		return;
	}
	auto Pool = [this](UStaticMesh* Mesh, UMaterialInterface* M) {
		UInstancedStaticMeshComponent* Set = NewPart<UInstancedStaticMeshComponent>();
		Set->SetStaticMesh(Mesh);
		Set->SetMaterial(0, M);
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		Set->SetVisibleInRayTracing(false);
		Set->bAffectDistanceFieldLighting = false;
		Set->bAffectDynamicIndirectLighting = false;
		Set->SetCanEverAffectNavigation(false);
		return Set;
	};
	// The hits: flat cards on the ground round the camera, laid as it moves (UpdateRainFx), each 16 cm across. Out past
	// 20 m they're gone (too small to see; the puddles' rings carry the rain there).
	UMaterialInstanceDynamic* Hit = UMaterialInstanceDynamic::Create(Fx, this);
	Hit->SetScalarParameterValue(TEXT("Mode"), 0.0f);
	Splashes = Pool(PlaneMesh, Hit);
	Splashes->SetCullDistances(0, 2000);
	for (int32 I = 0; I < SplashCount; ++I)
	{
		Splashes->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0, 0.0, -5000.0), FVector(0.16, 0.16, 1.0)));
	}
	SplashCenter = FVector2D(1.0e9, 1.0e9);
	// The drips off Fifth's awnings' valances (BuildFurniture noted their edges): thin lines to the sidewalk.
	UMaterialInstanceDynamic* Drip = UMaterialInstanceDynamic::Create(Fx, this);
	Drip->SetScalarParameterValue(TEXT("Mode"), 1.0f);
	Drips = Pool(CylinderMesh, Drip);
	Drips->SetNumCustomDataFloats(2);
	Drips->SetCullDistances(0, 3000);
	for (const TPair<FVector, FVector>& Edge : DripEdges)
	{
		AddDrips(Drips, GetActorTransform(), Edge.Key, Edge.Value, 0.0, 32.0);
	}
}

void AStreetStage::UpdateRainFx(const FVector& Camera)
{
	if (!Splashes)
	{
		return;
	}
	const bool bRain = Weather.Rain > 0.01;
	if (Splashes->IsVisible() != bRain)
	{
		Splashes->SetVisibility(bRain);
	}
	if (Drips)
	{
		const bool bDrip = Weather.Rain > 0.01 || Weather.Wetness > 0.4;
		if (Drips->IsVisible() != bDrip)
		{
			Drips->SetVisibility(bDrip);
		}
	}
	if (!bRain)
	{
		return;
	}
	// Laid again round the camera once it's 2 m on (snapped, so the pool doesn't swim), on open ground only, never in a room
	// or under an awning or the shelter; the rest wait under the street.
	const FVector L = GetActorTransform().InverseTransformPosition(Camera);
	const FVector2D C(FMath::GridSnap(L.X, 200.0), FMath::GridSnap(L.Y, 200.0));
	if (C.Equals(SplashCenter, 1.0))
	{
		return;
	}
	SplashCenter = C;
	TArray<FTransform> Placed;
	Placed.Reserve(SplashCount);
	for (int32 I = 0; I < SplashCount; ++I)
	{
		const FVector2D At = C + SplashOffset(I);
		bool bOpen = OpenGround(At.X, At.Y, bWideMarket);
		for (const FBox2D& Dry : Sheltered)
		{
			bOpen = bOpen && !Dry.IsInside(At);
		}
		const double Z = bOpen ? GroundZAt(At.X, At.Y, bWideMarket) + 0.6 : -5000.0;
		Placed.Add(FTransform(FRotator(0.0f, 37.0f * I, 0.0f), FVector(At.X, At.Y, Z), FVector(0.16, 0.16, 1.0)));
	}
	Splashes->BatchUpdateInstancesTransforms(0, Placed, false, true, true);
}

void AStreetStage::ApplyBudgetToggles()
{
	// MegaLights in the street's lens (the shops' volumes set their own).
	const bool bMega = CVarMegaLights.GetValueOnGameThread() != 0;
	if (Lens && static_cast<bool>(Lens->Settings.bMegaLights) != bMega)
	{
		Lens->Settings.bMegaLights = bMega ? 1 : 0;
	}
	// The trees, Fifth's and Market's, when the switch moves.
	const bool bTrees = CVarTrees.GetValueOnGameThread() != 0;
	if (static_cast<int8>(bTrees) == TreesApplied)
	{
		return;
	}
	TreesApplied = static_cast<int8>(bTrees);
	for (UStaticMeshComponent* Tree : Trees)
	{
		if (Tree)
		{
			Tree->SetVisibility(bTrees);
		}
	}
	if (AMarketStage* M = Market.Get())
	{
		TInlineComponentArray<UStaticMeshComponent*> Parts(M);
		for (UStaticMeshComponent* C : Parts)
		{
			const UStaticMesh* Mesh = C ? C->GetStaticMesh().Get() : nullptr;
			if (Mesh && Mesh->GetName().StartsWith(TEXT("SM_NS_Tree_")) && C->IsVisible() != bTrees)
			{
				C->SetVisibility(bTrees);
			}
		}
	}
}

// ------------------------------------------------------------------ the tests

FString AStreetStage::TestHour(float Hour)
{
	CVarHour->Set(*FString::SanitizeFloat(Hour < 0.0f ? -1.0f : FMath::Fmod(Hour, 24.0f)), ECVF_SetByConsole);
	UpdateClock(0.0f);
	AppliedHourKey = -1000.0f;
	ApplyHour();
	UpdateMpc();
	UpdateShops(0.0f);
	WindowHourApplied = -1000.0f;
	UpdateWindowsByHour();
	return TestLook();
}

FString AStreetStage::TestWeather(const FString& State)
{
	CVarWeather->Set(*State, ECVF_SetByConsole);
	UpdateWeather(-1.0f);
	AppliedHourKey = -1000.0f;
	ApplyHour();
	UpdateMpc();
	UpdateShops(0.0f);
	return TestLook();
}

FString AStreetStage::TestLook() const
{
	const double Hour = LookHour();
	const int32 H = static_cast<int32>(Hour);
	const int32 M = static_cast<int32>((Hour - H) * 60.0);
	return FString::Printf(TEXT("look %02d:%02d%s, sun %.1f deg az %.0f, %s (rain %.2f, cloud %.2f, wet %.2f, puddles %.2f, wind %.2f, fog %.2f%s), exposure %.1f..%.1f EV, glows warm %.0f neon %.0f signal %.0f nits, night %.2f; Market %s; plots %d kit, %d stand-in; signal phase %d (%.0f s)%s; %d windows by the hour%s"),
		H, M, CVarHour.GetValueOnGameThread() >= 0.0f ? TEXT(" (forced)") : TEXT(""), SunEl, SunAz, UTF8_TO_TCHAR(ss::weather::Word(Weather.State)), Weather.Rain, Weather.Cloud,
		Weather.Wetness, Weather.Puddle, Weather.Wind, Weather.Fog, CVarWeather.GetValueOnGameThread().Equals(TEXT("auto"), ESearchCase::IgnoreCase) ? TEXT("") : TEXT(", forced"), EvMin,
		EvMax, GlowLevels.Warm, GlowLevels.Neon, GlowLevels.Signal, GlowLevels.Night, bWideMarket ? TEXT("at its new width") : TEXT("as The Block built it"), PlotsKit, PlotsStandIn,
		SignalShown.Phase, SignalShown.Left, IsHolding() ? TEXT(", held (a skip)") : TEXT(""), NsWindows.Num(), Mpc ? TEXT("") : TEXT("; no MPC_NS (run ns_setup)"));
}

FString AStreetStage::TestBudget() const
{
	const float GameMs = static_cast<float>(FPlatformTime::ToMilliseconds(GGameThreadTime));
	const float DrawMs = static_cast<float>(FPlatformTime::ToMilliseconds(GRenderThreadTime));
	const float GpuMs = static_cast<float>(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
	UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	const FVector Cam = Pc && Pc->PlayerCameraManager ? Pc->PlayerCameraManager->GetCameraLocation() : GetActorLocation();
	// The local lights within 40 m of the camera (spec 3.1's budget: <= 60, <= 8 with volumetric shadows), every stage's.
	int32 Lights = 0;
	int32 Shadowed = 0;
	int32 Volumetric = 0;
	int32 Translucent = 0;
	int32 Tiers[4] = {0, 0, 0, 0};
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<ULightComponent*> Ls(*It);
			for (const ULightComponent* L : Ls)
			{
				if (!L || !L->IsVisible() || L->Intensity <= 0.0f || L->IsA<UDirectionalLightComponent>() ||
					FVector::DistSquared(L->GetComponentLocation(), Cam) > 4000.0 * 4000.0)
				{
					continue;
				}
				++Lights;
				Shadowed += L->CastShadows ? 1 : 0;
				Volumetric += L->CastShadows && L->bCastVolumetricShadow ? 1 : 0;
			}
			TInlineComponentArray<UPrimitiveComponent*> Ps(*It);
			for (const UPrimitiveComponent* P : Ps)
			{
				if (!P || !P->IsVisible() || FVector::DistSquared(P->Bounds.Origin, Cam) > 4000.0 * 4000.0)
				{
					continue;
				}
				for (int32 Slot = 0; Slot < P->GetNumMaterials(); ++Slot)
				{
					const UMaterialInterface* M = P->GetMaterial(Slot);
					const EBlendMode Blend = M ? M->GetBlendMode() : BLEND_Opaque;
					if (Blend == BLEND_Translucent || Blend == BLEND_Additive || Blend == BLEND_Modulate || Blend == BLEND_AlphaComposite)
					{
						++Translucent;
						break;
					}
				}
			}
		}
		for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
		{
			if (!It->IsHidden())
			{
				++Tiers[FMath::Clamp(It->GetDetailTier(), 0, 3)];
			}
		}
	}
	return FString::Printf(TEXT("budget: game %.1f ms, draw %.1f, gpu %.1f (spec 3.8: <= 16.6 gpu, <= 10 game) | within 40 m: %d local lights (<= 60), %d shadowed, %d in the fog (<= 8); %d translucent primitives | MetaHumans near %d, mid %d, far %d, gone %d (<= 4 near, <= 8 shown) | %d trees, %d windows by the hour, %d glows, %d shops | %s"),
		GameMs, DrawMs, GpuMs, Lights, Shadowed, Volumetric, Translucent, Tiers[0], Tiers[1], Tiers[2], Tiers[3], Trees.Num(), NsWindows.Num(), Lit.Glows.Num(), Lit.Shops.Num(),
		Traffic.IsValid() ? *Traffic->Describe() : TEXT("no traffic"));
}
