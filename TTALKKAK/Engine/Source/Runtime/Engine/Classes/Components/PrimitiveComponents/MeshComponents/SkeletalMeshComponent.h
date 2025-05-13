#pragma once
#include "MeshComponent.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"

class UStaticMeshComponent;
class UAnimInstance;

class USkeletalMeshComponent : public UMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, UMeshComponent)

public:
    USkeletalMeshComponent();
    USkeletalMeshComponent(const USkeletalMeshComponent& Other);

    virtual void BeginPlay() override;

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    virtual void PostDuplicate() override;
    virtual void TickComponent(float DeltaTime) override;

    PROPERTY(int, SelectedSubMeshIndex);

    virtual uint32 GetNumMaterials() const override;
    virtual UMaterial* GetMaterial(uint32 ElementIndex) const override;
    virtual uint32 GetMaterialIndex(FName MaterialSlotName) const override;
    virtual TArray<FName> GetMaterialSlotNames() const override;
    virtual void GetUsedMaterials(TArray<UMaterial*>& Out) const override;

    
    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance) override;
    
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
    void SetSkeletalMesh(USkeletalMesh* value);
    USkeletalMesh* LoadSkeletalMesh(const FString& FilePath);
    void CreateBoneComponents();
    void UpdateBoneHierarchy();

    // 이걸 여기 두는 것 자체가 별로긴 함. 근데 학습 자료에 그렇게 나와 있음.
    void PlayAnimation(UAnimSequence* NewAnimToPlay, bool bLooping = true);
    UAnimInstance* GetAnimInstance() const { return OwningAnimInstance; }
    UAnimSequence* GetAnimSequence() const { return CurrentAnimSequence; }

    void UpdateBoneTransformsFromAnim();
    void HandleAnimNotify(const FAnimNotifyEvent& Notify);
    bool HasAnimation() const { return OwningAnimInstance != nullptr && CurrentAnimSequence != nullptr; }
    
    /**
    * @brief 애니메이션 재생 위치를 설정하고 본 트랜스폼을 업데이트합니다.
    * @param InTime       재생할 시간(초 단위)
    * @param bFireNotifies true 이면 해당 시간에 걸친 Notify 이벤트를 HandleAnimNotify()로 전달
    */
    void SetPosition(float InTime, bool bFireNotifies = false);

private:
    TArray<UStaticMeshComponent*> BoneComponents;
    void SkinningVertex();
    
    UAnimInstance* OwningAnimInstance;
    UAnimSequence* CurrentAnimSequence;  // 안씀

protected:
    USkeletalMesh* SkeletalMesh;
    int SelectedSubMeshIndex = -1;
};
