#ifndef _PROTECTOBJECTINFO_H_
#define _PROTECTOBJECTINFO_H_

typedef struct _PROTECT_OBJECT_INFO //(sizeof=0x838)
{
	BOOLEAN IsProtect;						//�Ƿ񱻱�����ʵ��״̬
	BOOLEAN ProtectState;					//�Ƿ񱻱���(��Ҫ������Ӧ�ò�ͨ��,���������治ͬ)
	CHAR DriveLetter;
	ULONG PartitionIndex;					//���������ϵͳ�ķ�������
	ULONG DiskIndex;
	WCHAR VolumeDeviceName[257];			//����: \Device\HarddiskVolume1
	WCHAR DiskDeviceName[257];				//����: \Device\Harddisk0\DR0
	WCHAR filterDeviceSymbolicLinkName[257];
	LARGE_INTEGER StartingOffset;			//������ʼƫ��
	LARGE_INTEGER PartitionLength;			//��������
	PFILE_OBJECT FileObject;				//������������豸���ļ�����
	PDRIVE_LAYOUT_INFORMATION DriverLayoutInfo;
	ULONG PartitionCountInThisDisk;
	PARTITION_INFORMATION PartitionEntry[16];
} PROTECT_OBJECT_INFO, *PPROTECT_OBJECT_INFO;

extern PROTECT_OBJECT_INFO ProtectObjectInfoC;
extern PROTECT_OBJECT_INFO ProtectObjectInfoE;
extern CHAR ProtectPassword[32];

NTSTATUS SWReadConfigData();
NTSTATUS SWWriteConfigData();
NTSTATUS SWInitProtectInfo();
BOOLEAN SWTestOffsetIsHit(PDEVICE_OBJECT DeviceObject, PIRP Irp, PPROTECT_OBJECT_INFO ProtectObjectInfo);

#endif
