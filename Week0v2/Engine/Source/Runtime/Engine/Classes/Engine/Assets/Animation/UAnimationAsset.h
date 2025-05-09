#pragma once
#include "Engine/Assets/UAsset.h"
#include "Math/MathUtility.h"

class UAnimDataModel;

class UAnimationAsset : public UAsset
{
    DECLARE_CLASS(UAnimationAsset, UAsset)
public:
    UAnimDataModel* GetDataMode() const;
};

class UAnimSequenceBase : public UAnimationAsset
{
    
};

class UAnimSequence : public UAnimSequenceBase
{
    
};
