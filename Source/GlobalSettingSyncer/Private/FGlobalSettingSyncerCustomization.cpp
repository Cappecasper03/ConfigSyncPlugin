#include "FGlobalSettingSyncerCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Macros.h"
#include "UGlobalSettingSyncerConfig.h"

#define LOCTEXT_NAMESPACE "GlobalSettingSyncerCustomization"

void FGlobalSettingSyncerCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TRACE_CPU_SCOPE;

	TArray< TWeakObjectPtr< > > ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized( ObjectsBeingCustomized );

	if( !ObjectsBeingCustomized.IsEmpty() )
		ConfigObject = Cast< UGlobalSettingSyncerConfig >( ObjectsBeingCustomized[ 0 ].Get() );

	IDetailCategoryBuilder& ActionsCategory = DetailBuilder.EditCategory( "Actions", FText::FromString( "Actions" ), ECategoryPriority::Important );

	ActionsCategory.AddCustomRow( LOCTEXT( "SyncActionsRow", "Sync Actions" ) ).WholeRowContent()
	[
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2 )
		[
			SNew( SButton )
			.Text( LOCTEXT( "DiscoverFiles", "Discover All Config Files" ) )
			.ToolTipText( LOCTEXT( "DiscoverFilesTooltip", "Scan the project and add all .ini files to the sync list" ) )
			.OnClicked_Lambda( [this]
			{
				if( UGlobalSettingSyncerConfig* Config = ConfigObject.Get() )
				{
					Config->DiscoverAndAddConfigFiles();

					FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
					PropertyModule.NotifyCustomizationModuleChanged();
				}

				return FReply::Handled();
			} )
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2 )
		[
			SNew( SButton )
			.Text( LOCTEXT( "SaveToGlobal", "Save to Global" ) )
			.ToolTipText( LOCTEXT( "SaveToGlobalTooltip", "Save enabled config files to their global sync locations" ) )
			.OnClicked_Lambda( [this]
			{
				if( UGlobalSettingSyncerConfig* Config = ConfigObject.Get() )
					Config->SaveSettingsToGlobal();

				return FReply::Handled();
			} )
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2 )
		[
			SNew( SButton )
			.Text( LOCTEXT( "LoadFromGlobal", "Load from Global" ) )
			.ToolTipText( LOCTEXT( "LoadFromGlobalTooltip", "Load config files from their global sync locations. Restart may be required." ) )
			.OnClicked_Lambda( [this]
			{
				if( UGlobalSettingSyncerConfig* Config = ConfigObject.Get() )
					Config->LoadSettingsFromGlobal();

				return FReply::Handled();
			} )
		]
	];

	DetailBuilder.HideProperty( DetailBuilder.GetProperty( GET_MEMBER_NAME_CHECKED( UGlobalSettingSyncerConfig, ConfigFileSettingsStruct ) ) );
	BuildConfigFileItems( DetailBuilder );
}

void FGlobalSettingSyncerCustomization::BuildConfigFileItems( IDetailLayoutBuilder& DetailBuilder ) const
{
	TRACE_CPU_SCOPE;

	if( !ConfigObject.IsValid() )
		return;

	const TSharedRef< IPropertyHandle > StructHandle   = DetailBuilder.GetProperty( GET_MEMBER_NAME_CHECKED( UGlobalSettingSyncerConfig, ConfigFileSettingsStruct ) );
	const TSharedPtr< IPropertyHandle > SettingsHandle = StructHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettingsStruct, Settings ) );

	uint32 NumElements = 0;
	SettingsHandle->GetNumChildren( NumElements );

	if( NumElements == 0 )
	{
		IDetailCategoryBuilder& FilesCategory = DetailBuilder.EditCategory( "Configs", LOCTEXT( "Configs", "Configs" ), ECategoryPriority::Default );
		FilesCategory.AddCustomRow( LOCTEXT( "NoFilesRow", "No Files" ) ).WholeRowContent()
		[
			SNew( STextBlock )
			.Text( LOCTEXT( "NoFilesText", "No config files configured. Click 'Discover All Config Files' to scan your project." ) )
			.AutoWrapText( true )
		];
		return;
	}

	TMap< FString, IDetailCategoryBuilder* > RootCache;
	TMap< FString, IDetailGroup* >           GroupCache;
	for( uint32 i = 0; i < NumElements; ++i )
	{
		TSharedPtr< IPropertyHandle > ElementHandle = SettingsHandle->GetChildHandle( i );
		if( !ElementHandle.IsValid() )
			continue;

		TSharedPtr< IPropertyHandle > RelativePathHandle = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, RelativePath ) );
		TSharedPtr< IPropertyHandle > FileNameHandle     = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, FileName ) );
		TSharedPtr< IPropertyHandle > EnabledHandle      = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, bEnabled ) );
		TSharedPtr< IPropertyHandle > ScopeHandle        = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, SettingsScope ) );
		TSharedPtr< IPropertyHandle > AutoSyncHandle     = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, bAutoSyncEnabled ) );

		FString FullRelativePath;
		if( RelativePathHandle.IsValid() )
			RelativePathHandle->GetValue( FullRelativePath );

		FString FileName;
		if( FileNameHandle.IsValid() )
			FileNameHandle->GetValue( FileName );

		FullRelativePath.ReplaceInline( TEXT( "\\" ), TEXT( "/" ) );

		FString FolderPath;
		FullRelativePath.Split( "/", &FolderPath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd );

		TArray< FString > PathParts;
		FolderPath.ParseIntoArray( PathParts, TEXT( "/" ) );

		IDetailCategoryBuilder* RootCategory = nullptr;

		if( IDetailCategoryBuilder** Find = RootCache.Find( PathParts[ 0 ] ) )
			RootCategory = *Find;
		else
		{
			RootCategory = &DetailBuilder.EditCategory( FName( PathParts[ 0 ] ), FText::FromString( PathParts[ 0 ] ), ECategoryPriority::Default );
			RootCache.Add( PathParts[ 0 ], RootCategory );
		}

		IDetailGroup* CurrentParentGroup = nullptr;
		FString       CumulativePath     = "";
		for( int32 j = 1; j < PathParts.Num(); ++j )
		{
			FString PartName = PathParts[ j ];
			CumulativePath   += "/" + PartName;

			if( !GroupCache.Contains( CumulativePath ) )
			{
				IDetailGroup* NewGroup = nullptr;
				if( CurrentParentGroup == nullptr )
					NewGroup = &RootCategory->AddGroup( FName( *CumulativePath ), FText::FromString( PartName ) );
				else
					NewGroup = &CurrentParentGroup->AddGroup( FName( *CumulativePath ), FText::FromString( PartName ) );

				GroupCache.Add( CumulativePath, NewGroup );
			}

			CurrentParentGroup = GroupCache[ CumulativePath ];
		}

		IDetailGroup* FileGroup = nullptr;
		if( CurrentParentGroup )
			FileGroup = &CurrentParentGroup->AddGroup( FName( *FullRelativePath ), FText::FromString( FileName ) );
		else
			FileGroup = &RootCategory->AddGroup( FName( *FullRelativePath ), FText::FromString( FileName ) );

		FileGroup->HeaderRow().NameContent()
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0, 0, 8, 0 )
				.VAlign( VAlign_Center )
				[
					SNew( SCheckBox )
					.IsChecked_Lambda( [EnabledHandle]()
					{
						bool bValue = false;
						if( EnabledHandle.IsValid() )
							EnabledHandle->GetValue( bValue );
						return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					} )
					.OnCheckStateChanged_Lambda( [EnabledHandle, this]( ECheckBoxState NewState )
					{
						if( EnabledHandle.IsValid() )
						{
							EnabledHandle->SetValue( NewState == ECheckBoxState::Checked );
							if( ConfigObject.IsValid() )
								ConfigObject->OnSettingsChanged();
						}
					} )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 1 )
				.VAlign( VAlign_Center )
				[
					SNew( STextBlock )
					.Text( FText::FromString( FileName ) )
					.Font( IDetailLayoutBuilder::GetDetailFontBold() )
				]
			]
			.ValueContent()
			.MinDesiredWidth( 250 )
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0, 0, 12, 0 )
				.VAlign( VAlign_Center )
				[
					SNew( STextBlock )
					.Text_Lambda( [EnabledHandle, ScopeHandle]()
					{
						bool bEnabled = false;
						if( EnabledHandle.IsValid() )
							EnabledHandle->GetValue( bEnabled );
						if( !bEnabled )
							return LOCTEXT( "Disabled", "Disabled" );

						if( ScopeHandle.IsValid() )
						{
							uint8 ScopeValue = 0;
							ScopeHandle->GetValue( ScopeValue );
							switch( static_cast< EGlobalSettingSyncerScope >( ScopeValue ) )
							{
								case EGlobalSettingSyncerScope::Global:
									return LOCTEXT( "ScopeGlobal", "Global" );
								case EGlobalSettingSyncerScope::PerEngineVersion:
									return LOCTEXT( "ScopePerEngine", "Per Engine" );
								case EGlobalSettingSyncerScope::PerProject:
									return LOCTEXT( "ScopePerProject", "Per Project" );
							}
						}
						return FText::GetEmpty();
					} )
					.Font( IDetailLayoutBuilder::GetDetailFont() )
					.ColorAndOpacity_Lambda( [EnabledHandle]()
					{
						bool bEnabled = false;
						if( EnabledHandle.IsValid() )
							EnabledHandle->GetValue( bEnabled );
						return bEnabled ? FLinearColor( 0.7f, 0.7f, 1 ) : FLinearColor( 0.5f, 0.5f, 0.5f );
					} )
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign( VAlign_Center )
				[
					SNew( STextBlock )
					.Text_Lambda( [EnabledHandle, AutoSyncHandle]()
					{
						bool bEnabled = false;
						if( EnabledHandle.IsValid() )
							EnabledHandle->GetValue( bEnabled );
						if( !bEnabled )
							return FText::GetEmpty();

						bool bAutoSync = false;
						if( AutoSyncHandle.IsValid() )
							AutoSyncHandle->GetValue( bAutoSync );
						return bAutoSync ? LOCTEXT( "AutoSyncOn", "[Auto-Sync]" ) : LOCTEXT( "ManualSync", "[Manual]" );
					} )
					.Font( IDetailLayoutBuilder::GetDetailFont() )
					.ColorAndOpacity_Lambda( [AutoSyncHandle]()
					{
						bool bAutoSync = false;
						if( AutoSyncHandle.IsValid() )
							AutoSyncHandle->GetValue( bAutoSync );
						return bAutoSync ? FLinearColor( 0.3f, 1, 0.3f ) : FLinearColor( 0.5f, 0.5f, 0.5f );
					} )
				]
			];

		if( ScopeHandle.IsValid() )
			FileGroup->AddPropertyRow( ScopeHandle.ToSharedRef() ).DisplayName( LOCTEXT( "SettingsScope", "Sync Scope" ) );

		if( AutoSyncHandle.IsValid() )
			FileGroup->AddPropertyRow( AutoSyncHandle.ToSharedRef() ).DisplayName( LOCTEXT( "AutoSync", "Auto-Sync" ) );
	}
}

#undef LOCTEXT_NAMESPACE