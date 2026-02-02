#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "GlobalSettingSyncerConfig.generated.h"

UENUM( BlueprintType )
enum class EGlobalSettingSyncerScope : uint8
{
	Global,
	PerEngineVersion,
	PerProject,
};

USTRUCT( BlueprintType )
struct FIniSectionFilter
{
	GENERATED_BODY()

	FIniSectionFilter() = default;

	FIniSectionFilter( const FString& InSectionName )
	: SectionName( InSectionName ) {}

	UPROPERTY( EditAnywhere, Category = "Section Filter" )
	FString SectionName;

	UPROPERTY( EditAnywhere, Category = "Section Filter" )
	bool bEnabled = true;

	UPROPERTY( EditAnywhere, Category = "Section Filter" )
	EGlobalSettingSyncerScope SettingsScope = EGlobalSettingSyncerScope::PerEngineVersion;

	UPROPERTY( EditAnywhere, Category = "Section Filter" )
	bool bAutoSyncEnabled = true;

	UPROPERTY( EditAnywhere, Category = "Section Filter" )
	bool bOverrideFileSettings = false;
};

USTRUCT( BlueprintType )
struct FIniFileFilter
{
	GENERATED_BODY()

	FIniFileFilter() = default;

	FIniFileFilter( const FString& InFileName, const bool bInEnabled = true )
	: FileName( InFileName )
	, bEnabled( bInEnabled ) {}

	UPROPERTY( EditAnywhere, Category = "File Filter" )
	FString FileName;

	UPROPERTY( EditAnywhere, Category = "File Filter" )
	bool bEnabled = true;

	UPROPERTY( EditAnywhere, Category = "File Filter" )
	EGlobalSettingSyncerScope SettingsScope = EGlobalSettingSyncerScope::PerEngineVersion;

	UPROPERTY( EditAnywhere, Category = "File Filter" )
	bool bAutoSyncEnabled = true;

	UPROPERTY( EditAnywhere, Category = "File Filter" )
	bool bFilterSections = false;

	UPROPERTY( EditAnywhere, Category = "File Filter", meta = (EditCondition = "bFilterSections") )
	TArray< FString > IncludedSections;

	UPROPERTY( EditAnywhere, Category = "File Filter", meta = (EditCondition = "bFilterSections") )
	TArray< FString > ExcludedSections;

	UPROPERTY( EditAnywhere, Category = "File Filter" )
	TArray< FIniSectionFilter > SectionOverrides;

	// Check if a section should be synced based on this filter
	bool ShouldSyncSection( const FString& SectionName ) const;

	// Get effective settings for a section (considering overrides)
	EGlobalSettingSyncerScope GetEffectiveScopeForSection( const FString& SectionName ) const;
	bool                      GetEffectiveAutoSyncForSection( const FString& SectionName ) const;
};

UCLASS( Config="GlobalSettingSyncer", meta=(DisplayName="Global Settings Plugin") )
class GLOBALSETTINGSYNCER_API UGlobalSettingSyncerConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGlobalSettingSyncerConfig();

	UPROPERTY( Config, EditAnywhere, Category = "File Filters" )
	TArray< FIniFileFilter > ConfigFileFilters;

	static FString GetGlobalSettingSyncerDirectory();

	static FString GetScopedSettingsDirectory( EGlobalSettingSyncerScope Scope );

	static UGlobalSettingSyncerConfig* Get();

	UFUNCTION( BlueprintCallable, Category = "Global Setting Syncer" )
	bool SaveSettingsToGlobal();

	UFUNCTION( BlueprintCallable, Category = "Global Setting Syncer" )
	bool LoadSettingsFromGlobal();

	void Initialize() { EnableAutoSync(); }

	void Shutdown() { DisableAutoSync(); }

	FDateTime GetLastSyncTime() const { return LastSyncTime; }

	void OnSettingsChanged();

	static TArray< FString > DiscoverAllConfigFiles();

private:
	static bool SaveConfigFile( const FIniFileFilter& Filter, const FString& TargetDirectory );
	static bool LoadConfigFile( const FIniFileFilter& Filter, const FString& SourceDirectory );

	static bool SaveConfigFiles( const FString& TargetDirectory );
	static bool LoadConfigFiles( const FString& SourceDirectory );

	static bool CopyIniFile( const FString& SourcePath, const FString& DestPath );

	static bool CopyIniFileFiltered( const FString& SourcePath, const FString& DestPath, const FIniFileFilter& Filter );

	static bool EnsureDirectoryExists( const FString& DirectoryPath );

	void InitializeDefaultFileFilters();

	TArray< FString > GetConfigFilesToSync() const;

	void EnableAutoSync();

	void DisableAutoSync();

	void OnDirectoryChanged( const TArray< struct FFileChangeData >& FileChanges );

	static UGlobalSettingSyncerConfig* Instance;

	FDateTime                 LastSyncTime;
	FTimerHandle              AutoSyncTimerHandle;
	TArray< FDelegateHandle > DirectoryWatcherHandles;
	TSet< FString >           PendingChangedFiles;
	FTimerHandle              ProcessChangesTimerHandle;
};