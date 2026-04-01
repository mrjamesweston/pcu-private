#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "PlayflowTypes.h"
#include "PlayflowRegionSelector.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnRegionDetected, EPlayflowRegion, DetectedRegion);

/**
 * Playflow Region Selector
 * Handles automatic region detection and manual region selection
 */
UCLASS(BlueprintType)
class PLAYFLOW_API UPlayflowRegionSelector : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Get all available Playflow regions as an array
     */
    UFUNCTION(BlueprintPure, Category = "Playflow|Region")
    static TArray<EPlayflowRegion> GetAvailableRegions();

    /**
     * Get region display name from enum
     */
    UFUNCTION(BlueprintPure, Category = "Playflow|Region")
    static FString GetRegionDisplayName(EPlayflowRegion Region);

    /**
     * Get region ID string from enum (e.g., "us-east")
     */
    UFUNCTION(BlueprintPure, Category = "Playflow|Region")
    static FString GetRegionID(EPlayflowRegion Region);

    /**
     * Convert region ID string to enum
     */
    UFUNCTION(BlueprintPure, Category = "Playflow|Region")
    static EPlayflowRegion RegionIDToEnum(const FString& RegionID);

    /**
     * Auto-detect the best region based on player's IP using geolocation
     * @param OnComplete Callback when detection is complete
     */
    UFUNCTION(BlueprintCallable, Category = "Playflow|Region")
    static void DetectOptimalRegion(FOnRegionDetected OnComplete);

    /**
     * Set the preferred region manually (overrides auto-detection)
     * @param Region The region to set
     */
    UFUNCTION(BlueprintCallable, Category = "Playflow|Region")
    static void SetPreferredRegion(EPlayflowRegion Region);

    /**
     * Get the currently selected region (manual preference or auto-detected)
     * @param bForceAutoDetect If true, ignores saved preference and auto-detects
     */
    UFUNCTION(BlueprintCallable, Category = "Playflow|Region")
    static void GetBestRegion(bool bForceAutoDetect, FOnRegionDetected OnComplete);

    /**
     * Get the saved preferred region (returns UsEast if none saved)
     */
    UFUNCTION(BlueprintPure, Category = "Playflow|Region")
    static EPlayflowRegion GetSavedPreferredRegion();

    /**
     * Check if a region preference is saved
     */
    UFUNCTION(BlueprintPure, Category = "Playflow|Region")
    static bool HasSavedPreference();

    /**
     * Clear saved region preference
     */
    UFUNCTION(BlueprintCallable, Category = "Playflow|Region")
    static void ClearPreferredRegion();

private:
    /**
     * Map country code to optimal Playflow region
     * Uses longitude to differentiate US East/West
     */
    static EPlayflowRegion MapCountryToRegion(const FString& CountryCode, const FString& ContinentCode, const FString& StateRegion, double Longitude);

    /**
     * Save region preference to config
     */
    static void SaveRegionPreference(EPlayflowRegion Region);

    /**
     * Config file section name
     */
    static const FString ConfigSection;
};