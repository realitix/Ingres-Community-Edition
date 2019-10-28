/*
** Copyright 2011 Actian Corporation. All rights reserved.
*/

# include <compat.h>

# include <gl.h>
# include <st.h>
# include <lo.h>
# include <si.h>
# include <cm.h>
# include <cv.h>

# if defined( xCL_GTK_EXISTS )
# include <gtk/gtk.h>

# include <gip.h>
# include <gipdata.h>
# include <giputil.h>
# include <gipsysinfo.h>

/*
** Name: gipsysconf.c
**
** Description:
**
**      Module for getting and setting system parameters. Public functions
**	are:
**
**	    gipInitSysInfo()	- Initialize and populate sysinfo array
**	    gipGetSysPMem()	- Return amount of system physical memory
**	    gipGetSysFMem()	- Return amount of system free memory
**	    gipGetSysCpu()	- Return amount cpu cores on the system
**	    gipGetSysShmmax()	- Return the value for kernel.shmmax
**	    gipSetSysShmmax()	- Set the value for kernel.shmmax
**	    gipCheckVWMemReq()	- Compare VW memory requirements with physical
**				  memory
**	    
**
** History:
**	28-Jan-2011 (hanje04)
**	    Mantis 1303, 1348 & 1434
**	    Add santiy checking and upper limits to VW config parameters
**	    Created from functions in gipmain.c
**	16-Feb-2011 (hanje04)
**	    Mantis 1514
**	    Report the right value in the tracing when getting kernel.shmmax
**	25-Mar-2011 (hanje04)
**	    Mantis 1818
**	    Use fscanf() to convert the string value for shmmax to SIZE_TYPE.
**	    CVal8 can't handle u_i8 values and fscanf() can read directly from
**	    the file.
**	18-May-2011 (hanje04)
**	    Correct debug message formats and other misc changes to quiet
**	    complier warnings
**	    
*/

static STATUS get_sys_pmem(SIZE_TYPE *sysmemkb);
static STATUS get_sys_fmem(SIZE_TYPE *sysmemkb);
static STATUS get_num_cpu(i4 *cpus);
static STATUS get_sys_shmmax(SIZE_TYPE *shmmax);

enum{
    GIP_SYS_PHYSMEM,
    GIP_SYS_FREEMEM,
    GIP_SYS_SHMMAX,
    GIP_SYS_CPUS,
    GIP_MAX_SYSINFO
};
static SIZE_TYPE sysinfo[GIP_MAX_SYSINFO] = {0};
static bool sysinfinit = FALSE;


STATUS
gipInitSysInfo(bool update)
{
    SIZE_TYPE	sysval;
    STATUS	retval = OK;
    i2		i = 0;

    if ( sysinfinit == TRUE && update != TRUE )
	return(OK);
   
    if ( OK == get_sys_pmem(&sysval) )
	sysinfo[GIP_SYS_PHYSMEM] = sysval;
  
    if ( OK == get_sys_fmem(&sysval) )
	sysinfo[GIP_SYS_FREEMEM] = sysval;
   
    if ( OK == get_sys_shmmax(&sysval) )
	sysinfo[GIP_SYS_SHMMAX] = sysval;
   
    if ( OK == get_num_cpu((i4 *)&sysval) )
	sysinfo[GIP_SYS_CPUS] = sysval;

    while( i < GIP_MAX_SYSINFO )
	if ( sysinfo[i++] == 0 )
	    return(FAIL);

    sysinfinit = TRUE;
    return(OK);
}

STATUS
gipGetSysPMem(SIZE_TYPE *syspmemkb)
{
    *syspmemkb = sysinfo[GIP_SYS_PHYSMEM];

    return( *syspmemkb > 0 ? OK : FAIL );
}

static STATUS
get_sys_pmem(SIZE_TYPE *syspmemkb)
{
    char	meminfo[] = "/proc/meminfo";
# define MB_SIZE 29
    char	membuf[MB_SIZE];
    LOCATION	meminfo_loc;
    FILE	*meminfo_f;

    LOfroms(PATH|FILENAME, meminfo, &meminfo_loc);
    if (LOexist(&meminfo_loc) != OK)
	return FAIL;

    DBG_PRINT("Reading %s\n", meminfo);
    if (SIopen(&meminfo_loc, "r", &meminfo_f) != OK )
	return FAIL;

    while(SIgetrec(membuf, MB_SIZE, meminfo_f) != ENDFILE)
    {
	char *ptr;
	/*
        ** line we're looking for looks like this:
        ** MemTotal:         510264 kB
	*/
	if ( (ptr = STstrindex(membuf, "MemTotal:", 0, TRUE)) != NULL )
	{
	    char *memval;

	    /* find the value */
	    while(! CMdigit(ptr))
		ptr++;
	    memval=ptr;

	    /* find the end */
	    while(CMdigit(ptr))
		ptr++;
	    *ptr='\0';

	    /* pass it back */
	    *syspmemkb=atoi(memval);
	    DBG_PRINT("System memory detected as %sKb, returned as %ldKb\n",
			memval, *syspmemkb);
	}
    }
    SIclose(meminfo_f);
    return(OK);
}
    
STATUS
gipGetSysFMem(SIZE_TYPE *sysfmemkb)
{
    *sysfmemkb = sysinfo[GIP_SYS_FREEMEM];

    return( *sysfmemkb > 0 ? OK : FAIL );
}

static STATUS
get_sys_fmem(SIZE_TYPE *sysmemkb)
{
    char	meminfo[] = "/proc/meminfo";
# define MB_SIZE 29
    char	membuf[MB_SIZE];
    LOCATION	meminfo_loc;
    FILE	*meminfo_f;

    LOfroms(PATH|FILENAME, meminfo, &meminfo_loc);
    if (LOexist(&meminfo_loc) != OK)
	return FAIL;

    DBG_PRINT("Reading %s\n", meminfo);
    if (SIopen(&meminfo_loc, "r", &meminfo_f) != OK )
	return FAIL;

    while(SIgetrec(membuf, MB_SIZE, meminfo_f) != ENDFILE)
    {
	char *ptr;
	/*
        ** line we're looking for looks like this:
        ** MemFree:         510264 kB
	*/
	if ( (ptr = STstrindex(membuf, "MemFree:", 0, TRUE)) != NULL )
	{
	    char *memval;

	    /* find the value */
	    while(! CMdigit(ptr))
		ptr++;
	    memval=ptr;

	    /* find the end */
	    while(CMdigit(ptr))
		ptr++;
	    *ptr='\0';

	    /* pass it back */
	    *sysmemkb=atoi(memval);
	    DBG_PRINT("Free memory detected as %sKb, returned as %ldKb\n",
			memval, *sysmemkb);
	}
    }
    SIclose(meminfo_f);
    return(OK);
}
    
STATUS
gipGetSysCpu(i4 *cpus)
{
    *cpus = sysinfo[GIP_SYS_CPUS];

    return( *cpus > 0 ? OK : FAIL );
}

static STATUS
get_num_cpu(i4 *cpus)
{
    char	cpuinfo[] = "/proc/cpuinfo";
# define BUF_SIZE 30
    char	buf[BUF_SIZE];
    LOCATION	cpuinfo_loc;
    FILE	*cpuinfo_f;
    i4		numcpu = 0;

    LOfroms(PATH|FILENAME, cpuinfo, &cpuinfo_loc);
    if (LOexist(&cpuinfo_loc) != OK)
	return FAIL;

    DBG_PRINT("Reading %s\n", cpuinfo);
    if (SIopen(&cpuinfo_loc, "r", &cpuinfo_f) != OK )
	return FAIL;

    while(SIgetrec(buf, MB_SIZE, cpuinfo_f) != ENDFILE)
    {
	char *ptr;
	/*
        ** we're looking for lines which looks like this:
        ** processor	: 0
	*/
	if ( (ptr = STstrindex(buf, "processor\t:", 0, TRUE)) != NULL )
	    numcpu++;

    }
    *cpus=numcpu;
    DBG_PRINT("Found %d cpus\n", numcpu);
    SIclose(cpuinfo_f);
    if (numcpu > 0)
        return(OK);
    else
	return(FAIL); /* none found, something went wrong */
}

STATUS
gipGetSysShmmax(SIZE_TYPE *shmmax)
{
    *shmmax = sysinfo[GIP_SYS_SHMMAX];

    return( *shmmax > 0 ? OK : FAIL );
}

static STATUS
get_sys_shmmax(SIZE_TYPE *shmmax)
{
    char	sys_shmmax[] = "/proc/sys/kernel/shmmax";
# define BUF_SIZE 30
    char	buf[BUF_SIZE];
    LOCATION	shmmax_loc;
    FILE	*shmmax_f;
    SIZE_TYPE	shmmax_val = 0;

    LOfroms(PATH|FILENAME, sys_shmmax, &shmmax_loc);
    if (LOexist(&shmmax_loc) != OK)
	return FAIL;

    DBG_PRINT("Reading %s\n", sys_shmmax);
    if (SIopen(&shmmax_loc, "r", &shmmax_f) != OK )
	return FAIL;

    /* read string value and convert to u_long */
    if ( fscanf(shmmax_f, "%lu", &shmmax_val) == EOF )
	*shmmax = 0;
    else
    {
	SIclose(shmmax_f);
	*shmmax = shmmax_val;
    }

    if (*shmmax <= 0)
    {
	DBG_PRINT("Error reading shmmax\n");
	return(FAIL); /* none found, something went wrong */
    }

    DBG_PRINT("kernel.shmmax = %lu\n", *shmmax);
    return(OK);
}

STATUS
gipSetSysShmmax(SIZE_TYPE shmmax)
{
    char	sys_shmmax[] = "/proc/sys/kernel/shmmax";
# define BUF_SIZE 30
    char	buf[BUF_SIZE];
    LOCATION	shmmax_loc;
    FILE	*shmmax_f;

    if (shmmax <= 0)
	return FAIL;

    LOfroms(PATH|FILENAME, sys_shmmax, &shmmax_loc);
    if (LOexist(&shmmax_loc) != OK)
	return FAIL;

    DBG_PRINT("Opening for writing %s\n", sys_shmmax);
    if (SIopen(&shmmax_loc, "w", &shmmax_f) != OK )
	return FAIL;

    STprintf(buf, "%d", shmmax);
    SIputrec(buf, shmmax_f);

    SIclose(shmmax_f);
    DBG_PRINT("shmmax set to %s\n", buf);

    return(OK);
}

STATUS
gipCheckMemReq(void)
{
    SIZE_TYPE 	needmemkb;
    vw_cfg	*mmptr = vw_cfg_info[GIP_VWCFG_IDX_MAX_MEM];
    vw_cfg	*bpptr = vw_cfg_info[GIP_VWCFG_IDX_BUFF_POOL];

    /* sanity check what we're looking at */
    if ( mmptr->unit >= VWNOUNIT || bpptr->unit >= VWNOUNIT )
	return(FAIL);

    /* store total require memory, converting to KB as needed */
    needmemkb = (mmptr->value << (10 * mmptr->unit)) +
		    (bpptr->value << (10 * bpptr->unit)) ;

    DBG_PRINT("Max Mem req: %ldKb\nBuff Pool req: %ldKb\nTotal req: %ldKb\n",
		mmptr->value << (10 * mmptr->unit),
		bpptr->value << (10 * bpptr->unit),
		needmemkb );

    if ( needmemkb < sysinfo[GIP_SYS_PHYSMEM] * 0.9 )
	return(OK);
    else
	return(FAIL);
}
# endif /* xCL_GTK_EXISTS */
