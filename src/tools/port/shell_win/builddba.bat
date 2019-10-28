@echo off
setlocal
REM
REM  Name: builddba.bat
REM
REM  Description:
REM	Similiar buildrel, it build CD image for DBA Tools installer
REM
REM  History:
REM	09-Apr-2008 (drivi01)
REM	     Created.
REM	29-Apr-2008 (drivi01)
REM	     Remove MSredist image from DBA Tools cd image.
REM	07-May-2008 (drivi01)
REM	     Updated usage.  Removed references to II_DBA_RELEASE_DIR.
REM	     Updated the last command tag line to reflect the task
REM	     completed more accurately.
REM	15-Jul-2008 (drivi01)
REM          Update this script to digitally sign distribution and resulting
REM          CD images produced with verisign certificate for Ingres Corporation.
REM	01-Aug-2008 (drivi01)
REM	     Add -nosig file, to skip digital signatures.
REM	     This flag is useful when builddba is being run by buildrel b/c
REM	     buildrel already signed all the binaries by the time it gets 
REM	     to builddba.
REM	     Added "NOT SIGNED" option for digital signature status.
REM	02-Sep-2008 (drivi01)
REM	     Change the direction of slashes around wildcards in a directory
REM	     path, to fix issues with cygwin cp.exe.
REM	09-Dec-2008 (drivi01)
REM	     Run msiupd "Ingres DBA Tools.msi" before digitally signing
REM	     the binary.
REM	     If signing the image exe and msi fails, print an error.
REM	29-Aug-2009 (drivi01)
REM	     Update version in IngresII_DBA_DBL.ism module automatically
REM	     via msiversupdate utility.
REM	03-Nov-2009 (drivi01)
REM	     Add RTF and PDF versions of the license, copy all 3 images
REM	     to the DBA Tools image.  Note, RTF version is very important
REM	     for the license agreement dialog.
REM	18-Nov-2009 (drivi01)
REM	     Remove an old reference to the license location.
REM	     If license files are missing, exit the script.
REM	06-Jan-2009 (drivi01)
REM	     Because of the way redistributables are stored in picklist.dat
REM	     builddba doesn't know how to handle redistributable locations.
REM	     This change will modify the full path appended depending on
REM	     the directory and filename picked from picklist.dat.
REM	     This will ensure that MS redistributables are signed.
REM	06-Jan-2010 (drivi01)
REM	     chktrust utility no longer available with Visual Studio 2008.
REM	     SignTool is the recommended utility to use with the new
REM	     compiler.  This change replaces chktrust instances with 
REM	     SignTool.
REM	11-Jan-2010 (drivi01)
REM	     Replace /kp flag with /pa flag to ensure compatibility with
REM	     different version of signtool.  /pa flag ensures the signature
REM	     is verified using the "Default Authenticode" verification
REM	     policy.
REM	13-May-2010 (drivi01)
REM	     Add -VW flag which will enable builddba script to roll a
REM	     DBA Tools image for Ingres VectorWise installations.
REM	     The VW DBA Tools image will differ from Ingres image
REM	     with graphics.
REM	14-May-2011 (drivi01)
REM	     Introduce return codes for the buildrel script.
REM	     Return errorlevel 0 only if the script ran
REM	     100% successfully.
REM	     The following are the error codes:
REM		IsCmdBld error = 1
REM 		Could not rename final product MSI = 2
REM 		Copying support files (pdbs, api) failure = 3
REM 		Updating merge modules version failed = 4
REM 		Digital signatures failure = 5
REM 		Documentation image failed = 6
REM 		.NET Data Provider failed = 7
REM 		DBA Tools package failed = 8
REM 		Strong name failure = 9
REM	12-Jul-2011 (drivi01) b125546
REM	     For VectorWise installations, copy setup.ini
REM	     from enterprise_preinst_win to DBA cdimage.
REM	20-Oct-2011 (drivi01)
REM	     After the builds were switched over to Standalone build version of
REM	     InstallShield, default merge module path is insufficient for our builds
REM	     and there's no GUI to configure it.  This change adds MM_SEARCH_PATH
REM	     for IngresII_DBA_DBL.ism modules
REM	     as a parameter to ensure that all redistributables are found at
REM	     build time.   MM_SEARCH_PATH will be a new environment variable
REM	     that will need to be set, but only if Standalone build version of
REM	     InstallShield is used.  buildrel will detect the GUI version of 
REM	     InstallShield and will not prompt to set MM_SEARCH_PATH.
REM	24-Oct-2011 (frima01) b125884
REM	     DBA Tools are to be shipped with free License.
REM	31-Oct-2011 (drivi01)
REM	     Instead of adding physical files to include Management Studio
REM	     service/server piece, pull in IngresIIDBMSNetMgmt ISM into
REM	     a DBA Tools image.
REM	

set SUFFIX=
set MSI_RELEASE_FLAGS=
set SIGNED=NOT SIGNED
set ERROR_STATUS=0

if not x%II_RELEASE_DIR%==x goto CHECK0
echo DBA Tools release will not be built.
goto DONE

:CHECK0
if not x%II_DBA_CDIMAGE_DIR%==x goto CHECK1
echo DBA Tools cd image will not be built.
goto DONE

:CHECK1
if not x%II_SYSTEM%==x goto CHECK3
echo You must set II_SYSTEM !!
goto DONE


:CHECK3
if not x%ING_SRC%==x goto CHECK35
echo You must set ING_SRC !!
goto DONE

:CHECK35
if not x%ING_BUILD%==x goto CHECK36
echo You must set ING_BUILD !!
goto DONE

:CHECK36
if "%1" == "-p" goto LISTALL
goto CHECK6

:CHECK6
if exist %ING_SRC%\LICENSE.free goto CHECK6_1
echo You must have at least free license in %ING_SRC% !!
goto DONE

:CHECK6_1
if exist %ING_SRC%\LICENSE.free.rtf goto CHECK6_2
echo You must have RTF version of free license in %ING_SRC% !!
goto DONE

:CHECK6_2
if exist %ING_SRC%\LICENSE.free.pdf goto CHECK7
echo You must have PDF version of free license in %ING_SRC% !!
goto DONE

:CHECK6_3
REM Check if we're using StandaloneBuild, MM_SEARCH_PATH must be specified 
REM for StandaloneBuild of InstallShield

which IsCmdBld|grep StandaloneBuild
if errorlevel 1 goto CHECK7
set MSI_RELEASE_FLAGS=-o "%MM_SEARCH_PATH%"
if not x"%MM_SEARCH_PATH%"==x"" goto CHECK7
echo You must set MM_SEARCH_PATH to the merge module search path to look
echo for redistributables. The merge modules path located under InstallShield
echo Premier directories must be at the front of the path.  If the default
echo location path for the InstallShield was used, the suggested MM_SEARCH_PATH
echo is:
echo MM_SEARCH_PATH=C:\InstallShield 2010 Projects\MergeModules,C:\Program Files (x86)\InstallShield\2010\Modules\i386,C:\Program Files (x86)\InstallShield\2010\Objects,C:\Program Files (x86)\Common Files\Merge Modules
goto DONE

:CHECK7
if "%2" == "" goto CHECK8
goto CONT0_9

:CHECK8
if "%1" == "-?" goto CONT0_9
if "%1" == "-help"  goto CONT0_9
if "%1" == "-VW" set VW_IMAGE=TRUE
goto CONT1



:CONT0_9
REM ****************************************************
REM USAGE:
REM -a Verifies all files required for CD image are present in the build area
REM -p Lists the distribution 
REM -help Prints Usage
REM -nosig Skips digital signing portion of the script
REM ****************************************************
echo "USAGE: builddba [-a| -p|-help|-nosig]"
goto DONE

:CONT1
@echo.
"%MKSLOC%\echo" "Version: \c" 
cat %II_SYSTEM%/ingres/version.rel
echo Release directory: %II_RELEASE_DIR%
echo CD image directory: %II_DBA_CDIMAGE_DIR%
@echo.

:AUDIT
@echo.
echo Auditing DBA Tools setup package...
@echo.
rm -f %ING_SRC%\buildrel_report_dba.txt
call gd enterprise_is_win
@echo off
rm -f buildrel_report.txt
REM msidepcheck MSRedists.ism 
msidepcheck IngresII_DBA_DBL.ism 
goto AUDIT_CONT_2

:AUDIT_CONT_2
if exist buildrel_report.txt mv buildrel_report.txt %ING_SRC%\buildrel_report_dba.txt
if exist %ING_SRC%\buildrel_report_dba.txt echo There are some files missing or zero length:& echo.
if exist %ING_SRC%\buildrel_report_dba.txt type %ING_SRC%\buildrel_report_dba.txt& echo.
if exist %ING_SRC%\buildrel_report_dba.txt echo A log of these missing files has been saved in %ING_SRC%\buildrel_report_dba.txt.& goto DONE
if "%1" == "-a" goto CONT5
if "%1" == "-nosig" goto CONT_IMAGE


echo Digitally Signing the distribution files...
echo.
if exist %II_SYSTEM%\ingres\picklist.dat del %II_SYSTEM%\ingres\picklist.dat
goto LISTALL

:DIGSIG
@echo off
echo.
echo Generating List of Files that Need to be Digitally Signed
echo.
if not exist %II_SYSTEM%\ingres\picklist.dat goto ERROR4
set TEMP1=%TEMP%\picklist.$$1
if exist "%TEMP1%" del "%TEMP1%"
for /F %%I IN (%II_SYSTEM%\ingres\picklist.dat) do (
	if %%~xI==.cab echo %%I|grep \\ >> "%TEMP1%"
	if %%~xI==.exe echo %%I|grep \\ >> "%TEMP1%"
	if %%~xI==.dll echo %%I|grep \\ >> "%TEMP1%"
)

set TEMP2=%TEMP%\picklist.$$2
if exist "%TEMP2%" del "%TEMP2%"
setlocal ENABLEDELAYEDEXPANSION
set ing_files=FALSE
for /F "delims=\ tokens=1,2*" %%I IN (%TEMP1%) do (
for /F %%X in ('dir %II_SYSTEM%\ingres /AD /B') do (
if %%J==%%X echo %II_SYSTEM%\ingres\%%J\%%K >> "%TEMP2%"
if %%J==%%X set ing_files=TRUE
)
if "!ing_files!"=="FALSE" echo %II_SYSTEM%\%%J >> "%TEMP2%"
set ing_files=FALSE
)
if exist "%TEMP1%" del "%TEMP1%"


echo.
echo Checking if the files are signed already
echo.
for /F %%I in (%TEMP2%) do (
echo signtool verify /pa /q "%%~I"
signtool verify /pa /q "%%~I"
if ERRORLEVEL 1 call iisign -l "%%~I"
if not ERRORLEVEL 0 goto DIGSIG_ERROR
)
if exist "%TEMP2%" del %TEMP2%
if not ERRORLEVEL 1 goto DIGSIG_DONE 


:DIGSIG_DONE
echo.
echo All Files are digitally signed!
echo.
set SIGNED=TRUE
goto CONT_IMAGE

:CONT_IMAGE
@echo.
echo Building DBA Tools image...
call gd enterprise_is_win
@echo off
@echo.
echo Copy the installer images in place
@echo.
@echo on
if "%VW_IMAGE%"=="TRUE" copy /y resource\Block01_VW.bmp resource\Block01_img.bmp & copy /y resource\install_top_VW.bmp resource\install_top_img.bmp
if not "%VW_IMAGE%"=="TRUE" copy /y resource\Block01.bmp resource\Block01_img.bmp & copy /y resource\install_top.bmp resource\install_top_img.bmp
@echo off
call chmod 777 IngresII_DBA_DBL.ism IngresIIDBMSNetMgmt.ism
REM call chmod 777 MSRedists.ism 
@echo.
ISCmdBld.exe -x -p IngresIIDBMSNetMgmt.ism -b %II_RELEASE_DIR%\IngresIIDBMSNetMgmt_DBA
if errorlevel 1 goto ERROR
@Echo.
ISCmdBld.exe -x -p IngresII_DBA_DBL.ism -b %II_RELEASE_DIR%\IngresII_DBA %MSI_RELEASE_FLAGS%
if errorlevel 1 goto ERROR
@echo.
REM ISCmdBld.exe -x -p MSRedists.ism -b %II_RELEASE_DIR%\MSRedists
REM if errorlevel 1 goto ERROR
@echo.
goto CONT

:CONT
call chmod 444 IngresII_DBA_DBL.ism IngresIIDBMSNetMgmt.ism
@echo off
echo Creating cdimage...
if not exist %II_DBA_CDIMAGE_DIR% mkdir %II_DBA_CDIMAGE_DIR%
rm -rf %II_DBA_CDIMAGE_DIR%\*
mkdir %II_DBA_CDIMAGE_DIR%\files
REM mkdir %II_DBA_CDIMAGE_DIR%\files\msredist
REM cp -p -v %II_RELEASE_DIR%\MSRedists\build\release\DiskImages\Disk1\/*.* %II_DBA_CDIMAGE_DIR%\files\msredist
if "%EVALBUILD%" == "ON" goto SDK2
cp -p -v %II_RELEASE_DIR%\IngresII_DBA\dba_build\release2\DiskImages\Disk1\/*.* %II_DBA_CDIMAGE_DIR%\files
rm -rf %II_DBA_CDIMAGE_DIR%\files\setup.*
cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\Release\setup.exe %II_DBA_CDIMAGE_DIR%
cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\AUTORUN.INF %II_DBA_CDIMAGE_DIR%
if "%VW_IMAGE%"=="TRUE" copy /y %ING_SRC%\front\st\enterprise_preinst_win\setup.ini %II_DBA_CDIMAGE_DIR%
REM cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\sample.rsp %II_DBA_CDIMAGE_DIR%
REM cp -p -v %II_SYSTEM%\readme.html %II_DBA_CDIMAGE_DIR%
REM cp -p -v %II_SYSTEM%\readme_int_w32.html %II_DBA_CDIMAGE_DIR%
cp -p -v %ING_SRC%\LICENSE.free %II_DBA_CDIMAGE_DIR%\LICENSE
cp -p -v %ING_SRC%\LICENSE.free.rtf %II_DBA_CDIMAGE_DIR%\LICENSE.rtf
cp -p -v %ING_SRC%\LICENSE.free.pdf %II_DBA_CDIMAGE_DIR%\LICENSE.pdf
echo Update version in Ingres DBA Tools.msi
msiversupdate "%II_DBA_CDIMAGE_DIR%\files\Ingres DBA Tools.msi"
if ERRORLEVEL 1 goto ERROR3
echo Updating MSI database...
msiupd "%II_DBA_CDIMAGE_DIR%\files\Ingres DBA Tools.msi"
call iisign "%II_DBA_CDIMAGE_DIR%\files\Ingres DBA Tools.msi"
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
call iisign %II_DBA_CDIMAGE_DIR%\setup.exe
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
goto CONT4

:CONT3
goto CONT4

:LISTALL
if exist %ING_BUILD%\picklist.dat del %ING_BUILD%\picklist.dat
call gd enterprise_is_win 
REM msidepcheck -p MSRedists.ism 
msidepcheck -p IngresII_DBA_DBL.ism 
@echo off
@echo.
if not "%1"=="-p" goto DIGSIG
goto CONT5

:CONT4
echo DBA Tools image built successfully!!!
if "%SIGNED%"=="TRUE" echo YOUR IMAGE IS DIGITALLY SIGNED!!!
if "%SIGNED%"=="FALSE" echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED!!!
if "%SIGNED%"=="NOT SIGNED" echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED, BECAUSE -nosig FLAG WAS USED!!!
goto DONE

:CONT5
echo DBA Tools task completed!!!
goto DONE

:ERROR
echo A build error occurs with ISCmdBld.exe!!!
set ERROR_STATUS=1
goto DONE

:ERROR2
echo An error occured copying files for one of the build directories.
set ERROR_STATUS=3
goto DONE

:ERROR3
echo.
echo Fatal Error: An error occured updating version in one of the mergemodules or MSI projects.
set ERROR_STATUS=4
goto DONE

:ERROR4
echo An error occured printing distribution to digitally sign the files.
set SIGNED=FALSE
set ERROR_STATUS=5
goto CONT_IMAGE

:DIGSIG_ERROR
echo.
echo An error occured signing the distribution binaries!
echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED!!!
echo.
set SIGNED=FALSE
set ERROR_STATUS=5
goto CONT_IMAGE

:DIGSIG_ERROR2
echo.
echo An error occured signing the DBA Tools image!
echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED!!!
echo.
set SIGNED=FALSE
set ERROR_STATUS=5
goto CONT4

:DONE
exit /B %ERROR_STATUS%
endlocal


