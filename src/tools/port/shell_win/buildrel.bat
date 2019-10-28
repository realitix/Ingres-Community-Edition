@echo off
setlocal
REM
REM  Name: buildrel.bat
REM
REM  Description:
REM	Similiar to UNIX's buildrel, it basically builds a cdimage from
REM	scratch.
REM
REM  History:
REM	16-jun-2001 (somsa01)
REM	    Created.
REM	05-sep-2001 (penga03)
REM	    Copy ingwrap.exe, and run msiupd.exe. 
REM	02-oct-2001 (somsa01)
REM	    Updated for InstallShield Developer 7.0 .
REM	28-oct-2001 (somsa01)
REM	    The .ism files need to be writeable for the command-line
REM	    build process to succeed.
REM	08-nov-2001 (somsa01)
REM	    Made changes to follow the new CA branding.
REM	08-nov-2001 (somsa01)
REM	    Removed building IngresIIDBMSNetEsqlCobol.ism.
REM	09-nov-2001 (somsa01)
REM	    Added 64-bit Windows builds.
REM	08-dec-2001 (somsa01)
REM	    Corrected name of Readme.
REM	15-feb-2002 (penga03)
REM	    Made changes to build Ingres SDK.
REM	21-mar-2002 (penga03)
REM	    Copy Apache HTTP Server and DBupdate for Ingres SDK.
REM	27-mar-2002 (somsa01)
REM	    Adjusted the copying of the final MSI to handle the special
REM	    trademark symbol in the name.
REM	28-may-2002 (penga03)
REM	    Made changes to build DoubleByte Ingres or SingleByte Ingres 
REM	    depending on whether or not DOUBLEBYTE is set to ON.
REM	12-feb-2003 (penga03)
REM	    Unzip ca licensing files from command line. And rename 
REM	    silent.bat to silent.exe.
REM	24-feb-2003 (penga03)
REM	    Corrected the syntax error while renaming silent.bat to 
REM	    silent.exe.
REM	11-mar-2003 (somsa01)
REM	    For now, use the 32-bit version of ODBC in the 64-bit build.
REM	    ALso, updated licensing with version 1.53, and copy multiple
REM	    READMEs into the CD image area.
REM	20-may-2003 (penga03)
REM	    Added Ingres Net to SDK. Also, took away the comments on 
REM	    OpenROAD.
REM	02-oct-2003 (somsa01)
REM	    Ported to NT_AMD64.
REM	26-jan-2004 (penga03)
REM	    Added a new feature Ingres .NET Data Provider.
REM	06-feb-2004 (somsa01)
REM	    Updated to use the .NET 2003 compiler.
REM	19-feb-2004 (penga03)
REM	    Added building MSRedists.ism.
REM	20-feb-2004 (somsa01)
REM	    Make sure the .ism files are writeable for ISCmdBld.
REM	02-mar-2004 (penga03)
REM	    Copy Ingres merge modules and msiupd.exe to 
REM	    %ING_SRC%\mergemodules.
REM	22-apr-2004 (penga03)
REM	    Unzip Microsoft .NET Framework package to
REM	    %ING_SRC%\cdimage\dotnetfx.
REM	03-jun-2004 (penga03)
REM	    Added a command-line option "-a" so that IsCmdBld ONLY builds
REM	    the MSI tables.
REM	09-jun-2004 (penga03)
REM	    Added error checking for IsCmdBld.
REM	24-Jun-2004 (drivi01)
REM	    Removed licensing.
REM	06-jul-2004 (penga03)
REM	    Use msidepcheck.exe, instead of IsCmdBld.exe to check out all
REM	    missing files and save them in %ing_src%\buildrel_report.txt.
REM	26-jul-2004 (penga03)
REM	    Removed all references to "Advantage".
REM	10-aug-2004 (somsa01)
REM	    We now use the 64-bit version of ODBC in the 64-bit build.
REM	02-sep-2004 (somsa01)
REM	    Updated for Open Source.
REM	14-sep-2004 (Penga03)
REM	    Replaced "Ingres Enterprise Edition" with "Ingres".
REM	14-sep-2004 (Penga03)
REM	    Corrected some errors introduced by the last change.
REM	07-oct-2004 (somsa01)
REM	    Updated with some new variables, and new locations for Open
REM	    Source.
REM	11-oct-2004 (Penga03)
REM	    Clean up buildrel_report.txt before auditing.
REM	15-Oct-2004 (drivi01)
REM	    Removed script for building merge module dlls and
REM	    pre-installer b/c it's being build by jam with the
REM	    rest of ingres code.
REM	12-Jan-2005 (drivi01)
REM	    Added calls to mkpreinstallAPI.bat and mkthinclient.bat
REM	    to copy directories that will be available to customers.
REM	21-Jan-2005 (drivi01)
REM	    Moved mkpreinstallAPI.bat and mkthinclient.bat calls to
REM	    buildrel execute instead of buildrel -a.
REM	28-Jan-2005 (drivi01)
REM	    Modified buildrel -a to print out a version number and
REM	    a build number from version.rel file.
REM	21-Mar-2005 (drivi01)
REM	    Modified buildrel to run mkembedextras.bat to create
REM	    embedded extras directory during roll of cdimage.
REM	30-Mar-2005 (drivi01)
REM	    Fixed spelling errors in call to mkembedextras.
REM	15-Apr-2005 (drivi01)
REM	    Modified buildrel to run mkpdbdir.bat to create
REM	    pdbs directory during creation of cdimage.
REM	25-May-2005 (drivi01)
REM	    Added copying of LICENSE file from ING_SRC to
REM	    cdimage directory.
REM	08-Aug-2005 (drivi01)
REM	    Added errorlevel check for bat files
REM	    executions.
REM	21-Sep-2005 (drivi01)
REM	    Updated this script to flag zero-length files and
REM	    to run audit on buildrel as well as buildrel -a.
REM	19-Jan-2006 (drivi01)
REM	    SIR 115615
REM	    Added new merge module project to the build proccess.
REM	23-Jan-2006 (drivi01)
REM	    Expanded this script to build limited cdimages. 
REM	    Limitted cdimage is an image without documentation
REM	    or i18n files.  During these changes -lmtd flag
REM	    was introduced which sets internal variable 
REM	    II_RELEASE_LMTD to on and results in IngresIIi18n
REM	    and IngresIIDoc modules being left out of build 
REM	    proccess, as well as forces use of IngresII_DBL_LMTD
REM	    project instead of IngresII_DBL.ism.
REM	1-Feb-2006 (drivi01)
REM	    SIR 155688
REM	    Made the changes to copy two licenses on the cdimage.
REM	    One license should a flat text file copied from %ING_SRC%
REM	    Second license file should be identical except in rtf
REM	    format and be copied from 
REM	    %ING_SRC%\front\st\enterprise_is_win\resource to 
REM	    License.rtf.  
REM	    buildrel is modified to copy gpl license by default, if
REM	    image requires another license then it should be copied
REM	    by hand to replace LICENSE and Licenter.rtf files on the 
REM	    image.
REM	06-Mar-2006 (drivi01)
REM	    Updated script to understand flag -p which will
REM	    print all files being shipped to %ING_BUILD%\picklist.dat
REM	09-Mar-2006 (drivi01)
REM	    buildrel should check for new version of adobe.
REM	21-Mar-2006 (drivi01)
REM	    Replaced calls to awk with calls to %AWK_CMD% to ensure
REM	    this script works with cygwin.
REM	22-Mar-2006 (drivi01)
REM	    Added creation of build.timestamp on first successful run of
REM	    of buildrel and only if it isn't already in ING_BUILD.
REM	13-Jun-2006 (drivi01)
REM	    Added another flag -s which is used in combination with flag
REM	    -p and prints out sptial object binary to picklist in 
REM	    II_SYSTEM\ingres
REM	20-Oct-2006 (drivi01)
REM	    Added a check for .NET Framework 2.0 in 
REM	    ING_ROOT\Framework2\dotnetfx.exe
REM	27-Oct-2006 (drivi01)
REM	    Pull out IngresIIi18n.ism module out and put new 
REM	    module IngresIIDBMSDotNETDataProvider.ism containing demo.
REM	16-Nov-2006 (drivi01)
REM	    SIR 116599
REM	    Modified the structure of the cdimage as well as added
REM	    two more instalelrs to the roll of cdimage. setup.exe is 
REM	    still in the root directory, however data cab, and msi 
REM	    as well as redistributables were moved into files directory.
REM	    documentation and dotnet directories contain the images 
REM	    for the other two installers, these two directories are
REM	    optional.  Added flags to buildrel:
REM	    -doc will omit documentation image from ingres cdimage
REM	    -dotnet will omit .NET Data Provider image from ingres cdimage
REM	    -doconly will build documentation only image
REM	    -dotnetonly will build .NET Data Provider only image
REM	    -? or -help will display usage
REM	    .NET Framework 2.0 was added to the redistributable list.
REM	20-Nov-2006 (drivi01)
REM	    .NET Data Provider module needs _isconfig.xml file that
REM	    is built into project directory.  Temporarily define
REM	    project directory and then remove it.
REM	20-Nov-2006 (drivi01)
REM	    Add building IngresIIi18n.ism back, even though it doesn't contain
REM	    any files we can't remove features without performing a major
REM	    upgrade.
REM	06-Dec-2006 (drivi01)
REM	    For option -lmtd change "echo DONE" statement to "goto DONE".
REM	    The option is disabled and should exit buildrel script.
REM	03-Jan-2007 (drivi01)
REM	    Take out check for adobe, we do not redistribute adobe anymore. Installer
REM	    will redirect user to adobe website if adobe is not installed.
REM	07-Jul-2007 (drivi01)
REM	    Remove any references to Framework packages. Framework packages
REM	    will no longer be included in the image.
REM	15-Jan-2008 (drivi01)
REM	    Created routines for updating version in .msm and .msi files
REM	    automatically after cd image is done building.
REM	10-Apr-2008 (drivi01)
REM	    Integrate rolling of spatial objects into distribution.
REM	    If spatial objects aren't present roll the image without spatial objects.
REM	10-Apr-2008 (drivi01)
REM	    Added routines for building DBA Tools cdimage.
REM	30-May-2008 (drivi01)
REM	    Fixed -dotnetonly flag to not execute builddba.bat.
REM	    Added a statement that goes to DONE right after .NET Data 
REM	    Provider is rolled when -dotnetonly flag is passed to 
REM	    buildrel.
REM	16-Jun-2008 (drivi01)
REM	    In efforts to rebrand, dropping "Ingres 2006" from documentation msi
REM	    will use "Ingres Documentation 9.2" isntead.
REM	15-Jul-2008 (drivi01)
REM	    Update this script to digitally sign distribution and resulting
REM	    CD images produced with verisign certificate for Ingres Corporation.
REM	28-Jul-2008 (drivi01)
REM	    Add .dist extension (iilibudt.dist) to the list of files signed.  
REM	    Otherwise tests are failing b/c iilibudt.dist and iilibudt.dll are 
REM	    different size b/c one is signed and the other is not signed.
REM	30-Jul-2008 (drivi01)
REM	    Added .NET Data Provider strong name key signing.
REM	    Modified behavior of the buildrel in the following ways:
REM		1. .NET Data Provider rolling is isolated to .NET 
REM		   portion of buildrel
REM		2. Documentation rolling is isolated to documentation 
REM		   portion of buildrel
REM		3. msidepcheck checks for all files required unless 
REM		   -doc or -dotnet flag is specified.
REM		4. if strong name key signing fails, the script will
REM		   still complete successfully with a warning, 
REM		   unless -dotnetonly flag is specified in which case 
REM	    	   it will exit with error
REM		5. if digital signing fails, the script will still 
REM		   complete successfully but with a warning
REM		6. if script completes successfully, you will get a
REM		   SUMMARY block at the end, although it may contain
REM		   warnings b/c digital or strong name key signing 
REM		   has failed.
REM		7. The rest of the errors in the buildrel are still 
REM		   considered fatal and will abort script execution
REM		   at the first error, summary will not be provided
REM		   for fatal outcomes.
REM		8. For successful completion of the script, the 
REM		   summary will provide the list of the CD images
REM		   that completed successfully and information
REM		   about digital and strong name signing.
REM		Updated references to .NET Data Provider 2.0 to 
REM		reference version 2.1.
REM     06-Aug-2008 (drivi01)
REM	     On different machines, VS2005 environment variable 
REM	     may be quoted or not which would cause inconsistencies 
REM	     throughout the script.  Check the quotes around the 
REM	     variable before doing anything further in the script.
REM	21-Aug-2008 (drivi01)
REM	     Add "NoFlag" flag when building IngresII_DBL.ism 
REM	     to ensure Spatial Objects exclusion.
REM	27-Aug-2008 (chaki01)
REM	     Fixed some copy commands not working properly when try to copy
REM	     mutiple files with * and using \ in directories path.
REM	15-Nov-2008 (drivi01)
REM	     Automate retrieval of version number for the image name for
REM	     documentation msi name.
REM	03-Dec-2008 (drivi01)
REM	     Move digital signatures before mkthinclient and mkpreinstallAPI
REM	     mkpreinstallAPI scripts run so that the scripts copy 
REM	     already signed binaries.
REM	09-Dec-2009 (drivi01)
REM	     Add IngresIIDBMSDotNETDataProvider.ism to the list of
REM	     files that generate distribution list, to make sure 
REM	     IngresDemoApp.exe is signed.
REM	     Rename Ingres.msi and run msiupd on IngresII.msi before
REM	     it is signed.
REM	08-May-2009 (drivi01)
REM	     Don't check errorlevel after mkpdbdir.bat in case
REM	     if we have trouble building pdb files in Release mode
REM	     with Visual Studio 2008.
REM	29-Aug-2009 (drivi01)
REM	     Update version on documentation package automatically.
REM	22-Oct-2009 (drivi01)
REM	     Copy license to documentation and .NET Data Provider images
REM	     as we're adding license dialogs back.
REM	     Also, added rtf and pdf versions of license files to each 
REM	     image.
REM	18-Nov-2009 (drivi01)
REM	     Remove all references to the old License file locations.
REM	     If license files are missing, exit the script.
REM	03-Dec-2009 (drivi01)
REM	     Remove -nosig flag from builddba call.  The Microsoft 
REM	     redistributables no longer shipped with Ingres package.
REM	     -nosig flag doesn't sign any binaries for builddba
REM	     which in most cases is okay, except that Ingres DBA Tools
REM	     package ships the 4 microsoft redistributables which never
REM	     get signed as a result of that flag.
REM	04-Dec-2009 (drivi01)
REM	     Remove a duplicate of msiversupdate for documentation image.
REM	06-Jan-2010 (drivi01)
REM	     chktrust utility no longer available with Visual Studio 2008.
REM	     SignTool is the recommended utility to use with the new
REM	     compiler.  This change replaces chktrust instances with 
REM	     SignTool.
REM	06-Jan-2010 (drivi01)
REM	     Remove references to Visual Studio 2005 and VS2005 variable.
REM	     sn.exe is now available in Windows SDK 6.0A which is added
REM	     to the path by Visual Studio 2008 compiler.	
REM	11-Jan-2010 (drivi01)
REM	     Replace /kp flag with /pa flag to ensure compatibility with
REM	     different version of signtool.  /pa flag ensures the signature
REM	     is verified using the "Default Authenticode" verification
REM	     policy.
REM	06-Mar-2010 (drivi01)
REM	     Update the script to roll pure 64-bit image.
REM	07-Apr-2010 (drivi01)
REM	     Temporarily remove VDBA dependecies, as VDBA is not being
REM	     distributed for the moment.
REM	26-Apr-2010 (bonro01)
REM	     Fix typo caused by previous change.
REM	13-Aug-2010 (drivi01)
REM	     B/c 64-bit Ingres on Windows is a new product,
REM	     exclude ICE feature entirely.
REM	13-Aug-2010 (drivi01)
REM	     IngresIIDotNetDataProvider merge module can not be removed
REM	     from distribution as it ensures that upgrades from previous
REM	     versions of Ingres are successful.  Removing this module
REM	     will result in upgrade issues as it will remove components
REM	     from the resulting msi.
REM	04-Jan-2010 (drivi01)
REM	     Add Visual DBA ism for x64 that includes
REM	     all Visual tools.
REM	10-Dec-2010 (drivi01)
REM	     Update this script to roll Ingres vectorWise image as
REM	     well as Ingres image.
REM	     This script takes a mendatory parameter %PRODUCT% now.
REM	     The PRODUCT should be "IVW" or "II" and will run a custom
REM	     set of ism files for each product.
REM	     The image will be placed in %II_CDIMAGE_DIR%_%PRODUCT%
REM	     directory.
REM	     The license will be different.
REM	     IVW installer uses IngresVW_DBL64.ism as the main project
REM	     and a custom IngresVWDBMS64.ism module.
REM	     Changed where setup.exe is copied from to make sure copy
REM	     is always successful.
REM	11-Dec-2010 (drivi01)
REM	     IMPORTANT!! 
REM	     Updated the parameter list to check for %ING_SRC%\back\x100\x100
REM	     directory if no product is specified.  if x100 directory is
REM	     missing assume this is INGRES, otherwise IVW.
REM	     Revisit this when codev and main are merged.
REM	     Destination directory is changed back to II_CDIMAGE_DIR for the 
REM	     the time being.
REM	28-Feb-2011 (drivi01)
REM	     Add steps for rolling VectorWise documentation installer.
REM	07-Mar-2011 (drivi01)
REM	     Updated signing of redistributable to address msi as
REM	     MSredists%SUFFIX%.msi to accomodate signing of
REM	     Redistributable module for 64-bit builds.
REM	11-Mar-2011 (drivi01)
REM	     Add ICE for 64-bit installer for support of
REM	     32-bit to 64-bit upgrades.
REM	     Also add old documentation features/components
REM	     and i18n features/components.
REM	     Add 32-bit modules, they do not contain any 
REM	     files but contain missing components.
REM	     linking with 32-bit modules should not cause
REM	     any problems for upgrades, as long
REM	     as components are resolved.
REM	     Linking with 32-bit modules to avoid submitting
REM	     64-bit ISM files for each of those features.
REM	17-Mar-2011 (drivi01)
REM	     Fix buildrel to always build old Spatials module
REM	     which doesn't ship any files now.
REM	     Call readvers from this file to determine if
REM	     geospatial files need to be built. (Needs change 510134).
REM	     Add new geospatial merge module and conditionally
REM	     roll it into the image.  The feature inclusion is
REM	     controlled with Geos release flag.
REM	06-Apr-2011 (drivi01)
REM	     Fix the buildrel script to copy 64-bit readme
REM	     to the cdimage for 64-bit builds and 32-bit for
REM	     32-bit builds.
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
REM	14-May-2011 (drivi01)
REM	     Update version in the new IngresVWDBMS64.ism to
REM	     reflect the product version.
REM	10-Jun-2011 (drivi01)
REM	     Add a product specific sample.rsp.  Copy a
REM	     relevent response file based on product
REM	     image being created.
REM	08-Jul-2011 (drivi01)
REM	     Remove Vision, Replicator, and Star from the
REM	     installer features.  For IVW do not build
REM	     IngresDBMSNetVision64.ism
REM	     IngresIIDBMSVision64.ism
REM	     IngresIIToolsVision64.ism
REM	     IngresIIVision64.ism
REM	     IngresDBMSReplicator64.ism
REM	     IngresIIReplicator64.ism
REM	     IngresIIStar64.ism
REM	12-Jul-2011 (drivi01)
REM          Add IngresVWTools64.ism and IngresVWNet64.ism.
REM          The corresponding II modules contain dependencies
REM          on Vision which can not be removed without effecting
REM          Ingres product.
REM	09-Sep-2011 (drivi01)
REM	     Add Management Studio Discovery service/processes
REM	     to the distribution.
REM	08-Sep-2011 (bonro01)
REM	     Remove Ingres%PRODUCT%_Doc.ism and IngresII_DotNETDataProvider.ism
REM	     from -p output because these are separate packages and should not be
REM	     part of the patch.
REM	11-Oct-2011 (drivi01)
REM	     After the builds were switched over to Standalone build version of
REM	     InstallShield, default merge module path is insufficient for our builds
REM	     and there's no GUI to configure it.  This change adds MM_SEARCH_PATH
REM	     for MSredist%SUFFIX% and Ingres%PRODUCT%_DBL%SUFFIX%.ism modules
REM	     as a parameter to ensure that all redistributables are found at
REM	     build time.   MM_SEARCH_PATH will be a new environment variable
REM	     that will need to be set, but only if Standalone build version of
REM	     InstallShield is used.  buildrel will detect the GUI version of 
REM	     InstallShield and will not prompt to set MM_SEARCH_PATH.
REM	17-Oct-2011 (drivi01)
REM	     Frequent flyer demo will no longer be shipped with Vectorwise 
REM	     product.  Disable it from building for VW product.
REM	     Replace IngresIIDBMSDotNETDataProvider.ism with VW version that
REM	     contains empty components.
REM	31-Oct-2011 (drivi01)
REM	     Add chmod 777 and 444 back.  Looks like they got lost in
REM	     integrations somehow.
REM	

set SUFFIX=
set PRODUCT=
set NOSPAT=
set SIGNED=NOT SIGNED
set STRONG_NAME_SIG=NOT SIGNED
set INGRES_STATUS=SKIPPED
set DOC_STATUS=SKIPPED
set DOTNET_STATUS=SKIPPED
set DBA_STATUS=SKIPPED
set ERROR_STATUS=0
set MSI_RELEASE_FLAGS=

call readvers.bat

REM if "%CPU%" == "IA64" set SUFFIX=64
if "%CPU%" == "AMD64" set SUFFIX=64

if not x%II_RELEASE_DIR%==x goto CHECK1
echo You must set II_RELEASE_DIR !!
goto DONE

:CHECK1
if not x%II_SYSTEM%==x goto CHECK2
echo You must set II_SYSTEM !!
goto DONE

:CHECK2
if not x%ING_DOC%==x goto CHECK3
echo You must set ING_DOC !!
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
if "%1" == "-p" (
  if "%conf_IVW%" == "true" (set PRODUCT=VW) else (set PRODUCT=II) 
  goto LISTALL
) else (
  if "%2" == "-p" (
    if "%conf_IVW%" == "true" (set PRODUCT=VW& goto LISTALL)
    if not "%conf_IVW%" == "true" (set PRODUCT=II& goto LISTALL) 
    goto CONT0_9
  ) else (
    goto CHECK6
  )
)

:CHECK6
if exist %ING_SRC%\LICENSE.gpl goto CHECK7
echo You must have at least GPL license in %ING_SRC% !!
goto DONE

:CHECK7
if exist %ING_SRC%\LICENSE.gpl.rtf goto CHECK8
echo You must have RTF version of GPL license in %ING_SRC% !!
goto DONE

:CHECK8
if exist %ING_SRC%\LICENSE.gpl.pdf goto CHECK9
echo You must have pdf version of GPL license in %ING_SRC% !!
goto DONE

:CHECK9
REM Check if we're using StandaloneBuild, MM_SEARCH_PATH must be specified 
REM for StandaloneBuild of InstallShield

which IsCmdBld|grep StandaloneBuild
if errorlevel 1 goto CHECK10
set MSI_RELEASE_FLAGS=-o "%MM_SEARCH_PATH%"
if not x"%MM_SEARCH_PATH%"==x"" goto CHECK10
echo You must set MM_SEARCH_PATH to the merge module search path to look
echo for redistributables. The merge modules path located under InstallShield
echo Premier directories must be at the front of the path.  If the default
echo location path for the InstallShield was used, the suggested MM_SEARCH_PATH
echo is:
echo MM_SEARCH_PATH=C:\InstallShield 2010 Projects\MergeModules,C:\Program Files (x86)\InstallShield\2010\Modules\i386,C:\Program Files (x86)\InstallShield\2010\Objects,C:\Program Files (x86)\Common Files\Merge Modules
goto DONE

:CHECK10
if "%1"=="" (
    if "%conf_IVW%" == "true" (
	set PRODUCT=VW
    ) else (
	set PRODUCT=II
    )  
)
if "%1"=="-a" goto CONT1
if "%2"=="-lmtd" goto CONT0
if "%3"=="-lmtd" goto CONT0
if "%1"=="-doc" (set II_RELEASE_NODOC=ON&goto CONT1)
if "%2"=="-doc" (set II_RELEASE_NODOC=ON)
if "%3"=="-doc"  (set II_RELEASE_NODOC=ON)
if "%1"=="-dotnet" (set II_RELEASE_NODOTNET=ON&goto CONT1)
if "%2"=="-dotnet" (set II_RELEASE_NODOTNET=ON)
if "%3"=="-dotnet" (set II_RELEASE_NODOTNET=ON)
if "%1"=="-doconly" (set II_RELEASE_DOC=ON&goto CONT1)
if "%2"=="-doconly" (set II_RELEASE_DOC=ON)
if "%1"=="-dotnetonly" (set II_RELEASE_DOTNET=ON&goto CONT1)
if "%2"=="-dotnetonly" (set II_RELEASE_DOTNET=ON)
if "%1"=="-?" goto CONT0_9
if "%1"=="-help"  goto CONT0_9
goto CONT1

:CONT0
set II_RELEASE_LMTD=ON
echo This option has been disabled!
goto DONE


:CONT0_9
echo "USAGE: buildrel [<product(IVW|II)>] [-a| -p|-doc|-dotnet|-doconly|-dotnetonly]"
goto DONE

:CONT1
echo.
"%MKSLOC%\echo" "Version: \c" 
cat %II_SYSTEM%/ingres/version.rel
echo Release directory: %II_RELEASE_DIR%
echo CD image directory: %II_CDIMAGE_DIR%
echo Merge Modules directory: %II_MM_DIR%
echo II_RELEASE_LMTD=%II_RELEASE_LMTD%
if "%II_RELEASE_NODOC%"=="ON" (echo NO DOC BUILD )
if "%II_RELEASE_NODOTNET%"=="ON" (echo NO DOTNET BUILD )
if "%II_RELEASE_DOC%"=="ON" (echo DOC ONLY BUILD) 
if "%II_RELEASE_DOTNET%"=="ON" (echo DOTNET ONLY BUILD)
echo PRODUCT=%PRODUCT%
echo.
if "%II_RELEASE_DOC%"=="ON" goto DOC_IMAGE
if "%II_RELEASE_DOTNET%"=="ON" goto DOTNET_IMAGE
echo.

:AUDIT
echo Auditing Merge Modules...
rm -f %ING_SRC%\buildrel_report.txt
call gd enterprise_is_win
call chmod 777 *.ism
@echo off
rm -f buildrel_report.txt
if "%II_RELEASE_LMTD%" == "ON" rm -rf %II_RELEASE_DIR%\IngresIIi18n %II_RELEASE_DIR%\IngresIIDoc %II_RELEASE_DIR%\IngresII\*
msidepcheck Ingres%PRODUCT%DBMS%SUFFIX%.ism 
msidepcheck IngresIIDBMSNet_DBL%SUFFIX%.ism 
msidepcheck IngresIIDBMSNetMgmt%SUFFIX%.ism
msidepcheck IngresIIDBMSNetTools%SUFFIX%.ism 
msidepcheck IngresIIDBMSTools%SUFFIX%.ism 
msidepcheck IngresIIDBMSVdba%SUFFIX%.ism 
msidepcheck IngresIIEsqlC%SUFFIX%.ism 
msidepcheck IngresIIEsqlCEsqlCobol%SUFFIX%.ism 
msidepcheck IngresIIJdbc%SUFFIX%.ism 
msidepcheck IngresIIODBC%SUFFIX%.ism 
msidepcheck IngresIIVdba%SUFFIX%.ism 
msidepcheck Ingres%PRODUCT%Net%SUFFIX%.ism 
msidepcheck IngresIINetTools%SUFFIX%.ism 
msidepcheck MSRedists%SUFFIX%.ism 
msidepcheck Ingres%PRODUCT%DBMSDotNETDataProvider%SUFFIX%.ism
if "%EVALBUILD%" == "ON" goto AUDIT_SDK1
msidepcheck IngresIIEsqlCobol%SUFFIX%.ism 
msidepcheck IngresIIEsqlFortran%SUFFIX%.ism 
msidepcheck Ingres%PRODUCT%Tools%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIDBMSNetIce.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIReplicator%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIDBMSIce.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIDBMSNetVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIDBMSReplicator%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIDBMSVision%SUFFIX%.ism
if "%PRODUCT%" == "II" msidepcheck IngresIIIce.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIDotNETDataProvider.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIStar%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIToolsVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck IngresIIVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" if not exist IngresIISpat%SUFFIX%.ism set NOSPAT=ON
if "%PRODUCT%" == "II" if not "%NOSPAT%" == "ON" msidepcheck IngresIISpat%SUFFIX%.ism
if NOT "%II_RELEASE_LMTD%" == "ON" if "%PRODUCT%" == "II" msidepcheck IngresIIDoc.ism
if NOT "%II_RELEASE_LMTD%" == "ON" if "%PRODUCT%" == "II" msidepcheck IngresIIi18n.ism 
if "%conf_WITH_GEO%"=="true" if "%PRODUCT%" == "II" msidepcheck IngresIIDBMSNetGeos%SUFFIX%.ism
if NOT "x%II_RELEASE_NODOC%"=="xON" msidepcheck Ingres%PRODUCT%_Doc.ism
if NOT "x%II_RELEASE_NODOTNET%"=="xON" msidepcheck IngresII_DotNETDataProvider.ism
@echo Auditing Ingres setup package...
if "%II_RELEASE_LMTD%" == "ON" goto AUDIT_CONT_LMTD
msidepcheck Ingres%PRODUCT%_DBL%SUFFIX%.ism 
goto AUDIT_CONT_2
:AUDIT_SDK1
msidepcheck IngresSDK.ism 
:AUDIT_CONT_LMTD
msidepcheck IngresII_DBL_LMTD.ism
goto AUDIT_CONT_2

:AUDIT_CONT_2
if exist buildrel_report.txt mv buildrel_report.txt %ING_SRC%\buildrel_report.txt
if exist %ING_SRC%\buildrel_report.txt echo There are some files missing or zero length:& echo.
if exist %ING_SRC%\buildrel_report.txt type %ING_SRC%\buildrel_report.txt& echo.
if exist %ING_SRC%\buildrel_report.txt echo A log of these missing files has been saved in %ING_SRC%\buildrel_report.txt.& goto DONE
if "%1" == "-a" goto CONT4_1
if "%2" == "-a" goto CONT4_1
goto DIGSIG_0

:INST64
devenv /useenv /rebuild Release64 /project enterprise enterprise.sln
goto DIGSIG_0

:INSTAMD64
devenv /useenv /rebuild ReleaseAMD64 /project enterprise enterprise.sln


:DIGSIG_0
echo.
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
set TEMP1=%TEMP%\picklist.$$%RANDOM%
if exist "%TEMP1%" del "%TEMP1%"
for /F %%I IN (%II_SYSTEM%\ingres\picklist.dat) do (
	if %%~xI==.cab echo %%I|grep \\ >> "%TEMP1%"
	if %%~xI==.exe echo %%I|grep \\ >> "%TEMP1%"
	if %%~xI==.dll echo %%I|grep \\ >> "%TEMP1%"
	if %%~xI==.dist echo %%I|grep \\ >> "%TEMP1%"
)
set TEMP2=%TEMP%\picklist.$$%RANDOM%
if exist "%TEMP2%" del "%TEMP2%"
for /F "delims=. tokens=1*" %%I IN (%TEMP1%) do (
echo %II_SYSTEM%\ingres%%I.%%J >> "%TEMP2%"
)
if exist "%TEMP1%" del "%TEMP1%"

echo.
echo Checking if the files are signed already
echo.
for /F %%I in (%TEMP2%) do (
echo signtool verify /pa /q "%%~I"
signtool verify /pa /q "%%~I"
if ERRORLEVEL 1 call iisign "%%~I"
if not ERRORLEVEL 0 goto DIGSIG_ERROR
)
if exist "%TEMP2%" del %TEMP2%
if not ERRORLEVEL 1 goto DIGSIG_DONE 

:DIGSIG_DONE
echo.
echo All Files are digitally signed!
echo.
set SIGNED=TRUE
goto CONT2

:CONT2
echo.
echo Copying preinstall directory...
echo.
call mkpreinstallAPI
if errorlevel 1 goto ERROR2
echo.
echo Copying thinclient directory...
echo.
call mkthinclient
if errorlevel 1 goto ERROR2
echo.

:CONT_EXTRAS
echo.
echo Copying embedded extras directory...
echo.
call mkembedextras
if errorlevel 1 goto ERROR2
echo.
echo
echo Copying pdbs directory...
echo.
call mkpdbdir.bat
REM if errorlevel 1 goto ERROR2
@echo off
echo.
echo Building Merge Modules...
call gd enterprise_is_win
REM set SUFFIX=64
ISCmdBld.exe -x -p Ingres%PRODUCT%DBMS%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMS
if errorlevel 1 goto ERROR
REM set SUFFIX=
@echo.
ISCmdBld.exe -x -p IngresIIDBMSNet_DBL%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSNet
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIDBMSNetTools%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSNetTools
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIDBMSNetMgmt%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSNetMgmt
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIDBMSTools%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSTools
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIDBMSVdba%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSVdba
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIEsqlC%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIEsqlC
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIEsqlCEsqlCobol%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIEsqlCEsqlCobol
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIJdbc%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIJdbc
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIODBC%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIODBC
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIVdba%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIVdba
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p Ingres%PRODUCT%Net%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIINet
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIINetTools%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIINetTools
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p Ingres%PRODUCT%DBMSDotNETDataProvider.ism -b %II_RELEASE_DIR%\IngresIIDBMSDotNETDataProvider
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p Ingres%PRODUCT%Tools%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIITools
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p MSRedists%SUFFIX%.ism -b %II_RELEASE_DIR%\MSRedists %MSI_RELEASE_FLAGS%
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIReplicator%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIReplicator
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIStar%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIStar
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIToolsVision%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIToolsVision
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIVision%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIVision
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIIce.ism -b %II_RELEASE_DIR%\IngresIIIce
if errorlevel 1 goto ERROR
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIDBMSIce.ism -b %II_RELEASE_DIR%\IngresIIDBMSIce
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIDBMSNetIce.ism -b %II_RELEASE_DIR%\IngresIIDBMSNetIce
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIDBMSNetVision%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSNetVision
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIDBMSReplicator%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSReplicator
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIDBMSVision%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIDBMSVision
if errorlevel 1 goto ERROR
@echo.
if NOT "%II_RELEASE_LMTD%" == "ON" if "%PRODUCT%" == "II"  ISCmdBld.exe -x -p IngresIIDoc.ism -b %II_RELEASE_DIR%\IngresIIDoc
if errorlevel 1 goto ERROR
@echo.
if NOT "%II_RELEASE_LMTD%" == "ON" if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIi18n.ism -b %II_RELEASE_DIR%\IngresIIi18n
if errorlevel 1 goto ERROR
@echo.
if "%PRODUCT%" == "II" ISCmdBld.exe -x -p IngresIIDotNETDataProvider.ism -b %II_RELEASE_DIR%\IngresIIDotNETDataProvider
if errorlevel 1 goto ERROR
if "%PRODUCT%" == "II" if NOT "%NOSPAT%" == "ON" ISCmdBld.exe -x -p IngresIISpat%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIISpat
if errorlevel 1 goto ERROR
if "%conf_WITH_GEO%"=="true" if "%PRODUCT%" == "II" IsCmdBld.exe -x -p IngresIIDBMSNetGeos%SUFFIX%.ism
if errorlevel 1 goto ERROR
if "%EVALBUILD%" == "ON" goto SDK1
@echo.
ISCmdBld.exe -x -p IngresIIEsqlCobol%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIEsqlCobol
if errorlevel 1 goto ERROR
@echo.
ISCmdBld.exe -x -p IngresIIEsqlFortran%SUFFIX%.ism -b %II_RELEASE_DIR%\IngresIIEsqlFortran
if errorlevel 1 goto ERROR
@echo.
@echo Building Ingres setup package...
@echo.
if "%II_RELEASE_LMTD%" == "ON" goto DBL2_LMTD
set RELEASE_FLAGS=NoFlag
if "%conf_WITH_GEO%"=="true" if "%PRODUCT%" == "II" set RELEASE_FLAGS=Geos
ISCmdBld.exe -x -p Ingres%PRODUCT%_DBL%SUFFIX%.ism -f "%RELEASE_FLAGS%" -b %II_RELEASE_DIR%\IngresII  %MSI_RELEASE_FLAGS%
if errorlevel 1 goto ERROR
@echo.
goto CONT
:SDK1
ISCmdBld.exe -x -p IngresSDK.ism -b %II_RELEASE_DIR%\IngresSDK
if errorlevel 1 goto ERROR
@echo.

:DBL2_LMTD
ISCmdBld.exe -x -p IngresII_DBL_LMTD.ism -b %II_RELEASE_DIR%\IngresII
if errorlevel 1 goto ERROR
@echo.
goto CONT

:CONT
call chmod 444 *.ism
@echo off
echo Creating mergemodules...
if not exist %II_MM_DIR% mkdir %II_MM_DIR%
rm -rf %II_MM_DIR%/*
cp -p -v %II_RELEASE_DIR%\/*/\build\release\DiskImages\Disk1\/*.msm %II_MM_DIR%
cp -p -v %ING_BUILD%\bin\msiupd.exe %II_MM_DIR%

echo.
echo Update version in mergemodules
echo.
msiversupdate %II_MM_DIR%\IngresIIDBMS%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIDBMSNet_DBL%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIDBMSNetTools%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIDBMSNetMgmt%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIDBMSTools%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIDBMSVdba%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIEsqlC%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIEsqlCEsqlCobol%SUFFIX%.msm  
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIJdbc%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIODBC%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIVdba%SUFFIX%.msm
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIINet%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIINetTools%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIEsqlCobol%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIIEsqlFortran%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
msiversupdate %II_MM_DIR%\IngresIITools%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIIce.msm 
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDBMSIce.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDBMSNetIce.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDBMSVision%SUFFIX%.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDBMSNetVision%SUFFIX%.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDBMSReplicator%SUFFIX%.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIReplicator%SUFFIX%.msm  
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIStar%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIToolsVision%SUFFIX%.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIVision%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" if NOT "%NOSPAT%" == "ON" msiversupdate %II_MM_DIR%\IngresIISpat%SUFFIX%.msm 
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDoc.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIi18n.msm
if errorlevel 1 goto ERROR3
if "%PRODUCT%" == "II" msiversupdate %II_MM_DIR%\IngresIIDotNetDataProvider.msm
if errorlevel 1 goto ERROR3
if "%conf_WITH_GEO%"=="true" msiversupdate %II_MM_DIR%\IngresIIDBMSNetGeos%SUFFIX%.msm
if errorlevel 1 goto ERROR3
echo FINISHED with msiversupdate

echo Creating cdimage...
if not exist %II_CDIMAGE_DIR% mkdir %II_CDIMAGE_DIR%
rm -rf %II_CDIMAGE_DIR%\files/*
rm -rf %II_CDIMAGE_DIR%/*
mkdir %II_CDIMAGE_DIR%\files
mkdir %II_CDIMAGE_DIR%\files\msredist
cp -p -v %II_RELEASE_DIR%\MSRedists\build\release\DiskImages\Disk1\/*.* %II_CDIMAGE_DIR%\files\msredist
if "%EVALBUILD%" == "ON" goto SDK2
cp -p -v %II_RELEASE_DIR%\IngresII\build\release\DiskImages\Disk1\/*.* %II_CDIMAGE_DIR%\files
echo.
if "%PRODUCT%"=="II" if exist %II_CDIMAGE_DIR%\files\Ingres.msi ren %II_CDIMAGE_DIR%\files\Ingres.msi IngresII.msi
if errorlevel 1 goto ERROR1
echo.
echo Update version in Ingres msi
echo.
msiversupdate %II_CDIMAGE_DIR%\files\Ingres%PRODUCT%.msi
if errorlevel 1 goto ERROR3

rm -rf %II_CDIMAGE_DIR%\files\setup.*
if "%CPU%" == "IA64" cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\Release64\setup.exe %II_CDIMAGE_DIR%& goto CONT5
if "%CPU%" == "AMD64" if "%DEBUG%" == "ON" cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\DebugAMD64\setup.exe %II_CDIMAGE_DIR%& goto CONT5
if "%CPU%" == "AMD64" cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\ReleaseAMD64\setup.exe %II_CDIMAGE_DIR%& goto CONT5
cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\Release\setup.exe %II_CDIMAGE_DIR%
:CONT5
cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\AUTORUN.INF %II_CDIMAGE_DIR%
cp -p -v %II_SYSTEM%\readme.html %II_CDIMAGE_DIR%
if "%SUFFIX%" == "" (
cp -p -v %II_SYSTEM%\readme_int_w32.html %II_CDIMAGE_DIR% 
) else (
cp -p -v %II_SYSTEM%\readme_a64_win.html %II_CDIMAGE_DIR%
)
if "%PRODUCT%"=="VW" (
cp -p -v %ING_SRC%\LICENSE.eval %II_CDIMAGE_DIR%\LICENSE
cp -p -v %ING_SRC%\LICENSE.eval.rtf %II_CDIMAGE_DIR%\LICENSE.rtf
cp -p -v %ING_SRC%\LICENSE.eval.pdf %II_CDIMAGE_DIR%\LICENSE.pdf
cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\sampleVW.rsp %II_CDIMAGE_DIR%
) else (
cp -p -v %ING_SRC%\LICENSE.gpl %II_CDIMAGE_DIR%\LICENSE
cp -p -v %ING_SRC%\LICENSE.gpl.rtf %II_CDIMAGE_DIR%\LICENSE.rtf
cp -p -v %ING_SRC%\LICENSE.gpl.pdf %II_CDIMAGE_DIR%\LICENSE.pdf
cp -p -v %ING_SRC%\front\st\enterprise_preinst_win\sample.rsp %II_CDIMAGE_DIR%
)


:CONT5_1
echo Updating MSI database...
msiupd %II_CDIMAGE_DIR%\files\Ingres%PRODUCT%.msi
set INGRES_STATUS=COMPLETED SUCCESSFULLY
echo.
echo Signing CD image
echo.
call iisign %II_CDIMAGE_DIR%\files\Ingres%PRODUCT%.msi
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
call iisign %II_CDIMAGE_DIR%\files\Data1.cab
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
call iisign %II_CDIMAGE_DIR%\setup.exe
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
call iisign %II_CDIMAGE_DIR%\files\msredist\MSRedists%SUFFIX%.msi
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
call iisign %II_CDIMAGE_DIR%\files\msredist\Data1.cab
if not ERRORLEVEL 0 goto DIGSIG_ERROR2
call iisign %II_CDIMAGE_DIR%\files\msredist\setup.exe
if not ERRORLEVEL 0 goto DIGSIG_ERROR2


:DOC_IMAGE
echo.
echo Creating documentation image...
echo.
if "%II_RELEASE_NODOC%" == "ON" goto DOTNET_IMAGE
cd %ING_SRC%\front\st\enterprise_is_win
@echo
ISCmdBld.exe -x -p Ingres%PRODUCT%_Doc.ism -b %II_RELEASE_DIR%\IngresII_Doc
if errorlevel 1 goto DOC_ERROR
if not exist %II_CDIMAGE_DIR%\files\documentation mkdir %II_CDIMAGE_DIR%\files\documentation
rm -rf %II_CDIMAGE_DIR%\files\documentation/*
cp -p -v %II_RELEASE_DIR%\IngresII_Doc\build\release\DiskImages\Disk1\/*.* %II_CDIMAGE_DIR%\files\documentation
rm -rf %II_CDIMAGE_DIR%\files\documentation\setup.*
cp -p -v %II_SYSTEM%\ingres\bin\SetupDriver.exe %II_CDIMAGE_DIR%\files\documentation\setup.exe
cp -p -v %ING_SRC%\LICENSE.gpl %II_CDIMAGE_DIR%\files\documentation\LICENSE
cp -p -v %ING_SRC%\LICENSE.gpl.rtf %II_CDIMAGE_DIR%\files\documentation\LICENSE.rtf
cp -p -v %ING_SRC%\LICENSE.gpl.pdf %II_CDIMAGE_DIR%\files\documentation\LICENSE.pdf

FOR /F "usebackq tokens=2 delims= " %%A IN (`genrelid`) DO set ING_VERS=%%A
FOR /F "usebackq tokens=1,2,* delims=." %%A in (`echo %ING_VERS%`) DO set ING_VERS=%%A.%%B
if "%PRODUCT%"=="VW" set PRODUCTNAME=VectorWise
if "%PRODUCT%"=="II" set PRODUCTNAME=Ingres
msiversupdate "%II_CDIMAGE_DIR%\files\documentation\%PRODUCTNAME% Documentation %ING_VERS%.msi"
msiupd "%II_CDIMAGE_DIR%\files\documentation\%PRODUCTNAME% Documentation %ING_VERS%.msi"
if errorlevel 1 goto ERROR3
set DOC_STATUS=COMPLETED SUCCESSFULLY
echo.
echo Signing documentation CD image
echo.
call iisign "%II_CDIMAGE_DIR%\files\documentation\%PRODUCTNAME% Documentation %ING_VERS%.msi"
if not ERRORLEVEL 0 goto DIGSIG_ERROR3
call iisign %II_CDIMAGE_DIR%\files\documentation\Data1.cab
if not ERRORLEVEL 0 goto DIGSIG_ERROR3
call iisign %II_CDIMAGE_DIR%\files\documentation\setup.exe
if not ERRORLEVEL 0 goto DIGSIG_ERROR3
if "%II_RELEASE_DOC%" == "ON" goto CONT4

:DOTNET_IMAGE
echo.
echo Creating Ingres .NET Data Provider image...
echo.
if "%II_RELEASE_NODOTNET%" == "ON" goto DBA_TOOLS
echo.
echo Use Strong Name to Sign the Assembly
echo.
@echo off
cd %ING_SRC%\front\st\enterprise_is_win
if exist %ING_BUILD%\\picklist.dat cp %ING_BUILD%\\picklist.dat %ING_BUILD%\\picklist.$$temp
if exist %ING_BUILD%\\picklist.dat rm %ING_BUILD%\\picklist.dat
msidepcheck -p IngresII_DotNETDataProvider.ism
type %ING_BUILD%\\picklist.dat
for /F "delims=\ tokens=1*" %%I IN (%II_SYSTEM%\ingres\picklist.dat) do (
	if %%~xJ==.dll sn.exe -v %II_SYSTEM%\ingres\%%J
	if ERRORLEVEL 1 if %%~xJ==.dll sn.exe -Rca %II_SYSTEM%\ingres\%%J dotnet21
	if ERRORLEVEL 1 goto ASSEMBLY_ERROR
	if %%~xJ==.dll sn.exe -v %II_SYSTEM%\ingres\%%J
	if ERRORLEVEL 1 goto ASSEMBLY_ERROR
)
set STRONG_NAME_SIG=TRUE
echo.
echo .Net Data Provider assembly is successfully signed with a strong name key.
echo.
:DOTNET_CONT
cp %ING_BUILD%\\picklist.$$temp %iNG_BUILD%\\picklist.dat
cd %ING_SRC%\front\st\enterprise_is_win
if not exist IngresII_DotNETDataprovider mkdir IngresII_DotNETDataProvider
@echo
ISCmdBld.exe -x -p IngresII_DotNETDataProvider.ism -b %II_RELEASE_DIR%\IngresII_DotNETDataProvider
if errorlevel 1 goto DOTNET_ERROR
rm -rf IngresII_DotNETDataProvider
if not exist %II_CDIMAGE_DIR%\files\dotnet mkdir %II_CDIMAGE_DIR%\files\dotnet
rm -rf %II_CDIMAGE_DIR%\files\dotnet/*
cp -p -v %II_RELEASE_DIR%\IngresII_DotNETDataProvider\build\release\DiskImages\Disk1\/*.* %II_CDIMAGE_DIR%\files\dotnet
rm -rf %II_CDIMAGE_DIR%\files\dotnet\setup.*
cp -p -v %II_SYSTEM%\ingres\bin\SetupDriver.exe %II_CDIMAGE_DIR%\files\dotnet\setup.exe
cp -p -v %ING_SRC%\LICENSE.gpl %II_CDIMAGE_DIR%\files\dotnet\LICENSE
cp -p -v %ING_SRC%\LICENSE.gpl.rtf %II_CDIMAGE_DIR%\files\dotnet\LICENSE.rtf
cp -p -v %ING_SRC%\LICENSE.gpl.pdf %II_CDIMAGE_DIR%\files\dotnet\LICENSE.pdf
msiupd "%II_CDIMAGE_DIR%\files\dotnet\Ingres .NET Data Provider 2.1.msi"
set DOTNET_STATUS=COMPLETED SUCCESSFULLY
echo.
echo Signing .NET Data Provider CD image
echo.
call iisign "%II_CDIMAGE_DIR%\files\dotnet\Ingres .NET Data Provider 2.1.msi"
if not ERRORLEVEL 0 goto DIGSIG_ERROR4
call iisign %II_CDIMAGE_DIR%\files\dotnet\Data1.cab
if not ERRORLEVEL 0 goto DIGSIG_ERROR4
call iisign %II_CDIMAGE_DIR%\files\dotnet\setup.exe
if not ERRORLEVEL 0 goto DIGSIG_ERROR4
if "%II_RELEASE_DOTNET%" == "ON" goto CONT4
if "%SUFFIX%"=="" goto DBA_TOOLS
goto CONT3

:DBA_TOOLS
echo.
echo Attempting to creating DBA Tools image...
echo.
call builddba.bat 
if not ERRORLEVEL 0 goto DBA_ERROR
set DBA_STATUS=COMPLETED SUCCESSFULLY
goto CONT3

:SDK2
xcopy /y "%II_RELEASE_DIR%\IngresSDK\build\release\DiskImages\Disk1\*.msi" "%II_CDIMAGE_DIR%\files\*.msi"
cp -p -v %II_RELEASE_DIR%\IngresSDK\build\release\DiskImages\Disk1\/*.* %II_CDIMAGE_DIR%\files
cp -p -v %ING_SRC%\front\st\enterprise_preinst\AUTORUN.INF %II_CDIMAGE_DIR%
cp -p -v %II_SYSTEM%\readme_ingres_sdk.html %II_CDIMAGE_DIR%

:CONT3
if not exist %II_CDIMAGE_DIR%\files\dotnet mkdir %II_CDIMAGE_DIR%\files\dotnet
if "%II_RELEASE_DOTNET%" == "ON" goto DONE
cp -p -v %ING_BUILD%\bin\ingwrap.exe %II_CDIMAGE_DIR%\files\ingwrap.exe


if "%EVALBUILD%" == "ON" goto SDK3
goto CONT4

:SDK3
rm -rf %II_CDIMAGE_DIR%/*.msm
cp -p -v -r "%ING_SRC%\Forest And Trees RTL" "%II_CDIMAGE_DIR%\Forest And Trees RTL"
cp -p -v -r "%ING_SRC%\OpenROAD" "%II_CDIMAGE_DIR%\OpenROAD"
cp -p -v -r "%ING_SRC%\CleverPath Portal" "%II_CDIMAGE_DIR%\CleverPath Portal"
cp -p -v -r "%ING_SRC%\Apache HTTP Server" "%II_CDIMAGE_DIR%\Apache HTTP Server"
cp -p -v -r "%ING_SRC%\DBupdate" "%II_CDIMAGE_DIR%\DBupdate"
cd /d %II_CDIMAGE_DIR%
ren setup.exe install.exe
ren *.msi "Ingres SDK.msi"
echo Updating MSI database...
msiupd "%II_CDIMAGE_DIR%\Ingres SDK.msi"
goto CONT6

:CONT6
if not exist %II_SYSTEM%\ingres\temp mkdir %II_SYSTEM%\ingres\temp
ls %II_MM_DIR%|wc -l| %AWK_CMD% '{print "\n\n"$1" mergemodules created!!\n\n" }' 
goto CONT4

:LISTALL
if exist %ING_BUILD%\picklist.dat del %ING_BUILD%\picklist.dat
call gd enterprise_is_win
goto LISTALL_1

:LISTALL_1
msidepcheck -p Ingres%PRODUCT%DBMS%SUFFIX%.ism 
if "%DOUBLEBYTE%" == "ON" goto LISTALL_DBL1
msidepcheck -p IngresIIDBMSNet%SUFFIX%.ism 
goto LISTALL_CONT_1
:LISTALL_DBL1
msidepcheck -p IngresIIDBMSNet_DBL%SUFFIX%.ism 
:LISTALL_CONT_1
msidepcheck -p IngresIIDBMSNetMgmt%SUFFIX%.ism 
msidepcheck -p IngresIIDBMSNetTools%SUFFIX%.ism 
msidepcheck -p IngresIIDBMSTools%SUFFIX%.ism 
msidepcheck -p IngresIIDBMSVdba%SUFFIX%.ism 
msidepcheck -p IngresIIEsqlC%SUFFIX%.ism 
msidepcheck -p IngresIIEsqlCEsqlCobol%SUFFIX%.ism 
msidepcheck -p IngresIIJdbc%SUFFIX%.ism 
msidepcheck -p IngresIIODBC%SUFFIX%.ism 
msidepcheck -p IngresIIVdba%SUFFIX%.ism 
msidepcheck -p Ingres%PRODUCT%Net%SUFFIX%.ism 
msidepcheck -p IngresIINetTools%SUFFIX%.ism 
msidepcheck -p Ingres%PRODUCT%DBMSDotNETDataProvider%SUFFIX%.ism
msidepcheck -p MSRedists%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDBMSIce.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDBMSNetIce.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDBMSNetVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDBMSReplicator%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDBMSVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDoc.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIIce.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIDotNETDataProvider.ism 
if "%EVALBUILD%" == "ON" goto LISTALL_SDK1
msidepcheck -p IngresIIEsqlCobol%SUFFIX%.ism 
msidepcheck -p IngresIIEsqlFortran%SUFFIX%.ism 
msidepcheck -p Ingres%PRODUCT%Tools%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIReplicator%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIStar%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIToolsVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" msidepcheck -p IngresIIVision%SUFFIX%.ism 
if "%PRODUCT%" == "II" if NOT "%NOSPAT%" == "ON" msidepcheck -p IngresIISpat%SUFFIX%.ism
if "%conf_WITH_GEO%"=="true" if "%PRODUCT%" == "II" msidepcheck -p IngresIIDBMSNetGeos%SUFFIX%.ism
if "%DOUBLEBYTE%" == "ON" goto LISTALL_DBL2
msidepcheck -p IngresII%SUFFIX%.ism 
goto CONT4_0
:LISTALL_DBL2
msidepcheck -p Ingres%PRODUCT%_DBL%SUFFIX%.ism 
goto CONT4_0
:LISTALL_SDK1
msidepcheck -p IngresSDK.ism 

REM Call this statement only if we needed to get a distribution list for digital signing
REM If the distribution list wasn't for buildrel -p then return then it was for digital signing, return there
:CONT4_0
@echo off
if not "%2" == "-p" (if not "%1" == "-p" (goto DIGSIG))
goto CONT4_1

REM Call This statement if some type of image was rolled
:CONT4
if not exist %ING_BUILD%\build.timestamp touch  %ING_BUILD%\build.timestamp
echo.

if "%STRONG_NAME_SIG%"=="TRUE" set SN_STATUS=COMPLETED SUCCESSFULLY
if "%STRONG_NAME_SIG%"=="FALSE" set SN_STATUS=FAILED
if "%STRONG_NAME_SIG%"=="NOT SIGNED" set SN_STATUS=NOT SIGNED
if "%SIGNED%"=="TRUE" set SIG_STATUS=COMPLETED SUCCESSFULLY
if "%SIGNED%"=="FALSE" set SIG_STATUS=FAILED
if "%SIGNED%"=="NOT SIGNED" set SIG_STATUS=NOT SIGNED
echo.
echo ***********************BUILDREL SUMMARY***********************
echo * SCRIPT COMPLETED SUCCESSFULLY!!!
echo *** INGRES CD IMAGE: %INGRES_STATUS%
echo *** INGRES DOCUMENTATION IMAGE: %DOC_STATUS%
echo *** INGRES DOTNET IMAGE: %DOTNET_STATUS%
echo *** INGRES DBA TOOLS IMAGE: %DBA_STATUS%
echo * STRONG NAME SIGNING:  %SN_STATUS%
if "%STRONG_NAME_SIG%"=="FALSE" echo WARNING!!! YOUR .NET DATA PROVIDER IS NOT INSTALLABLE! STRONG NAME SIGNING FAILED!!!
echo * DIGITAL SIGNATURE SIGNING:  %SIG_STATUS%
if "%SIGNED%"=="FALSE" echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED!!!
echo **************************************************************
goto DONE

REM Call this statement if the image wasn't rolled but only
:CONT4_1
echo DONE!!!
goto DONE

:ERROR
echo.
echo Fatal Error: A build error occured while running ISCmdBld.exe to build Ingres CD image!!!
set INGRES_STATUS=FAILED
set ERROR_STATUS=1
goto DONE

:ERROR1
echo.
echo Error: Could not rename Ingres installation msi!!!
set INGRES_STATUS=FAILED
set ERROR_STATUS=2
goto CONT5_1

:ERROR2
echo.
echo Fatal Error: An error occured copying files for one of the build directories.
set ERROR_STATUS=3
goto DONE

:ERROR3
echo.
echo Fatal Error: An error occured updating version in one of the mergemodules or MSI projects.
set ERROR_STATUS=4
goto DONE

:ERROR4
echo.
echo Error: An error occured printing distribution to digitally sign the files.
set SIGNED=FALSE
set ERROR_STATUS=5
goto CONT2

:DOC_ERROR
echo.
echo Fatal Error: An error occured while building Documentation image!
echo You can try to rebuild Documentation image via buildrel -doconly.
set DOC_STATUS=FAILED
set ERROR_STATUS=6
goto DONE

:DOTNET_ERROR
echo.
echo Fatal Error: An error occured while building .NET Data Provider image!
echo You can try to rebuild .NET Data Provider image via buildrel -dotnetonly.
set DOTNET_STATUS=FAILED
set ERROR_STATUS=7
goto DONE

:DBA_ERROR
echo.
echo Fatal Error: An error occured while building DBA Tools image!
echo You can try to rebuild DBA Tools as a stand-alone image via builddba.bat.
set DBA_STATUS=FAILED
set ERROR_STATUS=8
goto DONE

:ASSEMBLY_ERROR
echo.
echo Error: An error occured while attempting to sign .NET Data Provider with strong name key.
set STRONG_NAME_SIG=FALSE
set ERROR_STATUS=9
if "%II_RELEASE_DOTNET%"=="ON" goto DONE
goto DOTNET_CONT

:DIGSIG_ERROR
echo.
set SIGNED=FALSE
set ERROR_STATUS=5
goto CONT2

:DIGSIG_ERROR2
echo.
set SIGNED=FALSE
set ERROR_STATUS=5
goto DOC_IMAGE

:DIGSIG_ERROR3
echo.
set SIGNED=FALSE
set ERROR_STATUS=5
if "%II_RELEASE_DOC%" == "ON" goto CONT4
goto DOTNET_IMAGE

:DIGSIG_ERROR4
echo.
set SIGNED=FALSE
set ERROR_STATUS=5
if "%II_RELEASE_DOTNET%"=="ON" goto CONT4
if "%SUFFIX%"=="" goto DBA_TOOLS
goto CONT3


:DONE
exit /b %ERROR_STATUS%
endlocal
