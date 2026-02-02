#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

class IDetailLayoutBuilder;
class UGlobalSettingSyncerConfig;

class FGlobalSettingSyncerCustomization : public IDetailCustomization
{
public:
	static TSharedRef< IDetailCustomization > MakeInstance();

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:
	void BuildConfigFileItems( IDetailLayoutBuilder& DetailBuilder ) const;

	TWeakObjectPtr< UGlobalSettingSyncerConfig > ConfigObject;
};