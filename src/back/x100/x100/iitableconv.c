#include <compat.h>

/* The whole thing is IVW only */
#if defined(conf_IVW)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <iicommon.h>
#include <cs.h>
#include <cm.h>
#include <dbdbms.h>
#include <gca.h>
#include <dmf.h>
#include <adf.h>
#include <qefmain.h>
#include <adudate.h>
#include <mh.h>
#include <stcl.h>
/* FIXME - in absence of inclusion of utils.h - kiria01 */
#ifndef __r
#define __r
#endif
#include <x100/vwlog.h>
#include <x100/int128export.h>

#include <x100.h>
#include <x100client.h>

#include <x100/tableconv.h>
#include <iitableconv.h>

#define VWLOG_FACILITY "TABLECONV"

#if defined(NT_GENERIC)
#define snprintf  sprintf_s
#endif

static int get_varchar_length(void *s)
{
    u_i2 result;
    struct { u_i2 len; } *src = s;
    I2ASSIGN_MACRO(src->len, result);
    return result;
}

static void set_varchar_length(void *d, int l)
{
    u_i2 length = l;
    struct { u_i2 len; } *dst = d;
    I2ASSIGN_MACRO(length, dst->len);
}

struct adf_user {
    ADF_CB *adfcb;
    DB_DATA_VALUE utf8_dt;
    DB_DATA_VALUE ingres_dt;
};

static int adf_convert_from(void *user, char *src, char *dst) {
    struct adf_user *u = (struct adf_user *) user;
    char *workbuf = (char*)(u + 1);
    int dst_len;
    DB_STATUS cvinto_status;
    u->ingres_dt.db_data = src;
    u->utf8_dt.db_data = workbuf;
    cvinto_status = adc_cvinto(u->adfcb, &u->ingres_dt, &u->utf8_dt);
    dst_len = get_varchar_length(workbuf);
    log_debug("convert from type %d len %d", (int)u->ingres_dt.db_datatype, dst_len);
    if (DB_FAILURE_MACRO(cvinto_status)) {
	log_error("adf conversion failed %x", (int)cvinto_status);
	return 0;
    }
    memcpy(dst, workbuf+2, dst_len);
    dst[dst_len] = 0;
    return 1;
}
static int adf_convert_into(void *user, char *src, char *dst) {
    struct adf_user *u = (struct adf_user *) user;
    char *workbuf = (char*)(u + 1);
    int src_len = strlen(src);
    DB_STATUS cvinto_status;
    set_varchar_length(workbuf, src_len);
    if (src_len + 2 > u->utf8_dt.db_length) return 0;
    memcpy(workbuf + 2, src, src_len);
    u->utf8_dt.db_data = workbuf;
    u->ingres_dt.db_data = dst;
    cvinto_status = adc_cvinto(u->adfcb, &u->utf8_dt, &u->ingres_dt);
    log_debug("convert into type %d len %d", (int)u->ingres_dt.db_datatype, src_len);
    if (DB_FAILURE_MACRO(cvinto_status)) return 0;
    return 1;
}

static int adf_convert_from16(void *user, char *src, char *dst) {
    struct adf_user *u = (struct adf_user *) user;
    char *workbuf = (char*)(u + 1);
    int dst_len;
    DB_STATUS cvinto_status;
    u->ingres_dt.db_data = src;
    u->utf8_dt.db_data = workbuf;
    if (u->ingres_dt.db_datatype == DB_NVCHR_TYPE) {
	int src_len = get_varchar_length(src);
	/* Limit conversion types to minumum possible for value specified
	   to make cnversion faster. */
	u->ingres_dt.db_length = src_len ? src_len * 2 + 2 : 4;
	u->utf8_dt.db_length = src_len ? src_len * 4 + 2 : 5;
    }
    cvinto_status = adu_nvchr_toutf8(u->adfcb, &u->ingres_dt, &u->utf8_dt);
    dst_len = get_varchar_length(workbuf);
    if (DB_FAILURE_MACRO(cvinto_status)) {
	log_error("adf conversion failed %x", (int)cvinto_status);
	return 0;
    }
    if (u->ingres_dt.db_datatype == DB_NCHR_TYPE) {
	while (dst_len && workbuf[dst_len+1] == ' ') dst_len--;
    }
    memcpy(dst, workbuf+2, dst_len);
    dst[dst_len] = 0;
    return 1;
}
static int adf_convert_into16(void *user, char *src, char *dst) {
    struct adf_user *u = (struct adf_user *) user;
    char *workbuf = (char*)(u + 1);
    int src_len = strlen(src);
    DB_STATUS cvinto_status;
    set_varchar_length(workbuf, src_len);
    if (src_len + 2 > u->utf8_dt.db_length) return 0;
    memcpy(workbuf + 2, src, src_len);
    u->utf8_dt.db_data = workbuf;
    u->ingres_dt.db_data = dst;
    cvinto_status = adu_nvchr_fromutf8(u->adfcb, &u->utf8_dt, &u->ingres_dt);
    log_debug("convert into type %d(%d) len %d", (int)u->ingres_dt.db_datatype, (int)u->ingres_dt.db_length, src_len);
    if (DB_FAILURE_MACRO(cvinto_status)) return 0;
    return 1;
}

static struct adf_user *make_adf_user(ADF_CB *adf, int datatype, int length) {
    struct adf_user *u;
    int worklen = length * 6 + 2;
    if (!adf) {
	log_error("no adf structure needed for utf8 conversion");
	return 0;
    }
    u = malloc(sizeof(struct adf_user) + worklen);
    if (!u) return 0;
    u->adfcb = adf;
    u->ingres_dt.db_datatype = datatype;
    u->ingres_dt.db_prec = 0;
    u->ingres_dt.db_length = length;
    u->utf8_dt.db_datatype = DB_UTF8_TYPE;
    u->utf8_dt.db_prec = 0;
    u->utf8_dt.db_length = worklen;
    return u;
}

static enum {
    UTF8_NON_COMAPATIBLE,
    UTF8_COMPATIBLE,
    UTF8_7BIT_COMPATIBLE
} utf8_compatibility_level(void) {
    char chname[CM_MAXATTRNAME+1];
    CMget_charset_name(&chname[0]);
    log_debug("installation charset is %s", chname);
    if (!STcasecmp(chname, "UTF8"))
	return UTF8_COMPATIBLE;
    else if (!STncasecmp(chname, "ISO8859", 7))
	return UTF8_7BIT_COMPATIBLE;
    else if (!STncasecmp(chname, "IS8859", 6))
	return UTF8_7BIT_COMPATIBLE;
    else if (!STncasecmp(chname, "WIN1252", 7))
	return UTF8_7BIT_COMPATIBLE;
    return UTF8_NON_COMAPATIBLE;
}

#define X100_LEAPYEARS(y) ((y)/4 + (y)/400 - (y)/100 + 1)
#define X100_LEAPYEAR(y) (((y)%4==0) && ((y)%100!=0 || (y)%400==0))
#define YEARDAYS(y) (X100_LEAPYEAR(y)?366:365)

i1 daymap_nonleap[365] = {
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
}, daymap_leap[366] = {
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
}, monthmap_nonleap[365] = {
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 
10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 
11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
}, monthmap_leap[366] = {
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 
10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 
11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
};

static int monthdays_nonleap[] = {
0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
static int monthdays_leap[] = {
0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
};

static int yearmonth_to_days(int month, int year)
{
	int v = year * 365 + X100_LEAPYEARS(year-1);
	v += (X100_LEAPYEAR(year) ? monthdays_leap : monthdays_nonleap)[month - 1];
	return v;
}
static void to_year_month_day(int v, i2* dn_year, i1 *dn_month, i1* dn_day)
{
	int year = v/365;
	int day = (v-year*365) - X100_LEAPYEARS(year-1);
	if (v < 0) {
		*dn_day = 0;
		*dn_month = 0;
		*dn_year = 0;
		return;
	}
	while(day<0) {
		year--;
		day += YEARDAYS(year);
	}
	*dn_year = year;
	*dn_month = (X100_LEAPYEAR(year) ? monthmap_leap : monthmap_nonleap)[day];
	*dn_day = (X100_LEAPYEAR(year) ? daymap_leap : daymap_nonleap)[day];
}

static int from_year_month_day(int dn_year, int dn_month, int dn_day)
{
	return yearmonth_to_days(dn_month, dn_year) + dn_day - 1;	
}


static void fill0_char(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*dst = 0;
}
static void check0_char(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(*src) attr->conv_error = X100_ERR_NULL_TO_NONNULL;
}
static void to_null(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*dst = !!*src;
}

TABLE_CONV_DEF_FUNCS(iinull, fill0_char, check0_char)
TABLE_CONV_DEF_FUNCS(x100null, check0_char, fill0_char)
TABLE_CONV_DEF_FUNCS(null, to_null, to_null)

static struct table_conv_func *nullconv[4] = {
0, &convert_iinull, &convert_x100null, &convert_null
};

static void to_money(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	double *dsrc = (double*)src;
	double *ddst = (double*)dst;
	if (*dsrc >= 100000000000000.) attr->conv_error = X100_ERR_MNYOVF_ERROR;
	else if (*dsrc <= -100000000000000.) attr->conv_error = X100_ERR_MNYUDF_ERROR;
	*ddst = *dsrc;
}

static void from_money(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	double *dsrc = (double*)src;
	double *ddst = (double*)dst;
	*ddst = *dsrc;
}
TABLE_CONV_DEF_FUNCS(money, to_money, from_money)

static void to_cha(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int dstlen = attr->attr_width;
	int len = strlen( *(char **)src );
	if(len>dstlen) {
		len = dstlen;
	}
	memcpy(dst,  *(char **)src, len);
	memset(dst+len, ' ', dstlen - len);
}

static void to_vch(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int dstlen = attr->attr_width;
	int len = strlen( *(char **)src );
	int maxlen = dstlen - 2;
	if(len>maxlen) {
		len = maxlen;
	}
	memcpy(dst+2,  *(char **)src, len);
	memset(dst+2+len, '\0', maxlen - len);
	set_varchar_length(dst, len);
}

#define ishex(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F'))
#define fromhex(c) (((c) >= 'A' && (c) <= 'F')? (c) - 'A' + 10 : (c) - '0')
static void catstr_to_cha(unsigned char *_src, unsigned char *dst, struct table_conv_attr*attr)
{
	int dstlen = attr->attr_width, i, j;
	unsigned char *psrc = *(unsigned char **)_src;
	int len = strlen(psrc);
	for (j = 0, i = psrc[0] == '_'; i < len && j < dstlen ; i++) {
		if (psrc[i] == '_' && i < len - 2 && ishex(psrc[i+1]) && ishex(psrc[i+2])) {
			dst[j++] = fromhex(psrc[i+1]) * 16 + fromhex(psrc[i+2]);
			i += 2;
		} else {
			dst[j++] = psrc[i];
		}
	}
	while (j < dstlen) dst[j++] = ' ';
}

static void from_cha(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int len = attr->attr_width;
	unsigned char *pdst = *(unsigned char**)dst;
	while (len && src[len - 1] == ' ') len--;
	memcpy(pdst, src, len);
	pdst[len] = 0;
}

static void from_vch(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int len = get_varchar_length(src);
	unsigned char *pdst = *(unsigned char**)dst;
	if (len > attr->attr_width) len = attr->attr_width;
	memcpy(pdst, src + 2, len);
	pdst[len] = 0;
}

static void from_adf_char(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(!adf_convert_from((struct adf_user *)attr->user,src,*(char **)dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void to_adf_char(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(!adf_convert_into((struct adf_user *)attr->user,*(char **)src,dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void from_adf_char16(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(!adf_convert_from16((struct adf_user *)attr->user,src,*(char **)dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void to_adf_char16(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(!adf_convert_into16((struct adf_user *)attr->user,*(char **)src,dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void from_nchar1(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*(int *)dst = *(short*)src;
}
static void to_nchar1(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int i;
	*(short *)dst = *(int*)src;
	for (i=1; i<attr->attr_width; i++) ((short*)dst)[i] = ' ';
}

static int is_7bit(char *s) {
	for (;*s;s++)
		if (*s & 0x80) return 0;
	return 1;
}
static int is_7bit_len(char *s, int len) {
	for (;len > 0;len--,s++)
		if (*s & 0x80) return 0;
	return 1;
}

static void from_cha_7bit(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(is_7bit_len(src,attr->attr_width)) {
		from_cha(src,dst,attr);
		return;
	}
	if(!adf_convert_from((struct adf_user *)attr->user,src,*(char **)dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void to_cha_7bit(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(is_7bit(*(char **)src)) {
		to_cha(src,dst,attr);
		return;
	}
	if(!adf_convert_into((struct adf_user *)attr->user,*(char **)src,dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void from_vch_7bit(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int len = get_varchar_length(src);
	if(is_7bit_len(src+2,len)) {
		from_vch(src,dst,attr);
		return;
	}
	if(!adf_convert_from((struct adf_user *)attr->user,src,*(char **)dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}
static void to_vch_7bit(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	if(is_7bit(*(char **)src)) {
		to_vch(src,dst,attr);
		return;
	}
	if(!adf_convert_into((struct adf_user *)attr->user,*(char **)src,dst))
		attr->conv_error = X100_ERR_UNICODE_CONVERSION;
}


#define INT128CONV(n1,t1) \
static void n1 ## _to_s128(unsigned char *src, \
		unsigned char *dst, \
		struct table_conv_attr*attr) \
{ \
	s128_fromint_ptr(dst, *(t1*)src); \
} \
static void s128_to_ ## n1(unsigned char *src, \
		unsigned char *dst, \
		struct table_conv_attr*attr) \
{ \
	*(t1*)dst = s128_toint_ptr(src); \
} \
TABLE_CONV_DEF_FUNCS(s128_ ## n1, s128_to_ ## n1, n1 ## _to_s128)

INT128CONV(schr,char)
INT128CONV(ssht,short)
INT128CONV(sint,int)
INT128CONV(slng,long long)


#define INTCONV(n1,n2,t1,t2) \
static void n1 ## _to_ ## n2(unsigned char *src, \
		unsigned char *dst, \
		struct table_conv_attr*attr) \
{ \
	*(t2*)dst = *(t1*)src; \
	if (sizeof(t2) < sizeof(t1) && *(t2*)dst != *(t1*)src) attr->conv_error = X100_ERR_INTOVERFLOW; \
} \
static void n2 ## _to_ ## n1(unsigned char *src, \
		unsigned char *dst, \
		struct table_conv_attr*attr) \
{ \
	*(t1*)dst = *(t2*)src; \
	if (sizeof(t1) < sizeof(t2) && *(t2*)dst != *(t1*)src) attr->conv_error = X100_ERR_INTOVERFLOW; \
} \
TABLE_CONV_DEF_FUNCS(n1 ## _ ## n2, n1 ## _to_ ## n2, n2 ## _to_ ## n1) \
TABLE_CONV_DEF_FUNCS(n2 ## _ ## n1, n2 ## _to_ ## n1, n1 ## _to_ ## n2)

#define FLTCONV(n1,n2,t1,t2) \
static void n1 ## _to_ ## n2(unsigned char *src, \
		unsigned char *dst, \
		struct table_conv_attr*attr) \
{ \
	*(t2*)dst = *(t1*)src; \
} \
static void n2 ## _to_ ## n1(unsigned char *src, \
		unsigned char *dst, \
		struct table_conv_attr*attr) \
{ \
	*(t1*)dst = *(t2*)src; \
} \
TABLE_CONV_DEF_FUNCS(n1 ## _ ## n2, n1 ## _to_ ## n2, n2 ## _to_ ## n1) \
TABLE_CONV_DEF_FUNCS(n2 ## _ ## n1, n2 ## _to_ ## n1, n1 ## _to_ ## n2)

static void dummy_read(unsigned char *src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	(void)dst;
	(void)src;
	(void)attr;
}

INTCONV(slng,sint,long long,int)
INTCONV(slng,ssht,long long,short)
INTCONV(slng,schr,long long,char)
INTCONV(sint,ssht,int,short)
INTCONV(sint,schr,int,char)
INTCONV(ssht,schr,short,char)

FLTCONV(flt,dbl,float,double)

static struct table_conv_func *intconv[] = {
                 0, &convert_schr_ssht, &convert_schr_sint, &convert_schr_slng,
&convert_ssht_schr,                  0, &convert_ssht_sint, &convert_ssht_slng,
&convert_sint_schr, &convert_sint_ssht,                  0, &convert_sint_slng,
&convert_slng_schr, &convert_slng_ssht, &convert_slng_sint,                  0,
&convert_s128_schr, &convert_s128_ssht, &convert_s128_sint, &convert_s128_slng,
};

static struct table_conv_func *fltconv[] = {
               0, &convert_flt_dbl,
&convert_dbl_flt,                0,
};

static void f_to_dec(double val, unsigned char *dst, struct table_conv_attr*attr)
{
	int digits = attr->attr_prec >> 8, valdigits;
	int i, pos, unusednibble, len = (digits + 1) / 2;
	int scale = (char)attr->attr_prec;
	char tmpbuf[128];
	for (i = 0; i < len; i++) dst[i] = 0;
	unusednibble = !(digits & 1);
	if (val == -0.0) val = 0.0;
	if (val < 0) { 
		dst[digits/2] = MH_PK_MINUS;
		val = -val;
	} else {
		dst[digits/2] = MH_PK_PLUS;
	}
	val *= pow(10,scale);
        val = MHtrunc(val, 0);
	valdigits = snprintf(tmpbuf, sizeof(tmpbuf)-1, "%.0f", val);
	if (valdigits < 0 || valdigits > sizeof(tmpbuf)-1 || valdigits > digits || tmpbuf[0] < '0' || tmpbuf[0] > '9') {		
		attr->conv_error = X100_ERR_DECOVF_ERROR;
		return;
	}
	pos = digits - valdigits + unusednibble; //nibble address of MSD
	for (i = 0; i < valdigits; i++) {
		dst[pos/2] |= (tmpbuf[i] - '0') << (pos & 1 ? 0 : 4);
		pos++;
	}
}

static void to_dec(long long val, int scalediff, unsigned char *dst, struct table_conv_attr*attr)
{
	int digits = attr->attr_prec >> 8;
	int i, pos, unusednibble, len = (digits + 1) / 2;
	for (i = 0; i < len; i++) dst[i] = 0;
	while (scalediff < 0) val/=10, scalediff++;
	unusednibble = !(digits & 1);
	if (val < 0) { 
		dst[digits/2] = MH_PK_MINUS;
		val = -val;
	} else {
		dst[digits/2] = MH_PK_PLUS;
	}
	pos = digits + unusednibble - 1 - scalediff; //nibble address of LSD
	while (val) {
		if (pos >= unusednibble) {
			dst[pos/2] |= (val % 10) << (pos & 1 ? 0 : 4);
		} else {
			attr->conv_error = X100_ERR_DECOVF_ERROR;
		}
		pos--;
		val /= 10;
	}
}

static long long from_dec(unsigned char *src, struct table_conv_attr*attr)
{
	int scalediff = (char)attr->attr_prec;
	int digits = attr->attr_prec >> 8;
	int i, pos, unusednibble;
	long long val = 0;
	unusednibble = !(digits & 1);
	pos = digits + unusednibble - (scalediff > 0 ? scalediff : 0);
	for (i = unusednibble; i < pos; i++) {
		val = val * 10 + (0xf & (src[i/2] >> (i & 1 ? 0 : 4)));
	}
	if ((src[digits/2] & 0xf) == MH_PK_MINUS || (src[digits/2] & 0xf) == MH_PK_AMINUS) {
		val = -val;
	}
	while (scalediff < 0) val*=10, scalediff++;
	return val;
}

static void schr_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int scalediff = (char)attr->attr_prec;
	to_dec(*(char*)src,scalediff,dst,attr);
}
static void ssht_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int scalediff = (char)attr->attr_prec;
	to_dec(*(short*)src,scalediff,dst,attr);
}
static void sint_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int scalediff = (char)attr->attr_prec;
	to_dec(*(int*)src,scalediff,dst,attr);
}
static void slng_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int scalediff = (char)attr->attr_prec;
	to_dec(*(long long*)src,scalediff,dst,attr);
}
static void flt_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	f_to_dec((double)(*(float*)src),dst,attr);
}
static void dbl_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	f_to_dec((*(double*)src),dst,attr);
}

static void schr_from_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*(char*)dst = from_dec(src,attr);
}
static void ssht_from_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*(short*)dst = from_dec(src,attr);
}
static void sint_from_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*(int*)dst = from_dec(src,attr);
}
static void slng_from_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	*(long long*)dst = from_dec(src,attr);
}
static void s128_from_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	char tmp[42], *t = tmp;
	int scalediff = (char)attr->attr_prec;
	int digits = attr->attr_prec >> 8;
	int i, pos, unusednibble;
	long long val = 0;
	if ((src[digits/2] & 0xf) == MH_PK_MINUS || (src[digits/2] & 0xf) == MH_PK_AMINUS) {
		*t++ = '-';
	}
	unusednibble = !(digits & 1);
	pos = digits + unusednibble - (scalediff > 0 ? scalediff : 0);
	for (i = unusednibble; i < pos; i++) {
		*t++ = '0' + (0xf & (src[i/2] >> (i & 1 ? 0 : 4)));
	}
	while (scalediff < 0) *t++ = '0', scalediff++;
	*t = 0;
	if (s128_from_string_ptr(tmp, dst)) {
		attr->conv_error = X100_ERR_DECOVF_ERROR;
	}
}

static void s128_to_dec(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	char tmp[42], *t = tmp;
	int scalediff = (char)attr->attr_prec;
	int digits = attr->attr_prec >> 8;
	int vallen;
	int i, pos, unusednibble, len = (digits + 1) / 2;
	s128_to_string_ptr(tmp, sizeof(tmp), src);
	for (i = 0; i < len; i++) dst[i] = 0;
	unusednibble = !(digits & 1);
	if (*t == '-') { 
		dst[digits/2] = MH_PK_MINUS;
		t++;
	} else {
		dst[digits/2] = MH_PK_PLUS;
	}
	vallen = strlen(t);
	if (scalediff < 0) vallen += scalediff, scalediff = 0;
	pos = digits + unusednibble - 1 - scalediff; //nibble address of LSD
	while (vallen > 0) {
		if (pos >= unusednibble) {
			dst[pos/2] |= (t[vallen-1]-'0') << (pos & 1 ? 0 : 4);
		} else {
			attr->conv_error = X100_ERR_DECOVF_ERROR;
		}
		pos--;
		vallen--;
	}
}

TABLE_CONV_DEF_FUNCS(schr_dec, schr_to_dec, schr_from_dec)
TABLE_CONV_DEF_FUNCS(ssht_dec, ssht_to_dec, ssht_from_dec)
TABLE_CONV_DEF_FUNCS(sint_dec, sint_to_dec, sint_from_dec)
TABLE_CONV_DEF_FUNCS(slng_dec, slng_to_dec, slng_from_dec)
TABLE_CONV_DEF_FUNCS(s128_dec, s128_to_dec, s128_from_dec)
TABLE_CONV_DEF_FUNCS(flt_dec, flt_to_dec, dummy_read)
TABLE_CONV_DEF_FUNCS(dbl_dec, dbl_to_dec, dummy_read)
static struct table_conv_func *decconv[] = {
&convert_schr_dec, &convert_ssht_dec, &convert_sint_dec, &convert_slng_dec, &convert_s128_dec
};

static int pow_of_10[] = {
1,
10,
100,
1000,
10000,
100000,
1000000,
10000000,
100000000,
1000000000,
};

static void to_time0(int src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	AD_TIME *time = (AD_TIME*)dst;
	time->dn_seconds = src;
	time->dn_nsecond = 0;
	time->dn_tzhour = time->dn_tzminute = 0;
}

static void to_time(long long src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	int div = pow_of_10[attr->attr_prec];
	int mul = pow_of_10[9 - attr->attr_prec];
	AD_TIME *time = (AD_TIME*)dst;
	long long val = src;
	time->dn_seconds = val / div;
	time->dn_nsecond = (val % div) * mul;
	time->dn_tzhour = time->dn_tzminute = 0;
}

static int from_time0(unsigned char *src,
		struct table_conv_attr*attr)
{
	 return ((AD_TIME*)src)->dn_seconds;
}


static long long from_time(unsigned char *src,
		struct table_conv_attr*attr)
{
	long long div = pow_of_10[9 - attr->attr_prec];
	long long mul = pow_of_10[attr->attr_prec];
	AD_TIME *time = (AD_TIME*)src;
	long long val;
	val = time->dn_seconds * mul;
	val += time->dn_nsecond / div;
	return val;
}

static void to_timestamp(long long src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	int div = pow_of_10[attr->attr_prec];
	int mul = pow_of_10[9 - attr->attr_prec];
	AD_TIMESTAMP *ts = (AD_TIMESTAMP*)dst;
	long long val =src, time;
	time = val % (86400ll * div);
	val = val / (86400ll * div);
	to_year_month_day(val, &ts->dn_year, &ts->dn_month, &ts->dn_day);
	ts->dn_seconds = time / div;
	ts->dn_nsecond = (time % div) * mul;
	ts->dn_tzhour = ts->dn_tzminute = 0;
}

static long long from_timestamp(unsigned char *src,
		struct table_conv_attr*attr)
{
	int div = pow_of_10[9 - attr->attr_prec];
	int mul = pow_of_10[attr->attr_prec];
	AD_TIMESTAMP *ts = (AD_TIMESTAMP*)src;
	long long val;
	val = from_year_month_day(ts->dn_year, ts->dn_month, ts->dn_day) * 86400ll;
	val += ts->dn_seconds;
	val *= mul;
	val += ts->dn_nsecond / div;
	return val;
}

static void to_intervalds(long long src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	int div = pow_of_10[attr->attr_prec];
	int mul = pow_of_10[9 - attr->attr_prec];
	AD_INTDS *intds = (AD_INTDS*)dst;
	long long val = src, time;
	time = val % (86400ll * div);
	val = val / (86400ll * div);
	intds->dn_days = val;
	intds->dn_seconds = time / div;
	intds->dn_nseconds = (time % div) * mul;
}

static long long from_intervalds(unsigned char *src,
		struct table_conv_attr*attr)
{
	int div = pow_of_10[9 - attr->attr_prec];
	int mul = pow_of_10[attr->attr_prec];
	AD_INTDS *intds = (AD_INTDS*)src;
	long long val;
	val = intds->dn_days * 86400ll + intds->dn_seconds;
	val = val * mul + intds->dn_nseconds / div;
	return val;
}

static void to_intervalym(int src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	AD_INTYM *intym = (AD_INTYM*)dst;
	int val = src;
	intym->dn_years = val / 12;
	intym->dn_months = val % 12;
}

static int from_intervalym(unsigned char *src,
		struct table_conv_attr*attr)
{
	AD_INTYM *intym = (AD_INTYM*)src;
	return intym->dn_years * 12 + intym->dn_months;
}

static void to_adte(int src,
		unsigned char *dst,
		struct table_conv_attr*attr)
{
	AD_ADATE *date = (AD_ADATE*)dst;
	to_year_month_day(src, &date->dn_year, &date->dn_month, &date->dn_day);
}

static int from_adte(unsigned char *src, struct table_conv_attr*attr)
{
	AD_ADATE *date = (AD_ADATE*)src;
	int v;
	v = yearmonth_to_days(date->dn_month, date->dn_year);
	v += date->dn_day - 1;
	return v;
}

static void from_null_str(unsigned char *dst, struct table_conv_attr*attr)
{
	unsigned char *pdst = *(unsigned char**)dst;
	pdst[0] = 0;
}

#define TABLE_CONF_DEF_DATE_FUNCS_1(n, tn, t) \
static void tn ## _from_ ## n (unsigned char *src, unsigned char *dst, struct table_conv_attr*attr) { \
	*(t*)dst = from_ ## n(src, attr); \
} \
static void tn ## _to_ ## n (unsigned char *src, unsigned char *dst, struct table_conv_attr*attr) { \
	to_ ## n(*(t*)src, dst, attr); \
} \
TABLE_CONV_DEF_FUNCS(tn ## _ ## n, tn ## _to_ ## n, tn ## _from_ ## n)

#define TABLE_CONF_DEF_DATE_FUNCS(n) \
TABLE_CONF_DEF_DATE_FUNCS_1(n, schr, char) \
TABLE_CONF_DEF_DATE_FUNCS_1(n, ssht, short) \
TABLE_CONF_DEF_DATE_FUNCS_1(n, sint, int) \
TABLE_CONF_DEF_DATE_FUNCS_1(n, slng, long long) \
static void s128_from_ ## n (unsigned char *src, unsigned char *dst, struct table_conv_attr*attr) { \
	s128_fromint_ptr(dst, from_ ## n(src, attr)); \
} \
static void s128_to_ ## n (unsigned char *src, unsigned char *dst, struct table_conv_attr*attr) { \
	to_ ## n(s128_toint_ptr(src), dst, attr); \
} \
TABLE_CONV_DEF_FUNCS(s128_ ## n, s128_to_ ## n, s128_from_ ## n) \
static struct table_conv_func *convtab_ ## n [] = { \
&convert_schr_ ## n, &convert_ssht_ ## n, &convert_sint_ ## n, &convert_slng_ ## n, &convert_s128_ ## n \
};

TABLE_CONF_DEF_DATE_FUNCS(intervalym)
TABLE_CONF_DEF_DATE_FUNCS(intervalds)
TABLE_CONF_DEF_DATE_FUNCS(time)
TABLE_CONF_DEF_DATE_FUNCS(time0)
TABLE_CONF_DEF_DATE_FUNCS(timestamp)
TABLE_CONF_DEF_DATE_FUNCS(adte)

TABLE_CONV_DEF_FUNCS_WITH_NULL(catstr_cha, catstr_to_cha, from_cha, from_null_str)
TABLE_CONV_DEF_FUNCS_WITH_NULL(adf, to_adf_char, from_adf_char, from_null_str)
TABLE_CONV_DEF_FUNCS_WITH_NULL(adf16, to_adf_char16, from_adf_char16, from_null_str)
TABLE_CONV_DEF_FUNCS(nchar1, to_nchar1, from_nchar1)
TABLE_CONV_DEF_FUNCS_WITH_NULL(cha_7bit, to_cha_7bit, from_cha_7bit, from_null_str)
TABLE_CONV_DEF_FUNCS_WITH_NULL(vch_7bit, to_vch_7bit, from_vch_7bit, from_null_str)
TABLE_CONV_DEF_FUNCS_WITH_NULL(cha, to_cha, from_cha, from_null_str)
TABLE_CONV_DEF_FUNCS_WITH_NULL(vch, to_vch, from_vch, from_null_str)

static void from_null_literal(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	attr->conv_error = X100_ERR_NO_TYPE_CONVERSION;
}
static void to_null_literal(unsigned char *src, unsigned char *dst, struct table_conv_attr*attr)
{
	int dstlen = attr->attr_width;
	memset(dst, '\0', dstlen);
	if (*src) attr->conv_error = X100_ERR_NO_TYPE_CONVERSION;
}

TABLE_CONV_DEF_FUNCS(null_literal, to_null_literal, from_null_literal);

static int log2tab[17] = { -1, 0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4 };

int table_conv_add_typed_column(struct table_converter *tc,
		GCA_DBMS_VALUE *type1,
		char *type2,
		int type2width, int type2float, ADF_CB *adf)
{
	int type = type1->db_datatype;
	int width = type1->db_length - (type1->db_datatype < 0);
	int prec = 0;
	void *user = NULL;
	struct table_conv_func *func = &convert_clear;
	switch (type < 0 ? -type : type) {
		case DB_LTXT_TYPE:
			/* special case for typeless null literal sent as null(uchr('0')) and expected in long text(1) type */
			if (!strcmp(type2, "uchr") || !strcmp(type2, "schr")) {
				func = &convert_null_literal;
			}
			break;
		case DB_TABKEY_TYPE:
			func = 0;
			break;
		case DB_MNY_TYPE:
			if (!strcmp(type2, "money")) {
				func = &convert_money;
			}
			break;
		case DB_FLT_TYPE:
			if (!strcmp(type2, "flt") || !strcmp(type2, "dbl")) {
				func = fltconv[(type2width/4-1) * 2 + (width/4-1)];
			}
			break;
		case DB_INT_TYPE: {
			int digits;
			if (!strcmp(type2, "uint") || !strcmp(type2, "sint") ||
			    !strcmp(type2, "ulng") || !strcmp(type2, "slng") ||
			    !strcmp(type2, "usht") || !strcmp(type2, "ssht") ||
			    !strcmp(type2, "uchr") || !strcmp(type2, "schr") ||
			    !strcmp(type2, "uidx") || !strcmp(type2, "s128") ||
			    sscanf(type2, "decimal(%d,0)", &digits) ||
			    sscanf(type2, "decimal(%d)", &digits)) {
				if (!type2float)
					func = intconv[log2tab[type2width] * 4 + log2tab[width]];
			}
			break;
		}
		case DB_ADTE_TYPE:
			if (!strcmp(type2, "date"))
				func = convtab_adte[log2tab[type2width]];
			break;
		case DB_VBYTE_TYPE:
		case DB_TXT_TYPE:
			if(!strcmp(type2, "str") || !strncmp(type2, "varchar", 7) ||
			   !strncmp(type2, "char", 4))
				func = &convert_vch;
			break;
		case DB_VCH_TYPE:
			if (!strcmp(type2, "char(1)")) break;
			if(!strcmp(type2, "str") || !strncmp(type2, "varchar", 7) ||
			   !strncmp(type2, "char", 4)) {
				switch (utf8_compatibility_level()) {
				    case UTF8_COMPATIBLE:
					func = &convert_vch;
					break;
				    case UTF8_NON_COMAPATIBLE:
					user = make_adf_user(adf, type < 0 ? -type : type, width);
					func = user ? &convert_adf : &convert_vch;
					break;
				    case UTF8_7BIT_COMPATIBLE:
					user = make_adf_user(adf, type < 0 ? -type : type, width);
					func = user ? &convert_vch_7bit : &convert_vch;
					break;
				}
			}
			break;
		case DB_BYTE_TYPE:
		case DB_CHR_TYPE:
			if (!strcmp(type2, "char(1)")) {
				if (width == 1) func = intconv[log2tab[type2width] * 4 + log2tab[width]];
			} else if(!strcmp(type2, "str") ||
					!strncmp(type2, "char", 4) || !strncmp(type2, "varchar", 7)) {
				func = &convert_cha;
			}
			break;
		case DB_CHA_TYPE:
			if (!strcmp(type2, "char(1)")) {
				if (width == 1) func = intconv[log2tab[type2width] * 4 + log2tab[width]];
			} else if(!strcmp(type2, "str") ||
					!strncmp(type2, "char", 4) || !strncmp(type2, "varchar", 7)) {
				switch (utf8_compatibility_level()) {
				    case UTF8_COMPATIBLE:
					func = &convert_cha;
					break;
				    case UTF8_NON_COMAPATIBLE:
					user = make_adf_user(adf, type < 0 ? -type : type, width);
					func = user ? &convert_adf : &convert_cha;
					break;
				    case UTF8_7BIT_COMPATIBLE:
					user = make_adf_user(adf, type < 0 ? -type : type, width);
					func = user ? &convert_cha_7bit : &convert_cha;
					break;
				}
					
			}
			// handle implicit conversion from TABKEY to CHA
			if (width == 8 && (!strcmp(type2, "slng"))) {
				func = 0;
			}
			break;
		case DB_NCHR_TYPE:
			if(!strcmp(type2, "char(1)")) {
				func = width == 2 ? intconv[log2tab[type2width] * 4 + log2tab[width]] : &convert_nchar1;
				break;
			}
			/* fall trough */
		case DB_NVCHR_TYPE:
			if(!strcmp(type2, "char(1)")) break;
			if(!strcmp(type2, "str") ||
					!strncmp(type2, "char", 4) || !strncmp(type2, "varchar", 7)) {
				user = make_adf_user(adf, type < 0 ? -type : type, width);
				if (user) func = &convert_adf16;
			}
			break;
		case DB_DEC_TYPE: {
			int xscale = 0, xdigits = 10;
			int idigits = type1->db_prec >> 8;
			int iscale = type1->db_prec & 255;
			if (!strcmp(type2, "uint") || !strcmp(type2, "sint") ||
			    !strcmp(type2, "ulng") || !strcmp(type2, "slng") ||
			    !strcmp(type2, "usht") || !strcmp(type2, "ssht") ||
			    !strcmp(type2, "uchr") || !strcmp(type2, "schr") ||
			    !strcmp(type2, "s128") ||
			    !strcmp(type2, "flt") || !strcmp(type2, "dbl") ||
			    sscanf(type2, "decimal(%d,%d)", &xdigits, &xscale) > 0) {
				log_debug("matching decimal %d %d to %d %d", xdigits, xscale, idigits, iscale);
				prec = (idigits << 8) | ((iscale - xscale) & 255);
				if (type2float) {
					func = type2width == 4 ? &convert_flt_dec : &convert_dbl_dec;
				} else {
					func = decconv[log2tab[type2width]];
				}
			}
			break;
		}
		case DB_TMWO_TYPE: {
			int xdigits = 0;
			if (sscanf(type2, "time(%d)", &xdigits) || !strcmp(type2, "time")) {
				prec = xdigits;
				func = (prec ? convtab_time : convtab_time0)[log2tab[type2width]];
			}
			break;
		}
		case DB_TSWO_TYPE: {
			int xdigits = 0;
			if (sscanf(type2, "timestamp(%d)", &xdigits) || !strcmp(type2, "timestamp")) {
				prec = xdigits;
				func = convtab_timestamp[log2tab[type2width]];
			}
			break;
		}
		case DB_INDS_TYPE: {
			int xdigits = 0;
			if (sscanf(type2, "intervalds(%d)", &xdigits) || !strcmp(type2, "intervalds")) {
				prec = xdigits;
				func = convtab_intervalds[log2tab[type2width]];
			}
			break;
		}
		case DB_INYM_TYPE:
			if (!strcmp(type2, "intervalym"))
				func = convtab_intervalym[log2tab[type2width]];
			break;
		case X100_CATNAME_TYPE:
			func = &convert_catstr_cha;
			break;
	}
	log_debug("column added size %d/%d", type2width,width);
	table_conv_add_column_with_null(tc, type2width, width, prec, func, user, type < 0);
	if (func == &convert_clear) {
		log_warn("type convertion not found for %s -> %d", type2, type);
		return -1;
	}
	return 0;
}

int table_conv_get_x100_type(int db_datatype, int db_length, int db_prec, char *buf)
{
	int t = db_datatype;
	int p = db_prec;
	int w = db_length - (db_datatype < 0);
	char *basename = "UNKNOWNTYPE";
	int p1 = 0, p2 = 0, len = 0;
	if (t < 0) t = -t;
	switch (t) {
		case DB_TABKEY_TYPE: basename = "slng"; len = 8; break;
		case DB_MNY_TYPE: basename = "money"; len = 8; break;
		case DB_FLT_TYPE: basename = w < 8 ? "flt" : "dbl"; len = w; break;
		case DB_INT_TYPE: basename = w < 8 ? w < 4 ? w < 2 ? "schr" : "ssht" : "sint" : "slng"; len = w; break;
		case DB_ADTE_TYPE: basename = "date"; len = 4; break;
		case DB_VBYTE_TYPE: basename = "varchar"; p1 = w - 2; len = -p1; break;
		case DB_BYTE_TYPE: basename = "char"; p1 = w; len = w == 1 ? 4 : -w; break;
		case DB_TXT_TYPE:
		case DB_VCH_TYPE: basename = "varchar"; p1 = w - 2; len = -6 * p1; break;
		case DB_CHR_TYPE:
		case DB_CHA_TYPE: basename = "char"; p1 = w; len = w == 1 ? 4 : -6 * w; break;
		case DB_NVCHR_TYPE: basename = "varchar"; p1 = w/2 - 1; len = -6 * p1; break;
		case DB_NCHR_TYPE: basename = "char"; p1 = w/2; len = w == 2 ? 4 : -6 * p1; break;
		case DB_DEC_TYPE: basename = "decimal"; p1 = p >> 8; p2 = p & 255; len = p1 <= 38 ? p1 < 19 ? p1 < 10 ? p1 < 5 ? p1 < 3 ? 1 : 2 : 4 : 8 : 16 : 0; break;
		case DB_TMWO_TYPE: basename = "time"; len = p < 5 ? 4 : 8; p1 = p; break;
		case DB_TSWO_TYPE: basename = "timestamp"; len = p < 8 ? 8 : 0; p1 = p; break;
		case DB_INDS_TYPE: basename = "intervalds"; len = p < 8 ? 8 : 0; p1 = p; break;
		case DB_INYM_TYPE: basename = "intervalym"; len = 4; break;
	}
	if (buf) {
		if (p2) sprintf(buf, "%s(%d,%d)", basename, p1, p2);
		else if (p1) sprintf(buf, "%s(%d)", basename, p1);
		else sprintf(buf, "%s", basename);
	}
	return len;
}
void table_conv_add_null_column(struct table_converter *tc,
		int x100null,
		int iinull)
{
	table_conv_add_column(tc, x100null, iinull, 0, nullconv[x100null * 2 + iinull], NULL);
}

#endif /* conf_IVW */
