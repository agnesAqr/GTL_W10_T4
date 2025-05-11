#pragma once
#include "String.h"

class FFileHelper
{
public:
    static bool WriteStringToLogFile(const FString& FilenameRelative, const FString& Content);
};
