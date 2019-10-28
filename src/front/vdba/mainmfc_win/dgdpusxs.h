/*
**  Copyright (c) 2005-2010 Actian Corporation. All Rights Reserved.
*/

/*
**
** History:
**
** 13-Dec-2010 (drivi01)
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
*/
#ifndef DOMPROP_USER_XGRANTED_SEQ_HEADER
#define DOMPROP_USER_XGRANTED_SEQ_HEADER

#include "domseri2.h"
#include "domilist.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropUserXGrantedSeq dialog

class CuDlgDomPropUserXGrantedSeq : public CDialog
{
public:
	CuDlgDomPropUserXGrantedSeq(CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK()     {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropUserXGrantedSeq)
	enum { IDD = IDD_DOMPROP_USER_XGRANTED_SEQ };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CuListCtrlCheckmarks  m_cListCtrl;
	CImageList            m_ImageCheck;
	CuDomImageList        m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropUserXGrantedSeq)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
	void RefreshDisplay();
	void AddUserXGrantedSeq (CuGrantedSeq* pGrantedSeq);

private:
	CuDomPropDataUserXGrantedSeq m_Data;

protected:
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropUserXGrantedSeq)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif // DOMPROP_USER_XGRANTED_SEQ_HEADER
