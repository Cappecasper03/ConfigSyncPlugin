#include "FConfigSync.h"

#include "FConfigSyncCustomization.h"
#include "ISettingsModule.h"
#include "Macros.h"
#include "UConfigSyncSettings.h"

#define LOCTEXT_NAMESPACE "FConfigSyncModule"

DEFINE_LOG_CATEGORY( ConfigSync );

void FConfigSyncModule::StartupModule()
{
	TRACE_CPU_SCOPE;

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
	{
		SettingsModule->RegisterSettings( "Editor",
		                                  "Plugins",
		                                  "ConfigSync",
		                                  LOCTEXT( "RuntimeSettingsName", "ConfigSync" ),
		                                  LOCTEXT( "RuntimeSettingsDescription", "Configure ConfigSync Settings" ),
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FConfigSyncModule, ConfigSync )