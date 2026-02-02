#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FGlobalSettingSyncerStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get() { return *StyleInstance; }

	static FName GetStyleSetName() { return TEXT( "GlobalSettingSyncerStyle" ); }

	static void ReloadTextures();

private:
	static TSharedRef< FSlateStyleSet > Create();

	static TSharedPtr< FSlateStyleSet > StyleInstance;
};