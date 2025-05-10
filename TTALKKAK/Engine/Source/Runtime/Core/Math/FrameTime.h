#pragma once
#include "MathUtility.h"
#include "HAL/PlatformType.h"

/// 정수 프레임 번호와 서브프레임을 합쳐 고정밀 타임코드를 표현하는 구조체
struct FFrameNumber
{
    int32 Value;
    explicit FFrameNumber(const int32 InValue = 0)
        : Value(InValue) {}

    FFrameNumber& operator+=(FFrameNumber RHS)                    { Value += RHS.Value; return *this; }
	FFrameNumber& operator-=(FFrameNumber RHS)                    { Value -= RHS.Value; return *this; }
	FFrameNumber& operator%=(FFrameNumber RHS)                    { Value %= RHS.Value; return *this; }

	FFrameNumber& operator++()                                    { ++Value; return *this; }
	FFrameNumber& operator--()                                    { --Value; return *this; }

	FFrameNumber operator++(int32)                                { FFrameNumber Ret = *this; ++Value; return Ret; }
	FFrameNumber operator--(int32)                                { FFrameNumber Ret = *this; --Value; return Ret; }

	friend bool operator==(FFrameNumber A, FFrameNumber B)        { return A.Value == B.Value; }
	friend bool operator!=(FFrameNumber A, FFrameNumber B)        { return A.Value != B.Value; }

	friend bool operator< (FFrameNumber A, FFrameNumber B)        { return A.Value < B.Value; }
	friend bool operator> (FFrameNumber A, FFrameNumber B)        { return A.Value > B.Value; }
	friend bool operator<=(FFrameNumber A, FFrameNumber B)        { return A.Value <= B.Value; }
	friend bool operator>=(FFrameNumber A, FFrameNumber B)        { return A.Value >= B.Value; }

	friend FFrameNumber operator+(FFrameNumber A, FFrameNumber B) { return FFrameNumber(A.Value + B.Value); }
	friend FFrameNumber operator-(FFrameNumber A, FFrameNumber B) { return FFrameNumber(A.Value - B.Value); }
	friend FFrameNumber operator%(FFrameNumber A, FFrameNumber B) { return FFrameNumber(A.Value % B.Value); }

	friend FFrameNumber operator-(FFrameNumber A)                 { return FFrameNumber(-A.Value); }
};

struct FFrameTime
{
    FFrameNumber FrameNumber;  // 전체 프레임 번호
    float        SubFrame;     // 0.0f ~ 1.0f 사이의 서브프레임

    // 생성자
    FFrameTime()
        : FrameNumber(0), SubFrame(0.0f) {}

    /// 정수 프레임만 지정할 때는 SubFrame을 0으로 기본 처리
    FFrameTime(const FFrameNumber InFrame, const float InSubFrame = 0.0f)
        : FrameNumber(InFrame), SubFrame(InSubFrame) {}

    FORCEINLINE FFrameNumber GetFrame() const
    {
        return FrameNumber;
    }

    FORCEINLINE float GetSubFrame() const
    {
        return SubFrame;
    }

    /// 정수 + 서브프레임을 소수(decimal) 형태로 반환 (e.g. 10.25)
    double AsDecimal() const
    {
        return static_cast<double>(FrameNumber.Value) + SubFrame;
    }

    /// 소수(decimal) 형태를 FFrameTime으로 변환
    static FFrameTime FromDecimal(const double InDecimal)
    {
        const int32 WholeFrame = FMath::Floor(InDecimal);
        const float Sub = static_cast<float>(InDecimal - WholeFrame);
        return FFrameTime(FFrameNumber(WholeFrame), Sub);
    }

    /// 소수 부분 무시 (10.75 -> 10)
    FFrameNumber FloorToFrame() const
    {
        return FrameNumber;
    }

    /// 서브프레임이 0이 아니면 다음 프레임으로 올림 (10.00->10, 10.75->11)
    FFrameNumber CeilToFrame() const
    {
        return SubFrame == 0.f ? FrameNumber : FrameNumber + FFrameNumber(1);
    }

    /// 0.5 기준 반올림 (10.49->10, 10.51->11)
    FFrameNumber RoundToFrame() const
    {
        return SubFrame < .5f ? FrameNumber : FrameNumber + FFrameNumber(1);
    }

    // 산술 연산자
    friend FFrameTime operator+(const FFrameTime& A, const FFrameTime& B)
    {
        int32 Frame = A.FrameNumber.Value + B.FrameNumber.Value;
        float   Sub   = A.SubFrame + B.SubFrame;
        if (Sub >= 1.0f)
        {
            Frame += static_cast<int32>(FMath::Floor(Sub));
            Sub    = FMath::Fmod(Sub, 1.0f);
        }
        return FFrameTime(FFrameNumber(Frame), Sub);
    }

    friend FFrameTime operator-(const FFrameTime& A, const FFrameTime& B)
    {
        int32_t Frame = A.FrameNumber.Value - B.FrameNumber.Value;
        float   Sub   = A.SubFrame - B.SubFrame;
        if (Sub < 0.0f)
        {
            Frame -= FMath::CeilToInt(-Sub);
            Sub    = FMath::Fmod(Sub + FMath::CeilToInt(-Sub), 1.0f);
        }
        return FFrameTime(FFrameNumber(Frame), Sub);
    }

    // 스칼라 곱/나눗셈
    friend auto operator*(const FFrameTime& A, double Scalar) -> FFrameTime
    {
        const double Dec = A.AsDecimal() * Scalar;
        return FromDecimal(Dec);
    }

    friend FFrameTime operator/(const FFrameTime& A, const double Scalar)
    {
        const double Dec = A.AsDecimal() / Scalar;
        return FromDecimal(Dec);
    }

    // 모듈로 연산: A % B
    friend FFrameTime operator%(const FFrameTime& A, const FFrameTime& B)
    {
        const double Dec = FMath::Fmod(A.AsDecimal(), B.AsDecimal());
        return FromDecimal(Dec);
    }

    // 비교 연산자
    friend bool operator==(const FFrameTime& A, const FFrameTime& B)
    {
        return A.FrameNumber.Value == B.FrameNumber.Value && A.SubFrame == B.SubFrame;
    }
    friend bool operator!=(const FFrameTime& A, const FFrameTime& B)
    {
        return !(A == B);
    }
    friend bool operator<(const FFrameTime& A, const FFrameTime& B)
    {
        if (A.FrameNumber.Value < B.FrameNumber.Value) return true;
        if (A.FrameNumber.Value > B.FrameNumber.Value) return false;
        return A.SubFrame < B.SubFrame;
    }
    friend bool operator<=(const FFrameTime& A, const FFrameTime& B)
    {
        return (A < B) || (A == B);
    }
    friend bool operator>(const FFrameTime& A, const FFrameTime& B)
    {
        return B < A;
    }
    friend bool operator>=(const FFrameTime& A, const FFrameTime& B)
    {
        return (B < A) || (A == B);
    }
};

