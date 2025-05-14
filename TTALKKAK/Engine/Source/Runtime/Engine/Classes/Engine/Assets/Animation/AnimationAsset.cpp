#include "AnimationAsset.h"

#include "AnimDataModel.h"
#include "FBXLoader.h"
#include "UObject/Casts.h"

UAnimationAsset::UAnimationAsset()
    : RefSkeletal(nullptr)
{
}

UAnimationAsset::~UAnimationAsset()
{
}


UObject* UAnimationAsset::Duplicate(UObject* InOuter)
{
    return UAsset::Duplicate(InOuter);
}

void UAnimationAsset::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UAsset::DuplicateSubObjects(Source, InOuter);
}


// UAnimDataModel* UAnimationAsset::GetDataMode() const
// {
//     return 
// }

bool UAnimationAsset::InitializeFromData(std::shared_ptr<FAssetData> Data)
{
    return true;
}

void UAnimationAsset::Unload()
{
}

EAssetType UAnimationAsset::GetAssetType() const
{
    return EAssetType::Animation;
}

void UAnimationAsset::SetSkeletal(const FRefSkeletal* InRefSkeletal)
{
    RefSkeletal = InRefSkeletal;
}

UAnimSequenceBase::UAnimSequenceBase()
{
}

UAnimSequenceBase::~UAnimSequenceBase()
{
}

UAnimDataModel* UAnimSequenceBase::GetAnimDataModel() const
{
    return DataModel;
}

void UAnimSequenceBase::SetAnimDataModel(UAnimDataModel* AnimDataModel)
{
    DataModel = AnimDataModel;
}

bool UAnimSequenceBase::InitializeFromData(std::shared_ptr<FAssetData> Data)
{
    return UAnimationAsset::InitializeFromData(Data);
}

void UAnimSequenceBase::Unload()
{
    UAnimationAsset::Unload();
}

float UAnimSequenceBase::GetPlayLength() const
{
    return SequenceLength;
}

int32 UAnimSequenceBase::GetNumberOfFrames() const
{
    return GetNumberOfSampledKeys();
}

int32 UAnimSequenceBase::GetNumberOfSampledKeys() const
{
    return GetSamplingFrameRate().AsFrameTime(GetPlayLength()).RoundToFrame().Value;
}

FFrameRate UAnimSequenceBase::GetSamplingFrameRate() const
{
    // TODO : 단 30 FRame으로 디폴트
    static const FFrameRate DefaultFrameRate = FFrameRate(30,1);
    return DefaultFrameRate;
}

int32 UAnimSequenceBase::GetFrameAtTime(const float Time) const
{
    return FMath::Clamp(GetSamplingFrameRate().AsFrameTime(Time).RoundToFrame().Value, 0, GetNumberOfSampledKeys() - 1);
}

float UAnimSequenceBase::GetTimeAtFrame(const int32 Frame) const
{
    return FMath::Clamp(static_cast<float>(GetSamplingFrameRate().AsSeconds(Frame)), 0.f, GetPlayLength());
}

void UAnimSequenceBase::TickAssetPlayer()
{
}

void UAnimSequenceBase::PopulateModel()
{
}

UAnimSequence::UAnimSequence()
{
}

UAnimSequence::~UAnimSequence()
{
}

bool UAnimSequence::InitializeFromData(std::shared_ptr<FAssetData> Data)
{
    return UAnimSequenceBase::InitializeFromData(Data);
}

void UAnimSequence::Unload()
{
    UAnimSequenceBase::Unload();
}

void UAnimSequence::SetAnimDataModel(UAnimDataModel* AnimDataModel)
{
    UAnimSequenceBase::SetAnimDataModel(AnimDataModel);

    ImportFileFrameRate = AnimDataModel->FrameRate.AsInterval();
    
    NumFrames = AnimDataModel->NumberOfFrames;
    NumberOfSampledKeys = AnimDataModel->NumberOfKeys;
    SamplingFrameRate = AnimDataModel->FrameRate;
    SequenceLength = AnimDataModel->PlayLength;
    
    for (auto BoneAnimTrack : AnimDataModel->BoneAnimationTracks)
    {
        RawAnimationData.Add(BoneAnimTrack.InternalTrackData);
        AnimationTrackNames.Add(BoneAnimTrack.Name);
    }


    RefFrameIndex = 0;
}

void UAnimSequence::PopulateModel()
{
    UAnimSequenceBase::PopulateModel();
    
    // DataModel이 없으면 새로 생성
    if (!DataModel)
    {
        return;
    }

    // 기본 메타정보 설정
    DataModel->FrameRate       = GetSamplingFrameRate();
    DataModel->PlayLength      = SequenceLength;
    DataModel->NumberOfFrames  = GetNumberOfSampledKeys();
    DataModel->NumberOfKeys    = DataModel->NumberOfFrames > 0 ? DataModel->NumberOfFrames : 1;

    // 트랙 채우기
    DataModel->BoneAnimationTracks.Empty();

    // 실제 키프레임 데이터는 UAnimSequence에만 있으므로 캐스트 후 처리
    if (UAnimSequence* Seq = Cast<UAnimSequence>(this))
    {
        const int32 NumTracks = Seq->RawAnimationData.Num();
        for (int32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
        {
            FBoneAnimationTrack NewTrack;
            NewTrack.InternalTrackData = Seq->RawAnimationData[TrackIndex];
            NewTrack.Name              = Seq->AnimationTrackNames.IsValidIndex(TrackIndex)
                                           ? Seq->AnimationTrackNames[TrackIndex]
                                           : TEXT("");
            NewTrack.BoneTreeIndex     = TrackIndex;

            DataModel->BoneAnimationTracks.Add(NewTrack);
        }
    }
}

// FIX-ME: UAnimSequence에 종속되지 않도록 추후 수정 요망
 void UAnimSequence::SamplePoseAtTime(float Time, const FRefSkeletal* Skeleton, FPoseData& OutPose) const
 {
     if (!Skeleton || RawAnimationData.Num() == 0)
     {
         if (Skeleton) OutPose.Reset();
         return;
     }
     OutPose.Reset();

     const int32 NumSkeletonBones = Skeleton->BoneTree.Num();
     OutPose.LocalBoneTransforms.Init(FCompactPoseBone(), NumSkeletonBones); // 모든 본을 항등 변환으로 초기화

     float CurrentTime = FMath::Fmod(Time, SequenceLength);
     if (CurrentTime < 0.0f) CurrentTime += SequenceLength;

     //UE_LOG(LogLevel::Display, "UAnimSequence-Time: %f", Time);
     //UE_LOG(LogLevel::Display, "UAnimSequence-CurrentTime: %f", CurrentTime);

     for (int32 BoneIndex = 0; BoneIndex < NumSkeletonBones; ++BoneIndex)
     {
         FCompactPoseBone& BoneTransform = OutPose.LocalBoneTransforms[BoneIndex];
         // 기본적으로 바인드 포즈 또는 Identity로 설정 (애니메이션 트랙이 없는 뼈를 위해)
         //BoneTransform = Skeleton->GetRefPoseTransform(BoneIndex); // 스켈레톤에서 바인드 포즈 가져오는 함수 필요

         if (RawAnimationData.IsValidIndex(BoneIndex))
         {
             const FRawAnimSequenceTrack& Track = RawAnimationData[BoneIndex];

             if (Track.PosKeys.Num() == 0 && Track.RotKeys.Num() == 0 && Track.ScaleKeys.Num() == 0)
             {
                 // 이 트랙에 키가 없으면 다음 뼈로
                 continue;
             }

             // 1. 현재 시간에 해당하는 키프레임 인덱스 찾기
             //    Track.KeyTimes 배열을 사용하여 CurrentTime 이전 키와 이후 키를 찾습니다.
             //    (간단하게는 선형 검색, 최적화하려면 이진 검색)
             int32 KeyIndex1 = -1;
             int32 KeyIndex2 = -1;

             for (int32 K = 0; K < Track.KeyTimes.Num(); ++K)
             {
                 if (Track.KeyTimes[K] <= CurrentTime)
                 {
                     KeyIndex1 = K;
                 }
                 else
                 {
                     KeyIndex2 = K;
                     break;
                 }
             }

             if (KeyIndex1 == -1) // 시간이 첫 키보다 이전 (또는 키 없음)
             {
                 if (Track.PosKeys.Num() > 0) BoneTransform.Translation = Track.PosKeys[0];
                 if (Track.RotKeys.Num() > 0) BoneTransform.Rotation = Track.RotKeys[0];
                 if (Track.ScaleKeys.Num() > 0) BoneTransform.Scale3D = Track.ScaleKeys[0];
                 continue;
             }
             if (KeyIndex2 == -1) // 시간이 마지막 키 이후 (또는 키 1개)
             {
                 if (Track.PosKeys.Num() > 0) BoneTransform.Translation = Track.PosKeys[KeyIndex1];
                 if (Track.RotKeys.Num() > 0) BoneTransform.Rotation = Track.RotKeys[KeyIndex1];
                 if (Track.ScaleKeys.Num() > 0) BoneTransform.Scale3D = Track.ScaleKeys[KeyIndex1];
                 continue;
             }

             // 2. 두 키프레임 간의 보간 알파 값 계산
             float Time1 = Track.KeyTimes[KeyIndex1];
             float Time2 = Track.KeyTimes[KeyIndex2];
             float Alpha = (Time1 == Time2) ? 0.0f : (CurrentTime - Time1) / (Time2 - Time1);
             Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

             // 3. 위치, 회전, 스케일 보간
             // 위치 (Lerp)
             if (Track.PosKeys.IsValidIndex(KeyIndex1) && Track.PosKeys.IsValidIndex(KeyIndex2))
             {
                 BoneTransform.Translation = FMath::Lerp(Track.PosKeys[KeyIndex1], Track.PosKeys[KeyIndex2], Alpha);
             }
             else if (Track.PosKeys.IsValidIndex(KeyIndex1))
             {
                 BoneTransform.Translation = Track.PosKeys[KeyIndex1];
             }


             // 회전 (Slerp 또는 Nlerp)
             if (Track.RotKeys.IsValidIndex(KeyIndex1) && Track.RotKeys.IsValidIndex(KeyIndex2))
             {
                 BoneTransform.Rotation = FQuat::Slerp(Track.RotKeys[KeyIndex1], Track.RotKeys[KeyIndex2], Alpha);
                 //BoneTransform.Rotation.Normalize(); // Slerp 후 정규화 권장
             }
             else if (Track.RotKeys.IsValidIndex(KeyIndex1))
             {
                 BoneTransform.Rotation = Track.RotKeys[KeyIndex1];
             }

             // 스케일 (Lerp)
             if (Track.ScaleKeys.IsValidIndex(KeyIndex1) && Track.ScaleKeys.IsValidIndex(KeyIndex2))
             {
                 BoneTransform.Scale3D = FMath::Lerp(Track.ScaleKeys[KeyIndex1], Track.ScaleKeys[KeyIndex2], Alpha);
             }
             else if (Track.ScaleKeys.IsValidIndex(KeyIndex1))
             {
                 BoneTransform.Scale3D = Track.ScaleKeys[KeyIndex1];
             }
             
             OutPose.LocalBoneTransformMap[AnimationTrackNames[BoneIndex]] = BoneTransform;
         }
         // else : 애니메이션 트랙이 없는 뼈는 기본 바인드 포즈(또는 Identity)를 유지합니다.
         // 이 부분은 OutPose.Reset()에서 FCompactPoseBone의 기본 생성자에 의해 Identity로 초기화되거나,
         // 위에서 Skeleton->GetRefPoseTransform(BoneIndex) 등으로 명시적으로 바인드 포즈를 넣어줘야 합니다.
         // 현재 코드는 Identity로 남아있게 됩니다.
     }
 }