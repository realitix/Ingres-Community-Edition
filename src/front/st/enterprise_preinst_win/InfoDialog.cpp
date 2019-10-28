/*
**  Copyright (c) 2001, 2006 Actian Corporation.
*/

/*
**  Name: InfoDialog.cpp : implementation file
**
**  Description:
**	This module is designed to display text information for the user.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**  10-Dec-2010 (drivi01)
**		Add a call to ConvertTextToIVW() to convert all the text
**		in the dialog to be relevant to Ingres VectorWise installation.
*/


#include "stdafx.h"
#include "enterprise.h"
#include "InfoDialog.h"



void ConvertTextToIVW(CDialog *dialog);

// InfoDialog dialog

IMPLEMENT_DYNAMIC(InfoDialog, CDialog)
InfoDialog::InfoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(InfoDialog::IDD, pParent)
{
}

InfoDialog::~InfoDialog()
{
}

void InfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL InfoDialog::OnInitDialog()
{	
	static int bReplaced = 0;
	
	if (thePreInstall.m_IngresVW)
		this->SetWindowText("Instance ID");
	else
		this->SetWindowText("Instance Name");

	if (!bReplaced)
	{
		ConvertTextToIVW(this);
		bReplaced = 1;
	}

	
	return CDialog::OnInitDialog();

}

BEGIN_MESSAGE_MAP(InfoDialog, CDialog)
END_MESSAGE_MAP()


// InfoDialog message handlers
