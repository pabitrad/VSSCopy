#pragma once

class CCopyFilter
{
private:
public:
    virtual bool IsDirectoryMatch(LPCTSTR path) = 0; 
    virtual bool IsFileMatch(LPCTSTR path) = 0; 
};