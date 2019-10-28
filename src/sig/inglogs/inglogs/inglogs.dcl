$!========================================================================
$!
$!  Copyright (c) 2011 Actian Corporation
$! 
$!  Name:  
$!      inglogs.com -- Take a snap shot from Ingres environment
$! 
$! USAGE: INGLOGS [-NOSYSTEMCONFIG] [-NOSYSTEMSTATE] 
$!                [-NOINGRESCONFIG] [-NOINGRESSTATE] [-NOLOGFILES] 
$!                [-NOPARALLEL]
$!                [-STATIC]
$!                [-DYNAMIC]
$!                [-NOSTACKS]
$!                [+FORCE_IIMON]
$!        INGLOGS [-H|-HELP]
$!        INGLOGS [--HELP]
$!        INGLOGS [-V|-VERSION|--VERSION]
$!
$!  Description: 
$!      This program collects all relevant info from current enviroment
$!      including Ingres, Sysem and user.
$!
$!      It is conform with the scripts available on Unix and Windows platforms
$!
$! Hints for usage:
$!   - If there is a need to use inglogs frequently then use:
$!       many times: inglogs  -DYNAMIC
$!                   reduced data and reduced runtime of inglogs
$!       at the end: inglogs 
$!                   Without any parameter inglogs will collect all relevant info
$!       
$! For special usage: 
$!        INGLOGS [-T]      shows how long inglogs took
$!        INGLOGS [-VERIFY] enables: SET VERIFY
$!
$! Prerequest: Use inglogs utility with the login account from 
$!             which Ingres processes are being started.
$!
$! Tips avoid process limitations:
$!     inglogs.com tries getting some info simoulteanouesly
$!         so several commands are performed as detach process.
$!     Having unsufficient quotas for running detach processes may cause the error:
$!        %SYSTEM-F-EXQUOTA   , process quota exceeded
$!        %SYSTEM-F-EXLNMQUOTA, logical name table is full
$!     See for detail:
$!            HELP/MESSAGE EXQUOTA
$!            HELP/MESSAGE EXLNMQUOTA 
$!     So defintely inglogs will fail to collect required info!
$! Therefor this error can be avoided:
$!     temporarily either run inglogs with option :
$!          @inglogs.com -noparallel
$!     or permanently increase the quota via:
$!          mc authorize modify <ingres-account> /jtquota=20000/prclm=50
$!          login as account owner once more and rerun inglogs
$!
$! If INGLOGSDIR logical has been setup with a directory name,
$!   all info will be collected in this specific directory.
$!
$! ################################################# Some limitations
$!  Only an inglogs process can be started at a time
$!  
$!  Wait for completeness of subprocesses!
$!     by default inglogs script will wait 30 seconds at maximum 
$!     before exiting.
$! 
$!  Gateway Server: It hasn't been considered yet.
$! 
$!  Bridge Server : It hasn't been considered yet.
$!
$!  Star Server   :
$!     server_class Star: Only default server_class "STAR" is being considered
$!
$!  IIGCC Server:
$!     BUG 123270 iimonitor may hang while connecting iigcc server
$!
$!  DASV Server:
$!     BUG 123270 iimonitor may hang while connecting iigcd server
$!
$!!  History:
$!!
$!!  14-Feb-2008	         Bilgihan Bircan          initial version       0.0
$!!     Collected all existing code and started building inglogs.com for VMS
$!! 
$!!  Dec-2009 and Feb-2010  Bilgihan Bircan         inglogs version 0.99
$!!    Many features have been added to initial release
$!!     Now inglogs can be used also with exception handler (II_EXCEPTION)
$!!                  
$!!  17-Feb-2010           Bilgihan Bircan          inglogs version 1.00/20100217
$!!    SHOW PROCESS/ALL <PROC> fails if the process owned by another user
$!!    Expected errr message: %SYSTEM-W-NONEXPR, nonexistent process
$!!      Now fixed
$!!    running inglogs from Ingres environment at production level II_SYSTEM system level
$!!    catches up, all other group Ingres processes on current machine, which is wrong.
$!! 
$!!  23-Feb-2010           Bilgihan Bircan          inglogs version 1.00/20100223
$!!    Add: iimonitor> stacks
$!! 
$!!  26-Feb-2010           Bilgihan Bircan          inglogs version 1.00/20100226
$!!    Change: SYSUAF was deassigned before MC AUTHORIZE could be completed
$!!    SWAPN ECHO fails --> SPAWN WRITE SYS$OUTPUT 
$!! 
$!!  01-Oct-2010           Bilgihan Bircan          inglogs version 1.00/20101001
$!!     Change: Added PLUS  for Recovery and DBMS process within iimonitor
$!!             Added Stack for Recovery          process within iimonitor
$!! 
$!!  05-Oct-2010           Bilgihan Bircan          inglogs version 1.10
$!!             Added comments how to handle with error
$!!                   %SYSTEM-F-EXLNMQUOTA, logical name table is full
$!!             Added "SHOW ERROR"
$!!             Added Ingres histroy file: ingres_installations.dat
$!!             Fixed the problem if UIC contains: [GROUP,ACCOUNT]
$!!                   This was failing: MC AUTHORIZE SHOW GROUP,ACCOUNT
$!!             Added PRODUCT SHOW HISTORY
$!!  
$!!  21-Oct-2010           Bilgihan Bircan          inglogs version 1.20
$!!             Added Mutex detection for recovery and Star Server
$!!             Added Star Server CLASSES will be detected
$!! 
$!! 
$!!  12-Nov-2010           Bilgihan Bircan          inglogs version 1.20.1
$!!            Change: 
$!!                     Removed all exception sorting out from [...,...] for vmsuic
$!!                     just do: mc authorize show [...], as it is
$!!             Added comment: increase account qouta Prclm to 50!
$!! 
$!!  07-Jun-2011           Bilgihan Bircan          inglogs version 1.40
$!!             Added HP Itanium VMS changes suggested by Andrew Sutherland
$!!                some local (:=) symbols have now been defined globally (:==)
$!!                use ucx instead of tcpip_services
$!!             Collecting some additional files
$!!                CKTMPL.DEF CC.COM DCC.COM UTCOM.DEF UTEXE.DEF
$!!             Added BUG 124993 -static and -dynamic options
$!!               -static  as synonym for -nosystemstate  -noingresstate
$!!               -dynamic as synonym for -nosystemconfig -nologfiles    -noingresconfig
$!!             Added listing of files in ii_system:[ingres.bin...] files...] library...] utility...]
$!!             Added some minor things keeping on track with inglogs on Linux version 1.32VI
$!!             Added option to force running iimonitor against iigcc iigcb GCD
$!!                +force_iimon
$!!                Due BUG 123270, below should be performed if only the fix has been included
$!!                Below Ingres versions and patches contain the fix
$!!                HP Itanium 
$!!                   II 9.2.2 (i64.vms/100)
$!!                   II 9.2.2 (i64.vms/101)
$!!                   II 9.2.3 (i64.vms/100)
$!!                HP Alpha
$!!                   II 9.2.1 (axm.vms/103) + p13953
$!!                   II 9.2.0 (axm.vms/143) + p13852
$!!                   II 9.1.2 (axm.vms/100) + p13786
$!!                   II 9.1.1 (axm.vms/104) + p13757
$!! 
$!!  11-Aug-2011           Kristoff Picard          inglogs version 1.41 
$!!             Added a -nostacks flag. This avoids calling "stacks" in iimonitor.
$!!             Use this flag on Open VMS Itanium, if you don't have the fix for
$!!             bug 125459. 
$!!             Removed spurious <CR>
$!!             
$!! ##############################################################################
$!
$ ON CONTROL_Y    THEN GOTO ERROR_CONTINUE
$ ON WARNING      THEN GOTO ERROR_CONTINUE
$ ON ERROR        THEN GOTO ERROR_CONTINUE
$ ON SEVERE_ERROR THEN GOTO ERROR_CONTINUE
$!
$ INGLOGS_VERSION     :== "INGLOGS Version: 1.41/20110811 axm.vms and i64.vms"
$ SAVED_MESSAGE_FORMAT == F$ENVIRONMENT( "MESSAGE" ) ! save message format
$ CURRENT_PID          == F$PID(CONTEXT)
$ CURRENT_DIR          == F$ENVIRONMENT("DEFAULT")
$ CURRENT_USER         == F$EXTRACT(0,F$LOCATE(" ", F$GETJPI("","USERNAME")),F$GETJPI("","USERNAME"))
$ CURRENT_PRCNAM       == F$GETJPI("","PRCNAM")
$ CURRENT_VERIFY       == F$VERIFY(0)
$ INGLOGS_START        == F$TIME()
$ ECHO                :== WRITE SYS$OUTPUT
$ SAY                 :== TYPE  SYS$INPUT
$ SPAWNDEF            :== SPAWN/NOWAIT/NOLOG/NONOTIFY
$ SPAWNWAIT           :== SPAWN  /WAIT/NOLOG/NONOTIFY
$!
$ IF ("" .EQS. F$TRNLNM("II_SYSTEM") )
$ THEN
$   ECHO "II_SYSTEM HAS NOT BEEN SET TO COLLECT INFO FROM INGRES ENVIRONMENT"
$   ECHO "EITHER LOGIN UNDER CORRECT ACCOUNT OR SET THE LOGICAL PROPERLY TO CONTINUE"
$   RETURN
$ ENDIF
$!
$ iigetres  :== $ii_system:[ingres.utility]iigetres.exe 
$ iimonitor :== $ii_system:[ingres.bin]iimonitor.exe 
$ iinamu    :== $ii_system:[ingres.bin]iinamu.exe
$!
$ NODE_NAME             = f$getsyi( "NODENAME" )
$ II_INSTALLATION_VALUE = f$trnlnm( "II_INSTALLATION" )
$!
$!######################################## PRESETTINGS
$!  All below can be user parameters
$ SET_HELP         := NO
$ SET__HELP        := NO
$ SET_VERSION      := NO
$ SET_LOGFILES     :=YES
$ SET_INGRESCONFIG :=YES
$ SET_INGRESSTATE  :=YES
$ SET_PARALLEL     :=YES
$ SET_SYSTEMCONFIG :=YES
$ SET_SYSTEMSTATE  :=YES
$ SET_FORCE_IIMON  := NO
$ SET_TIMEDIFF     := NO
$ SET_VERIFY       := NO
$ II_SYSTEM_SYSTEM := NO
$ SET_RUN_STACK    := YES
$!
$! ##########################  Internal setings
$ SLC              == 1  ! State Loop Count = SLC - Initialize the value
$ SET_MAXSNAPSHOT   = 2  ! in other words: MAXSLC 
$ SET_WAITCNT       =10  ! 10 seconds
$ SET_WAITMAX       =30  ! 30 seconds
$ SET_WAITSTATEMAX  =50  ! 50 seconds
$ IUSV_ON     :==NO
$ DBMS_ON     :==NO
$ STAR_ON     :==NO
$!
$ IUSV_LIST  :== ""
$ IUSV_COUNT   = 0
$ DBMS_LIST  :== ""
$ DBMS_COUNT   = 0
$ STAR_LIST  :== ""
$ STAR_COUNT   = 0
$!
$!######################################## READING p1 ... p8 parameters
$!
$ INGLOGS_PARAMS = ""
$ J = 1
$READ_NEXT_PARAM:
$! 
$ IF (J .LT. 9)
$ THEN
$   PP = F$EDIT(p'j', "TRIM, UPCASE")
$ ELSE
$   PP = ""
$ ENDIF
$!
$ IF (PP .NES. "")
$ THEN 
$   GOSUB VALIDATE_PARAMS 
$   J=J+1
$   GOTO READ_NEXT_PARAM
$ ENDIF
$!
$!######################################## INGLOGS PROCESSING WITH EXIT
$ IF (SET_HELP .EQS. "YES")
$ THEN
$   CALL INGLOGS_USAGE
$   GOTO INGLOGS_DONE
$ ENDIF
$!
$ IF (SET__HELP .EQS. "YES")
$ THEN
$   CALL INGLOGS_USAGE
$   CALL INGLOGS_USAGE_HELP
$   GOTO INGLOGS_DONE
$ ENDIF
$!
$ IF (SET_VERSION .EQS. "YES")
$ THEN
$   ECHO INGLOGS_VERSION
$   GOTO INGLOGS_DONE
$ ENDIF
$!
$!######################################## STICK PROCESS NAME
$ IF II_INSTALLATION_VALUE .EQS. "" 
$ THEN 
$   II_SYSTEM_SYSTEM := YES
$   SET_PROCNAME     := INGLOGS_00
$   ECHO "This is the production level installation WITH II_SYSTEM as SYSTEM LOGICAL"
$ ELSE
$   SET_PROCNAME := INGLOGS_'II_INSTALLATION_VALUE'
$   ECHO "This is the group level installation WITH II_INSTALLATION: ''II_INSTALLATION_VALUE'"
$ ENDIF
$!
$! STICK INGLOGS process name with II_INSTALLATION
$ SET MESSAGE/NOTEXT/NOSEV/NOFAC/NOID
$ SET PROCESS/NAME="''SET_PROCNAME'"
$ RETURN_CODE = $SEVERITY
$ SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$ IF RETURN_CODE .NE. 1 
$ THEN
$   SHOW PROC 'SET_PROCNAME'
$   ECHO "Currently another ''SET_PROCNAME' runs and has not been completed yet"
$   ECHO "Please restart inglogs later... Thank you :)"
$   GOTO INGLOGS_DONE
$ ENDIF
$! 
$!######################################## START INGLOGS PROCESSING ...
$ IF (SET_PARALLEL .EQS. "NO")
$ THEN
$   SPAWNDEF             := SPAWN/WAIT/NOLOG/NONOTIFY
$   ECHO "INGLOGS will perform each command sequentially, instead of parallel!"
$ ENDIF
$!
$ CALL CREATE_INGLOGS_DIR
$!
$ CALL SUB_SPAWN "WRITE SYS$OUTPUT ""''INGLOGS_VERSION'""" INGLOGS_VERSION.TXT "INGLOGS_VERSION"
$ IF (INGLOGS_PARAMS .NES. "")
$ THEN
$   CALL SUB_SPAWN "WRITE SYS$OUTPUT ""''INGLOGS_PARAMS'""" INGLOGS_PARAMS.TXT "INGLOGS_PARAMS"
$ ENDIF
$!
$ STATE_LOOP:
$ IF SET_SYSTEMSTATE .EQS. "YES"
$ THEN
$   ECHO "Collecting SYSTEM STATE: ''SLC'"
$   CALL SYSTEM_STATE 'SLC'
$   CALL SUB_SPAWN     "''SLC':tcpip_services NETSTAT -A"   NETSTAT_A_'SLC'.LOG "NETSTAT_A"
$ ENDIF
$!
$ IF SET_INGRESSTATE .EQS. "YES"
$ THEN
$   ECHO "Collecting INGRES STATE: ''SLC'"
$   GOSUB INGLOGS_IIGCN
$   IIMERGE :== IUSV
$   GOSUB INGLOGS_IIMERGE
$   IF IUSV_ON .EQS. "YES"
$   THEN
$     CALL SUB_SPAWNPIPE "lockstat"         LOCKSTAT_'SLC'.LOG        "lockstat"
$     CALL SUB_SPAWNPIPE "lockstat -dirty"  LOCKSTAT_DIRTY_'SLC'.LOG  "lockstat_dirty"
$     CALL SUB_SPAWNPIPE "logstat"          LOGSTAT_'SLC'.LOG         "logstat"
$     CALL SUB_SPAWNPIPE "logstat -verbose" LOGSTAT_VERBOSE_'SLC'.LOG "logstat_verbose"
$   ENDIF
$   IIMERGE :== DBMS
$   GOSUB INGLOGS_IIMERGE
$   IIMERGE :== STAR
$   GOSUB INGLOGS_IIMERGE
$!
$   COMPONENT :== II_GCC
$   GOSUB INGLOGS_COMPONENT
$   COMPONENT :== II_DASV
$   GOSUB INGLOGS_COMPONENT
$   COMPONENT :== II_GCB
$   GOSUB INGLOGS_COMPONENT
$!  Run logstat/lockstat if recovery process there!
$   ECHO "     Number of snap shots completed: ''SLC' of ''SET_MAXSNAPSHOT'"
$ ENDIF
$!
$ SET_WAITCNT = 10
$SUBPROCESS_STATELOOP:
$ SUBPROCNT = F$GETJPI("","PRCCNT")
$ IF SUBPROCNT .NE. 0
$ THEN
$   ECHO "SHOW SYSTEM/SUBPROCESS"
$   SHOW PROCESS/SUBPROCESS
$!
$   IF SET_WAITCNT .LE. SET_WAITSTATEMAX
$   THEN
$     ECHO "Waiting completion of subprocesses: State ''SLC' of ''SET_MAXSNAPSHOT'. " + -
             "''SET_WAITCNT' secs waiting, MAX=''SET_WAITSTATEMAX'..."
$     SET_WAITCNT=SET_WAITCNT+10
$     WAIT 00:00:10
$     GOTO SUBPROCESS_STATELOOP
$   ENDIF
$ ENDIF
$!
$ IF (SLC .LT. SET_MAXSNAPSHOT)
$ THEN 
$   SLC = SLC + 1
$   GOTO STATE_LOOP
$ ENDIF
$!
$ IF SET_SYSTEMCONFIG .EQS. "YES"
$ THEN
$   ECHO "Collecting SYSTEM CONFIGURATION"
$   CALL  SYSTEM_CONFIG
$   GOSUB INGRES_II_EXCEPTION
$ ENDIF
$!
$ IF SET_LOGFILES .EQS. "YES"
$ THEN
$   ECHO "Collecting INGRES LOG FILES"
$   CALL  INGRES_LOG 
$   GOSUB INGRES_DBMS_LOG
$ ENDIF
$!
$ IF SET_INGRESCONFIG .EQS. "YES"
$ THEN
$   ECHO "Collecting INGRES CONFIGURATION"
$   CALL INGRES_CONFIG 
$ ENDIF
$!
$ERROR_CONTINUE:
$! Check if all spawn/subprocesses are completed!
$ SET_WAITCNT = 10
$SUBPROCESS_LOOP:
$ SUBPROCNT = F$GETJPI("","PRCCNT")
$ IF SUBPROCNT .NE. 0
$ THEN
$   ECHO "SHOW SYSTEM/SUBPROCESS"
$   SHOW PROCESS/SUBPROCESS
$!
$   IF SET_WAITCNT .LE. SET_WAITMAX
$   THEN
$     IF SUBPROCNT .EQ. 1
$     THEN
$       ECHO "There is  still ''SUBPROCNT' subprocess   running. " + -
             "''SET_WAITCNT' secs waiting, MAX=''SET_WAITMAX' secs!"
$     ELSE
$       ECHO "There are still ''SUBPROCNT' subprocesses running. " + -
             "''SET_WAITCNT' secs waiting, MAX=''SET_WAITMAX' secs!"
$     ENDIF
$     SET_WAITCNT=SET_WAITCNT+10
$     WAIT 00:00:10
$     GOTO SUBPROCESS_LOOP
$   ENDIF
$   ECHO "inglogs has been completed, but some subprocesses may be still running"
$   ECHO "Ensure all subprocesses have been completed before running next inglogs script"
$ ELSE
$   ECHO "All subprocesses have been completed successfully"
$ ENDIF
$!
$ ECHO "Use the command to zip collected info, if necessary:"
$ ECHO "   ZIP INGLOGS_''DATE_TIME_DIR'.ZIP [.INGLOGS_''DATE_TIME_DIR']*.*"
$!
$INGLOGS_DONE:
$ INGLOGS_END           = F$TIME()
$ IF SET_TIMEDIFF .EQS. "YES"
$ THEN
$   ECHO "INGLOGS took to complete: " + F$DELTA_TIME(INGLOGS_START,INGLOGS_END)
$ ENDIF
$! Ensure having same environment before exiting DCL procedure
$ IF SET_VERIFY .EQS. "YES" THEN SET NOVERIFY
$ SET PROCESS/NAME='CURRENT_PRCNAM'
$ SET DEFAULT 'CURRENT_DIR'
$ SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$!
$ EXIT
$!#############################################################################
$!################################################# END OF MAIN PROGRAM #######
$!#############################################################################
$!
$!######################################## GOSUB BEGIN VALIDATE_PARAMS
$VALIDATE_PARAMS:
$!
$ N = 0  ! 0 for unknown parameter. In such case display error
$!
$!SAY "INGLOGS Parameter: ''PP'"
$ IF (PP .EQS. "-V") .OR. (PP .EQS. "-VERSION") .OR. (PP .EQS. "--VERSION") 
$ THEN
$   SET_VERSION:=YES
$   N = 1
$ ENDIF
$ IF (PP .EQS. "-H") .OR. (PP .EQS. "-HELP") 
$ THEN
$   SET_HELP:=YES
$   N = 1
$ ENDIF
$ IF (PP .EQS. "--HELP") 
$ THEN
$   SET__HELP:=YES
$   N = 1
$ ENDIF
$ IF (PP .EQS. "-NOINGRESCONFIG")
$ THEN
$   SET_INGRESCONFIG:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-NOINGRESSTATE")
$ THEN
$   SET_INGRESSTATE:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-NOLOGFILES")
$ THEN
$   SET_LOGFILES:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-NOPARALLEL")
$ THEN
$   SET_PARALLEL:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-NOSYSTEMCONFIG")
$ THEN
$   SET_SYSTEMCONFIG:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-NOSYSTEMSTATE")
$ THEN
$   SET_SYSTEMSTATE:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-STATIC")
$ THEN
$!   synonym for -nosystemstate -noingresstate
$   SET_SYSTEMSTATE:=NO
$   SET_INGRESSTATE:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-DYNAMIC")
$ THEN
$!  synonym for -nosystemconfig -nologfiles -noingresconfig
$   SET_SYSTEMCONFIG:=NO
$   SET_LOGFILES:=NO
$   SET_INGRESCONFIG:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "+FORCE_IIMON")
$ THEN
$   SET_FORCE_IIMON:=YES
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-NOSTACKS")
$ THEN
$   SET_RUN_STACK:=NO
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-T")
$ THEN
$   SET_TIMEDIFF:=YES
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (PP .EQS. "-VERIFY")
$ THEN
$   SET VERIFY
$   SET_VERIFY := YES
$   N = 1
$   INGLOGS_PARAMS = INGLOGS_PARAMS  + " " + PP
$ ENDIF
$ IF (N .EQ. 0)
$ THEN
$   SET_HELP:=YES
$   ECHO "INGLOGS parameter UNKNOWN : ''PP'"
$ ENDIF
$!
$ RETURN
$!######################################## GOSUB END   VALIDATE_PARAMS
$!
$! ********************************************************** INGLOGS_USAGE
$ INGLOGS_USAGE: SUBROUTINE
$!
$ type sys$input
Usage: inglogs [-nosystemconfig] [-nosystemstate] 
               [-noingresconfig] [-noingresstate] [-nologfiles]     
               [-static]
               [-dynamic]
               [-noparallel]
               [+force_iimon]
               [-nostacks]
       inglogs [-h|-help]
       inglogs [--help]
       inglogs [-v|-version|--version]
$!
$ EXIT
$ ENDSUBROUTINE INGLOGS_USAGE
$!
$! ********************************************************** INGLOGS_USAGE_HELP
$ INGLOGS_USAGE_HELP: SUBROUTINE
$!
$ type sys$input
The inglogs script collects some information from your system which
might be helpful for analysis. By default it collects as much information
as possible, but you can switch off some "modules" if wanted.
By default, most of the commands are running in the background, i.e in
parallel. This is especially useful in cases where a particular command
hangs - otherwise it would block all following commands.

The script will waits 30 seconds, till all subprocesses have been completed.
If there are still running subprocesses, it will complete and give warning, 
which subprocesses are still running.
 
Output is collected in directory inglogs_yyyymmdd_hhmmss which is by 
default created in the current working directory. Alternately define logical INGLOGSDIR, 
to specify another directory where inglogs creates its output directory

The following information(at least) is collected
module systemconfig:
  user authorization and quotas
  installed images
  sysgen quotas
  symbols and logicals
  Cluster
  system errors
  installed product

module systemstate:
  Process info
  CPU info
  Disk info
  Open files
  netstat
 
module ingresconfig:
  infodb
  syscheck
  config.dat config.log protect.dat
  version.rel version.dat
  cktmpl.def
  cc.com dcc.com utcom.def utexe.def
  dir /size/owner/date=(c,m)  bin...] files...] library...] utility...]
  iiexcept.opt

module ingreslogfiles:
  errlog.log iircp.log iiacp.log
  II_DBMS_LOG
  iivdb.log
  Directory for collected exception info

module ingresstate:
  iinamu
  lockstat, lockstat -dirty
  logstat , logstat  -verbose
  iimonitor: show all sessions plus, show all sessions formatted plus
  iimonitor: show mutex x
  iimonitor: stacks
  Full process info for each Ingres server

Flags:
-no<modulename>
  (-nosystemconfig -nosystemstate -nologfiles -noingresconfig -noingresstate)
  By default all the above modules are executed, you can switch off
  one or multiple modules by using -no<modulename> ... 

-static
  synonym for -nosystemstate -noingresstate

-dynamic
  synonym for -nosystemconfig -nologfiles -noingresconfig

-noparallel
  Disables the parallel mode, i.e. no process is started in the background

-nostacks
  Don't run the "stacks" command in iimonitor as this command is dangerous on 
  OpenVMS HP Itanium without the fix for bug 125459.

+force_iimon
  If fix for BUG 123270 has been included in current release, force running 
  iimonitor against iigcc iigcb iigcd process
  Below Ingres versions and patches already contain the fix
       OpenVMS HP Itanium
                  II 9.2.2 (i64.vms/100)
                  II 9.2.2 (i64.vms/101)
                  II 9.2.3 (i64.vms/100)
       OpenVMS HP Alpha
                  II 9.2.1 (axm.vms/103) + p13953
                  II 9.2.0 (axm.vms/143) + p13852
                  II 9.1.2 (axm.vms/100) + p13786
                  II 9.1.1 (axm.vms/104) + p13757
       Any higher patch numbers will contain the fix either

-help (-h)
  Prints out a short usage message and exits

--help
  Prints out this message and exits

--version (-v, -version)
  Prints out the version of inglogs and exits
$!
$ EXIT
$ ENDSUBROUTINE INGLOGS_USAGE_HELP
$!
$! ********************************************************** SYSTEM_CONFIG
$ SYSTEM_CONFIG: SUBROUTINE
$ ON ERROR     THEN CONTINUE
$ ECHO "           Account, SYSGEN, Logicals info"
$!                                          Getting Installation owner
$   PIPE IIGETRES "ii.''node_name'.*.*.vms_uic" | search SYS$PIPE "" /OUT=VMS_UIC.LOG
$   OPEN IN VMS_UIC.LOG
$   READ/END_OF_FILE=ENDIT IN VMS_UIC
$!
$ENDIT:
$   CLOSE IN
$   DEL/NOCONFIRM VMS_UIC.LOG.
$! Before running 'MCR AUTHORIZE' check if autorize file is accessable
$   IF ("" .EQS. F$TRNLNM("SYSUAF") )
$   THEN
$     DEFINE /JOB/USER SYSUAF SYS$SYSTEM:SYSUAF.DAT
$     CALL SUB_SPAWNWAIT "MCR AUTHORIZE SHOW ''VMS_UIC' /full" SYSTEM_AUTHORIZE.LOG "MC_AUTHORIZE"
$     DEASSIGN /JOB/USER SYSUAF 
$   ELSE
$     CALL SUB_SPAWN "MCR AUTHORIZE SHOW ''VMS_UIC' /full" SYSTEM_AUTHORIZE.LOG "MC_AUTHORIZE"
$   ENDIF
$!
$   CALL SUB_SPAWN "INSTALL LIST"               SYSTEM_INSTALL_LIST.LOG "INSTALL_LIST"
$   CALL SUB_SPAWN "MCR SYSGEN SHOW /ALL"       SYSTEM_SYSGEN.LOG       "MC_SYSGEN"
$   CALL SUB_SPAWN "MCR SYSGEN SHOW CHANNELCNT" SYSTEM_CHANNELCNT.LOG   "MC_CHANNELCNT"
$   CALL SUB_SPAWN "SHOW SYMBOL *"              SYSTEM_SYMBOLS.LOG      "SYSTEM_SYMBOLS"
$   CALL SUB_SPAWN "SHOW LOGICAL * /FULL"       SYSTEM_LOGICALS.LOG     "SYSTEM_LOGICALS"
$   CALL SUB_SPAWN "SHOW CLUSTER"               SYSTEM_CLUSTER.LOG      "SYSTEM_CLUSTER"
$   CALL SUB_SPAWN "SHOW ERROR"                 SYSTEM_SHOW_ERROR.LOG   "SHOW_ERROR"
$   CALL SUB_SPAWN "PRODUCT SHOW HISTORY"       SYSTEM_PROD_HISTORY.LOG "PROD_HISTORY"
$!
$! This is the user who performs the inglogs script. 
$! It needs to be performed within same process and not as a subprocess
$   ECHO "           VMS Process info for INGLOGS process: SYSTEM_CURPROCESS.LOG"
$   SHOW PROCESS/ALL/OUT=SYSTEM_CURPROCESS.LOG
$   ECHO "           Product Installation History File: vmsinstal.history"
$   ECHO "           Ingres  Installation History File: sys$common:[sysexe]ingres_installations.dat"
$ SET MESSAGE/NOTEXT/NOSEV/NOFAC/NOID
$   BACKUP /NOLOG/IGNORE=INTERLOCK sys$update:vmsinstal.history;0 *
$   BACKUP /NOLOG/IGNORE=INTERLOCK sys$common:[sysexe]ingres_installations.dat;0 *
$ SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$!
$ EXIT
$ ENDSUBROUTINE SYSTEM_CONFIG
$!
$! ********************************************************** SYSTEM_STATE
$ SYSTEM_STATE: SUBROUTINE
$ ON ERROR     THEN CONTINUE
$ CNT = P1
$ ECHO "           Process, CPU, Memory, File, Device, netstat info: ''CNT'"
$!
$   CALL SUB_SPAWN "SHOW SYSTEM /FULL"        SYSTEM_PROCESSES_'SLC'.LOG   "SYSTEM_PROCESSES"
$   CALL SUB_SPAWN "SHOW CPU    /FULL"        SYSTEM_CPU_'SLC'.LOG         "SYSTEM_CPU"
$   CALL SUB_SPAWN "SHOW MEMORY /FULL"        SYSTEM_MEMORY_'SLC'.LOG      "SYSTEM_MEMORY"
$   CALL SUB_SPAWN "SHOW DEV D  /FULL"        SYSTEM_DEV_D_FULL_'SLC'.LOG  "SYSTEM_DEV_D"
$   CALL SUB_SPAWN "SHOW DEV II_SYSTEM /FULL" SYSTEM_DEV_II_FULL_'SLC'.LOG "SYSTEM_DEV_II_FULL"
$   CALL SUB_SPAWN "SHOW DEV II_SYSTEM /FILE" SYSTEM_DEV_II_FILE_'SLC'.LOG "SYSTEM_DEV_II_FILE"
$!
$ EXIT
$ ENDSUBROUTINE SYSTEM_STATE
$!
$! ********************************************************** INGRES_CONFIG
$ INGRES_CONFIG: SUBROUTINE
$ ON ERROR     THEN CONTINUE
$!
$ IF DBMS_ON .EQS. "YES"
$ THEN
$   CALL SUB_SPAWN "infodb"   INFODB.LOG   "INFODB"
$   CALL SUB_SPAWN "SYSCHECK" SYSCHECK.LOG "SYSCHECK"
$ ENDIF
$!
$ ECHO "           config.dat protect.dat config.log"
$ SET MESSAGE/NOTEXT/NOSEV/NOFAC/NOID
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]CONFIG.DAT;0  *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]CONFIG.LOG;0  *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]PROTECT.DAT;0 *
$! Ifnore if below files are not available.
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES]VERSION.REL;0       *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES]VERSION.DAT;0       *
$ ECHO "           CC.COM DCC.COM CKTMPL.DEF UTCOM.DEF UTEXE.DEF"
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]CC.COM;0      *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]DCC.COM;0     *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]CKTMLP.DEF;0  *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_CKTMPL_FILE;0                      *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]UTCOM.DEF;0   *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]UTEXE.DEF;0   *
$ ECHO "           Listing the files: bin files library utility in dir-files.log"
$ dir /size/owner/date=(c,m) ii_system:[ingres.bin...],          -
   ii_system:[ingres.files...], ii_system:[ingres.library...],   -
   ii_system:[ingres.utility...] /out=dir-files.log
$ SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$!
$ EXIT
$ ENDSUBROUTINE INGRES_CONFIG
$!
$! ********************************************************** INGRES_LOG
$ INGRES_LOG: SUBROUTINE
$   ON ERROR     THEN CONTINUE
$   ECHO "           errlog.log iircp.log iiacp.log files"
$   SET MESSAGE/NOTEXT/NOSEV/NOFAC/NOID
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]ERRLOG.LOG;0  *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]IIRCP.LOG;0   *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]IIACP.LOG;0   *
$   BACKUP /NOLOG/IGNORE=INTERLOCK II_SYSTEM:[INGRES.FILES]IIVDB.LOG;0   *
$!
$  SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$ EXIT
$ ENDSUBROUTINE INGRES_LOG
$!
$! ********************************************************** SUB_SPAWN
$ SUB_SPAWN: SUBROUTINE
$ ON ERROR     THEN CONTINUE
$!
$! Process name has been limited upto 15 characters on VMS. Cut off the rest!
$! Otherwise you will see: %SYSTEM-F-IVLOGNAM, invalid logical name
$ PROCNAME = F$EXTRACT(0,15,P3)
$!
$ ECHO "           ''P3' is being processed into ''P2'"
$ SPAWNDEF /OUT='P2'  'P1' 
$!
$ EXIT
$ ENDSUBROUTINE SUB_SPAWN
$!
$! ********************************************************** SUB_SPAWNPIPE
$ SUB_SPAWNPIPE: SUBROUTINE
$ ON ERROR     THEN CONTINUE
$! SUB_SPAWNPIPE is the same like SUB_SPAWN, except it uses 'PIPE' mimic
$! so required output can be completed faster
$!
$! Process name has been limited upto 15 characters on VMS. Cut the rest!
$! Otherwise it appears: %SYSTEM-F-IVLOGNAM, invalid logical name
$ PROCNAME = F$EXTRACT(0,15,P3)
$!
$ ECHO "           ''P3' is being processed into ''P2'"
$ SPAWNDEF /OUT='P2'  PIPE 'P1' | search/mat=or sys$pipe ""
$!
$ EXIT
$ ENDSUBROUTINE SUB_SPAWNPIPE
$!
$! ********************************************************** SUB_SPAWNWAIT
$ SUB_SPAWNWAIT: SUBROUTINE
$ ON ERROR     THEN CONTINUE
$!
$! Process name has been limited upto 15 characters on VMS. Cut off the rest!
$! Otherwise you will see: %SYSTEM-F-IVLOGNAM, invalid logical name
$ PROCNAME = F$EXTRACT(0,15,P3)
$!
$ ECHO "           ''P3' is being processed into ''P2'"
$ SPAWNWAIT /OUT='P2'  'P1' 
$!
$ EXIT
$ ENDSUBROUTINE SUB_SPAWNWAIT
$!
$! ********************************************************** GET_PROCESS_INFO
$ GET_PROCESS_INFO: SUBROUTINE
$! Even this info is related to SYSTEMSTATE, it will be caught up!
$! Detecting Ingres processes is a complex exercise,
$!   process info will be collected only if INGRESSTATE will be in place.
$! 
$! 
$ ON ERROR     THEN CONTINUE
$ ECHO "           VMS Process info        : PROCESS_''P1'_''SLC'.log"
$    SHOW PROCESS/ALL /OUT=PROCESS_'P1'_'SLC'.log /ID='P2'
$ EXIT
$ ENDSUBROUTINE GET_PROCESS_INFO
$!
$! ********************************************************** CHECK_MUTEX
$ CHECK_MUTEX: SUBROUTINE
$ ON ERROR        THEN CONTINUE
$! P1  = IIMERGE              P2  = PROCNAME
$!
$ MUTEX_LIST_FILE = "MUTEX_" + P2 + "_" + "''SLC'" + ".TMP"
$!
$ SEARCH/NOWARN/NOHEAD /OUT='MUTEX_LIST_FILE' IIMON_FMT_'P2'_'SLC'.LOG CS_MUTEX 
$ OPEN/READ MUTEX_STREAM 'MUTEX_LIST_FILE'
$ ON ERROR THEN GOTO MUTEX_EOF
$ MUTEX_IDS = "."
$MUTEX_LOOP:
$!  If search has no match that means zero block size, i.e. No mutex
$ IF (F$FILE_ATTRIBUTE(MUTEX_LIST_FILE,"EOF") .EQ. 0) 
$ THEN 
$   ECHO "           NO MUTEX detected ..."
$   IF F$FILE_ATTRIBUTES("''MUTEX_LIST_FILE'","EOF") .EQ. 0 THEN -
                        DELETE /NOCONFIRM 'MUTEX_LIST_FILE';*
$   GOTO MUTEX_EOF
$ ENDIF
$!
$ READ/END_OF_FILE = MUTEX_EOF MUTEX_STREAM DATA_LINE
$ POS = F$LOCATE("CS_MUTEX",DATA_LINE)
$ IF (POS .EQ. F$LENGTH(DATA_LINE)) THEN GOTO MUTEX_LOOP
$ MUTEX_ID = F$EXTRACT(POS+14,8,DATA_LINE)
$ IF F$LOCATE(MUTEX_ID, MUTEX_IDS) .NE. F$LENGTH(MUTEX_IDS) -
                                    THEN GOTO MUTEX_LOOP
$ IF MUTEX_ID .EQS. ""              THEN GOTO MUTEX_EOF
$ MUTEX_IDS = "''MUTEX_IDS'''MUTEX_ID'."
$ ECHO "           ''P1': SHOW MUTEX ''MUTEX_ID': MUTEXES_''P2'_''SLC'_''MUTEX_ID'.LOG"
$ SPAWNDEF/OUT=MUTEXES_'P2'_'SLC'_'MUTEX_ID'.LOG    PIPE ECHO "SHOW MUTEX ''MUTEX_ID'" -
                                                      | IIMONITOR 'P2'
$ GOTO MUTEX_LOOP
$!
$MUTEX_EOF:
$ ON ERROR THEN CONTINUE
$ CLOSE MUTEX_STREAM
$! DELETE /NOCONF 'MUTEX_LIST_FILE';*
$END_MUTEX:
$!
$ EXIT
$ ENDSUBROUTINE CHECK_MUTEX
$!
$! ********************************************************** CREATE_INGLOGS_DIR
$ CREATE_INGLOGS_DIR: SUBROUTINE
$!                    Setting directory name with current date+time stamp
$ ON ERROR        THEN CONTINUE
$!
$ timestamp = f$time()
$ DATE_TIME_DIR == f$cvtime(timestamp,,"YEAR")   + f$cvtime(timestamp,,"MONTH") + -
              f$cvtime(timestamp,,"DAY") + "_"  + f$cvtime(timestamp,,"HOUR")  + -
                  f$cvtime(timestamp,,"MINUTE") + f$cvtime(timestamp,,"SECOND")
$! CHECK IF INGLOGSDIR LOGICALS has been set.
$ INGLOGSDIR_VALUE == f$trnlnm( "INGLOGSDIR" )
$ IF (INGLOGSDIR_VALUE .NES. "")
$ THEN 
$   SET DEFAULT 'INGLOGSDIR_VALUE'
$ ENDIF
$ CREATE /DIR [.INGLOGS_'DATE_TIME_DIR']
$! Change to directory so all files can be created there
$ SET DEFAULT [.INGLOGS_'DATE_TIME_DIR']
$ CURRENT_INGLOGS_DIR == F$ENVIRONMENT("DEFAULT")
$ ECHO "Working directory: ''CURRENT_INGLOGS_DIR'"
$!
$ EXIT
$ ENDSUBROUTINE CREATE_INGLOGS_DIR
$!
$! *************************************************** GOSUB INGRES_DBMS_LOG
$ INGRES_DBMS_LOG:
$   ON ERROR     THEN CONTINUE
$!Notice: ingstart accepts only if II_DBMS_LOG has been set at /SYSTEM level
$!
$ II_DBMS_LOG_FILE = F$EDIT( F$TRNLNM("II_DBMS_LOG"), "TRIM, UPCASE")
$ IF ( II_DBMS_LOG_FILE .EQS. "")
$ THEN
$   RETURN
$ ENDIF
$!
$ ECHO "           II_DBMS_LOG file(s): ''II_DBMS_LOG_FILE'"
$! In case '%P' has been used in logical, '%P' will be replaced by '*' for backup command
$! If it does not contain any '%P', it will use exact the same 
$ II_DBMS_LOG_PERCENTP = F$EXTRACT(0,F$LOCATE("%P", II_DBMS_LOG_FILE), II_DBMS_LOG_FILE)
$!
$ SET MESSAGE/NOTEXT/NOSEV/NOFAC/NOID
$!
$ IF II_DBMS_LOG_FILE .EQS. II_DBMS_LOG_PERCENTP
$ THEN
$   BACKUP /NOLOG/IGNORE=INTERLOCK 'II_DBMS_LOG_FILE';0 *
$ ELSE
$   BACKUP /NOLOG/IGNORE=INTERLOCK 'II_DBMS_LOG_PERCENTP'*.*;0 *
$ ENDIF
$!
$  SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$!
$RETURN
$! END GOSUB INGRES_DBMS_LOG
$!
$! *************************************************** GOSUB INGRES_II_EXCEPTION
$ INGRES_II_EXCEPTION:
$   ON ERROR     THEN CONTINUE
$!Notice: ingstart accepts only if II_DBMS_LOG has been set at /SYSTEM level
$!
$ II_EXCEPTION_DIR = F$EDIT( F$TRNLNM("II_EXCEPTION"), "TRIM, UPCASE")
$ IF ( II_EXCEPTION_DIR .EQS. "")
$ THEN
$   RETURN
$ ENDIF
$!
$ ECHO "           ''II_EXCEPTION_DIR'[ingres.files]iiexcept.opt is being used"
$ SET MESSAGE/NOTEXT/NOSEV/NOFAC/NOID
$ BACKUP /NOLOG/IGNORE=INTERLOCK 'II_EXCEPTION_DIR'[ingres.files]iiexcept.opt;0 *
$ SET MESSAGE 'SAVED_MESSAGE_FORMAT'
$!
$ ECHO "           List of exceptions at: ''II_EXCEPTION_DIR'[ingres.except]"
$ DIR/SIZE/DATE=(c,m) 'II_EXCEPTION_DIR'[ingres.except]
$!
$RETURN
$! END GOSUB INGRES_II_EXCEPTION
$!
$! *************************************************** GOSUB INGLOGS_IIGCN
$ INGLOGS_IIGCN: 
$ ON ERROR     THEN CONTINUE
$!
$ IF (II_INSTALLATION_VALUE .NES. "")
$ THEN
$    DUMMY = F$CONTEXT("PROCESS", ctx, "PRCNAM", -
        "II_GCN_''II_INSTALLATION_VALUE'", "EQL")
$ ELSE
$    DUMMY = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_GCN", "EQL")
$ ENDIF
$!
$ IIGCN_LOOP:
$ PID = F$PID( ctx )
$!      Get the process-id from the context
$ IF (PID .NES. "")
$ THEN
$   IIGCN_PROCNAME = F$GETJPI( PID, "PRCNAM" )
$   CALL GET_PROCESS_INFO 'IIGCN_PROCNAME' 'PID'
$   ECHO "           iinamu> SHOW SERVERS    : IINAMU_''SLC'.LOG"
$   SPAWNDEF/OUT=IINAMU_'SLC'.LOG PIPE ECHO "SHOW SERVERS" | iinamu
$   GOTO IIGCN_LOOP
$ ENDIF
$!
$RETURN
$! END GOSUB INGLOGS_IIGCN
$!
$! *************************************************** GOSUB INGLOGS_IIMERGE
$ INGLOGS_IIMERGE: 
$ ON WARNING      THEN GOTO CONTINUE
$ ON ERROR        THEN GOTO CONTINUE
$ ON SEVERE_ERROR THEN GOTO CONTINUE
$!
$ CLASS_NO = 0
$!
$ IF IIMERGE .EQS. "IUSV"
$ THEN 
$   CLASS_LIST = "IUSV"
$   CLASS      = "IUSV"
$ ENDIF
$ IF IIMERGE .EQS. "DBMS" THEN CLASS_LIST = "INGRES"
$ IF IIMERGE .EQS. "STAR" THEN CLASS_LIST = "STAR"
$!
$ IF IIMERGE .EQS. "IUSV" THEN GOTO DETECT_IIMERGE_PROC
$!
$! Determine the server classes
$ CLASSES_TMP := "''IIMERGE'_CLASSES.TMP"
$ SEARCH/NOWARN/OUT='CLASSES_TMP'/MATCH=AND II_CONFIG:CONFIG.DAT 'IIMERGE',server_class
$!
$ OPEN/READ/ERROR=CLASSES_FIN CLASSES 'CLASSES_TMP'
$!
$CLASSES_LOOP:
$ READ/END_OF_FILE=CLASSES_FIN /ERROR=CLASSES_FIN CLASSES CLASS
$!
$ NEW_CLASS = F$EDIT(F$ELEMENT(1, ":", CLASS),"COMPRESS,TRIM,UPCASE")
$!
$ IF F$LOCATE(NEW_CLASS,CLASS_LIST) .NE. F$LENGHT(CLASS_LIST) THEN GOTO CLASSES_LOOP
$!
$ CLASS_LIST = F$FAO("!AS,!AS",CLASS_LIST, NEW_CLASS)
$!
$ GOTO CLASSES_LOOP
$!
$CLASSES_FIN:
$ CLOSE CLASSES
$!
$ DELETE/NOCONFIRM 'CLASSES_TMP';*
$!
$IIMERGE_CLASS_LOOP:
$!
$ CLASS = F$ELEMENT(CLASS_NO, ",", CLASS_LIST)
$!
$ CLASS_NO = CLASS_NO + 1
$!
$ IF CLASS .EQS. "," THEN GOTO END_IIMERGE_CLASS_LOOP
$!
$ IF CLASS .EQS. "INGRES" THEN CLASS = "DBMS"
$!
$ IF F$LENGTH(CLASS) .GT. 4 THEN CLASS = F$EXTRACT(0, 4, CLASS)
$!
$DETECT_IIMERGE_PROC:
$ IF (II_INSTALLATION_VALUE .NES. "")
$ THEN
$    DUMMY = F$CONTEXT( "PROCESS", ctx, "PRCNAM", -
        "II_''CLASS'_''II_INSTALLATION_VALUE'*", "EQL" )
$ ELSE
$    DUMMY = F$CONTEXT( "PROCESS", ctx, "PRCNAM", "II_''CLASS'_*", "EQL" )
$ ENDIF
$!
$ IIMERGE_LOOP:
$ PID = F$PID( ctx )
$!      Get the process-id from the context
$ IF (PID .NES. "")
$ THEN 
$   IIMERGE_PROCNAME = F$GETJPI( PID, "PRCNAM" )
$!
$   'IIMERGE'_ON    :== "YES"
$   'IIMERGE'_LIST  :== 'IIMERGE_PROCNAME' + " " + 'IIMERGE_LIST' 
$   'IIMERGE'_COUNT   = 'IIMERGE'_COUNT + 1
$!
$!  For system wide installation 3rd element will return a '_'
$!      In this case ignore the processes at group level
$   IF (F$ELEMENT( 3, "_", IIMERGE_PROCNAME ) .NES. "_" .AND. -
        II_INSTALLATION_VALUE .EQS. "")                    THEN GOTO IIMERGE_LOOP
$   CALL GET_PROCESS_INFO 'IIMERGE_PROCNAME' 'PID'
$!
$   ECHO "           ''IIMERGE': SHOW SERVER       : IIMON_SERVER_''IIMERGE_PROCNAME'_''SLC'.LOG"
$   SPAWNDEF/OUT=IIMON_SERVER_'IIMERGE_PROCNAME'_'SLC'.LOG PIPE ECHO "SHOW SERVER"                 -
                                                      | IIMONITOR 'IIMERGE_PROCNAME'
$   ECHO "           ''IIMERGE': SHOW ALL SESSIONS+: IIMON_SES_''IIMERGE_PROCNAME'_''SLC'.LOG"
$   SPAWNDEF/OUT=IIMON_SES_'IIMERGE_PROCNAME'_'SLC'.LOG    PIPE ECHO "SHOW ALL SESSIONS PLUS"      -
                                                      | IIMONITOR 'IIMERGE_PROCNAME'
$   IF (SET_RUN_STACK  .EQS. "YES")
$   THEN
$     ECHO "           ''IIMERGE': STACKS            : IIMON_STACKS_''IIMERGE_PROCNAME'_''SLC'.LOG"
$     SPAWNDEF/OUT=IIMON_STACKS_'IIMERGE_PROCNAME'_'SLC'.LOG    PIPE ECHO "STACKS"           -
                                                      | IIMONITOR 'IIMERGE_PROCNAME'
$   ENDIF
$   ECHO "           ''IIMERGE': SHOW .. FORMATTED+: IIMON_FMT_''IIMERGE_PROCNAME'_''SLC'.LOG"
$!  RUN this script intentionally sequential as search needs the contents 
$   SPAWNWAIT/OUT=IIMON_FMT_'IIMERGE_PROCNAME'_'SLC'.LOG    PIPE ECHO "SHOW ALL SESSIONS FORMATTED PLUS" -
                                                      | IIMONITOR 'IIMERGE_PROCNAME'
$!
$   CALL CHECK_MUTEX 'IIMERGE 'IIMERGE_PROCNAME
$!
$   GOTO IIMERGE_LOOP
$ ENDIF
$ IF IIMERGE .NES. "IUSV" THEN GOTO IIMERGE_CLASS_LOOP
$!
$END_IIMERGE_CLASS_LOOP:
$!
$RETURN
$! END GOSUB INGLOGS_IIMERGE
$!
$! *************************************************** GOSUB INGLOGS_COMPONENT
$ INGLOGS_COMPONENT:
$ ON ERROR     THEN CONTINUE
$!
$ IF II_INSTALLATION_VALUE .NES. ""
$ THEN
$    DUMMY = F$CONTEXT( "PROCESS", CTX, "PRCNAM", -
        "''COMPONENT'_''II_INSTALLATION_VALUE'*", "EQL" )
$ ELSE
$    DUMMY = F$CONTEXT( "PROCESS", CTX, "PRCNAM", "''COMPONENT'_*", "EQL" )
$ ENDIF
$!
$COMPONENT_LOOP:
$ PID = F$PID( CTX )
$!      Get the process-id from the context
$ IF (PID .NES. "")
$ THEN 
$   COMPONENT_PROCNAME = F$GETJPI( PID, "PRCNAM" )
$!  For system wide installation 3rd element will return a '_'
$!      In this case ignore the processes at group level
$   IF (F$ELEMENT( 3, "_", COMPONENT_PROCNAME ) .NES. "_" .AND. -
        II_INSTALLATION_VALUE .EQS. "")   THEN GOTO COMPONENT_LOOP
$   CALL GET_PROCESS_INFO 'COMPONENT_PROCNAME' 'PID'
$!Due BUG 123270, below should be performed if only the fix has been included
$   IF (SET_FORCE_IIMON  .EQS. "YES")
$   THEN 
$     SHOW_ALL_FILE := "IIMON_FMT_''COMPONENT_PROCNAME'_''SLC'.LOG"
$     ECHO "           ''COMPONENT_PROCNAME' in file ''SHOW_ALL_FILE'"
$     SPAWNDEF/OUT='SHOW_ALL_FILE'  PIPE ECHO "SHOW ALL SESSIONS FORMATTED" -
                                                    | IIMONITOR 'COMPONENT_PROCNAME'
$   ENDIF
$   GOTO COMPONENT_LOOP
$ ENDIF 
$!
$RETURN
$! END GOSUB INGLOGS_COMPONENT
$!
