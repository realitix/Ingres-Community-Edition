#if defined(conf_IVW) && !defined(IITABLECONV_INCLUDED)
#define IITABLECONV_INCLUDED

struct table_converter;

int table_conv_add_typed_column(struct table_converter *tc,
		GCA_DBMS_VALUE *type1,
		char *type2,
		int type2len,
		int type2float,
		ADF_CB *adf);
void table_conv_add_null_column(struct table_converter *tc,
		int x100null,
		int iinull);
int table_conv_get_x100_type(int db_datatype,
		int db_length,
		int db_prec,
		char *buf);

/* fake type to mark char column using x100 name escaping scheme */
#define X100_CATNAME_TYPE 199

#endif
