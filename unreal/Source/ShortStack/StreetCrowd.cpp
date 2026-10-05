#include "StreetCrowd.h"

#include "BackRoomAnim.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStackCharacter.h"
#include "StreetAnim.h"
#include "StreetGameMode.h"
#include "StreetStage.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetCrowd, Log, All);

/**
 * The block as its people walk it, in the stage's cm: nodes on the sidewalks' lines and the rooms' aisles (the set's ends
 * among them), the edges between them (each with the room either side of its line), the spots people stop at (shop windows,
 * the Lucky Penny's shelves and counter, the Wash & Fold's machines and counter, doors, a doorway to smoke in, the bus
 * shelter) and what stands in the way.
 */
struct FStreetCrowdMap
{
	enum EEdge : uint8
	{
		/** Along a sidewalk (its fronts on one side: in the rain the umbrella-less keep under them). */
		Sidewalk = 1,
		/** Over a road at a crosswalk: a look each way at the curb first. */
		Crossing = 2,
		/** Through the Lucky Penny's sliding doors: they must be open. */
		StoreDoors = 4,
		/** Inside a room (the store, the Wash & Fold). */
		Indoors = 8,
		/** Up the Wash & Fold's steps and through its door: a hand on it, and it must have swung open. */
		WashDoor = 16,
	};
	enum class ESpot : uint8
	{
		Window,
		Shelf,
		Counter,
		Door,
		Smoke,
		Shelter,
		/** In the Wash & Fold: at a washer, a dryer, a folding table, by the chairs with the TV. */
		Laundry,
	};
	/** Which room a spot is in. */
	enum ERoom : uint8
	{
		Street = 0,
		Store = 1,
		Wash = 2,
	};

	struct FNode
	{
		FVector2D At = FVector2D::ZeroVector;
		bool bEnd = false;
		bool bIndoors = false;
	};
	struct FEdge
	{
		int32 A = INDEX_NONE;
		int32 B = INDEX_NONE;
		float Half = 100.0f;
		uint8 Flags = 0;
	};
	struct FSpot
	{
		ESpot Kind = ESpot::Window;
		/** Reached from this node, along Approach (the spot itself last). */
		int32 Node = INDEX_NONE;
		TArray<FVector2D> Approach;
		float Half = 60.0f;
		/** Facing there (degrees, the stage's) and what they look at (stage cm). */
		float Yaw = 0.0f;
		FVector LookAt = FVector::ZeroVector;
		/** The door a doorway to smoke in is beside (its spot). */
		int32 Door = INDEX_NONE;
		uint8 Room = Street;
		const TCHAR* Name = TEXT("");
	};

	TArray<FNode> Nodes;
	TArray<FEdge> Edges;
	/** Each node's edges. */
	TArray<TArray<int32>> Links;
	TArray<FSpot> Spots;
	/** What stands in the way (the street furniture, the stoops' ends, a sign on the store's floor): footprints, stage cm. */
	TArray<FBox2D> Blocks;

	TArray<int32> Ends;
	TArray<int32> Windows;
	TArray<int32> Shelves;
	TArray<int32> Homes;
	/** The Wash & Fold's washers, dryers, tables and chairs to stand at. */
	TArray<int32> Machines;
	int32 Counter = INDEX_NONE;
	int32 WashCounter = INDEX_NONE;
	/** Dee's steel door at the back of the Wash & Fold: through it to her game. */
	int32 BackDoor = INDEX_NONE;
	int32 SmokeSpot = INDEX_NONE;
	int32 ShelterSpot = INDEX_NONE;
	/** The rooms' doors: the node outside each and the one inside. */
	int32 StoreOut = INDEX_NONE;
	int32 StoreIn = INDEX_NONE;
	int32 WashOut = INDEX_NONE;
	int32 WashIn = INDEX_NONE;

	int32 AddNode(double X, double Y, bool bEnd = false, bool bIndoors = false)
	{
		FNode& N = Nodes.AddDefaulted_GetRef();
		N.At = FVector2D(X, Y);
		N.bEnd = bEnd;
		N.bIndoors = bIndoors;
		Links.AddDefaulted();
		if (bEnd)
		{
			Ends.Add(Nodes.Num() - 1);
		}
		return Nodes.Num() - 1;
	}

	void Link(int32 A, int32 B, float Half, uint8 Flags)
	{
		if (!Nodes.IsValidIndex(A) || !Nodes.IsValidIndex(B) || A == B)
		{
			return;
		}
		FEdge E;
		E.A = A;
		E.B = B;
		E.Half = Half;
		E.Flags = Flags;
		const int32 Index = Edges.Add(E);
		Links[A].Add(Index);
		Links[B].Add(Index);
	}

	/** Each node of Line linked to the next. */
	void Chain(const TArray<int32>& Line, float Half, uint8 Flags)
	{
		for (int32 I = 0; I + 1 < Line.Num(); ++I)
		{
			Link(Line[I], Line[I + 1], Half, Flags);
		}
	}

	int32 AddSpot(ESpot Kind, int32 Node, const TArray<FVector2D>& Approach, float Yaw, const FVector& LookAt, const TCHAR* Name, float Half = 60.0f, uint8 Room = Street)
	{
		if (!Nodes.IsValidIndex(Node) || Approach.Num() == 0)
		{
			return INDEX_NONE;
		}
		FSpot& S = Spots.AddDefaulted_GetRef();
		S.Kind = Kind;
		S.Node = Node;
		S.Approach = Approach;
		S.Yaw = Yaw;
		S.LookAt = LookAt;
		S.Name = Name;
		S.Half = Half;
		S.Room = Room;
		return Spots.Num() - 1;
	}

	/** A spot onto one of the lists people pick from, if it was made (AddSpot's INDEX_NONE never is). */
	void Keep(TArray<int32>& List, int32 Spot)
	{
		if (Spots.IsValidIndex(Spot))
		{
			List.Add(Spot);
		}
	}

	FVector2D SpotAt(int32 Spot) const { return Spots[Spot].Approach.Last(); }

	const FEdge* EdgeBetween(int32 A, int32 B) const
	{
		if (!Links.IsValidIndex(A))
		{
			return nullptr;
		}
		for (const int32 E : Links[A])
		{
			if ((Edges[E].A == A && Edges[E].B == B) || (Edges[E].A == B && Edges[E].B == A))
			{
				return &Edges[E];
			}
		}
		return nullptr;
	}

	/** What walking an edge costs: its length, and a crosswalk's wait at the curb on top (nobody crosses for nothing). */
	double Cost(const FEdge& E) const
	{
		return FVector2D::Distance(Nodes[E.A].At, Nodes[E.B].At) + ((E.Flags & Crossing) ? 400.0 : 0.0);
	}

	/** The cheapest walk From to To over the edges (its nodes, From first; empty when there's none), and its cost. */
	TArray<int32> Route(int32 From, int32 To, double* OutCost = nullptr) const
	{
		TArray<int32> Path;
		const int32 N = Nodes.Num();
		if (!Nodes.IsValidIndex(From) || !Nodes.IsValidIndex(To))
		{
			return Path;
		}
		// Dijkstra over a few dozen nodes: a plain scan for the next is quicker than a heap.
		TArray<double> Best;
		TArray<int32> Prev;
		TArray<bool> Done;
		Best.Init(TNumericLimits<double>::Max(), N);
		Prev.Init(INDEX_NONE, N);
		Done.Init(false, N);
		Best[From] = 0.0;
		for (int32 Pass = 0; Pass < N; ++Pass)
		{
			int32 At = INDEX_NONE;
			for (int32 I = 0; I < N; ++I)
			{
				if (!Done[I] && Best[I] < TNumericLimits<double>::Max() && (At == INDEX_NONE || Best[I] < Best[At]))
				{
					At = I;
				}
			}
			if (At == INDEX_NONE || At == To)
			{
				break;
			}
			Done[At] = true;
			for (const int32 E : Links[At])
			{
				const int32 Other = Edges[E].A == At ? Edges[E].B : Edges[E].A;
				const double C = Best[At] + Cost(Edges[E]);
				if (C < Best[Other])
				{
					Best[Other] = C;
					Prev[Other] = At;
				}
			}
		}
		if (Best[To] == TNumericLimits<double>::Max())
		{
			return Path;
		}
		for (int32 At = To; At != INDEX_NONE; At = Prev[At])
		{
			Path.Insert(At, 0);
		}
		if (OutCost)
		{
			*OutCost = Best[To];
		}
		return Path;
	}

	double RouteCost(int32 From, int32 To) const
	{
		double C = TNumericLimits<double>::Max();
		Route(From, To, &C);
		return C;
	}

	/** The node nearest At (indoors or out). */
	int32 Nearest(const FVector2D& At, bool bIndoors) const
	{
		int32 Best = INDEX_NONE;
		double BestD = TNumericLimits<double>::Max();
		for (int32 I = 0; I < Nodes.Num(); ++I)
		{
			const double D = FVector2D::DistSquared(Nodes[I].At, At);
			if (Nodes[I].bIndoors == bIndoors && D < BestD)
			{
				BestD = D;
				Best = I;
			}
		}
		return Best;
	}

	/** A node on the sidewalk's line nearest At: one there already, or a new one splitting its edge. */
	int32 Split(const FVector2D& At)
	{
		int32 Best = INDEX_NONE;
		double BestD = TNumericLimits<double>::Max();
		FVector2D BestQ = At;
		for (int32 E = 0; E < Edges.Num(); ++E)
		{
			if (!(Edges[E].Flags & Sidewalk))
			{
				continue;
			}
			const FVector2D A = Nodes[Edges[E].A].At;
			const FVector2D B = Nodes[Edges[E].B].At;
			const FVector2D AB = B - A;
			const double T = FMath::Clamp(FVector2D::DotProduct(At - A, AB) / FMath::Max(1.0, AB.SizeSquared()), 0.0, 1.0);
			const FVector2D Q = A + AB * T;
			const double D = FVector2D::DistSquared(Q, At);
			if (D < BestD)
			{
				BestD = D;
				Best = E;
				BestQ = Q;
			}
		}
		if (Best == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		const FEdge Old = Edges[Best];
		for (const int32 End : {Old.A, Old.B})
		{
			if (FVector2D::Distance(Nodes[End].At, BestQ) < 60.0)
			{
				return End;
			}
		}
		// The edge becomes A to the new node, and the new node to B.
		const int32 Mid = AddNode(BestQ.X, BestQ.Y);
		Edges[Best].B = Mid;
		Links[Mid].Add(Best);
		Links[Old.B].Remove(Best);
		Link(Mid, Old.B, Old.Half, Old.Flags);
		return Mid;
	}
};

namespace StreetCrowdDetail
{
// The street's plan (stage cm) where the stage has no accessor for it, as StreetStage.cpp lays it out (Fifth's sidewalks and
// the doors on them come from AStreetStage::Sidewalk and Doors): the home side's fronts stand on X 0, the road on X 420..1620,
// the far fronts on 2040; the set's ends at Y -6200 and 9200; Market's road on Y 5250..5850, west from the corner, its
// sidewalks either side of it to the building across its end at X -3000; the Lucky Penny's room behind X 0 on Y 3700..4950.
const double FarFront = 2040.0;
const double HomeCurbX = 370.0;
const double FarCurbX = 1670.0;
// Where the furniture at each curb begins (the lamp posts' bases): the free band beside a stoop runs from it to the stoop.
const double HomeCurbFree = 360.0;
const double FarCurbFree = 1680.0;
const double EndSouth = -6050.0;
const double EndNorth = 9050.0;
const double MarketEndX = -2850.0;
const double MarketSouthY = 5100.0;
const double MarketNorthY = 6030.0;
// The crosswalks over Fifth either side of Market (their paint Y 4990..5210 and 5890..6130; the south one walked north of its
// middle, clear of the lamp post on the home curb at the corner, Y 5020..5060), how far either side of their lines people
// walk them, and the curbs of the one over Market's mouth on Fifth's line.
const double CrossSouthY = 5140.0;
const double CrossNorthY = 6010.0;
const float CrossHalf = 80.0f;
const double MarketCurbSouth = 5205.0;
const double MarketCurbNorth = 5895.0;
// The pawn shop's lit window and the Lucky Penny's posters, on the home side; the Wash & Fold's long window across the street.
const double PawnWindowY = 1500.0;
const double StoreWindowY = 4130.0;
const double WashWindowY = 1000.0;
// Inside the Wash & Fold (BuildWashRoom, BuildWashMachines): its floor three steps up, the room behind the window from X
// 2085 to the dryers along its back wall (3140), Y 15..1825; two islands of washers back to back (X 2300..2852, round Y 635
// and 1180), folding tables between them, chairs along the window, the attendant's counter on the south wall (X 2600..2800,
// to Y 180), Dee's steel door in the back wall at Y 170; a wet-floor sign just in from the door (X 2219..2235, Y 236..266,
// its legs splayed), which the way in from the door passes on its south side.
const double WashFloor = 45.0;
const double WashBack = FarFront + 1100.0;
const double WashCounterX = 2700.0;
const double WashCounterFront = 180.0;
const double BackDoorY = 170.0;

/** Where the people are kept off the street (stage cm): under the set at its south end, hidden. */
const FVector2D Offstage(200.0, EndSouth);
const double OffstageZ = -1500.0;

/** MetaHumans are dear: never more out at once than this (and the cast one more, so who comes next isn't always who left). */
const int32 MaxOut = 6;
const int32 CastSize = MaxOut + 1;
/** How far off the camera can still make someone out (the night's haze takes the rest). */
const double SeenRange = 9000.0;
/** Nobody comes onto the street nearer the hero than this. */
const double HeroRoom = 900.0;
/** A body's radius for keeping apart (cm, at the cast's height), and the hero's. */
const float BodyRadius = 26.0f;
const float HeroRadius = 34.0f;
/** How far ahead (seconds) the steering looks for anyone to run into. */
const float Horizon = 2.2f;

const FName HandBone(TEXT("hand_r"));
const FName HeadBone(TEXT("head"));

// Tuning on the desktop, as the gait's ss.Walk.* (written blind).
TAutoConsoleVariable<int32> CVarCount(TEXT("ss.Crowd.Count"), -1, TEXT("How many people are out on Fifth (-1: as many as the hour and the rain want, at most 6)."));
TAutoConsoleVariable<int32> CVarDebug(TEXT("ss.Crowd.Debug"), 0, TEXT("Draws the pedestrians' routes, stops and plans (1)."));
TAutoConsoleVariable<float> CVarUmbrellaHold(TEXT("ss.Crowd.UmbrellaHold"), 0.72f, TEXT("How far an umbrella's hand comes up toward the chin (0 hanging .. 1 at the mouth)."));
TAutoConsoleVariable<float> CVarUmbrellaOver(TEXT("ss.Crowd.UmbrellaOver"), 34.0f, TEXT("How far over the head the umbrella's shaft aims (cm)."));

struct FExtra
{
	const TCHAR* Name;
	bool bWoman;
};
// The cast's extras (backroom_cast.py); those already at work on the street (StreetCast: Benny at the Lucky Penny's counter,
// Inez at the Wash & Fold's) aren't out walking too.
const FExtra Extras[] = {{TEXT("ExtraA"), true}, {TEXT("ExtraC"), true}, {TEXT("ExtraE"), true}, {TEXT("ExtraG"), true}, {TEXT("ExtraI"), true}, {TEXT("ExtraK"), true},
	{TEXT("ExtraB"), false}, {TEXT("ExtraD"), false}, {TEXT("ExtraF"), false}, {TEXT("ExtraH"), false}, {TEXT("ExtraJ"), false}, {TEXT("ExtraL"), false}};

bool AtWork(const TCHAR* Name)
{
	return FCString::Strcmp(Name, StreetCast::Clerk) == 0 || FCString::Strcmp(Name, StreetCast::Attendant) == 0;
}
// Jackets for a wet night in the city: charcoal, navy, olive, burgundy, camel, black, slate, forest, rust, denim, grey, stone, plum, mustard.
const uint32 Jackets[] = {0x2b2d33, 0x1f2a3d, 0x3d4a2f, 0x5a1f24, 0x8a6a45, 0x161618, 0x3e4650, 0x24402f, 0x7a3b22, 0x35506e, 0x6b6e73, 0xb9ad94, 0x4a2d4f, 0x9a7b2c};
// Umbrellas (the canopy made here before SM_Umbrella lands): mostly black, a few in a color.
const uint32 Canopies[] = {0x111214, 0x111214, 0x16171a, 0x1b2436, 0x4a1418, 0x1e3326, 0x2e2e33, 0x111214};
const TCHAR* const Sorry[] = {TEXT("Sorry."), TEXT("Oh, sorry."), TEXT("Excuse me."), TEXT("'Scuse me."), TEXT("My bad."), TEXT("Pardon me.")};
const TCHAR* const SorryInRain[] = {TEXT("Sorry, didn't see you."), TEXT("Sorry. The umbrella."), TEXT("Whoops. Sorry.")};

FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

/** A facing as a direction on the ground (degrees: 0 is +X, 90 is +Y), and back. */
FVector2D Heading(float Yaw)
{
	const double R = FMath::DegreesToRadians(static_cast<double>(Yaw));
	return FVector2D(FMath::Cos(R), FMath::Sin(R));
}

float YawOf(const FVector2D& Dir)
{
	return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)));
}

/** The right of someone walking along Dir (the stage turns +X toward +Y: its right). */
FVector2D RightOf(const FVector2D& Dir)
{
	return FVector2D(-Dir.Y, Dir.X);
}

/** Dir turned Deg degrees (+ to the right). */
FVector2D Turned(const FVector2D& Dir, float Deg)
{
	const double R = FMath::DegreesToRadians(static_cast<double>(Deg));
	const double C = FMath::Cos(R);
	const double S = FMath::Sin(R);
	return FVector2D(Dir.X * C - Dir.Y * S, Dir.X * S + Dir.Y * C);
}

/**
 * When (0..Limit seconds) someone moving at V comes within R of what's at Rel from them (Rel and V relative to it): Limit
 * when they don't; 0 when they're within R already and closing.
 */
float TimeToMeet(const FVector2D& Rel, const FVector2D& V, double R, float Limit)
{
	const double C = Rel.SizeSquared() - R * R;
	const double B = FVector2D::DotProduct(Rel, V);
	if (C < 0.0)
	{
		return B > 0.0 ? 0.0f : Limit;
	}
	const double A = V.SizeSquared();
	if (A < 1e-4 || B <= 0.0)
	{
		return Limit;
	}
	const double Disc = B * B - A * C;
	if (Disc < 0.0)
	{
		return Limit;
	}
	return FMath::Min(Limit, static_cast<float>((B - FMath::Sqrt(Disc)) / A));
}

/** Minutes into the day (0..1440). */
double TimeOfDay(double Minutes)
{
	return Minutes - FMath::FloorToDouble(Minutes / ss::net::MinutesPerDay) * ss::net::MinutesPerDay;
}

/**
 * Dee's game as Life has it ("dee-game": her nights, doors at nine, no new seats after three, the last hands dealt until
 * five): players on their way in from half an hour before her doors open, and on their way home from half an hour after the
 * last seat until the cards stop.
 */
void DeeGame(double Minutes, bool& bArriving, bool& bLeaving)
{
	// The catalog is built once and kept: its entries stay put.
	static const ss::life::Activity* const Game = ss::life::Find("dee-game");
	bArriving = false;
	bLeaving = false;
	if (!Game)
	{
		return;
	}
	const double Tod = TimeOfDay(Minutes);
	const int32 Day = ss::net::DayOf(Minutes);
	auto GameNight = [](int32 D) { return ((Game->Days >> ss::net::Weekday(D)) & 1) != 0; };
	const double Opens = static_cast<double>(Game->Opens);
	const double LastSeat = static_cast<double>(Game->Closes);
	bArriving = GameNight(Day) && Tod >= Opens - 30.0 && Tod < Opens + 150.0;
	bLeaving = GameNight(Day - 1) && Tod >= LastSeat + 30.0 && Tod < LastSeat + 150.0;
}

/** How many are out at an hour of the day in the dry: Fifth at its busiest in the evening, next to nobody in the dead hours. */
float CrowdAt(double Hour)
{
	static const float ByHour[24] = {3.0f, 2.5f, 1.0f, 1.0f, 0.8f, 1.2f, 2.0f, 3.0f, 4.5f, 4.0f, 3.5f, 3.5f, 4.5f, 4.5f, 4.0f, 4.0f, 4.5f, 5.0f, 5.5f, 6.0f, 6.0f, 6.0f, 5.0f, 4.0f};
	const double Floor = FMath::FloorToDouble(Hour);
	const int32 H = ((static_cast<int32>(Floor) % 24) + 24) % 24;
	return FMath::Lerp(ByHour[H], ByHour[(H + 1) % 24], static_cast<float>(Hour - Floor));
}

/** The share of people out in the rain who carry an umbrella. */
float UmbrellaShare(float Rain)
{
	return 0.85f * FMath::SmoothStep(0.15f, 0.7f, Rain);
}

/** How hunched someone walks in the rain (the gait's tired shoulders and head): not at all under an umbrella, or a roof. */
float HunchIn(float Rain, bool bSheltered)
{
	return bSheltered ? 0.0f : 0.35f * FMath::SmoothStep(0.3f, 0.6f, Rain);
}

/** A cast member's assembled MetaHuman exists (the same package ShortStackCharacter's ApplyCast loads). */
bool IsBuilt(const TCHAR* Name)
{
	return FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/BP_MHC_%s"), Name, Name));
}

/** A mesh from the Blender pipeline (unreal/Art/Meshes), nullptr until it's imported. */
UStaticMesh* ImportedMesh(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** A bone's transform in its mesh's component space, in the reference pose. */
FTransform RefPose(const FReferenceSkeleton& Ref, int32 Bone)
{
	FTransform T = Ref.GetRefBonePose()[Bone];
	for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
	{
		T = T * Ref.GetRefBonePose()[Parent];
	}
	return T;
}

/** The transform that fits one of the engine's shapes (its bounds) to a box of Size centered on Center, turned by Turn. */
FTransform FitShape(const UStaticMesh* Mesh, const FQuat& Turn, const FVector& Center, const FVector& Size)
{
	const FBox B = Mesh ? Mesh->GetBoundingBox() : FBox(FVector(-50.0), FVector(50.0));
	const FVector Extent = B.GetSize().ComponentMax(FVector(0.01));
	const FVector Scale = Size / Extent;
	return FTransform(Turn, Center - Turn.RotateVector(B.GetCenter() * Scale), Scale);
}

UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, FName Socket, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& At, bool bShadow)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner, MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), TEXT("CrowdProp")), RF_Transient);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCanEverAffectNavigation(false);
	C->SetGenerateOverlapEvents(false);
	C->SetCastShadow(bShadow);
	if (Material)
	{
		C->SetMaterial(0, Material);
	}
	C->SetupAttachment(Parent, Socket);
	C->SetRelativeTransform(At);
	C->RegisterComponent();
	return C;
}

/**
 * The block for its people: Fifth's two sidewalks end to end as the stage lays them out for walking (their two lanes, the
 * stoops they bend round, what stands on them), Market's two from the corner, the crosswalks; the doors on the street (the
 * Lucky Penny's and the Wash & Fold's into their rooms, 1812's and Dee's to be gone through) and the rooms' aisles; the
 * spots people stop at; a bus shelter, if one stands on the stage.
 */
TSharedPtr<FStreetCrowdMap> BuildMap(const AStreetStage& Stage)
{
	using ESpot = FStreetCrowdMap::ESpot;
	const uint8 Walkway = FStreetCrowdMap::Sidewalk;
	const FTransform& T = Stage.GetActorTransform();
	auto Local = [&T](const FVector& World) {
		const FVector L = T.InverseTransformPosition(World);
		return FVector2D(L.X, L.Y);
	};
	TSharedPtr<FStreetCrowdMap> Out = MakeShared<FStreetCrowdMap>();
	FStreetCrowdMap& M = *Out;

	// Each of Fifth's sidewalks: the line between its two lanes (each walker keeps to the one on their right), how far either
	// side of it there's room, and its stoops; the rest of what stands on it is in the way.
	struct FSide
	{
		double Line = 0.0;
		double Lane = 60.0;
		TArray<FBox2D> Stoops;
	};
	FSide Sides[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FStreetWalkway Walk = Stage.Sidewalk(Side);
		double X0 = TNumericLimits<double>::Max();
		double X1 = TNumericLimits<double>::Lowest();
		for (const FStreetLane& L : Walk.Lanes)
		{
			const double X = Local(L.Start).X;
			X0 = FMath::Min(X0, X);
			X1 = FMath::Max(X1, X);
		}
		FSide& S = Sides[Side];
		if (Walk.Lanes.Num() > 0)
		{
			S.Line = (X0 + X1) * 0.5;
			S.Lane = FMath::Max(30.0, (X1 - X0) * 0.5);
		}
		else
		{
			S.Line = Side == 0 ? 190.0 : 1845.0;
		}
		for (const FBox2D& B : Walk.Obstacles)
		{
			const FVector2D A = Local(FVector(B.Min, 0.0));
			const FVector2D C = Local(FVector(B.Max, 0.0));
			const FBox2D Box(FVector2D(FMath::Min(A.X, C.X), FMath::Min(A.Y, C.Y)), FVector2D(FMath::Max(A.X, C.X), FMath::Max(A.Y, C.Y)));
			// A stoop: a long run of the sidewalk taken from the fronts out (the line bends round it, out of the way).
			if (Box.GetSize().Y > 400.0 && (Side == 0 ? Box.Min.X < 5.0 : Box.Max.X > FarFront - 5.0))
			{
				S.Stoops.Add(Box);
			}
			M.Blocks.Add(Box);
		}
	}
	// What else stands in the way: the lamp post on Market's north sidewalk, the wet-floor signs inside the store's door and
	// the Wash & Fold's.
	M.Blocks.Add(FBox2D(FVector2D(-1716.0, 5894.0), FVector2D(-1684.0, 5926.0)));
	M.Blocks.Add(FBox2D(FVector2D(-185.0, 4505.0), FVector2D(-161.0, 4545.0)));
	M.Blocks.Add(FBox2D(FVector2D(FarFront + 172.0, 236.0), FVector2D(FarFront + 202.0, 266.0)));

	// The doors on the street, as the stage has them: where one stands before each (its sidewalk's node is level with it), and
	// its threshold.
	struct FDoor
	{
		FVector2D Out = FVector2D::ZeroVector;
		FVector2D In = FVector2D::ZeroVector;
		double Floor = 0.0;
		bool bFound = false;
	};
	FDoor StoreDoor;
	FDoor WashDoor;
	FDoor HomeDoor;
	FDoor DeeDoor;
	for (const FStreetDoor& D : Stage.Doors())
	{
		FDoor* Into = D.Id == TEXT("store") ? &StoreDoor : D.Id == TEXT("washfold") ? &WashDoor : D.Id == TEXT("home") ? &HomeDoor : D.Id == TEXT("dee") ? &DeeDoor : nullptr;
		if (Into)
		{
			Into->Out = Local(D.Outside.GetLocation());
			Into->In = Local(D.Threshold);
			Into->Floor = T.InverseTransformPosition(D.Threshold).Z;
			Into->bFound = true;
		}
	}

	// A bus shelter, if the street has one: waited under, back toward its fronts, facing the road.
	FBox Shelter(ForceInit);
	TInlineComponentArray<UStaticMeshComponent*> Meshes(&Stage);
	for (const UStaticMeshComponent* C : Meshes)
	{
		const UStaticMesh* Mesh = C ? C->GetStaticMesh().Get() : nullptr;
		if (Mesh && Mesh->GetName().Contains(TEXT("Shelter")))
		{
			Shelter = C->Bounds.GetBox().TransformBy(T.Inverse());
			break;
		}
	}

	// One side's line along Y, From to To: a node at each of Marks (OutMarks has their nodes), bending out round its stoops
	// (where the band beside one is narrower, so is the room either side of the line).
	auto Line = [&M, Walkway](const FSide& S, int32 Side, double From, double To, bool bEndFrom, bool bEndTo, const TArray<double>& Marks, TArray<int32>& OutMarks) {
		struct FPoint
		{
			double Y;
			double X;
			double Half;
			bool bEnd;
			int32 Mark;
		};
		const double CurbFree = Side == 0 ? HomeCurbFree : FarCurbFree;
		const double Half = FMath::Min(FMath::Abs(S.Line - CurbFree) - 10.0, S.Lane + 90.0);
		TArray<FPoint> P;
		P.Add({From, S.Line, Half, bEndFrom, INDEX_NONE});
		P.Add({To, S.Line, Half, bEndTo, INDEX_NONE});
		for (int32 I = 0; I < Marks.Num(); ++I)
		{
			P.Add({Marks[I], S.Line, Half, false, I});
		}
		for (const FBox2D& B : S.Stoops)
		{
			if (B.Max.Y < From || B.Min.Y > To)
			{
				continue;
			}
			const double Edge = Side == 0 ? B.Max.X : B.Min.X;
			const double X = (Edge + CurbFree) * 0.5;
			const double Narrow = FMath::Abs(CurbFree - Edge) * 0.5;
			P.Add({B.Min.Y - 200.0, S.Line, Half, false, INDEX_NONE});
			P.Add({B.Max.Y + 200.0, S.Line, Half, false, INDEX_NONE});
			for (FPoint& Q : P)
			{
				if (Q.Y > B.Min.Y - 60.0 && Q.Y < B.Max.Y + 60.0)
				{
					Q.X = X;
					Q.Half = Narrow;
				}
			}
			P.Add({B.Min.Y - 40.0, X, Narrow, false, INDEX_NONE});
			P.Add({B.Max.Y + 40.0, X, Narrow, false, INDEX_NONE});
		}
		P.Sort([](const FPoint& A, const FPoint& B) { return A.Y < B.Y; });
		OutMarks.Init(INDEX_NONE, Marks.Num());
		TArray<int32> Nodes;
		TArray<double> Room;
		for (const FPoint& Q : P)
		{
			if (Q.Y < From || Q.Y > To)
			{
				continue;
			}
			// Marks within a step of each other share a node.
			if (Nodes.Num() > 0 && FMath::Abs(M.Nodes[Nodes.Last()].At.Y - Q.Y) < 30.0 && !Q.bEnd)
			{
				if (Q.Mark != INDEX_NONE)
				{
					OutMarks[Q.Mark] = Nodes.Last();
				}
				Room.Last() = FMath::Min(Room.Last(), Q.Half);
				continue;
			}
			Nodes.Add(M.AddNode(Q.X, Q.Y, Q.bEnd));
			Room.Add(Q.Half);
			if (Q.Mark != INDEX_NONE)
			{
				OutMarks[Q.Mark] = Nodes.Last();
			}
		}
		// Each stretch has the room of the narrower of its ends (beside a stoop, the band's there).
		for (int32 I = 0; I + 1 < Nodes.Num(); ++I)
		{
			M.Link(Nodes[I], Nodes[I + 1], static_cast<float>(FMath::Max(40.0, FMath::Min(Room[I], Room[I + 1]))), Walkway);
		}
		return Nodes;
	};

	// Fifth's home side, from the set's south end to the corner of Market: 1812's door, the pawn shop's window, the Lucky
	// Penny's window and doors on it.
	const FSide& Home = Sides[0];
	TArray<double> HomeMarks = {HomeDoor.bFound ? HomeDoor.Out.Y : 0.0, PawnWindowY, StoreWindowY, StoreDoor.bFound ? StoreDoor.Out.Y : 4750.0};
	const bool bShelterHome = Shelter.IsValid && Shelter.GetCenter().X < 420.0 && Shelter.GetCenter().X > -20.0 && Shelter.GetCenter().Y < 4950.0;
	if (bShelterHome)
	{
		HomeMarks.Add(Shelter.GetCenter().Y);
	}
	TArray<int32> HomeAt;
	const TArray<int32> HomeLine = Line(Home, 0, EndSouth, MarketSouthY, true, false, HomeMarks, HomeAt);
	const int32 CornerSouth = HomeLine.Last();
	// Over Market's mouth on Fifth's line, and the home side on north from the corner to the set's end there.
	const int32 MarketCurb0 = M.AddNode(Home.Line, MarketCurbSouth);
	const int32 MarketCurb1 = M.AddNode(Home.Line, MarketCurbNorth);
	M.Link(CornerSouth, MarketCurb0, 115.0f, Walkway);
	M.Link(MarketCurb0, MarketCurb1, 140.0f, FStreetCrowdMap::Crossing);
	TArray<double> NorthMarks;
	const bool bShelterNorth = Shelter.IsValid && Shelter.GetCenter().X < 420.0 && Shelter.GetCenter().X > -20.0 && Shelter.GetCenter().Y > 6150.0;
	if (bShelterNorth)
	{
		NorthMarks.Add(Shelter.GetCenter().Y);
	}
	TArray<int32> NorthAt;
	const TArray<int32> NorthLine = Line(Home, 0, CrossNorthY, EndNorth, false, true, NorthMarks, NorthAt);
	const int32 CornerNorth = NorthLine[0];
	M.Link(MarketCurb1, CornerNorth, 115.0f, Walkway);
	// Market's two sidewalks, west from the corner to the set's end there.
	M.Chain({CornerSouth, M.AddNode(-1400.0, MarketSouthY), M.AddNode(MarketEndX, MarketSouthY, true)}, 110.0f, Walkway);
	M.Chain({CornerNorth, M.AddNode(-1400.0, MarketNorthY), M.AddNode(MarketEndX, MarketNorthY, true)}, 110.0f, Walkway);

	// The far side end to end: Dee's door, the Wash & Fold's steps and its window, the crosswalks' ends.
	const FSide& Far = Sides[1];
	TArray<double> FarMarks = {DeeDoor.bFound ? DeeDoor.Out.Y : -455.0, WashDoor.bFound ? WashDoor.Out.Y : 120.0, WashWindowY, CrossSouthY, CrossNorthY};
	const bool bShelterFar = Shelter.IsValid && Shelter.GetCenter().X > 1620.0;
	if (bShelterFar)
	{
		FarMarks.Add(Shelter.GetCenter().Y);
	}
	TArray<int32> FarAt;
	Line(Far, 1, EndSouth, EndNorth, true, true, FarMarks, FarAt);

	// The crosswalks over Fifth, curb to curb.
	const int32 HomeCurbS = M.AddNode(HomeCurbX, CrossSouthY);
	const int32 FarCurbS = M.AddNode(FarCurbX, CrossSouthY);
	M.Link(CornerSouth, HomeCurbS, CrossHalf, Walkway);
	M.Link(HomeCurbS, FarCurbS, CrossHalf, FStreetCrowdMap::Crossing);
	M.Link(FarCurbS, FarAt[3], CrossHalf, Walkway);
	const int32 HomeCurbN = M.AddNode(HomeCurbX, CrossNorthY);
	const int32 FarCurbN = M.AddNode(FarCurbX, CrossNorthY);
	M.Link(CornerNorth, HomeCurbN, CrossHalf, Walkway);
	M.Link(HomeCurbN, FarCurbN, CrossHalf, FStreetCrowdMap::Crossing);
	M.Link(FarCurbN, FarAt[4], CrossHalf, Walkway);

	// The Lucky Penny: in at its sliding doors, to the front of the store, the aisle's mouth, before the counter, the coffee
	// bar's end; its shelves and its counter (Benny's face to look at across it).
	if (StoreDoor.bFound)
	{
		const FVector2D Into = (StoreDoor.In - StoreDoor.Out).GetSafeNormal();
		M.StoreOut = M.AddNode(StoreDoor.Out.X, StoreDoor.Out.Y);
		M.StoreIn = M.AddNode(StoreDoor.In.X + Into.X * 166.0, StoreDoor.In.Y + Into.Y * 166.0, false, true);
		const int32 InFront = M.AddNode(-450.0, StoreDoor.Out.Y, false, true);
		const int32 AisleMouth = M.AddNode(-480.0, 4445.0, false, true);
		const int32 BeforeCounter = M.AddNode(-470.0, 4200.0, false, true);
		const int32 CoffeeEnd = M.AddNode(-1100.0, 4200.0, false, true);
		M.Link(HomeAt[3], M.StoreOut, 80.0f, 0);
		M.Link(M.StoreOut, M.StoreIn, 45.0f, FStreetCrowdMap::StoreDoors);
		M.Link(M.StoreIn, InFront, 80.0f, FStreetCrowdMap::Indoors);
		M.Link(InFront, AisleMouth, 60.0f, FStreetCrowdMap::Indoors);
		M.Link(AisleMouth, BeforeCounter, 60.0f, FStreetCrowdMap::Indoors);
		M.Link(BeforeCounter, CoffeeEnd, 70.0f, FStreetCrowdMap::Indoors);
		const uint8 Store = FStreetCrowdMap::Store;
		M.Keep(M.Shelves, M.AddSpot(ESpot::Shelf, InFront, {FVector2D(-1220.0, 4762.0)}, 180.0f, FVector(-1360.0, 4760.0, 125.0), TEXT("the cooler"), 70.0f, Store));
		M.Keep(M.Shelves, M.AddSpot(ESpot::Shelf, AisleMouth, {FVector2D(-830.0, 4452.0)}, 90.0f, FVector(-830.0, 4565.0, 115.0), TEXT("the snack aisle"), 55.0f, Store));
		M.Keep(M.Shelves, M.AddSpot(ESpot::Shelf, AisleMouth, {FVector2D(-830.0, 4438.0)}, -90.0f, FVector(-830.0, 4325.0, 100.0), TEXT("the chips"), 55.0f, Store));
		M.Keep(M.Shelves, M.AddSpot(ESpot::Shelf, CoffeeEnd, {FVector2D(-1100.0, 3850.0)}, -90.0f, FVector(-1100.0, 3730.0, 125.0), TEXT("the coffee bar"), 70.0f, Store));
		M.Counter = M.AddSpot(ESpot::Counter, BeforeCounter, {FVector2D(-300.0, 4135.0)}, -90.0f, FVector(-390.0, 3885.0, 160.0), TEXT("the counter"), 70.0f, Store);
	}

	// The Wash & Fold: up its steps and in at its door; past the wet-floor sign (south of it) to the front of the room, the
	// aisle along the south wall by the counter, back to the dryers and in between the islands; a washer, a dryer, a folding
	// table, the chairs with the TV up on the back wall, the attendant's counter, and Dee's steel door at the back.
	if (WashDoor.bFound)
	{
		const FVector2D Into = (WashDoor.In - WashDoor.Out).GetSafeNormal();
		const double Z = WashDoor.Floor > 1.0 ? WashDoor.Floor : WashFloor;
		M.WashOut = M.AddNode(WashDoor.Out.X, WashDoor.Out.Y);
		M.WashIn = M.AddNode(WashDoor.In.X + Into.X * 95.0, WashDoor.In.Y + Into.Y * 95.0, false, true);
		const int32 Front = M.AddNode(FarFront + 260.0, 200.0, false, true);
		const int32 South = M.AddNode(FarFront + 440.0, 330.0, false, true);
		const int32 Back = M.AddNode(WashBack - 180.0, 330.0, false, true);
		const int32 BackAisle = M.AddNode(WashBack - 180.0, 780.0, false, true);
		const int32 Between = M.AddNode(FarFront + 580.0, 786.0, false, true);
		M.Link(FarAt[1], M.WashOut, 70.0f, 0);
		M.Link(M.WashOut, M.WashIn, 40.0f, FStreetCrowdMap::WashDoor);
		M.Link(M.WashIn, Front, 60.0f, FStreetCrowdMap::Indoors);
		M.Link(Front, South, 70.0f, FStreetCrowdMap::Indoors);
		M.Link(South, Back, 70.0f, FStreetCrowdMap::Indoors);
		M.Link(Back, BackAisle, 60.0f, FStreetCrowdMap::Indoors);
		M.Link(BackAisle, Between, 55.0f, FStreetCrowdMap::Indoors);
		const uint8 Wash = FStreetCrowdMap::Wash;
		M.Keep(M.Machines, M.AddSpot(ESpot::Laundry, South, {FVector2D(FarFront + 460.0, 500.0)}, 90.0f, FVector(FarFront + 460.0, 565.0, Z + 70.0), TEXT("a washer"), 55.0f, Wash));
		M.Keep(M.Machines, M.AddSpot(ESpot::Laundry, BackAisle, {FVector2D(WashBack - 150.0, 600.0)}, 0.0f, FVector(WashBack - 80.0, 600.0, Z + 110.0), TEXT("a dryer"), 55.0f, Wash));
		M.Keep(M.Machines, M.AddSpot(ESpot::Laundry, Between, {FVector2D(FarFront + 420.0, 786.0)}, 90.0f, FVector(FarFront + 420.0, 907.0, Z + 92.0), TEXT("a folding table"), 50.0f, Wash));
		M.Keep(M.Machines, M.AddSpot(ESpot::Laundry, Front, {FVector2D(FarFront + 200.0, 700.0)}, 0.0f, FVector(WashBack - 10.0, 960.0, Z + 235.0), TEXT("the chairs"), 50.0f, Wash));
		M.WashCounter = M.AddSpot(ESpot::Counter, South, {FVector2D(WashCounterX, WashCounterFront + 75.0)}, -90.0f, FVector(WashCounterX - 8.0, 75.0, Z + 160.0), TEXT("the wash counter"), 55.0f, Wash);
		M.BackDoor = M.AddSpot(ESpot::Door, Back, {FVector2D(WashBack - 60.0, BackDoorY)}, 0.0f, FVector(WashBack, BackDoorY, Z + 150.0), TEXT("Dee's steel door"), 60.0f, Wash);
	}

	// Shop windows to stop at: the pawn shop's (lit low behind its bars), the Lucky Penny's posters, the Wash & Fold's.
	M.Keep(M.Windows, M.AddSpot(ESpot::Window, HomeAt[1], {FVector2D(68.0, PawnWindowY)}, 180.0f, FVector(0.0, PawnWindowY, 150.0), TEXT("the pawn shop's window")));
	M.Keep(M.Windows, M.AddSpot(ESpot::Window, HomeAt[2], {FVector2D(72.0, StoreWindowY)}, 180.0f, FVector(-20.0, StoreWindowY + 50.0, 150.0), TEXT("the Lucky Penny's window")));
	M.Keep(M.Windows, M.AddSpot(ESpot::Window, FarAt[2], {FVector2D(FarFront - 52.0, WashWindowY)}, 0.0f, FVector(FarFront + 160.0, WashWindowY, 135.0), TEXT("the Wash & Fold's window")));

	// Doors people live behind and go in at (gone once nobody sees them go): 1812's, and Dee's across the street.
	auto HomeSpot = [&M](const FDoor& D, int32 Node, const TCHAR* Name) {
		const FVector2D Into = (D.In - D.Out).GetSafeNormal();
		return M.AddSpot(ESpot::Door, Node, {D.Out, D.In - Into * 26.0}, YawOf(Into), FVector(D.In.X, D.In.Y, D.Floor + 130.0), Name);
	};
	if (HomeDoor.bFound)
	{
		M.Keep(M.Homes, HomeSpot(HomeDoor, HomeAt[0], TEXT("door 1812")));
	}
	int32 Dee = INDEX_NONE;
	if (DeeDoor.bFound)
	{
		Dee = HomeSpot(DeeDoor, FarAt[0], TEXT("Dee's door"));
		M.Keep(M.Homes, Dee);
	}
	// A doorway to smoke in: beside Dee's street door, back to the brick, looking out over the street.
	if (Dee != INDEX_NONE)
	{
		const double Y = DeeDoor.In.Y + 100.0;
		M.SmokeSpot = M.AddSpot(ESpot::Smoke, M.Spots[Dee].Node, {FVector2D(FarFront - 40.0, Y)}, 180.0f, FVector(1300.0, Y + 300.0, 160.0), TEXT("a doorway"));
		M.Spots[M.SmokeSpot].Door = Dee;
	}

	// Under the bus shelter: back toward its fronts, facing the road, looking up the street for the bus.
	const int32 ShelterNode = bShelterHome ? HomeAt.Last() : bShelterNorth ? NorthAt.Last() : bShelterFar ? FarAt.Last() : INDEX_NONE;
	if (ShelterNode != INDEX_NONE)
	{
		const FVector C = Shelter.GetCenter();
		const FVector E = Shelter.GetExtent();
		const bool bFar = C.X > 1620.0;
		const FVector2D Wait(bFar ? FMath::Clamp(C.X + 0.3 * E.X, 1700.0, 1990.0) : FMath::Clamp(C.X - 0.3 * E.X, 70.0, 340.0), C.Y);
		const float Facing = bFar ? 180.0f : 0.0f;
		const FVector2D Outward = Heading(Facing);
		const FVector2D Along = RightOf(Outward);
		M.ShelterSpot = M.AddSpot(ESpot::Shelter, ShelterNode, {Wait}, Facing, FVector(Wait.X + Outward.X * 500.0 - Along.X * 1500.0, Wait.Y + Outward.Y * 500.0 - Along.Y * 1500.0, 160.0),
			TEXT("the bus shelter"), 90.0f);
	}

	UE_LOG(LogStreetCrowd, Log, TEXT("The block for its people: %d nodes, %d edges, %d spots, %d in the way; lines at X %.0f and %.0f%s%s%s"), M.Nodes.Num(), M.Edges.Num(), M.Spots.Num(),
		M.Blocks.Num(), Home.Line, Far.Line, Home.Stoops.Num() + Far.Stoops.Num() > 0 ? TEXT(", round the stoops") : TEXT(""), M.WashOut != INDEX_NONE ? TEXT(", the Wash & Fold") : TEXT(""),
		M.ShelterSpot != INDEX_NONE ? TEXT(", a bus shelter") : TEXT(""));
	return Out;
}

/** Which way the fronts are from a point on a sidewalk (under them in the rain), or zero off the sidewalks. */
FVector2D FrontsSide(const FVector2D& At)
{
	if (At.X > 1620.0)
	{
		return FVector2D(1.0, 0.0);
	}
	if (At.X < 420.0 && At.Y > 4940.0 && At.Y < 5245.0)
	{
		return FVector2D(0.0, -1.0); // Market's south sidewalk, along the store's north wall
	}
	if (At.X < 420.0 && At.Y > 5855.0 && At.Y < 6160.0)
	{
		return FVector2D(0.0, 1.0);
	}
	return At.X < 420.0 ? FVector2D(-1.0, 0.0) : FVector2D::ZeroVector;
}
} // namespace StreetCrowdDetail

using namespace StreetCrowdDetail;

AStreetCrowd::AStreetCrowd()
{
	PrimaryActorTick.bCanEverTick = true;
	// The people's own ticks come first (AddTickPrerequisiteActor as each is built), their bodies' poses after.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CylinderMesh = Cylinder.Object;
	ConeMesh = Cone.Object;
	SphereMesh = Sphere.Object;
	FallbackMaterial = Basic.Object;
}

// ------------------------------------------------------------------ the session's side

void AStreetCrowd::Begin(AStreetStage* InStage, AShortStackCharacter* InHero)
{
	if (!InStage || !GetWorld())
	{
		UE_LOG(LogStreetCrowd, Warning, TEXT("No stage: Fifth Street stays empty"));
		return;
	}
	if (Stage && Stage != InStage)
	{
		// Another street: everyone from the last one goes.
		for (FStreetWalker& W : Walkers)
		{
			if (W.Body)
			{
				W.Body->Destroy();
			}
		}
		Walkers.Reset();
		bCastChosen = false;
		NumCast = 0;
	}
	Stage = InStage;
	Hero = InHero;
	Map = BuildMap(*Stage);
	for (FStreetWalker& W : Walkers)
	{
		// Begun again on the same street: whoever's out walks on from where they are, on the map as built now.
		W.Route.Reset();
		W.Spot = INDEX_NONE;
		const FVector Here = ToWorld(W.Pos, W.Floor + 50.0);
		W.Node = Map->Nearest(W.Pos, Stage->IsInsideStore(Here) || Stage->IsInsideWash(Here));
		W.NextNode = INDEX_NONE;
		W.Gate = INDEX_NONE;
		W.bLegStarted = false;
	}
	PropMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_Street.M_Street"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!PropMaterial)
	{
		PropMaterial = FallbackMaterial;
	}
	SpawnIn = 0.0f;
	BuildIn = 0.0f;
}

void AStreetCrowd::SetHour(float InDaylight, double InWorldMinutes, float InRain, bool bInPaused)
{
	Daylight = FMath::Clamp(InDaylight, 0.0f, 1.0f);
	WorldMinutes = InWorldMinutes;
	Rain = FMath::Max(0.0f, InRain);
	bPaused = bInPaused;
	if (!bHourKnown)
	{
		bHourKnown = true;
		Dice.Initialize(static_cast<int32>(FMath::FloorToDouble(WorldMinutes)) * 31 + 977);
	}
	// By the hour, fewer in a downpour, a little each way by the twenty minutes (the same each time for the same twenty), and
	// one more on the way in to Dee's game.
	const int32 Override = CVarCount.GetValueOnGameThread();
	if (Override >= 0)
	{
		Wanted = FMath::Min(Override, MaxOut);
	}
	else
	{
		const double Hour = TimeOfDay(WorldMinutes) / 60.0;
		const uint32 Slot = static_cast<uint32>(FMath::FloorToDouble(WorldMinutes / 20.0));
		const float Jitter = (static_cast<float>((Slot * 2654435761u) >> 16 & 0xffff) / 65535.0f - 0.5f) * 1.6f;
		bool bArriving = false;
		bool bLeaving = false;
		DeeGame(WorldMinutes, bArriving, bLeaving);
		const float RainFactor = 1.0f - 0.3f * FMath::SmoothStep(0.5f, 1.5f, Rain);
		const float Want = CrowdAt(Hour) * RainFactor + Jitter + (bArriving && Map && Map->BackDoor != INDEX_NONE ? 1.0f : 0.0f);
		Wanted = FMath::Clamp(FMath::RoundToInt(Want), 0, MaxOut);
	}
	if (bCastChosen)
	{
		Wanted = FMath::Min(Wanted, NumCast);
	}
}

bool AStreetCrowd::TakeBark(FVector& OutAt, FString& OutLine)
{
	if (Barks.Num() == 0)
	{
		return false;
	}
	OutAt = Barks[0].Key;
	OutLine = Barks[0].Value;
	Barks.RemoveAt(0);
	return true;
}

int32 AStreetCrowd::NumOut() const
{
	int32 N = 0;
	for (const FStreetWalker& W : Walkers)
	{
		N += W.bOut ? 1 : 0;
	}
	return N;
}

// ------------------------------------------------------------------ the people

void AStreetCrowd::ChooseCast()
{
	bCastChosen = true;
	// The day's regulars: a different few of the extras each day, the same all day.
	FRandomStream Pick(ss::net::DayOf(WorldMinutes) * 7919 + 211);
	TArray<const FExtra*> Women;
	TArray<const FExtra*> Men;
	for (const FExtra& E : Extras)
	{
		if (!AtWork(E.Name) && IsBuilt(E.Name))
		{
			(E.bWoman ? Women : Men).Add(&E);
		}
	}
	auto Shuffle = [&Pick](TArray<const FExtra*>& List) {
		for (int32 I = List.Num() - 1; I > 0; --I)
		{
			List.Swap(I, Pick.RandRange(0, I));
		}
	};
	Shuffle(Women);
	Shuffle(Men);
	TArray<const FExtra*> Chosen;
	for (int32 I = 0; Chosen.Num() < CastSize && (I < Women.Num() || I < Men.Num()); ++I)
	{
		for (const TArray<const FExtra*>* List : {&Women, &Men})
		{
			if (I < List->Num() && Chosen.Num() < CastSize)
			{
				Chosen.Add((*List)[I]);
			}
		}
	}
	if (Chosen.Num() == 0)
	{
		if (!bLoggedNoCast)
		{
			bLoggedNoCast = true;
			UE_LOG(LogStreetCrowd, Warning, TEXT("No extras are built (backroom_cast.py): Fifth Street stays empty"));
		}
		return;
	}
	TArray<uint32> Colors(Jackets, static_cast<int32>(UE_ARRAY_COUNT(Jackets)));
	for (int32 I = Colors.Num() - 1; I > 0; --I)
	{
		Colors.Swap(I, Pick.RandRange(0, I));
	}
	Walkers.SetNum(Chosen.Num());
	for (int32 I = 0; I < Chosen.Num(); ++I)
	{
		FStreetWalker& W = Walkers[I];
		W.Cast = FName(Chosen[I]->Name);
		W.bWoman = Chosen[I]->bWoman;
		W.Jacket = Srgb(Colors[I % Colors.Num()]);
		W.Rng.Initialize(Pick.RandRange(1, 1 << 30));
		// The cast is built to 175 cm: shorter and taller than that by the actor's scale.
		const float Height = W.bWoman ? W.Rng.FRandRange(158.0f, 173.0f) : W.Rng.FRandRange(167.0f, 189.0f);
		W.Scale = Height / 175.0f;
		W.Pace = W.Rng.FRandRange(118.0f, 148.0f);
		// The sidewalks' lanes stand 60 cm or so either side of their lines (AStreetStage::Sidewalk): each keeps near theirs.
		W.Lane = W.Rng.FRandRange(48.0f, 66.0f);
	}
	NumCast = Chosen.Num();
	Wanted = FMath::Min(Wanted, NumCast);
	UE_LOG(LogStreetCrowd, Log, TEXT("Fifth's people today: %d of the cast (%d women, %d men built)"), NumCast, Women.Num(), Men.Num());
}

bool AStreetCrowd::BuildBody(FStreetWalker& W)
{
	UWorld* World = GetWorld();
	if (!World || !Stage || W.Body)
	{
		return W.Body != nullptr;
	}
	// Built under the set and hidden from the start: nobody sees them until they come out.
	const FTransform Hidden(FRotator::ZeroRotator, ToWorld(Offstage, OffstageZ));
	AShortStackCharacter* B = World->SpawnActorDeferred<AShortStackCharacter>(AShortStackCharacter::StaticClass(), Hidden, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!B)
	{
		return false;
	}
	B->AutoPossessAI = EAutoPossessAI::Disabled;
	B->SetActorHiddenInGame(true);
	B->ApplyCast(*W.Cast.ToString(), W.Jacket);
	B->FinishSpawning(Hidden);
	W.Body = B;
	B->SetActorScale3D(FVector(W.Scale));
	// The crowd moves them: no movement of their own to run, and nothing for the hero's camera to stop at.
	if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		Move->SetComponentTickEnabled(false);
	}
	UCapsuleComponent* Capsule = B->GetCapsuleComponent();
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsule->SetGenerateOverlapEvents(false);
	// After the character's own tick (the crowd moves them, then sets the look over theirs), before their bodies pose.
	AddTickPrerequisiteActor(B);
	B->GetMesh()->AddTickPrerequisiteActor(this);
	// Nothing simulates out here: no cloth, no hair. And nothing they wear throws its shadow while they're hidden (the
	// character's own parts do, for the player's first person): someone gone in at a door mustn't leave theirs on the step,
	// nor anyone kept off the street cost a shadow pass.
	TInlineComponentArray<UActorComponent*> Parts(B);
	for (UActorComponent* C : Parts)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(C))
		{
			Prim->SetCastHiddenShadow(false);
		}
		if (USkeletalMeshComponent* Skin = Cast<USkeletalMeshComponent>(C))
		{
			Skin->bDisableClothSimulation = true;
			Skin->SetGenerateOverlapEvents(false);
		}
		else if (UGroomComponent* Hair = Cast<UGroomComponent>(C))
		{
			Hair->SetEnableSimulation(false);
		}
	}
	DressProps(W);
	Show(W, false);
	UE_LOG(LogStreetCrowd, Log, TEXT("%s dressed for Fifth: %.0f cm"), *W.Cast.ToString(), 175.0f * W.Scale);
	return true;
}

void AStreetCrowd::DressProps(FStreetWalker& W)
{
	AShortStackCharacter* B = W.Body;
	USkeletalMeshComponent* Skin = B ? B->GetMesh() : nullptr;
	const USkeletalMesh* Asset = Skin ? Skin->GetSkeletalMeshAsset() : nullptr;
	const int32 Hand = Asset ? Asset->GetRefSkeleton().FindBoneIndex(HandBone) : INDEX_NONE;
	if (Hand == INDEX_NONE)
	{
		return; // nothing to carry them with
	}
	const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
	const FTransform HandRef = RefPose(Ref, Hand);
	auto InHand = [&](const TCHAR* Bone, const FVector& Fallback) {
		const int32 I = Ref.FindBoneIndex(FName(Bone));
		return I == INDEX_NONE ? Fallback : HandRef.InverseTransformPosition(RefPose(Ref, I).GetLocation());
	};
	auto Paint = [this, B](uint32 Hex, float Roughness, float Metallic, float Wet) {
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(PropMaterial ? PropMaterial.Get() : FallbackMaterial.Get(), B);
		const FLinearColor C = Srgb(Hex);
		M->SetVectorParameterValue(TEXT("BaseColor"), C);
		M->SetVectorParameterValue(TEXT("Color"), C);
		M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		M->SetScalarParameterValue(TEXT("Pattern"), 0.0f);
		M->SetScalarParameterValue(TEXT("Emissive"), 0.0f);
		M->SetScalarParameterValue(TEXT("Wet"), Wet);
		return M;
	};

	// The umbrella: held at the palm (between the wrist and the middle finger's knuckle), turned upright each frame by the
	// crowd itself rather than by the hand. Its origin is the grip, its canopy up its Z.
	const FVector Palm = InHand(TEXT("middle_01_r"), FVector(9.0, 0.0, 0.0)) * 0.5;
	USceneComponent* Umbrella = NewObject<USceneComponent>(B, MakeUniqueObjectName(B, USceneComponent::StaticClass(), TEXT("CrowdUmbrella")), RF_Transient);
	Umbrella->SetupAttachment(Skin, HandBone);
	Umbrella->SetUsingAbsoluteRotation(true);
	Umbrella->SetRelativeLocation(Palm);
	Umbrella->RegisterComponent();
	if (UStaticMesh* Art = ImportedMesh(TEXT("SM_Umbrella")))
	{
		AddPart(B, Umbrella, NAME_None, Art, nullptr, FTransform::Identity, true);
	}
	else
	{
		// Before the art lands: a black canopy (a flat cone) on a steel shaft, a handle under the hand, a tip over it.
		UMaterialInterface* Canopy = Paint(Canopies[W.Rng.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Canopies)) - 1)], 0.3f, 0.0f, 0.6f);
		UMaterialInterface* Steel = Paint(0x2a2c31, 0.35f, 0.8f, 0.3f);
		UMaterialInterface* Grip = Paint(0x241a14, 0.5f, 0.0f, 0.0f);
		const FQuat Up = FQuat::Identity;
		AddPart(B, Umbrella, NAME_None, CylinderMesh, Grip, FitShape(CylinderMesh, Up, FVector(0.0, 0.0, -5.0), FVector(2.8, 2.8, 11.0)), true);
		AddPart(B, Umbrella, NAME_None, CylinderMesh, Steel, FitShape(CylinderMesh, Up, FVector(0.0, 0.0, 44.0), FVector(1.3, 1.3, 88.0)), true);
		AddPart(B, Umbrella, NAME_None, ConeMesh, Canopy, FitShape(ConeMesh, Up, FVector(0.0, 0.0, 80.0), FVector(112.0, 112.0, 26.0)), true);
		AddPart(B, Umbrella, NAME_None, CylinderMesh, Steel, FitShape(CylinderMesh, Up, FVector(0.0, 0.0, 96.5), FVector(1.0, 1.0, 7.0)), true);
	}
	Umbrella->SetVisibility(false, true);
	W.Umbrella = Umbrella;

	// The cigarette: between the index and middle fingers, across them, its lit end out past the index (the thumb's side).
	const FVector Index2 = InHand(TEXT("index_02_r"), FVector(12.0, 2.0, 0.0));
	const FVector Middle2 = InHand(TEXT("middle_02_r"), FVector(12.0, 0.0, 0.0));
	FVector Across = (InHand(TEXT("index_01_r"), FVector(9.0, 2.5, 0.0)) - InHand(TEXT("pinky_01_r"), FVector(8.0, -3.5, 0.0))).GetSafeNormal();
	if (Across.IsNearlyZero())
	{
		Across = FVector(0.0, 1.0, 0.0);
	}
	const FVector Between = (Index2 + Middle2) * 0.5;
	const FQuat Along = FRotationMatrix::MakeFromZ(Across).ToQuat();
	W.Cigarette = AddPart(B, Skin, HandBone, CylinderMesh, Paint(0xe9e4d8, 0.8f, 0.0f, 0.0f), FitShape(CylinderMesh, Along, Between + Across * 2.5, FVector(0.8, 0.8, 8.0)), false);
	W.EmberGlow = Paint(0xff6a1f, 0.6f, 0.0f, 0.0f);
	W.Ember = AddPart(B, Skin, HandBone, SphereMesh, W.EmberGlow, FitShape(SphereMesh, Along, Between + Across * 6.6, FVector(1.0)), false);
	W.Cigarette->SetVisibility(false);
	W.Ember->SetVisibility(false);
}

void AStreetCrowd::Show(FStreetWalker& W, bool bShow)
{
	AShortStackCharacter* B = W.Body;
	if (!B)
	{
		return;
	}
	B->SetActorHiddenInGame(!bShow);
	B->SetActorEnableCollision(bShow);
	B->SetActorTickEnabled(bShow);
	// Their bodies, faces, clothes and hair pose only while they're out.
	TInlineComponentArray<UActorComponent*> Parts(B);
	for (UActorComponent* C : Parts)
	{
		if (C->IsA<USkinnedMeshComponent>() || C->IsA<UGroomComponent>())
		{
			C->SetComponentTickEnabled(bShow);
		}
	}
	B->CustomTimeDilation = bShow && bPaused ? 0.0f : 1.0f;
	if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
	{
		Move->Velocity = FVector::ZeroVector;
	}
	if (!bShow)
	{
		// Back under the set: nothing of them left where they went (a door, an end of the street) for a trace to find.
		B->SetActorLocation(ToWorld(Offstage, OffstageZ), false, nullptr, ETeleportType::TeleportPhysics);
		W.Shown = FVector::ZeroVector;
		if (W.Umbrella)
		{
			W.Umbrella->SetVisibility(false, true);
		}
		if (W.Cigarette)
		{
			W.Cigarette->SetVisibility(false);
		}
		if (W.Ember)
		{
			W.Ember->SetVisibility(false);
		}
		W.bSmoking = false;
		W.UmbrellaOpen = 0.0f;
		W.bWantsDoors = false;
		W.Hunch = 0.0f;
		B->SetTired(0.0f);
	}
	W.Tier = -1;
}

void AStreetCrowd::Freeze(bool bFreeze)
{
	// Paused: everyone stops where they are, mid-stride (their own ticks and poses run on, with no time passing).
	bFrozen = bFreeze;
	for (FStreetWalker& W : Walkers)
	{
		if (W.Body && W.bOut)
		{
			W.Body->CustomTimeDilation = bFreeze ? 0.0f : 1.0f;
		}
	}
}

int32 AStreetCrowd::FreeWalker() const
{
	// Whoever's been off the street the longest (at first the cast's order, women and men in turn): the face just gone
	// isn't the next one back.
	int32 Best = INDEX_NONE;
	for (int32 I = 0; I < Walkers.Num(); ++I)
	{
		const FStreetWalker& W = Walkers[I];
		if (W.Body && !W.bOut && (Best == INDEX_NONE || W.AwaySince < Walkers[Best].AwaySince))
		{
			Best = I;
		}
	}
	return Best;
}

// ------------------------------------------------------------------ coming and going

void AStreetCrowd::UpdateCount(float Dt)
{
	if (!bCastChosen)
	{
		ChooseCast();
		Wanted = FMath::Min(Wanted, NumCast);
	}
	if (NumCast == 0)
	{
		return;
	}
	SpawnIn -= Dt;
	BuildIn -= Dt;
	int32 Out = 0;
	int32 Free = 0;
	int32 Unbuilt = 0;
	for (const FStreetWalker& W : Walkers)
	{
		Out += W.bOut ? 1 : 0;
		Free += W.Body && !W.bOut ? 1 : 0;
		Unbuilt += W.Body ? 0 : 1;
	}
	// The cast is dressed while the street fades in (two a frame: each is a hitch nobody sees, and the fade waits for them,
	// so the street comes up from black already peopled); later, only when someone's wanted and nobody's free, and then one
	// at a time.
	if (Unbuilt > 0 && (bFading || (Free == 0 && Out < Wanted && BuildIn <= 0.0f)))
	{
		int32 Budget = bFading ? 2 : 1;
		int32 Built = 0;
		for (FStreetWalker& W : Walkers)
		{
			if (!W.Body && Budget > 0)
			{
				--Budget;
				Built += BuildBody(W) ? 1 : 0;
			}
		}
		Free += Built;
		BuildIn = 2.5f;
		// Only for someone actually dressed (a spawn that fails is quick, and mustn't keep the screen black).
		bFadeHeld = bFading && Built > 0;
	}
	// More out: placed where they'd be while the screen is dark, otherwise in at an end of the set or a door nobody sees.
	while (Out < Wanted && (bDark || SpawnIn <= 0.0f))
	{
		const int32 I = FreeWalker();
		if (I == INDEX_NONE)
		{
			break;
		}
		const bool bOk = bDark ? PlaceAtStart(Walkers[I]) : StepOutAtEntry(Walkers[I]);
		const double Hour = TimeOfDay(WorldMinutes) / 60.0;
		SpawnIn = bOk ? Dice.FRandRange(3.0f, 7.0f) * (Hour < 6.0 ? 1.8f : 1.0f) : 0.8f;
		if (!bOk)
		{
			break;
		}
		++Out;
		if (!bDark)
		{
			break; // one at a time once the street can be seen
		}
	}
	// Fewer: one at a time, someone with nothing better to do sent off the set.
	if (Out > Wanted)
	{
		bool bAlreadySent = false;
		FStreetWalker* Pick = nullptr;
		for (FStreetWalker& W : Walkers)
		{
			bAlreadySent = bAlreadySent || (W.bOut && W.bSent);
			const bool bBusy = !W.bOut || W.Role == ECrowdRole::Smoke || W.Role == ECrowdRole::Errand || W.Role == ECrowdRole::Shelter || W.Role == ECrowdRole::Laundry ||
							   W.Role == ECrowdRole::Player;
			if (!bBusy && (!Pick || (W.Role == ECrowdRole::Stroll && Pick->Role != ECrowdRole::Stroll)))
			{
				Pick = &W;
			}
		}
		if (!bAlreadySent && Pick)
		{
			SendOff(*Pick);
		}
	}
}

ECrowdRole AStreetCrowd::PickRole(FStreetWalker& W, bool bPlaced) const
{
	const FStreetCrowdMap& M = *Map;
	const double Hour = TimeOfDay(WorldMinutes) / 60.0;
	bool bArriving = false;
	bool bLeaving = false;
	DeeGame(WorldMinutes, bArriving, bLeaving);
	bool bSmoker = false;
	bool bSheltered = false;
	for (const FStreetWalker& O : Walkers)
	{
		bSmoker = bSmoker || (O.bOut && O.Role == ECrowdRole::Smoke);
		bSheltered = bSheltered || (O.bOut && O.Role == ECrowdRole::Shelter);
	}
	TArray<TPair<ECrowdRole, float>, TInlineAllocator<8>> Odds;
	Odds.Emplace(ECrowdRole::Stroll, 50.0f);
	if (M.StoreOut != INDEX_NONE && M.Shelves.Num() > 0 && !HeroInStore())
	{
		Odds.Emplace(ECrowdRole::Errand, Hour < 6.0 || Hour >= 22.0 ? 18.0f : 24.0f);
	}
	if (M.Homes.Num() > 0)
	{
		Odds.Emplace(ECrowdRole::Home, Hour >= 17.0 || Hour < 3.0 ? 14.0f : 6.0f);
	}
	if (M.WashOut != INDEX_NONE && M.Machines.Num() > 0 && !HeroInWash())
	{
		Odds.Emplace(ECrowdRole::Laundry, Hour >= 1.0 && Hour < 7.0 ? 4.0f : 11.0f);
	}
	if (M.BackDoor != INDEX_NONE)
	{
		if (bArriving)
		{
			Odds.Emplace(ECrowdRole::Player, 30.0f);
		}
		if (bLeaving && !bPlaced)
		{
			Odds.Emplace(ECrowdRole::PlayerOut, 30.0f);
		}
	}
	if (M.SmokeSpot != INDEX_NONE && !bSmoker && (Hour >= 18.0 || Hour < 4.0 || Daylight < 0.3f))
	{
		Odds.Emplace(ECrowdRole::Smoke, 14.0f);
	}
	if (M.ShelterSpot != INDEX_NONE && !bSheltered)
	{
		Odds.Emplace(ECrowdRole::Shelter, 12.0f);
	}
	float Total = 0.0f;
	for (const TPair<ECrowdRole, float>& O : Odds)
	{
		Total += O.Value;
	}
	float Roll = W.Rng.FRandRange(0.0f, Total);
	for (const TPair<ECrowdRole, float>& O : Odds)
	{
		Roll -= O.Value;
		if (Roll <= 0.0f)
		{
			return O.Key;
		}
	}
	return ECrowdRole::Stroll;
}

bool AStreetCrowd::PlaceAtStart(FStreetWalker& W)
{
	// The screen is dark (the street fading in): they're wherever they'd be by now, if not on top of the hero or each other.
	const FStreetCrowdMap& M = *Map;
	const FVector2D HeroPos = ToStage(HeroAt());
	const bool bHero = Hero.IsValid();
	auto Clear = [&](const FVector2D& At, double Room) {
		if (bHero && FVector2D::Distance(At, HeroPos) < 600.0)
		{
			return false;
		}
		for (const FStreetWalker& O : Walkers)
		{
			if (&O != &W && O.bOut && FVector2D::Distance(O.Pos, At) < Room)
			{
				return false;
			}
		}
		return true;
	};
	ECrowdRole Purpose = PickRole(W, true);
	// Mid-cigarette in the doorway, at the shelter, or at a shelf in the store (not with the hero in it).
	auto AtSpot = [&](int32 Spot, ECrowdRole As) {
		if (Spot == INDEX_NONE || !Clear(M.SpotAt(Spot), 150.0))
		{
			return false;
		}
		const FStreetCrowdMap::FSpot& S = M.Spots[Spot];
		Enter(W, As, S.Approach.Last(), S.Yaw, S.Node, Spot);
		return true;
	};
	if (Purpose == ECrowdRole::Smoke && AtSpot(M.SmokeSpot, Purpose))
	{
		W.Plan[0].Duration = W.Rng.FRandRange(40.0f, W.Plan[0].Duration);
		return true;
	}
	if (Purpose == ECrowdRole::Shelter && AtSpot(M.ShelterSpot, Purpose))
	{
		return true;
	}
	if (Purpose == ECrowdRole::Errand && M.Shelves.Num() > 0 && W.Rng.FRand() < 0.5f && AtSpot(M.Shelves[W.Rng.RandRange(0, M.Shelves.Num() - 1)], Purpose))
	{
		// Already at a shelf: the counter next, then out.
		const int32 Shelf = W.Spot;
		W.Plan.Reset();
		W.Plan.Add(FCrowdLeg{ECrowdLeg::Visit, Shelf, static_cast<float>(W.Rng.FRandRange(2.0f, 6.0f))});
		W.Plan.Add(FCrowdLeg{ECrowdLeg::Visit, M.Counter, static_cast<float>(W.Rng.FRandRange(6.0f, 12.0f))});
		W.Plan.Add(FCrowdLeg{ECrowdLeg::Leave, PickEnd(W, INDEX_NONE), 0.0f});
		W.LegAt = 0;
		W.bLegStarted = false;
		return true;
	}
	if (Purpose == ECrowdRole::Laundry && M.Machines.Num() > 0 && W.Rng.FRand() < 0.6f)
	{
		// Already in the Wash & Fold, at a machine or by the chairs: done there, then off.
		const int32 Machine = M.Machines[W.Rng.RandRange(0, M.Machines.Num() - 1)];
		if (AtSpot(Machine, Purpose))
		{
			W.Plan.Reset();
			W.Plan.Add(FCrowdLeg{ECrowdLeg::Visit, Machine, static_cast<float>(W.Rng.FRandRange(10.0f, 60.0f))});
			W.Plan.Add(FCrowdLeg{ECrowdLeg::Leave, PickEnd(W, INDEX_NONE), 0.0f});
			W.LegAt = 0;
			W.bLegStarted = false;
			return true;
		}
	}
	if (Purpose == ECrowdRole::Smoke || Purpose == ECrowdRole::Shelter || Purpose == ECrowdRole::PlayerOut)
	{
		Purpose = ECrowdRole::Stroll;
	}
	// Otherwise somewhere along a sidewalk, walking.
	double Total = 0.0;
	for (const FStreetCrowdMap::FEdge& E : M.Edges)
	{
		Total += (E.Flags & FStreetCrowdMap::Sidewalk) ? FVector2D::Distance(M.Nodes[E.A].At, M.Nodes[E.B].At) : 0.0;
	}
	for (int32 Try = 0; Try < 12 && Total > 0.0; ++Try)
	{
		double Roll = W.Rng.FRandRange(0.0f, static_cast<float>(Total));
		for (const FStreetCrowdMap::FEdge& E : M.Edges)
		{
			if (!(E.Flags & FStreetCrowdMap::Sidewalk))
			{
				continue;
			}
			const double Len = FVector2D::Distance(M.Nodes[E.A].At, M.Nodes[E.B].At);
			Roll -= Len;
			if (Roll > 0.0)
			{
				continue;
			}
			const bool bForward = W.Rng.FRand() < 0.5f;
			const int32 From = bForward ? E.A : E.B;
			const int32 To = bForward ? E.B : E.A;
			const FVector2D At = FMath::Lerp(M.Nodes[From].At, M.Nodes[To].At, static_cast<double>(W.Rng.FRandRange(0.1f, 0.9f)));
			if (!Clear(At, 300.0))
			{
				break;
			}
			Enter(W, Purpose, At, YawOf(M.Nodes[To].At - M.Nodes[From].At), From, INDEX_NONE);
			W.NextNode = To;
			W.Speed = W.Pace * 0.8f;
			return true;
		}
	}
	return false;
}

bool AStreetCrowd::StepOutAtEntry(FStreetWalker& W)
{
	const FStreetCrowdMap& M = *Map;
	const FVector2D HeroPos = ToStage(HeroAt());
	const bool bHero = Hero.IsValid();
	// Free to come out at: unseen, away from the hero, nobody standing there.
	auto Usable = [&](const FVector2D& At) {
		if (bHero && FVector2D::Distance(At, HeroPos) < HeroRoom)
		{
			return false;
		}
		for (const FStreetWalker& O : Walkers)
		{
			if (O.bOut && FVector2D::Distance(O.Pos, At) < 200.0)
			{
				return false;
			}
		}
		return !IsSeen(At, FloorAt(At, 0.0f), 180.0f * W.Scale);
	};
	auto OutOfDoor = [&](int32 Door, ECrowdRole As) {
		if (Door == INDEX_NONE || !Usable(M.SpotAt(Door)))
		{
			return false;
		}
		const FStreetCrowdMap::FSpot& S = M.Spots[Door];
		Enter(W, As, S.Approach.Last(), S.Yaw + 180.0f, S.Node, Door);
		return true;
	};
	ECrowdRole Purpose = PickRole(W, false);
	// The smoker and the game's players come out of their doors.
	if (Purpose == ECrowdRole::Smoke)
	{
		if (M.SmokeSpot != INDEX_NONE && OutOfDoor(M.Spots[M.SmokeSpot].Door, Purpose))
		{
			return true;
		}
		Purpose = ECrowdRole::Stroll;
	}
	if (Purpose == ECrowdRole::PlayerOut)
	{
		if (OutOfDoor(M.BackDoor, Purpose))
		{
			return true;
		}
		Purpose = ECrowdRole::Stroll;
	}
	// Everyone else onto the street at an end of the set; now and then out of a home instead (not to go home again).
	TArray<TPair<int32, float>, TInlineAllocator<12>> Ways;
	for (const int32 End : M.Ends)
	{
		if (Usable(M.Nodes[End].At))
		{
			Ways.Emplace(End, 1.0f);
		}
	}
	if (Purpose != ECrowdRole::Home)
	{
		for (const int32 Door : M.Homes)
		{
			if (Usable(M.SpotAt(Door)))
			{
				Ways.Emplace(-2 - Door, 0.3f);
			}
		}
	}
	if (Ways.Num() == 0)
	{
		return false;
	}
	float Total = 0.0f;
	for (const TPair<int32, float>& Way : Ways)
	{
		Total += Way.Value;
	}
	float Roll = W.Rng.FRandRange(0.0f, Total);
	int32 Chosen = Ways.Last().Key;
	for (const TPair<int32, float>& Way : Ways)
	{
		Roll -= Way.Value;
		if (Roll <= 0.0f)
		{
			Chosen = Way.Key;
			break;
		}
	}
	if (Chosen <= -2)
	{
		return OutOfDoor(-2 - Chosen, Purpose);
	}
	// Onto the street from its end, facing along it.
	const TArray<int32>& Links = M.Links[Chosen];
	const FStreetCrowdMap::FEdge& E = M.Edges[Links[0]];
	const int32 Next = E.A == Chosen ? E.B : E.A;
	Enter(W, Purpose, M.Nodes[Chosen].At, YawOf(M.Nodes[Next].At - M.Nodes[Chosen].At), Chosen, INDEX_NONE);
	W.Speed = W.Pace * 0.7f;
	return true;
}

void AStreetCrowd::Enter(FStreetWalker& W, ECrowdRole Purpose, const FVector2D& At, float Yaw, int32 Node, int32 AtSpot)
{
	W.Pos = At;
	W.Yaw = Yaw;
	W.Speed = 0.0f;
	W.SteerAngle = 0.0f;
	W.Node = Node;
	W.NextNode = INDEX_NONE;
	W.Spot = AtSpot;
	W.FloorWant = FloorAt(At, 0.0f);
	W.Floor = W.FloorWant;
	W.Route.Reset();
	W.RouteAt = 0;
	W.Gate = INDEX_NONE;
	W.bArrived = false;
	W.bHold = false;
	W.Blocked = 0.0f;
	W.bSent = false;
	W.bOut = true;
	W.Refused = INDEX_NONE;
	W.BumpFor = 0.0f;
	W.ProbeIn = W.Rng.FRandRange(0.0f, 0.3f);
	W.LookAt = FVector::ZeroVector;
	W.LookWho = INDEX_NONE;
	W.LookUntil = 0.0f;
	W.NextLook = Clock + W.Rng.FRandRange(1.0f, 3.0f);
	W.LookYaw = 0.0f;
	W.LookPitch = 0.0f;
	W.LookDown = 0.0f;
	W.LookAside = 0.0f;
	W.HeroGlanceIn = W.Rng.FRandRange(2.0f, 6.0f);
	W.Frame = W.Rng.RandRange(0, 7);
	// The weather they came out in: an umbrella (already up, unless they're only now stepping out of a door) or none.
	W.bUmbrella = Purpose != ECrowdRole::Smoke && Rain > 0.2f && W.Rng.FRand() < UmbrellaShare(Rain);
	W.UmbrellaOpen = W.bUmbrella && AtSpot == INDEX_NONE ? 1.0f : 0.0f;
	W.DryFor = 0.0f;
	W.bSmoking = Purpose == ECrowdRole::Smoke;
	W.NextDrag = Clock + W.Rng.FRandRange(2.0f, 6.0f);
	W.DragAt = -100.0f;
	PlanFor(W, Purpose, Map->Nodes.IsValidIndex(Node) && Map->Nodes[Node].bEnd && AtSpot == INDEX_NONE ? Node : INDEX_NONE);
	Show(W, true);
	// Without an umbrella out in the rain, they come out hunched against it already (UpdateProps keeps it with the rain).
	const bool bIndoors = Map->Spots.IsValidIndex(AtSpot) && Map->Spots[AtSpot].Room != FStreetCrowdMap::Street;
	W.Hunch = HunchIn(Rain, W.bUmbrella || W.bSmoking || bIndoors);
	if (AShortStackCharacter* B = W.Body)
	{
		B->SetTired(W.Hunch);
	}
	Settle(W, 0.0f, false);
}

void AStreetCrowd::PutAway(FStreetWalker& W)
{
	W.bOut = false;
	W.bSent = false;
	W.AwaySince = Clock;
	W.Route.Reset();
	W.Plan.Reset();
	Show(W, false);
}

void AStreetCrowd::SendOff(FStreetWalker& W)
{
	// The nearest way off the set they haven't just been turned back from.
	const FStreetCrowdMap& M = *Map;
	int32 Best = INDEX_NONE;
	double BestCost = TNumericLimits<double>::Max();
	const int32 From = M.Nodes.IsValidIndex(W.NextNode) ? W.NextNode : W.Node;
	for (const int32 End : M.Ends)
	{
		if (End == W.Refused && Clock < W.RefusedUntil)
		{
			continue;
		}
		const double C = M.RouteCost(From, End);
		if (C < BestCost)
		{
			BestCost = C;
			Best = End;
		}
	}
	if (Best == INDEX_NONE)
	{
		return;
	}
	W.bSent = true;
	W.Plan.Reset();
	W.Plan.Add(FCrowdLeg{ECrowdLeg::Leave, Best, 0.0f});
	W.LegAt = 0;
	W.bLegStarted = false;
}

void AStreetCrowd::PlanFor(FStreetWalker& W, ECrowdRole Purpose, int32 From)
{
	const FStreetCrowdMap& M = *Map;
	W.Role = Purpose;
	W.Plan.Reset();
	W.LegAt = 0;
	W.bLegStarted = false;
	auto Leg = [&W](ECrowdLeg Kind, int32 Target, float Duration) {
		if (Target != INDEX_NONE)
		{
			W.Plan.Add(FCrowdLeg{Kind, Target, Duration});
		}
	};
	auto Window = [&](float Odds) {
		if (W.Rng.FRand() < Odds)
		{
			Leg(ECrowdLeg::Visit, PickWindow(W), W.Rng.FRandRange(3.5f, 9.0f));
		}
	};
	switch (Purpose)
	{
	case ECrowdRole::Stroll:
		Window(Daylight > 0.5f ? 0.5f : 0.4f);
		break;
	case ECrowdRole::Errand:
	{
		// A shelf or two, then the counter.
		const int32 First = M.Shelves.Num() > 0 ? M.Shelves[W.Rng.RandRange(0, M.Shelves.Num() - 1)] : INDEX_NONE;
		Leg(ECrowdLeg::Visit, First, W.Rng.FRandRange(4.0f, 9.0f));
		if (M.Shelves.Num() > 1 && W.Rng.FRand() < 0.45f)
		{
			int32 Second = First;
			while (Second == First)
			{
				Second = M.Shelves[W.Rng.RandRange(0, M.Shelves.Num() - 1)];
			}
			Leg(ECrowdLeg::Visit, Second, W.Rng.FRandRange(3.0f, 7.0f));
		}
		Leg(ECrowdLeg::Visit, M.Counter, W.Rng.FRandRange(6.0f, 12.0f));
		break;
	}
	case ECrowdRole::Home:
		Window(0.3f);
		if (M.Homes.Num() > 0)
		{
			Leg(ECrowdLeg::Enter, M.Homes[W.Rng.RandRange(0, M.Homes.Num() - 1)], 0.0f);
		}
		return;
	case ECrowdRole::Laundry:
		if (M.Machines.Num() == 0 || W.Rng.FRand() < 0.35f)
		{
			// A bag of wash-and-fold left with the attendant.
			Leg(ECrowdLeg::Visit, M.WashCounter, W.Rng.FRandRange(8.0f, 16.0f));
			break;
		}
		// A load in, a while by the chairs with the TV, then to a dryer or a table.
		Leg(ECrowdLeg::Visit, M.Machines[0], W.Rng.FRandRange(15.0f, 30.0f));
		Leg(ECrowdLeg::Visit, M.Machines.Last(), W.Rng.FRandRange(40.0f, 110.0f));
		if (M.Machines.Num() > 3)
		{
			Leg(ECrowdLeg::Visit, M.Machines[W.Rng.RandRange(1, M.Machines.Num() - 2)], W.Rng.FRandRange(15.0f, 35.0f));
		}
		break;
	case ECrowdRole::Player:
		// To Dee's steel door at the back of the Wash & Fold, and through it to her game.
		Leg(ECrowdLeg::Enter, M.BackDoor, 0.0f);
		return;
	case ECrowdRole::Smoke:
		if (M.SmokeSpot != INDEX_NONE)
		{
			Leg(ECrowdLeg::Smoke, M.SmokeSpot, W.Rng.FRandRange(150.0f, 330.0f));
			Leg(ECrowdLeg::Enter, M.Spots[M.SmokeSpot].Door, 0.0f);
		}
		return;
	case ECrowdRole::Shelter:
		Leg(ECrowdLeg::Visit, M.ShelterSpot, W.Rng.FRandRange(90.0f, 280.0f));
		break;
	case ECrowdRole::PlayerOut:
	default:
		break;
	}
	Leg(ECrowdLeg::Leave, PickEnd(W, From), 0.0f);
}

int32 AStreetCrowd::PickEnd(FStreetWalker& W, int32 Not) const
{
	// Any end but the one they came in by (or were just turned back from), the farther the likelier: a walk worth the name.
	const FStreetCrowdMap& M = *Map;
	float Total = 0.0f;
	TArray<TPair<int32, float>, TInlineAllocator<8>> Odds;
	for (const int32 End : M.Ends)
	{
		if (End == Not || (End == W.Refused && Clock < W.RefusedUntil))
		{
			continue;
		}
		const float Far = static_cast<float>(FVector2D::Distance(M.Nodes[End].At, W.Pos));
		Odds.Emplace(End, 500.0f + Far);
		Total += 500.0f + Far;
	}
	if (Odds.Num() == 0)
	{
		return M.Ends.Num() > 0 ? M.Ends[0] : INDEX_NONE;
	}
	float Roll = W.Rng.FRandRange(0.0f, Total);
	for (const TPair<int32, float>& O : Odds)
	{
		Roll -= O.Value;
		if (Roll <= 0.0f)
		{
			return O.Key;
		}
	}
	return Odds.Last().Key;
}

int32 AStreetCrowd::PickWindow(FStreetWalker& W) const
{
	// A window not too far out of their way, and not one someone's already at.
	const FStreetCrowdMap& M = *Map;
	TArray<TPair<int32, float>, TInlineAllocator<4>> Odds;
	float Total = 0.0f;
	for (const int32 Spot : M.Windows)
	{
		bool bTaken = false;
		for (const FStreetWalker& O : Walkers)
		{
			bTaken = bTaken || (O.bOut && &O != &W && O.Plan.IsValidIndex(O.LegAt) && O.Plan[O.LegAt].Target == Spot && O.Plan[O.LegAt].Kind == ECrowdLeg::Visit);
		}
		if (bTaken)
		{
			continue;
		}
		const float Odd = 1.0f / (1.0f + static_cast<float>(FVector2D::Distance(M.SpotAt(Spot), W.Pos)) / 2500.0f);
		Odds.Emplace(Spot, Odd);
		Total += Odd;
	}
	float Roll = W.Rng.FRandRange(0.0f, Total);
	for (const TPair<int32, float>& O : Odds)
	{
		Roll -= O.Value;
		if (Roll <= 0.0f)
		{
			return O.Key;
		}
	}
	return Odds.Num() > 0 ? Odds.Last().Key : INDEX_NONE;
}

// ------------------------------------------------------------------ the plan

void AStreetCrowd::RunLeg(FStreetWalker& W, float Dt)
{
	const FStreetCrowdMap& M = *Map;
	W.bHold = false;
	W.LookDown = 0.0f;
	if (!W.Plan.IsValidIndex(W.LegAt))
	{
		// Done with what they came out for: off the set.
		W.Plan.Reset();
		W.Plan.Add(FCrowdLeg{ECrowdLeg::Leave, PickEnd(W, INDEX_NONE), 0.0f});
		W.LegAt = 0;
		W.bLegStarted = false;
	}
	if (!W.bLegStarted)
	{
		StartLeg(W);
		if (!W.bLegStarted || !W.Plan.IsValidIndex(W.LegAt))
		{
			return; // nothing to walk to on that leg: the next one, next frame
		}
	}
	const FCrowdLeg Leg = W.Plan[W.LegAt];
	const bool bSpotLeg = Leg.Kind == ECrowdLeg::Visit || Leg.Kind == ECrowdLeg::Smoke || Leg.Kind == ECrowdLeg::Enter;
	if (bSpotLeg && !W.bArrived && M.Spots.IsValidIndex(Leg.Target))
	{
		// Someone at their spot already: they wait their turn a step off, and give it up after a while.
		const FVector2D At = M.SpotAt(Leg.Target);
		bool bTaken = Hero.IsValid() && FVector2D::Distance(ToStage(HeroAt()), At) < 80.0;
		for (const FStreetWalker& O : Walkers)
		{
			bTaken = bTaken || (&O != &W && O.bOut && FVector2D::Distance(O.Pos, At) < 70.0);
		}
		if (bTaken && FVector2D::Distance(W.Pos, At) < 240.0)
		{
			W.bHold = true;
			W.Waited += Dt;
			if (W.Waited > 7.0f)
			{
				NextLeg(W);
			}
			return;
		}
	}
	if (bSpotLeg && W.bArrived)
	{
		W.Spot = Leg.Target;
	}
	switch (Leg.Kind)
	{
	case ECrowdLeg::Go:
		if (W.bArrived)
		{
			NextLeg(W);
		}
		break;
	case ECrowdLeg::Visit:
	case ECrowdLeg::Smoke:
		if (!W.bArrived)
		{
			break;
		}
		W.Stay += Dt;
		if (W.Stay >= Leg.Duration)
		{
			NextLeg(W);
		}
		break;
	case ECrowdLeg::Enter:
		if (!W.bArrived)
		{
			break;
		}
		// A moment at the door (the keys, the handle, a look down; a knock at Dee's), then in, once nobody sees them go.
		W.Stay += Dt;
		W.LookDown = Leg.Target == M.BackDoor ? 4.0f : 24.0f;
		if (W.Stay > 1.2f && CanVanish(W))
		{
			PutAway(W);
			return;
		}
		if (W.Stay > (Leg.Target == M.BackDoor ? 15.0f : 6.5f))
		{
			// Watched at the door all this while: they think better of it and walk on (a smoker lights another).
			TurnBack(W, INDEX_NONE);
		}
		break;
	case ECrowdLeg::Leave:
		if (!W.bArrived)
		{
			break;
		}
		if (CanVanish(W))
		{
			PutAway(W);
			return;
		}
		// Seen at the end of the set: they turn and walk the block back the other way.
		TurnBack(W, Leg.Target);
		break;
	}
}

void AStreetCrowd::StartLeg(FStreetWalker& W)
{
	const FCrowdLeg& Leg = W.Plan[W.LegAt];
	W.bLegStarted = true;
	W.Stay = 0.0f;
	W.Waited = 0.0f;
	W.Blocked = 0.0f;
	if (Leg.Kind == ECrowdLeg::Go || Leg.Kind == ECrowdLeg::Leave)
	{
		RouteTo(W, Leg.Target, INDEX_NONE, false);
	}
	else if (Map->Spots.IsValidIndex(Leg.Target))
	{
		RouteTo(W, Map->Spots[Leg.Target].Node, Leg.Target, true);
	}
	else
	{
		NextLeg(W);
	}
}

void AStreetCrowd::NextLeg(FStreetWalker& W)
{
	++W.LegAt;
	W.bLegStarted = false;
	W.bArrived = false;
	if (W.Plan.IsValidIndex(W.LegAt - 1) && W.Plan[W.LegAt - 1].Kind == ECrowdLeg::Smoke && W.Plan.IsValidIndex(W.LegAt))
	{
		// The last drag: they head in with it.
		W.NextDrag = Clock + 1000.0f;
	}
}

void AStreetCrowd::TurnBack(FStreetWalker& W, int32 RefusedEnd)
{
	if (RefusedEnd != INDEX_NONE)
	{
		W.Refused = RefusedEnd;
		W.RefusedUntil = Clock + 45.0f;
	}
	W.Plan.Reset();
	W.LegAt = 0;
	W.bLegStarted = false;
	if (W.Role == ECrowdRole::Smoke && Map->SmokeSpot != INDEX_NONE)
	{
		// Another cigarette, and try the door again after it.
		W.Plan.Add(FCrowdLeg{ECrowdLeg::Smoke, Map->SmokeSpot, static_cast<float>(W.Rng.FRandRange(35.0f, 70.0f))});
		W.Plan.Add(FCrowdLeg{ECrowdLeg::Enter, Map->Spots[Map->SmokeSpot].Door, 0.0f});
		W.NextDrag = Clock + W.Rng.FRandRange(2.0f, 5.0f);
		return;
	}
	W.Plan.Add(FCrowdLeg{ECrowdLeg::Leave, PickEnd(W, RefusedEnd), 0.0f});
}

void AStreetCrowd::SkipRoom(FStreetWalker& W, uint8 Room)
{
	// What they meant to do in there keeps for another day: on with the rest of the plan (or off the set).
	auto InRoom = [this, Room](const FCrowdLeg& Leg) {
		return Leg.Kind != ECrowdLeg::Go && Leg.Kind != ECrowdLeg::Leave && Map->Spots.IsValidIndex(Leg.Target) && Map->Spots[Leg.Target].Room == Room;
	};
	while (W.Plan.IsValidIndex(W.LegAt) && InRoom(W.Plan[W.LegAt]))
	{
		++W.LegAt;
	}
	W.bLegStarted = false;
	W.bArrived = false;
}

void AStreetCrowd::RouteTo(FStreetWalker& W, int32 TargetNode, int32 TargetSpot, bool bStop)
{
	const FStreetCrowdMap& M = *Map;
	W.Route.Reset();
	W.RouteAt = 0;
	W.Gate = INDEX_NONE;
	W.GateWait = 0.0f;
	W.bStops = bStop;
	W.bArrived = false;
	auto Add = [&W](const FVector2D& At, int32 Node, float Half, uint8 Flags) {
		FCrowdWaypoint& P = W.Route.AddDefaulted_GetRef();
		P.At = At;
		P.Walk = At;
		P.Node = Node;
		P.Half = Half;
		P.Flags = Flags;
	};
	const FStreetCrowdMap::FSpot* From = M.Spots.IsValidIndex(W.Spot) ? &M.Spots[W.Spot] : nullptr;
	const FStreetCrowdMap::FSpot* To = M.Spots.IsValidIndex(TargetSpot) ? &M.Spots[TargetSpot] : nullptr;
	Add(W.Pos, INDEX_NONE, From ? From->Half : 90.0f, 0);
	if (To && W.Spot == TargetSpot)
	{
		// There already.
		W.bArrived = true;
		LayLanes(W);
		return;
	}
	if (To && From && (To->Door == W.Spot || From->Door == TargetSpot))
	{
		// Between a door and the doorway beside it, straight: out to smoke by the door, in again by its step.
		if (From->Door == TargetSpot)
		{
			for (const FVector2D& P : To->Approach)
			{
				Add(P, INDEX_NONE, To->Half, 0);
			}
		}
		else
		{
			Add(To->Approach.Last(), INDEX_NONE, To->Half, 0);
		}
		W.Spot = INDEX_NONE;
		LayLanes(W);
		return;
	}
	int32 Start = INDEX_NONE;
	if (From)
	{
		// Away from the spot the way they came to it.
		for (int32 I = From->Approach.Num() - 2; I >= 0; --I)
		{
			Add(From->Approach[I], INDEX_NONE, From->Half, 0);
		}
		Start = From->Node;
	}
	else
	{
		// From whichever of the nodes they're between makes the shorter walk.
		double Best = TNumericLimits<double>::Max();
		for (const int32 Node : {W.Node, W.NextNode})
		{
			if (!M.Nodes.IsValidIndex(Node))
			{
				continue;
			}
			const double C = FVector2D::Distance(W.Pos, M.Nodes[Node].At) + M.RouteCost(Node, TargetNode);
			if (C < Best)
			{
				Best = C;
				Start = Node;
			}
		}
		if (Start == INDEX_NONE)
		{
			const FVector Here = ToWorld(W.Pos, W.Floor + 50.0);
			Start = M.Nearest(W.Pos, Stage && (Stage->IsInsideStore(Here) || Stage->IsInsideWash(Here)));
		}
	}
	W.Spot = INDEX_NONE;
	const TArray<int32> Path = M.Route(Start, TargetNode);
	if (Path.Num() == 0)
	{
		// No way there on the map (it shouldn't happen): straight to it.
		if (M.Nodes.IsValidIndex(TargetNode))
		{
			Add(M.Nodes[TargetNode].At, TargetNode, 90.0f, 0);
		}
	}
	for (int32 I = 0; I < Path.Num(); ++I)
	{
		const int32 Node = Path[I];
		if (I == 0 && W.Route.Num() == 1 && FVector2D::Distance(W.Pos, M.Nodes[Node].At) < 30.0)
		{
			// Standing on it: walk on from here, along the stretch they're on.
			W.Route[0].Node = Node;
			continue;
		}
		const FStreetCrowdMap::FEdge* E = I + 1 < Path.Num() ? M.EdgeBetween(Node, Path[I + 1]) : nullptr;
		Add(M.Nodes[Node].At, Node, E ? E->Half : (To ? To->Half : 120.0f), E ? E->Flags : 0);
	}
	// The stretch from where they stand to the first node keeps the room of the edge they're on (mid-crosswalk, mid-doorway).
	if (W.Route.Num() >= 2 && !From)
	{
		const int32 First = W.Route[1].Node;
		const FStreetCrowdMap::FEdge* E = W.Route[0].Node != INDEX_NONE ? M.EdgeBetween(W.Route[0].Node, First) : M.EdgeBetween(W.Node, First);
		if (!E && W.Route[0].Node == INDEX_NONE)
		{
			E = M.EdgeBetween(W.NextNode, First);
		}
		if (E)
		{
			W.Route[0].Half = E->Half;
			W.Route[0].Flags = E->Flags;
		}
	}
	if (To)
	{
		if (W.Route.Num() > 0)
		{
			W.Route.Last().Half = To->Half;
		}
		for (const FVector2D& P : To->Approach)
		{
			Add(P, INDEX_NONE, To->Half, 0);
		}
	}
	// The first curb or door on the way (not the stretch they're already on).
	for (int32 I = 1; I < W.Route.Num() - 1; ++I)
	{
		if (W.Route[I].Flags & (FStreetCrowdMap::Crossing | FStreetCrowdMap::StoreDoors | FStreetCrowdMap::WashDoor))
		{
			W.Gate = I;
			break;
		}
	}
	W.GatePause = W.Rng.FRandRange(0.6f, 1.2f);
	LayLanes(W);
	if (W.Route.Num() < 2)
	{
		W.bArrived = true;
	}
}

void AStreetCrowd::LayLanes(FStreetWalker& W)
{
	// Each stretch walked a little right of its line (two people meeting pass on their rights), or in the rain without an
	// umbrella over toward the fronts (what shelter the cornices and the bays give); straight on at a doorway, near it
	// indoors, exactly onto a spot. Each corner between two stretches takes both.
	const int32 N = W.Route.Num();
	if (N == 0)
	{
		return;
	}
	TArray<FVector2D, TInlineAllocator<32>> Off;
	Off.SetNumZeroed(N);
	const bool bUnder = !W.bUmbrella && Rain > 0.3f && !W.bSmoking;
	for (int32 I = 0; I + 1 < N; ++I)
	{
		const FCrowdWaypoint& A = W.Route[I];
		const FCrowdWaypoint& B = W.Route[I + 1];
		FVector2D D = B.At - A.At;
		const double Len = D.Size();
		if (Len < 1.0)
		{
			continue;
		}
		D /= Len;
		float Room = FMath::Max(0.0f, A.Half - 42.0f);
		if (A.Flags & (FStreetCrowdMap::StoreDoors | FStreetCrowdMap::WashDoor))
		{
			Room = 0.0f;
		}
		else if (A.Flags & FStreetCrowdMap::Indoors)
		{
			Room = FMath::Min(Room, 12.0f);
		}
		if (bUnder && (A.Flags & FStreetCrowdMap::Sidewalk))
		{
			const FVector2D Side = FrontsSide((A.At + B.At) * 0.5);
			const FVector2D Across = Side - D * FVector2D::DotProduct(Side, D);
			if (!Across.IsNearlyZero())
			{
				Off[I] = Across.GetSafeNormal() * Room;
				continue;
			}
		}
		Off[I] = RightOf(D) * FMath::Min(W.Lane, Room);
	}
	W.Route[0].Walk = W.Pos;
	for (int32 I = 1; I < N; ++I)
	{
		FCrowdWaypoint& P = W.Route[I];
		if (I == N - 1)
		{
			P.Walk = P.At + (W.bStops ? FVector2D::ZeroVector : Off[I - 1]);
		}
		else if (P.Node == INDEX_NONE)
		{
			P.Walk = P.At; // a spot's approach: exactly
		}
		else
		{
			P.Walk = P.At + (Off[I - 1] + Off[I]) * 0.5;
		}
	}
}

// ------------------------------------------------------------------ walking

void AStreetCrowd::Walk(FStreetWalker& W, float Dt)
{
	const FStreetCrowdMap& M = *Map;
	const int32 N = W.Route.Num();
	const float Hurry = 1.0f + (W.bUmbrella ? 0.1f : 0.24f) * FMath::SmoothStep(0.3f, 1.0f, Rain);
	FVector2D Want = Heading(W.Yaw);
	float WantSpeed = 0.0f;
	float FaceYaw = W.Yaw;
	bool bAvoid = true;
	W.bWantsDoors = false;
	if (!W.bArrived && N >= 2)
	{
		// On along the route: to the next stretch once past this one's end (never past a curb or the doors before they clear).
		while (W.RouteAt < N - 2)
		{
			const int32 Next = W.RouteAt + 1;
			if (Next == W.Gate)
			{
				break;
			}
			const FVector2D A = W.Route[W.RouteAt].Walk;
			const FVector2D B = W.Route[Next].Walk;
			const FVector2D AB = B - A;
			const double T = AB.SizeSquared() > 1.0 ? FVector2D::DotProduct(W.Pos - A, AB) / AB.SizeSquared() : 1.0;
			if (T < 1.0 && FVector2D::Distance(W.Pos, B) > 35.0)
			{
				break;
			}
			W.RouteAt = Next;
			if (W.Route[Next].Node != INDEX_NONE)
			{
				W.Node = W.Route[Next].Node;
			}
			W.NextNode = INDEX_NONE;
			for (int32 J = Next + 1; J < N; ++J)
			{
				if (W.Route[J].Node != INDEX_NONE)
				{
					W.NextNode = W.Route[J].Node;
					break;
				}
			}
		}
		const int32 K = W.RouteAt;
		const FVector2D A = W.Route[K].Walk;
		const FVector2D B = W.Route[K + 1].Walk;
		const FVector2D AB = B - A;
		const double Len = AB.Size();
		const double T = Len > 1.0 ? FMath::Clamp(FVector2D::DotProduct(W.Pos - A, AB) / (Len * Len), 0.0, 1.0) : 1.0;
		// Where they must stop next: a curb or the doors before they clear, or the end of a route to a spot.
		const int32 Stop = W.Gate != INDEX_NONE ? W.Gate : (W.bStops ? N - 1 : INDEX_NONE);
		const int32 Last = Stop != INDEX_NONE ? Stop : N - 1;
		double ToLast = (1.0 - T) * Len;
		for (int32 I = K + 1; I < Last; ++I)
		{
			ToLast += FVector2D::Distance(W.Route[I].Walk, W.Route[I + 1].Walk);
		}
		// A point a stride or so ahead along the route (never past the stop) to walk toward.
		double Ahead = FMath::Clamp(static_cast<double>(W.Speed) * 0.8, 70.0, 160.0);
		FVector2D Target = A + AB * T;
		for (int32 I = K; I < Last && Ahead > 0.0; ++I)
		{
			const FVector2D From = I == K ? Target : W.Route[I].Walk;
			const FVector2D To = W.Route[I + 1].Walk;
			const double Step = FVector2D::Distance(From, To);
			if (Step >= Ahead)
			{
				Target = From + (To - From) * (Ahead / FMath::Max(Step, 1.0));
				Ahead = 0.0;
			}
			else
			{
				Target = To;
				Ahead -= Step;
			}
		}
		const FVector2D ToTarget = Target - W.Pos;
		if (ToTarget.SizeSquared() > 1.0)
		{
			Want = ToTarget.GetSafeNormal();
		}
		WantSpeed = W.Pace * Hurry;
		if (Stop != INDEX_NONE)
		{
			// Easing to a stop on the spot (or at the curb).
			WantSpeed = FMath::Min(WantSpeed, FMath::Sqrt(2.0f * 150.0f * FMath::Max(0.0f, static_cast<float>(ToLast) - 3.0f)));
		}
		// The doors open as they come up to them, and stay so while they're in the doorway (the stage opens them for whoever
		// it's told of, NoteWalker; the Wash & Fold's wants a hand on it, OpenWashDoor).
		const uint8 Doors = FStreetCrowdMap::StoreDoors | FStreetCrowdMap::WashDoor;
		const uint8 Here = W.Route[K].Flags;
		if ((Here & Doors) || (W.Gate != INDEX_NONE && (W.Route[W.Gate].Flags & Doors) && ToLast < 160.0))
		{
			W.bWantsDoors = true;
		}
		if (W.Gate != INDEX_NONE && ToLast < 30.0)
		{
			// At a curb or a door.
			WantSpeed = 0.0f;
			const FCrowdWaypoint& G = W.Route[W.Gate];
			const FVector2D Across = W.Route[W.Gate + 1].Walk - G.Walk;
			FaceYaw = YawOf(Across);
			W.GateWait += Dt;
			bool bClear = false;
			if (G.Flags & FStreetCrowdMap::Crossing)
			{
				// A look each way before stepping off.
				W.LookAside = W.GateWait < W.GatePause * 0.5f ? -55.0f : 50.0f;
				bClear = W.GateWait >= W.GatePause && FMath::Abs(FMath::FindDeltaAngleDegrees(W.Yaw, FaceYaw)) < 25.0f;
			}
			else
			{
				const bool bWash = (G.Flags & FStreetCrowdMap::WashDoor) != 0;
				const uint8 Room = bWash ? FStreetCrowdMap::Wash : FStreetCrowdMap::Store;
				const bool bGoingIn = G.Node == (bWash ? M.WashOut : M.StoreOut);
				W.bWantsDoors = true;
				if (bGoingIn && (bWash ? HeroInWash() && W.Role != ECrowdRole::Player : HeroInStore()))
				{
					// The hero's in there: not now (a room's light and its people are dear enough). Dee's players go on through.
					W.bWantsDoors = false;
					SkipRoom(W, Room);
					return Settle(W, Dt, true);
				}
				if (bWash && Stage)
				{
					Stage->OpenWashDoor();
				}
				const float Open = Stage ? (bWash ? Stage->WashDoorOpen() : Stage->DoorsOpen()) : 0.0f;
				if (Open > (bWash ? 0.7f : 0.8f))
				{
					bClear = true;
				}
				else if (W.GateWait > 2.0f)
				{
					// The door doesn't open for them (nobody's running the street's doors): going in, they give it up; coming
					// out, they're through it once nobody's looking.
					if (bGoingIn)
					{
						W.bWantsDoors = false;
						SkipRoom(W, Room);
						return Settle(W, Dt, true);
					}
					const FVector2D Outside = W.Route[W.Gate + 1].At;
					const float OutsideFloor = FloorAt(Outside, 0.0f);
					if (CanVanish(W) && !IsSeen(Outside, OutsideFloor, 180.0f * W.Scale))
					{
						W.Pos = Outside;
						W.Floor = OutsideFloor;
						W.FloorWant = OutsideFloor;
						W.Shown = FVector::ZeroVector;
						bClear = true;
					}
				}
			}
			if (bClear)
			{
				W.LookAside = 0.0f;
				W.GateWait = 0.0f;
				const int32 Was = W.Gate;
				W.Gate = INDEX_NONE;
				for (int32 I = Was + 1; I < N - 1; ++I)
				{
					if (W.Route[I].Flags & (FStreetCrowdMap::Crossing | Doors))
					{
						W.Gate = I;
						break;
					}
				}
				W.GatePause = W.Rng.FRandRange(0.6f, 1.2f);
			}
			bAvoid = false;
		}
		// There.
		const bool bEnd = K == N - 2 && W.Gate == INDEX_NONE;
		if (bEnd && ((W.bStops && ToLast < 10.0 && W.Speed < 40.0f) || (!W.bStops && ToLast < 45.0)))
		{
			W.bArrived = true;
			if (W.Route.Last().Node != INDEX_NONE)
			{
				W.Node = W.Route.Last().Node;
			}
		}
	}
	if (W.bArrived && W.bStops && W.Plan.IsValidIndex(W.LegAt) && Map->Spots.IsValidIndex(W.Plan[W.LegAt].Target))
	{
		// At the spot, facing what they came to.
		WantSpeed = 0.0f;
		FaceYaw = Map->Spots[W.Plan[W.LegAt].Target].Yaw;
		bAvoid = false;
	}
	if (W.bHold)
	{
		WantSpeed = 0.0f;
	}

	// Round whoever and whatever is in the way.
	float Speed = WantSpeed;
	FVector2D Dir = Want;
	if (bAvoid && WantSpeed > 1.0f)
	{
		if (W.Tier >= 2 && W.Frame % 3 != 0)
		{
			// Far off, the last choice of way holds a couple of frames.
			Dir = Turned(Want, W.SteerAngle);
		}
		else
		{
			// Up to a spot (the last stretches, off the map's nodes) or through a doorway, what stands there is where they're
			// going (1812's step, the Wash & Fold's): only people are kept clear of.
			bool bApproach = W.bStops;
			for (int32 I = W.RouteAt + 1; bApproach && I < N; ++I)
			{
				bApproach = W.Route[I].Node == INDEX_NONE;
			}
			const uint8 Here = W.Route.IsValidIndex(W.RouteAt) ? W.Route[W.RouteAt].Flags : 0;
			const bool bDoorway = (Here & (FStreetCrowdMap::StoreDoors | FStreetCrowdMap::WashDoor)) != 0;
			Dir = Steer(W, Want, WantSpeed, Speed, !bApproach && !bDoorway);
		}
		if (W.Tier < 2 || W.Frame % 4 == 0)
		{
			W.ProbeIn -= Dt * (W.Tier < 2 ? 1.0f : 4.0f);
			if (W.ProbeIn <= 0.0f)
			{
				W.ProbeIn = 0.3f;
				Probe(W);
			}
		}
	}
	W.BumpFor = FMath::Max(0.0f, W.BumpFor - Dt);
	// Held up where they meant to walk: a glance at whoever it is (the hero gets a word now and then); stuck long enough,
	// they give up on where they were going.
	if (WantSpeed > 20.0f && Speed < 5.0f && !W.bHold)
	{
		W.Blocked += Dt;
	}
	else
	{
		W.Blocked = FMath::Max(0.0f, W.Blocked - 2.0f * Dt);
	}
	if (const AShortStackCharacter* H = Hero.Get(); H && HeroLevelWith(W.Floor))
	{
		const FVector2D HeroPos = ToStage(H->GetActorLocation());
		const FVector2D Rel = HeroPos - W.Pos;
		const double Dist = Rel.Size();
		const FVector2D Closing = Heading(W.Yaw) * W.Speed - HeroVelocity();
		const bool bBump = Dist < BodyRadius * W.Scale + HeroRadius + 28.0 && FVector2D::DotProduct(Closing, Rel) > 40.0 * Dist;
		if ((bBump || (W.Blocked > 1.2f && Dist < 170.0)) && W.BarkIn <= 0.0f)
		{
			W.LookWho = -2;
			W.LookUntil = Clock + W.Rng.FRandRange(1.0f, 1.8f);
			W.BarkIn = W.Rng.FRandRange(18.0f, 30.0f);
			if (BarkQuiet <= 0.0f && W.Rng.FRand() < 0.5f)
			{
				Bark(W, Rain > 0.4f && W.bUmbrella);
			}
		}
	}
	W.BarkIn -= Dt;
	if (W.Blocked > 9.0f && W.Plan.IsValidIndex(W.LegAt))
	{
		W.Blocked = 0.0f;
		const ECrowdLeg Kind = W.Plan[W.LegAt].Kind;
		if (Kind == ECrowdLeg::Visit || Kind == ECrowdLeg::Smoke)
		{
			NextLeg(W);
		}
		else
		{
			TurnBack(W, Kind == ECrowdLeg::Leave ? W.Plan[W.LegAt].Target : INDEX_NONE);
		}
	}

	// Turning toward the way they go (slowing to turn hard, turning on the spot to go back), then a step along it: the gait
	// only walks forward, so they never move other than the way they face.
	const float TargetYaw = Speed > 1.0f ? YawOf(Dir) : FaceYaw;
	const float Error = FMath::Abs(FMath::FindDeltaAngleDegrees(W.Yaw, TargetYaw));
	W.Yaw = FMath::FixedTurn(W.Yaw, TargetYaw, (W.Speed > 40.0f ? 200.0f : 150.0f) * Dt);
	Speed *= 1.0f - FMath::SmoothStep(35.0f, 110.0f, Error);
	W.Speed = FMath::FInterpConstantTo(W.Speed, Speed, Dt, Speed > W.Speed ? 190.0f : 330.0f);
	FVector2D Next = W.Pos + Heading(W.Yaw) * W.Speed * Dt;
	// Kept to the stretch's room (off the road, out of the fronts and the shelves).
	if (N >= 2 && W.RouteAt + 1 < N)
	{
		const FCrowdWaypoint& A = W.Route[W.RouteAt];
		const FVector2D Line = W.Route[W.RouteAt + 1].At - A.At;
		const double Len = Line.Size();
		if (Len > 1.0)
		{
			const FVector2D D = Line / Len;
			const FVector2D R = RightOf(D);
			const double Side = FVector2D::DotProduct(Next - A.At, R);
			const double Room = FMath::Max(0.0, static_cast<double>(A.Half) - BodyRadius * W.Scale);
			if (FMath::Abs(Side) > Room && FMath::Abs(Side) > FMath::Abs(FVector2D::DotProduct(W.Pos - A.At, R)))
			{
				Next -= R * (Side - FMath::Sign(Side) * Room);
			}
		}
	}
	// Never into the hero: their capsule is the hero's wall, and set down overlapping the hero's it would shove the player
	// aside as they next move (the two capsules as they are: this one's scale, the hero's build).
	if (const AShortStackCharacter* H = Hero.Get())
	{
		const FVector HeroAtNow = H->GetActorLocation();
		const FVector2D HeroPos = ToStage(HeroAtNow);
		const double Min = W.Body->GetCapsuleComponent()->GetScaledCapsuleRadius() + H->GetCapsuleComponent()->GetScaledCapsuleRadius() + 2.0;
		FVector2D Off = Next - HeroPos;
		const double D = Off.Size();
		if (D < Min && HeroLevelWith(W.Floor))
		{
			Off = D > 1.0 ? Off / D : -Heading(W.Yaw);
			Next = HeroPos + Off * Min;
		}
	}
	W.Pos = Next;
	Settle(W, Dt, W.Tier < 2 || W.Frame % 4 == 0);
}

FVector2D AStreetCrowd::Steer(FStreetWalker& W, const FVector2D& Want, float WantSpeed, float& OutSpeed, bool bAvoidStatics) const
{
	// A handful of ways to go (straight on, a little either side, well aside) at full pace and at half, and standing: each
	// scored by how soon it runs into someone or something, how far off the route it turns and how much it slows, a touch
	// for passing on the right and for keeping the last choice. The best is taken.
	struct FBody
	{
		FVector2D P;
		FVector2D V;
		float R;
	};
	TArray<FBody, TInlineAllocator<24>> Near;
	const float Me = BodyRadius * W.Scale + (W.UmbrellaOpen > 0.5f ? 10.0f : 0.0f);
	for (const FStreetWalker& O : Walkers)
	{
		// Not those on another floor (up in the Wash & Fold, behind its wall, from down on the sidewalk).
		if (&O == &W || !O.bOut || FVector2D::DistSquared(O.Pos, W.Pos) > 600.0 * 600.0 || FMath::Abs(O.Floor - W.Floor) > 40.0f)
		{
			continue;
		}
		Near.Add({O.Pos, Heading(O.Yaw) * O.Speed, BodyRadius * O.Scale + (O.UmbrellaOpen > 0.5f ? 10.0f : 0.0f) + 8.0f});
	}
	if (const AShortStackCharacter* H = Hero.Get())
	{
		const FVector2D P = ToStage(H->GetActorLocation());
		if (FVector2D::DistSquared(P, W.Pos) < 700.0 * 700.0 && HeroLevelWith(W.Floor))
		{
			Near.Add({P, HeroVelocity(), HeroRadius + 18.0f});
		}
	}
	if (bAvoidStatics)
	{
		// What stands in the way, by the nearest point of its footprint (it slides along a long one as they pass it).
		for (const FBox2D& Block : Map->Blocks)
		{
			const FVector2D Nearest(FMath::Clamp(W.Pos.X, Block.Min.X, Block.Max.X), FMath::Clamp(W.Pos.Y, Block.Min.Y, Block.Max.Y));
			if (FVector2D::DistSquared(Nearest, W.Pos) < 400.0 * 400.0)
			{
				Near.Add({Nearest, FVector2D::ZeroVector, 6.0f});
			}
		}
		if (W.BumpFor > 0.0f)
		{
			Near.Add({W.Bump, FVector2D::ZeroVector, 26.0f});
		}
	}
	// The stretch's room either side of its line, for the ways that would leave it.
	FVector2D LineA = W.Pos;
	FVector2D LineD = Want;
	double Room = 1e6;
	if (W.Route.IsValidIndex(W.RouteAt + 1))
	{
		const FCrowdWaypoint& A = W.Route[W.RouteAt];
		const FVector2D Line = W.Route[W.RouteAt + 1].At - A.At;
		if (Line.SizeSquared() > 1.0)
		{
			LineA = A.At;
			LineD = Line.GetSafeNormal();
			Room = FMath::Max(0.0, static_cast<double>(A.Half) - Me);
		}
	}
	auto Danger = [&](const FVector2D& V) {
		float Soonest = Horizon;
		for (const FBody& O : Near)
		{
			Soonest = FMath::Min(Soonest, TimeToMeet(O.P - W.Pos, V - O.V, Me + O.R, Horizon));
		}
		const float D = 1.0f - Soonest / Horizon;
		return D * D;
	};
	static const float Angles[] = {0.0f, 18.0f, -18.0f, 36.0f, -36.0f, 58.0f, -58.0f, 85.0f, -85.0f};
	float BestCost = TNumericLimits<float>::Max();
	FVector2D BestDir = Want;
	float BestSpeed = 0.0f;
	float BestAngle = 0.0f;
	for (const float Angle : Angles)
	{
		const FVector2D D = Turned(Want, Angle);
		for (const float Share : {1.0f, 0.5f})
		{
			const float S = WantSpeed * Share;
			float Cost = 0.9f * FMath::Pow(FMath::Abs(Angle) / 85.0f, 1.3f) + 0.4f * (1.0f - Share) + (Angle < 0.0f ? 0.06f : 0.0f) +
						 0.12f * FMath::Abs(Angle - W.SteerAngle) / 85.0f;
			Cost += 2.6f * Danger(D * S);
			const FVector2D Then = W.Pos + D * S * 0.8f;
			const double Out = FMath::Abs(FVector2D::DotProduct(Then - LineA, RightOf(LineD))) - Room;
			if (Out > 0.0)
			{
				Cost += static_cast<float>(Out / 30.0);
			}
			if (Cost < BestCost)
			{
				BestCost = Cost;
				BestDir = D;
				BestSpeed = S;
				BestAngle = Angle;
			}
		}
	}
	// Or stand and let them by.
	const float StandCost = 0.75f + 2.6f * Danger(FVector2D::ZeroVector);
	if (StandCost < BestCost)
	{
		BestDir = Want;
		BestSpeed = 0.0f;
		BestAngle = 0.0f;
	}
	W.SteerAngle = BestAngle;
	OutSpeed = BestSpeed;
	return BestDir;
}

void AStreetCrowd::Probe(FStreetWalker& W)
{
	// Out on the street, walking: a look a step and a half ahead at knee height for anything solid the map doesn't know (a
	// prop set down since it was laid out).
	const uint8 Here = W.Route.IsValidIndex(W.RouteAt) ? W.Route[W.RouteAt].Flags : 0;
	if (W.Speed < 30.0f || !(Here & (FStreetCrowdMap::Sidewalk | FStreetCrowdMap::Crossing)) || !GetWorld())
	{
		return;
	}
	const FVector2D Ahead = Heading(W.Yaw);
	const FVector From = ToWorld(W.Pos + Ahead * 35.0, W.Floor + 55.0);
	const FVector To = ToWorld(W.Pos + Ahead * 150.0, W.Floor + 55.0);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(StreetCrowdProbe), false);
	Query.AddIgnoredActor(W.Body);
	FHitResult Hit;
	if (GetWorld()->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(16.0f), Query) &&
		Hit.ImpactNormal.Z < 0.6)
	{
		W.Bump = ToStage(Hit.ImpactPoint) + Ahead * 12.0;
		W.BumpFor = 2.5f;
	}
}

void AStreetCrowd::Settle(FStreetWalker& W, float Dt, bool bTrace)
{
	AShortStackCharacter* B = W.Body;
	if (!B)
	{
		return;
	}
	// On the floor under them (the sidewalk, the road down off the curb, the store's tiles), stepping between them.
	if (bTrace)
	{
		W.FloorWant = FloorAt(W.Pos, W.FloorWant);
	}
	W.Floor = Dt > 0.0f ? FMath::FInterpTo(W.Floor, W.FloorWant, Dt, 14.0f) : W.FloorWant;
	if (FMath::Abs(W.Floor - W.FloorWant) < 0.05f)
	{
		W.Floor = W.FloorWant;
	}
	const float Half = B->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector At = ToWorld(W.Pos, W.Floor + Half);
	const FQuat Facing = Stage->GetActorQuat() * FQuat(FVector::UpVector, FMath::DegreesToRadians(W.Yaw));
	B->SetActorLocationAndRotation(At, Facing, false, nullptr, ETeleportType::None);
	// The gait reads the ground speed: what they covered, against their size (a shorter body's stride is shorter).
	if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
	{
		FVector V = Dt > 0.0f && !W.Shown.IsZero() ? (At - W.Shown) / Dt : FVector::ZeroVector;
		V.Z = 0.0;
		Move->Velocity = V.GetClampedToMaxSize(450.0) / W.Scale;
	}
	W.Shown = At;
}

// ------------------------------------------------------------------ how they look

void AStreetCrowd::Present(FStreetWalker& W, float Dt)
{
	AShortStackCharacter* B = W.Body;
	if (!B)
	{
		return;
	}
	if (Dt > 0.0f || W.Tier < 0)
	{
		if (W.Tier < 0 || W.Frame % 8 == 0)
		{
			UpdateCost(W, bHasView ? FVector::Dist(ViewAt, ToWorld(W.Pos, W.Floor + 100.0)) : 0.0);
		}
		UpdateLook(W, Dt);
		UpdateProps(W, Dt);
	}
	// Over what the character set for this frame (its tick has run, its body hasn't posed): where the head turns, and the
	// right hand up under an open umbrella.
	if (UStreetBodyAnim* Anim = Cast<UStreetBodyAnim>(B->GetMesh()->GetAnimInstance()))
	{
		Anim->Pose.LookYaw = W.LookYaw;
		Anim->Pose.LookPitch = W.LookPitch;
		if (W.UmbrellaOpen > 0.0f)
		{
			const float Open = FMath::SmoothStep(0.0f, 1.0f, W.UmbrellaOpen);
			Anim->Pose.Sip = FMath::Max(Anim->Pose.Sip, FMath::Clamp(CVarUmbrellaHold.GetValueOnGameThread(), 0.0f, 1.0f) * Open);
		}
	}
	if (UBackRoomFaceAnim* Face = B->Face ? Cast<UBackRoomFaceAnim>(B->Face->GetAnimInstance()) : nullptr)
	{
		// The face's rig runs only near enough to read an expression; the eyes go where the head does.
		Face->bExpressionless = W.Tier >= 1;
		FVector Look = FVector::ZeroVector;
		if (W.Tier < 1)
		{
			if (W.LookWho == -2 && Hero.IsValid())
			{
				Look = Hero->EyeLocation();
			}
			else if (Walkers.IsValidIndex(W.LookWho) && Walkers[W.LookWho].bOut && Walkers[W.LookWho].Body)
			{
				Look = Walkers[W.LookWho].Body->EyeLocation();
			}
			else if (!W.LookAt.IsZero())
			{
				Look = ToWorld(FVector2D(W.LookAt.X, W.LookAt.Y), W.LookAt.Z);
			}
		}
		Face->LookAt = Look.IsZero() ? FVector::ZeroVector : B->Face->GetComponentTransform().InverseTransformPosition(Look);
	}
}

void AStreetCrowd::UpdateLook(FStreetWalker& W, float Dt)
{
	AShortStackCharacter* B = W.Body;
	const FStreetCrowdMap& M = *Map;
	const FCrowdLeg* Leg = W.Plan.IsValidIndex(W.LegAt) ? &W.Plan[W.LegAt] : nullptr;
	const bool bAtSpot = Leg && W.bArrived && W.bStops && M.Spots.IsValidIndex(Leg->Target);
	W.HeroGlanceIn -= Dt;
	if (Clock >= W.LookUntil)
	{
		W.LookAt = FVector::ZeroVector;
		W.LookWho = INDEX_NONE;
	}
	if (Clock >= W.NextLook)
	{
		// What next: the hero coming the other way (a glance, not a stare), someone passing, a lit window going by, off to
		// the side; at a spot, what they came to; mostly just ahead.
		W.NextLook = Clock + W.Rng.FRandRange(2.5f, 6.0f);
		const FVector2D Facing = Heading(W.Yaw);
		const float Roll = W.Rng.FRand();
		bool bChosen = false;
		if (const AShortStackCharacter* H = Hero.Get())
		{
			const FVector2D Rel = ToStage(H->GetActorLocation()) - W.Pos;
			const double Dist = Rel.Size();
			if (Dist < 700.0 && Dist > 60.0 && FVector2D::DotProduct(Rel / Dist, Facing) > 0.25 && W.HeroGlanceIn <= 0.0f && Roll < (bAtSpot ? 0.25f : 0.4f))
			{
				W.LookWho = -2;
				W.LookUntil = Clock + W.Rng.FRandRange(0.8f, 1.8f);
				W.HeroGlanceIn = W.Rng.FRandRange(9.0f, 16.0f);
				bChosen = true;
			}
		}
		if (!bChosen && bAtSpot)
		{
			const FVector At = M.Spots[Leg->Target].LookAt;
			W.LookAt = At + FVector(W.Rng.FRandRange(-30.0f, 30.0f), W.Rng.FRandRange(-30.0f, 30.0f), W.Rng.FRandRange(-20.0f, 15.0f));
			W.LookUntil = W.NextLook;
			bChosen = true;
		}
		if (!bChosen && Roll < 0.55f)
		{
			for (int32 I = 0; I < Walkers.Num() && !bChosen; ++I)
			{
				const FStreetWalker& O = Walkers[I];
				const FVector2D Rel = O.Pos - W.Pos;
				const double Dist = Rel.Size();
				if (&O != &W && O.bOut && Dist < 500.0 && Dist > 60.0 && FVector2D::DotProduct(Rel / Dist, Facing) > 0.3 && W.Rng.FRand() < 0.35f)
				{
					W.LookWho = I;
					W.LookUntil = Clock + W.Rng.FRandRange(0.6f, 1.4f);
					bChosen = true;
				}
			}
			for (const int32 Spot : M.Windows)
			{
				const FVector2D Rel = M.SpotAt(Spot) - W.Pos;
				const double Dist = Rel.Size();
				if (!bChosen && Dist < 450.0 && FVector2D::DotProduct(Rel / FMath::Max(Dist, 1.0), Facing) > -0.2 && W.Rng.FRand() < 0.5f)
				{
					W.LookAt = M.Spots[Spot].LookAt;
					W.LookUntil = Clock + W.Rng.FRandRange(1.0f, 2.2f);
					bChosen = true;
				}
			}
		}
		if (!bChosen && Roll > 0.82f)
		{
			// Off to one side or down (a puddle, a phone) for a moment.
			W.LookAside = W.Rng.FRandRange(-35.0f, 35.0f);
			W.LookUntil = Clock + W.Rng.FRandRange(0.8f, 1.8f);
		}
		else if (W.Gate == INDEX_NONE)
		{
			W.LookAside = 0.0f;
		}
	}
	// Where that puts the head (relative to the body; + right, + up), eased as heads turn: quicker to something, slower back.
	float WantYaw = 5.0f * FMath::Sin(Clock * 0.13f + W.Scale * 40.0f) + 3.0f * FMath::Sin(Clock * 0.41f + W.Pace);
	float WantPitch = -4.0f - W.LookDown - (W.bUmbrella ? 0.0f : 6.0f * FMath::SmoothStep(0.3f, 1.0f, Rain));
	FVector Target = FVector::ZeroVector;
	if (W.LookWho == -2 && Hero.IsValid())
	{
		Target = Hero->EyeLocation();
	}
	else if (Walkers.IsValidIndex(W.LookWho) && Walkers[W.LookWho].bOut && Walkers[W.LookWho].Body)
	{
		Target = Walkers[W.LookWho].Body->EyeLocation();
	}
	else if (!W.LookAt.IsZero())
	{
		Target = ToWorld(FVector2D(W.LookAt.X, W.LookAt.Y), W.LookAt.Z);
	}
	if (!Target.IsZero())
	{
		const FRotator To = (Target - B->EyeLocation()).Rotation();
		const float Yaw = FMath::FindDeltaAngleDegrees(static_cast<float>(B->GetActorRotation().Yaw), static_cast<float>(To.Yaw));
		if (FMath::Abs(Yaw) < 100.0f)
		{
			WantYaw = Yaw;
			WantPitch = static_cast<float>(FRotator::NormalizeAxis(To.Pitch));
		}
		else
		{
			// Gone round behind them: nobody wrings their neck for it.
			W.LookAt = FVector::ZeroVector;
			W.LookWho = INDEX_NONE;
		}
	}
	else
	{
		WantYaw += W.LookAside;
	}
	const float Alpha = 1.0f - FMath::Exp(-(Target.IsZero() ? 3.0f : 5.5f) * Dt);
	W.LookYaw = FMath::Lerp(W.LookYaw, WantYaw, Alpha);
	W.LookPitch = FMath::Lerp(W.LookPitch, WantPitch, Alpha);
}

void AStreetCrowd::UpdateProps(FStreetWalker& W, float Dt)
{
	AShortStackCharacter* B = W.Body;
	const FCrowdLeg* Leg = W.Plan.IsValidIndex(W.LegAt) ? &W.Plan[W.LegAt] : nullptr;
	const uint8 Here = W.Route.IsValidIndex(W.RouteAt) ? W.Route[W.RouteAt].Flags : 0;
	const bool bAtDoor = (Leg && Leg->Kind == ECrowdLeg::Enter && W.bArrived) ||
						 (W.Route.IsValidIndex(W.Gate) && W.GateWait > 0.0f && (W.Route[W.Gate].Flags & (FStreetCrowdMap::StoreDoors | FStreetCrowdMap::WashDoor)));
	const FVector Body = ToWorld(W.Pos, W.Floor + 50.0);
	const bool bIndoors = (Here & (FStreetCrowdMap::Indoors | FStreetCrowdMap::StoreDoors | FStreetCrowdMap::WashDoor)) || (Stage && (Stage->IsInsideStore(Body) || Stage->IsInsideWash(Body)));
	// Hunched against the rain without an umbrella, straightening once in out of it (or the rain lets up), slowly.
	const float Hunch = FMath::FInterpConstantTo(W.Hunch, HunchIn(Rain, W.bUmbrella || W.bSmoking || bIndoors), Dt, 0.4f);
	if (Hunch != W.Hunch)
	{
		W.Hunch = Hunch;
		B->SetTired(Hunch);
	}
	// The umbrella: open over them out in the rain, folded going in at a door (the hand comes down with it), put away once
	// it's been dry a while.
	if (USceneComponent* U = W.Umbrella)
	{
		W.DryFor = Rain < 0.08f ? W.DryFor + Dt : 0.0f;
		if (W.bUmbrella && W.DryFor > 25.0f && W.UmbrellaOpen <= 0.0f)
		{
			W.bUmbrella = false;
		}
		const bool bOpen = W.bUmbrella && !bAtDoor && !bIndoors && W.DryFor <= 25.0f;
		W.UmbrellaOpen = FMath::FInterpConstantTo(W.UmbrellaOpen, bOpen ? 1.0f : 0.0f, Dt, 1.6f);
		const bool bShow = W.UmbrellaOpen > 0.02f;
		const bool bWasShown = U->IsVisible();
		if (bWasShown != bShow)
		{
			U->SetVisibility(bShow, true);
		}
		if (bShow)
		{
			// Folding, the canopy closes in on the shaft.
			const float Open = FMath::SmoothStep(0.0f, 1.0f, W.UmbrellaOpen);
			const float Across = FMath::Lerp(0.12f, 1.0f, Open);
			U->SetRelativeScale3D(FVector(Across, Across, FMath::Lerp(0.85f, 1.0f, Open)));
			// Upright over the head: from the hand toward a point over the crown, a little into the walk, never far off
			// upright. Its turn is its own (the hand's swing would rock it), eased.
			const FVector Up = Stage ? Stage->GetActorUpVector() : FVector::UpVector;
			const FVector Grip = U->GetComponentLocation();
			const FVector Crown = B->GetMesh()->GetSocketLocation(HeadBone) + Up * (CVarUmbrellaOver.GetValueOnGameThread() * W.Scale);
			const FVector Fwd = B->GetActorForwardVector();
			FVector Shaft = (Crown - Grip).GetSafeNormal() + Fwd * (0.12 * FMath::Clamp(W.Speed / 150.0f, 0.0f, 1.5f));
			Shaft = Shaft.GetSafeNormal();
			const double MaxTilt = FMath::DegreesToRadians(26.0);
			const double Cos = FVector::DotProduct(Shaft, Up);
			if (Shaft.IsNearlyZero() || Cos < FMath::Cos(MaxTilt))
			{
				const FVector Side = (Shaft - Up * Cos).GetSafeNormal();
				Shaft = Side.IsNearlyZero() ? Up : Up * FMath::Cos(MaxTilt) + Side * FMath::Sin(MaxTilt);
			}
			const FQuat Want = FRotationMatrix::MakeFromZX(Shaft, Fwd).ToQuat();
			// Just opened (or out again after a while away): straight to it.
			const bool bSnap = !bWasShown || W.UmbrellaTurn.Equals(FQuat::Identity);
			W.UmbrellaTurn = bSnap ? Want : FQuat::Slerp(W.UmbrellaTurn, Want, 1.0f - FMath::Exp(-9.0f * Dt));
			U->SetWorldRotation(W.UmbrellaTurn);
		}
	}
	// The cigarette, lit: a drag now and then (the hand to the mouth, the ember bright a moment).
	const bool bLit = W.bSmoking && W.Cigarette && W.Ember;
	if (W.Cigarette && W.Cigarette->IsVisible() != bLit)
	{
		W.Cigarette->SetVisibility(bLit);
		W.Ember->SetVisibility(bLit);
	}
	if (bLit)
	{
		if (Leg && Leg->Kind == ECrowdLeg::Smoke && W.bArrived && Clock >= W.NextDrag)
		{
			B->Sip();
			W.DragAt = Clock;
			W.NextDrag = Clock + W.Rng.FRandRange(6.0f, 14.0f);
		}
		const float Since = Clock - W.DragAt;
		const float Draw = FMath::SmoothStep(0.45f, 0.85f, Since) * (1.0f - FMath::SmoothStep(1.2f, 2.2f, Since));
		if (W.EmberGlow)
		{
			W.EmberGlow->SetScalarParameterValue(TEXT("Emissive"), 4.0f + 1.5f * FMath::Sin(Clock * 7.3f + W.Pace) + 46.0f * Draw);
		}
	}
}

void AStreetCrowd::UpdateCost(FStreetWalker& W, double Dist)
{
	// Near, everything. Past 12 m the face's rig rests (nobody reads an expression that far off); past 25 m lower LODs and the
	// body posed 20 times a second (the face still follows it every frame, so it never lags the neck); past 30 m no shadows.
	// Each with a little room either way, so nobody flickers between two.
	AShortStackCharacter* B = W.Body;
	const double Near = 1200.0;
	const double Far = 2500.0;
	const double Room = 150.0;
	int32 Tier = W.Tier;
	if (Tier < 0)
	{
		Tier = Dist < Near ? 0 : (Dist < Far ? 1 : 2);
	}
	else if (Tier == 0 && Dist > Near + Room)
	{
		Tier = Dist < Far ? 1 : 2;
	}
	else if (Tier == 1)
	{
		Tier = Dist < Near - Room ? 0 : (Dist > Far + Room ? 2 : 1);
	}
	else if (Tier == 2 && Dist < Far - Room)
	{
		Tier = Dist < Near ? 0 : 1;
	}
	const bool bShadows = Dist < (W.bShadows ? 3000.0 + Room : 3000.0 - Room);
	TInlineComponentArray<UPrimitiveComponent*> Parts(B);
	if (Tier != W.Tier)
	{
		W.Tier = Tier;
		for (UPrimitiveComponent* P : Parts)
		{
			if (USkinnedMeshComponent* Skin = Cast<USkinnedMeshComponent>(P))
			{
				const bool bFace = Skin == B->Face.Get();
				// Forced LODs count from 1 (0 lets the engine choose), and stop at what the mesh has.
				Skin->SetForcedLOD(Tier < 2 ? 0 : (bFace ? 5 : 3));
				if (Skin == B->GetMesh())
				{
					Skin->SetComponentTickInterval(Tier < 2 ? 0.0f : 1.0f / 20.0f);
				}
			}
		}
	}
	if (bShadows != W.bShadows)
	{
		W.bShadows = bShadows;
		for (UPrimitiveComponent* P : Parts)
		{
			if (P->IsA<USkinnedMeshComponent>() || P->IsA<UGroomComponent>() || (W.Umbrella && P->IsAttachedTo(W.Umbrella)))
			{
				P->SetCastShadow(bShadows);
			}
		}
	}
}

void AStreetCrowd::Bark(FStreetWalker& W, bool bRain)
{
	const TCHAR* Line = bRain ? SorryInRain[W.Rng.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(SorryInRain)) - 1)] : Sorry[W.Rng.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Sorry)) - 1)];
	if (Barks.Num() >= 3)
	{
		Barks.RemoveAt(0);
	}
	Barks.Emplace(W.Body ? W.Body->EyeLocation() : ToWorld(W.Pos, W.Floor + 160.0), FString(Line));
	BarkQuiet = 8.0f;
}

// ------------------------------------------------------------------ what the camera sees

void AStreetCrowd::UpdateView()
{
	bHasView = false;
	bDark = false;
	bFading = false;
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return;
	}
	bHasView = true;
	ViewAt = Cam->GetCameraLocation();
	const FRotationMatrix R(Cam->GetCameraRotation());
	ViewAxes[0] = R.GetUnitAxis(EAxis::X);
	ViewAxes[1] = R.GetUnitAxis(EAxis::Y);
	ViewAxes[2] = R.GetUnitAxis(EAxis::Z);
	double Aspect = 16.0 / 9.0;
	if (UGameViewportClient* Viewport = World->GetGameViewport())
	{
		FVector2D Size;
		Viewport->GetViewportSize(Size);
		if (Size.X > 0.0 && Size.Y > 0.0)
		{
			Aspect = Size.X / Size.Y;
		}
	}
	// The frame's half-widths as slopes, with a margin round it (the camera turns; a step taken just outside the frame shows).
	const double Half = FMath::DegreesToRadians(FMath::Clamp(static_cast<double>(Cam->GetFOVAngle()), 20.0, 170.0) * 0.5);
	ViewTanX = FMath::Tan(Half) * 1.15 + 0.08;
	ViewTanY = FMath::Tan(Half) / Aspect * 1.15 + 0.08;
	// The street fading in from the door (not out, going home: nobody's dressed for a street being left), and dark enough,
	// either way, that nothing can be seen.
	bFading = Cam->bEnableFading && Cam->FadeAlpha.X > Cam->FadeAlpha.Y && Cam->FadeAmount > 0.05f;
	bDark = Cam->bEnableFading && Cam->FadeAmount > 0.85f;
}

bool AStreetCrowd::IsSeen(const FVector2D& At, float Floor, float Height) const
{
	if (!bHasView || bDark)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	for (const float Up : {15.0f, Height * 0.55f, Height})
	{
		const FVector P = ToWorld(At, Floor + Up);
		const FVector To = P - ViewAt;
		const double Dist = To.Size();
		if (Dist < 250.0)
		{
			return true; // beside the camera, in the frame or not
		}
		if (Dist > SeenRange)
		{
			continue;
		}
		const double Ahead = FVector::DotProduct(To, ViewAxes[0]);
		if (Ahead <= 0.0 || FMath::Abs(FVector::DotProduct(To, ViewAxes[1])) > Ahead * ViewTanX || FMath::Abs(FVector::DotProduct(To, ViewAxes[2])) > Ahead * ViewTanY)
		{
			continue;
		}
		// In the frame: seen, unless something solid and drawn stands between (the invisible blockers, the store's glass among
		// them, hide nothing).
		FCollisionQueryParams Query(SCENE_QUERY_STAT(StreetCrowdSight), false);
		bool bBlocked = false;
		for (int32 Try = 0; Try < 4; ++Try)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, ViewAt, P, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
			{
				break;
			}
			const UPrimitiveComponent* C = Hit.GetComponent();
			if (C && C->IsVisible())
			{
				bBlocked = true;
				break;
			}
			Query.AddIgnoredComponent(C);
		}
		if (!bBlocked)
		{
			return true;
		}
	}
	return false;
}

bool AStreetCrowd::CanVanish(const FStreetWalker& W) const
{
	// Not drawn of late (shadows count), and not where the camera would see them by the frame and the walls either.
	return W.Body && !W.Body->WasRecentlyRendered(0.3f) && !IsSeen(W.Pos, W.Floor, 180.0f * W.Scale);
}

// ------------------------------------------------------------------ the stage's frame

FVector AStreetCrowd::ToWorld(const FVector2D& At, double Z) const
{
	return Stage ? Stage->GetActorTransform().TransformPosition(FVector(At.X, At.Y, Z)) : FVector(At.X, At.Y, Z);
}

FVector2D AStreetCrowd::ToStage(const FVector& World) const
{
	const FVector L = Stage ? Stage->GetActorTransform().InverseTransformPosition(World) : World;
	return FVector2D(L.X, L.Y);
}

FVector AStreetCrowd::HeroAt() const
{
	const AShortStackCharacter* H = Hero.Get();
	return H ? H->GetActorLocation() : FVector(0.0, 0.0, -100000.0);
}

FVector2D AStreetCrowd::HeroVelocity() const
{
	const AShortStackCharacter* H = Hero.Get();
	if (!H || !Stage)
	{
		return FVector2D::ZeroVector;
	}
	const FVector V = Stage->GetActorTransform().InverseTransformVectorNoScale(H->GetVelocity());
	return FVector2D(V.X, V.Y);
}

bool AStreetCrowd::HeroLevelWith(float Floor) const
{
	const AShortStackCharacter* H = Hero.Get();
	if (!H || !Stage)
	{
		return false;
	}
	const double Feet = Stage->GetActorTransform().InverseTransformPosition(H->GetActorLocation()).Z - H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return FMath::Abs(Feet - Floor) < 40.0;
}

bool AStreetCrowd::HeroInStore() const
{
	return Stage && Hero.IsValid() && Stage->IsInsideStore(HeroAt());
}

bool AStreetCrowd::HeroInWash() const
{
	return Stage && Hero.IsValid() && Stage->IsInsideWash(HeroAt());
}

float AStreetCrowd::FloorAt(const FVector2D& At, float Fallback) const
{
	const UWorld* World = GetWorld();
	if (!World || !Stage)
	{
		return Fallback;
	}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(StreetCrowdFloor), false);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, ToWorld(At, 120.0), ToWorld(At, -250.0), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
	{
		return static_cast<float>(Stage->GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).Z);
	}
	return Fallback;
}

AShortStackCharacter* AStreetCrowd::GetHero() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}

// ------------------------------------------------------------------ each frame

void AStreetCrowd::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Stage || !Map || !GetWorld())
	{
		return;
	}
	if (!Hero.IsValid())
	{
		// The hero's pawn was replaced (put back at the door after falling off the set): look for the new one.
		Hero = GetHero();
	}
	UpdateView();
	if (bFadeHeld)
	{
		// The last frame dressed someone as the street faded in: a long one, and this frame's time. The camera takes it off
		// the fade once everyone has ticked, so it's given back first: the street still comes up from black at the fade's
		// own pace, with everyone in place, rather than all at once after the hitch.
		bFadeHeld = false;
		const APlayerController* Pc = GetWorld()->GetFirstPlayerController();
		if (APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr; Cam && Cam->bEnableFading && Cam->FadeTime > 0.0f && Cam->FadeAlpha.X > Cam->FadeAlpha.Y)
		{
			Cam->FadeTimeRemaining = FMath::Min(Cam->FadeTime, Cam->FadeTimeRemaining + GetWorld()->GetDeltaSeconds());
		}
	}
	if (bPaused != bFrozen)
	{
		Freeze(bPaused);
	}
	const float Dt = bPaused ? 0.0f : FMath::Min(DeltaSeconds, 0.05f);
	if (!bPaused && bHourKnown)
	{
		Clock += Dt;
		BarkQuiet -= Dt;
		UpdateCount(Dt);
		for (FStreetWalker& W : Walkers)
		{
			if (W.bOut && W.Body)
			{
				++W.Frame;
				RunLeg(W, Dt);
				if (W.bOut)
				{
					Walk(W, Dt);
				}
			}
		}
	}
	// The rooms' doors open for whoever's coming through them, and don't shut on anyone in the doorway (the stage hears of
	// them each frame, paused or not: a door frozen open stays so).
	const FStreetCrowdMap& M = *Map;
	for (const FStreetWalker& W : Walkers)
	{
		bool bAtDoor = W.bOut && W.bWantsDoors;
		for (int32 I = W.RouteAt; W.bOut && !bAtDoor && I <= W.RouteAt + 1 && I < W.Route.Num(); ++I)
		{
			const int32 Node = W.Route[I].Node;
			bAtDoor = Node != INDEX_NONE && (Node == M.StoreOut || Node == M.StoreIn || Node == M.WashOut || Node == M.WashIn);
		}
		if (bAtDoor)
		{
			Stage->NoteWalker(ToWorld(W.Pos, W.Floor + 90.0));
		}
	}
	for (FStreetWalker& W : Walkers)
	{
		if (W.bOut && W.Body)
		{
			Present(W, Dt);
		}
	}
	DrawDebug();
}

void AStreetCrowd::EndPlay(const EEndPlayReason::Type Reason)
{
	// The crowd taken off the street takes its people with it (a level closing takes them all by itself).
	if (Reason == EEndPlayReason::Destroyed)
	{
		for (FStreetWalker& W : Walkers)
		{
			if (W.Body)
			{
				W.Body->Destroy();
			}
		}
	}
	Walkers.Reset();
	Super::EndPlay(Reason);
}

// ------------------------------------------------------------------ checks

FString AStreetCrowd::Describe() const
{
	static const TCHAR* const Roles[] = {TEXT("stroll"), TEXT("errand"), TEXT("home"), TEXT("laundry"), TEXT("Dee's game"), TEXT("out of Dee's game"), TEXT("smoke"), TEXT("bus")};
	static const TCHAR* const Legs[] = {TEXT("go"), TEXT("visit"), TEXT("enter"), TEXT("leave"), TEXT("smoke")};
	FString Out = FString::Printf(TEXT("Crowd: %d out of %d wanted (%d of the cast), %s, rain %.2f, daylight %.2f%s"), NumOut(), Wanted, NumCast,
		UTF8_TO_TCHAR(ss::net::TimeLabel(WorldMinutes).c_str()), Rain, Daylight, bPaused ? TEXT(", paused") : TEXT(""));
	for (const FStreetWalker& W : Walkers)
	{
		if (!W.bOut)
		{
			continue;
		}
		const FCrowdLeg* Leg = W.Plan.IsValidIndex(W.LegAt) ? &W.Plan[W.LegAt] : nullptr;
		const TCHAR* Where = TEXT("");
		if (Leg && Map && (Leg->Kind == ECrowdLeg::Visit || Leg->Kind == ECrowdLeg::Enter || Leg->Kind == ECrowdLeg::Smoke) && Map->Spots.IsValidIndex(Leg->Target))
		{
			Where = Map->Spots[Leg->Target].Name;
		}
		Out += FString::Printf(TEXT("\n  %s (%s): %s %d/%d %s%s at (%.0f, %.0f) %.0f cm/s, tier %d%s%s%s"), *W.Cast.ToString(), Roles[static_cast<int32>(W.Role)],
			Leg ? Legs[static_cast<int32>(Leg->Kind)] : TEXT("-"), W.LegAt + 1, W.Plan.Num(), Where, W.bArrived ? TEXT(" (there)") : TEXT(""), W.Pos.X, W.Pos.Y, W.Speed, W.Tier,
			W.bUmbrella ? TEXT(", umbrella") : TEXT(""), W.bSmoking ? TEXT(", smoking") : TEXT(""), W.bSent ? TEXT(", sent off") : TEXT(""));
	}
	return Out;
}

void AStreetCrowd::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (CVarDebug.GetValueOnGameThread() <= 0 || !Stage)
	{
		return;
	}
	const UWorld* World = GetWorld();
	for (const FStreetWalker& W : Walkers)
	{
		if (!W.bOut)
		{
			continue;
		}
		for (int32 I = W.RouteAt; I + 1 < W.Route.Num(); ++I)
		{
			const FColor Color = (W.Route[I].Flags & FStreetCrowdMap::Crossing) ? FColor::Yellow : ((W.Route[I].Flags & FStreetCrowdMap::StoreDoors) ? FColor::Cyan : FColor::Green);
			DrawDebugLine(World, ToWorld(W.Route[I].Walk, W.Floor + 5.0), ToWorld(W.Route[I + 1].Walk, W.Floor + 5.0), Color, false, -1.0f, 0, 1.5f);
		}
		if (W.Route.IsValidIndex(W.Gate))
		{
			DrawDebugSphere(World, ToWorld(W.Route[W.Gate].Walk, W.Floor + 10.0), 12.0f, 8, FColor::Orange, false, -1.0f);
		}
		const FCrowdLeg* Leg = W.Plan.IsValidIndex(W.LegAt) ? &W.Plan[W.LegAt] : nullptr;
		DrawDebugString(World, ToWorld(W.Pos, W.Floor + 205.0 * W.Scale),
			FString::Printf(TEXT("%s %d/%d%s%s"), *W.Cast.ToString(), W.LegAt + 1, W.Plan.Num(), W.bArrived ? TEXT(" there") : TEXT(""), W.Blocked > 0.5f ? TEXT(" held") : TEXT("")),
			nullptr, Leg && Leg->Kind == ECrowdLeg::Leave ? FColor::Red : FColor::White, 0.0f, true, 1.0f);
	}
#endif
}

namespace StreetCrowdDetail
{
FAutoConsoleCommandWithWorld CmdCrowdDescribe(TEXT("ss.Crowd.Describe"), TEXT("Logs Fifth Street's people: how many are out and wanted, and each one's plan."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) {
		if (!World)
		{
			return;
		}
		for (TActorIterator<AStreetCrowd> It(World); It; ++It)
		{
			UE_LOG(LogStreetCrowd, Log, TEXT("%s"), *It->Describe());
		}
	}));
} // namespace StreetCrowdDetail
