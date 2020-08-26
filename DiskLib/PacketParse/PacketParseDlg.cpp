// PacketParseDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PacketParse.h"
#include "PacketParseDlg.h"
#include "PathSelectDlg.h"
#include "..\DiskLib\DiskLib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPacketParseDlg �Ի���

CPacketParseDlg::CPacketParseDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPacketParseDlg::IDD, pParent)
	, m_radioTypeSel(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPacketParseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILEPATHNAME, m_edtPathFileName);
	DDX_Control(pDX, ID_SOURCE_PATH, m_SourcePath);
	DDX_Control(pDX, ID_DEST_PATH, m_DestPath);
	DDX_Control(pDX, IDC_CREATE_PROGRESS, m_CreateProgress);
	DDX_Control(pDX, IDC_VIEWNAME, m_ViewName);
	DDX_Radio(pDX, IDR_SNAPSHOT_UPDATE, m_radioTypeSel);
}

BEGIN_MESSAGE_MAP(CPacketParseDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CPacketParseDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON, &CPacketParseDlg::OnBnClickedButton)
	ON_BN_CLICKED(IDC_SOURCE_BROW, &CPacketParseDlg::OnBnClickedSourceBrow)
	ON_BN_CLICKED(IDC_DEST_BROW, &CPacketParseDlg::OnBnClickedDestBrow)
	ON_BN_CLICKED(IDC_START_UPDATE, &CPacketParseDlg::OnBnClickedStartUpdate)
	ON_BN_CLICKED(IDC_BUTTON1, &CPacketParseDlg::OnBnClickedButton1)
END_MESSAGE_MAP()

#define WM_UPDATE_PROGRESS (WM_USER+0x33)

VOID ProgressReport(PVOID ReportContext, ULONG uFlag, PCHAR FileName, LARGE_INTEGER & UParam)
{
	CPacketParseDlg * Dlg = (CPacketParseDlg *)ReportContext;
	switch(uFlag)
	{
	case PROGRESS_REPORT_START:
		Dlg->m_MaxPieceCount = (LPARAM)(UParam.QuadPart/dwPieceSize) + ((UParam.QuadPart%dwPieceSize) == 0 ? 0 : 1);
		Dlg->m_PieceCount = 0;
		Dlg->m_strViewName.Format("׼������ [%u\\%u]  ����:%s", Dlg->m_MaxPieceCount, Dlg->m_PieceCount, FileName);
		Dlg->PostMessage(WM_UPDATE_PROGRESS, PROGRESS_REPORT_START, 0);
		break;
	case PROGRESS_REPORT_DISP:
		Dlg->m_PieceCount = (LPARAM)(UParam.QuadPart/dwPieceSize) + ((UParam.QuadPart%dwPieceSize) == 0 ? 0 : 1);
		Dlg->m_strViewName.Format("���ڴ��� [%u\\%u]  ·��:%s", Dlg->m_MaxPieceCount, Dlg->m_PieceCount, FileName);
		Dlg->PostMessage(WM_UPDATE_PROGRESS, PROGRESS_REPORT_DISP, 0);
		break;
	case PROGRESS_REPORT_END:
		break;
	}
}

void CPacketParseDlg::ThreadRouter(PVOID Context)
{
	CPacketParseDlg * Dlg = (CPacketParseDlg *)Context;
	while (TRUE)
	{
		WaitForSingleObject(Dlg->m_WaitEvent, INFINITE);
/*
		PPACKET_CHECK_INFO PacketCheckInfoHead;
		BOOL bResult = DiskLibPacketCheckStart(Dlg->m_strPathFileName.GetBuffer(), &PacketCheckInfoHead, ProgressReport, Dlg);
		if (bResult)
		{
			for (PPACKET_CHECK_INFO Head = PacketCheckInfoHead; Head != NULL; Head = Head->Next)
			{
				::MessageBox(0, Head->FileName, 0, 0);
			}
			DiskLibPacketCheckEnd(PacketCheckInfoHead);
		}
*/

		BOOL bResult = DiskLibCreateIndexFile(Dlg->m_strPathFileName.GetBuffer(), ProgressReport, Dlg);
		if (bResult)
		{
			AfxMessageBox("�ɹ�");
		}
		else
		{
			AfxMessageBox("ʧ��");
		}

		ResetEvent(Dlg->m_WaitEvent);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPacketParseDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CPacketParseDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPacketParseDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CPathSelectDlg PathSelectDlg(this, "��ѡ��һ��Ŀ¼");
	if(PathSelectDlg.DoModal())
	{
		m_edtPathFileName.SetWindowText(PathSelectDlg.m_strPath);
	}
}

void CPacketParseDlg::OnBnClickedButton()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_edtPathFileName.GetWindowText(m_strPathFileName);
	m_strPathFileName.Trim();
	if (m_strPathFileName == "")
	{
		AfxMessageBox("������·��");
		return;
	}
	SetEvent(m_WaitEvent);
}

void CPacketParseDlg::OnBnClickedSourceBrow()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CPathSelectDlg PathSelectDlg(this, "��ѡ��һ��Ŀ¼");
	if(PathSelectDlg.DoModal())
	{
		m_SourcePath.SetWindowText(PathSelectDlg.m_strPath);
	}
}

void CPacketParseDlg::OnBnClickedDestBrow()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CPathSelectDlg PathSelectDlg(this, "��ѡ��һ��Ŀ¼");
	if(PathSelectDlg.DoModal())
	{
		m_DestPath.SetWindowText(PathSelectDlg.m_strPath);
	}
}

LRESULT CPacketParseDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: �ڴ����ר�ô����/����û���
	if (message == WM_UPDATE_PROGRESS)
	{
		switch(wParam)
		{
		case PROGRESS_REPORT_START:
			m_CreateProgress.SetRange32(0, m_MaxPieceCount);
			m_ViewName.SetWindowText(m_strViewName);
			break;

		case PROGRESS_REPORT_DISP:
			m_CreateProgress.SetPos(m_PieceCount);
			m_ViewName.SetWindowText(m_strViewName);
		    break;
		}
	}
	return CDialog::WindowProc(message, wParam, lParam);
}

void CPacketParseDlg::OnBnClickedStartUpdate()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	m_SourcePath.GetWindowText(m_strSourcePath);
	m_DestPath.GetWindowText(m_strDestPath);
	SetEvent(m_UpdateWaitEvent);
}



VOID CopyProgressReport(PVOID ReportContext, ULONG uFlag, PCHAR FileName, LARGE_INTEGER & UParam)
{
	CPacketParseDlg * Dlg = (CPacketParseDlg *)ReportContext;
	switch(uFlag)
	{
	case PROGRESS_REPORT_START:
		Dlg->m_MaxPieceCount = (LPARAM)UParam.QuadPart;
		Dlg->m_PieceCount = 0;
		Dlg->m_strViewName.Format("��ʼ���� [%u\\%u] ����:%s", Dlg->m_MaxPieceCount, Dlg->m_PieceCount, FileName);
		Dlg->PostMessage(WM_UPDATE_PROGRESS, PROGRESS_REPORT_START, 0);
		break;
	case PROGRESS_REPORT_DISP:
		Dlg->m_PieceCount = (LPARAM)UParam.QuadPart;
		Dlg->m_strViewName.Format("���ڸ��� [%u\\%u] ·��:%s", Dlg->m_MaxPieceCount, Dlg->m_PieceCount, FileName);
		Dlg->PostMessage(WM_UPDATE_PROGRESS, PROGRESS_REPORT_DISP, 0);
		break;
	case PROGRESS_REPORT_END:
		break;

	case CHECKPROGRESS_REPORT_START:
		Dlg->m_MaxPieceCount = (LPARAM)UParam.QuadPart;
		Dlg->m_PieceCount = 0;
		Dlg->m_strViewName.Format("��ʼ����ļ� [%u\\%u] ����:%s", Dlg->m_MaxPieceCount, Dlg->m_PieceCount, FileName);
		Dlg->PostMessage(WM_UPDATE_PROGRESS, PROGRESS_REPORT_START, 0);
		break;
	case CHECKPROGRESS_REPORT_DISP:
		Dlg->m_PieceCount = (LPARAM)UParam.QuadPart;
		Dlg->m_strViewName.Format("���ڼ���ļ� [%u\\%u] ·��:%s", Dlg->m_MaxPieceCount, Dlg->m_PieceCount, FileName);
		Dlg->PostMessage(WM_UPDATE_PROGRESS, PROGRESS_REPORT_DISP, 0);
		break;
	}
}

void CPacketParseDlg::UpdateThreadRouter(PVOID Context)
{
	BOOL bResult = FALSE;;
	CPacketParseDlg * Dlg = (CPacketParseDlg *)Context;
	while (TRUE)
	{
		WaitForSingleObject(Dlg->m_UpdateWaitEvent, INFINITE);
		DiskLibEnableThroughWrite();
		switch(Dlg->m_radioTypeSel)
		{
		case 0:
			bResult = DiskLibSnapshotUpdate(Dlg->m_strSourcePath.GetBuffer(), Dlg->m_strDestPath.GetBuffer(), CopyProgressReport, Dlg);
			break;
		case 1:
			bResult = DiskLibRepairUpdate(Dlg->m_strSourcePath.GetBuffer(), Dlg->m_strDestPath.GetBuffer(), CopyProgressReport, Dlg);
			break;
		case 2:
			bResult = DiskLibCompleteUpdate(Dlg->m_strSourcePath.GetBuffer(), Dlg->m_strDestPath.GetBuffer(), CopyProgressReport, Dlg);
		    break;
		}
		DiskLibDisableThroughWrite();
		if (bResult)
		{
			AfxMessageBox("�ɹ�");
		}
		else
		{
			AfxMessageBox("ʧ��");	
		}
		ResetEvent(Dlg->m_UpdateWaitEvent);
	}
}

// CPacketParseDlg ��Ϣ�������
BOOL CPacketParseDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	m_WaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_beginthread(ThreadRouter, 0, this);

	m_UpdateWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_beginthread(UpdateThreadRouter, 0, this);

	DiskLibInitialize();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CPacketParseDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//���ñ���״̬
	DiskLibSetProtectState(FALSE, FALSE);
	MessageBox("OK");
}
