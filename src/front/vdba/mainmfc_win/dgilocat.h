/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIlocat.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Detail page of Location.                       //              
//                                                                                     //
//                                                                                     //
//    History  :                                                                       //
//      13-Dec-2010 (drivi01)                                                          //
//         Port the solution to x64. Clean up the warnings.                            //
//         Clean up datatypes. Port function calls to x64.                             //
****************************************************************************************/
#ifndef DGILOCAT_HEADER
#define DGILOCAT_HEADER
#include "statfrm.h"
#include "statlege.h"
extern "C"
{
#include "monitor.h"
}

class CuDlgIpmDetailLocation: public CDialog
{
public:
    CuDlgIpmDetailLocation(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgIpmDetailLocation)
    enum { IDD = IDD_IPMDETAIL_LOCATION };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuStatisticBarLegendList    m_cListLegend;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgIpmDetailLocation)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

    // Implementation
protected:
    CuStatisticBarFrame* m_pDlgFrame;
    LOCATIONDATAMIN m_locStruct;
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgIpmDetailLocation)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetData    (WPARAM wParam, LPARAM lParam);
};
#endif
