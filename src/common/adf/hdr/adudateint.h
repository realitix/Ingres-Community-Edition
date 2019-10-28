/*
** Copyright (c) 2011 Actian Corporation
*/

/*
** Name: ADUDATEINT.H - Internal date definitions for ADU.
**
** Description:
**      This file defines functions, typedefs, constants, and variables
**      used internally by ADU particularly for processing dates.
**
** History:
**	10-Nov-2011 (kiria01) b125945
**	    Pulled out AD_NEWDTNTRNL related parts into internal
**	    header.
*/
#ifndef __ADUDATEINT_H_INC__
#define __ADUDATEINT_H_INC__
#include <tmtz.h>
#include <adudate.h>


/*}
** Name: AD_NEWDTNTRNL	- defines the common date/time representing
**	all date/time datatypes
**
** Description:
**	This structure describes the fields used to define a common date/time
**	structure used by ADF to manipulate all date/time values (including
**	the "old" Ingres DATE type) avoiding the need for datatype specific
**	handling in code.
**	Due to representing all the date/time structures defined in adudate.h,
**	the fields must be big enough to hold component values from all these
**	types.
**
**	In essence this is a common interchange form, so it has to support
**	all the interchange paths defined in the ANSI SQL standard augmented
**	with paths to integrate INGRESDATE, TIME WITH LOCAL TIME ZONE and
**	TIMESTAMP WITH LOCAL TIME ZONE. It is therefore vitally important that
**	the structure support a direct and efficent conversion from source
**	type to destination type.
**
**	To this end, the structure models all the fields necessary to transfer
**	common elements plus a set of flags that determine key state information
**	about the date. These extra flags are chosen to encapsulate the essence
**	of what make a date/time type different from the next.
**
**	The first state flag, AD_DN2_IN_LCLTZ models whether we have the date/time
**	represented in GMT or the local TZ. TSW, TSTMP, TMW, and TME retain their
**	data in GMT. INGRESDATE does too but ONLY if the flags AD_DN_TIMESPEC and
**	AD_DN_ABSOLUTE are set. IF and INGRESDATE has AD_DN_ABSOLUTE but not the
**	AD_DN_TIMESPEC, then this will be like ANSIDATE and will notionally be in
**	local TZ. We do not switch dates into local or GMT without good reason so
**	there are two macros: ADU_NEWDT_TO_GMT and ADU_NEWDT_TO_LCLTZ that will
**	do what is needed (usually nothing) to get into the appropriate TZ.
**	As has been hinted at, ANSIDATE is treated as storing in local TZ as are
**	TSWO and TMWO. Most date processing doesn't need a TZ change, so often,
**	the call to the macro will not invoke the retrieval of TZ offset whic is
**	just as well as the operation can be costly, especially with DST TZs.
**
**	The second state flag, AD_DN2_NO_DATE, identifies data that only had a
**	TIME portion and that might need a CURRENT_DATE adding.
**
**	The third state flag, AD_DN2_TZ_VALID, models whether the TZ field
**	is set or not. If not we will need to default it to the local timezone
**	offest. ADU_NEWDT_PEND_TZ will manage this but usually this will be
**	called internally by ADU_NEWDT_TO_GMT or ADU_NEWDT_TO_LCLTZ if the TZ
**	happens not to be known.
**
**	The remaining flags don't really model state as such, AD_DN2_NEED_NORM
**	is just a means of reducing normalisation calls to a minimum, and
**	AD_DN2_DATE_DEFAULT for tracking string date defaulting.
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL. Added definitions for int & float
**	    nanoseconds per second: AD_49DTE_INSPERS & AD_50DTE_NSPERS.
**	16-Mar-2008 (kiria01) b120004
**	    As part of cleanup timezone handling the internal date
**	    structure has had added the field dn_status2 to allow
**	    deferring tz adjustment.
**	10-Nov-2011 (kiria01) b125945
**	    Pulled out AD_NEWDTNTRNL related parts into internal
**	    header. Completed what was begun in b120004, the TZ
**	    adjustment can now be more easily deferred and is often
**	    avoided altogether.	    
*/

typedef	struct _AD_NEWDTNTRNL
{
    i2		dn_year;		/* year component (1-9999) */
    i2		dn_month;		/* month component (1-12) */
    i4		dn_day;			/* day component (1-31) */
    i4		dn_seconds;		/* seconds since midnight */
    i4		dn_nsecond;		/* nanoseconds since last second */
    i2		dn_tzoffset;		/* timezone offset in minutes needed to
					**     denormalise dn_seconds
					*/
    char	dn_status;		/* status byte from old date */
    char	dn_status2;		/* extended status */
#define AD_DN2_IN_LCLTZ		0x01	/* Date/Time is in LOCAL TZ and NOT
					** in GMT. This will be usual for
					** dates sourced from ADTE, TMWO,
					** TSWO and on some of the presentation
					** routines.
					*/
#define AD_DN2_NO_DATE		0x02	/* Src time has no date
					*/
#define AD_DN2_TZ_VALID		0x04	/* TZ field (.dn_tzoffset) is valid
					** otherwise a call to determine TZ is
					** outstanding to deal with this. The
					** routines adu_dtntrnl_toLCL and
					** adu_dtntrnl_toGMT will manage this.
					*/
#define	AD_DN2_NEED_NORM	0x08    /* Normalise pending */
#define AD_DN2_DATE_DEFAULT	0x10	/* No date was present in literal
					** str. Used to allow legacy flag
					** setting for DTE.
					*/
    DB_DT_ID	dn_dttype;		/* type code for source value */
}   AD_NEWDTNTRNL;

/*
** Timezone related access macros for AD_NEWDTNTRNL.
** These should be used in preference to directly accessing
** the .dn_tzoffset field.
*/
/* AD_TZ_OFFSETNEW extract the tz offset in seconds from AD_NEWDTNTRNL */
#define AD_TZ_OFFSETNEW(d) ((d)->dn_tzoffset*AD_10DTE_ISECPERMIN)

/* AD_TZ_SETNEW assign offset in secs to AD_NEWDTNTRNL */
#define AD_TZ_SETNEW(d, val) ((d)->dn_tzoffset=(val)/AD_10DTE_ISECPERMIN)

/* AD_TZ_SIGNNEW extract sign of offset from AD_NEWDTNTRNL */
#define AD_TZ_SIGNNEW(d) ((d)->dn_tzoffset<0?-1:1)

/* AD_TZ_HOURSNEW extract hours from AD_NEWDTNTRNL */
#define AD_TZ_HOURSNEW(d) (abs((d)->dn_tzoffset)/60)

/* AD_TZ_MINUTESNEW extract minutes from AD_NEWDTNTRNL */
#define AD_TZ_MINUTESNEW(d) ((d)->dn_tzoffset>=0\
			    ?(d)->dn_tzoffset-((d)->dn_tzoffset/60)*60\
			    :-(d)->dn_tzoffset-((d)->dn_tzoffset/-60)*60)

/*
** AD_NEWDTNTRNL access rules:
**	Most of the basic work is done with adu_6to_dtntrnl and
**	adu_7from_dtntrnl but depending on the callers needs, certain
**	actions are deferred until needed.
**
**	Most routines that need to process a date/time DBV will usually
**	just need to call adu_6to_dtntrnl() and the date elements will
**	then be arranged in a common form. Usually, the caller will
**	need to call ADU_NEWDT_PEND_DATE and ADU_NEWDT_TO_LCLTZ to setup
**	the common fields to reflect a date/time in the presentation
**	timezone but note that the use of the macros encapsulates the
**	flag tests that will usually mean that the actual routine call
**	is avoided. See adu_14strtodate or adu_datefmt for examples of
**	usual needs and by way of contrast: adu_dgmt.
**
**	Deviations from this pattern are sometimes needed, particularly
**	where a routine needs to process two dates like the binary operators.
**	In thess situations, it is usual to get the dates into a common
**	timezone for processing. See ad0_5sub_date and adu_4date_cmp for
**	examples.
*/
FUNC_EXTERN DB_STATUS adu_6to_dtntrnl(
	ADF_CB *adf_scb,
	DB_DATA_VALUE *indv,
	AD_NEWDTNTRNL *outval);
FUNC_EXTERN DB_STATUS adu_7from_dtntrnl(
	ADF_CB *adf_scb,
	DB_DATA_VALUE *outdv,
	AD_NEWDTNTRNL *inval);
/* Apply pending DATE retrieval */
#define ADU_NEWDT_PEND_DATE(_scb,_d) (((_d)->dn_status2 & AD_DN2_NO_DATE)\
		? adu_dtntrnl_pend_date(_scb,_d) : E_DB_OK)

/* Set time to LCLTZ from GMT */
#define ADU_NEWDT_TO_LCLTZ(_scb,_d) ((~(_d)->dn_status2 & AD_DN2_IN_LCLTZ)\
		? adu_dtntrnl_toLCL(_scb,_d) : E_DB_OK)

/* Set time to GMT from LCLTZ */
#define ADU_NEWDT_TO_GMT(_scb,_d) (((_d)->dn_status2 & AD_DN2_IN_LCLTZ)\
		? adu_dtntrnl_toGMT(_scb,_d) : E_DB_OK)

/* Set TZ field - should not need to be called directly as ADU_NEWDT_TO_LCLTZ
** or ADU_NEWDT_TO_GMT will have done this or adu_7from_dtntrnl will get to. */
#define ADU_NEWDT_PEND_TZ(_scb,_d,_bias) ((~(_d)->dn_status2 & AD_DN2_TZ_VALID)\
		? adu_dtntrnl_pend_tz(_scb,_d,_bias) : E_DB_OK)

/* These 4 routines should ONLY be called via the above macros */
FUNC_EXTERN DB_STATUS adu_dtntrnl_pend_date(ADF_CB*, AD_NEWDTNTRNL *);
FUNC_EXTERN DB_STATUS adu_dtntrnl_toGMT(ADF_CB*, AD_NEWDTNTRNL*);
FUNC_EXTERN DB_STATUS adu_dtntrnl_toLCL(ADF_CB*, AD_NEWDTNTRNL*);
FUNC_EXTERN DB_STATUS adu_dtntrnl_pend_tz(ADF_CB*, AD_NEWDTNTRNL*, TM_TIMETYPE);

/* Normalise absolute date */
FUNC_EXTERN VOID adu_1normldate(AD_NEWDTNTRNL*);

/* Normalise interval and return length in seconds */
FUNC_EXTERN i4 adu_2normldint(AD_NEWDTNTRNL*);

/* Compute absolute number of days since epoch. */
FUNC_EXTERN i4 adu_3day_of_date(AD_NEWDTNTRNL  *d);

/* Compute the number of seconds since epoch */
FUNC_EXTERN i4 adu_5time_of_date(AD_NEWDTNTRNL  *d);

#endif /* __ADUDATEINT_H_INC__ */
