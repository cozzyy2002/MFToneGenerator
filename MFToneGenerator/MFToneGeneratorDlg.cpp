
// MFToneGeneratorDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MFToneGenerator.h"
#include "MFToneGeneratorDlg.h"
#include "afxdialogex.h"

#include <utility>

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

// static declarations
namespace {

const int SliderMaxValue = 100;

template<typename T>
struct TNameValue : std::pair<LPCTSTR, T>
{
	TNameValue(first_type name, second_type value, bool isDefault = false)
		: std::pair<LPCTSTR, T>(name, value), name(name), value(second), isDefault(isDefault) {}
	first_type name;
	second_type& value;
	bool isDefault;
};

const TNameValue<DWORD> samplesPerSecondList[] = {
	{_T("16Khz"), 16000},
	{_T("22.05Khz"), 22050},
	{_T("32Khz"), 32000},
	{_T("44.1Khz"), 44100, true},
	{_T("48Khz"), 48000},
};

const TNameValue<WORD> channelsList[] = {
	{_T("1ch"), 1},
	{_T("2ch"), 2, true},
	{_T("4ch"), 4},
	{_T("5.1ch"), 6},
};

// Default items to be selected initially in each ComboBox.
const auto defaultSampeDataType = IPcmData::SampleDataType::PCM_16bits;
const auto defaultWaveForm = IPcmData::WaveFormType::SineWave;

}

CMFToneGeneratorDlg::CMFToneGeneratorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFTONEGENERATOR_DIALOG, pParent)
	, m_status(Status::Stopped)
	, m_statusMessage(_T(""))
	, m_sampleDataTypeProperties(PcmDataEnumerator::getSampleDatatypeProperties())
	, m_WaveFormProperties(PcmDataEnumerator::getWaveFormProperties())
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFToneGeneratorDlg::onStarted(bool canPause)
{
	DragAcceptFiles(FALSE);
	m_startStopButton.SetWindowText(_T("Stop"));
	if(canPause) {
		m_pauseResumeButton.SetWindowText(_T("Pause"));
		m_pauseResumeButton.EnableWindow(TRUE);
	}
	showStatus(Status::Playing);
}

void CMFToneGeneratorDlg::onStopped()
{
	m_pcmData.reset();

	DragAcceptFiles(TRUE);
	m_startStopButton.SetWindowText(_T("Start"));
	m_pauseResumeButton.SetWindowText(_T("Pause"));
	m_pauseResumeButton.EnableWindow(FALSE);
	showStatus(Status::Stopped);
}

void CMFToneGeneratorDlg::onPaused()
{
	m_pauseResumeButton.SetWindowText(_T("Resume"));
	showStatus(Status::Paused);
}

void CMFToneGeneratorDlg::onResumed()
{
	m_pauseResumeButton.SetWindowText(_T("Pause"));
	showStatus(Status::Playing);
}

void CMFToneGeneratorDlg::onError(LPCTSTR source, HRESULT hr, LPCTSTR message)
{
	StringFormatter fmt;
	auto msg = fmt.format(_T("%s failed. Error 0x%p: %s"), source, hr, message);
	showStatus(msg);
	MessageBox(msg.c_str(), _T("Error"), MB_OK | MB_ICONERROR);
}

void CMFToneGeneratorDlg::showStatus(Status status)
{
	switch(status)
	{
#define SHOW_STATUS(x) case Status::x: showStatus(_T(#x)); break;
	SHOW_STATUS(Stopped);
	SHOW_STATUS(Playing);
	SHOW_STATUS(Paused);
#undef SHOW_STATUS

	default:
		showStatus(_T("Unknown Status: %d"), status);
		return;
	}

	m_status = status;
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
	DDX_Control(pDX, IDC_BUTTON_START_STOP, m_startStopButton);
	DDX_Control(pDX, IDC_BUTTON_PAUSE_RESUME, m_pauseResumeButton);
	DDX_Text(pDX, IDC_STATIC_STATUS, m_statusMessage);
	DDX_Control(pDX, IDC_COMBO_SAMPLE_TYPE, m_sampleType);
	DDX_Control(pDX, IDC_COMBO_WAVE_FORM, m_waveForm);
	DDX_Control(pDX, IDC_COMBO_SAMPLES_PER_SEC, m_SamplesPerSecond);
	DDX_Control(pDX, IDC_COMBO_CHANNELS, m_channels);
	DDX_Control(pDX, IDC_SLIDER_DUTY, m_duty);
	DDX_Control(pDX, IDC_SLIDER_PEAK_POSITION, m_peakPosition);
	DDX_Control(pDX, IDC_SLIDER_LEVEL, m_level);
	DDX_Control(pDX, IDC_SLIDER_PHASE_SHIFT, m_phaseShift);
	DDX_Control(pDX, IDC_PICTURE_VIDEO, m_PictureVideo);
}

BEGIN_MESSAGE_MAP(CMFToneGeneratorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_START_STOP, &CMFToneGeneratorDlg::OnBnClickedButtonStartStop)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE_RESUME, &CMFToneGeneratorDlg::OnBnClickedButtonPauseResume)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON_E4, &CMFToneGeneratorDlg::OnBnClickedButtonE4)
	ON_BN_CLICKED(IDC_BUTTON_B3, &CMFToneGeneratorDlg::OnBnClickedButtonB3)
	ON_BN_CLICKED(IDC_BUTTON_G3, &CMFToneGeneratorDlg::OnBnClickedButtonG3)
	ON_BN_CLICKED(IDC_BUTTON_D3, &CMFToneGeneratorDlg::OnBnClickedButtonD3)
	ON_BN_CLICKED(IDC_BUTTON_A2, &CMFToneGeneratorDlg::OnBnClickedButtonA2)
	ON_BN_CLICKED(IDC_BUTTON_E2, &CMFToneGeneratorDlg::OnBnClickedButtonE2)
	ON_CBN_SELCHANGE(IDC_COMBO_WAVE_FORM, &CMFToneGeneratorDlg::OnCbnSelchangeComboWaveForm)
	ON_WM_GETMINMAXINFO()
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

	for(auto& x : m_sampleDataTypeProperties) {
		ATL::CA2T name(x.name);
		auto i = m_sampleType.AddString((LPCTSTR)name);
		if(x.type == defaultSampeDataType) {
			m_sampleType.SetCurSel(i);
		}
	}

	for(auto& x : m_WaveFormProperties) {
		ATL::CA2T name(x.name);
		auto i = m_waveForm.AddString((LPCTSTR)name);
		if(x.type == defaultWaveForm) {
			m_waveForm.SetCurSel(i);
		}
	}
	OnCbnSelchangeComboWaveForm();

	for(auto& x : samplesPerSecondList) {
		auto i = m_SamplesPerSecond.AddString(x.name);
		if(x.isDefault) {
			m_SamplesPerSecond.SetCurSel(i);
		}
	}

	for(auto& x : channelsList) {
		auto i = m_channels.AddString(x.name);
		if(x.isDefault) {
			m_channels.SetCurSel(i);
		}
	}

	std::tuple<CSliderCtrl*, const float&> sliders[] = {
		std::make_tuple<CSliderCtrl*, const float&>(&m_duty, PcmDataEnumerator::DefaultDuty),
		std::make_tuple<CSliderCtrl*, const float&>(&m_peakPosition, PcmDataEnumerator::DefaultPeakPosition),
		std::make_tuple<CSliderCtrl*, const float&>(&m_level, 0.5),
		std::make_tuple<CSliderCtrl*, const float&>(&m_phaseShift, 0.5),
	};

	for(auto& p : sliders) {
		auto ctrl = std::get<0>(p);
		ctrl->SetRange(0, SliderMaxValue, FALSE);
		ctrl->SetTicFreq(SliderMaxValue / 10);
		ctrl->SetPos((int)(SliderMaxValue * std::get<1>(p)));
	}

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



void CMFToneGeneratorDlg::OnClose()
{
	m_context->shutdown();

	CDialogEx::OnClose();
}


void CMFToneGeneratorDlg::OnBnClickedButtonStartStop()
{
	switch(m_status)
	{
	case Status::Stopped:
	{
		CFileDialog dlg(TRUE);
		if(dlg.DoModal() == IDOK) {
			auto fileName = dlg.GetPathName();
			m_context->startFile(fileName.GetString(), m_PictureVideo.GetSafeHwnd());
		}
	}
		break;

	case Status::Playing:
	case Status::Paused:
		m_context->stop();
		break;
	}
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
		//m_audioFileName.SetWindowText(fileName.get());
		UpdateData(FALSE);
	}
}


void CMFToneGeneratorDlg::OnKeyButtonClicked(float key)
{
	UpdateData();
	if(!m_pcmData) {
		auto& sp = m_sampleDataTypeProperties[m_sampleType.GetCurSel()];
		auto& wp = m_WaveFormProperties[m_waveForm.GetCurSel()];
		float param = 0.0f;
		switch(wp.parameter) {
		case PcmDataEnumerator::FactoryParameter::Duty:
			param = (float)m_duty.GetPos() / SliderMaxValue;
			break;
		case PcmDataEnumerator::FactoryParameter::PeakPosition:
			param = (float)m_peakPosition.GetPos() / SliderMaxValue;
			break;
		}
		auto generator = wp.factory(sp.type, param);
		auto samplesPerSecond = samplesPerSecondList[m_SamplesPerSecond.GetCurSel()].value;
		auto channels = channelsList[m_channels.GetCurSel()].value;
		m_pcmData = createPcmData(samplesPerSecond, channels, generator);
		if(m_pcmData) {
			ATL::CA2T waveForm(m_pcmData->getWaveFormTypeName());
			logger.log(_T("Created PcmData %s(%f) %d bps, %d Hz, %d channels")
				, (LPCTSTR)waveForm, param, m_pcmData->getBitsPerSample(), m_pcmData->getSamplesPerSec(), m_pcmData->getChannels());
		} else {
			showStatus(_T("Failed to create PcmData for generator(0x%p)"), generator);
			return;
		}

		// NOTE: Calling IContext::startTone() causes calling IPcmData::copyTo() that should be called after IPcmData::generate() bellow.
		//       WindowProcStateMachine used by Context, performs methods of Context on the UI thread as same as this method.
		//       So the calling sequence is assured.
		m_context->startTone(m_pcmData);
	}

	auto level = (float)m_level.GetPos() / SliderMaxValue;
	auto phaseShift = (float)m_phaseShift.GetPos() / SliderMaxValue;
	m_pcmData->generate(key, level, phaseShift);
}

void CMFToneGeneratorDlg::OnBnClickedButtonE4()
{
	OnKeyButtonClicked(82.407f);
}


void CMFToneGeneratorDlg::OnBnClickedButtonB3()
{
	OnKeyButtonClicked(110.0f);
}


void CMFToneGeneratorDlg::OnBnClickedButtonG3()
{
	OnKeyButtonClicked(146.832f);
}


void CMFToneGeneratorDlg::OnBnClickedButtonD3()
{
	OnKeyButtonClicked(195.998f);
}


void CMFToneGeneratorDlg::OnBnClickedButtonA2()
{
	OnKeyButtonClicked(246.942f);
}


void CMFToneGeneratorDlg::OnBnClickedButtonE2()
{
	OnKeyButtonClicked(329.628f);
}


void CMFToneGeneratorDlg::OnCbnSelchangeComboWaveForm()
{
	auto sel = m_waveForm.GetCurSel();
	auto& wp = m_WaveFormProperties[sel];
	m_duty.EnableWindow((wp.parameter == PcmDataEnumerator::FactoryParameter::Duty) ? TRUE : FALSE);
	m_peakPosition.EnableWindow((wp.parameter == PcmDataEnumerator::FactoryParameter::PeakPosition) ? TRUE : FALSE);
}


void CMFToneGeneratorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	__super::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize = POINT({ 640, 460 });
}
