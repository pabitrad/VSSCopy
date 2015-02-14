#pragma once

#include "stdafx.h"
#include <string>
#include <iostream>

using namespace std;

class CComException
{

private: 
    HRESULT _hresult; 
    const char* _file; 
    int _line; 

public: 
    CComException::CComException(HRESULT hresult, const char* file, int line)
    {
        _hresult = hresult; 
        _file = file; 
        _line = line; 
    }

    HRESULT get_Hresult(void)
    {
        return _hresult; 
    }

    void get_File(CString& file)
    {
        // Hack: this part is not TCHAR-aware, but this is the only place
        size_t length = strlen(_file); 
        WCHAR* buffer = new WCHAR[length + 1]; 
        ::MultiByteToWideChar(CP_ACP, 0, _file, (int) length, buffer, (int) length); 
        buffer[length] = L'\0';
        file.Empty(); 
        file.Append(buffer); 
        delete buffer; 
    }

    int get_Line(void)
    {
        return _line; 
    }

};