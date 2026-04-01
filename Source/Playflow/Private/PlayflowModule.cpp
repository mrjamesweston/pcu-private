#include "PlayflowModule.h"

#define LOCTEXT_NAMESPACE "FPlayflowModule"

void FPlayflowModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow module started"));
}

void FPlayflowModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPlayflowModule, Playflow)