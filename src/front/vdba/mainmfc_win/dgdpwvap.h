
/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2010 Actian Corporation. All Rights Reserved.		       //
//                                                                                     //
//  dgdpwvap.h : header file                                                           //
//                                                                                     // 
//  History:                                                                           //										 
//                                                                                     //
//    13-Dec-2010 (drivi01)                                                            //
//      Port the solution to x64. Clean up the warnings.                               //
//      Clean up datatypes. Port function calls to x64.                                //
****************************************************************************************/

#ifndef DOMPROP_ICE_SERVER_APPLICATIONS_HEADER
#define DOMPROP_ICE_SERVER_APPLICATIONS_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri3.h"
#include "domilist.h"  // CuDomImageList

class CuDlgDomPropIceServerApplications : public CDialog
{
public:
    CuDlgDomPropIceServerApplications(CWnd* pParent = NULL);  
    void AddRole (CuNameOnly *pIceServerApplications);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropIceServerApplications)
    enum { IDD = IDD_DOMPROP_ICE_SERVER_APPLICATIONS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CuDomImageList m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropIceServerApplications)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceServerApplications m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropIceServerApplications)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_ICE_SERVER_APPLICATIONS_HEADER
