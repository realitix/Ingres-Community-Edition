/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comsynon.cpp: implementation of the CmtColumn, CmtFolderColumn class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Column object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "comaccnt.h"
#include "comdbase.h"
#include "comcolum.h"
#include "ingobdml.h"
#include "a2stream.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// ************************************************************************************************
//                                 LOCAL FUNCTIONS
// ************************************************************************************************
static HRESULT CopyColumns2Stream(CTypedPtrList< CObList, CmtColumn* >& listObject, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	IStream* pStreamBuffer = NULL;
	//
	// Prepare to transfer data through the ISream interface:
	try
	{
		CMemFile file;
		CArchive ar(&file, CArchive::store);

		//
		// List of objects to store to the Archive;
		CTypedPtrList <CObList, CaColumn*> listObject2Store;

		CaColumn* pObj = NULL;
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CmtColumn* pMtObj = listObject.GetNext(pos);
			pMtObj->GetColumn (pObj);
			listObject2Store.AddTail (pObj);
		}
		//
		// Serialize the list of objects and cleanup the list.
		listObject2Store.Serialize (ar);
		while (!listObject2Store.IsEmpty())
			delete listObject2Store.RemoveHead();

		//
		// Force everything to be written to File (memory file)
		ar.Flush();
		CArchiveToIStream (ar, &pStreamBuffer);
	}
	catch (...)
	{
		hError = E_FAIL;
		if (pStreamBuffer)
			pStreamBuffer->Release();
		pStreamBuffer = NULL;
	}

	pStream = pStreamBuffer;
	return hError;
}



//
// Object: Procedure
// *****************
CmtSessionManager* CmtColumn::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}
//
// Folder of Procedure:
// ********************
CmtFolderColumn::~CmtFolderColumn()
{
	CmtMutexReadWrite mAccess(this);
	CmtWaitUntilNoAccess mWait(this);

	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

CmtSessionManager* CmtFolderColumn::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}

CmtColumn* CmtFolderColumn::SearchObject(CaLLQueryInfo* pInfo, CaColumn* pObj, BOOL b2QueryIfNotDone)
{
	if (b2QueryIfNotDone && GetLastQuery() == -1)
	{
		IStream* pStream = NULL;
		BOOL bCopy2Stream = FALSE;
		HRESULT hError = GetListObject (pInfo, pStream, bCopy2Stream);
		ASSERT (pStream == NULL);
		if FAILED(hError)
			return NULL;
	}

	//
	// Block the threads that want to modify (update) the list of objects:
	CmtAccess access(this);

	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CmtColumn* pMtObj = m_listObject.GetNext (pos);
		CaColumn& obj = pMtObj->GetColumn();
		if (obj.Matched(pObj))
		{
			pMtObj->Access();
			return pMtObj;
		}
	}

	return NULL;
}


HRESULT CmtFolderColumn::GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream)
{
	HRESULT hError = NOERROR;
	pStream = NULL;
	//
	// Wait until another thread has finised updating the list of nodes:
	CmtMutexReadWrite mReadWrite(this);
	//
	// At this point, All threads want to read or update are blocked by 'mReadWrite' mutex.
	// All entries to the cache are blocked !

	if (GetLastQuery() != -1)
	{
		CmtAccess access(this);
		//
		// Release all blockeing threads that wait for 'mReadWrite' mutex:
		mReadWrite.UnLock();
		if (bCopy2Stream)
			hError = CopyColumns2Stream (m_listObject, pStream);
		return hError;
	}

	// ALL ACCESSES TO THE CACHE HAVE BEEN BLOCKED BUT
	// Wait until all threads have finished referenced to this object,
	// because this object would be destroyed while the thread might have access to 
	// the descendants of this object.
	// Ex: Another thread might be querying the list of users of a given node
	//     that would be removed.
	CmtWaitUntilNoAccess mWait(this);

	CmtDatabase* pmtDatabase = NULL;
	//
	// CmtTable or CmtView or CmtIndex:
	ASSERT (m_pBackParent);
	if (!m_pBackParent)
		return E_FAIL;

	int nOTType = OBT_TABLECOLUMN;
	CString strItem;
	CString strItemOwner;

	if (m_nFolderType == F_COLUMN_TABLE)
	{
		nOTType = OBT_TABLECOLUMN;
		ASSERT (m_pBackParent->IsKindOf(RUNTIME_CLASS(CmtTable)));
		if (m_pBackParent->IsKindOf(RUNTIME_CLASS(CmtTable)))
		{
			CaTable& table =((CmtTable*)m_pBackParent)->GetTable();
			CmtFolderTable* pParent2 = ((CmtTable*)m_pBackParent)->GetBackParent();
			pmtDatabase = pParent2->GetBackParent();
			ASSERT (pmtDatabase);
			if (!pmtDatabase)
				return E_FAIL;

			strItem = table.GetName();
			strItemOwner = table.GetOwner();
		}
	}
	else
	if (m_nFolderType == F_COLUMN_VIEW)
	{
		nOTType = OBT_VIEWCOLUMN;
		ASSERT (m_pBackParent->IsKindOf(RUNTIME_CLASS(CmtView)));
		if (m_pBackParent->IsKindOf(RUNTIME_CLASS(CmtView)))
		{
			CaView& view =((CmtView*)m_pBackParent)->GetView();
			CmtFolderView* pParent2 = ((CmtView*)m_pBackParent)->GetBackParent();
			pmtDatabase = pParent2->GetBackParent();
			ASSERT (pmtDatabase);
			if (!pmtDatabase)
				return E_FAIL;

			strItem = view.GetName();
			strItemOwner = view.GetOwner();
		}
	}
	else
	if (m_nFolderType == F_COLUMN_INDEX)
	{
		/*
		nOTType = OBT_TABLECOLUMN;
		ASSERT (m_pBackParent->IsKindOf(RUNTIME_CLASS(CmtIndex)));
		if (m_pBackParent->IsKindOf(RUNTIME_CLASS(CmtIndex)))
		{
			CaIndex& index =((CmtIndex*)m_pBackParent)->GetIndex();
			CmtFolderIndex* pParent2 = ((CmtIndex*)m_pBackParent)->GetBackParent();
			pmtDatabase = pParent2->GetBackParent();
			ASSERT (pmtDatabase);
			if (!pmtDatabase)
				return E_FAIL;

			strItem = index.GetName();
			strItemOwner = index.GetOwner();
		}
		*/
	}

	//
	// CmtDatabase:
	ASSERT (pmtDatabase);
	if (!pmtDatabase)
		return E_FAIL;
	CaDatabase& database = pmtDatabase->GetDatabase();

	//
	// CmtFolderDatabase:
	CmtFolderDatabase* pParent3 = pmtDatabase->GetBackParent();
	ASSERT (pParent3);
	if (!pParent3)
		return E_FAIL;
	//
	// CmtNode:
	CmtNode* pParent4 = pParent3->GetBackParent();
	ASSERT (pParent4);
	if (!pParent4)
		return E_FAIL;
	CaNode& node = pParent4->GetNode();

	//
	// Recursive until reaching the Account Folder and return the 
	// &m_sessionMager of that account:
	CmtSessionManager* pOpaqueSessionManager = GetSessionManager();
	CTypedPtrList< CObList, CaDBObject* > lNew;
	pInfo->SetObjectType(nOTType);
	pInfo->SetNode (node.GetName());
	pInfo->SetDatabase(database.GetName());
	pInfo->SetItem2(strItem, strItemOwner);

	BOOL bOk = INGRESII_llQueryObject (pInfo, lNew, pOpaqueSessionManager);
	if (!bOk)
	{
		//
		// TODO:HANDLE ERRROR

		return E_FAIL;
	}

	//
	// Mark all old node as being deleted:
	CmtColumn* pMtObj = NULL;
	POSITION p = NULL, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		pMtObj = m_listObject.GetNext (pos);
		pMtObj->SetState (CmtItemData::ITEM_DELETE);
	}
	
	while (!lNew.IsEmpty())
	{
		CaColumn* pObj = (CaColumn*)lNew.RemoveHead();
		CmtColumn* pObjExist = SearchObject(pInfo, pObj, FALSE);

		//
		// The new queried object already exists in the old list, we destroy it !
		if (pObjExist != NULL)
		{
			pObjExist->SetState (CmtItemData::ITEM_EXIST);
			delete pObj;
			continue;
		}

		//
		// New object that is not in the old list, add it to the list:
		CmtColumn* pNewMtObject = new CmtColumn (pObj);

		pNewMtObject->SetBackParent (this);
		m_listObject.AddTail (pNewMtObject);

		delete pObj;
	}

	//
	// Delete all items that have the flag = ITEM_DELETE.
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pMtObj = m_listObject.GetNext (pos);
		if (pMtObj && pMtObj->GetState () == CmtItemData::ITEM_DELETE)
		{
			m_listObject.RemoveAt (p);
			delete pMtObj;
		}
	}

	if (bCopy2Stream)
		hError = CopyColumns2Stream (m_listObject, pStream);
	SetLastQuery();

	//
	// Release all blockeing threads that wait for 'mWrite' mutex:
	mReadWrite.UnLock();

	return hError;
}

