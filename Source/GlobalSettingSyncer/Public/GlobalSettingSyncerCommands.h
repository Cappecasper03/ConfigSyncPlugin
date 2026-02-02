#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FGlobalSettingSyncerCommands : public TCommands< FGlobalSettingSyncerCommands >
{
public:
	FGlobalSettingSyncerCommands()
	: TCommands( TEXT( "GlobalSettingSyncer" ), NSLOCTEXT( "Contexts", "GlobalSettingSyncer", "GlobalSettingSyncer Plugin" ), NAME_None, FAppStyle::GetAppStyleSetName() ) {}

	virtual void RegisterCommands() override;

	TSharedPtr< FUICommandInfo > OpenPluginWindow;
	TSharedPtr< FUICommandInfo > SaveToGlobal;
	TSharedPtr< FUICommandInfo > LoadFromGlobal;
};