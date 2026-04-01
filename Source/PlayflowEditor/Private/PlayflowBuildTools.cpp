#include "PlayflowBuildTools.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformFilemanager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "Interfaces/IProjectManager.h"
#include "Interfaces/IPluginManager.h"
#include "Async/Async.h"
#include "Misc/FileHelper.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "PlayflowEditorSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

FString UPlayflowBuildTools::CachedZipFilePath = TEXT("");

void UPlayflowBuildTools::BuildAndPushToCloud()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Starting Build and Push process..."));

    FText DialogMessage = FText::FromString(TEXT("Build Linux Server and upload to Playflow Cloud?\n\nThis may take several minutes."));

    EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, DialogMessage);

    if (Result == EAppReturnType::No)
    {
        UE_LOG(LogTemp, Log, TEXT("Playflow: Build and Push cancelled by user"));
        return;
    }

    FNotificationInfo Info(FText::FromString(TEXT("Building Linux Server for Playflow...")));
    Info.bFireAndForget = false;
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 0.0f;

    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
    }

    BuildLinuxServer(NotificationItem);
}

bool UPlayflowBuildTools::BuildLinuxServer(TSharedPtr<SNotificationItem> NotificationItem)
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Building Linux Server (64-bit)..."));

    FString ProjectPath = GetProjectFilePath();
    if (ProjectPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to get project file path"));
        FMessageDialog::Open(EAppMsgType::Ok,
            FText::FromString(TEXT("Failed to get project file path.\n\nPlease ensure your project is saved.")));
        return false;
    }

    FString OutputDir = GetBuildOutputDirectory();

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!FPaths::DirectoryExists(OutputDir))
    {
        if (!PlatformFile.CreateDirectoryTree(*OutputDir))
        {
            UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to create output directory: %s"), *OutputDir);
            FMessageDialog::Open(EAppMsgType::Ok,
                FText::FromString(TEXT("Failed to create build output directory.\n\nCheck disk permissions.")));
            return false;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Starting Linux Server build..."));
    UE_LOG(LogTemp, Log, TEXT("Playflow: Project: %s"), *ProjectPath);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Output: %s"), *OutputDir);

    Async(EAsyncExecution::Thread, [ProjectPath, OutputDir, NotificationItem]()
        {
            UE_LOG(LogTemp, Log, TEXT("Playflow: Build thread started"));

            FString UATPath = GetUATBatchPath();
            if (UATPath.IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to find UAT"));

                AsyncTask(ENamedThreads::GameThread, [NotificationItem]()
                    {
                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build failed: UAT not found")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }
                    });
                return;
            }

            UPlayflowEditorSettings* Settings = UPlayflowEditorSettings::Get();
            const FString BuildConfig = (Settings && Settings->bUseDevelopmentBuild) ? TEXT("Development") : TEXT("Shipping");

            FString CommandLine = FString::Printf(
                TEXT("BuildCookRun -project=\"%s\" -platform=Linux -clientconfig=%s -serverconfig=%s -server -serverplatform=Linux -noclient -cook -build -stage -pak -archive -archivedirectory=\"%s\" -utf8output"),
                *ProjectPath,
                *BuildConfig,
                *BuildConfig,
                *OutputDir
            );

            UE_LOG(LogTemp, Log, TEXT("Playflow: Executing UAT: %s"), *CommandLine);

            void* ReadPipe = nullptr;
            void* WritePipe = nullptr;
            FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

            FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
                *UATPath,
                *CommandLine,
                false,
                true,
                true,
                nullptr,
                0,
                nullptr,
                WritePipe,
                ReadPipe
            );

            if (!ProcessHandle.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to launch UAT process"));
                FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

                AsyncTask(ENamedThreads::GameThread, [NotificationItem]()
                    {
                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build failed: Could not start UAT")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }
                    });
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("Playflow: Build process started, monitoring output..."));

            FString OutputLog;
            while (FPlatformProcess::IsProcRunning(ProcessHandle))
            {
                FString NewOutput = FPlatformProcess::ReadPipe(ReadPipe);
                if (NewOutput.Len() > 0)
                {
                    OutputLog += NewOutput;
                    TArray<FString> Lines;
                    NewOutput.ParseIntoArrayLines(Lines);
                    for (const FString& Line : Lines)
                    {
                        UE_LOG(LogTemp, Log, TEXT("UAT: %s"), *Line);
                    }
                }
                FPlatformProcess::Sleep(0.1f);
            }

            FString RemainingOutput = FPlatformProcess::ReadPipe(ReadPipe);
            if (RemainingOutput.Len() > 0)
            {
                OutputLog += RemainingOutput;
                TArray<FString> Lines;
                RemainingOutput.ParseIntoArrayLines(Lines);
                for (const FString& Line : Lines)
                {
                    UE_LOG(LogTemp, Log, TEXT("UAT: %s"), *Line);
                }
            }

            int32 ReturnCode = 0;
            FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode);
            FPlatformProcess::CloseProc(ProcessHandle);
            FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

            UE_LOG(LogTemp, Log, TEXT("Playflow: Build process completed with return code: %d"), ReturnCode);

            FString LogFilePath = FPaths::Combine(OutputDir, TEXT("PlayflowBuildLog.txt"));
            FFileHelper::SaveStringToFile(OutputLog, *LogFilePath);
            UE_LOG(LogTemp, Log, TEXT("Playflow: Build log saved to: %s"), *LogFilePath);

            AsyncTask(ENamedThreads::GameThread, [ReturnCode, OutputLog, LogFilePath, NotificationItem]()
                {
                    if (ReturnCode == 0)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Build succeeded! Running post-build steps..."));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build complete! Packaging and uploading...")));
                        }

                        if (!RunPostBuildSteps(NotificationItem))
                        {
                            if (NotificationItem.IsValid())
                            {
                                NotificationItem->SetText(FText::FromString(TEXT("Packaging failed - check Output Log")));
                                NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                                NotificationItem->SetFadeOutDuration(0.5f);
                                NotificationItem->SetExpireDuration(3.0f);
                                NotificationItem->ExpireAndFadeout();
                            }

                            UE_LOG(LogTemp, Error, TEXT("Playflow: Post-build steps failed"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Playflow: Build failed with error code: %d"), ReturnCode);

                        FString ErrorSummary = TEXT("Build failed");
                        TArray<FString> Lines;
                        OutputLog.ParseIntoArrayLines(Lines);

                        int32 StartLine = FMath::Max(0, Lines.Num() - 50);
                        for (int32 i = StartLine; i < Lines.Num(); i++)
                        {
                            if (Lines[i].Contains(TEXT("ERROR:")) || Lines[i].Contains(TEXT("Error:")) ||
                                Lines[i].Contains(TEXT("error C")) || Lines[i].Contains(TEXT("error LNK")))
                            {
                                ErrorSummary = Lines[i];
                                if (ErrorSummary.Len() > 100)
                                {
                                    ErrorSummary = ErrorSummary.Left(100) + TEXT("...");
                                }
                                break;
                            }
                        }

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(ErrorSummary));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->SetHyperlink(FSimpleDelegate::CreateLambda([LogFilePath]()
                                {
                                    FPlatformProcess::LaunchFileInDefaultExternalApplication(*LogFilePath);
                                }), FText::FromString(TEXT("View Log")));
                            NotificationItem->ExpireAndFadeout();
                        }
                    }
                });
        });

    return true;
}

static FString GetResolvedBuildName()
{
    UPlayflowEditorSettings* Settings = UPlayflowEditorSettings::Get();
    if (!Settings)
    {
        return TEXT("default");
    }

    switch (Settings->BuildName)
    {
    case EPlayflowBuildName::Beta:        return TEXT("beta");
    case EPlayflowBuildName::Staging:     return TEXT("staging");
    case EPlayflowBuildName::Development: return TEXT("development");
    case EPlayflowBuildName::Custom:
    {
        FString Name = Settings->CustomBuildName.TrimStartAndEnd();
        if (!Name.IsEmpty())
        {
            return Name;
        }
        UE_LOG(LogTemp, Warning, TEXT("Playflow: Custom build name is empty, falling back to 'default'"));
        return TEXT("default");
    }
    default: return TEXT("default");
    }
}

bool UPlayflowBuildTools::PackageBuild()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Packaging build..."));

    FString BuildOutputDir = GetBuildOutputDirectory();
    FString LinuxServerDir = FPaths::Combine(BuildOutputDir, TEXT("LinuxServer"));

    if (!FPaths::DirectoryExists(LinuxServerDir))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Linux Server directory not found at: %s"), *LinuxServerDir);
        return false;
    }

    // Check for 7-Zip
    FString SevenZipPath = Find7ZipPath();
    if (SevenZipPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: 7-Zip not found!"));

        FText ErrorMessage = FText::FromString(
            TEXT("7-Zip is required to package builds for upload.\n\n")
            TEXT("Please install 7-Zip from: https://www.7-zip.org/\n\n")
            TEXT("The build has completed successfully at:\n") + LinuxServerDir + TEXT("\n\n")
            TEXT("But it cannot be packaged and uploaded without 7-Zip.")
        );

        FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage);
        return false;
    }

    FString BuildNameStr = GetResolvedBuildName();
    FString ZipFileName = BuildNameStr + TEXT(".zip");
    FString ZipFilePath = FPaths::Combine(BuildOutputDir, ZipFileName);

    UE_LOG(LogTemp, Log, TEXT("Playflow: Build name: %s"), *BuildNameStr);

    UE_LOG(LogTemp, Log, TEXT("Playflow: Creating zip file: %s"), *ZipFilePath);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Source directory: %s"), *LinuxServerDir);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Using 7-Zip at: %s"), *SevenZipPath);

    bool bSuccess = CreateZipWith7Zip(LinuxServerDir, ZipFilePath, SevenZipPath);

    if (bSuccess)
    {
        int64 FileSize = IFileManager::Get().FileSize(*ZipFilePath);
        float FileSizeMB = FileSize / (1024.0f * 1024.0f);

        UE_LOG(LogTemp, Log, TEXT("Playflow: Package created successfully!"));
        UE_LOG(LogTemp, Log, TEXT("Playflow: File: %s"), *ZipFilePath);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Size: %.2f MB"), FileSizeMB);

        SetLatestZipFilePath(ZipFilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to create package"));
    }

    return bSuccess;
}

bool UPlayflowBuildTools::UploadToCloud(TSharedPtr<SNotificationItem> NotificationItem)
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Starting upload to Playflow Cloud..."));

    FString ZipFilePath = GetLatestZipFilePath();
    if (ZipFilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: No zip file found to upload"));
        return false;
    }

    FString APIKey = GetAPIKey();
    if (APIKey.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Playflow: No API key configured"));
        return false;
    }

    if (!FPaths::FileExists(ZipFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Zip file not found: %s"), *ZipFilePath);
        return false;
    }

    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *ZipFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to read zip file"));
        return false;
    }

    int64 FileSize = FileData.Num();
    float FileSizeMB = FileSize / (1024.0f * 1024.0f);
    UE_LOG(LogTemp, Log, TEXT("Playflow: File loaded: %.2f MB"), FileSizeMB);

    if (NotificationItem.IsValid())
    {
        NotificationItem->SetText(FText::FromString(TEXT("Getting upload URL from Playflow Cloud...")));
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
    }

    FString ResolvedBuildName = GetResolvedBuildName();
    FString UploadEndpoint = FString::Printf(TEXT("https://api.computeflow.cloud/api/v3/builds/upload-url?name=%s"), *ResolvedBuildName);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Requesting upload URL for build name: %s"), *ResolvedBuildName);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(UploadEndpoint);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetTimeout(300.0f);

    HttpRequest->OnProcessRequestComplete().BindLambda([FileData, FileSizeMB, NotificationItem, APIKey](
        FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            AsyncTask(ENamedThreads::GameThread, [FileData, FileSizeMB, NotificationItem, APIKey, Response, bWasSuccessful]()
                {
                    if (!bWasSuccessful || !Response.IsValid())
                    {
                        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to get upload URL - Network error"));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Upload failed: Could not get upload URL")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }

                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                        return;
                    }

                    int32 ResponseCode = Response->GetResponseCode();
                    FString ResponseString = Response->GetContentAsString();

                    UE_LOG(LogTemp, Log, TEXT("Playflow: Get URL Response Code: %d"), ResponseCode);
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Get URL Response: %s"), *ResponseString);

                    if (ResponseCode < 200 || ResponseCode >= 300)
                    {
                        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to get upload URL - HTTP %d"), ResponseCode);

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Upload failed: HTTP %d getting URL"), ResponseCode)));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }

                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                        return;
                    }

                    TSharedPtr<FJsonObject> JsonObject;
                    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

                    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
                    {
                        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to parse upload URL response"));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Upload failed: Invalid response from server")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }

                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                        return;
                    }

                    FString UploadUrl;
                    if (!JsonObject->TryGetStringField(TEXT("upload_url"), UploadUrl) || UploadUrl.IsEmpty())
                    {
                        UE_LOG(LogTemp, Error, TEXT("Playflow: No upload_url in response"));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Upload failed: No upload URL received")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }

                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                        return;
                    }

                    FString BuildId;
                    if (!JsonObject->TryGetStringField(TEXT("build_id"), BuildId) || BuildId.IsEmpty())
                    {
                        UE_LOG(LogTemp, Error, TEXT("Playflow: No build_id in response"));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Upload failed: No build ID received")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->SetFadeOutDuration(0.5f);
                            NotificationItem->SetExpireDuration(5.0f);
                            NotificationItem->ExpireAndFadeout();
                        }

                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                        return;
                    }

                    UE_LOG(LogTemp, Log, TEXT("Playflow: Got upload URL: %s"), *UploadUrl);
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Got build ID: %s"), *BuildId);

                    if (NotificationItem.IsValid())
                    {
                        NotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Uploading %.1f MB to Playflow Cloud..."), FileSizeMB)));
                        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
                    }

                    UE_LOG(LogTemp, Log, TEXT("Playflow: Uploading file"));

                    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UploadRequest = FHttpModule::Get().CreateRequest();
                    UploadRequest->SetURL(UploadUrl);
                    UploadRequest->SetVerb(TEXT("PUT"));
                    UploadRequest->SetHeader(TEXT("Content-Type"), TEXT("application/zip"));
                    UploadRequest->SetContent(FileData);
                    UploadRequest->SetTimeout(900.0f);

                    UE_LOG(LogTemp, Log, TEXT("Playflow: Prepared: %d bytes"), FileData.Num());

                    int64 TotalBytes = FileData.Num();

#if ENGINE_MAJOR_VERSION == 4
                    UploadRequest->OnRequestProgress().BindLambda([NotificationItem, TotalBytes](FHttpRequestPtr Req, int32 BytesSent, int32 BytesReceived)
#else
                    UploadRequest->OnRequestProgress64().BindLambda([NotificationItem, TotalBytes](FHttpRequestPtr Req, int64 BytesSent, int64 BytesReceived)
#endif
                        {
                            if (TotalBytes > 0 && NotificationItem.IsValid())
                            {
                                float Percent = FMath::Clamp((float)BytesSent / (float)TotalBytes * 100.0f, 0.0f, 100.0f);
                                float SentMB = BytesSent / (1024.0f * 1024.0f);
                                float TotalMB = TotalBytes / (1024.0f * 1024.0f);

                                AsyncTask(ENamedThreads::GameThread, [NotificationItem, Percent, SentMB, TotalMB]()
                                    {
                                        if (NotificationItem.IsValid())
                                        {
                                            NotificationItem->SetText(FText::FromString(
                                                FString::Printf(TEXT("Uploading... %.0f%% (%.1f / %.1f MB)"), Percent, SentMB, TotalMB)
                                            ));
                                        }
                                    });
                            }
                        });

                    UploadRequest->OnProcessRequestComplete().BindLambda([FileSizeMB, NotificationItem](
                        FHttpRequestPtr UploadReq, FHttpResponsePtr UploadResp, bool bUploadSuccessful)
                        {
                            AsyncTask(ENamedThreads::GameThread, [FileSizeMB, NotificationItem, UploadResp, bUploadSuccessful]()
                                {
                                    if (!bUploadSuccessful || !UploadResp.IsValid())
                                    {
                                        UE_LOG(LogTemp, Error, TEXT("Playflow: File upload failed"));

                                        if (NotificationItem.IsValid())
                                        {
                                            NotificationItem->SetText(FText::FromString(TEXT("Upload failed: Network error during file upload")));
                                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                                            NotificationItem->SetFadeOutDuration(0.5f);
                                            NotificationItem->SetExpireDuration(5.0f);
                                            NotificationItem->ExpireAndFadeout();
                                        }

                                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                                        return;
                                    }

                                    int32 UploadResponseCode = UploadResp->GetResponseCode();
                                    FString UploadResponseString = UploadResp->GetContentAsString();

                                    UE_LOG(LogTemp, Log, TEXT("Playflow: Upload Response Code: %d"), UploadResponseCode);
                                    UE_LOG(LogTemp, Log, TEXT("Playflow: Upload Response: %s"), *UploadResponseString);

                                    if (UploadResponseCode < 200 || UploadResponseCode >= 300)
                                    {
                                        UE_LOG(LogTemp, Error, TEXT("Playflow: File upload failed - HTTP %d"), UploadResponseCode);

                                        if (NotificationItem.IsValid())
                                        {
                                            NotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Upload failed: HTTP %d"), UploadResponseCode)));
                                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                                            NotificationItem->SetFadeOutDuration(0.5f);
                                            NotificationItem->SetExpireDuration(5.0f);
                                            NotificationItem->ExpireAndFadeout();
                                        }

                                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
                                    }
                                    else
                                    {
                                        UE_LOG(LogTemp, Log, TEXT("Playflow: Upload successful!"));

                                        if (NotificationItem.IsValid())
                                        {
                                            NotificationItem->SetText(FText::FromString(TEXT("Successfully uploaded to Playflow Cloud!")));
                                            NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
                                            NotificationItem->SetFadeOutDuration(0.5f);
                                            NotificationItem->SetExpireDuration(3.0f);
                                            NotificationItem->ExpireAndFadeout();
                                        }

                                        GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
                                    }
                                });
                        });

                    UploadRequest->ProcessRequest();
                    UE_LOG(LogTemp, Log, TEXT("Playflow: File upload started..."));
                        });
                });

    HttpRequest->ProcessRequest();
    UE_LOG(LogTemp, Log, TEXT("Playflow: Requesting upload URL..."));

    return true;
}

FString UPlayflowBuildTools::Find7ZipPath()
{
    TArray<FString> PossiblePaths = {
        TEXT("C:\\Program Files\\7-Zip\\7z.exe"),
        TEXT("C:\\Program Files (x86)\\7-Zip\\7z.exe")
    };

    for (const FString& Path : PossiblePaths)
    {
        if (FPaths::FileExists(Path))
        {
            return Path;
        }
    }

    FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
    TArray<FString> PathDirs;
    PathEnv.ParseIntoArray(PathDirs, TEXT(";"), true);

    for (const FString& Dir : PathDirs)
    {
        FString SevenZipExe = FPaths::Combine(Dir, TEXT("7z.exe"));
        if (FPaths::FileExists(SevenZipExe))
        {
            return SevenZipExe;
        }
    }

    return TEXT("");
}

bool UPlayflowBuildTools::CreateZipWith7Zip(const FString& SourceDir, const FString& ZipFilePath, const FString& SevenZipPath)
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Creating zip with 7-Zip..."));

    // Delete existing zip if it exists
    if (FPaths::FileExists(ZipFilePath))
    {
        IFileManager::Get().Delete(*ZipFilePath);
    }

    FString CommandLine = FString::Printf(
        TEXT("a -tzip \"%s\" \".\""),
        *ZipFilePath
    );

    UE_LOG(LogTemp, Log, TEXT("Playflow: 7-Zip command: %s %s"), *SevenZipPath, *CommandLine);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Working directory: %s"), *SourceDir);

    FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
        *SevenZipPath,
        *CommandLine,
        false,  // bLaunchDetached
        true,   // bLaunchHidden - hide the window for 7-Zip
        true,   // bLaunchReallyHidden
        nullptr,
        0,
        *SourceDir, // Working directory
        nullptr
    );

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to launch 7-Zip"));
        return false;
    }

    int32 ReturnCode = 0;
    while (FPlatformProcess::IsProcRunning(ProcessHandle))
    {
        FPlatformProcess::Sleep(0.1f);
    }

    FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode);
    FPlatformProcess::CloseProc(ProcessHandle);

    if (ReturnCode != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: 7-Zip failed with return code: %d"), ReturnCode);
        return false;
    }

    if (!FPaths::FileExists(ZipFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Zip file not found after 7-Zip compression"));
        return false;
    }

    int64 FileSize = IFileManager::Get().FileSize(*ZipFilePath);
    float FileSizeMB = FileSize / (1024.0f * 1024.0f);
    UE_LOG(LogTemp, Log, TEXT("Playflow: 7-Zip compression complete! Size: %.2f MB"), FileSizeMB);

    return true;
}

FString UPlayflowBuildTools::GetProjectFilePath()
{
    const FString& ProjectFilePath = FPaths::GetProjectFilePath();

    if (ProjectFilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Could not get project file path"));
        return TEXT("");
    }

    FString FullPath = FPaths::ConvertRelativePathToFull(ProjectFilePath);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Project file: %s"), *FullPath);

    return FullPath;
}

FString UPlayflowBuildTools::GetUATBatchPath()
{
    FString EngineDir = FPaths::EngineDir();

#if PLATFORM_WINDOWS
    FString UATPath = FPaths::Combine(EngineDir, TEXT("Build/BatchFiles/RunUAT.bat"));
#elif PLATFORM_MAC
    FString UATPath = FPaths::Combine(EngineDir, TEXT("Build/BatchFiles/RunUAT.sh"));
#else
    FString UATPath = FPaths::Combine(EngineDir, TEXT("Build/BatchFiles/RunUAT.sh"));
#endif

    if (!FPaths::FileExists(UATPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: UAT not found at: %s"), *UATPath);
        return TEXT("");
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: UAT path: %s"), *UATPath);
    return UATPath;
}

FString UPlayflowBuildTools::GetBuildOutputDirectory()
{
    UPlayflowEditorSettings* Settings = UPlayflowEditorSettings::Get();

    if (Settings && !Settings->BuildOutputDirectory.Path.IsEmpty())
    {
        FString CustomPath = FPaths::ConvertRelativePathToFull(Settings->BuildOutputDirectory.Path);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Using custom build output directory: %s"), *CustomPath);
        return CustomPath;
    }

    FString ProjectDir = FPaths::ProjectDir();
    FString OutputDir = FPaths::Combine(ProjectDir, TEXT("Saved/PlayflowBuilds"));
    OutputDir = FPaths::ConvertRelativePathToFull(OutputDir);

    UE_LOG(LogTemp, Log, TEXT("Playflow: Build output directory: %s"), *OutputDir);
    return OutputDir;
}

FString UPlayflowBuildTools::GetProjectName()
{
    FString ProjectPath = GetProjectFilePath();
    FString ProjectName = FPaths::GetBaseFilename(ProjectPath);

    UE_LOG(LogTemp, Log, TEXT("Playflow: Project name: %s"), *ProjectName);
    return ProjectName;
}

FString UPlayflowBuildTools::GetServerScriptTemplatePath()
{
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("Playflow");
    if (!Plugin.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Could not find Playflow plugin"));
        return TEXT("");
    }

    FString TemplatePath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/Server.x86_64.template"));
    UE_LOG(LogTemp, Log, TEXT("Playflow: Template path: %s"), *TemplatePath);

    return TemplatePath;
}

bool UPlayflowBuildTools::ReplaceServerScript()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Replacing Server.sh with custom Server.x86_64..."));

    FString ProjectName = GetProjectName();
    FString BuildOutputDir = GetBuildOutputDirectory();
    FString TemplatePath = GetServerScriptTemplatePath();

    if (ProjectName.IsEmpty() || TemplatePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to get project name or template path"));
        return false;
    }

    FString TemplateContent;
    if (!FFileHelper::LoadFileToString(TemplateContent, *TemplatePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to read template file: %s"), *TemplatePath);
        return false;
    }

    // Replace placeholders with correct values
    // For shipping builds: ProjectNameServer-Linux-Shipping
    // For development builds: ProjectNameServer (no suffix)
    UPlayflowEditorSettings* Settings = UPlayflowEditorSettings::Get();
    const bool bIsDevelopment = Settings && Settings->bUseDevelopmentBuild;
    FString ServerBinaryName = bIsDevelopment ? ProjectName + TEXT("Server") : ProjectName + TEXT("Server-Linux-Shipping");
    FString ProcessedContent = TemplateContent;

    // Replace <PROJECT_NAME> with just the project name (for folders and command line)
    ProcessedContent = ProcessedContent.Replace(TEXT("<PROJECT_NAME>"), *ProjectName);

    // Replace <SERVER_BINARY> with the server binary name (ProjectNameServer-Linux-Shipping)
    ProcessedContent = ProcessedContent.Replace(TEXT("<SERVER_BINARY>"), *ServerBinaryName);

    ProcessedContent = ProcessedContent.Replace(TEXT("\r\n"), TEXT("\n"));
    ProcessedContent = ProcessedContent.Replace(TEXT("\r"), TEXT("\n"));

    UE_LOG(LogTemp, Log, TEXT("Playflow: Project name: %s"), *ProjectName);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Server binary name: %s"), *ServerBinaryName);

    // Find the Server.sh file in the build output
    // It should be at: BuildOutputDir/LinuxServer/ProjectNameServer.sh
    FString ServerShPath = FPaths::Combine(BuildOutputDir, TEXT("LinuxServer"), ProjectName + TEXT("Server.sh"));

    if (!FPaths::FileExists(ServerShPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Playflow: Server.sh not found at: %s"), *ServerShPath);
        UE_LOG(LogTemp, Warning, TEXT("Playflow: Checking for alternative locations..."));

        // Try alternative path without "Server" suffix
        ServerShPath = FPaths::Combine(BuildOutputDir, TEXT("LinuxServer"), ProjectName + TEXT(".sh"));

        if (!FPaths::FileExists(ServerShPath))
        {
            UE_LOG(LogTemp, Error, TEXT("Playflow: Could not find server script"));
            return false;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Found server script at: %s"), *ServerShPath);

    FString NewServerPath = FPaths::Combine(FPaths::GetPath(ServerShPath), TEXT("Server.x86_64"));

    // Rename Server.sh to Server.x86_64
    IFileManager& FileManager = IFileManager::Get();
    if (!FileManager.Move(*NewServerPath, *ServerShPath, true, true))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to rename Server.sh to Server.x86_64"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Renamed Server.sh to Server.x86_64"));

    if (!FFileHelper::SaveStringToFile(ProcessedContent, *NewServerPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to write content to Server.x86_64 file"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Successfully updated Server.x86_64 with Unix line endings at: %s"), *NewServerPath);
    return true;
}



bool UPlayflowBuildTools::RunPostBuildSteps(TSharedPtr<SNotificationItem> NotificationItem)
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Running post-build steps..."));

    if (!ReplaceServerScript())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to replace server script"));
        return false;
    }

    if (!CleanupDebugFiles())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to clean up debug files"));
        return false;
    }

    if (!PackageBuild())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to package build"));
        return false;
    }

    // Step 4: Upload to Playflow Cloud
    UploadToCloud(NotificationItem);

    return true;
}

bool UPlayflowBuildTools::CleanupDebugFiles()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Checking debug file cleanup settings..."));

    UPlayflowEditorSettings* Settings = UPlayflowEditorSettings::Get();
    if (Settings && Settings->bKeepDebugFiles)
    {
        UE_LOG(LogTemp, Log, TEXT("Playflow: Keep Debug Files is enabled, skipping cleanup"));
        return true;
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Cleaning up debug files..."));

    FString ProjectName = GetProjectName();
    FString BuildOutputDir = GetBuildOutputDirectory();
    FString BinariesDir = FPaths::Combine(BuildOutputDir, TEXT("LinuxServer"), ProjectName, TEXT("Binaries/Linux"));

    if (!FPaths::DirectoryExists(BinariesDir))
    {
        UE_LOG(LogTemp, Warning, TEXT("Playflow: Binaries directory not found: %s"), *BinariesDir);
        return true;
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Scanning directory: %s"), *BinariesDir);

    // Find all debug and sym files
    TArray<FString> DebugFiles;
    IFileManager& FileManager = IFileManager::Get();

    FileManager.FindFilesRecursive(DebugFiles, *BinariesDir, TEXT("*.debug"), true, false);

    TArray<FString> SymFiles;
    FileManager.FindFilesRecursive(SymFiles, *BinariesDir, TEXT("*.sym"), true, false);

    DebugFiles.Append(SymFiles);

    if (DebugFiles.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Playflow: No debug files found to clean up"));
        return true;
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Found %d debug file(s) to remove"), DebugFiles.Num());

    int64 TotalSize = 0;
    for (const FString& File : DebugFiles)
    {
        TotalSize += FileManager.FileSize(*File);
    }

    float TotalSizeMB = TotalSize / (1024.0f * 1024.0f);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Total debug file size: %.2f MB"), TotalSizeMB);

    int32 DeletedCount = 0;
    for (const FString& File : DebugFiles)
    {
        FString FileName = FPaths::GetCleanFilename(File);

        if (FileManager.Delete(*File, false, true))
        {
            UE_LOG(LogTemp, Log, TEXT("Playflow: Deleted: %s"), *FileName);
            DeletedCount++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Playflow: Failed to delete: %s"), *FileName);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Cleanup complete! Removed %d file(s), saved %.2f MB"), DeletedCount, TotalSizeMB);

    return true;
}

void UPlayflowBuildTools::SetLatestZipFilePath(const FString& ZipPath)
{
    CachedZipFilePath = ZipPath;
    UE_LOG(LogTemp, Log, TEXT("Playflow: Cached zip file path: %s"), *CachedZipFilePath);
}

FString UPlayflowBuildTools::GetLatestZipFilePath()
{
    return CachedZipFilePath;
}

FString UPlayflowBuildTools::GetAPIKey()
{
    UPlayflowEditorSettings* Settings = UPlayflowEditorSettings::Get();
    FString APIKey = Settings->APIKey.TrimStartAndEnd();

    if (APIKey.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Playflow: No API Key configured"));
    }
    else
    {
        FString MaskedKey = APIKey.Left(8) + TEXT("...");
        UE_LOG(LogTemp, Log, TEXT("Playflow: Using API Key: %s"), *MaskedKey);
    }

    return APIKey;
}