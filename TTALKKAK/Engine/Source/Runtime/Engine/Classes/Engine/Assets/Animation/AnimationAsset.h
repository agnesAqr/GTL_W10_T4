#pragma once
#include "Engine/Assets/UAsset.h"
#include "Math/FrameRate.h"
#include "Classes/Engine/Assets/Animation/AnimTypes.h"

struct FRefSkeletal;
struct FRawAnimSequenceTrack;
class UAnimDataModel;

class UAnimationAsset : public UAsset
{
    DECLARE_CLASS(UAnimationAsset, UAsset)
public:
    UAnimationAsset();
    ~UAnimationAsset() override;
    
    //UAnimDataModel* GetDataMode() const;

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;

    bool InitializeFromData(std::shared_ptr<FAssetData> Data) override;
    void Unload() override;
    EAssetType GetAssetType() const override;

    void SetSkeletal(const FRefSkeletal* InRefSkeletal);
    const FRefSkeletal* GetReferenceSkeleton() const
    {
        return RefSkeletal;
    }
protected:
    /** 이 애셋을 플레이할 수 있는 스켈레톤에 대한 포인터 */
    const FRefSkeletal* RefSkeletal;    
};

class UAnimSequenceBase : public UAnimationAsset
{
    DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)
public:
    UAnimSequenceBase();
    ~UAnimSequenceBase() override;

    UAnimDataModel* GetAnimDataModel() const;
    virtual void SetAnimDataModel(UAnimDataModel* AnimDataModel);
    bool InitializeFromData(std::shared_ptr<FAssetData> Data) override;
    void Unload() override;

    virtual float GetPlayLength() const;

    virtual int32 GetNumberOfFrames() const;

    virtual int32 GetNumberOfSampledKeys() const;

    virtual FFrameRate GetSamplingFrameRate() const;

    virtual int32 GetFrameAtTime(const float Time) const;

    virtual float GetTimeAtFrame(const int32 Frame) const;

    // TickAssetPlayer는 주어진 애니메이션 인스턴스( Instance )를 한 프레임만큼 진행
    // 델타 타임에 맞춰 현재 재생 위치를 업데이트하고 포즈를 샘플링
    // 애니메이션 노티파이 이벤트( NotifyQueue )에 필요한 알림을 생성
    // TODO : 필요한 매개변수 설정 및 함수 구현
    virtual void TickAssetPlayer();

    virtual void PopulateModel();

    PROPERTY(float, RateScale);
protected:
    /** 재생 속도가 1.0일 때 이 AnimSequence의 길이(초) */
    float SequenceLength;
    
    /** 이 애니메이션의 전체 재생 속도를 조정하기 위한 값 */
    float RateScale = 1.f;
    bool bLoop;

    // 소스 애니메이션 데이터
    UAnimDataModel* DataModel;
};

class UAnimSequence : public UAnimSequenceBase
{
    DECLARE_CLASS(UAnimSequence, UAnimSequenceBase)
public:
    UAnimSequence();
    ~UAnimSequence() override;

public:
    float ImportFileFrameRate;
    int32 ImportResampleFrameRate;
    
    bool InitializeFromData(std::shared_ptr<FAssetData> Data) override;
    void Unload() override;

    void SetAnimDataModel(UAnimDataModel* AnimDataModel) override;
    float GetPlayLength() const override;
    int32 GetNumberOfFrames() const override;
    int32 GetNumberOfSampledKeys() const override;
    FFrameRate GetSamplingFrameRate() const override;
    int32 GetFrameAtTime(const float Time) const override;
    float GetTimeAtFrame(const int32 Frame) const override;
    void TickAssetPlayer() override;
    void PopulateModel() override;

    //void SamplePoseAtTime(float Time, const FRefSkeletal* Skeleton, FPoseData& OutPose, const uint32 AnimCount) const;
    void SamplePoseAtTime(float Time, const FRefSkeletal* Skeleton, FPoseData& OutPose) const;

protected:
    /** 개별 애니메이션 트랙에 예상되는 키의 수를 포함합니다. */
    int32 NumFrames;
    /** 개별 (비균등) 애니메이션 트랙에 예상되는 키의 수를 포함합니다. */
    int32 NumberOfSampledKeys;
    /** 소스 애니메이션이 샘플링되는 프레임 속도입니다. */
    FFrameRate SamplingFrameRate;

    // 애니메이션 시퀀스의 원시 키프레임 데이터를 저장하는 배열
    TArray<FRawAnimSequenceTrack> RawAnimationData;

    // 이게 뭐하는 지 모르겠음
    TArray<FName> AnimationTrackNames;

    // “만약 RefPoseType 이 AnimFrame 일 때, 애디티브(additive) 블렌딩을 위해 이 프레임을 기준(참조) 포즈로 사용하라”
    // 기준 포즈를 어떤 값으로 잡을지를 결정하는 조건 주석.
    int32 RefFrameIndex;
};

inline float UAnimSequence::GetPlayLength() const
{
    return UAnimSequenceBase::GetPlayLength();
}

inline int32 UAnimSequence::GetNumberOfFrames() const
{
    return UAnimSequenceBase::GetNumberOfFrames();
}

inline int32 UAnimSequence::GetNumberOfSampledKeys() const
{
    return NumberOfSampledKeys;
}

inline FFrameRate UAnimSequence::GetSamplingFrameRate() const
{
    return UAnimSequenceBase::GetSamplingFrameRate();
}

inline int32 UAnimSequence::GetFrameAtTime(const float Time) const
{
    return UAnimSequenceBase::GetFrameAtTime(Time);
}

inline float UAnimSequence::GetTimeAtFrame(const int32 Frame) const
{
    return UAnimSequenceBase::GetTimeAtFrame(Frame);
}

inline void UAnimSequence::TickAssetPlayer()
{
    UAnimSequenceBase::TickAssetPlayer();
}

static void FindKeyIndices(const TArray<float>& KeyTimes, float CurrentTime, int32& OutKeyIndex1, int32& OutKeyIndex2);
