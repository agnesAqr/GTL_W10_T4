#pragma once

#include "APawn.h"

class USkeletalMeshComponent;
struct FAnimNotifyEvent;

class ACharacter : public APawn
{
    DECLARE_CLASS(ACharacter, APawn)

public:
    ACharacter();
    ACharacter(const ACharacter& Other);
    virtual ~ACharacter();

    // Actor
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Destroyed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool Destroy() override;

    // Duplicate
    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void DuplicateSubObjects(const UObject* Source, UObject* InOuter) override;
    virtual void PostDuplicate() override;

    /** Anim Notify 전달을 받는 함수 */
    virtual void HandleAnimNotify(const FAnimNotifyEvent& Notify) override;

protected:
    /** 캐릭터 전용 움직임 컴포넌트 */
    //UCharacterMovementComponent*  CharacterMovement   = nullptr;
    /** 스켈레탈 메시 컴포넌트 */
    USkeletalMeshComponent*       SkeletalMeshComponent = nullptr;

    void TestNotify();
};
