#pragma once

#include "Utilities.h"
#include "CCopyFilter.h"

class CModifiedSinceCopyFilter : public CCopyFilter
{
private: 
    FILETIME _since; 
	bool _skipDenied; 

public: 
    CModifiedSinceCopyFilter(LPSYSTEMTIME since, bool skipDenied)
    {
		_skipDenied = skipDenied; 
        BOOL worked = ::SystemTimeToFileTime(since, &_since);  

        if (!worked)
        {
            DWORD error = ::GetLastError(); 
            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("SystemTimeToFileTime failed with error %s"), errorMessage); 
            throw new CVSSCopyException(message); 
        }
    }

    bool IsDirectoryMatch(LPCTSTR path)
    {
        return true; 
    }
    
    bool IsFileMatch(LPCTSTR path)
    {
        HANDLE hFile = ::CreateFile(
            path, 
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, 
            NULL, 
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL); 

        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD error = ::GetLastError();

			if (error == 5 && _skipDenied)
			{
				return false; 
			}

            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("Unable to open file %s exists. Error %s."), path, errorMessage);
            throw new CVSSCopyException(message); 
        }

        FILETIME modified; 
        BOOL worked = ::GetFileTime(hFile, NULL, NULL, &modified); 

        if (!worked)
        {
            DWORD error = ::GetLastError(); 

			if (error == 5 && _skipDenied)
			{
				::CloseHandle(hFile); 
				return false; 
			}

            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("Unable to retrieve file time from file %s. Error %s."), path, errorMessage); 
            ::CloseHandle(hFile); 
            throw new CVSSCopyException(message); 
        }

        ::CloseHandle(hFile); 

        int comparison = ::CompareFileTime(&_since, &modified); 

        if (comparison == -1)
        {
            return true;
        }
        else
        {
            return false; 
        }
    }
};