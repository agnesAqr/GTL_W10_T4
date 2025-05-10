#pragma once
#include "Assets/UAsset.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

struct FAssetRegistry
{
    TMap<FName, FAssetMetaData> PathNameToAssetInfo;
};

class UAssetManager : public UObject
{
    DECLARE_CLASS(UAssetManager, UObject)

private:
    std::unique_ptr<FAssetRegistry> AssetRegistry;

public:
    UAssetManager() = default;

    static bool IsInitialized();

    /** UAssetManager를 가져옵니다. */
    static UAssetManager& Get();

    /** UAssetManager가 존재하면 가져오고, 없으면 nullptr를 반환합니다. */
    static UAssetManager* GetIfInitialized();
    
    void InitAssetManager();

    const TMap<FName, FAssetMetaData>& GetAssetRegistry();

public:
    void LoadObjFiles();
};