/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipllis.h, Header file. 
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Lock Lists page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/


#ifndef DGIPLLIS_HEADER
#define DGIPLLIS_HEADER
#include "calsctrl.h"

class CuDlgIpmPageLockLists : public CDialog
{
public:
	CuDlgIpmPageLockLists(CWnd* pParent = NULL);  
	void AddLockList (
		LPCTSTR strID, 
		LPCTSTR strSession, 
		LPCTSTR strCount, 
		LPCTSTR strLogical, 
		LPCTSTR strMaxL,
		LPCTSTR strStatus,
		void* pStruct);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageLockLists)
	enum { IDD = IDD_IPMPAGE_LOCKLISTS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageLockLists)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	void AddItem (LPCTSTR lpszItem, LOCKLISTDATAMIN* pData);
	void DisplayItems();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageLockLists)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
