/*
** Copyright 2009-2011 Actian Corporation. All rights reserved
**
** Description:
**	Wrapper function to invoke "java.exe" with various
**	command line options.
**
** History:
**    07-Dec-2009 (whiro01)
**	Initial prototype version.
**    05-Jan-2010 (whiro01)
**	Adding search in the standard Ingres places for the given
**	.jar file.
**    12-Jan-2010 (whiro01)
**	Changed to invoke "execute_java" function (new entry in java.c)
**	instead of exec'ing "java.exe"
**    22-Apr-2011 (whiro01)
**	Making things simpler by just adding the "-jar" jarname into
**	the beginning of the "argv" list and letting java do whatever it
**	does to set things up.  This is necessary in order to correctly
**	pass command line options.
**    31-Aug-2011 (whiro01)
**	Reworked to not use the Ingres functions to find the .jar file
**	and to pass the splashscreen value along to "execute_java".
**    19-Sep-2011 (whiro01)
**	Pass along the program's path in order to find the .jar file
**	along with the executable (if possible).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "execute.h"
#include "util.h"

using namespace std;

static void
find_jar_file(string& exename, string& jarname)
{
	string exepath = exename.substr(0, exename.find_last_of(FILE_SEPARATOR) + 1);
	string path = exepath + jarname;
	if (!file_exists(path.c_str())) {
	    const char *ii_system = getenv("II_SYSTEM");
	    if (ii_system == NULL || *ii_system == '\0')
		return;
	    string ingpath = string(ii_system) + FILE_SEPARATOR + "ingres" + FILE_SEPARATOR;
	    path = ingpath + "bin" + FILE_SEPARATOR + jarname;
	    if (!file_exists(path.c_str())) {
		path = ingpath + "lib" + FILE_SEPARATOR + jarname;
		if (!file_exists(path.c_str())) {
		    path = ingpath + "utility" + FILE_SEPARATOR + jarname;
		    if (!file_exists(path.c_str()))
			return;
		}
	    }
	}
	jarname = path;
}


int
execute(const char *exename, const char *jarname, const char *checksum, const unsigned char *splash_image, unsigned int splash_size, int argc, char **argv)
{
	string execname(exename);
	string jar(jarname);
	size_t ix = jar.find_last_of('.');
	if (ix == string::npos)
	    jar += ".jar";
	find_jar_file(execname, jar);
	return execute_java(jar.c_str(), checksum, splash_image, splash_size, argc, argv);
}
