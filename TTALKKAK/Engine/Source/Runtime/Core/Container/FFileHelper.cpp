#include "FFileHelper.h"

#include "Path.h"
#include "UserInterface/Console.h"

bool FFileHelper::WriteStringToLogFile(const FString& FilenameRelative, const FString& Content)
{
    // 최종 경로: Saved/<FilenameRelative>
    FPath FullPath = FPath(FPath::Combine(FPath::ProjectSavedDir(), FilenameRelative)).Normalize();

    // 2) 폴더(디렉터리) 존재 여부 확인 및 생성
    std::filesystem::path NativePath = FullPath.GetPath();
    std::filesystem::path ParentDir = NativePath.parent_path();

    if (!std::filesystem::exists(ParentDir))
    {
        if (!std::filesystem::create_directories(ParentDir))
        {
            UE_LOG(LogLevel::Error, TEXT("Failed to create directory: %s"), *FString(ParentDir.string().c_str()));
            return false;
        }
    }

    // 3) std::string 으로 변환
    std::string PathStr = NativePath.string();  // narrow string
    std::ofstream OutFile(PathStr, std::ios::out | std::ios::trunc);
    if (!OutFile.is_open())
    {
        UE_LOG(LogLevel::Error, TEXT("WriteStringToLogFile: Failed to open '%s'"), GetData(FullPath.ToString()));
        return false;
    }

    // 4) 내용 쓰기
    OutFile << GetData(Content);

    // 5) 닫기
    OutFile.close();
    if (OutFile.fail())
    {
        UE_LOG(LogLevel::Error, TEXT("WriteStringToLogFile: Failed to write to '%s'"), GetData(FullPath.ToString()));
        return false;
    }

    UE_LOG(LogLevel::Display, TEXT("WriteStringToLogFile: Successfully wrote to '%s'"), GetData(FullPath.ToString()));
    return true;
}
