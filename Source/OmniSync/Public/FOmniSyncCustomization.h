#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class UOmniSyncSettings;

struct FPropertyHandles
{
	TSharedPtr< IPropertyHandle > EnabledHandle;
	TSharedPtr< IPropertyHandle > ScopeHandle;
	TSharedPtr< IPropertyHandle > AutoSyncHandle;
};

struct FConfigTreeItem
{
	FString Name;
	FString FullPath;
	bool    bIsFolder   = false;
	bool    bIsSettings = false;

	TSharedPtr< FPropertyHandles > PropertyHandles = nullptr;

	TArray< TSharedRef< FConfigTreeItem > > Children;
};

class FOmniSyncCustomization : public IDetailCustomization
{
public:
	static TSharedRef< IDetailCustomization > MakeInstance() { return MakeShareable( new FOmniSyncCustomization ); }

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:
	void RefreshTreeData( const IDetailLayoutBuilder& DetailBuilder );

	TSharedRef< ITableRow > OnGenerateRow( TSharedRef< FConfigTreeItem > InItem, const TSharedRef< STableViewBase >& OwnerTable ) const;
	void                    OnGetChildren( TSharedRef< FConfigTreeItem > InItem, TArray< TSharedRef< FConfigTreeItem > >& OutChildren ) { OutChildren = InItem->Children; }

	TWeakObjectPtr< UOmniSyncSettings >                      ConfigObject;
	TSharedPtr< STreeView< TSharedRef< FConfigTreeItem > > > TreeView;
	TArray< TSharedRef< FConfigTreeItem > >                  RootItems;
};