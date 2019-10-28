@echo off
@cls
REM Name: FASTI.BAT 
REM
REM Description: 
REM	Construct an Ingres installation after successful completion of bang.bat
REM 
REM Syntax:
REM 	FASTI <II_INSTALLATION> [clean] [utf8]
REM	Where clean will remove any junk left around from a previous
REM	installation.
REM	Where utf8 will set new II_CHARSETxx to UTF8 and clean up
REM	if previous setting was not UTF8.
REM 
REM History:
REM     23-sep-98 (mcgem01)
REM	    Created.
REM	22-oct-1998 (somsa01)
REM	    For RUN_ALL, the transaction log file size should be 32k.
REM	15-jan-1999 (mcgem01)
REM	    Allow for embedded spaces in %II_SYSTEM%
REM	22-feb-1999 (somsa01)
REM	    For testing purposes, we need configurations for security and
REM	    star. Therefore, create config.dat based upon a "test.rfm"
REM	    file.
REM	01-mar-1999 (somsa01)
REM	    Pass in the installation identifier. This allows for multiple
REM	    concurrent installations.
REM	12-mar-1999 (somsa01)
REM	    Set II_INSTALLATION before running iigenres.
REM	06-apr-1999 (somsa01)
REM	    Not passing II_INSTALLATION as the first parameter is an error!
REM	26-oct-1999 (cucjo01)
REM	    Clean and Reset Licensing for builds when run with the "clean"
REM	    option. iigenres must be run in the %II_SYSTEM%\ingres\files
REM	    directory. Also, the config.dat file, must exist before running
REM	    iigenres.
REM	31-dec-1999 (somsa01)
REM	    Unset II_GCNII_LCL_VNODE and reset it so that we take into
REM	    account the proper installation identifier. Do the same with
REM	    II_CHARSETII.
REM	28-mar-2001 (mcgem01)
REM	    Now that we're part of the domain and some of us are running
REM	    under our domain accounts, add SERVER_CONTROL privileges for
REM	    the user running this script.
REM	24-Jun-2004 (drivi01)
REM	    Removed licensing.
REM	26-Aug-2004 (kodse01)
REM	    Integrated the contents of setenv.bat into fasti.bat.
REM	26-Aug-2004 (drivi01) 
REM	    Replaced any references to tools/port/bat with tools/port/bat_win
REM	    to reflect a new directory structure.
REM	31-Aug-2004 (kodse01)
REM	    Moved ingsetenv II_TIMEZONE_NAME call prior to ingstart call.
REM	14-Sep-2004 (drivi01)
REM	    Added system account user and Administrator privileges to config.dat
REM	07-Oct-2004 (somsa01)
REM	    Install the Ingres service too. Remove adding privileges for
REM	    "ingres" and "system".
REM	20-oct-2004 (somsa01)
REM	    Updated II_TEMPORARY setting to be %II_SYSTEM%\ingres\temp.
REM	27-Oct-2004 (drivi01)
REM	    bat directories on windows were replaced with shell_win,
REM	    this script was updated to reflect that.
REM	26-Oct-2005 (drivi01)
REM	    Added a touch command on symbol.tbl file since symbol.tbl is
REM	    no longer created during build.
REM	09-Mar-2006 (drivi01)
REM	    Added wait between creation of iidbdb and ckpdb.
REM	03-Jan-2007 (drivi01)
REM	    Default date_type_alias to INGRESDATE for now.
REM	10-Jul-2007 (kibro01) b118702
REM	    Moved date_type_alias from dbms to config section
REM	25-Jul-2007 (drivi01)
REM	    Move II_TEMPORARY to %TEMP% directory instead of
REM	    II_SYSTEM\ingres\temp.
REM	07-Jan-2008 (drivi01)
REM	    Add routine for registering VDBA activeX controls.
REM	27-May-2008 (smeke01) b120424
REM	    On NT makiman.sql is copied to %II_SYSTEM%\ingres\bin,
REM	    not %II_SYSTEM%\ingres\vdba.
REM	26-Aug-2008 (whiro01)
REM	    Add "gettimezone" utility to set a reasonable default
REM	    timezone given the Windows setting; also "getcp" utility
REM	    for the code page (II_CHARSET); also added "utf8" flag
REM	    to command line to force a clean if the previous charset
REM	    was NOT UTF8.
REM	26-Oct-2010 (hweho01)
REM         For Ingres Vectorwise release, set date_alias to ANSIDATE.
REM     14-Apr-2011 (drivi01)
REM         Call readvers.bat from here to setup conf_IVW variable
REM         to make sure that correct config.dat is generated from
REM         crs files and also set II_VWDATA based on conf_IVW.
REM	05-May-2011 (drivi01)
REM	    Add system user with SERVER_CONTROL privileges to config.dat.
REM	    We need it if service is installed to make sure ckpdb
REM	    has proper privileges to set vectorwise write_lock/unlock
REM	    on x100 server.
REM	    Also, hardcode 512MB for bufferpool_size and
REM	    1G for max_memory_size, the defaults on the build
REM	    machine tend to crash machine b/c of multiple
REM	    instances.
REM	26-May-2011 (drivi01)
REM	    Set ii_charset in config.dat and initialize 
REM	    default_collation to ensure it is set correctly.
REM	03-Jul-2011 (drivi01) Mantis 1404
REM	    Move default_collation setting before iidbdb
REM	    is created to have proper setting reflected in the catalogs.
REM	8-Jul-2011 (kschendel) SIR 125253
REM	    Copy over codev fasti.bat;  set date-alias according to conf_IVW.
REM	29-Jul-2011 (kschendel) SIR 125253
REM	    Fix missing label from above change.
REM     05-Oct-2011 (maspa05) b122669
REM	    Remove setting of II_GCNxx_LCL_VNODE
REM
REM

setlocal
set CLEAN_UP=false

echo.
if "%1"=="" goto USAGE
if "%1"=="clean" goto USAGE
if "%1"=="CLEAN" goto USAGE
if "%1"=="utf8" goto USAGE
if "%1"=="UTF8" goto USAGE

set II_ID=%1

REM Get current charset (either II_CHARSET or Windows setting)
for /f %%I in ('getcp') do set II_CHARSET=%%I
REM Remember the previous setting of II_CHARSETxx
set II_OLD_CHARSET=
for /f %%I in ('ingprenv II_CHARSET%II_ID%') do set II_OLD_CHARSET=%%I

:CHECK_CMDS_LOOP
if "%2" == "" goto START
if "%2" == "clean" ( set CLEAN_UP=true& shift & goto CHECK_CMDS_LOOP )
if "%2" == "CLEAN" ( set CLEAN_UP=true& shift & goto CHECK_CMDS_LOOP )
if "%2" == "utf8" ( set II_CHARSET=UTF8& shift & goto CHECK_CMDS_LOOP )
if "%2" == "UTF8" ( set II_CHARSET=UTF8& shift & goto CHECK_CMDS_LOOP )

:USAGE
echo Usage: fasti ^<II_INSTALLATION^> [clean] [utf8]
goto END

:START
call readvers.bat

REM always UTF8 for vectorwise
if "%conf_IVW%" == "true" set II_CHARSET=UTF8

if "%CLEAN_UP%"=="true" goto CLEAN_UP_FIRST
if "%II_CHARSET%" == "UTF8" if "%II_OLD_CHARSET%" neq "UTF8" goto CLEAN_UP_FIRST
if "%II_CHARSET%" neq "UTF8" if "%II_OLD_CHARSET%" == "UTF8" goto CLEAN_UP_FIRST
goto FRESH_INSTALLATION

:CLEAN_UP_FIRST

echo Cleaning up from prior installation
echo.

echo. Running ingstop to be sure the installation isn't running
echo.

REM Using -kill here since we don't care if we trash the installation since
REM we're going to remove everything anyway.

ingstop -kill

echo Removing the config.dat file
echo.
cd /D "%II_SYSTEM%\ingres\files"
if exist "%II_SYSTEM%\ingres\files\config.dat" del "%II_SYSTEM%\ingres\files\config.dat"

echo Removing the transaction log file
echo.
cd /D "%II_SYSTEM%\ingres\log"
if exist "%II_SYSTEM%\ingres\log\ingres.log*" del /q *

echo Removing II_DATABASE
echo.
cd /D "%II_SYSTEM%\ingres\data\default"
rm -rf *
cd /D "%II_SYSTEM%\ingres\data\vectorwise"
rm -rf *

echo Removing II_WORK
echo.
cd /D "%II_SYSTEM%\ingres\work\default"
rm -rf *
cd /D "%II_SYSTEM%\ingres\work\vectorwise"
rm -rf *

echo Removing II_CHECKPOINT
echo.
cd /D "%II_SYSTEM%\ingres\ckp\default"
rm -rf *
cd /D "II_SYSTEM%\ingres\ckp\vectorwise"
rm -rf *

echo Removing II_JOURNAL
echo.
cd /D "%II_SYSTEM%\ingres\jnl\default"
rm -rf *

echo Removing II_DUMP
echo.
cd /D "%II_SYSTEM%\ingres\dmp\default"
rm -rf *
cd /D "%II_SYSTEM%\ingres\dmp\vectorwise"
rm -rf *

:FRESH_INSTALLATION
echo.
echo Generating config.dat
echo.

if not exist "%II_SYSTEM%\ingres\temp" mkdir "%II_SYSTEM%\ingres\temp"
copy "%ING_SRC%\tools\port\shell_win\test.rfm" "%II_SYSTEM%\ingres\files"
touch "%II_SYSTEM%\ingres\files\symbol.tbl"
touch "%II_SYSTEM%\ingres\files\config.dat"
cd /D "%II_SYSTEM%\ingres\files"
ingsetenv ING_ABFDIR            "%II_SYSTEM%\ingres\abf"
ingsetenv II_CONFIG             "%II_SYSTEM%\ingres\files"
ingsetenv II_INSTALLATION       %II_ID%
ingsetenv II_DATABASE           "%II_SYSTEM%"
if "%conf_IVW%"=="true" ingsetenv II_VWDATA	"%II_SYSTEM%"
ingsetenv II_CHECKPOINT		"%II_SYSTEM%"
ingsetenv II_JOURNAL            "%II_SYSTEM%"
ingsetenv II_DUMP               "%II_SYSTEM%"
ingsetenv II_WORK               "%II_SYSTEM%"
ingsetenv II_TEMPORARY          "%TEMP%"
ingsetenv ING_EDIT              NOTEPAD.EXE
ingsetenv II_LANGUAGE           ENGLISH
ingsetenv II_MSGDIR             "%II_SYSTEM%\INGRES\FILES\ENGLISH"
ingsetenv II_NETBIOS_NODE       %COMPUTERNAME%
ingunset II_CHARSETII
echo Setting II_CHARSET%II_ID%=%II_CHARSET%
ingsetenv II_CHARSET%II_ID%     %II_CHARSET%
ingsetenv TERM_INGRES           IBMPC
gettimezone | ingsetenv II_TIMEZONE_NAME
echo.

iigenres %COMPUTERNAME% %ING_SRC%\tools\port\shell_win\test.rfm

echo For now, default date_type_alias to:
if "%conf_IVW%" == "" iisetres ii.%COMPUTERNAME%.config.date_alias INGRESDATE
if NOT "%conf_IVW%" == "" iisetres ii.%COMPUTERNAME%.config.date_alias ANSIDATE
iigetres ii.%COMPUTERNAME%.config.date_alias

echo Adding the Ingres service
echo.
opingsvc install

echo Adding the "%USERNAME%" user and Administrator privileges to config.dat
echo.
iisetres ii.%COMPUTERNAME%.privileges.user.%USERNAME% SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.%COMPUTERNAME%.privileges.user.Administrator SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.%COMPUTERNAME%.privileges.user.system SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

REM Setting the local_vnode

iisetres ii.%COMPUTERNAME%.gcn.local_vnode %COMPUTERNAME%

REM Ingres 2.5 no longer uses the II_LOG_FILE and II_LOG_FILE_NAME environment
REM variables.   Log configuration has been moved to config.dat

echo Unsetting the II_LOG_FILE and II_LOG_FILE_NAME environment variables 
echo and setting the appropriate config.dat parameters.

iisetres ii.%COMPUTERNAME%.rcp.log.log_file_1 "%II_SYSTEM%"
ingunset II_LOG_FILE_NAME
iisetres ii.%COMPUTERNAME%.rcp.log.log_file_name ingres.log
ingunset II_LOG_FILE
iisetres ii.%COMPUTERNAME%.rcp.file.kbytes 32768

REM Create the log file directory.

if not exist "%II_SYSTEM%\ingres\log" mkdir "%II_SYSTEM%\ingres\log"

REM Generate the primary transaction log file.

echo Generating the transaction log file.
echo.

iimklog

echo Initializing the transaction log file.
echo.

rcpconfig -init_log

echo Setting the rmcmd startup count to 0 since we don't yet have an iidbdb
echo We'll reset it to 1, once we create iidbdb and run rmcmdgen
echo Also setting the star startup count to 0; it will be reset when needed
echo.
iisetres ii.%COMPUTERNAME%.ingstart.*.star 0
iisetres ii.%COMPUTERNAME%.ingstart.*.rmcmd 0

echo Set ii_charset in config.dat and reset default_collation
echo.
for /f %%I in ('ingprsym II_CHARSET%II_ID%') do iisetres ii.*.setup.ii_charset %%I
iiinitres default_collation %II_SYSTEM%\ingres\files\dbms.rfm

echo Starting the Ingres installation
echo.

ingstart

echo Creating the system database.
echo.

createdb -S iidbdb
echo Checkpointing iidbdb
echo.

ckpdb +j +w iidbdb

echo Creating and populating the imadb
echo.
REM This script must be run in the directory where makiman.sql resides.

createdb -u$ingres imadb
cd /D "%II_SYSTEM%\ingres\bin"
sql -u$ingres imadb<makiman.sql

if NOT "%conf_IVW%"=="true" goto CONTINUE2
if "%conf_IVW%"=="true" echo Creating vectorwise checkpoint directory
if "%conf_IVW%"=="true" echo.
if "%conf_IVW%"=="true" mkdir %II_SYSTEM%\ingres\ckp\vectorwise
if "%conf_IVW%"=="true" echo Copying vectorwise configuration file
if "%conf_IVW%"=="true" echo.
if "%conf_IVW%"=="true" if not exist %II_SYSTEM%\ingres\data\vectorwise mkdir %II_SYSTEM%\ingres\data\vectorwise
for /F "usebackq" %%I IN (`vwsysinfo memory G`) do set vw_mem=%%I&set units=G
for /F "usebackq" %%I IN (`vwsysinfo cpu`) do set vw_cpu=%%I
if %vw_mem% LEQ 4 for /F "usebackq" %%I IN (`vwsysinfo memory M`) do set vw_mem=%%I&set units=M
SET /A max_mem=%vw_mem%/2
SET /A buff_pool=%vw_mem%/4
:MAX_MEM
if "%units%"=="G" if %max_mem% GEQ 1 set max_mem=1%units%& goto BUFF_POOL
if "%units%"=="G" if %max_mem% LEQ 1 set max_mem=%max_mem%%units%& goto BUFF_POOL
if "%units%"=="M" if %max_mem% LEQ 1024 set max_mem=%max_mem%%units%& goto BUFF_POOL
if "%units%"=="M" if %max_mem% GEQ 1024 set max_mem=1024%units%& goto BUFF_POOL
:BUFF_POOL
if "%units%"=="G" set buff_pool=512MB& goto CONTINUE
if "%units%"=="M" if %buff_pool% LEQ 512 set buff_pool=%buff_pool%%units%& goto CONTINUE
if "%units%"=="M" if %buff_pool% GEQ 512 set buff_pool=512%units%& goto CONTINUE

:CONTINUE
if "%conf_IVW%" == "true" type "%II_SYSTEM%\ingres\files\vectorwise.conf_TMPL"|sed s:"#max_memory_size=0 GB":max_memory_size=%max_mem%:g|sed s:"#bufferpool_size=0 GB":bufferpool_size=%buff_pool%:g|sed s:#max_parallelism_level=1:max_parallelism_level=%vw_cpu%:g|sed s:#num_cores=1:num_cores=%vw_cpu%:g  > "%II_SYSTEM%\ingres\data\vectorwise\vectorwise.conf" 
 
:CONTINUE2

echo Setting up the VDBA environment
echo.

rmcmdgen

echo Setting the rmcmd startup count to 1
echo.

iisetres ii.%COMPUTERNAME%.ingstart.*.rmcmd 1

echo Configure the installation for running run_all
echo.

ingsetenv II_MAX_SEM_LOOPS 200

call fixcfg.bat

call sql iidbdb < %ING_SRC%\tools\port\shell_win\users.sql

echo Registering ActiveX controls
regsvr32 /s %II_SYSTEM%\ingres\bin\ijactrl.ocx
regsvr32 /s %II_SYSTEM%\ingres\bin\ipm.ocx
regsvr32 /s %II_SYSTEM%\ingres\bin\sqlquery.ocx
regsvr32 /s %II_SYSTEM%\ingres\bin\vcda.ocx
regsvr32 /s %II_SYSTEM%\ingres\bin\vsda.ocx
regsvr32 /s %II_SYSTEM%\ingres\bin\svriia.dll
regsvr32 /s %II_SYSTEM%\ingres\bin\svrsqlas.dll

echo Your installation has been configured and started. It is ready for use.

:END
endlocal
@echo off
