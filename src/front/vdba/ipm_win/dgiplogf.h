/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiplogf.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Log File page. (For drawing the pie chart)
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/


#ifndef DGIPLOGFILE_HEADER
#define DGIPLOGFILE_HEADER
#include "statfrm.h"

class CuDlgIpmPageLogFile : public CDialog
{
public:
	CuDlgIpmPageLogFile(CWnd* pParent = NULL); 

	void OnCancel() {return;}
	void OnOK()     {return;}


	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageLogFile)
	enum { IDD = IDD_IPMPAGE_LOGFILE };
	CEdit	m_cEditStatus;
	CStatic	m_cStaticStatus;
	CString	m_strStatus;
	//}}AFX_DATA
	LOGHEADERDATAMIN m_headStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageLogFile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CfStatisticFrame* m_pPieCtrl;
	int m_cyPadding;
	void ResetDisplay();
	
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageLogFile)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif

