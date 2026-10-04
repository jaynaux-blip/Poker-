#include "ShortStack.h"

#include "DynamicRHI.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "RHIStats.h"

DEFINE_LOG_CATEGORY(LogNightOne);

namespace ShortStackModuleDetail
{
/**
 * The texture streaming pool, sized to the card: every room stacks 4K baked props, MetaHumans and screens, and the default
 * 1 GB pool runs dry (the far textures sit at blurry mips and the editor prints "texture streaming pool over budget"). A
 * third of the video memory, up to 3.5 GB; never less than what the engine already chose.
 */
void SizeTexturePool()
{
	IConsoleVariable* Pool = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.PoolSize"));
	if (!Pool || !GDynamicRHI)
	{
		return;
	}
	FTextureMemoryStats Stats;
	RHIGetTextureMemoryStats(Stats);
	if (Stats.DedicatedVideoMemory <= 0)
	{
		return;
	}
	const int32 VideoMb = static_cast<int32>(Stats.DedicatedVideoMemory / (1024 * 1024));
	const int32 Want = FMath::Min(VideoMb / 3, 3500);
	if (Want > Pool->GetInt())
	{
		Pool->Set(Want, ECVF_SetByCode);
		UE_LOG(LogNightOne, Log, TEXT("Texture pool %d MB (%d MB of video memory)"), Want, VideoMb);
	}
}
} // namespace ShortStackModuleDetail

class FShortStackModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		ShortStackModuleDetail::SizeTexturePool();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FShortStackModule, ShortStack, "ShortStack");
