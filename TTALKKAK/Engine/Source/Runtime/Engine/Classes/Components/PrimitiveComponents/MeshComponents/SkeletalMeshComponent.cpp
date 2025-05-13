#include "SkeletalMeshComponent.h"

#include "FBXLoader.h"
#include "LaunchEngineLoop.h"
#include "Animation/AnimationSettings.h"
#include "Engine/World.h"
#include "Launch/EditorEngine.h"
#include "UObject/ObjectFactory.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Classes/Engine/FLoaderOBJ.h"
#include "GameFramework/Actor.h"
#include "StaticMeshComponents/StaticMeshComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendAnimInstance.h"

USkeletalMeshComponent::USkeletalMeshComponent()
    : SkeletalMesh(nullptr)
    , SelectedSubMeshIndex(-1)
    , OwningAnimInstance(nullptr)
{
    OwningAnimInstance = FObjectFactory::ConstructObject<UAnimInstance>(this);
}

USkeletalMeshComponent::USkeletalMeshComponent(const USkeletalMeshComponent& Other)
    : UMeshComponent(Other)
    , SkeletalMesh(Other.SkeletalMesh)
    , SelectedSubMeshIndex(Other.SelectedSubMeshIndex)
    , OwningAnimInstance(Other.OwningAnimInstance)
{
    //if (Other.OverrideMaterials.Num() > 0)
    //{
    //    OverrideMaterials = Other.OverrideMaterials;
    //}
}

uint32 USkeletalMeshComponent::GetNumMaterials() const
{
    if (SkeletalMesh == nullptr) return 0;

    return SkeletalMesh->GetMaterials().Num();
}

UMaterial* USkeletalMeshComponent::GetMaterial(uint32 ElementIndex) const
{
    if (SkeletalMesh != nullptr)
    {
        if (OverrideMaterials[ElementIndex] != nullptr)
        {
            return OverrideMaterials[ElementIndex];
        }
    
        if (SkeletalMesh->GetMaterials().IsValidIndex(ElementIndex))
        {
            return SkeletalMesh->GetMaterials()[ElementIndex]->Material;
        }
    }
    return nullptr;
}

uint32 USkeletalMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
    if (SkeletalMesh == nullptr) return -1;

    return SkeletalMesh->GetMaterialIndex(MaterialSlotName);
}

TArray<FName> USkeletalMeshComponent::GetMaterialSlotNames() const
{
    TArray<FName> MaterialNames;
    if (SkeletalMesh == nullptr) return MaterialNames;

    for (const FMaterialSlot* Material : SkeletalMesh->GetMaterials())
    {
        MaterialNames.Emplace(Material->MaterialSlotName);
    }

    return MaterialNames;
}

void USkeletalMeshComponent::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    if (SkeletalMesh == nullptr) return;
    SkeletalMesh->GetUsedMaterials(Out);
    for (int materialIndex = 0; materialIndex < GetNumMaterials(); materialIndex++)
    {
        if (OverrideMaterials[materialIndex] != nullptr)
        {
            Out[materialIndex] = OverrideMaterials[materialIndex];
        }
    }
}

int USkeletalMeshComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    //if (!AABB.IntersectRay(rayOrigin, rayDirection, pfNearHitDistance)) return 0;
    int nIntersections = 0;
    if (SkeletalMesh == nullptr) return 0;

    FSkeletalMeshRenderData renderData = SkeletalMesh->GetRenderData();

    FSkeletalVertex* vertices = renderData.Vertices.GetData();
    int vCount = renderData.Vertices.Num();
    UINT* indices = renderData.Indices.GetData();
    int iCount = renderData.Indices.Num();

    if (!vertices) return 0;
    BYTE* pbPositions = reinterpret_cast<BYTE*>(renderData.Vertices.GetData());

    int nPrimitives = (!indices) ? (vCount / 3) : (iCount / 3);
    float fNearHitDistance = FLT_MAX;
    for (int i = 0; i < nPrimitives; i++) {
        int idx0, idx1, idx2;
        if (!indices) {
            idx0 = i * 3;
            idx1 = i * 3 + 1;
            idx2 = i * 3 + 2;
        }
        else {
            idx0 = indices[i * 3];
            idx2 = indices[i * 3 + 1];
            idx1 = indices[i * 3 + 2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        uint32 stride = sizeof(FSkeletalVertex);
        FVector v0 = *reinterpret_cast<FVector*>(pbPositions + idx0 * stride);
        FVector v1 = *reinterpret_cast<FVector*>(pbPositions + idx1 * stride);
        FVector v2 = *reinterpret_cast<FVector*>(pbPositions + idx2 * stride);

        float fHitDistance;
        if (IntersectRayTriangle(rayOrigin, rayDirection, v0, v1, v2, fHitDistance)) {
            if (fHitDistance < fNearHitDistance) {
                pfNearHitDistance = fNearHitDistance = fHitDistance;
            }
            nIntersections++;
        }

    }
    return nIntersections;
}


void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* value)
{
    if (SkeletalMesh == value) return;

    SkeletalMesh = value;
    if (SkeletalMesh)
    {
        VBIBTopologyMappingName = SkeletalMesh->GetFName();
        SkeletalMesh->UpdateBoneHierarchy();

        // 머티리얼 오버라이드 배열 크기 조정
        OverrideMaterials.Init(nullptr, SkeletalMesh->GetMaterials().Num());
        AABB = SkeletalMesh->GetRenderData().BoundingBox;

        if (OwningAnimInstance == nullptr)
        {
            OwningAnimInstance = FObjectFactory::ConstructObject<UAnimInstance>(this);
        }

        if (OwningAnimInstance)
        {
            OwningAnimInstance->TargetSkeleton = SkeletalMesh->GetRefSkeletal();
            OwningAnimInstance->NativeInitializeAnimation();

            // Begin Test
            UpdateBoneTransformsFromAnim();

            SkinningVertex();
        }
    }
    else // SkeletalMesh가 nullptr로 설정된 경우
    {
        VBIBTopologyMappingName = TEXT("");
        OverrideMaterials.Empty();

        // Begin Test
        SetAnimInstance(nullptr);
        //if (OwningAnimInstance)
        //{
        //    OwningAnimInstance = nullptr;
        //}
    }
}

void USkeletalMeshComponent::CreateBoneComponents()
{
    FManagerOBJ::CreateStaticMesh("Contents/helloBlender.obj");
    for (const auto& Bone : GetSkeletalMesh()->GetRenderData().Bones)
    {
        UStaticMeshComponent* BoneComp = GetOwner()->AddComponent<UStaticMeshComponent>(EComponentOrigin::Runtime);
        BoneComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"helloBlender.obj"));
        BoneComp->SetWorldLocation(Bone.GlobalTransform.GetTranslationVector());
        BoneComp->SetFName(Bone.BoneName);
        BoneComponents.Add(BoneComp);
    }
}

USkeletalMesh* USkeletalMeshComponent::LoadSkeletalMesh(const FString& FilePath)
{
    USkeletalMesh* NewSkeletalMesh = FBXLoader::GetSkeletalMesh(FilePath);
    if (NewSkeletalMesh)
    {
        SetSkeletalMesh(NewSkeletalMesh);
    }

    return NewSkeletalMesh;
}

void USkeletalMeshComponent::UpdateBoneHierarchy()
{
    for (int i=0;i<SkeletalMesh->GetRenderData().Vertices.Num();i++)
    {
         SkeletalMesh->GetRenderData().Vertices[i].Position
        = SkeletalMesh->GetRefSkeletal()->RawVertices[i].Position;
    }
    
    SkeletalMesh->UpdateBoneHierarchy();
    SkinningVertex();
}

void USkeletalMeshComponent::SetAnimInstance(UAnimInstance* InAnimInstance)
{
    if (OwningAnimInstance == InAnimInstance)
    {
        return;
    }

    if (OwningAnimInstance)
    {
        UE_LOG(LogLevel::Warning, "Delete: %s", *OwningAnimInstance->GetName());
        GUObjectArray.MarkRemoveObject(OwningAnimInstance);
    }

    OwningAnimInstance = InAnimInstance;
    if(OwningAnimInstance)
    {
        UE_LOG(LogLevel::Warning, "Create: %s", *OwningAnimInstance->GetName());
    }
}

void USkeletalMeshComponent::SkinningVertex()
{
    for (auto& Vertex : SkeletalMesh->GetRenderData().Vertices)
    {
        Vertex.SkinningVertex(SkeletalMesh->GetRenderData().Bones);
    }

    FBXLoader::UpdateBoundingBox(SkeletalMesh->GetRenderData());
    AABB = SkeletalMesh->GetRenderData().BoundingBox;

    SkeletalMesh->SetData(SkeletalMesh->GetRenderData(), SkeletalMesh->GetRefSkeletal()); // TODO: Dynamic VertexBuffer Update하게 바꾸기
}

// std::unique_ptr<FActorComponentInfo> USkeletalMeshComponent::GetComponentInfo()
// {
//     auto Info = std::make_unique<FStaticMeshComponentInfo>();
//     SaveComponentInfo(*Info);
//     
//     return Info;
// }

// void UStaticMeshComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
// {
//     FStaticMeshComponentInfo* Info = static_cast<FStaticMeshComponentInfo*>(&OutInfo);
//     Super::SaveComponentInfo(*Info);
//
//     Info->StaticMeshPath = staticMesh->GetRenderData()->PathName;
// }

// void UStaticMeshComponent::LoadAndConstruct(const FActorComponentInfo& Info)
// {
//     Super::LoadAndConstruct(Info);
//
//     const FStaticMeshComponentInfo& StaticMeshInfo = static_cast<const FStaticMeshComponentInfo&>(Info);
//     UStaticMesh* Mesh = FManagerOBJ::CreateStaticMesh(FString::ToFString(StaticMeshInfo.StaticMeshPath));
//     SetStaticMesh(Mesh);
// }

// FIX-ME
void USkeletalMeshComponent::BeginPlay()
{
    Super::BeginPlay();

    if (OwningAnimInstance)
    {
        if (SkeletalMesh && SkeletalMesh->GetRefSkeletal() && OwningAnimInstance->TargetSkeleton == nullptr)
        {
            OwningAnimInstance->TargetSkeleton = SkeletalMesh->GetRefSkeletal();
        }
        OwningAnimInstance->NativeInitializeAnimation();
    }
    else if (SkeletalMesh)
    {
        UAnimInstance* DefaultInstance = FObjectFactory::ConstructObject<UAnimInstance>(this);
        SetAnimInstance(DefaultInstance);

        if (OwningAnimInstance)
        {
            OwningAnimInstance->TargetSkeleton = SkeletalMesh->GetRefSkeletal();
            OwningAnimInstance->NativeInitializeAnimation();
        }
    }
    
    if (SkeletalMesh)
    {
        UpdateBoneTransformsFromAnim(); 
        SkeletalMesh->UpdateSkinnedVertices();
    }

    //CreateBoneComponents();
}

UObject* USkeletalMeshComponent::Duplicate(UObject* InOuter)
{
    USkeletalMeshComponent* NewComp = FObjectFactory::ConstructObjectFrom<USkeletalMeshComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();
    return NewComp;
}

void USkeletalMeshComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UMeshComponent::DuplicateSubObjects(Source, InOuter);
    const USkeletalMeshComponent* SourceComp = Cast<USkeletalMeshComponent>(Source);
    if (SourceComp)
    {
        this->SkeletalMesh = SourceComp->SkeletalMesh;
        this->OverrideMaterials = SourceComp->OverrideMaterials;
        this->SelectedSubMeshIndex = SourceComp->SelectedSubMeshIndex;
        this->OwningAnimInstance = Cast<UAnimInstance>(SourceComp->OwningAnimInstance->Duplicate(this));
    }
}

void USkeletalMeshComponent::PostDuplicate()
{
    Super::PostDuplicate();
    // if (SkeletalMesh)
    // {
    //     USkeletalMesh* TempMesh = SkeletalMesh;
    //     SkeletalMesh = nullptr;
    //     SetSkeletalMesh(TempMesh);
    // }
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    if (OwningAnimInstance && SkeletalMesh)
    {
        OwningAnimInstance->NativeUpdateAnimation(DeltaTime);
        UpdateBoneTransformsFromAnim();
        SkeletalMesh->UpdateSkinnedVertices();
    }

    if (BoneComponents.Num() > 0 && SkeletalMesh)
    {
        const auto& RenderBones = SkeletalMesh->GetRenderData().Bones;
        for (int i = 0; i < BoneComponents.Num() && i < RenderBones.Num(); ++i)
        {
            if (BoneComponents[i] && RenderBones.IsValidIndex(i))
            {
                FTransform BoneComponentLocalSpaceTransform(RenderBones[i].GlobalTransform);
                FTransform BoneFinalWorldTransform = BoneComponentLocalSpaceTransform * this->GetWorldTransform();

                BoneComponents[i]->SetWorldTransform(BoneFinalWorldTransform);
            }
        }
    }
}

void USkeletalMeshComponent::UpdateBoneTransformsFromAnim()
{
    if (!OwningAnimInstance || !SkeletalMesh)
    {
        // 애니메이션 인스턴스나 스켈레탈 메시가 없으면 아무것도 하지 않거나 참조 포즈로.
        // 참조 포즈로 되돌리려면:
        if (SkeletalMesh) SkeletalMesh->UpdateBoneHierarchy(); // 참조 포즈의 본 트랜스폼으로 설정
        return;
    }

    const FRefSkeletal* RefSkeleton = OwningAnimInstance->TargetSkeleton; // 또는 GetTargetSkeleton() 같은 함수
    if (!RefSkeleton)
    {
        // 참조 스켈레톤 정보가 없으면 진행 불가
        if (SkeletalMesh) SkeletalMesh->UpdateBoneHierarchy();
        return;
    }

    const FPoseData& CurrentPose = OwningAnimInstance->GetCurrentPose();
    FSkeletalMeshRenderData& RenderData = SkeletalMesh->GetRenderData(); // Non-const 접근

    if (CurrentPose.LocalBoneTransforms.Num() != RefSkeleton->BoneTree.Num() ||
        RenderData.Bones.Num() != RefSkeleton->BoneTree.Num())
    {
        UE_LOG(LogLevel::Warning, TEXT("Mismatch in bone counts between AnimPose, RefSkeleton, and RenderData."));
        SkeletalMesh->UpdateBoneHierarchy();
        return;
    }

    for (int32 BoneTreeIndex = 0; BoneTreeIndex < RefSkeleton->BoneTree.Num(); ++BoneTreeIndex)
    {
        if (!RenderData.Bones.IsValidIndex(BoneTreeIndex) || !CurrentPose.LocalBoneTransforms.IsValidIndex(BoneTreeIndex))
        {
            UE_LOG(LogLevel::Warning, TEXT("Invalid bone index during local transform application."));
            continue;
        }

        RenderData.Bones[BoneTreeIndex].LocalTransform = CurrentPose.LocalBoneTransforms[BoneTreeIndex].ToTransform().ToMatrix();
    }

    for (int32 BoneTreeIndex = 0; BoneTreeIndex < RefSkeleton->BoneTree.Num(); ++BoneTreeIndex)
    {
        if (!RenderData.Bones.IsValidIndex(BoneTreeIndex) || !RefSkeleton->BoneTree.IsValidIndex(BoneTreeIndex))
        {
            UE_LOG(LogLevel::Warning, TEXT("Invalid bone index during global transform calculation."));
            continue;
        }

        FMatrix ParentGlobalTransformMatrix = FMatrix::Identity;

        int32 ParentBoneDataIndex = RenderData.Bones[BoneTreeIndex].ParentIndex;

        if (ParentBoneDataIndex != INDEX_NONE && RenderData.Bones.IsValidIndex(ParentBoneDataIndex)) 
        {
            ParentGlobalTransformMatrix = RenderData.Bones[ParentBoneDataIndex].GlobalTransform;
        }
        // else: 루트 본이거나 ParentIndex가 유효하지 않으면, ParentGlobalTransformMatrix는 Identity로 유지됩니다.

        RenderData.Bones[BoneTreeIndex].GlobalTransform = RenderData.Bones[BoneTreeIndex].LocalTransform * ParentGlobalTransformMatrix;

        RenderData.Bones[BoneTreeIndex].SkinningMatrix = RenderData.Bones[BoneTreeIndex].InverseBindPoseMatrix * RenderData.Bones[BoneTreeIndex].GlobalTransform;
    }
}

void USkeletalMeshComponent::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    //Owner->HandleAnimNotify(Notify);
}

void USkeletalMeshComponent::SetPosition(float InTime, bool bFireNotifies)
{
    if (OwningAnimInstance)
    {
        // UAnimSingleNodeInstance에 시간 세팅
        UAnimSingleNodeInstance* SingleNodeInst = Cast<UAnimSingleNodeInstance>(OwningAnimInstance);
        if (SingleNodeInst)
        {
            // AnimSingleNodeInstance 의 내부 시간·포즈 업데이트
            SingleNodeInst->SetPosition(InTime, bFireNotifies);
        }
    }
    else
    {
        // 애니메이션이 없는 경우 참조 포즈로 초기화
        if (SkeletalMesh)
            SkeletalMesh->UpdateBoneHierarchy();
    }

    // 애니메이션에 따라 본 트랜스폼을 재계산하여 RenderData 에 반영
    UpdateBoneTransformsFromAnim();
    SkeletalMesh->UpdateSkinnedVertices();
}