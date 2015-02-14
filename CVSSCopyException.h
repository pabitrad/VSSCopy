#pragma once

class CVSSCopyException
{
private:
    CString _message;

public: 
    CVSSCopyException::CVSSCopyException(LPCTSTR message)
    {
        _message.Append(message); 
    }

    LPCTSTR get_Message(void)
    {
        return _message; 
    }
};