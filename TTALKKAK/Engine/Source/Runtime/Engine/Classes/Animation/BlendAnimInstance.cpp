#include "BlendAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h" // AnimSequence
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"
#include "FBXLoader.h"

UBlendAnimInstance::UBlendAnimInstance()
    : AnimationA(nullptr)
    , AnimationB(nullptr)
    , BlendAlpha(1.f)
    , FinalBlendedPose(FPoseData())
    , CurrentTimeA(0.f)
    , CurrentTimeB(0.f)
{
    // constructor에서 하는게 부적절할 수도 있음.
    AnimationA = FBXLoader::GetAnimationSequence("FBX/Walking.fbx");
    AnimationB = FBXLoader::GetAnimationSequence("FBX/Sneak_Walking.fbx");
}

UBlendAnimInstance::UBlendAnimInstance(const UBlendAnimInstance& Other)
    : AnimationA(Other.AnimationA)
    , AnimationB(Other.AnimationB)
    , BlendAlpha(Other.BlendAlpha)
    , FinalBlendedPose(Other.FinalBlendedPose)
    , CurrentTimeA(Other.CurrentTimeA)
    , CurrentTimeB(Other.CurrentTimeB)
{
}

UObject* UBlendAnimInstance::Duplicate(UObject* InOuter)
{
    UBlendAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UBlendAnimInstance>(this, InOuter);
    NewAnimInstance->DuplicateSubObjects(this, InOuter);
    NewAnimInstance->PostDuplicate();
    return NewAnimInstance;
}

void UBlendAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    UBlendAnimInstance* Origin = Cast<UBlendAnimInstance>(Source);
}

void UBlendAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UBlendAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    //AnimationA=
    //AnimationB;

    SetBlendAlpha(1.f);
}

void UBlendAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 참고할 자료는 UAniumSequenceBase의 TickAssetPlayer로 생성은 해둔 상태인 것 같은데
    
    // 이걸로 반복문 돌려가지고 여차저차
    // 빼오는거는 initialize에서 하는 게 괜찮아 보이긴 하는데 여기서 하는게 맞을지 모르겠음. 다른 곳에서 하고 가져오는게 좋아보임
    //TArray<FBoneAnimationTrack> BoneAnimTracks_A = AnimationA->GetAnimDataModel()->GetBoneAnimationTracks();


    if (AnimationA && AnimationB && TargetSkeleton)
    {
        if (AnimationA->GetPlayLength() > 0.f)
        {
            CurrentTimeA += DeltaSeconds * AnimationA->GetRateScale(); // RateScale이 있다면 곱해줌
            CurrentTimeA = FMath::Fmod(CurrentTimeA, AnimationA->GetPlayLength());
        }
        if (AnimationB->GetPlayLength() > 0.f)
        {
            CurrentTimeB += DeltaSeconds * AnimationB->GetRateScale();
            CurrentTimeB = FMath::Fmod(CurrentTimeB, AnimationB->GetPlayLength());
        }

        FPoseData PoseDataA;
        FPoseData PoseDataB;

        // TargetSkeleton에서 뼈 개수를 가져와서 Reset 호출해야하는데 일단 지금 본 갯수 빼고 했음. 문제 생기면 이게 문제일 확률이 매우 높음.
        PoseDataA.Reset();
        PoseDataB.Reset();

        // FIX-ME
        AnimationA->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, PoseDataA);
        AnimationB->SamplePoseAtTime(CurrentTimeB, TargetSkeleton, PoseDataB);

        AnimationUtils::BlendPoses(PoseDataA, PoseDataB, BlendAlpha, FinalBlendedPose);

    }
}
