#pragma once
#include <fbxsdk.h>

#include "Components/Mesh/SkeletalMesh.h"
#include "Components/Mesh/StaticMesh.h"
#include "Container/Map.h"
#include "FBX/FBXDefine.h"

class UAnimSequence;
class UAnimDataModel;

class FBXLoader
{
public:
    static bool InitFBXManager();
    static FSkeletalMeshRenderData* ParseFBX(const FString& FilePath, bool bIsAbsolutePath = false);

    static void ExtractFBXMeshData(const FbxScene* Scene, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, TMap<FString, FbxNode*>& OutNodeMap);
    static void ExtractSkeleton(FbxScene* Scene, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal);
    static void ExtractMeshFromNode(FbxNode* Node, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, TMap<FString, FbxNode*>& OutNodeMap);
    static void ExtractVertices(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal);
    static void ExtractNormals(FbxMesh* Mesh, FSkeletalMeshRenderData* RenderData, int BaseVertexIndex);
    static void ExtractUVs(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int BaseVertexIndex);
    static void ExtractTangents(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int BaseVertexIndex);
    static void ExtractSkinningData(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, int BaseVertexIndex);
    static void ProcessSkinning(FbxSkin* skin, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, int base_vertex_index);
    static void ExtractIndices(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int BaseVertexIndex);
    static void ExtractMaterials(FbxNode* Node, FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, int BaseIndexOffset);
    static void ExtractAnimation(int BoneTreeIndex, const FRefSkeletal& RefSkeletal, FbxAnimLayer* AnimLayer, UAnimDataModel* AnimModel, const TMap<FString, FbxNode*>& NodeMap);
    
    static void UpdateBoundingBox(FSkeletalMeshRenderData* MeshData);

    static FSkeletalMeshRenderData* GetSkeletalRenderData(FString FilePath);
    static FSkeletalMeshRenderData GetCopiedSkeletalRenderData(FString FilePath);
    
    static FRefSkeletal* GetRefSkeletal(FString FilePath);
    static TMap<FName, FRefSkeletal*> GetAllRefSkeletals() { return RefSkeletalData; }
    
    static void UpdateBoundingBox(FSkeletalMeshRenderData MeshData);
    
    static FObjMaterialInfo ConvertFbxToObjMaterialInfo(FbxSurfaceMaterial* FbxMat, const FString& BasePath = TEXT(""));

    static USkeletalMesh* CreateSkeletalMesh(const FString& FilePath);
    static TMap<FName, FSkeletalMeshRenderData*> GetAllSkeletalMeshes() { return SkeletalMeshData; }
    static USkeletalMesh* GetSkeletalMesh(const FString& FilePath);
    static USkeletalMesh* GetSkeletalMeshData(const FString& FilePath) { return SkeletalMeshMap[FilePath]; }
    static const TMap<FString, USkeletalMesh*>& GetSkeletalMeshes() { return SkeletalMeshMap;}

    static UAnimSequence* CreateAnimationSequence(const FString& FilePath);
    static UAnimSequence* GetAnimationSequence(const FString& FilePath);

    static UAnimDataModel* GetAnimDataModel(const FString& FilePath);

    static void AddVertexFromControlPoint(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int ControlPointIndex);

    static void DebugWriteAnimationModel(const UAnimDataModel* AnimModel);

    // 일단 SkeletalMesh 안 만들어지면 false 반환
    static bool ImportFBX(const FString& FilePath);
private:
    // Helper: convert FbxAMatrix to Unreal FMatrix
    static FMatrix ConvertFbxMatrix(const FbxAMatrix& M);
    
    static FbxManager* FbxManager;
    
    static TMap<FName, FSkeletalMeshRenderData*> SkeletalMeshData;
    static TMap<FString, USkeletalMesh*> SkeletalMeshMap;
    static TMap<FName, FRefSkeletal*> RefSkeletalData;

    static TMap<FName, UAnimSequence*> SkeletalAnimSequences;
    static TMap<FName, UAnimDataModel*> AnimDataModels;
};