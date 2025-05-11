#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"
#include "Math/Vector4.h"

class alignas(16) FTransform
{
public:
    FVector Translation;
    FVector Scale3D;
    FQuat Rotation;

public:
    FTransform()
        : Rotation(FQuat::FQuat()), Translation(FVector::ZeroVector), Scale3D(FVector::OneVector) // FQuat, Vector에 Identity, ZeroVector, OneVector 필요
    {
    }

    /** 특정 값으로 초기화하는 생성자 */
    explicit FTransform(const FQuat& InRotation, const FVector& InTranslation = FVector::ZeroVector, const FVector& InScale3D = FVector::OneVector)
        : Rotation(InRotation), Translation(InTranslation), Scale3D(InScale3D)
    {
        // Rotation 정규화 보장
        Rotation.Normalize();
    }

    /** 위치만으로 초기화 */
    explicit FTransform(const FVector& InTranslation)
        : Rotation(FQuat::FQuat()), Translation(InTranslation), Scale3D(FVector::OneVector)
    {
    }

    /** 행렬로부터 생성 (주의: Shear는 손실될 수 있음) */
    explicit FTransform(const FMatrix& InMatrix);

    // --- 단위 변환 ---
    static const FTransform& Identity()
    {
        static const FTransform IdentityTransform;
        return IdentityTransform;
    }

    // --- 행렬 변환 ---

    /** FTransform을 4x4 FMatrix로 변환합니다. */
    FMatrix ToMatrix() const;

    /** FTransform을 위치 이동이 없는 4x4 FMatrix로 변환합니다 (주로 방향 변환용). */
    FMatrix ToMatrixNoScale() const;


    // --- 변환 결합 (A * B: B를 먼저 적용하고 A를 적용) ---
    FTransform operator*(const FTransform& Other) const;
    FTransform& operator*=(const FTransform& Other);
    FTransform& operator=(const FTransform& Other);

    // --- 역변환 ---
    FTransform Inverse() const;

    // --- 점/벡터 변환 ---

    /** 로컬 공간 위치를 월드 공간 위치로 변환합니다. */
    FVector TransformPosition(const FVector& V) const;

    /** 월드 공간 위치를 로컬 공간 위치로 변환합니다 (역변환). */
    FVector InverseTransformPosition(const FVector& V) const;

    /** 로컬 공간 방향 벡터를 월드 공간 방향 벡터로 변환합니다 (위치 무시). */
    FVector TransformVector(const FVector& V) const;

    /** 월드 공간 방향 벡터를 로컬 공간 방향 벡터로 변환합니다 (역변환, 위치 무시). */
    FVector InverseTransformVector(const FVector& V) const;

    // --- Getters / Setters ---
    const FQuat& GetRotation() const { return Rotation; }
    const FVector& GetTranslation() const { return Translation; }
    const FVector& GetScale3D() const { return Scale3D; }
    FVector GetLocation() const { return Translation; } // UE 스타일 별칭

    void SetRotation(const FQuat& NewRotation) { Rotation = NewRotation; Rotation.Normalize(); }
    void SetTranslation(const FVector& NewTranslation) { Translation = NewTranslation; }
    void SetScale3D(const FVector& NewScale3D) { Scale3D = NewScale3D; }
    void SetLocation(const FVector& NewLocation) { Translation = NewLocation; } // UE 스타일 별칭

    /** 회전만 설정하고 나머지는 단위 변환으로 설정합니다. */
    void SetRotationOnly(const FQuat& NewRotation);


    // --- 유틸리티 ---

    /** 회전 쿼터니언을 정규화합니다. */
    void NormalizeRotation();

    /** 두 FTransform 사이를 선형 보간합니다 (Translation, Scale3D) */
    static FTransform Lerp(const FTransform& A, const FTransform& B, float Alpha);

    /** 두 FTransform 사이를 구면 선형 보간합니다 (Rotation - Slerp 사용). Translation/Scale은 Lerp. */
    static FTransform Slerp(const FTransform& A, const FTransform& B, float Alpha);


    // --- 내부 사용 함수 ---
private:
    /**
     * 행렬에서 스케일과 회전을 분리합니다.
     * @param Matrix 입력 행렬 (반드시 직교 행렬일 필요는 없음)
     * @param OutScale 추출된 스케일
     * @param OutRotation 추출된 회전 쿼터니언
     */
    static void GetScaleRotationFromMatrix(const FMatrix& Matrix, FVector& OutScale, FQuat& OutRotation);

};
