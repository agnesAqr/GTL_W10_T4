#include "Transform.h"


FTransform::FTransform(const FMatrix& InMatrix)
{
    // 행렬에서 스케일과 회전을 추출
    GetScaleRotationFromMatrix(InMatrix, Scale3D, Rotation);

    // 위치(Translation) 추출
    Translation = FVector(InMatrix.M[3][0], InMatrix.M[3][1], InMatrix.M[3][2]);
}

FMatrix FTransform::ToMatrix() const
{
    // 스케일 * 회전 * 위치 순서로 결합
    FMatrix ScaleMatrix = FMatrix::GetScaleMatrix(Scale3D);
    FMatrix RotationMatrix = FMatrix::GetRotationMatrix(Rotation); // FMatrix에 FQuat->Matrix 변환 함수 필요
    FMatrix TranslationMatrix = FMatrix::GetTranslationMatrix(Translation);

    // 적용 순서: Scale -> Rotate -> Translate
    // 행렬 곱셈 순서: Translate * Rotate * Scale (오른쪽부터 적용되므로)
    return ScaleMatrix * RotationMatrix * TranslationMatrix; // FMatrix 곱셈 순서 확인 필요! UE는 M1 * M2 = M2 적용 후 M1 적용
    // 만약 FMatrix 구현이 M1*M2 = M1 적용 후 M2 적용이면 순서 반대
    // 제공된 코드는 SSE::VectorMatrixMultiply(&Result, this, &Other); -> this 적용 후 other 적용 순서로 보임
    // 따라서 UE와 같은 순서 (Translate * Rotate * Scale) 를 사용하려면
    // return TranslationMatrix * RotationMatrix * ScaleMatrix;
}

FMatrix FTransform::ToMatrixNoScale() const
{
    // 회전 * 위치 순서로 결합 (스케일 무시)
    FMatrix RotationMatrix = FMatrix::GetRotationMatrix(Rotation);
    FMatrix TranslationMatrix = FMatrix::GetTranslationMatrix(Translation);

    // 적용 순서: Rotate -> Translate
    // 행렬 곱셈 순서: Translate * Rotate
    // return TranslationMatrix * RotationMatrix; // ToMatrix와 동일한 이유로 순서 가정
    return RotationMatrix * TranslationMatrix;
}

// 변환 결합 (A * B)
FTransform FTransform::operator*(const FTransform& Other) const
{
    FTransform Result;

    // UE의 FTransform::Multiply 코드를 참고하거나 단순화된 접근 사용
    // 단순화 접근:
    // Result.Scale3D = Scale3D * Other.Scale3D; // 주의: 비균등 스케일+회전 시 부정확 가능성
    // Result.Rotation = Rotation * Other.Rotation;
    // Result.Translation = Rotation.RotateVector(Other.Translation * Scale3D) + Translation;
    // Result.NormalizeRotation();

    // 더 정확한 접근 (행렬 변환 활용 - Shear 처리 불가):
    FMatrix M1 = this->ToMatrix();
    FMatrix M2 = Other.ToMatrix();
    FMatrix ResultMatrix = M1 * M2; // FMatrix 곱셈 순서 중요!
    Result = FTransform(ResultMatrix); // 행렬 -> FTransform 변환

    return Result;
}

FTransform& FTransform::operator*=(const FTransform& Other)
{
    *this = *this * Other;
    return *this;
}

// 역변환
FTransform FTransform::Inverse() const
{
    FQuat InvRotation = Rotation.Inverse();
    // 0으로 나누는 것 방지
    FVector InvScale3D(
        (std::abs(Scale3D.X) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.X : 0.0f,
        (std::abs(Scale3D.Y) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.Y : 0.0f,
        (std::abs(Scale3D.Z) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.Z : 0.0f
    );
    // 역 위치 계산: 원본 위치를 역 스케일링하고 역 회전
    FVector InvTranslation = InvRotation.RotateVector(-Translation * InvScale3D); // Vector 컴포넌트 곱셈 필요

    return FTransform(InvRotation, InvTranslation, InvScale3D);
}

// 로컬 -> 월드 위치 변환
FVector FTransform::TransformPosition(const FVector& V) const
{
    // 스케일 -> 회전 -> 위치 순서 적용
    return Rotation.RotateVector(V * Scale3D) + Translation; // Vector 컴포넌트 곱셈 필요
}

// 월드 -> 로컬 위치 변환
FVector FTransform::InverseTransformPosition(const FVector& V) const
{
    // 위치 이동 제거 -> 역회전 -> 역스케일
    return (Rotation.Inverse().RotateVector(V - Translation)) * // Vector 컴포넌트 곱셈 필요
        FVector((std::abs(Scale3D.X) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.X : 0.0f,
            (std::abs(Scale3D.Y) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.Y : 0.0f,
            (std::abs(Scale3D.Z) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.Z : 0.0f);

    // 또는 간단히: return Inverse().TransformPosition(V); (효율은 약간 떨어짐)
}

// 로컬 -> 월드 방향 변환
FVector FTransform::TransformVector(const FVector& V) const
{
    // 스케일 -> 회전 (위치 무시)
    return Rotation.RotateVector(V * Scale3D); // Vector 컴포넌트 곱셈 필요
}

// 월드 -> 로컬 방향 변환
FVector FTransform::InverseTransformVector(const FVector& V) const
{
    // 역회전 -> 역스케일 (위치 무시)
    return (Rotation.Inverse().RotateVector(V)) * // Vector 컴포넌트 곱셈 필요
        FVector((std::abs(Scale3D.X) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.X : 0.0f,
            (std::abs(Scale3D.Y) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.Y : 0.0f,
            (std::abs(Scale3D.Z) > std::numeric_limits<float>::epsilon()) ? 1.0f / Scale3D.Z : 0.0f);

    // 또는 간단히: return Inverse().TransformVector(V);
}

void FTransform::SetRotationOnly(const FQuat& NewRotation)
{
    Rotation = NewRotation;
    Rotation.Normalize();
    Translation = FVector::ZeroVector;
    Scale3D = FVector::OneVector;
}


void FTransform::NormalizeRotation()
{
    Rotation.Normalize();
}

// 선형 보간 (Lerp)
FTransform FTransform::Lerp(const FTransform& A, const FTransform& B, float Alpha)
{
    FTransform Result;
    Result.Translation = FMath::Lerp(A.Translation, B.Translation, Alpha); // FMath::Lerp 필요
    Result.Scale3D = FMath::Lerp(A.Scale3D, B.Scale3D, Alpha);
    Result.Rotation = FQuat::Slerp(A.Rotation, B.Rotation, Alpha); // FQuat::Slerp 필요
    // 결과 Rotation 정규화는 Slerp 내부에서 처리되거나 여기서 한번 더 해도 됨
    // Result.NormalizeRotation();
    return Result;
}

// 구면 선형 보간 (Slerp) - Lerp와 동일하게 동작 (UE 관례)
FTransform FTransform::Slerp(const FTransform& A, const FTransform& B, float Alpha)
{
    // 일반적으로 FTransform의 Slerp는 Rotation만 Slerp하고 나머지는 Lerp함
    return Lerp(A, B, Alpha);
}


// 행렬에서 스케일과 회전 분리
void FTransform::GetScaleRotationFromMatrix(const FMatrix& Matrix, FVector& OutScale, FQuat& OutRotation)
{
    // X, Y, Z 축 벡터 추출 (행렬의 첫 3개 열)
    // FMatrix가 Row-Major인지 Column-Major인지 확인 필요!
    // 제공된 코드는 M[row][column] 형태이므로 Row-Major로 보임.
    // 따라서 열(Column)은 M[0][i], M[1][i], M[2][i] 형태.
    FVector AxisX(Matrix.M[0][0], Matrix.M[1][0], Matrix.M[2][0]);
    FVector AxisY(Matrix.M[0][1], Matrix.M[1][1], Matrix.M[2][1]);
    FVector AxisZ(Matrix.M[0][2], Matrix.M[1][2], Matrix.M[2][2]);

    // 각 축 벡터의 길이로부터 스케일 추출
    OutScale.X = AxisX.Magnitude();
    OutScale.Y = AxisY.Magnitude();
    OutScale.Z = AxisZ.Magnitude();

    // 스케일이 0에 가까운 경우 처리 (나눗셈 오류 방지)
    if (OutScale.X < std::numeric_limits<float>::epsilon() ||
        OutScale.Y < std::numeric_limits<float>::epsilon() ||
        OutScale.Z < std::numeric_limits<float>::epsilon())
    {
        // 스케일이 거의 0이면 회전을 단위 쿼터니언으로 설정
        OutRotation = FQuat::FQuat();
        return;
    }

    // 스케일을 제거하여 순수 회전 행렬 만들기 (정규화된 축 벡터 사용)
    // 주의: 비균등 스케일이 적용된 경우, 이 축들은 더 이상 직교하지 않을 수 있음 (Shear)
    // FTransform은 Shear를 표현하지 않으므로, 가장 가까운 직교 행렬로 근사함.
    FMatrix RotationMatrix = FMatrix::Identity; // 초기화

    // 열 벡터 정규화 (만약 Vector::Normalize()가 원본 변경 시 복사 후 사용)
    FVector NormAxisX = AxisX / OutScale.X;
    FVector NormAxisY = AxisY / OutScale.Y;
    FVector NormAxisZ = AxisZ / OutScale.Z;

    // 정규화된 열 벡터로 회전 행렬 구성 (Row-Major 기준)
    RotationMatrix.M[0][0] = NormAxisX.X; RotationMatrix.M[1][0] = NormAxisX.Y; RotationMatrix.M[2][0] = NormAxisX.Z;
    RotationMatrix.M[0][1] = NormAxisY.X; RotationMatrix.M[1][1] = NormAxisY.Y; RotationMatrix.M[2][1] = NormAxisY.Z;
    RotationMatrix.M[0][2] = NormAxisZ.X; RotationMatrix.M[1][2] = NormAxisZ.Y; RotationMatrix.M[2][2] = NormAxisZ.Z;


    // 회전 행렬을 쿼터니언으로 변환
    OutRotation = FQuat(RotationMatrix); // FQuat에 Matrix->Quat 변환 생성자 필요
    // 또는 FMatrix::ToQuat() 활용: OutRotation = Matrix.ToQuat(); (스케일 제거 후?)
    // 제공된 Matrix.ToQuat()를 사용하려면 스케일이 제거된 RotationMatrix에서 호출해야 할 가능성이 높음.
    // OutRotation = RotationMatrix.ToQuat();

    OutRotation.Normalize(); // 최종 정규화
}
