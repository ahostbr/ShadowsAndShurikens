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
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "MoviePlayer.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "ShaderPipelineCache.h"
#include "SOTS_ShaderWarmupLoadListDataAsset.h"
#include "SOTS_UIRouterSubsystem.h"
#include "SOTS_UISettings.h"
#include "UObject/CoreUObjectDelegates.h"
#include "Widgets/Text/STextBlock.h"

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

	if (!Router->PushWidgetById(WidgetId, FInstancedStruct()))
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup failed: widget id %s was not found in the registry."), *WidgetId.ToString());
		return false;
	}

	ActiveRequest = Request;
	bWarmupActive = true;

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

	BuildAssetQueue(Request);
	TotalCount = AssetQueue.Num();
	DoneCount = 0;
	QueueIndex = 0;

	OnProgress.Broadcast(0.0f, FText::FromString(TEXT("Warming up shaders...")));

	if (TotalCount == 0)
	{
		EndWarmup();
		OnFinished.Broadcast();
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

	if (CompilePollHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CompilePollHandle);
		CompilePollHandle.Reset();
	}

	if (ActiveLoadHandle.IsValid())
	{
		ActiveLoadHandle->CancelHandle();
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

	if (ActiveRequest.bUseMoviePlayerDuringMapLoad && IsMoviePlayerEnabled())
	{
		GetMoviePlayer()->StopMovie();
	}

	AssetQueue.Reset();
	TotalCount = 0;
	DoneCount = 0;
	QueueIndex = 0;
	bWarmupActive = false;
}

void USOTS_ShaderWarmupSubsystem::BuildAssetQueue(const F_SOTS_ShaderWarmupRequest& Request)
{
	AssetQueue.Reset();
	TSet<FSoftObjectPath> SeenPaths;

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
		USOTS_ShaderWarmupLoadListDataAsset* LoadList = Request.LoadListOverride;
		if (!LoadList)
		{
			if (const USOTS_UISettings* Settings = USOTS_UISettings::Get())
			{
				if (!Settings->DefaultShaderWarmupLoadList.IsNull())
				{
					LoadList = Settings->DefaultShaderWarmupLoadList.LoadSynchronous();
				}
			}
		}

		if (LoadList)
		{
			for (const FSoftObjectPath& Path : LoadList->Assets)
			{
				AddUniquePath(Path);
			}

			bLoadedList = AssetQueue.Num() > 0;
		}
	}

	if (bLoadedList)
	{
		return;
	}

	if (Request.TargetLevelPackageName.IsNone())
	{
		UE_LOG(LogSOTS_ShaderWarmup, Warning, TEXT("Shader warmup fallback requested, but TargetLevelPackageName is empty."));
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& Registry = AssetRegistryModule.Get();

	TSet<FName> PackageNames;
	PackageNames.Add(Request.TargetLevelPackageName);

	TArray<FName> Dependencies;
	Registry.GetDependencies(Request.TargetLevelPackageName, Dependencies, EAssetRegistryDependencyType::Hard | EAssetRegistryDependencyType::Soft);
	for (const FName& Dependency : Dependencies)
	{
		PackageNames.Add(Dependency);
	}

	for (const FName& PackageName : PackageNames)
	{
		TArray<FAssetData> AssetDataArray;
		Registry.GetAssetsByPackageName(PackageName, AssetDataArray);

		for (const FAssetData& AssetData : AssetDataArray)
		{
			if (AssetData.IsInstanceOf(UMaterialInterface::StaticClass()) || AssetData.IsInstanceOf(UNiagaraSystem::StaticClass()))
			{
				AddUniquePath(AssetData.ToSoftObjectPath());
			}
		}
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
	OnProgress.Broadcast(Percent, FText::FromString(Status));

	LoadNext();
}

void USOTS_ShaderWarmupSubsystem::BeginWaitForCompiling()
{
	OnProgress.Broadcast(1.0f, FText::FromString(TEXT("Finalizing shaders...")));

	if (CompilePollHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CompilePollHandle);
		CompilePollHandle.Reset();
	}

	CompilePollHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &USOTS_ShaderWarmupSubsystem::HandleCompilePoll),
		0.1f);
}

bool USOTS_ShaderWarmupSubsystem::HandleCompilePoll(float DeltaTime)
{
	UE_UNUSED(DeltaTime);
	if (!bWarmupActive)
	{
		return false;
	}

	if (IsStillCompiling())
	{
		return true;
	}

	EndWarmup();
	OnFinished.Broadcast();
	return false;
}

bool USOTS_ShaderWarmupSubsystem::IsStillCompiling() const
{
#if WITH_EDITOR
	if (GShaderCompilingManager)
	{
		return (GShaderCompilingManager->GetNumPendingJobs() + GShaderCompilingManager->GetNumRemainingJobs()) > 0;
	}
	return false;
#else
	return FShaderPipelineCache::NumPrecompilesRemaining() > 0;
#endif
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
	UE_UNUSED(MapName);
	if (!bWarmupActive || !ActiveRequest.bUseMoviePlayerDuringMapLoad)
	{
		return;
	}

	SetupMoviePlayerScreen();
}

void USOTS_ShaderWarmupSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	UE_UNUSED(LoadedWorld);
	if (!bWarmupActive || !ActiveRequest.bUseMoviePlayerDuringMapLoad)
	{
		return;
	}
}

void USOTS_ShaderWarmupSubsystem::SetupMoviePlayerScreen()
{
	if (!IsMoviePlayerEnabled())
	{
		UE_LOG(LogSOTS_ShaderWarmup, Verbose, TEXT("MoviePlayer is disabled; skipping loading screen setup."));
		return;
	}

	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = false;
	Attributes.MinimumLoadingScreenDisplayTime = 0.0f;
	Attributes.WidgetLoadingScreen = SNew(STextBlock).Text(FText::FromString(TEXT("Loading...")));

	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

FGameplayTag USOTS_ShaderWarmupSubsystem::ResolveWidgetId(const F_SOTS_ShaderWarmupRequest& Request) const
{
	if (Request.ScreenWidgetId.IsValid())
	{
		return Request.ScreenWidgetId;
	}

	return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Screen.Loading.ShaderWarmup")), false);
}
