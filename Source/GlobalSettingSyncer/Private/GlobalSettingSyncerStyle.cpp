#include "GlobalSettingSyncerStyle.h"

#include "Macros.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

void FGlobalSettingSyncerStyle::Initialize()
{
	TRACE_CPU_SCOPE;

	if( !StyleInstance.IsValid() )
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *StyleInstance );
	}
}

void FGlobalSettingSyncerStyle::Shutdown()
{
	TRACE_CPU_SCOPE;

	FSlateStyleRegistry::UnRegisterSlateStyle( *StyleInstance );
	ensure( StyleInstance.IsUnique() );
	StyleInstance.Reset();
}

void FGlobalSettingSyncerStyle::ReloadTextures()
{
	TRACE_CPU_SCOPE;

	if( FSlateApplication::IsInitialized() )
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

TSharedRef< FSlateStyleSet > FGlobalSettingSyncerStyle::Create()
{
	TRACE_CPU_SCOPE;

	TSharedRef< FSlateStyleSet > Style = MakeShareable( new FSlateStyleSet( GetStyleSetName() ) );
	Style->SetContentRoot( IPluginManager::Get().FindPlugin( "GlobalSettingSyncer" )->GetBaseDir() / TEXT( "Resources" ) );

	Style->Set( "GlobalSettingSyncer.OpenPluginWindow",
	            new FSlateVectorImageBrush( Style->RootToContentDir( L"PlaceholderButtonIcon", TEXT( ".svg" ) ), FVector2D( 20.0f, 20.0f ) ) );
	return Style;
}

TSharedPtr< FSlateStyleSet > FGlobalSettingSyncerStyle::StyleInstance = nullptr;