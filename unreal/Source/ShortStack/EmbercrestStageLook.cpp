// The Embercrest's look (spec 4): the sun and the moon for the hour, the SkyAtmosphere at 2.1 km, the thin clouds by day,
// the stars and the moon's disc at night, the height fog with the valley's mist pooled under it, the fixtures under their
// rules, the exposure clamps and the grade blended by the sun's elevation; and the screens (spec 5.6), painted into
// render targets (EmbercrestPaint.h).
#include "EmbercrestStage.h"

#include "EmbercrestShared.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EmbercrestPaint.h"
#include "Slate/WidgetRenderer.h"
#include "SlateDrawList.h"

#include <string>

namespace EcxLookDetail
{
TAutoConsoleVariable<float> CVarGlow(TEXT("ss.ECX.Glow"), 1.0f, TEXT("The Embercrest's LED lines, lenses, letters and screens: a scale on their strength."));
TAutoConsoleVariable<float> CVarNightEV(TEXT("ss.ECX.NightEV"), 0.0f, TEXT("Stops added to the Embercrest's night exposure clamps (+ darker)."));
TAutoConsoleVariable<float> CVarDayEV(TEXT("ss.ECX.DayEV"), 0.0f, TEXT("Stops added to the Embercrest's day exposure clamps (+ darker)."));
TAutoConsoleVariable<int32> CVarClouds(TEXT("ss.ECX.Clouds"), 1, TEXT("The thin cloud layer over the peaks by day and at dusk (0 off)."));
TAutoConsoleVariable<int32> CVarVolFog(TEXT("ss.ECX.VolFog"), 1, TEXT("Volumetric fog at dusk and at night (0 off)."));
TAutoConsoleVariable<float> CVarLit(TEXT("ss.ECX.Lit"), 1.0f, TEXT("A scale on the light baked into the building's windows and the city."));

/** One row of the spec's table 4.4, by the sun's elevation. */
struct FHourRow
{
	double El;
	float EvMin, EvMax, Ember, Warm, Ash, Screens, Lit, Crown;
};
const FHourRow HourRows[5] = {{12.0, 9.5f, 14.5f, 4000.0f, 1500.0f, 3000.0f, 3500.0f, 0.20f, 3000.0f}, {4.0, 8.5f, 13.0f, 1200.0f, 600.0f, 1200.0f, 2000.0f, 0.45f, 1500.0f},
	{-2.0, 7.0f, 11.0f, 250.0f, 150.0f, 300.0f, 900.0f, 0.80f, 500.0f}, {-8.0, 3.5f, 6.5f, 80.0f, 60.0f, 90.0f, 500.0f, 1.00f, 200.0f},
	{-14.0, 1.5f, 4.5f, 30.0f, 25.0f, 40.0f, 350.0f, 1.00f, 90.0f}};

/** The tunables together (any change applies the hour again). */
float TuningKey()
{
	return CVarNightEV.GetValueOnGameThread() + 3.1f * CVarDayEV.GetValueOnGameThread() + 7.7f * static_cast<float>(CVarClouds.GetValueOnGameThread()) +
		13.3f * static_cast<float>(CVarVolFog.GetValueOnGameThread()) + 17.9f * CVarLit.GetValueOnGameThread();
}

FHourRow HourRowAt(double El)
{
	if (El >= HourRows[0].El)
	{
		return HourRows[0];
	}
	for (int32 I = 0; I < 4; ++I)
	{
		const FHourRow& A = HourRows[I];
		const FHourRow& B = HourRows[I + 1];
		if (El >= B.El)
		{
			const float T = static_cast<float>((A.El - El) / (A.El - B.El));
			FHourRow R;
			R.El = El;
			R.EvMin = FMath::Lerp(A.EvMin, B.EvMin, T);
			R.EvMax = FMath::Lerp(A.EvMax, B.EvMax, T);
			R.Ember = FMath::Lerp(A.Ember, B.Ember, T);
			R.Warm = FMath::Lerp(A.Warm, B.Warm, T);
			R.Ash = FMath::Lerp(A.Ash, B.Ash, T);
			R.Screens = FMath::Lerp(A.Screens, B.Screens, T);
			R.Lit = FMath::Lerp(A.Lit, B.Lit, T);
			R.Crown = FMath::Lerp(A.Crown, B.Crown, T);
			return R;
		}
	}
	return HourRows[4];
}

/** A line of the screens' text in one of the brand's colors, shrunk to fit MaxW (Align 0 left, 1 centered, 2 right). */
void SlideLine(FEcxPaint& P, const FString& Text, float X, float Y, float Size, int32 Weight, uint32 Hex, float MaxW, int32 Align = 0)
{
	P.Text(Text, X, Y, Size, Weight, FEcxPaint::Ink(Hex), Align, MaxW);
}
} // namespace EcxLookDetail

using namespace EcxLookDetail;
using namespace Ecx;

// ------------------------------------------------------------------ the sky

void AEmbercrestStage::BuildSky()
{
	// The sun: 85,000 lux at the top of the atmosphere, which reddens and dims it at dusk; virtual shadow maps.
	Sun = NewPart<UDirectionalLightComponent>();
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(85000.0f);
	Sun->SetLightSourceAngle(0.5357f);
	Sun->SetCastShadows(true);
	Sun->ContactShadowLength = 0.03f;
	Sun->bCastCloudShadows = true;
	Sun->SetForwardShadingPriority(1);
	Sun->SetVolumetricScatteringIntensity(1.0f);
	Sun->MarkRenderStateDirty();
	// The moon: fixed WSW over the valley, cold, soft, a sixth as bright again as a real full moon so moonlit snow reads. The
	// atmosphere doesn't draw its disc (the sky's material does, with its face).
	Moon = NewPart<UDirectionalLightComponent>();
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(1.6f);
	Moon->SetLightColor(Srgb(0x9fb4e0));
	Moon->SetLightSourceAngle(1.0f);
	Moon->SetCastShadows(true);
	Moon->SetAtmosphereSunDiskColorScale(FLinearColor::Black);
	Moon->SetVolumetricScatteringIntensity(1.0f);
	Moon->SetRelativeRotation(FRotator(-22.0f, 145.0f, 0.0f));
	// The atmosphere, its planet's top 2.1 km below the plaza: a deeper blue at altitude, haze in the valley.
	Atmosphere = NewPart<USkyAtmosphereComponent>();
	Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetTopAtComponentTransform;
	Atmosphere->SetRelativeLocation(FVector(0.0, 0.0, -210000.0));
	Atmosphere->SetMieScatteringScale(0.005f);
	Atmosphere->SetMieAnisotropy(0.8f);
	Atmosphere->SetAerialPespectiveViewDistanceScale(1.2f);
	Atmosphere->MarkRenderStateDirty();
	// A thin layer of altocumulus over the peaks by day (the engine's simple cloud, resolved in the editor and saved).
	UMaterialInterface* CloudBase = CloudMaterial ? CloudMaterial.Get()
												  : LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"), nullptr,
														LOAD_NoWarn | LOAD_Quiet);
	if (CloudBase)
	{
		Clouds = NewPart<UVolumetricCloudComponent>();
		CloudMid = UMaterialInstanceDynamic::Create(CloudBase, this);
		Clouds->SetMaterial(CloudMid);
		Clouds->SetLayerBottomAltitude(6.0f);
		Clouds->SetLayerHeight(2.5f);
		Clouds->SetViewSampleCountScale(0.5f);
		Clouds->SetShadowViewSampleCountScale(0.5f);
		Clouds->SetReflectionViewSampleCountScale(0.25f);
		Clouds->SetShadowReflectionViewSampleCountScale(0.25f);
	}
	// The sky's light, captured from the sky alone (beyond 1 km), a dark ground under it.
	SkyLight = NewPart<USkyLightComponent>();
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLight->SkyDistanceThreshold = 100000.0f;
	SkyLight->bLowerHemisphereIsBlack = false;
	SkyLight->SetLowerHemisphereColor(Srgb(0x1a2030) * 0.2f);
	SkyLight->SetIntensity(1.0f);
	SkyLight->MarkRenderStateDirty();
	SkyLight->SetCaptureIsDirty();
	// Height fog that keeps the plaza crisp, and a second layer pooled 720 m down: the valley's mist round the city.
	Fog = NewPart<UExponentialHeightFogComponent>();
	Fog->SetFogDensity(0.0003f);
	Fog->SetFogHeightFalloff(0.02f);
	Fog->SetStartDistance(1500.0f);
	Fog->SetFogMaxOpacity(0.85f);
	Fog->SetFogInscatteringColor(FLinearColor(0.004f, 0.005f, 0.008f));
	Fog->SetSkyAtmosphereAmbientContributionColorScale(FLinearColor::White);
	// The stars' sphere (50 km out) stays out of it.
	Fog->SetFogCutoffDistance(4000000.0f);
	FExponentialHeightFogData Valley;
	Valley.FogDensity = 0.0008f;
	Valley.FogHeightFalloff = 0.03f;
	Valley.FogHeightOffset = -72000.0f;
	Fog->SetSecondFogData(Valley);
	Fog->SetVolumetricFogDistance(6000.0f);
	Fog->SetVolumetricFogExtinctionScale(3.0f);
	Fog->SetVolumetricFogAlbedo(FColor(0xc4, 0xc8, 0xd0));
	Fog->SetVolumetricFogScatteringDistribution(0.7f);
	// The stars, the Milky Way, the moon's face and the city's glow low in the west: a sphere past everything.
	if (UMaterialInterface* SkyMat = Ecx::LoadMat(TEXT("M_ECX_Sky")))
	{
		StarMid = UMaterialInstanceDynamic::Create(SkyMat, this);
		if (UTexture* Stars = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_StarField")))
		{
			StarMid->SetTextureParameterValue(TEXT("StarTex"), Stars);
			StarMid->SetTextureParameterValue(TEXT("StarTexB"), Stars);
			StarMid->SetScalarParameterValue(TEXT("HasStarTex"), 1.0f);
		}
		if (UTexture* MoonFace = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_Moon")))
		{
			StarMid->SetTextureParameterValue(TEXT("MoonTex"), MoonFace);
			StarMid->SetScalarParameterValue(TEXT("HasMoonTex"), 1.0f);
		}
		UStaticMeshComponent* Dome = NewPart<UStaticMeshComponent>();
		StarDome = Dome;
		Dome->SetStaticMesh(SphereMesh);
		Dome->SetMaterial(0, StarMid);
		Dome->SetRelativeScale3D(FVector(100000.0));
		Dome->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dome->SetCastShadow(false);
		Dome->SetVisibleInRayTracing(false);
		Dome->bAffectDistanceFieldLighting = false;
		Dome->bAffectDynamicIndirectLighting = false;
		Dome->bVisibleInRealTimeSkyCaptures = false;
	}
	// The lens: auto exposure inside the hour's clamps, MegaLights for every local light (as the card room), Lumen's
	// reflections on the glass, the grade (ApplyHour), the player's grain and fringe (ApplyLens).
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
	S.HistogramLogMax = 18.0f;
	S.bOverride_AutoExposureMinBrightness = true;
	S.bOverride_AutoExposureMaxBrightness = true;
	S.bOverride_AutoExposureBias = true;
	S.bOverride_AutoExposureSpeedUp = true;
	S.AutoExposureSpeedUp = 2.0f;
	S.bOverride_AutoExposureSpeedDown = true;
	S.AutoExposureSpeedDown = 1.2f;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = 0;
	S.bOverride_LocalExposureHighlightContrastScale = true;
	S.LocalExposureHighlightContrastScale = 0.70f;
	S.bOverride_LocalExposureShadowContrastScale = true;
	S.LocalExposureShadowContrastScale = 0.90f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.65f;
	S.bOverride_BloomThreshold = true;
	S.BloomThreshold = 1.0f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.35f;
	S.bOverride_LensFlareIntensity = true;
	S.LensFlareIntensity = 0.0f;
	S.bOverride_FilmGrainIntensity = true;
	S.bOverride_SceneFringeIntensity = true;
	S.bOverride_MotionBlurAmount = true;
	S.bOverride_WhiteTemp = true;
	S.bOverride_ColorSaturation = true;
	S.bOverride_ColorContrast = true;
	S.bOverride_ColorGainHighlights = true;
	S.bOverride_ColorOffsetShadows = true;
	S.bOverride_ColorSaturationShadows = true;
}

// ------------------------------------------------------------------ the fixtures (spec 4.3)

void AEmbercrestStage::BuildLights()
{
	// The canopy's downlights, the grazers along its slats.
	for (const double X : DownlightX)
	{
		for (const double Y : DownlightY)
		{
			USpotLightComponent* L = SpotAt(RuleDimByDay, 2000.0f, FVector(X, Y, 694.0), FRotator(-90.0f, 0.0f, 0.0f), 3000.0f, 0, 1600.0f, 22.0f, 48.0f, true, 0.4f);
			L->SetSourceRadius(6.0f);
		}
	}
	for (const FVector2D& Col : CanopyColumns)
	{
		URectLightComponent* G = RectAt(RuleNight, 220.0f, FVector(Col.X, Col.Y, 480.0), FRotator(70.0f, 0.0f, 0.0f), 2700.0f, 0, 1400.0f, 120.0f, 20.0f, false, 0.0f);
		G->SetBarnDoorAngle(60.0f);
	}
	for (const double Y : {-800.0, 800.0})
	{
		URectLightComponent* G = RectAt(RuleNight, 220.0f, FVector(80.0, Y, 650.0), FRotator(15.0f, 0.0f, 0.0f), 2700.0f, 0, 1400.0f, 120.0f, 20.0f, false, 0.0f);
		G->SetBarnDoorAngle(60.0f);
	}
	// The lobby's light pouring out through the glass, and the room's own (the chandelier, the feature wall, the desk, the portal).
	for (const double Y : {-900.0, -300.0, 300.0, 900.0})
	{
		RectAt(RuleAlways, 1800.0f, FVector(-150.0, Y, 450.0), FRotator(-20.0f, 0.0f, 0.0f), 3000.0f, 0, 2200.0f, 500.0f, 300.0f, true, 0.0f);
	}
	PointAt(RuleAlways, 3000.0f, FVector(-700.0, 0.0, 900.0), 2200.0f, 0, 2500.0f, 80.0f, false, 0.0f);
	RectAt(RuleAlways, 800.0f, FVector(-1500.0, 0.0, 1100.0), FRotator(-35.0f, 180.0f, 0.0f), 2700.0f, 0, 1600.0f, 1200.0f, 300.0f, false, 0.0f);
	PointAt(RuleAlways, 400.0f, FVector(-1350.0, 1150.0, 260.0), 2900.0f, 0, 900.0f, 30.0f, false, 0.0f);
	RectAt(RuleAlways, 500.0f, FVector(-1400.0, -1450.0, 250.0), FRotator(0.0f, 90.0f, 0.0f), 0.0f, 0xffa060, 1000.0f, 500.0f, 400.0f, false, 0.0f);
	RectAt(RuleAlways, 300.0f, FVector(-1650.0, 0.0, 640.0), FRotator(80.0f, 0.0f, 0.0f), 2700.0f, 0, 900.0f, 3000.0f, 40.0f, false, 0.0f);
	// Uplights grazing the stone along both wings (none under the marquee: it lights itself) and on the entry's piers.
	for (double Y = -5600.0; Y <= -2200.0; Y += 400.0)
	{
		if (Y < -4400.0 - 1.0 || Y > -3200.0 + 1.0)
		{
			SpotAt(RuleNight, 500.0f, FVector(400.0, Y, 20.0), FRotator(88.0f, 180.0f, 0.0f), 3000.0f, 0, 1800.0f, 8.0f, 22.0f, true, 0.0f);
		}
	}
	for (double Y = 2200.0; Y <= 5400.0; Y += 400.0)
	{
		SpotAt(RuleNight, 500.0f, FVector(400.0, Y, 20.0), FRotator(88.0f, 180.0f, 0.0f), 3000.0f, 0, 1800.0f, 8.0f, 22.0f, true, 0.0f);
	}
	for (const double Y : {-1720.0, 1720.0})
	{
		SpotAt(RuleNight, 500.0f, FVector(330.0, Y, 5.0), FRotator(88.0f, 180.0f, 0.0f), 3000.0f, 0, 1800.0f, 8.0f, 22.0f, true, 0.0f);
	}
	// The marquee's own light on the plaza.
	MarqueeSpill = RectAt(RuleNight, 1200.0f, FVector(380.0, -3800.0, 800.0), FRotator(0.0f, 0.0f, 0.0f), 0.0f, 0xffa070, 2600.0f, 1600.0f, 600.0f, false, 0.0f);
	// The lamp posts' pools.
	for (const FVector& P : LampPosts)
	{
		const double Z = FMath::Abs(P.Y) > 3300.0 && P.X > 2125.0 && P.X < XBay1 ? ArmZ(P.X) : P.Z;
		USpotLightComponent* L = SpotAt(RuleNight, 900.0f, FVector(P.X, P.Y, Z + 428.0), FRotator(-90.0f, 0.0f, 0.0f), 2700.0f, 0, 1800.0f, 35.0f, 70.0f, true, 1.2f);
		L->SetSourceRadius(10.0f);
	}
	// The step lights under each nosing (five a flight).
	for (const FVector2D& F : {FVector2D(XFlight1, ZPlatform), FVector2D(XFlight2, ZLandA), FVector2D(XFlight3, ZLandB)})
	{
		for (int32 K = 0; K < 5; ++K)
		{
			RectAt(RuleNight, 30.0f, FVector(F.X - 40.0 * K + 2.0, 0.0, F.Y + 15.0 * (K + 1) - 3.0), FRotator(-90.0f, 0.0f, 0.0f), 3000.0f, 0, 300.0f, 700.0f, 2.0f, false, 0.0f);
		}
	}
	// The fire bowls: gas flames, lit at every hour, flickering (UpdateFires).
	for (FEcxFire& F : Fires)
	{
		F.Light = PointAt(RuleFire, 500.0f, F.At, 1900.0f, 0, 1500.0f, 30.0f, true, 3.0f);
	}
	// The monument's uplights, the shelter's strip, inside the crown, the terrace's lanterns, the grill's light out of its glass.
	for (const double Y : {-2400.0, -1700.0})
	{
		const FVector At(4690.0, Y, ZLandA + 5.0);
		const FVector Aim(4550.0, -2050.0 + (Y + 2050.0) * 0.6, ZLandA + 250.0);
		SpotAt(RuleNight, 400.0f, At, (Aim - At).Rotation(), 3000.0f, 0, 1200.0f, 10.0f, 30.0f, true, 0.0f);
	}
	RectAt(RuleNight, 150.0f, FVector(5060.0, -1200.0, ZPlatform + 262.0), FRotator(-90.0f, 0.0f, 0.0f), 4300.0f, 0, 900.0f, 700.0f, 20.0f, false, 0.0f);
	for (const double Y : {1700.0, 2600.0, 3500.0, 4400.0})
	{
		RectAt(RuleCrown, 3500.0f, FVector(-3700.0, Y, 6400.0), FRotator(80.0f, 0.0f, 0.0f), 0.0f, EmberHex, 2500.0f, 1500.0f, 100.0f, false, 0.0f);
	}
	for (const double Y : {3600.0, 4800.0})
	{
		PointAt(RuleNight, 250.0f, FVector(650.0, Y, 950.0), 2400.0f, 0, 900.0f, 8.0f, false, 0.0f);
	}
	for (const double Y : {2600.0, 3400.0, 4200.0})
	{
		RectAt(RuleAlways, 900.0f, FVector(150.0, Y, 300.0), FRotator(0.0f, 0.0f, 0.0f), 2800.0f, 0, 1800.0f, 400.0f, 250.0f, true, 0.0f);
	}
}

// ------------------------------------------------------------------ the hour

void AEmbercrestStage::SetHour(double InHour)
{
	Hour = FMath::Fmod(FMath::Fmod(InHour, 24.0) + 24.0, 24.0);
	SunEl = SunElevation(Hour);
	if (FMath::Abs(Hour - AppliedHour) > 0.005 || CVarGlow.GetValueOnGameThread() != AppliedGlowScale || TuningKey() != AppliedTuning)
	{
		ApplyHour();
	}
}

void AEmbercrestStage::ApplyHour()
{
	if (!bBuilt)
	{
		return;
	}
	AppliedHour = Hour;
	AppliedGlowScale = CVarGlow.GetValueOnGameThread();
	AppliedTuning = TuningKey();
	const double El = SunEl;
	const FHourRow R = HourRowAt(El);
	const float GlowK = FMath::Max(0.0f, AppliedGlowScale);
	// Dusk to dawn: the fixtures on below +4 degrees (a photocell's ramp to +2), the moon up past -4, the stars past -4..-12.
	const float NightOn = BelowEl(El, 4.0, 2.0);
	const float MoonUp = BelowEl(El, -4.0, -10.0);
	const float Stars = BelowEl(El, -4.0, -12.0);
	const float Day = 1.0f - BelowEl(El, 12.0, -2.0);
	// The exposure's clamps, nudged by the tunables (day and night weighted by how far the sun is from each).
	const float NightW = BelowEl(El, -2.0, -14.0);
	const float DayW = 1.0f - BelowEl(El, 12.0, 4.0);
	EvMin = R.EvMin + CVarNightEV.GetValueOnGameThread() * NightW + CVarDayEV.GetValueOnGameThread() * DayW;
	EvMax = FMath::Max(EvMin + 0.5f, R.EvMax + CVarNightEV.GetValueOnGameThread() * NightW + CVarDayEV.GetValueOnGameThread() * DayW);

	// The sun and the moon.
	if (Sun)
	{
		const double Az = SunAzimuth(Hour);
		Sun->SetRelativeRotation(FRotator(static_cast<float>(-El), static_cast<float>(Az + 270.0), 0.0f));
		Sun->SetVisibility(El > -4.0);
	}
	if (Moon)
	{
		Moon->SetIntensity(1.6f * MoonUp);
		Moon->SetVisibility(MoonUp > 0.001f);
	}
	if (Clouds)
	{
		// Thin cirrus and altocumulus by day, lit pink at dusk; gone by -10 degrees.
		const float Cover = (CVarClouds.GetValueOnGameThread() != 0 ? 1.0f : 0.0f) * (1.0f - BelowEl(El, -6.0, -10.0));
		Clouds->SetVisibility(Cover > 0.01f);
		if (CloudMid)
		{
			TArray<FMaterialParameterInfo> Infos;
			TArray<FGuid> Ids;
			CloudMid->Parent->GetAllScalarParameterInfo(Infos, Ids);
			for (const FMaterialParameterInfo& Info : Infos)
			{
				float Base = 0.0f;
				if (Info.Name.ToString().Contains(TEXT("Coverage"), ESearchCase::IgnoreCase) && CloudMid->Parent->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Base))
				{
					CloudMid->SetScalarParameterValue(Info.Name, Base * 0.45f * Cover);
				}
			}
		}
	}
	if (StarDome)
	{
		// By day and well into dusk the sphere adds nothing: no full-screen translucent pass for it then.
		StarDome->SetVisibility(Stars > 0.001f);
	}
	if (StarMid)
	{
		StarMid->SetScalarParameterValue(TEXT("Night"), Stars);
		if (Moon)
		{
			StarMid->SetVectorParameterValue(TEXT("MoonDir"), FLinearColor(-Moon->GetComponentRotation().Vector()));
		}
	}
	if (SkyLight)
	{
		SkyLight->SetLightColor(FMath::Lerp(FLinearColor::White, Srgb(0xa8b8d8), MoonUp));
		if (FMath::Abs(Hour - CapturedHour) > 0.25)
		{
			CapturedHour = static_cast<float>(Hour);
			if (HasActorBegunPlay())
			{
				SkyLight->RecaptureSky();
			}
		}
	}
	if (Fog)
	{
		Fog->SetVolumetricFog(CVarVolFog.GetValueOnGameThread() != 0 && NightOn > 0.01f);
	}

	// The game's glows, each family its color and strength for the hour (nits over the color's luminance).
	for (const FEcxGlow& G : Glows)
	{
		if (!G.Mid)
		{
			continue;
		}
		uint32 Hex = EmberHex;
		float Nits = R.Ember;
		switch (G.Family)
		{
		case GlowWarm:
			Hex = WarmHex;
			Nits = R.Warm;
			break;
		case GlowCool:
			Hex = CoolHex;
			Nits = R.Warm * 1.2f;
			break;
		case GlowAsh:
			Hex = AshHex;
			Nits = R.Ash;
			break;
		case GlowCrown:
			Nits = R.Crown;
			break;
		case GlowLantern:
			Hex = WarmHex;
			Nits = R.Warm * 1.6f * NightOn;
			break;
		case GlowSlope:
			Hex = 0xffb066;
			Nits = 400.0f * NightOn;
			break;
		case GlowInterior:
			Hex = 0xffb070;
			Nits = 140.0f;
			break;
		default:
			break;
		}
		const FLinearColor C = Srgb(Hex);
		const float Strength = Nits / Luma(C) * GlowK * G.Scale;
		G.Mid->SetVectorParameterValue(TEXT("BaseColor"), C);
		G.Mid->SetVectorParameterValue(TEXT("Color"), C);
		G.Mid->SetVectorParameterValue(TEXT("Tint"), C);
		G.Mid->SetScalarParameterValue(TEXT("Emissive"), Strength);
		G.Mid->SetScalarParameterValue(TEXT("Strength"), Strength);
	}
	// The light baked into the building (the casino, the grill, the rooms, the city below): low by day, never below a room's floor.
	const float LitK = FMath::Max(0.0f, CVarLit.GetValueOnGameThread());
	for (const FEcxLit& L : LitMeshes)
	{
		UStaticMeshComponent* C = L.Mesh;
		const UStaticMesh* Source = C ? C->GetStaticMesh().Get() : nullptr;
		if (!Source)
		{
			continue;
		}
		for (int32 Slot = 0; Slot < L.Floors.Num(); ++Slot)
		{
			if (L.Floors[Slot] < 0.0f)
			{
				continue;
			}
			UMaterialInterface* Baked = Source->GetMaterial(Slot);
			if (!Baked)
			{
				continue;
			}
			const float Level = FMath::Max(L.Floors[Slot], R.Lit) * L.Scale * LitK;
			UMaterialInstanceDynamic* Dim = Cast<UMaterialInstanceDynamic>(C->GetMaterial(Slot));
			if (!Dim || Dim->Parent != Baked)
			{
				Dim = UMaterialInstanceDynamic::Create(Baked, this);
				Dim->SetFlags(RF_Transient);
				C->SetMaterial(Slot, Dim);
			}
			bool bFactor = false;
			TArray<FMaterialParameterInfo> Infos;
			TArray<FGuid> Ids;
			Baked->GetAllVectorParameterInfo(Infos, Ids);
			for (const FMaterialParameterInfo& Info : Infos)
			{
				// As DimImportedGlow (StreetStage.cpp) reads the glTF importer's names: no underscores, no spaces.
				const FString K = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
				FLinearColor Glow;
				if ((K.StartsWith(TEXT("emissivefactor")) || K == TEXT("emissivecolor")) && Baked->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Glow))
				{
					bFactor = true;
					Dim->SetVectorParameterValue(Info.Name, FLinearColor(Glow.R * Level, Glow.G * Level, Glow.B * Level, Glow.A));
				}
			}
			if (!bFactor)
			{
				Infos.Reset();
				Ids.Reset();
				Baked->GetAllScalarParameterInfo(Infos, Ids);
				for (const FMaterialParameterInfo& Info : Infos)
				{
					const FString K = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
					float Strength = 0.0f;
					if ((K == TEXT("emissivestrength") || K == TEXT("emissiveintensity")) && Baked->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Strength))
					{
						Dim->SetScalarParameterValue(Info.Name, Strength * Level);
					}
				}
			}
		}
	}
	// The materials that read the hour: the snow's glints, the flames paler by day, the mist under the city's glow.
	for (UMaterialInstanceDynamic* M : HourMids)
	{
		if (M)
		{
			M->SetScalarParameterValue(TEXT("Day"), Day);
			M->SetScalarParameterValue(TEXT("Night"), MoonUp);
			// The stand-in tower's rooms (M_StreetWindow) and city (M_StreetCity) were made for the street's darker night.
			M->SetScalarParameterValue(TEXT("Glow"), FMath::Lerp(6.0f, 1.0f, Day));
			M->SetScalarParameterValue(TEXT("Strength"), 8.0f + 2500.0f * Day * Day);
			M->SetVectorParameterValue(TEXT("UnderGlow"), Srgb(0xffa860) * (0.6f * MoonUp));
		}
	}
	// The local lights under their rules.
	for (const FEcxLight& E : Lights)
	{
		ULightComponent* L = E.Light;
		if (!L)
		{
			continue;
		}
		float K = 1.0f;
		switch (E.Rule)
		{
		case RuleDimByDay:
			K = FMath::Lerp(0.4f, 1.0f, NightOn);
			break;
		case RuleNight:
		case RuleCrown:
			K = NightOn;
			break;
		case RuleFire:
		case RuleManual:
			continue;
		default:
			break;
		}
		L->SetIntensity(E.Level * K);
		L->SetVisibility(K > 0.001f);
	}
	// Steam rises off the heated pools into the cold, seen at dusk and at night.
	for (UStaticMeshComponent* Card : Steam)
	{
		if (Card)
		{
			Card->SetVisibility(NightOn > 0.01f);
		}
	}
	// The grade: day, golden and night presets blended by the sun.
	if (Lens)
	{
		const float WDay = static_cast<float>(FMath::Clamp((El - 4.0) / 8.0, 0.0, 1.0));
		const float WNight = static_cast<float>(FMath::Clamp((-2.0 - El) / 6.0, 0.0, 1.0));
		const float WGold = FMath::Max(0.0f, 1.0f - WDay - WNight);
		FPostProcessSettings& S = Lens->Settings;
		S.WhiteTemp = 6500.0f * WDay + 6000.0f * WGold + 5200.0f * WNight;
		const float Sat = 1.0f * WDay + 1.08f * WGold + 1.0f * WNight;
		S.ColorSaturation = FVector4(1.0, 1.0, 1.0, Sat);
		const float Contrast = 1.04f * WDay + 1.04f * WGold + 1.06f * WNight;
		S.ColorContrast = FVector4(1.0, 1.0, 1.0, Contrast);
		S.ColorGainHighlights = FVector4(1.0 + 0.03 * WGold, 1.0, 1.0 - 0.04 * WGold, 1.0);
		S.ColorOffsetShadows = FVector4(0.0, 0.002 * WNight, 0.006 * WNight, 0.0);
		S.ColorSaturationShadows = FVector4(1.0, 1.0, 1.0, 1.0 - 0.15 * WNight);
	}
	if (MarqueeMid)
	{
		MarqueeMid->SetScalarParameterValue(TEXT("Strength"), R.Screens * GlowK);
	}
	if (TickerMid)
	{
		TickerMid->SetScalarParameterValue(TEXT("Strength"), R.Screens * 0.8f * GlowK);
	}
	for (UMaterialInstanceDynamic* M : BusSignMids)
	{
		if (M)
		{
			M->SetScalarParameterValue(TEXT("Strength"), FMath::Max(150.0f, R.Screens * 0.5f) * GlowK);
		}
	}
	ApplyLens();
}

void AEmbercrestStage::ApplyLens()
{
	if (!Lens)
	{
		return;
	}
	FPostProcessSettings& S = Lens->Settings;
	S.AutoExposureMinBrightness = EvMin;
	S.AutoExposureMaxBrightness = EvMax;
	S.AutoExposureBias = ExposureBias + BrightnessBias;
	S.FilmGrainIntensity = 0.08f * GrainScale;
	S.SceneFringeIntensity = 0.25f * FringeScale;
	S.MotionBlurAmount = bMotionBlur ? 0.3f : 0.0f;
}

void AEmbercrestStage::SetLensOptions(float InBrightnessBias, float InGrainScale, float InFringeScale, bool bInMotionBlur)
{
	BrightnessBias = InBrightnessBias;
	GrainScale = InGrainScale;
	FringeScale = InFringeScale;
	bMotionBlur = bInMotionBlur;
	ApplyLens();
}

void AEmbercrestStage::UpdateFires(float Dt)
{
	// intensity x (0.78 + 0.22 n1 + 0.08 n2): n1 smoothed noise at 2 Hz, n2 at 9 Hz, each bowl its own (spec 4.3).
	for (FEcxFire& F : Fires)
	{
		const uint32 S = static_cast<uint32>(F.Seed);
		const float K = 0.78f + 0.22f * Noise1(Clock * 2.0, S) + 0.08f * Noise1(Clock * 9.0, S + 17u);
		if (F.Light)
		{
			F.Light->SetIntensity(500.0f * K);
		}
		if (F.Flame)
		{
			F.Flame->SetScalarParameterValue(TEXT("Flicker"), K);
		}
		if (F.Media && !F.MediaParam.IsNone())
		{
			F.Media->SetVectorParameterValue(F.MediaParam, F.MediaGlow * K);
		}
	}
}

void AEmbercrestStage::UpdateSteam(const FVector& Camera, float Dt)
{
	if (Steam.Num() == 0 || !Steam[0] || !Steam[0]->IsVisible())
	{
		return;
	}
	const FTransform& T = GetActorTransform();
	for (int32 I = 0; I < Steam.Num() && I < SteamBase.Num(); ++I)
	{
		UStaticMeshComponent* Card = Steam[I];
		if (!Card)
		{
			continue;
		}
		// Each puff rises 0.3 m/s over a couple of meters and starts again, turned to the camera.
		const float Life = FMath::Frac(Clock * 0.15f + I * 0.137f);
		const FVector Local = SteamBase[I] + FVector(FMath::Sin(Clock * 0.4f + I) * 20.0f, 0.0f, Life * 220.0f);
		const FVector World = T.TransformPosition(Local);
		FVector To = Camera - World;
		To.Z = 0.0;
		if (To.IsNearlyZero())
		{
			continue;
		}
		Card->SetWorldLocationAndRotation(World, FRotationMatrix::MakeFromZX(To.GetSafeNormal(), FVector::UpVector).Rotator());
		if (UMaterialInstanceDynamic* M = Cast<UMaterialInstanceDynamic>(Card->GetMaterial(0)))
		{
			M->SetScalarParameterValue(TEXT("Life"), Life);
		}
	}
}

// ------------------------------------------------------------------ the screens (spec 5.6)

UTextureRenderTarget2D* AEmbercrestStage::MakeTarget(int32 Width, int32 Height)
{
	if (!FApp::CanEverRender())
	{
		return nullptr;
	}
	// As the widget component makes its own: the canvas draws into it through Slate.
	UTextureRenderTarget2D* T = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
	T->ClearColor = FLinearColor::Transparent;
	T->InitCustomFormat(FMath::Max(Width, 8), FMath::Max(Height, 8), PF_B8G8R8A8, false);
	T->UpdateResourceImmediate(true);
	return T;
}

void AEmbercrestStage::RenderToTarget(UTextureRenderTarget2D* Target, const TSharedPtr<const ss::ui::DrawList>& List)
{
	if (!Target || !List.IsValid() || !FSlateApplication::IsInitialized() || !FApp::CanEverRender())
	{
		return;
	}
	if (!Renderer)
	{
		Renderer = new FWidgetRenderer(false, true);
	}
	const FVector2D Size(Target->SizeX, Target->SizeY);
	TSharedRef<SDrawListWidget> Widget = SNew(SDrawListWidget).DesiredSize(Size);
	Widget->SetDrawList(List);
	Renderer->DrawWidget(Target, Widget, Size, 0.0f);
}

void AEmbercrestStage::BuildScreens()
{
	// The marquee on the casino wing: the imported screen, or a stand-in panel 2 cm behind the bezel's lip.
	MarqueeImages[0] = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_MarqueeBg"));
	if (!MarqueeImages[0])
	{
		MarqueeImages[0] = Ecx::LoadTex(TEXT("Brand"), TEXT("T_EC_ScreenBg"));
	}
	MarqueeImages[1] = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_MarqueeSeries"));
	if (UStaticMeshComponent* Screen = Site(TEXT("SM_ECX_Marquee_Screen")))
	{
		MarqueeMid = ScreenMat(TEXT("Marquee"), false);
		SetSlot(Screen, SlotOf(Screen, 0, TEXT("ecx_screen")), MarqueeMid);
		Screen->SetCastShadow(false);
	}
	else
	{
		MarqueeMid = ScreenMat(TEXT("MarqueeStandIn"), true);
		Box(MarqueeMid, FVector(357.0, -4600.0, 500.0), FVector(358.0, -3000.0, 1100.0), false)->SetCastShadow(false);
	}
	MarqueeMid->SetScalarParameterValue(TEXT("Cells"), 640.0f);
	MarqueeMid->SetScalarParameterValue(TEXT("Grid"), 0.25f);
	MarqueeMid->SetScalarParameterValue(TEXT("CellAspect"), 768.0f / 2048.0f);
	MarqueePages[0] = MakeTarget(2048, 768);
	MarqueePages[1] = MakeTarget(2048, 768);
	if (MarqueePages[0] && MarqueePages[1])
	{
		MarqueeMid->SetTextureParameterValue(TEXT("TextA"), MarqueePages[0]);
		MarqueeMid->SetTextureParameterValue(TEXT("TextB"), MarqueePages[1]);
		// M_ScreenImage (the fallback material) shows one image: the front page.
		MarqueeMid->SetTextureParameterValue(TEXT("Image"), MarqueePages[0]);
	}
	// The monument's LED strip, both faces.
	TickerMid = ScreenMat(TEXT("Ticker"), Ecx::LoadMesh(TEXT("SM_ECX_MonumentSign_Screen")) == nullptr);
	TickerMid->SetScalarParameterValue(TEXT("Ticker"), 1.0f);
	TickerMid->SetScalarParameterValue(TEXT("Cells"), 512.0f);
	TickerMid->SetScalarParameterValue(TEXT("Grid"), 0.35f);
	TickerMid->SetScalarParameterValue(TEXT("CellAspect"), 0.5f / 8.0f);
	if (UStaticMeshComponent* Strip = Prop(TEXT("SM_ECX_MonumentSign_Screen"), FVector(4550.0, -2050.0, ZLandA), 165.0f))
	{
		SetSlot(Strip, SlotOf(Strip, 0, TEXT("ecx_screen")), TickerMid);
		Strip->SetCastShadow(false);
	}
	else
	{
		USceneComponent* Frame = NewPart<USceneComponent>();
		Frame->SetRelativeLocationAndRotation(FVector(4550.0, -2050.0, ZLandA), FRotator(0.0f, 165.0f, 0.0f));
		// One box through the slab: each face reads from its own side (the material's local UVs).
		Box(TickerMid, FVector(-61.5, -400.0, 65.0), FVector(61.5, 400.0, 115.0), false, Frame)->SetCastShadow(false);
	}
	TickerTarget = MakeTarget(2048, 128);
	if (TickerTarget)
	{
		TickerMid->SetTextureParameterValue(TEXT("TextA"), TickerTarget);
		TickerMid->SetTextureParameterValue(TEXT("Image"), TickerTarget);
	}
	SlideIndex = -1;
	SlideClock = 0.0f;
	FrontPage = 0;
	PageMix = 1.0f;
	bTickerDirty = !TickerLine.IsEmpty();
}

void AEmbercrestStage::SetMarquee(const TArray<FEcxSlide>& InSlides)
{
	Slides = InSlides;
	SlideIndex = -1;
	SlideClock = 7.0f; // the first slide at once
	PageMix = 1.0f;
}

void AEmbercrestStage::SetTicker(const FString& InLine)
{
	TickerLine = InLine;
	bTickerDirty = true;
}

void AEmbercrestStage::UpdateScreens(float Dt)
{
	if (MarqueeMid && Slides.Num() > 0)
	{
		SlideClock += Dt;
		if (SlideClock >= 7.0f || SlideIndex < 0)
		{
			// The next slide on the page behind, then a 0.6 s crossfade to it.
			const bool bFirst = SlideIndex < 0;
			SlideClock = 0.0f;
			SlideIndex = (SlideIndex + 1) % Slides.Num();
			const int32 Back = bFirst ? FrontPage : 1 - FrontPage;
			DrawSlide(Back, Slides[SlideIndex]);
			if (bFirst)
			{
				PageMix = 1.0f;
				MarqueeMid->SetScalarParameterValue(TEXT("Mix"), FrontPage == 0 ? 0.0f : 1.0f);
			}
			else
			{
				FrontPage = Back;
				PageMix = 0.0f;
			}
		}
		if (PageMix < 1.0f)
		{
			PageMix = FMath::Min(1.0f, PageMix + Dt / 0.6f);
			const float T = FMath::SmoothStep(0.0f, 1.0f, PageMix);
			// Mix 0 shows page A (0), 1 page B (1).
			MarqueeMid->SetScalarParameterValue(TEXT("Mix"), FrontPage == 1 ? T : 1.0f - T);
			if (PageMix >= 1.0f && MarqueePages[FrontPage])
			{
				MarqueeMid->SetTextureParameterValue(TEXT("Image"), MarqueePages[FrontPage]);
			}
		}
	}
	if (TickerMid && TickerTarget)
	{
		if (bTickerDirty)
		{
			bTickerDirty = false;
			DrawTicker();
		}
		// 1.2 m/s along an 8 m strip that shows 2048 of the line's pixels.
		TickerScroll = FMath::Frac(TickerScroll + Dt * (1.2f / 8.0f) * 2048.0f / FMath::Max(TickerSpan, 1.0f));
		TickerMid->SetScalarParameterValue(TEXT("Scroll"), TickerScroll);
	}
}

void AEmbercrestStage::DrawSlide(int32 Page, const FEcxSlide& Slide)
{
	UTextureRenderTarget2D* Target = MarqueePages[FMath::Clamp(Page, 0, 1)];
	if (!Target || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	UTexture* Image = MarqueeImages[Slide.Image == 1 && MarqueeImages[1] ? 1 : 0];
	if (MarqueeMid)
	{
		MarqueeMid->SetTextureParameterValue(Page == 0 ? TEXT("ImageA") : TEXT("ImageB"), Image);
		MarqueeMid->SetScalarParameterValue(Page == 0 ? TEXT("HasImageA") : TEXT("HasImageB"), Image ? 1.0f : 0.0f);
	}
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	FEcxPaint P(*List, 2048.0f, 768.0f);
	const bool bSeriesArt = Slide.Image == 1 && MarqueeImages[1] != nullptr;
	if (!Image)
	{
		// No art imported: the night over the ranges in navy, an ember line along the foot.
		P.Quad(0.0f, 0.0f, 2048.0f, 768.0f, FEcxPaint::Ink(MidnightHex), FEcxPaint::Ink(NavyHex));
		P.Rect(0.0f, 690.0f, 2048.0f, 6.0f, FEcxPaint::Ink(EmberHex, 0.9f));
	}
	if (Slide.Image == 1)
	{
		if (!bSeriesArt)
		{
			// The series lockup drawn: the mark, EMBERCREST, POKER SERIES, the line under it.
			P.Mark(1024.0f, 250.0f, 300.0f);
			SlideLine(P, TEXT("EMBERCREST"), 1024.0f, 480.0f, 170.0f, 900, AshHex, 1800.0f, 1);
			SlideLine(P, TEXT("POKER SERIES"), 1024.0f, 600.0f, 84.0f, 700, EmberHex, 1600.0f, 1);
			SlideLine(P, TEXT("PLAY  \xB7  COMPETE  \xB7  RISE"), 1024.0f, 700.0f, 54.0f, 700, GoldHex, 1600.0f, 1);
		}
	}
	else
	{
		if (!MarqueeImages[0])
		{
			P.Mark(360.0f, 330.0f, 420.0f);
			SlideLine(P, TEXT("EMBERCREST"), 360.0f, 560.0f, 96.0f, 900, AshHex, 640.0f, 1);
		}
		const float X = 780.0f;
		const float W = 1210.0f;
		if (Slide.Rows.Num() > 0)
		{
			SlideLine(P, Slide.Kicker, X, 150.0f, 64.0f, 700, EmberHex, W);
			for (int32 I = 0; I < Slide.Rows.Num() && I < 4; ++I)
			{
				TArray<FString> Cells;
				Slide.Rows[I].ParseIntoArray(Cells, TEXT("\t"), false);
				const float Y = 280.0f + 130.0f * I;
				SlideLine(P, Cells.Num() > 0 ? Cells[0] : FString(), X, Y, 70.0f, 700, GoldHex, 300.0f);
				SlideLine(P, Cells.Num() > 1 ? Cells[1] : FString(), X + 330.0f, Y, 70.0f, 700, AshHex, 640.0f);
				SlideLine(P, Cells.Num() > 2 ? Cells[2] : FString(), X + W, Y, 70.0f, 700, EmberHex, 220.0f, 2);
			}
		}
		else
		{
			SlideLine(P, Slide.Kicker, X, 210.0f, 76.0f, 700, EmberHex, W);
			SlideLine(P, Slide.Title, X, 380.0f, 150.0f, 900, AshHex, W);
			SlideLine(P, Slide.Detail, X, 545.0f, 80.0f, 700, GoldHex, W);
		}
	}
	RenderToTarget(Target, List);
}

void AEmbercrestStage::DrawTicker()
{
	if (!TickerTarget || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	const float Size = 84.0f;
	// One period of the line (and its gap), the target sized to it, so the strip's scroll wraps without a seam.
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	FEcxPaint P(*List, 2048.0f, 128.0f);
	const float TextW = TickerLine.IsEmpty() ? 0.0f : P.Measure(TickerLine, Size, 700);
	const int32 Span = FMath::Clamp(FMath::CeilToInt(TextW + 420.0f), 2048, 8192);
	TickerSpan = static_cast<float>(Span);
	TickerTarget->ResizeTarget(Span, 128);
	List->Width = static_cast<float>(Span);
	P.Rect(0.0f, 0.0f, static_cast<float>(Span), 128.0f, FEcxPaint::Ink(MidnightHex));
	P.Text(TickerLine, 210.0f, 66.0f, Size, 700, FEcxPaint::Ink(AshHex));
	// A small ember diamond between repeats.
	P.Poly({FVector2f(105.0f, 50.0f), FVector2f(121.0f, 66.0f), FVector2f(105.0f, 82.0f), FVector2f(89.0f, 66.0f)}, FEcxPaint::Ink(EmberHex));
	RenderToTarget(TickerTarget, List);
	TickerMid->SetScalarParameterValue(TEXT("ScrollScale"), 2048.0f / TickerSpan);
}

void AEmbercrestStage::TextPanel(FName Key, const TArray<FString>& Lines, const FVector& Center, const FVector2D& SizeCm, float Yaw, uint8 Family, float Thick, USceneComponent* Parent)
{
	// Lit letters only (M_ECX_Sign is masked by the text: the fascia shows round them), else the screen's black panel.
	UMaterialInstanceDynamic* M = nullptr;
	if (UMaterialInterface* SignParent = LoadMat(TEXT("M_ECX_Sign")))
	{
		M = UMaterialInstanceDynamic::Create(SignParent, this);
		M->SetScalarParameterValue(TEXT("LocalUV"), 1.0f);
		Materials.Add(Key, M);
	}
	else
	{
		M = ScreenMat(Key, true);
	}
	M->SetScalarParameterValue(TEXT("Cells"), 0.0f);
	M->SetScalarParameterValue(TEXT("Grid"), 0.0f);
	M->SetScalarParameterValue(TEXT("Sign"), 1.0f);
	FEcxGlow G;
	G.Mid = M;
	G.Family = Family;
	Glows.Add(G);
	UStaticMeshComponent* C = TurnedBox(M, Center, FVector(Thick, SizeCm.X, SizeCm.Y), Yaw, false, Parent);
	C->SetCastShadow(false);
	// Drawn once, about a pixel a centimeter (at most 2048 across).
	const float Scale = FMath::Min(1.4f, 2048.0f / FMath::Max(1.0f, static_cast<float>(SizeCm.X)));
	const int32 W = FMath::Max(64, FMath::RoundToInt(SizeCm.X * Scale));
	const int32 H = FMath::Max(32, FMath::RoundToInt(SizeCm.Y * Scale));
	UTextureRenderTarget2D* Target = MakeTarget(W, H);
	if (!Target || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	PanelTargets.Add(Target);
	M->SetTextureParameterValue(TEXT("TextA"), Target);
	M->SetTextureParameterValue(TEXT("Image"), Target);
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	FEcxPaint P(*List, static_cast<float>(W), static_cast<float>(H));
	if (Lines.Num() > 0 && Lines[0] == TEXT("@mark"))
	{
		P.Mark(W * 0.5f, H * 0.52f, FMath::Min(W * 0.92f, H * 1.3f));
	}
	else if (Lines.Num() == 1)
	{
		SlideLine(P, Lines[0], W * 0.5f, H * 0.52f, H * 0.78f, 900, 0xffffff, W * 0.98f, 1);
	}
	else if (Lines.Num() > 1)
	{
		// The wordmark large, the subline small and wide under it.
		SlideLine(P, Lines[0], W * 0.5f, H * 0.36f, H * 0.62f, 900, 0xffffff, W * 0.98f, 1);
		SlideLine(P, Lines[1], W * 0.5f, H * 0.84f, H * 0.2f, 700, 0xffffff, W * 0.7f, 1);
	}
	RenderToTarget(Target, List);
}
