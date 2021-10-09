
// HZGoogleDialog.h : header file
//

#pragma once

#include "HZBitmap.h"

// CHZGoogleDialog dialog
class CHZGoogleDialog : public CDialogEx
{
// Construction
public:
	CHZGoogleDialog(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HZGOOGLE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	static DWORD WINAPI DoLoad(LPVOID lpvThreadParam);

	CFont m_objFont;
	CArray<CHZBitmap*, CHZBitmap*> m_arrThumbs;
	CArray<CHZBitmap*, CHZBitmap*> m_arrPics;
	CString m_strWord;
	int m_nIndex = -1;
	CRect m_rcCrop;

	void ClearThumbs();

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
