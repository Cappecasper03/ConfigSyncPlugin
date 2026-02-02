#include "GlobalSettingSyncerConfig.h"

#include "Macros.h"
#include "DirectoryWatcherModule.h"
#include "Editor.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "IDirectoryWatcher.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

UGlobalSettingSyncerConfig::UGlobalSettingSyncerConfig()
: LastSyncTime( FDateTime::MinValue() )
{
	TRACE_CPU_SCOPE;

	CategoryName = "Plugins";

	InitializeDefaultFileFilters();
}

FString UGlobalSettingSyncerConfig::GetGlobalSettingSyncerDirectory()
{
	TRACE_CPU_SCOPE;

	static FString BaseDir = FPlatformProcess::UserSettingsDir();
	return FPaths::Combine( BaseDir, "UnrealEngine", "GlobalSettingSyncer" );
}

FString UGlobalSettingSyncerConfig::GetScopedSettingsDirectory( EGlobalSettingSyncerScope Scope )
{
	TRACE_CPU_SCOPE;

	FString BaseDir = GetGlobalSettingSyncerDirectory();

	switch( Scope )
	{
		case EGlobalSettingSyncerScope::Global:
			return FPaths::Combine( BaseDir, "Global" );
		case EGlobalSettingSyncerScope::PerEngineVersion:
		{
			static FString EngineVersion = FString::Printf( TEXT( "%d.%d" ), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION );
			return FPaths::Combine( BaseDir, "PerEngineVersion", EngineVersion );
		}
		case EGlobalSettingSyncerScope::PerProject:
			return FPaths::Combine( BaseDir, "PerProject", FApp::GetProjectName() );
		default:
			return FPaths::Combine( BaseDir, "Global" );
	}
}

UGlobalSettingSyncerConfig* UGlobalSettingSyncerConfig::Get()
{
	TRACE_CPU_SCOPE;

	if( !Instance )
	{
		Instance = GetMutableDefault< UGlobalSettingSyncerConfig >();
		Instance->AddToRoot();
	}

	return Instance;
}

bool UGlobalSettingSyncerConfig::SaveSettingsToGlobal()
{
	TRACE_CPU_SCOPE;

	bool bSuccess = false;

	// Save each enabled file to its configured scope
	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const FIniFileFilter& Filter: ConfigFileFilters )
	{
		if( !Filter.bEnabled )
			continue;

		const FString TargetDirectory = GetScopedSettingsDirectory( Filter.SettingsScope );

		if( !EnsureDirectoryExists( TargetDirectory ) )
			continue;

		// Save this specific file
		if( SaveConfigFile( Filter, TargetDirectory ) )
			bSuccess = true;
	}

	if( bSuccess )
		LastSyncTime = FDateTime::Now();

	return bSuccess;
}

bool UGlobalSettingSyncerConfig::LoadSettingsFromGlobal()
{
	TRACE_CPU_SCOPE;

	bool bSuccess = false;

	// Load each enabled file from its configured scope
	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const FIniFileFilter& Filter: ConfigFileFilters )
	{
		if( !Filter.bEnabled )
			continue;

		const FString SourceDirectory = GetScopedSettingsDirectory( Filter.SettingsScope );

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if( !PlatformFile.DirectoryExists( *SourceDirectory ) )
			continue;

		// Load this specific file
		if( LoadConfigFile( Filter, SourceDirectory ) )
			bSuccess = true;
	}

	if( bSuccess )
	{
		GConfig->Flush( false );
		LastSyncTime = FDateTime::Now();
	}

	return bSuccess;
}

TArray< FString > UGlobalSettingSyncerConfig::DiscoverAllConfigFiles()
{
	TRACE_CPU_SCOPE;

	TArray< FString > AllConfigFiles;
	IPlatformFile&    PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TRACE_CPU_SCOPE_STR( "ProjectConfigDiscovery" );
	const FString     ProjectConfigDir = FPaths::ProjectConfigDir();
	TArray< FString > ConfigDirFiles;
	PlatformFile.FindFiles( ConfigDirFiles, *ProjectConfigDir, TEXT( ".ini" ) );

	TRACE_CPU_SCOPE_STR( "ProjectConfigLoop" );
	for( const FString& FilePath: ConfigDirFiles )
	{
		FString FileName = FPaths::GetCleanFilename( FilePath );
		AllConfigFiles.AddUnique( FileName );
	}

	TRACE_CPU_SCOPE_STR( "SavedConfigDiscovery" );
	const FString SavedConfigDir = FPaths::ProjectSavedDir() / TEXT( "Config" );
	if( PlatformFile.DirectoryExists( *SavedConfigDir ) )
	{
		TArray< FString > SavedConfigFiles;
		PlatformFile.FindFilesRecursively( SavedConfigFiles, *SavedConfigDir, TEXT( ".ini" ) );

		TRACE_CPU_SCOPE_STR( "SavedConfigLoop" );
		for( const FString& FilePath: SavedConfigFiles )
		{
			FString FileName = FPaths::GetCleanFilename( FilePath );

			if( !FileName.Contains( TEXT( ".bak" ) ) &&
			    !FileName.Contains( TEXT( ".tmp" ) ) &&
			    !FileName.Contains( TEXT( "~" ) ) )
			{
				AllConfigFiles.AddUnique( FileName );
			}
		}
	}

	TRACE_CPU_SCOPE_STR( "PluginConfigDiscovery" );
	const FString PluginsDir = FPaths::ProjectPluginsDir();
	if( PlatformFile.DirectoryExists( *PluginsDir ) )
	{
		TArray< FString > PluginConfigFiles;
		PlatformFile.FindFilesRecursively( PluginConfigFiles, *PluginsDir, TEXT( ".ini" ) );

		TRACE_CPU_SCOPE_STR( "PluginConfigLoop" );
		for( const FString& FilePath: PluginConfigFiles )
		{
			if( FilePath.Contains( TEXT( "/Config/" ) ) || FilePath.Contains( TEXT( "\\Config\\" ) ) )
			{
				FString FileName = FPaths::GetCleanFilename( FilePath );
				if( !FileName.Contains( TEXT( ".bak" ) ) &&
				    !FileName.Contains( TEXT( ".tmp" ) ) )
				{
					AllConfigFiles.AddUnique( FileName );
				}
			}
		}
	}

	return AllConfigFiles;
}

bool UGlobalSettingSyncerConfig::SaveConfigFile( const FIniFileFilter& Filter, const FString& TargetDirectory )
{
	TRACE_CPU_SCOPE;

	const FString ProjectConfigDir = FPaths::ProjectConfigDir();
	const FString SavedConfigDir   = FPaths::ProjectSavedDir() / TEXT( "Config" );

	FString SourcePath = FPaths::Combine( ProjectConfigDir, Filter.FileName );

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if( !PlatformFile.FileExists( *SourcePath ) )
	{
		SourcePath = FPaths::Combine( SavedConfigDir, Filter.FileName );
	}

	const FString DestinationPath = FPaths::Combine( TargetDirectory, "Config", Filter.FileName );

	if( Filter.bFilterSections )
	{
		return CopyIniFileFiltered( SourcePath, DestinationPath, Filter );
	}
	else
	{
		return CopyIniFile( SourcePath, DestinationPath );
	}
}

bool UGlobalSettingSyncerConfig::LoadConfigFile( const FIniFileFilter& Filter, const FString& SourceDirectory )
{
	TRACE_CPU_SCOPE;

	const FString ProjectConfigDir = FPaths::ProjectConfigDir();
	const FString SavedConfigDir   = FPaths::ProjectSavedDir() / TEXT( "Config" );

	const FString SourcePath = FPaths::Combine( SourceDirectory, "Config", Filter.FileName );

	FString DestinationPath;
	if( Filter.FileName.Contains( TEXT( "Layout" ) ) ||
	    Filter.FileName.Contains( TEXT( "UserSettings" ) ) ||
	    Filter.FileName.Contains( TEXT( "AssetEditor" ) ) )
	{
		DestinationPath = FPaths::Combine( SavedConfigDir, Filter.FileName );
	}
	else
	{
		DestinationPath = FPaths::Combine( ProjectConfigDir, Filter.FileName );
	}

	if( Filter.bFilterSections )
	{
		return CopyIniFileFiltered( SourcePath, DestinationPath, Filter );
	}
	else
	{
		return CopyIniFile( SourcePath, DestinationPath );
	}
}

bool UGlobalSettingSyncerConfig::SaveConfigFiles( const FString& TargetDirectory )
{
	TRACE_CPU_SCOPE;

	UGlobalSettingSyncerConfig*     Config      = Get();
	const TArray< FIniFileFilter >& FileFilters = Config->ConfigFileFilters;

	bool          bSuccess         = false;
	const FString ProjectConfigDir = FPaths::ProjectConfigDir();
	const FString SavedConfigDir   = FPaths::ProjectSavedDir() / TEXT( "Config" );

	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const FIniFileFilter& Filter: FileFilters )
	{
		if( !Filter.bEnabled )
			continue;

		FString SourcePath = FPaths::Combine( ProjectConfigDir, Filter.FileName );

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if( !PlatformFile.FileExists( *SourcePath ) )
		{
			SourcePath = FPaths::Combine( SavedConfigDir, Filter.FileName );
		}

		const FString DestinationPath = FPaths::Combine( TargetDirectory, "Config", Filter.FileName );

		if( Filter.bFilterSections )
		{
			if( CopyIniFileFiltered( SourcePath, DestinationPath, Filter ) )
				bSuccess = true;
		}
		else
		{
			if( CopyIniFile( SourcePath, DestinationPath ) )
				bSuccess = true;
		}
	}

	return bSuccess;
}

bool UGlobalSettingSyncerConfig::LoadConfigFiles( const FString& SourceDirectory )
{
	TRACE_CPU_SCOPE;

	UGlobalSettingSyncerConfig*     Config      = Get();
	const TArray< FIniFileFilter >& FileFilters = Config->ConfigFileFilters;

	bool    bSuccess         = false;
	FString ProjectConfigDir = FPaths::ProjectConfigDir();
	FString SavedConfigDir   = FPaths::ProjectSavedDir() / TEXT( "Config" );

	TRACE_CPU_SCOPE_STR( "FileLoop" );
	for( const FIniFileFilter& Filter: FileFilters )
	{
		if( !Filter.bEnabled )
			continue;

		const FString SourcePath = FPaths::Combine( SourceDirectory, "Config", Filter.FileName );

		FString DestinationPath;
		if( Filter.FileName.Contains( TEXT( "Layout" ) ) ||
		    Filter.FileName.Contains( TEXT( "UserSettings" ) ) ||
		    Filter.FileName.Contains( TEXT( "AssetEditor" ) ) )
		{
			DestinationPath = FPaths::Combine( SavedConfigDir, Filter.FileName );
		}
		else
		{
			DestinationPath = FPaths::Combine( ProjectConfigDir, Filter.FileName );
		}

		if( Filter.bFilterSections )
		{
			if( CopyIniFileFiltered( SourcePath, DestinationPath, Filter ) )
				bSuccess = true;
		}
		else
		{
			if( CopyIniFile( SourcePath, DestinationPath ) )
				bSuccess = true;
		}
	}

	return bSuccess;
}

bool UGlobalSettingSyncerConfig::CopyIniFile( const FString& SourcePath, const FString& DestPath )
{
	TRACE_CPU_SCOPE;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if( !PlatformFile.FileExists( *SourcePath ) )
		return false;

	const FString DestDir = FPaths::GetPath( DestPath );
	if( !EnsureDirectoryExists( DestDir ) )
		return false;

	return PlatformFile.CopyFile( *DestPath, *SourcePath );
}

bool UGlobalSettingSyncerConfig::CopyIniFileFiltered( const FString& SourcePath, const FString& DestPath, const FIniFileFilter& Filter )
{
	TRACE_CPU_SCOPE;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if( !PlatformFile.FileExists( *SourcePath ) )
		return false;

	const FString DestDir = FPaths::GetPath( DestPath );
	if( !EnsureDirectoryExists( DestDir ) )
		return false;

	TRACE_CPU_SCOPE_STR( "ReadSourceConfig" );
	FConfigFile SourceConfig;
	SourceConfig.Read( SourcePath );

	FConfigFile DestConfig;

	if( Filter.bFilterSections )
	{
		TRACE_CPU_SCOPE_STR( "SectionLoop" );
		for( const auto& SectionPair: AsConst( SourceConfig ) )
		{
			const FString& SectionName = SectionPair.Key;

			if( Filter.ShouldSyncSection( SectionName ) )
			{
				DestConfig.Add( SectionName, SectionPair.Value );
			}
		}
	}
	else
	{
		DestConfig = SourceConfig;
	}

	TRACE_CPU_SCOPE_STR( "WriteDestConfig" );
	DestConfig.Write( DestPath );

	return true;
}

bool UGlobalSettingSyncerConfig::EnsureDirectoryExists( const FString& DirectoryPath )
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if( PlatformFile.DirectoryExists( *DirectoryPath ) )
		return true;

	return PlatformFile.CreateDirectoryTree( *DirectoryPath );
}

void UGlobalSettingSyncerConfig::InitializeDefaultFileFilters()
{
	TRACE_CPU_SCOPE;

	if( ConfigFileFilters.Num() == 0 )
	{
		TRACE_CPU_SCOPE_STR( "FileDiscovery" );
		TArray< FString > DiscoveredFiles = DiscoverAllConfigFiles();

		TRACE_CPU_SCOPE_STR( "FileLoop" );
		for( const FString& FileName: DiscoveredFiles )
		{
			ConfigFileFilters.Add( FIniFileFilter( FileName, false ) );
		}
	}
}

TArray< FString > UGlobalSettingSyncerConfig::GetConfigFilesToSync() const
{
	TRACE_CPU_SCOPE;

	TArray< FString > ConfigFiles;

	for( const FIniFileFilter& Filter: ConfigFileFilters )
	{
		if( Filter.bEnabled )
			ConfigFiles.Add( Filter.FileName );
	}

	return ConfigFiles;
}

void UGlobalSettingSyncerConfig::EnableAutoSync()
{
	TRACE_CPU_SCOPE;

	// Check if any file has auto-sync enabled
	bool bAnyAutoSyncEnabled = false;
	for( const FIniFileFilter& Filter: ConfigFileFilters )
	{
		if( Filter.bEnabled && Filter.bAutoSyncEnabled )
		{
			bAnyAutoSyncEnabled = true;
			break;
		}
	}

	if( !bAnyAutoSyncEnabled )
		return;

	if( DirectoryWatcherHandles.Num() > 0 )
		return;

	SaveSettingsToGlobal();

	// Set up directory watchers for config directories
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked< FDirectoryWatcherModule >( "DirectoryWatcher" );
	IDirectoryWatcher*       DirectoryWatcher       = DirectoryWatcherModule.Get();

	if( DirectoryWatcher )
	{
		TRACE_CPU_SCOPE_STR( "DirectoryWatchSetup" );

		// Watch project config directory
		FString ProjectConfigDir = FPaths::ProjectConfigDir();
		if( FPaths::DirectoryExists( ProjectConfigDir ) )
		{
			FDelegateHandle Handle;
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
				ProjectConfigDir,
				IDirectoryWatcher::FDirectoryChanged::CreateUObject( this, &UGlobalSettingSyncerConfig::OnDirectoryChanged ),
				Handle,
				IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges );
			DirectoryWatcherHandles.Add( Handle );
		}

		// Watch saved config directory
		FString SavedConfigDir = FPaths::ProjectSavedDir() / TEXT( "Config" );
		if( FPaths::DirectoryExists( SavedConfigDir ) )
		{
			FDelegateHandle Handle;
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
				SavedConfigDir,
				IDirectoryWatcher::FDirectoryChanged::CreateUObject( this, &UGlobalSettingSyncerConfig::OnDirectoryChanged ),
				Handle,
				IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges );
			DirectoryWatcherHandles.Add( Handle );
		}

		// Watch project plugins config directories
		FString PluginsDir = FPaths::ProjectPluginsDir();
		if( FPaths::DirectoryExists( PluginsDir ) )
		{
			FDelegateHandle Handle;
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
				PluginsDir,
				IDirectoryWatcher::FDirectoryChanged::CreateUObject( this, &UGlobalSettingSyncerConfig::OnDirectoryChanged ),
				Handle,
				IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges );
			DirectoryWatcherHandles.Add( Handle );
		}
	}
}

void UGlobalSettingSyncerConfig::DisableAutoSync()
{
	TRACE_CPU_SCOPE;

	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked< FDirectoryWatcherModule >( "DirectoryWatcher" );
	IDirectoryWatcher*       DirectoryWatcher       = DirectoryWatcherModule.Get();

	if( DirectoryWatcher )
	{
		for( const FDelegateHandle& Handle: DirectoryWatcherHandles )
		{
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle( FPaths::ProjectConfigDir(), Handle );
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle( FPaths::ProjectSavedDir() / TEXT( "Config" ), Handle );
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle( FPaths::ProjectPluginsDir(), Handle );
		}
	}

	DirectoryWatcherHandles.Empty();
	PendingChangedFiles.Empty();

	if( ProcessChangesTimerHandle.IsValid() )
	{
		GEditor->GetTimerManager()->ClearTimer( ProcessChangesTimerHandle );
	}
}

void UGlobalSettingSyncerConfig::OnDirectoryChanged( const TArray< FFileChangeData >& FileChanges )
{
	TRACE_CPU_SCOPE;

	TRACE_CPU_SCOPE_STR( "FileChangeLoop" );
	for( const FFileChangeData& Change: FileChanges )
	{
		// Only care about .ini files that were modified
		if( Change.Action != FFileChangeData::FCA_Modified || !Change.Filename.EndsWith( TEXT( ".ini" ) ) )
			continue;

		FString FileName = FPaths::GetCleanFilename( Change.Filename );

		// Find the filter for this file
		const FIniFileFilter* MatchedFilter = nullptr;
		for( const FIniFileFilter& Filter: ConfigFileFilters )
		{
			if( Filter.bEnabled && FileName == Filter.FileName )
			{
				MatchedFilter = &Filter;
				break;
			}
		}

		// If no filter matches or file is not enabled, don't sync
		if( !MatchedFilter )
			continue;

		// Check if auto-sync is enabled for this file or any of its sections
		if( !MatchedFilter->bAutoSyncEnabled )
		{
			TRACE_CPU_SCOPE_STR( "SectionOverrideCheck" );

			// Check if any section override has auto-sync enabled
			bool bAnySectionAutoSync = false;
			for( const FIniSectionFilter& SectionFilter: MatchedFilter->SectionOverrides )
			{
				if( SectionFilter.bOverrideFileSettings && SectionFilter.bAutoSyncEnabled )
				{
					bAnySectionAutoSync = true;
					break;
				}
			}

			if( !bAnySectionAutoSync )
				continue;
		}

		// Add to pending changes and debounce
		PendingChangedFiles.Add( Change.Filename );
	}

	// Debounce: wait a short time to collect multiple changes before syncing
	if( PendingChangedFiles.Num() > 0 && !ProcessChangesTimerHandle.IsValid() )
	{
		GEditor->GetTimerManager()->SetTimer(
			ProcessChangesTimerHandle,
			[ this ]()
			{
				// Process all pending changes
				if( PendingChangedFiles.Num() > 0 )
				{
					SaveSettingsToGlobal();
					PendingChangedFiles.Empty();
				}
				ProcessChangesTimerHandle.Invalidate();
			},
			0.5f,
			false );
	}
}

void UGlobalSettingSyncerConfig::OnSettingsChanged()
{
	TRACE_CPU_SCOPE;

	// Check if any file has auto-sync enabled
	bool bAnyAutoSyncEnabled = false;
	for( const FIniFileFilter& Filter: ConfigFileFilters )
	{
		if( Filter.bEnabled && Filter.bAutoSyncEnabled )
		{
			bAnyAutoSyncEnabled = true;
			break;
		}

		// Check section overrides
		for( const FIniSectionFilter& SectionFilter: Filter.SectionOverrides )
		{
			if( SectionFilter.bOverrideFileSettings && SectionFilter.bAutoSyncEnabled )
			{
				bAnyAutoSyncEnabled = true;
				break;
			}
		}

		if( bAnyAutoSyncEnabled )
			break;
	}

	// Re-enable auto-sync if it's enabled for any file/section
	if( bAnyAutoSyncEnabled )
	{
		// Restart directory watchers with new settings
		DisableAutoSync();
		EnableAutoSync();

		// Immediately sync the changed settings
		SaveSettingsToGlobal();
	}
	else
	{
		DisableAutoSync();
	}
}

UGlobalSettingSyncerConfig* UGlobalSettingSyncerConfig::Instance = nullptr;

bool FIniFileFilter::ShouldSyncSection( const FString& SectionName ) const
{
	TRACE_CPU_SCOPE;

	if( !bFilterSections )
		return true;

	// Check if section should be included
	bool bInclude = IncludedSections.Num() == 0;
	if( IncludedSections.Num() > 0 )
	{
		for( const FString& IncludedSection: IncludedSections )
		{
			if( SectionName.Equals( IncludedSection, ESearchCase::IgnoreCase ) ||
			    SectionName.Contains( IncludedSection ) )
			{
				bInclude = true;
				break;
			}
		}
	}

	// Check if section should be excluded
	bool bExclude = false;
	for( const FString& ExcludedSection: ExcludedSections )
	{
		if( SectionName.Equals( ExcludedSection, ESearchCase::IgnoreCase ) ||
		    SectionName.Contains( ExcludedSection ) )
		{
			bExclude = true;
			break;
		}
	}

	return bInclude && !bExclude;
}

EGlobalSettingSyncerScope FIniFileFilter::GetEffectiveScopeForSection( const FString& SectionName ) const
{
	TRACE_CPU_SCOPE;

	// Check if there's a section override
	for( const FIniSectionFilter& SectionOverride: SectionOverrides )
	{
		if( SectionOverride.bOverrideFileSettings &&
		    SectionOverride.SectionName.Equals( SectionName, ESearchCase::IgnoreCase ) )
		{
			return SectionOverride.SettingsScope;
		}
	}

	// Use file-level setting
	return SettingsScope;
}

bool FIniFileFilter::GetEffectiveAutoSyncForSection( const FString& SectionName ) const
{
	TRACE_CPU_SCOPE;

	// Check if there's a section override
	for( const FIniSectionFilter& SectionOverride: SectionOverrides )
	{
		if( SectionOverride.bOverrideFileSettings &&
		    SectionOverride.SectionName.Equals( SectionName, ESearchCase::IgnoreCase ) )
		{
			return SectionOverride.bAutoSyncEnabled && SectionOverride.bEnabled;
		}
	}

	// Use file-level setting
	return bAutoSyncEnabled;
}