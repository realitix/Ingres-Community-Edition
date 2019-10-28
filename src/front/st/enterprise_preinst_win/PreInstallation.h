/*
**  Copyright (c) 2001 Actian Corporation
*/

/*
**  Name: PreInstallation.h: interface for the CPreInstallation class.
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	07-Jun-2001 (penga03)
**	    Remove the obsolete member variable m_ModulePath.
**	23-July-2001 (penga03)
**	    Added m_Ver25.
**	05-nov-2001 (penga03)
**	    Added a new class CInstallation.
**	    Added a new member variable m_Installations in class CPreInstallation.
**	31-dec-2001 (penga03)
**	    Removed m_SkipLicenseCheck.
**	30-jul-2002 (penga03)
**	    Added m_embedded and Modified AddInstallation() and CInstallation().
**	30-aug-2002 (penga03)
**	    Added m_MSILog to hold the path of the log file.
**	16-jul-2004 (penga03)
**	    Added m_InstallMDB.
**	10-sep-2004 (penga03)
**	    Removed MDB.
**	13-dec-2004 (penga03)
**	    Added m_UpgradeType, m_RestartIngres.
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**		Added a few variables to support new properties s.a. displaying of
**		version number in install list as well as upgrading of user databases,
**		InstallType, and express/advanced install feature.
**  01-Mar-2007 (drivi01)
**	    Add variables for routines that overwrite wizard banner picture.
**  25-May-2007 (drivi01)
**	    Added new property to installer INGRES_WAIT4GUI which will force 
**	    setup.exe to wait for post installer to finish if this property 
**	    is set to 1. INGRES_WAIT4GUI which is referred to as m_Wait4GUI
**	    within setup.exe is set to TRUE if "/w" flag is passed to setup.exe.
**	    /w is a new flag within setup.exe.
**  22-Jun-2007 (horda03)
**		Parameter added to LaunchWindowsInstaller
**  04-Apr-2008 (drivi01)
**		Adding functionality for DBA Tools installer to the Ingres code.
**		DBA Tools installer is a simplified package of Ingres install
**		containing only NET and Visual tools component.  
**		Update the necessary routines to work with DBA installer update/copy
**		msi files and msi database in the context relative to DBA installer.
**  14-Jul-2009 (drivi01)
**		SIR 122309
**		Added new dialog to specify Configuration System for ingres instance.
**		The dialog contains 4 options which will be represented as
**		0 for Transactional System
**		1 for Traditional Ingres System
**		2 for Business Intelligence System
**		3 for Content Management System
**  17-Sep-2009 (drivi01)
**		Add IsPre92Release function.
**  04-Nov-2009 (drivi01)
**	    Add m_DBATools_count variable to keep track of total number of
**	    DBA Tools installations on the machine.
**	    Add prototypes for a few functions.
**  10-Dec-2010 (drivi01)
**		Add new fields to keep track of Ingres VectorWise installations.
**      Add prototype ConvertTextToIVW().
**  22-Jun-2011 (drivi01)
**	Add m_ii_system to the list of fields defined in CPreInstallation class.
**	This variable will contain ii_system path so we don't have to keep
**	figuring it out.
**  12-Jul-2011 (drivi01) b125546
**      Add m_DBAToolsVW to be set for DBA Tools isntallations targeted
**      for VW server.  This will ensure the defaults more relevant
**      for VW are selected.
*/

#if !defined(AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_)
#define AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPreInstallation  
{
public:
	CString m_MSILog;
	CObArray m_Installations;
	CString m_Ver25;
	CString m_ResponseFile;
	CString m_CreateResponseFile;
	CString m_EmbeddedRelease;
	CString m_InstallCode;
	CString m_ii_system;
	BOOL	m_Wait4GUI;
	BOOL m_DBATools;  //variable global to CPreInstallation object, used to determine if current installer running is DBATools installer.
	BOOL m_DBAToolsVW; //variable global to CPreInstallation object, used to determine if current installer running DBA Tools installer targeted for VW with VW default options
	BOOL m_IngresVW;  //Variable global to CPreInstallation object, used to determine if current setup is for Ingres VectorWise product.
	int m_DBATools_count; //Number of DBA Tools installations on the machine
	int m_IngresVW_count; //Number of Ingres VectorWise installations on the machine
	int m_Ingres_count;   // Number of Ingres installations on the machine
	
	CPreInstallation();
	virtual ~CPreInstallation();
	BOOL LaunchWindowsInstaller(BOOL CheckReg);
	CString CalculateInstallCode();
	CString CalculateInstallCode(char *);
	int IsPre92Release(CString version);
	INT CompareIngresVersion(char *ii_system);
	CString GetVersion(char *ii_system);
	void ConvertTextToIVW(CDialog *dialog);
private:
	void FindOldInstallations();
	void AddInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded);
public:
	int m_InstallType;  /* 0 - Upgrade, 1 - Modify, 2 - New Install */
	int m_UpgradeType;
	int m_RestartIngres;
	int m_UpgradeDatabases; /* User has to decide early on in upgrade if they want to upgrade user databases. */
	int m_express; /* 1 - Perform Express Install, 0 - Perform Advanced Install */
	int m_ConfigType; /* 0 - Transactional System, 1 - Traditional Ingres System, 2 - BI, 3 - Content Mgmt */
	C256bmp					m_bmpHeader; /* store banner bitmap */
	int						m_eHeaderStyle; /* The style of bitmap, in this case it's stretched "1" stands for stretched */
	UINT					m_uHeaderID; /* banner bitmap resource id */
	CBrush					m_HollowBrush; /* brush */
};

class CInstallation : public CObject  
{
public:
	CInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded);
	CString m_embedded;
	BOOL m_ver25;
	CString m_path;
	CString m_id;
	int m_UpgradeCode;
	CString m_ReleaseVer; /* variable that is used to store release number to be displayed in instance list */
	CString m_BuildNum;   /* variable that is used to store build number of the release to be displayed in instance list */
	BOOL m_isDBATools; /* variable determines if installation is DBA Tools or full Ingres installation, set in CInstallation object */
	BOOL m_isIVW;      /* variable is set if the installation is an Ingres VectorWise installation. */
	CInstallation();
	virtual ~CInstallation();

};

#endif // !defined(AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_)
