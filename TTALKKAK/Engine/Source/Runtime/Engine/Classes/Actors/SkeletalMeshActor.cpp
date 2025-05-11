#include "SkeletalMeshActor.h"
#include "FBXLoader.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    USkeletalMeshComponent* SkeletalMeshComp = AddComponent<USkeletalMeshComponent>(EComponentOrigin::Constructor);
    RootComponent = SkeletalMeshComp;
    SkeletalMeshComp->LoadSkeletalMesh(TEXT("FBX/mixmix2.fbx"));
}