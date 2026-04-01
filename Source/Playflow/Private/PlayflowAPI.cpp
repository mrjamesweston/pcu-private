#include "PlayflowAPI.h"
#include "PlayflowTypes.h"
#include "HttpModule.h"
#include "Json.h"
#include "JsonUtilities.h"


UPlayflowCreateLobby* UPlayflowCreateLobby::CreateLobby(
    UObject* WorldContextObject,
    FString APIKey,
    FString ConfigName,
    FString LobbyName,
    int32 MaxPlayers,
    bool bIsPrivate,
    bool bUseInviteCode,
    bool bAllowLateJoin,
    EPlayflowRegion Region,
    FString Host)
{
    UPlayflowCreateLobby* Node = NewObject<UPlayflowCreateLobby>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->ConfigName = ConfigName;
    Node->LobbyName = LobbyName;
    Node->MaxPlayers = MaxPlayers;
    Node->bIsPrivate = bIsPrivate;
    Node->bUseInviteCode = bUseInviteCode;
    Node->bAllowLateJoin = bAllowLateJoin;
    Node->Region = Region;
    Node->Host = Host;

    return Node;
}

void UPlayflowCreateLobby::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies?name=%s"), *ConfigName);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("name"), LobbyName);
    JsonObject->SetNumberField(TEXT("maxPlayers"), MaxPlayers);
    JsonObject->SetBoolField(TEXT("isPrivate"), bIsPrivate);
    JsonObject->SetBoolField(TEXT("useInviteCode"), bUseInviteCode);
    JsonObject->SetBoolField(TEXT("allowLateJoin"), bAllowLateJoin);
    JsonObject->SetStringField(TEXT("region"), PlayflowRegionToString(Region));
    JsonObject->SetStringField(TEXT("host"), Host);
    TSharedPtr<FJsonObject> SettingsObject = MakeShareable(new FJsonObject);
    JsonObject->SetObjectField(TEXT("settings"), SettingsObject);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(JsonString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowCreateLobby::OnResponseReceived);

    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Creating lobby - URL: %s"), *URL);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Request Body: %s"), *JsonString);
}

void UPlayflowCreateLobby::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FPlayflowLobbyResponse LobbyResponse;

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {

                LobbyResponse.Id = JsonObject->GetStringField(TEXT("id"));

                LobbyResponse.Name = JsonObject->GetStringField(TEXT("name"));
                LobbyResponse.Host = JsonObject->GetStringField(TEXT("host"));
                LobbyResponse.MaxPlayers = JsonObject->GetIntegerField(TEXT("maxPlayers"));
                LobbyResponse.CurrentPlayers = JsonObject->GetIntegerField(TEXT("currentPlayers"));
                LobbyResponse.Region = JsonObject->GetStringField(TEXT("region"));
                LobbyResponse.Status = JsonObject->GetStringField(TEXT("status"));
                LobbyResponse.IsPrivate = JsonObject->GetBoolField(TEXT("isPrivate"));
                LobbyResponse.UseInviteCode = JsonObject->GetBoolField(TEXT("useInviteCode"));
                LobbyResponse.bAllowLateJoin = JsonObject->GetBoolField(TEXT("allowLateJoin"));
                if (JsonObject->HasField(TEXT("inviteCode")))
                {
                    LobbyResponse.InviteCode = JsonObject->GetStringField(TEXT("inviteCode"));
                }

                if (JsonObject->HasField(TEXT("timeout")))
                {
                    LobbyResponse.Timeout = JsonObject->GetIntegerField(TEXT("timeout"));
                }
                if (JsonObject->HasField(TEXT("players")))
                {
                    const TArray<TSharedPtr<FJsonValue>>* PlayersArray;
                    if (JsonObject->TryGetArrayField(TEXT("players"), PlayersArray))
                    {
                        for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
                        {
                            LobbyResponse.Players.Add(PlayerValue->AsString());
                        }
                    }
                }
                if (JsonObject->HasField(TEXT("createdAt")))
                {
                    LobbyResponse.CreatedAt = JsonObject->GetStringField(TEXT("createdAt"));
                }
                if (JsonObject->HasField(TEXT("updatedAt")))
                {
                    LobbyResponse.UpdatedAt = JsonObject->GetStringField(TEXT("updatedAt"));
                }
                if (JsonObject->HasField(TEXT("matchmakingMode")))
                {
                    LobbyResponse.MatchmakingMode = JsonObject->GetStringField(TEXT("matchmakingMode"));
                }
                if (JsonObject->HasField(TEXT("matchmakingStartedAt")))
                {
                    LobbyResponse.MatchmakingStartedAt = JsonObject->GetStringField(TEXT("matchmakingStartedAt"));
                }
                if (JsonObject->HasField(TEXT("matchmakingTicketId")))
                {
                    LobbyResponse.MatchmakingTicketId = JsonObject->GetStringField(TEXT("matchmakingTicketId"));
                }
                if (JsonObject->HasField(TEXT("gameServer")))
                {
                    const TSharedPtr<FJsonObject>* GameServerObj;
                    if (JsonObject->TryGetObjectField(TEXT("gameServer"), GameServerObj))
                    {
                        LobbyResponse.GameServer.InstanceId = (*GameServerObj)->GetStringField(TEXT("instance_id"));
                        LobbyResponse.GameServer.Name = (*GameServerObj)->GetStringField(TEXT("name"));
                        LobbyResponse.GameServer.Status = (*GameServerObj)->GetStringField(TEXT("status"));
                        LobbyResponse.GameServer.Region = (*GameServerObj)->GetStringField(TEXT("region"));

                        if ((*GameServerObj)->HasField(TEXT("startup_args")))
                        {
                            LobbyResponse.GameServer.StartupArgs = (*GameServerObj)->GetStringField(TEXT("startup_args"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("service_type")))
                        {
                            LobbyResponse.GameServer.ServiceType = (*GameServerObj)->GetStringField(TEXT("service_type"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("compute_size")))
                        {
                            LobbyResponse.GameServer.ComputeSize = (*GameServerObj)->GetStringField(TEXT("compute_size"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("version_tag")))
                        {
                            LobbyResponse.GameServer.VersionTag = (*GameServerObj)->GetStringField(TEXT("version_tag"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("started_at")))
                        {
                            LobbyResponse.GameServer.StartedAt = (*GameServerObj)->GetStringField(TEXT("started_at"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("ttl")))
                        {
                            LobbyResponse.GameServer.Ttl = (*GameServerObj)->GetIntegerField(TEXT("ttl"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("network_ports")))
                        {
                            const TArray<TSharedPtr<FJsonValue>>* PortsArray;
                            if ((*GameServerObj)->TryGetArrayField(TEXT("network_ports"), PortsArray))
                            {
                                for (const TSharedPtr<FJsonValue>& PortValue : *PortsArray)
                                {
                                    const TSharedPtr<FJsonObject>* PortObj;
                                    if (PortValue->TryGetObject(PortObj))
                                    {
                                        FPlayflowNetworkPort Port;
                                        Port.Name = (*PortObj)->GetStringField(TEXT("name"));
                                        Port.InternalPort = (*PortObj)->GetIntegerField(TEXT("internal_port"));
                                        Port.ExternalPort = (*PortObj)->GetIntegerField(TEXT("external_port"));
                                        Port.Protocol = (*PortObj)->GetStringField(TEXT("protocol"));
                                        Port.Host = (*PortObj)->GetStringField(TEXT("host"));
                                        Port.bTlsEnabled = (*PortObj)->GetBoolField(TEXT("tls_enabled"));

                                        LobbyResponse.GameServer.NetworkPorts.Add(Port);
                                    }
                                }
                            }
                        }
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("Playflow: Lobby created successfully! ID: %s"), *LobbyResponse.Id);
                OnSuccess.Broadcast(LobbyResponse);
                return;
            }
        }
        TSharedPtr<FJsonObject> ErrorJsonObj;

        TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

        if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

        {

            LobbyResponse.ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

        }

        else

        {

            LobbyResponse.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

        }
    }
    else
    {
        LobbyResponse.ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Request failed"));
    }

    OnFailure.Broadcast(LobbyResponse);
}


UPlayflowGetGameServerInfo* UPlayflowGetGameServerInfo::GetGameServerInfo(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName)
{
    UPlayflowGetGameServerInfo* Node = NewObject<UPlayflowGetGameServerInfo>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;

    return Node;
}

void UPlayflowGetGameServerInfo::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s?name=%s"), *LobbyID, *ConfigName);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowGetGameServerInfo::OnResponseReceived);
    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Getting game server info - URL: %s"), *URL);
}

void UPlayflowGetGameServerInfo::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString Link = TEXT("");
    FString ErrorMessage = TEXT("");
    bool bGameServerReady = false;

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (!JsonObject->HasField(TEXT("gameServer")))
                {
                    ErrorMessage = TEXT("No gameServer field in lobby response - server not deployed yet");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }

                const TSharedPtr<FJsonObject>* GameServerObj;
                if (!JsonObject->TryGetObjectField(TEXT("gameServer"), GameServerObj) || !GameServerObj->IsValid())
                {
                    ErrorMessage = TEXT("Game server object is null - server not deployed yet");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                if (!(*GameServerObj)->HasField(TEXT("status")))
                {
                    ErrorMessage = TEXT("Game server has no status field");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }

                FString ServerStatus = (*GameServerObj)->GetStringField(TEXT("status"));
                if (!ServerStatus.Equals(TEXT("running"), ESearchCase::IgnoreCase))
                {
                    ErrorMessage = FString::Printf(TEXT("Game server status is '%s' (not running)"), *ServerStatus);
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                if (!(*GameServerObj)->HasField(TEXT("network_ports")))
                {
                    ErrorMessage = TEXT("Game server has no network_ports field");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }

                const TArray<TSharedPtr<FJsonValue>>* PortsArray;
                if (!(*GameServerObj)->TryGetArrayField(TEXT("network_ports"), PortsArray))
                {
                    ErrorMessage = TEXT("Failed to parse network_ports array");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                if (PortsArray->Num() == 0)
                {
                    ErrorMessage = TEXT("No network ports available in game server");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                const TSharedPtr<FJsonObject>* FirstPortObj;
                if (!(*PortsArray)[0]->TryGetObject(FirstPortObj))
                {
                    ErrorMessage = TEXT("Failed to parse first network port");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                if (!(*FirstPortObj)->HasField(TEXT("host")))
                {
                    ErrorMessage = TEXT("Network port has no host field");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                FString Host = (*FirstPortObj)->GetStringField(TEXT("host"));
                if (Host.IsEmpty())
                {
                    ErrorMessage = TEXT("Host IP address is empty");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                if (!(*FirstPortObj)->HasField(TEXT("external_port")))
                {
                    ErrorMessage = TEXT("Network port has no external_port field");
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }

                int32 ExternalPort = (*FirstPortObj)->GetIntegerField(TEXT("external_port"));
                if (ExternalPort <= 0 || ExternalPort > 65535)
                {
                    ErrorMessage = FString::Printf(TEXT("Invalid external port: %d"), ExternalPort);
                    UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ErrorMessage);
                    OnFailure.Broadcast(TEXT(""), ErrorMessage);
                    return;
                }
                Link = FString::Printf(TEXT("%s:%d"), *Host, ExternalPort);
                bGameServerReady = true;

                UE_LOG(LogTemp, Log, TEXT("Playflow: Game server is READY! Link: %s"), *Link);
                OnSuccess.Broadcast(Link, TEXT(""));
                return;
            }
            else
            {
                ErrorMessage = TEXT("Failed to parse JSON response");
            }
        }
        else
        {
            TSharedPtr<FJsonObject> ErrorJsonObj;

            TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

            {

                ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

            }

            else

            {

                ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Game server NOT ready - %s"), *ErrorMessage);
    OnFailure.Broadcast(TEXT(""), ErrorMessage);
}


UPlayflowLobbyHeartbeat* UPlayflowLobbyHeartbeat::LobbyHeartbeat(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName,
    FString PlayerID)
{
    UPlayflowLobbyHeartbeat* Node = NewObject<UPlayflowLobbyHeartbeat>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;
    Node->PlayerID = PlayerID;

    return Node;
}

void UPlayflowLobbyHeartbeat::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s/heartbeat?name=%s"), *LobbyID, *ConfigName);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("playerId"), PlayerID);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(JsonString);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowLobbyHeartbeat::OnResponseReceived);
    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Sending lobby heartbeat - URL: %s"), *URL);
}

void UPlayflowLobbyHeartbeat::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Heartbeat Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Heartbeat Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("success")))
                {
                    bool bSuccess = JsonObject->GetBoolField(TEXT("success"));

                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Heartbeat successful"));
                        OnSuccess.Broadcast(TEXT(""));
                        return;
                    }
                    else
                    {
                        ErrorMessage = TEXT("Heartbeat returned success: false");
                    }
                }
                else
                {
                    ErrorMessage = TEXT("No success field in heartbeat response");
                }
            }
            else
            {
                ErrorMessage = TEXT("Failed to parse JSON response");
            }
        }
        else
        {
            TSharedPtr<FJsonObject> ErrorJsonObj;

            TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

            {

                ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

            }

            else

            {

                ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Heartbeat request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Heartbeat failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(ErrorMessage);
}


UPlayflowListPlayers* UPlayflowListPlayers::ListPlayers(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName)
{
    UPlayflowListPlayers* Node = NewObject<UPlayflowListPlayers>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;

    return Node;
}

void UPlayflowListPlayers::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s/players?name=%s"), *LobbyID, *ConfigName);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowListPlayers::OnResponseReceived);
    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Listing players - URL: %s"), *URL);
}

void UPlayflowListPlayers::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    TArray<FString> Players;
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: List Players Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: List Players Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TArray<TSharedPtr<FJsonValue>> JsonArray;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonArray))
            {
                for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
                {
                    Players.Add(JsonValue->AsString());
                }

                UE_LOG(LogTemp, Log, TEXT("Playflow: Listed %d players"), Players.Num());
                OnSuccess.Broadcast(Players, TEXT(""));
                return;
            }
            else
            {
                ErrorMessage = TEXT("Failed to parse JSON array response");
            }
        }
        else
        {
            TSharedPtr<FJsonObject> ErrorJsonObj;

            TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

            {

                ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

            }

            else

            {

                ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: List players request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: List players failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(Players, ErrorMessage);
}


UPlayflowGetLobby* UPlayflowGetLobby::GetLobby(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName)
{
    UPlayflowGetLobby* Node = NewObject<UPlayflowGetLobby>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;

    return Node;
}

void UPlayflowGetLobby::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s?name=%s"), *LobbyID, *ConfigName);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowGetLobby::OnResponseReceived);

    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Getting lobby - URL: %s"), *URL);
}

void UPlayflowGetLobby::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FPlayflowLobbyResponse LobbyResponse;

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Get Lobby Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Get Lobby Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            if (UPlayflowAPIHelpers::ParseLobbyResponse(ResponseString, LobbyResponse))
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Got lobby successfully! ID: %s"), *LobbyResponse.Id);
                OnSuccess.Broadcast(LobbyResponse);
                return;
            }
            UE_LOG(LogTemp, Warning, TEXT("Playflow: ParseLobbyResponse failed for valid HTTP response"));
        }

        TSharedPtr<FJsonObject> ErrorJsonObj;


        TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);


        if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))


        {


            LobbyResponse.ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));


        }


        else


        {


            LobbyResponse.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);


        }
    }
    else
    {
        LobbyResponse.ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Get lobby request failed"));
    }

    OnFailure.Broadcast(LobbyResponse);
}



UPlayflowLeaveLobby* UPlayflowLeaveLobby::LeaveLobby(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString PlayerID,
    FString ConfigName)
{
    UPlayflowLeaveLobby* Node = NewObject<UPlayflowLeaveLobby>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->PlayerID = PlayerID;
    Node->ConfigName = ConfigName;

    return Node;
}

void UPlayflowLeaveLobby::Activate()
{
    FString URL = FString::Printf(
        TEXT("https://api.scale.computeflow.cloud/lobbies/%s/players/%s?name=%s&isKick=false"),
        *LobbyID,
        *PlayerID,
        *ConfigName
    );

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("DELETE"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowLeaveLobby::OnResponseReceived);
    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Leaving lobby - URL: %s"), *URL);
}

void UPlayflowLeaveLobby::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Leave Lobby Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Leave Lobby Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("success")))
                {
                    bool bSuccess = JsonObject->GetBoolField(TEXT("success"));

                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Player left lobby successfully"));
                        OnSuccess.Broadcast(TEXT(""));
                        return;
                    }
                    else
                    {
                        if (JsonObject->HasField(TEXT("message")))
                        {
                            ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                        }
                        else
                        {
                            ErrorMessage = TEXT("Leave lobby returned success: false");
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Player left lobby successfully (no success field)"));
                    OnSuccess.Broadcast(TEXT(""));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Player left lobby successfully (non-JSON response)"));
                OnSuccess.Broadcast(TEXT(""));
                return;
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("message")))
                {
                    ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                }
                else
                {
                    TSharedPtr<FJsonObject> ErrorJsonObj;

                    TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                    if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                    {

                        ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                    }

                    else

                    {

                        ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                    }
                }
            }
            else
            {
                TSharedPtr<FJsonObject> ErrorJsonObj;

                TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                {

                    ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                }

                else

                {

                    ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                }
            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Leave lobby request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Leave lobby failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(ErrorMessage);
}

UPlayflowKickPlayer* UPlayflowKickPlayer::KickPlayer(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString PlayerID,
    FString ConfigName,
    FString RequesterID)
{
    UPlayflowKickPlayer* Node = NewObject<UPlayflowKickPlayer>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->PlayerID = PlayerID;
    Node->ConfigName = ConfigName;
    Node->RequesterID = RequesterID;

    return Node;
}

void UPlayflowKickPlayer::Activate()
{
    FString URL = FString::Printf(
        TEXT("https://api.scale.computeflow.cloud/lobbies/%s/players/%s?name=%s&requesterId=%s&isKick=true"),
        *LobbyID,
        *PlayerID,
        *ConfigName,
        *RequesterID
    );

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("DELETE"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowKickPlayer::OnResponseReceived);
    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Kicking player from lobby - URL: %s"), *URL);
}

void UPlayflowKickPlayer::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Kick Player Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Kick Player Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("success")))
                {
                    bool bSuccess = JsonObject->GetBoolField(TEXT("success"));

                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Player kicked successfully"));
                        OnSuccess.Broadcast(TEXT(""));
                        return;
                    }
                    else
                    {
                        if (JsonObject->HasField(TEXT("message")))
                        {
                            ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                        }
                        else
                        {
                            ErrorMessage = TEXT("Kick player returned success: false");
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Player kicked successfully (no success field)"));
                    OnSuccess.Broadcast(TEXT(""));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Player kicked successfully (non-JSON response)"));
                OnSuccess.Broadcast(TEXT(""));
                return;
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("message")))
                {
                    ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                }
                else
                {
                    ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);
                }
            }
            else
            {
                ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);
            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Kick player request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Kick player failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(ErrorMessage);
}

UPlayflowJoinLobbyByCode* UPlayflowJoinLobbyByCode::JoinLobbyByCode(
    UObject* WorldContextObject,
    FString APIKey,
    FString InviteCode,
    FString PlayerID,
    FString ConfigName)
{
    UPlayflowJoinLobbyByCode* Node = NewObject<UPlayflowJoinLobbyByCode>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->InviteCode = InviteCode;
    Node->PlayerID = PlayerID;
    Node->ConfigName = ConfigName;

    return Node;
}

void UPlayflowJoinLobbyByCode::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/code/%s/players?name=%s"), *InviteCode, *ConfigName);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("playerId"), PlayerID);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(JsonString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowJoinLobbyByCode::OnResponseReceived);

    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Joining lobby by code - URL: %s"), *URL);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Request Body: %s"), *JsonString);
}

void UPlayflowJoinLobbyByCode::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FPlayflowLobbyResponse LobbyResponse;

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Join Lobby Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Join Lobby Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                LobbyResponse.Id = JsonObject->GetStringField(TEXT("id"));
                LobbyResponse.Name = JsonObject->GetStringField(TEXT("name"));
                LobbyResponse.Host = JsonObject->GetStringField(TEXT("host"));
                LobbyResponse.MaxPlayers = JsonObject->GetIntegerField(TEXT("maxPlayers"));
                LobbyResponse.CurrentPlayers = JsonObject->GetIntegerField(TEXT("currentPlayers"));
                LobbyResponse.Region = JsonObject->GetStringField(TEXT("region"));
                LobbyResponse.Status = JsonObject->GetStringField(TEXT("status"));
                LobbyResponse.IsPrivate = JsonObject->GetBoolField(TEXT("isPrivate"));
                LobbyResponse.UseInviteCode = JsonObject->GetBoolField(TEXT("useInviteCode"));
                LobbyResponse.bAllowLateJoin = JsonObject->GetBoolField(TEXT("allowLateJoin"));
                if (JsonObject->HasField(TEXT("inviteCode")))
                {
                    LobbyResponse.InviteCode = JsonObject->GetStringField(TEXT("inviteCode"));
                }

                if (JsonObject->HasField(TEXT("timeout")))
                {
                    LobbyResponse.Timeout = JsonObject->GetIntegerField(TEXT("timeout"));
                }
                if (JsonObject->HasField(TEXT("players")))
                {
                    const TArray<TSharedPtr<FJsonValue>>* PlayersArray;
                    if (JsonObject->TryGetArrayField(TEXT("players"), PlayersArray))
                    {
                        for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
                        {
                            LobbyResponse.Players.Add(PlayerValue->AsString());
                        }
                    }
                }
                if (JsonObject->HasField(TEXT("settings")))
                {
                    const TSharedPtr<FJsonObject>* SettingsObj;
                    if (JsonObject->TryGetObjectField(TEXT("settings"), SettingsObj))
                    {
                        FString SettingsString;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SettingsString);
                        FJsonSerializer::Serialize(SettingsObj->ToSharedRef(), Writer);
                        LobbyResponse.Settings = SettingsString;
                    }
                }
                if (JsonObject->HasField(TEXT("lobbyStateRealTime")))
                {
                    const TSharedPtr<FJsonObject>* StateObj;
                    if (JsonObject->TryGetObjectField(TEXT("lobbyStateRealTime"), StateObj))
                    {
                        FString StateString;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&StateString);
                        FJsonSerializer::Serialize(StateObj->ToSharedRef(), Writer);
                        LobbyResponse.LobbyStateRealTime = StateString;
                    }
                }
                if (JsonObject->HasField(TEXT("createdAt")))
                {
                    LobbyResponse.CreatedAt = JsonObject->GetStringField(TEXT("createdAt"));
                }
                if (JsonObject->HasField(TEXT("updatedAt")))
                {
                    LobbyResponse.UpdatedAt = JsonObject->GetStringField(TEXT("updatedAt"));
                }
                if (JsonObject->HasField(TEXT("matchmakingMode")))
                {
                    LobbyResponse.MatchmakingMode = JsonObject->GetStringField(TEXT("matchmakingMode"));
                }
                if (JsonObject->HasField(TEXT("matchmakingStartedAt")))
                {
                    LobbyResponse.MatchmakingStartedAt = JsonObject->GetStringField(TEXT("matchmakingStartedAt"));
                }
                if (JsonObject->HasField(TEXT("matchmakingTicketId")))
                {
                    LobbyResponse.MatchmakingTicketId = JsonObject->GetStringField(TEXT("matchmakingTicketId"));
                }
                if (JsonObject->HasField(TEXT("matchmakingData")))
                {
                    const TSharedPtr<FJsonObject>* MatchmakingDataObj;
                    if (JsonObject->TryGetObjectField(TEXT("matchmakingData"), MatchmakingDataObj))
                    {
                        FString MatchmakingDataString;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MatchmakingDataString);
                        FJsonSerializer::Serialize(MatchmakingDataObj->ToSharedRef(), Writer);
                        LobbyResponse.MatchmakingData = MatchmakingDataString;
                    }
                }
                if (JsonObject->HasField(TEXT("gameServer")))
                {
                    const TSharedPtr<FJsonObject>* GameServerObj;
                    if (JsonObject->TryGetObjectField(TEXT("gameServer"), GameServerObj))
                    {
                        LobbyResponse.GameServer.InstanceId = (*GameServerObj)->GetStringField(TEXT("instance_id"));
                        LobbyResponse.GameServer.Name = (*GameServerObj)->GetStringField(TEXT("name"));
                        LobbyResponse.GameServer.Status = (*GameServerObj)->GetStringField(TEXT("status"));
                        LobbyResponse.GameServer.Region = (*GameServerObj)->GetStringField(TEXT("region"));

                        if ((*GameServerObj)->HasField(TEXT("startup_args")))
                        {
                            LobbyResponse.GameServer.StartupArgs = (*GameServerObj)->GetStringField(TEXT("startup_args"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("service_type")))
                        {
                            LobbyResponse.GameServer.ServiceType = (*GameServerObj)->GetStringField(TEXT("service_type"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("compute_size")))
                        {
                            LobbyResponse.GameServer.ComputeSize = (*GameServerObj)->GetStringField(TEXT("compute_size"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("version_tag")))
                        {
                            LobbyResponse.GameServer.VersionTag = (*GameServerObj)->GetStringField(TEXT("version_tag"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("started_at")))
                        {
                            LobbyResponse.GameServer.StartedAt = (*GameServerObj)->GetStringField(TEXT("started_at"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("stopped_at")))
                        {
                            LobbyResponse.GameServer.StoppedAt = (*GameServerObj)->GetStringField(TEXT("stopped_at"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("ttl")))
                        {
                            LobbyResponse.GameServer.Ttl = (*GameServerObj)->GetIntegerField(TEXT("ttl"));
                        }
                        if ((*GameServerObj)->HasField(TEXT("custom_data")))
                        {
                            const TSharedPtr<FJsonObject>* CustomDataObj;
                            if ((*GameServerObj)->TryGetObjectField(TEXT("custom_data"), CustomDataObj))
                            {
                                FString CustomDataString;
                                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&CustomDataString);
                                FJsonSerializer::Serialize(CustomDataObj->ToSharedRef(), Writer);
                                LobbyResponse.GameServer.CustomData = CustomDataString;
                            }
                        }
                        if ((*GameServerObj)->HasField(TEXT("network_ports")))
                        {
                            const TArray<TSharedPtr<FJsonValue>>* PortsArray;
                            if ((*GameServerObj)->TryGetArrayField(TEXT("network_ports"), PortsArray))
                            {
                                for (const TSharedPtr<FJsonValue>& PortValue : *PortsArray)
                                {
                                    const TSharedPtr<FJsonObject>* PortObj;
                                    if (PortValue->TryGetObject(PortObj))
                                    {
                                        FPlayflowNetworkPort Port;
                                        Port.Name = (*PortObj)->GetStringField(TEXT("name"));
                                        Port.InternalPort = (*PortObj)->GetIntegerField(TEXT("internal_port"));
                                        Port.ExternalPort = (*PortObj)->GetIntegerField(TEXT("external_port"));
                                        Port.Protocol = (*PortObj)->GetStringField(TEXT("protocol"));
                                        Port.Host = (*PortObj)->GetStringField(TEXT("host"));
                                        Port.bTlsEnabled = (*PortObj)->GetBoolField(TEXT("tls_enabled"));

                                        LobbyResponse.GameServer.NetworkPorts.Add(Port);
                                    }
                                }
                            }
                        }
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("Playflow: Joined lobby successfully! ID: %s"), *LobbyResponse.Id);
                OnSuccess.Broadcast(LobbyResponse);
                return;
            }
        }

        TSharedPtr<FJsonObject> ErrorJsonObj;


        TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);


        if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))


        {


            LobbyResponse.ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));


        }


        else


        {


            LobbyResponse.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);


        }
    }
    else
    {
        LobbyResponse.ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Join lobby request failed"));
    }

    OnFailure.Broadcast(LobbyResponse);
}


UPlayflowStartMatchmaking* UPlayflowStartMatchmaking::StartMatchmaking(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName,
    FString RequesterID,
    FString MatchmakingMode)
{
    UPlayflowStartMatchmaking* Node = NewObject<UPlayflowStartMatchmaking>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;
    Node->RequesterID = RequesterID;
    Node->MatchmakingMode = MatchmakingMode;

    return Node;
}

void UPlayflowStartMatchmaking::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s?name=%s"), *LobbyID, *ConfigName);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("requesterId"), RequesterID);
    TSharedPtr<FJsonObject> MatchmakingObject = MakeShareable(new FJsonObject);
    MatchmakingObject->SetStringField(TEXT("action"), TEXT("start"));
    MatchmakingObject->SetStringField(TEXT("mode"), MatchmakingMode);
    JsonObject->SetObjectField(TEXT("matchmaking"), MatchmakingObject);
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("PUT"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(JsonString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowStartMatchmaking::OnResponseReceived);

    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Starting matchmaking - URL: %s"), *URL);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Request Body: %s"), *JsonString);
}

void UPlayflowStartMatchmaking::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Start Matchmaking Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Start Matchmaking Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("success")))
                {
                    bool bSuccess = JsonObject->GetBoolField(TEXT("success"));

                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Matchmaking started successfully"));
                        OnSuccess.Broadcast(TEXT(""));
                        return;
                    }
                    else
                    {
                        if (JsonObject->HasField(TEXT("message")))
                        {
                            ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                        }
                        else
                        {
                            ErrorMessage = TEXT("Start matchmaking returned success: false");
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Matchmaking started successfully (no success field)"));
                    OnSuccess.Broadcast(TEXT(""));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Matchmaking started successfully (non-JSON response)"));
                OnSuccess.Broadcast(TEXT(""));
                return;
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("message")))
                {
                    ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                }
                else
                {
                    TSharedPtr<FJsonObject> ErrorJsonObj;

                    TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                    if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                    {

                        ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                    }

                    else

                    {

                        ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                    }
                }
            }
            else
            {
                TSharedPtr<FJsonObject> ErrorJsonObj;

                TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                {

                    ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                }

                else

                {

                    ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                }
            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Start matchmaking request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Start matchmaking failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(ErrorMessage);
}


UPlayflowStopMatchmaking* UPlayflowStopMatchmaking::StopMatchmaking(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName,
    FString RequesterID)
{
    UPlayflowStopMatchmaking* Node = NewObject<UPlayflowStopMatchmaking>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;
    Node->RequesterID = RequesterID;

    return Node;
}

void UPlayflowStopMatchmaking::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s?name=%s"), *LobbyID, *ConfigName);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("requesterId"), RequesterID);
    TSharedPtr<FJsonObject> MatchmakingObject = MakeShareable(new FJsonObject);
    MatchmakingObject->SetStringField(TEXT("action"), TEXT("cancel"));
    JsonObject->SetObjectField(TEXT("matchmaking"), MatchmakingObject);
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("PUT"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(JsonString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowStopMatchmaking::OnResponseReceived);

    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Stopping matchmaking - URL: %s"), *URL);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Request Body: %s"), *JsonString);
}

void UPlayflowStopMatchmaking::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Stop Matchmaking Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Stop Matchmaking Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("success")))
                {
                    bool bSuccess = JsonObject->GetBoolField(TEXT("success"));

                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Matchmaking stopped successfully"));
                        OnSuccess.Broadcast(TEXT(""));
                        return;
                    }
                    else
                    {
                        if (JsonObject->HasField(TEXT("message")))
                        {
                            ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                        }
                        else
                        {
                            ErrorMessage = TEXT("Stop matchmaking returned success: false");
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Matchmaking stopped successfully (no success field)"));
                    OnSuccess.Broadcast(TEXT(""));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Matchmaking stopped successfully (non-JSON response)"));
                OnSuccess.Broadcast(TEXT(""));
                return;
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("message")))
                {
                    ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                }
                else
                {
                    TSharedPtr<FJsonObject> ErrorJsonObj;

                    TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                    if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                    {

                        ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                    }

                    else

                    {

                        ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                    }
                }
            }
            else
            {
                TSharedPtr<FJsonObject> ErrorJsonObj;

                TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                {

                    ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                }

                else

                {

                    ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                }
            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Stop matchmaking request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Stop matchmaking failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(ErrorMessage);
}

UPlayflowUpdatePlayerState* UPlayflowUpdatePlayerState::UpdatePlayerState(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    FString ConfigName,
    FString RequesterID,
    const TArray<FPlayflowPlayerStateEntry>& PlayerState,
    FString TargetPlayerID)
{
    UPlayflowUpdatePlayerState* Node = NewObject<UPlayflowUpdatePlayerState>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->ConfigName = ConfigName;
    Node->RequesterID = RequesterID;
    Node->PlayerState = PlayerState;
    Node->TargetPlayerID = TargetPlayerID;

    return Node;
}

UPlayflowReadServerConfig* UPlayflowReadServerConfig::ReadServerConfig(UObject* WorldContextObject)
{
    UPlayflowReadServerConfig* Node = NewObject<UPlayflowReadServerConfig>();
    Node->WorldContextObject = WorldContextObject;

    return Node;
}


void UPlayflowUpdatePlayerState::Activate()
{
    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s?name=%s"), *LobbyID, *ConfigName);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("requesterId"), RequesterID);

    if (!TargetPlayerID.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("targetPlayerId"), TargetPlayerID);
    }

    TSharedPtr<FJsonObject> PlayerStateObject = MakeShareable(new FJsonObject);

    for (const FPlayflowPlayerStateEntry& Entry : PlayerState)
    {
        if (Entry.Key.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Playflow: Skipping player state entry with empty key"));
            continue;
        }

        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Entry.ValueAsJsonString);
        TSharedPtr<FJsonValue> ParsedValue;

        if (FJsonSerializer::Deserialize(Reader, ParsedValue) && ParsedValue.IsValid())
        {
            PlayerStateObject->SetField(Entry.Key, ParsedValue);
        }
        else
        {
            PlayerStateObject->SetStringField(Entry.Key, Entry.ValueAsJsonString);
        }
    }

    JsonObject->SetObjectField(TEXT("playerState"), PlayerStateObject);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("PUT"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(JsonString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowUpdatePlayerState::OnResponseReceived);

    HttpRequest->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Updating player state - URL: %s"), *URL);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Request Body: %s"), *JsonString);
}

void UPlayflowUpdatePlayerState::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString ErrorMessage = TEXT("");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseString = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Playflow: Update Player State Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Playflow: Update Player State Response Body: %s"), *ResponseString);

        if (ResponseCode >= 200 && ResponseCode < 300)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("success")))
                {
                    bool bSuccess = JsonObject->GetBoolField(TEXT("success"));

                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Player state updated successfully"));
                        OnSuccess.Broadcast(TEXT(""));
                        return;
                    }
                    else
                    {
                        if (JsonObject->HasField(TEXT("message")))
                        {
                            ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                        }
                        else
                        {
                            ErrorMessage = TEXT("Update player state returned success: false");
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Player state updated successfully (no success field)"));
                    OnSuccess.Broadcast(TEXT(""));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Player state updated successfully (non-JSON response)"));
                OnSuccess.Broadcast(TEXT(""));
                return;
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                if (JsonObject->HasField(TEXT("message")))
                {
                    ErrorMessage = JsonObject->GetStringField(TEXT("message"));
                }
                else
                {
                    TSharedPtr<FJsonObject> ErrorJsonObj;

                    TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                    if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                    {

                        ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                    }

                    else

                    {

                        ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                    }
                }
            }
            else
            {
                TSharedPtr<FJsonObject> ErrorJsonObj;

                TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);

                if (FJsonSerializer::Deserialize(ErrorReader, ErrorJsonObj) && ErrorJsonObj.IsValid() && ErrorJsonObj->HasField(TEXT("message")))

                {

                    ErrorMessage = ErrorJsonObj->GetStringField(TEXT("message"));

                }

                else

                {

                    ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString);

                }
            }
        }
    }
    else
    {
        ErrorMessage = TEXT("Request failed - no response from server");
        UE_LOG(LogTemp, Error, TEXT("Playflow: Update player state request failed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Update player state failed - %s"), *ErrorMessage);
    OnFailure.Broadcast(ErrorMessage);
}

void UPlayflowReadServerConfig::Activate()
{
    FPlayflowServerConfig ServerConfig;

    FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::LaunchDir()) + TEXT("/playflow.json");

    UE_LOG(LogTemp, Log, TEXT("Playflow: Attempting to read server config from: %s"), *FilePath);

    if (!FPaths::FileExists(FilePath))
    {
        ServerConfig.bSuccess = false;
        ServerConfig.ErrorMessage = FString::Printf(TEXT("playflow.json not found at: %s"), *FilePath);
        UE_LOG(LogTemp, Warning, TEXT("Playflow: %s"), *ServerConfig.ErrorMessage);
        OnFailure.Broadcast(ServerConfig);
        return;
    }
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        ServerConfig.bSuccess = false;
        ServerConfig.ErrorMessage = TEXT("Failed to read playflow.json file");
        UE_LOG(LogTemp, Error, TEXT("Playflow: %s"), *ServerConfig.ErrorMessage);
        OnFailure.Broadcast(ServerConfig);
        return;
    }
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        ServerConfig.bSuccess = false;
        ServerConfig.ErrorMessage = TEXT("Failed to parse playflow.json - invalid JSON format");
        UE_LOG(LogTemp, Error, TEXT("Playflow: %s"), *ServerConfig.ErrorMessage);
        OnFailure.Broadcast(ServerConfig);
        return;
    }
    ServerConfig.bSuccess = true;

    if (JsonObject->HasField(TEXT("instance_id")))
    {
        ServerConfig.InstanceId = JsonObject->GetStringField(TEXT("instance_id"));
    }

    if (JsonObject->HasField(TEXT("region")))
    {
        ServerConfig.Region = JsonObject->GetStringField(TEXT("region"));
    }

    if (JsonObject->HasField(TEXT("api-key")))
    {
        ServerConfig.ApiKey = JsonObject->GetStringField(TEXT("api-key"));
    }

    if (JsonObject->HasField(TEXT("startup_args")))
    {
        ServerConfig.StartupArgs = JsonObject->GetStringField(TEXT("startup_args"));
    }

    if (JsonObject->HasField(TEXT("version_tag")))
    {
        ServerConfig.VersionTag = JsonObject->GetStringField(TEXT("version_tag"));
    }

    if (JsonObject->HasField(TEXT("match_id")))
    {
        ServerConfig.MatchId = JsonObject->GetStringField(TEXT("match_id"));
    }

    if (JsonObject->HasField(TEXT("arguments")))
    {
        ServerConfig.Arguments = JsonObject->GetStringField(TEXT("arguments"));
    }

    if (JsonObject->HasField(TEXT("custom_data")))
    {
        const TSharedPtr<FJsonObject>* CustomDataObj;
        if (JsonObject->TryGetObjectField(TEXT("custom_data"), CustomDataObj))
        {
            if ((*CustomDataObj)->HasField(TEXT("lobby_id")))
            {
                ServerConfig.LobbyID = (*CustomDataObj)->GetStringField(TEXT("lobby_id"));
            }
            if ((*CustomDataObj)->HasField(TEXT("matchmaking_mode")))
            {
                ServerConfig.MatchmakingMode = (*CustomDataObj)->GetStringField(TEXT("matchmaking_mode"));
            }
            if ((*CustomDataObj)->HasField(TEXT("all_players")))
            {
                const TArray<TSharedPtr<FJsonValue>>* PlayersArray;
                if ((*CustomDataObj)->TryGetArrayField(TEXT("all_players"), PlayersArray))
                {
                    for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
                    {
                        ServerConfig.AllPlayers.Add(PlayerValue->AsString());
                    }
                }
            }
            if ((*CustomDataObj)->HasField(TEXT("lobby_ids")))
            {
                const TArray<TSharedPtr<FJsonValue>>* LobbyIDsArray;
                if ((*CustomDataObj)->TryGetArrayField(TEXT("lobby_ids"), LobbyIDsArray))
                {
                    for (const TSharedPtr<FJsonValue>& LobbyIDValue : *LobbyIDsArray)
                    {
                        ServerConfig.LobbyIDs.Add(LobbyIDValue->AsString());
                    }
                }
            }
            if ((*CustomDataObj)->HasField(TEXT("match_configuration")))
            {
                const TSharedPtr<FJsonObject>* MatchConfigObj;
                if ((*CustomDataObj)->TryGetObjectField(TEXT("match_configuration"), MatchConfigObj))
                {
                    if ((*MatchConfigObj)->HasField(TEXT("teams_count")))
                    {
                        ServerConfig.MatchConfiguration.TeamsCount = (*MatchConfigObj)->GetIntegerField(TEXT("teams_count"));
                    }
                    if ((*MatchConfigObj)->HasField(TEXT("players_per_team")))
                    {
                        ServerConfig.MatchConfiguration.PlayersPerTeam = (*MatchConfigObj)->GetIntegerField(TEXT("players_per_team"));
                    }
                    if ((*MatchConfigObj)->HasField(TEXT("min_players_per_team")))
                    {
                        ServerConfig.MatchConfiguration.MinPlayersPerTeam = (*MatchConfigObj)->GetIntegerField(TEXT("min_players_per_team"));
                    }
                    if ((*MatchConfigObj)->HasField(TEXT("timeout")))
                    {
                        ServerConfig.MatchConfiguration.Timeout = (*MatchConfigObj)->GetIntegerField(TEXT("timeout"));
                    }
                }
            }
            if ((*CustomDataObj)->HasField(TEXT("lobby_settings")))
            {
                const TSharedPtr<FJsonObject>* LobbySettingsObj;
                if ((*CustomDataObj)->TryGetObjectField(TEXT("lobby_settings"), LobbySettingsObj))
                {
                    FString LobbySettingsString;
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&LobbySettingsString);
                    FJsonSerializer::Serialize(LobbySettingsObj->ToSharedRef(), Writer);
                    ServerConfig.LobbySettingsJson = LobbySettingsString;
                }
            }
            if ((*CustomDataObj)->HasField(TEXT("teams")))
            {
                const TArray<TSharedPtr<FJsonValue>>* TeamsArray;
                if ((*CustomDataObj)->TryGetArrayField(TEXT("teams"), TeamsArray))
                {
                    for (const TSharedPtr<FJsonValue>& TeamValue : *TeamsArray)
                    {
                        const TSharedPtr<FJsonObject>* TeamObj;
                        if (TeamValue->TryGetObject(TeamObj))
                        {
                            FPlayflowTeam Team;
                            if ((*TeamObj)->HasField(TEXT("team_id")))
                            {
                                Team.TeamID = (*TeamObj)->GetIntegerField(TEXT("team_id"));
                            }
                            if ((*TeamObj)->HasField(TEXT("lobbies")))
                            {
                                const TArray<TSharedPtr<FJsonValue>>* LobbiesArray;
                                if ((*TeamObj)->TryGetArrayField(TEXT("lobbies"), LobbiesArray))
                                {
                                    for (const TSharedPtr<FJsonValue>& LobbyValue : *LobbiesArray)
                                    {
                                        const TSharedPtr<FJsonObject>* LobbyObj;
                                        if (LobbyValue->TryGetObject(LobbyObj))
                                        {
                                            FPlayflowTeamLobby Lobby;

                                            if ((*LobbyObj)->HasField(TEXT("lobby_id")))
                                            {
                                                Lobby.LobbyID = (*LobbyObj)->GetStringField(TEXT("lobby_id"));
                                            }
                                            if ((*LobbyObj)->HasField(TEXT("lobby_name")))
                                            {
                                                Lobby.LobbyName = (*LobbyObj)->GetStringField(TEXT("lobby_name"));
                                            }
                                            if ((*LobbyObj)->HasField(TEXT("host")))
                                            {
                                                Lobby.Host = (*LobbyObj)->GetStringField(TEXT("host"));
                                            }
                                            if ((*LobbyObj)->HasField(TEXT("player_count")))
                                            {
                                                Lobby.PlayerCount = (*LobbyObj)->GetIntegerField(TEXT("player_count"));
                                            }
                                            if ((*LobbyObj)->HasField(TEXT("players")))
                                            {
                                                const TArray<TSharedPtr<FJsonValue>>* PlayersArray;
                                                if ((*LobbyObj)->TryGetArrayField(TEXT("players"), PlayersArray))
                                                {
                                                    for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
                                                    {
                                                        Lobby.Players.Add(PlayerValue->AsString());
                                                    }
                                                }
                                            }
                                            if ((*LobbyObj)->HasField(TEXT("player_states")))
                                            {
                                                const TSharedPtr<FJsonObject>* PlayerStatesObject;
                                                if ((*LobbyObj)->TryGetObjectField(TEXT("player_states"), PlayerStatesObject))
                                                {
                                                    for (const auto& PlayerEntry : (*PlayerStatesObject)->Values)
                                                    {
                                                        FString PlayerID = PlayerEntry.Key;

                                                        if (PlayerEntry.Value->Type == EJson::Object)
                                                        {
                                                            TSharedPtr<FJsonObject> StateObject = PlayerEntry.Value->AsObject();

                                                            FPlayflowPlayerState PlayerState;
                                                            PlayerState.PlayerID = PlayerID;

                                                            for (const auto& StateEntry : StateObject->Values)
                                                            {
                                                                FString Key = StateEntry.Key;
                                                                FString Value;

                                                                if (StateEntry.Value->Type == EJson::String)
                                                                {
                                                                    Value = StateEntry.Value->AsString();
                                                                }
                                                                else if (StateEntry.Value->Type == EJson::Number)
                                                                {
                                                                    Value = FString::Printf(TEXT("%f"), StateEntry.Value->AsNumber());
                                                                }
                                                                else if (StateEntry.Value->Type == EJson::Boolean)
                                                                {
                                                                    Value = StateEntry.Value->AsBool() ? TEXT("true") : TEXT("false");
                                                                }

                                                                PlayerState.StateData.Add(Key, Value);
                                                            }

                                                            Lobby.PlayerStates.Add(PlayerState);
                                                        }
                                                    }
                                                }
                                            }

                                            if ((*LobbyObj)->HasField(TEXT("lobby_settings")))
                                            {
                                                const TSharedPtr<FJsonObject>* LobbySettingsObj;
                                                if ((*LobbyObj)->TryGetObjectField(TEXT("lobby_settings"), LobbySettingsObj))
                                                {
                                                    FString LobbySettingsString;
                                                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&LobbySettingsString);
                                                    FJsonSerializer::Serialize(LobbySettingsObj->ToSharedRef(), Writer);
                                                    Lobby.LobbySettingsJson = LobbySettingsString;
                                                }
                                            }

                                            Team.Lobbies.Add(Lobby);
                                        }
                                    }
                                }
                            }
                            ServerConfig.Teams.Add(Team);
                        }
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Server config loaded! Instance: %s, Match: %s, Players: %d, Teams: %d"),
        *ServerConfig.InstanceId, *ServerConfig.MatchId, ServerConfig.AllPlayers.Num(), ServerConfig.Teams.Num());

    OnSuccess.Broadcast(ServerConfig);
}

TArray<FPlayflowRolePlayers> UPlayflowUtilities::SortPlayersByRole(const TArray<FPlayflowTeam>& Teams)
{
    TMap<FString, TArray<FString>> RoleMap;

    for (const FPlayflowTeam& Team : Teams)
    {
        for (const FPlayflowTeamLobby& Lobby : Team.Lobbies)
        {
            for (const FPlayflowPlayerState& PlayerState : Lobby.PlayerStates)
            {
                FString Role = PlayerState.StateData.Contains("role") ? PlayerState.StateData["role"] : TEXT("unknown");
                FString PlayerID = PlayerState.PlayerID;

                if (!RoleMap.Contains(Role))
                {
                    RoleMap.Add(Role, TArray<FString>());
                }
                RoleMap[Role].Add(PlayerID);
            }
        }
    }

    TArray<FPlayflowRolePlayers> SortedRoles;
    for (const auto& RolePair : RoleMap)
    {
        FPlayflowRolePlayers RolePlayers;
        RolePlayers.Role = RolePair.Key;
        RolePlayers.PlayerIDs = RolePair.Value;
        SortedRoles.Add(RolePlayers);

        UE_LOG(LogTemp, Log, TEXT("Playflow: Role '%s' has %d players"), *RolePlayers.Role, RolePlayers.PlayerIDs.Num());
    }

    return SortedRoles;
}

TArray<FString> UPlayflowUtilities::GetPlayersByRole(const TArray<FPlayflowTeam>& Teams, FString Role)
{
    TArray<FString> PlayerIDs;

    for (const FPlayflowTeam& Team : Teams)
    {
        for (const FPlayflowTeamLobby& Lobby : Team.Lobbies)
        {
            for (const FPlayflowPlayerState& PlayerState : Lobby.PlayerStates)
            {
                FString PlayerRole = PlayerState.StateData.Contains("role") ? PlayerState.StateData["role"] : TEXT("");
                if (PlayerRole.Equals(Role, ESearchCase::IgnoreCase))
                {
                    PlayerIDs.Add(PlayerState.PlayerID);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: Found %d players with role '%s'"), PlayerIDs.Num(), *Role);
    return PlayerIDs;
}

FPlayflowPlayerState UPlayflowUtilities::GetPlayerState(const TArray<FPlayflowTeam>& Teams, FString PlayerID, bool& bFound)
{
    bFound = false;

    for (const FPlayflowTeam& Team : Teams)
    {
        for (const FPlayflowTeamLobby& Lobby : Team.Lobbies)
        {
            for (const FPlayflowPlayerState& PlayerState : Lobby.PlayerStates)
            {
                if (PlayerState.PlayerID.Equals(PlayerID, ESearchCase::IgnoreCase))
                {
                    bFound = true;
                    FString Role = PlayerState.StateData.Contains("role") ? PlayerState.StateData["role"] : TEXT("N/A");
                    FString MMR = PlayerState.StateData.Contains("mmr") ? PlayerState.StateData["mmr"] : TEXT("N/A");
                    UE_LOG(LogTemp, Log, TEXT("Playflow: Found player state for %s (Role: %s, MMR: %s)"),
                        *PlayerID, *Role, *MMR);
                    return PlayerState;
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Player state not found for %s"), *PlayerID);
    return FPlayflowPlayerState();
}

FString UPlayflowUtilities::GetPlayerRole(const TArray<FPlayflowTeam>& Teams, FString PlayerID)
{
    for (const FPlayflowTeam& Team : Teams)
    {
        for (const FPlayflowTeamLobby& Lobby : Team.Lobbies)
        {
            for (const FPlayflowPlayerState& PlayerState : Lobby.PlayerStates)
            {
                if (PlayerState.PlayerID == PlayerID)
                {
                    if (PlayerState.StateData.Contains(TEXT("role")))
                    {
                        FString Role = PlayerState.StateData[TEXT("role")];
                        UE_LOG(LogTemp, Log, TEXT("Playflow: Player %s has role '%s'"), *PlayerID, *Role);
                        return Role;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Playflow: Player %s found but has no 'role' key in state data"), *PlayerID);
                        return TEXT("");
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Player %s not found in any team"), *PlayerID);
    return TEXT("");
}

int32 UPlayflowUtilities::GetPlayerTeamID(const TArray<FPlayflowTeam>& Teams, FString PlayerID)
{
    for (const FPlayflowTeam& Team : Teams)
    {
        for (const FPlayflowTeamLobby& Lobby : Team.Lobbies)
        {
            if (Lobby.Players.Contains(PlayerID))
            {
                UE_LOG(LogTemp, Log, TEXT("Playflow: Player %s is in team %d"), *PlayerID, Team.TeamID);
                return Team.TeamID;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Player %s not found in any team"), *PlayerID);
    return -1;
}
FString UPlayflowUtilities::GetPlayerStateValue(const FPlayflowPlayerState& PlayerState, FString Key, FString DefaultValue)
{
    if (PlayerState.StateData.Contains(Key))
    {
        return PlayerState.StateData[Key];
    }
    return DefaultValue;
}

UPlayflowUpdateLobbyRegion* UPlayflowUpdateLobbyRegion::UpdateLobbyRegion(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyID,
    EPlayflowRegion Region,
    FString RequesterID,
    FString ConfigName)
{
    UPlayflowUpdateLobbyRegion* Node = NewObject<UPlayflowUpdateLobbyRegion>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyID = LobbyID;
    Node->Region = Region;
    Node->RequesterID = RequesterID;
    Node->ConfigName = ConfigName;
    return Node;
}

void UPlayflowUpdateLobbyRegion::Activate()
{
    if (APIKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: API Key is required"));
        OnFailure.Broadcast(TEXT("API Key is required"));
        return;
    }

    if (LobbyID.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Lobby ID is required"));
        OnFailure.Broadcast(TEXT("Lobby ID is required"));
        return;
    }

    if (RequesterID.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Requester ID is required"));
        OnFailure.Broadcast(TEXT("Requester ID is required"));
        return;
    }

    FString URL = FString::Printf(TEXT("https://api.scale.computeflow.cloud/lobbies/%s"), *LobbyID);

    if (!ConfigName.IsEmpty())
    {
        URL += FString::Printf(TEXT("?name=%s"), *ConfigName);
    }

    FString RegionString = PlayflowRegionToString(Region);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("requesterId", RequesterID);

    TSharedPtr<FJsonObject> SettingsObject = MakeShareable(new FJsonObject);
    SettingsObject->SetStringField("region", RegionString);
    JsonObject->SetObjectField("settings", SettingsObject);

    FString ContentString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    UE_LOG(LogTemp, Log, TEXT("Playflow: Updating lobby region to: %s"), *RegionString);
    UE_LOG(LogTemp, Log, TEXT("Playflow: PUT %s"), *URL);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb(TEXT("PUT"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("api-key"), APIKey);
    HttpRequest->SetContentAsString(ContentString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPlayflowUpdateLobbyRegion::OnResponseReceived);
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to send update lobby region request"));
        OnFailure.Broadcast(TEXT("Failed to send request"));
    }
}

void UPlayflowUpdateLobbyRegion::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Update lobby region request failed"));
        OnFailure.Broadcast(TEXT("Request failed"));
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    FString ResponseString = Response->GetContentAsString();

    UE_LOG(LogTemp, Log, TEXT("Playflow: Update lobby region response code: %d"), ResponseCode);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Response: %s"), *ResponseString);

    if (ResponseCode >= 200 && ResponseCode < 300)
    {
        UE_LOG(LogTemp, Log, TEXT("Playflow: Lobby region updated successfully"));
        OnSuccess.Broadcast(TEXT("Region updated successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Playflow: Failed to update lobby region. HTTP %d"), ResponseCode);
        OnFailure.Broadcast(FString::Printf(TEXT("HTTP Error %d"), ResponseCode));
    }
}

bool UPlayflowAPIHelpers::ParseLobbyResponse(const FString& JsonString, FPlayflowLobbyResponse& OutResponse)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ParseLobbyResponse: Failed to parse JSON."));
        return false;
    }


    auto SafeGetString = [&](const FString& Key, FString& OutValue)
        {
            if (JsonObject->HasField(Key)) OutValue = JsonObject->GetStringField(Key);
        };
    auto SafeGetInt = [&](const FString& Key, int32& OutValue)
        {
            if (JsonObject->HasField(Key)) OutValue = JsonObject->GetIntegerField(Key);
        };
    auto SafeGetBool = [&](const FString& Key, bool& OutValue)
        {
            if (JsonObject->HasField(Key)) OutValue = JsonObject->GetBoolField(Key);
        };
    SafeGetString(TEXT("id"), OutResponse.Id);
    SafeGetString(TEXT("name"), OutResponse.Name);
    SafeGetString(TEXT("host"), OutResponse.Host);
    SafeGetString(TEXT("region"), OutResponse.Region);
    SafeGetString(TEXT("status"), OutResponse.Status);
    SafeGetString(TEXT("inviteCode"), OutResponse.InviteCode);
    SafeGetString(TEXT("createdAt"), OutResponse.CreatedAt);
    SafeGetString(TEXT("updatedAt"), OutResponse.UpdatedAt);
    SafeGetString(TEXT("matchmakingMode"), OutResponse.MatchmakingMode);
    SafeGetString(TEXT("matchmakingStartedAt"), OutResponse.MatchmakingStartedAt);
    SafeGetString(TEXT("matchmakingTicketId"), OutResponse.MatchmakingTicketId);

    SafeGetInt(TEXT("maxPlayers"), OutResponse.MaxPlayers);
    SafeGetInt(TEXT("currentPlayers"), OutResponse.CurrentPlayers);
    SafeGetInt(TEXT("timeout"), OutResponse.Timeout);
    SafeGetBool(TEXT("isPrivate"), OutResponse.IsPrivate);
    SafeGetBool(TEXT("useInviteCode"), OutResponse.UseInviteCode);
    SafeGetBool(TEXT("allowLateJoin"), OutResponse.bAllowLateJoin);

    if (JsonObject->HasField(TEXT("players")))
    {
        const TArray<TSharedPtr<FJsonValue>>* PlayersArray;
        if (JsonObject->TryGetArrayField(TEXT("players"), PlayersArray))
        {
            for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
            {
                OutResponse.Players.Add(PlayerValue->AsString());
            }
        }
    }

    auto StoreJsonObject = [&](const FString& Field, FString& Target)
        {
            const TSharedPtr<FJsonObject>* Obj;
            if (JsonObject->TryGetObjectField(Field, Obj))
            {
                FString Tmp;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Tmp);
                FJsonSerializer::Serialize(Obj->ToSharedRef(), Writer);
                Target = Tmp;
            }
        };
    StoreJsonObject(TEXT("settings"), OutResponse.Settings);
    StoreJsonObject(TEXT("lobbyStateRealTime"), OutResponse.LobbyStateRealTime);
    StoreJsonObject(TEXT("matchmakingData"), OutResponse.MatchmakingData);
    if (JsonObject->HasField(TEXT("gameServer")))
    {
        const TSharedPtr<FJsonObject>* GameServerObj;
        if (JsonObject->TryGetObjectField(TEXT("gameServer"), GameServerObj))
        {
            FPlayflowGameServer& S = OutResponse.GameServer;

            auto GetInnerString = [&](const FString& Key, FString& OutValue)
                {
                    if ((*GameServerObj)->HasField(Key))
                    {
                        OutValue = (*GameServerObj)->GetStringField(Key);
                    }
                };
            auto GetInnerInt = [&](const FString& Key, int32& OutValue)
                {
                    if ((*GameServerObj)->HasField(Key))
                    {
                        OutValue = (*GameServerObj)->GetIntegerField(Key);
                    }
                };

            GetInnerString(TEXT("instance_id"), S.InstanceId);
            GetInnerString(TEXT("name"), S.Name);
            GetInnerString(TEXT("status"), S.Status);
            GetInnerString(TEXT("region"), S.Region);
            GetInnerString(TEXT("startup_args"), S.StartupArgs);
            GetInnerString(TEXT("service_type"), S.ServiceType);
            GetInnerString(TEXT("compute_size"), S.ComputeSize);
            GetInnerString(TEXT("version_tag"), S.VersionTag);
            GetInnerString(TEXT("started_at"), S.StartedAt);
            GetInnerString(TEXT("stopped_at"), S.StoppedAt);
            GetInnerInt(TEXT("ttl"), S.Ttl);

            const TSharedPtr<FJsonObject>* CustomDataObj;
            if ((*GameServerObj)->TryGetObjectField(TEXT("custom_data"), CustomDataObj))
            {
                FString JsonStr;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
                FJsonSerializer::Serialize(CustomDataObj->ToSharedRef(), Writer);
                S.CustomData = JsonStr;
            }

            const TArray<TSharedPtr<FJsonValue>>* PortsArray;
            if ((*GameServerObj)->TryGetArrayField(TEXT("network_ports"), PortsArray))
            {
                for (const TSharedPtr<FJsonValue>& PortValue : *PortsArray)
                {
                    const TSharedPtr<FJsonObject>* PortObj;
                    if (PortValue->TryGetObject(PortObj))
                    {
                        FPlayflowNetworkPort Port;
                        Port.Name = (*PortObj)->GetStringField(TEXT("name"));
                        Port.InternalPort = (*PortObj)->GetIntegerField(TEXT("internal_port"));
                        Port.ExternalPort = (*PortObj)->GetIntegerField(TEXT("external_port"));
                        Port.Protocol = (*PortObj)->GetStringField(TEXT("protocol"));
                        Port.Host = (*PortObj)->GetStringField(TEXT("host"));
                        Port.bTlsEnabled = (*PortObj)->GetBoolField(TEXT("tls_enabled"));

                        S.NetworkPorts.Add(Port);
                    }
                }
            }
        }
    }

    return true;
}

UPlayflowUpdateLobbyStatus* UPlayflowUpdateLobbyStatus::UpdateLobbyStatus(
    UObject* WorldContextObject,
    FString APIKey,
    FString LobbyId,
    FString RequesterId,
    FString ConfigName,
    EPlayflowLobbyStatus Status)
{
    UPlayflowUpdateLobbyStatus* Node = NewObject<UPlayflowUpdateLobbyStatus>();
    Node->WorldContextObject = WorldContextObject;
    Node->APIKey = APIKey;
    Node->LobbyId = LobbyId;
    Node->RequesterId = RequesterId;
    Node->ConfigName = ConfigName;
    Node->Status = Status;
    return Node;
}

void UPlayflowUpdateLobbyStatus::Activate()
{
    FString StatusString = (Status == EPlayflowLobbyStatus::Waiting) ? TEXT("waiting") : TEXT("in_game");

    FString URL = FString::Printf(
        TEXT("https://api.scale.computeflow.cloud/lobbies/%s?name=%s"),
        *LobbyId,
        *ConfigName
    );

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb(TEXT("PUT"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("api-key"), APIKey);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("requesterId"), RequesterId);
    JsonObject->SetStringField(TEXT("status"), StatusString);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    Request->SetContentAsString(OutputString);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (!bWasSuccessful || !Response.IsValid())
            {
                FPlayflowLobbyResponse EmptyResponse;
                OnFailure.Broadcast(EmptyResponse);
                return;
            }

            int32 ResponseCode = Response->GetResponseCode();
            FString ResponseBody = Response->GetContentAsString();

            if (ResponseCode >= 200 && ResponseCode < 300)
            {
                FPlayflowLobbyResponse LobbyResponse;
                if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseBody, &LobbyResponse, 0, 0))
                {
                    OnSuccess.Broadcast(LobbyResponse);
                }
                else
                {
                    FPlayflowLobbyResponse EmptyResponse;
                    OnFailure.Broadcast(EmptyResponse);
                }
            }
            else
            {
                FPlayflowLobbyResponse EmptyResponse;
                OnFailure.Broadcast(EmptyResponse);
            }
        });

    Request->ProcessRequest();
}