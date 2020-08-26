#pragma once
#include "afxcoll.h"
#include "..\..\boostdsk\common.h"

#undef MAX_PATH
#define MAX_PATH 460
//==============================================================================
// �����������ļ���ض���
//

#define dwBytesOfKilo			1024
#define dwBytesOfMega			(dwBytesOfKilo*dwBytesOfKilo)
#define dwBytesOfGiga			(dwBytesOfMega*dwBytesOfKilo)
#define dwPieceSize				(256*dwBytesOfKilo)
#define dwSizeOfReserve			512
#define PacketMagicFlag			'  DQ'
#define CurrentVersion			1
#define SHA_HASH_LENGTH			20
#define INDEX_FILE_NAME			"KeydoneIndex.dat"

#pragma pack(push, 1)

typedef union _PACKET_RESERVE
{
	UCHAR ByteOf[dwSizeOfReserve];
	struct
	{
		ULONG         PacketMagic;				// ħ����־ == 'QD  '
		ULONG         PacketVersion;			// �����ļ��İ汾��
		ULONG         PacketFileCount;			// �����ļ��а������ٸ��ļ�
		LARGE_INTEGER PacketFileMaxSize;		// �����ļ��������ļ����ܴ�С
		CHAR          RootDirName[MAX_PATH];	// �����ļ��������ļ��ĸ�Ŀ¼��(����ǵ��ļ���ʽ�Ļ�����ȫ��Ϊ0)
	};
} PACKET_RESERVE, *PPACKET_RESERVE;

#pragma pack(pop)

// ������: 1�ֽ��ļ����·������(��0��β) + 8�ֽ��ļ���С + 8�ֽ��ļ�����޸�ʱ�� + �ļ�SHA
// �����ļ����20���ֽ������������ļ���SHA
//==============================================================================

typedef struct _FILE_INDEX_INFO
{
	PCHAR FilePathName;
	PLARGE_INTEGER FileSize;
	PFILETIME FileTime;
	PBYTE Sha1;
	//////////////////////////////////
	// ������ֶ�ÿ�ζԱȿ�����ʱʹ��
	_FILE_INDEX_INFO * Next;	//�������Ҫ�������ļ�������
	ULONG PieceCount;			//����ļ�һ��Ҫ�������ٸ�PIECE
	PULONG PieceList;			//PIECE����б�
	BOOL FileExist;				//Ҫ�������ļ��ǲ��Ǵ���
	LARGE_INTEGER RealFileSize;	//�ļ��������,��ôҪ�������ļ���ǰ��ʵ�ʴ�С�Ƕ���,����ͳ��ʣ��ռ��С
} FILE_INDEX_INFO, *PFILE_INDEX_INFO;

ULONG CreateIndexFile(PCHAR PathFileName, PCHAR TargetIndexFile, PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

enum UPDATE_TYPE
{
	SNAPSHOT_UPDATE,	//�Ƚ�ԴĿ¼��Ŀ��Ŀ¼�������ļ�
	REPAIR_UPDATE,		//��ԴĿ¼�е������ļ�ȥУ��Ŀ��Ŀ¼�е�ʵ���ļ�
	DIRECT_UPDATE		//�����καȽ�,ֱ�Ӹ���ԴĿ¼�������ļ������ļ�����
};

class CFolder
{
public:
	CFolder() : m_hFile(INVALID_HANDLE_VALUE), m_hMap(NULL), m_IndexFileMap(NULL), m_IndexFileSize(0), m_FileIndexInfo(NULL), m_FileCount(0), m_PieceList(NULL), m_LastError(ERROR_SUCCESS)
	{}
	~CFolder();
	BOOL Load(PCHAR Path);
	BOOL CopyTo(CFolder & DestFolder, UPDATE_TYPE UpdateType);
	BOOL PacketCheck(CFolder & DestFolder, PPACKET_CHECK_INFO * PacketCheckInfo);
	void PacketCheckEnd(PPACKET_CHECK_INFO PacketCheckInfo);
	ULONG GetLastErrorCode(void);
	void AttachCallBack(PCREATE_PROGRESS_REPORT ReportRoutine, PVOID ReportContext);

private:
	CMapStringToPtr m_FileListMap;
	PFILE_INDEX_INFO m_FileIndexInfo;
	CHAR m_Path[MAX_PATH];	//Ŀ¼·��
	CHAR m_Name[MAX_PATH];	//Ŀ¼����
	HANDLE m_hFile;
	HANDLE m_hMap;
	PBYTE m_IndexFileMap;
	ULONG m_IndexFileSize;	//�����ļ���С
	ULONG m_FileCount;		//Ŀ¼�������ļ�����
	PULONG m_PieceList;		//�������б�
	ULONG m_LastError;
	PCREATE_PROGRESS_REPORT m_ReportRoutine;
	PVOID m_ReportContext;

private:
	PFILE_INDEX_INFO CompareWith(CFolder & DestFolder);
	PFILE_INDEX_INFO RepairWith (CFolder & DestFolder);
	PFILE_INDEX_INFO ParseWith();
	void CloseFolder();
};
