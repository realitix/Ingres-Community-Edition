/*
** Copyright (c) 2004 Actian Corporation
*/
#ifndef GVOS_INCLUDED
#define GVOS_INCLUDED

/**
** Name: gvos.h - Operating system information compatibility library
**
** Description:
**      The file contains the types used by the GVOS functions and the
**	definition of the functions. These are used to obtain information
**	about the operating system we are running on.
**
**	11-apr-2002 (abbjo03)
**	    Created.
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
**/

/*
** Operating system version information structure
*/
typedef struct
{
    i4	ver_size;	/* size of this structure */
    i4	ver_major;	/* OS major version number */
    i4	ver_minor;	/* OS minor version number */
    i4	ver_sub;	/* OS sub-version number, e.g., Service Pack number */
} GV_OSVERSIONINFO;

/*
** Function prototypes
*/
FUNC_EXTERN STATUS GVverifyOSversion(GV_OSVERSIONINFO *version);
FUNC_EXTERN bool   GVosvers(char *OSVersionString);

#endif
