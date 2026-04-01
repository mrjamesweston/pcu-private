#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "PlayflowTypes.h"
#include "Delegates/Delegate.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TimerManager.h"
#include "PlayflowLobbySSEManager.generated.h"

typedef TMulticastDelegate<void(const FPlayflowLobbyResponse&)> FOnLobbyResponseUpdated;
typedef TMulticastDelegate<void(const FString&, int32)> FOnSSEError;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUpdatedEvent, const FPlayflowLobbyResponse&, LobbyData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSSEErrorEvent, const FString&, ErrorMessage, int32, ErrorCode);

class FPlayflowLobbySSEManagerImpl
{
public:
    FPlayflowLobbySSEManagerImpl();
    ~FPlayflowLobbySSEManagerImpl();

    void SubscribeToLobbyUpdates(
        const FString& LobbyId,
        const FString& PlayerId,
        const FString& ConfigName,
        const FString& ApiKey
    );

    void UnsubscribeFromLobbyUpdates();
    bool IsSubscribed() const;

    void ProcessCallbacks();

    FOnLobbyResponseUpdated& OnLobbyUpdated() { return LobbyUpdatedDelegate; }
    FOnSSEError& OnSSEError() { return SSEErrorDelegate; }

private:
    void HandleSSEError(const FString& ErrorMessage, int32 ErrorCode);
    void HandleSSEMessage(const FString& Message);
    void HandleSSEConnected();
    void HandleSSEDisconnected();
    void AttemptReconnect();
    void QueueCallback(TFunction<void()> Callback);
    void ProcessSSEBuffer();

    FString CurrentLobbyId;
    FString CurrentPlayerId;
    FString CurrentConfigName;
    FString CurrentApiKey;

    bool bIsSubscribed;

    FOnLobbyResponseUpdated LobbyUpdatedDelegate;
    FOnSSEError SSEErrorDelegate;

    FHttpRequestPtr CurrentRequest;
    int32 LastProcessedBytes;

    FCriticalSection CallbackLock;
    TArray<TFunction<void()>> PendingCallbacks;

    FTimerHandle ReconnectTimerHandle;
    int32 ReconnectAttempts;
    bool bShouldReconnect;
    static constexpr int32 MaxReconnectAttempts = 5;
    static constexpr float ReconnectDelay = 3.0f;

    FString PartialData;
    FString LastEventID;

    // Heartbeat tracking to detect stale connections
    FTimerHandle HeartbeatTimerHandle;
    double LastDataReceivedTime;
    static constexpr float HeartbeatCheckInterval = 30.0f; // Check every 30 seconds
    static constexpr float MaxIdleTime = 90.0f; // Reconnect if no data for 90 seconds

    void CheckHeartbeat();

    friend class UPlayflowLobbySSEManager;
};

UCLASS()
class PLAYFLOW_API UPlayflowLobbySSEManager : public UObject
{
    GENERATED_BODY()

public:
    UPlayflowLobbySSEManager();
    virtual ~UPlayflowLobbySSEManager();

    UPROPERTY(BlueprintAssignable, Category = "Playflow|Lobby SSE")
    FOnLobbyUpdatedEvent OnLobbyUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Playflow|Lobby SSE")
    FOnSSEErrorEvent OnSSEError;

    static UPlayflowLobbySSEManager* GetInstance();

    void Tick(float DeltaTime);

    FPlayflowLobbySSEManagerImpl* ManagerImpl;

private:
    static UPlayflowLobbySSEManager* Instance;

    friend class UPlayflowLobbySSEBlueprintLibrary;
};

UCLASS()
class PLAYFLOW_API UPlayflowLobbySSEBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Playflow|Lobby SSE", meta = (WorldContext = "WorldContextObject"))
    static void SubscribeToLobbyUpdates(UObject* WorldContextObject, const FString& LobbyId, const FString& PlayerId, const FString& ConfigName, const FString& ApiKey);

    UFUNCTION(BlueprintCallable, Category = "Playflow|Lobby SSE")
    static void UnsubscribeFromLobbyUpdates();

    UFUNCTION(BlueprintCallable, Category = "Playflow|Lobby SSE")
    static bool IsSubscribed();

    UFUNCTION(BlueprintCallable, Category = "Playflow|Lobby SSE")
    static UPlayflowLobbySSEManager* GetLobbySSEManager();

    UFUNCTION(BlueprintCallable, Category = "Playflow|Lobby SSE")
    static void TickSSEManager(float DeltaTime);
};