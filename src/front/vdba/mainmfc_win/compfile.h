/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : compfile.h: header file.
**    Project  : AxtiveX Component
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Compound File Management
**
** History:
**
** 07-Mar-2002 (uk$so01)
**    Created
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Apr-2008 (drivi01)
**    Add CreateDirectoryRecursively function.
** 08-Sep-2009 (drivi01)
**    Add IsDBATools function defintion.
**/

#if !defined (COMPOUND_FILE_MANAGEMENT_HEADER)
#define COMPOUND_FILE_MANAGEMENT_HEADER
class CeException
{
public:
	CeException():m_lErrorCode(-1){};
	CeException(LONG lErrorCode):m_lErrorCode(lErrorCode){};
	LONG GetErrorCode(){return m_lErrorCode;}
	void SetErrorCode(LONG lErrorCode){m_lErrorCode=lErrorCode;}
protected:
	LONG  m_lErrorCode;

};

class CeCompoundFileException: public CeException
{
public:
	CeCompoundFileException():CeException(){};
	CeCompoundFileException(LONG lErrorCode):CeException(lErrorCode){};
	~CeCompoundFileException(){};
};

//
// CLASS: CaCompoundFile 
// ************************************************************************************************
class CaCompoundFile
{
public:
	CaCompoundFile(LPCTSTR lpszFileName, DWORD grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
	CaCompoundFile(IStorage* pStorage);
	~CaCompoundFile();

	BOOL FindElement(IStorage* pParent, LPCTSTR lpszName, DWORD dwElemntType);
	IStorage* GetRootStorage(){return m_pIStorage;}
	IStorage* NewStorage (IStorage* pParent, LPCTSTR lpszName, DWORD grfMode = STGM_FAILIFTHERE);
	IStorage* OpenStorage(IStorage* pParent, LPCTSTR lpszName, DWORD grfMode);

	IStream* NewStream (IStorage* pIStorage, LPCTSTR lpszName, DWORD grfMode = STGM_FAILIFTHERE);
	IStream* OpenStream(IStorage* pParent, LPCTSTR lpszName, DWORD grfMode);
	int CreateDirectoryRecursively(char *szDirectory);
	BOOL IsDBATools(CString strII);
protected:
	IStorage* m_pIStorage;

};



//
// Utilities Functions
// ************************************************************************************************
BOOL SaveStreamInit (IUnknown* pUnk, LPCTSTR lpszStreamName, IStorage* pStorage = NULL);
BOOL OpenStreamInit (IUnknown* pUnk, LPCTSTR lpszStreamName, IStorage* pStorage = NULL);

class CaPersistentStreamState: public COleStreamFile
{
public:
	CaPersistentStreamState(LPCTSTR lpszStreamName, BOOL bLoad);
	~CaPersistentStreamState()
	{
		if (m_pcpFile)
			delete m_pcpFile;
	}

	//
	// Before using the file, you must check by calling IsStateOK()!
	BOOL IsStateOK(){return m_bStateOk;}

protected:
	CaCompoundFile* m_pcpFile;
	BOOL m_bStateOk;

};


#endif // COMPOUND_FILE_MANAGEMENT_HEADER
