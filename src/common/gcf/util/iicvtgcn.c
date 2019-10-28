/*
** Copyright (c) 2010 Actian Corporation
*/

/*
**  Name: iicvtgcn -- Convert RACC (relative) GCN files to optimized
**      RACC files and convert encoding
**
**  Description:
**
**      Iicvtgcn converts Name Server vnode and login files in standard RACC 
**      format to optimized RACC format.  If the source file is found to be
**      in optimized RACC format, iicvtgcn exits without performing
**      any conversions.
**    
**      To execute:
**
**      $ iicvtgcn [ -v ] [ -c | -u ] [ node_name ]
**
**      Optional arguments:
**
**      -v 	 Verbose.  Write detailed information to standard output.
**      -c       Cluster.  Merge node-specifc GCN info to common
**               clustered info.
**      -u	 Uncluster.  Extract common clustered GCN info into 
**               node-specific info.
**      nodeName Node name.  The name of a non-local host as it appears
**               in the config.dat file.
**
**      The -u and -c flags are mutually exclusive.
**    
**  Entry points:
**	This file contains no externally visible functions.
**
** History:
**     06-Dec-2005 (loera01)
**         Created.
**     11-Feb-2008 (Ralph Loen) Bug 116097 
**         Formalized for Ingres release.
**     14-Jan-2010 (horda03) Bug 123153
**         Allow optinal parameter of node name (for use in setting
**         up cluster installations.
**     20-Oct-2010 (Ralph Loen) bug 122895
**         Reverse the behavior.  The previous version of iicvtgcn
**         converted optimized files to un-optimized files, due to
**         misinterpretation of the GCN_RACC_FILE modifier to SIfOpen().
**         This version opens the output files as GCN_RACC_FILE.
**         Do not append the host name for global files.  Include
**         attribute and ticket files.
**     24-Oct-2010 (Ralph Loen) Bug 122895
**         Read config.cluster_mode in config.dat to determine whether
**         the installation is clustered--not iiname.all.
**     16-Nov-2010 (Ralph Loen) Bug 122895
**         Revised to be sensitive to encoding version.  Passwords encoded
**         in Ingres 2.0 and Ingres 2.6 V0, but Ingres 9.1 and later use
**         V1.  Clustered installations must be converted to V0 and
**         unclustered installations must be converted to V1.  
**         The hostname argument is deprecated, since its use is confusing
**         when encoding is added to the picture.  Instead, the -c and -u
**         arguments either merge the node-specific info into common, or
**         extract common into node-specific.  The default behavior is 
**         determined by ii.[hostname].config_cluster_mode (ON or OFF).
**     20-Nov-2010 (Ralph Loen) Bug 122895
**         Cleaned up output of usage() display.
**     08-Dec-2010 (Ralph Loen) SIR 124815   
**         Added capabiilty to handle GCN_DBREC_V1 (variable-length)   
**         records.  New functions gcn_extract_rec1() and   
**         gcn_rewrite_rec1() read and rewrite GCN_DBREC_V1 records,   
**         respectively. 
**    09-Dec-2010 (Ralph Loen) Bug 124825
**         Ignore blank command-line arguments.
**    13-Dec-2010 (Ralph Loen) Bug 124832
**         Added missing argument to an SIprintf() call that led to an ACCVIO.
**         Moved initialization of isLogin to FALSE at its proper position,
**         which is before action is performed conditional to the file type.
**   05-Jan-2011 (Ralph Loen) Bug 124897
**         Ignore arguments not beginning with a hyphen.
**   18-jan-2011 (Ralph Loen) Bug 124932
**         Restore alternate node name argument and undo changes for 
**         Bug 124897.
*/

#include <compat.h>
#include <gl.h>

#include <cm.h>
#include <cv.h>
#include <er.h>
#include <gc.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <pm.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <tm.h>
#include <tr.h>

#include <erclf.h>
#include <erglf.h>

#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcu.h>
#include <gcnint.h>
#include <gcaint.h>

#include <descrip.h>
#include <lib$routines.h>
#include <stsdef.h>

#define NAME_FILE_SIZE 80
#define NBR_NAMED_FILES 5
#define GCN_LOGIN_PREFIX        "IILOGIN-"
#define NBR_SUBFIELDS   4
#define ARG_FLAG_SIZE   8
#define GC_L_UID       32
#define GC_L_OBJ      256       
#define GC_L_VAL       64

typedef long VMS_STATUS;

/*
** The open enum table must be in sync with the arg array.
*/
typedef enum
{
    HELP1, HELP2, HELP3, HELP4, VERBOSE, CLUSTER, UNCLUSTER
} opt;

char arg[][ARG_FLAG_SIZE] =
{
    "-h",
    "--h", 
    "-help", 
    "--help",
    "-v",
    "-c",
    "-u"
};

i2 argLen = sizeof(arg) / ARG_FLAG_SIZE;

/*
** The nfiles enum must be in sync with the named_file array.
*/
typedef enum
{
    IILOGIN, IINODE, IIATTR, IIRTICKET, IILTICKET
} nfiles;

typedef struct
{
    bool add_cluster_node;
    char file[NAME_FILE_SIZE];
} NAMED_FILE;

NAMED_FILE named_file[NBR_NAMED_FILES] =
{   
    FALSE, "LOGIN", 
    FALSE, "NODE",
    FALSE, "ATTR",
    TRUE, "RTICKET",
    TRUE, "LTICKET"
};

typedef struct
{
    QUEUE q;
    GCN_DB_REC0 rec0;
} GCN_QUEUE;

QUEUE gcn_qhead;
QUEUE merge_qhead;

bool verboseArg = FALSE;

char *empty = "";

void usage();
void cleanup_queues();
bool gcn_merge_rec( GCN_QUEUE *dst_q, bool isLogin );
STATUS gcn_recrypt( bool clustered, u_i1 *mask, char *gcn_val, 
    bool *decryptErr, bool *writeOutput);
void gcn_cvt_named_files(char *gcn_node, bool clusterArg, bool unclusterArg,
    bool writeOutput);
bool gcn_get_rec0(GCN_DB_RECORD *rec, GCN_DB_REC0 *rec0);
bool gcn_extract_rec1( GCN_DB_REC1 *record, char *gcn_uid, char *gcn_obj,
   char *gcn_val );
STATUS gcn_rewrite_rec1( char *uid, char *obj, char *val, 
    GCN_DB_REC1 *record );

main(int argc, char **argv)
{
    STATUS status = OK;
    bool writeOutput = FALSE;
    bool validSyntax;
    char gcn_node[MAX_LOC+CM_MAXATTRNAME];
    i2 i,j;
    QUEUE *q;
    bool clusterArg = FALSE;
    bool unclusterArg = FALSE;
    bool nodeArg = FALSE;

    MEadvise(ME_INGRES_ALLOC);
    SIeqinit();
    PMinit();
    if ( PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL ) != OK )
    {
        SIprintf("Error reading config.dat, exiting\n");
        goto cvt_exit;      
    }

    QUinit(&gcn_qhead);
    QUinit(&merge_qhead);

    if (argc == 1)
        validSyntax = TRUE;

    /*
    ** Parse input arguments.
    */    
    for (i = 1; i < argc; i++)
    {
        validSyntax = FALSE;
        if (argv[i][0] == '-')
        {
            for (j = 0; j < argLen; j++)
            {
                if (!STncasecmp(arg[j],argv[i], STlength(argv[i])))
                {
                    switch(j)
                    {
                    case HELP1:
                    case HELP2:
                    case HELP3:
                    case HELP4:
                       usage();
                       break;

                    case VERBOSE:
                       validSyntax = TRUE;
                       verboseArg = TRUE;
                       break;

                    case CLUSTER:
                       validSyntax = TRUE;
                       clusterArg = TRUE;
                       writeOutput = TRUE;
                       break;

                    case UNCLUSTER:
                       validSyntax = TRUE;
                       unclusterArg = TRUE;
                       writeOutput = TRUE;
                       break;
                    }
                } /* if (!STncasecmp(arg[j],argv[i], STlength(argv[i]))) */
                if (validSyntax)
                    break;
            } /* for (j = 0; j < argLen; j++ */
        } /* if (argv[i][0] == '-') */
        else if (!nodeArg)
        {
            /*
            ** Check syntax of optional node name argument.
            */
            validSyntax = TRUE;
            for (j = 0; j < STlength(argv[i]); j++)
            {
                if (!CMalpha(&argv[i][j]) && !CMdigit(&argv[i][j]))
                {
                    validSyntax = FALSE;
                    break;
                }
            }
            if (!validSyntax)
                break;
            nodeArg = TRUE;
            CVupper(argv[i]);
            STcopy(argv[i], gcn_node);
        } /*  if (argv[i][0] == '-') */
        if (!validSyntax)
            break;
    } /* for (i = 1; i < argc; i++) */

    if (!validSyntax)
    {
        usage();
        goto cvt_exit;
    }

    if (clusterArg && unclusterArg)
    {
        SIprintf("Cannot specify both -c and -u\n\n");
            usage();
        goto cvt_exit;
    }

    if (!nodeArg)
        GChostname( gcn_node, sizeof(gcn_node));

    gcn_cvt_named_files(gcn_node, clusterArg, unclusterArg, writeOutput);

cvt_exit:

    PCexit(OK);
}

/*
** Name: gcn_cvt_named_files
**
** Description:
**      Convert GCN named files according the requirements for file
**      optimization, naming and encoding.
**
** Input:
**      gcn_node       Node name of file files to be modified.
**      clusterArg     Whether or not clustering has been requested.
**      unclusterArg   Whether or not unclustering has been requested.
**      writeOutp      Whether or not output files are to be written.
**
** Output:             None.
**
** Side effects:
**                     The writeOutp argument may be overwritten.
**
** Returns:            Void.
**      
** History:
**      15-Nov-2010 (Ralph Loen)
**          Created.
*/

void gcn_cvt_named_files(char *gcn_node, bool clusterArg, bool unclusterArg,
    bool writeOutp)
{
    STATUS status = OK;
    VMS_STATUS vStatus;
    char srcbuf[NAME_FILE_SIZE];
    char dstbuf[NAME_FILE_SIZE];
    char delFile[NAME_FILE_SIZE];
    struct dsc$descriptor_s filename_d = 
        { sizeof (delFile) - 1,
            DSC$K_DTYPE_T,
            DSC$K_CLASS_S,
            delFile
        };
    FILE *srcFile = NULL;
    FILE *dstFile = NULL;
    FILE *nameFile = NULL;
    LOCATION loc;
    bool clustered = FALSE;
    bool v1DecryptErr = FALSE;
    bool rewrite = FALSE;
    bool isLogin = FALSE;
    bool writeOutput = writeOutp;
    i4   gcn_rec_type;
    i4   total_recs = 0;
    i2 i,j;
    i4 active_rec;
    i4 offset;
    char *onOff = NULL;
    bool srcOpened=FALSE;
    bool dstOpened=FALSE;
    bool printable = TRUE;
    GCN_QUEUE *gcn_q;
    GCN_QUEUE *merge_q;
    i4 rec_len = 0;
    QUEUE *q;
    u_i1  local_mask[ 8 ];        /* Must be 8 bytes */
    char name[MAX_LOC+CM_MAXATTRNAME];
    i4 count;
    char *p = NULL;
    i4 dcryptFail = 0;
    i2 pc;
    char *pv[ 3 ];
    GCN_DB_RECORD tmp_rec;
    GCN_DB_REC0 rec0;

    /*
    ** Set defaults for searching config.dat.
    */
    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, gcn_node );
    PMsetDefault( 2, ERx("gcn") );

    /*
    ** See if this is a clustered installation.  If it is, 
    ** the node, login, and attribute files have no file extension.
    */
    status = PMget( ERx("!.cluster_mode"), &onOff);
    if (status != OK)
    {
        SIprintf("iicvtgcn: node %s not found in config.dat, exiting\n",
           gcn_node);
        PCexit(OK);
    }
    if (onOff && *onOff)
        ;
    else
        onOff = "OFF";

    if (verboseArg)
        SIprintf("Cluster mode is %s\n", onOff);

    if (!clusterArg && !unclusterArg)
        clustered = !STncasecmp(onOff, "ON", STlength(onOff));

    /*
    ** Generate key seeds for encoding and decoding.
    */
    STpolycat( 2, GCN_LOGIN_PREFIX, gcn_node, name );
    gcn_init_mask( name, sizeof( local_mask ), local_mask );

    /*
    ** Rewrite the named GCN files.  For clustered installations, the
    ** node, login and attribute files have no hostname extension.
    */
    for ( i = 0; i < NBR_NAMED_FILES; i++ )
    {  
        /*
        ** Ticket files are simply deleted.
        */
        if (i == IILTICKET || i == IIRTICKET)
        {
            STprintf(delFile, "II_SYSTEM:[INGRES.FILES.NAME]II%s*;*", 
                named_file[i].file);
            if (verboseArg)
                SIprintf("Deleting %s\n", delFile);
            filename_d.dsc$w_length = STlength(delFile);
            vStatus = lib$delete_file(&filename_d,0,0,0,0,0,0,0,0,0);
            if (!vStatus & STS$M_SUCCESS)
                SIprintf("delete of %s failed, status is 0x%d\n", delFile,
                    vStatus);
            continue;
        }    
 
        rewrite = FALSE;
        if (!clusterArg && !unclusterArg)
            writeOutput = FALSE;
        if ( ( status = NMloc( FILES, PATH & FILENAME, 
            (char *)NULL, &loc ) ) != OK )
        {
            SIprintf("iicvtgcn: Could not find II_SYSTEM:[ingres.files]\n");
            goto cvt_exit;
        }
     
        LOfaddpath( &loc, "name", &loc );

        if (clustered || unclusterArg)
        {
            if (named_file[i].add_cluster_node)
                STprintf( srcbuf, "II%s_%s", named_file[i].file,gcn_node);
            else
                STprintf(srcbuf, "II%s", named_file[i].file);
        }
        else 
            STprintf( srcbuf, "II%s_%s", named_file[i].file,gcn_node);

        if (verboseArg)
            SIprintf("Opening %s for input\n", srcbuf);

        LOfstfile( srcbuf, &loc );

        /*
        ** Ignore non-existent files.
        */
        if ( LOexist( &loc ) != OK )
        {
            if (verboseArg)
                SIprintf("%s does not exist\n", srcbuf);
            continue;
        }
        /*
        ** Open the existing file as "regular" RACC.
        */
        status = SIfopen( &loc, "r", (i4)SI_RACC, sizeof( GCN_DB_RECORD ), 
            &srcFile );
        /*
        ** If the file exists but can't be opened, it's already optimized.
        */
        if (status == E_CL1904_SI_CANT_OPEN && ( LOexist( &loc ) == OK ) )
        {
            /*
            ** Open the existing file as "optimized" RACC.
            */
            status = SIfopen( &loc, "r", (i4)GCN_RACC_FILE, 
                sizeof( GCN_DB_RECORD ), &srcFile );
            if (status != OK)
            {
                SIprintf( "iicvtgcn: Error opening %s, status is %x\n",
                    srcbuf, status );
                continue;
            }
            if (verboseArg)
                SIprintf("%s is already optimized\n", srcbuf);
        }
        else if (status != OK)
        {
            SIprintf( "iicvtgcn: Error opening %s, status is %x\n",
                srcbuf, status );
            continue;
        }
        /*
        ** A successful open as SI_RACC means the file is not optimized.
        ** This file needs a rewrite.
        */
        else
        {
            if (verboseArg)
                SIprintf("Rewriting %s as optimized\n", srcbuf);
            writeOutput = TRUE;    
        }

        srcOpened = TRUE;

        while ( status == OK )
        {
            /*
            ** Read the source data and store in a queue for analysis.
            */
            status = SIread( srcFile, sizeof( GCN_DB_RECORD ),
                &count, (PTR)&tmp_rec );
            if ( status == ENDFILE )
                break;

            if ( status != OK )
            {
                SIprintf("iicvtgcn: Error reading %s, status is %x\n",
                    srcbuf, status);
                goto cvt_exit;
            }

            /*
            ** Convert any GCN_DB_REC1 records to GCN_DB_REC0.
            */
            if (!gcn_get_rec0(&tmp_rec, &rec0))
            {
                if (verboseArg)
                    SIprintf("Could not convert GCN_DB_REC0 to GCN_DB_REC1\n");
                continue;
            }

            /*
            ** Ignore deleted records.
            */
            if (rec0.gcn_invalid && rec0.gcn_tup_id != -1)
                continue;
            else 
            {
                gcn_q = (GCN_QUEUE *)MEreqmem(0, sizeof(GCN_QUEUE),0,NULL);
                if (!gcn_q)
                {
                    SIprintf("iicvtgcn: Cannot allocate memory, exiting\n");
                    goto cvt_exit;
                }

                MEcopy((PTR)&rec0, sizeof(GCN_DB_REC0), (PTR)&gcn_q->rec0);
                QUinsert(&gcn_q->q, &gcn_qhead);
                /*
                ** EOF record found.
                */
                if (gcn_q->rec0.gcn_tup_id == -1)
                  break;
            }
        } /* while ( status == OK ) */

        /*
        ** Decrypt passwords for IILOGIN files.  If any V1 records are found, 
        ** the IILOGIN file will need to be rewritten.
        */
        isLogin = FALSE;
        for (q = gcn_qhead.q_prev; q != &gcn_qhead; q = q->q_prev)
        {
            gcn_q = (GCN_QUEUE *)q;

            /*
            ** EOF record found.
            */
            if (gcn_q->rec0.gcn_tup_id == -1)
                break;

            if (i == IILOGIN)
            {
                isLogin = TRUE;

                MEcopy((PTR)&gcn_q->rec0, sizeof(GCN_DB_REC0), 
                    (PTR)&rec0);
                if (verboseArg)
                     SIprintf("\tEncoding vnode %s\n", gcn_q->rec0.gcn_obj);

                if (unclusterArg)
                    status = gcn_recrypt( FALSE, local_mask, 
                        gcn_q->rec0.gcn_val, &v1DecryptErr, &writeOutput);
                else if (clusterArg)
                    status = gcn_recrypt( TRUE, local_mask, 
                        gcn_q->rec0.gcn_val, &v1DecryptErr, &writeOutput);
                else
                    status = gcn_recrypt( clustered, local_mask, 
                        gcn_q->rec0.gcn_val, &v1DecryptErr, &writeOutput);

                if (status != OK)
                {
                    if (verboseArg)
                        SIprintf("Cannot decrypt password from " \
                            "vnode %s status %x\n", gcn_q->rec0.gcn_obj, status);
                    dcryptFail++;
                    MEcopy((PTR)&rec0, sizeof(GCN_DB_REC0), 
                        (PTR)&gcn_q->rec0);
                    continue;                
                }
                if (v1DecryptErr)
                {
                    if (verboseArg)
                        SIprintf("Cannot decrypt password from " \
                            "vnode %s\n", gcn_q->rec0.gcn_obj);
                    dcryptFail++;
                    MEcopy((PTR)&rec0, sizeof(GCN_DB_REC0), 
                        (PTR)&gcn_q->rec0);
                    continue;
                }
            }  /* if (LOGIN) */

        } /* for (q = gcn_qhead.q_prev; q != &gcn_qhead; q = q->q_prev) */
        if (dcryptFail && verboseArg && isLogin)
        {
            if (clustered || unclusterArg )
                SIprintf("\n%d vnode(s) could not be decrypted.\n" \
                    "Probably some login entries were created on " \
                    "another node.\nTry executing iicvtgcn on another " \
                    "node to merge the other node's entries.\n\n", dcryptFail);
            else
                SIprintf("\n%d vnode(s) could not be decrypted.\n" \
                    "Probably the login file was created on " \
                    "another host.\nTry executing iicvtgcn on " \
                    "a different host.\n\n", dcryptFail);
        }

        if (!writeOutput)
        {
            if (srcOpened)
                SIclose(srcFile);
            srcOpened = FALSE;
            cleanup_queues();
            continue;
        }
   
        /*
        ** Open the destination file with special GCN_RACC_FILE flag, for 
        ** optimized writes.
        */
        if (clustered || clusterArg) 
        {
            if (named_file[i].add_cluster_node)
                STprintf( dstbuf, "II%s_%s", named_file[i].file, gcn_node);
            else
                STprintf(dstbuf, "II%s", named_file[i].file);
        }
        else
            STprintf( dstbuf, "II%s_%s", named_file[i].file, gcn_node);

        if (clusterArg && !named_file[i].add_cluster_node)
            rewrite = TRUE;

        LOfstfile( dstbuf, &loc );

        if (rewrite)
        {
            if (verboseArg)
                SIprintf("Opening %s for input/output\n", dstbuf);
            status = SIfopen( &loc, "rw", (i4)GCN_RACC_FILE, 
                sizeof( GCN_DB_RECORD ), &dstFile );
            if ( status != OK )
            {
                if (verboseArg)
                    SIprintf("Doesn't exist.  Opening %s for output\n", dstbuf);
                status = SIfopen( &loc, "w", (i4)GCN_RACC_FILE, 
                    sizeof( GCN_DB_RECORD ), &dstFile );
                if (status == OK)
                {
                    SIclose( dstFile);
                    if (verboseArg)
                        SIprintf("Re-opening %s for input/output\n", dstbuf);
                    status = SIfopen( &loc, "rw", (i4)GCN_RACC_FILE, 
                        sizeof( GCN_DB_RECORD ), &dstFile );
                    if (status != OK)
                    {
                        SIprintf("FATAL error, cannot open dest file for rewrite\n");
                        PCexit(OK);
                    }
                }
            }
        } /* if (rewrite) */
        else
        {
            if (verboseArg)
                SIprintf("Opening %s for output\n", dstbuf);
            status = SIfopen( &loc, "w", (i4)GCN_RACC_FILE, 
                sizeof( GCN_DB_RECORD ), &dstFile );
        }
        if ( status != OK )
        {
            SIprintf( "iicvtgcn: Error opening %s, status is %x\n",dstbuf,
                status );
            goto cvt_exit;
        }

        dstOpened = TRUE;

        if (verboseArg)
            SIprintf("%s %s\n", rewrite ? "Rewriting" : "Writing", dstbuf);

        /*
        ** If this is a merge operation (-c), login, attribute or
        ** node files may change the location of EOF, since the
        ** file to be merged may have different records than
        ** the destination file. 
        ** Before merging, the output file is read and fed into a queue.
        ** Then each merge record is compared to each output record.
        ** If the entire records match, nothing is done. 
        ** If a global login record matches only the vnode name
        ** global or private records will be added if not present;
        ** otherwise, nothing is done.
        ** Node or attribute records may be added if only one field
        ** fails to match.
        */
        status = SIfseek(dstFile, (i4)0, SI_P_START);
        if (rewrite)
        {
            while ( status == OK )
            {
                /*
                ** Read the source data and store in a queue for analysis.
                */
                status = SIread( dstFile, sizeof( GCN_DB_RECORD ),
                    &count, (PTR)&tmp_rec );
                if ( status == ENDFILE )
                    break;
                if ( status != OK )
                {
                    SIprintf("iicvtgcn: Error reading %s, status is %x\n",
                        dstbuf, status);
                    goto cvt_exit;
                }

                /*
                ** Convert any GCN_DB_REC1 records to GCN_DB_REC0.
                */
                if (!gcn_get_rec0(&tmp_rec, &rec0))
                {    
                    if (verboseArg)
                        SIprintf("Could not convert GCN_DB_REC0 to " \
                            "GCN_DB_REC1\n");
                    continue;
                }


                /*
                ** Ignore deleted records.
                */
                if (rec0.gcn_invalid && rec0.gcn_tup_id != -1)
                    continue;

                merge_q = (GCN_QUEUE *)MEreqmem(0, sizeof(GCN_QUEUE),0,NULL);
                if (!merge_q)
                {
                    SIprintf("iicvtgcn: Cannot allocate memory, exiting\n");
                    goto cvt_exit;
                }
                MEcopy((PTR)&rec0, sizeof(GCN_DB_REC0), 
                    (PTR)&merge_q->rec0);
                QUinsert(&merge_q->q, &merge_qhead);

                /*
                ** EOF record found.
                */
                if (merge_q->rec0.gcn_tup_id == -1)
                    break;
            } /* while ( status == OK ) */

            /*
            ** Go through the input queue.  Compare each record with
            ** the output (merge) queue.  If the record is invalid
            ** or doesn't match, it's ignored.
            */
            dcryptFail = 0;
            total_recs = 0;
            for (q = gcn_qhead.q_prev; q != &gcn_qhead; q = q->q_prev)
            {
                gcn_q = (GCN_QUEUE *)q;
              
                if (gcn_q->rec0.gcn_tup_id == -1)
                    break;

                if ( !gcn_merge_rec( gcn_q, isLogin ) )
                    continue;

                if (isLogin)
                {
                    /*
                    ** Login passwords get encrypted as V0 in a cluster
                    ** environment.
                    */
                    MEcopy((PTR)&gcn_q->rec0, sizeof(GCN_DB_REC0), 
                        (PTR)&rec0);

                    if (verboseArg)
                         SIprintf("\tEncoding vnode %s\n", 
                             gcn_q->rec0.gcn_obj);

                    status = gcn_recrypt( TRUE, local_mask, 
                        gcn_q->rec0.gcn_val, &v1DecryptErr, &writeOutput);
    
                    if (status != OK)
                    {
                        if (verboseArg)
                            SIprintf("Cannot decrypt password from " \
                                "vnode %s status %x\n", gcn_q->rec0.gcn_obj, 
                                    status);
                        dcryptFail++;
                        MEcopy((PTR)&rec0, sizeof(GCN_DB_REC0), 
                            (PTR)&gcn_q->rec0);
                        continue;                
                    }
                    if (v1DecryptErr)
                    {
                        if (verboseArg)
                            SIprintf("Cannot decrypt password from " \
                                "vnode %s\n", gcn_q->rec0.gcn_obj);
                        dcryptFail++;
                        MEcopy((PTR)&rec0, sizeof(GCN_DB_REC0), 
                            (PTR)&gcn_q->rec0);
                        continue;
                    }
                }
                merge_q = (GCN_QUEUE *)MEreqmem(0, sizeof(GCN_QUEUE),0,NULL);
                if (!merge_q)
                {
                    SIprintf("iicvtgcn: Cannot allocate memory, exiting\n");
                    goto cvt_exit;
                }

                MEcopy((PTR)&gcn_q->rec0, sizeof(GCN_DB_REC0), 
                    (PTR)&merge_q->rec0);
                total_recs++;
              
                QUinsert(&merge_q->q, &merge_qhead);
            } /* for (q = gcn_qhead.q_prev; q != &gcn_qhead; q = q->q_prev) */

            if (dcryptFail && verboseArg && isLogin)
            {
                if (clustered || unclusterArg )
                    SIprintf("\n%d vnode(s) could not be decrypted.\n" \
                        "Probably some login entries were created on " \
                        "another node.\nTry executing iicvtgcn on another " \
                        "node to merge the other node's entries.\n\n", 
                        dcryptFail);
                    else
                        SIprintf("\n%d vnode(s) could not be decrypted.\n" \
                            "Probably the login file was created on " \
                            "another host.\nTry executing iicvtgcn on " \
                            "a different host.\n\n", dcryptFail);
            }
            if (verboseArg)
                SIprintf("Total records merged: %d\n", total_recs);

            /*
            ** If no records to merge, clean up and continue.
            */
            if (!total_recs)
            {
                cleanup_queues();
                continue;
            }

            status = SIfseek(dstFile, (i4)0, SI_P_START);
            active_rec = 0;
            for (q = merge_qhead.q_prev; q != &merge_qhead; q = q->q_prev)
            {
                merge_q = (GCN_QUEUE *)q;
                if (verboseArg)
                   SIprintf("Rewriting %s record vnode %s val %s\n", 
                       !STcasecmp("*", merge_q->rec0.gcn_uid) ? "global" : 
                       "private", merge_q->rec0.gcn_obj, merge_q->rec0.gcn_val);
                if (merge_q->rec0.gcn_tup_id == -1)
                    continue;
                status = SIwrite(sizeof( GCN_DB_REC0 ), 
                    (char *)&merge_q->rec0, &count, dstFile );
                if ( status != OK)
                {
                    SIprintf( "iicvtgcn: Failed to write file %s, " \
                        "status is %x\n", srcbuf, status );
                    goto cvt_exit;
                }
                active_rec++;
            } /* for (q = merge_qhead.q_prev; q != &merge_qhead; q = q->q_prev) */
        
            /*
            ** Write new EOF record.
            */
            rec0.gcn_tup_id = -1;
            rec0.gcn_l_uid = 0;
            rec0.gcn_uid[0] = '\0';
            rec0.gcn_l_obj = 0;
            rec0.gcn_obj[0] = '\0';
            rec0.gcn_l_val = 0;
            rec0.gcn_val[0] = '\0';
            rec0.gcn_invalid = TRUE;
            offset = active_rec * sizeof(GCN_DB_REC0);
            status = SIfseek(dstFile, (i4)offset, SI_P_START);
            status = SIwrite(sizeof( GCN_DB_REC0 ), (PTR)&rec0, 
                &count, dstFile );
        
        } /* if rewrite */
        else
        {
            for (q = gcn_qhead.q_prev; q != &gcn_qhead; q = q->q_prev)
            {
                gcn_q = (GCN_QUEUE *)q;
                gcn_q->rec0.gcn_l_val = STlength(gcn_q->rec0.gcn_val);
                if (verboseArg)
                   SIprintf("Writing %s record vnode %s val %s\n", 
                       !STcasecmp("*", gcn_q->rec0.gcn_uid) ? "global" : 
                           "private", gcn_q->rec0.gcn_obj, 
                               gcn_q->rec0.gcn_val);
                status = SIwrite(sizeof( GCN_DB_REC0 ), 
                    (char *)&gcn_q->rec0, &count, dstFile );
                if ( status != OK)
                {
                    SIprintf( "iicvtgcn: Failed to write file %s, " \
                        "status is %x\n", srcbuf, status );
                    goto cvt_exit;
                }
            }
        }  /* else if (!rewrite) */

        cleanup_queues();

        SIclose( srcFile );
        srcOpened = FALSE;
        SIclose( dstFile );
        dstOpened = FALSE;

        cleanup_queues();
    }

cvt_exit:

    if (srcOpened)
        SIclose(srcFile);
    if (dstOpened)
        SIclose(dstFile);
}

/*
** Name: usage
**
** Description:
**      Display info about iicvtgcn syntax.
**
** Input:    None.
**
** Output:   None.
**
** Returns:  Void.
**
** History:
**      15-Nov-2010 (Ralph Loen)
**          Created.
*/

void usage()
{
    SIprintf("Usage: iicvtgcn [ -h | --h | -help | --help ] " \
        "[ -v ] [ -c | -u ]\n\n");

    SIprintf("-h, --h, -help, --help\tDisplays this help page\n");
    SIprintf("-v\t\t\tVerbose   - Display detailed output\n");
    SIprintf("-c\t\t\tCluster   - Convert unclustered to clustered " \
        "\n\t\t\t(merge files)\n");
    SIprintf("-u\t\t\tUncluster - Convert clustered to unclustered " \
        "\n\t\t\t(unmerge files)\n");
    SIprintf("\t\t\tnodeName - The name of a non-local host " \
        "\n\t\t\tas it appears in the config.dat file\n");

    PCexit(OK);
}
/*
** Name: gcn_merge_rec
**
** Description:
**      See if a GCN record qualifies for merging.
**
** Input:
**      gcn_q      A GCN DB record in a queue.
**      isLogin    TRUE if source file is a login file, FALSE otherwise.
**
** Output:  None.
**
** Returns:
**      TRUE if record qualifies for merging, FALSE if not
**
** History:
**      15-Nov-2010 (Ralph Loen)
**          Created.  Based on gcn_tup_compare().
*/

bool gcn_merge_rec( GCN_QUEUE *gcn_q, bool isLogin )
{
    bool add_record;
    GCN_QUEUE *merge_q;
    QUEUE *q;

    /*
    ** Invalid records don't get added.
    */
     if (gcn_q->rec0.gcn_tup_id == -1)
        return FALSE;
    if (gcn_q->rec0.gcn_invalid)
        return FALSE;
    if ( !gcn_q->rec0.gcn_uid[0])
        return FALSE; 
    if ( !gcn_q->rec0.gcn_l_uid)
        return FALSE; 
    if ( !gcn_q->rec0.gcn_obj[0])
        return FALSE; 
    if ( !gcn_q->rec0.gcn_l_obj)
        return FALSE; 

    /*
    ** Assume a valid record that doesn't match.
    */
    add_record = TRUE;
    for (q = merge_qhead.q_prev; q != &merge_qhead; q = q->q_prev)
    {
        merge_q = (GCN_QUEUE *)q;

        /*
        ** EOF of merge queue reached.
        */
        if (merge_q->rec0.gcn_tup_id == -1)
            break;

        /*
        ** Records marked as delete (invalid) don't get merged. Search
        ** the next record.
        */
        if (merge_q->rec0.gcn_invalid)
            continue;

        /*
        ** Compare the uid and obj values.  No match means the record can 
        ** potentially be added.  Search the next record.
        */
        if (STncasecmp( gcn_q->rec0.gcn_uid, 
            merge_q->rec0.gcn_uid, STlength(gcn_q->rec0.gcn_uid)))
        {
            if (verboseArg)
                SIprintf("source uid <%s> != merge uid <%s>\n",
                    gcn_q->rec0.gcn_uid, merge_q->rec0.gcn_uid);
            continue;
        }
        if (verboseArg)
            SIprintf("source uid <%s> matches merge uid <%s>\n",
                gcn_q->rec0.gcn_uid, merge_q->rec0.gcn_uid);

        if (STncasecmp( gcn_q->rec0.gcn_obj, merge_q->rec0.gcn_obj, 
             STlength(gcn_q->rec0.gcn_obj)))
        {
            if (verboseArg)
                SIprintf("source obj <%s> != merge obj <%s>\n",
                    gcn_q->rec0.gcn_obj, merge_q->rec0.gcn_obj);
            continue;
        }
        if (verboseArg)
            SIprintf("source obj <%s> matches merge obj <%s>\n",
                gcn_q->rec0.gcn_obj, merge_q->rec0.gcn_obj);

        /*
        ** If this is a login file, and the uid and obj (global/private and
        ** vnode name) entries match, don't merge.
        */
        if (isLogin)
            return FALSE;

        if ( gcn_q->rec0.gcn_val[0] )
        {
            char        tmp1[ GCN_VAL_MAX_LEN + 1 ];
            char        tmp2[ GCN_VAL_MAX_LEN + 1 ];
            char        *pv1[ NBR_SUBFIELDS ], *pv2[ NBR_SUBFIELDS ];
            i4          v1, v2;
                
            /*
            ** Extract sub-fields.
            */
            v1 = gcu_words( gcn_q->rec0.gcn_val, tmp1, pv1, ',', 
                NBR_SUBFIELDS );
            v2 = gcu_words( merge_q->rec0.gcn_val, tmp2, pv2, ',', 
                NBR_SUBFIELDS );
                
            /*
            ** Limit comparison to requested number of sub-fields
            ** Make sure there are enough sub-fields for comparison.
            */
            if ( v2 > NBR_SUBFIELDS )  v2 = NBR_SUBFIELDS;
            if ( v1 < v2 )
                continue;
                
            /*
            ** Compare the sub-fields.
            */
            while( v2-- )
            {
                if ( *pv2[v2]  &&  
                    !STncasecmp(pv1[v2], pv2[v2], STlength(pv2[v2])))
                {
                    add_record = FALSE;
                    if (verboseArg)
                        SIprintf("subfield %s matches subfield %s\n", pv1[v2],
                           pv2[v2]);
                    break;
                }
                else if (verboseArg)
                    SIprintf("subfield %s != subfield %s\n", pv1[v2], pv2[v2]);
            }
           
            /*
            ** Nothing matched, so search the next record.
            */
            if (add_record)
                continue;
           
            /*
            ** Everything matched.  Don't add the record and stop
            ** searching.
            */
            add_record = FALSE;
            break;
        }
    } /* for (q = merge_qhead.q_prev; q != &merge_qhead; q = q->q_prev) */

    if (verboseArg)
    {
        if (add_record)
           SIprintf("Merging vnode %s\n", gcn_q->rec0.gcn_obj);
        else
           SIprintf("Vnode %s rejected for merging\n", gcn_q->rec0.gcn_obj);
    }
    return add_record;
}

/*
** Name: gcn_recrypt
**
** Description:
**      Decrypt password and re-encrypt password, possbily in new version
**      format. 
**
** Input:
**      clustered          Indicates cluster-style output
**      local_mask         Encryption mask
**      gcn_val            login info
**
** Output:
**      v1DecryptErr       Indicates an error other than GCN function error
**      writeOutput        TRUE means result should be written
**
** Returns:
**      status             Status from GCN encryption/decryption routines
**
** History:
**      15-Nov-2010 (Ralph Loen)
**          Created.
*/

STATUS gcn_recrypt( bool clustered, u_i1 *local_mask, char *gcn_val, 
    bool *v1DecryptErr, bool *writeOutput)
{
    i2 pc;
    char *pv[ 3 ];
    STATUS status = OK;
    char *p;
    char pwd[GC_L_VAL];
    i2 j;
    bool printable;

    *v1DecryptErr = FALSE;

    pc = gcu_words( gcn_val, NULL, pv, ',', 3 );
    if (pc < 2 )  
        pv[1] = "";
    if (pc < 3 )  
        pv[2] = "";

    if (!STcasecmp(pv[2],"V0") || (pc < 3))
        status = gcn_decrypt( pv[0], pv[1], pwd );
    else
        status = gcn_decode( pv[0],(u_i1*)local_mask, pv[1], pwd );
    
    if (status != OK)
    {
        if (!STcasecmp(pv[2],"V1"))
            *v1DecryptErr = TRUE;
        goto end_routine;      
    }
    if (!STlength(pwd))
    {
        if (!STcasecmp(pv[2],"V1"))
            *v1DecryptErr = TRUE;
        status = FAIL;
        goto end_routine;
    }
    p = &pwd[0];
    printable = TRUE;
    for (j = 0; j < STlength(pwd); j++)
    {
        if (!CMprint(p))
        {
            printable = FALSE;
            break;
        }
        CMnext(p);
    }
    if (!printable)
    {
         if (!STcasecmp(pv[2],"V1"))
             *v1DecryptErr = TRUE;
         status = FAIL;
         goto end_routine;
    }

    if (clustered)
    {
        if (!STncasecmp("V1", pv[2], STlength(pv[2])))
            *writeOutput = TRUE;
        status = gcu_encode( pv[0], pwd, pv[1] );
        STpolycat( 5, pv[0], ",", pv[1], ",", "V0", gcn_val );
    }
    else
    {
        status = gcn_encode( pv[0],(u_i1*)local_mask, pwd, pv[1] );
        STpolycat( 5, pv[0], ",", pv[1], ",", "V1", gcn_val );
    }

end_routine:
     return status;
}

/*
** Name: cleanup_queues
**
** Description:
**      Empty reamaining info in the merge and GCN queues.
**
** Input:
**
** Output:
**
** Returns:
**      VOID
**
** History:
**      15-Nov-2010 (Ralph Loen)
**          Created.
*/
void cleanup_queues()
{
    QUEUE *q;

    /*
    ** Empty information remaining in queues, if any.
    */
    for (q = gcn_qhead.q_prev; q != &gcn_qhead; q = gcn_qhead.q_prev)
    {
        QUremove(q);
        MEfree((PTR)q);
    }

    for (q = merge_qhead.q_prev; q != &merge_qhead; q = merge_qhead.q_prev)
    {
        QUremove(q);
        MEfree((PTR)q);
    }
}

/*
** Name: gcn_get_rec0
**
** Description:
**      Extract components of a V1 DB record.  Input record
**      may be corrupted as a result of processing.
**
** Input:
**      record      V1 DB record buffer.
**
** Output:
**      gcn_uid     global/private indicator.
**      gcn_obj     vnode.
**      gcn_val     login/password, connection info, or attribute info.
**
** Returns:
**      status  OK or error code.
**
** Side-Effects:
**      None.
**
** History:
**       12-Jan-2010 (Ralph Loen)
**          Created.
*/
bool gcn_get_rec0(GCN_DB_RECORD *rec, GCN_DB_REC0 *rec0)
{
    char gcn_uid[GC_L_UID] = { empty };
    char gcn_obj[GC_L_OBJ] = { empty };
    char gcn_val[GC_L_VAL] = { empty };
    bool ret = TRUE;

    if (rec->rec1.gcn_rec_type == GCN_DBREC_V0)    
    {
        MEcopy((PTR)rec, sizeof(GCN_DB_REC0), (PTR)rec0);
        goto end_routine;
    }
    if (rec->rec1.gcn_rec_type == GCN_DBREC_V1)    
    {
        ret = gcn_extract_rec1(&rec->rec1, gcn_uid, gcn_obj, gcn_val);
        if (!ret)
            goto end_routine;

        if (verboseArg)
             SIprintf("Extracted REC1 fields uid: %s obj: %s val: %s\n", 
                 gcn_uid, gcn_obj, gcn_val);

        rec0->gcn_tup_id = 0;

        rec0->gcn_l_uid = STlength(gcn_uid);
        STcopy(gcn_uid, rec0->gcn_uid);        

        rec0->gcn_l_val = STlength(gcn_val);
        STcopy(gcn_val, rec0->gcn_val);        

        rec0->gcn_l_obj = STlength(gcn_obj);
        STcopy(gcn_obj, rec0->gcn_obj);        
          
        rec0->gcn_obsolete = FALSE;

        rec0->gcn_invalid = FALSE;

    }
    else if (rec->rec1.gcn_rec_type == GCN_DBREC_DEL)
    {
        rec0->gcn_tup_id = 0;
        rec0->gcn_l_uid = 0;
        rec0->gcn_uid[0] = '\0';
        rec0->gcn_l_obj = 0;
        rec0->gcn_obj[0] = '\0';
        rec0->gcn_l_val = 0;
        rec0->gcn_val[0] = '\0';
        rec0->gcn_invalid = TRUE;
    }    
    else if (rec->rec1.gcn_rec_type == GCN_DBREC_EOF)    
    {
        rec0->gcn_tup_id = -1;
        rec0->gcn_l_uid = 0;
        rec0->gcn_uid[0] = '\0';
        rec0->gcn_l_obj = 0;
        rec0->gcn_obj[0] = '\0';
        rec0->gcn_l_val = 0;
        rec0->gcn_val[0] = '\0';
        rec0->gcn_invalid = TRUE;
    }
end_routine:
    return ret;    
}

/*
** Name: gcn_extract_rec1
**
** Description:
**      Extract components of a V1 DB record.  Input record
**      may be corrupted as a result of processing.
**
** Input:
**      record      V1 DB record buffer.
**
** Output:
**      gcn_uid     global/private indicator.
**      gcn_obj     vnode.
**      gcn_val     login/password, connection info, or attribute info.
**
** Returns:
**      status  OK or error code.
**
** Side-Effects:
**      None.
**
** History:
**       08-Dec-2010 (Ralph Loen) SIR 124815
**          Cloned from gcn_read_rec1.
*/

bool gcn_extract_rec1( GCN_DB_REC1 *record, char *gcn_uid, char *gcn_obj,
    char *gcn_val )
{
    bool        status = TRUE;
    u_i1        *ptr = (u_i1 *)record + sizeof( GCN_DB_REC1 );
    u_i1        *end = (u_i1 *)record + record->gcn_rec_len;
    char        g_uid[ GC_L_UID ];
    char        g_obj[ GC_L_OBJ ];
    char        g_val[ GC_L_VAL ];
    char        *uid = empty;
    char        *obj = empty;
    char        *val = empty;
    i4          uidlen = 0;
    i4          objlen = 0;
    i4          vallen = 0;
    i4          tot_len = 0;

    while( (ptr + sizeof( GCN_DB_PARAM )) <= end )
    {
        GCN_DB_PARAM    param;

        MEcopy( (PTR)ptr, sizeof( GCN_DB_PARAM ), (PTR)&param );

        if ( ! param.gcn_p_type )  break;
        ptr += sizeof( GCN_DB_PARAM );

        if ( (ptr + param.gcn_p_len) > end )
        {
            if (verboseArg)
                SIprintf( "value length exceeds record length: %d\n",
                      param.gcn_p_len );
            status = FALSE;
            goto end_routine;
        }

        switch( param.gcn_p_type )
        {
        case GCN_P_UID :
            uid = (char *)ptr;
            uidlen = param.gcn_p_len;
            break;

        case GCN_P_OBJ :
            obj = (char *)ptr;
            objlen = param.gcn_p_len;
            break;

        case GCN_P_VAL :
            val = (char *)ptr;
            vallen = param.gcn_p_len;
            break;

        case GCN_P_TID :
            if ( param.gcn_p_len != sizeof( i4 ) )
            {
                if (verboseArg)
                    SIprintf( "invalid TID size %d\n", param.gcn_p_len );
                status = FALSE;
                goto end_routine;
            }

            break;

        default :
            if (verboseArg)
                SIprintf( "invalid value ID %d\n", param.gcn_p_type );
            status = FALSE;
            goto end_routine;
        }

        tot_len += param.gcn_p_len;
        ptr += param.gcn_p_len;
    }

    /*
    ** Allow for non-terminated parameter strings.
    */
    if (uid[uidlen] != EOS)
        uid[uidlen] = EOS;

    if (obj[objlen] != EOS)
        obj[objlen] = EOS;

    if (val[vallen] != EOS)
        val[vallen] = EOS;

    MEcopy((PTR)uid, uidlen+1, (PTR)gcn_uid);
    MEcopy((PTR)obj, objlen+1, (PTR)gcn_obj);
    MEcopy((PTR)val, vallen+1, (PTR)gcn_val);

end_routine:
    return status;
}
/*
** Name: gcn_rewrite_rec1
**
** Description:
**	Rewrites a V1 DB record from an existing login record.  
**      Adds parameters for the record components.  If tuple ID is not 0, 
**      also adds a parameter for the tuple ID.
**
**	The formatted record size is checked against the record length.
**
** Input:
**	uid     global or private indicator.
**	obj     vnode.
**	val     login/password, connection info, or attribute info.
**
** Output:
**	record	Formatted V1 record.
**	
** Returns:
**	STATUS	FAIL if formatted components exceed record length.
**
** History:
**	 08-Dec-10 (Ralph Loen) SIR 124815
**	    Cloned from gcn_write_rec1.
*/
STATUS
gcn_rewrite_rec1( char *uid, char *obj, char *val, GCN_DB_REC1 *record )
{
    GCN_DB_PARAM	param;
    GCN_DB_RECORD       tmp_rec;
    u_i1                *ptr = (u_i1 *)&tmp_rec + sizeof( GCN_DB_REC1 );
    i4			uidlen = STlength( uid ) + 1;
    i4			objlen = STlength( obj ) + 1;
    i4			vallen = STlength( val ) + 1;
    i4			size = sizeof( GCN_DB_REC1 ) + 
    			       (sizeof( GCN_DB_PARAM ) * 3) + 
			       uidlen + objlen + vallen;

    MEfill(sizeof(GCN_DB_RECORD), 0, (PTR)&tmp_rec);

    MEcopy((PTR)&record->gcn_rec_type, sizeof(i4), 
        (PTR)&tmp_rec.rec1.gcn_rec_type);
    MEcopy((PTR)&record->gcn_rec_len, sizeof(i4), 
        (PTR)&tmp_rec.rec1.gcn_rec_len);

    if ( size > record->gcn_rec_len )
    {
	if ( verboseArg)  
	    SIprintf( "Formatted record size %d exceeds record length %d\n", 
		       size, record->gcn_rec_len );
    	return( FAIL );
    }
    
    param.gcn_p_type = GCN_P_UID;
    param.gcn_p_len = uidlen;
    MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
    ptr += sizeof( param );
    MEcopy( (PTR)uid, uidlen, (PTR)ptr );
    ptr += uidlen;

    param.gcn_p_type = GCN_P_OBJ;
    param.gcn_p_len = objlen;
    MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
    ptr += sizeof( param );
    MEcopy( (PTR)obj, objlen, (PTR)ptr );
    ptr += objlen;

    param.gcn_p_type = GCN_P_VAL;
    param.gcn_p_len = vallen;
    MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
    ptr += sizeof( param );
    MEcopy( (PTR)val, vallen, (PTR)ptr );
    ptr += vallen;
    
    MEcopy( (PTR)&tmp_rec, sizeof(tmp_rec), (PTR)record);
    return( OK );
}
