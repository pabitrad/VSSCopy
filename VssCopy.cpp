#include "stdafx.h"
#include "CComException.h"
#include "CVSSCopyException.h"
#include "COptions.h"
#include "CDirectoryAction.h"
#include "CCopyAction.h"
#include "OutputWriter.h"
#include "CBackupState.h"
#include "CIncludeAllCopyFilter.h"
#include "CModifiedSinceCopyFilter.h"
#include "CFilespecCopyFilter.h"
#include "CDeleteAction.h"
#include "CWriter.h"
#include "CWriterComponent.h"

bool s_cancel = false; 


// Forward declarations
void CalculateSourcePath(LPCTSTR wszSnapshotDevice, LPCTSTR wszBackupSource, LPCTSTR wszMountPoint, CString& output);
void Cleanup(bool bAbnormalAbort, bool bSnapshotcreated, CComPtr<IVssBackupComponents> pBackupComponents, GUID snapshotSetId);
bool Confirm(LPCTSTR message); 
void CopyRecursive(LPCTSTR wszSource, LPCTSTR wszDestination, bool skipDenied, CCopyFilter& filter);
BOOL WINAPI CtrlHandler(DWORD dwCtrlType); 
void DeleteRecursive(LPCTSTR target, wregex* ignorePattern); 
void ProcessDirectory(LPCTSTR srcbase, CDirectoryAction& action, LPCTSTR directory, bool recursive, wregex* ignorePattern);
bool ShouldAddComponent(CWriterComponent& component);
LPCSTR WideToNarrow(LPCWSTR wsz);
bool ShouldProcess(wregex* ignorePattern, const CString& path);
bool ShouldProcess(wregex* ignorePattern, const CString& directory, const CString& name);

int _tmain(int argc, _TCHAR* argv[]){
	int nArgc = 6;
	_TCHAR **data = new _TCHAR*[nArgc];

	data[0] = argv[0];
	data[1] = new _TCHAR[3];

	data[1][0] = '/';
	data[1][1] = 'r';
	data[1][2] = '\0';

	data[2] = new _TCHAR[12];
	data[2][0] = '/';
	data[2][1] = 's';
	data[2][2] = 'k';
	data[2][3] = 'i';
	data[2][4] = 'p';
	data[2][5] = 'd';
	data[2][6] = 'e';
	data[2][7] = 'n';
	data[2][8] = 'i';
	data[2][9] = 'e';
	data[2][10] = 'd';
	data[2][11] = '\0';
		
	data[3] = new _TCHAR[MAX_PATH];
	data[4] = new _TCHAR[MAX_PATH];
	data[5] = new _TCHAR[MAX_PATH];

	while (true){
		OutputWriter::SetVerbosityLevel(VERBOSITY_LEVEL_NORMAL); 
		OutputWriter::WriteLine(TEXT("Volume Shadow Copy"), 
			VERBOSITY_THRESHOLD_UNLESS_SILENT); 
		OutputWriter::WriteLine(TEXT(""), VERBOSITY_THRESHOLD_UNLESS_SILENT); 

		GUID snapshotSetId = GUID_NULL; 
		bool bSnapshotCreated = false;
		bool bAbnormalAbort = true; 
		CComPtr<IVssBackupComponents> pBackupComponents; 

		int fileCount = 0; 
		LONGLONG byteCount = 0; 
		int directoryCount = 0; 
		int skipCount = 0; 
		SYSTEMTIME startTime;

		int choice;

		while (true){
			cout << "Enter your choice\n\n";
			cout << "1) Volume Shadow Copy\n";
			cout << "2) Quit\n\n";
			cout << "Enter your choice: ";
			cin >> choice;

			if (choice == 1 || choice == 2){
				break;
			}

			cout << "Invalid choice. Please try again" << endl << endl;
		}

		if (choice != 1){
			break;
		}

		try{			
			cout << "\nEnter source folder: ";
			_tscanf(_T("%s"), data[3]);

			cout << "Enter destination folder: ";
			_tscanf(_T("%s"), data[4]);

			cout << "\nEnter file filter \n(filename or file filter such as *.txt. 'W' to copy whole directory): ";
			_tscanf(_T("%s"), data[5]);

			if (_tcslen(data[5]) == 1){
				if (_tcscmp(data[5], _T("W")) == 0){
					nArgc--;
				}
			}

			COptions options = COptions::Parse(nArgc, data);

			OutputWriter::SetVerbosityLevel((VERBOSITY_LEVEL) options.get_VerbosityLevel()); 

			for (int i = 0; i < argc; ++i)
			{
				CString message; 
				message.AppendFormat(TEXT("Argument %d: %s"), i, argv[i]);
				OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_IF_VERBOSE);
			}

			OutputWriter::WriteLine(TEXT("Calling CoInitialize")); 
			CHECK_HRESULT(::CoInitialize(NULL)); 
			CHECK_HRESULT(
				::CoInitializeSecurity(
				NULL, 
				-1, 
				NULL, 
				NULL, 
				RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
				RPC_C_IMP_LEVEL_IDENTIFY, 
				NULL, 
				EOAC_NONE, 
				NULL)); 

			::GetSystemTime(&startTime); 
			CString startTimeString; 
			Utilities::FormatDateTime(&startTime, TEXT(" "), false, startTimeString); 

			CString message; 
			message.AppendFormat(TEXT("Starting a %s copy from %s to %s"), 
				options.get_BackupType() == VSS_BT_FULL ? TEXT("full") : TEXT("incremental"), 
				options.get_Source(), 
				options.get_Destination()); 
			OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 

			if (options.get_ClearDestination())
			{
				if (!Utilities::DirectoryExists(options.get_Destination()))
				{
					CString message; 
					message.AppendFormat(TEXT("Skipping recursive delete of destination directory %s because it appears not to exist."), 
						options.get_Destination()); 
					OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 
				}
				else
				{
					CString message; 
					message.AppendFormat(TEXT("Recursively deleting destination directory %s."), 
						options.get_Destination()); 
					OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 

					bool doDelete = options.get_AcceptAll(); 

					if (!doDelete)
					{
						if (Confirm(message))
						{
							doDelete = true; 
						}
						else
						{
							OutputWriter::WriteLine(TEXT("Aborting backup."), VERBOSITY_THRESHOLD_NORMAL); 
							return 3; 
						}
					}

					if (doDelete)
					{
						DeleteRecursive(options.get_Destination(), options.get_IgnorePattern()); 
					}
				}
			}

			CBackupState backupState; 

			LPSYSTEMTIME lastBackupTime; 

			if (options.get_BackupType() == VSS_BT_INCREMENTAL)
			{
				backupState.Load(options.get_StateFile()); 

				LPSYSTEMTIME lastFullBackupTime = backupState.get_LastFullBackupTime(); 
				LPSYSTEMTIME lastIncrementalBackupTime = backupState.get_LastIncrementalBackupTime(); 
				if (lastIncrementalBackupTime != NULL)
				{
					lastBackupTime = lastIncrementalBackupTime; 
				}
				else
				{
					lastBackupTime = lastFullBackupTime; 
				}
			}


			OutputWriter::WriteLine(TEXT("Calling CreateVssBackupComponents")); 
			CHECK_HRESULT(::CreateVssBackupComponents(&pBackupComponents)); 

			OutputWriter::WriteLine(TEXT("Calling InitializeForBackup")); 
			CHECK_HRESULT(pBackupComponents->InitializeForBackup()); 

			CComPtr<IVssAsync> pWriterMetadataStatus; 

			OutputWriter::WriteLine(TEXT("Calling GatherWriterMetadata")); 
			CHECK_HRESULT(pBackupComponents->GatherWriterMetadata(&pWriterMetadataStatus)); 

			OutputWriter::WriteLine(TEXT("Waiting for call to GatherWriterMetadata to finish...")); 
			CHECK_HRESULT(pWriterMetadataStatus->Wait()); 

			HRESULT hrGatherStatus; 
			OutputWriter::WriteLine(TEXT("Calling QueryStatus for GatherWriterMetadata")); 
			CHECK_HRESULT(pWriterMetadataStatus->QueryStatus(&hrGatherStatus, NULL)); 

			if (hrGatherStatus == VSS_S_ASYNC_CANCELLED)
			{
				throw new CVSSCopyException(L"GatherWriterMetadata was cancelled."); 
			}

			OutputWriter::WriteLine(TEXT("Call to GatherWriterMetadata finished.")); 


			OutputWriter::WriteLine(TEXT("Calling GetWriterMetadataCount")); 

			vector<CWriter> writers;

			UINT cWriters; 
			CHECK_HRESULT(pBackupComponents->GetWriterMetadataCount(&cWriters)); 

			for (UINT iWriter = 0; iWriter < cWriters; ++iWriter)
			{
				CWriter writer; 
				CComPtr<IVssExamineWriterMetadata> pExamineWriterMetadata; 
				GUID id; 
				OutputWriter::WriteLine(TEXT("Calling GetWriterMetadata")); 
				CHECK_HRESULT(pBackupComponents->GetWriterMetadata(iWriter, &id, &pExamineWriterMetadata)); 
				GUID idInstance; 
				GUID idWriter; 
				BSTR bstrWriterName;
				VSS_USAGE_TYPE usage; 
				VSS_SOURCE_TYPE source; 
				CHECK_HRESULT(pExamineWriterMetadata->GetIdentity(&idInstance, &idWriter, &bstrWriterName, &usage, &source)); 

				writer.set_InstanceId(idInstance); 
				writer.set_Name(bstrWriterName); 
				writer.set_WriterId(idWriter); 

				CComBSTR writerName(bstrWriterName); 
				CString message; 
				message.AppendFormat(TEXT("Writer %d named %s"), iWriter, (LPCTSTR) writerName); 
				OutputWriter::WriteLine(message); 

				UINT cIncludeFiles;
				UINT cExcludeFiles; 
				UINT cComponents; 
				CHECK_HRESULT(pExamineWriterMetadata->GetFileCounts(&cIncludeFiles, &cExcludeFiles, &cComponents)); 

				message.Empty(); 
				message.AppendFormat(TEXT("Writer has %d components"), cComponents); 
				OutputWriter::WriteLine(message); 

				for (UINT iComponent = 0; iComponent < cComponents; ++iComponent)
				{
					CWriterComponent component; 

					CComPtr<IVssWMComponent> pComponent; 
					CHECK_HRESULT(pExamineWriterMetadata->GetComponent(iComponent, &pComponent)); 

					PVSSCOMPONENTINFO pComponentInfo; 
					CHECK_HRESULT(pComponent->GetComponentInfo(&pComponentInfo)); 

					CString message; 
					message.AppendFormat(TEXT("Component %d is named %s, has a path of %s, and is %sselectable for backup. %d files, %d databases, %d log files."), 
						iComponent,
						pComponentInfo->bstrComponentName, 
						pComponentInfo->bstrLogicalPath, 
						pComponentInfo->bSelectable ? TEXT("") : TEXT("not "), 
						pComponentInfo->cFileCount, 
						pComponentInfo->cDatabases,
						pComponentInfo->cLogFiles); 
					OutputWriter::WriteLine(message); 

					component.set_LogicalPath(pComponentInfo->bstrLogicalPath); 
					component.set_SelectableForBackup(pComponentInfo->bSelectable); 
					component.set_Writer(iWriter); 
					component.set_Name(pComponentInfo->bstrComponentName);
					component.set_Type(pComponentInfo->type);

					for (UINT iFile = 0; iFile < pComponentInfo->cFileCount; ++iFile)
					{
						CComPtr<IVssWMFiledesc> pFileDesc; 
						CHECK_HRESULT(pComponent->GetFile(iFile, &pFileDesc)); 

						CComBSTR bstrPath; 
						CHECK_HRESULT(pFileDesc->GetPath(&bstrPath)); 

						CComBSTR bstrFileSpec; 
						CHECK_HRESULT(pFileDesc->GetFilespec(&bstrFileSpec)); 

						CString message; 
						message.AppendFormat(TEXT("File %d has path %s\\%s"), iFile, bstrPath, bstrFileSpec); 
						OutputWriter::WriteLine(message); 
					}

					for (UINT iDatabase = 0; iDatabase < pComponentInfo->cDatabases; ++iDatabase)
					{
						CComPtr<IVssWMFiledesc> pFileDesc; 
						CHECK_HRESULT(pComponent->GetDatabaseFile(iDatabase, &pFileDesc)); 

						CComBSTR bstrPath; 
						CHECK_HRESULT(pFileDesc->GetPath(&bstrPath)); 

						CComBSTR bstrFileSpec; 
						CHECK_HRESULT(pFileDesc->GetFilespec(&bstrFileSpec)); 

						CString message; 
						message.AppendFormat(TEXT("Database file %d has path %s\\%s"), iDatabase, bstrPath, bstrFileSpec); 
						OutputWriter::WriteLine(message); 
					}

					for (UINT iDatabaseLogFile = 0; iDatabaseLogFile < pComponentInfo->cLogFiles; ++iDatabaseLogFile)
					{
						CComPtr<IVssWMFiledesc> pFileDesc; 
						CHECK_HRESULT(pComponent->GetDatabaseLogFile(iDatabaseLogFile, &pFileDesc)); 

						CComBSTR bstrPath; 
						CHECK_HRESULT(pFileDesc->GetPath(&bstrPath)); 

						CComBSTR bstrFileSpec; 
						CHECK_HRESULT(pFileDesc->GetFilespec(&bstrFileSpec)); 

						CString message; 
						message.AppendFormat(TEXT("Database log file %d has path %s\\%s"), iDatabaseLogFile, bstrPath, bstrFileSpec); 
						OutputWriter::WriteLine(message); 
					}

					CHECK_HRESULT(pComponent->FreeComponentInfo(pComponentInfo)); 

					writer.get_Components().push_back(component); 

				}

				writer.ComputeComponentTree(); 

				for (unsigned int iComponent = 0; iComponent < writer.get_Components().size(); ++iComponent)
				{
					CWriterComponent& component = writer.get_Components()[iComponent]; 
					CString message; 
					message.AppendFormat(TEXT("Component %d has name %s, path %s, is %sselectable for backup, and has parent %s"), 
						iComponent, 
						component.get_Name(), 
						component.get_LogicalPath(), 
						component.get_SelectableForBackup() ? TEXT("") : TEXT("not "), 
						component.get_Parent() == NULL ? TEXT("(no parent)") : component.get_Parent()->get_Name()); 
					OutputWriter::WriteLine(message); 
				}

				writers.push_back(writer); 
			}

			OutputWriter::WriteLine(TEXT("Calling StartSnapshotSet")); 
			CHECK_HRESULT(pBackupComponents->StartSnapshotSet(&snapshotSetId));

			OutputWriter::WriteLine(TEXT("Calling GetVolumePathName")); 
			WCHAR wszVolumePathName[MAX_PATH]; 
			BOOL bWorked = ::GetVolumePathName(options.get_Source(), wszVolumePathName, MAX_PATH); 

			if (!bWorked)
			{
				DWORD error = ::GetLastError(); 
				CString errorMessage; 
				Utilities::FormatErrorMessage(error, errorMessage); 
				CString message; 
				message.AppendFormat(TEXT("There was an error retrieving the volume name from the path. Path: %s Error: %s"), 
					options.get_Source(), errorMessage); 
				throw new CVSSCopyException(message.GetString()); 
			}

			OutputWriter::WriteLine(TEXT("Calling AddToSnapshotSet")); 
			GUID snapshotId; 
			CHECK_HRESULT(pBackupComponents->AddToSnapshotSet(wszVolumePathName, GUID_NULL, &snapshotId)); 

			for (unsigned int iWriter = 0; iWriter < writers.size(); ++iWriter)
			{
				CWriter writer = writers[iWriter];

				CString message; 
				message.AppendFormat(TEXT("Adding components to snapshot set for writer %s"), writer.get_Name()); 
				OutputWriter::WriteLine(message); 
				for (unsigned int iComponent = 0; iComponent < writer.get_Components().size(); ++iComponent)
				{
					CWriterComponent component = writer.get_Components()[iComponent];

					if (ShouldAddComponent(component))
					{
						CString message; 
						message.AppendFormat(TEXT("Adding component %s (%s) from writer %s"), 
							component.get_Name(), 
							component.get_LogicalPath(), 
							writer.get_Name()); 
						OutputWriter::WriteLine(message); 
						CHECK_HRESULT(pBackupComponents->AddComponent(
							writer.get_InstanceId(), 
							writer.get_WriterId(),
							component.get_Type(), 
							component.get_LogicalPath(), 
							component.get_Name()
							));
					}
					else
					{
						CString message; 
						message.AppendFormat(TEXT("Not adding component %s from writer %s."), 
							component.get_Name(), writer.get_Name()); 
						OutputWriter::WriteLine(message); 
					}
				}
			}

			OutputWriter::WriteLine(TEXT("Calling SetBackupState")); 
			// Issue #29: trying to figure out if using VSS_BT_INCREMENTAL causes a problem
			CHECK_HRESULT(pBackupComponents->SetBackupState(TRUE, FALSE, VSS_BT_FULL, FALSE)); 

			OutputWriter::WriteLine(TEXT("Calling PrepareForBackup")); 
			CComPtr<IVssAsync> pPrepareForBackupResults; 
			CHECK_HRESULT(pBackupComponents->PrepareForBackup(&pPrepareForBackupResults)); 

			OutputWriter::WriteLine(TEXT("Waiting for call to PrepareForBackup to finish...")); 
			CHECK_HRESULT(pPrepareForBackupResults->Wait()); 

			HRESULT hrPrepareForBackupResults; 
			CHECK_HRESULT(pPrepareForBackupResults->QueryStatus(&hrPrepareForBackupResults, NULL)); 

			if (hrPrepareForBackupResults != VSS_S_ASYNC_FINISHED)
			{
				throw new CVSSCopyException(TEXT("Prepare for backup failed.")); 
			}

			OutputWriter::WriteLine(TEXT("Call to PrepareForBackup finished.")); 
		
			SYSTEMTIME snapshotTime; 
			::GetSystemTime(&snapshotTime); 

			bWorked = ::SetConsoleCtrlHandler(CtrlHandler, TRUE); 

			if (!bWorked)
			{
				OutputWriter::WriteLine(TEXT("Unable to set control handler. Ctrl-C and Ctrl-Break may have undesirable results."), 
					VERBOSITY_THRESHOLD_NORMAL);
			}

			if (!options.get_Simulate())
			{
				OutputWriter::WriteLine(TEXT("Calling DoSnapshotSet")); 
				CComPtr<IVssAsync> pDoSnapshotSetResults;
				CHECK_HRESULT(pBackupComponents->DoSnapshotSet(&pDoSnapshotSetResults)); 

				OutputWriter::WriteLine(TEXT("Waiting for call to DoSnapshotSet to finish...")); 
		   
				CHECK_HRESULT(pDoSnapshotSetResults->Wait());

				bSnapshotCreated = true; 

				if (s_cancel)
				{
					throw new CVSSCopyException(TEXT("Processing was cancelled by control-c, control-break, or a shutdown event. Terminating.")); 
				}

				bWorked = ::SetConsoleCtrlHandler(CtrlHandler, FALSE); 

				if (!bWorked)
				{
					OutputWriter::WriteLine(TEXT("Unable to reset control handler. Ctrl-C and Ctrl-Break may have undesirable results."), VERBOSITY_THRESHOLD_NORMAL);
				}

				HRESULT hrDoSnapshotSetResults; 
				CHECK_HRESULT(pDoSnapshotSetResults->QueryStatus(&hrDoSnapshotSetResults, NULL)); 

				if (hrDoSnapshotSetResults != VSS_S_ASYNC_FINISHED)
				{
					throw new CVSSCopyException(L"DoSnapshotSet failed."); 
				}

				OutputWriter::WriteLine(TEXT("Call to DoSnapshotSet finished.")); 

				OutputWriter::WriteLine(TEXT("Calling GetSnapshotProperties")); 
				VSS_SNAPSHOT_PROP snapshotProperties; 
				CHECK_HRESULT(pBackupComponents->GetSnapshotProperties(snapshotId, &snapshotProperties));

				OutputWriter::WriteLine(TEXT("Calling CalculateSourcePath")); 
				// TODO: We'll eventually have to deal with mount points
				CString wszSource;
				CalculateSourcePath(
					snapshotProperties.m_pwszSnapshotDeviceObject, 
					options.get_Source(),
					wszVolumePathName, 
					wszSource
					);

				message.Empty(); 
				message.AppendFormat(TEXT("Recursively creating destination directory %s."), 
					options.get_Destination()); 
				OutputWriter::WriteLine(message); 

				Utilities::CreateDirectory(options.get_Destination()); 

				OutputWriter::WriteLine(TEXT("Calling CopyRecursive")); 

				vector<CCopyFilter*> filters; 

				if (options.get_BackupType() == VSS_BT_FULL)
				{
					filters.push_back(new CIncludeAllCopyFilter()); 
				}
				else if (options.get_BackupType() == VSS_BT_INCREMENTAL)
				{
					filters.push_back(new CModifiedSinceCopyFilter(lastBackupTime, options.get_SkipDenied()));
				}

				filters.push_back(new CFilespecCopyFilter(options.get_Filespecs())); 

				CCopyAction copyAction(wszSource, options.get_Destination(), options.get_SkipDenied(), filters); 
				ProcessDirectory(wszSource, copyAction, TEXT(""), options.get_Recursive(), options.get_IgnorePattern()); 

				for (unsigned int iFilter = 0; iFilter < filters.size(); ++iFilter)
				{
					delete filters[iFilter]; 
				}

				fileCount = copyAction.get_FileCount(); 
				directoryCount = copyAction.get_DirectoryCount();
				skipCount = copyAction.get_SkipCount(); 
				byteCount = copyAction.get_ByteCount(); 

				OutputWriter::WriteLine(TEXT("Calling BackupComplete")); 
				CComPtr<IVssAsync> pBackupCompleteResults; 
				CHECK_HRESULT(pBackupComponents->BackupComplete(&pBackupCompleteResults)); 

				OutputWriter::WriteLine(TEXT("Waiting for call to BackupComplete to finish...")); 
				CHECK_HRESULT(pBackupCompleteResults->Wait());

				HRESULT hrBackupCompleteResults; 
				CHECK_HRESULT(pBackupCompleteResults->QueryStatus(&hrBackupCompleteResults, NULL)); 

				if (hrBackupCompleteResults != VSS_S_ASYNC_FINISHED)
				{
					throw new CVSSCopyException(TEXT("Completion of backup failed.")); 
				}

				OutputWriter::WriteLine(TEXT("Call to BackupComplete finished.")); 

				bAbnormalAbort = false; 

				if (options.get_StateFile() != NULL)
				{
					OutputWriter::WriteLine(TEXT("Calling SaveAsXML"));
					CComBSTR bstrBackupDocument; 
					CHECK_HRESULT(pBackupComponents->SaveAsXML(&bstrBackupDocument)); 

					if (options.get_BackupType() == VSS_BT_FULL)
					{
						backupState.set_LastFullBackupTime(&snapshotTime); 
					}
					else if (options.get_BackupType() == VSS_BT_INCREMENTAL)
					{
						backupState.set_LastIncrementalBackupTime(&snapshotTime); 
					}
					else
					{
						throw new CVSSCopyException(TEXT("Unsupported backup type.")); 
					}

					backupState.Save(options.get_StateFile(), bstrBackupDocument); 
				}
			}
		}
		catch (CComException* e)
		{
			Cleanup(bAbnormalAbort, bSnapshotCreated, pBackupComponents, snapshotSetId);
			CString message; 
			CString file; 
			e->get_File(file); 
			message.Format(TEXT("There was a COM failure 0x%x - %s (%d)"), 
				e->get_Hresult(), file, e->get_Line()); 
			OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_UNLESS_SILENT); 
			return 1; 
		}
		catch (CVSSCopyException* e)
		{
			Cleanup(bAbnormalAbort, bSnapshotCreated, pBackupComponents, snapshotSetId);
			OutputWriter::WriteLine(e->get_Message(), VERBOSITY_THRESHOLD_UNLESS_SILENT); 
			return 1; 
		}
		catch (CParseOptionsException* e)
		{
			Cleanup(bAbnormalAbort, bSnapshotCreated, pBackupComponents, snapshotSetId);
			CString message; 
			message.AppendFormat(TEXT("Error: %s\n"), e->get_Message()); 
			OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_UNLESS_SILENT);
			OutputWriter::WriteLine(COptions::get_Usage(), VERBOSITY_THRESHOLD_UNLESS_SILENT); 
			return 2; 
		}

		Cleanup(false, bSnapshotCreated, pBackupComponents, snapshotSetId);
		OutputWriter::WriteLine(TEXT("Backup successfully completed."), VERBOSITY_THRESHOLD_UNLESS_SILENT); 

		CString message; 
		CString startTimeStringLocal; 
		Utilities::FormatDateTime(&startTime, TEXT(" "), true, startTimeStringLocal); 
		CString finishTimeString; 
		SYSTEMTIME finishTime; 
		::GetSystemTime(&finishTime); 
		Utilities::FormatDateTime(&finishTime, TEXT(" "), true, finishTimeString); 
		message.AppendFormat(TEXT("Backup started at %s, completed at %s."), 
			startTimeStringLocal, finishTimeString); 
		OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 
		message.Empty(); 

		float unitCount = (float) byteCount; 
		LPCTSTR units = TEXT("bytes"); 

		if (unitCount > 1024)
		{
			unitCount = unitCount / 1024.0F; 
			units = TEXT("KB"); 
		}

		if (unitCount > 1024)
		{
			unitCount = unitCount / 1024.0F; 
			units = TEXT("MB"); 
		}

		if (unitCount > 1024)
		{
			unitCount = unitCount / 1024.0F; 
			units = TEXT("GB"); 
		}

		message.AppendFormat(TEXT("%d files (%.2f %s, %d directories) copied, %d files skipped"), 
			fileCount, unitCount, units, directoryCount, skipCount); 
		OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 
	}
    return 0;
}


void CalculateSourcePath(LPCTSTR wszSnapshotDevice, LPCTSTR wszBackupSource, LPCTSTR wszMountPoint, CString& output)
{
    CString backupSource(wszBackupSource); 
    CString mountPoint(wszMountPoint); 

    CString subdirectory = backupSource.Mid(mountPoint.GetLength()); 

    Utilities::CombinePath(wszSnapshotDevice, subdirectory, output); 
}

void Cleanup(bool bAbnormalAbort, bool bSnapshotCreated, CComPtr<IVssBackupComponents> pBackupComponents, GUID snapshotSetId)
{
    if (pBackupComponents == NULL)
    {
        return; 
    }

    if (bAbnormalAbort)
    {
        pBackupComponents->AbortBackup(); 
    }
    if (bSnapshotCreated)
    {
        LONG cDeletedSnapshots; 
        GUID nonDeletedSnapshotId; 
        pBackupComponents->DeleteSnapshots(snapshotSetId, VSS_OBJECT_SNAPSHOT_SET, TRUE, 
            &cDeletedSnapshots, &nonDeletedSnapshotId); 
    }
}

bool Confirm(LPCTSTR message)
{
    Console::Write(message); 
    Console::Write(TEXT(" Proceed? [y/N] ")); 
    TCHAR response = Console::ReadChar(); 

    return (response == TEXT('Y') || response == TEXT('y')); 
}

BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
        OutputWriter::WriteLine(TEXT("Ctrl-C event received. Shutting down."), VERBOSITY_THRESHOLD_NORMAL); 
        break;
    case CTRL_BREAK_EVENT:
        OutputWriter::WriteLine(TEXT("Ctrl-Break event received. Shutting down."), VERBOSITY_THRESHOLD_NORMAL); 
        break;
    case CTRL_CLOSE_EVENT:
        OutputWriter::WriteLine(TEXT("Application is being closed. Shutting down."), VERBOSITY_THRESHOLD_NORMAL); 
        break;
    case CTRL_LOGOFF_EVENT:
        OutputWriter::WriteLine(TEXT("User is logging off. Shutting down."), VERBOSITY_THRESHOLD_NORMAL); 
        break;
    case CTRL_SHUTDOWN_EVENT:
        OutputWriter::WriteLine(TEXT("System is shutting down. Terminating copy."), VERBOSITY_THRESHOLD_NORMAL); 
        break;
    }

    s_cancel = true; 

    return TRUE; 

}
void DeleteRecursive(LPCTSTR target, wregex* ignorePattern)
{
    if (!Utilities::DirectoryExists(target))
    {
        CString message; 
        message.Format(TEXT("Cannot delete directory %s because it does not exist. Proceeding anyway."), 
            target); 
        OutputWriter::WriteLine(message, VERBOSITY_THRESHOLD_NORMAL); 
        return; 
    }

    CDeleteAction deleteAction(target); 
    ProcessDirectory(target, deleteAction, TEXT(""), true, ignorePattern); 
}
void ProcessDirectory(LPCTSTR srcbase, CDirectoryAction& action, LPCTSTR directory, bool recursive, wregex* ignorePattern)
{
    WIN32_FIND_DATA findData;
    HANDLE hFindHandle;

    CString srcdir;
    Utilities::CombinePath(srcbase, directory, srcdir);

	if (!ShouldProcess(ignorePattern, srcdir)) 
    {
        return;
    }

    action.VisitDirectoryInitial(directory); 

    CString pattern;
    Utilities::CombinePath(srcdir, TEXT("*"), pattern);

	Utilities::FixLongFilenames(pattern);

    hFindHandle = ::FindFirstFile(pattern, &findData);
    if (hFindHandle != INVALID_HANDLE_VALUE)
    {
		BOOL done = false;
        while (!done)
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                if (recursive)
                {
                    if (Utilities::AreEqual(findData.cFileName, L".") || Utilities::AreEqual(findData.cFileName, L".."))
                    {
                        // Do nothing
                    }
                    else
                    {
                        CString subdirectory;
                        Utilities::CombinePath(directory, findData.cFileName, subdirectory);
						if (!ShouldProcess(ignorePattern, srcbase, subdirectory)) 
                        {
                            continue;
                        }
						ProcessDirectory(srcbase, action, subdirectory, recursive, ignorePattern);
                    }
                }
            }
            else
            {
                CString file; 
                Utilities::CombinePath(directory, findData.cFileName, file);
				if (!ShouldProcess(ignorePattern, srcbase, file)) continue;
                action.VisitFile(file); 
            }

			BOOL worked = ::FindNextFile(hFindHandle, &findData);

			if (!worked)
			{
				int error = GetLastError(); 

				if (error == ERROR_NO_MORE_FILES)
				{
					done = true; 
				}
				else
				{
					CString errorMessage; 
					Utilities::FormatErrorMessage(error, errorMessage); 
					CString message; 
					message.AppendFormat(TEXT("There was an error calling FindNextFile. Path: %s Error: %s"), 
						pattern, errorMessage); 
					throw new CVSSCopyException(message.GetString()); 
				}
			}
        } 
    }
	else 
	{
		int error = GetLastError(); 

		if (error != ERROR_FILE_NOT_FOUND)
		{
            CString errorMessage; 
            Utilities::FormatErrorMessage(error, errorMessage); 
            CString message; 
            message.AppendFormat(TEXT("There was an error calling FindFirstFile. Path: %s Error: %s"), 
                pattern, errorMessage); 
            throw new CVSSCopyException(message.GetString()); 
		}
	}
    ::FindClose(hFindHandle);


    // Important to put this after FindClose, since otherwise there's still an 
    // open handle to the directory, and that can interfere with (e.g.) directory
    // deletion
    action.VisitDirectoryFinal(directory); 

}

bool ShouldAddComponent(CWriterComponent& component)
{
    // Component should not be added if
    // 1) It is not selectable for backup and 
    // 2) It has a selectable ancestor
    // Otherwise, add it. 

    if (component.get_SelectableForBackup())
    {
        return true; 
    }

    return !component.get_HasSelectableAncestor();

}

bool IsMatch(wregex* pattern, const CString& input)
{
	return regex_match(input.GetString(), *pattern); 
}

bool ShouldProcess(wregex* ignorePattern, const CString& path) 
{
	if (!ignorePattern)
    {
        return true;
    }
	bool ignore = IsMatch(ignorePattern, path);
	if (ignore)
    {
		CString message;
		message.Format(TEXT("Ignoring %s"), path);
		OutputWriter::WriteLine(message); 
	}
	return !ignore;
}

bool ShouldProcess(wregex* ignorePattern, const CString& directory, const CString& name) 
{
	CString fullPath;
	Utilities::CombinePath(directory, name, fullPath);
	return ShouldProcess(ignorePattern, fullPath);
}