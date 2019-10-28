/*
** Copyright (c) 2003, 2011 Actian Corporation. All Rights Reserved.
**
** History:
**	12-Jan-11 (thoda04) B122661
**	    Upgraded obsolete managed C++ syntax to support 64-bit.
*/

#include "windows.h"                // Windows stuff
#include "windowsx.h"               // More Windows stuff
#include "xa.h"

#using "Ingres.Client.dll"
using namespace System;
using namespace Ingres::ProviderInternals;
using namespace System::Runtime::InteropServices;


namespace Ingres{
	namespace Support
{
typedef struct  xa_switch_t  IIXA_SWITCH;
IIXA_SWITCH * piixa_switch = NULL;

ref class XAProvider
{
public:
	static Ingres::ProviderInternals::XASwitch^  xasw;
};

typedef DWORD XA_SWITCH_FLAGS;

Ingres::ProviderInternals::AdvanXID^ newAdvanXID(XID * xid);
Ingres::ProviderInternals::XAXID^         newXid(XID * xid);
void CopyAdvanXidToXID(
	Ingres::ProviderInternals::AdvanXID^ advanXID, XID * xid);


/* local XA functions */
int xa_open    (char *, int, long);       /* xa_open function pointer */
int xa_close   (char *, int, long);       /* xa_close function pointer*/
int xa_start   (XID *, int, long);        /* xa_start function pointer */
int xa_end     (XID *, int, long);        /* xa_end function pointer */
int xa_rollback(XID *, int, long);        /* xa_rollback function pointer */
int xa_prepare (XID *, int, long);        /* xa_prepare function pointer */
int xa_commit  (XID *, int, long);        /* xa_commit function pointer */
int xa_recover (XID *, long, int, long);  /* xa_recover function pointer*/
int xa_forget  (XID *, int, long);        /* xa_forget function pointer */
int xa_complete(int *, int *, int, long); /* xa_complete function pointer */



struct xa_switch_t xa_switch= {
    "INGRES VIA .NET Data Provider",  /* name of resource manager */
    TMNOFLAGS | TMNOMIGRATE,       /* resource manager specific options */
    0,                             /* version; must be 0       */
    xa_open,
    xa_close,
    xa_start,
    xa_end,
    xa_rollback,
    xa_prepare,
    xa_commit,
    xa_recover,
    xa_forget,
    xa_complete
};

/*
**  GetXaSwitch
**
**  Return pointer to the XA switch.
**
**  This function is implemented and exported by the ODBC XA TM Interface.
**  After the MS XA transaction manager (TM) LoadLibrary's the Ingres ODBC driver,
**  it obtains the function by calling GetProcAddress.  The TM then
**  invokes this function to obtain a pointer to the XA switch.
**
**  On entry: XaSwitchFlags      Any flags that provide information about
**                               the TM making the call.  Currently
**                               XA_SWITCH_F_DTC is the only defined flag.
**            ppXaSwitch      -->to the pointer to XA Switch to be returned.
**
**  Returns:  S_OK            Success
**            E_FAIL          Unable to provide the XA switch
**            E_INVALIDARG    One or more of the parameters are not valid
*/

extern "C" __declspec(dllexport) HRESULT GetXaSwitch(
	XA_SWITCH_FLAGS XaSwitchFlags,  struct xa_switch_t** ppXaSwitch)
{
	if (!ppXaSwitch)
		return(E_INVALIDARG);

	*ppXaSwitch = &xa_switch;   // table of functions within this module
	XAProvider::xasw = gcnew Ingres::ProviderInternals::XASwitch();

	return S_OK;
}


int xa_open    (char * xa_info, int rmid, long flags)
{
	int rc = XAProvider::xasw->xa_open(gcnew String(xa_info), rmid, flags);
	return(rc);
}

int xa_close   (char * xa_info, int rmid, long flags)
{
	int rc = XAProvider::xasw->xa_close(gcnew String(xa_info), rmid, flags);
	return(rc);
}

int xa_start   (XID * xid, int rmid, long flags)
{
	int rc = XAProvider::xasw->xa_start(newAdvanXID(xid), rmid, flags);
	return(rc);
}

int xa_end     (XID * xid, int rmid, long flags)
{
	int rc = XAProvider::xasw->xa_end(newAdvanXID(xid), rmid, flags);
	return(rc);
}

int xa_rollback(XID * xid, int rmid, long flags)
{
	int rc = XAProvider::xasw->xa_rollback(newAdvanXID(xid), rmid, flags);
    return(rc);
}

int xa_prepare (XID * xid, int rmid, long flags)
{
	int rc=XAProvider::xasw->xa_prepare(newAdvanXID(xid), rmid, flags);
	return(rc);
}

int xa_commit  (XID * xid, int rmid, long flags)
{
	int rc=XAProvider::xasw->xa_commit(newAdvanXID(xid), rmid, flags);
	return(rc);
}

int xa_recover (XID * xids, long count, int rmid, long flags)
{
	array<AdvanXID^>^  advanXids  = nullptr;
	int xidCount = XAProvider::xasw->xa_recover(advanXids, count, rmid, flags);
	if (xidCount <= 0)  // return if all done or in error
		return xidCount;

	for (int i = 0; i < xidCount; i++)  // convert and copy back XID to caller
	{
		CopyAdvanXidToXID(advanXids[i], &xids[i]);
	}

	return(xidCount);
}

int xa_forget  (XID * xid, int rmid, long flags)
{
	int rc=XAProvider::xasw->xa_forget(newAdvanXID(xid), rmid, flags);
	return(rc);
}

int xa_complete(int * handle, int * retval, int rmid, long flags)
{
	int handleValue = *handle;      // stage the native stack values 
	int %phandle    = handleValue;  //   onto managed heap
	int retValue    = *retval;
	int %pretval    = retValue;

	int rc=XAProvider::xasw->xa_complete(phandle, pretval, rmid, flags);

	*retval = retValue;
	return(rc);
}


Ingres::ProviderInternals::AdvanXID^ newAdvanXID(XID * xid)
{
	if (xid == NULL)
		return nullptr;

	int  formatID     = (int)(xid->formatID); // format identifier 
	long gtridLength  = xid->gtrid_length;    // global tx qualifier length
	long bqualLength  = xid->bqual_length;    // branch qualifier length
	char* dataGtrid   = xid->data;            // qualifiers data bytes;
	char* dataBqual   = dataGtrid+gtridLength; // qualifiers data bytes;

	// safety checks
	if (gtridLength > 64)  // too big
		gtridLength = 64;
	if (gtridLength < 0)   // too negative
		gtridLength = 0;
	if (bqualLength > 64)
		bqualLength = 64;
	if (bqualLength < 0)
		bqualLength = 0;


	array<Byte>^ gtridBytes  = gcnew array<Byte>(gtridLength);
	if (gtridLength > 0  &&  gtridLength <= 64)
	{
	pin_ptr<BYTE> pinGtrid = &(gtridBytes[0]);     // pin managed array
	CopyMemory(pinGtrid, dataGtrid, gtridLength);  // copy bytes to .NET object
	pinGtrid = nullptr;                            // release pinned reference;
	}

	array<Byte>^ bqualBytes  = gcnew array<Byte>(bqualLength);
	if (bqualLength > 0  &&  bqualLength <= 64)
	{
	pin_ptr<BYTE> pinBqual = &(bqualBytes[0]);     // pin managed array
	CopyMemory(pinBqual, dataBqual, bqualLength);  // copy bytes
	pinBqual = nullptr;                            // release pinned reference;
	}

	Ingres::ProviderInternals::AdvanXID^ advanXID =
		gcnew Ingres::ProviderInternals::AdvanXID(formatID, gtridBytes, bqualBytes);
	return advanXID;
}


Ingres::ProviderInternals::XAXID^ newXid(XID * xid)
{
	int   formatID     = (int)(xid->formatID); // format identifier 
	long  gtridLength  = xid->gtrid_length;    // global tx qualifier length
	long  bqualLength  = xid->bqual_length;    // branch qualifier length
	char* data         = xid->data;            // qualifiers data bytes;
	int   length   = (int)(gtridLength+bqualLength); // length of bytes;

	array<Byte>^ dataBytes  = gcnew array<Byte>(length);
	if (length != 0)
	{
		pin_ptr<BYTE> pinData = &(dataBytes[0]);    // pin managed array
		CopyMemory(pinData, data, length);          // copy bytes to .NET object
		pinData = nullptr;                          // release pinned reference;
	}

	Ingres::ProviderInternals::XAXID^ Xid =
		gcnew Ingres::ProviderInternals::XAXID(
			formatID, gtridLength, bqualLength, dataBytes);
	return Xid;
}


void CopyAdvanXidToXID(Ingres::ProviderInternals::AdvanXID^ advanXID, XID * xid)
{
	xid->formatID     = advanXID->FormatId;
	xid->gtrid_length = advanXID->getGlobalTransactionId()->Length;
	xid->bqual_length = advanXID->getBranchQualifier()->Length;


	long  glength = xid->gtrid_length;  // global tx qualifier length
	long  blength = xid->bqual_length;  // branch qualifier length
	char* data    = xid->data;          // qualifiers data bytes;

	if (glength > 0)
	{
		pin_ptr<BYTE> pinData = &(advanXID->getGlobalTransactionId()[0]); // pin managed array
		CopyMemory(data, pinData, glength);         // copy bytes from .NET object
		pinData = nullptr;                          // release pinned reference;
		data += glength;                            // position to bqual
	}

	if (blength > 0)
	{
		pin_ptr<BYTE> pinData = &(advanXID->getBranchQualifier()[0]); // pin managed array
		CopyMemory(data, pinData, blength);         // copy bytes from .NET object
		pinData = nullptr;                          // release pinned reference;
	}
}


//
//  DllMain
//
//  Win32 DLL entry and exit routines.
//  Replaces Libmain and WEP used in 16 bit Windows.
//
//  Input:   hmodule     = Module handle (for 16 bit DLL's this was the
//                         instance handle, but it is used the same way).
//
//           fdwReason   = DLL_PROCESS_ATTACH:
//                         DLL_THREAD_ATTACH:
//                         DLL_THREAD_DETACH:
//                         DLL_PROCESS_DETACH:
//
//           lpvReserved = just what it says.
//
//  Output:  Finds path and options for FD processing.
//
//  Returns: TRUE
//
//
//extern "C" __declspec(dllexport) 
BOOL APIENTRY DllMain (
	HMODULE hmodule,
	DWORD   fdwReason,
	LPVOID  lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		XAProvider::xasw = nullptr;  // make sure it's released
		break;
	}
	return TRUE;
}  // end DllMain

}}  // namespace