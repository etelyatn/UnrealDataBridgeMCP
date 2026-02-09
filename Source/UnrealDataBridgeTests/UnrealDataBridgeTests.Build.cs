// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

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
