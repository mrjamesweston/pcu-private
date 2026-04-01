#pragma once

#include "CoreMinimal.h"
#include "PlayflowTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayflowRegion : uint8
{
    UsEast UMETA(DisplayName = "US East"),
    UsWest UMETA(DisplayName = "US West"),
    EuNorth UMETA(DisplayName = "Sweden"),
    EuWest UMETA(DisplayName = "France"),
    EuUK UMETA(DisplayName = "London"),
    ApSouth UMETA(DisplayName = "India"),
    Sea UMETA(DisplayName = "Singapore"),
    Ea UMETA(DisplayName = "Hong Kong"),
    ApNorth UMETA(DisplayName = "Japan"),
    ApSoutheast UMETA(DisplayName = "Australia"),
    SouthAfrica UMETA(DisplayName = "South Africa"),
    SouthAmericaBrazil UMETA(DisplayName = "Brazil"),
    SouthAmericaChile UMETA(DisplayName = "Chile")
};

static FString PlayflowRegionToString(EPlayflowRegion Region)
{
    switch (Region)
    {
    case EPlayflowRegion::UsEast: return TEXT("us-east");
    case EPlayflowRegion::UsWest: return TEXT("us-west");
    case EPlayflowRegion::EuNorth: return TEXT("eu-north");
    case EPlayflowRegion::EuWest: return TEXT("eu-west");
    case EPlayflowRegion::EuUK: return TEXT("eu-uk");
    case EPlayflowRegion::ApSouth: return TEXT("ap-south");
    case EPlayflowRegion::Sea: return TEXT("sea");
    case EPlayflowRegion::Ea: return TEXT("ea");
    case EPlayflowRegion::ApNorth: return TEXT("ap-north");
    case EPlayflowRegion::ApSoutheast: return TEXT("ap-southeast");
    case EPlayflowRegion::SouthAfrica: return TEXT("south-africa");
    case EPlayflowRegion::SouthAmericaBrazil: return TEXT("south-america-brazil");
    case EPlayflowRegion::SouthAmericaChile: return TEXT("south-america-chile");
    default: return TEXT("us-east");
    }
}

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowNetworkPort
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    int32 InternalPort;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    int32 ExternalPort;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Protocol;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Host;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    bool bTlsEnabled;

    FPlayflowNetworkPort()
        : InternalPort(0)
        , ExternalPort(0)
        , bTlsEnabled(false)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowGameServer
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString InstanceId;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    TArray<FPlayflowNetworkPort> NetworkPorts;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Status;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString StartupArgs;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString ServiceType;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString ComputeSize;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Region;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString VersionTag;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString StartedAt;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString StoppedAt;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString CustomData;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    int32 Ttl;

    FPlayflowGameServer()
        : Ttl(0)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowPlayerStateEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playflow|PlayerState")
    FString Key;

    // Store the value as a JSON string to support any type (string, number, bool, object, array)
    // Examples:
    // - String: "\"ready\""
    // - Number: "100"
    // - Bool: "true"
    // - Object: "{\"score\":50,\"kills\":10}"
    // - Array: "[1,2,3]"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playflow|PlayerState")
    FString ValueAsJsonString;

    FPlayflowPlayerStateEntry()
        : Key(TEXT(""))
        , ValueAsJsonString(TEXT(""))
    {
    }

    FPlayflowPlayerStateEntry(FString InKey, FString InValue)
        : Key(InKey)
        , ValueAsJsonString(InValue)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowLobbyResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString Host;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    int32 MaxPlayers;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    int32 CurrentPlayers;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString Region;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString Status;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    bool IsPrivate;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    bool UseInviteCode;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString InviteCode;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    bool bAllowLateJoin;

    // PLAYER LIST
    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    TArray<FString> Players;

    // Custom lobby settings as JSON string
    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString Settings;

    // Real-time state data as JSON string
    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString LobbyStateRealTime;

    // GAME SERVER INFO
    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FPlayflowGameServer GameServer;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString CreatedAt;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    FString UpdatedAt;

    // MATCHMAKING
    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString MatchmakingMode;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString MatchmakingStartedAt;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString MatchmakingTicketId;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString MatchmakingData;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Lobby")
    int32 Timeout;

    FPlayflowLobbyResponse()
        : MaxPlayers(0)
        , CurrentPlayers(0)
        , IsPrivate(false)
        , UseInviteCode(false)
        , bAllowLateJoin(false)
        , Timeout(0)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowMatchConfiguration
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    int32 TeamsCount;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    int32 PlayersPerTeam;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    int32 MinPlayersPerTeam;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    int32 Timeout;

    FPlayflowMatchConfiguration()
        : TeamsCount(1)
        , PlayersPerTeam(1)
        , MinPlayersPerTeam(1)
        , Timeout(300)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowPlayerState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString PlayerID;

    // All player state data as key-value pairs (e.g., "mmr"="1500", "role"="top", "level"="10")
    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TMap<FString, FString> StateData;

    FPlayflowPlayerState()
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowTeamLobby
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString LobbyID;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString LobbyName;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString Host;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    int32 PlayerCount;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FString> Players;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FPlayflowPlayerState> PlayerStates;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString LobbySettingsJson;

    FPlayflowTeamLobby()
        : PlayerCount(0)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowTeam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    int32 TeamID;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FPlayflowTeamLobby> Lobbies;

    FPlayflowTeam()
        : TeamID(0)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowServerConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString InstanceId;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Region;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString ApiKey;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString StartupArgs;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString VersionTag;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString MatchId;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Server")
    FString Arguments;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString LobbyID;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString MatchmakingMode;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FString> LobbyIDs;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FString> AllPlayers;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FPlayflowMatchConfiguration MatchConfiguration;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FPlayflowTeam> Teams;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString LobbySettingsJson;

    FPlayflowServerConfig()
        : bSuccess(false)
    {
    }
};

USTRUCT(BlueprintType)
struct PLAYFLOW_API FPlayflowRolePlayers
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    FString Role;

    UPROPERTY(BlueprintReadOnly, Category = "Playflow|Matchmaking")
    TArray<FString> PlayerIDs;

    FPlayflowRolePlayers()
        : Role(TEXT(""))
    {
    }
};

UENUM(BlueprintType)
enum class EPlayflowLobbyStatus : uint8
{
    Waiting UMETA(DisplayName = "Waiting"),
    InGame UMETA(DisplayName = "In Game")
};