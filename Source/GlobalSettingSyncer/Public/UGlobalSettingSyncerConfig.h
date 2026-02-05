#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "UGlobalSettingSyncerConfig.generated.h"

UENUM( BlueprintType )
enum class EGlobalSettingSyncerScope : uint8
{
	Global,
	PerEngineVersion,
	PerProject,
};

USTRUCT( BlueprintType )
struct FConfigFileSettings
{
	GENERATED_BODY()

	UPROPERTY( VisibleAnywhere )
	FString FileName;

	UPROPERTY( VisibleAnywhere )
	FString RelativePath;

	UPROPERTY( EditAnywhere )
	bool bEnabled = false;

	UPROPERTY( EditAnywhere )
	EGlobalSettingSyncerScope SettingsScope = EGlobalSettingSyncerScope::PerEngineVersion;

	UPROPERTY( EditAnywhere )
	bool bAutoSyncEnabled = true;
};

USTRUCT()
struct FConfigFileSettingsStruct
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere )
	TArray< FConfigFileSettings > Settings;
};

UCLASS()
class GLOBALSETTINGSYNCER_API UGlobalSettingSyncerConfig : public UObject
{
	GENERATED_BODY()

public:
	static UGlobalSettingSyncerConfig* Get();

	void Initialize() { EnableAutoSync(); }
	void Shutdown() const { DisableAutoSync(); }

	void DiscoverAndAddConfigFiles();
	void SaveSettingsToGlobal();
	void LoadSettingsFromGlobal();

	void OnSettingsChanged();

	UPROPERTY( EditAnywhere )
	FConfigFileSettingsStruct ConfigFileSettingsStruct;

private:
	void SavePluginSettings() const;
	void LoadPluginSettings();

	void EnableAutoSync();
	void DisableAutoSync() const;

	bool AutoSyncTick( float DeltaTime );

	static bool CopyIniFile( const FString& Source, const FString& Destination );
	static bool EnsureDirectoryExists( const FString& DirectoryPath );

	static FString GetScopedSettingsDirectory( EGlobalSettingSyncerScope Scope );
	static FString GetPluginSettingsFilePath();

	FTSTicker::FDelegateHandle AutoSyncHandle;

	static UGlobalSettingSyncerConfig* Instance;
};