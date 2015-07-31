#include "afxwin.h"
#if !defined(AFX_SLIDERS_H__3542255E_1376_4FB7_91E7_B4841BB4F173__INCLUDED_)
#define AFX_SLIDERS_H__3542255E_1376_4FB7_91E7_B4841BB4F173__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Sliders.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Sliders dialog
class MySliderCtrl : public CSliderCtrl
{
public:
	MySliderCtrl () {}   // standard constructor

protected:
	afx_msg virtual void OnLButtonDown(UINT   nFlags,   CPoint   point);
};

class Sliders : public CDialog
{
// Construction
public:
	Sliders(CWnd* pParent = NULL);   // standard constructor

	void SetVolume();
	Bool m_grabbed;
	bool AUDIO_MUTED;
	enum slider_type{NONE, NAVIGATION, PLAY};

// Dialog Data
	//{{AFX_DATA(Sliders)
	enum { IDD = IDD_SLIDERS };
	CSliderCtrl	m_AudioVol;
	CSliderCtrl	m_PosSlider;
	CButton m_AudioMute;
	CButton m_Play;
	CButton m_Nav_Prev;
	CButton m_Nav_Next;
	CEdit m_cur_page;
	CStatic m_page_count;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Sliders)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Sliders)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void setPlay();
	void setPause();
	void setLayout(slider_type type);

	afx_msg void OnBnClickedAudioMute();
	afx_msg  void OnBnClickedPlay();
	afx_msg void OnBnClickedNext();
	afx_msg  void OnBnClickedPrev();
	afx_msg void OnNMCustomdrawAudioVol(NMHDR *pNMHDR, LRESULT *pResult);
	CButton m_playpause;
	afx_msg void OnBnClickedPlayPause();

private:
	void updateNav();
	void updatePlay();
	void onSizeNav(UINT nType, int cx, int cy);
	void onSizeSliders(UINT nType, int cx, int cy);

private:
	slider_type type;
public:
	afx_msg void OnNMCustomdrawSlider(NMHDR *pNMHDR, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SLIDERS_H__3542255E_1376_4FB7_91E7_B4841BB4F173__INCLUDED_)
