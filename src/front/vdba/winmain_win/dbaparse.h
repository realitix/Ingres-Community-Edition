/********************************************************************
//
//  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.
//
//    Project : Ingres Visual DBA
//
//    Source : dbaparse.h
//    Parse objects description
//
//      12-Mar-2001 (schph01)
//      SIR 97068 add parameter BOOL bSqlViewType in prototype function
//      GetViewTables(): language used in statement to create the view,
//      TRUE SQL language, FALSE QUEL language.
//    10-Aug-2011 (frima01) Bug 125297
//      Declare ParsePermitUserType to avoid confusing grants to non-
//      existing roles with grants of existing users of the same name.
********************************************************************/

#define SEGMENT_TEXT_SIZE 240
#define MAXCOLDEFAULTLEN 1501+1
#define MAX_PRIVILEGES 7
struct SecuritySets{
	int Value;
	int TypeOn : 1;
};

void FreeText();

BOOL ConcatText(UCHAR *Data);

UCHAR *ParseRuleText(UCHAR *PUout);

UCHAR *ParseProcText(UCHAR *PUout);

void ParseSecurityText(UCHAR *extradata, 
										struct SecuritySets *SecuritySet,
                        int jmax);

int ParsePermitText(int rt1, UCHAR *extradata);

int ParsePermitUserType(UCHAR *extradata);

LPUCHAR ParseRuleText(LPUCHAR PUout);
int GetViewTables(LPUCHAR tablename, LPUCHAR lpOwnerName,BOOL bSqlViewType);
