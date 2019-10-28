/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Actian Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : IpmDisp.cpp, Implementation file.                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : CuPageInformation, maintains the information for displaying           //
//               the property of IPM.                                                  //
//               It contain the number of pages, the Dialog ID, the Label for the      //
//               TabCtrl, .., for each item data of IPM Tree.                          //
//               CuIpmProperty is maintained for keeping track of the object being     //
//               displayed in the TabCtrl, its current page, ...                       //
/***************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"
#include "ipmdisp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CuPageInformation Implementation
// ********************************
IMPLEMENT_SERIAL (CuPageInformation, CObject, 1)
CuPageInformation::CuPageInformation()
{
    m_strTitle     = "";
    m_strClassName = ""; 
    m_nNumber      = 0;  
    m_pTabID       = NULL;         
    m_pDlgID       = NULL;      
    m_Ups          = new IPMUPDATEPARAMS;
    memset (m_Ups, 0, sizeof (IPMUPDATEPARAMS));
    m_crMask       = RGB (255, 0, 255);
    nImageType = -1;
    m_hIcon = NULL;
    m_nBitmap = 0;
    m_nIDTitle = 0;
}
   
CuPageInformation::CuPageInformation(CString strClass, int nSize, UINT* tabArray, UINT* dlgArray, UINT nIDTitle)
{
    m_strTitle     = "";
    m_strClassName = strClass; 
    m_nNumber      = nSize;  
    m_pTabID       = NULL;         
    m_pDlgID       = NULL;  
    m_Ups          = new IPMUPDATEPARAMS;
    memset (m_Ups, 0, sizeof (IPMUPDATEPARAMS));
    m_nIDTitle     = nIDTitle;
    if (m_nNumber > 0)
    {
        m_pTabID = new UINT [m_nNumber];
        m_pDlgID = new UINT [m_nNumber];
        for (int i=0; i<m_nNumber; i++)
        {
            m_pTabID [i] = tabArray [i];
            m_pDlgID [i] = dlgArray [i];
        }
    }
    nImageType = -1;
    m_hIcon = NULL;
    m_nBitmap = 0;
}


CuPageInformation::CuPageInformation(CString strClass)
{
    m_strTitle     = "";
    m_strClassName = strClass; 
    m_nNumber      = 0;  
    m_pTabID       = NULL;         
    m_pDlgID       = NULL;  
    m_Ups          = new IPMUPDATEPARAMS;
    memset (m_Ups, 0, sizeof (IPMUPDATEPARAMS));
    m_nIDTitle     = 0;
    nImageType = -1;
    m_hIcon = NULL;
    m_nBitmap = 0;
}
    
CuPageInformation::~CuPageInformation()
{
    if (m_pTabID)
        delete[] m_pTabID;
    if (m_pDlgID)
        delete[] m_pDlgID;
    if (m_Ups)
        delete m_Ups;
}

void CuPageInformation::SetUpdateParam (LPIPMUPDATEPARAMS ups)
{
    m_Ups->nType    = ups->nType;
    m_Ups->pStruct  = ups->pStruct;
    m_Ups->pSFilter = ups->pSFilter;
    m_Ups->nIpmHint = ups->nIpmHint;
}

CString CuPageInformation::GetClassName()   {return m_strClassName;}
int     CuPageInformation::GetNumberOfPage(){return m_nNumber;}
UINT*   CuPageInformation::GetTabID ()      {return m_pTabID;}
UINT*   CuPageInformation::GetDlgID ()      {return m_pDlgID;}
UINT    CuPageInformation::GetTabID (int nIndex)
{
    ASSERT(nIndex>=0 && nIndex <m_nNumber); 
    return m_pTabID[nIndex];
}

UINT    CuPageInformation::GetDlgID (int nIndex)
{
    ASSERT(nIndex>=0 && nIndex <m_nNumber); 
    return m_pDlgID[nIndex];
}

void CuPageInformation::GetTitle (CString& strTile)
{
    if (m_nIDTitle == 0)
        strTile = "";
    else
        strTile = m_strTitle;
}

void CuPageInformation::SetTitle (LPCTSTR  strItem, CTreeItem* pItem, int hNode)
{
    int ires;
    if (m_nIDTitle == 0)
    {
        m_strTitle = "";
        return;
    }
    if (m_strClassName == "CuTMSession" || m_strClassName == "CuTMBlockingSession"
                                        || m_strClassName == "CuTMSessBlockingSession" )
    {
        SERVERDATAMIN svrdta;
        ASSERT (hNode != -1);
        if (hNode == -1)
        {
            m_strTitle = "";
            return;
        }
        TCHAR tchszServer [100];
        ires = MonGetRelatedInfo (
            hNode,
            OT_MON_SESSION,
            pItem->GetPTreeItemData()? pItem->GetPTreeItemData()->GetDataPtr(): NULL,
            OT_MON_SERVER,
            &svrdta);
        if (ires == RES_SUCCESS) 
        {
            GetMonInfoName (hNode, OT_MON_SERVER, (void*)&svrdta, tchszServer, sizeof (tchszServer));
            AfxFormatString2 (m_strTitle, m_nIDTitle, (LPCTSTR)tchszServer, (LPCTSTR)strItem);
        }
        else
            AfxFormatString2 (m_strTitle, m_nIDTitle, VDBA_MfcResourceString (IDS_NOT_AVAILABLE), (LPCTSTR)strItem);//"<n/a>"
    }
    else
        AfxFormatString1 (m_strTitle, m_nIDTitle, (LPCTSTR)strItem);
}

void CuPageInformation::SetImage (UINT  nBitmapID)
{
    if (nBitmapID <= 0 )
    {
        nImageType = -1;
        m_hIcon = NULL;
        m_nBitmap = 0;
    }
    else
    {
        nImageType = 1;
        m_hIcon = NULL;
        m_nBitmap = nBitmapID;
    }
}

void CuPageInformation::SetImage (HICON hIcon)
{
    if (hIcon == NULL )
    {
        nImageType = -1;
        m_hIcon = NULL;
        m_nBitmap = 0;
    }
    else
    {
        nImageType = 0;
        m_hIcon = hIcon;
        m_nBitmap = 0;
    }
}

void CuPageInformation::Serialize (CArchive& ar)
{
    int i;
    if (ar.IsStoring())
    {
        ar << m_strTitle;
        ar << m_strClassName;
        ar << m_nIDTitle;
        ar << m_nNumber;
        if (m_nNumber > 0)
        {
            for (i=0; i<m_nNumber; i++)
            {
                ar << m_pTabID [i];
                ar << m_pDlgID [i];
            }
        }
        ar.Write(m_Ups, sizeof(IPMUPDATEPARAMS));
        ar << nImageType;
        ar << m_nBitmap;
    }
    else
    {
        ar >> m_strTitle;
        ar >> m_strClassName;
        ar >> m_nIDTitle;
        ar >> m_nNumber;
        if (m_nNumber > 0)
        {
            m_pTabID = new UINT [m_nNumber];
            m_pDlgID = new UINT [m_nNumber];
            for (i=0; i<m_nNumber; i++)
            {
                ar >> m_pTabID [i];
                ar >> m_pDlgID [i];
            }
        }
        //
        // Arcive only the structure, the valid pointer will be set
        // at the loading time !!!.
        ar.Read(m_Ups, sizeof(IPMUPDATEPARAMS));
        ar >> nImageType;
        ar >> m_nBitmap;
    }
}


//
// CuIpmProperty Implementation
// ****************************
IMPLEMENT_SERIAL (CuIpmProperty, CObject, 1)
CuIpmProperty::CuIpmProperty()
{
    m_nCurrentSelected = -1;
    m_pPageInfo = NULL;
    m_pPropertyData = NULL;
}

CuIpmProperty::CuIpmProperty(int nCurSel, CuPageInformation*  pPageInfo)
{
    m_nCurrentSelected = nCurSel;
    m_pPageInfo = pPageInfo;
    m_pPropertyData = NULL;
}

CuIpmProperty::~CuIpmProperty()
{
    if (m_pPropertyData) delete m_pPropertyData;
}

void CuIpmProperty::SetCurSel  (int nSel) {m_nCurrentSelected = nSel;}
void CuIpmProperty::SetPageInfo(CuPageInformation*  pPageInfo){m_pPageInfo = pPageInfo;}
int  CuIpmProperty::GetCurSel (){return m_nCurrentSelected;}
CuPageInformation* CuIpmProperty::GetPageInfo(){return m_pPageInfo;}
void CuIpmProperty::SetPropertyData (CuIpmPropertyData* pData)
{
    if (m_pPropertyData) delete m_pPropertyData;
    m_pPropertyData = pData;
}

void CuIpmProperty::Serialize (CArchive& ar)
{
    if (ar.IsStoring())
    {
        ar << m_nCurrentSelected;
        ar << m_pPageInfo;
        ar << m_pPropertyData;
    }
    else
    {
        try
        {
            ar >> m_nCurrentSelected;
            ar >> m_pPageInfo;
            ar >> m_pPropertyData;
        }
        catch (CMemoryException* e)
        {
            VDBA_OutOfMemoryMessage();
            e->Delete();
        }
    }
}
