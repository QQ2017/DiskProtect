#pragma once

#define dwBytesOfKilo			1024
#define dwPieceSize				(256*dwBytesOfKilo)

//����Ч�������ļ��� ���ͳ����Ϣ
typedef struct _PACKET_CHECK_INFO
{
	CHAR FileName[512];			//����ļ����ļ���
	ULONG uPieceCount;			//����ļ������ж��ٸ��鲻��
	BOOL FileExist;				//Ҫ�������ļ��ǲ��Ǵ���
	LARGE_INTEGER FileSize;		//����ļ�Ӧ�õĴ�С
	LARGE_INTEGER RealFileSize;	//����ļ�ʵ���Ƕ��
	struct _PACKET_CHECK_INFO * Next;	//��һ���ڵ�
} PACKET_CHECK_INFO, *PPACKET_CHECK_INFO;

#define PROGRESS_REPORT_START		0
#define PROGRESS_REPORT_DISP		1
#define PROGRESS_REPORT_END			2
#define CHECKPROGRESS_REPORT_START	3
#define CHECKPROGRESS_REPORT_DISP	4

typedef VOID (*PCREATE_PROGRESS_REPORT) (PVOID ReportContext, ULONG uFlag, PCHAR FileName, LARGE_INTEGER & UParam);

//д��ԭ�������ò�������,��ûװ����֮ǰ���ò���Ч
EXTERN_C BOOL WINAPI DiskLibWriteParamSector();

//�����ʼ��
EXTERN_C BOOL WINAPI DiskLibInitialize();

//��黹ԭ���������Ƿ���ȷ
EXTERN_C BOOL WINAPI DiskLibCheckPassword(PCHAR Password);

//�����µĻ�ԭ��������
EXTERN_C BOOL WINAPI DiskLibSetPassword(PCHAR Password);

//���ñ���״̬
EXTERN_C BOOL WINAPI DiskLibSetProtectState(BOOL ProtectStateC, BOOL ProtectStateE);

//��ȡ����״̬
EXTERN_C BOOL WINAPI DiskLibGetProtectState(PBOOL ProtectStateC, PBOOL ProtectStateE);

//����ԭ��͸д�����������������ò�������
EXTERN_C BOOL WINAPI DiskLibEnableThroughWrite();

//��ֹ��ԭ��͸д�ͽ�ֹ�������ò�������
EXTERN_C BOOL WINAPI DiskLibDisableThroughWrite();

//���������ļ�
EXTERN_C BOOL WINAPI DiskLibCreateIndexFile(PCHAR PathFileName, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

//�޸�����
EXTERN_C BOOL WINAPI DiskLibRepairUpdate(PCHAR strSourcePath, PCHAR strDestPath, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

//���ո���(�����Աȸ���)
EXTERN_C BOOL WINAPI DiskLibSnapshotUpdate(PCHAR strSourcePath, PCHAR strDestPath, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

//��ȫ����(���¿���)
EXTERN_C BOOL WINAPI DiskLibCompleteUpdate(PCHAR strSourcePath, PCHAR strDestPath, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

//��ʼ��������ļ�
EXTERN_C BOOL WINAPI DiskLibPacketCheckStart(PCHAR strPath, PPACKET_CHECK_INFO * PacketCheckInfo, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

//��������ļ�����
EXTERN_C void WINAPI DiskLibPacketCheckEnd(PPACKET_CHECK_INFO PacketCheckInfo);

//ʣ��ռ��С�ж�
