// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IKProject : ModuleRules
{
	public IKProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core",
                        "AnimationCore",
                        "BlueprintGraph",
                        "UnrealEd",
                        "GraphEditor",
                        "PropertyEditor",
                        "SkeletonEditor",
                        "AdvancedPreviewScene",
                        "EditorStyle",
                        "AnimGraph",
                        "Persona",
                        "AnimGraphRuntime",
                        "SteamVR",
                        "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay"
        });

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                        "InputCore",
                        "SlateCore",
                        "UnrealEd",
                        "AnimGraph",
                        "GraphEditor",
                        "PropertyEditor",
                        "EditorStyle",
                        "ContentBrowser",
        });

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                        "AnimGraph",
                        "Persona",
                        "SkeletonEditor",
                        "AdvancedPreviewScene",
        });
    }
}
