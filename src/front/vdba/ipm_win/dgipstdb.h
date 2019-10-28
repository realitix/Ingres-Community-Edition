/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipstdb.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : Page of Table control: Display list of databases 
**                when tree selection is on static item "Databases"
**
** History:
**
** xx-Sep-1997 (Emmanuel Blattes)
**    Created
** 13-Dec-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/

#ifndef DGIPSTDB_HEADER
#define DGIPSTDB_HEADER
#include "calsctrl.h"

class CdIpmDoc;
class CuDlgIpmPageStaticDatabases : public CDialog
{
public:
	CuDlgIpmPageStaticDatabases(CWnd* pParent = NULL);
	void AddDatabase (
		LPCTSTR strName, 
		int imageIndex,
		void* pStruct);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageStaticDatabases)
	enum { IDD = IDD_IPMPAGE_STATIC_SERVERS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageStaticDatabases)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	void GetDisplayInfo (CdIpmDoc* pDoc, LPRESOURCEDATAMIN pData, CString& strName, int& imageIndex);

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageStaticDatabases)
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
