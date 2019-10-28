/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
*/

/*
** Source   : serproc.h, header File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Procedure in the compound file.
**           
** History:
**
** 18-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#if !defined (_VSDSERIALIZE_PROCEDURE_HEADER)
#define _VSDSERIALIZE_PROCEDURE_HEADER
#include "snaparam.h"
#include "compfile.h"


BOOL VSD_StoreProcedure (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile);

#endif // _VSDSERIALIZE_PROCEDURE_HEADER

