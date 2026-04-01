#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "PlayflowEditorStyle.h"

class FToolBarBuilder;
class FMenuBuilder;
class UToolMenu;

class FPlayflowEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void RegisterToolbarExtension();
    void OnModulesChanged(FName ModuleName, EModuleChangeReason Reason);
    void AddToolbarExtension(FToolBarBuilder& Builder);
    void FillPlayflowMenu(UToolMenu* Menu);
    void OnBuildAndPush();
    void OnSetAPIKey();
    TSharedRef<SWidget> MakePlayflowMenu();
    TSharedRef<SWidget> GenerateToolbarMenu();

private:
    TSharedPtr<class FUICommandList> PluginCommands;
};