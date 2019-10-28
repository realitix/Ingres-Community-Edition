#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <pc.h>
#include <nm.h>
#include <cv.h>
#include <cs.h>
#include <ex.h>
#include <evset.h>
# define Factab iiFactab
#include <erfac.h>
#include <tr.h>

/*
**      Copyright (c) 1996, Actian Corporation
**
**  Name: EXdump - Dumping routines
**
**  Description:
**      This module contains EXdump a routine for causing a dump of the current
**      process and EXdumpInit a routine for initialising it
**
**  History:
**	20-mar-96 -- File branched from UNIX.  Code still to be written.
**	16-jul-98 (mcgem01)
**	    Add a stub for the EXdumpInit function.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      28-Jan-2010 (horda03) Bug 121811
**         Ported Unix code to Windows.
**      12-Jan-2011 (horda03) b124923
**          Ex_diag_link now returns 0 if Server doesn't support
**          queries.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/

#define MAP_SIZE 100
static STATUS DIAGhandler( EX_ARGS *exargs);
VOID DIAGoutput();
VOID DIAGtr_output();
VOID DIAGerror();
static i4 to_hex(PTR);
static FILE *summary_file=NULL;
static char iiexcept_opt[MAX_LOC+1] = "iiexcept.opt";
static char iidbms[MAX_LOC+1] = "iidbms";
GLOBALDEF i4    (*Ex_diag_link)();     /*NULL or CSdiagDumpSched */
GLOBALREF Version[];

static struct IIERRMAP
{
    i4  valid;
#define EXDUMP_INIT CV_C_CONST_MACRO('E','X','O','K')
    i4  map_size;
    struct IIMAP
    {
       u_i4 from;
       u_i4 to;
    } map[MAP_SIZE];
} errmap={0};

/*
** Full path name of problem management server
**
** This is initialised to iigcore (assuming we can find it on the path). If
** EXdumpInit is called then it is replaced with the full pathname (avoiding
** the above assumption)
*/

/*
*/ 

static VOID
do_stats(evset,name,type)
i4      evset;
char    *name;
i4      type;
{
    i4  status;
    char filename[EVSET_MAX_PATH];
    EX_CONTEXT ex;

    status=EVSetCreateFile(evset,EVSET_TEXT|EVSET_SYSTEM,name,filename,sizeof(filename));
    DIAGoutput
    ("----------------------- %s -----------------------\n",name);

    if (status!=OK)
    {
        TRdisplay("Cannot get stats for %s\n",name);
        return;
    }

    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        Ex_diag_link(type,DIAGoutput,DIAGerror,filename);
        DIAGoutput("CREATED OK");
    }
    else
        DIAGoutput("Error creating");
    EXdelete();
                
}
/*{
**  Name: do_exec - Execute an evidence collection script
**
**  Description:
**      This routine executes an evidence collection script putting its stdout
**      into a new file in the evidence set
**
**  Inputs:
**      i4  evset       Evidence set to put output in
**      PTR script    Script to execute
**
**  Exceptions:
**      All failures are contained via an exception handler
**
**  History:
*/
 
static VOID
do_exec(evset,script)
i4  evset;
PTR script;
{
    EX_CONTEXT  ex;
    char        title[80];
    char        filename[EVSET_MAX_PATH];
 
    /* Scripts are given 5 minutes to execute and then aborted */
 
    if(EXdeclare(DIAGhandler,&ex)!=OK)
    {
        EXdelete();
    }
    else
    {
        i4 len;
        STcopy("EXEC ",title);
        len = (STlength(script) > 74 ? 74 : STlength(script) );
        STncpy( &title[5], script, len );
        title[ 5 + len ] = EOS;
 
        if(EVSetCreateFile(evset,EVSET_SYSTEM|EVSET_TEXT,title,filename,
           sizeof(filename))==OK)
        {
            CL_ERR_DESC err_code;
            LOCATION my_loc;
 
            LOfroms(PATH & FILENAME,filename,&my_loc);
            PCcmdline(NULL,script,PC_WAIT,&my_loc,&err_code);
        }
 
        EXdelete();
    }
}
/*{
**  Name: execute - Execute a command and put its output in evidence set
**
**  Description:
**     This routine executes the given command and puts its output into a new
**     evidence set file with the given title.
**
**     If an evidence set file with the given title already exists then it is
**     displayed.
**
**  Inputs:
**     i4  id              Evidence set id
**     PTR command       Comamnd to execute
**     PTR title         Title of evidence set file
**
**  History:
*/
 
static void
execute(id,command,title)
i4  id;
PTR command;
PTR title;
{
   EVSET_ENTRY fdetails;
   CL_ERR_DESC err_code;
   LOCATION my_loc;
   STATUS status;
   FILE *fdes;
   i4  file=0;
 
      if(status = EVSetCreateFile(id,EVSET_SYSTEM|EVSET_TEXT,title,
                fdetails.value,sizeof(fdetails.value))!=OK)
      {
         DIAGoutput("Failure 0x%x",status);
         DIAGoutput("Fail to create evidence set file \"%s\"", (i4)title);
         return;
      }
 
      if(LOfroms(PATH&FILENAME,fdetails.value,&my_loc)==OK &&
         SIfopen(&my_loc,"w",SI_TXT,2048,&fdes)==OK)
      {
          PCcmdline(NULL,command,PC_WAIT,&my_loc,&err_code);
      }
 
      SIclose(fdes);
 
 
}

/*{
**  Name: copy - Copy a file (or end of a file) to an evidence set
**
**  Description:
**      This routine copies the contents of a file or the last part of a file
**      into a named evidence set file and echos the data to the screen.
**
**      If the lines parameter is supplied (ie not -1) then this routine scans
**      backwards counting newlines until it reaches the start of the file or
**      the required number of lines. The copy is then done from this point.
**
**      If an evidence set file already exists with the given name then this is
**      instead displayed to the screen.
**
**  Inputs:
**      i4      id              Evidence set id
**      LOCATION *location      Location of file to copy
**      PTR     title           Title for evidence set file
**      i4      lines           Lines to copy (from end of file) or -1 for all
**
**  History:
**      08-May-12 (wanfr01)
**          Bug 120370
**          Remove unneeded set to zero which can cause a memory overwrite.
*/
 
char copy_buffer[OUTPUT_BUFFER];
static void
copy(id,location,title,lines)
i4  id;
LOCATION *location;
PTR title;
i4  lines;
{
   LOCATION my_loc;
   EVSET_ENTRY fdetails;
   FILE *source=NULL;
   FILE *dest=NULL;
   i4  file=0;
   i4  len;
 
 
   /* If we didn't find it or couldn't open it create a new one */
   /* and use the caller specified file as the source           */
 
      if(EVSetCreateFile(id,EVSET_SYSTEM|EVSET_TEXT,title,fdetails.value,
         sizeof(fdetails.value))!=OK)
      {
         DIAGoutput("Failed");
         return;
      }
 
      if(LOfroms(PATH&FILENAME,fdetails.value,&my_loc)!=OK ||
         SIfopen(&my_loc,"w",SI_RACC,OUTPUT_BUFFER,&dest)!=OK ||
         SIfopen(location,"r",SI_RACC,OUTPUT_BUFFER,&source)!=OK)
      {
         DIAGoutput("Fail to open files");
         return;
      }
 
      if(lines!= -1)
      {
         /* Position to required start position */
 
         i4  offset;
         i4  pos;
 
         /* Find end of file */
 
         SIfseek(source,0,SI_P_END);
         offset = SIftell(source);
 
         /* Move offset onto buffer sized boundary */
 
         offset=((offset/sizeof(copy_buffer))*sizeof(copy_buffer))+sizeof(copy_buffer);
 
         while(offset!=0 && lines>0)
         {
             offset-=sizeof(copy_buffer);
             SIfseek(source,offset,SI_P_START);
             SIread(source,sizeof(copy_buffer),&len,copy_buffer);
             for(pos=len-1;pos>=0&&lines>0;--pos)
             {
                if(copy_buffer[pos]=='\n')
                   lines--;
             }
         }
 
         /* Move to the require start position */
 
         SIfseek(source,offset+pos+1,SI_P_START);
      }
 
   /* Copy the data over */
 
   while(SIread(source,sizeof(copy_buffer),&len,copy_buffer)==OK)
   {
      if(dest!=NULL)
         SIwrite(len,copy_buffer,&len,dest);
   }
 
   if(len!=0)
   {
      if(dest!=NULL)
         SIwrite(len,copy_buffer,&len,dest);
   }
 
   SIclose(source);
   if(dest!=NULL)
      SIclose(dest);
}

/* {
** do_dump - examine core files and produce stack traces
**
** Description:
**     This routine examines the stack traces 
**     for each thread in the server.
**
**      It is platform specific and needs to stay in the CL.
**
** History:
**     12-Jan-2011 (horda03) b124923
**        Only output Open Tables and Current Queries if the
**        server supports them.
*/
do_dump(stack)
u_i4 *stack;
{
    EX_CONTEXT ex;
    i4         ret;
    i4         do_tables = 0;

    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        ret = Ex_diag_link(DIAG_DUMP_STACKS,DIAGtr_output,DIAGoutput,(PTR)stack);

        if (ret) do_tables++;
    }
    else
    {
        DIAGoutput("Error occurred while dumping stacks Ex_diag_link 0x%x",Ex_diag_link);
        DIAGoutput("To get more information set dump on fatal");
    }
    EXdelete();

    if (do_tables)
    {
       if(EXdeclare(DIAGhandler,&ex)==OK)
       {
           DIAGoutput
              ("----------------------- OPEN TABLES -----------------------\n");
           Ex_diag_link(DMF_OPEN_TABLES,DIAGoutput,DIAGerror,NULL); 

           DIAGoutput
              ("---------------------- CURRENT QUERY ----------------------\n");
           Ex_diag_link(SCF_CURR_QUERY,DIAGoutput,DIAGerror,NULL); 
       }
       EXdelete();
    }
}

i4  in_dump = 0;
VOID
EXdump(error,stack)
u_i4 error;
u_i4 *stack;
{
    char        buffer[200];
    i4          c,i;
    i4          status;
    i4          evset;
    char        error_desc[30];
    i4          fac_num;
    EVSET_DETAILS details;
    EVSET_ENTRY fdetails;
    LOCATION    my_loc;
    FILE        *iiexcept;
    char        *summary_name;
    SYSTIME     temp;
    i4          id;
    EX_CONTEXT  ex;
    bool        do_dmf_stats = FALSE;
    bool        do_psf_stats = FALSE;
    bool        do_qsf_stats = FALSE;




    static i4  last_message = 0;
    /* ignore ERopt message */
    if (error == 0 || last_message == error || in_dump || 
        error == E_CS0030_CS_PARAM)
    {
        return;
    }
    last_message = error;

    /* Is the dump system initialised ? */

    if(errmap.valid != EXDUMP_INIT)
    {
        in_dump = 0;
        return;
    }

    /* Do we want to ignore this error ? */
    for(c=0;c<errmap.map_size && error>=errmap.map[c].from;++c)
    {
        if(error>=errmap.map[c].from && error<=errmap.map[c].to)
        {
            in_dump = 0;
            return;
        }
    }
    TRdisplay("Calling EXdump error 0x%x\n",error);

    fac_num = (error & 0xffff0000) >> 16; 
    for (i=0; i < NUMFAC; ++i)
    {
        if (Factab[i].num == fac_num)     
                break;
    }


    switch (error)
    {
        case EV_SIGVIO_DUMP: STprintf(error_desc,"Access Violation");break;
        case EV_DUMP_ON_FATAL : STprintf(error_desc,"Fatal Error");break;
        break;
        default: 
        if (i == NUMFAC)
                STprintf(error_desc,"E_??%04x\n",0xffff&error);
        else
                STprintf(error_desc,"E_%s%04x",Factab[i].fac,0xffff&error);
        break;
        
    }
        

    if(EVSetCreate(&evset,error_desc,(PTR)Version))
    {
        TRdisplay ("Failed to create evidence set \n");
        in_dump = 0;
        return;
    }
    in_dump = 1;

    status=EVSetCreateFile(evset,EVSET_TEXT|EVSET_SYSTEM,"Evidence summary",
          fdetails.value,sizeof(fdetails.value));
    if(status!=OK)
    {
        TRdisplay("Can create evidence summary\n");
    }
    else
    {
        summary_name = STalloc(fdetails.value);
    }
    if (summary_name != NULL)
    {
        if(LOfroms(PATH&FILENAME,summary_name,&my_loc) == OK)
             SIopen(&my_loc,"w",&summary_file); 
    }
    else
    {
        TRdisplay("DIAG: Failed to create summary file\n");
        in_dump = 0;
        return;
    }


    id = evset;
    if ((status=EVSetList(&id,&details)))
   {
        TRdisplay("EVSetList Error\n");
   }

    DIAGoutput("INGRES EVIDENCE SUMMARY");
    DIAGoutput("=======================");
    DIAGoutput("Description:   %s",error_desc);
    DIAGoutput("Version:       %s",Version);
    temp.TM_secs=details.created;
    TMstr(&temp,buffer);
    DIAGoutput("Creation date: %s\n",buffer);
        
    STprintf( iiexcept_opt, "%sexcept.opt", SystemCfgPrefix );
    if(NMloc(FILES,FILENAME,iiexcept_opt,&my_loc)==OK &&
       SIfopen(&my_loc,"r",SI_TXT,sizeof(buffer),&iiexcept)==OK)
    {
        while(SIgetrec(buffer,sizeof(buffer),iiexcept)==OK)
        {
            u_i4 from=0;
            u_i4 to=0;
 
            buffer[STlength(buffer)-1]=EOS;
            for(c=0;c<8;++c)
                from=(from*16)+to_hex(&buffer[c]);
            for(c=8;c<16;++c)
                to=(to*16)+to_hex(&buffer[c]);

            if (EV_DMF_DIAGNOSTICS >=from && EV_DMF_DIAGNOSTICS <= to && buffer[16] == 'D')
                do_dmf_stats = TRUE; 
            if (EV_QSF_DIAGNOSTICS >=from && EV_QSF_DIAGNOSTICS <= to && buffer[16] == 'D')
                do_qsf_stats = TRUE; 
            if (EV_PSF_DIAGNOSTICS >=from && EV_PSF_DIAGNOSTICS <= to && buffer[16] == 'D')
                do_psf_stats = TRUE; 

            if (error >= from && error <= to)
            {
                switch(buffer[16])
                {
                    case 'F':
                    case 'D':
                        TRdisplay ("II_EXCEPTION is creating a dump file.\n");
                        do_dump(stack); 
                        break;
                    case 'E':
                        TRdisplay ("II_EXCEPTION Running %s\n",buffer);
                        do_exec(evset,&buffer[17]);
                        break;
                    default : break;
                }
            }
        }

        SIclose( iiexcept );
    }
    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        DIAGoutput
        ("----------------------- ERRLOG.LOG ------------------------\n");
 
        if(NMloc(FILES,FILENAME,"errlog.log",&my_loc)==OK)
            copy(evset,&my_loc,"Errlog.log file",100);
        DIAGoutput
        ("CREATED OK\n");
    }
    EXdelete();

   if(EXdeclare(DIAGhandler,&ex)==OK)
   {
        DIAGoutput
        ("----------------- ENVIRONMENT VARIABLES -------------------\n");
 
/* This define should be set in evset.h for your platform */
#ifndef EVSET_SHOW_ENV_CMD
#define EVSET_SHOW_ENV_CMD "show logical ii_*"
#endif /* EVSET_SHOW_ENV_CMD */
 
        execute(evset,EVSET_SHOW_ENV_CMD,"Environment variables");
        DIAGoutput
        ("CREATED OK\n");
   }
   EXdelete();


    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        DIAGoutput
        ("----------------------- CONFIG.DAT ------------------------\n");
 
        if(NMloc(FILES,FILENAME,"config.dat",&my_loc)==OK)
            copy(evset,&my_loc,"Config.dat file",-1);
        DIAGoutput
        ("CREATED OK\n");
    }
    EXdelete();


    if (do_dmf_stats == TRUE) do_stats(evset,"DMF STATS",DMF_SELF_DIAGNOSTICS);
    if (do_qsf_stats == TRUE) do_stats(evset,"QSF STATS",QSF_SELF_DIAGNOSTICS);
    if (do_psf_stats == TRUE) do_stats(evset,"PSF STATS",PSF_SELF_DIAGNOSTICS);

    DIAGoutput(NULL);
    SIclose(summary_file); 
    in_dump = 0;
}

VOID
EXdumpInit()
{
    FILE        *iiexcept;
    char        buffer[200];
    LOCATION    my_loc;
    PTR         ingbuild;
    char        ingbuild_var[30];

    errmap.valid=0;

    /* Open problem database file (if it exists) */

    STprintf( iiexcept_opt, "%sexcept.opt", SystemCfgPrefix );
    if(NMloc(FILES,FILENAME,iiexcept_opt,&my_loc)==OK &&
       SIfopen(&my_loc,"r",SI_TXT,sizeof(buffer),&iiexcept)==OK)
    {
        errmap.map_size=0;

        TRdisplay("Server Diagnostics found %s file\n", iiexcept_opt);
        while(SIgetrec(buffer,sizeof(buffer),iiexcept)==OK &&
              errmap.map_size < MAP_SIZE)
        {
            /* Only interested in IGNORE actions */

            if(buffer[16]=='I')
            {
                u_i4 from=0;
                u_i4 to=0;
                i4  c;

                /* Decode from and to values */

                for(c=0;c<8;++c)
                    from=(from*16)+to_hex(&buffer[c]);
                for(c=8;c<16;++c)
                    to=(to*16)+to_hex(&buffer[c]);

                /* Remember for future reference */

                errmap.map[errmap.map_size].from = from;
                errmap.map[errmap.map_size].to = to;
                errmap.map_size++;
            }
        }

        errmap.valid = EXDUMP_INIT;

        SIclose(iiexcept);
    }

    STprintf( iidbms, "%sdbms", SystemCfgPrefix );

    if ( STcompare( SystemVarPrefix, "II" ) == 0 )
        STprintf( ingbuild_var, "ING_BUILD" );
    else
        STprintf( ingbuild_var, "%s_BUILD", SystemVarPrefix );

    NMgtAt(ingbuild_var,&ingbuild);
    if (ingbuild == NULL || *ingbuild == EOS)
    {
        if (NMloc(BIN,FILENAME,iidbms,&my_loc) != OK)
            return;
    }
    else
    {
        STlpolycat(3, sizeof(buffer)-1, ingbuild, "/bin/", iidbms,
                &buffer[0]);
        if (LOfroms(FILENAME&PATH,buffer,&my_loc) != OK)
            return;
    }
}

/*{
**  Name: to_hex - Convert an character digit to a number 0-15
**
**  Description:
**      This routine is used to convert a character representation of
**      a hex digit into a number in the range 0-15
**
**  Inputs:
**      char *pointer       Pointer to the digit to convert
**
**  Returns:
**      -1       Invalid hex digit
**     0-15      Value of hex digit
**
**  History:
*/

static i4
to_hex(pointer)
PTR pointer;
{
    i4  ret_val;

    switch(*pointer)
    {
       case '0':
           ret_val=0;
           break;
       case '1':
           ret_val=1;
           break;
       case '2':
           ret_val=2;
           break;
       case '3':
           ret_val=3;
           break;
       case '4':
           ret_val=4;
           break;
       case '5':
           ret_val=5;
           break;
       case '6':
           ret_val=6;
           break;
       case '7':
           ret_val=7;
           break;
       case '8':
           ret_val=8;
           break;
       case '9':
           ret_val=9;
           break;
       case 'a':
       case 'A':
           ret_val=10;
           break;
       case 'b':
       case 'B':
           ret_val=11;
           break;
       case 'c':
       case 'C':
           ret_val=12;
           break;
       case 'd':
       case 'D':
           ret_val=13;
           break;
       case 'e':
       case 'E':
           ret_val=14;
           break;
       case 'f':
       case 'F':
           ret_val=15;
           break;
       default:
          ret_val= -1;
    }

    return(ret_val);
}
 
/*{
**  Name: output - Output information to evidence summary
**
**  Description:
**     This routine acts very much like SIprintf buts it outputs the information
**     to a file in the evidence set and to stdout. Information is only output
**     to stdout if less than 128K of data has been output (which is the
**     current telservice limit).
**
**     If the supplied format string is NULL then an end of evidence summary
**     line is output containing the percentage of the evidence summary maximum
**     we have used - to enable tracking effect adding extra evidence to the
**     summary would have.
**
**  Inputs:
**     PTR format                    Printf style format string (NULL for end
**                                   of summary)
**     PTR  a1,a2,a3,a4,a5,a6,a7,a8   Parameter inserts
**
**  Side effects:
**     Internally keeps track of the amount of data output and stops sending
**     data to stdout when more than 128K would result.
**
**  History:
*/

VOID
DIAGoutput(format,a1,a2,a3,a4,a5,a6,a7,a8)
PTR format;
PTR a1;
PTR a2;
PTR a3;
PTR a4;
PTR a5;
PTR a6;
PTR a7;
PTR a8;
{
    char output_buffer[OUTPUT_BUFFER];

    if(format==NULL)
    {
        STprintf(output_buffer,
        "============= END OF EVIDENCE SUMMARY =============\n");
    }
    else
    {
         STprintf(output_buffer,format,a1,a2,a3,a4,a5,a6,a7,a8);
    }
                                               
    if (summary_file!=NULL)
    {
        SIfprintf(summary_file,"%s\n",output_buffer);
        SIflush(summary_file);
    }
}

VOID
DIAGtr_output(arg, length, buffer)
i4             arg;
i4             length;
char                *buffer;
{
    i4             count;
 
    if (summary_file!=NULL)
    {
        SIwrite(length, buffer, &count, summary_file);
        SIwrite(1, "\n", &count, summary_file);
        SIflush(summary_file);
    }
    else
    {
        TRdisplay("DIAG: evset_output: Cannot write to evset file:\n");
        TRdisplay("DIAG: evset_output: evset_file == NULL FILE *.\n");
    }
}

/*{
**  Name: DIAGerror - Error handler
**
**  Description:
**     Any error in any of the analysis routines calls this routine. It is
**     responsible for outputing an error to the evidence set and recovering
**
**     The recovery is actually handled by the driver which declares exception
**     handlers which are signalled by this routine
**
**  Inputs:
**     PTR format                    Printf style format string
**     PTR a1,a2,a3,a4,a5,a6         Parameter inserts
**
**  Returns:
**     Doesn't
**
**  Exceptions:
**     Raises EXKILL signal
**
**  History:
*/
 
VOID
DIAGerror(format,a1,a2,a3,a4,a5,a6)
PTR format;
PTR a1;
PTR a2;
PTR a3;
PTR a4;
PTR a5;
PTR a6;
{
    DIAGoutput(NULL,format,a1,a2,a3,a4,a5,a6,0,0);
}


static STATUS
DIAGhandler(ex_args)
EX_ARGS *ex_args;
{
    return(EXDECLARE);
}
