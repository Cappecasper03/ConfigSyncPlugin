#include "FConfigSync.h"

#include "FConfigSyncCustomization.h"
#include "ISettingsModule.h"
#include "Macros.h"
#include "UConfigSyncSettings.h"

DEFINE_LOG_CATEGORY( LogConfigSync );

void FConfigSyncModule::StartupModule()
{
	TRACE_CPU_SCOPE;

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
	{
		SettingsModule->RegisterSettings( "Editor",
		                                  "Plugins",
		                                  "ConfigSync",
		                                  FText::FromString( "ConfigSync" ),
		                                  FText::FromString( "Configure ConfigSync Settings" ),
		                                  UConfigSyncSettings::Get() );
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
	PropertyModule.RegisterCustomClassLayout( UConfigSyncSettings::StaticClass()->GetFName(),
	                                          FOnGetDetailCustomizationInstance::CreateStatic( &FConfigSyncCustomization::MakeInstance ) );

	UConfigSyncSettings::Get()->Initialize();
}

void FConfigSyncModule::ShutdownModule()
{
	TRACE_CPU_SCOPE;

	UConfigSyncSettings::Get()->Shutdown();

	UToolMenus::UnRegisterStartupCallback( this );
	UToolMenus::UnregisterOwner( this );

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
		SettingsModule->UnregisterSettings( "Editor", "Plugins", "ConfigSync" );

	if( FModuleManager::Get().IsModuleLoaded( "PropertyEditor" ) )
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
		PropertyModule.UnregisterCustomClassLayout( UConfigSyncSettings::StaticClass()->GetFName() );
	}
}

void FConfigSyncModule::PluginButtonClicked()
{
	TRACE_CPU_SCOPE;

	FModuleManager::LoadModuleChecked< ISettingsModule >( "Settings" ).ShowViewer( "Editor", "Plugins", "ConfigSync" );
}

IMPLEMENT_MODULE( FConfigSyncModule, ConfigSync )