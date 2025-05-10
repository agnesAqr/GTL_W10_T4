#pragma once
#include "FrameTime.h"
#include "HAL/PlatformType.h"

struct FFrameRate
{
    uint32 Numerator;   // 분자: 초당 프레임 수
    uint32 Denominator; // 분모: 기준 단위 (보통 1)

    /// 기본 생성자: 60000/1 (Unreal 타임코드 기본)
    constexpr FFrameRate(const uint32 InNumerator = 60000, const uint32 InDenominator = 1)
        : Numerator(InNumerator), Denominator(InDenominator)
    {}

    /// 역수
    constexpr FFrameRate Reciprocal() const
    {
        return FFrameRate(Denominator, Numerator);
    }

    /// 유효 여부: 분모가 양수여야 합니다.
    bool IsValid() const
    {
        return Denominator > 0;
    }

    /// FPS를 소수 형태로 반환 (e.g. 30.0)
    double AsDecimal() const
    {
        return static_cast<double>(Numerator) / Denominator;
    }

    /// 한 프레임의 시간(초)을 반환 (e.g. 1/30 = 0.0333s)
    double AsInterval() const
    {
        return static_cast<double>(Denominator) / Numerator;
    }
    
    /// 프레임 수 → FFrameTime
    FFrameTime AsFrameTime(const double FrameCount) const
    {
        return FFrameTime::FromDecimal(FrameCount);
    }


    /// 프레임 수 → 시간(초) 변환
    double AsSeconds(const double FrameCount) const
    {
        return FrameCount * AsInterval();
    }

    /// 시간(초) → 프레임 수 변환
    double AsFrames(const double Seconds) const
    {
        return Seconds * AsDecimal();
    }

    /// 시간(초) → FFrameTime
    FFrameTime AsFrameTimeFromSeconds(const double Seconds) const
    {
        const double Frames = AsFrames(Seconds);
        return FFrameTime::FromDecimal(Frames);
    }

    /// 시간(초) → 정수 프레임 번호(내림)
    FFrameNumber AsFrameNumber(const double Seconds) const
    {
        const double Frames = AsFrames(Seconds);
        const int32 Whole = FMath::Floor(Frames);
        return FFrameNumber(Whole);
    }
    
    /// FFrameTime 기반 변환: this rate -> DestRate
    FFrameTime TransformTime(const FFrameTime& SourceTime, const FFrameRate& DestRate) const
    {
        const double Seconds = AsInterval() * SourceTime.AsDecimal();
        const double DestFrames = DestRate.AsDecimal() * Seconds;
        return FFrameTime::FromDecimal(DestFrames);
    }

    /// SourceTime을 이 프레임레이트 기준으로 반올림해 스냅
    FFrameTime Snap(const FFrameTime& SourceTime) const
    {
        const FFrameTime Local = TransformTime(SourceTime, *this);
        return Local.RoundToFrame();
    }

    /// SourceTime을 이 프레임레이트 기준으로 내림해 스냅
    FFrameTime SnapFloor(const FFrameTime& SourceTime) const
    {
        const FFrameTime Local = TransformTime(SourceTime, *this);
        return Local.FloorToFrame();
    }

    /// SourceTime을 이 프레임레이트 기준으로 올림해 스냅
    FFrameTime SnapCeil(const FFrameTime& SourceTime) const
    {
        const FFrameTime Local = TransformTime(SourceTime, *this);
        return Local.CeilToFrame();
    }
    
    /// 다른 프레임레이트로 "프레임 수"를 변환
    /// sourceFrames: sourceRate 단위의 프레임 수
    /// 반환: destRate 단위의 프레임 수
    static double TransformFrame(const double sourceFrames, const FFrameRate& sourceRate, const FFrameRate& destRate)
    {
        // 1) sourceFrames → 초
        double seconds = sourceRate.AsInterval() * sourceFrames;
        // 2) 초 → destRate 프레임 수
        return destRate.AsDecimal() * seconds;
    }

    /// FFrameTime 기반 변환: SourceRate -> DestRate
    static FFrameTime TransformTime(const FFrameTime& SourceTime, const FFrameRate& SourceRate, const FFrameRate& DestRate)
    {
        const double Seconds = SourceRate.AsInterval() * SourceTime.AsDecimal();
        const double DestFrames = DestRate.AsDecimal() * Seconds;
        return FFrameTime::FromDecimal(DestFrames);
    }

    /// FFrameTime 반올림 스냅
    static FFrameTime Snap(const FFrameTime& SourceTime, const FFrameRate& SourceRate, const FFrameRate& SnapToRate)
    {
        const FFrameTime Local = TransformTime(SourceTime, SourceRate, SnapToRate);
        return Local.RoundToFrame();
    }

    /// FFrameTime 내림 스냅
    static FFrameTime SnapFloor(const FFrameTime& SourceTime, const FFrameRate& SourceRate, const FFrameRate& SnapToRate)
    {
        const FFrameTime Local = TransformTime(SourceTime, SourceRate, SnapToRate);
        return Local.FloorToFrame();
    }

    /// FFrameTime 올림 스냅
    static FFrameTime SnapCeil(const FFrameTime& SourceTime, const FFrameRate& SourceRate, const FFrameRate& SnapToRate)
    {
        const FFrameTime Local = TransformTime(SourceTime, SourceRate, SnapToRate);
        return Local.CeilToFrame();
    }

    /// 이 프레임레이트가 Other의 배수인지
    bool IsMultipleOf(const FFrameRate& Other) const
    {
        const int64 lhs = static_cast<int64>(Numerator) * Other.Denominator;
        const int64 rhs = static_cast<int64>(Other.Numerator) * Denominator;
        return rhs % lhs == 0;
    }

    /// 이 프레임레이트가 Other의 약수인지
    bool IsFactorOf(const FFrameRate& Other) const
    {
        return Other.IsMultipleOf(*this);
    }

    /// 해시
    friend uint32 GetTypeHash(const FFrameRate& Rate)
    {
        return (Rate.Numerator * 397) ^ Rate.Denominator;
    }

    /// 사칙연산: 분수 곱
    friend constexpr FFrameRate operator*(const FFrameRate& A, const FFrameRate& B)
    {
        return FFrameRate(A.Numerator * B.Numerator, A.Denominator * B.Denominator);
    }

    /// 사칙연산: 분수 나눗셈
    friend constexpr FFrameRate operator/(const FFrameRate& A, const FFrameRate& B)
    {
        return FFrameRate(A.Numerator * B.Denominator, A.Denominator * B.Numerator);
    }
};
