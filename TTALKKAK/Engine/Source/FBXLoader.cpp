#include "FBXLoader.h"

#include "Components/Material/Material.h"
#include "Container/FFileHelper.h"
#include "Container/Path.h"
#include "Engine/AssetManager.h"
#include "Engine/FLoaderOBJ.h"
#include "Engine/Assets/Animation/AnimDataModel.h"
#include "Engine/Assets/Animation/IAnimationDataModel.h"
#include "Engine/Assets/Animation/UAnimationAsset.h"

FbxManager* TestFBXLoader::FbxManager = nullptr;
    
TMap<FName, FSkeletalMeshRenderData*> TestFBXLoader::SkeletalMeshData = {};
TMap<FString, USkeletalMesh*> TestFBXLoader::SkeletalMeshMap = {};
TMap<FName, FRefSkeletal*> TestFBXLoader::RefSkeletalData = {};

TMap<FName, UAnimSequence*> TestFBXLoader::SkeletalAnimSequences = {};
TMap<FName, UAnimDataModel*> TestFBXLoader::AnimDataModels = {};

bool FBXLoader::InitFBXManager()
{
    FbxManager = FbxManager::Create();

    FbxIOSettings* IOSetting = FbxIOSettings::Create(FbxManager, IOSROOT);
    FbxManager->SetIOSettings(IOSetting);

    return true;
}

FSkeletalMeshRenderData* FBXLoader::ParseFBX(const FString& FilePath)
{
    static bool bInitialized = false;
    if (bInitialized == false)
    {
        InitFBXManager();
        bInitialized = true;
    }
    FbxImporter* Importer = FbxImporter::Create(FbxManager, "myImporter");
    FbxScene* Scene = FbxScene::Create(FbxManager, "myScene");

    if (bIsAbsolutePath == false)
    {
        const bool bResult = Importer->Initialize(GetData("Contents\\" + FilePath), -1, FbxManager->GetIOSettings());
        if (!bResult)
            return nullptr;
    }
    else
    {
        const bool bResult = Importer->Initialize(GetData(FilePath), -1, FbxManager->GetIOSettings());
        if (!bResult)
            return nullptr;
    }
    
    Importer->Import(Scene);

    FbxGeometryConverter geomConverter(FbxManager);
    geomConverter.Triangulate(Scene, /*replace=*/true);
    
    const FbxAxisSystem UnrealAxisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
    
    // FBX 파일 내 원본 단위
    FbxSystemUnit SceneUnit = Scene->GetGlobalSettings().GetSystemUnit();
    // 언리얼 기본 단위: 센티미터(cm)
    const FbxSystemUnit UnrealUnit = FbxSystemUnit::cm;

    // 이미 cm 단위가 아니라면 변환
    if (SceneUnit.GetScaleFactor() != UnrealUnit.GetScaleFactor())
    {
        UnrealUnit.ConvertScene(Scene);
    }
    
    UnrealAxisSystem.ConvertScene(Scene);
    
    Importer->Import(Scene);
    Importer->Destroy();

    FSkeletalMeshRenderData* NewMeshData = new FSkeletalMeshRenderData();
    FRefSkeletal* RefSkeletal = new FRefSkeletal();
    
    NewMeshData->Name = FilePath;
    RefSkeletal->Name = FilePath;

    TMap<FString, FbxNode*> NodeMap;
    ExtractFBXMeshData(Scene, NewMeshData, RefSkeletal, NodeMap);

    for (int i = 0; i < NewMeshData->Vertices.Num(); ++i)
    {
        FSkeletalVertex Vertex;
        Vertex = NewMeshData->Vertices[i];
        RefSkeletal->RawVertices.Add(Vertex);
    }

    int AnimStackCount = Scene->GetSrcObjectCount<FbxAnimStack>();
    
    for (int s = 0; s < AnimStackCount; ++s)
    {
        // AnimStack 순회 내부
        FbxTimeSpan TimeSpan;
        
        FbxAnimStack* AnimStack = Scene->GetSrcObject<FbxAnimStack>(s);
        Scene->SetCurrentAnimationStack(AnimStack);

        // 첫 번째 레이어만 사용
        FbxAnimLayer* Layer = AnimStack->GetMember<FbxAnimLayer>(0);

        // 애니메이션 모델 초기화
        UAnimDataModel* AnimModel = FObjectFactory::ConstructObject<UAnimDataModel>(nullptr);
        // 1. 해당 스택 이름으로 TakeInfo 조회
        FbxTakeInfo* TakeInfo = Scene->GetTakeInfo(AnimStack->GetName());
        if (TakeInfo)
        {
            TimeSpan = TakeInfo->mLocalTimeSpan;
        }
        else
        {
            // TakeInfo가 없으면 글로벌 타임라인 사용
            Scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(TimeSpan);
        }

        // 2. 재생 길이 계산
        double DurationSeconds = TimeSpan.GetDuration().GetSecondDouble();
        AnimModel->PlayLength = static_cast<float>(DurationSeconds);
        
        // 2-2) FBX 파일에 정의된 프레임레이트 읽기
        FbxTime::EMode TimeMode = Scene->GetGlobalSettings().GetTimeMode();
        double FBX_FPS = FbxTime::GetFrameRate(TimeMode);

        // 3) 분모=1 로 간단히 만들거나, 필요하다면 분수 처리
        uint32 Numer = static_cast<uint32>(FMath::RoundToInt(FBX_FPS));
        AnimModel->FrameRate = FFrameRate(Numer, 1);

        // 2-3) NumberOfFrames 계산 (프레임 사이 간격 1/FrameRate)
        AnimModel->NumberOfFrames = AnimModel->FrameRate.AsFrames(AnimModel->PlayLength) + 1;


        // 3) 루트 본부터 재귀하며 키 트랙 추출
        for (const int RootIdx : RefSkeletal->RootBoneIndices)
        {
            ExtractAnimation(RootIdx, *RefSkeletal, Layer, AnimModel, NodeMap);
        }

        DebugWriteAnimationModel(AnimModel);

        AnimDataModels.Add(FilePath, AnimModel);
    }
    
    
    SkeletalMeshData.Add(FilePath, NewMeshData);
    RefSkeletalData.Add(FilePath, RefSkeletal);
    
    Importer->Destroy();
    Scene->Destroy();
    
    return NewMeshData;
}

void FBXLoader::ExtractFBXMeshData(const FbxScene* Scene, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal)
{
    FbxNode* RootNode = Scene->GetRootNode();
    if (RootNode == nullptr)
        return;
    
    ExtractMeshFromNode(RootNode, MeshData, RefSkeletal, OutNodeMap);
}

/* Extract할 때 FBX의 Mapping Mode와 Reference Mode에 따라 모두 다르게 파싱을 진행해야 함!! */
void FBXLoader::ExtractMeshFromNode(FbxNode* Node, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal)
{
    if (Node != nullptr)
    {
        OutNodeMap.Add(FString(Node->GetName()), Node);
    }
    
    FbxMesh* Mesh = Node->GetMesh();
    if (Mesh)
    {
        int BaseVertexIndex = MeshData->Vertices.Num();
        int BaseIndexOffset = MeshData->Indices.Num();
        
        for (int d = 0; d < Mesh->GetDeformerCount(FbxDeformer::eSkin); ++d)
        {
            auto* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(d, FbxDeformer::eSkin));
            if (Skin)
            {
                // BaseVertexIndex는 Vertex 추출 직후 오프셋을 위해 넘겨 줍니다.
                ProcessSkinning(Skin, MeshData, RefSkeletal, BaseVertexIndex);
                // 보통 메시당 하나의 Skin만 쓰므로 break 해도 무방합니다.
                break;
            }
        }

        // 버텍스 데이터 추출
        ExtractVertices(Mesh, MeshData, RefSkeletal);
        
        // 인덱스 데이터 추출, 
        ExtractIndices(Mesh, MeshData, BaseVertexIndex);
        
        // 머테리얼 데이터 추출
        ExtractMaterials(Node, Mesh, MeshData, RefSkeletal, BaseIndexOffset);
        
        // 바운딩 박스 업데이트
        UpdateBoundingBox(*MeshData);
    }

    // 자식 노드들에 대해 재귀적으로 수행
    const int childCount = Node->GetChildCount();
    for (int i = 0; i < childCount; i++)
    {
        ExtractMeshFromNode(Node->GetChild(i), MeshData, RefSkeletal, OutNodeMap);
    }
}

void FBXLoader::ExtractVertices(
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* MeshData,
    FRefSkeletal* RefSkeletal)
{

    // 1) Normal Element의 매핑 모드로 분기
    auto* NormalElem = Mesh->GetElementNormal();
    auto  mapMode    = NormalElem
                      ? NormalElem->GetMappingMode()
                      : FbxGeometryElement::eByControlPoint;

    int BaseVertexIndex = MeshData->Vertices.Num();

    // 2) 모드별로 정점 생성
    if (mapMode == FbxGeometryElement::eByControlPoint)
    {
        // ControlPoint 기준 1:1
        int CPCount = Mesh->GetControlPointsCount();
        MeshData->Vertices.Reserve(BaseVertexIndex + CPCount);

        for (int cpIdx = 0; cpIdx < CPCount; ++cpIdx)
            AddVertexFromControlPoint(Mesh, MeshData, cpIdx);
    }
    if (mapMode == FbxGeometryElement::eByPolygonVertex)
    {
        // 폴리곤×버텍스 수만큼 복제
        int totalPV = 0;
        int polyCnt = Mesh->GetPolygonCount();
        for (int p = 0; p < polyCnt; ++p)
            totalPV += Mesh->GetPolygonSize(p);

        MeshData->Vertices.Reserve(BaseVertexIndex + totalPV);

        for (int p = 0; p < polyCnt; ++p)
        {
            int polySize = Mesh->GetPolygonSize(p);
            for (int v = 0; v < polySize; ++v)
            {
                int cpIdx = Mesh->GetPolygonVertex(p, v);
                AddVertexFromControlPoint(Mesh, MeshData, cpIdx);
            }
        }
    }

    // 3) Attribute(법선, UV, 탄젠트, 스키닝) 채우기
    ExtractNormals       (Mesh, MeshData, BaseVertexIndex);
    ExtractUVs           (Mesh, MeshData, BaseVertexIndex);
    ExtractTangents      (Mesh, MeshData, BaseVertexIndex);
    ExtractSkinningData  (Mesh, MeshData, RefSkeletal, BaseVertexIndex);
}

void FBXLoader::ExtractNormals(
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* RenderData,
    int BaseVertexIndex)
{
    auto* NormalElem = Mesh->GetElementNormal();
    if (!NormalElem)
    {
        // 노말 정보가 없으면 (0,0,1)로 초기화 예시
        for (int i = BaseVertexIndex; i < RenderData->Vertices.Num(); ++i)
        {
            RenderData->Vertices[i].Normal = FVector(0, 0, 1);
        }
        return;
    }

    // 매핑·레퍼런스 모드
    auto mapMode = NormalElem->GetMappingMode();
    auto refMode = NormalElem->GetReferenceMode();


    int directCount = NormalElem->GetDirectArray().GetCount();
    int indexCount  = refMode != FbxGeometryElement::eDirect ? NormalElem->GetIndexArray().GetCount() : 0;

    int polyVertCounter   = 0;
    int vertexBufferIndex = BaseVertexIndex;
    int polyCount         = Mesh->GetPolygonCount();

    for (int p = 0; p < polyCount; ++p)
    {
        int polySize = Mesh->GetPolygonSize(p);
        for (int v = 0; v < polySize; ++v)
        {
            int ctrlIdx = Mesh->GetPolygonVertex(p, v);
            int idx = -1;

            // 1) 인덱스 계산 (switch 문으로 람다 대체)
            switch (mapMode)
            {
                case FbxGeometryElement::eByControlPoint:
                    idx = (refMode == FbxGeometryElement::eDirect)
                          ? ctrlIdx
                          : NormalElem->GetIndexArray().GetAt(ctrlIdx);
                    break;

                case FbxGeometryElement::eByPolygonVertex:
                    idx = (refMode == FbxGeometryElement::eDirect)
                          ? polyVertCounter
                          : NormalElem->GetIndexArray().GetAt(polyVertCounter);
                    break;

                default:
                    // eByPolygon, eAllSame 등 지원 필요 시 여기에 추가
                    idx = -1;
                    break;
            }

            // 2) directArray 범위를 벗어나면 0으로 클램프
            if (idx < 0 || idx >= directCount)
            {
                UE_LOG(LogLevel::Warning,TEXT("Normal index %d out of range [0,%d). Clamped to 0."), idx, directCount);
                idx = 0;
            }

            // 3) 노말 추출
            auto Nor = NormalElem->GetDirectArray().GetAt(idx);

            // 4) vertexBufferIndex 범위 검사 후 대입
            if (vertexBufferIndex < RenderData->Vertices.Num())
            {
                auto& V = RenderData->Vertices[vertexBufferIndex];
                V.Normal.X = static_cast<float>(Nor[0]);
                V.Normal.Y = static_cast<float>(Nor[1]);
                V.Normal.Z = static_cast<float>(Nor[2]);
            }
            else
            {
                UE_LOG(LogLevel::Error,TEXT("vertexBufferIndex %d >= Vertices.Num() %d"), vertexBufferIndex, RenderData->Vertices.Num());
                return;  // 더 이상 진행하지 않음
            }

            ++polyVertCounter;
            ++vertexBufferIndex;
        }
    }
}

void FBXLoader::ExtractUVs(
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* MeshData,
    int BaseVertexIndex)
{
    auto* UVElem = Mesh->GetElementUV(0);
    if (!UVElem)
    {
        // UV가 없으면 (0,0)으로 초기화
        for(int i = BaseVertexIndex; i < MeshData->Vertices.Num(); ++i)
        {
            MeshData->Vertices[i].TexCoord = FVector2D::ZeroVector;
        }
        return;
    }

    // Mapping / Reference 모드
    auto mapMode = UVElem->GetMappingMode();
    auto refMode = UVElem->GetReferenceMode();
    int directCount = UVElem->GetDirectArray().GetCount();
    int indexCount  = (refMode != FbxGeometryElement::eDirect)
                      ? UVElem->GetIndexArray().GetCount()
                      : 0;

    int polyVertCounter   = 0;
    int vertexBufferIndex = BaseVertexIndex;
    int polyCount         = Mesh->GetPolygonCount();

    for (int p = 0; p < polyCount; ++p)
    {
        int polySize = Mesh->GetPolygonSize(p);
        for (int v = 0; v < polySize; ++v)
        {
            int ctrlIdx = Mesh->GetPolygonVertex(p, v);
            int idx = -1;

            // 인덱스 계산
            if (mapMode == FbxGeometryElement::eByControlPoint)
            {
                idx = (refMode == FbxGeometryElement::eDirect)
                      ? ctrlIdx
                      : UVElem->GetIndexArray().GetAt(ctrlIdx);
            }
            else if (mapMode == FbxGeometryElement::eByPolygonVertex)
            {
                idx = (refMode == FbxGeometryElement::eDirect)
                      ? polyVertCounter
                      : UVElem->GetIndexArray().GetAt(polyVertCounter);
            }
            else
            {
                // eByPolygon, eAllSame 등 지원 필요 시 여기에 추가
                idx = -1;
            }

            // DirectArray 범위 체크
            if (idx < 0 || idx >= directCount)
            {
                UE_LOG(LogLevel::Warning,
                    TEXT("UV index %d out of range [0,%d). Clamped to 0."), idx, directCount);
                idx = 0;
            }

            // vertexBufferIndex 범위 체크
            if (vertexBufferIndex >= MeshData->Vertices.Num())
            {
                UE_LOG(LogLevel::Error,
                    TEXT("vertexBufferIndex %d >= Vertices.Num() %d"),
                    vertexBufferIndex, MeshData->Vertices.Num());
                return;
            }

            // 실제 UV 추출
            auto UV = UVElem->GetDirectArray().GetAt(idx);
            auto& V  = MeshData->Vertices[vertexBufferIndex];
            V.TexCoord.X = static_cast<float>(UV[0]);
            V.TexCoord.Y = 1.0f - static_cast<float>(UV[1]);  // DirectX용 Y 반전

            ++polyVertCounter;
            ++vertexBufferIndex;
        }
    }
}

void FBXLoader::ExtractTangents(FbxMesh* Mesh, FSkeletalMeshRenderData* MeshData, int BaseVertexIndex)
{
   // 1) Tangent 요소 가져오기 / 생성
    auto* TanElem = Mesh->GetElementTangent(0);
    if (!TanElem || TanElem->GetDirectArray().GetCount() == 0)
    {
        UE_LOG(LogLevel::Warning, TEXT("FBX Tangent Empty! Generating..."));
        Mesh->GenerateTangentsData(0, /*overwrite=*/ true);
        TanElem = Mesh->GetElementTangent(0);
        if (!TanElem || TanElem->GetDirectArray().GetCount() == 0)
        {
            UE_LOG(LogLevel::Error, TEXT("Failed to generate FBX tangents."));
            return;
        }
    }

    // 2) 모드 & 카운트
    auto mapMode = TanElem->GetMappingMode();
    auto refMode = TanElem->GetReferenceMode();
    int directCount = TanElem->GetDirectArray().GetCount();
    int indexCount  = (refMode != FbxGeometryElement::eDirect)
                      ? TanElem->GetIndexArray().GetCount()
                      : 0;

    int polyVertCounter   = 0;
    int vertexBufferIndex = BaseVertexIndex;
    int polyCount         = Mesh->GetPolygonCount();
    int totalVertices     = MeshData->Vertices.Num();

    // 3) 폴리곤-버텍스 순회
    for (int p = 0; p < polyCount; ++p)
    {
        int polySize = Mesh->GetPolygonSize(p);
        for (int v = 0; v < polySize; ++v)
        {
            int ctrlIdx = Mesh->GetPolygonVertex(p, v);
            int idx = -1;

            // 3-1) 인덱스 계산
            if (mapMode == FbxGeometryElement::eByControlPoint)
            {
                idx = (refMode == FbxGeometryElement::eDirect)
                      ? ctrlIdx
                      : TanElem->GetIndexArray().GetAt(ctrlIdx);
            }
            else if (mapMode == FbxGeometryElement::eByPolygonVertex)
            {
                idx = (refMode == FbxGeometryElement::eDirect)
                      ? polyVertCounter
                      : TanElem->GetIndexArray().GetAt(polyVertCounter);
            }
            else
            {
                UE_LOG(LogLevel::Warning,TEXT("Unsupported Tangent MappingMode %d"), (int)mapMode);
                return;
            }

            // 3-2) DirectArray 범위 검사
            if (idx < 0 || idx >= directCount)
            {
                UE_LOG(LogLevel::Warning,TEXT("Tangent index %d out of range [0,%d). Clamped to 0."),
                    idx, directCount);
                idx = 0;
            }

            // 3-3) vertexBufferIndex 범위 검사
            if (vertexBufferIndex < 0 || vertexBufferIndex >= totalVertices)
            {
                UE_LOG(LogLevel::Error,TEXT("vertexBufferIndex %d >= Vertices.Num() %d"),
                    vertexBufferIndex, totalVertices);
                return;
            }

            // 4) 실제 Tangent 복사
            auto Tan = TanElem->GetDirectArray().GetAt(idx);
            auto& V = MeshData->Vertices[vertexBufferIndex];
            V.Tangent.X = static_cast<float>(Tan[0]);
            V.Tangent.Y = static_cast<float>(Tan[1]);
            V.Tangent.Z = static_cast<float>(Tan[2]);

            ++polyVertCounter;
            ++vertexBufferIndex;
        }
    }
}

void FBXLoader::ExtractSkinningData(
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* MeshData,
    FRefSkeletal* RefSkeletal,
    int BaseVertexIndex)
{
    // 1) 매핑 모드 재확인
    auto* NormalElem = Mesh->GetElementNormal();
    auto  mapMode    = NormalElem
                      ? NormalElem->GetMappingMode()
                      : FbxGeometryElement::eByControlPoint;

    // 2) ControlPoint별로 최대 4개 본/웨이트 수집
    int cpCount = Mesh->GetControlPointsCount();
    struct BW { int Indices[4]; float Weights[4]; };
    std::vector<BW> cpWeights(cpCount);
    // 초기화
    for (int i = 0; i < cpCount; ++i)
        for (int j = 0; j < 4; ++j)
            cpWeights[i].Indices[j] = 0,
            cpWeights[i].Weights[j] = 0.f;

    // 스킨(Deformer) 순회
    int skinCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int s = 0; s < skinCount; ++s)
    {
        auto* skin = static_cast<FbxSkin*>(Mesh->GetDeformer(s, FbxDeformer::eSkin));
        if (!skin) continue;

        // 클러스터(본) 순회
        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; ++c)
        {
            auto* clus = skin->GetCluster(c);
            FbxNode* linkNode = clus->GetLink();
            if (!linkNode) continue;

            // ★ 하드코딩: Map에서 본 이름으로 인덱스 꺼내기
            FString boneName(linkNode->GetName());
            int*   pBoneIdx = RefSkeletal->BoneNameToIndexMap.Find(boneName);
            if (!pBoneIdx)
                continue;   // 레퍼런스 스켈레톤에 본이 없으면 스킵
            int boneIdx = *pBoneIdx;

            // ControlPoint 웨이트 애셋
            int*    cpIdxArr = clus->GetControlPointIndices();
            double* cpWArr   = clus->GetControlPointWeights();
            int     cnt      = clus->GetControlPointIndicesCount();

            for (int k = 0; k < cnt; ++k)
            {
                int   cpId = cpIdxArr[k];
                float w    = static_cast<float>(cpWArr[k]);
                auto& B    = cpWeights[cpId];

                // 가장 작은 슬롯 교체
                int minSlot = 0;
                for (int m = 1; m < 4; ++m)
                    if (B.Weights[m] < B.Weights[minSlot])
                        minSlot = m;
                if (w > B.Weights[minSlot])
                {
                    B.Indices[minSlot] = boneIdx;
                    B.Weights[minSlot] = w;
                }
            }
        }
    }

    // 3) ControlPoint별 정규화
    for (auto& B : cpWeights)
    {
        float sum = B.Weights[0] + B.Weights[1] + B.Weights[2] + B.Weights[3];
        if (sum > 0.f)
            for (int j = 0; j < 4; ++j)
                B.Weights[j] /= sum;
        else
            B.Indices[0] = 0, B.Weights[0] = 1.f;
    }

    // 4) 매핑 모드별로 복제된 Vertices에 웨이트 복사
    if (mapMode == FbxGeometryElement::eByControlPoint)
    {
        // ControlPoint 기준 1:1
        for (int cpIdx = 0; cpIdx < cpCount; ++cpIdx)
        {
            auto& Bsrc = cpWeights[cpIdx];
            auto& V    = MeshData->Vertices[BaseVertexIndex + cpIdx];
            for (int j = 0; j < 4; ++j)
            {
                V.BoneIndices[j] = Bsrc.Indices[j];
                V.BoneWeights[j] = Bsrc.Weights[j];
            }
        }
    }
    else // eByPolygonVertex
    {
        int polyVertCounter   = 0;
        int vertexBufferIndex = BaseVertexIndex;
        int polyCount         = Mesh->GetPolygonCount();

        for (int p = 0; p < polyCount; ++p)
        {
            int sz = Mesh->GetPolygonSize(p);
            for (int v = 0; v < sz; ++v)
            {
                int cpIdx = Mesh->GetPolygonVertex(p, v);
                auto& Bsrc = cpWeights[cpIdx];
                auto& V    = MeshData->Vertices[vertexBufferIndex];
                for (int j = 0; j < 4; ++j)
                {
                    V.BoneIndices[j] = Bsrc.Indices[j];
                    V.BoneWeights[j] = Bsrc.Weights[j];
                }

                ++polyVertCounter;
                ++vertexBufferIndex;
            }
        }
    }
}


void FBXLoader::ProcessSkinning(FbxSkin* Skin, FSkeletalMeshRenderData* MeshData, FRefSkeletal* RefSkeletal, int BaseVertexIndex)
{
    int ClusterCount = Skin->GetClusterCount();

    // Clear existing bone tree data
    RefSkeletal->BoneTree.Empty();
    RefSkeletal->RootBoneIndices.Empty();
    RefSkeletal->BoneNameToIndexMap.Empty();
    
    // First pass - collect all bone nodes from clusters and add to flat bone array
    for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();
        
        if (!BoneNode)
            continue;
            
        FString BoneName = BoneNode->GetName();
        
        // Check if this bone already exists
        int* ExistingBoneIndex = RefSkeletal->BoneNameToIndexMap.Find(BoneName);
        if (ExistingBoneIndex)
            continue;
            
        // Create new bone and add to array
        FBone NewBone;
        NewBone.BoneName = BoneName;
        NewBone.ParentIndex = -1; // Will be set in second pass
        
        // Get transform matrices
        FbxAMatrix GlobalTransform = BoneNode->EvaluateGlobalTransform();
        FbxAMatrix LocalTransform = BoneNode->EvaluateLocalTransform();
        
        // Store transforms
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                NewBone.GlobalTransform.M[i][j] = static_cast<float>(GlobalTransform.Get(i, j));
                NewBone.LocalTransform.M[i][j] = static_cast<float>(LocalTransform.Get(i, j));
            }
        }
        
        // Get binding pose transformation
      //  NewBone.InverseBindPoseMatrix = FMatrix::Inverse(NewBone.GlobalTransform);
        FbxAMatrix MeshTransform; // Mesh의 바인드 시점의 Global Transform
        Cluster->GetTransformMatrix(MeshTransform); // 메시의 변환 행렬 (기준점)

        FbxAMatrix LinkTransform; // Bone의 바인드 시점의 Global Transform
        Cluster->GetTransformLinkMatrix(LinkTransform); // 본의 바인드 포즈 행렬
        
        FbxAMatrix InverseBindMatrix = LinkTransform.Inverse() * MeshTransform;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                NewBone.InverseBindPoseMatrix.M[i][j] = static_cast<float>(InverseBindMatrix.Get(i, j));
            }
        }
        
        NewBone.SkinningMatrix = NewBone.InverseBindPoseMatrix * NewBone.GlobalTransform;
        
        // Add bone to array and create mapping
        int BoneIndex = MeshData->Bones.Add(NewBone);
        RefSkeletal->BoneNameToIndexMap.Add(BoneName, BoneIndex);
        
        // Create corresponding bone tree node
        FBoneNode NewNode;
        NewNode.BoneName = BoneName;
        NewNode.BoneIndex = BoneIndex;
        RefSkeletal->BoneTree.Add(NewNode);
    }
    
    // Second pass - establish parent-child relationships
    for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();
        
        if (!BoneNode)
            continue;
            
        FString BoneName = BoneNode->GetName();
        FbxNode* ParentNode = BoneNode->GetParent();
        
        if (!RefSkeletal->BoneNameToIndexMap.Contains(BoneName))
            continue;
            
        int BoneIndex = RefSkeletal->BoneNameToIndexMap[BoneName];
        
        if (ParentNode)
        {
            FString ParentName = ParentNode->GetName();
            
            // If parent is also a bone, establish the relationship
            if (RefSkeletal->BoneNameToIndexMap.Contains(ParentName))
            {
                int ParentIndex = RefSkeletal->BoneNameToIndexMap[ParentName];
                
                // Update parent index in the bone
                MeshData->Bones[BoneIndex].ParentIndex = ParentIndex;
                
                // Add this bone as a child of the parent in the tree structure
                RefSkeletal->BoneTree[ParentIndex].ChildIndices.Add(BoneIndex);
            }
        }
    }
    
    // Find root bones (bones with no parents in our bone set)
    for (int i = 0; i < MeshData->Bones.Num(); i++)
    {
        if (MeshData->Bones[i].ParentIndex == -1)
        {
            RefSkeletal->RootBoneIndices.Add(i);
        }
    }
    
    // Process vertex weights
    for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ClusterIndex++)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();
        
        if (!BoneNode || !RefSkeletal->BoneNameToIndexMap.Contains(BoneNode->GetName()))
            continue;
        
        int BoneIndex = RefSkeletal->BoneNameToIndexMap[BoneNode->GetName()];
        
        // Get control point indices and weights
        int VertexCount = Cluster->GetControlPointIndicesCount();
        int* ControlPointIndices = Cluster->GetControlPointIndices();
        double* ControlPointWeights = Cluster->GetControlPointWeights();
        
        // Apply weights to vertices
        for (int i = 0; i < VertexCount; i++)
        {
            int VertexIndex = BaseVertexIndex + ControlPointIndices[i];
            float Weight = static_cast<float>(ControlPointWeights[i]);
            
            // Make sure vertex index is valid
            if (VertexIndex >= 0 && VertexIndex < MeshData->Vertices.Num())
            {
                // Find first empty weight slot
                for (int j = 0; j < 4; j++)
                {
                    if (MeshData->Vertices[VertexIndex].BoneWeights[j] == 0.0f)
                    {
                        MeshData->Vertices[VertexIndex].BoneIndices[j] = BoneIndex;
                        MeshData->Vertices[VertexIndex].BoneWeights[j] = Weight;
                        break;
                    }
                }
            }
        }
    }
}

void FBXLoader::ExtractIndices(
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* MeshData,
    int BaseVertexIndex)
{
    // 1) 정점 생성 때 쓴 매핑 모드 재확인
    auto* NormalElem = Mesh->GetElementNormal();
    auto  mapMode    = NormalElem
                      ? NormalElem->GetMappingMode()
                      : FbxGeometryElement::eByControlPoint;

    int polyVertCounter = 0;   // 폴리곤-버텍스 모드용 누적 카운터
    int polyCount       = Mesh->GetPolygonCount();

    for (int p = 0; p < polyCount; ++p)
    {
        int polySize = Mesh->GetPolygonSize(p);
        int pvStart  = polyVertCounter;  // 이 폴리곤의 시작 polyVert 인덱스

        // 삼각형 팬 트라이앵글
        for (int i = 2; i < polySize; ++i)
        {
            if (mapMode == FbxGeometryElement::eByControlPoint)
            {
                // ControlPoint 기준 인덱스
                int c0 = Mesh->GetPolygonVertex(p, 0);
                int c1 = Mesh->GetPolygonVertex(p, i - 1);
                int c2 = Mesh->GetPolygonVertex(p, i);
                MeshData->Indices.Add(BaseVertexIndex + c0);
                MeshData->Indices.Add(BaseVertexIndex + c1);
                MeshData->Indices.Add(BaseVertexIndex + c2);
            }
            else
            {
                // 폴리곤-버텍스 기준 인덱스
                MeshData->Indices.Add(BaseVertexIndex + pvStart + 0);
                MeshData->Indices.Add(BaseVertexIndex + pvStart + (i - 1));
                MeshData->Indices.Add(BaseVertexIndex + pvStart + i);
            }
        }

        polyVertCounter += polySize;
    }
}


void FBXLoader::ExtractMaterials(
    FbxNode* Node,
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* MeshData,
    FRefSkeletal* RefSkeletal,
    int BaseIndexOffset)
{
    auto* MatElem = Mesh->GetElementMaterial();
    int  matCount = Node->GetMaterialCount();

    // 매핑·레퍼런스 모드
    auto mapMode = MatElem
                 ? MatElem->GetMappingMode()
                 : FbxGeometryElement::eAllSame;
    auto refMode = MatElem
                 ? MatElem->GetReferenceMode()
                 : FbxGeometryElement::eDirect;

    int polyVertCounter = 0;
    int polyCount       = Mesh->GetPolygonCount();

    // 총 삼각형 수 미리 계산 (eAllSame, eByPolygon 모두 공통)
    int totalTris = 0;
    for (int p = 0; p < polyCount; ++p)
        totalTris += Mesh->GetPolygonSize(p) - 2;

    int currentOffset = BaseIndexOffset;

    for (int matIdx = 0; matIdx < matCount; ++matIdx)
    {
        // 이 재질에 속하는 삼각형 개수 세기
        int triCount = 0;
        polyVertCounter = 0;

        for (int p = 0; p < polyCount; ++p)
        {
            int polySize = Mesh->GetPolygonSize(p);

            // 폴리곤 하나당 매핑된 재질 인덱스를 구하는 방법
            int thisMat = 0;
            switch (mapMode)
            {
            case FbxGeometryElement::eAllSame:
                thisMat = 0; 
                break;
            case FbxGeometryElement::eByPolygon:
                thisMat = MatElem->GetIndexArray().GetAt(p);
                break;
            case FbxGeometryElement::eByPolygonVertex:
                {
                    // 1) 항상 IndexArray에서 머티리얼 레이어 인덱스 꺼내기
                    int layerMatIdx = MatElem->GetIndexArray().GetAt(polyVertCounter);

                    // 2) 그 인덱스로 노드의 실제 머티리얼 얻기
                    //    (thisMat은 단순히 머티리얼 번호로 사용)
                    thisMat = layerMatIdx;
                    break;
                }
            default:
                thisMat = 0;
            }

            if (thisMat == matIdx)
                triCount += (polySize - 2);

            polyVertCounter += (mapMode == FbxGeometryElement::eByPolygonVertex
                                ? polySize
                                : 1);
        }

        // Subset 만들기
        // Material 생성 & 등록
        FbxSurfaceMaterial* srcMtl = Node->GetMaterial(matIdx);
        FString            mtlName = srcMtl ? FString(srcMtl->GetName()) : TEXT("Mat") + FString::FromInt(matIdx);
        auto                newMtl = FManagerOBJ::CreateMaterial(ConvertFbxToObjMaterialInfo(srcMtl));
        int                 finalIdx = RefSkeletal->Materials.Add(newMtl);

        FMaterialSubset subset;
        subset.MaterialName  = mtlName;
        subset.MaterialIndex = finalIdx;
        subset.IndexStart    = currentOffset;
        subset.IndexCount    = triCount * 3;
        RefSkeletal->MaterialSubsets.Add(subset);

        currentOffset += triCount * 3;
    }

    // 재질이 하나도 없으면 디폴트
    if (matCount == 0)
    {
        UMaterial* DefaultMaterial = FManagerOBJ::GetDefaultMaterial();
        int MaterialIndex = RefSkeletal->Materials.Add(DefaultMaterial);
        
        FMaterialSubset Subset;
        Subset.MaterialName = DefaultMaterial->GetName();
        Subset.MaterialIndex = MaterialIndex;
        Subset.IndexStart = BaseIndexOffset;
        Subset.IndexCount = MeshData->Indices.Num() - BaseIndexOffset;
        
        RefSkeletal->MaterialSubsets.Add(Subset);
    }
}

void TestFBXLoader::ExtractAnimation(const int BoneTreeIndex, const FRefSkeletal& RefSkeletal, FbxAnimLayer* AnimLayer, UAnimDataModel* AnimModel,
    const TMap<FString, FbxNode*>& NodeMap)
{
        // 1) 엔진 본 노드 정보 가져오기
    const FBoneNode& BoneNode = RefSkeletal.BoneTree[BoneTreeIndex];
    // FBX 노드 매핑 테이블에서 해당 이름의 FbxNode*를 찾음
    FbxNode* Node = NodeMap.FindRef(BoneNode.BoneName);
    if (Node)
    {
        // 2) 트랙 생성 및 기본 정보 설정
        FBoneAnimationTrack Track;
        Track.Name          = FName(*BoneNode.BoneName);
        Track.BoneTreeIndex = BoneTreeIndex;
        FRawAnimSequenceTrack& Raw = Track.InternalTrackData;

        // 3) Translation/Rotation/Scale 커브 가져오기
        const FbxAnimCurve* Tx = Node->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
        const FbxAnimCurve* Ty = Node->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        const FbxAnimCurve* Tz = Node->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
        
        const FbxAnimCurve* Rx = Node->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
        const FbxAnimCurve* Ry = Node->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        const FbxAnimCurve* Rz = Node->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
        
        const FbxAnimCurve* Sx = Node->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
        const FbxAnimCurve* Sy = Node->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        const FbxAnimCurve* Sz = Node->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);

        // 4) 키 개수 결정 (Translation 중 하나의 커브 개수를 기준으로)
        int KeyCount = 0;
        if (Tx)       KeyCount = Tx->KeyGetCount();
        else if (Ty)  KeyCount = Ty->KeyGetCount();
        else if (Tz)  KeyCount = Tz->KeyGetCount();

        // 5) 키프레임별로 값 읽어서 Raw에 추가
        for (int k = 0; k < KeyCount; ++k)
        {
            // 시간 (초 단위)
            FbxTime t = Tx ? Tx->KeyGetTime(k)
                                  : Ty ? Ty->KeyGetTime(k)
                                  : Tz->KeyGetTime(k);
            float Time = static_cast<float>(t.GetSecondDouble());
            Raw.KeyTimes.Add(Time);

            // 위치
            Raw.PosKeys.Add(FVector(
                Tx ? (float)Tx->KeyGetValue(k) : Node->LclTranslation.Get()[0],
                Ty ? (float)Ty->KeyGetValue(k) : Node->LclTranslation.Get()[1],
                Tz ? (float)Tz->KeyGetValue(k) : Node->LclTranslation.Get()[2]
            ));

            // 회전 (Euler → Quat)
            FbxQuaternion q;
            if (Rx || Ry || Rz)
            {
                double ex = Rx ? Rx->KeyGetValue(k) : Node->LclRotation.Get()[0];
                double ey = Ry ? Ry->KeyGetValue(k) : Node->LclRotation.Get()[1];
                double ez = Rz ? Rz->KeyGetValue(k) : Node->LclRotation.Get()[2];
                
                // ex, ey, ez 는 Degree 단위 Euler 각
                FbxDouble3 EulerAngles(ex, ey, ez);

                // 매트릭스에 Euler 세팅
                FbxAMatrix RotMat;
                RotMat.SetR(EulerAngles);

                // 매트릭스에서 쿼터니언 추출
                q = RotMat.GetQ();
            }
            else
            {
                FbxDouble3 euler = Node->LclRotation.Get();
                
                // 매트릭스에 Euler 세팅
                FbxAMatrix RotMat;
                RotMat.SetR(euler);

                // 매트릭스에서 쿼터니언 추출
                q = RotMat.GetQ();
            }
            Raw.RotKeys.Add(FQuat(static_cast<float>(q[3]), static_cast<float>(q[0]), static_cast<float>(q[1]), static_cast<float>(q[2])));

            // 스케일
            Raw.ScaleKeys.Add(FVector(
                Sx ? (float)Sx->KeyGetValue(k) : Node->LclScaling.Get()[0],
                Sy ? (float)Sy->KeyGetValue(k) : Node->LclScaling.Get()[1],
                Sz ? (float)Sz->KeyGetValue(k) : Node->LclScaling.Get()[2]
            ));
        }

        // 6) 완성된 트랙을 모델에 추가
        AnimModel->BoneAnimationTracks.Add(Track);
    }

    // 7) 자식 본 트리 순회
    for (const int ChildIdx : BoneNode.ChildIndices)
    {
        ExtractAnimation(ChildIdx, RefSkeletal, AnimLayer, AnimModel, NodeMap);
    }
}

void FBXLoader::UpdateBoundingBox(FSkeletalMeshRenderData MeshData)
{
    if (MeshData.Vertices.Num() == 0)
        return;
        
    // 초기값 설정
    FVector Min = MeshData.Vertices[0].Position.xyz();
    FVector Max = MeshData.Vertices[0].Position.xyz();
    
    // 모든 정점을 순회하며 최소/최대값 업데이트
    for (int i = 1; i < MeshData.Vertices.Num(); i++)
    {
        const FVector& Pos = MeshData.Vertices[i].Position.xyz();
        
        // 최소값 갱신
        Min.X = FMath::Min(Min.X, Pos.X);
        Min.Y = FMath::Min(Min.Y, Pos.Y);
        Min.Z = FMath::Min(Min.Z, Pos.Z);
        
        // 최대값 갱신
        Max.X = FMath::Max(Max.X, Pos.X);
        Max.Y = FMath::Max(Max.Y, Pos.Y);
        Max.Z = FMath::Max(Max.Z, Pos.Z);
    }
    
    // 바운딩 박스 설정
    MeshData.BoundingBox.min = Min;
    MeshData.BoundingBox.max = Max;
}

FSkeletalMeshRenderData* FBXLoader::GetSkeletalRenderData(FString FilePath)
{
    // TODO: 폴더에서 가져올 수 있으면 가져오기
    if (SkeletalMeshData.Contains(FilePath))
    {
        return SkeletalMeshData[FilePath];
    }
    
    return nullptr;
}

// FBX 머티리얼 → FObjMaterialInfo 변환 헬퍼
FObjMaterialInfo FBXLoader::ConvertFbxToObjMaterialInfo(
    FbxSurfaceMaterial* FbxMat,
    const FString& BasePath)
{
    FObjMaterialInfo OutInfo;

    // Material Name
    OutInfo.MTLName = FString(FbxMat->GetName());
    
    // Lambert 전용 프로퍼티
    if (auto* Lam = FbxCast<FbxSurfaceLambert>(FbxMat))
    {
        // Ambient
        {
            auto c = Lam->Ambient.Get();
            float f = (float)Lam->AmbientFactor.Get();
            OutInfo.Ambient = FVector(c[0]*f, c[1]*f, c[2]*f);
        }
        // Diffuse
        {
            auto c = Lam->Diffuse.Get();
            float f = (float)Lam->DiffuseFactor.Get();
            OutInfo.Diffuse = FVector(c[0]*f, c[1]*f, c[2]*f);
        }
        // Emissive
        {
            auto c = Lam->Emissive.Get();
            float f = (float)Lam->EmissiveFactor.Get();
            OutInfo.Emissive = FVector(c[0]*f, c[1]*f, c[2]*f);
        }
        // BumpScale
        OutInfo.NormalScale = (float)Lam->BumpFactor.Get();
    }

    // Phong 전용 프로퍼티
    if (auto* Pho = FbxCast<FbxSurfacePhong>(FbxMat))
    {
        // Specular
        {
            auto c = Pho->Specular.Get();
            OutInfo.Specular = FVector((float)c[0], (float)c[1], (float)c[2]);
        }
        // Shininess
        OutInfo.SpecularScalar = (float)Pho->Shininess.Get();
    }

    // 공통 프로퍼티
    {
        // TransparencyFactor
        if (auto prop = FbxMat->FindProperty(FbxSurfaceMaterial::sTransparencyFactor); prop.IsValid())
        {
            double tf = prop.Get<FbxDouble>();
            OutInfo.TransparencyScalar = (float)tf;
            OutInfo.bTransparent = OutInfo.TransparencyScalar < 1.f - KINDA_SMALL_NUMBER;
        }

        // Index of Refraction
        constexpr char const* sIndexOfRefraction = "IndexOfRefraction";
        if (auto prop = FbxMat->FindProperty(sIndexOfRefraction); prop.IsValid())
        {
            OutInfo.DensityScalar = (float)prop.Get<FbxDouble>();
        }

        // Illumination Model은 FBX에 따로 없으므로 기본 0
        OutInfo.IlluminanceModel = 0;
    }

    // 텍스처 채널 (Diffuse, Ambient, Specular, Bump/Normal, Alpha)
    auto ReadFirstTexture = [&](const char* PropName, FString& OutName, FWString& OutPath)
    {
        auto prop = FbxMat->FindProperty(PropName);
        if (!prop.IsValid()) return;
        int nbTex = prop.GetSrcObjectCount<FbxFileTexture>();
        if (nbTex <= 0) return;
        if (auto* Tex = prop.GetSrcObject<FbxFileTexture>(0))
        {
            FString fname = FString(Tex->GetFileName());
            OutName = fname;
            OutPath = (BasePath + fname).ToWideString();
            OutInfo.bHasTexture = true;
        }
    };

    // map_Kd
    ReadFirstTexture(FbxSurfaceMaterial::sDiffuse,
                     OutInfo.DiffuseTextureName,
                     OutInfo.DiffuseTexturePath);
    // map_Ka
    ReadFirstTexture(FbxSurfaceMaterial::sAmbient,
                     OutInfo.AmbientTextureName,
                     OutInfo.AmbientTexturePath);
    // map_Ks
    ReadFirstTexture(FbxSurfaceMaterial::sSpecular,
                     OutInfo.SpecularTextureName,
                     OutInfo.SpecularTexturePath);
    // map_Bump 또는 map_Ns
    ReadFirstTexture(FbxSurfaceMaterial::sBump,
                     OutInfo.BumpTextureName,
                     OutInfo.BumpTexturePath);
    ReadFirstTexture(FbxSurfaceMaterial::sNormalMap,
                     OutInfo.NormalTextureName,
                     OutInfo.NormalTexturePath);
    // map_d (Alpha)
    ReadFirstTexture(FbxSurfaceMaterial::sTransparentColor,
                     OutInfo.AlphaTextureName,
                     OutInfo.AlphaTexturePath);

    return OutInfo;
}

USkeletalMesh* FBXLoader::CreateSkeletalMesh(const FString& FilePath)
{
    FSkeletalMeshRenderData* MeshData = ParseFBX(FilePath);
    if (MeshData == nullptr)
        return nullptr;

    USkeletalMesh* SkeletalMesh = GetSkeletalMesh(MeshData->Name);
    if (SkeletalMesh != nullptr)
    {
        USkeletalMesh* NewSkeletalMesh = SkeletalMesh->Duplicate(nullptr);
        NewSkeletalMesh->SetData(FilePath);
        return NewSkeletalMesh;
    }
    
    SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
    SkeletalMesh->SetData(FilePath);
    
    SkeletalMeshMap.Add(FilePath, SkeletalMesh);
    return SkeletalMesh;
}

USkeletalMesh* FBXLoader::GetSkeletalMesh(const FString& FilePath)
{
    if (SkeletalMeshMap.Contains(FilePath))
        return SkeletalMeshMap[FilePath];

    return nullptr;
}

FSkeletalMeshRenderData FBXLoader::GetCopiedSkeletalRenderData(FString FilePath)
{
    // 있으면 가져오고
    FSkeletalMeshRenderData* OriginRenderData = SkeletalMeshData[FilePath];
    if (OriginRenderData != nullptr)
    {
        return *OriginRenderData;
    }
    // 없으면 기본 생성자
    return {};
}

FRefSkeletal* FBXLoader::GetRefSkeletal(FString FilePath)
{
    // TODO: 폴더에서 가져올 수 있으면 가져오기
    if (RefSkeletalData.Contains(FilePath))
    {
        return RefSkeletalData[FilePath];
    }
    
    return nullptr;
}

UAnimSequence* FBXLoader::CreateAnimationSequence(const FString& FilePath)
{
    ParseFBX(FilePath);

    UAnimSequence* AnimSequence = GetAnimationSequence(FilePath);
    
    if (AnimSequence == nullptr)
    {
        AnimSequence = FObjectFactory::ConstructObject<UAnimSequence>(nullptr);

        UAnimDataModel* AnimDataModel = GetAnimDataModel(FilePath);
        AnimSequence->SetAnimDataModel(AnimDataModel);
        SkeletalAnimSequences.Add(FilePath, AnimSequence);
    }
    
    return AnimSequence;
}

UAnimSequence* FBXLoader::GetAnimationSequence(const FString& FilePath)
{
    if (SkeletalAnimSequences.Contains(FilePath))
        return SkeletalAnimSequences[FilePath];

    return nullptr;
}

UAnimDataModel* FBXLoader::GetAnimDataModel(const FString& FilePath)
{
    if (AnimDataModels.Contains(FilePath))
        return AnimDataModels[FilePath];

    return nullptr;
}

void FBXLoader::AddVertexFromControlPoint(
    FbxMesh* Mesh,
    FSkeletalMeshRenderData* MeshData,
    int ControlPointIndex)
{
    auto* ControlPoints = Mesh->GetControlPoints();
    FSkeletalVertex Vertex;

    // 위치
    auto& CP = ControlPoints[ControlPointIndex];
    Vertex.Position.X = static_cast<float>(CP[0]);
    Vertex.Position.Y = static_cast<float>(CP[1]);
    Vertex.Position.Z = static_cast<float>(CP[2]);
    Vertex.Position.W = 1.0f;

    // 기본값
    Vertex.Normal   = FVector(0.0f, 0.0f, 1.0f);
    Vertex.TexCoord = FVector2D(0.0f, 0.0f);
    Vertex.Tangent  = FVector(1.0f, 0.0f, 0.0f);

    MeshData->Vertices.Add(Vertex);
}

void FBXLoader::DebugWriteAnimationModel(const UAnimDataModel* AnimModel)
{
    if (!AnimModel)
    {
        UE_LOG(LogLevel::Warning, TEXT("DebugWriteAnimationModel: AnimModel is null."));
        return;
    }

    // 누적할 출력 문자열
    FString Output;

    // 각 본 트랙별로 정보 작성
    for (const FBoneAnimationTrack& Track : AnimModel->BoneAnimationTracks)
    {
        Output += FString::Printf(TEXT("Bone: %s\n"), *Track.Name.ToString());
        const FRawAnimSequenceTrack& Raw = Track.InternalTrackData;
        int32 KeyCount = Raw.KeyTimes.Num();

        for (int32 i = 0; i < KeyCount; ++i)
        {
            float Time = Raw.KeyTimes.IsValidIndex(i) ? Raw.KeyTimes[i] : 0.f;
            FVector Pos = Raw.PosKeys.IsValidIndex(i)   ? Raw.PosKeys[i]   : FVector::ZeroVector;
            FQuat   Rot = Raw.RotKeys.IsValidIndex(i)   ? Raw.RotKeys[i]   : FQuat();
            FVector Scale = Raw.ScaleKeys.IsValidIndex(i) ? Raw.ScaleKeys[i] : FVector::OneVector;

            Output += FString::Printf(
                TEXT("  Key %2d: Time=%.4f | Pos=(%.3f,%.3f,%.3f) | Rot=(%.3f,%.3f,%.3f,%.3f) | Scale=(%.3f,%.3f,%.3f)\n"),
                i, Time,
                Pos.X, Pos.Y, Pos.Z,
                Rot.X, Rot.Y, Rot.Z, Rot.W,
                Scale.X, Scale.Y, Scale.Z
            );
        }
        Output += TEXT("\n");
    }

    // 파일 경로: Saved/Logs 폴더
    const FString FilePath = TEXT("Logs/ExtractedAnimationDebug.txt");

    if (FFileHelper::WriteStringToLogFile(*FilePath, Output))
    {
        UE_LOG(LogLevel::Warning, TEXT("Animation debug written to %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogLevel::Error, TEXT("Failed to write animation debug to %s"), *FilePath);
    }
}

bool FBXLoader::ImportFBX(const FString& FilePath)
{
    ParseFBX(FilePath, true);

    FString StriptedPath = FPath::StripContentsPrefix(FilePath);
    
    USkeletalMesh* SkeletalMesh = GetSkeletalMesh(FilePath);
    if (SkeletalMesh == nullptr)
    {
        SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
        const FSkeletalMeshRenderData SkeletalMeshRenderData = GetCopiedSkeletalRenderData(FilePath);
        FRefSkeletal* RefSkeletal = GetRefSkeletal(FilePath);

        if (SkeletalMeshRenderData.Name == "Empty" || RefSkeletal == nullptr)
            return false;
        
        SkeletalMesh->SetData(SkeletalMeshRenderData, RefSkeletal);
        SkeletalMeshMap.Add(FilePath, SkeletalMesh); 
    }

    UAnimSequence* AnimSequence = GetAnimationSequence(FilePath);
    if (AnimSequence == nullptr)
    {
        AnimSequence = FObjectFactory::ConstructObject<UAnimSequence>(nullptr);

        UAnimDataModel* AnimDataModel = GetAnimDataModel(FilePath);
        
        AnimSequence->SetAnimDataModel(AnimDataModel);
        SkeletalAnimSequences.Add(FilePath, AnimSequence);
    }
    
    return true;
}
