#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PlayflowEditorSettings.generated.h"

UENUM()
enum class EPlayflowBuildName : uint8
{
    Default     UMETA(DisplayName = "Default"),
    Beta        UMETA(DisplayName = "Beta"),
    Staging     UMETA(DisplayName = "Staging"),
    Development UMETA(DisplayName = "Development"),
    Custom      UMETA(DisplayName = "Custom")
};

UCLASS(config = EditorPerProjectUserSettings)
class PLAYFLOWEDITOR_API UPlayflowEditorSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(config, EditAnywhere, Category = "Playflow", meta = (DisplayName = "API Key"))
    FString APIKey;

    UPROPERTY(config, EditAnywhere, Category = "Playflow", meta = (DisplayName = "Build Name", ToolTip = "The name used for this build when uploading to Playflow Cloud. Also determines the output ZIP filename."))
    EPlayflowBuildName BuildName;

    UPROPERTY(config, EditAnywhere, Category = "Playflow", meta = (DisplayName = "Custom Build Name", ToolTip = "Custom build name to use when Build Name is set to Custom.", EditCondition = "BuildName == EPlayflowBuildName::Custom", EditConditionHides))
    FString CustomBuildName;

    UPROPERTY(config, EditAnywhere, Category = "Playflow", meta = (DisplayName = "Build Output Directory", ToolTip = "Directory where the server build and ZIP will be saved. Leave empty to use the default: Saved/PlayflowBuilds."))
    FDirectoryPath BuildOutputDirectory;

    UPROPERTY(config, EditAnywhere, Category = "Playflow", meta = (DisplayName = "Keep Debug Files", ToolTip = "If enabled, .debug and .sym files will not be deleted after building."))
    bool bKeepDebugFiles;

    UPROPERTY(config, EditAnywhere, Category = "Playflow", meta = (DisplayName = "Use Development Build", ToolTip = "If enabled, the server will be built in Development configuration instead of Shipping."))
    bool bUseDevelopmentBuild;

    UPlayflowEditorSettings()
        : APIKey(TEXT(""))
        , BuildName(EPlayflowBuildName::Default)
        , CustomBuildName(TEXT(""))
        , bKeepDebugFiles(false)
        , bUseDevelopmentBuild(false)
    {
        BuildOutputDirectory.Path = TEXT("");
    }

    static UPlayflowEditorSettings* Get();

    void SaveSettings();
};