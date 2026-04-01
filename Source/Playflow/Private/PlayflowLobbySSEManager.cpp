#include "PlayflowLobbySSEManager.h"
#include "PlayflowAPI.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Misc/ScopeLock.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "PlayflowTypes.h"

FPlayflowLobbySSEManagerImpl::FPlayflowLobbySSEManagerImpl()
    : bIsSubscribed(false)
    , LastProcessedBytes(0)
    , ReconnectAttempts(0)
    , bShouldReconnect(true)
    , LastDataReceivedTime(0.0)
{
}

FPlayflowLobbySSEManagerImpl::~FPlayflowLobbySSEManagerImpl()
{
    UnsubscribeFromLobbyUpdates();
}

void FPlayflowLobbySSEManagerImpl::SubscribeToLobbyUpdates(
    const FString& LobbyId,
    const FString& PlayerId,
    const FString& ConfigName,
    const FString& ApiKey
)
{
    if (bIsSubscribed)
    {
        UE_LOG(LogTemp, Warning, TEXT("Already subscribed to lobby updates"));
        return;
    }

    CurrentLobbyId = LobbyId;
    CurrentPlayerId = PlayerId;
    CurrentConfigName = ConfigName;
    CurrentApiKey = ApiKey;
    bShouldReconnect = true;
    ReconnectAttempts = 0;
    LastProcessedBytes = 0;
    PartialData.Empty();

    FString URL = FString::Printf(
        TEXT("https://api.scale.computeflow.cloud/lobbies-sse/%s/events?player-id=%s&lobby-config=%s"),
        *LobbyId,
        *PlayerId,
        *ConfigName
    );

    CurrentRequest = FHttpModule::Get().CreateRequest();
    CurrentRequest->SetURL(URL);
    CurrentRequest->SetVerb(TEXT("GET"));
    CurrentRequest->SetHeader(TEXT("Accept"), TEXT("text/event-stream"));
    CurrentRequest->SetHeader(TEXT("Cache-Control"), TEXT("no-cache"));
    CurrentRequest->SetHeader(TEXT("Connection"), TEXT("keep-alive"));
    CurrentRequest->SetHeader(TEXT("api-key"), ApiKey);
    CurrentRequest->SetTimeout(0.0f);

    if (!LastEventID.IsEmpty())
    {
        CurrentRequest->SetHeader(TEXT("Last-Event-ID"), LastEventID);
    }

    CurrentRequest->OnHeaderReceived().BindLambda([this](FHttpRequestPtr Request, const FString& HeaderName, const FString& NewHeaderValue)
        {
            if (!Request.IsValid())
                return;

            FHttpResponsePtr Response = Request->GetResponse();
            if (!Response.IsValid())
                return;

            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode >= 400)
            {
                UE_LOG(LogTemp, Error, TEXT("Playflow SSE: HTTP Error %d detected in OnHeaderReceived"), ResponseCode);
                if (ResponseCode == 401 || ResponseCode == 403)
                {
                    UE_LOG(LogTemp, Error, TEXT("Playflow SSE: KICKED - HTTP %d"), ResponseCode);
                }
                return;
            }

            if (!bIsSubscribed)
            {
                bIsSubscribed = true;
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        HandleSSEConnected();
                    });
            }

            const TArray<uint8>& Content = Response->GetContent();
            int32 NewBytes = Content.Num() - LastProcessedBytes;

            if (NewBytes > 0)
            {
                FString NewData;
                const uint8* DataPtr = Content.GetData() + LastProcessedBytes;
                FUTF8ToTCHAR Converter((const ANSICHAR*)DataPtr, NewBytes);
                NewData = FString(Converter.Length(), Converter.Get());

                LastProcessedBytes = Content.Num();
                PartialData += NewData;
                ProcessSSEBuffer();
            }
        });
#if ENGINE_MAJOR_VERSION == 4
    CurrentRequest->OnRequestProgress().BindLambda([this](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)

#else 
    CurrentRequest->OnRequestProgress64().BindLambda([this](FHttpRequestPtr Request, int64 BytesSent, int64 BytesReceived)
#endif
        {
            if (!Request.IsValid())
                return;

            FHttpResponsePtr Response = Request->GetResponse();
            if (!Response.IsValid())
                return;

            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode >= 400)
            {
                UE_LOG(LogTemp, Error, TEXT("Playflow SSE: HTTP Error %d detected in OnRequestProgress"), ResponseCode);
                if (ResponseCode == 401 || ResponseCode == 403)
                {
                    UE_LOG(LogTemp, Error, TEXT("Playflow SSE: KICKED - HTTP %d"), ResponseCode);
                }
                return;
            }

            if (!bIsSubscribed && BytesReceived > 0)
            {
                bIsSubscribed = true;
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        HandleSSEConnected();
                    });
            }

            const TArray<uint8>& Content = Response->GetContent();
            int32 NewBytes = Content.Num() - LastProcessedBytes;

            if (NewBytes > 0)
            {
                FString NewData;
                const uint8* DataPtr = Content.GetData() + LastProcessedBytes;
                FUTF8ToTCHAR Converter((const ANSICHAR*)DataPtr, NewBytes);
                NewData = FString(Converter.Length(), Converter.Get());

                LastProcessedBytes = Content.Num();
                PartialData += NewData;
                ProcessSSEBuffer();
            }
        });

    CurrentRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (Response.IsValid())
            {
                const TArray<uint8>& Content = Response->GetContent();
                if (Content.Num() > LastProcessedBytes)
                {
                    FString RemainingData;
                    const uint8* DataPtr = Content.GetData() + LastProcessedBytes;
                    int32 RemainingBytes = Content.Num() - LastProcessedBytes;
                    FUTF8ToTCHAR Converter((const ANSICHAR*)DataPtr, RemainingBytes);
                    RemainingData = FString(Converter.Length(), Converter.Get());

                    PartialData += RemainingData;
                    LastProcessedBytes = Content.Num();
                    ProcessSSEBuffer();
                }
            }

            bIsSubscribed = false;

            if (!bSuccess)
            {
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        if (bShouldReconnect && ReconnectAttempts < MaxReconnectAttempts)
                        {
                            AttemptReconnect();
                        }
                        else if (ReconnectAttempts >= MaxReconnectAttempts)
                        {
                            HandleSSEError(TEXT("Max reconnection attempts reached"), 0);
                        }
                    });
            }
            else if (Response.IsValid())
            {
                int32 ResponseCode = Response->GetResponseCode();

                if (ResponseCode >= 400)
                {
                    UE_LOG(LogTemp, Error, TEXT("HTTP Error %d"), ResponseCode);

                    if (ResponseCode == 401 || ResponseCode == 403)
                    {
                        bShouldReconnect = false;

                        AsyncTask(ENamedThreads::GameThread, [this, ResponseCode]()
                            {
                                HandleSSEError(FString::Printf(TEXT("Kicked: HTTP %d"), ResponseCode), ResponseCode);
                            });
                    }
                    else
                    {
                        AsyncTask(ENamedThreads::GameThread, [this]()
                            {
                                if (bShouldReconnect && ReconnectAttempts < MaxReconnectAttempts)
                                {
                                    AttemptReconnect();
                                }
                                else if (ReconnectAttempts >= MaxReconnectAttempts)
                                {
                                    HandleSSEError(TEXT("Max reconnection attempts reached"), 0);
                                }
                            });
                    }
                }
                else
                {
                    AsyncTask(ENamedThreads::GameThread, [this]()
                        {
                            if (bShouldReconnect && ReconnectAttempts < MaxReconnectAttempts)
                            {
                                AttemptReconnect();
                            }
                            else if (ReconnectAttempts >= MaxReconnectAttempts)
                            {
                                HandleSSEError(TEXT("Max reconnection attempts reached"), 0);
                            }
                        });
                }
            }
        });

    if (CurrentRequest->ProcessRequest())
    {
        UE_LOG(LogTemp, Log, TEXT("Playflow SSE: Connection started"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow SSE: Failed to start request"));
        HandleSSEError(TEXT("Failed to start request"), 0);
    }
}

void FPlayflowLobbySSEManagerImpl::ProcessSSEBuffer()
{
    while (true)
    {
        int32 EventEnd = PartialData.Find(TEXT("\n\n"));
        if (EventEnd == INDEX_NONE)
            break;

        FString EventBlock = PartialData.Left(EventEnd);
        PartialData = PartialData.Mid(EventEnd + 2);

        TArray<FString> Lines;
        EventBlock.ParseIntoArrayLines(Lines);

        FString EventType = TEXT("message");
        FString EventID;
        FString DataPayload;
        bool bHasData = false;

        for (const FString& Line : Lines)
        {
            if (Line.IsEmpty() || Line.StartsWith(TEXT(":")))
                continue;

            int32 ColonPos = INDEX_NONE;
            if (Line.FindChar(TEXT(':'), ColonPos))
            {
                FString Field = Line.Left(ColonPos);
                FString Value = Line.Mid(ColonPos + 1).TrimStart();

                if (Field == TEXT("event"))
                {
                    EventType = Value;
                }
                else if (Field == TEXT("id"))
                {
                    EventID = Value;
                }
                else if (Field == TEXT("data"))
                {
                    if (!DataPayload.IsEmpty())
                        DataPayload += TEXT("\n");
                    DataPayload += Value;
                    bHasData = true;
                }
            }
        }

        if (bHasData)
        {
            if (!EventID.IsEmpty())
                LastEventID = EventID;

            LastDataReceivedTime = FPlatformTime::Seconds();

            AsyncTask(ENamedThreads::GameThread, [this, DataPayload]()
                {
                    HandleSSEMessage(DataPayload);
                });
        }
    }
}

void FPlayflowLobbySSEManagerImpl::UnsubscribeFromLobbyUpdates()
{
    bShouldReconnect = false;

    UWorld* World = GEngine ? GEngine->GetWorld() : nullptr;
    if (World)
    {
        World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
        World->GetTimerManager().ClearTimer(ReconnectTimerHandle);
    }

    if (CurrentRequest.IsValid())
    {
        CurrentRequest->CancelRequest();
    }

    CurrentRequest.Reset();
    LastProcessedBytes = 0;
    PartialData.Empty();
    bIsSubscribed = false;
    ReconnectAttempts = 0;
    CurrentLobbyId.Empty();
    CurrentPlayerId.Empty();
    CurrentConfigName.Empty();
    CurrentApiKey.Empty();

    UE_LOG(LogTemp, Log, TEXT("Playflow SSE: Unsubscribed"));
}

bool FPlayflowLobbySSEManagerImpl::IsSubscribed() const
{
    return bIsSubscribed;
}

void FPlayflowLobbySSEManagerImpl::ProcessCallbacks()
{
    TArray<TFunction<void()>> CallbacksCopy;

    {
        FScopeLock Lock(&CallbackLock);
        CallbacksCopy = MoveTemp(PendingCallbacks);
        PendingCallbacks.Empty();
    }

    for (auto& Callback : CallbacksCopy)
    {
        Callback();
    }
}

void FPlayflowLobbySSEManagerImpl::QueueCallback(TFunction<void()> Callback)
{
    FScopeLock Lock(&CallbackLock);
    PendingCallbacks.Add(MoveTemp(Callback));
}

void FPlayflowLobbySSEManagerImpl::HandleSSEConnected()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow SSE: Connected to lobby %s"), *CurrentLobbyId);
    ReconnectAttempts = 0;
    LastDataReceivedTime = FPlatformTime::Seconds();

    UWorld* World = GEngine ? GEngine->GetWorld() : nullptr;
    if (World)
    {
        FTimerDelegate HeartbeatDelegate;
        HeartbeatDelegate.BindLambda([this]()
            {
                CheckHeartbeat();
            });

        World->GetTimerManager().SetTimer(
            HeartbeatTimerHandle,
            HeartbeatDelegate,
            HeartbeatCheckInterval,
            true
        );
    }
}

void FPlayflowLobbySSEManagerImpl::HandleSSEMessage(const FString& Message)
{
    FPlayflowLobbyResponse LobbyState;

    if (UPlayflowAPIHelpers::ParseLobbyResponse(Message, LobbyState))
    {
        LobbyUpdatedDelegate.Broadcast(LobbyState);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        const TSharedPtr<FJsonObject>* LobbyObject = nullptr;
        if (JsonObject->TryGetObjectField(TEXT("lobby"), LobbyObject) && LobbyObject->IsValid())
        {
            FString NestedJson;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NestedJson);
            FJsonSerializer::Serialize(LobbyObject->ToSharedRef(), Writer);

            if (UPlayflowAPIHelpers::ParseLobbyResponse(NestedJson, LobbyState))
            {
                LobbyUpdatedDelegate.Broadcast(LobbyState);
                return;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to parse lobby message"));
}

void FPlayflowLobbySSEManagerImpl::HandleSSEError(const FString& ErrorMessage, int32 ErrorCode)
{
    UE_LOG(LogTemp, Error, TEXT("Playflow SSE Error: %s (Code: %d)"), *ErrorMessage, ErrorCode);
    SSEErrorDelegate.Broadcast(ErrorMessage, ErrorCode);
}

void FPlayflowLobbySSEManagerImpl::HandleSSEDisconnected()
{
    UE_LOG(LogTemp, Warning, TEXT("Playflow SSE: Disconnected"));

    if (bShouldReconnect)
    {
        if (ReconnectAttempts < MaxReconnectAttempts)
        {
            AttemptReconnect();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Playflow SSE: Max reconnection attempts reached"));
            HandleSSEError(TEXT("Max reconnection attempts reached"), 0);
        }
    }
}

void FPlayflowLobbySSEManagerImpl::AttemptReconnect()
{
    ReconnectAttempts++;
    float Delay = (ReconnectAttempts == 1) ? 0.1f : (ReconnectDelay * ReconnectAttempts);

    UWorld* World = GEngine ? GEngine->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot get world for reconnect"));
        HandleSSEError(TEXT("World context unavailable"), 0);
        return;
    }

    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([this]()
        {
            SubscribeToLobbyUpdates(CurrentLobbyId, CurrentPlayerId, CurrentConfigName, CurrentApiKey);
        });

    World->GetTimerManager().SetTimer(
        ReconnectTimerHandle,
        TimerDelegate,
        Delay,
        false
    );
}

void FPlayflowLobbySSEManagerImpl::CheckHeartbeat()
{
    if (!bIsSubscribed)
        return;
}

UPlayflowLobbySSEManager* UPlayflowLobbySSEManager::Instance = nullptr;

UPlayflowLobbySSEManager::UPlayflowLobbySSEManager()
{
    ManagerImpl = new FPlayflowLobbySSEManagerImpl();
    Instance = this;

    ManagerImpl->OnLobbyUpdated().AddLambda([this](const FPlayflowLobbyResponse& LobbyData)
        {
            OnLobbyUpdated.Broadcast(LobbyData);
        });

    ManagerImpl->OnSSEError().AddLambda([this](const FString& ErrorMessage, int32 ErrorCode)
        {
            OnSSEError.Broadcast(ErrorMessage, ErrorCode);
        });
}

UPlayflowLobbySSEManager::~UPlayflowLobbySSEManager()
{
    if (ManagerImpl)
    {
        delete ManagerImpl;
        ManagerImpl = nullptr;
    }

    if (Instance == this)
    {
        Instance = nullptr;
    }
}

void UPlayflowLobbySSEManager::Tick(float DeltaTime)
{
    if (ManagerImpl)
    {
        ManagerImpl->ProcessCallbacks();
    }
}

UPlayflowLobbySSEManager* UPlayflowLobbySSEManager::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlayflowLobbySSEManager>();
        Instance->AddToRoot();
    }
    return Instance;
}

void UPlayflowLobbySSEBlueprintLibrary::SubscribeToLobbyUpdates(
    UObject* WorldContextObject,
    const FString& LobbyId,
    const FString& PlayerId,
    const FString& ConfigName,
    const FString& ApiKey
)
{
    UPlayflowLobbySSEManager* Manager = GetLobbySSEManager();
    if (Manager && Manager->ManagerImpl)
    {
        Manager->ManagerImpl->SubscribeToLobbyUpdates(LobbyId, PlayerId, ConfigName, ApiKey);
    }
}

void UPlayflowLobbySSEBlueprintLibrary::UnsubscribeFromLobbyUpdates()
{
    UPlayflowLobbySSEManager* Manager = GetLobbySSEManager();
    if (Manager && Manager->ManagerImpl)
    {
        Manager->ManagerImpl->UnsubscribeFromLobbyUpdates();
    }
}

bool UPlayflowLobbySSEBlueprintLibrary::IsSubscribed()
{
    UPlayflowLobbySSEManager* Manager = GetLobbySSEManager();
    return Manager && Manager->ManagerImpl && Manager->ManagerImpl->IsSubscribed();
}

UPlayflowLobbySSEManager* UPlayflowLobbySSEBlueprintLibrary::GetLobbySSEManager()
{
    return UPlayflowLobbySSEManager::GetInstance();
}

void UPlayflowLobbySSEBlueprintLibrary::TickSSEManager(float DeltaTime)
{
    UPlayflowLobbySSEManager* Manager = GetLobbySSEManager();
    if (Manager)
    {
        Manager->Tick(DeltaTime);
    }
}