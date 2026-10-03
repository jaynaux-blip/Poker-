// ABackRoomStage as the Riverside Casino's card room (bCardRoom): the room the tournaments play in.
//
// The old showroom of a riverboat casino moored for good in 1994 (docs/LIVE_TOURNAMENTS.md section 4), built from
// the card room kit (art/blender/assets/cardroom.py, instanced): walnut and damask walls with brass sconces, a
// coffered ceiling, mirrored columns, the proscenium with the stream table on its stage, the tournament desk and
// the cashier's cage on the near wall with the champions' board between them, the bar along the right wall and the
// glass doors onto the river deck, the entrance from the casino floor on the left.
//
// Twenty numbered 6-max tables stand in five columns and four rows, each under its green-shaded billiard pendant
// with its number hung from the ceiling. The player's table is always the one at the origin (BuildTable's): the
// room hangs off RoomRoot, placed so that the slot the tournament seated the player at is there, so the table
// numbers are real places and a move walks toward the new one. The far tables' people are the crowd kit
// (art/blender/assets/crowd.py), instanced; the host seats MetaHumans at the near ones.
//
// Room space (cm): the floor at z = 0, x along the room (the stage at +X, the desk and the cage at -X), y across
// it (the entrance at -Y, the bar and the river at +Y). Every table's frame has its player side at -X.

#include "BackRoomStage.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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
const double RX0 = -1800.0, RX1 = 1800.0;
const double RY0 = -1680.0, RY1 = 1680.0;
const double RCeil = 420.0;
const double Module = 400.0; // a bay of wall, a coffer of ceiling
// The entrance from the casino floor (a bay of the -Y wall left open) and the river doors (a bay of the +Y wall).
const double DoorX0 = -1400.0, DoorX1 = -1000.0, DoorH = 300.0;
const double RiverX = 400.0;
// The stage: its front edge, its height; the stream table on it.
const double StageX = 1240.0, StageZ = 45.0;
const FVector StreamAt(1530.0, 0.0, StageZ);
// The tables: five columns, four rows; the aisles run between them.
const double Cols[5] = {-1300.0, -775.0, -250.0, 275.0, 800.0};
const double Rows[4] = {-1150.0, -450.0, 250.0, 950.0};
constexpr int32 TableSlots = 20;
constexpr int32 SlotCount = TableSlots + 1; // and the stream table
const double AisleX = 262.0; // from a table's middle to the aisle on its player side
const double AisleY = 1550.0; // the walkway along the -Y wall, inside the entrance
// The near wall: the desk, the champions' board, the cage.
const double DeskY = -1000.0, CageY = 1000.0, CounterX = RX0 + 162.0;
const double BarX = -900.0;
// The room's light (candelas): the felt under each pendant, the coffers' downlights and coves, the sconces' washes.
const float TableCandela = 460.0f;
const float DownlightCandela = 240.0f;
const float CoveCandela = 18.0f;
const float SconceCandela = 85.0f;
const float WasherCandela = 95.0f;
// Columns between the tables, in the gaps between rows.
const FVector2D Columns[6] = {{-775.0, -800.0}, {-775.0, -100.0}, {-775.0, 600.0}, {275.0, -800.0}, {275.0, -100.0}, {275.0, 600.0}};
// The crowd kit, by kind.
const TCHAR* CrowdNames[ABackRoomStage::CrowdKinds] = {TEXT("SM_Crowd_Seated_A"), TEXT("SM_Crowd_Seated_B"), TEXT("SM_Crowd_Seated_C"),
	TEXT("SM_Crowd_Seated_D"), TEXT("SM_Crowd_Seated_E"), TEXT("SM_Crowd_Seated_F"), TEXT("SM_Crowd_Dealer"), TEXT("SM_Crowd_Standing_A"),
	TEXT("SM_Crowd_Standing_B")};

FVector SlotPoint(int32 Slot)
{
	if (Slot >= TableSlots)
	{
		return StreamAt;
	}
	const int32 S = FMath::Clamp(Slot, 0, TableSlots - 1);
	return FVector(Cols[S % 5], Rows[S / 5], 0.0);
}

/** A kit piece's rotation for its face to look along Yaw (the kit's pieces face -X). */
FRotator FaceYaw(float Yaw)
{
	return FRotator(0.0f, Yaw + 180.0f, 0.0f);
}
} // namespace CardRoomDetail

using namespace CardRoomDetail;

// ------------------------------------------------------------------ layout

int32 ABackRoomStage::NumTableSlots()
{
	return SlotCount;
}

int32 ABackRoomStage::StreamSlot()
{
	return TableSlots;
}

FTransform ABackRoomStage::TableSlotLocal(int32 Slot)
{
	return FTransform(FRotator::ZeroRotator, SlotPoint(Slot));
}

FTransform ABackRoomStage::TableSlot(int32 Slot) const
{
	return TableSlotLocal(Slot) * (RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform());
}

void ABackRoomStage::SetRoomAnchor(int32 Slot)
{
	AnchorSlot = FMath::Clamp(Slot, 0, SlotCount - 1);
	if (RoomRoot)
	{
		RoomRoot->SetRelativeLocation(-SlotPoint(AnchorSlot));
	}
	for (int32 S = 0; S < SlotRoots.Num(); ++S)
	{
		if (SlotRoots[S])
		{
			SlotRoots[S]->SetVisibility(S != AnchorSlot, true);
		}
	}
	// At the stream table the truss lights the player's table; anywhere else its own pendant does.
	const bool bStream = AnchorSlot == StreamSlot();
	if (HeroPendant)
	{
		HeroPendant->SetVisibility(!bStream, true);
	}
	if (bStream && SlotRoots.IsValidIndex(AnchorSlot) && SlotRoots[AnchorSlot])
	{
		// The truss and the rope stay (they're the stage's), only the stand-in table hides.
		SlotRoots[AnchorSlot]->SetVisibility(true, false);
		TArray<USceneComponent*> Parts;
		SlotRoots[AnchorSlot]->GetChildrenComponents(false, Parts);
		for (USceneComponent* C : Parts)
		{
			C->SetVisibility(!C->ComponentHasTag(TEXT("Stand-in")), true);
		}
	}
}

void ABackRoomStage::SetTableOpen(int32 Slot, bool bOpen)
{
	if (TableLights.IsValidIndex(Slot) && TableLights[Slot])
	{
		TableLights[Slot]->SetIntensity(bOpen ? TableCandela : 0.0f);
	}
	if (TableCovers.IsValidIndex(Slot) && TableCovers[Slot])
	{
		TableCovers[Slot]->SetVisibility(!bOpen && Slot != AnchorSlot);
	}
	if (TableGlows.IsValidIndex(Slot) && TableGlows[Slot])
	{
		TableGlows[Slot]->SetVisibility(bOpen && Slot != AnchorSlot);
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

namespace CardRoomDetail
{
void Fill(const TArray<TObjectPtr<UTextRenderComponent>>& Out, const TArray<FString>& Lines)
{
	for (int32 I = 0; I < Out.Num(); ++I)
	{
		if (Out[I])
		{
			Out[I]->SetText(FText::FromString(Lines.IsValidIndex(I) ? Lines[I] : FString()));
		}
	}
}
} // namespace CardRoomDetail

void ABackRoomStage::SetSchedule(const TArray<FString>& Lines)
{
	Fill(ScheduleLines, Lines);
}

void ABackRoomStage::SetChampions(const TArray<FString>& Lines)
{
	Fill(ChampionLines, Lines);
}

void ABackRoomStage::SetCashList(const TArray<FString>& Lines)
{
	Fill(CashLines, Lines);
}

void ABackRoomStage::SetOnAir(bool bOn)
{
	bOnAir = bOn;
}

void ABackRoomStage::ClearCrowd()
{
	for (UInstancedStaticMeshComponent* C : CrowdMeshes)
	{
		if (C)
		{
			C->ClearInstances();
		}
	}
}

void ABackRoomStage::AddCrowd(int32 Kind, const FTransform& RoomLocal)
{
	if (CrowdMeshes.IsValidIndex(Kind) && CrowdMeshes[Kind])
	{
		CrowdMeshes[Kind]->AddInstance(RoomLocal);
	}
}

// ------------------------------------------------------------------ walks

namespace CardRoomDetail
{
/** Up out of the chair and back into the aisle on the player's side (world, the player's table at the origin). */
void StandUp(TArray<FVector>& Out, const FVector& Eye)
{
	Out.Add(Eye);
	Out.Add(Eye + FVector(-34.0, -16.0, 44.0));
	Out.Add(FVector(-205.0, -70.0, 165.0));
	Out.Add(FVector(-AisleX, -60.0, 166.0));
}

/** From the aisle back into the chair. */
void SitDown(TArray<FVector>& Out, const FVector& Eye)
{
	Out.Add(FVector(-AisleX, -60.0, 166.0));
	Out.Add(FVector(-205.0, -110.0, 162.0));
	Out.Add(FVector(-160.0, 6.0, 148.0));
	Out.Add(Eye);
}
} // namespace CardRoomDetail

TArray<FVector> ABackRoomStage::CardRoomWalkIn(const FVector& Eye) const
{
	// In from the casino floor along the walkway by the wall, up the aisle beside the player's table, round
	// behind the chair. Room points move with the room; the last ones are the player's table's (the origin's).
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	const FVector Me = SlotPoint(AnchorSlot);
	const double Door = (DoorX0 + DoorX1) * 0.5;
	TArray<FVector> Out;
	for (const FVector& P : {FVector(Door, RY0 - 150.0, 166.0), FVector(Door, -AisleY, 166.0), FVector(Me.X - AisleX, -AisleY, 166.0)})
	{
		Out.Add(Room.TransformPosition(P));
	}
	Out.Add(FVector(-AisleX, FMath::Max(-AisleY - Me.Y + 1.0, -420.0), 166.0));
	TArray<FVector> Sit;
	SitDown(Sit, Eye);
	Out.Append(Sit);
	return Out;
}

TArray<FVector> ABackRoomStage::CardRoomWalkOut(const FVector& Eye) const
{
	TArray<FVector> In = CardRoomWalkIn(Eye);
	TArray<FVector> Out;
	StandUp(Out, Eye);
	for (int32 I = 3; I >= 0; --I)
	{
		Out.Add(In[I]);
	}
	return Out;
}

TArray<FVector> ABackRoomStage::CardRoomMoveOut(const FVector& Eye, int32 ToSlot) const
{
	// Up and along the aisle toward the new table, three meters or so before the room turns into the other one.
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	TArray<FVector> Out;
	StandUp(Out, Eye);
	const FVector To = Room.TransformPosition(SlotPoint(ToSlot));
	const double Dy = FMath::Clamp(To.Y, -300.0, 300.0);
	Out.Add(FVector(-AisleX, -60.0 + Dy, 166.0));
	return Out;
}

TArray<FVector> ABackRoomStage::CardRoomMoveIn(const FVector& Eye, int32 FromSlot) const
{
	// Coming up the aisle from where the old table is, into the chair.
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	const FVector From = Room.TransformPosition(SlotPoint(FromSlot));
	const double Dy = FMath::Clamp(From.Y, -300.0, 300.0);
	TArray<FVector> Out;
	Out.Add(FVector(-AisleX, -60.0 + Dy, 166.0));
	SitDown(Out, Eye);
	return Out;
}

TArray<FVector> ABackRoomStage::CardRoomToCage(const FVector& Eye) const
{
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	const FVector Me = SlotPoint(AnchorSlot);
	TArray<FVector> Out;
	StandUp(Out, Eye);
	// Down the aisle to the walkway behind the last column of tables, along it to the cage's middle window.
	// The lane between the row's chairs and the columns (a row's gap is 350 out), clear of every table.
	Out.Add(Room.TransformPosition(FVector(Me.X - AisleX, Me.Y - 255.0, 166.0)));
	Out.Add(Room.TransformPosition(FVector(CounterX + 158.0, Me.Y - 255.0, 166.0)));
	Out.Add(Room.TransformPosition(FVector(CounterX + 158.0, CageY, 166.0)));
	Out.Add(Room.TransformPosition(FVector(CounterX + 70.0, CageY, 164.0)));
	return Out;
}

TArray<FVector> ABackRoomStage::CardRoomCageToDoor(const FVector& At) const
{
	const FTransform Room = RoomRoot ? RoomRoot->GetComponentTransform() : GetActorTransform();
	const double Door = (DoorX0 + DoorX1) * 0.5;
	TArray<FVector> Out;
	Out.Add(At);
	for (const FVector& P : {FVector(CounterX + 158.0, CageY, 166.0), FVector(CounterX + 158.0, -AisleY, 166.0), FVector(Door, -AisleY, 166.0), FVector(Door, RY0 - 150.0, 166.0)})
	{
		Out.Add(Room.TransformPosition(P));
	}
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

UStaticMesh* ABackRoomStage::Kit(const TCHAR* Name) const
{
	return Prop(Name);
}

UInstancedStaticMeshComponent* ABackRoomStage::Instances(UStaticMesh* Mesh, UMaterialInterface* Override, USceneComponent* Parent, bool bShadows)
{
	UInstancedStaticMeshComponent* I = NewPart<UInstancedStaticMeshComponent>(Parent);
	I->SetStaticMesh(Mesh);
	if (Override && Mesh)
	{
		for (int32 K = 0; K < Mesh->GetStaticMaterials().Num(); ++K)
		{
			I->SetMaterial(K, Override);
		}
	}
	I->SetCastShadow(bShadows);
	I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return I;
}

void ABackRoomStage::BuildCardRoom()
{
	TextMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_ScreenText.M_ScreenText"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	RoomRoot = NewPart<USceneComponent>();
	BoardLines.Reset();
	TableLights.Reset();
	TableCovers.Reset();
	TableGlows.Reset();
	SlotRoots.Reset();
	CrowdMeshes.Reset();
	ScheduleLines.Reset();
	ChampionLines.Reset();
	CashLines.Reset();
	USceneComponent* R = RoomRoot;
	auto RBox = [&](UMaterialInterface* M, const FVector& Min, const FVector& Max, bool bShadows) {
		return AddMesh(CubeMesh, M, (Min + Max) * 0.5, (Max - Min) / 100.0, FRotator::ZeroRotator, R, bShadows);
	};
	// Where the kit isn't imported yet, the room still stands (plain boxes).
	UMaterialInterface* Carpet = Room(TEXT("Carpet"), 5, 0x3d0f19, 0xb38a3e);
	UMaterialInterface* Panels = Room(TEXT("Panels"), 6, 0x4a2a18, 0x8c6a3a, 1.05f);
	UMaterialInterface* Brass = Room(TEXT("Brass"), 4, 0xb8913f, 0, 0.0f, 0.95f);

	// ---- the floor: the casino's teal-and-rust carpet, worn in the aisles (M_CardRoom's pattern 5).
	RBox(Carpet, FVector(RX0 - 24.0, RY0 - 600.0, -24.0), FVector(RX1 + 24.0, RY1 + 24.0, 0.0), true);

	// ---- the walls: bays of walnut and damask, sconces between them; the entrance and the river doors are bays.
	UStaticMesh* WallMesh = Kit(TEXT("SM_CR_Wall"));
	UStaticMesh* SconceMesh = Kit(TEXT("SM_CR_Sconce"));
	UStaticMesh* SconceGlowMesh = Kit(TEXT("SM_CR_Sconce_Glow"));
	UInstancedStaticMeshComponent* Walls = WallMesh ? Instances(WallMesh, nullptr, R, true) : nullptr;
	UInstancedStaticMeshComponent* Sconces = SconceMesh ? Instances(SconceMesh, nullptr, R, false) : nullptr;
	UInstancedStaticMeshComponent* SconceGlows = SconceGlowMesh ? Instances(SconceGlowMesh, Glow(TEXT("CR_Sconce"), Srgb(0xffd9a0), 26.0f), R, false) : nullptr;
	auto Bay = [&](const FVector& At, float FacingYaw, double ScaleY) {
		if (Walls)
		{
			Walls->AddInstance(FTransform(FaceYaw(FacingYaw), At, FVector(1.0, ScaleY, 1.0)));
		}
		else
		{
			const FVector In = FRotator(0.0f, FacingYaw, 0.0f).Vector();
			const FVector Along = FRotator(0.0f, FacingYaw + 90.0f, 0.0f).Vector() * Module * 0.5 * ScaleY;
			RBox(Panels, FVector::Min(At - Along - In * 12.0, At + Along) + FVector(0, 0, 0), FVector::Max(At - Along - In * 12.0, At + Along) + FVector(0, 0, RCeil), true);
		}
	};
	auto SconceAt = [&](const FVector& At, float FacingYaw) {
		if (Sconces)
		{
			Sconces->AddInstance(FTransform(FaceYaw(FacingYaw), At));
		}
		if (SconceGlows)
		{
			SconceGlows->AddInstance(FTransform(FaceYaw(FacingYaw), At));
		}
	};
	for (double X = RX0 + Module * 0.5; X < RX1; X += Module)
	{
		// The -Y wall (facing +Y), the entrance bay left open; the +Y wall (facing -Y), the river doors' bay.
		if (!(X > DoorX0 && X < DoorX1))
		{
			Bay(FVector(X, RY0, 0.0), 90.0f, 1.0);
		}
		if (FMath::Abs(X - RiverX) > 1.0)
		{
			Bay(FVector(X, RY1, 0.0), -90.0f, 1.0);
		}
		if (X + Module * 0.5 < RX1 - 1.0)
		{
			SconceAt(FVector(X + Module * 0.5, RY0, 0.0), 90.0f);
			SconceAt(FVector(X + Module * 0.5, RY1, 0.0), -90.0f);
		}
	}
	for (double Y = RY0 + Module * 0.5; Y < RY1 + Module * 0.5; Y += Module)
	{
		// The near wall (facing +X) runs on past the corner (hidden behind the side wall).
		Bay(FVector(RX0, Y, 0.0), 0.0f, 1.0);
		const double Sy = Y + Module * 0.5;
		if (Sy < RY1 - 1.0 && FMath::Abs(Sy) > 300.0 && FMath::Abs(Sy - DeskY) > 340.0 && FMath::Abs(Sy - CageY) > 340.0)
		{
			SconceAt(FVector(RX0, Sy, 0.0), 0.0f);
		}
	}
	// The far wall either side of the proscenium (which carries its own wall round the opening).
	for (const double S : {-1.0, 1.0})
	{
		Bay(FVector(RX1, S * 1480.0, 0.0), 180.0f, 1.0);
		Bay(FVector(RX1, S * 1080.0, 0.0), 180.0f, 1.0);
		Bay(FVector(RX1, S * 780.0, 0.0), 180.0f, 0.5);
		SconceAt(FVector(RX1, S * 1280.0, 0.0), 180.0f);
	}

	// ---- the ceiling: walnut coffers, a downlight in each.
	if (UStaticMesh* CofferMesh = Kit(TEXT("SM_CR_Ceiling")))
	{
		UInstancedStaticMeshComponent* Coffers = Instances(CofferMesh, nullptr, R, false);
		UStaticMesh* CofferGlowMesh = Kit(TEXT("SM_CR_Ceiling_Glow"));
		UInstancedStaticMeshComponent* Downlights = CofferGlowMesh ? Instances(CofferGlowMesh, Glow(TEXT("CR_Downlight"), Srgb(0xfff0d8), 34.0f), R, false) : nullptr;
		for (double X = RX0 + Module * 0.5; X < RX1; X += Module)
		{
			for (double Y = RY0 + Module * 0.5 - 40.0; Y < RY1 + Module * 0.5; Y += Module)
			{
				Coffers->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, RCeil)));
				if (Downlights)
				{
					Downlights->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, RCeil)));
				}
			}
		}
	}
	else
	{
		RBox(Room(TEXT("BlackCeiling"), 7, 0x121214), FVector(RX0 - 24.0, RY0 - 24.0, RCeil), FVector(RX1 + 24.0, RY1 + 24.0, RCeil + 24.0), true);
	}

	// ---- the columns, mirrored above the rail.
	if (UStaticMesh* ColumnMesh = Kit(TEXT("SM_CR_Column")))
	{
		UInstancedStaticMeshComponent* Cols6 = Instances(ColumnMesh, nullptr, R, true);
		for (const FVector2D& C : Columns)
		{
			Cols6->AddInstance(FTransform(FRotator::ZeroRotator, FVector(C.X, C.Y, 0.0)));
		}
	}

	// ---- the stage: the proscenium, the curtains, the footlights; the stream's sign and the big clock on the backdrop.
	if (UStaticMesh* StageMesh = Kit(TEXT("SM_CR_Stage")))
	{
		AddMesh(StageMesh, nullptr, FVector(StageX, 0.0, 0.0), FVector(1.0), FRotator::ZeroRotator, R);
		if (UStaticMesh* Foot = Kit(TEXT("SM_CR_Stage_Glow")))
		{
			AddMesh(Foot, Glow(TEXT("CR_Footlights"), Srgb(0xffcf8a), 30.0f), FVector(StageX, 0.0, 0.0), FVector(1.0), FRotator::ZeroRotator, R, false);
		}
	}
	else
	{
		RBox(Panels, FVector(StageX, -500.0, 0.0), FVector(RX1, 500.0, StageZ), true);
	}
	AddText(TEXT("RIVERSIDE  LIVE"), FVector(RX1 - 12.0, 0.0, 380.0), FRotator(0.0f, 180.0f, 0.0f), 30.0f, Srgb(0xffd27a), R);

	// ---- the casino floor through the entrance: bright, busy, out of focus from the tables.
	{
		UMaterialInterface* FloorBeyond = Room(TEXT("CasinoFloorCarpet"), 5, 0x2a0d1a, 0xc79a42);
		RBox(FloorBeyond, FVector(DoorX0 - 700.0, RY0 - 900.0, -24.0), FVector(DoorX1 + 700.0, RY0 - 24.0, 0.0), true);
		RBox(Room(TEXT("CasinoCeiling"), 7, 0x2a2420), FVector(DoorX0 - 700.0, RY0 - 900.0, 480.0), FVector(DoorX1 + 700.0, RY0 - 24.0, 500.0), true);
		const FLinearColor SlotColors[] = {Srgb(0xff3d7f), Srgb(0x3dd6ff), Srgb(0xffc23d), Srgb(0x7dff6a), Srgb(0xb36bff), Srgb(0xff8a3d)};
		UMaterialInterface* Cabinet = Room(TEXT("SlotCabinet"), 4, 0x1a1a1f, 0, 0.0f, 0.4f);
		int32 K = 0;
		for (int32 Row = 0; Row < 2; ++Row)
		{
			for (int32 I = 0; I < 9; ++I, ++K)
			{
				const double X = DoorX0 - 520.0 + I * 115.0;
				const double Y = RY0 - 380.0 - Row * 300.0;
				RBox(Cabinet, FVector(X - 38.0, Y - 30.0, 0.0), FVector(X + 38.0, Y + 30.0, 168.0), false);
				AddMesh(CubeMesh, Glow(*FString::Printf(TEXT("SlotScreen%d"), K % 6), SlotColors[K % 6] * 0.55f + FLinearColor(0.05f, 0.05f, 0.08f), 9.0f), FVector(X, Y + 31.2, 122.0),
					FVector(0.54, 0.01, 0.46), FRotator::ZeroRotator, R, false);
				AddMesh(CubeMesh, Glow(*FString::Printf(TEXT("SlotTop%d"), (K + 2) % 6), SlotColors[(K + 2) % 6], 22.0f), FVector(X, Y + 8.0, 196.0), FVector(0.7, 0.14, 0.26),
					FRotator::ZeroRotator, R, false);
			}
		}
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
		// The opening's brass frame and the sign over it, both sides.
		RBox(Brass, FVector(DoorX0 - 12.0, RY0 - 14.0, 0.0), FVector(DoorX0, RY0 + 6.0, DoorH), true);
		RBox(Brass, FVector(DoorX1, RY0 - 14.0, 0.0), FVector(DoorX1 + 12.0, RY0 + 6.0, DoorH), true);
		RBox(Panels, FVector(DoorX0 - 12.0, RY0 - 14.0, DoorH), FVector(DoorX1 + 12.0, RY0 + 6.0, RCeil), true);
		AddText(TEXT("POKER ROOM"), FVector((DoorX0 + DoorX1) * 0.5, RY0 + 8.0, DoorH + 50.0), FRotator(0.0f, 90.0f, 0.0f), 34.0f, Srgb(0xffd27a), R);
		AddText(TEXT("POKER ROOM"), FVector((DoorX0 + DoorX1) * 0.5, RY0 - 16.0, DoorH + 50.0), FRotator(0.0f, -90.0f, 0.0f), 34.0f, Srgb(0xffd27a), R);
	}

	// ---- the near wall: the tournament desk, the champions' board, the cashier's cage.
	auto Counter = [&](const TCHAR* Mesh, const TCHAR* GlowMesh, double Y, const FString& Sign) {
		if (UStaticMesh* M = Kit(Mesh))
		{
			AddMesh(M, nullptr, FVector(CounterX, Y, 0.0), FVector(1.0), FaceYaw(0.0f), R);
		}
		if (UStaticMesh* G = Kit(GlowMesh))
		{
			AddMesh(G, Glow(TEXT("CR_SignBox"), Srgb(0xfff4e0), 6.0f), FVector(CounterX, Y, 0.0), FVector(1.0), FaceYaw(0.0f), R, false);
		}
		AddText(Sign, FVector(CounterX - 120.0, Y, 265.0), FRotator::ZeroRotator, 30.0f, Srgb(0x3a1d0a), R);
	};
	Counter(TEXT("SM_CR_Desk"), TEXT("SM_CR_Desk_Glow"), DeskY, TEXT("TOURNAMENTS"));
	Counter(TEXT("SM_CR_Cage"), TEXT("SM_CR_Cage_Glow"), CageY, TEXT("CASHIER"));
	{
		// The cash list on the desk's whiteboard (handwritten, the game fills it).
		const double Bx = CounterX - 145.0;
		const double By = DeskY + 275.0;
		AddText(TEXT("CASH GAMES"), FVector(Bx, By, 214.0), FRotator::ZeroRotator, 9.0f, Srgb(0x1a2a6a), R);
		for (int32 I = 0; I < 6; ++I)
		{
			CashLines.Add(AddText(TEXT(""), FVector(Bx, By, 198.0 - I * 11.0), FRotator::ZeroRotator, 7.0f, Srgb(I == 0 ? 0x8a1a1a : 0x1a1a22), R));
		}
	}
	{
		// The champions' board: a walnut frame round a black felt board with gold lettering.
		const double Wx = RX0 + 3.0;
		RBox(Room(TEXT("BoardFelt"), 4, 0x0c0d10), FVector(Wx, -230.0, 130.0), FVector(Wx + 3.0, 230.0, 340.0), false);
		for (const FVector& A : {FVector(Wx, -240.0, 120.0), FVector(Wx, -240.0, 340.0)})
		{
			RBox(Brass, A, A + FVector(5.0, 480.0, 10.0), false);
		}
		for (const FVector& A : {FVector(Wx, -240.0, 120.0), FVector(Wx, 230.0, 120.0)})
		{
			RBox(Brass, A, A + FVector(5.0, 10.0, 230.0), false);
		}
		AddText(TEXT("RIVERSIDE  CHAMPIONS"), FVector(Wx + 4.0, 0.0, 318.0), FRotator::ZeroRotator, 18.0f, Srgb(0xffd27a), R);
		for (int32 I = 0; I < 8; ++I)
		{
			ChampionLines.Add(AddText(TEXT(""), FVector(Wx + 4.0, 0.0, 290.0 - I * 20.0), FRotator::ZeroRotator, 12.0f, Srgb(I == 0 ? 0xfff0c8 : 0xe0c890), R));
		}
	}

	// ---- the bar along the +Y wall.
	if (UStaticMesh* BarMesh = Kit(TEXT("SM_CR_Bar")))
	{
		AddMesh(BarMesh, nullptr, FVector(BarX, RY1 - 188.0, 0.0), FVector(1.0), FaceYaw(-90.0f), R);
		if (UStaticMesh* G = Kit(TEXT("SM_CR_Bar_Glow")))
		{
			AddMesh(G, Glow(TEXT("CR_BarShelves"), Srgb(0xffb35c), 14.0f), FVector(BarX, RY1 - 188.0, 0.0), FVector(1.0), FaceYaw(-90.0f), R, false);
		}
	}
	UPointLightComponent* BarLight = NewPart<UPointLightComponent>(R);
	BarLight->SetRelativeLocation(FVector(BarX, RY1 - 120.0, 260.0));
	BarLight->SetIntensityUnits(ELightUnits::Candelas);
	BarLight->SetIntensity(320.0f);
	BarLight->SetLightColor(Srgb(0xffb070));
	BarLight->SetAttenuationRadius(800.0f);
	BarLight->SetCastShadows(false);

	// ---- the river doors and the deck beyond, the river's dark under barge lights.
	if (UStaticMesh* Doors = Kit(TEXT("SM_CR_RiverDoors")))
	{
		AddMesh(Doors, nullptr, FVector(RiverX, RY1, 0.0), FVector(1.0), FaceYaw(-90.0f), R);
		if (UStaticMesh* G = Kit(TEXT("SM_CR_RiverDoors_Glow")))
		{
			AddMesh(G, Glow(TEXT("CR_Deck"), Srgb(0xffe2b0), 18.0f), FVector(RiverX, RY1, 0.0), FVector(1.0), FaceYaw(-90.0f), R, false);
		}
		RBox(Room(TEXT("River"), 4, 0x05080b, 0, 0.0f, 0.6f), FVector(RiverX - 3000.0, RY1 + 520.0, -260.0), FVector(RiverX + 3000.0, RY1 + 6000.0, -250.0), false);
		for (int32 I = 0; I < 7; ++I)
		{
			AddMesh(SphereMesh, Glow(TEXT("BargeLight"), Srgb(I % 3 == 0 ? 0xff5a3a : 0xffd08a), 60.0f),
				FVector(RiverX - 2400.0 + I * 760.0, RY1 + 2600.0 + (I % 2) * 900.0, -180.0 + (I % 3) * 20.0), FVector(0.25), FRotator::ZeroRotator, R, false);
		}
	}

	// ---- the boards: today's events by the entrance, the tournament clocks.
	{
		// The schedule screen on the -Y wall, near the entrance.
		const FVector At(-400.0, RY0 + 6.0, 250.0); // a bay's middle, between two sconces
		AddMesh(CubeMesh, Room(TEXT("ScreenBezel"), 4, 0x0a0a0b, 0, 0.0f, 0.2f), At + FVector(0.0, -2.0, 0.0), FVector(3.5, 0.06, 2.0), FRotator::ZeroRotator, R, false);
		AddMesh(CubeMesh, Glow(TEXT("ScreenGlass"), Srgb(0x081428), 3.0f), At + FVector(0.0, 1.5, 0.0), FVector(3.34, 0.02, 1.86), FRotator::ZeroRotator, R, false);
		AddText(TEXT("TODAY AT THE RIVERSIDE"), At + FVector(0.0, 3.0, 76.0), FRotator(0.0f, 90.0f, 0.0f), 14.0f, Srgb(0xffd27a), R);
		for (int32 I = 0; I < 5; ++I)
		{
			ScheduleLines.Add(AddText(TEXT(""), At + FVector(0.0, 3.0, 44.0 - I * 26.0), FRotator(0.0f, 90.0f, 0.0f), 13.0f, Srgb(I == 0 ? 0xffffff : 0xcfd6e0), R));
		}
	}
	auto Screen = [&](const FVector& At, const FRotator& Facing, double Scale) {
		const FVector Fwd = Facing.Vector();
		AddMesh(CubeMesh, Room(TEXT("ScreenBezel"), 4, 0x0a0a0b, 0, 0.0f, 0.2f), At - Fwd * 4.0, FVector(0.06, 3.9 * Scale, 2.3 * Scale), Facing, R, false);
		AddMesh(CubeMesh, Glow(TEXT("ScreenGlass"), Srgb(0x081428), 3.0f), At - Fwd * 0.5, FVector(0.02, 3.7 * Scale, 2.1 * Scale), Facing, R, false);
		const double Lines[6][2] = {{86.0, 15.0}, {54.0, 20.0}, {8.0, 52.0}, {-40.0, 22.0}, {-66.0, 13.0}, {-90.0, 13.0}};
		const FLinearColor Colors[6] = {Srgb(0xffd27a), Srgb(0x9fd8ff), Srgb(0xffffff), Srgb(0xffffff), Srgb(0x9fd8ff), Srgb(0xcfd6e0)};
		for (int32 I = 0; I < 6; ++I)
		{
			BoardLines.Add(AddText(TEXT(""), At + Fwd * 1.0 + FVector(0.0, 0.0, Lines[I][0] * Scale), Facing, static_cast<float>(Lines[I][1] * Scale), Colors[I], R));
		}
	};
	Screen(FVector(RX1 - 8.0, 0.0, 255.0), FRotator(0.0f, 180.0f, 0.0f), 1.0);
	Screen(FVector(450.0, RY0 + 8.0, 270.0), FRotator(0.0f, 90.0f, 0.0f), 0.75);
	Screen(FVector(1000.0, RY1 - 8.0, 270.0), FRotator(0.0f, -90.0f, 0.0f), 0.75);
	SetBoard(TEXT("RIVERSIDE"), TEXT("LEVEL 1"), TEXT("20:00"), TEXT("100 / 200"), TEXT("NEXT  200 / 300"), TEXT("SHUFFLE UP AND DEAL"));

	// ---- the crowd at the far tables (the host fills them).
	for (int32 K = 0; K < CrowdKinds; ++K)
	{
		UStaticMesh* M = Kit(CrowdNames[K]);
		CrowdMeshes.Add(M ? Instances(M, nullptr, R, true) : nullptr);
	}

	BuildCardRoomTables();
	BuildCardRoomLights();
	BuildCardRoomAir();
	SetRoomAnchor(AnchorSlot);
}

void ABackRoomStage::BuildCardRoomTables()
{
	// The player's table: the same table as the Back Room's, at the origin, under its own pendant.
	BuildTable();
	UStaticMesh* Chair = Prop(TEXT("SM_FoldingChair"));
	UStaticMesh* Chips = Prop(TEXT("SM_ChipStacks"));
	UStaticMesh* PendantMesh = Kit(TEXT("SM_CR_Pendant"));
	UStaticMesh* PendantGlowMesh = Kit(TEXT("SM_CR_Pendant_Glow"));
	UMaterialInterface* PendantGlow = Glow(TEXT("CR_PendantGlow"), Srgb(0xfff1d6), 40.0f);
	USceneComponent* R = RoomRoot;
	auto Pendant = [&](USceneComponent* Parent, double Z) -> UStaticMeshComponent* {
		UStaticMeshComponent* Lit = nullptr;
		if (PendantMesh)
		{
			AddMesh(PendantMesh, nullptr, FVector(0.0, 0.0, Z), FVector(1.0), FRotator::ZeroRotator, Parent, false);
			if (PendantGlowMesh)
			{
				Lit = AddMesh(PendantGlowMesh, PendantGlow, FVector(0.0, 0.0, Z), FVector(1.0), FRotator::ZeroRotator, Parent, false);
			}
		}
		else
		{
			AddMesh(CubeMesh, Room(TEXT("Pendant"), 4, 0x16161a, 0, 0.0f, 0.5f), FVector(0.0, 0.0, Z + 20.0), FVector(0.7, 1.8, 0.12), FRotator::ZeroRotator, Parent, false);
			Lit = AddMesh(CubeMesh, PendantGlow, FVector(0.0, 0.0, Z + 13.0), FVector(0.62, 1.7, 0.01), FRotator::ZeroRotator, Parent, false);
		}
		return Lit;
	};
	auto TableLight = [&](USceneComponent* Parent, double Z) {
		URectLightComponent* L = NewPart<URectLightComponent>(Parent);
		L->SetRelativeLocationAndRotation(FVector(0.0, 0.0, Z), FRotator(-90.0f, 0.0f, 0.0f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(TableCandela);
		L->SetLightColor(Srgb(0xffe2bd));
		L->SetSourceWidth(150.0f);
		L->SetSourceHeight(36.0f);
		L->SetBarnDoorAngle(70.0f);
		L->SetAttenuationRadius(520.0f);
		L->SetCastShadows(true);
		L->SetVolumetricScatteringIntensity(0.4f);
		return L;
	};
	HeroPendant = NewPart<USceneComponent>();
	Pendant(HeroPendant, 215.0);
	HeroLight = TableLight(HeroPendant, 212.0);
	HeroLight->SetIntensity(TableCandela * 1.15f);

	// The tables: each with its chairs, cards and chips, a cover for when it's closed, its pendant and its number.
	UMaterialInterface* Cover = Room(TEXT("TableCover"), 4, 0x15161b, 0, 0.0f, 0.0f);
	UMaterialInterface* CardBack = Room(TEXT("CardBack"), 4, 0x9a1b22, 0, 0.0f, 0.0f);
	UMaterialInterface* Plaque = Room(TEXT("Plaque"), 4, 0x0c0d10, 0, 0.0f, 0.2f);
	UMaterialInterface* Brass = Room(TEXT("Brass"), 4, 0xb8913f, 0, 0.0f, 0.95f);
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		const bool bStream = Slot == StreamSlot();
		USceneComponent* Table = NewPart<USceneComponent>(R);
		Table->SetRelativeTransform(TableSlotLocal(Slot));
		SlotRoots.Add(Table);
		// The table and its dressing are a stand-in for the real one when it's the player's.
		USceneComponent* StandIn = NewPart<USceneComponent>(Table);
		StandIn->ComponentTags.Add(TEXT("Stand-in"));
		if (TableMesh)
		{
			AddMesh(TableMesh, nullptr, FVector::ZeroVector, FVector(1.0), FRotator(0.0f, 90.0f, 0.0f), StandIn);
		}
		for (int32 S = 0; S < 7; ++S)
		{
			if (S == 4)
			{
				continue;
			}
			const FVector SeatAt = SeatEdge(S);
			const FVector Out = FVector(SeatAt.X, SeatAt.Y, 0.0).GetSafeNormal2D();
			const FVector At(SeatAt.X - Out.X * 22.0, SeatAt.Y - Out.Y * 22.0, FeltZ + 0.15);
			AddMesh(CubeMesh, CardBack, At, FVector(0.064, 0.089, 0.002), FRotator(0.0f, Out.Rotation().Yaw + 90.0f + (S * 13 % 9) - 4.0f, 0.0f), StandIn, false);
			if (Chips)
			{
				AddMesh(Chips, nullptr, FVector(SeatAt.X - Out.X * 14.0 + Out.Y * 14.0, SeatAt.Y - Out.Y * 14.0 - Out.X * 14.0, FeltZ), FVector(0.55 + 0.1 * ((S + Slot) % 3)),
					FRotator(0.0f, S * 40.0f, 0.0f), StandIn, false);
			}
			if (Chair)
			{
				const FTransform SeatT = ABackRoomStage::SeatTransform(S);
				AddMesh(Chair, nullptr, SeatT.TransformPosition(FVector(-19.5, 0.0, 0.0)), FVector(1.0), SeatT.Rotator(), StandIn);
			}
		}
		for (int32 C = 0; C < 3; ++C)
		{
			AddMesh(CubeMesh, Room(TEXT("CardFace"), 4, 0xf2efe6, 0, 0.0f, 0.0f), FVector(-2.0, (C - 1) * 7.5, FeltZ + 0.15), FVector(0.064, 0.089, 0.002), FRotator(0.0f, 90.0f, 0.0f),
				StandIn, false);
		}
		UStaticMeshComponent* CoverMesh = AddMesh(CubeMesh, Cover, FVector(0.0, 0.0, FeltZ + 1.5), FVector(1.4, 2.7, 0.03), FRotator::ZeroRotator, StandIn, false);
		CoverMesh->SetVisibility(false);
		TableCovers.Add(CoverMesh);
		if (bStream)
		{
			// The stream table has the truss's lights (BuildCardRoomLights); its glow and light slots stay empty.
			TableGlows.Add(nullptr);
			TableLights.Add(nullptr);
			continue;
		}
		TableGlows.Add(Pendant(StandIn, 215.0));
		TableLights.Add(TableLight(StandIn, 212.0));
		// The table's number, hung from the ceiling over the pendant, both faces.
		AddMesh(CubeMesh, Plaque, FVector(0.0, 0.0, 345.0), FVector(0.04, 0.46, 0.28), FRotator::ZeroRotator, Table, false);
		AddMesh(CubeMesh, Brass, FVector(0.0, 0.0, 360.0 + 30.0), FVector(0.012, 0.012, 0.6), FRotator::ZeroRotator, Table, false);
		for (const float Yaw : {0.0f, 180.0f})
		{
			AddText(FString::FromInt(Slot + 1), FRotator(0.0f, Yaw, 0.0f).Vector() * 2.6 + FVector(0.0, 0.0, 345.0), FRotator(0.0f, Yaw, 0.0f), 22.0f, Srgb(0xffd27a), Table);
		}
	}

	// The stream table's dressing on the stage: the rope, the camera, the truss.
	USceneComponent* Stream = SlotRoots[StreamSlot()];
	UMaterialInterface* Velvet = Room(TEXT("Velvet"), 4, 0x6b0d16, 0, 0.0f, 0.0f);
	const double RopeX = 230.0, RopeY = 300.0;
	TArray<FVector> Posts = {FVector(-RopeX, -RopeY, 0.0), FVector(RopeX, -RopeY, 0.0), FVector(RopeX, RopeY, 0.0), FVector(-RopeX, RopeY, 0.0)};
	for (const FVector& P : Posts)
	{
		AddMesh(CylinderMesh, Brass, P + FVector(0.0, 0.0, 47.0), FVector(0.05, 0.05, 0.94), FRotator::ZeroRotator, Stream);
		AddMesh(SphereMesh, Brass, P + FVector(0.0, 0.0, 96.0), FVector(0.08), FRotator::ZeroRotator, Stream);
	}
	for (int32 I = 0; I < 4; ++I)
	{
		if (I == 3)
		{
			continue; // the player's side stays open
		}
		const FVector Ta = Posts[I] + FVector(0.0, 0.0, 88.0);
		const FVector Tb = Posts[(I + 1) % 4] + FVector(0.0, 0.0, 88.0);
		const FVector D = Tb - Ta;
		AddMesh(CylinderMesh, Velvet, (Ta + Tb) * 0.5 - FVector(0.0, 0.0, 8.0), FVector(0.03, 0.03, D.Size() / 100.0), FRotationMatrix::MakeFromZ(D).Rotator(), Stream, false);
	}
	UMaterialInterface* Black = Room(TEXT("GearBlack"), 4, 0x111113, 0, 0.0f, 0.3f);
	const FVector Cam(-200.0, 260.0, 0.0);
	for (int32 L = 0; L < 3; ++L)
	{
		const FVector Foot = Cam + FRotator(0.0f, 120.0f * L, 0.0f).Vector() * 38.0;
		const FVector D = (Cam + FVector(0.0, 0.0, 140.0)) - Foot;
		AddMesh(CylinderMesh, Black, (Foot + Cam + FVector(0.0, 0.0, 140.0)) * 0.5, FVector(0.025, 0.025, D.Size() / 100.0), FRotationMatrix::MakeFromZ(D).Rotator(), Stream, false);
	}
	const FRotator CamAim = (FVector(0.0, 0.0, 90.0) - (Cam + FVector(0.0, 0.0, 150.0))).Rotation();
	AddMesh(CubeMesh, Black, Cam + FVector(0.0, 0.0, 150.0), FVector(0.42, 0.18, 0.2), CamAim, Stream, false);
	TallyGlow = Glow(TEXT("Tally"), Srgb(0xff1a1a), 1.0f);
	AddMesh(SphereMesh, TallyGlow, Cam + FVector(0.0, 0.0, 164.0) - CamAim.Vector() * 10.0, FVector(0.03), FRotator::ZeroRotator, Stream, false);
	Tally = NewPart<UPointLightComponent>(Stream);
	Tally->SetRelativeLocation(Cam + FVector(0.0, 0.0, 168.0));
	Tally->SetIntensityUnits(ELightUnits::Candelas);
	Tally->SetIntensity(0.0f);
	Tally->SetLightColor(Srgb(0xff2020));
	Tally->SetAttenuationRadius(140.0f);
	Tally->SetCastShadows(false);
	const double TrussZ = 340.0 - StageZ;
	for (const FVector& S : {FVector(0.0, -190.0, TrussZ), FVector(0.0, 190.0, TrussZ)})
	{
		AddMesh(CubeMesh, Black, S, FVector(3.0, 0.12, 0.12), FRotator::ZeroRotator, Stream, false);
	}
	for (const FVector& S : {FVector(-150.0, 0.0, TrussZ), FVector(150.0, 0.0, TrussZ)})
	{
		AddMesh(CubeMesh, Black, S, FVector(0.12, 3.9, 0.12), FRotator::ZeroRotator, Stream, false);
	}
}

void ABackRoomStage::BuildCardRoomLights()
{
	// The stream table: a broad soft key from the truss and four spots angled in from its corners.
	LampLight = nullptr;
	USceneComponent* Stream = SlotRoots.IsValidIndex(StreamSlot()) ? SlotRoots[StreamSlot()].Get() : RoomRoot.Get();
	const double TrussZ = 330.0 - StageZ;
	URectLightComponent* Key = NewPart<URectLightComponent>(Stream);
	Key->SetRelativeLocationAndRotation(FVector(0.0, 0.0, TrussZ), FRotator(-90.0f, 0.0f, 0.0f));
	Key->SetIntensityUnits(ELightUnits::Candelas);
	Key->SetIntensity(650.0f);
	Key->SetLightColor(Srgb(0xfff0dc));
	Key->SetSourceWidth(380.0f);
	Key->SetSourceHeight(260.0f);
	Key->SetBarnDoorAngle(80.0f);
	Key->SetAttenuationRadius(900.0f);
	Key->SetCastShadows(true);
	for (int32 I = 0; I < 4; ++I)
	{
		const FVector At((I % 2 == 0 ? -140.0 : 140.0), (I < 2 ? -180.0 : 180.0), TrussZ - 4.0);
		USpotLightComponent* Spot = NewPart<USpotLightComponent>(Stream);
		Spot->SetRelativeLocationAndRotation(At, (FVector(0.0, 0.0, 105.0) - At).Rotation());
		Spot->SetIntensityUnits(ELightUnits::Candelas);
		Spot->SetIntensity(1200.0f);
		Spot->SetLightColor(Srgb(I % 2 == 0 ? 0xfff2e0 : 0xffe4c4));
		Spot->SetOuterConeAngle(34.0f);
		Spot->SetInnerConeAngle(16.0f);
		Spot->SetSourceRadius(6.0f);
		Spot->SetAttenuationRadius(800.0f);
		Spot->SetCastShadows(true);
		Spot->SetVolumetricScatteringIntensity(0.5f);
	}
	// The showroom's curtains, warmed from the stage's front corners.
	for (const double S : {-1.0, 1.0})
	{
		USpotLightComponent* Drape = NewPart<USpotLightComponent>(RoomRoot);
		const FVector At(StageX - 80.0, S * 300.0, 380.0);
		Drape->SetRelativeLocationAndRotation(At, (FVector(StageX + 20.0, S * 440.0, 200.0) - At).Rotation());
		Drape->SetIntensityUnits(ELightUnits::Candelas);
		Drape->SetIntensity(900.0f);
		Drape->SetLightColor(Srgb(0xffc49a));
		Drape->SetOuterConeAngle(38.0f);
		Drape->SetInnerConeAngle(12.0f);
		Drape->SetAttenuationRadius(900.0f);
		Drape->SetCastShadows(true);
	}
	// The felt's bounce under the player.
	URectLightComponent* Bounce = NewPart<URectLightComponent>();
	Bounce->SetRelativeLocationAndRotation(FVector(0.0, 0.0, FeltZ + 2.0), FRotator(90.0f, 0.0f, 0.0f));
	Bounce->SetIntensityUnits(ELightUnits::Candelas);
	Bounce->SetIntensity(60.0f);
	Bounce->SetLightColor(Srgb(0xbfd0e0));
	Bounce->SetSourceWidth(200.0f);
	Bounce->SetSourceHeight(90.0f);
	Bounce->SetAttenuationRadius(320.0f);
	Bounce->SetCastShadows(false);
	Bounce->SetVolumetricScatteringIntensity(0.0f);
	// The room: a downlight in every coffer (MegaLights shadows them all at a fixed cost), and the coffers warmed
	// from below by cove lights hidden in the beams, so the ceiling reads instead of hanging black over the room.
	for (double X = RX0 + Module * 0.5; X < RX1; X += Module)
	{
		for (double Y = RY0 + Module * 0.5 - 40.0; Y < RY1; Y += Module)
		{
			USpotLightComponent* Down = NewPart<USpotLightComponent>(RoomRoot);
			Down->SetRelativeLocationAndRotation(FVector(X, Y, RCeil + 22.0), FRotator(-90.0f, 0.0f, 0.0f));
			Down->SetIntensityUnits(ELightUnits::Candelas);
			Down->SetIntensity(DownlightCandela);
			Down->SetLightColor(Srgb(0xffe1b8));
			Down->SetOuterConeAngle(56.0f);
			Down->SetInnerConeAngle(18.0f);
			Down->SetSourceRadius(8.0f);
			Down->SetAttenuationRadius(950.0f);
			Down->SetCastShadows(true);
			Down->SetVolumetricScatteringIntensity(0.15f);
			URectLightComponent* Cove = NewPart<URectLightComponent>(RoomRoot);
			Cove->SetRelativeLocationAndRotation(FVector(X, Y, RCeil - 4.0), FRotator(90.0f, 0.0f, 0.0f));
			Cove->SetIntensityUnits(ELightUnits::Candelas);
			Cove->SetIntensity(CoveCandela);
			Cove->SetLightColor(Srgb(0xffc890));
			Cove->SetSourceWidth(320.0f);
			Cove->SetSourceHeight(320.0f);
			Cove->SetAttenuationRadius(120.0f);
			Cove->SetCastShadows(false);
			Cove->SetVolumetricScatteringIntensity(0.0f);
		}
	}
	// Wall washers in the crown: a grazing light down every bay, so the walnut's panels and the damask read.
	auto Washer = [&](const FVector& At, float WallYaw) {
		URectLightComponent* W = NewPart<URectLightComponent>(RoomRoot);
		W->SetRelativeLocationAndRotation(At + FRotator(0.0f, WallYaw, 0.0f).Vector() * -58.0 + FVector(0.0, 0.0, RCeil - 26.0), FRotator(-71.0f, WallYaw, 0.0f));
		W->SetIntensityUnits(ELightUnits::Candelas);
		W->SetIntensity(WasherCandela);
		W->SetLightColor(Srgb(0xffd2a0));
		W->SetSourceWidth(300.0f);
		W->SetSourceHeight(14.0f);
		W->SetBarnDoorAngle(75.0f);
		W->SetAttenuationRadius(560.0f);
		W->SetCastShadows(true);
		W->SetVolumetricScatteringIntensity(0.0f);
	};
	for (double X = RX0 + Module * 0.5; X < RX1; X += Module)
	{
		if (!(X > DoorX0 && X < DoorX1))
		{
			Washer(FVector(X, RY0, 0.0), -90.0f);
		}
		if (FMath::Abs(X - RiverX) > 1.0)
		{
			Washer(FVector(X, RY1, 0.0), 90.0f);
		}
	}
	for (double Y = RY0 + Module * 0.5; Y < RY1; Y += Module)
	{
		Washer(FVector(RX0, Y, 0.0), 180.0f);
	}
	for (const double Y : {-1480.0, -1080.0, 1080.0, 1480.0})
	{
		Washer(FVector(RX1, Y, 0.0), 0.0f);
	}
	// The sconces' warm pools on the walls.
	for (double X = RX0 + Module; X < RX1; X += Module * 2.0)
	{
		for (const double S : {-1.0, 1.0})
		{
			UPointLightComponent* Wash = NewPart<UPointLightComponent>(RoomRoot);
			Wash->SetRelativeLocation(FVector(X, S * (RY1 - 40.0), 215.0));
			Wash->SetIntensityUnits(ELightUnits::Candelas);
			Wash->SetIntensity(SconceCandela);
			Wash->SetLightColor(Srgb(0xffc98a));
			Wash->SetAttenuationRadius(420.0f);
			Wash->SetSourceRadius(10.0f);
			Wash->SetCastShadows(false);
		}
	}
}

void ABackRoomStage::BuildCardRoomAir()
{
	// A clean room with a little haze: the pendants and the stream's lights show their beams faintly.
	Fog = NewPart<UExponentialHeightFogComponent>();
	Fog->SetFogDensity(0.015f);
	Fog->SetFogHeightFalloff(0.06f);
	Fog->SetFogInscatteringColor(FLinearColor(0.02f, 0.018f, 0.016f));
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.4f);
	Fog->SetVolumetricFogAlbedo(FColor(240, 236, 230));
	Fog->SetVolumetricFogExtinctionScale(1.5f);
	Fog->SetVolumetricFogDistance(3000.0f);

	Lens = NewPart<UPostProcessComponent>();
	Lens->bUnbound = true;
	FPostProcessSettings& P = Lens->Settings;
	// MegaLights (ray-traced, stochastic): every table's pendant casts shadows at a fixed cost. Here only.
	P.bOverride_bMegaLights = true;
	P.bMegaLights = 1;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	P.bOverride_AutoExposureMinBrightness = true;
	P.AutoExposureMinBrightness = 4.6f;
	P.bOverride_AutoExposureMaxBrightness = true;
	P.AutoExposureMaxBrightness = 9.6f;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = ExposureCompensation + 0.4f; // a card room is brighter than a back room
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
