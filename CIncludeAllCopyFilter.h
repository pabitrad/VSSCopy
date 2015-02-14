#pragma once

#include "CCopyFilter.h"

class CIncludeAllCopyFilter : public CCopyFilter
{
public: 
    bool IsDirectoryMatch(LPCTSTR path)
    {
        return true; 
    }

    bool IsFileMatch(LPCTSTR path)
    {
        return true; 
    }
};