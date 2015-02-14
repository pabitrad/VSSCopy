#pragma once

class CDirectoryAction
{
public:
    virtual void VisitDirectoryFinal(LPCTSTR path) = 0; 
    virtual void VisitDirectoryInitial(LPCTSTR path) = 0; 
    virtual void VisitFile(LPCTSTR path) = 0; 
};