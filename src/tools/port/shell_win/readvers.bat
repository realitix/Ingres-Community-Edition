REM @echo off
REM 
REM  readvers.bat
REM 
REM  History:
REM 	13-jul-95 (tutto01)
REM	    Created.
REM	13-dec-1999 (somsa01)
REM	    Set all variables that are in the VERS file.
REM 	14-Jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.
REM	24-Aug-2009 (bonro01)
REM	    Make the VERS override file optional.
REM	10-Sep-2009 (bonro01)
REM	    Fix previous change to make VERSFILE a global variable.
REM	03-Mar-2011 (bonro01)
REM	    Read new VERS file format for #set: options


REM Locate the VERS file
REM
set VERSDIR=%ING_SRC%\tools\port\conf
if exist %VERSDIR%\VERS (
    set VERSFILE=%VERSDIR%\VERS
) else if exist %VERSDIR%\VERS.%config_string% (
    set VERSFILE=%VERSDIR%\VERS.%config_string%
) else (
    echo.
    echo There is no VERS or VERS.%config_string% file!!!!!!
    echo.
    goto EXIT
)


REM Transform the VERS file into an executable batch file.
REM Only deal with lines that contain "#set:", as other
REM lines are for jam (or are comments).
REM Lines with ##set: are comments
REM Do this for now so that VERS files looks just like other systems.
REM
set AWK_SCRIPT=' ^
	/##set: / {next} ^
	/^[^#]*#set: / { ^
		setline = substr($0,index($0,"#set: ")+6); ^
		split(setline, toks); ^
		if (toks[1] == "config") { ^
			print "set config=" toks[2] ^
		} else if (toks[1] == "config32") { ^
			print "set config32=" toks[2] ; ^
		} else if (toks[1] == "config64") { ^
			print "set config64=" toks[2] ; ^
		} else if (toks[1] == "arch") { ^
			print "set build_arch=" toks[2] ; ^
		} else if (toks[1] == "build") { ^
			print "set build=" toks[2] ; ^
		} else if (toks[1] == "inc") { ^
			print "set inc=" toks[2] ; ^
		} else if (toks[1] == "option") { ^
			opts=opts " " toks[2]; ^
			print "set conf_" toks[2] "=true"; ^
			dopts=dopts " /Dconf_" toks[2]; ^
		} else if (toks[1] == "param") { ^
			conf_params=conf_params " " toks[2]; ^
			print "conf_" toks[2] "=" toks[3]; ^
		} ^
	} ^
	END { ^
		print "set optdef=" dopts ; ^
		print "set conf_params=" conf_params ; ^
		print "set opts=" opts ^
	}' 

%AWK_CMD% %AWK_SCRIPT% %VERSFILE% > %VERSDIR%\VERS.bat

call %VERSDIR%\VERS.bat
DEL %VERSDIR%\VERS.bat

REM unset local variables
set AWK_SCRIPT=
set VERSDIR=
set VERSFILE=

:EXIT
