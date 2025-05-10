#pragma once
#include "IAnimationDataModel.h"
#include "Math/FrameRate.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UAnimDataModel : public UObject, public IAnimationDataModel
{
    DECLARE_CLASS(UAnimDataModel, UObject)
public:
    UAnimDataModel();
    ~UAnimDataModel() override;

    /** 모든 개별 본 애니메이션 트랙 */
    TArray<FBoneAnimationTrack> BoneAnimationTracks;
    /** 포함된 애니메이션 데이터의 총 재생 길이 */
    float PlayLength;
    /** 애니메이션 데이터가 샘플링되는 프레임 속도 */
    FFrameRate FrameRate;
    /** 샘플링된 애니메이션 프레임의 총 개수 */
    int32 NumberOfFrames;
    /** 샘플링된 애니메이션 키의 총 개수 */
    int32 NumberOfKeys;
    //FAnimationCurveData CurveData;

    virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const override;
};
