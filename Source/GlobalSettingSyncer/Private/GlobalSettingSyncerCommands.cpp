#include "GlobalSettingSyncerCommands.h"

#include "Macros.h"

#define LOCTEXT_NAMESPACE "FGlobalSettingSyncerModule"

void FGlobalSettingSyncerCommands::RegisterCommands()
{
	TRACE_CPU_SCOPE;

	UI_COMMAND( OpenPluginWindow, "Global Setting Syncer", "Open Global Settings window", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( SaveToGlobal, "Save Settings Globally", "Save current settings to global location", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( LoadFromGlobal, "Load Settings Globally", "Load settings from global location", EUserInterfaceActionType::Button, FInputChord() );
}

#undef LOCTEXT_NAMESPACE