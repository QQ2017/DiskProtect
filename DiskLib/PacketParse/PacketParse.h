// PacketParse.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CPacketParseApp:
// �йش����ʵ�֣������ PacketParse.cpp
//

class CPacketParseApp : public CWinApp
{
public:
	CPacketParseApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPacketParseApp theApp;