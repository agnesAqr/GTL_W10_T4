#include "CustomAnimInstance.h"
#include "CoreUObject/UObject/Casts.h"
#include "Classes/Engine/Assets/Animation/AnimationAsset.h" // AnimSequence
#include "Classes/Engine/Assets/Animation/AnimDataModel.h"

UCustomAnimInstance::UCustomAnimInstance()
{
    AnimationA = nullptr;
    AnimationB = nullptr;
    BlendAlpha = 1.f;
    FinalBlendedPose = FPoseData();
}

UCustomAnimInstance::UCustomAnimInstance(const UCustomAnimInstance& Other)
    : AnimationA(Other.AnimationA)
    , AnimationB(Other.AnimationB)
    , BlendAlpha(Other.BlendAlpha)
    , FinalBlendedPose(Other.FinalBlendedPose)
{
}

UObject* UCustomAnimInstance::Duplicate(UObject* InOuter)
{
    UCustomAnimInstance* NewAnimInstance = FObjectFactory::ConstructObjectFrom<UCustomAnimInstance>(this, InOuter);
    NewAnimInstance->DuplicateSubObjects(this, InOuter);
    NewAnimInstance->PostDuplicate();
    return NewAnimInstance;
}

void UCustomAnimInstance::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    Super::DuplicateSubObjects(Source, InOuter);
    UCustomAnimInstance* Origin = Cast<UCustomAnimInstance>(Source);
}

void UCustomAnimInstance::PostDuplicate()
{
    Super::PostDuplicate();
}

void UCustomAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    // 애님 시퀀스 가져오는 함수 호출
    //AnimationA;
    //AnimationB;

    SetBlendAlpha(1.f);
}

void UCustomAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 여기에 애니메이션 블렌딩하는 부분을 추가할 예정

    // 1. 현재 deltatime에 대한 anim sequence의 값을 가져오고
    // 2. deltatime을 애니메이션 프레임에 맞게 수정하고
    // 3. 블렌딩 함수를 구현할 것.

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

        FPoseData PoseA_Data;
        FPoseData PoseB_Data;

        // TargetSkeleton에서 뼈 개수를 가져와서 Reset 호출해야하는데 일단 지금 본 갯수 빼고 했음. 문제 생기면 이게 문제일 확률이 매우 높음.
        PoseA_Data.Reset();
        PoseB_Data.Reset();

        // FIX-ME
        //AnimationA->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, PoseA_Data, 0);
        //AnimationB->SamplePoseAtTime(CurrentTimeB, TargetSkeleton, PoseB_Data, 1);

        AnimationUtils::BlendPoses(PoseA_Data, PoseB_Data, BlendAlpha, FinalBlendedPose);

    }
    else if (AnimationA && TargetSkeleton) // AnimA만 있을 경우
    {
        if (AnimationA->GetPlayLength() > 0.f)
        {
            CurrentTimeA += DeltaSeconds * AnimationA->GetRateScale();
            CurrentTimeA = FMath::Fmod(CurrentTimeA, AnimationA->GetPlayLength());
        }
        //AnimationA->SamplePoseAtTime(CurrentTimeA, TargetSkeleton, FinalBlendedPose, 0);
    }
}
