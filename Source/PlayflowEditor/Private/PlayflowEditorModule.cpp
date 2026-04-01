#include "PlayflowEditorModule.h"
#include "PlayflowBuildTools.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PlayflowEditorStyle.h"
#include "Widgets/Input/SButton.h"  
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"  
#include "PlayflowEditorSettings.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FPlayflowEditorModule"

void FPlayflowEditorModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow Editor Module Started"));

    FPlayflowEditorStyle::Initialize();

    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings("Editor", "Plugins", "Playflow",
            LOCTEXT("PlayflowSettingsName", "Playflow"),
            LOCTEXT("PlayflowSettingsDescription", "Configure Playflow Cloud settings"),
            GetMutableDefault<UPlayflowEditorSettings>()
        );
    }

    if (!IsRunningCommandlet())
    {
#if ENGINE_MAJOR_VERSION == 4
        FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
        if (LevelEditorModule)
        {
            UE_LOG(LogTemp, Warning, TEXT("Playflow: LevelEditor module already loaded, registering toolbar now"));
            RegisterToolbarExtension();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Playflow: LevelEditor module not loaded yet, delaying toolbar registration"));
            FModuleManager::Get().OnModulesChanged().AddRaw(this, &FPlayflowEditorModule::OnModulesChanged);
        }
#endif

        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPlayflowEditorModule::RegisterMenus));
    }
}

void FPlayflowEditorModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow Editor Module Shutdown"));

    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FModuleManager::Get().OnModulesChanged().RemoveAll(this);

    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Editor", "Plugins", "Playflow");
    }

    FPlayflowEditorStyle::Shutdown();
}

#if ENGINE_MAJOR_VERSION == 4
void FPlayflowEditorModule::OnModulesChanged(FName ModuleName, EModuleChangeReason Reason)
{
    if (ModuleName == "LevelEditor" && Reason == EModuleChangeReason::ModuleLoaded)
    {
        UE_LOG(LogTemp, Warning, TEXT("Playflow: LevelEditor module loaded, registering toolbar"));
        RegisterToolbarExtension();
        FModuleManager::Get().OnModulesChanged().RemoveAll(this);
    }
}

void FPlayflowEditorModule::RegisterToolbarExtension()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Creating toolbar extender"));


    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    ToolbarExtender->AddToolBarExtension(
        "Settings",
        EExtensionHook::After,
        nullptr,
        FToolBarExtensionDelegate::CreateRaw(this, &FPlayflowEditorModule::AddToolbarExtension)
    );

    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
    UE_LOG(LogTemp, Warning, TEXT("Playflow: Toolbar extender registered"));
}

void FPlayflowEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
    UE_LOG(LogTemp, Warning, TEXT("Playflow: Adding toolbar button..."));

    Builder.AddComboButton(
        FUIAction(),
        FOnGetContent::CreateRaw(this, &FPlayflowEditorModule::MakePlayflowMenu),
        LOCTEXT("PlayflowCombo", "Playflow"),
        LOCTEXT("PlayflowComboTooltip", "Playflow Cloud"),
        FSlateIcon(FPlayflowEditorStyle::GetStyleSetName(), "Playflow.BuildAndPush"),
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Toolbar button added!"));
}
#else
void FPlayflowEditorModule::OnModulesChanged(FName ModuleName, EModuleChangeReason Reason)
{
    // Not used in UE5
}

void FPlayflowEditorModule::RegisterToolbarExtension()
{
    // Not used in UE5
}

void FPlayflowEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
    // Not used in UE5
}
#endif

void FPlayflowEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Starting menu registration..."));

#if ENGINE_MAJOR_VERSION >= 5
    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
        if (ToolbarMenu)
        {
            UE_LOG(LogTemp, Warning, TEXT("Playflow: User toolbar found, adding Playflow button..."));

            FToolMenuSection& Section = ToolbarMenu->AddSection("Playflow");

            Section.AddEntry(FToolMenuEntry::InitComboButton(
                "PlayflowCombo",
                FUIAction(),
                FOnGetContent::CreateRaw(this, &FPlayflowEditorModule::GenerateToolbarMenu),
                LOCTEXT("PlayflowCombo", "Playflow"),
                LOCTEXT("PlayflowComboTooltip", "Playflow Cloud"),
                FSlateIcon(FPlayflowEditorStyle::GetStyleSetName(), "Playflow.BuildAndPush"),
                false
            ));

            UE_LOG(LogTemp, Warning, TEXT("Playflow: Toolbar button registered successfully!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Playflow: User toolbar not found!"));
        }
    }
#endif

    {
        UToolMenu* MenuBar = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
        if (MenuBar)
        {
            UE_LOG(LogTemp, Warning, TEXT("Playflow: MainMenu found, adding Playflow menu..."));

            FToolMenuSection& Section = MenuBar->FindOrAddSection("Playflow");

            Section.AddSubMenu(
                "PlayflowSubMenu",
                LOCTEXT("PlayflowSubMenu", "Playflow"),
                LOCTEXT("PlayflowSubMenuTooltip", "Playflow Cloud Tools"),
                FNewToolMenuDelegate::CreateRaw(this, &FPlayflowEditorModule::FillPlayflowMenu),
                false,
                FSlateIcon(FPlayflowEditorStyle::GetStyleSetName(), "Playflow.BuildAndPush")
            );

            UE_LOG(LogTemp, Warning, TEXT("Playflow: Main menu registered successfully!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Playflow: MainMenu not found!"));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Playflow: Menu registration complete"));
}

void FPlayflowEditorModule::FillPlayflowMenu(UToolMenu* Menu)
{
    if (!Menu)
    {
        return;
    }

    FToolMenuSection& Section = Menu->AddSection("PlayflowActions", LOCTEXT("PlayflowActions", "Actions"));

    Section.AddMenuEntry(
        "BuildAndPush",
        LOCTEXT("BuildAndPush", "Build & Push to Cloud"),
        LOCTEXT("BuildAndPushTooltip", "Build Linux Server and Push to Playflow Cloud"),
        FSlateIcon(FName("EditorStyle"), "MainFrame.PackageProject"),
        FUIAction(FExecuteAction::CreateRaw(this, &FPlayflowEditorModule::OnBuildAndPush))
    );

    Section.AddMenuEntry(
        "Settings",
        LOCTEXT("Settings", "Settings"),
        LOCTEXT("SettingsTooltip", "Configure Playflow Cloud settings"),
        FSlateIcon(FName("EditorStyle"), "ProjectSettings.TabIcon"),
        FUIAction(FExecuteAction::CreateRaw(this, &FPlayflowEditorModule::OnSetAPIKey))
    );
}

TSharedRef<SWidget> FPlayflowEditorModule::MakePlayflowMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.AddMenuEntry(
        LOCTEXT("BuildAndPush", "Build & Push to Cloud"),
        LOCTEXT("BuildAndPushTooltip", "Build Linux Server and Push to Playflow Cloud"),
        FSlateIcon(FName("EditorStyle"), "MainFrame.PackageProject"),
        FUIAction(FExecuteAction::CreateRaw(this, &FPlayflowEditorModule::OnBuildAndPush))
    );

    MenuBuilder.AddMenuEntry(
        LOCTEXT("Settings", "Settings"),
        LOCTEXT("SettingsTooltip", "Configure Playflow Cloud settings"),
        FSlateIcon(FName("EditorStyle"), "ProjectSettings.TabIcon"),
        FUIAction(FExecuteAction::CreateRaw(this, &FPlayflowEditorModule::OnSetAPIKey))
    );

    return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FPlayflowEditorModule::GenerateToolbarMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.AddMenuEntry(
        LOCTEXT("BuildAndPush", "Build & Push to Cloud"),
        LOCTEXT("BuildAndPushTooltip", "Build Linux Server and Push to Playflow Cloud"),
        FSlateIcon(FName("EditorStyle"), "MainFrame.PackageProject"),
        FUIAction(FExecuteAction::CreateRaw(this, &FPlayflowEditorModule::OnBuildAndPush))
    );

    MenuBuilder.AddMenuEntry(
        LOCTEXT("Settings", "Settings"),
        LOCTEXT("SettingsTooltip", "Configure Playflow Cloud settings"),
        FSlateIcon(FName("EditorStyle"), "ProjectSettings.TabIcon"),
        FUIAction(FExecuteAction::CreateRaw(this, &FPlayflowEditorModule::OnSetAPIKey))
    );

    return MenuBuilder.MakeWidget();
}

void FPlayflowEditorModule::OnBuildAndPush()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Build and Push clicked!"));

    UPlayflowBuildTools::BuildAndPushToCloud();
}

void FPlayflowEditorModule::OnSetAPIKey()
{
    UE_LOG(LogTemp, Log, TEXT("Playflow: Settings clicked!"));

    FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
        .ShowViewer("Editor", "Plugins", "Playflow");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPlayflowEditorModule, PlayflowEditor)