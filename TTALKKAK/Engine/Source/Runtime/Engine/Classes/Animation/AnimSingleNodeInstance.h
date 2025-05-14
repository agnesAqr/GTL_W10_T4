#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"

class UAnimSequence;
struct FRefSkeletal;

class UAnimSingleNodeInstance : public UAnimInstance
{
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
    UAnimSingleNodeInstance();
    UAnimSingleNodeInstance(const UAnimSingleNodeInstance& Other);

    UObject* Duplicate(UObject* InOuter) override;
    void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    void PostDuplicate() override;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void TriggerAnimNotifies(float PrevTime, float CurrTime) override;
    
private:
    UAnimSequence* AnimationSequence;
    float CurrentTime;
    float PreviousTime;
    bool bLooping;

public:
    UAnimSequence* GetAnimation() const { return AnimationSequence; }
    void SetAnimation(UAnimSequence* AnimSequence);
    void SetAnimation(UAnimSequence* AnimSequence, bool bShouldLoop);

    void SetLooping(bool InValue) { bLooping = InValue; }
    bool IsLooping() const { return bLooping; }

    /** 
     * @brief 재생 위치를 직접 설정하고, 필요하면 노티파이를 트리거합니다.
     * @param InTime        설정할 시간(초)
     * @param bFireNotifies 이 시간 구간의 Notify 이벤트를 발생시킬지 여부
     */
    void SetPosition(float InTime, bool bFireNotifies = false);
};