
// MFToneGeneratorDlg.h : header file
//

#pragma once

#include "StateMachine/IContext.h"
#include "PcmData/PcmData.h"
#include "Utils.h"

#include <memory>

class IPcmData;

// CMFToneGeneratorDlg dialog
class CMFToneGeneratorDlg : public CDialogEx, public statemachine::IContext::ICallback
{
// Construction
public:
	CMFToneGeneratorDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFTONEGENERATOR_DIALOG };
#endif

#pragma region Implementation of statemachine::IContext::ICallback
	virtual void onStarted(bool canPause) override;
	virtual void onStopped() override;
	virtual void onPaused() override;
	virtual void onResumed() override;
	virtual void onError(LPCTSTR source, HRESULT hr, LPCTSTR message) override;
#pragma endregion

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

protected:
	const std::vector<PcmDataEnumerator::SampleDataTypeProperty>& m_sampleDataTypeProperties;
	const std::vector<PcmDataEnumerator::WaveFormProperty>& m_WaveFormProperties;

	std::unique_ptr<statemachine::IContext> m_context;
	CComPtr<IPcmData> m_pcmData;

	enum class Status
	{
		Stopped,
		Playing,
		Paused,
	};

	Status m_status;

	void showStatus(Status status);
	void showStatus(LPCTSTR msg, ...);
	Logger logger;

public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedButtonStartStop();
	afx_msg void OnBnClickedButtonPauseResume();
	CButton m_startStopButton;
	CButton m_pauseResumeButton;
	CString m_statusMessage;
	afx_msg void OnDropFiles(HDROP hDropInfo);
protected:
	CComboBox m_sampleType;
	CComboBox m_waveForm;
	void OnKeyButtonClicked(float key);
public:
	afx_msg void OnBnClickedButtonE4();
	afx_msg void OnBnClickedButtonB3();
	afx_msg void OnBnClickedButtonG3();
	afx_msg void OnBnClickedButtonD3();
	afx_msg void OnBnClickedButtonA2();
	afx_msg void OnBnClickedButtonE2();
	CComboBox m_SamplesPerSecond;
	CComboBox m_channels;
	CSliderCtrl m_duty;
	CSliderCtrl m_peakPosition;
	CSliderCtrl m_level;
	CSliderCtrl m_phaseShift;
	afx_msg void OnCbnSelchangeComboWaveForm();
};
