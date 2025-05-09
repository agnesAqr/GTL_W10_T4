#pragma once
#include "IAnimationDataModel.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UAnimDataModel : public UObject, public IAnimationDataModel
{
    DECLARE_CLASS(UAnimDataModel, UObject)
public:
    TArray<FBoneAnimationTrack> BoneAnimationTracks;
    float PlayLength;
    //FFrameRate FrameRate;
    int32 NumberOfFrames;
    int32 NumberOfKeys;
    //FAnimationCurveData CurveData;

    virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const override;
};
