#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGlobalSettingSyncerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static void PluginButtonClicked();

	void RegisterMenus();

	TSharedPtr< FUICommandList > PluginCommands;
};