#include "UConfigSyncSettings.h"

#include "FConfigSync.h"
#include "JsonObjectConverter.h"
#include "Macros.h"

UConfigSyncSettings* UConfigSyncSettings::Get()
{
	TRACE_CPU_SCOPE;

	if( !Instance )
	{
		Instance = GetMutableDefault< UConfigSyncSettings >();
		Instance->AddToRoot();
		Instance->LoadPluginSettings();
		Instance->DiscoverAndAddConfigFiles();
	}

	return Instance;
}

void UConfigSyncSettings::DiscoverAndAddConfigFiles()
{
	TRACE_CPU_SCOPE;

	IPlatformFile&    PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray< FString > AllConfigPaths;

	static const FString ProjectConfigDir = FPaths::ProjectConfigDir();
	TArray< FString >    ConfigDirFiles;
	PlatformFile.FindFiles( ConfigDirFiles, *ProjectConfigDir, TEXT( ".ini" ) );

	for( const FString& FilePath: ConfigDirFiles )
		AllConfigPaths.AddUnique( FilePath );

	static const FString SavedConfigDir = FPaths::Combine( FPaths::ProjectSavedDir(), "Config" );
	TArray< FString >    SavedConfigFiles;
	PlatformFile.FindFilesRecursively( SavedConfigFiles, *SavedConfigDir, TEXT( ".ini" ) );

	for( const FString& FilePath: SavedConfigFiles )
		AllConfigPaths.AddUnique( FilePath );

	static const FString PluginsDir = FPaths::ProjectPluginsDir();
	TArray< FString >    PluginConfigFiles;
	PlatformFile.FindFilesRecursively( PluginConfigFiles, *PluginsDir, TEXT( ".ini" ) );

	for( const FString& FilePath: PluginConfigFiles )
		AllConfigPaths.AddUnique( FilePath );

	TSet< FString > ExistingFileNames;
	for( const FConfigFileSettings& Filter: ConfigFileSettingsStruct.Settings )
		ExistingFileNames.Add( Filter.FileName );

	static const FString ProjectDir = FPaths::ProjectDir();
	int32                FilesAdded = 0;
	for( FString& ConfigPath: AllConfigPaths )
	{
		const FString Filename = FPaths::GetCleanFilename( ConfigPath );
		if( ExistingFileNames.Contains( Filename ) )
			continue;

		FConfigFileSettings Setting{};
		Setting.FileName = Filename;

		FPaths::MakePathRelativeTo( ConfigPath, *ProjectDir );
		Setting.RelativePath = ConfigPath;

		FilesAdded++;
		ConfigFileSettingsStruct.Settings.Add( Setting );
	}

	if( FilesAdded > 0 )
		SavePluginSettings();
}

void UConfigSyncSettings::SaveSettingsToGlobal()
{
	TRACE_CPU_SCOPE;

	static const FString ProjectDir = FPaths::ProjectDir();
	for( const FConfigFileSettings& Setting: ConfigFileSettingsStruct.Settings )
	{
		if( !Setting.bEnabled )
			continue;

		const FString Source      = FPaths::Combine( ProjectDir, Setting.RelativePath );
		const FString Destination = FPaths::Combine( GetScopedSettingsDirectory( Setting.SettingsScope ), Setting.RelativePath );
		CopyIniFile( Source, Destination );
	}
}

void UConfigSyncSettings::LoadSettingsFromGlobal()
{
	TRACE_CPU_SCOPE;

	static const FString ProjectDir = FPaths::ProjectDir();
	for( const FConfigFileSettings& Setting: ConfigFileSettingsStruct.Settings )
	{
		if( !Setting.bEnabled )
			continue;

		const FString Source      = FPaths::Combine( GetScopedSettingsDirectory( Setting.SettingsScope ), Setting.RelativePath );
		const FString Destination = FPaths::Combine( ProjectDir, Setting.RelativePath );
		CopyIniFile( Source, Destination );
	}
}

void UConfigSyncSettings::OnSettingsChanged()
{
	TRACE_CPU_SCOPE;

	SavePluginSettings();
	SaveSettingsToGlobal();
}

void UConfigSyncSettings::SavePluginSettings() const
{
	TRACE_CPU_SCOPE;

	const FString SettingsFilePath = GetPluginSettingsFilePath();
	const FString SettingsDir      = FPaths::GetPath( SettingsFilePath );
	if( !EnsureDirectoryExists( SettingsDir ) )
	{
		UE_LOG( ConfigSync, Error, TEXT( "Failed to create settings directory: %s" ), *SettingsDir );
		return;
	}

	FString OutputString;
	if( !FJsonObjectConverter::UStructToJsonObjectString( ConfigFileSettingsStruct, OutputString, 0, 0, 0, nullptr, true ) )
	{
		UE_LOG( ConfigSync, Error, TEXT( "Failed to convert settings JSON: %s" ), *SettingsFilePath );
		return;
	}

	if( !FFileHelper::SaveStringToFile( OutputString, *SettingsFilePath ) )
	{
		UE_LOG( ConfigSync, Error, TEXT( "Failed to write settings file: %s" ), *SettingsFilePath );
		return;
	}

	UE_LOG( ConfigSync, Log, TEXT( "Plugin settings saved to: %s" ), *SettingsFilePath );
}

void UConfigSyncSettings::LoadPluginSettings()
{
	TRACE_CPU_SCOPE;

	const FString SettingsFilePath = GetPluginSettingsFilePath();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if( !PlatformFile.FileExists( *SettingsFilePath ) )
		return;

	FString JsonString;
	if( !FFileHelper::LoadFileToString( JsonString, *SettingsFilePath ) )
	{
		UE_LOG( ConfigSync, Error, TEXT( "Failed to read settings file: %s" ), *SettingsFilePath );
		return;
	}

	if( !FJsonObjectConverter::JsonObjectStringToUStruct( JsonString, &ConfigFileSettingsStruct ) )
	{
		UE_LOG( ConfigSync, Error, TEXT( "Failed to convert settings JSON: %s" ), *SettingsFilePath );
		return;
	}

	UE_LOG( ConfigSync, Log, TEXT( "Plugin settings loaded from: %s" ), *SettingsFilePath );
}

void UConfigSyncSettings::EnableAutoSync()
{
	TRACE_CPU_SCOPE;

	if( AutoSyncHandle.IsValid() )
		return;

	AutoSyncHandle = FTSTicker::GetCoreTicker().AddTicker( FTickerDelegate::CreateUObject( this, &UConfigSyncSettings::AutoSyncTick ), 10 );
}

void UConfigSyncSettings::DisableAutoSync() const
{
	TRACE_CPU_SCOPE;

	FTSTicker::GetCoreTicker().RemoveTicker( AutoSyncHandle );
}

bool UConfigSyncSettings::AutoSyncTick( const float DeltaTime )
{
	TRACE_CPU_SCOPE;

	IFileManager& FileManager = IFileManager::Get();

	static const FString ProjectDir = FPaths::ProjectDir();
	for( const FConfigFileSettings& Setting: ConfigFileSettingsStruct.Settings )
	{
		if( !Setting.bEnabled || !Setting.bAutoSyncEnabled )
			continue;

		const FString Source      = FPaths::Combine( ProjectDir, Setting.RelativePath );
		const FString Destination = FPaths::Combine( GetScopedSettingsDirectory( Setting.SettingsScope ), Setting.RelativePath );

		if( FileManager.FileSize( *Source ) == FileManager.FileSize( *Destination ) )
			continue;

		CopyIniFile( Source, Destination );
	}

	return true;
}

bool UConfigSyncSettings::CopyIniFile( const FString& Source, const FString& Destination )
{
	TRACE_CPU_SCOPE;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if( !PlatformFile.FileExists( *Source ) )
		return false;

	if( !EnsureDirectoryExists( FPaths::GetPath( Destination ) ) )
		return false;

	return PlatformFile.CopyFile( *Destination, *Source );
}

bool UConfigSyncSettings::EnsureDirectoryExists( const FString& DirectoryPath )
{
	TRACE_CPU_SCOPE;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if( PlatformFile.DirectoryExists( *DirectoryPath ) )
		return true;

	return PlatformFile.CreateDirectoryTree( *DirectoryPath );
}

FString UConfigSyncSettings::GetScopedSettingsDirectory( const EConfigSyncScope Scope )
{
	TRACE_CPU_SCOPE;

	static FString UserSettingsDir = FPlatformProcess::UserSettingsDir();
	FString        BaseDir         = FPaths::Combine( UserSettingsDir, "UnrealEngine", "ConfigSync" );
	switch( Scope )
	{
		case EConfigSyncScope::Global:
			return FPaths::Combine( BaseDir, "Global" );
		case EConfigSyncScope::PerEngineVersion:
		{
			static FString EngineVersion = FString::Printf( TEXT( "%d.%d" ), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION );
			return FPaths::Combine( BaseDir, "PerEngineVersion", EngineVersion );
		}
		case EConfigSyncScope::PerProject:
			return FPaths::Combine( BaseDir, "PerProject", FApp::GetProjectName() );
		default:
			return BaseDir;
	}
}

FString UConfigSyncSettings::GetPluginSettingsFilePath()
{
	TRACE_CPU_SCOPE;
	return FPaths::Combine( GetScopedSettingsDirectory( EConfigSyncScope::PerProject ), "ConfigSyncSettings.json" );
}

UConfigSyncSettings* UConfigSyncSettings::Instance = nullptr;