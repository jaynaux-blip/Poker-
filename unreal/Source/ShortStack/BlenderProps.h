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
 * The material slot that a prop's Blender material Index landed in. The glTF importer names each slot (and its
 * material) after the Blender material, "M_SM_Sedan_0", and needn't keep Blender's order; Index itself when no
 * name says. The slots' names are asked first, then the materials' own (a tint made over one goes by its parent's:
 * its own name, "MaterialInstanceDynamic_0", ends in a number too).
 */
inline int32 ImportedSlot(const UMeshComponent* Mesh, int32 Index)
{
	if (!Mesh)
	{
		return Index;
	}
	const FString Suffix = FString::Printf(TEXT("_%d"), Index);
	const TArray<FName> Slots = Mesh->GetMaterialSlotNames();
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (Slots[Slot].ToString().EndsWith(Suffix))
		{
			return Slot;
		}
	}
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		const UMaterialInterface* Material = Mesh->GetMaterial(Slot);
		if (const UMaterialInstanceDynamic* Tint = Cast<UMaterialInstanceDynamic>(Material))
		{
			Material = Tint->Parent.Get();
		}
		if (Material && Material->GetName().EndsWith(Suffix))
		{
			return Slot;
		}
	}
	return Index;
}

/**
 * Tints one of an imported prop's materials (the parked cars' paint, a hat's fabric, a frame): the props bake
 * those surfaces light grey so the tint is their color. The glTF importer's materials (Interchange's
 * MI_Default_Opaque and its kin) multiply the base color texture by the vector BaseColorFactor; that is set,
 * and any other parameter that looks like it. False when the material listed none of them (BaseColorFactor is
 * then set blind, in case the listing was incomplete).
 */
inline bool TintImported(UMeshComponent* Mesh, int32 Slot, const FLinearColor& Tint)
{
	UMaterialInterface* Material = Mesh ? Mesh->GetMaterial(Slot) : nullptr;
	if (!Material)
	{
		return false;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
	if (!Mid)
	{
		Mid = UMaterialInstanceDynamic::Create(Material, Mesh);
		Mesh->SetMaterial(Slot, Mid);
	}
	bool bTinted = false;
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Mid->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString Key = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
		// The glTF importer's factor is BaseColorFactor_RGB in this engine (BaseColorFactor in others).
		if (Key == TEXT("basecolorfactor") || Key == TEXT("basecolorfactorrgb") || Key == TEXT("basecolor") || Key == TEXT("basecolortint") || Key == TEXT("tint"))
		{
			Mid->SetVectorParameterValue(Info.Name, Tint);
			bTinted = true;
		}
	}
	if (!bTinted)
	{
		// Parameters the listing didn't report (a parent loaded late): Interchange's name, set blind.
		Mid->SetVectorParameterValue(TEXT("BaseColorFactor"), Tint);
	}
	return bTinted;
}
