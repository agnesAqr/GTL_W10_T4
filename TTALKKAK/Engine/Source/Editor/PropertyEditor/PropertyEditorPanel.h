#pragma once
#include "Define.h"
#include "UnrealEd/EditorPanel.h"

class AActor;
class USkeletalMeshComponent;
class UActorComponent;
class UStaticMeshComponent;
class USceneComponent;
class ULevel;

struct FBoneRotation
{
    float X;
    float Y;
    float Z;
    
    FBoneRotation() : X(0.0f), Y(0.0f), Z(0.0f) {}
    FBoneRotation(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
};

class PropertyEditorPanel : public UEditorPanel
{
public:
    void Initialize(float InWidth, float InHeight);
    virtual void Render() override;
    void DrawSceneComponentTree(USceneComponent* Component, UActorComponent*& PickedComponent);
    void DrawActorComponent(UActorComponent* Component, UActorComponent*& PickedComponent);
    virtual void OnResize(HWND hWnd) override;


private:
    void RGBToHSV(float r, float g, float b, float& h, float& s, float& v) const;
    void HSVToRGB(float h, float s, float v, float& r, float& g, float& b) const;

    /* Static Mesh Settings */
    void RenderForStaticMesh(UStaticMeshComponent* StaticMeshComp);
    void RenderForSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp);
    void RenderForSkeletalMesh2(USkeletalMeshComponent* SkeletalMesh);
    void RenderBoneHierarchy(USkeletalMesh* SkeletalMesh, int BoneIndex);
    void RenderForSkeletalMesh3(USkeletalMeshComponent* SkeletalMesh);
    void OnBoneSelected(int BoneIndex);

    /* Materials Settings */
    void RenderForMaterial(UStaticMeshComponent* StaticMeshComp);
    void RenderForMaterial(USkeletalMeshComponent* SkeletalMeshComp);
    void RenderMaterialView(UMaterial* Material, bool IsStaticMesh);
    void RenderCreateMaterialView();

    void RenderForLua(class ULuaComponent* LuaComponent);
    void RenderShapeProperty(AActor* PickedActor);
    void RenderDelegate(ULevel* level);
    
    void DrawSkeletalMeshPreviewButton(const FString& FilePath);
    void DrawAnimationPreviewButton(const FString& FilePath);
    
private:
    float Width = 0, Height = 0;
    
    /* Material Property */
    int SelectedMaterialIndex = -1;
    int CurMaterialIndex = -1;
    UStaticMeshComponent* SelectedStaticMeshComp = nullptr;
    USkeletalMeshComponent* SelectedSkeletalMeshComp = nullptr;
    int SelectedBoneIndex = -1;

    FObjMaterialInfo tempMaterialInfo;
    bool IsCreateMaterial;
    UActorComponent* PickedComponent = nullptr;
    UActorComponent* LastComponent = nullptr;
    bool bFirstFrame = true;


    //FBX
private:
    float XRotation = 0.0f;
    float YRotation = 0.0f;
    float ZRotation = 0.0f;
    TMap<int, FBoneRotation> BoneRotations;

};
