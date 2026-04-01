#include "PlayflowEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FPlayflowEditorStyle::StyleInstance = nullptr;

void FPlayflowEditorStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FPlayflowEditorStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FPlayflowEditorStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("PlayflowEditorStyle"));
    return StyleSetName;
}

const ISlateStyle& FPlayflowEditorStyle::Get()
{
    return *StyleInstance;
}

TSharedRef<FSlateStyleSet> FPlayflowEditorStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("PlayflowEditorStyle"));
    
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("Playflow");
    if (Plugin.IsValid())
    {
        Style->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
    }

    Style->Set("Playflow.BuildAndPush", new FSlateImageBrush(
        Style->RootToContentDir(TEXT("Icon128.png")),
        FVector2D(40.0f, 40.0f)
    ));

    return Style;
}