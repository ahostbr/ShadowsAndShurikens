#include "SOTS_BodyDragTargetComponent.h"
#include "Components/SkeletalMeshComponent.h"

USOTS_BodyDragTargetComponent::USOTS_BodyDragTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    BodyType = ESOTS_BodyDragBodyType::KnockedOut;
    BodyMesh = nullptr;
    bIsCurrentlyDragged = false;

    bCachedSimulatePhysics = false;
    CachedCollisionProfileName = NAME_None;
    bCachedCollisionEnabled = false;
}

bool USOTS_BodyDragTargetComponent::CanBeDragged() const
{
    return !bIsCurrentlyDragged && IsValid(ResolveBodyMesh());
}

USkeletalMeshComponent* USOTS_BodyDragTargetComponent::ResolveBodyMesh() const
{
    if (BodyMesh && IsValid(BodyMesh))
    {
        return BodyMesh;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    return OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
}

void USOTS_BodyDragTargetComponent::BeginDragged()
{
    USkeletalMeshComponent* Mesh = ResolveBodyMesh();
    if (!Mesh)
    {
        return;
    }

    bIsCurrentlyDragged = true;

    // Cache current state
    bCachedSimulatePhysics = Mesh->IsSimulatingPhysics();
    CachedCollisionProfileName = Mesh->GetCollisionProfileName();
    bCachedCollisionEnabled = Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision;

    // Disable collision and physics while dragging
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Mesh->GetCollisionProfileName() != NAME_None)
    {
        Mesh->SetCollisionProfileName(NAME_None);
    }

    if (bCachedSimulatePhysics)
    {
        Mesh->SetSimulatePhysics(false);
    }
}

void USOTS_BodyDragTargetComponent::EndDragged(bool bDropToPhysics)
{
    USkeletalMeshComponent* Mesh = ResolveBodyMesh();
    if (!Mesh)
    {
        bIsCurrentlyDragged = false;
        return;
    }

    // Restore collision profile and enablement
    if (CachedCollisionProfileName != NAME_None)
    {
        Mesh->SetCollisionProfileName(CachedCollisionProfileName);
    }
    Mesh->SetCollisionEnabled(bCachedCollisionEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    // Restore physics if requested and previously enabled
    if (bDropToPhysics && bCachedSimulatePhysics)
    {
        Mesh->SetSimulatePhysics(true);
    }

    bIsCurrentlyDragged = false;
}
