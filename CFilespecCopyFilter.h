#pragma once

using namespace std; 

class CFilespecCopyFilter : public CCopyFilter
{
private:
    vector<CString>& _filespecs; 

public:
    CFilespecCopyFilter::CFilespecCopyFilter(vector<CString>& filespecs) : _filespecs(filespecs)
    {
    }

    bool IsDirectoryMatch(LPCTSTR path)
    {
        return true; 
    }
    bool IsFileMatch(LPCTSTR path)
    {
        // No filespecs means "match everything"
        if (_filespecs.size() == 0)
        {
            return true; 
        }

        CString filename; 
        Utilities::GetFileName(CString(path), filename); 

        for (unsigned int iFilespec = 0; iFilespec < _filespecs.size(); ++iFilespec)
        {
            if (Utilities::IsMatch(filename, _filespecs[iFilespec]))
            {
                return true; 
            }
        }

        return false; 
    }
};
