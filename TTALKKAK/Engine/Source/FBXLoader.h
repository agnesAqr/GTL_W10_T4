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

    static void ExtractFBXMeshData(const FbxScene* Scene, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, TMap<FString
                                   , FbxNode*>& OutNodeMap);
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
    
    static void ExtractAnimation(int BoneTreeIndex, const FRefSkeletal& RefSkeletal, FbxAnimLayer* AnimLayer, UAnimDataModel* AnimModel, const TMap<FString, FbxNode*>& NodeMap, const TArray<FbxTime>& SampleTimes);

    static TArray<FbxTime> CollectSampleTimes(FbxAnimStack* AnimStack, FbxAnimLayer* AnimLayer, FbxScene* Scene, FbxTime::EMode pTimeMode = FbxTime::eFrames60, bool
                                              bUseKeyTimes = true);
    
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
    static void DebugWriteUV(const FSkeletalMeshRenderData* MeshData);
    static void DebugWriteVertex(const FSkeletalMeshRenderData* MeshData);
    static void DebugWriteIndex(const FSkeletalMeshRenderData* MeshData);

    // 일단 SkeletalMesh 안 만들어지면 false 반환
    static bool ImportFBX(const FString& FilePath);

    static void ExtractSkinningData(FbxMesh* Mesh, FRefSkeletal* RefSkeletal);
    static FSkeletalVertex GetVertexFromControlPoint(FbxMesh* Mesh, int PolygonIndex, int VertexIndex);
    static void ExtractNormal(FbxMesh* Mesh, FSkeletalVertex& Vertex, int PolygonIndex, int VertexIndex);
    static void ExtractUV(FbxMesh* Mesh, FSkeletalVertex& Vertex, int PolygonIndex, int VertexIndex);
    static void ExtractTangent(FbxMesh* Mesh, FSkeletalVertex& Vertex, int PolygonIndex, int VertexIndex);
    static void StoreWeights(FbxMesh* Mesh, FSkeletalVertex& Vertex, int PolygonIndex, int VertexIndex);
    static void StoreVertex(FSkeletalVertex& vertex, FSkeletalMeshRenderData* MeshData);

private:
    // Helper: convert FbxAMatrix to Unreal FMatrix
    static FMatrix ConvertFbxMatrix(const FbxAMatrix& M);
    
    static FbxManager* FbxManager;
    
    static TMap<FName, FSkeletalMeshRenderData*> SkeletalMeshData;
    static TMap<FString, USkeletalMesh*> SkeletalMeshMap;
    static TMap<FName, FRefSkeletal*> RefSkeletalData;

    static TMap<FString, uint32> IndexMap;
    
    struct FBoneWeightInfo
    {
        int BoneIndex;
        float BoneWeight;
    };
    
    static TMap<uint32, TArray<FBoneWeightInfo>> SkinWeightMap;
    
    static TMap<FName, UAnimSequence*> SkeletalAnimSequences;
    static TMap<FName, UAnimDataModel*> AnimDataModels;
};

inline void FBXLoader::ExtractSkinningData(FbxMesh* Mesh, FRefSkeletal* RefSkeletal)
{
    if (!Mesh) return;
    
    int skinCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int s = 0; s < skinCount; ++s)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(Mesh->GetDeformer(s, FbxDeformer::eSkin));
        int clustureCount = skin->GetClusterCount();
        for (int c = 0; c < clustureCount; ++c)
        {
            FbxCluster* cluster = skin->GetCluster(c);
            FbxNode* linkedBone = cluster->GetLink();
            if (!linkedBone)
                continue;

            FString boneName = linkedBone->GetName();
            int boneIndex = RefSkeletal->BoneNameToIndexMap[boneName];

            int* indices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            int count = cluster->GetControlPointIndicesCount();
            for (int i = 0; i < count; ++i)
            {
                int ctrlIdx = indices[i];
                float weight = static_cast<float>(weights[i]);
                if (!SkinWeightMap.Contains(ctrlIdx))
                    SkinWeightMap.Add(ctrlIdx, TArray<FBoneWeightInfo>());
                SkinWeightMap[ctrlIdx].Add({boneIndex, weight});
            }
        }
    } 
}
