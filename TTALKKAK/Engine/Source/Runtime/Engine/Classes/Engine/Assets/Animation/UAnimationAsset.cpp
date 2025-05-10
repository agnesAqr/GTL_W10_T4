#include "UAnimationAsset.h"

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

void UAnimationAsset::SetSkeletal(FRefSkeletal* InRefSkeletal)
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
    
    for (auto BoneAnimTrack : AnimDataModel->BoneAnimationTracks)
    {
        RawAnimationData.Add(BoneAnimTrack.InternalTrackData);
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
