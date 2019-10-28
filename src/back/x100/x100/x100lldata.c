/*
 * Copyright (C) 2009-2011 VectorWise B.V.
 * All Rights Reserved.
 *
 * Maintainer: kschendel (Ingres-ified from giel/michal original)
 *
 * History:
 *	06-Jun-2011 (drivi01)
 *	    Created.
 *	    Redefine x100_global_ctx as global variable instead of
 *	    static and export it in iilibdmfdata on Windows.
 *	08-Jun-2011 (drivi01)
 *	    Move x100_x100cb_offset global define to this file to
 *	    properly export it on Windows in iilibdmfdata.dll.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
 */
#include <compat.h>
#include <iicommon.h>
#include <cs.h>
#include <x100ll.h>


GLOBALDEF X100Context *x100_global_ctx = NULL;
GLOBALDEF i4 x100_x100cb_offset = 0;

