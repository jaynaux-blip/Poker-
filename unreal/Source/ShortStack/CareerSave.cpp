#include "CareerSave.h"

#include "Compression/OodleDataCompression.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NightOneSaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogCareerSave, Log, All);

namespace CareerSaveDetail
{
// File layout: magic, version, flags, the text's size, the packed size, then the Oodle stream.
const uint8 Magic[8] = {'S', 'S', 'C', 'A', 'R', 'E', 'E', 'R'};
const uint32 Version = 1;
const int64 HeaderSize = 8 + 4 + 4 + 8 + 8;

const FString& Dir()
{
	static const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
	return Path;
}

FString FilePath()
{
	return FPaths::Combine(Dir(), TEXT("NightOne.ssave"));
}

FString LegacyPath()
{
	return FPaths::Combine(Dir(), FString(UNightOneSaveGame::SlotName()) + TEXT(".sav"));
}

void Put(TArray<uint8>& Out, const void* Data, int64 Size)
{
	Out.Append(static_cast<const uint8*>(Data), Size);
}

bool Unpack(const TArray<uint8>& Bytes, std::string& OutText)
{
	if (Bytes.Num() < HeaderSize || FMemory::Memcmp(Bytes.GetData(), Magic, 8) != 0)
	{
		return false;
	}
	uint32 FileVersion = 0;
	int64 Raw = 0;
	int64 Packed = 0;
	FMemory::Memcpy(&FileVersion, Bytes.GetData() + 8, 4);
	FMemory::Memcpy(&Raw, Bytes.GetData() + 16, 8);
	FMemory::Memcpy(&Packed, Bytes.GetData() + 24, 8);
	if (FileVersion != Version || Raw < 0 || Raw > (int64(1) << 31) || Packed < 0 || HeaderSize + Packed > Bytes.Num())
	{
		return false;
	}
	OutText.resize(static_cast<size_t>(Raw));
	return Raw == 0 || FOodleDataCompression::Decompress(OutText.data(), Raw, Bytes.GetData() + HeaderSize, Packed);
}
} // namespace CareerSaveDetail

using namespace CareerSaveDetail;

bool CareerSave::LoadText(std::string& OutText)
{
	OutText.clear();
	TArray<uint8> Bytes;
	if (FFileHelper::LoadFileToArray(Bytes, *FilePath(), FILEREAD_Silent))
	{
		if (Unpack(Bytes, OutText))
		{
			return true;
		}
		UE_LOG(LogCareerSave, Warning, TEXT("%s is damaged; trying the older save."), *FilePath());
	}
	// A career from before the compressed format.
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SlotName(), 0))
	{
		if (const UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SlotName(), 0)))
		{
			OutText = std::string(TCHAR_TO_UTF8(*Obj->Data));
			return !OutText.empty();
		}
	}
	return false;
}

bool CareerSave::Exists()
{
	return IFileManager::Get().FileExists(*FilePath()) || UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SlotName(), 0);
}

bool CareerSave::SaveNow(const std::string& Text, bool bQuick)
{
	const double T0 = FPlatformTime::Seconds();
	const int64 Raw = static_cast<int64>(Text.size());
	TArray<uint8> Bytes;
	Bytes.Reserve(HeaderSize + FOodleDataCompression::CompressedBufferSizeNeeded(Raw));
	Put(Bytes, Magic, 8);
	const uint32 Flags = 0;
	Put(Bytes, &Version, 4);
	Put(Bytes, &Flags, 4);
	Put(Bytes, &Raw, 8);
	const int64 PackedAt = Bytes.Num();
	int64 Packed = 0;
	Put(Bytes, &Packed, 8);
	Bytes.AddUninitialized(FOodleDataCompression::CompressedBufferSizeNeeded(Raw));
	Packed = Raw == 0 ? 0
		: FOodleDataCompression::Compress(Bytes.GetData() + HeaderSize, Bytes.Num() - HeaderSize, Text.data(), Raw, bQuick ? FOodleDataCompression::ECompressor::Mermaid : FOodleDataCompression::ECompressor::Kraken,
			bQuick ? FOodleDataCompression::ECompressionLevel::SuperFast : FOodleDataCompression::ECompressionLevel::Fast);
	if (Raw > 0 && Packed <= 0)
	{
		UE_LOG(LogCareerSave, Error, TEXT("Couldn't compress the career save (%lld bytes)."), Raw);
		return false;
	}
	FMemory::Memcpy(Bytes.GetData() + PackedAt, &Packed, 8);
	Bytes.SetNum(HeaderSize + Packed, EAllowShrinking::No);
	const double T1 = FPlatformTime::Seconds();

	// Whole, then into place: a crash mid-write leaves the last good save.
	const FString Final = FilePath();
	const FString Temp = Final + TEXT(".tmp");
	if (!FFileHelper::SaveArrayToFile(Bytes, *Temp) || !IFileManager::Get().Move(*Final, *Temp, true, true))
	{
		UE_LOG(LogCareerSave, Error, TEXT("Couldn't write %s."), *Final);
		return false;
	}
	// The career has moved to the new file: the old slot stays as a backup under another name.
	const FString Legacy = LegacyPath();
	if (IFileManager::Get().FileExists(*Legacy))
	{
		IFileManager::Get().Move(*(Legacy + TEXT(".migrated")), *Legacy, true, true);
	}
	UE_LOG(LogCareerSave, Log, TEXT("Career saved: %lld KB of text in %lld KB (compressed in %.1f ms, written in %.1f ms)."), Raw / 1024, Bytes.Num() / 1024, (T1 - T0) * 1000.0,
		(FPlatformTime::Seconds() - T1) * 1000.0);
	return true;
}

std::string CareerSave::Serialize(ss::SaveData& Data)
{
	if (Data.WorldSnapshot && Data.WorldText.empty())
	{
		Data.WorldSnapshot->Write(Data.WorldText);
	}
	return Data.Serialize();
}

// ------------------------------------------------------------------ in the background

FCareerSaver::~FCareerSaver()
{
	Flush();
}

void FCareerSaver::Submit(const ss::SaveData& Data)
{
	if (IsBusy())
	{
		// The newest save is the one that matters: it waits for the worker.
		Waiting = Data;
		return;
	}
	Start(ss::SaveData(Data));
}

void FCareerSaver::Tick()
{
	if (Waiting.IsSet() && !IsBusy())
	{
		ss::SaveData Next = MoveTemp(Waiting.GetValue());
		Waiting.Reset();
		Start(MoveTemp(Next));
	}
}

void FCareerSaver::Flush()
{
	if (InFlight.IsValid())
	{
		InFlight.Wait();
		InFlight = UE::Tasks::FTask();
	}
	if (Waiting.IsSet())
	{
		Write(Waiting.GetValue());
		Waiting.Reset();
	}
}

void FCareerSaver::Start(ss::SaveData&& Data)
{
	InFlight = UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, D = MoveTemp(Data)]() mutable { Write(D); });
}

void FCareerSaver::Write(ss::SaveData& Data)
{
	const double T0 = FPlatformTime::Seconds();
	// The world's text: written once per snapshot (the same one comes back until the world moves on).
	bool bLent = false;
	if (Data.WorldSnapshot && Data.WorldText.empty())
	{
		if (Data.WorldSnapshot != TextOf)
		{
			WorldText.clear();
			Data.WorldSnapshot->Write(WorldText);
			TextOf = Data.WorldSnapshot;
		}
		Data.WorldText.swap(WorldText);
		bLent = true;
	}
	const double T1 = FPlatformTime::Seconds();
	const std::string Text = Data.Serialize();
	if (bLent)
	{
		Data.WorldText.swap(WorldText);
	}
	CareerSave::SaveNow(Text, false);
	UE_LOG(LogCareerSave, Verbose, TEXT("Background save: world text %.1f ms, the rest %.1f ms."), (T1 - T0) * 1000.0, (FPlatformTime::Seconds() - T1) * 1000.0);
}
