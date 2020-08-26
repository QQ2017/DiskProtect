#include "stdafx.h"
#include "DiskLib.h"
#include "Packet.h"
#include "openssl\sha.h"
#include <string>
#include <vector>

//�˺������ڼ���ptr�е�SHA1 Hashֵ��������������20�ֽڣ�����dm�С�
void Sha1(PCHAR ptr, size_t len, PBYTE dm)
{
	SHA_CTX context;
	SHA1_Init(&context);
	SHA1_Update(&context, (PBYTE)ptr, len);
	SHA1_Final(dm, &context);
}

BOOL IsDirectory(PCHAR szPath)
{
	WIN32_FIND_DATA stData;
	CHAR szNewPath[MAX_PATH];
	strcpy(szNewPath, szPath);
	if (szNewPath[strlen(szNewPath) - 1] == '\\')
	{
		szNewPath[strlen(szNewPath) - 1] = '\0';
	}
	HANDLE hSearch = ::FindFirstFile(szNewPath, &stData);
	if (hSearch == INVALID_HANDLE_VALUE)
	{
		LOG_ERROR(0, "IsDirectory:�����ļ�ʧ��(FindFirstFile)(%s)", szNewPath)
		return TRUE;
	}
	FindClose(hSearch);
	if (stData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{	//��Ŀ¼
		return TRUE;
	}
	return FALSE;
}

ULONG GetDirectorySizeAndFileCount(PCHAR lpPath, LARGE_INTEGER & DirSize, ULONG & FileCount)
{
	ULONG uError = ERROR_SUCCESS;
	WIN32_FIND_DATA stData;
	CHAR szNewPath[MAX_PATH];
	CHAR szPath[MAX_PATH];
	strcpy(szNewPath, lpPath);
	if (szNewPath[strlen(szNewPath) - 1] != '\\')
	{
		strcat(szNewPath, "\\");
	}
	strcpy(szPath, szNewPath);
	strcat(szNewPath, "*.*");
	HANDLE hSearch = ::FindFirstFile(szNewPath, &stData);
	if (hSearch == INVALID_HANDLE_VALUE) 
	{
		uError = GetLastError();
		LOG_ERROR(uError, "GetDirectorySizeAndFileCount:�����ļ�ʧ��(FindFirstFile)(%s)", szNewPath)
		return uError;
	}
	do
	{
		if (!strcmp(stData.cFileName, "..") || !strcmp(stData.cFileName, ".")) { continue; }
		if (stData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{	//��Ŀ¼
			strcpy(szNewPath, szPath);
			if (szNewPath[strlen(szNewPath) - 1] != '\\') { strcat(szNewPath, "\\"); }
			strcat(szNewPath, stData.cFileName);
			strcat(szNewPath, "\\");
			uError = GetDirectorySizeAndFileCount(szNewPath, DirSize, FileCount);
			if (uError != ERROR_SUCCESS)
			{
				break;
			}
		}
		else
		{	//���ļ�
			if (stricmp(stData.cFileName, INDEX_FILE_NAME) == 0)
			{
				continue;
			}
			strcpy(szNewPath, szPath);
			strcat(szNewPath, stData.cFileName);
			HANDLE hFile = CreateFile(szNewPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE) 
			{ 
				uError = GetLastError();
				LOG_ERROR(uError, "GetDirectorySizeAndFileCount:���ļ�ʧ��(CreateFile)(%s)", szNewPath)
				break;
			}
			LARGE_INTEGER FileSize;
			if (!GetFileSizeEx(hFile, &FileSize))
			{
				CloseHandle(hFile);
				uError = GetLastError();
				LOG_ERROR(uError, "GetDirectorySizeAndFileCount:��ȡ�ļ���Сʧ��(GetFileSizeEx)(%s)", szNewPath)
				break;
			}
			DirSize.QuadPart += FileSize.QuadPart;
			FileCount++;
			CloseHandle(hFile);
		}
	}
	while (::FindNextFile(hSearch, &stData));
	FindClose(hSearch);
	return uError;
}

ULONG AddSingleFile(PCHAR szPath, PBYTE & Offset, ULONG uBaseLength, ULONG & IndexFileSize, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext, LARGE_INTEGER & uProgress)
{
	ULONG uError = ERROR_SUCCESS;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	__try
	{
		LARGE_INTEGER FileSize;
		hFile = CreateFile(szPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			uError = GetLastError();
			LOG_ERROR(uError, "AddSingleFile:���ļ�ʧ��(CreateFile)(%s)", szPath)
			__leave;
		}
		*((PBYTE)Offset) = strlen(&szPath[uBaseLength]);
		Offset += sizeof(BYTE);
		IndexFileSize += sizeof(BYTE);
		strcpy((PCHAR)Offset, &szPath[uBaseLength]);
		Offset += (strlen(&szPath[uBaseLength]) + sizeof('\0'));
		IndexFileSize += (strlen(&szPath[uBaseLength]) + sizeof('\0'));
		if (!GetFileSizeEx(hFile, &FileSize))
		{
			uError = GetLastError();
			LOG_ERROR(uError, "AddSingleFile:��ȡ�ļ���Сʧ��(GetFileSizeEx)(%s)", szPath)
			__leave;
		}
		*((PLARGE_INTEGER)Offset) = FileSize;
		Offset += sizeof(LARGE_INTEGER);
		IndexFileSize += sizeof(LARGE_INTEGER);
		if (!GetFileTime(hFile, NULL, NULL, (PFILETIME)Offset))
		{
			uError = GetLastError();
			LOG_ERROR(uError, "AddSingleFile:��ȡ�ļ�ʱ��ʧ��(GetFileTime)(%s)", szPath)
			__leave;
		}
		Offset += sizeof(FILETIME);
		IndexFileSize += sizeof(FILETIME);
		for (; FileSize.QuadPart != 0; )
		{ 
			static CHAR Buffer[dwPieceSize];
			ULONG Readed;
			if (!ReadFile(hFile, Buffer, dwPieceSize, &Readed, NULL))
			{
				uError = GetLastError();
				LOG_ERROR(uError, "AddSingleFile:�ļ���ȡʧ��(ReadFile)(%s)", szPath)
				__leave;
			}
			Sha1(Buffer, Readed, Offset);
			Offset += SHA_HASH_LENGTH;
			IndexFileSize += SHA_HASH_LENGTH;
			FileSize.QuadPart -= Readed;
			uProgress.QuadPart += Readed;
			ReportRoutine(ReportContext, PROGRESS_REPORT_DISP, szPath, uProgress); //�������
		}
	}
	__finally
	{
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
		}
	}
	return uError;
}

ULONG AddMultiFile(PCHAR lpPath, PBYTE & Offset, ULONG uBaseLength, ULONG & IndexFileSize, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext, LARGE_INTEGER & uProgress)
{
	ULONG uError = ERROR_SUCCESS;
	WIN32_FIND_DATA stData;
	CHAR szNewPath[MAX_PATH];
	CHAR szPath[MAX_PATH];
	strcpy(szNewPath, lpPath);
	if (szNewPath[strlen(szNewPath) - 1] != '\\') 
	{ 
		strcat(szNewPath, "\\");
	}
	strcpy(szPath, szNewPath);
	strcat(szNewPath, "*.*");
	HANDLE hSearch = ::FindFirstFile(szNewPath, &stData);
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		uError = GetLastError();
		LOG_ERROR(uError, "AddMultiFile:�����ļ�ʧ��(FindFirstFile)(%s)", szNewPath)
		return uError;
	}
	do
	{
		if (!strcmp(stData.cFileName, "..") || !strcmp(stData.cFileName, ".")) { continue; }
		if (stData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{	//��Ŀ¼
			strcpy(szNewPath, szPath);
			if (szNewPath[strlen(szNewPath) - 1] != '\\') { strcat(szNewPath, "\\"); }
			strcat(szNewPath, stData.cFileName);
			strcat(szNewPath, "\\");
			uError = AddMultiFile(szNewPath, Offset, uBaseLength, IndexFileSize, ReportRoutine, ReportContext, uProgress);
			if (uError != ERROR_SUCCESS)
			{
				break;
			}
		}
		else
		{	//���ļ�
			if (stricmp(stData.cFileName, INDEX_FILE_NAME) == 0)
			{
				continue;
			}
			strcpy(szNewPath, szPath);
			strcat(szNewPath, stData.cFileName);
			uError = AddSingleFile(szNewPath, Offset, uBaseLength, IndexFileSize, ReportRoutine, ReportContext, uProgress);
			if (uError != ERROR_SUCCESS)
			{
				LOG_ERROR(uError, "AddMultiFile:����ļ�������Ϣʧ��(AddSingleFile)(%s)", szNewPath)
				break;
			}
		}
	}
	while (::FindNextFile(hSearch, &stData));
	FindClose(hSearch);
	return uError;
}

/***********************************************************/
/* ����һ: Ҫ���������ļ���Ŀ¼���ļ���·��                */
/* ������: ���ɵ������ļ����·��                          */
/***********************************************************/
ULONG CreateIndexFile(PCHAR PathFileName, PCHAR TargetIndexFile, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	LARGE_INTEGER DirSize = {0};
	ULONG FileCount = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMap = NULL;
	PBYTE BeginOffset = NULL;
	ULONG uBaseLength = 0;
	ULONG IndexFileSize  = 0;
	ULONG uError = ERROR_SUCCESS;
	PBYTE Offset = NULL;
	LARGE_INTEGER uProgress = {0};
	CHAR SourcePathName[MAX_PATH];
	strcpy(SourcePathName, PathFileName);
	if (SourcePathName[strlen(SourcePathName) - 1] == '\\')
	{
		SourcePathName[strlen(SourcePathName) - 1] = '\0';
	}
	if (SourcePathName[1] != ':' || SourcePathName[2] != '\\') // ��֤·����ȷ��
	{
		LOG_ERROR(ERROR_INVALID_PARAMETER, "CreateIndexFile:����·�������Ϲ淶", NULL)
		return ERROR_INVALID_PARAMETER;
	}
	__try
	{
		hFile = CreateFile(TargetIndexFile, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) 
		{ 
			uError = GetLastError();
			LOG_ERROR(uError, "CreateIndexFile:���������ļ�ʧ��(CreateFile)", NULL)
			__leave;
		}
		hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, dwBytesOfMega * 10, NULL);
		if (hMap == NULL)
		{ 
			uError = GetLastError();
			LOG_ERROR(uError, "CreateIndexFile:���������ļ�ʧ��(CreateFileMapping)", NULL)
			__leave;
		}
		BeginOffset = (PBYTE)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (BeginOffset == NULL)
		{ 
			uError = GetLastError();
			LOG_ERROR(uError, "CreateIndexFile:���������ļ�ʧ��(MapViewOfFile)", NULL)
			__leave;
		}
		Offset = BeginOffset + sizeof(PACKET_RESERVE);
		IndexFileSize += sizeof(PACKET_RESERVE);
		((PPACKET_RESERVE)BeginOffset)->PacketMagic = PacketMagicFlag;
		((PPACKET_RESERVE)BeginOffset)->PacketVersion = CurrentVersion;
		if (IsDirectory(SourcePathName))
		{
			uError = GetDirectorySizeAndFileCount(SourcePathName, DirSize, FileCount);
			if (uError != ERROR_SUCCESS)
			{
				LOG_ERROR(uError, "CreateIndexFile:��ȡĿ¼��Сʧ��(GetDirectorySizeAndFileCount)", NULL)
				__leave;
			}
			((PPACKET_RESERVE)BeginOffset)->PacketFileCount = FileCount;
			((PPACKET_RESERVE)BeginOffset)->PacketFileMaxSize = DirSize;
			strcpy(((PPACKET_RESERVE)BeginOffset)->RootDirName, strrchr(SourcePathName, '\\') + 1);
			uBaseLength = strlen(SourcePathName) + 1;
			ReportRoutine(ReportContext, PROGRESS_REPORT_START, SourcePathName, DirSize); //��ʼ�������
			uError = AddMultiFile(SourcePathName, Offset, uBaseLength, IndexFileSize, ReportRoutine, ReportContext, uProgress);
			ReportRoutine(ReportContext, PROGRESS_REPORT_END, NULL, LARGE_INTEGER()); //�����������
			if (uError != ERROR_SUCCESS)
			{
				LOG_ERROR(uError, "CreateIndexFile:����ļ�������Ϣʧ��(AddMultiFile)", NULL)
				__leave;
			}
		}
		else
		{
			HANDLE hFile = CreateFile(SourcePathName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE) 
			{
				uError = GetLastError();
				__leave;
			}
			LARGE_INTEGER FileSize;
			if (!GetFileSizeEx(hFile, &FileSize))
			{
				CloseHandle(hFile);
				uError = GetLastError();
				__leave;
			}
			CloseHandle(hFile);
			((PPACKET_RESERVE)BeginOffset)->PacketFileCount = 1;
			((PPACKET_RESERVE)BeginOffset)->PacketFileMaxSize = FileSize;
			memset(((PPACKET_RESERVE)BeginOffset)->RootDirName, 0, sizeof(((PPACKET_RESERVE)BeginOffset)->RootDirName));
			uBaseLength = 0;
			ReportRoutine(ReportContext, PROGRESS_REPORT_START, SourcePathName, FileSize); //��ʼ�������
			uError = AddSingleFile(SourcePathName, Offset, uBaseLength, IndexFileSize, ReportRoutine, ReportContext, uProgress);
			ReportRoutine(ReportContext, PROGRESS_REPORT_END, NULL, LARGE_INTEGER()); //�����������
			if (uError != ERROR_SUCCESS)
			{
				__leave;
			}
		}
		Sha1((PCHAR)BeginOffset, IndexFileSize, Offset);
		Offset += SHA_HASH_LENGTH;
		IndexFileSize += SHA_HASH_LENGTH;
		FlushViewOfFile(BeginOffset, IndexFileSize);
		UnmapViewOfFile(BeginOffset); BeginOffset = NULL;
		CloseHandle(hMap); hMap = NULL;
		uError = SetFilePointer(hFile, IndexFileSize, NULL, FILE_BEGIN);
		if (uError != IndexFileSize)
		{
			uError = GetLastError();
			LOG_ERROR(uError, "CreateIndexFile:�����ļ�ָ��ʧ��(SetFilePointer)", NULL)
			__leave;
		}
		if (!SetEndOfFile(hFile))
		{
			uError = GetLastError();
			LOG_ERROR(uError, "CreateIndexFile:�ض��ļ�ʧ��(SetEndOfFile)", NULL)
			__leave;
		}
		CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
		uError = ERROR_SUCCESS;
	}
	__finally
	{
		if (BeginOffset != NULL)
		{
			UnmapViewOfFile(BeginOffset);
		}
		if (hMap != NULL)
		{
			CloseHandle(hMap);
		}
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
		}
		if (uError != ERROR_SUCCESS)
		{
			DeleteFile(TargetIndexFile);
		}
	}
	return uError;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// �����ļ���С
ULONG SetFileSize(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
	LARGE_INTEGER FileSize = *lpFileSize;
	if(
		(SetFilePointer(hFile, FileSize.LowPart, &FileSize.HighPart, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
		&& 
		(SetEndOfFile(hFile))
		)
	{
		return ERROR_SUCCESS;
	}
	return GetLastError();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// �ص�IOд
ULONG FileOverlapWrite(HANDLE hFile, LPVOID lpBuffer, PLARGE_INTEGER lpOffset, ULONG nNumberOfBytes, HANDLE hEvent)
{
	OVERLAPPED Overlapped;
	ULONG NumberOfBytesWritten = 0;
	ULONG dwError = ERROR_SUCCESS;
	Overlapped.hEvent       = hEvent;
	Overlapped.Offset       = lpOffset->LowPart;
	Overlapped.OffsetHigh   = lpOffset->HighPart;
	Overlapped.Internal     = 0;
	Overlapped.InternalHigh = 0;
	if(!WriteFile(hFile, lpBuffer, nNumberOfBytes, &NumberOfBytesWritten, &Overlapped))
	{
		if((dwError = GetLastError()) == ERROR_IO_PENDING)
		{
			WaitForSingleObject(hEvent, INFINITE);
			if(!GetOverlappedResult(hFile, &Overlapped, &NumberOfBytesWritten, FALSE))
			{
				dwError = GetLastError();
			}
			else
			{
				dwError = ERROR_SUCCESS;
			}
		}
	}
	ResetEvent(hEvent);
	return dwError;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// �ص�IO��
ULONG FileOverlapRead (HANDLE hFile, LPVOID lpBuffer, PLARGE_INTEGER lpOffset, ULONG nNumberOfBytes, PULONG NumberOfBytesRead, HANDLE hEvent)
{
	OVERLAPPED Overlapped;
	ULONG dwError = ERROR_SUCCESS;
	Overlapped.hEvent       = hEvent;
	Overlapped.Offset       = lpOffset->LowPart;
	Overlapped.OffsetHigh   = lpOffset->HighPart;
	Overlapped.Internal     = 0;
	Overlapped.InternalHigh = 0;
	if(!ReadFile(hFile, lpBuffer, nNumberOfBytes, NumberOfBytesRead, &Overlapped))
	{
		if((dwError = GetLastError()) == ERROR_IO_PENDING)
		{
			WaitForSingleObject(hEvent, INFINITE);
			if(!GetOverlappedResult(hFile, &Overlapped, NumberOfBytesRead, FALSE))
			{
				dwError = GetLastError();
			}
			else
			{
				dwError = ERROR_SUCCESS;
			}
		}
	}
	ResetEvent(hEvent);
	return dwError;
}

//-------------------------------------------------------------------------------------
//Description:
// This function maps a wide-character string to a new character string
//
//Parameters:
// lpcwszStr: [in] Pointer to the character string to be converted 
// lpszStr: [out] Pointer to a buffer that receives the translated string. 
// dwSize: [in] Size of the buffer
//
//Return Values:
// TRUE: Succeed
// FALSE: Failed
// 
//Example:
// MByteToWChar(szW,szA,sizeof(szA)/sizeof(szA[0]));
//---------------------------------------------------------------------------------------
BOOL WCharToMByte(LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize)
{
	DWORD dwMinSize;
	dwMinSize = WideCharToMultiByte(CP_OEMCP,NULL,lpcwszStr,-1,NULL,0,NULL,FALSE);
	if(dwSize < dwMinSize)
	{
		return FALSE;
	}
	WideCharToMultiByte(CP_OEMCP,NULL,lpcwszStr,-1,lpszStr,dwSize,NULL,FALSE);
	return TRUE;
}

BOOL QueryDeviceName(CHAR cDriverLetter, PCHAR lpDeviceName)
{
	DWORD  bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	DWORD BytesReturned = 0;
	CHAR VolumeName[] = "\\\\.\\ :";
	VolumeName[4] = cDriverLetter;
	__try
	{
		WCHAR wDeviceName[MAX_PATH];
		CHAR  szDeviceName[MAX_PATH];
		hDevice = CreateFile(VolumeName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		if (!DeviceIoControl(hDevice, IOCTL_QUERY_TW_DEVICE_NAME, NULL, 0, wDeviceName, sizeof(wDeviceName), &BytesReturned, NULL))
		{
			__leave;
		}
		if(!WCharToMByte(wDeviceName, szDeviceName, sizeof(szDeviceName)/sizeof(szDeviceName[0])))
		{
			__leave;	
		}
		lstrcpy(lpDeviceName, szDeviceName);
		bResult = TRUE;
	}
	__finally
	{
		if (hDevice != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hDevice);
		}
	}
	return bResult;
}

BOOL TransformDevicePath(PCHAR GeneralPath, PCHAR DevicePath)
{
	CHAR szDeviceName[MAX_PATH];
	if (!QueryDeviceName(GeneralPath[0], szDeviceName))
	{
		return FALSE;
	}
	strcat(szDeviceName, strchr(GeneralPath, '\\'));
	strcpy(DevicePath, "\\\\.\\");
	strcat(DevicePath, &szDeviceName[strlen("\\DosDevices\\")]);
	return TRUE;
}
/*-------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------*/

CFolder::~CFolder()
{
	CloseFolder();
}

void CFolder::CloseFolder()
{
	if (m_IndexFileMap != NULL)
	{
		UnmapViewOfFile(m_IndexFileMap);
		m_IndexFileMap = NULL;
	}
	if (m_hMap != NULL)
	{
		CloseHandle(m_hMap);
		m_hMap = NULL;
	}
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	if (m_FileIndexInfo != NULL)
	{
		delete [] m_FileIndexInfo;
		m_FileIndexInfo = NULL;
	}
	if (m_PieceList != NULL)
	{
		delete [] m_PieceList;
		m_PieceList = NULL;
	}
	m_FileListMap.RemoveAll();
}

static int getcount(char* str, char ch)
{
	int n = 0;
	while(*str++ != NULL)
	{
		if((char)*str == ch) {++n;}
	}
	return n;
}

BOOL CFolder::Load(PCHAR Path)
{
	BOOL bResult = TRUE;
	strcpy(m_Path, Path);
	if (m_Path[strlen(m_Path) - 1] != '\\')
	{ 
		strcat(m_Path, "\\");
	}
	if (m_Path[1] != ':' || m_Path[2] != '\\') // ��֤·����ȷ��
	{
		m_LastError = ERROR_INVALID_PARAMETER;
		LOG_ERROR(m_LastError, "CFolder::Load: ����·�������Ϲ淶", NULL)
		return FALSE;
	}
	if (getcount(m_Path, '\\') < 2) // ��֤·����ȷ��,����Ҫ��2���ָ��
	{
		m_LastError = ERROR_INVALID_PARAMETER;
		LOG_ERROR(m_LastError, "CFolder::Load: ����·�������Ϲ淶", NULL)
		return FALSE;
	}
	m_LastError = ERROR_SUCCESS;
	__try
	{
		ULONG i;
		PBYTE CurOffset;
		ULONG uPieceCount;
		ULONG uPieceTotal;
		LARGE_INTEGER uFileSize;
		BYTE SHABuf[SHA_HASH_LENGTH];
		CHAR szIndexFileName[MAX_PATH];
		strcpy(szIndexFileName, m_Path);
		strcat(szIndexFileName, INDEX_FILE_NAME);
		m_hFile = CreateFile(szIndexFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
		{ 
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::Load: �����ļ���ʧ��(CreateFile)(%s)", szIndexFileName)
			__leave;
		}
		if (!GetFileSizeEx(m_hFile, &uFileSize) && uFileSize.QuadPart <= sizeof(PACKET_RESERVE)+SHA_HASH_LENGTH)
		{
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::Load: ��ȡ�����ļ���Сʧ��(GetFileSizeEx)(%s)", szIndexFileName)
			__leave;
		}
		m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, uFileSize.LowPart, NULL);
		if (m_hMap == NULL)
		{ 
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::Load: �����ļ�ӳ��ʧ��(CreateFileMapping)(%s)", szIndexFileName)
			__leave;
		}
		m_IndexFileMap = (PBYTE)MapViewOfFile(m_hMap, FILE_MAP_READ, 0, 0, 0);
		if (m_IndexFileMap == NULL)
		{ 
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::Load: �����ļ�ӳ��ʧ��(MapViewOfFile)(%s)", szIndexFileName)
			__leave;
		}
		m_IndexFileSize = uFileSize.LowPart;
		Sha1((PCHAR)m_IndexFileMap, (size_t)(uFileSize.LowPart - SHA_HASH_LENGTH), SHABuf);
		if (memcmp(m_IndexFileMap + uFileSize.LowPart - SHA_HASH_LENGTH, SHABuf, SHA_HASH_LENGTH) != 0)
		{
			m_LastError = ERROR_ACCESS_DENIED;
			LOG_ERROR(m_LastError, "CFolder::Load: �����ļ�У��ʧ��(%s)", szIndexFileName)
			__leave;
		}
		//�Ƚϸ�Ŀ¼���ƣ�ħ����־���汾��,Ŀ¼�ܴ�С�������ڽ���

		if (
			((PPACKET_RESERVE)m_IndexFileMap)->PacketMagic != PacketMagicFlag  ||
			((PPACKET_RESERVE)m_IndexFileMap)->PacketVersion != CurrentVersion ||
			strstr(m_Path, ((PPACKET_RESERVE)m_IndexFileMap)->RootDirName) == NULL
			)
		{
			m_LastError = ERROR_ACCESS_DENIED;
			LOG_ERROR(m_LastError, "CFolder::Load: �����ļ���ƥ��(%s)", szIndexFileName)
			__leave;
		}
		strcpy(m_Name, ((PPACKET_RESERVE)m_IndexFileMap)->RootDirName);
		m_FileCount = ((PPACKET_RESERVE)m_IndexFileMap)->PacketFileCount;
		m_FileIndexInfo = new FILE_INDEX_INFO[m_FileCount];
		if (m_FileIndexInfo == NULL)
		{
			m_LastError = ERROR_NOT_ENOUGH_MEMORY;
			LOG_ERROR(m_LastError, "CFolder::Load: �����ڴ�ʧ��(%s)", m_Path)
			__leave;
		}
		uPieceCount = 0;
		uPieceTotal = 0;
		CurOffset = m_IndexFileMap + sizeof(PACKET_RESERVE);
		for(i = 0; i < m_FileCount; i++)
		{
			m_FileIndexInfo[i].FilePathName = (PCHAR)CurOffset + sizeof(BYTE);
			CurOffset += (sizeof(BYTE) + CurOffset[0] + sizeof('\0')); //Ҫ��֤�ļ�������
			m_FileIndexInfo[i].FileSize = (PLARGE_INTEGER)CurOffset;
			CurOffset += sizeof(LARGE_INTEGER);
			m_FileIndexInfo[i].FileTime = (PFILETIME)CurOffset;
			CurOffset += sizeof(FILETIME);
			m_FileIndexInfo[i].Sha1 = (m_FileIndexInfo[i].FileSize->QuadPart != 0 ? CurOffset : NULL);
			uPieceCount = 	
				(m_FileIndexInfo[i].FileSize->QuadPart/dwPieceSize + (m_FileIndexInfo[i].FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));
			uPieceTotal += uPieceCount;
			CurOffset += uPieceCount * SHA_HASH_LENGTH;
		}
		m_PieceList = new ULONG[uPieceTotal];
		if (m_PieceList == NULL)
		{
			m_LastError = ERROR_NOT_ENOUGH_MEMORY;
			LOG_ERROR(m_LastError, "CFolder::Load: �����ڴ�ʧ��(2)(%s)", m_Path)
			__leave;
		}
		m_FileListMap.InitHashTable(m_FileCount, TRUE);
		for(i = 0; i < m_FileCount; i++)
		{
			m_FileListMap[m_FileIndexInfo[i].FilePathName] = &m_FileIndexInfo[i];
		}
	}
	__finally
	{
		if (m_LastError != ERROR_SUCCESS)
		{
			if (m_IndexFileMap != NULL)
			{
				UnmapViewOfFile(m_IndexFileMap);
				m_IndexFileMap = NULL;
			}
			if (m_hMap != NULL)
			{
				CloseHandle(m_hMap);
				m_hMap = NULL;
			}
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}
			if (m_FileIndexInfo != NULL)
			{
				delete [] m_FileIndexInfo;
				m_FileIndexInfo = NULL;
			}
			if (m_PieceList != NULL)
			{
				delete [] m_PieceList;
				m_PieceList = NULL;
			}
			bResult = FALSE;
		}
	}
	return bResult;
}

ULONG CFolder::GetLastErrorCode(void)
{
	return m_LastError;
}

void  CFolder::AttachCallBack(PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	m_ReportRoutine = ReportRoutine;
	m_ReportContext = ReportContext;
}

//ֻ�Ƚ�2��Ŀ¼�е������ļ�,�Ƚ����м�¼���ļ���ʱ��,��С,��SHA1
PFILE_INDEX_INFO CFolder::CompareWith(CFolder & DestFolder)
{
	PVOID Object;
	PFILE_INDEX_INFO DestFileIndexInfo;
	PFILE_INDEX_INFO SourceFileIndexInfo;
	PFILE_INDEX_INFO lpResult = NULL;
	PULONG PieceList = m_PieceList;
	for(ULONG i = 0; i < m_FileCount; i++)
	{
		SourceFileIndexInfo = &m_FileIndexInfo[i];
		SourceFileIndexInfo->PieceList  = NULL;
		SourceFileIndexInfo->PieceCount = 0;

		if (DestFolder.m_FileListMap.Lookup(SourceFileIndexInfo->FilePathName, Object))
		{
			DestFileIndexInfo = (PFILE_INDEX_INFO)Object;
			//�ǲ��ǻ�Ҫ�ȱȽ�ʱ��ʹ�С
			ULONG SourcePieceCount = 
				(SourceFileIndexInfo->FileSize->QuadPart/dwPieceSize + (SourceFileIndexInfo->FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));
			ULONG DestPieceCount = 
				(DestFileIndexInfo->FileSize->QuadPart/dwPieceSize + (DestFileIndexInfo->FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));
			for (ULONG j = 0; j < SourcePieceCount; j++) //�������п�
			{
				if (j < DestPieceCount) //Ŀ���ļ���Ҳ�������,�Ƚ���
				{
					if (
						memcmp(SourceFileIndexInfo->Sha1+j*SHA_HASH_LENGTH, DestFileIndexInfo->Sha1+j*SHA_HASH_LENGTH, SHA_HASH_LENGTH) != 0
						)
					{
						PieceList[SourceFileIndexInfo->PieceCount] = j;
						SourceFileIndexInfo->PieceCount++;
					}
				}
				else //Ŀ���ļ���û�����
				{
					PieceList[SourceFileIndexInfo->PieceCount] = j;
					SourceFileIndexInfo->PieceCount++;
				}
			}
			if (SourceFileIndexInfo->PieceCount != 0) //����ļ���Ҫ��������
			{
				SourceFileIndexInfo->PieceList = PieceList;
				PieceList += SourceFileIndexInfo->PieceCount;
				SourceFileIndexInfo->FileExist = TRUE; //��־����ļ�����
				SourceFileIndexInfo->RealFileSize = *DestFileIndexInfo->FileSize;//�ļ�ʵ�ʴ�С
			}
		}
		else //�ļ�������
		{
			ULONG SourcePieceCount = 
				(SourceFileIndexInfo->FileSize->QuadPart/dwPieceSize + (SourceFileIndexInfo->FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));
			for (ULONG j = 0; j < SourcePieceCount; j++) //�������п�
			{
				PieceList[SourceFileIndexInfo->PieceCount] = j;
				SourceFileIndexInfo->PieceCount++;
			}
			SourceFileIndexInfo->PieceList = PieceList;
			PieceList += SourceFileIndexInfo->PieceCount;
			SourceFileIndexInfo->FileExist = FALSE; //��־����ļ�������
			SourceFileIndexInfo->RealFileSize.QuadPart = 0;//ʵ�ʴ�СΪ0
		}
		if (SourceFileIndexInfo->PieceCount != 0) //����ļ���Ҫ��������
		{
			SourceFileIndexInfo->Next = NULL;
			if (lpResult == NULL)
			{
				lpResult = SourceFileIndexInfo;
			}
			else
			{
				SourceFileIndexInfo->Next = lpResult;
				lpResult = SourceFileIndexInfo;
			}
		}
	}
	return lpResult;
}

//��ԴĿ¼�е������ļ�ȥУ��Ŀ��Ŀ¼�е�ʵ���ļ�(����Ŀ��Ŀ¼�е������ļ�),�Ƚϴ�С,ʱ��,SHA1
PFILE_INDEX_INFO CFolder::RepairWith(CFolder & DestFolder)
{
	LARGE_INTEGER uProgressCount;
	LARGE_INTEGER uMaxPieceCount;
	PFILE_INDEX_INFO lpResult = NULL;
	PFILE_INDEX_INFO SourceFileIndexInfo = NULL;
	HANDLE hEvent = NULL;
	PVOID DataBuffer = NULL;
	PULONG PieceList = m_PieceList;

	uMaxPieceCount.QuadPart = 0;
	for(ULONG i = 0; i < m_FileCount; i++) //���������ļ�
	{
		SourceFileIndexInfo = &m_FileIndexInfo[i];
		uMaxPieceCount.QuadPart += (SourceFileIndexInfo->FileSize->QuadPart/dwPieceSize + (SourceFileIndexInfo->FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));
	}
	__try
	{
		DataBuffer = malloc(dwPieceSize);
		if (DataBuffer == NULL)
		{
			m_LastError = ERROR_NOT_ENOUGH_MEMORY;
			LOG_ERROR(m_LastError, "CFolder::RepairWith: malloc Fail", NULL)
			__leave;
		}
		// �����ļ�IO������Event
		if((hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
		{
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::RepairWith: CreateEvent Fail", NULL)
			__leave;
		}
		//��ʼ�����ļ�У�����
		m_ReportRoutine(m_ReportContext, CHECKPROGRESS_REPORT_START, m_Name, uMaxPieceCount);
		uProgressCount.QuadPart = 0; //������0
		for(ULONG i = 0; i < m_FileCount; i++) //���������ļ�
		{
			HANDLE hFile = INVALID_HANDLE_VALUE;
			ULONG SourcePieceCount = 0;
			CHAR szFilePath[MAX_PATH];
			__try
			{
				SourceFileIndexInfo = &m_FileIndexInfo[i];
				SourceFileIndexInfo->PieceList  = NULL;
				SourceFileIndexInfo->PieceCount = 0;

				//��������ļ��ж��ٸ���
				SourcePieceCount = (SourceFileIndexInfo->FileSize->QuadPart/dwPieceSize + (SourceFileIndexInfo->FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));

				strcpy(szFilePath, DestFolder.m_Path);
				strcat(szFilePath, SourceFileIndexInfo->FilePathName);

				if((hFile = CreateFile(szFilePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
				{	//�ļ���ʧ��,���߲�����
					for (ULONG j = 0; j < SourcePieceCount; j++) //�������п�
					{
						PieceList[SourceFileIndexInfo->PieceCount] = j;
						SourceFileIndexInfo->PieceCount++;
						//�����ļ�У�����
						uProgressCount.QuadPart++;
						m_ReportRoutine(m_ReportContext, CHECKPROGRESS_REPORT_DISP, SourceFileIndexInfo->FilePathName, uProgressCount);
					}
					SourceFileIndexInfo->FileExist = FALSE; //��־����ļ�������
					SourceFileIndexInfo->RealFileSize.QuadPart = 0;//ʵ�ʴ�СΪ0
				}
				else	//�ļ��򿪳ɹ�
				{
					FILETIME FileTime;
					LARGE_INTEGER FileSize;
					if (
						GetFileSizeEx(hFile, &FileSize) && GetFileTime(hFile, NULL, NULL, &FileTime) &&
						SourceFileIndexInfo->FileSize->QuadPart == FileSize.QuadPart &&
						SourceFileIndexInfo->FileTime->dwLowDateTime == FileTime.dwLowDateTime &&
						SourceFileIndexInfo->FileTime->dwHighDateTime == FileTime.dwHighDateTime
						)
					{
						//�����ļ�У�����
						uProgressCount.QuadPart += SourcePieceCount;
						m_ReportRoutine(m_ReportContext, CHECKPROGRESS_REPORT_DISP, SourceFileIndexInfo->FilePathName, uProgressCount);

						continue; //����ļ����ļ�ʱ����ļ���С���ͼ�¼��һ��,����Ϊ����ļ�����Ҫ����
					}
					//�Ƚ�����ļ���Щ����Ҫ����
					for (ULONG j = 0; j < SourcePieceCount; j++) //�������п�,�ж���Щ��Ҫ����
					{
						BYTE Sha1Buf[128]; 
						ULONG NumberOfBytesRead;
						LARGE_INTEGER ByteOffset;
						ByteOffset.QuadPart = j * dwPieceSize;
						m_LastError = FileOverlapRead (hFile, DataBuffer, &ByteOffset, dwPieceSize, &NumberOfBytesRead, hEvent);
						if (m_LastError == ERROR_SUCCESS)
						{
							Sha1((PCHAR)DataBuffer, NumberOfBytesRead, Sha1Buf);
							if (memcmp(Sha1Buf, SourceFileIndexInfo->Sha1+j*SHA_HASH_LENGTH, SHA_HASH_LENGTH) != 0)
							{
								PieceList[SourceFileIndexInfo->PieceCount] = j;
								SourceFileIndexInfo->PieceCount++;
							}
						}
						else
						{
							PieceList[SourceFileIndexInfo->PieceCount] = j;
							SourceFileIndexInfo->PieceCount++;
						}
						//�����ļ�У�����
						uProgressCount.QuadPart ++;
						m_ReportRoutine(m_ReportContext, CHECKPROGRESS_REPORT_DISP, SourceFileIndexInfo->FilePathName, uProgressCount);
					}
					if (SourceFileIndexInfo->PieceCount != 0) //����ļ���Ҫ��������
					{
						SourceFileIndexInfo->FileExist = TRUE;			//��־����ļ�����
						SourceFileIndexInfo->RealFileSize = FileSize;	//�ļ�ʵ�ʴ�С
					}
				}
				if (SourceFileIndexInfo->PieceCount != 0) //����ļ���Ҫ��������
				{
					SourceFileIndexInfo->PieceList = PieceList;
					PieceList += SourceFileIndexInfo->PieceCount;

					SourceFileIndexInfo->Next = NULL;
					if (lpResult == NULL)
					{
						lpResult = SourceFileIndexInfo;
					}
					else
					{
						SourceFileIndexInfo->Next = lpResult;
						lpResult = SourceFileIndexInfo;
					}
				}
			}
			__finally
			{
				if (hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hFile);
				}
			}
		}
		m_ReportRoutine(m_ReportContext, PROGRESS_REPORT_END, NULL, LARGE_INTEGER());
	}
	__finally
	{
		if (hEvent != NULL)
		{
			CloseHandle(hEvent);
		}
		if (DataBuffer != NULL)
		{
			free(DataBuffer);
		}
	}
	return lpResult;
}

//ֱ�ӽ������ļ�������,������ȫ�����ļ�,�����καȽϹ���
PFILE_INDEX_INFO CFolder::ParseWith()
{
	PFILE_INDEX_INFO SourceFileIndexInfo;
	PFILE_INDEX_INFO lpResult = NULL;
	PULONG PieceList = m_PieceList;
	for(ULONG i = 0; i < m_FileCount; i++)
	{
		SourceFileIndexInfo = &m_FileIndexInfo[i];
		SourceFileIndexInfo->PieceList  = NULL;
		SourceFileIndexInfo->PieceCount = 0;
		{
			ULONG SourcePieceCount = 
				(SourceFileIndexInfo->FileSize->QuadPart/dwPieceSize + (SourceFileIndexInfo->FileSize->QuadPart%dwPieceSize == 0 ? 0 : 1));
			for (ULONG j = 0; j < SourcePieceCount; j++) //�������п�
			{
				PieceList[SourceFileIndexInfo->PieceCount] = j;
				SourceFileIndexInfo->PieceCount++;
			}
			SourceFileIndexInfo->PieceList = PieceList;
			PieceList += SourceFileIndexInfo->PieceCount;
		}
		if (SourceFileIndexInfo->PieceCount != 0) //����ļ���Ҫ��������
		{
			SourceFileIndexInfo->Next = NULL;
			if (lpResult == NULL)
			{
				lpResult = SourceFileIndexInfo;
			}
			else
			{
				SourceFileIndexInfo->Next = lpResult;
				lpResult = SourceFileIndexInfo;
			}
			SourceFileIndexInfo->FileExist = TRUE; //��־����ļ�����
			SourceFileIndexInfo->RealFileSize = *SourceFileIndexInfo->FileSize;//�ļ�ʵ�ʴ�С
		}
	}
	return lpResult;
}

bool createMultipleDirectory(const char* pszDir)
{
	std::string strDir(pszDir);//���Ҫ������Ŀ¼�ַ���
	std::vector<std::string> vPath;//���ÿһ��Ŀ¼�ַ���
	std::string strTemp;//һ����ʱ����,���Ŀ¼�ַ���
	bool bSuccess = false;//�ɹ���־

	std::string::const_iterator sIter;//�����ַ���������
	//����Ҫ�������ַ���
	for (sIter = strDir.begin(); sIter != strDir.end(); sIter++) {
		if (*sIter != '\\') {//�����ǰ�ַ�����'\\'
			strTemp += (*sIter);
		} else {//�����ǰ�ַ���'\\'
			vPath.push_back(strTemp);//����ǰ����ַ�����ӵ�������
			strTemp += '\\';
		}
	}

	//�������Ŀ¼������,����ÿ��Ŀ¼
	std::vector<std::string>::const_iterator vIter;
	for (vIter = vPath.begin(); vIter != vPath.end(); vIter++) {
		//���CreateDirectoryִ�гɹ�,����true,���򷵻�false
		bSuccess = CreateDirectory(vIter->c_str(), NULL) ? true : false;    
	}

	return bSuccess;
}

BOOL CFolder::CopyTo(CFolder & DestFolder, UPDATE_TYPE UpdateType)
{
	int i;
	LARGE_INTEGER uProgressSize;
	LARGE_INTEGER uFileSize;
	PFILE_INDEX_INFO FileIndexInfo = NULL;
	HANDLE hEvent = NULL;
	HANDLE hSourceFile = INVALID_HANDLE_VALUE;
	HANDLE hDestFile   = INVALID_HANDLE_VALUE;
	HANDLE hDestTWFile = INVALID_HANDLE_VALUE;
	PVOID DataBuffer   = NULL;
	BOOL bThroughWrite = FALSE;
	ULONG FileCount   = 0; // �ж��ٸ��ļ���Ҫ����
	ULONG uPieceTotal = 0; // �����ļ���Ҫ�����Ŀ���ܺ�
	PFILE_INDEX_INFO InfoHead = NULL;
	CHAR DevicePath[MAX_PATH];
	CHAR SourceFilePath[MAX_PATH];
	CHAR DestFilePath[MAX_PATH];
	CHAR DestFileTWPath[MAX_PATH];
	if (
		strstr(m_Path, DestFolder.m_Path) != NULL ||
		strstr(DestFolder.m_Path, m_Path) != NULL
		)
	{
		m_LastError = ERROR_ACCESS_DENIED;
		CHAR szErrorMsg[512];
		wsprintf(szErrorMsg, "(%s)(%s)", m_Path, DestFolder.m_Path);
		LOG_ERROR(m_LastError, "CFolder::CopyTo: ·���ص� %s", szErrorMsg)
		return FALSE;
	}
	switch(UpdateType)
	{
	case SNAPSHOT_UPDATE:
		if (stricmp(m_Name, DestFolder.m_Name) != 0)
		{
			m_LastError = ERROR_ACCESS_DENIED;
			CHAR szErrorMsg[512];
			wsprintf(szErrorMsg, "(%s)(%s)", m_Name, DestFolder.m_Name);
			LOG_ERROR(m_LastError, "CFolder::CopyTo: ����Ŀ¼���� %s", szErrorMsg)
			return FALSE;
		}
		InfoHead = CompareWith(DestFolder);	//�Ա������ļ�����
		break;
	case REPAIR_UPDATE:
		InfoHead = RepairWith(DestFolder);	//�޸�����(��ԴĿ¼�е������ļ�ȥ��֤Ŀ��Ŀ¼�е��ļ�)
	    break;
	case DIRECT_UPDATE:
		InfoHead = ParseWith();				//ֱ�Ӹ���(��ȫ����)
		break;
	}
	__try
	{
		// �����ļ�IO������Event
		if((hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
		{
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::CopyTo: CreateEvent Fail", NULL)
			__leave;
		}
		if (InfoHead == NULL) //2��Ŀ¼�����ļ�ʲô��һ��,����Ҫ����
		{
			m_LastError = ERROR_SUCCESS;
			LOG_ERROR(m_LastError, "CFolder::CopyTo:Ŀ¼��ȫ��ͬ,����Ҫ����", NULL)
			goto __CopyIndexFile;
		}
		for (FileIndexInfo = InfoHead; FileIndexInfo != NULL; FileIndexInfo = FileIndexInfo->Next) //ͳ���ж����ļ���Ҫ����,һ���ж��ٸ�����Ҫ����
		{
			FileCount++;
			uPieceTotal += FileIndexInfo->PieceCount;
		}
		DataBuffer = malloc(dwPieceSize);
		if (DataBuffer == NULL)
		{
			m_LastError = ERROR_NOT_ENOUGH_MEMORY;
			LOG_ERROR(m_LastError, "CFolder::CopyTo: malloc Fail", NULL)
			__leave;
		}
		bThroughWrite = TransformDevicePath(DestFolder.m_Path, DevicePath); //ת��Ϊ��͸д·��
		{	//׼���������
			LARGE_INTEGER uMaxSize;
			uMaxSize.QuadPart = uPieceTotal;
			m_ReportRoutine(m_ReportContext, PROGRESS_REPORT_START, m_Name, uMaxSize);
		}
		uProgressSize.QuadPart = 0; //���ȴ�С��0
		for (FileIndexInfo = InfoHead; FileIndexInfo != NULL; FileIndexInfo = FileIndexInfo->Next) //����Ҫд�������ļ�
		{
			strcpy(SourceFilePath, m_Path);
			strcat(SourceFilePath, FileIndexInfo->FilePathName);
			strcpy(DestFilePath, DestFolder.m_Path);
			strcat(DestFilePath, FileIndexInfo->FilePathName);
			if (bThroughWrite) //�����͸д
			{
				strcpy(DestFileTWPath, DevicePath);
				strcat(DestFileTWPath, FileIndexInfo->FilePathName);
			}
			__try
			{
				if((hSourceFile = CreateFile(SourceFilePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
				{
					m_LastError = GetLastError();
					LOG_ERROR(m_LastError, "CFolder::CopyTo: CreateFile[S](%s)", SourceFilePath)
					__leave;
				}
				createMultipleDirectory(DestFilePath);
				if((hDestFile = CreateFile(DestFilePath, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
				{
					m_LastError = GetLastError();
					LOG_ERROR(m_LastError, "CFolder::CopyTo: CreateFile[D](%s)", DestFilePath)
					__leave;
				}
				if (bThroughWrite) //�����͸д
				{
					createMultipleDirectory(DestFileTWPath);
					if((hDestTWFile = CreateFile(DestFileTWPath, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
					{
						m_LastError = GetLastError();
						LOG_ERROR(m_LastError, "CFolder::CopyTo: CreateFile[TW](%s)", DestFileTWPath)
						__leave;
					}
				}
				if (!GetFileSizeEx(hDestFile, &uFileSize))
				{
					m_LastError = GetLastError();
					LOG_ERROR(m_LastError, "CFolder::CopyTo: GetFileSizeEx(%s)", DestFilePath)
					__leave;
				}
				if (uFileSize.QuadPart > FileIndexInfo->FileSize->QuadPart)
				{
					SetFileSize(hDestFile, FileIndexInfo->FileSize); //�ļ����ľ�Ҫ�ض�
				}
				if (bThroughWrite) //�����͸д
				{
					if (!GetFileSizeEx(hDestTWFile, &uFileSize))
					{
						m_LastError = GetLastError();
						LOG_ERROR(m_LastError, "CFolder::CopyTo: GetFileSizeEx(%s)", DestFileTWPath)
						__leave;
					}
					if (uFileSize.QuadPart > FileIndexInfo->FileSize->QuadPart)
					{
						SetFileSize(hDestTWFile, FileIndexInfo->FileSize); //�ļ����ľ�Ҫ�ض�
					}
				}
				for (i = 0; i < FileIndexInfo->PieceCount; i++) //���������ļ���Ҫ���������п�
				{
					ULONG NumberOfBytesRead;
					LARGE_INTEGER ByteOffset;
					ByteOffset.QuadPart = FileIndexInfo->PieceList[i] * dwPieceSize;
					m_LastError = FileOverlapRead (hSourceFile, DataBuffer, &ByteOffset, dwPieceSize, &NumberOfBytesRead, hEvent);
					if (m_LastError != ERROR_SUCCESS)
					{
						LOG_ERROR(m_LastError, "CFolder::CopyTo: FileOverlapRead(%s)", SourceFilePath)
						__leave;
					}
					m_LastError = FileOverlapWrite (hDestFile, DataBuffer, &ByteOffset, NumberOfBytesRead, hEvent);
					if (m_LastError != ERROR_SUCCESS)
					{
						LOG_ERROR(m_LastError, "CFolder::CopyTo: FileOverlapWrite(%s)", DestFilePath)
						__leave;
					}
					if (bThroughWrite) //�����͸д
					{
						m_LastError = FileOverlapWrite (hDestTWFile, DataBuffer, &ByteOffset, NumberOfBytesRead, hEvent);
						if (m_LastError != ERROR_SUCCESS)
						{
							LOG_ERROR(m_LastError, "CFolder::CopyTo: FileOverlapWrite(%s)", DestFileTWPath)
							__leave;
						}
					}
					//�������
					uProgressSize.QuadPart ++;
					m_ReportRoutine(m_ReportContext, PROGRESS_REPORT_DISP, FileIndexInfo->FilePathName, uProgressSize);
				}
				SetFileTime(hDestFile, NULL, NULL, FileIndexInfo->FileTime);
				if (bThroughWrite) //�����͸д
				{
					SetFileTime(hDestTWFile, NULL, NULL, FileIndexInfo->FileTime);
				}
			}
			__finally
			{
				if (hSourceFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hSourceFile);
					hSourceFile = INVALID_HANDLE_VALUE;
				}
				if (hDestFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hDestFile);
					hDestFile = INVALID_HANDLE_VALUE;
				}
				if (bThroughWrite) //�����͸д
				{
					if (hDestTWFile != INVALID_HANDLE_VALUE)
					{
						CloseHandle(hDestTWFile);
						hDestTWFile = INVALID_HANDLE_VALUE;
					}
				}
				if (m_LastError != ERROR_SUCCESS)
				{
					__leave;
				}
			}
		}
		//�����������
		m_ReportRoutine(m_ReportContext, PROGRESS_REPORT_END, NULL, LARGE_INTEGER());
__CopyIndexFile:
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ���Ҫ���������ļ�
		DestFolder.CloseFolder();
		strcpy(DestFilePath, DestFolder.m_Path);
		strcat(DestFilePath, INDEX_FILE_NAME);
		if (bThroughWrite) //�����͸д
		{
			strcpy(DestFileTWPath, DevicePath);
			strcat(DestFileTWPath, INDEX_FILE_NAME);
		}
		if((hDestFile = CreateFile(DestFilePath, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
		{
			m_LastError = GetLastError();
			LOG_ERROR(m_LastError, "CFolder::CopyTo: CreateFile[D](%s)", DestFilePath)
			__leave;
		}
		if (bThroughWrite) //�����͸д
		{
			if((hDestTWFile = CreateFile(DestFileTWPath, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
			{
				m_LastError = GetLastError();
				LOG_ERROR(m_LastError, "CFolder::CopyTo: CreateFile[TW](%s)", DestFileTWPath)
				__leave;
			}
		}
		uFileSize.QuadPart = m_IndexFileSize;
		for (i = 0; uFileSize.QuadPart != 0; i++)
		{
			ULONG NumberOfBytesWrite;
			LARGE_INTEGER ByteOffset;
			ByteOffset.QuadPart = i * dwPieceSize;
			NumberOfBytesWrite = (uFileSize.QuadPart >= dwPieceSize ? dwPieceSize : uFileSize.QuadPart);
			m_LastError = FileOverlapWrite (hDestFile, m_IndexFileMap+i*dwPieceSize, &ByteOffset, NumberOfBytesWrite, hEvent);
			if (m_LastError != ERROR_SUCCESS)
			{
				LOG_ERROR(m_LastError, "CFolder::CopyTo: FileOverlapWrite(%s)", DestFilePath)
				__leave;
			}
			if (bThroughWrite) //�����͸д
			{
				m_LastError = FileOverlapWrite (hDestTWFile, m_IndexFileMap+i*dwPieceSize, &ByteOffset, NumberOfBytesWrite, hEvent);
				if (m_LastError != ERROR_SUCCESS)
				{
					LOG_ERROR(m_LastError, "CFolder::CopyTo: FileOverlapWrite(%s)", DestFileTWPath)
					__leave;
				}
			}
			uFileSize.QuadPart -= NumberOfBytesWrite;
		}
	}
	__finally
	{
		if (hSourceFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hSourceFile);
		}
		if (hDestFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hDestFile);
		}
		if (bThroughWrite) //�����͸д
		{
			if (hDestTWFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hDestTWFile);
			}
		}
		if (hEvent != NULL)
		{
			CloseHandle(hEvent);
		}
		if (DataBuffer != NULL)
		{
			free(DataBuffer);
		}
	}
	if (m_LastError == ERROR_SUCCESS)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CFolder::PacketCheck(CFolder & DestFolder, PPACKET_CHECK_INFO * PacketCheckInfo)
{
	PFILE_INDEX_INFO InfoHead = RepairWith(DestFolder);	//��ԴĿ¼�е������ļ�ȥ��֤Ŀ��Ŀ¼�е��ļ�
	if (InfoHead == NULL)
	{
		*PacketCheckInfo = NULL;
		return TRUE;
	}
	ULONG FileCount = 0;
	PFILE_INDEX_INFO FileIndexInfo = NULL;
	for (FileIndexInfo = InfoHead; FileIndexInfo != NULL; FileIndexInfo = FileIndexInfo->Next) //ͳ���ж����ļ�
	{
		FileCount++;
	}
	PPACKET_CHECK_INFO PacketCheckInfoPreHead = NULL;
	PPACKET_CHECK_INFO PacketCheckInfoHead = (PPACKET_CHECK_INFO)malloc(sizeof(PACKET_CHECK_INFO) * FileCount);
	*PacketCheckInfo = PacketCheckInfoHead;
	for (FileIndexInfo = InfoHead; FileIndexInfo != NULL; FileIndexInfo = FileIndexInfo->Next, PacketCheckInfoHead++)
	{
		if (PacketCheckInfoPreHead != NULL)
		{
			PacketCheckInfoPreHead->Next = PacketCheckInfoHead;
		}
		strcpy(PacketCheckInfoHead->FileName, FileIndexInfo->FilePathName);
		PacketCheckInfoHead->FileSize = *FileIndexInfo->FileSize;
		PacketCheckInfoHead->RealFileSize = FileIndexInfo->RealFileSize;
		PacketCheckInfoHead->FileExist = FileIndexInfo->FileExist;
		PacketCheckInfoHead->uPieceCount = FileIndexInfo->PieceCount;
		PacketCheckInfoHead->Next = NULL;
		PacketCheckInfoPreHead = PacketCheckInfoHead;
	}
	return TRUE;
}

void CFolder::PacketCheckEnd(PPACKET_CHECK_INFO PacketCheckInfo)
{
	if (PacketCheckInfo != NULL)
	{
		free(PacketCheckInfo);
	}
}
