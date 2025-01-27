﻿#include "stdafx.h"
#include "KSJDemoVC.h"
#include "KSJDemoVCDlg.h"
#include "afxdialogex.h"
#include "timeapi.h"

#include "opencv.Inc/opencv/cv.hpp"
#include "opencv.Inc/opencv/highgui.h"
#include "opencv.Inc/opencv/cxcore.hpp"
#pragma comment(lib, "opencv.Lib\\opencv_world300.lib")


#include "../../../KSJApi.Inc/KSJApiCustom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////////////////////
void CKSJDemoVCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PREVIEWWND, m_SnapStatic);
}

BEGIN_MESSAGE_MAP(CKSJDemoVCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_MESSAGE(WM_CAPTURE_END, MsgCaptureEnd)
	ON_EN_CHANGE(IDC_EDIT_EXPOSURE_TIME_MS, &CKSJDemoVCDlg::OnEnChangeEditExposureTimeMs)
	ON_CBN_SELCHANGE(IDC_COMBO_DEVICE_LIST, &CKSJDemoVCDlg::OnCbnSelchangeComboDeviceList)
	ON_EN_CHANGE(IDC_EDIT_EXPOSURE_LINES, &CKSJDemoVCDlg::OnEnChangeEditExposureLines)
	ON_BN_CLICKED(IDC_BUTTON_CAPTURE_FOV_SET, &CKSJDemoVCDlg::OnBnClickedButtonCaptureFovSet)
	ON_EN_CHANGE(IDC_EDIT_GAIN, &CKSJDemoVCDlg::OnEnChangeEditGain)
	ON_BN_CLICKED(IDC_BUTTON_CAPTURE, &CKSJDemoVCDlg::OnBnClickedButtonCapture)
	ON_BN_CLICKED(IDC_CHECK_ZOOMIMAGE, &CKSJDemoVCDlg::OnBnClickedCheckZoomimage)
END_MESSAGE_MAP()

void CKSJDemoVCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CKSJDemoVCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKSJDemoVCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
CKSJDemoVCDlg::CKSJDemoVCDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CKSJDemoVCDlg::IDD, pParent)
, m_nDeviceNum(0)
, m_nDeviceCurSel(-1)
, m_hCaptureThread(NULL)
, m_hCaptureExitEvent(NULL)
, m_bIsAcquiring(false)
, m_nCaptureTickCount(0)
, m_bZoomImage(false)
, m_nZoomImgWidth(1280)
, m_nZoomImgHeight(720)
, m_bSaveVideo(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_hCaptureExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// You only call KSJ_Init once.
	int nRet = KSJ_Init();//��̬���ʼ��
}

void CKSJDemoVCDlg::OnDestroy()
{
	KillTimer(TIMERID_GET_FRAME_RATE);

	StopAcquisition();

	CDialogEx::OnDestroy();

	int nRet = KSJ_UnInit();//�ͷŶ�̬��

	if (m_hCaptureExitEvent != NULL)
	{
		CloseHandle(m_hCaptureExitEvent);
		m_hCaptureExitEvent = NULL;
	}
}

BOOL CKSJDemoVCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//CEdit *pEditCtrlExposureTimeMs = ((CEdit*)GetDlgItem(IDC_EDIT_EXPOSURE_TIME_MS));
	CSpinButtonCtrl * pSpinCtrlExposureTimeMs = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_EXPOSURE_TIME_MS);//��ʼ��SPIN�ؼ�
	pSpinCtrlExposureTimeMs->SetBuddy(GetDlgItem(IDC_EDIT_EXPOSURE_TIME_MS));
	pSpinCtrlExposureTimeMs->SetBase(10);

	//CEdit *pEditCtrlExposureLines = ((CEdit*)GetDlgItem(IDC_EDIT_EXPOSURE_LINES));
	CSpinButtonCtrl * pSpinCtrlExposureLines = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_EXPOSURE_LINES);
	pSpinCtrlExposureLines->SetBuddy(GetDlgItem(IDC_EDIT_EXPOSURE_LINES));
	pSpinCtrlExposureLines->SetBase(10);

	CSpinButtonCtrl * pSpinCtrlGain = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_GAIN);
	pSpinCtrlGain->SetBuddy(GetDlgItem(IDC_EDIT_GAIN));
	pSpinCtrlGain->SetBase(10);

	CListCtrl *pListFunction = (CListCtrl*)GetDlgItem(IDC_LIST_FUNCTION);//��ʼ�������б�
	pListFunction->DeleteAllItems();
	pListFunction->InsertColumn(0, _T("Function"));
	pListFunction->InsertColumn(1, _T("Support"));
	pListFunction->SetColumnWidth(0, 300);
	pListFunction->SetColumnWidth(1, 100);

	DWORD dwStyleEx = pListFunction->GetExtendedStyle();
	pListFunction->SetExtendedStyle(dwStyleEx | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	UpdateDeviceList();
	UpdateInterface();

	GetDlgItem(IDC_BUTTON_CAPTURE)->EnableWindow(m_nDeviceCurSel >= 0);
	GetDlgItem(IDC_BUTTON_CAPTURE_FOV_SET)->EnableWindow(m_nDeviceCurSel >= 0 && !m_bIsAcquiring);

	SetTimer(TIMERID_GET_FRAME_RATE, 2000, NULL);

	((CButton*)GetDlgItem(IDC_CHECK_ZOOMIMAGE))->SetCheck(m_bZoomImage ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CKSJDemoVCDlg::UpdateInterface()
{
	if (m_nDeviceCurSel == -1)   return;

	// Initial Exposure Time
	int nMin = 0;
	int nMax = 0;
	int nCur = 0;

	KSJ_GetParamRange(m_nDeviceCurSel, KSJ_EXPOSURE, &nMin, &nMax);//�ع�ʱ��
	KSJ_GetParam(m_nDeviceCurSel, KSJ_EXPOSURE, &nCur);

	CSpinButtonCtrl * pSpinCtrl = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_EXPOSURE_TIME_MS);
	pSpinCtrl->SetRange32(nMin, nMax);
	pSpinCtrl->SetPos32(nCur);

	TCHAR   szText[64] = { '\0' };
	_stprintf_s(szText, _T("%d-%d ms (%0.2fsec, %0.2fmin)"), nMin, nMax, (float)nMin / 1000.0f, (float)nMax / 60000.0f);
	((CStatic*)GetDlgItem(IDC_STATIC_EXPOSURE_TIME_RANGE))->SetWindowText(szText);

	KSJ_GetParamRange(m_nDeviceCurSel, KSJ_EXPOSURE_LINES, &nMin, &nMax);//�ع���
	KSJ_GetParam(m_nDeviceCurSel, KSJ_EXPOSURE_LINES, &nCur);
	pSpinCtrl = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_EXPOSURE_LINES);
	pSpinCtrl->SetRange32(nMin, nMax);
	pSpinCtrl->SetPos32(nCur);

	_stprintf_s(szText, _T("%d-%d Lines"), nMin, nMax );
	((CStatic*)GetDlgItem(IDC_STATIC_EXPOSURE_LINES_RANGE))->SetWindowText(szText);
	
	// GAIN, Because R,G,B Gain has same range. 
	KSJ_GetParamRange(m_nDeviceCurSel, KSJ_RED, &nMin, &nMax);//����
	KSJ_GetParam(m_nDeviceCurSel, KSJ_RED, &nCur);

	pSpinCtrl = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_GAIN);
	pSpinCtrl->SetRange32(nMin, nMax);
	pSpinCtrl->SetPos32(nCur);

	_stprintf_s(szText, _T("%d-%d"), nMin, nMax );
	((CStatic*)GetDlgItem(IDC_STATIC_GAIN_RANGE))->SetWindowText(szText);

	// FOV
	int nColStart;
	int nRowStart;
	int nColSize;
	int nRowSize;
	KSJ_ADDRESSMODE ColAddressMode;
	KSJ_ADDRESSMODE RowAddressMode;

	KSJ_CaptureGetDefaultFieldOfView(m_nDeviceCurSel, &nColStart, &nRowStart, &nColSize, &nRowSize, &ColAddressMode, &RowAddressMode);//�ɼ�Ĭ���ӳ�
	SetDlgItemInt(IDC_EDIT_CAPTURE_COL_START, nColStart);
	SetDlgItemInt(IDC_EDIT_CAPTURE_ROW_START, nRowStart);
	SetDlgItemInt(IDC_EDIT_CAPTURE_COL_SIZE, nColSize);
	SetDlgItemInt(IDC_EDIT_CAPTURE_ROW_SIZE, nRowSize);
	_stprintf_s(szText, _T("%d-%d"), nColSize, nRowSize);
	((CStatic*)GetDlgItem(IDC_STATIC_FOV_RANGE))->SetWindowText(szText);
}

void CKSJDemoVCDlg::UpdateDeviceList(void)
{
	KSJ_UnInit();
	KSJ_Init();

	CComboBox    *pComboBox = NULL;
	pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_DEVICE_LIST);
	pComboBox->ResetContent();

	m_nDeviceNum = KSJ_DeviceGetCount();
	TCHAR  szDeviceNum[64] = { '\0' };
	_stprintf_s(szDeviceNum, _T("%d Device Found:"), m_nDeviceNum);
	GetDlgItem(IDC_STATIC_DEVICENUM)->SetWindowText(szDeviceNum);

	if (m_nDeviceNum == 0)
	{
		m_nDeviceCurSel = -1;
		return;
	}

	if (m_nDeviceCurSel <0 || m_nDeviceCurSel >= m_nDeviceNum)
	{
		m_nDeviceCurSel = 0;
	}

	for (int i = 0; i<m_nDeviceNum; i++)
	{
		m_DeviceInfo[i].nIndex = i;
		// KSJ_DeviceGetInformation( i, &(m_DeviceInfo[i].DeviceType), &(m_DeviceInfo[i].nSerials), &(m_DeviceInfo[i].wFirmwareVersion) );
		KSJ_DeviceGetInformationEx(i, &(m_DeviceInfo[i].DeviceType), &(m_DeviceInfo[i].nSerials), &(m_DeviceInfo[i].wFirmwareVersion), &(m_DeviceInfo[i].wFpgaVersion));

		BYTE btMajVersion = (m_DeviceInfo[i].wFirmwareVersion & 0xFF00) >> 8;
		BYTE btMinVersion = m_DeviceInfo[i].wFirmwareVersion & 0x00FF;

		BYTE btFpgaMajVersion = (m_DeviceInfo[i].wFpgaVersion & 0xFF00) >> 8;
		BYTE btFpgaMinVersion = m_DeviceInfo[i].wFpgaVersion & 0x00FF;

		TCHAR  szMenuItem[256] = { '\0' };
		_stprintf_s(szMenuItem, _T("Index(%d)-Type(%s)-Serials(%d)-FwVer(%d.%d)-FpgaVer(%d.%d)"), i, g_szDeviceType[m_DeviceInfo[i].DeviceType], m_DeviceInfo[i].nSerials, btMajVersion, btMinVersion, btFpgaMajVersion, btFpgaMinVersion);
		pComboBox->AddString(szMenuItem);
	}

	pComboBox->SetCurSel(m_nDeviceCurSel);
}

void CKSJDemoVCDlg::OnCbnSelchangeComboDeviceList()
{
	CComboBox    *pComboBox = NULL;
	pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_DEVICE_LIST);
	m_nDeviceCurSel = pComboBox->GetCurSel();

	UpdateInterface();

	GetDlgItem(IDC_BUTTON_CAPTURE)->EnableWindow(m_nDeviceCurSel >= 0);
	GetDlgItem(IDC_BUTTON_CAPTURE_FOV_SET)->EnableWindow(m_nDeviceCurSel >= 0 && !m_bIsAcquiring);
}

void CKSJDemoVCDlg::GetRealExposureTime()
{
	float fExposureTimeMs = 0.0f;
	KSJ_ExposureTimeGet(m_nDeviceCurSel, &fExposureTimeMs);

	TCHAR   szExposureTimeMs[32] = { '\0' };
	_stprintf_s(szExposureTimeMs, _T("Exp Time: %0.5f ms"), fExposureTimeMs);
	((CStatic*)GetDlgItem(IDC_STATIC_EXPOSURE_TIME_REAL))->SetWindowText(szExposureTimeMs);

}

void CKSJDemoVCDlg::OnEnChangeEditExposureTimeMs()
{
	if (m_nDeviceCurSel == -1)   return;
	CEdit *pEditCtrl = ((CEdit*)GetDlgItem(IDC_EDIT_EXPOSURE_TIME_MS));

	TCHAR    szExposureTimeMs[32] = { '\0' };
	pEditCtrl->GetWindowText(szExposureTimeMs, 32);

	DWORD    m_dwExposureTimeMs = _ttoi(szExposureTimeMs);

	KSJ_SetParam(m_nDeviceCurSel, KSJ_EXPOSURE, m_dwExposureTimeMs);

	GetRealExposureTime();
}

void CKSJDemoVCDlg::OnEnChangeEditGain()
{
	if (m_nDeviceCurSel == -1)   return;
	CEdit *pEditCtrl = ((CEdit*)GetDlgItem(IDC_EDIT_GAIN));

	TCHAR    szGain[32] = { '\0' };
	pEditCtrl->GetWindowText(szGain, 32);

	int nGain = _ttoi(szGain);

	KSJ_SetParam(m_nDeviceCurSel, KSJ_RED, nGain);
	KSJ_SetParam(m_nDeviceCurSel, KSJ_GREEN, nGain);
	KSJ_SetParam(m_nDeviceCurSel, KSJ_BLUE, nGain);
}

void CKSJDemoVCDlg::OnEnChangeEditExposureLines()
{
	if (m_nDeviceCurSel == -1)   return;
	CEdit *pEditCtrl = ((CEdit*)GetDlgItem(IDC_EDIT_EXPOSURE_LINES));

	TCHAR    szExposureLines[32] = { '\0' };
	pEditCtrl->GetWindowText(szExposureLines, 32);

	DWORD    m_dwExposureLines = _ttoi(szExposureLines);

	KSJ_SetParam(m_nDeviceCurSel, KSJ_EXPOSURE_LINES, m_dwExposureLines);

	GetRealExposureTime();

}

void CKSJDemoVCDlg::OnBnClickedButtonCaptureFovSet()
{
	int nColStart;
	int nRowStart;
	int nColSize;
	int nRowSize;

	KSJ_ADDRESSMODE ColAddressMode = KSJ_SKIPNONE;
	KSJ_ADDRESSMODE RowAddressMode = KSJ_SKIPNONE;

	nColStart = GetDlgItemInt(IDC_EDIT_CAPTURE_COL_START);
	nRowStart = GetDlgItemInt(IDC_EDIT_CAPTURE_ROW_START);
	nColSize = GetDlgItemInt(IDC_EDIT_CAPTURE_COL_SIZE);
	nRowSize = GetDlgItemInt(IDC_EDIT_CAPTURE_ROW_SIZE);
	KSJ_CaptureSetFieldOfView(m_nDeviceCurSel, nColStart, nRowStart, nColSize, nRowSize, ColAddressMode, RowAddressMode);//���òɼ��ӳ�

	KSJ_CaptureGetFieldOfView(m_nDeviceCurSel, &nColStart, &nRowStart, &nColSize, &nRowSize, &ColAddressMode, &RowAddressMode);
	SetDlgItemInt(IDC_EDIT_CAPTURE_COL_START, nColStart);
	SetDlgItemInt(IDC_EDIT_CAPTURE_ROW_START, nRowStart);
	SetDlgItemInt(IDC_EDIT_CAPTURE_COL_SIZE, nColSize);
	SetDlgItemInt(IDC_EDIT_CAPTURE_ROW_SIZE, nRowSize);
}

void CKSJDemoVCDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMERID_GET_FRAME_RATE)
	{
		if (m_bIsAcquiring)
		{
			m_dwTimeEnd = timeGetTime();
			
			if (m_nCaptureTickCount <= m_nLastTickCount)
			{
				GetDlgItem(IDC_STATIC_INFORMATION)->SetWindowText(_T("0 fps"));
			}
			else
			{
				unsigned long ulElapse = m_dwTimeEnd - m_dwTimeBegin;

				float fFPS = (float)((m_nCaptureTickCount - m_nLastTickCount) * 1000) / (float)(ulElapse);

				CString strFps;

				strFps.Format(_T("%.2f fps"), fFPS);

				GetDlgItem(IDC_STATIC_INFORMATION)->SetWindowText(strFps);
			}

			m_dwTimeBegin = m_dwTimeEnd = timeGetTime();
			m_nLastTickCount = m_nCaptureTickCount;
		}
		else
		{
			m_nLastTickCount = 0;
			m_nCaptureTickCount = 0;
			m_dwTimeBegin = timeGetTime();
			GetDlgItem(IDC_STATIC_INFORMATION)->SetWindowText(_T("0 fps"));
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

afx_msg LRESULT CKSJDemoVCDlg::MsgCaptureEnd(WPARAM wParam, LPARAM lParam)
{
	m_bIsAcquiring = false;

	GetDlgItem(IDC_BUTTON_CAPTURE)->SetWindowText(_T("Start Capture"));
	GetDlgItem(IDC_BUTTON_CAPTURE_FOV_SET)->EnableWindow(m_nDeviceCurSel >= 0);

	return 0;
}


bool CKSJDemoVCDlg::StartAcquisition()
{
	if (m_nDeviceCurSel < 0) return false;

	if (m_bIsAcquiring) return true;

	m_nLastTickCount = 0;
	m_nCaptureTickCount = 0;
	m_dwTimeBegin = timeGetTime();

	GetDlgItem(IDC_BUTTON_CAPTURE)->SetWindowText(_T("Stop Capture"));
	GetDlgItem(IDC_BUTTON_CAPTURE_FOV_SET)->EnableWindow(FALSE);

	unsigned int nThreadID;
	ResetEvent(m_hCaptureExitEvent);//启动线程进行采集
	m_hCaptureThread = (HANDLE)_beginthreadex(NULL, 0, CaptureThread, this, 0, &nThreadID);
	
	m_bIsAcquiring = true;

	return RET_SUCCESS;
}

bool CKSJDemoVCDlg::StopAcquisition()
{
	if (!m_bIsAcquiring) return true;

	if (m_hCaptureExitEvent != NULL)
	{
		SetEvent(m_hCaptureExitEvent); // 停止采集
	}

	if (m_hCaptureThread != NULL)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(m_hCaptureThread, 10000))
		{
			TerminateThread(m_hCaptureThread, 0);
		}
		CloseHandle(m_hCaptureThread);
		m_hCaptureThread = NULL;
	}

	GetDlgItem(IDC_BUTTON_CAPTURE)->SetWindowText(_T("Start Capture"));
	GetDlgItem(IDC_BUTTON_CAPTURE_FOV_SET)->EnableWindow(m_nDeviceCurSel >= 0);
	m_bIsAcquiring = false;

	return true;
}

UINT WINAPI CKSJDemoVCDlg::CaptureThread(LPVOID pParam)
{
	CKSJDemoVCDlg *pMain = (CKSJDemoVCDlg*)pParam;
	pMain->CaptureThreadX();

	return 0;
}

void CKSJDemoVCDlg::CaptureThreadX()
{
	m_nCaptureTickCount = 0;

	int nColSize, nRowSize, nBitCount;

	if (KSJ_CaptureGetSizeEx(m_nDeviceCurSel, &nColSize, &nRowSize, &nBitCount) != RET_SUCCESS)
	{
		// end
		PostMessage(WM_CAPTURE_END);
	}

	unsigned char* pImageBuffer = new unsigned char[nColSize*nRowSize*(nBitCount >> 3)];
	unsigned char* pZoomImgBuffer = new unsigned char[1280*720*(nBitCount >> 3)];

	if (m_bSaveVideo)
	{
		cv::VideoWriter writer("e:\\video.avi", -1, 25.0, cv::Size(1280, 720), true);		
		while (1)
		{
			if (WaitForSingleObject(m_hCaptureExitEvent, 0) == 0)
			{
				break;
			}

			int nRet = KSJ_CaptureRgbData(m_nDeviceCurSel, pImageBuffer);

			if (RET_SUCCESS == nRet)
			{
				++m_nCaptureTickCount;

				cv::Mat Dst;
					
				if (nBitCount == 8)
				{
					cv::Mat Src = cv::Mat(nRowSize, nColSize, CV_8UC1, pImageBuffer, nColSize);
						
					cv::resize(Src, Dst, cv::Size(m_nZoomImgWidth, m_nZoomImgHeight));

					memcpy(pZoomImgBuffer, Dst.data, 1280 * 720 * (nBitCount >> 3));
				}
				else if (nBitCount == 24)
				{
					cv::Mat Src = cv::Mat(nRowSize, nColSize, CV_8UC3, pImageBuffer, nColSize * 3);

					cv::resize(Src, Dst, cv::Size(m_nZoomImgWidth, m_nZoomImgHeight));

					memcpy(pZoomImgBuffer, Dst.data, 1280 * 720 * (nBitCount >> 3));
				}
				else if (nBitCount == 32)
				{
					cv::Mat Src = cv::Mat(nRowSize, nColSize, CV_8UC4, pImageBuffer, nColSize * 4);

					cv::resize(Src, Dst, cv::Size(m_nZoomImgWidth, m_nZoomImgHeight));

					memcpy(pZoomImgBuffer, Dst.data, 1280 * 720 * (nBitCount >> 3));
				}

				m_SnapStatic.UpdateImage(pZoomImgBuffer, m_nZoomImgWidth, m_nZoomImgHeight, nBitCount, 2);

				writer.write(Dst);
			}
		}

	}
	else
	{
		while (1)
		{
			if (WaitForSingleObject(m_hCaptureExitEvent, 0) == 0)
			{
				break;
			}

			int nRet = KSJ_CaptureRgbData(m_nDeviceCurSel, pImageBuffer);

			if (RET_SUCCESS == nRet)
			{
				++m_nCaptureTickCount;

				if (m_bZoomImage)
				{
					if (nBitCount == 8)
					{
						cv::Mat Src = cv::Mat(nRowSize, nColSize, CV_8UC1, pImageBuffer, nColSize);
						cv::Mat Dst;
						cv::resize(Src, Dst, cv::Size(m_nZoomImgWidth, m_nZoomImgHeight));

						memcpy(pZoomImgBuffer, Dst.data, 1280 * 720 * (nBitCount >> 3));
					}
					else if (nBitCount == 24)
					{
						cv::Mat Src = cv::Mat(nRowSize, nColSize, CV_8UC3, pImageBuffer, nColSize * 3);
						cv::Mat Dst;
						cv::resize(Src, Dst, cv::Size(m_nZoomImgWidth, m_nZoomImgHeight));

						memcpy(pZoomImgBuffer, Dst.data, 1280 * 720 * (nBitCount >> 3));
					}
					else if (nBitCount == 32)
					{
						cv::Mat Src = cv::Mat(nRowSize, nColSize, CV_8UC4, pImageBuffer, nColSize * 4);
						cv::Mat Dst;
						cv::resize(Src, Dst, cv::Size(m_nZoomImgWidth, m_nZoomImgHeight));

						memcpy(pZoomImgBuffer, Dst.data, 1280 * 720 * (nBitCount >> 3));
					}

					m_SnapStatic.UpdateImage(pZoomImgBuffer, m_nZoomImgWidth, m_nZoomImgHeight, nBitCount, 10);
				}
				else
				{
					int i;
					int mKSJ_VWsize = nColSize;
					int mKSJ_VHsize = nRowSize;
					int sublen = mKSJ_VWsize * mKSJ_VHsize;

					unsigned char* pKSJBuffer = new unsigned char[sublen * 3];

					memset(pKSJBuffer, 0, sublen * 3);

					for (i = 0; i < sublen; i++)
					{
						pKSJBuffer[i] = pImageBuffer[i];
					}

					for (i = 0; i < sublen; i++)
					{
						int p0 = sublen - i - 1;
						int p1 = (sublen - i) * 3 - 1;
						int p2 = (sublen - i) * 3 - 2;
						int p3 = (sublen - i) * 3 - 3;
						pKSJBuffer[p1] = 0;
						pKSJBuffer[p2] = pKSJBuffer[p0];
						pKSJBuffer[p3] = pKSJBuffer[p0];
					}

					m_SnapStatic.UpdateImage(pKSJBuffer, nColSize, nRowSize, 24, 10);
					//KSJ_HelperSaveToBmp(pImageBuffer, nColSize, nRowSize, 8, _T("E:\\SS01.BMP"));
					//KSJ_HelperSaveToBmp(pKSJBuffer, nColSize, nRowSize, 24, _T("E:\\SS02.BMP"));

					delete[]pKSJBuffer;

					m_SnapStatic.UpdateImage(pImageBuffer, nColSize, nRowSize, nBitCount, 10);
				}
			}
		}
	}

	delete[]pImageBuffer;
	delete[]pZoomImgBuffer;

	// end
	PostMessage(WM_CAPTURE_END);
}

void CKSJDemoVCDlg::OnBnClickedButtonCapture()
{
	if (m_bIsAcquiring) StopAcquisition();
	else StartAcquisition();
}

void CKSJDemoVCDlg::OnBnClickedCheckZoomimage()
{
	m_bZoomImage = ((CButton*)GetDlgItem(IDC_CHECK_ZOOMIMAGE))->GetCheck() == BST_CHECKED;
}
























