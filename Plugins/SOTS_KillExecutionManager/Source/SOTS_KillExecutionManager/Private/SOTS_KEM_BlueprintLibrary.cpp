#include "SOTS_KEM_BlueprintLibrary.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "ContextualAnimTypes.h"
#include "SOTS_KillExecutionManagerModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "SOTS_TagAccessHelpers.h"



namespace
{
    static USOTS_KEMManagerSubsystem* GetKEMSubsystem(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        UGameInstance* GameInstance = World->GetGameInstance();
        if (!GameInstance)
        {
            return nullptr;
        }

        return GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
    }
}

bool USOTS_KEM_BlueprintLibrary::KEM_RequestExecution_Simple(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags)
{
    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return false;
    }

    return Subsystem->RequestExecution(WorldContextObject, Instigator, Target, ContextTags);
}

void USOTS_KEM_BlueprintLibrary::KEM_RunDebug_Simple(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags)
{
    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return;
    }

    Subsystem->RunKEMDebug(WorldContextObject, Instigator, Target, ContextTags);
}

void USOTS_KEM_BlueprintLibrary::KEM_NotifyExecutionEnded(
    const UObject* WorldContextObject,
    bool bSuccess)
{
    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return;
    }

    Subsystem->NotifyExecutionEnded(bSuccess);
}

bool USOTS_KEM_BlueprintLibrary::SOTS_IsPlayerSafeForExecution(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return true;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return true;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject))
    {
        const FGameplayTag HasLOSTag =
            TagSubsystem->GetTagByName(TEXT("SAS.AI.Perception.HasLOS.Player"));

        if (HasLOSTag.IsValid())
        {
            // Conservative default: if any actor in the world reports this tag, we treat
            // the player as unsafe for execution. Callers can supply a more specific
            // guard list later if needed.
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                AActor* Actor = *It;
                if (!Actor)
                {
                    continue;
                }

                if (TagSubsystem->ActorHasTag(Actor, HasLOSTag))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool USOTS_KEM_BlueprintLibrary::KEM_ResolveWarpPointByName(
    const USOTS_KEM_ExecutionDefinition* ExecutionDefinition,
    FName WarpPointName,
    const FSOTS_ExecutionContext& Context,
    FTransform& OutTransform)
{
    if (!ExecutionDefinition)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogSOTSKEM, Warning, TEXT("KEM_ResolveWarpPointByName: ExecutionDefinition is null."));
#endif
        return false;
    }

    USOTS_KEMManagerSubsystem* Subsystem = nullptr;

    if (Context.Instigator.IsValid())
    {
        if (UWorld* World = Context.Instigator->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                Subsystem = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
            }
        }
    }

    if (!Subsystem && Context.Target.IsValid())
    {
        if (UWorld* World = Context.Target->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                Subsystem = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
            }
        }
    }

    if (!Subsystem)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogSOTSKEM, Warning, TEXT("KEM_ResolveWarpPointByName: No KEM subsystem found for Instigator=%s Target=%s"),
            Context.Instigator.IsValid() ? *Context.Instigator->GetName() : TEXT("None"),
            Context.Target.IsValid() ? *Context.Target->GetName() : TEXT("None"));
#endif
        return false;
    }

    return Subsystem->ResolveWarpPointByName(ExecutionDefinition, WarpPointName, Context, OutTransform);
}

void USOTS_KEM_BlueprintLibrary::KEM_BuildCASBindingContexts(
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags,
    FContextualAnimSceneBindingContext& OutInstigatorBinding,
    FContextualAnimSceneBindingContext& OutTargetBinding)
{
    OutInstigatorBinding = Instigator ? FContextualAnimSceneBindingContext(Instigator, ContextTags) : FContextualAnimSceneBindingContext();
    OutTargetBinding = Target ? FContextualAnimSceneBindingContext(Target, ContextTags) : FContextualAnimSceneBindingContext();
}

bool USOTS_KEM_BlueprintLibrary::KEM_BuildCASBindingContextsForDefinition(
    const UObject* WorldContextObject,
    const USOTS_KEM_ExecutionDefinition* ExecutionDefinition,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags,
    FContextualAnimSceneBindingContext& OutInstigatorBinding,
    FContextualAnimSceneBindingContext& OutTargetBinding)
{
    OutInstigatorBinding = FContextualAnimSceneBindingContext();
    OutTargetBinding = FContextualAnimSceneBindingContext();

    if (!ExecutionDefinition || !Instigator || !Target)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogSOTSKEM, Warning,
            TEXT("KEM_BuildCASBindingContextsForDefinition: Missing inputs (Exec=%s Instigator=%s Target=%s)."),
            ExecutionDefinition ? *ExecutionDefinition->GetName() : TEXT("None"),
            *GetNameSafe(Instigator),
            *GetNameSafe(Target));
#endif
        return false;
    }

    const FSOTS_KEM_CASConfig& CASCfg = ExecutionDefinition->CASConfig;
    if (!CASCfg.Scene.IsValid() && !CASCfg.Scene.ToSoftObjectPath().IsValid())
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogSOTSKEM, Warning,
            TEXT("KEM_BuildCASBindingContextsForDefinition: CAS Scene is not set for Exec=%s"),
            *ExecutionDefinition->GetName());
#endif
        return false;
    }

    FVector InstigatorLocation = Instigator->GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation();
    FVector InstigatorForward = Instigator->GetActorForwardVector();
    FVector TargetForward = Target->GetActorForwardVector();

    const FVector TargetLoc = TargetLocation;
    const FRotator TargetRot = TargetForward.Rotation();
    const FTransform TargetXf(TargetRot, TargetLoc);

    const FVector LocalOffset = CASCfg.OffsetConfig.InstigatorLocalOffsetFromTarget;
    const FRotator LocalRotOffset = CASCfg.OffsetConfig.InstigatorLocalRotationOffset;
    const FTransform LocalXf(LocalRotOffset, LocalOffset);
    const FTransform WarpTarget = LocalXf * TargetXf;

    OutInstigatorBinding = FContextualAnimSceneBindingContext(Instigator, TOptional<FTransform>(WarpTarget));
    OutTargetBinding = FContextualAnimSceneBindingContext(Target, TOptional<FTransform>(Target->GetActorTransform()));

    for (const FGameplayTag& Tag : ContextTags)
    {
        if (Tag.IsValid())
        {
            OutInstigatorBinding.AddGameplayTag(Tag);
            OutTargetBinding.AddGameplayTag(Tag);
        }
    }

    return true;
}

#if WITH_EDITOR
void USOTS_KEM_BlueprintLibrary::KEM_ValidateRegistryCoverage(
    const UObject* WorldContextObject,
    TArray<FString>& OutWarnings)
{
    OutWarnings.Reset();

    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        OutWarnings.Add(TEXT("KEM subsystem not found; cannot validate coverage."));
        UE_LOG(LogSOTSKEM, Warning, TEXT("[KEM Coverage] Subsystem missing; validation skipped."));
        return;
    }

    TArray<USOTS_KEM_ExecutionDefinition*> Definitions;
    TSet<USOTS_KEM_ExecutionDefinition*> Seen;

    auto AddSoftDefinition = [&](const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef)
    {
        if (!SoftDef.IsValid() && !SoftDef.ToSoftObjectPath().IsValid())
        {
            return;
        }
        USOTS_KEM_ExecutionDefinition* Def = SoftDef.Get();
        if (!Def)
        {
            Def = SoftDef.LoadSynchronous();
        }
        if (Def && !Seen.Contains(Def))
        {
            Seen.Add(Def);
            Definitions.Add(Def);
        }
    };

    for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : Subsystem->ExecutionDefinitions)
    {
        AddSoftDefinition(SoftDef);
    }

    if (USOTS_KEM_ExecutionCatalog* Catalog = USOTS_KEMCatalogLibrary::GetExecutionCatalog(Subsystem))
    {
        for (const TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>& SoftDef : Catalog->ExecutionDefinitions)
        {
            AddSoftDefinition(SoftDef);
        }
    }

    auto LoadDefaultRegistryConfig = [&]() -> USOTS_KEM_ExecutionRegistryConfig*
    {
        if (Subsystem->DefaultRegistryConfig.IsValid())
        {
            return Subsystem->DefaultRegistryConfig.Get();
        }
        if (Subsystem->DefaultRegistryConfig.ToSoftObjectPath().IsValid())
        {
            return Subsystem->DefaultRegistryConfig.LoadSynchronous();
        }
        return nullptr;
    };

    if (USOTS_KEM_ExecutionRegistryConfig* RegistryConfig = LoadDefaultRegistryConfig())
    {
        for (const FSOTS_KEM_ExecutionRegistryEntry& Entry : RegistryConfig->Entries)
        {
            AddSoftDefinition(Entry.ExecutionDefinition);
        }
    }

    TSet<ESOTS_KEM_ExecutionFamily> Families;
    TSet<FName> PositionTagNames;
    bool bHasFallback = false;

    for (USOTS_KEM_ExecutionDefinition* Def : Definitions)
    {
        if (!Def)
        {
            continue;
        }

        if (Def->ExecutionFamily != ESOTS_KEM_ExecutionFamily::Unknown)
        {
            Families.Add(Def->ExecutionFamily);
        }
        else
        {
            bHasFallback = true;
        }

        if (Def->PositionTag.IsValid())
        {
            PositionTagNames.Add(Def->PositionTag.GetTagName());
        }
        for (const FGameplayTag& Additional : Def->AdditionalPositionTags)
        {
            if (Additional.IsValid())
            {
                PositionTagNames.Add(Additional.GetTagName());
            }
        }
    }

    const TArray<ESOTS_KEM_ExecutionFamily> ExpectedFamilies = {
        ESOTS_KEM_ExecutionFamily::GroundFront,
        ESOTS_KEM_ExecutionFamily::GroundRear,
        ESOTS_KEM_ExecutionFamily::GroundLeft,
        ESOTS_KEM_ExecutionFamily::GroundRight,
        ESOTS_KEM_ExecutionFamily::CornerLeft,
        ESOTS_KEM_ExecutionFamily::CornerRight,
        ESOTS_KEM_ExecutionFamily::VerticalAbove,
        ESOTS_KEM_ExecutionFamily::VerticalBelow
    };

    for (ESOTS_KEM_ExecutionFamily Family : ExpectedFamilies)
    {
        if (!Families.Contains(Family))
        {
            const FString FamilyName = GetExecutionFamilyName(Family);
            const FString Warning = FString::Printf(TEXT("Missing execution family coverage: %s"), *FamilyName);
            OutWarnings.Add(Warning);
            UE_LOG(LogSOTSKEM, Warning, TEXT("[KEM Coverage] %s"), *Warning);
        }
    }

    const TArray<FName> ExpectedPositionTags = {
        FName(TEXT("SOTS.KEM.Position.Ground.Front")),
        FName(TEXT("SOTS.KEM.Position.Ground.Rear")),
        FName(TEXT("SOTS.KEM.Position.Ground.Left")),
        FName(TEXT("SOTS.KEM.Position.Ground.Right")),
        FName(TEXT("SOTS.KEM.Position.Corner.FrontLeft")),
        FName(TEXT("SOTS.KEM.Position.Corner.FrontRight")),
        FName(TEXT("SOTS.KEM.Position.Corner.RearLeft")),
        FName(TEXT("SOTS.KEM.Position.Corner.RearRight")),
        FName(TEXT("SOTS.KEM.Position.Vertical.Above")),
        FName(TEXT("SOTS.KEM.Position.Vertical.Below"))
    };

    for (const FName& PositionName : ExpectedPositionTags)
    {
        if (!PositionTagNames.Contains(PositionName))
        {
            const FString Warning = FString::Printf(TEXT("Missing position tag coverage: %s"), *PositionName.ToString());
            OutWarnings.Add(Warning);
            UE_LOG(LogSOTSKEM, Warning, TEXT("[KEM Coverage] %s"), *Warning);
        }
    }

    if (!bHasFallback && Families.Num() > 0)
    {
        const FString Warning = TEXT("No fallback (ExecutionFamily Unknown) execution defined.");
        OutWarnings.Add(Warning);
        UE_LOG(LogSOTSKEM, Warning, TEXT("[KEM Coverage] %s"), *Warning);
    }
}
#endif // WITH_EDITOR
