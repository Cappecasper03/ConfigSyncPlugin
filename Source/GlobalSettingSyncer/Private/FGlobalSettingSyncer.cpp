#include "FGlobalSettingSyncer.h"

#include "FGlobalSettingSyncerCustomization.h"
#include "ISettingsModule.h"
#include "Macros.h"
#include "UGlobalSettingSyncerConfig.h"

#define LOCTEXT_NAMESPACE "FGlobalSettingSyncerModule"

DEFINE_LOG_CATEGORY( GlobalSettingSyncer );

void FGlobalSettingSyncerModule::StartupModule()
{
	TRACE_CPU_SCOPE;

	if( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr< ISettingsModule >( "Settings" ) )
	{
		SettingsModule->RegisterSettings( "Editor",
		                                  "Plugins",
		                                  "GlobalSettingSyncer",
		                                  LOCTEXT( "RuntimeSettingsName", "Global Setting Syncer" ),
		                                  LOCTEXT( "RuntimeSettingsDescription", "Configure Global Setting Syncer Settings" ),
		                                  UGlobalSettingSyncerConfig::Get() );
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FGlobalSettingSyncerModule, GlobalSettingSyncer )