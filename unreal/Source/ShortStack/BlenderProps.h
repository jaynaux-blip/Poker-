#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

/**
 * Props from the Blender pipeline (art/blender) are modeled with their front toward Blender's -Y. The yaw
 * that turns one to face -X here is worked out as NightOneStage does: the laptop base runs forward from its
 * hinge, so its bounds show which way the importer turned it. Without the laptop, 90 (the glTF importer's
 * usual mapping, Blender -Y to +Y).
 */
inline float BlenderImportYaw()
{
	static bool bKnown = false;
	static float Yaw = 90.0f;
	if (!bKnown)
	{
		bKnown = true;
		if (const UStaticMesh* Laptop = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ShortStack/Meshes/SM_Laptop_Base/SM_Laptop_Base.SM_Laptop_Base"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			const FVector Ahead = Laptop->GetBoundingBox().GetCenter() * FVector(1.0, 1.0, 0.0);
			if (!Ahead.IsNearlyZero())
			{
				Yaw = 180.0f - static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Ahead.Y, Ahead.X)));
			}
		}
	}
	return Yaw;
}

/** The rotation that turns a Blender prop so its front faces Yaw (degrees; 0 is +X). */
inline FRotator BlenderFacing(float Yaw)
{
	return FRotator(0.0f, BlenderImportYaw() + Yaw - 180.0f, 0.0f);
}

/**
 * Tints one of an imported prop's materials (the parked cars' paint, a hat's fabric, a frame): the props bake
 * those surfaces light grey so the tint is their color. The glTF importer's materials multiply the base color
 * texture by a factor; whatever it named that parameter, the ones that look like it are set.
 */
inline void TintImported(UMeshComponent* Mesh, int32 Slot, const FLinearColor& Tint)
{
	UMaterialInterface* Material = Mesh ? Mesh->GetMaterial(Slot) : nullptr;
	if (!Material)
	{
		return;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
	if (!Mid)
	{
		Mid = UMaterialInstanceDynamic::Create(Material, Mesh);
		Mesh->SetMaterial(Slot, Mid);
	}
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Mid->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString Key = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
		if (Key == TEXT("basecolorfactor") || Key == TEXT("basecolor") || Key == TEXT("basecolortint") || Key == TEXT("tint"))
		{
			Mid->SetVectorParameterValue(Info.Name, Tint);
		}
	}
}
