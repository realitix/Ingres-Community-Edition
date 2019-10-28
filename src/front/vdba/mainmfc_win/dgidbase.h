/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Actian Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIDbase.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Detail page of Database.                       //  
//                                                                                     //
//    History  :                                                                       //
//      13-Dec-2010 (drivi01)                                                          //
//         Port the solution to x64. Clean up the warnings.                            //
//         Clean up datatypes. Port function calls to x64.                             //            
//                                                                                     //
****************************************************************************************/
#ifndef DGIDBASE_HEADER
#define DGIDBASE_HEADER
class CuDlgIpmDetailDatabase : public CFormView
{
protected:
    CuDlgIpmDetailDatabase();           
    DECLARE_DYNCREATE(CuDlgIpmDetailDatabase)

public:
    // Dialog Data
    //{{AFX_DATA(CuDlgIpmDetailDatabase)
    enum { IDD = IDD_IPMDETAIL_DATABASE };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgIpmDetailDatabase)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    virtual ~CuDlgIpmDetailDatabase();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgIpmDetailDatabase)
    virtual void OnInitialUpdate();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    afx_msg LRESULT OnUpdateData (WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};
#endif
