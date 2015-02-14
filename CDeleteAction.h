#pragma once

#include "Utilities.h"
#include "OutputWriter.h"

class CDeleteAction : public CDirectoryAction
{
private:
    LPCTSTR _target; 

public: 
    CDeleteAction::CDeleteAction(LPCTSTR target)
    {
        _target = target; 
    }

    void VisitDirectoryFinal(LPCTSTR path)
    {
        CString fullPath;
        Utilities::CombinePath(_target, path, fullPath);
        Utilities::FixLongFilenames(fullPath); 

        BOOL bWorked = ::RemoveDirectory(fullPath); 

        if (!bWorked)
        {
            DWORD error = ::GetLastError(); 

            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("Error %s calling RemoveDirectory on %s"), errorMessage, fullPath); 
            throw new CVSSCopyException(message); 
        }
        else
        {
            CString message; 
            message.AppendFormat(TEXT("Deleted directory %s"), fullPath); 
            OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 
        }
    }
  
    void VisitDirectoryInitial(LPCTSTR path)
    {
        // Do nothing
    }

    virtual void VisitFile(LPCTSTR path)
    {
        CString fullPath;
        Utilities::CombinePath(_target, path, fullPath);

        Utilities::FixLongFilenames(fullPath); 

        BOOL bWorked = ::DeleteFile(fullPath); 

        if (!bWorked)
        {
            DWORD error = ::GetLastError(); 

            // Maybe it's read-only
            if (error == 5)
            {
                CString message; 
                message.AppendFormat(TEXT("Permission denied when deleting file %s. Resetting read-only bit and retrying."), 
                    fullPath); 
                OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_IF_VERBOSE); 

                DWORD attributes = ::GetFileAttributes(fullPath); 

                if (attributes == INVALID_FILE_ATTRIBUTES)
                {
                    CString message;
                    message.AppendFormat(TEXT("Failed to retrieve attributes for file %s."), fullPath); 
                    throw new CVSSCopyException(message); 
                }

                attributes &= ~FILE_ATTRIBUTE_READONLY; 

                bWorked = ::SetFileAttributes(fullPath, attributes); 

                if (!bWorked)
                {
                    CString message;
                    message.AppendFormat(TEXT("Failed to clear read-only bit on %s"), fullPath); 
                    throw new CVSSCopyException(message); 
                }

                bWorked = ::DeleteFile(fullPath); 
                if (!bWorked)
                {
                    error = ::GetLastError(); 
                }
            }

            if (!bWorked)
            {
                CString errorMessage; 
                Utilities::FormatErrorMessage(error, errorMessage); 
                CString message; 
                message.AppendFormat(TEXT("Error %s calling DeleteFile on %s"), errorMessage, path); 
                throw new CVSSCopyException(message); 
            }
        }

        if (bWorked)
        {
            CString message; 
            message.AppendFormat(TEXT("Successfully deleted file %s."), fullPath); 
            OutputWriter::WriteLine(message); 
        }
    }

};