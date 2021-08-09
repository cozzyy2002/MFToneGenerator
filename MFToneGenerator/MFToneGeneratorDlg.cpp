
// MFToneGeneratorDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MFToneGenerator.h"
#include "MFToneGeneratorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFToneGeneratorDlg dialog



CMFToneGeneratorDlg::CMFToneGeneratorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFTONEGENERATOR_DIALOG, pParent)
	, m_statusMessage(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFToneGeneratorDlg::onStarted(bool canPause)
{
	DragAcceptFiles(FALSE);
	m_audioFileName.EnableWindow(FALSE);
	m_startStopButton.SetWindowText(_T("Stop"));
	if(canPause) {
		m_pauseResumeButton.SetWindowText(_T("Pause"));
		m_pauseResumeButton.EnableWindow(TRUE);
	}
	showStatus(_T("Playing"));
}

void CMFToneGeneratorDlg::onStopped()
{
	DragAcceptFiles(TRUE);
	m_audioFileName.EnableWindow(TRUE);
	m_startStopButton.SetWindowText(_T("Start"));
	m_pauseResumeButton.SetWindowText(_T("Pause"));
	m_pauseResumeButton.EnableWindow(FALSE);
	showStatus(_T("Stopped"));
}

void CMFToneGeneratorDlg::onPaused()
{
	m_pauseResumeButton.SetWindowText(_T("Resume"));
	showStatus(_T("Paused"));
}

void CMFToneGeneratorDlg::onResumed()
{
	m_pauseResumeButton.SetWindowText(_T("Pause"));
	showStatus(_T("Playing"));
}

void CMFToneGeneratorDlg::onError(LPCTSTR source, HRESULT hr, LPCTSTR message)
{
	showStatus(_T("%s failed. Error 0x%p: %s"), source, hr, message);
}

void CMFToneGeneratorDlg::showStatus(LPCTSTR msg, ...)
{
	va_list args;
	va_start(args, msg);
	m_statusMessage.FormatV(msg, args);
	va_end(args);

	m_statusMessage.TrimRight(_T("\r\n"));

	UpdateData(FALSE);
}

void CMFToneGeneratorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_AUDIO_FILE_NAME, m_audioFileName);
	DDX_Control(pDX, IDC_BUTTON_START_STOP, m_startStopButton);
	DDX_Control(pDX, IDC_BUTTON_PAUSE_RESUME, m_pauseResumeButton);
	DDX_Text(pDX, IDC_STATIC_STATUS, m_statusMessage);
}

BEGIN_MESSAGE_MAP(CMFToneGeneratorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_BUTTON_E, &CMFToneGeneratorDlg::OnBnClickedButtonE)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_START_STOP, &CMFToneGeneratorDlg::OnBnClickedButtonStartStop)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE_RESUME, &CMFToneGeneratorDlg::OnBnClickedButtonPauseResume)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CMFToneGeneratorDlg message handlers

BOOL CMFToneGeneratorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// TODO: Add extra initialization here
	m_context.reset(statemachine::IContext::create(m_hWnd, WM_USER));
	m_context->setCallback(this);
	m_context->setup();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFToneGeneratorDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFToneGeneratorDlg::OnPaint()
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
HCURSOR CMFToneGeneratorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFToneGeneratorDlg::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	TCHAR msg[1000];
	_stprintf_s(msg, _T("OnChar(%c, %d, 0x%08x)\n"), nChar, nRepCnt, nFlags);
	OutputDebugString(msg);
	//CDialogEx::OnChar(nChar, nRepCnt, nFlags);
}


void CMFToneGeneratorDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	TCHAR msg[1000];
	_stprintf_s(msg, _T("OnKeyDown(%c, %d, 0x%08x)\n"), nChar, nRepCnt, nFlags);
	OutputDebugString(msg);
	CDialogEx::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CMFToneGeneratorDlg::OnBnClickedButtonE()
{
	OutputDebugString(_T("OnBnClickedButtonE()\n"));
	m_pcmData->generate(300);
}


void CMFToneGeneratorDlg::OnClose()
{
	m_context->shutdown();

	CDialogEx::OnClose();
}


void CMFToneGeneratorDlg::OnBnClickedButtonStartStop()
{
	UpdateData();
	//CString fileName;
	//m_audioFileName.GetWindowText(fileName);
	//m_context->setAudioFileName(fileName);
	if(!m_pcmData) {
		auto generator = IPcmData::createSineWaveGenerator(IPcmData::SampleDataType::_16bits);
		m_pcmData = IPcmData::create(44100, 2, generator);
		m_pcmData->generate(440, 0.5f, 0.5f);
		m_context->setPcmData(m_pcmData);
	}
	m_context->startStop();
}


void CMFToneGeneratorDlg::OnBnClickedButtonPauseResume()
{
	m_context->pauseResume();
}


void CMFToneGeneratorDlg::OnDropFiles(HDROP hDropInfo)
{
	auto size = DragQueryFile(hDropInfo, 0, nullptr, 0);
	if(0 < size) {
		size++;
		std::unique_ptr<TCHAR[]> fileName(new TCHAR[size]);
		DragQueryFile(hDropInfo, 0, fileName.get(), size);
		m_audioFileName.SetWindowText(fileName.get());
		UpdateData(FALSE);
	}
}
