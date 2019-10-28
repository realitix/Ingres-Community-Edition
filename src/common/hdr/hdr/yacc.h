/*
** Copyright (c) 2004 Actian Corporation
*/

/**
** Name: YACC.H - Definitions for re-entrant yacc.
**
** Description:
**      This file must be included in re-entrant parsers generated by RTI's
**      version of yacc.  It defines the control block used by the driver
**      program.
**
** History: $Log-for RCS$
**      26-dec-85 (jeff)
**          written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Jan-2004 (schka24)
**	    Invent an alternate-keyword flag for WITH PARTITION=.
**/

#define                 YYMAXDEPTH      150

/*}
** Name: YYTOK - queue of token values from scanner (SQL)
**
** Description:
**	This structure is for scanners that support read-ahead.
**  When told to readahead, the scanner will save tokens and their
**  semantic values until the desired token is found. When told to
**  unload the stack, the scanner will do so. For example, given
**  'select col1 from tab1', after the select token is returned, the
**  scanner will be told to readahead to the FROM token. All values after
**  the select but prior to the FROM token will be saved. The first value
**  in the queue will always be TARGET. This allows the grammar to be written
**  with the saved phrase marked by a special value. When the scanner reaches
**  the end of the input stream, the saved phrase will be returned to the
**  grammar.
**
** (schka24 note) It appears that this structure is unused and the above
** comment is bogus, at least in the backend.  I don't see it in the
** character front-ends either.
**
** History:
**	9-feb-87 (daved)
**	    written
*/
typedef struct _YYTOK
{
    i4		ret_val;	/* token */
    YYSTYPE	tok_val;	/* semantic value */
    PTR		err_val;	/* pointer to string giving rise to above 
				** values. pss_prvtok
				*/
    PTR		err_end;	/* end of token pss_nxtchar */
    i4		err_line;	/* line number of token */
} YYTOK;

/*}
** Name: YYTOK_LIST - Array of YYTOK elements that can be linked together
**
** Description:
**	Queries with large SQL target lists can easily blow a moderately
**  sized YYTOK stack.  Instead of making the maximum size very big, and
**  thus consuming a lot of space that may never get used, we have opted
**  to make the YYTOK stack grow.  To that end, this structure is
**  used to link YYTOK stacks together.
**
** (schka24 note) It appears that this structure is unused, at least in
** the backend.  I don't see it in the character front-ends either.
**
** History:
**     19-jun-87 (daved)
**          written
*/
typedef struct _YYTOK_LIST
{
    struct _YYTOK_LIST  *yy_next;
    YYTOK	    yyr[YYMAXDEPTH];
}   YYTOK_LIST;

/*}
** Name: YACC_CB - Control block for re-entrant yacc parsers.
**
** Description:
**      Re-entrant yacc parsers, as generated by RTI's version of yacc, use
**      control blocks of this type for keeping track of state information.
**      Re-entrant parsers take a pointer to this structure as their single
**      parameter.
**
** History:
**     26-dec-85 (jeff)
**          written
**	10-sep-01 (inkdo01)
**	    Added yy_rwdstr & yyreswd to support context sensitive reswds.
**	26-Jan-2004 (schka24)
**	    Add yy_partalt_kwds to turn on PARTITION= context keywords.
*/
typedef struct _YACC_CB
{
    i4              yychar;		/* Last token from scanner */
    i4		    yytmp;
    i4		    yystate;
    i4		    yynerrs;
    i4		    yydebug;		/* TRUE iff debugging desired */
    i4		    *yyps;
    i4		    yyerrflag;
    i4		    yyerr_decl;		/* Rtn val from action if in function */
    i4		    (*yylex)();		/* Pointer to scanner function */
    VOID	    (*yyerror)();	/* Pointer to error handling function */
    i4		    yys[YYMAXDEPTH];
    YYSTYPE	    *yypv;
    YYSTYPE	    yylval;		/* Semantic value set up by scanner */
    YYSTYPE	    yyval;
    YYSTYPE	    yyv[YYMAXDEPTH];	/* Semantics stack */
    YYSTYPE	    *yypvt;
    YYTOK_LIST	    yyr;
    YYTOK_LIST	    *yyr_push;		/* current YYTOK queue - write token */
    YYTOK_LIST	    *yyr_pop;		/* current YYTOK queue - read token */
    i4		    yyr_top;		/* last token in yyr_push */
    i4		    yyr_cur;		/* current value in yyr_pop */
    i4		    yyr_save;		/* save until this token is found */
    i4		    yyrestok;		/* saved value of overturned resword */
    char	    *yy_rwdstr;		/* ptr to string determined to be reswd */
    bool	    yyreswd;		/* TRUE - token is/was reserved word 
					** FALSE - token is no longer reswd */
    bool	    yy_partalt_kwds;	/* TRUE to recognize PARTITION= keywords
					** only, FALSE = normal set.
					*/
}   YACC_CB;
