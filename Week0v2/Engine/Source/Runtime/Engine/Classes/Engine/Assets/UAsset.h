#pragma once
#include "Container/Path.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

enum class EAssetType : uint8
{
    StaticMesh,
    SkeletalMesh,
    Texture2D,
    Material,
    Animation,
    MAX,
};

struct FAssetMetaData
{
    FName AssetName;      // Asset의 이름
    FPath FullPath;    // Asset의 패키지 경로
    EAssetType AssetType; // Asset의 타입
    uint32 Size;          // Asset의 크기 (바이트 단위)
};

// Base class for all intermediate asset data types
struct FAssetData
{
    virtual ~FAssetData() = default;
};

class UAsset : public UObject
{
    DECLARE_ABSTRACT_CLASS(UAsset, UObject)
public:
    UAsset() = default;
    ~UAsset() override;
    /**
    * 중간 데이터(AssetData)를 기반으로 런타임 리소스를 초기화합니다.
    * @param Data 파싱된 중간 데이터
    * @return 초기화 성공 여부
    */
    virtual bool InitializeFromData(std::shared_ptr<FAssetData> Data) = 0;

    /**
     * 런타임에 할당된 리소스(GPU 버퍼, 텍스처 등)를 해제합니다.
     */
    virtual void Unload() = 0;
    
    /**
     * 이 에셋의 메타데이터를 반환합니다.
     */
    const FAssetMetaData& GetMetaData() const { return MetaData; }

    /**
    * 메타데이터를 설정합니다. AssetManager나 Importer에서 호출
    */
    void SetMetaData(const FAssetMetaData& InMeta) { MetaData = InMeta; }

    /**
     * 에셋 타입 정보 반환 (StaticMesh, SkeletalMesh, Texture2D 등)
     */
    virtual EAssetType GetAssetType() const = 0;

private:
    /**
    * Import 단계에서 채워진 에셋 메타데이터
    */
    FAssetMetaData MetaData;
    
};
