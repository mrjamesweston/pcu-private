#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PlayflowBuildTools.generated.h"


UCLASS()
class PLAYFLOWEDITOR_API UPlayflowBuildTools : public UObject
{
    GENERATED_BODY()

public:

    static void BuildAndPushToCloud();

private:

    static bool BuildLinuxServer(TSharedPtr<SNotificationItem> NotificationItem);

    static bool PackageBuild();

    static bool UploadToCloud(TSharedPtr<SNotificationItem> NotificationItem);

    static FString GetProjectFilePath();

    static FString GetUATBatchPath();

    static FString GetBuildOutputDirectory();

    static FString GetProjectName();

    static bool ReplaceServerScript();

    static FString GetServerScriptTemplatePath();

    static bool RunPostBuildSteps(TSharedPtr<SNotificationItem> NotificationItem);

    static FString Find7ZipPath();

    static bool CreateZipWith7Zip(const FString& SourceDir, const FString& ZipFilePath, const FString& SevenZipPath);

    static bool CleanupDebugFiles();

    static FString GetLatestZipFilePath();

    static void SetLatestZipFilePath(const FString& ZipPath);

    static FString GetAPIKey();

private:
    static FString CachedZipFilePath;
};