
// MFToneGeneratorDlg.h : header file
//

#pragma once

#include "StateMachine/Context.h"

#include <memory>

// CMFToneGeneratorDlg dialog
class CMFToneGeneratorDlg : public CDialogEx, public Context::ICallback
{
// Construction
public:
	CMFToneGeneratorDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFTONEGENERATOR_DIALOG };
#endif

#pragma region Implementation of Context::ICallback
	virtual void onStarted();
	virtual void onStopped();
	virtual void onPaused();
	virtual void onResumed();
	virtual void onError(HRESULT hr, LPCTSTR message);
#pragma endregion

	void showStatus(LPCTSTR msg, ...);

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
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnBnClickedButtonE();

protected:
	std::unique_ptr<Context> m_context;
public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedButtonStartStop();
	afx_msg void OnBnClickedButtonPauseResume();
	CString m_audioFileName;
	CButton m_startStopButton;
	CButton m_pauseResumeButton;
	CString m_statusMessage;
	afx_msg void OnDropFiles(HDROP hDropInfo);
};
