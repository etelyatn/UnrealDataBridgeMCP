// Copyright Eugene Telyatnik. All Rights Reserved. https://github.com/etelyatn/UnrealDataBridgeMCP

using UnrealBuildTool;

public class UnrealDataBridgeTests : ModuleRules
{
	public UnrealDataBridgeTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealDataBridge",
			"Sockets",
			"Networking",
			"Json",
			"JsonUtilities",
			"GameplayTags",
			"StructUtils",
		});
	}
}
