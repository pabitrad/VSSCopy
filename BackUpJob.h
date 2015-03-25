#pragma once
#include <vector>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

class BackUpJob
{
	enum BackupType
	{
		FLAT_FILE_BACKUP,
		ONE_TO_ONE_BACKUP
	};

private:
	wstring _name;
	BackupType _backupType;
	wstring _targetLocation;
	vector<wstring> sporceLocations;
	void split(TCHAR * str);

public:
	BackUpJob();
	~BackUpJob();

	void loadJob(wstring jobFileName);
	vector<wstring>& getSourceLocations();
	BackupType getBackupType() { return _backupType; }
	wstring& getJobName() { return _name; }
	wstring getTargetLocation(wstring sourceLocation);
};

