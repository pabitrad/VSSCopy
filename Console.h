#pragma once

#include "Utilities.h"

class Console
{
private:
    static HANDLE s_hStdOut; 
public: 
    static HANDLE get_StandardOutput(void)
    {
        if (s_hStdOut == INVALID_HANDLE_VALUE)
        {
            HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE); 

            if (hOut != INVALID_HANDLE_VALUE)
            {
                ::DuplicateHandle(::GetCurrentProcess(), 
                    hOut, 
                    ::GetCurrentProcess(), 
                    &s_hStdOut, 
                    0, 
                    FALSE, 
                    DUPLICATE_SAME_ACCESS); 
            }
        }
        return s_hStdOut;
    }

    static TCHAR ReadChar()
    {
        HANDLE hStdIn = ::GetStdHandle(STD_INPUT_HANDLE); 

        TCHAR buffer[1]; 
        DWORD charsRead; 
        BOOL bWorked = ::ReadConsole(hStdIn, 
            buffer, 
            1, 
            &charsRead, 
            NULL); 

        if (!bWorked)
        {
            DWORD error = ::GetLastError();
            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("There was an error calling ReadConsole. Error %s"), errorMessage); 
            throw new CVSSCopyException(message); 
        }

        if (charsRead != 1)
        {
            throw new CVSSCopyException(TEXT("ReadConsole was unable to read a character.")); 
        }

        return buffer[0]; 
    }

    static void Write(LPCTSTR message)
    {
        CString messageString(message); 

        LPCSTR narrowMessage = Utilities::ConvertToMultibyteString(message); 

        DWORD charsWritten; 
        BOOL bWorked = ::WriteFile(get_StandardOutput(), narrowMessage, messageString.GetLength(), &charsWritten, NULL); 

        Utilities::Free(narrowMessage); 

        if (!bWorked)
        {
            DWORD error = ::GetLastError();
            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("Unable to write to the console. %s."), errorMessage); 
            throw new CVSSCopyException(message);             
        }
    }

    static void WriteLine(LPCTSTR message)
    {
        Write(message); 
        Write(TEXT("\r\n")); 
    }
};