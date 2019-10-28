/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiplres.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Locked Resource page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/


#ifndef DGIPLRES_HEADER
#define DGIPLRES_HEADER
#include "calsctrl.h"

class CuDlgIpmPageLockResources : public CDialog
{
public:
	CuDlgIpmPageLockResources(CWnd* pParent = NULL);  
	void AddLockResource (
		LPCTSTR strID,
		LPCTSTR strGR, 
		LPCTSTR strCV,
		LPCTSTR strLockType,
		LPCTSTR strDB,
		LPCTSTR strTable,
		LPCTSTR strPage);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageLockResources)
	enum { IDD = IDD_IPMPAGE_LOCKEDRESOURCES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageLockResources)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageLockResources)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad   (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData  (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
