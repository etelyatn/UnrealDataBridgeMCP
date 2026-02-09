// Copyright Eugene Telyatnik. All Rights Reserved. https://github.com/etelyatn/UnrealDataBridgeMCP

#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUDBModuleLoadTest,
	"UDB.Module.PluginLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FUDBModuleLoadTest::RunTest(const FString& Parameters)
{
	const bool bLoaded = FModuleManager::Get().IsModuleLoaded(TEXT("UnrealDataBridge"));
	TestTrue(TEXT("UnrealDataBridge module should be loaded"), bLoaded);
	return true;
}
