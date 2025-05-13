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

    void UpdateBoneTransformsFromAnim();

private:
    TArray<UStaticMeshComponent*> BoneComponents;
    void SkinningVertex();
    
    UAnimInstance* OwningAnimInstance;
    UAnimSequence* CurrentAnimSequence;

protected:
    USkeletalMesh* SkeletalMesh;
    int SelectedSubMeshIndex = -1;
};
