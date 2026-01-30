// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Gamejam2026 : ModuleRules
{
	public Gamejam2026(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Gamejam2026",
			"Gamejam2026/Variant_Platforming",
			"Gamejam2026/Variant_Platforming/Animation",
			"Gamejam2026/Variant_Combat",
			"Gamejam2026/Variant_Combat/AI",
			"Gamejam2026/Variant_Combat/Animation",
			"Gamejam2026/Variant_Combat/Gameplay",
			"Gamejam2026/Variant_Combat/Interfaces",
			"Gamejam2026/Variant_Combat/UI",
			"Gamejam2026/Variant_SideScrolling",
			"Gamejam2026/Variant_SideScrolling/AI",
			"Gamejam2026/Variant_SideScrolling/Gameplay",
			"Gamejam2026/Variant_SideScrolling/Interfaces",
			"Gamejam2026/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
