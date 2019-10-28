/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
**    Source   : fryeiass.h: interface for the CaFactoryImportExport class
**    Project  : COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : COM Class factory for Import/Export assistant Objects.
**
** History:
**
** 15-Oct-2000 (uk$so01)
**    Created
**/


#if !defined(FACTORY_IMPORTxEXPORT_HEADER)
#define FACTORY_IMPORTxEXPORT_HEADER
#include "bthread.h"



class CaFactoryImportExport : public IUnknown, public CaComThreaded
{
public:
	//
	// Main Object Constructor & Destructor.
	CaFactoryImportExport(IUnknown* pUnkOuter, CaComServer* pServer);
	~CaFactoryImportExport(void);

	//
	// IUnknown methods. Main object, non-delegating.
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

private:
	// We declare nested class interface implementations here.

	// We implement the IClassFactory interface (ofcourse) in this class
	// factory COM object class.
	class CaImpIClassFactory : public IClassFactory
	{
	public:
		//
		// Interface Implementation Constructor & Destructor.
		CaImpIClassFactory(){};
		CaImpIClassFactory(CaFactoryImportExport* pBackObj, IUnknown* pUnkOuter, CaComServer* pServer);
		~CaImpIClassFactory(void);
		void Init(CaFactoryImportExport* pBackObj, IUnknown* pUnkOuter, CaComServer* pServer)
		{
			m_cRefI = 0;
			m_pBackObj = pBackObj;
			m_pServer = pServer;

			if (NULL == pUnkOuter)
			{
				m_pUnkOuter = pBackObj;
//				INGCHSVR_TraceLog1("L<%X>: CaFactoryImportExport::CImpIClassFactory Constructor. Non-Aggregating.",TID);
			}
			else
			{
				m_pUnkOuter = pUnkOuter;
//				INGCHSVR_TraceLog1("L<%X>: CaFactoryImportExport::CImpIClassFactory Constructor. Aggregating.",TID);
			}
		}

		// IUnknown methods.
		STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		// IClassFactory methods.
		STDMETHODIMP         CreateInstance(IUnknown*, REFIID, LPVOID*);
		STDMETHODIMP         LockServer(BOOL);

	private:
		// Data private to this interface implementation of IClassFactory.
		ULONG          m_cRefI;       // Interface Ref Count (for debugging).
		CaFactoryImportExport* m_pBackObj;    // Parent Object back pointer.
		IUnknown*      m_pUnkOuter;   // Outer unknown for Delegation.
		CaComServer*   m_pServer;     // Server's control object.
	};

	// Make the otherwise private and nested IClassFactory interface
	// implementation a friend to COM object instantiations of this
	// selfsame CaFactoryImportExport COM object class.
	friend CaImpIClassFactory;

	// Private data of CaFactoryImportExport COM objects.

	// Nested IClassFactory implementation instantiation.
	CaImpIClassFactory m_ImpIClassFactory;

	// Main Object reference count.
	ULONG             m_cRefs;

	// Outer unknown (aggregation & delegation). Used when this
	// CaFactoryImportExport object is being aggregated.  Otherwise it is used
	// for delegation if this object is reused via containment.
	IUnknown*         m_pUnkOuter;

	// Pointer to this component server's control object.
	CaComServer*      m_pServer;
};



#endif // FACTORY_IMPORTxEXPORT_HEADER
