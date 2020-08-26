#include "stdafx.h"
#include "DiskLib.h"
#include "Packet.h"
#include "md5.h"

static CHAR ProtectPassword[32];
static ULONG uSessionID;

//���ʼ��
BOOL WINAPI DiskLibInitialize()
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARAM_GET_SESSION_ID ParamSessionID;
	PARAM_GET_PASSWORD   ParamPassword;
	ULONG BytesReturned;
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		ParamSessionID.Magic = PARAM_MAGIC_COOKIE;
		if (!DeviceIoControl(hDevice, IOCTL_GET_SESSION_ID, &ParamSessionID, sizeof(ParamSessionID), &ParamSessionID, sizeof(ParamSessionID), &BytesReturned, NULL))
		{
			__leave;
		}
		uSessionID = ParamSessionID.uSessionID;

		ParamPassword.Magic = PARAM_MAGIC_COOKIE;
		ParamPassword.uSessionID = uSessionID;
		if (!DeviceIoControl(hDevice, IOCTL_GET_PASSWORD, &ParamPassword, sizeof(ParamPassword), &ParamPassword, sizeof(ParamPassword), &BytesReturned, NULL))
		{
			__leave;
		}
		memcpy(ProtectPassword, ParamPassword.Password, sizeof(ProtectPassword));
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

//����վ������֤
BOOL WINAPI DiskLibCheckPassword(PCHAR Password)
{
	CHAR szMd5[128] = {0};
	md5(Password, lstrlen(Password), szMd5);
	if (memcmp(ProtectPassword, szMd5, sizeof(ProtectPassword)) == 0)
	{
		return TRUE;
	}
	return FALSE;
}

//���ù���վ����
BOOL WINAPI DiskLibSetPassword(PCHAR Password)
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARAM_SET_PASSWORD ParamPassword;
	ULONG BytesReturned;
	CHAR szMd5[128] = {0};
	md5(Password, lstrlen(Password), szMd5);
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		ParamPassword.Magic = PARAM_MAGIC_COOKIE;
		ParamPassword.uSessionID = uSessionID;
		memcpy(ParamPassword.OrgPassword, ProtectPassword, sizeof(ParamPassword.OrgPassword));
		memcpy(ParamPassword.NewPassword, szMd5, sizeof(ParamPassword.NewPassword));
		if (!DeviceIoControl(hDevice, IOCTL_SET_PASSWORD, &ParamPassword, sizeof(ParamPassword), NULL, 0, &BytesReturned, NULL))
		{
			__leave;
		}
		memcpy(ProtectPassword, ParamPassword.NewPassword, sizeof(ProtectPassword));
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

//���÷�������״̬,����վ�������Ч
BOOL WINAPI DiskLibSetProtectState(BOOL ProtectStateC, BOOL ProtectStateE)
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARAM_SET_PROTECT_STATE ParamProtectState;
	ULONG BytesReturned;
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		ParamProtectState.Magic = PARAM_MAGIC_COOKIE;
		ParamProtectState.uSessionID = uSessionID;
		memcpy(ParamProtectState.Password, ProtectPassword, sizeof(ParamProtectState.Password));
		ParamProtectState.ProtectC = ProtectStateC;
		ParamProtectState.ProtectE = ProtectStateE;
		if (!DeviceIoControl(hDevice, IOCTL_SET_PROTECT_STATE, &ParamProtectState, sizeof(ParamProtectState), NULL, 0, &BytesReturned, NULL))
		{
			__leave;
		}
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

//��ȡ�����Ƿ񱻱���
BOOL WINAPI DiskLibGetProtectState(PBOOL ProtectStateC, PBOOL ProtectStateE)
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARAM_GET_PROTECT_STATE ParamProtectState;
	ULONG BytesReturned;
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		ParamProtectState.Magic = PARAM_MAGIC_COOKIE;
		ParamProtectState.uSessionID = uSessionID;
		memcpy(ParamProtectState.Password, ProtectPassword, sizeof(ParamProtectState.Password));
		if (!DeviceIoControl(hDevice, IOCTL_GET_PROTECT_STATE, &ParamProtectState, sizeof(ParamProtectState), &ParamProtectState, sizeof(ParamProtectState), &BytesReturned, NULL))
		{
			__leave;
		}
		*ProtectStateC = ParamProtectState.ProtectC;
		*ProtectStateE = ParamProtectState.ProtectE;
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

//��������͸д
BOOL WINAPI DiskLibEnableThroughWrite()
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARAM_ENABLE_TW Param;
	ULONG BytesReturned;
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		Param.Magic = PARAM_MAGIC_COOKIE;
		Param.uSessionID = uSessionID;
		memcpy(Param.Password, ProtectPassword, sizeof(Param.Password));
		if (!DeviceIoControl(hDevice, IOCTL_ENABLE_THROUGH_WRITE, &Param, sizeof(Param), NULL, 0, &BytesReturned, NULL))
		{
			__leave;
		}
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

//���ý�ֹ��͸д
BOOL WINAPI DiskLibDisableThroughWrite()
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARAM_DISABLE_TW Param;
	ULONG BytesReturned;
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		Param.Magic = PARAM_MAGIC_COOKIE;
		Param.uSessionID = uSessionID;
		memcpy(Param.Password, ProtectPassword, sizeof(Param.Password));
		if (!DeviceIoControl(hDevice, IOCTL_DISABLE_THROUGH_WRITE, &Param, sizeof(Param), NULL, 0, &BytesReturned, NULL))
		{
			__leave;
		}
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

//���������ļ�
BOOL WINAPI DiskLibCreateIndexFile(PCHAR PathFileName, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	ULONG uError;
	CHAR szTargetIndexFile[MAX_PATH];
	strcpy(szTargetIndexFile, PathFileName);
	if (szTargetIndexFile[strlen(szTargetIndexFile) - 1] != '\\')
	{
		strcat(szTargetIndexFile, "\\");
	}
	strcat(szTargetIndexFile, INDEX_FILE_NAME);
	uError = CreateIndexFile(PathFileName, szTargetIndexFile, ReportRoutine, ReportContext);
	if (uError == ERROR_SUCCESS)
	{
		return TRUE;
	}
	return FALSE;
}

//�޸�����
BOOL WINAPI DiskLibRepairUpdate(PCHAR strSourcePath, PCHAR strDestPath, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	CFolder SourceDir;
	if (!SourceDir.Load(strSourcePath))
	{
		return FALSE;
	}
	CFolder DestDir;
	DestDir.Load(strDestPath);
	SourceDir.AttachCallBack(ReportRoutine, ReportContext);
	return SourceDir.CopyTo(DestDir, REPAIR_UPDATE);
}

//�Ա���������
BOOL WINAPI DiskLibSnapshotUpdate(PCHAR strSourcePath, PCHAR strDestPath, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	CFolder SourceDir;
	if (!SourceDir.Load(strSourcePath))
	{
		return FALSE;
	}
	CFolder DestDir;
	if(!DestDir.Load(strDestPath))
	{
		return FALSE;
	}
	SourceDir.AttachCallBack(ReportRoutine, ReportContext);
	return SourceDir.CopyTo(DestDir, SNAPSHOT_UPDATE);
}

//��ȫ����
BOOL WINAPI DiskLibCompleteUpdate(PCHAR strSourcePath, PCHAR strDestPath, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	CFolder SourceDir;
	if (!SourceDir.Load(strSourcePath))
	{
		return FALSE;
	}
	CFolder DestDir;
	DestDir.Load(strDestPath);
	SourceDir.AttachCallBack(ReportRoutine, ReportContext);
	return SourceDir.CopyTo(DestDir, DIRECT_UPDATE);
}

//д��������
BOOL WINAPI DiskLibWriteParamSector()
{
	BOOL   bResult = FALSE;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	ULONG BytesReturned;
	CHAR  szDigest[128];
	PROTECT_CONFIG_DATA ConfigData;
	__try
	{
		hDevice = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		if (SetFilePointer(hDevice, dwConfigDataSectorOffset, NULL, FILE_BEGIN) != dwConfigDataSectorOffset)
		{
			__leave;
		}
		if(!ReadFile(hDevice, &ConfigData, sizeof(ConfigData), &BytesReturned, NULL))
		{
			__leave;
		}
		ENCRYPT_DECODE_DATA(&ConfigData);
		if (memcmp(ConfigData.Magic, MagicFlag, sizeof(ConfigData.Magic)) == 0)
		{
			__leave;
		}
		memcpy(ConfigData.Magic, MagicFlag, sizeof(ConfigData.Magic));
		ConfigData.IsProtectC = TRUE;
		ConfigData.IsProtectE = TRUE;
		md5("123456789", lstrlen("123456789"), szDigest);
		memcpy(ConfigData.Password, szDigest, sizeof(ConfigData.Password));
		SYSTEMTIME SystemTime;
		GetSystemTime(&SystemTime);
		ConfigData.uKey = (SystemTime.wMilliseconds << 16 | SystemTime.wSecond);
		ENCRYPT_DECODE_DATA(&ConfigData);
		if (SetFilePointer(hDevice, dwConfigDataSectorOffset, NULL, FILE_BEGIN) != dwConfigDataSectorOffset)
		{
			__leave;
		}
		if(!WriteFile(hDevice, &ConfigData, sizeof(ConfigData), &BytesReturned, NULL))
		{
			__leave;
		}
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

//��ʼЧ�������ļ�
BOOL WINAPI DiskLibPacketCheckStart(PCHAR strPath, PPACKET_CHECK_INFO * PacketCheckInfo, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext)
{
	CFolder SourceDir;
	if (!SourceDir.Load(strPath))
	{
		return FALSE;
	}
	CFolder DestDir;
	DestDir.Load(strPath);
	SourceDir.AttachCallBack(ReportRoutine, ReportContext);
	return SourceDir.PacketCheck(DestDir, PacketCheckInfo);
}

//����Ч�������ļ�
void WINAPI DiskLibPacketCheckEnd(PPACKET_CHECK_INFO PacketCheckInfo)
{
	CFolder SourceDir;
	SourceDir.PacketCheckEnd(PacketCheckInfo);
}
