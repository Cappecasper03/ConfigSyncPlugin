#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

enum class EGlobalSettingSyncerScope : uint8;
class IDetailLayoutBuilder;
class UGlobalSettingSyncerConfig;
class FIniFileFilterTreeItem;

class FGlobalSettingSyncerCustomization : public IDetailCustomization
{
public:
	static TSharedRef< IDetailCustomization > MakeInstance();

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:
	void BuildFileFilterUI( IDetailLayoutBuilder& DetailBuilder );

	// Discover methods
	TArray< FString >                  DiscoverAllConfigFiles() const;
	TMap< FString, TArray< FString > > DiscoverFileSections( const FString& FilePath ) const;

	// UI Callbacks
	FReply OnRefreshFileList();
	FReply OnEnableAllFiles();
	FReply OnDisableAllFiles();

	// Tree item generation
	void                                 GenerateConfigFileTree();
	void                                 RebuildFileList();
	TSharedPtr< FIniFileFilterTreeItem > FindFileInPluginDirectory(
		const FString&                       FileName,
		const FString&                       PluginsDir,
		TSharedPtr< FIniFileFilterTreeItem > PluginsRootGroup,
		FString&                             OutFullPath,
		IPlatformFile&                       PlatformFile );

	// Widget creation
	TSharedRef< SWidget > CreateDirectoryGroupWidget( const TSharedPtr< FIniFileFilterTreeItem >&              DirItem,
	                                                  const TArray< TSharedPtr< EGlobalSettingSyncerScope > >& ScopeOptions,
	                                                  int32                                                    IndentLevel = 0 );
	TSharedRef< SWidget > CreateFileRowWidget( const TSharedPtr< FIniFileFilterTreeItem >& FileItem, const TArray< TSharedPtr< EGlobalSettingSyncerScope > >& ScopeOptions );
	TSharedRef< SWidget > CreateSectionsWidget( TSharedPtr< FIniFileFilterTreeItem > FileItem );

	// Filter methods
	void OnSearchTextChanged( const FText& InFilterText );
	bool PassesSearchFilter( const TSharedPtr< FIniFileFilterTreeItem >& Item ) const;

	TWeakObjectPtr< UGlobalSettingSyncerConfig > ConfigObject;

	// Tree items for all config files
	TArray< TSharedPtr< FIniFileFilterTreeItem > > ConfigTreeItems;

	// Search filter text
	FText SearchFilterText;

	// Widget references for dynamic updates
	TSharedPtr< SVerticalBox > FileListContainer;
};

// Tree item structure for file/section hierarchy
class FIniFileFilterTreeItem : public TSharedFromThis< FIniFileFilterTreeItem >
{
public:
	enum class EItemType
	{
		Directory,
		File,
		Section
	};

	FIniFileFilterTreeItem( const FString& InName, EItemType InType )
	: Name( InName )
	, Type( InType )
	, bEnabled( true ) {}

	FString                                        Name;
	EItemType                                      Type;
	bool                                           bEnabled;
	TArray< TSharedPtr< FIniFileFilterTreeItem > > Children;
	TWeakPtr< FIniFileFilterTreeItem >             Parent;

	void AddChild( TSharedPtr< FIniFileFilterTreeItem > Child )
	{
		Child->Parent = AsShared();
		Children.Add( Child );
	}

	bool IsDirectory() const { return Type == EItemType::Directory; }
	bool IsFile() const { return Type == EItemType::File; }
	bool IsSection() const { return Type == EItemType::Section; }
};