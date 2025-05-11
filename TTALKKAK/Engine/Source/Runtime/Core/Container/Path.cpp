#include "Path.h"

#include "String.h"

using namespace std::filesystem;

FString FPath::StripContentsPrefix(const FString& InFullPath)
{
    static const std::string Prefix = TEXT("C:\\Users\\Jungle\\Desktop\\Github\\GTL_W10_T4\\Week0v2\\Contents\\");
    if (InFullPath.RFind(Prefix) == 0)
    {
        return InFullPath.Substr(Prefix.length());
    }
    return InFullPath;
}

FString FPath::GetBaseFilename(const FString& InPath)
{
    return PathType(GetData(InPath)).stem().string();
}

FString FPath::GetExtension(const FString& InPath, const bool bIncludeDot)
{
    auto ext = PathType(GetData(InPath)).extension().string();
    if (!bIncludeDot && !ext.empty() && ext.front() == '.')
        return ext.substr(1);
    return ext;
}

FString FPath::GetDirectory(const FString& InPath)
{
    return PathType(GetData(InPath)).parent_path().string();
}

FString FPath::Combine(const FString& A, const FString& B)
{
    return (PathType(GetData(A)) / PathType(GetData(B))).string();
}

FPath FPath::Combine(const FPath& Other) const
{
    return FPath(InternalPath / Other.InternalPath);
}

FString FPath::ChangeExtension(const FString& InPath, const FString& NewExt)
{
    PathType p(GetData(InPath));
    p.replace_extension(GetData(NewExt));
    return p.string();
}

FString FPath::Normalize(const FString& InPath)
{
    return PathType(GetData(InPath)).lexically_normal().string();
}

FString FPath::GetAbsolutePath(const FString& InPath)
{
    return absolute(PathType(GetData(InPath))).string();
}

FString FPath::GetRelativePath(const FString& Base, const FString& To)
{
    return relative(PathType(GetData(To)), PathType(GetData(Base))).string();
}

bool FPath::Exists(const FString& InPath)
{
    return exists(PathType(GetData(InPath)));
}

bool FPath::IsDirectory(const FString& InPath)
{
    return is_directory(PathType(GetData(InPath)));
}

bool FPath::IsFile(const FString& InPath)
{
    return is_regular_file(PathType(GetData(InPath)));
}

bool FPath::CreateDirectories(const FString& InPath)
{
    return create_directories(PathType(GetData(InPath)));
}

bool FPath::RemoveFile(const FString& InPath)
{
    return remove(PathType(GetData(InPath)));
}

TArray<FString> FPath::ListDirectory(const FString& InPath, const bool bRecursive)
{
    TArray<FString> Out;
    if (bRecursive)
    {
        for (auto& E : std::filesystem::recursive_directory_iterator(PathType(GetData(InPath))))
            Out.Add(E.path().string());
    }
    else
    {
        for (auto& E : std::filesystem::directory_iterator(PathType(GetData(InPath))))
            Out.Add(E.path().string());
    }
    return Out;
}

FPath FPath::operator/(const FPath& Other) const
{
    return Combine(Other);
}
