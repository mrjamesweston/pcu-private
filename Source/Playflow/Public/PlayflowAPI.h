#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "PlayflowTypes.h"
#include "PlayflowAPI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateLobbyComplete, FPlayflowLobbyResponse, Response);

UCLASS()
class PLAYFLOW_API UPlayflowCreateLobby : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnCreateLobbyComplete OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnCreateLobbyComplete OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowCreateLobby* CreateLobby(
        UObject* WorldContextObject,
        FString APIKey,
        FString ConfigName,
        FString LobbyName,
        int32 MaxPlayers,
        bool bIsPrivate,
        bool bUseInviteCode,
        bool bAllowLateJoin,
        EPlayflowRegion Region,
        FString Host
    );

    virtual void Activate() override;

private:

    UObject* WorldContextObject;
    FString APIKey;
    FString ConfigName;
    FString LobbyName;
    int32 MaxPlayers;
    bool bIsPrivate;
    bool bUseInviteCode;
    bool bAllowLateJoin;
    EPlayflowRegion Region;
    FString Host;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetGameServerInfo, FString, Link, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowGetGameServerInfo : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Game Server Ready"))
    FOnGetGameServerInfo OnSuccess;

    UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Game Server Not Ready"))
    FOnGetGameServerInfo OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowGetGameServerInfo* GetGameServerInfo(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyHeartbeat, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowLobbyHeartbeat : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnLobbyHeartbeat OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnLobbyHeartbeat OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowLobbyHeartbeat* LobbyHeartbeat(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName,
        FString PlayerID
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;
    FString PlayerID;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnListPlayers, const TArray<FString>&, Players, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowListPlayers : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnListPlayers OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnListPlayers OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowListPlayers* ListPlayers(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGetLobby, FPlayflowLobbyResponse, LobbyData);

UCLASS()
class PLAYFLOW_API UPlayflowGetLobby : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnGetLobby OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnGetLobby OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowGetLobby* GetLobby(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaveLobby, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowLeaveLobby : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnLeaveLobby OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnLeaveLobby OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowLeaveLobby* LeaveLobby(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString PlayerID,
        FString ConfigName,
        FString RequesterID,
        bool bIsKick
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString PlayerID;
    FString ConfigName;
    FString RequesterID;
    bool bIsKick;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinLobbyByCode, FPlayflowLobbyResponse, LobbyData);

UCLASS()
class PLAYFLOW_API UPlayflowJoinLobbyByCode : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnJoinLobbyByCode OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnJoinLobbyByCode OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowJoinLobbyByCode* JoinLobbyByCode(
        UObject* WorldContextObject,
        FString APIKey,
        FString InviteCode,
        FString PlayerID,
        FString ConfigName
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString InviteCode;
    FString PlayerID;
    FString ConfigName;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartMatchmaking, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowStartMatchmaking : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnStartMatchmaking OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnStartMatchmaking OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Matchmaking")
    static UPlayflowStartMatchmaking* StartMatchmaking(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName,
        FString RequesterID,
        FString MatchmakingMode
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;
    FString RequesterID;
    FString MatchmakingMode;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStopMatchmaking, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowStopMatchmaking : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnStopMatchmaking OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnStopMatchmaking OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Matchmaking")
    static UPlayflowStopMatchmaking* StopMatchmaking(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName,
        FString RequesterID
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;
    FString RequesterID;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdatePlayerState, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowUpdatePlayerState : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnUpdatePlayerState OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnUpdatePlayerState OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|PlayerState")
    static UPlayflowUpdatePlayerState* UpdatePlayerState(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        FString ConfigName,
        FString RequesterID,
        const TArray<FPlayflowPlayerStateEntry>& PlayerState,
        FString TargetPlayerID = TEXT("")
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    FString ConfigName;
    FString RequesterID;
    TArray<FPlayflowPlayerStateEntry> PlayerState;
    FString TargetPlayerID;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadServerConfig, FPlayflowServerConfig, ServerConfig);

UCLASS()
class PLAYFLOW_API UPlayflowReadServerConfig : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnReadServerConfig OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnReadServerConfig OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Server")
    static UPlayflowReadServerConfig* ReadServerConfig(UObject* WorldContextObject);

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
};

UCLASS()
class PLAYFLOW_API UPlayflowUtilities : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Playflow|Utilities")
    static TArray<FPlayflowRolePlayers> SortPlayersByRole(const TArray<FPlayflowTeam>& Teams);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Playflow|Utilities")
    static TArray<FString> GetPlayersByRole(const TArray<FPlayflowTeam>& Teams, FString Role);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Playflow|Utilities")
    static FPlayflowPlayerState GetPlayerState(const TArray<FPlayflowTeam>& Teams, FString PlayerID, bool& bFound);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Playflow|Utilities")
    static FString GetPlayerRole(const TArray<FPlayflowTeam>& Teams, FString PlayerID);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Playflow|Utilities")
    static int32 GetPlayerTeamID(const TArray<FPlayflowTeam>& Teams, FString PlayerID);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Playflow|Utilities")
    static FString GetPlayerStateValue(const FPlayflowPlayerState& PlayerState, FString Key, FString DefaultValue = TEXT(""));
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateLobbyRegion, FString, ErrorMessage);

UCLASS()
class PLAYFLOW_API UPlayflowUpdateLobbyRegion : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnUpdateLobbyRegion OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnUpdateLobbyRegion OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowUpdateLobbyRegion* UpdateLobbyRegion(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyID,
        EPlayflowRegion Region,
        FString RequesterID,
        FString ConfigName
    );

    virtual void Activate() override;

private:
    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyID;
    EPlayflowRegion Region;
    FString RequesterID;
    FString ConfigName;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    static void UpdateLobbyStatus(
        const FString& LobbyId,
        const FString& RequesterId,
        EPlayflowLobbyStatus Status,
        const FString& ApiKey,
        TFunction<void(const FString&)> OnSuccess,
        TFunction<void(const FString&, int32)> OnError
    );
};

UCLASS()
class PLAYFLOW_API UPlayflowAPIHelpers : public UObject
{
    GENERATED_BODY()

public:

    UFUNCTION()
    static bool ParseLobbyResponse(const FString& JsonString, FPlayflowLobbyResponse& OutResponse);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateLobbyStatusComplete, FPlayflowLobbyResponse, Response);

UCLASS()
class PLAYFLOW_API UPlayflowUpdateLobbyStatus : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnUpdateLobbyStatusComplete OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnUpdateLobbyStatusComplete OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Playflow|Lobby")
    static UPlayflowUpdateLobbyStatus* UpdateLobbyStatus(
        UObject* WorldContextObject,
        FString APIKey,
        FString LobbyId,
        FString RequesterId,
        FString ConfigName,
        EPlayflowLobbyStatus Status
    );

    virtual void Activate() override;

private:

    UObject* WorldContextObject;
    FString APIKey;
    FString LobbyId;
    FString RequesterId;
    FString ConfigName;
    EPlayflowLobbyStatus Status;

};