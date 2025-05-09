#pragma once
#include <filesystem>

#include "Array.h"
#include "String.h"

class FString;

struct FPath
{
    using PathType = std::filesystem::path;

private:
    PathType InternalPath;
    
public:
    //–– 생성자
    FPath() = default;
    explicit FPath(const FString& InPath)            : InternalPath(GetData(InPath)) {}
    explicit FPath(const char* InPath)               : InternalPath(InPath) {}
    explicit FPath(const PathType& InFsPath)         : InternalPath(InFsPath) {}

    //–– 문자열 변환
    FString ToString() const
    {
        return InternalPath.string();
    }

    FString GetBaseFilename() const
    {
        return InternalPath.stem().string();
    }
    
    FString GetExtension(const bool bIncludeDot = true) const
    {
        auto ext = InternalPath.extension().string();
        if (!bIncludeDot && !ext.empty() && ext.front()=='.')
            return ext.substr(1);
        return ext;
    }

    FString GetFilename() const                   { return InternalPath.filename().string(); }
    FString GetDirectory() const                  { return InternalPath.parent_path().string(); }
    //
    FPath        Combine(const FString& Other) const { return FPath(InternalPath / GetData(Other)); }
    FPath        Normalize() const                    { return FPath(InternalPath.lexically_normal()); }
    FPath        Append(const FString& Other) const
    {
        FString s = InternalPath.string().c_str();  // FString으로 변환
        s += Other;                                // FString 연산자+
        return FPath(s);
    }

    bool         Exists() const                       { return std::filesystem::exists(InternalPath); }
    bool         IsDirectory() const                  { return std::filesystem::is_directory(InternalPath); }
    bool         IsFile() const                       { return std::filesystem::is_regular_file(InternalPath); }
    bool         CreateDirectories() const            { return std::filesystem::create_directories(InternalPath); }
    bool         RemoveFile() const                   { return std::filesystem::remove(InternalPath); }

   TArray<FString> ListDirectory(const bool bRecursive = false) const
    {
        TArray<FString> Out;
        if (bRecursive)
        {
            for (auto& E : std::filesystem::recursive_directory_iterator(InternalPath))
                Out.Add(E.path().string());
        }
        else
        {
            for (auto& E : std::filesystem::directory_iterator(InternalPath))
                Out.Add(E.path().string());
        }
        return Out;
    }
    
    /** 파일명(확장자 제외) 반환 */
    static FString GetBaseFilename(const FString& InPath);

    /** 확장자 반환 (bIncludeDot=true → ".txt", false → "txt") */
    static FString GetExtension(const FString& InPath, bool bIncludeDot = true);

    /** 디렉토리 경로 반환 (마지막 구분자 제외) */
    static FString GetDirectory(const FString& InPath);

    /** 두 경로를 합침 (중간에 Separator 자동 처리) */
    static FString Combine(const FString& A, const FString& B);

    FPath Combine(const FPath& Other) const;

    /** 확장자 변경 (NewExt에 "." 포함 가능) */
    static FString ChangeExtension(const FString& InPath, const FString& NewExt);

    /** 경로 정규화 ("//", "../" 등 제거) */
    static FString Normalize(const FString& InPath);

    /** 절대 경로 반환 */
    static FString GetAbsolutePath(const FString& InPath);

    /** base로부터 to까지의 상대 경로 반환 */
    static FString GetRelativePath(const FString& Base, const FString& To);

    /** 존재 여부 확인 */
    static bool Exists(const FString& InPath);

    /** 디렉토리 여부 확인 */
    static bool IsDirectory(const FString& InPath);

    /** 일반 파일 여부 확인 */
    static bool IsFile(const FString& InPath);

    /** 디렉토리(및 하위폴더) 생성 */
    static bool CreateDirectories(const FString& InPath);

    /** 파일 삭제 */
    static bool RemoveFile(const FString& InPath);

    /** 디렉토리 내 파일/폴더 목록 반환 */
    static TArray<FString> ListDirectory(const FString& InPath, bool bRecursive = false);

    //–– 연산자 오버로드
    FPath   operator/(const FPath& Other)         const;
    explicit operator FString()                            const { return ToString(); }
};
