/*
** Copyright 2009-2011 Actian Corporation. All rights reserved
**
** Description:
**	Header file for the "execute java" wrapper function.
**
** History:
**    07-Dec-2009 (whiro01)
**	Initial prototype version.
**    30-Aug-2011 (whiro01)
**	New version that doesn't use "java.exe" code;
**	add splashscreen pointer and size.
**    19-Sep-2011 (whiro01)
**	Pass program path down the line in order to
**	find the .jar file where the program resides.
*/

#ifndef _EXECUTE_H_INCLUDED
#define _EXECUTE_H_INCLUDED

// For NULL
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int execute(const char *exename,
	const char *jarname,
	const char *checksum,
	const unsigned char *splash_image,
	unsigned int splash_size,
	int argc,
	char **argv);

int execute_java(const char *jarname,
	const char *checksum,
	const unsigned char *splash_image,
	unsigned int splash_size,
	int argc,
	char **argv);

#ifdef __cplusplus
}
#endif

#endif /* _EXECUTE_H_INCLUDED */
