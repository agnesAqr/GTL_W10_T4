#include "SkeletalMeshActor.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    USkeletalMeshComponent* SkeletalMeshComp = AddComponent<USkeletalMeshComponent>(EComponentOrigin::Constructor);
    RootComponent = SkeletalMeshComp;
    USkeletalMesh* Mesh = FBXLoader::GetSkeletalMesh(TEXT("FBX/mixmix2_2.fbx"));
    SkeletalMeshComp->SetSkeletalMesh(Mesh);
    //SkeletalMeshComp->LoadSkeletalMesh(TEXT("FBX/Swat.fbx"));
    //SkeletalMeshComp->LoadSkeletalMesh(TEXT("FBX/man.fbx"));
}