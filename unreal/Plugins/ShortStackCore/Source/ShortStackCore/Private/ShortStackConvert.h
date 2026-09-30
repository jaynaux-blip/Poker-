// String conversions between the engine (UTF-8 std::string) and Unreal (FString).
#pragma once

#include "CoreMinimal.h"

#include <string>

namespace ShortStackConvert
{
inline std::string ToStd(const FString& Text)
{
	FTCHARToUTF8 Utf8(*Text);
	return std::string(Utf8.Get(), static_cast<size_t>(Utf8.Length()));
}

inline FString ToUnreal(const std::string& Text)
{
	return FString(UTF8_TO_TCHAR(Text.c_str()));
}
} // namespace ShortStackConvert
