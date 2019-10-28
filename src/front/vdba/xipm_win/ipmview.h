/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmview.h : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the view CvIpm class  (MFC frame/view/doc).
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
*/


#if !defined(AFX_IPMVIEW_H__AE712EFC_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_IPMVIEW_H__AE712EFC_E8C7_11D5_8792_00C04F1F754A__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CvIpm : public CView
{
protected: // create from serialization only
	CvIpm();
	DECLARE_DYNCREATE(CvIpm)

public:
	CdIpm* GetDocument();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvIpm)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CvIpm();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuIpm m_cIpm;
protected:
	void CreateIpmCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CvIpm)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	//
	// Event Handler:
	afx_msg void OnIpmTreeSelChange();
	afx_msg void OnPropertiesChange();
	DECLARE_EVENTSINK_MAP()
public:
	afx_msg void OnSysColorChange();
};

#ifndef _DEBUG  // debug version in ipmview.cpp
inline CdIpm* CvIpm::GetDocument() { return (CdIpm*)m_pDocument; }
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPMVIEW_H__AE712EFC_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
