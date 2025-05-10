#include "AnimDataModel.h"

UAnimDataModel::UAnimDataModel()
    : PlayLength(0)
    , NumberOfFrames(0)
    , NumberOfKeys(0)
{
}

UAnimDataModel::~UAnimDataModel()
{
}

const TArray<FBoneAnimationTrack>& UAnimDataModel::GetBoneAnimationTracks() const
{
    return BoneAnimationTracks;
}
