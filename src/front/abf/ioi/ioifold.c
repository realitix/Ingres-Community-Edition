
/*
**	Copyright (c) 1986, 2004 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include "ioistd.h"
#include <ds.h>
#include <feds.h>
#ifdef MSDOS
#include <clstatus.h>
#endif

/**
** Name:	ioifld.c - form load
**
** Description:
**	load form from image file
**
** History:
**	12/10/89 (dkh) - Using pointer to DS template array defined in frame
**			 as part of VMS shared lib changes.
**	09/06/90 (dkh) - Changed to use IIFDgfdGetFdDstab() instead of
**			 IIFDdstab directly.
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
*/

FUNC_EXTERN DSTARRAY *IIFDgfdGetFdDstab();

#ifdef PCINGRES
extern bool ME_recoverable;
#endif

/*{
** Name:	IIOIflFormLoad - load form from image file
**
** Description:
**	Uses DS to load a form into memory from an image file
**
** Inputs:
**	ohdr	OLIMHDR describing image file
**	fname	file name
**	fpos	position of form in image file
**	tag	storage tag for form memory
**
** Outputs:
**	faddr	returned form address
**
**	return:
**		OK		success
**
** Side Effects:
**
** History:
**	10/86 (rlm)	written
**
*/

STATUS
IIOIflFormLoad(ohdr,fname,fpos,tag,faddr)
OLIMHDR *ohdr;
char *fname;
i4 fpos;
i2 tag;
PTR *faddr;
{
	SH_DESC	sh_desc;
	FILE *fp;
	STATUS DSload();
	STATUS rc;
#ifdef PCINGRES
	bool oldreco;
#endif

	fp = ohdr->fp;

	if (SIfseek(fp,fpos,SI_P_START) != OK)
		return (ILE_FRDWR);

	sh_desc.sh_type = IS_RACC;
	sh_desc.sh_reg.file.fp = fp;
	sh_desc.sh_keep = TRUE;
	sh_desc.sh_tag = tag;

	/*
	** For PCINGRES, DSload call is bracketed with flag setting for ME
	** to handle allocation failure.  This flag allows DS to handle
	** an exception generated by ME, rather than causing a syserr()
	*/
#ifdef PCINGRES
	oldreco = ME_recoverable;
	ME_recoverable = TRUE;
	rc = DSload(&sh_desc, (i4 **) faddr, IIFDgfdGetFdDstab());
	ME_recoverable = oldreco;
#else
	rc = DSload(&sh_desc,  faddr, IIFDgfdGetFdDstab());
#endif

	switch ((int) rc)
	{
	case OK:
		return (OK);
#ifdef MSDOS
	case ME_GONE:
		return (ILE_NOMEM);
#endif
	default:
		return (ILE_FRDWR);
	}
}
