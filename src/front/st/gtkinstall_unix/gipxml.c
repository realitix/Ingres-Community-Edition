/*
** Copyright 2007 Actian Corporation. All rights reserved.
*/

# include <compat.h>
# include <gl.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <st.h>

# if defined( xCL_GTK_EXISTS )
# include <gtk/gtk.h>

# include <gip.h>
# include <gipdata.h>
# include <giputil.h>

/* libXML headers for parsing instance info */
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/*
** Name: gipxml.c
**
** Description:
**	
**	Parse XML file generated by python preinstallation script contain
**	info on all Ingres instances found on host.
**	Use this to populate "instances" structure.
**
** History:
**	03-Sep-2007 (hanje04)
**	    Created.
**	01-Mar-2010 (hanje04)
**	    BUG 123358
**	    Make sure startup mode is set correctly when only "ignored"
**	    instances are found (greater in version than saveset)
**	20-May-2010 (hanje04)
**	    SIR 123791
**	    Retrieve new 'product' attribute from 'saveset' element. Used
**	    to determin execution mode.
**	16-Feb-2011 (hanje04)
**	    Don't force rename if non-renamed instance has action == UM_FALSE.
**	    i.e. it's not the same product
**	18-Feb-2011 (hanje04)
**	    Backout "no-rename" change, breaks renaming for new instances.
**	    need to re-think.
**	18-May-2011 (hanje04)
**	    Add support for DEBs packaging and flag restrictions (no rename,
**	    one instance).
**	    Replace STprintf() with snprintf() to reduce risk of buffer 
**	    overflow.
**	    Improve tracing and error handling.
**	21-Jun-2011 (hanje04)
**	    Mantis 2135 & 2138
**	    Add "checksig" attribute to "saveset" to toggle sig checking
**	04-Oct-2011 (hanje04)
**	    Bug 125688
**	    Add gipParseJREInfo to extract location of JRE for remote
**	    management service.
*/


/*
** Name: gipParseInstance()
**
** Description:
**
**	Extract instance data from XML DOM nodes and use them to populate
**	"instances" structure.
**
** input:
**	doc - Pointer to XML document containing instance info
**	cur - Pointer to instance element in XML doc 
**	cur_inst - Pointer to instance structure allocated for storing info
*/
static STATUS
gipParseInstance( xmlDocPtr doc, xmlNodePtr cur, instance *inst, UGMODE *state)
{
    xmlChar	*ele_text;
    STATUS	status;

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
	char		tmpbuf[MAX_LOC];
	char		inst_id[3];

	if ( ! xmlStrcmp( cur->name, (const xmlChar *)"action" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( ! xmlStrcmp( ele_text, (const xmlChar *)"modify" ) )
	    {
		inst->action |= UM_MOD;
	        *state |= UM_TRUE|UM_MOD;
	    }
	    else if ( ! xmlStrcmp( ele_text, (const xmlChar *)"upgrade" ) )
	    {
		inst->action |= UM_UPG;
	        *state |= UM_TRUE|UM_UPG;
	    }
	    else 
		inst->action = UM_FALSE;

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"renamed" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( ! xmlStrcmp( ele_text, (const xmlChar *)"True" ) )
		if ( new_pkgs_info.pkgfmt == PKGDEB )
		{
		    DBG_PRINT("DEB instance marked as renamed, not supported\n");
		    return(FAIL);
		}
		else
		    inst->action |= UM_RENAME;
	    else if ( new_pkgs_info.pkgfmt == PKGRPM )
	    {
		/*
 		** non-renamed instance so any new install must rename
 		** bump the instance ID too
 		*/
		*state |= UM_RENAME;
		dfltID[1] = '1';
	    }
	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"basename" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < MAX_REL_LEN )
	    {
		snprintf( tmpbuf, MAX_LOC, "%s", ele_text );
		STcopy( tmpbuf, inst->pkg_basename );
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"ID" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < 3 )
	    {
		snprintf( tmpbuf, MAX_LOC, "Ingres %s", ele_text );
		STcopy( tmpbuf, inst->instance_name );
		inst->instance_ID = inst->instance_name +
						STlen(tmpbuf) - 2;
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"version" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < MAX_VERS_LEN )
	    {
		snprintf( tmpbuf, MAX_LOC, "%s", ele_text );
		STcopy( tmpbuf, inst->version );
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"location" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < MAX_LOC )
	    {
		snprintf( tmpbuf, MAX_LOC, "%s", ele_text );
		STcopy( tmpbuf, inst->inst_loc );
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"package" ) )
	{
	    xmlChar	*attrib;

	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    attrib = xmlGetProp(cur, "idx"); /* get the attribute */
	    inst->installed_pkgs |= packages[atoi(attrib)]->bit;

	    xmlFree( ele_text );
	    xmlFree( attrib );
	}
	else if ( ! xmlStrncmp( cur->name, (const xmlChar *)"II_", 3 ) )
	{
	    xmlChar	*attrib;
	    i4		idx;

	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    attrib = xmlGetProp(cur, "idx"); /* get the attribute */
	    idx = atoi(attrib);
	  
	    status = GIPAddDBLoc(inst, idx, ele_text);

	    xmlFree( ele_text );
	    xmlFree( attrib );

	    if (status != OK)
	    {
		DBG_PRINT("Error adding %s\n", cur->name);
		return(status);
	    }
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"primarylog" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    status = GIPAddLogLoc(inst, 0, ele_text);

	    xmlFree( ele_text );

	    if (status != OK)
	    {
		DBG_PRINT("Error adding %s\n", cur->name);
		return(status);
	    }
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"duallog" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    status = GIPAddLogLoc(inst, 1, ele_text);

	    xmlFree( ele_text );

	    if (status != OK)
	    {
		DBG_PRINT("Error adding %s\n", cur->name);
		return(status);
	    }
	}
	cur = cur->next;
    }

    return(OK);
}
  
/*
** Name: gipParseInstances()
**
** Description:
**
**	Import generated XML doc, validate it against the DTD and
**	extract instance info about all existing "instances" with
**	gipParseInstance().
**
** History:
**	03-Sep-2007 (hanje04)
**	    Created.
**	01-Mar-2010 (hanje04)
**	    BUG 123358
**	    Make sure startup mode is set correctly when only "ignored"
*	    instances are found (greater in version than saveset)
*/
static STATUS
gipParseInstances( xmlDocPtr doc, xmlNodePtr cur, UGMODE *state, i4 *count )
{
    i4 inst_count = 0;
    i4 skp_count = 0;

    cur = cur->xmlChildrenNode;

    while (cur != NULL)
    {
	instance	*cur_inst;
	i4		inst_idx;
	u_i2		memtag;
	STATUS		mestat;

	inst_count++;
	inst_idx = inst_count - 1;

	/*
	** Get tag for each instance, request the memory
	** and store the tag
	*/
	memtag = MEgettag();
	cur_inst= existing_instances[inst_idx] = (instance *)MEreqmem( memtag,
                                                    sizeof(instance),
                                                    TRUE,
                                                    &mestat );
	cur_inst->memtag = memtag ;

	/* return error if we failed to get memory */
       	if ( mestat != OK )
	{
	    DBG_PRINT("Error allocating memory\n");
	    return( mestat );
	}

	/* initialize package info */
	if ( selected_instance == NULL )
	    selected_instance = *existing_instances ;

	if ( ! xmlStrcmp( cur->name, (const xmlChar *)"instance" ) )
	    if ( gipParseInstance( doc, cur, cur_inst, state ) != OK )
	    {
	 	DBG_PRINT("Error occurred parsing instance: %s\n",
						 cur_inst->instance_ID);
		return(FAIL);
	    }

	if ( cur_inst->action == UM_FALSE )
	    skp_count++;

	cur = cur->next;
    }

    if ( new_pkgs_info.pkgfmt == PKGDEB ) 
    {
	/* options greatly restricted for DEBs */
	if ( skp_count > 0 )
	{
	    /* shouldn't get here as ingpkgqry should flag it and abort */
	    DBG_PRINT("Newer instance found, multiple instances not supported for DEBs.\nCannot continue...\n");
	    return(FAIL); /* newer instance instance */
 	}
	if ( inst_count == 0 )
	{
	    /*
            ** No existing installations found so force
            ** new installation mode
            */
            *state |= UM_INST;
            ug_mode |= UM_INST;
	}
    }
    else if ( inst_count - skp_count == 0 )
    {
	/*
        ** No existing installations found so force
        ** new installation mode
        */
        *state |= UM_INST;
        ug_mode |= UM_INST;
    }
    else if (inst_count - skp_count > 1)
	*state |= UM_MULTI;
 
    *count=inst_count;

    return(OK);
}


/*
** Name: gipParseSaveSet()
**
** Description:
**	
**	Extract save set data from XML DOM nodes and use the to populate
**	new_pkgs_info structure.
**
** input:
**	doc - Pointer to XML document containing instance info
**	cur - Pointer to saveset element in XML doc 
**
** History:
**	04-Sep-2007 (hanje04)
**	    Created.
*/
static STATUS
gipParseSaveSet( xmlDocPtr doc, xmlNodePtr cur )
{
    xmlChar *ele_text;
    xmlChar	*attrib;

    attrib = xmlGetProp(cur, "product"); /* ingres or vectorwise */
    STlcopy(attrib, new_pkgs_info.product, MAX_REL_LEN);
    DBG_PRINT("Parsing saveset: product=%s\n",new_pkgs_info.product);

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
	char tmpbuf[MAX_LOC];

	if ( ! xmlStrcmp( cur->name, (const xmlChar *)"basename" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < MAX_REL_LEN )
	    {
		snprintf( tmpbuf, MAX_LOC, "%s", ele_text );
		STcopy( tmpbuf, new_pkgs_info.pkg_basename );
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"version" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < MAX_VERS_LEN )
	    {
		snprintf( tmpbuf, MAX_LOC, "%s", ele_text );
		STcopy( tmpbuf, new_pkgs_info.version );
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"arch" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STlen( ele_text ) < MAX_ARCH_LEN )
	    {
		snprintf( tmpbuf, MAX_LOC, "%s", ele_text );
		STcopy( tmpbuf, new_pkgs_info.arch );
	    }

	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"format" ) )
	{
	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    if ( STcompare( ele_text, "rpm" ) == OK )
	    {
		new_pkgs_info.pkgfmt=PKGRPM;
		STcopy(ele_text, new_pkgs_info.pkgdir); 
	    }
	    else if ( STcompare( ele_text, "deb" ) == OK )
	    {
		new_pkgs_info.pkgfmt=PKGDEB;
		STcopy("apt", new_pkgs_info.pkgdir);
	    }
	    else
	    {
		new_pkgs_info.pkgfmt=PKGUDEF;
		DBG_PRINT("Undefined package type: %s\n", ele_text);
		xmlFree( ele_text );
		return(FAIL);
	    }
	    xmlFree( ele_text );
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"location" ) )
	{
	    /* skip this one, new_pkgs_info.file_loc is set at startup */
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"package" ) )
	{
	    u_char	idxstr[5];

	    ele_text = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    attrib = xmlGetProp(cur, "idx"); /* get the attribute */
	    new_pkgs_info.pkgs |= packages[atoi(attrib)]->bit;

	    xmlFree( ele_text );
	    xmlFree( attrib );
	}
	cur = cur->next;
    }

    return(OK);
}

/*
** Name: gipParseJREInfo()
**
** Description:
**	
**	Load JRE location and version info into jre_info structure
**
** input:
**	doc - Pointer to XML document containing java info
**	cur - Pointer to java element in XML doc 
**
** History:
**	27-Sep-2011 (hanje04)
**	    Created.
*/
static STATUS
gipParseJREInfo( xmlDocPtr doc, xmlNodePtr cur )
{
    xmlChar	*attrib;

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
	if ( ! xmlStrcmp( cur->name, (const xmlChar *)"jre" ) )
	{
	    attrib = xmlGetProp(cur, "JAVA_HOME"); /* get the attribute */
	    strncpy(javainfo.home, attrib, MAX_LOC);
	    DBG_PRINT("javainfo.home=%s, ", attrib);

	    xmlFree( attrib );
	    attrib = xmlGetProp(cur, "version"); /* get the attribute */
	    strncpy(javainfo.version, attrib, MAXJVLEN);
	    DBG_PRINT("javainfo.version=%s\n ", attrib);

	    xmlFree( attrib );
	    attrib = xmlGetProp(cur, "bits"); /* get the attribute */
	    javainfo.bits=atoi(attrib);
	    DBG_PRINT("javainfo.bits=%d\n ", javainfo.bits);

	    xmlFree( attrib );

	    /* only care about the first entry for now */
	    break;
	}
	cur = cur->next;
    }

    return(OK);
}

/*
** Name: gipImportInstXML()
**
** Description:
**
**	Import generated XML doc containing save set and instance info.
**
** History:
**	05-Sep-2007 (hanje04)
**	    Created.
**	21-Jun-2011 (hanje04)
**	    Mantis 2135 & 2138
**	    Add "checksig" attribute to "saveset" to toggle sig checking
**	
*/
STATUS
gipImportPkgInfo( LOCATION *xmlfileloc, UGMODE *state, i4 *count )
{
    char	*xmlfilestr = NULL;
    xmlDocPtr	doc;
    xmlNodePtr	cur;

    /* sanity check */
    if ( LOexist( xmlfileloc ) != OK )
	return( FAIL );

    /* get location string */
    LOtos( xmlfileloc, &xmlfilestr );

    /* load instance/saveset document */
    doc = xmlParseFile( xmlfilestr );
    if ( doc == NULL )
	return( FAIL );

    /* load first element */
    cur = xmlDocGetRootElement( doc );

    /* basic validation */
    /* FIX ME, NEED DTD!!! */
    if ( cur == NULL ||
	    xmlStrcmp( cur->name, (const xmlChar *) "IngresPackageInfo" ) )
    {
	xmlFreeDoc( doc );
	return( FAIL );
    }

    /* walk the tree */
    cur = cur->xmlChildrenNode;
    while ( cur != NULL )
    {
	if ( ! xmlStrcmp( cur->name, (const xmlChar *)"saveset" ) )
	{
	    xmlChar	*attrib;

	    if ( gipParseSaveSet( doc, cur ) != OK )
	    {
		DBG_PRINT("Error parsing saveset info\n");
		return( FAIL );
	    }

	    attrib = xmlGetProp(cur, "checksig"); /* get the attribute */
	    new_pkgs_info.checksig = ( xmlStrcmp(attrib, "True") == OK );

	    xmlFree( attrib );

	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"instances" ) )
	{
	    if ( gipParseInstances( doc, cur, state, count ) != OK )
	    {
		DBG_PRINT("Error parsing instance info\n");
		return( FAIL );
	    }
	}
	else if ( ! xmlStrcmp( cur->name, (const xmlChar *)"java" ) )
	{
	    if ( gipParseJREInfo( doc, cur ) != OK )
	    {
		DBG_PRINT("Error parsing JRE info\n");
		return( FAIL );
	    }
	}
	cur = cur->next;
    }

    xmlFreeDoc( doc );
    return( OK );
}

# endif /* xCL_GTK_EXISTS */
