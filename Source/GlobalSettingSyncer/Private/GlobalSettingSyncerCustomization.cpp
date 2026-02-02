#include "GlobalSettingSyncerCustomization.h"
#include "GlobalSettingSyncerConfig.h"
#include "Macros.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "EditorStyleSet.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "GlobalSettingSyncerCustomization"

TSharedRef< IDetailCustomization > FGlobalSettingSyncerCustomization::MakeInstance()
{
	TRACE_CPU_SCOPE;
	return MakeShareable( new FGlobalSettingSyncerCustomization );
}

void FGlobalSettingSyncerCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TRACE_CPU_SCOPE;
	TArray< TWeakObjectPtr< UObject > > ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized( ObjectsBeingCustomized );

	if( ObjectsBeingCustomized.Num() > 0 )
	{
		ConfigObject = Cast< UGlobalSettingSyncerConfig >( ObjectsBeingCustomized[ 0 ].Get() );
	}

	// Hide the default categories - we'll build custom UI
	DetailBuilder.HideCategory( "Sync Settings" );
	DetailBuilder.HideCategory( "File Filters" );

	// Build our custom UI
	BuildFileFilterUI( DetailBuilder );
}

void FGlobalSettingSyncerCustomization::BuildFileFilterUI( IDetailLayoutBuilder& DetailBuilder )
{
	TRACE_CPU_SCOPE;
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory( "Settings", FText::FromString( "Settings" ), ECategoryPriority::Important );

	// Manual Save/Load buttons
	TRACE_CPU_SCOPE_STR( "CreateHeader" );
	Category.AddCustomRow( LOCTEXT( "ManualSyncRow", "Manual Sync" ) )
	        .WholeRowContent()
	[
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2.0f )
		[
			SNew( SButton )
			.Text( LOCTEXT( "SaveToGlobal", "Save Settings Globally" ) )
			.ToolTipText( LOCTEXT( "SaveToGlobalTooltip", "Save current editor and project settings to global location" ) )
			.OnClicked_Lambda( []
			{
				UGlobalSettingSyncerConfig* Config = UGlobalSettingSyncerConfig::Get();
				if( Config && Config->SaveSettingsToGlobal() )
				{
					FNotificationInfo Info( LOCTEXT( "SaveSuccess", "Settings saved globally!" ) );
					Info.ExpireDuration = 3;
					FSlateNotificationManager::Get().AddNotification( Info );
				}
				else
				{
					FNotificationInfo Info( LOCTEXT( "SaveFailed", "Failed to save settings globally!" ) );
					Info.ExpireDuration = 5;
					FSlateNotificationManager::Get().AddNotification( Info );
				}
				return FReply::Handled();
			} )
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2.0f )
		[
			SNew( SButton )
			.Text( LOCTEXT( "LoadFromGlobal", "Load Settings Globally" ) )
			.ToolTipText( LOCTEXT( "LoadFromGlobalTooltip", "Load editor and project settings from global location" ) )
			.OnClicked_Lambda( []
			{
				UGlobalSettingSyncerConfig* Config = UGlobalSettingSyncerConfig::Get();
				if( Config && Config->LoadSettingsFromGlobal() )
				{
					FNotificationInfo Info( LOCTEXT( "LoadSuccess", "Settings loaded globally! Please restart the editor." ) );
					Info.ExpireDuration = 5;
					FSlateNotificationManager::Get().AddNotification( Info );
				}
				else
				{
					FNotificationInfo Info( LOCTEXT( "LoadFailed", "Failed to load settings globally!" ) );
					Info.ExpireDuration = 5;
					FSlateNotificationManager::Get().AddNotification( Info );
				}
				return FReply::Handled();
			} )
		]
	];

	// Refresh button
	Category.AddCustomRow( LOCTEXT( "RefreshRow", "Refresh" ) )
	        .WholeRowContent()
	[
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2.0f )
		[
			SNew( SButton )
			.Text( LOCTEXT( "RefreshFiles", "Refresh Available Files" ) )
			.ToolTipText( LOCTEXT( "RefreshFilesTooltip", "Scan project for .ini files and their sections" ) )
			.OnClicked( this, &FGlobalSettingSyncerCustomization::OnRefreshFileList )
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2.0f )
		[
			SNew( SButton )
			.Text( LOCTEXT( "EnableAll", "Enable All Files" ) )
			.OnClicked( this, &FGlobalSettingSyncerCustomization::OnEnableAllFiles )
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 2.0f )
		[
			SNew( SButton )
			.Text( LOCTEXT( "DisableAll", "Disable All Files" ) )
			.OnClicked( this, &FGlobalSettingSyncerCustomization::OnDisableAllFiles )
		]
	];

	// Generate tree data
	GenerateConfigFileTree();

	// Search box
	Category.AddCustomRow( LOCTEXT( "SearchRow", "Search" ) )
	        .WholeRowContent()
	[
		SNew( SSearchBox )
		.HintText( LOCTEXT( "SearchHint", "Search config files and sections..." ) )
		.OnTextChanged( this, &FGlobalSettingSyncerCustomization::OnSearchTextChanged )
	];

	// All Config Files Section
	Category.AddCustomRow( LOCTEXT( "ConfigFilesHeader", "Config Files" ) )
	        .WholeRowContent()
	[
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0, 8, 0, 4 )
		[
			SNew( STextBlock )
			.Text( LOCTEXT( "AllConfigFilesTitle", "All Configuration Files (Project, Editor, Plugins, Saved)" ) )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SSeparator )
		]
		+ SVerticalBox::Slot()
		.MaxHeight( 600.0f )
		.Padding( 4, 4 )
		[
			SNew( SScrollBox )
			+ SScrollBox::Slot()
			[
				SAssignNew( FileListContainer, SVerticalBox )
			]
		]
	];

	// Initial population
	RebuildFileList();
}

void FGlobalSettingSyncerCustomization::RebuildFileList()
{
	TRACE_CPU_SCOPE;
	if( !FileListContainer.IsValid() )
		return;

	FileListContainer->ClearChildren();

	// Scope options for combo box
	static TArray< TSharedPtr< EGlobalSettingSyncerScope > > ScopeOptions;
	if( ScopeOptions.Num() == 0 )
	{
		ScopeOptions.Add( MakeShareable( new EGlobalSettingSyncerScope( EGlobalSettingSyncerScope::Global ) ) );
		ScopeOptions.Add( MakeShareable( new EGlobalSettingSyncerScope( EGlobalSettingSyncerScope::PerEngineVersion ) ) );
		ScopeOptions.Add( MakeShareable( new EGlobalSettingSyncerScope( EGlobalSettingSyncerScope::PerProject ) ) );
	}

	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const TSharedPtr< FIniFileFilterTreeItem >& Item: ConfigTreeItems )
	{
		if( !Item.IsValid() )
			continue;

		// Check if this item or any of its children pass the filter
		if( !PassesSearchFilter( Item ) )
			continue;

		if( Item->IsDirectory() )
		{
			FileListContainer->AddSlot()
			                 .AutoHeight()
			[
				CreateDirectoryGroupWidget( Item, ScopeOptions, 0 )
			];
		}
		else if( Item->IsFile() )
		{
			// Standalone file (shouldn't happen with grouping, but handle it)
			FileListContainer->AddSlot()
			                 .AutoHeight()
			                 .Padding( 2, 2 )
			[
				CreateFileRowWidget( Item, ScopeOptions )
			];
		}
	}
}

TSharedRef< SWidget > FGlobalSettingSyncerCustomization::CreateDirectoryGroupWidget(
	const TSharedPtr< FIniFileFilterTreeItem >&              DirItem,
	const TArray< TSharedPtr< EGlobalSettingSyncerScope > >& ScopeOptions,
	int32                                                    IndentLevel )
{
	TRACE_CPU_SCOPE;
	TSharedRef< SVerticalBox > DirBox = SNew( SVerticalBox );

	// Directory header
	float LeftPadding = IndentLevel * 16.0f;
	DirBox->AddSlot()
	      .AutoHeight()
	      .Padding( LeftPadding, IndentLevel == 0 ? 8.0f : 4.0f, 0, 4 )
	[
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( STextBlock )
			.Text( FText::FromString( DirItem->Name ) )
			.Font( IndentLevel == 0 ? IDetailLayoutBuilder::GetDetailFontBold() : IDetailLayoutBuilder::GetDetailFont() )
			.ColorAndOpacity( IndentLevel == 0 ? FLinearColor( 0.9f, 0.9f, 0.9f ) : FLinearColor( 0.8f, 0.8f, 0.8f ) )
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0, 2, 0, IndentLevel == 0 ? 4.0f : 2.0f )
		[
			SNew( SSeparator )
			.Thickness( IndentLevel == 0 ? 1.5f : 1.0f )
		]
	];

	// Process children
	TRACE_CPU_SCOPE_STR( "ChildLoop" );
	for( const TSharedPtr< FIniFileFilterTreeItem >& ChildItem: DirItem->Children )
	{
		// Check search filter
		if( !PassesSearchFilter( ChildItem ) )
			continue;

		if( !ChildItem.IsValid() )
			continue;

		if( ChildItem->IsDirectory() )
		{
			// Nested subdirectory
			DirBox->AddSlot()
			      .AutoHeight()
			[
				CreateDirectoryGroupWidget( ChildItem, ScopeOptions, IndentLevel + 1 )
			];
		}
		else if( ChildItem->IsFile() )
		{
			// File row
			DirBox->AddSlot()
			      .AutoHeight()
			      .Padding( LeftPadding + 8.0f, 2, 2, 2 )
			[
				CreateFileRowWidget( ChildItem, ScopeOptions )
			];

			// Section rows (indented)
			if( ChildItem->Children.Num() > 0 )
			{
				DirBox->AddSlot()
				      .AutoHeight()
				      .Padding( LeftPadding + 40.0f, 0, 0, 4 )
				[
					CreateSectionsWidget( ChildItem )
				];
			}
		}
	}

	return DirBox;
}

TSharedRef< SWidget > FGlobalSettingSyncerCustomization::CreateFileRowWidget(
	const TSharedPtr< FIniFileFilterTreeItem >&              FileItem,
	const TArray< TSharedPtr< EGlobalSettingSyncerScope > >& ScopeOptions )
{
	TRACE_CPU_SCOPE;
	TRACE_CPU_SCOPE_STR( "CreateFileRow" );
	return SNew( SHorizontalBox )
	       // File checkbox
	       + SHorizontalBox::Slot()
	       .AutoWidth()
	       .VAlign( VAlign_Center )
	       .Padding( 4, 0 )
	       [
		       SNew( SCheckBox )
		       .IsChecked_Lambda( [this, FileItem]()
		       {
			       if( !ConfigObject.IsValid() )
				       return ECheckBoxState::Unchecked;

			       for( const FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
			       {
				       if( Filter.FileName == FileItem->Name )
					       return Filter.bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			       }
			       return ECheckBoxState::Unchecked;
		       } )
		       .OnCheckStateChanged_Lambda( [this, FileItem]( ECheckBoxState NewState )
		       {
			       if( !ConfigObject.IsValid() )
				       return;

			       for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
			       {
				       if( Filter.FileName == FileItem->Name )
				       {
					       Filter.bEnabled = ( NewState == ECheckBoxState::Checked );
					       ConfigObject->SaveConfig();
					       ConfigObject->OnSettingsChanged();
				       }

				       // If not found, add new filter
				       FIniFileFilter NewFilter;
				       NewFilter.FileName = FileItem->Name;
				       NewFilter.bEnabled = ( NewState == ECheckBoxState::Checked );
				       ConfigObject->ConfigFileFilters.Add( NewFilter );
				       ConfigObject->SaveConfig();
				       ConfigObject->OnSettingsChanged();
			       }
		       } )
	       ]
	       // File name
	       + SHorizontalBox::Slot()
	       .FillWidth( 0.5f )
	       .VAlign( VAlign_Center )
	       [
		       SNew( STextBlock )
		       .Text( FText::FromString( FileItem->Name ) )
		       .Font( IDetailLayoutBuilder::GetDetailFont() )
	       ]
	       // Settings Scope Combo
	       + SHorizontalBox::Slot()
	       .AutoWidth()
	       .VAlign( VAlign_Center )
	       .Padding( 8, 0 )
	       [
		       SNew( SBox )
		       .MinDesiredWidth( 120 )
		       [
			       SNew( SComboBox< TSharedPtr< EGlobalSettingSyncerScope > > )
			       .OptionsSource( &ScopeOptions )
			       .OnGenerateWidget_Lambda( []( TSharedPtr< EGlobalSettingSyncerScope > Option )
			       {
				       FString ScopeText;
				       switch( *Option )
				       {
					       case EGlobalSettingSyncerScope::Global:
						       ScopeText = "Global";
						       break;
					       case EGlobalSettingSyncerScope::PerEngineVersion:
						       ScopeText = "Per Engine";
						       break;
					       case EGlobalSettingSyncerScope::PerProject:
						       ScopeText = "Per Project";
						       break;
				       }
				       return SNew( STextBlock )
					       .Text( FText::FromString( ScopeText ) );
			       } )
			       .OnSelectionChanged_Lambda( [this, FileItem]( TSharedPtr< EGlobalSettingSyncerScope > NewScope, ESelectInfo::Type )
			       {
				       if( !ConfigObject.IsValid() || !NewScope.IsValid() )
					       return;

				       for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
				       {
					       if( Filter.FileName == FileItem->Name )
					       {
						       Filter.SettingsScope = *NewScope;
						       ConfigObject->SaveConfig();
						       ConfigObject->OnSettingsChanged();
						       return;
					       }
				       }
			       } )
			       [
				       SNew( STextBlock )
				       .Text_Lambda( [this, FileItem]()
				       {
					       if( !ConfigObject.IsValid() )
						       return FText::FromString( "Per Engine" );

					       for( const FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
					       {
						       if( Filter.FileName == FileItem->Name )
						       {
							       switch( Filter.SettingsScope )
							       {
								       case EGlobalSettingSyncerScope::Global:
									       return FText::FromString( "Global" );
								       case EGlobalSettingSyncerScope::PerEngineVersion:
									       return FText::FromString( "Per Engine" );
								       case EGlobalSettingSyncerScope::PerProject:
									       return FText::FromString( "Per Project" );
							       }
						       }
					       }
					       return FText::FromString( "Per Engine" );
				       } )
			       ]
		       ]
	       ]
	       // Auto-Sync checkbox
	       + SHorizontalBox::Slot()
	       .AutoWidth()
	       .VAlign( VAlign_Center )
	       .Padding( 8, 0 )
	       [
		       SNew( SCheckBox )
		       .IsChecked_Lambda( [this, FileItem]()
		       {
			       if( !ConfigObject.IsValid() )
				       return ECheckBoxState::Checked;

			       for( const FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
			       {
				       if( Filter.FileName == FileItem->Name )
					       return Filter.bAutoSyncEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			       }
			       return ECheckBoxState::Checked;
		       } )
		       .OnCheckStateChanged_Lambda( [this, FileItem]( ECheckBoxState NewState )
		       {
			       if( !ConfigObject.IsValid() )
				       return;

			       for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
			       {
				       if( Filter.FileName == FileItem->Name )
				       {
					       Filter.bAutoSyncEnabled = ( NewState == ECheckBoxState::Checked );
					       ConfigObject->SaveConfig();
					       ConfigObject->OnSettingsChanged();
					       return;
				       }
			       }
		       } )
		       [
			       SNew( STextBlock )
			       .Text( LOCTEXT( "AutoSync", "Auto-Sync" ) )
		       ]
	       ]
	       // Filter sections toggle
	       + SHorizontalBox::Slot()
	       .AutoWidth()
	       .VAlign( VAlign_Center )
	       .Padding( 8, 0 )
	       [
		       SNew( SCheckBox )
		       .Style( FAppStyle::Get(), "ToggleButtonCheckbox" )
		       .IsChecked_Lambda( [this, FileItem]()
		       {
			       if( !ConfigObject.IsValid() )
				       return ECheckBoxState::Unchecked;

			       for( const FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
			       {
				       if( Filter.FileName == FileItem->Name )
					       return Filter.bFilterSections ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			       }
			       return ECheckBoxState::Unchecked;
		       } )
		       .OnCheckStateChanged_Lambda( [this, FileItem]( ECheckBoxState NewState )
		       {
			       if( !ConfigObject.IsValid() )
				       return;

			       for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
			       {
				       if( Filter.FileName == FileItem->Name )
				       {
					       Filter.bFilterSections = ( NewState == ECheckBoxState::Checked );
					       ConfigObject->SaveConfig();
					       ConfigObject->OnSettingsChanged();
					       return;
				       }
			       }
		       } )
		       [
			       SNew( STextBlock )
			       .Text( LOCTEXT( "FilterSections", "Filter Sections" ) )
			       .Font( IDetailLayoutBuilder::GetDetailFontItalic() )
		       ]
	       ];
}

TSharedRef< SWidget > FGlobalSettingSyncerCustomization::CreateSectionsWidget(
	TSharedPtr< FIniFileFilterTreeItem > FileItem )
{
	TRACE_CPU_SCOPE;
	TSharedRef< SVerticalBox > SectionsBox = SNew( SVerticalBox );

	TRACE_CPU_SCOPE_STR( "SectionLoop" );
	for( const TSharedPtr< FIniFileFilterTreeItem >& SectionItem: FileItem->Children )
	{
		if( !SectionItem.IsValid() )
			continue;

		TRACE_CPU_SCOPE_STR( "CreateSectionRow" );
		SectionsBox->AddSlot()
		           .AutoHeight()
		           .Padding( 0, 1 )
		[
			SNew( SHorizontalBox )
			// Include checkbox
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign( VAlign_Center )
			.Padding( 4, 0 )
			[
				SNew( SCheckBox )
				.ToolTipText( LOCTEXT( "IncludeSectionTooltip", "Include this section when syncing" ) )
				.IsChecked_Lambda( [this, FileItem, SectionItem]()
				{
					if( !ConfigObject.IsValid() )
						return ECheckBoxState::Unchecked;

					for( const FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
					{
						if( Filter.FileName == FileItem->Name )
						{
							if( Filter.IncludedSections.Contains( SectionItem->Name ) )
								return ECheckBoxState::Checked;
						}
					}
					return ECheckBoxState::Unchecked;
				} )
				.OnCheckStateChanged_Lambda( [this, FileItem, SectionItem]( ECheckBoxState NewState )
				{
					if( !ConfigObject.IsValid() )
						return;

					for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
					{
						if( Filter.FileName == FileItem->Name )
						{
							if( NewState == ECheckBoxState::Checked )
							{
								Filter.IncludedSections.AddUnique( SectionItem->Name );
								Filter.ExcludedSections.Remove( SectionItem->Name );
							}
							else
							{
								Filter.IncludedSections.Remove( SectionItem->Name );
							}
							ConfigObject->SaveConfig();
							ConfigObject->OnSettingsChanged();
							return;
						}
					}
				} )
			]
			// Section name
			+ SHorizontalBox::Slot()
			.FillWidth( 1.0f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromString( SectionItem->Name ) )
				.Font( IDetailLayoutBuilder::GetDetailFontItalic() )
				.ColorAndOpacity( FLinearColor( 0.7f, 0.7f, 0.7f ) )
			]
			// Exclude button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign( VAlign_Center )
			.Padding( 4, 0 )
			[
				SNew( SButton )
				.ButtonStyle( FAppStyle::Get(), "HoverHintOnly" )
				.ToolTipText( LOCTEXT( "ExcludeSectionTooltip", "Exclude this section from syncing" ) )
				.OnClicked_Lambda( [this, FileItem, SectionItem]()
				{
					if( !ConfigObject.IsValid() )
						return FReply::Handled();

					for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
					{
						if( Filter.FileName == FileItem->Name )
						{
							Filter.ExcludedSections.AddUnique( SectionItem->Name );
							Filter.IncludedSections.Remove( SectionItem->Name );
							ConfigObject->SaveConfig();
							ConfigObject->OnSettingsChanged();
							break;
						}
					}
					return FReply::Handled();
				} )
				[
					SNew( STextBlock )
					.Text( LOCTEXT( "ExcludeButton", "✕" ) )
					.Font( FCoreStyle::GetDefaultFontStyle( "Bold", 10 ) )
					.ColorAndOpacity_Lambda( [this, FileItem, SectionItem]()
					{
						if( !ConfigObject.IsValid() )
							return FLinearColor::White;

						for( const FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
						{
							if( Filter.FileName == FileItem->Name )
							{
								if( Filter.ExcludedSections.Contains( SectionItem->Name ) )
									return FLinearColor::Red;
							}
						}
						return FLinearColor( 0.5f, 0.5f, 0.5f );
					} )
				]
			]
		];
	}

	return SectionsBox;
}

TArray< FString > FGlobalSettingSyncerCustomization::DiscoverAllConfigFiles() const
{
	TRACE_CPU_SCOPE;
	TRACE_CPU_SCOPE_STR( "FileDiscovery" );
	// Delegate to the config class which has comprehensive discovery
	return UGlobalSettingSyncerConfig::DiscoverAllConfigFiles();
}

TMap< FString, TArray< FString > > FGlobalSettingSyncerCustomization::DiscoverFileSections( const FString& FilePath ) const
{
	TRACE_CPU_SCOPE;
	TRACE_CPU_SCOPE_STR( "SectionDiscovery" );
	TMap< FString, TArray< FString > > FileSections;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if( !PlatformFile.FileExists( *FilePath ) )
		return FileSections;

	FConfigFile ConfigFile;
	ConfigFile.Read( FilePath );

	TArray< FString > Sections;
	for( const TTuple< FString, FConfigSection >& SectionPair: AsConst( ConfigFile ) )
	{
		Sections.Add( SectionPair.Key );
	}

	FileSections.Add( FPaths::GetCleanFilename( FilePath ), Sections );

	return FileSections;
}

void FGlobalSettingSyncerCustomization::GenerateConfigFileTree()
{
	TRACE_CPU_SCOPE;
	TRACE_CPU_SCOPE_STR( "BuildTree" );
	ConfigTreeItems.Empty();

	TArray< FString > AllConfigFiles    = DiscoverAllConfigFiles();
	const FString     ProjectConfigDir  = FPaths::ProjectConfigDir();
	const FString     SavedConfigDir    = FPaths::ProjectSavedDir() / TEXT( "Config" );
	const FString     ProjectPluginsDir = FPaths::ProjectPluginsDir();

	// Root groups
	TSharedPtr< FIniFileFilterTreeItem > ProjectConfigGroup = MakeShareable(
		new FIniFileFilterTreeItem( TEXT( "Project Config" ), FIniFileFilterTreeItem::EItemType::Directory ) );
	TSharedPtr< FIniFileFilterTreeItem > SavedConfigGroup = MakeShareable(
		new FIniFileFilterTreeItem( TEXT( "Saved Config" ), FIniFileFilterTreeItem::EItemType::Directory ) );
	TSharedPtr< FIniFileFilterTreeItem > ProjectPluginsGroup = MakeShareable(
		new FIniFileFilterTreeItem( TEXT( "Project Plugins" ), FIniFileFilterTreeItem::EItemType::Directory ) );

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const FString& FileName: AllConfigFiles )
	{
		TSharedPtr< FIniFileFilterTreeItem > FileItem = MakeShareable(
			new FIniFileFilterTreeItem( FileName, FIniFileFilterTreeItem::EItemType::File ) );

		FString                              FullPath;
		TSharedPtr< FIniFileFilterTreeItem > ParentGroup;

		// Check Project Config
		FullPath = FPaths::Combine( ProjectConfigDir, FileName );
		if( PlatformFile.FileExists( *FullPath ) )
		{
			ParentGroup = ProjectConfigGroup;
		}
		else
		{
			// Check Saved Config (with subdirectories)
			TArray< FString > SavedConfigFiles;
			if( PlatformFile.DirectoryExists( *SavedConfigDir ) )
			{
				PlatformFile.FindFilesRecursively( SavedConfigFiles, *SavedConfigDir, TEXT( ".ini" ) );
			}

			bool bFoundInSaved = false;
			for( const FString& SavedFile: SavedConfigFiles )
			{
				if( FPaths::GetCleanFilename( SavedFile ) == FileName )
				{
					FullPath = SavedFile;

					// Get relative path from Saved/Config
					FString RelativePath = SavedFile;
					FPaths::MakePathRelativeTo( RelativePath, *( SavedConfigDir + TEXT( "/" ) ) );
					FString SubDir = FPaths::GetPath( RelativePath );

					// Create nested directory structure
					ParentGroup = SavedConfigGroup;
					if( !SubDir.IsEmpty() )
					{
						TArray< FString > PathParts;
						SubDir.ParseIntoArray( PathParts, TEXT( "/" ), true );

						for( const FString& DirName: PathParts )
						{
							// Find or create subdirectory group
							TSharedPtr< FIniFileFilterTreeItem >* ExistingDir = ParentGroup->Children.FindByPredicate(
								[ &DirName ]( const TSharedPtr< FIniFileFilterTreeItem >& Item )
								{
									return Item.IsValid() && Item->IsDirectory() && Item->Name == DirName;
								} );

							if( ExistingDir )
							{
								ParentGroup = *ExistingDir;
							}
							else
							{
								TSharedPtr< FIniFileFilterTreeItem > NewDir = MakeShareable(
									new FIniFileFilterTreeItem( DirName, FIniFileFilterTreeItem::EItemType::Directory ) );
								ParentGroup->AddChild( NewDir );
								ParentGroup = NewDir;
							}
						}
					}

					bFoundInSaved = true;
					break;
				}
			}

			// Check Project Plugins
			if( !bFoundInSaved )
			{
				ParentGroup = FindFileInPluginDirectory( FileName, ProjectPluginsDir, ProjectPluginsGroup, FullPath, PlatformFile );
			}
		}

		// If we couldn't find the file, default to Project Config directory for path
		if( FullPath.IsEmpty() )
		{
			FullPath    = FPaths::Combine( ProjectConfigDir, FileName );
			ParentGroup = ProjectConfigGroup;
		}

		// Discover sections
		TMap< FString, TArray< FString > > FileSections = DiscoverFileSections( FullPath );

		if( FileSections.Contains( FileName ) )
		{
			for( const FString& SectionName: FileSections[ FileName ] )
			{
				TSharedPtr< FIniFileFilterTreeItem > SectionItem = MakeShareable(
					new FIniFileFilterTreeItem( SectionName, FIniFileFilterTreeItem::EItemType::Section ) );
				FileItem->AddChild( SectionItem );
			}
		}

		// Add the file item to its parent group
		if( ParentGroup.IsValid() )
		{
			ParentGroup->AddChild( FileItem );
		}
	}

	auto SortChildren = []( TSharedPtr< FIniFileFilterTreeItem > Group, auto& SortChildrenRef ) ->void
	{
		TRACE_CPU_SCOPE_STR( "SortTree" );
		if( !Group.IsValid() )
			return;

		// Sort children alphabetically
		Group->Children.Sort( []( const TSharedPtr< FIniFileFilterTreeItem >& A, const TSharedPtr< FIniFileFilterTreeItem >& B )
		{
			// Directories first, then files
			if( A->IsDirectory() && !B->IsDirectory() )
				return true;
			if( !A->IsDirectory() && B->IsDirectory() )
				return false;
			// Then alphabetically
			return A->Name < B->Name;
		} );

		// Recursively sort children's children
		for( const TSharedPtr< FIniFileFilterTreeItem >& Child: Group->Children )
		{
			SortChildrenRef( Child, SortChildrenRef );
		}
	};

	SortChildren( SavedConfigGroup, SortChildren );
	SortChildren( ProjectPluginsGroup, SortChildren );
	SortChildren( ProjectConfigGroup, SortChildren );

	// Add root groups to tree in desired order: Saved, Plugins, Project
	if( SavedConfigGroup->Children.Num() > 0 )
		ConfigTreeItems.Add( SavedConfigGroup );
	if( ProjectPluginsGroup->Children.Num() > 0 )
		ConfigTreeItems.Add( ProjectPluginsGroup );
	if( ProjectConfigGroup->Children.Num() > 0 )
		ConfigTreeItems.Add( ProjectConfigGroup );
}

TSharedPtr< FIniFileFilterTreeItem > FGlobalSettingSyncerCustomization::FindFileInPluginDirectory(
	const FString&                       FileName,
	const FString&                       PluginsDir,
	TSharedPtr< FIniFileFilterTreeItem > PluginsRootGroup,
	FString&                             OutFullPath,
	IPlatformFile&                       PlatformFile )
{
	TRACE_CPU_SCOPE;
	TArray< FString > PluginConfigFiles;
	if( PlatformFile.DirectoryExists( *PluginsDir ) )
	{
		PlatformFile.FindFilesRecursively( PluginConfigFiles, *PluginsDir, TEXT( ".ini" ) );
	}

	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const FString& PluginFile: PluginConfigFiles )
	{
		if( FPaths::GetCleanFilename( PluginFile ) == FileName )
		{
			OutFullPath = PluginFile;

			// Get relative path from Plugins directory
			FString RelativePath = PluginFile;
			FPaths::MakePathRelativeTo( RelativePath, *( PluginsDir + TEXT( "/" ) ) );

			// Extract plugin name and path parts
			TArray< FString > PathParts;
			FString           PathOnly = FPaths::GetPath( RelativePath );
			PathOnly.ParseIntoArray( PathParts, TEXT( "/" ), true );

			if( PathParts.Num() > 0 )
			{
				TSharedPtr< FIniFileFilterTreeItem > CurrentGroup = PluginsRootGroup;

				// First part is plugin name
				FString PluginName = PathParts[ 0 ];

				// Find or create plugin group
				TSharedPtr< FIniFileFilterTreeItem >* ExistingPlugin = CurrentGroup->Children.FindByPredicate(
					[ &PluginName ]( const TSharedPtr< FIniFileFilterTreeItem >& Item )
					{
						return Item.IsValid() && Item->IsDirectory() && Item->Name == PluginName;
					} );

				if( ExistingPlugin )
				{
					CurrentGroup = *ExistingPlugin;
				}
				else
				{
					TSharedPtr< FIniFileFilterTreeItem > PluginGroup = MakeShareable(
						new FIniFileFilterTreeItem( PluginName, FIniFileFilterTreeItem::EItemType::Directory ) );
					PluginsRootGroup->AddChild( PluginGroup );
					CurrentGroup = PluginGroup;
				}

				// Create nested subdirectories (skip "Config" directory name)
				for( int32 i = 1; i < PathParts.Num(); i++ )
				{
					if( PathParts[ i ] != TEXT( "Config" ) )
					{
						FString DirName = PathParts[ i ];

						TSharedPtr< FIniFileFilterTreeItem >* ExistingDir = CurrentGroup->Children.FindByPredicate(
							[ &DirName ]( const TSharedPtr< FIniFileFilterTreeItem >& Item )
							{
								return Item.IsValid() && Item->IsDirectory() && Item->Name == DirName;
							} );

						if( ExistingDir )
						{
							CurrentGroup = *ExistingDir;
						}
						else
						{
							TSharedPtr< FIniFileFilterTreeItem > NewDir = MakeShareable(
								new FIniFileFilterTreeItem( DirName, FIniFileFilterTreeItem::EItemType::Directory ) );
							CurrentGroup->AddChild( NewDir );
							CurrentGroup = NewDir;
						}
					}
				}

				return CurrentGroup;
			}
		}
	}

	return nullptr;
}

FReply FGlobalSettingSyncerCustomization::OnRefreshFileList()
{
	TRACE_CPU_SCOPE;
	GenerateConfigFileTree();
	return FReply::Handled();
}

FReply FGlobalSettingSyncerCustomization::OnEnableAllFiles()
{
	TRACE_CPU_SCOPE;
	if( !ConfigObject.IsValid() )
		return FReply::Handled();

	for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
	{
		Filter.bEnabled = true;
	}

	ConfigObject->SaveConfig();
	ConfigObject->OnSettingsChanged();
	return FReply::Handled();
}

FReply FGlobalSettingSyncerCustomization::OnDisableAllFiles()
{
	TRACE_CPU_SCOPE;
	if( !ConfigObject.IsValid() )
		return FReply::Handled();

	for( FIniFileFilter& Filter: ConfigObject->ConfigFileFilters )
	{
		Filter.bEnabled = false;
	}

	ConfigObject->SaveConfig();
	ConfigObject->OnSettingsChanged();
	return FReply::Handled();
}

void FGlobalSettingSyncerCustomization::OnSearchTextChanged( const FText& InFilterText )
{
	TRACE_CPU_SCOPE;
	SearchFilterText = InFilterText;
	RebuildFileList();
}

bool FGlobalSettingSyncerCustomization::PassesSearchFilter( const TSharedPtr< FIniFileFilterTreeItem >& Item ) const
{
	TRACE_CPU_SCOPE;
	if( !Item.IsValid() )
		return false;

	// If no search filter, show everything
	if( SearchFilterText.IsEmpty() )
		return true;

	FString SearchString = SearchFilterText.ToString();

	// Check if this item's name matches
	if( Item->Name.Contains( SearchString, ESearchCase::IgnoreCase ) )
		return true;

	// Check if any children match (recursively)
	for( const TSharedPtr< FIniFileFilterTreeItem >& Child: Item->Children )
	{
		if( PassesSearchFilter( Child ) )
			return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE