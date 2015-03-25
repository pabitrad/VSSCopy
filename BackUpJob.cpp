#include "stdafx.h"
#include "BackUpJob.h"

using namespace std;

BackUpJob::BackUpJob()
{

}

BackUpJob::~BackUpJob()
{
}

//Absolute path of file
void BackUpJob::loadJob(wstring jobFileName)
{
	wifstream jobFile(jobFileName);
	wstring line;
	int linePos = 0;

	while (getline(jobFile, line))
	{
		size_t pos = line.find(_T("files"));
		if (pos != string::npos)
		{
			wstring strSourceLocations = line.substr(pos + 6);
			split((TCHAR *) strSourceLocations.c_str());
		}
		else if ((pos = line.find(_T("name"))) != string::npos)
		{
			_name = line.substr(pos + 5);
		}
		else if ((pos = line.find(_T("type"))) != string::npos)
		{
			wstring strFileType = line.substr(pos + 5);
			if (strFileType == _T("One to One"))
			{
				_backupType = ONE_TO_ONE_BACKUP;
			}
			else if (strFileType == _T("Flat File Backup"))
			{
				_backupType = FLAT_FILE_BACKUP;
			}
		}
		else if ((pos = line.find(_T("path"))) != string::npos)
		{
			_targetLocation = line.substr(pos + 5);
		}
	}
}

void BackUpJob::split(TCHAR * str)
{
	TCHAR * token = wcstok(str, _T("|"));

	while (token != NULL)
	{
		sporceLocations.push_back(token);
		token = wcstok(NULL, _T("|"));
	}
}

vector<wstring>& BackUpJob::getSourceLocations()
{
	return sporceLocations;
}

wstring BackUpJob::getTargetLocation(wstring sourceLocation)
{
	wstring targetLocation = _targetLocation;
	if (_backupType == ONE_TO_ONE_BACKUP)
	{
		wstring sourceFolder = sporceLocations[0];
		size_t pos = sourceFolder.rfind(_T("\\"));
		if (pos != wstring::npos)
		{
			if (pos == sourceFolder.length() - 1)
			{
				sourceFolder.erase(pos);
				pos = sourceFolder.rfind(_T("\\"));
			}
			sourceFolder.erase(0, pos);
			//targetLocation += _T("\\");
			targetLocation += sourceFolder;
		}
	}
	else if (_backupType == FLAT_FILE_BACKUP)
	{
		size_t pos = sourceLocation.find(_T("\\"));
		if (pos != wstring::npos)
		{
			sourceLocation.erase(0, pos);

			CTime t = CTime::GetCurrentTime();
			CString strDate = t.Format("%m%d%Y");

			targetLocation = targetLocation + _name + _T("\\") + (LPCTSTR)strDate + sourceLocation + _T("\\");
		}
	}

	return targetLocation;
}