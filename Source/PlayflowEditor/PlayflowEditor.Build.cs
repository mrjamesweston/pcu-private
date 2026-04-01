using UnrealBuildTool;

public class PlayflowEditor : ModuleRules
{
    public PlayflowEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Playflow"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "UnrealEd",
            "LevelEditor",
            "ToolMenus",
            "Projects",
            "EditorStyle",
            "ImageWrapper",
            "InputCore",
            "HTTP",
            "Json",
            "JsonUtilities",
            "Settings"
        });  
    }
}