#include "FOmniSync.h"

#include "FOmniSyncCustomization.h"
#include "ISettingsModule.h"
#include "Macros.h"
#include "UOmniSyncSettings.h"

#define LOCTEXT_NAMESPACE "FOmniSyncModule"

DEFINE_LOG_CATEGORY( OmniSync );

void FOmniSyncModule::StartupModule()
{
	TRACE_CPU_SCOPE;

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
	{
		SettingsModule->RegisterSettings( "Editor",
		                                  "Plugins",
		                                  "OmniSync",
		                                  LOCTEXT( "RuntimeSettingsName", "OmniSync" ),
		                                  LOCTEXT( "RuntimeSettingsDescription", "Configure OmniSync Settings" ),
		                                  UOmniSyncSettings::Get() );
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
	PropertyModule.RegisterCustomClassLayout( UOmniSyncSettings::StaticClass()->GetFName(),
	                                          FOnGetDetailCustomizationInstance::CreateStatic( &FOmniSyncCustomization::MakeInstance ) );

	UOmniSyncSettings::Get()->Initialize();
}

void FOmniSyncModule::ShutdownModule()
{
	TRACE_CPU_SCOPE;

	UOmniSyncSettings::Get()->Shutdown();

	UToolMenus::UnRegisterStartupCallback( this );
	UToolMenus::UnregisterOwner( this );

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
		SettingsModule->UnregisterSettings( "Editor", "Plugins", "OmniSync" );

	if( FModuleManager::Get().IsModuleLoaded( "PropertyEditor" ) )
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
		PropertyModule.UnregisterCustomClassLayout( UOmniSyncSettings::StaticClass()->GetFName() );
	}
}

void FOmniSyncModule::PluginButtonClicked()
{
	TRACE_CPU_SCOPE;

	FModuleManager::LoadModuleChecked< ISettingsModule >( "Settings" ).ShowViewer( "Editor", "Plugins", "OmniSync" );
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FOmniSyncModule, OmniSync )