// ABackRoomStage as the Riverside Casino's poker room (bCardRoom): the Sunday tournament's room.
//
// The player's table stays at the origin, like the Back Room's: it is the feature table, roped off
// under the stream's lights with a camera on the rail. Everything else hangs off RoomRoot, which turns
// in quarter turns around the feature table when the tournament moves the player to another table, so
// the view of the room changes the way it would from a new seat. The other tables stand in a grid with
// aisles between; each can be running (its pendant lit) or closed for the night (covered, dark), and
// the room empties as the field shrinks.
//
// Room space (cm): the floor at z = 0, the feature table in the middle, the player at -X facing the
// dealer at +X. The far wall (+X) carries the big tournament clock, the right wall (+Y) the bar and a
// second clock, the left wall (-Y) the doors to the casino floor, the wall behind the player the cage.

#include "BackRoomStage.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace CardRoomDetail
{
FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor(FColor((Hex >> 16) & 0xff, (Hex >> 8) & 0xff, Hex & 0xff));
}

UStaticMesh* Prop(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

// The room.
const double RX0 = -1400.0, RX1 = 1450.0;
const double RY0 = -1050.0, RY1 = 1050.0;
const double RCeil = 400.0;
const double RWall = 24.0;
// Doors to the casino floor in the left wall.
const double DoorX0 = 260.0, DoorX1 = 540.0, DoorH = 300.0;
// The rope around the feature table.
const double RopeX0 = -270.0, RopeX1 = 270.0, RopeY = 340.0;
const double GapX0 = -210.0, GapX1 = -60.0; // the opening, on the left near the player's end

// The other tables (room space), long side along Y like the feature table.
const FVector2D Slots[] = {
	{720.0, -540.0}, {720.0, 540.0}, {-720.0, -540.0}, {-720.0, 540.0}, {0.0, -790.0}, {0.0, 790.0}, {1180.0, 0.0}, {-1160.0, 0.0},
};
constexpr int32 SlotCount = 8;

} // namespace CardRoomDetail

using namespace CardRoomDetail;

// ------------------------------------------------------------------ layout

int32 ABackRoomStage::NumTableSlots()
{
	return SlotCount;
}

FTransform ABackRoomStage::TableSlotLocal(int32 Slot)
{
	const FVector2D& S = Slots[FMath::Clamp(Slot, 0, SlotCount - 1)];
	return FTransform(FRotator::ZeroRotator, FVector(S.X, S.Y, 0.0));
}

FTransform ABackRoomStage::TableSlot(int32 Slot) const
{
	return TableSlotLocal(Slot) * (RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform());
}

void ABackRoomStage::SetRoomTurn(int32 QuarterTurns)
{
	RoomTurn = ((QuarterTurns % 4) + 4) % 4;
	if (RoomRoot)
	{
		RoomRoot->SetRelativeRotation(FRotator(0.0f, 90.0f * RoomTurn, 0.0f));
	}
}

void ABackRoomStage::SetTableOpen(int32 Slot, bool bOpen)
{
	if (TableLights.IsValidIndex(Slot) && TableLights[Slot])
	{
		TableLights[Slot]->SetIntensity(bOpen ? 260.0f : 12.0f);
	}
	if (TableCovers.IsValidIndex(Slot) && TableCovers[Slot])
	{
		TableCovers[Slot]->SetVisibility(!bOpen);
	}
	if (TableGlows.IsValidIndex(Slot) && TableGlows[Slot])
	{
		TableGlows[Slot]->SetVisibility(bOpen);
	}
}

void ABackRoomStage::SetBoard(const FString& Title, const FString& Level, const FString& Clock, const FString& Blinds, const FString& Next, const FString& Field)
{
	// Each screen has six lines in this order.
	const FString* Lines[6] = {&Title, &Level, &Clock, &Blinds, &Next, &Field};
	for (int32 I = 0; I < BoardLines.Num(); ++I)
	{
		if (BoardLines[I])
		{
			BoardLines[I]->SetText(FText::FromString(*Lines[I % 6]));
		}
	}
}

void ABackRoomStage::SetOnAir(bool bOn)
{
	bOnAir = bOn;
}

TArray<FVector> ABackRoomStage::CardRoomWalkIn(const FVector& Eye) const
{
	// Through the doors from the casino floor, down the aisle past the next table, through the gap in
	// the rope and round behind the chair. Room space, turned with the room (the feature table doesn't
	// turn, so the last points are in world space).
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	TArray<FVector> Out;
	for (const FVector& P : {FVector(400.0, RY0 - 150.0, 166.0), FVector(400.0, RY0 + 60.0, 166.0), FVector(390.0, -560.0, 165.0), FVector(80.0, -470.0, 165.0)})
	{
		Out.Add(Room.TransformPosition(P));
	}
	Out.Add(FVector(-135.0, -400.0, 165.0));
	Out.Add(FVector(-150.0, -300.0, 164.0));
	Out.Add(FVector(-200.0, -120.0, 162.0));
	Out.Add(FVector(-160.0, 6.0, 148.0));
	Out.Add(Eye);
	return Out;
}

TArray<FVector> ABackRoomStage::CardRoomWalkOut(const FVector& Eye) const
{
	TArray<FVector> In = CardRoomWalkIn(Eye);
	TArray<FVector> Out;
	Out.Add(Eye);
	Out.Add(Eye + FVector(-34.0, -16.0, 44.0));
	for (int32 I = In.Num() - 3; I >= 1; --I)
	{
		Out.Add(In[I]);
	}
	// Out through the doors: the slot floor's lights swim up as the night fades.
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	Out.Add(Room.TransformPosition(FVector(400.0, RY0 - 40.0, 166.0)));
	return Out;
}

// ------------------------------------------------------------------ building

UTextRenderComponent* ABackRoomStage::AddText(const FString& Text, const FVector& At, const FRotator& Facing, float Size, const FLinearColor& Color, USceneComponent* Parent)
{
	UTextRenderComponent* T = NewPart<UTextRenderComponent>(Parent);
	T->SetRelativeLocationAndRotation(At, Facing);
	T->SetText(FText::FromString(Text));
	T->SetWorldSize(Size);
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetTextRenderColor(Color.ToFColor(true));
	if (TextMaterial)
	{
		T->SetTextMaterial(TextMaterial);
	}
	T->SetCastShadow(false);
	return T;
}

void ABackRoomStage::BuildCardRoom()
{
	TextMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_ScreenText.M_ScreenText"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	RoomRoot = NewPart<USceneComponent>();
	RoomRoot->SetRelativeRotation(FRotator(0.0f, 90.0f * RoomTurn, 0.0f));
	BoardLines.Reset();
	TableLights.Reset();
	TableCovers.Reset();
	TableGlows.Reset();
	USceneComponent* R = RoomRoot;

	// ---- the shell: patterned carpet, walnut panelling with a dark fabric wall above, a black ceiling.
	UMaterialInterface* Carpet = Room(TEXT("Carpet"), 5, 0x3d0f19, 0xb38a3e);
	UMaterialInterface* Panels = Room(TEXT("Panels"), 6, 0x4a2a18, 0x8c6a3a, 1.05f);
	UMaterialInterface* Fabric = Room(TEXT("Fabric"), 4, 0x1b1d2a, 0, 0.0f, 0.0f);
	UMaterialInterface* Ceil = Room(TEXT("BlackCeiling"), 7, 0x121214);
	auto RBox = [&](UMaterialInterface* M, const FVector& Min, const FVector& Max, bool bShadows) {
		return AddMesh(CubeMesh, M, (Min + Max) * 0.5, (Max - Min) / 100.0, FRotator::ZeroRotator, R, bShadows);
	};
	const double T = RWall;
	RBox(Carpet, FVector(RX0 - T, RY0 - 600.0, -T), FVector(RX1 + T, RY1 + T, 0.0), true);
	RBox(Ceil, FVector(RX0 - T, RY0 - T, RCeil), FVector(RX1 + T, RY1 + T, RCeil + T), true);
	// Panelling to 1.1 m and fabric above, on each wall (the left wall around the doors).
	auto Wall = [&](const FVector& Min, const FVector& Max) {
		RBox(Panels, Min, FVector(Max.X, Max.Y, 110.0), true);
		RBox(Fabric, FVector(Min.X, Min.Y, 110.0), Max, true);
	};
	Wall(FVector(RX1, RY0 - T, 0.0), FVector(RX1 + T, RY1 + T, RCeil));
	Wall(FVector(RX0 - T, RY0 - T, 0.0), FVector(RX0, RY1 + T, RCeil));
	Wall(FVector(RX0, RY1, 0.0), FVector(RX1, RY1 + T, RCeil));
	Wall(FVector(RX0, RY0 - T, 0.0), FVector(DoorX0, RY0, RCeil));
	Wall(FVector(DoorX1, RY0 - T, 0.0), FVector(RX1, RY0, RCeil));
	RBox(Fabric, FVector(DoorX0, RY0 - T, DoorH), FVector(DoorX1, RY0, RCeil), true);
	// Brass trim on the chair rail.
	UMaterialInterface* Brass = Room(TEXT("Brass"), 4, 0xb8913f, 0, 0.0f, 0.95f);
	RBox(Brass, FVector(RX1 - 3.0, RY0, 108.0), FVector(RX1, RY1, 113.0), true);
	RBox(Brass, FVector(RX0, RY0, 108.0), FVector(RX0 + 3.0, RY1, 113.0), true);
	RBox(Brass, FVector(RX0, RY1 - 3.0, 108.0), FVector(RX1, RY1, 113.0), true);
	// Square columns down the room, panelled.
	for (const FVector2D& C : {FVector2D(-380.0, -1000.0), FVector2D(-380.0, 1000.0), FVector2D(1000.0, -1000.0), FVector2D(1000.0, 1000.0)})
	{
		RBox(Panels, FVector(C.X - 30.0, C.Y - 30.0, 0.0), FVector(C.X + 30.0, C.Y + 30.0, RCeil), true);
	}

	// ---- the casino floor through the doors: bright, busy, out of focus from the tables.
	UMaterialInterface* FloorBeyond = Room(TEXT("CasinoFloorCarpet"), 5, 0x2a0d1a, 0xc79a42);
	RBox(FloorBeyond, FVector(DoorX0 - 700.0, RY0 - 900.0, -T), FVector(DoorX1 + 700.0, RY0 - T, 0.0), true);
	RBox(Room(TEXT("CasinoCeiling"), 7, 0x2a2420), FVector(DoorX0 - 700.0, RY0 - 900.0, 480.0), FVector(DoorX1 + 700.0, RY0 - T, 500.0), true);
	const FLinearColor SlotColors[] = {Srgb(0xff3d7f), Srgb(0x3dd6ff), Srgb(0xffc23d), Srgb(0x7dff6a), Srgb(0xb36bff), Srgb(0xff8a3d)};
	UMaterialInterface* Cabinet = Room(TEXT("SlotCabinet"), 4, 0x1a1a1f, 0, 0.0f, 0.4f);
	int32 K = 0;
	for (int32 Row = 0; Row < 2; ++Row)
	{
		for (int32 I = 0; I < 9; ++I, ++K)
		{
			const double X = DoorX0 - 520.0 + I * 115.0;
			const double Y = RY0 - 380.0 - Row * 300.0;
			// Cabinet and its rounded crown, the screen set in a chrome bezel, a lit topper, a button deck.
			RBox(Cabinet, FVector(X - 38.0, Y - 30.0, 0.0), FVector(X + 38.0, Y + 30.0, 168.0), false);
			AddMesh(CylinderMesh, Cabinet, FVector(X, Y, 168.0), FVector(0.76, 0.6, 0.12), FRotator(0.0f, 0.0f, 90.0f), R, false);
			const FLinearColor Lit = SlotColors[K % 6];
			AddMesh(CubeMesh, Room(TEXT("SlotChrome"), 4, 0xc9c9cc, 0, 0.0f, 0.95f), FVector(X, Y + 30.5, 122.0), FVector(0.62, 0.01, 0.56), FRotator::ZeroRotator, R, false);
			AddMesh(CubeMesh, Glow(*FString::Printf(TEXT("SlotScreen%d"), K % 6), Lit * 0.55f + FLinearColor(0.05f, 0.05f, 0.08f), 9.0f), FVector(X, Y + 31.2, 122.0), FVector(0.54, 0.01, 0.46), FRotator::ZeroRotator, R, false);
			AddMesh(CubeMesh, Glow(*FString::Printf(TEXT("SlotTop%d"), (K + 2) % 6), SlotColors[(K + 2) % 6], 22.0f), FVector(X, Y + 8.0, 196.0), FVector(0.7, 0.14, 0.26), FRotator::ZeroRotator, R, false);
			AddMesh(CubeMesh, Cabinet, FVector(X, Y + 42.0, 86.0), FVector(0.7, 0.26, 0.05), FRotator(-12.0f, 0.0f, 0.0f), R, false);
			AddMesh(CubeMesh, Glow(TEXT("SlotButtons"), Srgb(0xffe08a), 12.0f), FVector(X, Y + 44.0, 89.0), FVector(0.5, 0.06, 0.012), FRotator(-12.0f, 0.0f, 0.0f), R, false);
		}
	}
	// Its light spilling in through the doors, warm and colored.
	URectLightComponent* Spill = NewPart<URectLightComponent>(R);
	Spill->SetRelativeLocationAndRotation(FVector((DoorX0 + DoorX1) * 0.5, RY0 - 120.0, DoorH * 0.6), FRotator(-6.0f, 90.0f, 0.0f));
	Spill->SetIntensityUnits(ELightUnits::Candelas);
	Spill->SetIntensity(420.0f);
	Spill->SetLightColor(Srgb(0xffc9a0));
	Spill->SetSourceWidth(DoorX1 - DoorX0);
	Spill->SetSourceHeight(DoorH);
	Spill->SetBarnDoorAngle(70.0f);
	Spill->SetAttenuationRadius(1400.0f);
	Spill->SetCastShadows(false);
	for (int32 I = 0; I < 3; ++I)
	{
		UPointLightComponent* Chandelier = NewPart<UPointLightComponent>(R);
		Chandelier->SetRelativeLocation(FVector(DoorX0 - 300.0 + I * 380.0, RY0 - 520.0, 420.0));
		Chandelier->SetIntensityUnits(ELightUnits::Candelas);
		Chandelier->SetIntensity(900.0f);
		Chandelier->SetLightColor(Srgb(0xffe0b0));
		Chandelier->SetAttenuationRadius(900.0f);
		Chandelier->SetCastShadows(false);
		AddMesh(SphereMesh, Glow(TEXT("Chandelier"), Srgb(0xfff0d0), 40.0f), FVector(DoorX0 - 300.0 + I * 380.0, RY0 - 520.0, 430.0), FVector(0.9, 0.9, 0.5), FRotator::ZeroRotator, R, false);
	}
	// The doors themselves, swung back against the wall, and the sign over them.
	UMaterialInterface* DoorWood = Room(TEXT("DoorWood"), 6, 0x3a2112, 0x8c6a3a, 0.0f);
	AddMesh(CubeMesh, DoorWood, FVector(DoorX0 - 6.0, RY0 + 70.0, DoorH * 0.5 - 20.0), FVector(0.05, 1.4, (DoorH - 40.0) / 100.0), FRotator::ZeroRotator, R);
	AddMesh(CubeMesh, DoorWood, FVector(DoorX1 + 6.0, RY0 + 70.0, DoorH * 0.5 - 20.0), FVector(0.05, 1.4, (DoorH - 40.0) / 100.0), FRotator::ZeroRotator, R);
	AddText(TEXT("POKER ROOM"), FVector((DoorX0 + DoorX1) * 0.5, RY0 + 2.0, DoorH + 40.0), FRotator(0.0f, 90.0f, 0.0f), 34.0f, Srgb(0xffd27a), R);
	AddText(TEXT("POKER ROOM"), FVector((DoorX0 + DoorX1) * 0.5, RY0 - RWall - 2.0, DoorH + 40.0), FRotator(0.0f, -90.0f, 0.0f), 34.0f, Srgb(0xffd27a), R);

	// ---- the cage behind the player: a lit window, a brass grille, a sign.
	UMaterialInterface* Grille = Room(TEXT("Grille"), 4, 0xa98638, 0, 0.0f, 0.9f);
	RBox(Room(TEXT("CageCounter"), 6, 0x2e1a10, 0x8c6a3a, 0.0f), FVector(RX0, -260.0, 0.0), FVector(RX0 + 60.0, 260.0, 105.0), true);
	AddMesh(CubeMesh, Glow(TEXT("CageWindow"), Srgb(0xffe6c0), 8.0f), FVector(RX0 + 2.0, 0.0, 165.0), FVector(0.02, 4.8, 1.1), FRotator::ZeroRotator, R, false);
	for (int32 I = 0; I < 13; ++I)
	{
		RBox(Grille, FVector(RX0 + 8.0, -240.0 + I * 40.0, 110.0), FVector(RX0 + 10.0, -238.0 + I * 40.0, 222.0), false);
	}
	AddText(TEXT("CASHIER"), FVector(RX0 + 4.0, 0.0, 255.0), FRotator(0.0f, 0.0f, 0.0f), 30.0f, Srgb(0xffd27a), R);

	// ---- the bar along the right wall, bottles glowing on lit shelves.
	UMaterialInterface* BarWood = Room(TEXT("BarWood"), 6, 0x2a170c, 0x8c6a3a, 0.0f);
	RBox(BarWood, FVector(-200.0, RY1 - 140.0, 0.0), FVector(560.0, RY1 - 80.0, 108.0), true);
	RBox(Room(TEXT("BarTop"), 4, 0x0e0d0c, 0, 0.0f, 0.3f), FVector(-210.0, RY1 - 146.0, 108.0), FVector(570.0, RY1 - 74.0, 112.0), true);
	for (int32 Shelf = 0; Shelf < 3; ++Shelf)
	{
		const double Z = 130.0 + Shelf * 42.0;
		AddMesh(CubeMesh, Glow(TEXT("ShelfLight"), Srgb(0xffb35c), 12.0f), FVector(180.0, RY1 - 18.0, Z), FVector(7.4, 0.2, 0.02), FRotator::ZeroRotator, R, false);
		for (int32 B = 0; B < 22; ++B)
		{
			const FLinearColor Glass = SlotColors[(B * 7 + Shelf) % 6] * 0.25f + Srgb(0x8a5a20) * 0.75f;
			AddMesh(CylinderMesh, Glow(*FString::Printf(TEXT("Bottle%d"), (B * 7 + Shelf) % 6), Glass, 3.0f),
				FVector(-170.0 + B * 32.0 + (Shelf % 2) * 9.0, RY1 - 18.0, Z + 14.0), FVector(0.07, 0.07, 0.26 + 0.06 * ((B + Shelf) % 3)), FRotator::ZeroRotator, R, false);
		}
	}
	UPointLightComponent* BarLight = NewPart<UPointLightComponent>(R);
	BarLight->SetRelativeLocation(FVector(180.0, RY1 - 60.0, 200.0));
	BarLight->SetIntensityUnits(ELightUnits::Candelas);
	BarLight->SetIntensity(260.0f);
	BarLight->SetLightColor(Srgb(0xffb070));
	BarLight->SetAttenuationRadius(700.0f);
	BarLight->SetCastShadows(false);

	// ---- the tournament clocks: the big screen on the far wall, a second over the bar.
	auto Screen = [&](const FVector& At, const FRotator& Facing, double Scale) {
		const FVector Fwd = Facing.Vector();
		AddMesh(CubeMesh, Room(TEXT("ScreenBezel"), 4, 0x0a0a0b, 0, 0.0f, 0.2f), At - Fwd * 4.0, FVector(0.06, 3.9 * Scale, 2.3 * Scale), Facing, R, false);
		AddMesh(CubeMesh, Glow(TEXT("ScreenGlass"), Srgb(0x081428), 3.0f), At - Fwd * 0.5, FVector(0.02, 3.7 * Scale, 2.1 * Scale), Facing, R, false);
		const FRotator TextFacing = Facing;
		const double Lines[6][2] = {{86.0, 15.0}, {54.0, 20.0}, {8.0, 52.0}, {-40.0, 22.0}, {-66.0, 13.0}, {-90.0, 13.0}};
		const FLinearColor Colors[6] = {Srgb(0xffd27a), Srgb(0x9fd8ff), Srgb(0xffffff), Srgb(0xffffff), Srgb(0x9fd8ff), Srgb(0xcfd6e0)};
		for (int32 I = 0; I < 6; ++I)
		{
			BoardLines.Add(AddText(TEXT(""), At + Fwd * 1.0 + FVector(0.0, 0.0, Lines[I][0] * Scale), TextFacing, static_cast<float>(Lines[I][1] * Scale), Colors[I], R));
		}
	};
	Screen(FVector(RX1 - 2.0, 0.0, 255.0), FRotator(0.0f, 180.0f, 0.0f), 1.0);
	Screen(FVector(700.0, RY1 - 2.0, 270.0), FRotator(0.0f, -90.0f, 0.0f), 0.7);
	SetBoard(TEXT("RIVERSIDE SUNDAY $150"), TEXT("LEVEL 1"), TEXT("20:00"), TEXT("100 / 200"), TEXT("NEXT  200 / 300"), TEXT("SHUFFLE UP AND DEAL"));

	BuildCardRoomTables();
	BuildCardRoomLights();
	BuildCardRoomAir();
}

void ABackRoomStage::BuildCardRoomTables()
{
	// The feature table: the same table as the Back Room's, at the origin (it doesn't turn with the room).
	BuildTable();
	UStaticMesh* Chair = Prop(TEXT("SM_FoldingChair"));
	UStaticMesh* Chips = Prop(TEXT("SM_ChipStacks"));
	USceneComponent* R = RoomRoot;

	// The rope around it: brass posts, red velvet, a gap to walk through.
	UMaterialInterface* Brass = Room(TEXT("Brass"), 4, 0xb8913f, 0, 0.0f, 0.95f);
	UMaterialInterface* Velvet = Room(TEXT("Velvet"), 4, 0x6b0d16, 0, 0.0f, 0.0f);
	TArray<FVector> Posts = {FVector(RopeX0, -RopeY, 0.0), FVector(GapX0, -RopeY, 0.0), FVector(GapX1, -RopeY, 0.0), FVector(RopeX1, -RopeY, 0.0), FVector(RopeX1, RopeY, 0.0),
		FVector(RopeX0, RopeY, 0.0)};
	for (const FVector& P : Posts)
	{
		AddMesh(CylinderMesh, Brass, P + FVector(0.0, 0.0, 47.0), FVector(0.05, 0.05, 0.94));
		AddMesh(CylinderMesh, Brass, P + FVector(0.0, 0.0, 1.5), FVector(0.3, 0.3, 0.03));
		AddMesh(SphereMesh, Brass, P + FVector(0.0, 0.0, 96.0), FVector(0.08));
	}
	auto Rope = [&](const FVector& A, const FVector& B) {
		// A velvet rope sagging between two posts (three straight pieces).
		const FVector Ta = A + FVector(0.0, 0.0, 88.0);
		const FVector Tb = B + FVector(0.0, 0.0, 88.0);
		const FVector Mid = (Ta + Tb) * 0.5 - FVector(0.0, 0.0, FVector::Dist(Ta, Tb) * 0.06);
		for (const TPair<FVector, FVector>& Seg : {TPair<FVector, FVector>(Ta, (Ta + Mid) * 0.5 + FVector(0.0, 0.0, -2.0)), TPair<FVector, FVector>((Ta + Mid) * 0.5 + FVector(0.0, 0.0, -2.0), (Tb + Mid) * 0.5 + FVector(0.0, 0.0, -2.0)),
				 TPair<FVector, FVector>((Tb + Mid) * 0.5 + FVector(0.0, 0.0, -2.0), Tb)})
		{
			const FVector D = Seg.Value - Seg.Key;
			AddMesh(CylinderMesh, Velvet, (Seg.Key + Seg.Value) * 0.5, FVector(0.03, 0.03, D.Size() / 100.0), FRotationMatrix::MakeFromZ(D).Rotator(), nullptr, false);
		}
	};
	Rope(Posts[0], Posts[1]);
	Rope(Posts[2], Posts[3]);
	Rope(Posts[3], Posts[4]);
	Rope(Posts[4], Posts[5]);
	Rope(Posts[5], Posts[0]);

	// The stream: a camera on a tripod at the rail and a truss of lights over the table.
	UMaterialInterface* Black = Room(TEXT("GearBlack"), 4, 0x111113, 0, 0.0f, 0.3f);
	const FVector Cam(240.0, 300.0, 0.0);
	for (int32 L = 0; L < 3; ++L)
	{
		const float A = 120.0f * L;
		const FVector Foot = Cam + FRotator(0.0f, A, 0.0f).Vector() * 38.0;
		const FVector D = (Cam + FVector(0.0, 0.0, 140.0)) - Foot;
		AddMesh(CylinderMesh, Black, (Foot + Cam + FVector(0.0, 0.0, 140.0)) * 0.5, FVector(0.025, 0.025, D.Size() / 100.0), FRotationMatrix::MakeFromZ(D).Rotator(), nullptr, false);
	}
	const FRotator CamAim = (FVector(0.0, 0.0, 90.0) - (Cam + FVector(0.0, 0.0, 150.0))).Rotation();
	AddMesh(CubeMesh, Black, Cam + FVector(0.0, 0.0, 150.0), FVector(0.42, 0.18, 0.2), CamAim);
	AddMesh(CylinderMesh, Black, Cam + FVector(0.0, 0.0, 150.0) + CamAim.Vector() * 28.0, FVector(0.11, 0.11, 0.2), CamAim + FRotator(-90.0f, 0.0f, 0.0f));
	TallyGlow = Glow(TEXT("Tally"), Srgb(0xff1a1a), 1.0f);
	AddMesh(SphereMesh, TallyGlow, Cam + FVector(0.0, 0.0, 164.0) - CamAim.Vector() * 10.0, FVector(0.03), FRotator::ZeroRotator, nullptr, false);
	Tally = NewPart<UPointLightComponent>();
	Tally->SetRelativeLocation(Cam + FVector(0.0, 0.0, 168.0));
	Tally->SetIntensityUnits(ELightUnits::Candelas);
	Tally->SetIntensity(0.0f);
	Tally->SetLightColor(Srgb(0xff2020));
	Tally->SetAttenuationRadius(140.0f);
	Tally->SetCastShadows(false);
	AddText(TEXT("RIVERSIDE LIVE"), Cam + FVector(0.0, 0.0, 182.0), FRotator(0.0f, -120.0f, 0.0f), 7.0f, Srgb(0xff5050), nullptr);
	// The truss: a square frame of black box section over the table.
	const double TrussZ = 330.0;
	for (const FVector& S : {FVector(0.0, -190.0, TrussZ), FVector(0.0, 190.0, TrussZ)})
	{
		AddMesh(CubeMesh, Black, S, FVector(3.0, 0.12, 0.12), FRotator::ZeroRotator, nullptr, false);
	}
	for (const FVector& S : {FVector(-150.0, 0.0, TrussZ), FVector(150.0, 0.0, TrussZ)})
	{
		AddMesh(CubeMesh, Black, S, FVector(0.12, 3.9, 0.12), FRotator::ZeroRotator, nullptr, false);
	}
	AddMesh(CubeMesh, Black, FVector(0.0, 0.0, (TrussZ + RCeil) * 0.5), FVector(0.06, 0.06, (RCeil - TrussZ) / 100.0), FRotator::ZeroRotator, nullptr, false);

	// The other tables, each with its own dealer's light and a cover for when it closes.
	UMaterialInterface* Cover = Room(TEXT("TableCover"), 4, 0x15161b, 0, 0.0f, 0.0f);
	UMaterialInterface* CardBack = Room(TEXT("CardBack"), 4, 0x9a1b22, 0, 0.0f, 0.0f);
	UMaterialInterface* Fixture = Room(TEXT("Pendant"), 4, 0x16161a, 0, 0.0f, 0.5f);
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		const FTransform Frame = TableSlotLocal(Slot);
		USceneComponent* Table = NewPart<USceneComponent>(R);
		Table->SetRelativeTransform(Frame);
		if (TableMesh)
		{
			AddMesh(TableMesh, nullptr, FVector::ZeroVector, FVector(1.0), FRotator(0.0f, 90.0f, 0.0f), Table);
		}
		else
		{
			AddMesh(CubeMesh, Room(TEXT("FeltStandIn"), 0, 0x245a3b), FVector(0.0, 0.0, FeltZ - 2.0), FVector(1.0, 2.2, 0.04), FRotator::ZeroRotator, Table);
		}
		// A few cards and chips on the felt: hole cards in front of the seats, a flop in the middle.
		for (int32 S = 0; S < 7; ++S)
		{
			if (S == 4)
			{
				continue;
			}
			const FVector SeatAt = SeatEdge(S);
			const FVector2D E(SeatAt.X, SeatAt.Y);
			const FVector Out = FVector(E.X, E.Y, 0.0).GetSafeNormal2D();
			const FVector At(E.X - Out.X * 22.0, E.Y - Out.Y * 22.0, FeltZ + 0.15);
			AddMesh(CubeMesh, CardBack, At, FVector(0.064, 0.089, 0.002), FRotator(0.0f, Out.Rotation().Yaw + 90.0f + (S * 13 % 9) - 4.0f, 0.0f), Table, false);
			if (Chips)
			{
				AddMesh(Chips, nullptr, FVector(E.X - Out.X * 14.0 + Out.Y * 14.0, E.Y - Out.Y * 14.0 - Out.X * 14.0, FeltZ), FVector(0.55 + 0.1 * ((S + Slot) % 3)), FRotator(0.0f, S * 40.0f, 0.0f), Table, false);
			}
			// An empty chair at each seat (the extras bring their own when they sit).
			if (Chair)
			{
				const FTransform SeatT = ABackRoomStage::SeatTransform(S);
				const FVector ChairAt = SeatT.TransformPosition(FVector(-19.5, 0.0, 0.0));
				AddMesh(Chair, nullptr, ChairAt, FVector(1.0), SeatT.Rotator(), Table);
			}
		}
		for (int32 C = 0; C < 3; ++C)
		{
			AddMesh(CubeMesh, Room(TEXT("CardFace"), 4, 0xf2efe6, 0, 0.0f, 0.0f), FVector(-2.0, (C - 1) * 7.5, FeltZ + 0.15), FVector(0.064, 0.089, 0.002), FRotator(0.0f, 90.0f, 0.0f), Table, false);
		}
		// The cover, for a closed table.
		UStaticMeshComponent* CoverMesh = AddMesh(CubeMesh, Cover, FVector(0.0, 0.0, FeltZ + 1.5), FVector(1.4, 2.7, 0.03), FRotator::ZeroRotator, Table, false);
		CoverMesh->SetVisibility(false);
		TableCovers.Add(CoverMesh);
		// The pendant: a long dark shade with a glowing underside, and its light.
		AddMesh(CubeMesh, Fixture, FVector(0.0, 0.0, 236.0), FVector(0.7, 1.8, 0.12), FRotator::ZeroRotator, Table, false);
		UStaticMeshComponent* Underside = AddMesh(CubeMesh, Glow(TEXT("PendantGlow"), Srgb(0xfff1d6), 40.0f), FVector(0.0, 0.0, 229.5), FVector(0.62, 1.7, 0.01), FRotator::ZeroRotator, Table, false);
		TableGlows.Add(Underside);
		AddMesh(CylinderMesh, Fixture, FVector(0.0, 0.0, (242.0 + RCeil) * 0.5), FVector(0.01, 0.01, (RCeil - 242.0) / 100.0), FRotator::ZeroRotator, Table, false);
		URectLightComponent* L = NewPart<URectLightComponent>(Table);
		L->SetRelativeLocationAndRotation(FVector(0.0, 0.0, 227.0), FRotator(-90.0f, 0.0f, 0.0f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(260.0f);
		L->SetLightColor(Srgb(0xffe2bd));
		L->SetSourceWidth(170.0f);
		L->SetSourceHeight(62.0f);
		L->SetBarnDoorAngle(75.0f);
		L->SetAttenuationRadius(600.0f);
		L->SetCastShadows(false);
		TableLights.Add(L);
	}
}

void ABackRoomStage::BuildCardRoomLights()
{
	// The feature table: a broad soft key from the truss (the stream wants faces), four spots angled in
	// from the corners for shape, and the felt's bounce.
	LampLight = nullptr;
	URectLightComponent* Key = NewPart<URectLightComponent>();
	Key->SetRelativeLocationAndRotation(FVector(0.0, 0.0, 322.0), FRotator(-90.0f, 0.0f, 0.0f));
	Key->SetIntensityUnits(ELightUnits::Candelas);
	Key->SetIntensity(650.0f);
	Key->SetLightColor(Srgb(0xfff0dc));
	Key->SetSourceWidth(380.0f);
	Key->SetSourceHeight(260.0f);
	Key->SetBarnDoorAngle(80.0f);
	Key->SetAttenuationRadius(900.0f);
	Key->SetCastShadows(true);
	Key->SetVolumetricScatteringIntensity(0.3f);
	for (int32 I = 0; I < 4; ++I)
	{
		const FVector At((I % 2 == 0 ? -140.0 : 140.0), (I < 2 ? -180.0 : 180.0), 318.0);
		USpotLightComponent* Spot = NewPart<USpotLightComponent>();
		Spot->SetRelativeLocationAndRotation(At, (FVector(0.0, 0.0, 105.0) - At).Rotation());
		Spot->SetIntensityUnits(ELightUnits::Candelas);
		Spot->SetIntensity(1400.0f);
		Spot->SetLightColor(Srgb(I % 2 == 0 ? 0xfff2e0 : 0xffe4c4));
		Spot->SetOuterConeAngle(34.0f);
		Spot->SetInnerConeAngle(16.0f);
		Spot->SetSourceRadius(6.0f);
		Spot->SetAttenuationRadius(800.0f);
		Spot->SetCastShadows(I == 1);
		Spot->SetVolumetricScatteringIntensity(0.5f);
		AddMesh(CylinderMesh, Room(TEXT("GearBlack"), 4, 0x111113, 0, 0.0f, 0.3f), At, FVector(0.16, 0.16, 0.3), (FVector(0.0, 0.0, 105.0) - At).Rotation() + FRotator(-90.0f, 0.0f, 0.0f), nullptr, false);
	}
	URectLightComponent* Bounce = NewPart<URectLightComponent>();
	Bounce->SetRelativeLocationAndRotation(FVector(0.0, 0.0, FeltZ + 2.0), FRotator(90.0f, 0.0f, 0.0f));
	Bounce->SetIntensityUnits(ELightUnits::Candelas);
	Bounce->SetIntensity(90.0f);
	Bounce->SetLightColor(Srgb(0xbfd0e0));
	Bounce->SetSourceWidth(200.0f);
	Bounce->SetSourceHeight(90.0f);
	Bounce->SetAttenuationRadius(320.0f);
	Bounce->SetCastShadows(false);
	Bounce->SetVolumetricScatteringIntensity(0.0f);

	// The room: recessed downlights in the black ceiling (glowing cans) and a soft fill under them.
	UMaterialInterface* Can = Glow(TEXT("Downlight"), Srgb(0xfff2dc), 60.0f);
	for (double X = RX0 + 250.0; X < RX1; X += 420.0)
	{
		for (double Y = RY0 + 220.0; Y < RY1; Y += 420.0)
		{
			AddMesh(CylinderMesh, Can, FVector(X, Y, RCeil - 0.6), FVector(0.14, 0.14, 0.01), FRotator::ZeroRotator, RoomRoot, false);
		}
	}
	for (int32 I = 0; I < 6; ++I)
	{
		UPointLightComponent* Fill = NewPart<UPointLightComponent>(RoomRoot);
		Fill->SetRelativeLocation(FVector(RX0 + 400.0 + (I % 3) * 1000.0, I < 3 ? -600.0 : 600.0, RCeil - 40.0));
		Fill->SetIntensityUnits(ELightUnits::Candelas);
		Fill->SetIntensity(220.0f);
		Fill->SetLightColor(Srgb(0xffd9b0));
		Fill->SetAttenuationRadius(1100.0f);
		Fill->SetCastShadows(false);
		Fill->SetVolumetricScatteringIntensity(0.0f);
	}
}

void ABackRoomStage::BuildCardRoomAir()
{
	// A clean room with a little haze: the stream's lights show their beams faintly.
	Fog = NewPart<UExponentialHeightFogComponent>();
	Fog->SetFogDensity(0.02f);
	Fog->SetFogHeightFalloff(0.06f);
	Fog->SetFogInscatteringColor(FLinearColor(0.02f, 0.018f, 0.016f));
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.4f);
	Fog->SetVolumetricFogAlbedo(FColor(240, 236, 230));
	Fog->SetVolumetricFogExtinctionScale(2.0f);
	Fog->SetVolumetricFogDistance(2600.0f);

	Lens = NewPart<UPostProcessComponent>();
	Lens->bUnbound = true;
	FPostProcessSettings& P = Lens->Settings;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	P.bOverride_AutoExposureMinBrightness = true;
	P.AutoExposureMinBrightness = 4.6f;
	P.bOverride_AutoExposureMaxBrightness = true;
	P.AutoExposureMaxBrightness = 9.6f;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = ExposureCompensation;
	P.bOverride_AutoExposureSpeedUp = true;
	P.AutoExposureSpeedUp = 1.5f;
	P.bOverride_AutoExposureSpeedDown = true;
	P.AutoExposureSpeedDown = 1.0f;
	P.bOverride_BloomIntensity = true;
	P.BloomIntensity = 0.7f;
	P.bOverride_BloomThreshold = true;
	P.BloomThreshold = 1.0f;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = 0.35f;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = 0.12f;
	P.bOverride_SceneFringeIntensity = true;
	P.SceneFringeIntensity = 0.2f;
	// A rich, warm casino grade: gold highlights, deep reds, a little teal in the shadows.
	P.bOverride_WhiteTemp = true;
	P.WhiteTemp = 6200.0f;
	P.bOverride_ColorSaturation = true;
	P.ColorSaturation = FVector4(1.05, 1.02, 1.0, 1.0);
	P.bOverride_ColorGainShadows = true;
	P.ColorGainShadows = FVector4(0.96, 1.0, 1.04, 1.0);
	P.bOverride_ColorGainHighlights = true;
	P.ColorGainHighlights = FVector4(1.04, 1.01, 0.96, 1.0);
	P.bOverride_ColorContrast = true;
	P.ColorContrast = FVector4(1.08, 1.08, 1.08, 1.0);
}
