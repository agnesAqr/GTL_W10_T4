#pragma once
#include "MeshComponent.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h"

class UStaticMeshComponent;

class UAnimInstance;
class UAnimSingleNodeInstance;
class UBlendAnimInstance;

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
    void CreateBoneComponents();
    void UpdateBoneHierarchy();

    void SetAnimInstance(UAnimInstance* InAnimInstance);
    UAnimInstance* GetAnimInstance() const { return OwningAnimInstance; }

    void UpdateBoneTransformsFromAnim();
    void HandleAnimNotify(const FAnimNotifyEvent& Notify);
    
    /**
    * @brief 애니메이션 재생 위치를 설정하고 본 트랜스폼을 업데이트합니다.
    * @param InTime       재생할 시간(초 단위)
    * @param bFireNotifies true 이면 해당 시간에 걸친 Notify 이벤트를 HandleAnimNotify()로 전달
    */
    void SetPosition(float InTime, bool bFireNotifies = false);

    bool HasAnimation() const;

private:
    TArray<UStaticMeshComponent*> BoneComponents;
    void SkinningVertex();
    
    UAnimInstance* OwningAnimInstance;

protected:
    USkeletalMesh* SkeletalMesh;
    int SelectedSubMeshIndex = -1; 
};
