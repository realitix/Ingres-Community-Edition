/*
**  Copyright (c) 2004, 2008, 2011 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cv.h>
#include    <mh.h>
#include    <me.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adudate.h>
#include    <adudateint.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <aduint.h>
#include    "adutime.h"
#include    "adu1dates.h"


/**
** Name:  ADULLDATES.C -    Low level date routines which primarily support ADF
**			    interface.
**
** Description:
**	  This file defines low level date routines which primarily support
**	the ADF interface.  Other routines are used for the normalization
**	of dates.
**
**	This file defines:
**
**	    adu_1normldate()   - Normalizes an absolute date.
**	    adu_2normldint()   - Normalizes a internal interval date.
**          adu_4date_cmp()    - Internal routine used for comparing dates.
**          adu_dlenchk()      - Required function for checking create length
**                                 and precision
**          adu_3day_of_date() - Compute absolute number of days since epoch
**                                 for the given ABSOLUTE date.
**          adu_5time_of_date()- Calculate the number of second since 1/1/70.
**
**  Function prototypes defined in ADUDATE.H except adu_4date_cmp, which is
**  defined in ADUINT.H file.
** 
**  History:
**	21-jun-85 (wong)
**	    Used 'I1_CHECK_MACRO' macro to hide unsigned character problems
**	    with AD_NEWDTNTRNL member 'dn_highday'.
**	11-apr-86 (ericj)
**	    Assimilated into Jupiter from 4.0/02 code.
**	03-feb-87 (thurston)
**	    Upgraded the adu_4date_cmp() routine to be able to be a function
**	    instance routine, as well as the adc_compare() routine for dates.
**      25-feb-87 (thurston)
**          Made changes for using server CB instead of GLOBALDEF/REFs.
**	16-aug-89 (jrb)
**	    Put adu_prelim_norml() back in and re-wrote for performance.
**	24-jan-90 (jrb)
**	    Fixed bug 9859.  date('60 mins') was returning "2 hours".
**	06-sep-90 (jrb)
**	    Use adf_timezone (session specific) rather than call TMzone.
**      29-aug-92 (stevet)
**          Added adu_5time_of_date to calculate the number of second
**          since 1/1/70 for a given Ingres date.  Modified the timezone
**          adjustment method in adu_4date_cmp().
**      04-jan-1993 (stevet)
**          Added function prototypes and moved adu_prelim_norml to adudates.c
**          since this routine only used in adudates.c file.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	10-feb-1998 (hanch04)
**	    changed PRE_EPOCH_DAYS when calculating days to support
**	    years 0001 - 9999, EPOCH will be calculated as 14-sep-1752
**	    dates between, but not including  02-sep-1752 and 14-sep-1752
**	    do not exist.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
**	19-Feb-2008 (kiria01) b119943
**	    Include dn_nseconds with dn_seconds in checks for setting
**	    dn_status);
**	03-Jun-2008 (kiria01) b112144
**	    Correct TZ type flags as passed to TMtz_search to that
**	    TM_TIMETYPE_LOCAL not used against adjusted NEWDTNTRNL form & v.v. 
**      17-Apr-2009 (horda03) Bug 121949
**          AD_DN2_ADTE_TZ must be set for all Ingres Date's that are ABSOLUTE
**          but don't have TIMESPEC. Old Ingres versions may insert Ingres Date's
**          with some flags not set (e.g. back in 2.6 the date('today') function
**          would insert a date represented by '01xxxxxx' (only ABSOLUTE flag set).
**      17-jul-2009 (horda03) b122337
**          For an ABSOLUTE Ingresdate check to see if it need to be
**          normalised. In olden days, a date( '10:10:10') could have
**          a negative Num seconds, or num seconds > MAX SECS PER DAY
**          depending on the timezone of the session entering the
**          value.
**      11-Nov-2009 (horda03) Bug 122871
**          Set DATE Status flags based on the DATE type being converted
**          not on the value of the DATE part. If a timestamp with 0 time
**          is converted to an IngresDate, the Ingres date will be a DATE-ONLY
**          value; it should be a DATE+TIME value.
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added date function support for date_format/time_format, dayofweek,
**	    dayofmonth, dayofyear/doy, unix_timestamp, from_unixtime, last_day
**	    and months_between.
**	31-Aug-2011 (kiria01) SIR 125693
**	    Adjusted previous change to cater for timezones and the date
**	    normalisation.
**	31-Aug-2011 (kiria01) b125709
**	    Add millisecond() & dayofweek(date,i)
**	10-Nov-2011 (kiria01) b125945
**	    Pulled out AD_NEWDTNTRNL related parts into internal
**	    header.
**/

FUNC_EXTERN DB_STATUS
ad0_1cur_init(
ADF_CB		    *adf_scb,
struct	timevect    *tv,
bool                now,
bool		    highprec,
i4		    *tzoff);

static DB_STATUS
ad0_tz_offset(
ADF_CB              *adf_scb,
AD_NEWDTNTRNL       *base,
TM_TIMETYPE	    src_type,
i4                  *tzsoffset);


/*  Static variable references	*/

static	i4	    dum1;   /* Dummy to give to ult_check_macro() */
static	i4	    dum2;   /* Dummy to give to ult_check_macro() */

static const int daysBefore[] = {
	0,	/* notused, dn_month is 1 origin */
	0,	/* Jan */
	31,	/* Feb */
	59,	/* Mar */
	90,	/* Apr */
	120,	/* May */
	151,	/* Jun */
	181,	/* Jul */
	212,	/* Aug */
	243,	/* Sep */
	273,	/* Oct */
	304,	/* Nov */
	334	/* Dec */
};

static const i4 nano_mult[] = {
    1000000000,  /* 0 */
     100000000,  /* 1 */
      10000000,  /* 2 */
       1000000,  /* 3 */
        100000,  /* 4 */
         10000,  /* 5 */
          1000,  /* 6 */
           100,  /* 7 */
            10,  /* 8 */
             1,  /* 9 */
};

static const char weekdays[][10] =
{
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
};
static const char monthName[][10] = {
    "",	/* notused, month is 1 origin */
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

/*
** Define weekday start offsets for use with
** day of date result and the epoch reference date.
*/
#define SUNDAYOFFSET (-2)
#define PRE_EPOCH_DAYS 639810
static const char offsetSun[] = {0, 6, 5, 4, 3, 2, 1};
static const char offsetMon[] = {1, 0, -1, -2, -3, 3, 2};
static const char declimit[] = {33, 32, 31, 30, 29, 35, 34};


/*
**  Name:   adu_1normldate() - normalize an internal date.
**
**  Description:
**	Normalizes an absolute time as follows:
**	    1 <= month <= 12
**	    1 <= day <= adu_2monthsize(month, year)
**	    0 <= time <= 24 * 3600000
**
**	normldate works for both positive dates and negative
**      dates (for subtraction).
**
**  Inputs:
**      d   - Pointer to date to normalize.
**
**  Outputs:
**	*d  - normalized date.
**
**  Returns:
**      (none)
**
**  History:
**      03/25/85 (lichtman) -- fixed bug #5281: timezone of 0 allows
**              24:00:00 to be stored.  Fixed normalization
**              of time part of date to check for >= AD_5DTE_MSPERDAY
**              instead of > AD_5DTE_MSPERDAY.
**	10-apr-86 (ericj) - snatched from 4.0 code and modified for Jupiter.
**	13-jan-87 (daved) - performance enhancements
**	10-Dec-2004 (schka24)
**	    More (minor) speed tweaks.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_1normldate for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	01-aug-2006 (gupsh01)
**	    Added support for nanosecond precision level.
**	18-Feb-2008 (kiria01) b120004
**	    Use standard approach to normalising numbers.
*/

VOID
adu_1normldate(
AD_NEWDTNTRNL   *d)
{
    register i4	    days;
    register i4     t;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_006_NORMDATE, &dum1, &dum2))
    {
        TRdisplay("normldate:abs.date input:year=%d, month=%d, day=%d\n",
            d->dn_year, d->dn_month, d->dn_day);
        TRdisplay("normldate:and time=%d.%09d, status=%d\n",
            d->dn_seconds, d->dn_nsecond, d->dn_status);
    }
#   endif

    d->dn_status2 &= ~AD_DN2_NEED_NORM;

    if ((u_i4)d->dn_nsecond >= AD_49DTE_INSPERS)
    {
        t = d->dn_nsecond / AD_49DTE_INSPERS;
        d->dn_seconds += t;
	d->dn_nsecond -= t * AD_49DTE_INSPERS;
	if (d->dn_nsecond < 0)
	{
	    d->dn_nsecond += AD_49DTE_INSPERS;
	    d->dn_seconds--; /* Borrow a second */
	}
    }

    days = d->dn_day;
    if ((u_i4)d->dn_seconds >= AD_41DTE_ISECPERDAY)
    {
	t = d->dn_seconds / AD_41DTE_ISECPERDAY;
	days += t;
	d->dn_seconds -= t * AD_41DTE_ISECPERDAY;
	if (d->dn_seconds < 0)
	{
	    d->dn_seconds += AD_41DTE_ISECPERDAY;
	    days--; /* Borrow a day */
	}
    }


    /* normalize months so we can handle days correctly */

    d->dn_month--; /* zero bias temporarily */
    if ((u_i4)d->dn_month >= 12 && d->dn_year > 0)
    {
	t = d->dn_month / 12;
	d->dn_year += t;
	d->dn_month -= t * 12;
	if (d->dn_month < 0)
	{
	    d->dn_month += 12;
	    d->dn_year--;
	}
    }
    d->dn_month++;


    /* now do days remembering to handle months on overflow/underflow.
    ** Don't bother if # of days is ok for any month.
    */

    if (days > 28)
    {
	while (days > (t = adu_2monthsize((i4) d->dn_month, (i4) d->dn_year)))
	{
	    days -= t;
	    d->dn_month++;

	    if (d->dn_month > 12)
	    {
		d->dn_year++;
		d->dn_month -= 12;
	    }
	}
    }

    while ((days < 1) && (d->dn_month > 0))
    {
        d->dn_month--;

        if (d->dn_month < 1)
        {
            d->dn_year--;
            d->dn_month += 12;
        }

        days += adu_2monthsize((i4) d->dn_month, (i4) d->dn_year);
    }

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_006_NORMDATE, &dum1, &dum2))
    {
        TRdisplay("normldate:abs.date output:year=%d, month=%d, day=%d\n",
            d->dn_year, d->dn_month, days);
        TRdisplay("normldate:and time=%d, status=%d\n",
            d->dn_seconds, d->dn_status);
    }
#   endif

    d->dn_day    = days;
}


/*
**  Name:   adu_2normldint() - Internal routine used to normalize interval dates
**
**  Description:
**      Normalizes an interval date as follows:
**      0 <= time  <= 24 * 3600000
**      0 <= month <  12
**
**  NOTE: days and times are not normalized to months and years
**      since these are inexact transformations.
**
**  Returns number of days in interval, used for comparisons.
**  Computed using 365 days/year and 30.5 days/month
**
**      11/02/84 (lin) - if time interval = 0, reset DAYSPEC status bit instead
**           of the original YEARSPEC.
**
**  12/9/85 (ac) - bug # 6399. Rounding error in normalizing the dn_time in
**             mini second.
**	17-jan-02 (inkdo01)
**	    All these years, and MONTHSPEC hasn't been set for date values.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_2normldint for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	28-jul-2006 (gupsh01)
**	    Added support for nanoseconds.
**	16-oct-2006 (gupsh01)
**	    Fix day calculation.
**	13-feb-2006 (gupsh01)
**	    Another fix to days calculation.
**	4-sep-2007 (dougi)
**	    Fix normalization involving nsecs. 3 unit, multi-radix, mixed sign
**	    normalization ain't very easy.
**	18-Feb-2008 (kiria01) b120004
**	    Use standard approach to normalising numbers and remove some of
**	    the unnecessary float processing. Documented previous change to
**	    isolate some normalisation errors due to combinations not being
**	    handled.
*/

i4
adu_2normldint(
AD_NEWDTNTRNL   *d)
{
    i1	status = 0;
    i4  nsec = d->dn_nsecond;
    i4  secs = d->dn_seconds;
    i4  days = d->dn_day;
    i2  months = 0;
    i2  years = 0;
    f8  month_days = 0.0;
    f8  flt_days = 0.0;
    f8  flt_years = 0.0;
    f8  flt_months = 0.0;
    f8  flt_temp = 0.0;

    /* normalize days and time */
    d->dn_status2 &= ~AD_DN2_NEED_NORM;

    /* Does nsec have (or owe) more than a whole second? */
    if ((u_i4)nsec >= AD_49DTE_INSPERS)
    {
	i4 tmp = nsec / AD_49DTE_INSPERS;
        secs += tmp;
        nsec -= tmp * AD_49DTE_INSPERS;
    }

    /* Does secs have (or owe) more than a whole day? */
    if ((u_i4)secs >= AD_40DTE_SECPERDAY)
    {
	i4 tmp = secs / AD_40DTE_SECPERDAY;
        days += tmp;
        secs -= tmp * AD_40DTE_SECPERDAY;
    }

    /* We now have both nsec and secs in valid ranges BUT might be -ve */

    /* We need to normalize across all 3 units (days, seconds, nanoseconds)
    ** and we aim to get all three components to have the same sign (or 0)
    **
    ** Switch on encoding (ternary number, in which leftmost digit is
    ** for days units, middle is time units and rightmost is nanoseconds,
    ** and 2 means value is > 0, 1 means value is < 0 and 0 means value
    ** is 0). All combinations of signs of the 3 units are represented,
    ** many of which don't require any wor. E.g. all values with 
    ** nanosecond value 0 are ignored, because normalization of days/
    ** seconds has already been done. And all combinations of days and
    ** seconds with different, non-zero signs will have been normalized
    ** to the same sign.
    ** This code may look a little unconventional, but it handles the
    ** problems of multi-radix normalization. If anyone can think of
    ** a better way of doing it, be my guest.
    */
#define N_eq_0 (0 * 1)
#define N_lt_0 (1 * 1)
#define N_gt_0 (2 * 1)
#define S_eq_0 (0 * 3)
#define S_lt_0 (1 * 3)
#define S_gt_0 (2 * 3)
#define D_eq_0 (0 * 9)
#define D_lt_0 (1 * 9)
#define D_gt_0 (2 * 9)
    switch ((nsec > 0 ? N_gt_0 : (nsec < 0 ? N_lt_0 : N_eq_0)) +
	    (secs > 0 ? S_gt_0 : (secs < 0 ? S_lt_0 : S_eq_0)) +
	    (days > 0 ? D_gt_0 : (days < 0 ? D_lt_0 : D_eq_0)))
    {
    /*  0  D_eq_0 S_eq_0 N_eq_0	    =0  */
    /*  1  D_eq_0 S_eq_0 N_lt_0	    <0 */
    /*  2  D_eq_0 S_eq_0 N_gt_0	    >0 */
    /*  3  D_eq_0 S_lt_0 N_eq_0     <0 */
    /*  4  D_eq_0 S_lt_0 N_lt_0	    <0 */
    /*  5  D_eq_0 S_lt_0 N_gt_0  ** <0 */
    /*  6  D_eq_0 S_gt_0 N_eq_0     >0 */
    /*  7  D_eq_0 S_gt_0 N_lt_0  ** >0 */
    /*  8  D_eq_0 S_gt_0 N_gt_0     >0 */
    /*  9  D_lt_0 S_eq_0 N_eq_0     <0 */
    /* 10  D_lt_0 S_eq_0 N_lt_0     <0 */
    /* 11  D_lt_0 S_eq_0 N_gt_0 *** <0 */
    /* 12  D_lt_0 S_lt_0 N_eq_0     <0 */
    /* 13  D_lt_0 S_lt_0 N_lt_0     <0 */
    /* 14  D_lt_0 S_lt_0 N_gt_0  ** <0 */
    /* 15  D_lt_0 S_gt_0 N_eq_0  ** <0 */
    /* 16  D_lt_0 S_gt_0 N_lt_0  ** <0 */
    /* 17  D_lt_0 S_gt_0 N_gt_0 *** <0 */
    /* 18  D_gt_0 S_eq_0 N_eq_0     >0 */
    /* 19  D_gt_0 S_eq_0 N_lt_0  ** >0 */
    /* 20  D_gt_0 S_eq_0 N_gt_0     >0 */
    /* 21  D_gt_0 S_lt_0 N_eq_0  ** >0 */
    /* 22  D_gt_0 S_lt_0 N_lt_0 *** >0 */
    /* 23  D_gt_0 S_lt_0 N_gt_0  ** >0 */
    /* 24  D_gt_0 S_gt_0 N_eq_0     >0 */
    /* 25  D_gt_0 S_gt_0 N_lt_0  ** >0 */
    /* 26  D_gt_0 S_gt_0 N_gt_0     >0 */

    case D_eq_0 + S_lt_0 + N_gt_0:
    case D_lt_0 + S_lt_0 + N_gt_0:
	nsec -= AD_42DTE_INSUPPERLIMIT;		/* remove a second */
	secs++;					/* add a second */
	break;

    case D_lt_0 + S_eq_0 + N_gt_0:
    case D_lt_0 + S_gt_0 + N_gt_0:
	nsec -= AD_42DTE_INSUPPERLIMIT;		/* remove a second */
	secs++;					/* add a second */
	/*  FALLTHROUGH to remove+add a day */

    case D_lt_0 + S_gt_0 + N_eq_0:
    case D_lt_0 + S_gt_0 + N_lt_0:
	secs -= AD_40DTE_SECPERDAY;		/* remove a day */
	days++;					/* add a day */
	break;

    case D_eq_0 + S_gt_0 + N_lt_0:
    case D_gt_0 + S_gt_0 + N_lt_0:
	nsec += AD_42DTE_INSUPPERLIMIT;		/* add a second */
	secs--;					/* remove a second */
	break;

    case D_gt_0 + S_eq_0 + N_lt_0:
    case D_gt_0 + S_lt_0 + N_lt_0:
	nsec += AD_42DTE_INSUPPERLIMIT;		/* add a second */
	secs--;					/* remove a second */
	/*  FALLTHROUGH to add+remove a day */
	
    case D_gt_0 + S_lt_0 + N_eq_0:
    case D_gt_0 + S_lt_0 + N_gt_0:
	secs += AD_40DTE_SECPERDAY;		/* add a day */
	days--;					/* remove a day */
	break;

    }

    d->dn_day = days;
    d->dn_seconds = secs;
    d->dn_nsecond = nsec;

    /* normalize years and months */
    flt_days     = days + secs / AD_40DTE_SECPERDAY;
    flt_years    = (i4)d->dn_year;
    flt_months   = (i4)d->dn_month;
    flt_years   += flt_months / 12;
    d->dn_year   = years = flt_years;

    /*
        Since conversion from double to long truncates the double,
            round here to hide any floating point accuracy
            problems.
        Bug 1344 peb 6/25/83.
    */

    flt_temp = (flt_years - d->dn_year) * 12;
    d->dn_month = months = flt_temp + (flt_temp >= 0.0 ? 0.5 : -0.5);

    /* calculate number of days in interval */
    month_days   = flt_years * AD_14DTE_DAYPERYEAR + flt_days;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_006_NORMDATE, &dum1, &dum2))
        TRdisplay("normldint: interval converted to days = %d\n", (i4)month_days); 
#   endif

    /* now normalize status bits according to LRC-approved rules */

    status = d->dn_status;

    if (years != 0)
    {
	status |= AD_DN_YEARSPEC;
	
	if (months == 0)
	    status &= ~AD_DN_MONTHSPEC;
	else status |= AD_DN_MONTHSPEC;
    }
    else
    {
	if (months != 0)
	{
	    status &= ~AD_DN_YEARSPEC;
	    status |= AD_DN_MONTHSPEC;
	}
	else
	{
	    if (status & AD_DN_YEARSPEC)
		status &= ~AD_DN_MONTHSPEC;
	}
    }

    if (days != 0)
    {
	status |= AD_DN_DAYSPEC;
	
	if (secs == 0)
	    status &= ~AD_DN_TIMESPEC;
    }
    else
    {
	if (secs != 0)
	{
	    status &= ~AD_DN_DAYSPEC;
	    status |= AD_DN_TIMESPEC;
	}
	else
	{
	    if (status & AD_DN_DAYSPEC)
		status &= ~AD_DN_TIMESPEC;
	}
    }

    d->dn_status    = status;
    d->dn_day    = days;

    return((i4)month_days);
}


/*
**  Name:  adu_4date_cmp() - Internal routine used for comparing dates
**
**  Parameters:
**	adf_scb -- ADF session control block
**      dv1     -- first date
**      dv2     -- second date
**	cmp     -- Result of comparison, as follows:
**			<0  if  dv1 <  dv2
**			>0  if  dv1 >  dv2
**			=0  if  dv1 == dv2
**
**  Since intervals are always < absolute dates, the status of the dates
**  are first checked. If one date is absolute and the other an interval
**  the return code is set (absolute > interval) without looking at the
**  actual dates.
**
**  Empty dates are not compared to either intervals or absolute dates.
**  The empty date is considered to be less than all other dates values,
**  but equal to another empty date.
**
**  Returns:
**	E_DB_OK
**
**  History:
**	03-feb-87 (thurston)
**	    Upgraded the adu_4date_cmp() routine to be able to be a function
**	    instance routine, as well as the adc_compare() routine for dates.
**	06-sep-90 (jrb)
**	    Use adf_timezone (session specific) rather than call TMzone.
**      29-aug-92 (stevet) 
**          Modified timezone adjustment method by call TMtz_search()
**          to calcuate correct timezone offset based on a given time
**          value.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_4date_cmp for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	25-sep-2006 (gupsh01)
**	    Added support for normalizing time to UTC for time/timestamp
**	    with time zone types.
**	07-nov-2006 (gupsh01)
**	    Fixed the second calculations.
**	02-jan-2007 (gupsh01)
**	    Account for the nanosecond difference if the absolute dates/time
**	    values are same upto nanoseconds.
**	04-feb-2007 (gupsh01)
**	    Fixed the sorting of the time values.
**	18-Feb-2008 (kiria01) b120004
**	    Remove timezone code from compare as dealing with UTC times
**	11-May-2008 (kiria01) b120004
**	    Apply any defered application of time/timezone if needed.
**      24-Mar-2009 (coomi01) b121830
**          Test for normalisation pending flag, and normalise before
**          comparing if required.
*/

/*ARGSUSED*/
DB_STATUS
adu_4date_cmp(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
i4		*cmp)
{
    DB_STATUS db_stat = E_DB_OK;
    i4	    days1;
    i4	    days2;
    i4	    tmp_val;
    AD_NEWDTNTRNL    *d1;
    AD_NEWDTNTRNL    *d2;
    AD_NEWDTNTRNL    tempd1;
    AD_NEWDTNTRNL    tempd2;     /* aligned storage */

    
    /*
    **  MEcopy()'s were put in for BYTE_ALIGN but are needed even
    **      on VAX as we can't do comparisions in place since
    **      we normalize the values in place.
    **  Think what happens when the sorter does these comparisons
    **      (effectively changing the internal representation
    **      to the normalized representation) and then returns
    **      these to the query processor to be stuffed into a
    **      temporary (or a new relation in modify!!!).
    **
    **  Change 8/24/83 by peb.
    */

    if (db_stat = adu_6to_dtntrnl (adf_scb, dv1, &tempd1))
	return (db_stat);
    if (db_stat = adu_6to_dtntrnl (adf_scb, dv2, &tempd2))
	return (db_stat);

    d1 = &tempd1;
    d2 = &tempd2;

    if     (     d1->dn_status == AD_DN_NULL && d2->dn_status == AD_DN_NULL)
        tmp_val = 0;
    else if(     d1->dn_status == AD_DN_NULL && d2->dn_status != AD_DN_NULL)
        tmp_val = -1;
    else if(     d1->dn_status != AD_DN_NULL && d2->dn_status == AD_DN_NULL)
        tmp_val = 1;
    else if(!(d1->dn_status & AD_DN_ABSOLUTE && d2->dn_status & AD_DN_ABSOLUTE))
    {
        if (d1->dn_status & AD_DN_ABSOLUTE)
        {
            /* d1 is absolute and d2 an interval; d1 > d2 */

            tmp_val = 1;
        }
        else if (d2->dn_status & AD_DN_ABSOLUTE)
        {
            /* d1 is an interval and d2 absolute; d1 < d2 */

            tmp_val = -1;
        }
        else        /* both dates are intervals */
        {
            days1   = adu_2normldint(d1);
            days2   = adu_2normldint(d2);

            if (!(tmp_val = days1 - days2))
                tmp_val = d1->dn_seconds - d2->dn_seconds;
        }
    }
    else
    {
	if ((d1->dn_status2 ^ d2->dn_status2) & AD_DN2_NO_DATE)
	{
	    if (d1->dn_status2 & AD_DN2_NO_DATE)
		db_stat = ADU_NEWDT_PEND_DATE(adf_scb, d1);
	    else
		db_stat = ADU_NEWDT_PEND_DATE(adf_scb, d2);
	    if (db_stat)
		return db_stat;
	}
	if ((d1->dn_status2 ^ d2->dn_status2) & AD_DN2_IN_LCLTZ)
	{
	    if (d1->dn_status2 & AD_DN2_IN_LCLTZ)
		db_stat = ADU_NEWDT_TO_GMT(adf_scb, d1);
	    else
		db_stat = ADU_NEWDT_TO_GMT(adf_scb, d2);
	    if (db_stat)
		return db_stat;
	}

	/* 
	** Finally, check for normalisation pending flag 
	*/
	if (d1->dn_status2 & AD_DN2_NEED_NORM)
	{
	    adu_1normldate(d1);
	}
	if (d2->dn_status2 & AD_DN2_NEED_NORM)
	{
	    adu_1normldate(d2);
	}

        /* both dates are absolute */
        if (!(tmp_val = d1->dn_year - d2->dn_year) &&
            !(tmp_val = d1->dn_month - d2->dn_month) &&
            !(tmp_val = d1->dn_day - d2->dn_day))
            tmp_val = d1->dn_seconds - d2->dn_seconds;
    }

    if (tmp_val == 0)
    {
	/* check the nanosecond diff */
        tmp_val = d1->dn_nsecond - d2->dn_nsecond;
    }

    if (tmp_val < 0)
	*cmp = -1;
    else if (tmp_val > 0)
	*cmp = 1;
    else
	*cmp = 0;

    return(E_DB_OK);
}


/*
**  Name:  adu_dlenchk() - Required function for checking create length
**                         and precision
**
**  Parameters:
**      length  -- length in create specification
**      prec    -- precision in create specification
**
**  Returns:
**      true    -- valid size and precision
**      false   -- invalid size or precision
**
*/
bool
adu_dlenchk(
i4  *length,
i4  prec)
{
    return(*length == ADF_DTE_LEN && prec == 0);
}


/*
**  Name:  adu_3llday_of_date() - Compute absolute number of days since epoch
**                              for the given ABSOLUTE date.
**
**  Description:
**	This is used for computing intervals in date subtraction and for
**	computing the day of week for a given date.
**
**	NOTE: The epoch is Jan 1, 1582 for all date operations.
**	This date was a Friday
**
**  Parameters:
**      year, month, day
**
**  Returns:
**      days since epoch for this date.
**
**  History
**	13-jan-87 (daved)
**	    avoid loop in computing days in year.
**	10-feb-1998 (hanch04)
**	    changed PRE_EPOCH_DAYS when calculating days to support
**	    years 0001 - 9999, EPOCH will be calculated as 14-sep-1752
**	    dates between, but not including  02-sep-1752 and 14-sep-1752
**	    do not exist.
**	19-May-2004 (schka24)
**	    Eliminate time consuming loop calculating days-before-
**	    this-month, use table lookup.
**	1-Jul-2004 (schka24)
**	    Back out year optimization, wrong because of integer divide.
**	    Century dates prior to 1752 were in fact leap years.
**	    Note that we're assuming the English calendar here.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_3day_of_date for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	12-Aug-2011 (kiria01)
**	    Renamed to adu_3llday_of_date for more usable interface.
*/

i4
adu_3llday_of_date(
	register i4 year,
	register i4 month,
	register i4 day)
{
    i4 i;
    i4 days;

    days = 0;

    /* Compute number of days in previous years , 14-sep-1752 */

    year--;
    /* 365 days a year plus one on leap year, except every hundred years
    ** not divisible by 400.
    ** Can't do y/4 - y/100 + y/400 = 97y/400, because the individual
    ** divisions truncate differently than the combined.
    ** 1800 was the first century year to not be a leap year, after which
    ** 2000 was the first quadricentury year to be a leap year.
    ** 1700 WAS a leap year, hence the -1 (adding it back in), and
    ** setting i=1 for early dates makes it all work out...
    */
    i = 1;
    if (year >= 1800)
	i = (year-1600) / 100;
    days = 365 * year + (year >> 2) - (i-1) + (i >> 2);

    year++;
    /* Compute number of days in previous months */

    days += daysBefore[month];

    /* Check for leap year, minimizing divides on modern day hardware.
    ** According to the Julian calendar, there was no century leap year
    ** adjustment.  The English calendar switched in 1752.
    ** Very old dates assume a backwards extension of the Julian calendar,
    ** but of course that's strictly a convenience and becomes increasingly
    ** bogus as you go backwards.
    */

    i = year % 400;
    if (month >= 3 &&
	    ((year & 3) == 0) &&
	    (days < PRE_EPOCH_DAYS || (i != 100 && i != 200 && i != 300)) )
	days++;

    /* For absolute date, high order day byte is always zero */
    /* Add in number of days in current month and return */

    days += day;
    if (days < PRE_EPOCH_DAYS)
    {
	if (days > (PRE_EPOCH_DAYS - 11))
	    days = (PRE_EPOCH_DAYS - 12);
    }
    else
	days -= 11;

    return days; 
}   

/*
**  Name:  adu_3day_of_date()  - Compute absolute number of days since epoch
**                              for the given ABSOLUTE date.
**
**  Description:
**	This is used for computing intervals in date subtraction and for
**	computing the day of week for a given date.
**
**	NOTE: The epoch is Jan 1, 1582 for all date operations.
**
**  Parameters:
**      date    -- internal date
**
**  Returns:
**      days since epoch for this date.
**
**  History
*/

i4
adu_3day_of_date(
register AD_NEWDTNTRNL  *d)
{
    return adu_3llday_of_date(d->dn_year, d->dn_month, d->dn_day);
}   

/*
**  Name:  adu_3llday_of_year() - Compute day number in this year.
**
**  Description:
**	This is used for computing the day of a year
**
**  Parameters:
**      year, month, day
**
**  Returns:
**      day number in year
**
**  History
**	11-Aug-2011 (kiria01)
**	    Added for DATE_FORMAT
*/
i4
adu_llday_of_year(
	i4 year,
	i4 month,
	i4 day)
{
    i4 days = 0;
    i4 totdays = daysBefore[month];
    i4 i = 1;

    /* Compute number of days in previous years , 14-sep-1752 */
    year--;
    /* 365 days a year plus one on leap year, except every hundred years
    ** not divisible by 400.
    ** Can't do y/4 - y/100 + y/400 = 97y/400, because the individual
    ** divisions truncate differently than the combined.
    ** 1800 was the first century year to not be a leap year, after which
    ** 2000 was the first quadricentury year to be a leap year.
    ** 1700 WAS a leap year, hence the -1 (adding it back in), and
    ** setting i=1 for early dates makes it all work out...
    */
    if (year >= 1800)
	i = (year-1600) / 100;
    days = 365 * year + (year >> 2) - (i-1) + (i >> 2);
    year++;
    days += totdays;
    i = year % 400; /*leap adjust*/
    if (month >= 3 && ((year & 3) == 0) &&
	    (days < PRE_EPOCH_DAYS || (i != 100 && i != 200 && i != 300)) )
	totdays++;
    totdays += day;
    if (year == 1752 && totdays > 243+2+1) /* =daysBefore[9]+2nd +1 for leap yr */
	totdays -= 11;
    return totdays;
}


/*
**  Name:  adu_5time_of_date() - Compute the number of seconds since 1/1/70
**                               00:00. 
**
**  Description:
**	This internal routine is used for calculating number of seconds since
**      1/1/70 00:00.
**
**      The number of seconds is returned in an I4 and capped and MIN/MAX I4.
**      This means that the range is circa +/- 68 years
**
**  Parameters:
**      d    -- internal date
**
**  Returns:
**      seconds since 1/1/70 00:00.
**
**  History
**	25-sep-1992 (stevet)
**	    Initial creation.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_5time_of_date for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	07-nov-2006 (gupsh01)
**	    Fixed the seconds calculations.
**      05-Nov-2009 (horda03) Bug 122859
**          If the date is not normalized, then the seconds could be
**          negative. Modified the way that the boundary conditions
**          are detected.
**      12-Nov-2009 (horda03) b122859
**          Fix typo.
*/
i4
adu_5time_of_date(
register AD_NEWDTNTRNL  *d)
{
    register i4  i;
    register i4  days;
    register i4  yr;
    i4          pos_days;

    /* Compute number of days in previous years */
#define DAYS_SINCE_1970  719163
#define MAX_DAYS          24855   /* Maximum number of days that can be converted to
                                  ** seconds and held in an I4
                                  ** 1 DAY = 86400 seconds
                                  ** MAXI4 / 86400 = 24855
                                  */
#define MAX_LAST_DAY_SECS 11647   /* MAXI4 - (MAXI4 / 86400 * 86400) */

    yr = d->dn_year - 1;
    /* 365 days a year plus one on leap year, except every hundred years
    ** not divisible by 400.
    **
    ** This doesn't handle the lost days due to the switch to Gregorian
    ** Calendar. But this occured beyond the range of seconds that can be stored in
    ** an I4 from 1-Jan-1970; so not an issue.
    */
    days = 365 * yr - DAYS_SINCE_1970 + yr/4 - yr/100 + yr/400;

    /* Compute number of days in previous months */

    for (i = 1; i < d->dn_month; i++)
        days += adu_2monthsize((i4) i, (i4) d->dn_year);

    /* Always absolute date, do not need to look at high day */

    days += d->dn_day;

    pos_days = max(days, -days);

    if (pos_days > MAX_DAYS)
    {
       return (days > 0) ? MAXI4 : MINI4;
    }
    else if ( pos_days == MAX_DAYS)
    {
       /* We're close to the boundary, so need to check the seconds. */

       if ( (days < 0) && (d->dn_seconds < -MAX_LAST_DAY_SECS) )
       {
          return MINI4;
       }
       else if ( (days > 0) && (d->dn_seconds > MAX_LAST_DAY_SECS) )
       {
          return MAXI4;
       }
    }

    /* To reach here means that the Seconds since 1-Jan-1970 can be stored
    ** in an I4.
    */
    return (days * AD_41DTE_ISECPERDAY) + d->dn_seconds;
}   


/*
**  Name:  ad0_tz_offset	- Return appropriate tz offset in seconds
**
**  Description:
**
**  Parameters:
**      adf_scb		- ptr to SCB
**	base		- ptr to AD_NEWDTNTRNL which is to be used as the
**			  date off which to bias the TZ. If passed as NULL
**			  the bias date will be taken from current time.
**	src_type	- TM_TIMETYPE of TM_TIMETYPE_LOCAL or TM_TIMETYPE_GMT
**	offset		- ptr to i4 to receive the TZ offset value in seconds.
**
**  Returns:
**      DB_STATUS
**
**  History
**	18-Feb-2008 (kiria01) b120004
**	    Created to factor out timezone calculating.
**	01-May-2008 (kiria01) b120004
**	    Catch calls where base is passed but with no
**	    date present (ie. from TME,TMW or TMWO)
**      11-Jan-2011 (hanal04) Bug 124820
**          Binary load of data to the wrong table format can cause us to
**          have an entirely invalid date in this call. A dn_month value
**          that is out of range will SEGV in adu_5time_of_date(). Plug the 
**          hole allowing for a null date. 
**          
*/
static DB_STATUS
ad0_tz_offset(
ADF_CB              *adf_scb,
AD_NEWDTNTRNL	    *base,
TM_TIMETYPE	    src_type,
i4		    *tzoffset)
{
    STATUS	     db_stat = E_DB_OK;

    if (!base || base->dn_status2 & AD_DN2_NO_DATE)
	/* Just get the TZ cheaply */
	return ad0_1cur_init(adf_scb, NULL, TRUE, FALSE, tzoffset);

    if ( (base->dn_month < 0) || (base->dn_month > 12) )
    {
        
        return (adu_error(adf_scb, (i4) E_AD1014_BAD_VALUE_FOR_DT, 0));
    }
 
    /* Record the Session time zone displacement */
    *tzoffset = TMtz_search (adf_scb->adf_tzcb, src_type, 
				adu_5time_of_date(base));
    return (db_stat);
}

/*
**  Name:  adu_6to_dtntrnl	- map a value of one of the supported types
**	to the ADF internal format
**
**  Description:
**
**  Parameters:
**      indv		- ptr to DB_DATA_VALUE for input value
**	outval		- ptr to AD_NEWDTNTRNL into which value is written
**
**  Returns:
**      none
**
**  NOTE: This routine MUST initialise ALL fields of the outval structure
**
**  History
**	21-apr-06 (dougi)
**	    Written for date/time enhancements.
**	29-jun-2006 (gupsh01)
**	    Modified setting up the status bit and verifying.
**	19-jul-2006 (gupsh01)
**	    Modified setting up the vacant fields.
**	01-aug-2006 (gupsh01)
**	    Fixed memory alignment for ANSI date/time internal
**	    data.
**	02-aug-2006 (gupsh01)
**	    Fixed the copying of Timestamp without timezone 
**	    datatypes.
**	22-sep-06 (gupsh01)
**	    Even for empty values of ANSI date/times set the 
**	    flags so 0 is printed out.
**	16-sep-06 (gupsh01)
**	    Make sure that we check for absolute data type value.
**	20-oct-06 (gupsh01)
**	    Protect this routine from segv if the input and output
**	    pointers are uninitialized.
**	22-nov-06 (gupsh01)
**	    Modified so that the status flag is set only for non 
**	    zero values of date and time.
**	13-nov-06 (gupsh01)
**	    Fixed the time with time zone case and timestamp with
**	    local time zone.
**	18-Feb-2008 (kiria01) b120004
**	    Reworked to do the brunt of conversion work and added
**	    handling.
**	10-Apr-2008 (kiria01) b120004
**	    Ensure that zero timestamp is a valid Absolute and not
**	    treated as null date.
**	17-Apr-2008 (hweho01)
**	    Avoid SIGBUS on some plastforms, copy caller's data  
**	    to local buffer, so the fields are aligned. 
**	07-May-2008 (hweho01)
**	    Modified the previous change on 16-Apr-2008 :
**	    1) only copy the data to local buffer if it is not aligned,
**	    2) check the length of data to be copied, prevent buffer 
**	       overrun. 
**	11-May-2008 (kiria01) b120004
**	    Defered application of time/timezone from pure dates.
**	20-May-2008 (kiria01) b120004
**	    When converting from pure date to TSWO or TMWO, leave the
**	    time as 00:00.
**	26-Feb-2009 (kiria01) b121734
**	    Don't calculate the status bits for TSTMP, TSW and TSWO
**	    after TZ adjustment as this will lose empty dates with
**	    non-GMT timezones.
**      17-Apr-2009 (horda03) b121949
**          Set AD_DN2_ADTE_TZ for an ABSOLUTE IngresDate with no
**          AD_DN_TIMESPEC.
**      17-jul-2009 (horda03) b122337
**          For an ABSOLUTE Ingresdate check to see if it need to be
**          normalised. In olden days, a date( '10:10:10') could have
**          a negative Num seconds, or num seconds > MAX SECS PER DAY
**          depending on the timezone of the session entering the
**          value.
**      11-nov-2009 (horda03) b122871
**          Set dn_status flags based on the DATE type being converted.
**	18-May-2011 (kiria01) m1796
**	    Set INDS status flags to allow proper interaction with ANSIDATE.
*/
DB_STATUS
adu_6to_dtntrnl(
ADF_CB         *adf_scb,
DB_DATA_VALUE	*indv,
AD_NEWDTNTRNL	*outval)
{
    DB_STATUS   db_stat = E_DB_OK;
    AD_DTUNION	*dp, date_buff;
    i4		offset;
    if (indv && outval && (dp = (AD_DTUNION*)indv->db_data))
    {
	if (indv->db_length == (i4)sizeof(AD_NEWDTNTRNL) &&
		indv->db_datatype == DB_DTE_TYPE)
	{
	    memcpy(outval, indv->db_data, sizeof(AD_NEWDTNTRNL));
	    return db_stat;
	}
	/* NOTE - All AD_NEWDTNTRNL fields must be initialised
	** in this routine */
	outval->dn_dttype = abs(indv->db_datatype);
	AD_TZ_SETNEW(outval, 0);
	outval->dn_status = 0;
	outval->dn_status2 = 0;  /* Specifically, default TZ to invalid & GMT */

	if(ME_ALIGN_MACRO(indv->db_data, DB_ALIGN_SZ) != (PTR) indv->db_data) 
        { 
	  /* Copy data to buffer, so the fields are aligned correctly. */
	  MEcopy((PTR)indv->db_data, (indv->db_length > (i4)sizeof(date_buff))
	         ? (i4)sizeof(date_buff) : indv->db_length, (PTR)&date_buff );
	  dp = &date_buff;
        }

	/* Just switch to the relevant conversion code and do it. */
	switch(outval->dn_dttype)
	{
	case DB_DTE_TYPE:		/* old Ingres DATE */
	    outval->dn_year = dp->ing_date.dn_year;
	    outval->dn_month = dp->ing_date.dn_month;
	    outval->dn_day = dp->ing_date.dn_lowday |
		    (dp->ing_date.dn_highday << AD_21DTE_LOWDAYSIZE);
	    outval->dn_seconds = dp->ing_date.dn_time/AD_6DTE_IMSPERSEC;
	    outval->dn_nsecond = (dp->ing_date.dn_time % AD_6DTE_IMSPERSEC) *
							AD_38DTE_NSPERMS;
	    outval->dn_status = dp->ing_date.dn_status;
	    if (outval->dn_status != AD_DN_NULL &&
		    (outval->dn_status & (AD_DN_YEARSPEC |
				AD_DN_MONTHSPEC | AD_DN_DAYSPEC)) == 0)
		outval->dn_status2 |= AD_DN2_NO_DATE;
	    if ((outval->dn_status & (AD_DN_ABSOLUTE|AD_DN_TIMESPEC)) ==
				AD_DN_ABSOLUTE)
	    {
		/* Ingresdate with no time portion is similar to ANSIDATE
		** in that it is treated as LCLTZ. Originally, Ingres used
		** the unset AD_DN_TIMESPEC to mean that this was a pure date
		** and hence had no TZ. However, if a calculation happens to
		** clear the AD_DN_TIMESPEC flag due to hiting a real midnight
		** time then the TZ state is in limbo. The AD_DN2_IN_LCLTZ is
		** a more reliable way of tracking this & less TZ emphasis
		** should be taken from the AD_DN_TIMESPEC flag which ought 
		** not to be cleared by calculation as it can lead to the
		** anomally that we cannot have an actual *GMT* date+time
		** of midnight as it will look like local TZ! */
		outval->dn_status2 |= AD_DN2_IN_LCLTZ;
	    }
	    else if (outval->dn_status & AD_DN_ABSOLUTE &&
		    (u_i4)outval->dn_seconds >= AD_41DTE_ISECPERDAY)
            {
               /* An ABSOLUTE date shouldn't have -ive nor >AD_41DTE_ISECPERDAY
               ** so need to normalise. */
               outval->dn_status2 |= AD_DN2_NEED_NORM;
            }
	    break;

	case DB_ADTE_TYPE:	/* new standard DATE */
	    outval->dn_year = dp->adate.dn_year;
	    outval->dn_month = dp->adate.dn_month;
	    outval->dn_day = dp->adate.dn_day;
	    outval->dn_seconds = 0;
	    outval->dn_nsecond = 0;

            outval->dn_status = (AD_DN_ABSOLUTE  | AD_DN_YEARSPEC | 
                                 AD_DN_MONTHSPEC | AD_DN_DAYSPEC);
	    outval->dn_status2 |= AD_DN2_IN_LCLTZ;
	    break;

	case DB_TME_TYPE:	/* Ingres TIME (WITH LOCAL TIME ZONE) */
	case DB_TMW_TYPE:	/* TIME WITH TIME ZONE */
	case DB_TMWO_TYPE:	/* TIME WITHOUT TIME ZONE */
	    outval->dn_year = 0;
	    outval->dn_month = 0;
	    outval->dn_day = 0;
	    outval->dn_seconds = dp->atime.dn_seconds;
	    outval->dn_nsecond = dp->atime.dn_nsecond;
	    if (outval->dn_dttype == DB_TMW_TYPE)
	    {
		AD_TZ_SETNEW(outval, AD_TZ_OFFSET(&dp->atime));
		outval->dn_status2 |= AD_DN2_TZ_VALID;
		if (AD_TZ_ISLEGACY(&dp->atime))
		{
		    /* Might need to normalise if legacy */
		    outval->dn_seconds -= AD_TZ_OFFSETNEW(outval);
		    outval->dn_status2 |= AD_DN2_NEED_NORM;
		}
	    }
	    else
	    {	/* Clear TZ but do not mark valid */
		AD_TZ_SETNEW(outval, 0);
		if (outval->dn_dttype == DB_TMWO_TYPE)
		    outval->dn_status2 |= AD_DN2_IN_LCLTZ;
	    }
            outval->dn_status = (AD_DN_ABSOLUTE | AD_DN_TIMESPEC);
	    outval->dn_status2 |= AD_DN2_NO_DATE;
	    break;

	case DB_TSTMP_TYPE:	/* Ingres TIMESTAMP (WITH LOCAL TIME ZONE) */
	case DB_TSW_TYPE:		/* TIMESTAMP WITH TIME ZONE */
	case DB_TSWO_TYPE:	/* TIMESTAMP WITHOUT TIME ZONE */
            outval->dn_year = dp->atimestamp.dn_year;
            outval->dn_month = dp->atimestamp.dn_month;
            outval->dn_day = dp->atimestamp.dn_day;
            outval->dn_seconds = dp->atimestamp.dn_seconds;
            outval->dn_nsecond = dp->atimestamp.dn_nsecond;
            outval->dn_status = (AD_DN_ABSOLUTE  | AD_DN_YEARSPEC |
                                 AD_DN_MONTHSPEC | AD_DN_DAYSPEC  |
                                 AD_DN_TIMESPEC);
	    if (outval->dn_dttype == DB_TSW_TYPE)
	    {
		AD_TZ_SETNEW(outval, AD_TZ_OFFSET(&dp->atimestamp));
		outval->dn_status2 |= AD_DN2_TZ_VALID;
		if (AD_TZ_ISLEGACY(&dp->atimestamp))
		{
		    /* Might need to normalise if legacy */
		    outval->dn_seconds -= AD_TZ_OFFSETNEW(outval);
		    outval->dn_status2 |= AD_DN2_NEED_NORM;
		}
	    }
	    else
	    {	/* Clear TZ but do not mark valid */
		AD_TZ_SETNEW(outval, 0);
		if (outval->dn_dttype == DB_TSWO_TYPE)
		    outval->dn_status2 |= AD_DN2_IN_LCLTZ;
	    }
	    break;

	case DB_INYM_TYPE:	/* INTERVAL YEAR TO MONTH */
	    outval->dn_year = dp->aintym.dn_years;
	    outval->dn_month = dp->aintym.dn_months;
	    outval->dn_day = 0;
	    outval->dn_seconds = 0;
	    outval->dn_nsecond = 0;
	    outval->dn_status = (AD_DN_LENGTH | AD_DN_YEARSPEC | 
                                 AD_DN_MONTHSPEC); 
	    break;

	case DB_INDS_TYPE:	/* INTERVAL DAY TO SECOND */
	    outval->dn_status = AD_DN_LENGTH;
	    outval->dn_year = 0;
	    outval->dn_month = 0;
	    outval->dn_day = dp->aintds.dn_days;
	    outval->dn_seconds = dp->aintds.dn_seconds;
	    outval->dn_nsecond = dp->aintds.dn_nseconds;
	    if (outval->dn_day != 0)
		outval->dn_status |= AD_DN_DAYSPEC;
	    if (outval->dn_seconds != 0 ||
		outval->dn_nsecond != 0)
		outval->dn_status |= AD_DN_TIMESPEC;
	    break;
	
	default:
	    if (adf_scb)
		return(adu_error(adf_scb, E_AD1001_BAD_DATE, 1,
					outval->dn_dttype));
	}
    }
    else if (adf_scb)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    else
	db_stat = E_AD9999_INTERNAL_ERROR;
    return (db_stat);
}


/*
**  Name:  adu_dtntrnl_pend_date - set date if needed
**
**  Description: set date if pending - only call via ADU_NEWDT_PEND_DATE
**	as the flag test is external to this routine.
**
**  Parameters:
**	inval		- ptr to AD_NEWDTNTRNL from which value is copied
**
**  Returns:
**      none
**
**  History
**	28-Mar-2008 (kiria01) b120004
**	    Created to allow pending date to be set.
*/
DB_STATUS
adu_dtntrnl_pend_date (
ADF_CB         *adf_scb,
AD_NEWDTNTRNL	*inval)
{
    DB_STATUS   db_stat = E_DB_OK;
    struct timevect cur_tv;
    i4 tzoff;

    /* Initialize current time vector */ 
    if (db_stat = ad0_1cur_init(adf_scb, &cur_tv, FALSE, FALSE, &tzoff))
	return (db_stat);

    /* No date component provided and we need one */
    inval->dn_year	= cur_tv.tm_year + AD_TV_NORMYR_BASE;
    inval->dn_month	= cur_tv.tm_mon + AD_TV_NORMMON;
    inval->dn_day	= cur_tv.tm_mday;
    inval->dn_status |= AD_DN_YEARSPEC|AD_DN_MONTHSPEC|
			AD_DN_DAYSPEC; 
    inval->dn_status2 &= ~AD_DN2_NO_DATE;

    return db_stat;
}


/*
**  Name:  adu_dtntrnl_toGMT - set time to GMT
**
**  Description:
**	Set time to GMT if needed - only call via ADU_NEWDT_TO_GMT
**	as the flag test is external to this routine.
**
**  Parameters:
**	adf_scb		ADF control block for error handling
**	dn		ptr to AD_NEWDTNTRNL from which value is copied
**			and will be updated
**
**  Returns:
**      none
**
**  History
**	10-Nov-2011 (kiria01) b125945
**	    Created to allow TZ handling to be more streamlined.
*/
DB_STATUS
adu_dtntrnl_toGMT (
	ADF_CB		*adf_scb,
	AD_NEWDTNTRNL	*dn)
{
    DB_STATUS db_stat = ADU_NEWDT_PEND_TZ(adf_scb, dn, TM_TIMETYPE_LOCAL);

    if (db_stat == E_DB_OK)
    {
	/* We adjust for GMT */
	dn->dn_seconds -= AD_TZ_OFFSETNEW(dn);
	dn->dn_status |= AD_DN_TIMESPEC;  /* Set as seconds will yet be normed */
	dn->dn_status2 &= ~AD_DN2_IN_LCLTZ;
	dn->dn_status2 |= AD_DN2_NEED_NORM;
    }
    return db_stat;
}


/*
**  Name:  adu_dtntrnl_toLCL - set time to LOCAL (non-GMT)
**
**  Description:
**	Set time to Local TZ if needed - only call via ADU_NEWDT_TO_LCLTZ
**	as the flag test is external to this routine.
**
**  Parameters:
**	adf_scb		ADF control block for error handling
**	dn		ptr to AD_NEWDTNTRNL from which value is copied
**			and will be updated
**
**  Returns:
**      none
**
**  History
**	10-Nov-2011 (kiria01) b125945
**	    Created to allow TZ handling to be more streamlined.
*/
DB_STATUS
adu_dtntrnl_toLCL (
	ADF_CB		*adf_scb,
	AD_NEWDTNTRNL	*dn)
{
    DB_STATUS db_stat = ADU_NEWDT_PEND_TZ(adf_scb, dn, TM_TIMETYPE_GMT);

    if (db_stat == E_DB_OK)
    {
	dn->dn_seconds += AD_TZ_OFFSETNEW(dn);
	dn->dn_status |= AD_DN_TIMESPEC;  /* Set as seconds will yet be normed */
	dn->dn_status2 |= AD_DN2_IN_LCLTZ | AD_DN2_NEED_NORM;
    }
    return db_stat;
}


/*
**  Name:  adu_dtntrnl_pend_tz - set timezone field
**
**  Description:
**	Make the TZ valid by setting with teh standard TZ - only call via
**	ADU_NEWDT_PEND_TZ as the flag test is external to this routine.
**
**  Parameters:
**	adf_scb		ADF control block for error handling
**	dn		ptr to AD_NEWDTNTRNL from which value is copied
**	src_type	Either TM_TIMETYPE_GMT or TM_TIMETYPE_LOCAL
**			indicating whether dn is in GMT or LCLTZ
**
**  Returns:
**      none
**
**  History
**	10-Nov-2011 (kiria01) b125945
**	    Created to allow TZ handling to be more streamlined.
*/
DB_STATUS
adu_dtntrnl_pend_tz (
	ADF_CB		*adf_scb,
	AD_NEWDTNTRNL	*dn,
	TM_TIMETYPE	src_type)
{
    DB_STATUS db_stat = E_DB_OK;
    i4 offset = 0;

    if (adf_scb && (db_stat = ad0_tz_offset(adf_scb, dn,
					src_type, &offset)))
	return db_stat;
    AD_TZ_SETNEW(dn, offset);
    /* We note that tz is saved */
    dn->dn_status2 |= AD_DN2_TZ_VALID;
    return db_stat;
}


/*
**  Name:  adu_7from_dtntrnl	- map a value of one of the supported types
**	from the ADF internal format to the storage format
**
**  Description:
**
**  Parameters:
**      outdv		- ptr to DB_DATA_VALUE for output value
**	inval		- ptr to AD_NEWDTNTRNL from which value is copied
**
**  Returns:
**      none
**
**  History
**	21-apr-06 (dougi)
**	    Written for date/time enhancements.
**	17-jun-06 (gupsh01)
**	    Fixed the DB_DTE_TYPE low/high day calculation.
**	01-aug-06 (gupsh01)
**	    Fixed time calculation for DB_DTE_TYPE considering
**	    the internal structure now holds nanoseconds.
**	08-sep-06 (gupsh01)
**	    Fixed the sigbus error calling this function.
**	16-sep-06 (gupsh01)
**	    Make sure that we check for absolute data type value.
**	20-oct-06 (gupsh01)
**	    Protect this routine from segv if the input and output
**	    pointers are uninitialized.
**	5-Nov-07 (kibro01) b119412
**	    Apply timezone and normalise since AD_DATENTRNL doesn't
**	    have a timezone, so it would get lost.
**	18-Feb-2008 (kiria01) b120004
**	    Reworked to do the brunt of conversion work and added
**	    handling.
**	11-May-2008 (kiria01) b120004
**	    Apply any defered application of time/timezone if needed.
**	20-May-2008 (kiria01) b120004
**	    When converting from pure date to TSWO or TMWO, leave the
**	    time as 00:00. Otherwise, when converting to TSWO or TMWO
**	    denormalise any TZ specified. Correct INTDS rounding error.
**	26-Nov-2008 (kibro01) b121250
**	    Adjust output value for a DTE type based on the required
**	    output precision so we don't end up with extraneous nanoseconds.
**	12-Dec-2008 (kiria01) b121366
**	    Correct potential abend on outdv
**	12-Oct-2011 (kiria01) b125835 
**	    Updated to support INTERVAL DTOS to INT coercion in the context
**	    of ANSIDATE-ANSIDATE. Making this routine able to handle an INT
**	    return type alongside the fuller DB_INDS_TYPE is a little cleaner
**	    that messing about with the other subtraction code.
*/
DB_STATUS
adu_7from_dtntrnl(
ADF_CB         *adf_scb,
DB_DATA_VALUE	*outdv,
AD_NEWDTNTRNL	*inval)
{
    DB_STATUS   db_stat = E_DB_OK;
    AD_DTUNION	*dp, _dp;
    i4		nsecs;

    /* Switch on target data type and copy the fields over. */
    if (outdv && inval && (dp = (AD_DTUNION*)outdv->db_data))
    {
	if (ME_ALIGN_MACRO(dp, sizeof(i4)) != (PTR)dp)
	    dp = &_dp;
	switch (abs(outdv->db_datatype))
	{
	case DB_TME_TYPE:
	case DB_TMW_TYPE:
	case DB_TMWO_TYPE:
	case DB_INYM_TYPE:
	case DB_INDS_TYPE:
	case DB_INT_TYPE:
	    /* None of these types will use the date */
	    break;
	case DB_DTE_TYPE:
	    if (!(inval->dn_status & AD_DN_ABSOLUTE))
		break;
	    /*FALLTHROUGH*/
	default:
	    /* Getdate assigned if delayed */ 
	    if (db_stat = ADU_NEWDT_PEND_DATE(adf_scb, inval))
		return db_stat;
	}
	switch (abs(outdv->db_datatype))
	{
	case DB_DTE_TYPE:
	    if ((inval->dn_status & (AD_DN_ABSOLUTE|AD_DN_TIMESPEC)) ==
				    (AD_DN_ABSOLUTE|AD_DN_TIMESPEC) &&
		    (db_stat = ADU_NEWDT_TO_GMT(adf_scb, inval)))
		return db_stat;
	    if (inval->dn_status2 & AD_DN2_NEED_NORM)
	    {
		if (inval->dn_status & AD_DN_ABSOLUTE)
		    adu_1normldate(inval);
		else
	    	    adu_2normldint(inval);
	    }

	    nsecs = inval->dn_nsecond;

	    /* Now adjust the nanoseconds based on the output value precision
	    ** if putting result in an absolute date. (kibro01) b121250
	    */
	    if (inval->dn_status & AD_DN_ABSOLUTE)
	    {
        	if (outdv->db_prec <= 0 || outdv->db_prec > 9)
                    nsecs = 0;
        	else
        	{
                    nsecs /= nano_mult[outdv->db_prec];
                    nsecs *= nano_mult[outdv->db_prec];
        	}
	    }

	    dp->ing_date.dn_year = inval->dn_year;
	    dp->ing_date.dn_month = inval->dn_month;
	    dp->ing_date.dn_highday = (inval->dn_day >> AD_21DTE_LOWDAYSIZE);
	    dp->ing_date.dn_lowday = (inval->dn_day & AD_22DTE_LOWDAYMASK);
	    dp->ing_date.dn_time = inval->dn_seconds * AD_6DTE_IMSPERSEC + 
				    nsecs/AD_29DTE_NSPERMS;
	    dp->ing_date.dn_status = inval->dn_status;
    
	    /* Make sure that the status field is 
	    ** consistent This will help fix up conversion to string 
	    ** If the date value may change due to various operations. 
	    */
	    if (dp->ing_date.dn_time)
		dp->ing_date.dn_status |= AD_DN_TIMESPEC;
	    /* If no date bits were set and we're absolute then treat
	    ** this as a time and don't update bits*/
	    if ((inval->dn_status2 & AD_DN2_DATE_DEFAULT) &&
		(dp->ing_date.dn_status & (
			AD_DN_YEARSPEC|AD_DN_MONTHSPEC|
			AD_DN_DAYSPEC|AD_DN_ABSOLUTE)) == AD_DN_ABSOLUTE)
		break;
	    if (dp->ing_date.dn_year)
		dp->ing_date.dn_status |= AD_DN_YEARSPEC;
	    if (dp->ing_date.dn_month)
		dp->ing_date.dn_status |= AD_DN_MONTHSPEC;
	    if (dp->ing_date.dn_lowday)
		dp->ing_date.dn_status |= AD_DN_DAYSPEC;
	    break;

        case DB_ADTE_TYPE:
	    if (db_stat = ADU_NEWDT_TO_LCLTZ(adf_scb, inval))
		return db_stat;
	    if (inval->dn_status2 & AD_DN2_NEED_NORM)
		adu_1normldate(inval);
	    dp->adate.dn_year = inval->dn_year;
	    dp->adate.dn_month = inval->dn_month;
	    dp->adate.dn_day = inval->dn_day;
	    break;

	case DB_TMW_TYPE:	/* TIME WITH TIME ZONE */
	    if (db_stat = ADU_NEWDT_TO_GMT(adf_scb, inval))
		return db_stat;
	    if (db_stat = ADU_NEWDT_PEND_TZ(adf_scb, inval, TM_TIMETYPE_GMT))
		return db_stat;
	    AD_TZ_SET(&dp->atime, AD_TZ_OFFSETNEW(inval));
	    goto TM_common;

	case DB_TMWO_TYPE:	/* TIME WITHOUT TIME ZONE */
	    if (db_stat = ADU_NEWDT_TO_LCLTZ(adf_scb, inval))
		return db_stat;
	    /* Clear TZ in structure */
	    AD_TZ_SET(&dp->atime, 0);
	    goto TM_common;

	case DB_TME_TYPE:	/* Ingres TIME (WITH LOCAL TIME ZONE) */
	    if (db_stat = ADU_NEWDT_TO_GMT(adf_scb, inval))
		return db_stat;
	    /* Clear TZ in structure */
	    AD_TZ_SET(&dp->atime, 0);

TM_common:  if (inval->dn_status2 & AD_DN2_NEED_NORM)
		adu_1normldate(inval);
	    dp->atime.dn_seconds = inval->dn_seconds;
	    dp->atime.dn_nsecond = inval->dn_nsecond;
	    break;

        case DB_TSW_TYPE:	/* TIMESTAMP WITH TIME ZONE */
	    if (db_stat = ADU_NEWDT_TO_GMT(adf_scb, inval))
		return db_stat;
	    if (db_stat = ADU_NEWDT_PEND_TZ(adf_scb, inval, TM_TIMETYPE_GMT))
		return db_stat;
	    AD_TZ_SET(&dp->atimestamp, AD_TZ_OFFSETNEW(inval));
	    goto TS_common;

	case DB_TSWO_TYPE:	/* TIMESTAMP WITHOUT TIME ZONE */
	    if (db_stat = ADU_NEWDT_TO_LCLTZ(adf_scb, inval))
		return db_stat;
	    /* Clear TZ in structure */
	    AD_TZ_SET(&dp->atimestamp, 0);
	    goto TS_common;

	case DB_TSTMP_TYPE:	/* Ingres TIMESTAMP (WITH LOCAL TIME ZONE) */
	    /* Clear TZ in structure */
	    if (db_stat = ADU_NEWDT_TO_GMT(adf_scb, inval))
		return db_stat;
	    AD_TZ_SET(&dp->atimestamp, 0);

TS_common:  if (inval->dn_status2 & AD_DN2_NEED_NORM)
		adu_1normldate(inval);
	    dp->atimestamp.dn_year = inval->dn_year;
	    dp->atimestamp.dn_month = inval->dn_month;
	    dp->atimestamp.dn_day = inval->dn_day;
	    dp->atimestamp.dn_seconds = inval->dn_seconds;
	    dp->atimestamp.dn_nsecond = inval->dn_nsecond;
	    break;

	case DB_INYM_TYPE:	/* INTERVAL YEAR TO MONTH */
	    adu_2normldint(inval);
	    dp->aintym.dn_years	= inval->dn_year;
	    dp->aintym.dn_months = inval->dn_month;
	    
	    if (adf_scb && (inval->dn_status & AD_DN_ABSOLUTE))
		return(adu_error(adf_scb, E_AD505E_NOABSDATES, 0));
	    break;

	case DB_INDS_TYPE:	/* INTERVAL DAY TO SECOND */
	    adu_2normldint(inval);
	    dp->aintds.dn_days	= inval->dn_day;
	    if (inval->dn_month | inval->dn_year)
	    {
		f8 days = inval->dn_month * AD_13DTE_DAYPERMONTH +
			    inval->dn_year * AD_14DTE_DAYPERYEAR;
		if (days < 0)
		    days -= 0.5;
		else
		    days += 0.5;
		dp->aintds.dn_days += (i4)days;
	    }
	    dp->aintds.dn_seconds = inval->dn_seconds;
	    dp->aintds.dn_nseconds = inval->dn_nsecond;
	    
	    if (adf_scb && (inval->dn_status & AD_DN_ABSOLUTE))
		return(adu_error(adf_scb, E_AD505E_NOABSDATES, 0));
	    break;

	case DB_INT_TYPE:	/* INTERVAL DAY as INT */
	    {
		/* If INT used as result type then we have INTERVAL DAY
		** and we use just the first field of dp->aintds ie.
		** .dn_days.
		** Presently this is only used for ADTE-ADTE which
		** returns this short form of INDS. */
		i4 days;
		if (adf_scb && (inval->dn_status & AD_DN_ABSOLUTE))
		    return(adu_error(adf_scb, E_AD505E_NOABSDATES, 0));
		if (adf_scb && outdv->db_length != (i4)sizeof(i4))
		    return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
		adu_2normldint(inval);
		dp->aintds.dn_days = inval->dn_day;
		if (inval->dn_month | inval->dn_year)
		{
		    f8 days = inval->dn_month * AD_13DTE_DAYPERMONTH +
				inval->dn_year * AD_14DTE_DAYPERYEAR;
		    if (days < 0)
			days -= 0.5;
		    else
			days += 0.5;
		    dp->aintds.dn_days += days;
		}
	    }
	    break;

	default:
	    if (adf_scb)
		return(adu_error(adf_scb, E_AD1001_BAD_DATE, 1,
					abs(outdv->db_datatype)));
	}
	if (dp == &_dp)
	    MEcopy((PTR)dp, outdv->db_length, outdv->db_data);
    }
    else if (adf_scb)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    else
	db_stat = E_AD9999_INTERNAL_ERROR;
    return (db_stat);
}

/*
**  Name:  adu_datefmt() - Formatted date output after MySQL DATE_FORMAT.
**
**  Description:
**	This is used for formatting various parts of a datetime
**
**	Based on MySQL implementation but aliased for both time_format and
**	date_format based on passed datatypes.
**
**	NOTE: can generate explicit NULL so needs null_preinst ADE_0CXI_IGNORE
**	and adi_res_type() adjustment.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**      dv2		DBV Format string. Must be VCH
**	rdv		DBV buffer for output. Must be VCH
**
**  Returns:
**      day number in year
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for DATE_FORMAT
*/
DB_STATUS
adu_datefmt(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,	/* date */
	DB_DATA_VALUE	*dv2,	/* format */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    char *fmt, *fmtend, *out, *end;
    AD_NEWDTNTRNL nd;
    i4 year;
    i4 month;
    i4 day;
    i4 secs;
    i4 nsecs;
    i4 mins;
    i4 hours;
    i4 i, doy, jan1;
    DB_DATA_VALUE tdv1, tdv2;
    bool nodate;


    if (dv1->db_datatype < 0)
    {
	if (ADI_ISNULL_MACRO(dv1))
	    goto return_NULL;
	tdv1 = *dv1;
	dv1 = &tdv1;
	dv1->db_datatype = -dv1->db_datatype;
	dv1->db_length--;
    }

    if (dv2->db_datatype < 0)
    {
	if (ADI_ISNULL_MACRO(dv2))
	    goto return_NULL;
	tdv2 = *dv2;
	dv2 = &tdv2;
	dv2->db_datatype = -dv2->db_datatype;
	dv2->db_length--;
    }
    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd))
	return status;
    nodate = ((nd.dn_status2 & (AD_DN2_DATE_DEFAULT|AD_DN2_NO_DATE)) != 0 ||
		(nd.dn_status & AD_DN_ABSOLUTE) == 0); 
    if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd))
	return status;
    if (status = ADU_NEWDT_TO_LCLTZ(adf_scb, &nd))
	return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    if (dv2->db_datatype != DB_VCH_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    if (abs(rdv->db_datatype) != DB_VCH_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    fmt = dv2->db_data + sizeof(u_i2);
    fmtend = fmt + *(u_i2*)dv2->db_data;
    out = rdv->db_data + sizeof(u_i2);
    end = rdv->db_data + rdv->db_length;
    if (rdv->db_datatype < 0)
    {
	end--;
	ADF_CLRNULL_MACRO(rdv);
    }
    year = nd.dn_year;
    month = nd.dn_month;
    day = nd.dn_day;
    secs = nd.dn_seconds;
    nsecs = nd.dn_nsecond;
    mins = secs / 60;
    secs -= mins * 60;
    hours = mins / 60;
    mins -= hours * 60;

    while (fmt < fmtend)
    {
	if (*fmt != '%')
	{
	    i4 l = CMbytecnt(fmt);
	    if (out + l > end)
		goto overflow;
	    while(l--)
		*out++ = *fmt++;
	}
	else
	{
	    fmt++; /* Skip % */
	    if (fmt >= fmtend)
		/* If this is a trailing % just emit it */
		fmt--;
	    switch (*fmt++)
	    {
	    case 'a': /* Abbreviated weekday name (Sun..Sat) */
		if (nodate)
		    goto return_NULL;
		i = adu_3llday_of_date(year, month, day) + SUNDAYOFFSET;
		if (out + 3 > end)
		    goto overflow;
		strncpy(out, weekdays[i % 7], 3);
		out += 3;
		break;
	    case 'b': /* Abbreviated month name (Jan..Dec) */
		if (nodate)
		    goto return_NULL;
		if (out + 3 > end)
		    goto overflow;
		strncpy(out, monthName[month], 3);
		out += 3;
		break;
	    case 'm': /* Month, numeric (00..12) */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		if (month < 10)
		    *out++ = '0';
		CVla(month, out);
		goto commoninc;
	    case 'c': /* Month, numeric (0..12) */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		CVla(month, out);
		goto commoninc;
	    case 'D': /* Day of the month with English suffix (0th, 1st, 2nd, 3rd, …) */
		if (nodate)
		    goto return_NULL;
		if (out + 4 > end)
		    goto overflow;
		CVla(day, out);
		out += strlen(out);
		if ((day / 10) % 10 == 1)
		    strcpy(out, "th");
		else switch (day % 10)
		{
		case 1: strcpy(out, "st"); break;
		case 2: strcpy(out, "nd"); break;
		case 3: strcpy(out, "rd"); break;
		default: strcpy(out, "th"); break;
		}
		goto commoninc;
	    case 'd': /* Day of the month, numeric (00..31) */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		if (day < 10) *out++ = '0';
		CVla(day, out);
		goto commoninc;
	    case 'e': /* Day of the month, numeric (0..31) */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		CVla(day, out);
		goto commoninc;
	    case 'f': /* Microseconds (000000..999999) */
		if (out + 6 > end)
		    goto overflow;
		STprintf(out, "%06d", nsecs / 1000);
		out += 6;
		break;
	    case 'H': /* Hour (00..23) */
		if (out + 2 > end)
		    goto overflow;
		if (hours < 10) *out++ = '0';
		CVla(hours, out);
		goto commoninc;
	    case 'k': /* Hour (0..23) */
		if (out + 2 > end)
		    goto overflow;
		CVla(hours, out);
		goto commoninc;
	    case 'h': /* Hour (01..12) */
	    case 'I': /* Hour (01..12) */
		if (out + 2 > end)
		    goto overflow;
		{
		    i4 hours12 = hours % 12;
		    if (hours12 == 0) hours12 = 12;
		    if (hours12 < 10) *out++ = '0';
		    CVla(hours12, out);
		}
		goto commoninc;
	    case 'i': /* Minutes, numeric (00..59) */
		if (out + 2 > end)
		    goto overflow;
		if (mins < 10) *out++ = '0';
		CVla(mins, out);
		goto commoninc;
	    case 'j': /* Day of year (001..366) */
		if (out + 3 > end)
		    goto overflow;
		STprintf(out, "%03d", adu_llday_of_year(year, month, day));
		out += 3;
		break;
	    case 'l': /* Hour (1..12) */
		if (out + 2 > end)
		    goto overflow;
		{
		    i4 hours12 = hours % 12;
		    if (hours12 == 0) hours12 = 12;
		    CVla(hours12, out);
		}
		goto commoninc;
	    case 'M': /* Month name (January..December) */
		if (nodate)
		    goto return_NULL;
		if (out + 10 > end)
		    goto overflow;
		strcpy(out, monthName[month]);
		goto commoninc;
	    case 'r': /* Time, 12-hour (hh:mm:ss followed by AM or PM) */
		if (out + 11 > end)
		    goto overflow;
		if (hours % 12 < 10) *out++ = '0';
		CVla(hours % 12, out);
		out += strlen(out);
		*out++ = ':';
		if (mins < 10) *out++ = '0';
		CVla(mins, out);
		out += strlen(out);
		*out++ = ':';
		if (secs < 10) *out++ = '0';
		CVla(secs, out);
		out += strlen(out);
		*out++ = ' ';
		/*FALLTHROUGH*/
	    case 'p': /* AM or PM */
		if (out + 2 > end)
		    goto overflow;
		*out++ = (hours < 12) ? 'A' : 'P';
		*out++ = 'M';
		break;
	    case 'S': /* Seconds (00..59) */
	    case 's': /* Seconds (00..59) */
		if (out + 2 > end)
		    goto overflow;
		if (secs < 10)
		    *out++ = '0';
		CVla(secs, out);
		goto commoninc;
	    case 'T': /* Time, 24-hour (hh:mm:ss) */
		if (out + 8 > end)
		    goto overflow;
		if (hours < 10) *out++ = '0';
		CVla(hours, out);
		out += strlen(out);
		*out++ = ':';
		if (mins < 10) *out++ = '0';
		CVla(mins, out);
		out += strlen(out);
		*out++ = ':';
		if (secs < 10) *out++ = '0';
		CVla(secs, out);
		goto commoninc;
	    case 'U': /* Week (00..53), where Sunday is the first day of the week */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		jan1 = adu_3llday_of_date(year, 1, 1) + SUNDAYOFFSET;
		doy = adu_llday_of_year(year, month, day) - 1;
		i = (doy + 7 - offsetSun[jan1 % 7]) / 7;
		if (i < 10) *out++ = '0';
		CVla(i, out);
		goto commoninc;
	    case 'u': /* Week (00..53), where Monday is the first day of the week */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		jan1 = adu_3llday_of_date(year, 1, 1) + SUNDAYOFFSET;
		doy = adu_llday_of_year(year, month, day) - 1;
		i = (doy + 7 - offsetMon[jan1 % 7]) / 7;
		if (i < 10) *out++ = '0';
		CVla(i, out);
		goto commoninc;
	    case 'V': /* Week (01..53), where Sunday is the first day of the week; used with %X */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		jan1 = adu_3llday_of_date(year, 1, 1) + SUNDAYOFFSET;
		doy = adu_llday_of_year(year, month, day) - 1;
		i = (doy + 7 - offsetSun[jan1 % 7])/7;
		if (i < 1)
		{
		    jan1 = adu_3llday_of_date(year-1, 1, 1) + SUNDAYOFFSET;
		    doy += adu_llday_of_year(year-1, 12, 31);
		    i = (doy + 7 - offsetSun[jan1 % 7])/7;
		}
		if (i < 10) *out++ = '0';
		CVla(i, out);
		goto commoninc;
	    case 'v': /* Week (01..53), where Monday is the first day of the week; used with %x */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		jan1 = adu_3llday_of_date(year, 1, 1) + SUNDAYOFFSET;
		doy = adu_llday_of_year(year, month, day) - 1;
		if (doy < offsetMon[jan1 % 7])
		{
		    jan1 = adu_3llday_of_date(year-1, 1, 1) + SUNDAYOFFSET;
		    doy += adu_llday_of_year(year-1, 12, 31);
		}
		if (month == 12 && day >= 29 &&
			    day >= declimit[(adu_3llday_of_date(year+1, 1, 1) + SUNDAYOFFSET)%7])
		    i = 1;
		else
		    i = (doy - offsetMon[jan1 % 7]) / 7 + 1;
		if (i < 10) *out++ = '0';
		CVla(i, out);
		goto commoninc;
	    case 'W': /* Weekday name (Sunday..Saturday) */
		if (nodate)
		    goto return_NULL;
		if (out + 10 > end)
		    goto overflow;
		i = adu_3llday_of_date(year, month, day) + SUNDAYOFFSET;
		strcpy(out, weekdays[i % 7]);
		goto commoninc;
	    case 'w': /* Day of the week (0=Sunday..6=Saturday) */
		if (nodate)
		    goto return_NULL;
		if (out + 1 > end)
		    goto overflow;
		i = adu_3llday_of_date(year, month, day) + SUNDAYOFFSET;
		CVla(i % 7, out);
		goto commoninc;
	    case 'X': /* Year for the week where Sunday is the first day of the week, numeric, four digits; used with %V */
		if (nodate)
		    goto return_NULL;
		if (out + 4 > end)
		    goto overflow;
		jan1 = adu_3llday_of_date(year, 1, 1) + SUNDAYOFFSET;
		i = year;
		if (adu_llday_of_year(year, month, day) - 1 < offsetSun[jan1 % 7])
		    i--;
		CVla(i, out);
		goto commoninc;
	    case 'x': /* Year for the week, where Monday is the first day of the week, numeric, four digits; used with %v */
		if (nodate)
		    goto return_NULL;
		if (out + 4 > end)
		    goto overflow;
		jan1 = adu_3llday_of_date(year, 1, 1) + SUNDAYOFFSET;
		doy = adu_llday_of_year(year, month, day) - 1;
		i = year;
		if (doy < offsetMon[jan1 % 7])
		    i--;
		if (month == 12 && day >= 29 &&
			    day >= declimit[(adu_3llday_of_date(year+1, 1, 1) + SUNDAYOFFSET)%7])
		    i++;
		CVla(i, out);
		goto commoninc;
	    case 'Y': /* Year, numeric, four digits */
		if (nodate)
		    goto return_NULL;
		if (out + 4 > end)
		    goto overflow;
		STprintf(out, "%04d", year);
		out += 4;
		break;
	    case 'y': /* Year, numeric (two digits) */
		if (nodate)
		    goto return_NULL;
		if (out + 2 > end)
		    goto overflow;
		if (year % 100 < 10) *out++ = '0';
		CVla(year % 100, out);
		goto commoninc;
	    case EOS: /* Bad trailing format character emit % */
		fmt--;
		/*FALLTHROUGH*/
	    default:
		{
		    i4 l;
		    fmt--;
		    l = CMbytecnt(fmt);
		    if (out + l > end)
			goto overflow;
		    while(l--)
			*out++ = *fmt++;
		}
		break;
commoninc:	out += strlen(out);
		break;
overflow:	if (adf_scb->adf_strtrunc_opt == ADF_ERR_STRTRUNC)
		{
		    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE, 2,
				*(u_i2*)dv2->db_data,
				dv2->db_data + sizeof(u_i2));
		}
		*(u_i2*)rdv->db_data = out - rdv->db_data - sizeof(u_i2);
		return status;
	    }
	}
    }
    *(u_i2*)rdv->db_data = out - rdv->db_data - sizeof(u_i2);
    return status;

return_NULL:
    if (rdv->db_datatype >= 0)
	return adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
    ADF_SETNULL_MACRO(rdv);
    return E_DB_OK;
}

/*
**  Name:  adu_datefmt_calclen() - Formatted date calclen helper.
**
**  Description:
**	This is used for finding out how big the output should be.
**
**  Parameters:
**	adf_scb		ADF context
**      dv1		DBV Format string. Must be VCH
**	olen		max length of data formats into
**
**  Returns:
**      status
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for DATE_FORMAT
*/
DB_STATUS
adu_datefmt_calclen(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,
	i4		*olen)
{
    char *fmt, *fmtend;
    i4 len = 0;

    if (dv1->db_datatype != DB_VCH_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    fmt = dv1->db_data + sizeof(u_i2);
    fmtend = fmt + *(u_i2*)dv1->db_data;

    while (fmt < fmtend)
    {
	if (*fmt != '%')
	{
	    i4 l = CMbytecnt(fmt);
	    fmt += l;
	    len += l;
	}
	else
	{
	    fmt++; /* Skip % */
	    if (fmt >= fmtend)
		fmt--;
	    switch (*fmt++)
	    {
	    case 'a': /* Abbreviated weekday name (Sun..Sat) */
	    case 'b': /* Abbreviated month name (Jan..Dec) */
	    case 'j': /* Day of year (001..366) */
		len += 3;
		break;
	    case 'm': /* Month, numeric (00..12) */
	    case 'c': /* Month, numeric (0..12) */
	    case 'd': /* Day of the month, numeric (00..31) */
	    case 'e': /* Day of the month, numeric (0..31) */
	    case 'H': /* Hour (00..23) */
	    case 'k': /* Hour (0..23) */
	    case 'h': /* Hour (01..12) */
	    case 'I': /* Hour (01..12) */
	    case 'i': /* Minutes, numeric (00..59) */
	    case 'l': /* Hour (1..12) */
	    case 'p': /* AM or PM */
	    case 'S': /* Seconds (00..59) */
	    case 's': /* Seconds (00..59) */
	    case 'U': /* Week (00..53), where Sunday is the first day of the week */
	    case 'u': /* Week (00..53), where Monday is the first day of the week */
	    case 'V': /* Week (01..53), where Sunday is the first day of the week; used with %X */
	    case 'v': /* Week (01..53), where Monday is the first day of the week; used with %x */
	    case 'y': /* Year, numeric (two digits) */
		len += 2;
		break;
	    case 'D': /* Day of the month with English suffix (0th, 1st, 2nd, 3rd, …) */
	    case 'X': /* Year for the week where Sunday is the first day of the week, numeric, four digits; used with %V */
	    case 'x': /* Year for the week, where Monday is the first day of the week, numeric, four digits; used with %v */
	    case 'Y': /* Year, numeric, four digits */
		len += 4;
		break;
	    case 'f': /* Microseconds (000000..999999) */
		len += 6;
		break;
	    case 'M': /* Month name (January..December) */
	    case 'W': /* Weekday name (Sunday..Saturday) */
		len += 10;
		break;
	    case 'r': /* Time, 12-hour (hh:mm:ss followed by AM or PM) */
		len += 11;
		break;
	    case 'T': /* Time, 24-hour (hh:mm:ss) */
		len += 8;
		break;
	    case 'w': /* Day of the week (0=Sunday..6=Saturday) */
		len++;
		break;
	    case EOS: /* Bad trailing format character emit % */
		/*FALLTHROUGH*/
	    default:
		{
		    i4 l;
		    fmt--;
		    l = CMbytecnt(fmt);
		    fmt += l;
		    len += l;
		}
		break;
	    }
	}
    }
    *olen = len;
    return E_DB_OK;
}

/*
**  Name:  adu_dayofwk1() - Return day of week.
**
**  Description:
**	Returns the weekday index for date (1 = Sunday, 2 = Monday, ... 7 = Saturday). 
**
**	Based on MySQL implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output. Must be i2
**
**  Returns:
**      day number in week
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for DAYOFWEEK()
*/
DB_STATUS
adu_dayofwk1(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    AD_NEWDTNTRNL nd;
    i4 n;

    memset(&nd, 0, sizeof(nd));
    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd))
	return status;

    if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd))
	return status;
    if (status = ADU_NEWDT_TO_LCLTZ(adf_scb, &nd))
	return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    if (rdv->db_datatype != DB_INT_TYPE || rdv->db_length != sizeof(i2))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    /* Get week day 0..6 */
    n = (adu_3llday_of_date(nd.dn_year, nd.dn_month, nd.dn_day) + SUNDAYOFFSET) % 7;
    /* Offset result */
    n++;

    /* Return value */
    *(i2*)rdv->db_data = n;

    return status;
}

/*
**  Name:  adu_dayofwk2() - Return day of week.
**
**  Description:
**	Returns the weekday index for date (1 = Sunday, 2 = Monday, ... 7 = Saturday). 
**
**	Based on ? implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output. Must be i2
**
**  Returns:
**      day number in week
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for DAYOFWEEK()
**	13-Oct-2011 (kiria01) SIR 125693
**	    Adjust offset to match data.
*/
DB_STATUS
adu_dayofwk2(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date */
	DB_DATA_VALUE	*dv2,		/* int */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    AD_NEWDTNTRNL nd;
    i4 n, offset;

    memset(&nd, 0, sizeof(nd));
    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd))
	return status;
  
    if (dv2->db_datatype != DB_INT_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    switch (dv2->db_length)
    {
    case sizeof(i1):
	offset = *(i1*)dv2->db_data;
	break;
    case sizeof(i2):
	offset = *(i2*)dv2->db_data;
	break;
    case sizeof(i4):
	offset = *(i4*)dv2->db_data;
	break;
    default:
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    }
    if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd))
	return status;
    if (status = ADU_NEWDT_TO_LCLTZ(adf_scb, &nd))
	return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    if (rdv->db_datatype != DB_INT_TYPE || rdv->db_length != sizeof(i2))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    /* Get week day 0..6 */
    n = (adu_3llday_of_date(nd.dn_year, nd.dn_month, nd.dn_day) +
		SUNDAYOFFSET - offset) % 7;
    /* Offset result */
    n++;

    /* Return value */
    *(i2*)rdv->db_data = n;

    return status;
}

/*
**  Name:  adu_dayofmth() - Return day of month.
**
**  Description:
**	Returns the month day. 
**
**	Based on MySQL implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output. Must be i2
**
**  Returns:
**      day number in month
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for DAYOFWEEK()
*/
DB_STATUS
adu_dayofmth(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    AD_NEWDTNTRNL nd;

    memset(&nd, 0, sizeof(nd));
    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd))
	return status;

    if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd))
	return status;
    if (status = ADU_NEWDT_TO_LCLTZ(adf_scb, &nd))
	return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    if (rdv->db_datatype != DB_INT_TYPE || rdv->db_length != sizeof(i2))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    /* Trivial value */
    *(i2*)rdv->db_data = nd.dn_day;

    /* Return value */
    return status;
}

/*
**  Name:  adu_dayofyr() - Return day of year.
**
**  Description:
**	Returns the day of year 1..366. 
**
**	Based on MySQL implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output. Must be i2
**
**  Returns:
**      day number in year
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for DAYOFYEAR()/DOY()
*/
DB_STATUS
adu_dayofyr(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    AD_NEWDTNTRNL nd;
    i4 n;

    memset(&nd, 0, sizeof(nd));
    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd))
	return status;

    if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd))
	return status;
    if (status = ADU_NEWDT_TO_LCLTZ(adf_scb, &nd))
	return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    if (rdv->db_datatype != DB_INT_TYPE || rdv->db_length != sizeof(i2))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    /* Get day number 1..366 */
    n = adu_llday_of_year(nd.dn_year, nd.dn_month, nd.dn_day);

    /* Return value */
    *(i2*)rdv->db_data = n;

    return status;
}

/*
**  Name:  adu_unxtmstmp() - Return unix_timestamp
**
**  Description:
**	Returns the day as a count of seconds since 1-Jan-1970
**
**	Based on MySQL implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	rdv		DBV buffer for output. Must be i4
**
**  Returns:
**      unix timestamp
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for UNIX_TIMESTAMP()
*/
DB_STATUS
adu_unxtmstmp(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    SYSTIME t;

    if (rdv->db_datatype != DB_INT_TYPE || rdv->db_length != sizeof(i4))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    adf_scb->adf_const_const &= ~AD_CNST_DSUPLMT;
    adf_scb->adf_const_const |= AD_CNST_FOR_X100;
    TMnow(&t);

    /* Return value */
    *(i4*)rdv->db_data = t.TM_secs;

    return E_DB_OK;
}

/*
**  Name:  adu_unxtmstmp1() - Return date as seconds from 1-Jan-1970
**
**  Description:
**	Returns the time as number of seconds since 1-Jan-1970 
**
**	NOTE: can generate explicit NULL so needs null_preinst ADE_0CXI_IGNORE
**	and adi_res_type() adjustment.
**
**	Based on MySQL implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output. Must be i4
**
**  Returns:
**      day number in year
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for UNIX_TIMESTAMP(date)
**	30-Sep-2011 (kiria01) SIR 125806
**	    Correct TZ handling to fully normalize to GMT.
*/
DB_STATUS
adu_unxtmstmp1(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    DB_DATA_VALUE tdv1, trdv;
    AD_NEWDTNTRNL nd;
    i4 n;

    if (dv1->db_datatype < 0)
    {
	if (ADI_ISNULL_MACRO(dv1))
	    goto return_NULL;
	tdv1 = *dv1;
	dv1 = &tdv1;
	dv1->db_datatype = -dv1->db_datatype;
	dv1->db_length--;
    }
    memset(&nd, 0, sizeof(nd));
    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd))
	goto return_NULL;
    if (status = ADU_NEWDT_TO_GMT(adf_scb, &nd))
	    return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    /* If no date portion - return NULL */
    if ((nd.dn_status2 & (AD_DN2_DATE_DEFAULT|AD_DN2_NO_DATE)) != 0 ||
		(nd.dn_status & AD_DN_ABSOLUTE) == 0)
	goto return_NULL;

    trdv = *rdv;
    if (rdv->db_datatype < 0)
    {
	ADF_CLRNULL_MACRO(rdv);
	trdv.db_datatype = -trdv.db_datatype;
	trdv.db_length--;
    }
    if (trdv.db_datatype != DB_INT_TYPE || trdv.db_length != sizeof(i4))
	return adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0);

    n = adu_5time_of_date(&nd);
    if (n == MINI4 || n == MAXI4)
	goto return_NULL;

    /* Return value */
    *(i4*)rdv->db_data = n;
    return status;

return_NULL:
    if (rdv->db_datatype >= 0)
	return adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
    ADF_SETNULL_MACRO(rdv);
    return E_DB_OK;
}

/*
**  Name:  adu_fromunxtm1() - Return timestamp from unixtime.
**
**  Description:
**	Returns the timestamp equivalent for a unixtime.
**
**	Based on MySQL implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output. Must be tmwo
**
**  Returns:
**      day number in year
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for FROM_UNIXTIME(i4)
**	30-Sep-2011 (kiria01) SIR 125806
**	    Correct TZ handling to fully normalize to GMT.
*/
DB_STATUS
adu_fromunxtm1(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* i4 unixtime */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    struct timevect tv;
    AD_NEWDTNTRNL nd;
    i4 n;
    i4 offset = 0;
    if (dv1->db_datatype != DB_INT_TYPE || dv1->db_length > (i4)sizeof(i4))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    if (rdv->db_datatype != DB_TSWO_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    if (dv1->db_length == (i4)sizeof(i4))
	n = *(i4*)dv1->db_data;
    else if (dv1->db_length == (i4)sizeof(i2))
	n = *(i2*)dv1->db_data;
    else if (dv1->db_length == (i4)sizeof(i1))
	n = *(i1*)dv1->db_data;
    /* Pull apart passed unixtime with no nanoseconds */
    adu_cvtime(n, 0, &tv);

    nd.dn_year =	tv.tm_year + AD_TV_NORMYR_BASE;
    nd.dn_month =	tv.tm_mon + AD_TV_NORMMON;
    nd.dn_day =		tv.tm_mday;
    nd.dn_seconds =	tv.tm_hour * AD_39DTE_ISECPERHOUR +
				tv.tm_min * AD_10DTE_ISECPERMIN +
				tv.tm_sec;
    nd.dn_nsecond =	tv.tm_nsec;
    if (adf_scb && (status = ad0_tz_offset(adf_scb, &nd,
					TM_TIMETYPE_GMT, &offset)))
	return status;
    AD_TZ_SETNEW(&nd, offset);
    nd.dn_status =	AD_DN_ABSOLUTE|AD_DN_YEARSPEC|AD_DN_MONTHSPEC|
			AD_DN_DAYSPEC|AD_DN_TIMESPEC|AD_DN_AFTER_EPOCH;
    nd.dn_status2 =	AD_DN2_NEED_NORM|AD_DN2_TZ_VALID;
    return adu_7from_dtntrnl(adf_scb, rdv, &nd);
}

/*
**  Name:  adu_fromunxtm2() - Return timestamp from unixtime.
**
**  Description:
**	Returns the formatted string equivalent for a unixtime value using
**	DATE_FORMAT processing.
**
**	Based on MySQL implementation.
**
**	NOTE: can generate explicit NULL so needs null_preinst ADE_0CXI_IGNORE
**	and adi_res_type() adjustment.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	dv2		DBV format string ala DATE_FORMAT
**	rdv		DBV buffer for output. Must be tmwo
**
**  Returns:
**      day number in year
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for FROM_UNIXTIME(i4,fmt)
**	30-Sep-2011 (kiria01) SIR 125806
**	    Correct TZ handling to fully normalize to GMT.
**	13-Oct-2011 (kiria01)
**	    Widen input width as might not be i4.
*/
DB_STATUS
adu_fromunxtm2(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* i4 unixtime */
	DB_DATA_VALUE	*dv2,		/* vch format */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    struct timevect tv;
    AD_NEWDTNTRNL nd;
    DB_DATA_VALUE tdv1, tdv2;
    i4 offset = 0;
    i4 tim;

    if (dv1->db_datatype < 0)
    {
	if (ADI_ISNULL_MACRO(dv1))
	    goto return_NULL;
	tdv1 = *dv1;
	dv1 = &tdv1;
	dv1->db_datatype = -dv1->db_datatype;
	dv1->db_length--;
    }

    if (dv2->db_datatype < 0)
    {
	if (ADI_ISNULL_MACRO(dv2))
	    goto return_NULL;
	tdv2 = *dv2;
	dv2 = &tdv2;
	dv2->db_datatype = -dv2->db_datatype;
	dv2->db_length--;
    }

    if (dv1->db_datatype != DB_INT_TYPE || dv1->db_length > (i4)sizeof(i4))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    if (dv2->db_datatype != DB_VCH_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    if (abs(rdv->db_datatype) != DB_VCH_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    if (dv1->db_length == (i4)sizeof(i4))
	tim = *(i4*)dv1->db_data;
    else if (dv1->db_length == (i4)sizeof(i2))
	tim = *(i2*)dv1->db_data;
    else if (dv1->db_length == (i4)sizeof(i1))
	tim = *(i1*)dv1->db_data;
    else
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    /* Pull apart passed unixtime with no nanoseconds */
    adu_cvtime(tim, 0, &tv);

    /* Setup full AD_NEWDTNTRNL */
    nd.dn_year =	tv.tm_year + AD_TV_NORMYR_BASE;
    nd.dn_month =	tv.tm_mon + AD_TV_NORMMON;
    nd.dn_day =		tv.tm_mday;
    nd.dn_seconds =	tv.tm_hour * AD_39DTE_ISECPERHOUR +
				tv.tm_min * AD_10DTE_ISECPERMIN +
				tv.tm_sec;
    nd.dn_nsecond =	tv.tm_nsec;
    if (adf_scb && (status = ad0_tz_offset(adf_scb, &nd,
					TM_TIMETYPE_GMT, &offset)))
	return status;
    AD_TZ_SETNEW(&nd, offset);
    nd.dn_status =	AD_DN_ABSOLUTE|AD_DN_YEARSPEC|AD_DN_MONTHSPEC|
			AD_DN_DAYSPEC|AD_DN_TIMESPEC|AD_DN_AFTER_EPOCH;
    nd.dn_status2 =	AD_DN2_NEED_NORM|AD_DN2_TZ_VALID;

    /* Pass on AD_NEWDTNTRNL as DB_DTE_TYPE to avoid extra conversions */
    tdv1.db_length = (i4)sizeof(nd);
    tdv1.db_datatype = DB_DTE_TYPE;
    tdv1.db_data = (PTR)&nd;
    tdv1.db_prec = 0;
    tdv1.db_collID = DB_NOCOLLATION;
    /* Let date_format finish off */
    return adu_datefmt(adf_scb, &tdv1, dv2, rdv);

return_NULL:
    if (rdv->db_datatype >= 0)
	return adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
    ADF_SETNULL_MACRO(rdv);
    return E_DB_OK;
}

/*
**  Name:  adu_lastday() - Return last day of month
**
**  Description:
**	Returns the last day of the month for the given date
**
**	Based on MySQL implementation but returning same type as argument.
**
**	NOTE: can generate explicit NULL so needs null_preinst ADE_0CXI_IGNORE
**	and adi_res_type() adjustment.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date.
**	rdv		DBV buffer for output.
**
**  Returns:
**      last day in month
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for LAST_DAY(date)
*/
DB_STATUS
adu_lastday(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    AD_NEWDTNTRNL nd;
    DB_DATA_VALUE tdv1, trdv;

    if (dv1->db_datatype < 0)
    {
	if (ADI_ISNULL_MACRO(dv1))
	    goto return_NULL;
	tdv1 = *dv1;
	dv1 = &tdv1;
	dv1->db_datatype = -dv1->db_datatype;
	dv1->db_length--;
    }
    trdv = *rdv;
    if (trdv.db_datatype < 0)
    {
	trdv.db_datatype = -trdv.db_datatype;
	trdv.db_length--;
    }
    memset(&nd, 0, sizeof(nd));
    if (adu_6to_dtntrnl (adf_scb, dv1, &nd))
	goto return_NULL;
    
    if (status = ADU_NEWDT_TO_LCLTZ(adf_scb, &nd))
	return status;
    if (nd.dn_status2 & AD_DN2_NEED_NORM)
	adu_1normldate(&nd);

    /* If no date portion - return NULL */
    if ((nd.dn_status2 & (AD_DN2_DATE_DEFAULT|AD_DN2_NO_DATE)) != 0 ||
		(nd.dn_status & AD_DN_ABSOLUTE) == 0) 
	goto return_NULL;

    nd.dn_day = adu_2monthsize(nd.dn_month, nd.dn_year);

    if (E_DB_OK == adu_7from_dtntrnl(adf_scb, &trdv, &nd))
	return E_DB_OK;

return_NULL:
    if (rdv->db_datatype >= 0)
	return adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
    ADF_SETNULL_MACRO(rdv);
    return E_DB_OK;
}

/*
**  Name:  adu_mthsbtwn() - Return months_between.
**
**  Description:
**	Returns the approx number of months between the two dates.
**	In otherwords: date1-date2 (ish) as a float count of months
**	and treating difference of non-equal days as 1/31 of a month.
**	The last days of months are also considered equal thus in the same
**	year:
**	    Sep 30th - Aug 30th -> 1 month as days are the same
**	    Sep 30th - Aug 31th -> 1 month as both days at end of month
**	    Sep 30th - Aug 29th -> 1.032258064516129 months = 1 1/13 month
**
**	Based on Oracle implementation.
**
**  Parameters:
**	adf_scb		ADF context
**	dv1		DBV date1.
**	dv2		DBV date2.
**	rdv		DBV buffer for output. Must be FLOAT8
**
**  Returns:
**      number of months as a decimal(31,15)
**
**  History
**	26-Aug-2011 (kiria01) SIR 125693
**	    Added for MONTHS_BETWEEN(date, date)
*/
DB_STATUS
adu_mthsbtwn(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv1,		/* date1 */
	DB_DATA_VALUE	*dv2,		/* date2 */
	DB_DATA_VALUE	*rdv)
{
    DB_STATUS status;
    AD_NEWDTNTRNL nd1, nd2;
    i4 int_part;
    f8 fraction = 0.0;

    if (status = adu_6to_dtntrnl (adf_scb, dv1, &nd1))
	return status;
    if (status = adu_6to_dtntrnl (adf_scb, dv2, &nd2))
	return status;
    if (rdv->db_datatype != DB_FLT_TYPE || rdv->db_length != (i4)sizeof(f8))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    if ((nd1.dn_status2 ^ nd2.dn_status2) & AD_DN2_NO_DATE)
    {
	if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd1))
	    return status;
	if (status = ADU_NEWDT_PEND_DATE(adf_scb, &nd2))
	    return status;
    }
    if ((nd1.dn_status2 ^ nd2.dn_status2) & AD_DN2_IN_LCLTZ)
    {
	if (status = ADU_NEWDT_TO_GMT(adf_scb, &nd1))
	    return status;
	if (status = ADU_NEWDT_TO_GMT(adf_scb, &nd2))
	    return status;
    }
    int_part = (nd1.dn_year - nd2.dn_year) * 12 +
		nd1.dn_month - nd2.dn_month;

    if (nd1.dn_day != nd2.dn_day && (
	    adu_2monthsize(nd1.dn_month, nd1.dn_year) != nd1.dn_day ||
	    adu_2monthsize(nd2.dn_month, nd2.dn_year) != nd2.dn_day))
    {

	fraction = nd1.dn_day - nd2.dn_day;
	fraction *= AD_40DTE_SECPERDAY;
	fraction += nd1.dn_seconds - nd2.dn_seconds;
	fraction *= AD_50DTE_NSPERS;
	fraction += nd1.dn_nsecond - nd2.dn_nsecond;
	fraction /= 31 * AD_40DTE_SECPERDAY * AD_50DTE_NSPERS;
    }
    fraction += (f8)int_part;
    memcpy(rdv->db_data, &fraction, sizeof(f8));

    return E_DB_OK;
}

/*
**  Name:  adu_int_to_intds() - Convert int to interval d-to-s
**
**  Description:
**	Returns the interval_dtos equivalent of int days
**
**	Intended to support ADTE-INT -> ADTE
**	Intended to support ADTE+INT -> ADTE
**
**  Parameters:
**	adf_scb		ADF context
**	dv		DBV int.
**	rdv		DBV buffer for output. Must be INDS
**
**  Returns:
**      interval equivalent of int days
**
**  History
**	12-Oct-2011 (kiria01) b125835 
**	    Created to support INT to INTERVAL DTOS coercion
*/
DB_STATUS
adu_int_to_intds(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv,		/* int */
	DB_DATA_VALUE	*rdv)		/* inds */
{
    DB_STATUS status;
    AD_INTDS *inds = (AD_INTDS *)rdv->db_data;
    i4 days;

    if (dv->db_datatype != DB_INT_TYPE)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    if (rdv->db_datatype != DB_INDS_TYPE || rdv->db_length != ADF_INTDS_LEN)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    if (dv->db_length == (i4)sizeof(i4))
	inds->dn_days = *(i4*)dv->db_data;
    else if (dv->db_length == (i4)sizeof(i2))
	inds->dn_days = *(i2*)dv->db_data;
    else if (dv->db_length == (i4)sizeof(i1))
	inds->dn_days = *(i1*)dv->db_data;
    else if (dv->db_length == (i4)sizeof(i8))
	inds->dn_days = *(i8*)dv->db_data;
    else
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    inds->dn_seconds = 0;
    inds->dn_nseconds = 0;

    return E_DB_OK;
}

/*
**  Name:  adu_intds_to_int() - Convert interval d-to-s to int
**
**  Description:
**	Returns the int days equivalent of interval_dtos
**
**	Intended to support ADTE-ADTE -> INT
**
**  Parameters:
**	adf_scb		ADF context
**	dv		DBV int.
**	rdv		DBV buffer for output. Must be INDS
**
**  Returns:
**      interval equivalent of int days
**
**  History
**	12-Oct-2011 (kiria01) b125835 
**	    Created to support INTERVAL DTOS to INT coercion
*/
DB_STATUS
adu_intds_to_int(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv,		/* inds */
	DB_DATA_VALUE	*rdv)		/* int */
{
    DB_STATUS status;
    AD_INTDS *inds = (AD_INTDS *)dv->db_data;
    i4 days;

    if (dv->db_datatype != DB_INDS_TYPE || dv->db_length != ADF_INTDS_LEN)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    if (rdv->db_datatype != DB_INT_TYPE || rdv->db_length != (i4)sizeof(i4))
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));

    days = inds->dn_days + (inds->dn_seconds +
			inds->dn_nseconds/AD_49DTE_INSPERS) / AD_41DTE_ISECPERDAY;

    *(i4*)rdv->db_data = days;

    return E_DB_OK;
}
