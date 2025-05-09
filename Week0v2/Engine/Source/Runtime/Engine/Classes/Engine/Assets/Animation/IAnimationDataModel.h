#pragma once
#include "AnimTypes.h"
#include "Container/String.h"
#include "UObject/NameTypes.h"

struct FBoneAnimationTrack
{
    /** Internally stored data representing the animation bone data */
    FRawAnimSequenceTrack InternalTrackData;

    /** Index corresponding to the bone this track corresponds to within the target USkeleton */
    int32 BoneTreeIndex = INDEX_NONE;

    /** Name of the bone this track corresponds to */
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