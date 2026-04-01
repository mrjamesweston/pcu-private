#include "PlayflowEditorSettings.h"

UPlayflowEditorSettings* UPlayflowEditorSettings::Get()
{
    return GetMutableDefault<UPlayflowEditorSettings>();
}

void UPlayflowEditorSettings::SaveSettings()
{
    SaveConfig();
    UE_LOG(LogTemp, Log, TEXT("Playflow: Settings saved"));
}