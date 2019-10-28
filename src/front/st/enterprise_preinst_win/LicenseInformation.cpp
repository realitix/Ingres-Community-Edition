/*
**  Copyright (c) 2009 Actian Corporation
*/

/*
**  Name: LicenseInformation.cpp : implementation file
**  Description: License Agreement Dialog.
**
**  History:
**	02-Nov-2009 (drivi01)
**		Created.
**  23-Nov-2009 (drivi01)
**		If license file is removed, refer users to
**		pdf version of the license on the image.
**		Clean up the code a bit.
**		Update this dialog to mimic the behavior
**		of the license agreement dialog from 
**		Windows installer.
**  14-Feb-2011 (drivi01)
**      Cleanup the warnings.
**  10-Dec-2010 (drivi01)
**		Allow installer to set custom dialgo title depending on
**		the product by replacing PSP_USETITLE instead of PSP_DEFAULT.
**		Remove the code in OnWizardNext b/c the code is almost identical
**		to the code in enterprise.cpp.  
**		Add additional routines to figure out the next dialog
**		and make sure the code works for all 3 installers.
**		Add a call to ConvertTextToIVW().
**		
*/

#include "stdafx.h"
#include "enterprise.h"
#include "LicenseInformation.h"
#include "PreInstallation.h"


INT CompareIngresVersion(char *ii_system);
CString GetVersion(char *ii_system);
CString GetUpgradeVersion();
void ConvertTextToIVW(CDialog *dialog);
// LicenseInformation dialog

IMPLEMENT_DYNAMIC(LicenseInformation, CPropertyPage)

LicenseInformation::LicenseInformation()
	: CPropertyPage(LicenseInformation::IDD)
{
	m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_USETITLE|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LICTITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LICSUBTITLE);

}

LicenseInformation::~LicenseInformation()
{
}

void LicenseInformation::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(LicenseInformation, CPropertyPage)
END_MESSAGE_MAP()

static DWORD CALLBACK EStreamCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	
	CFile *cFile = (CFile*)dwCookie;
	
	if (cb > 0)
	{
	*pcb = (LONG)cFile->Read(pbBuff, cb); 
	}

	return 0;

}

BOOL LicenseInformation::OnInitDialog()
{

	EDITSTREAM es;
	DWORD dwFlags = SF_TEXT;
	LOGFONT lf; 
	TCHAR lic_loc[MAX_PATH+1];
	TCHAR *lic_end = NULL;
	TCHAR delim='\\';
	CRichEditCtrl *richEdit=NULL;

	//Create font for Rich Edit Control

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = 15; 
	lf.lfWeight=FW_THIN;
	strcpy(lf.lfFaceName, "Courier");    
	m_font.CreateFontIndirect(&lf);    // Create the font.

	//Set "not accept" radio as selected
	((CButton *)GetDlgItem(IDC_RADIO2))->SetCheck(1);
	((CButton *)GetDlgItem(IDC_RADIO1))->SetCheck(0);
				

	try
	{
		ZeroMemory((PVOID)&es, sizeof(EDITSTREAM));
		GetModuleFileName(AfxGetInstanceHandle(), lic_loc, MAX_PATH); 
		lic_end = _tcsrchr(lic_loc, delim);
		*lic_end='\0';
		_tcscat(lic_loc, "\\LICENSE");
		CFile cFile(lic_loc, CFile::modeRead);
		if (cFile != NULL)
		{
			//Setup EDITSTREAM
			es.dwCookie = (DWORD_PTR)&cFile;
			es.dwError = 0;
			es.pfnCallback = (EDITSTREAMCALLBACK)EStreamCallback;
			
			//Set Font in Rich Edit control
			richEdit = (CRichEditCtrl *)GetDlgItem(IDC_RICHEDIT21);
			richEdit->SetFont(&m_font);
	
			//Stream the file into the Rich Edit Control
			BOOL b = ::SendNotifyMessage(GetDlgItem(IDC_RICHEDIT21)->GetSafeHwnd(), EM_STREAMIN, (WPARAM)dwFlags, (LPARAM)&es);
			Invalidate();
		}
	}
	catch(CFileException* cfe)
	{
		TCHAR errorBuf[256];
		CString str;
		cfe->GetErrorMessage(errorBuf, 255);
		AfxMessageBox (errorBuf, MB_OK|MB_ICONEXCLAMATION);
		strcat(lic_loc, ".pdf");
		str.Format(IDS_ACCEPT_TERMS_TXT, lic_loc);
		if (richEdit == NULL)
			richEdit = (CRichEditCtrl *)GetDlgItem(IDC_RICHEDIT21);
		richEdit->SetWindowText(str);
	}
	catch(...)
	{
		AfxMessageBox(IDS_GENERIC_LIC, MB_OK|MB_ICONEXCLAMATION);
		exit(1);
	}

	richEdit->SetReadOnly();
	

	return CPropertyPage::OnInitDialog();
}

BOOL
LicenseInformation::OnSetActive()
{
	static int bReplaced = 0;

	property->GetDlgItem(ID_WIZBACK)->EnableWindow(TRUE);

	if (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2) == IDC_RADIO2)
	{
		property->GetDlgItem(ID_WIZNEXT)->EnableWindow(FALSE);
	}

	if (!bReplaced)
	{
		ConvertTextToIVW(this);
		bReplaced = 1;
	}

	return CPropertyPage::OnSetActive();
}


BOOL
LicenseInformation::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case IDC_RADIO1:
		if (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2) == IDC_RADIO1)
		{
			property->GetDlgItem(ID_WIZNEXT)->EnableWindow(TRUE);
			SendMessage(DM_SETDEFID, ID_WIZNEXT, 0);
			SetDefID(ID_WIZNEXT);
		}
		return TRUE;
	case IDC_RADIO2:
		if (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2) == IDC_RADIO2)
			property->GetDlgItem(ID_WIZNEXT)->EnableWindow(FALSE);
		return TRUE;
	}


	return CPropertyPage::OnCommand(wParam, lParam);

}


LRESULT 
LicenseInformation::OnWizardNext()
{

	if (thePreInstall.m_Installations.GetSize() <= 0 || 
		(thePreInstall.m_IngresVW && thePreInstall.m_IngresVW_count <= 0) ||
		(!thePreInstall.m_IngresVW && !thePreInstall.m_DBATools && thePreInstall.m_Ingres_count < 0))
	{
		CPropertyPage *pg = property->GetActivePage();
		int page = property->GetPageIndex(pg);
		property->SetActivePage(page+2);
	}

	return CPropertyPage::OnWizardNext();
}
// LicenseInformation message handlers
