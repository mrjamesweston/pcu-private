#include "PlayflowRegionSelector.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ConfigCacheIni.h"

const FString UPlayflowRegionSelector::ConfigSection = TEXT("Playflow.Region");

TArray<EPlayflowRegion> UPlayflowRegionSelector::GetAvailableRegions()
{
    TArray<EPlayflowRegion> Regions;

    const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPlayflowRegion"), true);
    if (EnumPtr)
    {
        for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
        {
            Regions.Add((EPlayflowRegion)EnumPtr->GetValueByIndex(i));
        }
    }

    return Regions;
}

FString UPlayflowRegionSelector::GetRegionDisplayName(EPlayflowRegion Region)
{
    switch (Region)
    {
    case EPlayflowRegion::UsEast:
        return TEXT("US East");
    case EPlayflowRegion::UsWest:
        return TEXT("US West");
    case EPlayflowRegion::EuNorth:
        return TEXT("Sweden");
    case EPlayflowRegion::EuWest:
        return TEXT("France");
    case EPlayflowRegion::EuUK:
        return TEXT("London");
    case EPlayflowRegion::ApSouth:
        return TEXT("India");
    case EPlayflowRegion::Sea:
        return TEXT("Singapore");
    case EPlayflowRegion::Ea:
        return TEXT("Hong Kong");
    case EPlayflowRegion::ApNorth:
        return TEXT("Japan");
    case EPlayflowRegion::ApSoutheast:
        return TEXT("Australia");
    case EPlayflowRegion::SouthAfrica:
        return TEXT("South Africa");
    case EPlayflowRegion::SouthAmericaBrazil:
        return TEXT("Brazil");
    case EPlayflowRegion::SouthAmericaChile:
        return TEXT("Chile");
    default:
        return TEXT("Unknown Region");
    }
}

FString UPlayflowRegionSelector::GetRegionID(EPlayflowRegion Region)
{
    return PlayflowRegionToString(Region);
}

EPlayflowRegion UPlayflowRegionSelector::RegionIDToEnum(const FString& RegionID)
{
    TArray<EPlayflowRegion> AllRegions = GetAvailableRegions();

    for (EPlayflowRegion Region : AllRegions)
    {
        if (PlayflowRegionToString(Region).Equals(RegionID, ESearchCase::IgnoreCase))
        {
            return Region;
        }
    }

    return EPlayflowRegion::UsEast;
}

void UPlayflowRegionSelector::DetectOptimalRegion(FOnRegionDetected OnComplete)
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Detecting optimal region via IP geolocation..."));

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(TEXT("http://ip-api.com/json/?fields=status,country,countryCode,continent,continentCode,region,regionName,city,lat,lon"));
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetTimeout(10.0f);

    HttpRequest->OnProcessRequestComplete().BindLambda([OnComplete](
        FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            EPlayflowRegion DetectedRegion = EPlayflowRegion::UsEast;

            if (bWasSuccessful && Response.IsValid())
            {
                int32 ResponseCode = Response->GetResponseCode();
                FString ResponseString = Response->GetContentAsString();

                UE_LOG(LogTemp, Log, TEXT("Playflow: Geolocation Response Code: %d"), ResponseCode);
                UE_LOG(LogTemp, Log, TEXT("Playflow: Geolocation Response: %s"), *ResponseString);

                if (ResponseCode == 200)
                {
                    TSharedPtr<FJsonObject> JsonObject;
                    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

                    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                    {
                        FString Status = JsonObject->GetStringField(TEXT("status"));

                        if (Status == TEXT("success"))
                        {
                            FString CountryCode = JsonObject->GetStringField(TEXT("countryCode"));
                            FString ContinentCode = JsonObject->GetStringField(TEXT("continentCode"));
                            FString Country = JsonObject->GetStringField(TEXT("country"));
                            FString Continent = JsonObject->GetStringField(TEXT("continent"));
                            FString Region = JsonObject->GetStringField(TEXT("region"));
                            FString RegionName = JsonObject->GetStringField(TEXT("regionName"));
                            FString City = JsonObject->GetStringField(TEXT("city"));

                            double Latitude = JsonObject->GetNumberField(TEXT("lat"));
                            double Longitude = JsonObject->GetNumberField(TEXT("lon"));

                            UE_LOG(LogTemp, Log, TEXT("Playflow: Detected location - Country: %s (%s), Region: %s (%s), City: %s, Lat/Lon: %.2f, %.2f"),
                                *Country, *CountryCode, *RegionName, *Region, *City, Latitude, Longitude);

                            DetectedRegion = MapCountryToRegion(CountryCode, ContinentCode, Region, Longitude);
                            UE_LOG(LogTemp, Log, TEXT("Playflow: Mapped to region: %s"), *PlayflowRegionToString(DetectedRegion));
                        }
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Playflow: Failed to detect region, using default: %s"),
                    *PlayflowRegionToString(DetectedRegion));
            }

            AsyncTask(ENamedThreads::GameThread, [OnComplete, DetectedRegion]()
                {
                    OnComplete.ExecuteIfBound(DetectedRegion);
                });
        });

    HttpRequest->ProcessRequest();
}

void UPlayflowRegionSelector::SetPreferredRegion(EPlayflowRegion Region)
{
    SaveRegionPreference(Region);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Preferred region set to: %s"), *PlayflowRegionToString(Region));
}

void UPlayflowRegionSelector::GetBestRegion(bool bForceAutoDetect, FOnRegionDetected OnComplete)
{
    if (!bForceAutoDetect && HasSavedPreference())
    {
        EPlayflowRegion SavedRegion = GetSavedPreferredRegion();
        UE_LOG(LogTemp, Log, TEXT("Playflow: Using saved preferred region: %s"), *PlayflowRegionToString(SavedRegion));
        OnComplete.ExecuteIfBound(SavedRegion);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Playflow: No saved preference, auto-detecting..."));
    DetectOptimalRegion(OnComplete);
}

EPlayflowRegion UPlayflowRegionSelector::GetSavedPreferredRegion()
{
    FString SavedRegionID;
    GConfig->GetString(*ConfigSection, TEXT("PreferredRegion"), SavedRegionID, GGameUserSettingsIni);

    if (SavedRegionID.IsEmpty())
    {
        return EPlayflowRegion::UsEast;
    }

    return RegionIDToEnum(SavedRegionID);
}

bool UPlayflowRegionSelector::HasSavedPreference()
{
    FString SavedRegionID;
    GConfig->GetString(*ConfigSection, TEXT("PreferredRegion"), SavedRegionID, GGameUserSettingsIni);
    return !SavedRegionID.IsEmpty();
}

void UPlayflowRegionSelector::ClearPreferredRegion()
{
    GConfig->RemoveKey(*ConfigSection, TEXT("PreferredRegion"), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    UE_LOG(LogTemp, Log, TEXT("Playflow: Cleared preferred region"));
}

EPlayflowRegion UPlayflowRegionSelector::MapCountryToRegion(const FString& CountryCode, const FString& ContinentCode, const FString& StateRegion, double Longitude)
{
    if (ContinentCode == TEXT("NA"))
    {
        if (CountryCode == TEXT("US"))
        {
            return EPlayflowRegion::UsWest;
        }
        else if (CountryCode == TEXT("CA"))
        {
            return EPlayflowRegion::UsWest;
        }
        else if (CountryCode == TEXT("MX"))
        {
            return EPlayflowRegion::UsWest;
        }
        return EPlayflowRegion::UsEast;
    }

    if (ContinentCode == TEXT("SA"))
    {
        if (CountryCode == TEXT("BR"))
        {
            return EPlayflowRegion::SouthAmericaBrazil;
        }
        else if (CountryCode == TEXT("CL") || CountryCode == TEXT("AR") || CountryCode == TEXT("PE"))
        {
            return EPlayflowRegion::SouthAmericaChile;
        }
        return EPlayflowRegion::SouthAmericaBrazil;
    }

    if (ContinentCode == TEXT("EU"))
    {
        if (CountryCode == TEXT("GB") || CountryCode == TEXT("IE"))
        {
            return EPlayflowRegion::EuUK;
        }
        else if (CountryCode == TEXT("SE") || CountryCode == TEXT("NO") || CountryCode == TEXT("FI") ||
            CountryCode == TEXT("DK") || CountryCode == TEXT("IS"))
        {
            return EPlayflowRegion::EuNorth;
        }
        return EPlayflowRegion::EuWest;
    }

    if (ContinentCode == TEXT("AS"))
    {
        if (CountryCode == TEXT("SG") || CountryCode == TEXT("MY") || CountryCode == TEXT("TH") ||
            CountryCode == TEXT("ID") || CountryCode == TEXT("PH") || CountryCode == TEXT("VN"))
        {
            return EPlayflowRegion::Sea;
        }
        else if (CountryCode == TEXT("HK") || CountryCode == TEXT("MO") || CountryCode == TEXT("TW"))
        {
            return EPlayflowRegion::Ea;
        }
        else if (CountryCode == TEXT("JP") || CountryCode == TEXT("KR"))
        {
            return EPlayflowRegion::ApNorth;
        }
        else if (CountryCode == TEXT("IN") || CountryCode == TEXT("PK") || CountryCode == TEXT("BD") ||
            CountryCode == TEXT("LK") || CountryCode == TEXT("NP"))
        {
            return EPlayflowRegion::ApSouth;
        }
        else if (CountryCode == TEXT("CN"))
        {
            return EPlayflowRegion::Ea;
        }
        return EPlayflowRegion::Sea;
    }

    if (ContinentCode == TEXT("OC"))
    {
        return EPlayflowRegion::ApSoutheast;
    }
    if (ContinentCode == TEXT("AF"))
    {
        return EPlayflowRegion::SouthAfrica;
    }
    return EPlayflowRegion::UsEast;
}

void UPlayflowRegionSelector::SaveRegionPreference(EPlayflowRegion Region)
{
    FString RegionID = PlayflowRegionToString(Region);
    GConfig->SetString(*ConfigSection, TEXT("PreferredRegion"), *RegionID, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}