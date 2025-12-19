#include "SOTS_ShaderWarmupSubsystem.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "InstancedStruct.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "MoviePlayer.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "ShaderPipelineCache.h"
#include "SOTS_ShaderWarmupLoadListDataAsset.h"
#include "SOTS_UIRouterSubsystem.h"
#include "SOTS_UISettings.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_ShaderWarmup, Log, All);

USOTS_ShaderWarmupSubsystem* USOTS_ShaderWarmupSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<USOTS_ShaderWarmupSubsystem>();
		}
	}

	return nullptr;
}

USOTS_ShaderWarmupLoadListDataAsset* USOTS_ShaderWarmupSubsystem::ResolveLoadList(const F_SOTS_ShaderWarmupRequest& Request) const
{
	if (Request.LoadListOverride)
	{
		return Request.LoadListOverride;
	}

	if (const USOTS_UISettings* Settings = USOTS_UISettings::Get())
	{
		if (!Settings->DefaultShaderWarmupLoadList.IsNull())
		{
			return Settings->DefaultShaderWarmupLoadList.LoadSynchronous();
		}
	}

	return nullptr;
}

void USOTS_ShaderWarmupSubsystem::Deinitialize()
{
	if (bWarmupActive)
	{
		EndWarmup(true);
	}

	if (bMoviePlayerHooksBound)
	{
		FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
		FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
		bMoviePlayerHooksBound = false;
	}

	Super::Deinitialize();
}

bool USOTS_ShaderWarmupSubsystem::BeginWarmup(const F_SOTS_ShaderWarmupRequest& Request)
{
	if (bWarmupActive)
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup already active; ignoring BeginWarmup."));
		return false;
	}

	USOTS_UIRouterSubsystem* Router = USOTS_UIRouterSubsystem::Get(this);
	if (!Router)
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup failed: UI router subsystem missing."));
		return false;
	}

	const FGameplayTag WidgetId = ResolveWidgetId(Request);
	if (!WidgetId.IsValid())
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup failed: invalid widget id."));
		return false;
	}

	UUserWidget* LoadingWidget = Router->CreateWidgetById(WidgetId);
	LoadingWidgetRef = LoadingWidget;

	if (!Router->PushWidgetById(WidgetId, FInstancedStruct()))
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup failed: widget id %s was not found in the registry."), *WidgetId.ToString());
		return false;
	}

	ActiveRequest = Request;
	bWarmupActive = true;
	bWarmupReadyToTravel = false;
	WarmupStartTimeSeconds = FPlatformTime::Seconds();

	if (Request.bFreezeWithGlobalTimeDilation)
	{
		if (UWorld* World = GetWorld())
		{
			PrevTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
			UGameplayStatics::SetGlobalTimeDilation(World, 5.0f);
			bAppliedTimeDilation = true;
		}
		else
		{
			UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup freeze requested, but world was missing."));
		}
	}

	if (Request.bUseMoviePlayerDuringMapLoad && !bMoviePlayerHooksBound)
	{
		FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USOTS_ShaderWarmupSubsystem::HandlePreLoadMap);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USOTS_ShaderWarmupSubsystem::HandlePostLoadMap);
		bMoviePlayerHooksBound = true;
	}

	if (Request.bUseMoviePlayerDuringMapLoad)
	{
		SetupMoviePlayerScreen();
	}

	BuildAssetQueue(Request);
	TotalCount = AssetQueue.Num();
	DoneCount = 0;
	QueueIndex = 0;

	BroadcastProgress(0.0f, FText::FromString(TEXT("Warming up shaders...")));

	if (TotalCount == 0)
	{
		HandleWarmupComplete();
		return true;
	}

	LoadNext();
	return true;
}

void USOTS_ShaderWarmupSubsystem::EndWarmup(bool bRestoreTimeDilation)
{
	if (!bWarmupActive)
	{
		return;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UE_LOG(LogSOTS_ShaderWarmup, Log, TEXT("UI: ShaderWarmup teardown (StopMovie/PopWidget/RestoreDilation)"));
#endif

	if (CompilePollHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CompilePollHandle);
		CompilePollHandle = FTSTicker::FDelegateHandle();
	}

	if (ActiveLoadHandle.IsValid())
	{
		ActiveLoadHandle->ReleaseHandle();
		ActiveLoadHandle.Reset();
	}

	if (bRestoreTimeDilation && bAppliedTimeDilation)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
		}

		bAppliedTimeDilation = false;
	}

	if (USOTS_UIRouterSubsystem* Router = USOTS_UIRouterSubsystem::Get(this))
	{
		const FGameplayTag WidgetId = ResolveWidgetId(ActiveRequest);
		if (WidgetId.IsValid())
		{
			Router->PopWidgetById(WidgetId);
		}
	}

	if (ActiveRequest.bUseMoviePlayerDuringMapLoad && bMoviePlayerPlaying && IsMoviePlayerEnabled())
	{
		GetMoviePlayer()->StopMovie();
	}

	if (bMoviePlayerHooksBound)
	{
		FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
		FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
		bMoviePlayerHooksBound = false;
	}

	LoadingWidgetRef.Reset();
	bMoviePlayerConfigured = false;
	bMoviePlayerPlaying = false;

	AssetQueue.Reset();
	TotalCount = 0;
	DoneCount = 0;
	QueueIndex = 0;
	bWarmupActive = false;
	bWarmupReadyToTravel = false;
	WarmupStartTimeSeconds = 0.0;
}

void USOTS_ShaderWarmupSubsystem::CancelWarmup(const FText& Reason)
{
	if (!bWarmupActive)
	{
		return;
	}

	EndWarmup(true);
	OnCancelled.Broadcast(Reason);
}

void USOTS_ShaderWarmupSubsystem::BuildAssetQueue(const F_SOTS_ShaderWarmupRequest& Request)
{
	AssetQueue.Reset();
	TSet<FSoftObjectPath> SeenPaths;
	const TArray<FString> AllowedRoots = { TEXT("/Game/") };
	const TArray<FString> WhitelistedRoots;

	int32 TotalDepsVisited = 0;
	int32 TotalAssetsFound = 0;
	int32 AcceptedAssets = 0;
	int32 RejectedEngine = 0;
	int32 RejectedPlugins = 0;
	int32 RejectedScript = 0;
	int32 RejectedNiagara = 0;
	int32 RejectedOther = 0;
	int32 RejectedRedirector = 0;
	int32 RejectedEditorOnly = 0;
	bool bUsedFallbackScan = false;

	auto AddUniquePath = [&SeenPaths, this](const FSoftObjectPath& Path)
	{
		if (Path.IsNull() || SeenPaths.Contains(Path))
		{
			return;
		}

		SeenPaths.Add(Path);
		AssetQueue.Add(Path);
	};

	bool bLoadedList = false;
	if (Request.SourceMode == ESOTS_ShaderWarmupSourceMode::UseLoadListDA)
	{
		USOTS_ShaderWarmupLoadListDataAsset* LoadList = ResolveLoadList(Request);

		if (LoadList)
		{
			for (const FSoftObjectPath& Path : LoadList->Assets)
			{
				AddUniquePath(Path);
			}

			bLoadedList = AssetQueue.Num() > 0;
			TotalAssetsFound = AssetQueue.Num();
			AcceptedAssets = AssetQueue.Num();
		}
	}

	if (bLoadedList)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		UE_LOG(LogSOTS_ShaderWarmup, Log,
			TEXT("WarmupQueue: Level=%s Deps=0 Found=%d Accepted=%d Rejected(Engine=0, Plugins=0, Script=0, Niagara=0, Other=0)"),
			*Request.TargetLevelPackageName.ToString(),
			TotalAssetsFound,
			AcceptedAssets);
#endif
		return;
	}

	if (Request.TargetLevelPackageName.IsNone())
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup fallback requested, but TargetLevelPackageName is empty."));
		return;
	}

	bUsedFallbackScan = true;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& Registry = AssetRegistryModule.Get();

	TSet<FName> PackageNames;
	PackageNames.Add(Request.TargetLevelPackageName);

	TArray<FName> Dependencies;
	// Collect package dependencies with UE5.7-compatible query (defaults to no additional filters).
	UE::AssetRegistry::FDependencyQuery Query;
	Query.Required = UE::AssetRegistry::EDependencyProperty::None;
	Query.Excluded = UE::AssetRegistry::EDependencyProperty::None;
	Registry.GetDependencies(
		Request.TargetLevelPackageName,
		Dependencies,
		UE::AssetRegistry::EDependencyCategory::Package,
		Query);
	for (const FName& Dependency : Dependencies)
	{
		PackageNames.Add(Dependency);
	}

	auto IsWhitelisted = [&AllowedRoots, &WhitelistedRoots](const FString& PackagePath) -> bool
	{
		for (const FString& Root : AllowedRoots)
		{
			if (PackagePath.StartsWith(Root))
			{
				return true;
			}
		}

		for (const FString& Root : WhitelistedRoots)
		{
			if (PackagePath.StartsWith(Root))
			{
				return true;
			}
		}

		return false;
	};

	for (const FName& PackageName : PackageNames)
	{
		++TotalDepsVisited;
		const FString PackagePath = PackageName.ToString();
		TArray<FAssetData> AssetDataArray;
		Registry.GetAssetsByPackageName(PackageName, AssetDataArray);

		for (const FAssetData& AssetData : AssetDataArray)
		{
			++TotalAssetsFound;

			if (AssetData.IsRedirector())
			{
				++RejectedOther;
				++RejectedRedirector;
				continue;
			}

			if ((AssetData.PackageFlags & PKG_EditorOnly) != 0)
			{
				++RejectedOther;
				++RejectedEditorOnly;
				continue;
			}

			if (PackagePath.StartsWith(TEXT("/Engine/")))
			{
				++RejectedEngine;
				continue;
			}

			if (PackagePath.StartsWith(TEXT("/Script/")))
			{
				++RejectedScript;
				continue;
			}

			if (PackagePath.StartsWith(TEXT("/Niagara/")))
			{
				++RejectedNiagara;
				continue;
			}

			if (PackagePath.StartsWith(TEXT("/Plugins/")))
			{
				++RejectedPlugins;
				continue;
			}

			if (!IsWhitelisted(PackagePath))
			{
				++RejectedOther;
				continue;
			}

			if (AssetData.IsInstanceOf(UMaterialInterface::StaticClass()) || AssetData.IsInstanceOf(UNiagaraSystem::StaticClass()))
			{
				AddUniquePath(AssetData.ToSoftObjectPath());
				++AcceptedAssets;
			}
			else
			{
				++RejectedOther;
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UE_LOG(LogSOTS_ShaderWarmup, Log,
		TEXT("WarmupQueue: Level=%s Deps=%d Found=%d Accepted=%d Rejected(Engine=%d, Plugins=%d, Script=%d, Niagara=%d, Other=%d)"),
		*Request.TargetLevelPackageName.ToString(),
		TotalDepsVisited,
		TotalAssetsFound,
		AcceptedAssets,
		RejectedEngine,
		RejectedPlugins,
		RejectedScript,
		RejectedNiagara,
		RejectedOther);
#endif

	if (bUsedFallbackScan && AcceptedAssets == 0)
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning,
			TEXT("WarmupQueue: Level=%s produced zero accepted assets after filtering."),
			*Request.TargetLevelPackageName.ToString());
	}
}

void USOTS_ShaderWarmupSubsystem::LoadNext()
{
	if (!bWarmupActive)
	{
		return;
	}

	if (QueueIndex >= AssetQueue.Num())
	{
		BeginWaitForCompiling();
		return;
	}

	const FSoftObjectPath AssetPath = AssetQueue[QueueIndex];
	if (AssetPath.IsNull())
	{
		++QueueIndex;
		++DoneCount;
		LoadNext();
		return;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	ActiveLoadHandle = Streamable.RequestAsyncLoad(
		AssetPath,
		FStreamableDelegate::CreateUObject(this, &USOTS_ShaderWarmupSubsystem::HandleAssetLoaded, AssetPath));

	if (!ActiveLoadHandle.IsValid())
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup failed to request async load for %s."), *AssetPath.ToString());
		++QueueIndex;
		++DoneCount;
		LoadNext();
	}
}

void USOTS_ShaderWarmupSubsystem::HandleAssetLoaded(FSoftObjectPath LoadedPath)
{
	ActiveLoadHandle.Reset();

	UObject* LoadedObject = LoadedPath.ResolveObject();
	if (!LoadedObject)
	{
		LoadedObject = LoadedPath.TryLoad();
	}

	if (UMaterialInterface* Material = Cast<UMaterialInterface>(LoadedObject))
	{
		EnsureTouchMesh();
		if (UStaticMeshComponent* Mesh = TouchMesh.Get())
		{
			Mesh->SetMaterial(0, Material);
			Mesh->MarkRenderStateDirty();
		}
	}
	else if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(LoadedObject))
	{
		EnsureTouchNiagara();
		if (UNiagaraComponent* NiagaraComponent = TouchNiagara.Get())
		{
			NiagaraComponent->SetAsset(NiagaraSystem);
			NiagaraComponent->Activate(true);
			NiagaraComponent->Deactivate();
		}
	}

	++DoneCount;
	++QueueIndex;

	const float Percent = TotalCount > 0 ? static_cast<float>(DoneCount) / static_cast<float>(TotalCount) : 1.0f;
	const FString Status = FString::Printf(TEXT("Loaded %d / %d"), DoneCount, TotalCount);
	BroadcastProgress(Percent, FText::FromString(Status));

	LoadNext();
}

void USOTS_ShaderWarmupSubsystem::BeginWaitForCompiling()
{
	int32 RemainingJobs = 0;
	const bool bIsCompiling = GetCompileStatus(RemainingJobs);
	FString Status = TEXT("Finalizing...");
#if WITH_EDITOR
	if (bIsCompiling && RemainingJobs > 0)
	{
		Status = FString::Printf(TEXT("Finalizing... (Jobs remaining: %d)"), RemainingJobs);
	}
#else
	if (bIsCompiling && RemainingJobs > 0)
	{
		Status = FString::Printf(TEXT("Finalizing... (PSOs remaining: %d)"), RemainingJobs);
	}
#endif
	BroadcastProgress(1.0f, FText::FromString(Status), bIsCompiling, RemainingJobs);

	if (CompilePollHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CompilePollHandle);
		CompilePollHandle = FTSTicker::FDelegateHandle();
	}

	CompilePollHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &USOTS_ShaderWarmupSubsystem::HandleCompilePoll),
		0.1f);
}

bool USOTS_ShaderWarmupSubsystem::HandleCompilePoll(float DeltaTime)
{
	(void)DeltaTime;
	if (!bWarmupActive)
	{
		return false;
	}

	int32 RemainingJobs = 0;
	const bool bIsCompiling = GetCompileStatus(RemainingJobs);
	if (bIsCompiling)
	{
		FString Status = TEXT("Finalizing...");
#if WITH_EDITOR
		if (RemainingJobs > 0)
		{
			Status = FString::Printf(TEXT("Finalizing... (Jobs remaining: %d)"), RemainingJobs);
		}
#else
		if (RemainingJobs > 0)
		{
			Status = FString::Printf(TEXT("Finalizing... (PSOs remaining: %d)"), RemainingJobs);
		}
#endif
		BroadcastProgress(1.0f, FText::FromString(Status), bIsCompiling, RemainingJobs);
		return true;
	}

	HandleWarmupComplete();
	return false;
}

bool USOTS_ShaderWarmupSubsystem::IsStillCompiling() const
{
	int32 RemainingJobs = 0;
	return GetCompileStatus(RemainingJobs);
}

bool USOTS_ShaderWarmupSubsystem::GetCompileStatus(int32& OutRemainingJobs) const
{
#if WITH_EDITOR
	if (GShaderCompilingManager)
	{
		OutRemainingJobs = GShaderCompilingManager->GetNumPendingJobs() + GShaderCompilingManager->GetNumRemainingJobs();
		return OutRemainingJobs > 0;
	}

	OutRemainingJobs = 0;
	return false;
#else
	OutRemainingJobs = FShaderPipelineCache::NumPrecompilesRemaining();
	return OutRemainingJobs > 0;
#endif
}

void USOTS_ShaderWarmupSubsystem::HandleWarmupComplete()
{
	if (!bWarmupActive)
	{
		return;
	}

	if (!bWarmupReadyToTravel)
	{
		bWarmupReadyToTravel = true;
		OnReadyToTravel.Broadcast();
	}

	if (!ActiveRequest.bUseMoviePlayerDuringMapLoad)
	{
		FinalizeWarmup();
	}
}

void USOTS_ShaderWarmupSubsystem::FinalizeWarmup()
{
	if (!bWarmupActive)
	{
		return;
	}

	EndWarmup();
	OnFinished.Broadcast();
}

void USOTS_ShaderWarmupSubsystem::BroadcastProgress(float Percent, const FText& StatusText)
{
	int32 RemainingJobs = 0;
	const bool bIsCompiling = GetCompileStatus(RemainingJobs);
	BroadcastProgress(Percent, StatusText, bIsCompiling, RemainingJobs);
}

void USOTS_ShaderWarmupSubsystem::BroadcastProgress(float Percent, const FText& StatusText, bool bIsCompiling, int32 RemainingCompileJobs)
{
	const double NowSeconds = FPlatformTime::Seconds();
	const float ElapsedSeconds = WarmupStartTimeSeconds > 0.0 ? static_cast<float>(NowSeconds - WarmupStartTimeSeconds) : 0.0f;

	OnProgress.Broadcast(Percent, DoneCount, TotalCount, bIsCompiling, RemainingCompileJobs, ElapsedSeconds, StatusText);
}

void USOTS_ShaderWarmupSubsystem::EnsureTouchActor()
{
	if (TouchActor.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags = RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!Actor)
	{
		return;
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("WarmupRoot"));
	Root->RegisterComponent();
	Actor->SetRootComponent(Root);

	TouchActor = Actor;
}

void USOTS_ShaderWarmupSubsystem::EnsureTouchMesh()
{
	if (TouchMesh.IsValid())
	{
		return;
	}

	EnsureTouchActor();
	AActor* Actor = TouchActor.Get();
	if (!Actor)
	{
		return;
	}

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, TEXT("WarmupMesh"));
	Mesh->SetupAttachment(Actor->GetRootComponent());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false, true);
	Mesh->SetHiddenInGame(true);
	Mesh->RegisterComponent();

	if (!Mesh->GetStaticMesh())
	{
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh)
		{
			Mesh->SetStaticMesh(CubeMesh);
		}
	}

	TouchMesh = Mesh;
}

void USOTS_ShaderWarmupSubsystem::EnsureTouchNiagara()
{
	if (TouchNiagara.IsValid())
	{
		return;
	}

	EnsureTouchActor();
	AActor* Actor = TouchActor.Get();
	if (!Actor)
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = NewObject<UNiagaraComponent>(Actor, TEXT("WarmupNiagara"));
	NiagaraComponent->SetupAttachment(Actor->GetRootComponent());
	NiagaraComponent->SetAutoActivate(false);
	NiagaraComponent->SetHiddenInGame(true);
	NiagaraComponent->RegisterComponent();

	TouchNiagara = NiagaraComponent;
}

void USOTS_ShaderWarmupSubsystem::HandlePreLoadMap(const FString& MapName)
{
	(void)MapName;
	if (!bWarmupActive || !ActiveRequest.bUseMoviePlayerDuringMapLoad)
	{
		return;
	}

	if (!bMoviePlayerConfigured)
	{
		SetupMoviePlayerScreen();
	}

	if (bMoviePlayerConfigured && IsMoviePlayerEnabled())
	{
		GetMoviePlayer()->PlayMovie();
		bMoviePlayerPlaying = true;
	}
}

void USOTS_ShaderWarmupSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	(void)LoadedWorld;
	if (!bWarmupActive || !ActiveRequest.bUseMoviePlayerDuringMapLoad)
	{
		return;
	}

	if (bWarmupReadyToTravel)
	{
		FinalizeWarmup();
	}
}

void USOTS_ShaderWarmupSubsystem::SetupMoviePlayerScreen()
{
	if (!IsMoviePlayerEnabled())
	{
		UE_LOG(LogSOTS_ShaderWarmup, Verbose, TEXT("MoviePlayer is disabled; skipping loading screen setup."));
		return;
	}

	UUserWidget* LoadingWidget = LoadingWidgetRef.Get();
	if (!LoadingWidget)
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("MoviePlayer setup skipped: loading widget was not created."));
		return;
	}

	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = false;
	Attributes.bWaitForManualStop = true;
	Attributes.MinimumLoadingScreenDisplayTime = 0.0f;
	Attributes.WidgetLoadingScreen = LoadingWidget->TakeWidget();

	GetMoviePlayer()->SetupLoadingScreen(Attributes);
	bMoviePlayerConfigured = true;
}

FGameplayTag USOTS_ShaderWarmupSubsystem::ResolveWidgetId(const F_SOTS_ShaderWarmupRequest& Request) const
{
	if (Request.ScreenWidgetId.IsValid())
	{
		return Request.ScreenWidgetId;
	}

	return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Screen.Loading.ShaderWarmup")), false);
}
