#pragma once
#include "AnimTypes.h"
#include "Container/String.h"
#include "UObject/NameTypes.h"

struct FBoneAnimationTrack
{
    /** 애니메이션 본 데이터를 나타내는 내부 저장 데이터 */
    FRawAnimSequenceTrack InternalTrackData;

    /** 대상 USkeleton 내에서 이 트랙이 대응하는 본의 인덱스 */
    int32 BoneTreeIndex = INDEX_NONE;

    /** 이 트랙이 대응하는 본의 이름 */
    FName Name;
};
// struct FAnimationCurveData
// {
//     /** Float-based animation curves */
//     TArray<FFloatCurve>	FloatCurves;
//
//     /** FTransform-based animation curves, used for animation layer editing */
//     TArray<FTransformCurve>	TransformCurves;
// };

class IAnimationDataModel
{
public:
    virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const = 0;
};