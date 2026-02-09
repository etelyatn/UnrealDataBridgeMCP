// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

using UnrealBuildTool;

public class UnrealDataBridge : ModuleRules
{
	public UnrealDataBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"Sockets",
			"Networking",
			"Json",
			"JsonUtilities",
			"GameplayTags",
			"AssetRegistry",
			"StructUtils",
			"UnrealEd",
		});
	}
}
