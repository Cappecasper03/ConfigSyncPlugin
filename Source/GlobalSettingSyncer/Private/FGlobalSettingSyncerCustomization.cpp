#include "FGlobalSettingSyncerCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Macros.h"
#include "UGlobalSettingSyncerConfig.h"

#define LOCTEXT_NAMESPACE "GlobalSettingSyncerCustomization"

TSharedRef< IDetailCustomization > FGlobalSettingSyncerCustomization::MakeInstance()
{
	TRACE_CPU_SCOPE;
	return MakeShareable( new FGlobalSettingSyncerCustomization );
}

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

	TSharedRef< IPropertyHandle > ConfigFileSettingsStructHandle = DetailBuilder.GetProperty( GET_MEMBER_NAME_CHECKED( UGlobalSettingSyncerConfig, ConfigFileSettingsStruct ) );
	TSharedPtr< IPropertyHandle > SettingsHandle                 = ConfigFileSettingsStructHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettingsStruct, Settings ) );

	uint32 NumElements = 0;
	SettingsHandle->GetNumChildren( NumElements );

	TMap< FString, TMap< FString, TArray< uint32 > > > CategorizedFiles;
	for( uint32 i = 0; i < NumElements; ++i )
	{
		TSharedPtr< IPropertyHandle > ElementHandle = SettingsHandle->GetChildHandle( i );
		if( !ElementHandle.IsValid() )
			continue;

		TSharedPtr< IPropertyHandle > FileNameHandle     = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, FileName ) );
		TSharedPtr< IPropertyHandle > RelativePathHandle = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, RelativePath ) );

		FString FileName;
		FString RelativePath;

		if( FileNameHandle.IsValid() )
			FileNameHandle->GetValue( FileName );

		if( RelativePathHandle.IsValid() )
			RelativePathHandle->GetValue( RelativePath );

		RelativePath.ReplaceInline( TEXT( "\\" ), TEXT( "/" ) );

		FString BaseCategory;
		FString SubCategory;
		if( RelativePath.Contains( TEXT( "Plugins/" ) ) )
		{
			BaseCategory = TEXT( "Plugin Configs" );

			int32   PluginIndex      = RelativePath.Find( TEXT( "Plugins/" ) );
			FString PathAfterPlugins = RelativePath.RightChop( PluginIndex + 8 );

			TArray< FString > PathParts;
			PathAfterPlugins.ParseIntoArray( PathParts, TEXT( "/" ) );

			if( PathParts.Num() > 0 )
			{
				SubCategory = PathParts[ 0 ];

				for( int32 j = 1; j < PathParts.Num() - 1; ++j )
				{
					if( PathParts[ j ] != TEXT( "Config" ) )
						SubCategory += TEXT( "/" ) + PathParts[ j ];
				}
			}
		}
		else if( RelativePath.Contains( TEXT( "Saved" ) ) || RelativePath.Contains( TEXT( "SavedConfig" ) ) )
		{
			BaseCategory = TEXT( "Saved Configs" );

			int32   SavedIndex     = RelativePath.Find( TEXT( "Saved" ) );
			FString PathAfterSaved = RelativePath.RightChop( SavedIndex + 5 );

			TArray< FString > PathParts;
			PathAfterSaved.ParseIntoArray( PathParts, TEXT( "/" ) );

			for( int32 j = 0; j < PathParts.Num() - 1; ++j )
			{
				if( PathParts[ j ] != TEXT( "Config" ) && !PathParts[ j ].IsEmpty() )
				{
					if( !SubCategory.IsEmpty() )
						SubCategory += TEXT( "/" );

					SubCategory += PathParts[ j ];
				}
			}
		}
		else
		{
			BaseCategory = TEXT( "Project Configs" );

			TArray< FString > PathParts;
			RelativePath.ParseIntoArray( PathParts, TEXT( "/" ) );

			for( int32 j = 0; j < PathParts.Num() - 1; ++j )
			{
				if( PathParts[ j ] != TEXT( "Config" ) && PathParts[ j ] != TEXT( "ProjectConfig" ) && !PathParts[ j ].IsEmpty() )
				{
					if( !SubCategory.IsEmpty() )
						SubCategory += TEXT( "/" );

					SubCategory += PathParts[ j ];
				}
			}
		}

		if( SubCategory.IsEmpty() )
			SubCategory = TEXT( "" );

		CategorizedFiles.FindOrAdd( BaseCategory ).FindOrAdd( SubCategory ).Add( i );
	}

	TArray< FString > SortedBaseCategories;
	CategorizedFiles.GetKeys( SortedBaseCategories );
	SortedBaseCategories.Sort();

	for( const FString& BaseCategory: SortedBaseCategories )
	{
		IDetailCategoryBuilder& BaseCategoryBuilder = DetailBuilder.EditCategory( FName( *BaseCategory ), FText::FromString( BaseCategory ) );

		const TMap< FString, TArray< uint32 > >& SubCategories = CategorizedFiles[ BaseCategory ];

		TArray< FString > SortedSubCategories;
		SubCategories.GetKeys( SortedSubCategories );
		SortedSubCategories.Sort();

		for( const FString& SubCategoryName: SortedSubCategories )
		{
			const TArray< uint32 >& FileIndices = SubCategories[ SubCategoryName ];

			IDetailGroup* SubCategoryGroup = nullptr;
			if( !SubCategoryName.IsEmpty() )
				SubCategoryGroup = &BaseCategoryBuilder.AddGroup( FName( *SubCategoryName ), FText::FromString( SubCategoryName ) );

			for( uint32 i: FileIndices )
			{
				TSharedPtr< IPropertyHandle > ElementHandle = SettingsHandle->GetChildHandle( i );
				if( !ElementHandle.IsValid() )
					continue;

				TSharedPtr< IPropertyHandle > FileNameHandle = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, FileName ) );
				TSharedPtr< IPropertyHandle > EnabledHandle  = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, bEnabled ) );
				TSharedPtr< IPropertyHandle > ScopeHandle    = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, SettingsScope ) );
				TSharedPtr< IPropertyHandle > AutoSyncHandle = ElementHandle->GetChildHandle( GET_MEMBER_NAME_CHECKED( FConfigFileSettings, bAutoSyncEnabled ) );

				FString FileName;
				if( FileNameHandle.IsValid() )
					FileNameHandle->GetValue( FileName );

				if( FileName.IsEmpty() )
					FileName = FString::Printf( TEXT( "Config File %d" ), i );

				IDetailGroup* FileGroup = nullptr;
				if( SubCategoryGroup )
					FileGroup = &SubCategoryGroup->AddGroup( FName( *FileName ), FText::FromString( FileName ) );
				else
					FileGroup = &BaseCategoryBuilder.AddGroup( FName( *FileName ), FText::FromString( FileName ) );

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
								if( EnabledHandle.IsValid() )
								{
									bool bValue = false;
									EnabledHandle->GetValue( bValue );
									return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
								}

								return ECheckBoxState::Unchecked;
							} )
							.OnCheckStateChanged_Lambda( [EnabledHandle, this]( ECheckBoxState NewState )
							{
								if( EnabledHandle.IsValid() )
								{
									EnabledHandle->SetValue( NewState == ECheckBoxState::Checked );
									if( ConfigObject.IsValid() )
									{
										ConfigObject->SavePluginSettings();
										ConfigObject->OnSettingsChanged();
									}
								}
							} )
							.ToolTipText( LOCTEXT( "EnabledTooltip", "Enable/disable syncing for this file" ) )
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
								if( EnabledHandle.IsValid() )
								{
									bool bEnabled = false;
									EnabledHandle->GetValue( bEnabled );

									if( !bEnabled )
										return LOCTEXT( "Disabled", "Disabled" );
								}

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
								if( EnabledHandle.IsValid() )
								{
									bool bEnabled = false;
									EnabledHandle->GetValue( bEnabled );
									return bEnabled ? FLinearColor( 0.7f, 0.7f, 1 ) : FLinearColor( 0.5f, 0.5f, 0.5f );
								}
								return FLinearColor( 0.7f, 0.7f, 1 );
							} )
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign( VAlign_Center )
						[
							SNew( STextBlock )
							.Text_Lambda( [EnabledHandle, AutoSyncHandle]()
							{
								if( EnabledHandle.IsValid() )
								{
									bool bEnabled = false;
									EnabledHandle->GetValue( bEnabled );

									if( !bEnabled )
										return FText::GetEmpty();
								}

								if( AutoSyncHandle.IsValid() )
								{
									bool bAutoSync = false;
									AutoSyncHandle->GetValue( bAutoSync );
									return bAutoSync ? LOCTEXT( "AutoSyncOn", "[Auto-Sync]" ) : LOCTEXT( "ManualSync", "[Manual]" );
								}

								return FText::GetEmpty();
							} )
							.Font( IDetailLayoutBuilder::GetDetailFont() )
							.ColorAndOpacity_Lambda( [AutoSyncHandle]()
							{
								if( AutoSyncHandle.IsValid() )
								{
									bool bAutoSync = false;
									AutoSyncHandle->GetValue( bAutoSync );
									return bAutoSync ? FLinearColor( 0.3f, 1, 0.3f ) : FLinearColor( 0.5f, 0.5f, 0.5f );
								}

								return FLinearColor( 0.5f, 0.5f, 0.5f );
							} )
						]
					];

				if( FileNameHandle.IsValid() )
				{
					FileGroup->AddWidgetRow().NameContent()
						[
							SNew( STextBlock )
							.Text( LOCTEXT( "FileName", "File Name" ) )
							.Font( IDetailLayoutBuilder::GetDetailFont() )
						]
						.ValueContent()
						[
							SNew( STextBlock )
							.Text_Lambda( [FileNameHandle]()
							{
								if( FileNameHandle.IsValid() )
								{
									FString Value;
									FileNameHandle->GetValue( Value );
									return FText::FromString( Value );
								}

								return FText::GetEmpty();
							} )
							.Font( IDetailLayoutBuilder::GetDetailFont() )
						];
				}

				if( ScopeHandle.IsValid() )
				{
					ScopeHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateLambda( [this]()
					{
						if( ConfigObject.IsValid() )
						{
							ConfigObject->SavePluginSettings();
							ConfigObject->OnSettingsChanged();
						}
					} ) );

					FileGroup->AddPropertyRow( ScopeHandle.ToSharedRef() )
					         .DisplayName( LOCTEXT( "SettingsScope", "Sync Scope" ) )
					         .ToolTip( LOCTEXT( "SettingsScopeTooltip",
					                            "Global: Shared across all projects\\nPer Engine: Shared for this engine version\\nPer Project: Project-specific only" ) );
				}

				if( AutoSyncHandle.IsValid() )
				{
					AutoSyncHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateLambda( [this]()
					{
						if( ConfigObject.IsValid() )
						{
							ConfigObject->SavePluginSettings();
							ConfigObject->OnSettingsChanged();
						}
					} ) );

					FileGroup->AddPropertyRow( AutoSyncHandle.ToSharedRef() )
					         .DisplayName( LOCTEXT( "AutoSync", "Auto-Sync" ) )
					         .ToolTip( LOCTEXT( "AutoSyncTooltip", "Automatically sync when this file changes" ) );
				}
			}
		}
	}

	if( NumElements == 0 )
	{
		IDetailCategoryBuilder& FilesCategory = DetailBuilder.EditCategory( "Config Files", FText::FromString( "Config Files" ), ECategoryPriority::Default );

		FilesCategory.AddCustomRow( LOCTEXT( "NoFilesRow", "No Files" ) )
		             .WholeRowContent()
		[
			SNew( STextBlock )
			.Text( LOCTEXT( "NoFilesText", "No config files configured. Click 'Discover All Config Files' to scan your project." ) )
			.AutoWrapText( true )
		];
	}
}

#undef LOCTEXT_NAMESPACE