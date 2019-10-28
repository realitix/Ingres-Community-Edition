/*
**	Copyright (c) 2004 Actian Corporation
*/

#include <compat.h>
#include <gl.h>
#include <iicommon.h>

#include <cs.h>
#include <mu.h>
#include <gc.h>
#include <lo.h>
#include <er.h>
#include <st.h>
#include <pm.h>
#include <sl.h>
#include <gca.h>


/*
**  Name:chkpriv.c
**
**  Description:
**      Contains the following functions:
**
**          chk_priv - check a user's level of privilege
**
**  History:   
**          
**      19-Jun-1998 (horda03)
**          Created.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/*{
**
**  Name: chk_priv - validate a given user's ownership of a given privilege
**
**  Description:
**      Calls PM to determine the user's allotted privileges, then
**      checks whether the specific required privilege is among them.
**
**  Inputs:
**      usr  -  user to validate
**      priv -  name of required privilege
**
**  Returns:
**      OK   - user has required privilege.
**      E_GC003F_PM_NOPERM - user does not have required privilege.
**
**  History:
**      19-Jun-1998 (horda03)
**          Created.
**/

STATUS
chk_priv( char *user_name, char *priv_name )
{
        char    pmsym[128], *value, *valueptr ;
        char    *strbuf = 0;
        int     priv_len;

        /* 
        ** privileges entries are of the form
        **   ii.<host>.privileges.<user>:   SERVER_CONTROL, NET_ADMIN ...
        */

        STpolycat(2, "$.$.privileges.user.", user_name, pmsym);

        /* check to see if entry for given user */
        /* Assumes PMinit() and PMload() have already been called */

        if( PMget(pmsym, &value) != OK )
            return E_GC003F_PM_NOPERM;

        valueptr = value ;
        priv_len = STlength(priv_name) ;

        /*
        ** STindex the PM value string and compare each individual string
        ** with priv_name
        */
        while ( *valueptr != EOS && (strbuf=STindex(valueptr, "," , 0)))
        {
            if (!STscompare(valueptr, priv_len, priv_name, priv_len))
                return OK ;

            /* skip blank characters after the ','*/
            valueptr = STskipblank(strbuf+1, 10); 
        }

        /* we're now at the last or only (or NULL) word in the string */
        if ( *valueptr != EOS  && 
              !STscompare(valueptr, priv_len, priv_name, priv_len))
                return OK ;

        return E_GC003F_PM_NOPERM;
}
