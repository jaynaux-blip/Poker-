// Runs the golden vectors exported from the TypeScript prototype inside Unreal.
// Session Frontend > Automation > ShortStack.Core.GoldenVectors, or from the command line:
//   UnrealEditor-Cmd <Project>.uproject -ExecCmds="Automation RunTests ShortStack.Core; Quit" -unattended -nullrhi

#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ShortStackConvert.h"

#include "ShortStack/Golden.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShortStackGoldenVectorsTest, "ShortStack.Core.GoldenVectors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FShortStackGoldenVectorsTest::RunTest(const FString& /*Parameters*/)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ShortStackCore"));
	if (!TestTrue(TEXT("ShortStackCore plugin is loaded"), Plugin.IsValid()))
	{
		return false;
	}
	const FString Path = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Tests"), TEXT("golden_vectors.txt"));
	FString Text;
	if (!TestTrue(FString::Printf(TEXT("Read %s"), *Path), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	const ss::GoldenResult Result = ss::RunGoldenVectors(ShortStackConvert::ToStd(Text));
	AddInfo(ShortStackConvert::ToUnreal(Result.Report));
	AddInfo(FString::Printf(TEXT("Golden vectors: %d passed, %d failed, %d skipped"), Result.Passed, Result.Failed, Result.Skipped));
	TestEqual(TEXT("Mismatches against the TypeScript build"), Result.Failed, 0);
	TestTrue(TEXT("All golden vectors ran"), Result.Passed > 4000 && Result.Skipped == 0);
	return Result.Failed == 0;
}

#endif // WITH_DEV_AUTOMATION_TESTS
