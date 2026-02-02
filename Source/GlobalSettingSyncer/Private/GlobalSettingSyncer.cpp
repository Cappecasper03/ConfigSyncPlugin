#include "GlobalSettingSyncer.h"

#include "Macros.h"
#include "GlobalSettingSyncerCommands.h"
#include "GlobalSettingSyncerConfig.h"
#include "GlobalSettingSyncerCustomization.h"
#include "GlobalSettingSyncerStyle.h"
#include "ISettingsModule.h"
#include "ToolMenus.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FGlobalSettingSyncerModule"

void FGlobalSettingSyncerModule::StartupModule()
{
	TRACE_CPU_SCOPE;

	FGlobalSettingSyncerStyle::Initialize();
	FGlobalSettingSyncerStyle::ReloadTextures();

	FGlobalSettingSyncerCommands::Register();

	PluginCommands = MakeShareable( new FUICommandList );

	PluginCommands->MapAction(
		FGlobalSettingSyncerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateStatic( &FGlobalSettingSyncerModule::PluginButtonClicked ),
		FCanExecuteAction() );

	PluginCommands->MapAction(
		FGlobalSettingSyncerCommands::Get().SaveToGlobal,
		FExecuteAction::CreateLambda( []
		{
			UGlobalSettingSyncerConfig* Config = UGlobalSettingSyncerConfig::Get();
			if( Config && Config->SaveSettingsToGlobal() )
			{
				FNotificationInfo Info( LOCTEXT( "SaveSuccess", "Settings saved globally!" ) );
				Info.ExpireDuration = 3;
				FSlateNotificationManager::Get().AddNotification( Info );
			}
			else
			{
				FNotificationInfo Info( LOCTEXT( "SaveFailed", "Failed to save settings globally!" ) );
				Info.ExpireDuration = 5;
				FSlateNotificationManager::Get().AddNotification( Info );
			}
		} ),
		FCanExecuteAction() );

	PluginCommands->MapAction(
		FGlobalSettingSyncerCommands::Get().LoadFromGlobal,
		FExecuteAction::CreateLambda( []
		{
			UGlobalSettingSyncerConfig* Config = UGlobalSettingSyncerConfig::Get();
			if( Config && Config->LoadSettingsFromGlobal() )
			{
				FNotificationInfo Info( LOCTEXT( "LoadSuccess", "Settings loaded globally! Please restart the editor." ) );
				Info.ExpireDuration = 5;
				FSlateNotificationManager::Get().AddNotification( Info );
			}
			else
			{
				FNotificationInfo Info( LOCTEXT( "LoadFailed", "Failed to load settings globally!" ) );
				Info.ExpireDuration = 5;
				FSlateNotificationManager::Get().AddNotification( Info );
			}
		} ),
		FCanExecuteAction() );

	UToolMenus::RegisterStartupCallback( FSimpleMulticastDelegate::FDelegate::CreateRaw( this, &FGlobalSettingSyncerModule::RegisterMenus ) );

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
	{
		SettingsModule->RegisterSettings( "Editor",
		                                  "Plugins",
		                                  "GlobalSettingSyncer",
		                                  LOCTEXT( "RuntimeSettingsName", "Global Setting Syncer" ),
		                                  LOCTEXT( "RuntimeSettingsDescription", "Configure Global Settings" ),
		                                  GetMutableDefault< UGlobalSettingSyncerConfig >() );
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
	PropertyModule.RegisterCustomClassLayout( UGlobalSettingSyncerConfig::StaticClass()->GetFName(),
	                                          FOnGetDetailCustomizationInstance::CreateStatic( &FGlobalSettingSyncerCustomization::MakeInstance ) );

	UGlobalSettingSyncerConfig::Get()->Initialize();
}

void FGlobalSettingSyncerModule::ShutdownModule()
{
	TRACE_CPU_SCOPE;

	UToolMenus::UnRegisterStartupCallback( this );
	UToolMenus::UnregisterOwner( this );

	FGlobalSettingSyncerStyle::Shutdown();
	FGlobalSettingSyncerCommands::Unregister();

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
		SettingsModule->UnregisterSettings( "Editor", "Plugins", "GlobalSettingSyncer" );

	if( FModuleManager::Get().IsModuleLoaded( "PropertyEditor" ) )
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
		PropertyModule.UnregisterCustomClassLayout( UGlobalSettingSyncerConfig::StaticClass()->GetFName() );
	}

	UGlobalSettingSyncerConfig::Get()->Shutdown();
}

void FGlobalSettingSyncerModule::PluginButtonClicked()
{
	TRACE_CPU_SCOPE;

	FModuleManager::LoadModuleChecked< ISettingsModule >( "Settings" ).ShowViewer( "Editor", "Plugins", "GlobalSettingSyncer" );
}

void FGlobalSettingSyncerModule::RegisterMenus()
{
	TRACE_CPU_SCOPE;

	FToolMenuOwnerScoped OwnerScoped( this );

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu( "LevelEditor.MainMenu.Window" );
		{
			FToolMenuSection& Section = Menu->FindOrAddSection( "WindowLayout" );
			Section.AddMenuEntryWithCommandList( FGlobalSettingSyncerCommands::Get().OpenPluginWindow, PluginCommands );
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FGlobalSettingSyncerModule, GlobalSettingSyncer )