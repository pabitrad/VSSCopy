#pragma once

class CParseOptionsException
{
private: 
    CString _message; 

public: 
    CParseOptionsException::CParseOptionsException(LPCTSTR message)
    {
        _message = message; 
    }

    CParseOptionsException::CParseOptionsException(CString& message)
    {
        _message = message.GetString(); 
    }

    LPCTSTR get_Message(void)
    {
        return _message; 
    }
};
