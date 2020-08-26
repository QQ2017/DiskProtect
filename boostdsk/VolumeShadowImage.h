#ifndef _VOLUMESHADOWIMAGE_H_
#define _VOLUMESHADOWIMAGE_H_

#pragma pack(push, 1)
typedef struct _BLOCK_NODE //(sizeof=0x29)
{
	BYTE IsUsed;				//�Ƿ�ʹ��
	LARGE_INTEGER BlockNum;		//��ǰ���, һ��Ϊ128KB
	BYTE Bitmap[32];			//ÿ��λ����һ������,һ�����Ա�ʾ32*8=256������,Ҳ����128KB,����һ��
} BLOCK_NODE, *PBLOCK_NODE;
#pragma pack(pop)

typedef struct _VOLUME_SHADOW_IMAGE //(sizeof=0x20)
{
	HANDLE FileHandle;
	ULONG BlockCount;		//C��:0x8001 or E��:0x40001
	PBLOCK_NODE BlockNodeList;
	LARGE_INTEGER OffsetOfImageFile;
	LARGE_INTEGER EndOfImageFile;
} VOLUME_SHADOW_IMAGE, *PVOLUME_SHADOW_IMAGE;

NTSTATUS SWInitializeVolumeShadowImage(PVOLUME_SHADOW_IMAGE * lpVolumeShadowImage, ULONG BlockCount, PWCHAR FileNameTemplate);
NTSTATUS SWReadVolumeShadowImage(PVOLUME_SHADOW_IMAGE VolumeShadowImage, PVOID SystemBuffer, LARGE_INTEGER Offset, ULONG Length);
BOOLEAN SWFullDataInImage(PVOLUME_SHADOW_IMAGE VolumeShadowImage, PVOID SystemBuffer, LARGE_INTEGER Offset, ULONG Length);
NTSTATUS SWWriteVolumeShadowImage(PVOLUME_SHADOW_IMAGE VolumeShadowImage, PVOID SystemBuffer, LARGE_INTEGER Offset, ULONG Length);

#endif
