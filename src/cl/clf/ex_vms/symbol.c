/*
**    Copyright (c) 1995, 2003 Actian Corporation
*/
#include <compat.h>
#include <rms.h>
#include <objrecdef.h>
#include <lo.h>
#include <si.h>
#include <pc.h>
#include <cv.h>
#include <st.h>
#include <tm.h>
#include <me.h>
#include <nm.h>
#include <ex.h>

#include <evset.h>

#include <descrip.h>

#ifdef axm_vms
#   include <egpsdef.h>
#   include <egsdef.h>
#   include <egstdef.h>
#   include <egsydef.h>
#   include <emhdef.h>
#   include <eobjrecdef.h>
#   include <esdfdef.h>
#   include <esdfmdef.h>
#   include <esdfvdef.h>
#   include <esgpsdef.h>
#endif

#ifdef i64_vms
#   include <elfdef.h>
#endif

#include <rms.h>

#include <starlet.h>

/**
**  Name: symbol.c - Routines to produce symbolic stack dumps
**
**  Description:
**
**	This module provides routines to produce symbolic
**	stack dumps.
**
**	It is part of the VMS port of the ICL Diagnostics project.
**
**	The DBMS symbol table is read to build a symbol table
**	in memory.
**
**	Stack traces are read from a binary file produced by
**	a dumping routine in the server.
**
** History:
**	20-AUG-1995 (mckba01)
**		Initial version.
**	01-Nov-1995 (mckba01)
**		Move file to VMS CL.
**      18-May-1998 (horda03)
**         Ported to OI 1.2/01.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      05-Oct-2011 (horda03) b125821
**          Made to work with Alpha and i64 VMS.
*/


/*
**	RMS control blocks used for accessing the
**	 symbol table (file.STB)
*/
typedef struct
{
   struct FAB fab;
   struct NAM nam;
   struct RAB rab;
   char *ptr;          /* next address to read */
   char *eod;          /* end of user data */
   char block[16*512]; /* must be a multiple of 512 */
} STB_FILE;

STB_FILE *stb_file = NULL;
STB_FILE local_stb_file;

static char		nam_name[NAM$C_MAXRSS];

/*
**	Symbol table entry structure
*/
typedef struct syment
	{
		struct syment	*Next;
		struct syment	*Prev;
		char		SymName[32];
		PTR        	SymValue;
	} SYM_ENT;

static	SYM_ENT	Sym_Table = { &Sym_Table, &Sym_Table, NULL, NULL };


static	i4	SYM_TABread(char *buf, u_i4 size, u_i4 *read);
static	i4	SYM_TABopen();
static	VOID	SymTabSort();
static  VOID	SymTabAdd();
static	VOID	ProcessStacks();
static	VOID	ProcessNSStacks();
static	VOID	SYM_TABclose();
static VOID	SymTabFree();

static SIZE_TYPE STB_File_pos = 0;

static i4 SYM_TABget_record( char *buf, u_i4 *recsize );


#ifdef axm_vms
    /* On Alpha the .STB file is constructed as follows;
    **
    **     EMH$C_MHD header record
    **     EMH$C_LNM header record
    **     Object records
    **     EOBJ$C_EEOM  (end of file)
    **
    ** The only Object record we're interested in are EOBJ$C_EGSD
    ** and we only pick out the symbol records that refer to functions.
    ** Note many symbol records will be contained in the EOBJ$C_EGSD
    ** record.
    **
    ** Also, the first 2 bytes in the file provides the length of
    ** the EMH$C_MHD header record, after this record the next 2 bytes
    ** refer to the length of the EMH$C_LNM header record. The next 2
    ** bytes (in the file) refer to the lemgth of the subsequent record.
    */

    /* On Alpha the SYMBOL records can be one of several types, only interested in
    ** ESDF, ESDFV, ESDFM, EGST, EGPS and ESGPS types.
    **
    ** axp_entry is an array containing the offset into each structure where the
    ** information we're after is located.
    */

#   define POSITION( typ, feld)  (int)&(((struct _##typ##def *)0)->typ##$##feld)

   i4 axp_entry [][5] =
   {
#   define ESDF_POS 0
        POSITION( esdf, l_value),
        POSITION( esdf, l_psindx),
        POSITION( esdf, l_code_address),
        POSITION( esdf, l_ca_psindx),
        POSITION( esdf, b_namlng),
#   define ESDFV_POS 1
        POSITION( esdfv, l_value),
        POSITION( esdfv, l_psindx),
        POSITION( esdfv, l_vector),
        0,
        POSITION( esdfv, b_namlng),
#   define ESDFM_POS 2
        POSITION( esdfm, l_value),
        POSITION( esdfm, l_psindx),
        POSITION( esdfm, l_version_mask),
        0,
        POSITION( esdfm, b_namlng),
#   define ESDFG_POS 3
        POSITION( egst, l_value),
        POSITION( egst, l_psindx),
        POSITION( egst, l_lp_1),
        POSITION( egst, l_lp_2),
        POSITION( egst, b_namlng),
#   define EGPS_POS 4
        0,
        0,
        0,
        POSITION( egps, l_alloc),
        POSITION( egps, b_namlng),
#   define ESGPS_POS 5
        POSITION( esgps, l_value),
        0,
        POSITION( esgps, l_base),
        POSITION( esgps, l_alloc),
        POSITION( esgps, b_namlng)
   };

/*{
**  Name: alpha_get_egsd_details - Processa EGSD record
**
**  Description:
**      Trawl through the EGSYDEF records, picking
**      the ones that define function entry points
**      and add the function details to the Symbol Table.
**
**  Inputs:
**	PTR	buf - Pointer ot the egsdef record
**
**  Side effects:
**
**      Entries added to the Sym_Table
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
i4
alpha_get_egsd_details( char *buf )
{
   EGSDEF  *egsdef = (EGSDEF *) buf;
   EGSYDEF *egsydef;
   EGSYDEF *endrec = (EGSYDEF *) (buf + egsdef->egsd$w_recsiz);
   i4      offset;
   PTR     *addr;
   typedef struct
   {
      u_i1 len;
      char name [1];
   }
   NAME;

   NAME *name;

   for( egsydef = (EGSYDEF *) &egsdef->egsd$w_gsdtyp; egsydef < endrec;
        egsydef = (EGSYDEF *) ((char *)egsydef + egsydef->egsy$w_size) )
   {
#define SET_OFFSET( type, pos ) if (egsydef->egsy$w_gsdtyp == type) offset = pos

      switch( egsydef->egsy$w_gsdtyp)
      {
         case EGSD$C_SYM :
            SET_OFFSET( EGSD$C_SYM, ESDF_POS );
            /* drop through */

         case EGSD$C_SYMG :
            SET_OFFSET( EGSD$C_SYMG, ESDFG_POS );
            /* drop through */

         case EGSD$C_SYMM :
            SET_OFFSET( EGSD$C_SYMM, ESDFM_POS );
            /* drop through */

         case EGSD$C_SYMV :
            SET_OFFSET( EGSD$C_SYMV, ESDFV_POS );

            if ( (egsydef->egsy$v_def != 1) ||
                 !(egsydef->egsy$v_norm) )
            {
                continue;
            }
            break;

         case EGSD$C_PSC :
         case EGSD$C_SPSC :
            continue;

         default :
             printf( "BAD RECORD\n");

             return FAIL;
      }

      name = (NAME *) ( (char *) egsydef + axp_entry [offset][4]);
      addr = (PTR *)  ( (char *) egsydef + axp_entry [offset][2]);

      SymTabAdd( *addr, name->name, name->len);
   }

   return OK;
}

/*{
**  Name: alpha_check_header - Verify record  correct type
**
**  Description:
**      Verify the record and header are of the expected
**      types.
**
**  Inputs:
**	PTR	buf - Pointer ot the EMHDEF record
**
**  Side effects:
**
**      None.
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
static i4
alpha_check_header( char *buf, i4 headertyp)
{
   EMHDEF *header = (EMHDEF *) buf;

   if ( (header->emh$w_rectyp != EOBJ$C_EMH) ||
        (header->emh$w_hdrtyp != headertyp) )
   {
      return FAIL;               /* Not a Header record */
   }

   return OK;
}

/*{
**  Name: alpha_record - Processa EOBJRECDEF record
**
**  Description:
**      Only need to process EOBJ$C_EGSD records.
**      An EOBJ$C_EEOM record indicates end of file.
**
**  Inputs:
**	PTR	buf - Pointer to the EOBJRECDEF record
**
**  Side effects:
**
**      None
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
static i4
alpha_record( char *buf )
{
   EOBJRECDEF *obj = (EOBJRECDEF *) buf;

   if (obj->eobj$w_rectyp == EOBJ$C_EGSD)
   {
      if (alpha_get_egsd_details( buf ))
      {
         return FAIL;
      }
   }
   else if (obj->eobj$w_rectyp == EOBJ$C_EEOM)
   {
      return ENDFILE;
   }

   return OK;
}

/*{
**  Name: alpha_load - Parse the alpha STB file
**
**  Description:
**      Parses the .STB file on a Alpha.
**
**  Inputs:
**	None.
**
**  Side effects:
**
**      None.
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
static i4
alpha_load()
{
   char    buffer [EOBJ$C_MAXRECSIZ];
   u_i4    recsize;
   STATUS  status = FAIL;

   /* Verify that the first two records are an
   ** EMH$C_MHD and an EMH$C_LNM.
   */

   if (! (SYM_TABget_record( buffer, &recsize )   ||
          alpha_check_header( buffer, EMH$C_MHD) ||
          SYM_TABget_record( buffer, &recsize )   ||
          alpha_check_header( buffer, EMH$C_LNM)  ))
   {
      /* Now read the symbol records until EOF. */

      while( (!SYM_TABget_record( buffer, &recsize )) && recsize)
      {
         status = alpha_record( buffer );

         if (status == FAIL)
         {
            break;
         }
         else if (status == ENDFILE)
         {
            status = OK;
            break;
         }
      }
   }

   return status;
}
#endif

#ifdef i64_vms
   /* On IA64 the .STB file is an ELF file, constructed as follows;
   **
   **    ELF64_EHDR header record
   **    ELF64_SHDR section header records
   **    ELF64_SYM  symbol tables
   **               string tables
   **
   ** The ELF64_EHDR is at the start of the file, and details the
   ** number of section header records (these are consequative in the
   ** .STB file) and their position in the file.
   **
   ** We're only interested in the section headers that refer to
   ** Symbol tables and refer to a string table section header.
   ** The section specifies the size in bytes and position of the
   ** referenced Symbol table, and the string table section header
   ** does the same for the string table.
   **
   ** Symbols with symtab$b_st_other defined, are a reference value,
   ** so the PC address is only availble in the application at the
   ** address listed.
   */


/*{
**  Name: elf_symbols - Find C Symbols
**
**  Description:
**      For section records which refer to String and Symbol
**      records, group the Symbol with its C name and
**      memory address.
**
**  Inputs:
**      elf_section  - Array of Section header records
**      num          - Section header index to process
**
**  Side effects:
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
static i4
elf_symbols( ELF64_SHDR *elf_section, i4 num)
{
   i4         number;
   i4         status = FAIL;
   i4         i;
   i4         recsize;
   ELF64_SYM  *syms = NULL;
   ELF64_SYM  *p_sym;
   ELF64_SHDR *str_section;
   ELF64_SHDR *section = &(elf_section [num]);
   char       *strtab = NULL;

   do
   {
      if ( section->shdr$q_sh_entsize == 0)
      {
         TRdisplay( "ELF SYMBOL. invalid shdr$q_sh_entsize\n");

         break;
      }

      if (!section->shdr$l_sh_link)
      {
         /* THe SYM Table section header doesn't refer
         ** to a string table, so don't process.
         */
         status = OK;

         break;
      }

      syms = (ELF64_SYM *) MEreqmem( 0, section->shdr$q_sh_size, FALSE, &status );

      if ( !syms )
      {
         TRdisplay( "ELF SYMBOL.  Failed to allocate %d bytes for syms\n", section->shdr$q_sh_size);

         break;
      }

      /* Create space to hold the SYN Table, and position the file
      ** to load the SYM table into memory.
      */
      recsize = section->shdr$q_sh_size;

      if (SYM_TABposition( section->shdr$q_sh_offset))
      {
         TRdisplay( "ELF SYMBOL. Position to %d failed\n", section->shdr$q_sh_offset);

         break;
      }

      if (SYM_TABget_record( (PTR) syms, &recsize))
      {
         TRdisplay( "ELF SYMBOL. Get record failed\n");

         break;
      }

      number = section->shdr$q_sh_size / section->shdr$q_sh_entsize;

      /* Now create space for the associated STRING table, position the
      ** file to the STRING table location and load it into memory.
      */

      str_section = &(elf_section [section->shdr$l_sh_link]);

      strtab = (char *) MEreqmem( 0, str_section->shdr$q_sh_size, FALSE, &status );

      if ( !strtab )
      {
         TRdisplay( "ELF SYMBOL. Failed to allocate %d bytes for strtab\n",
                    str_section->shdr$q_sh_size);

         break;
      }

      if (SYM_TABposition( str_section->shdr$q_sh_offset))
      {
         TRdisplay( "ELF SYMBOL. Position to %d failed\n",
                    str_section->shdr$q_sh_offset);

         break;
      }

      recsize = str_section->shdr$q_sh_size;

      if (SYM_TABget_record( (PTR) strtab, &recsize))
      {
         TRdisplay( "ELF SYMBOL. Get strtab record failed\n");

         break;
      }

      /* Add each symbol. Note, if the symbol's p_sym->symtab$pq_st_value
      ** is defined, then the p_sym->symtab$pq_st_value is not the memory
      ** location of the function source code, rather its a reference to
      ** where the address of the function can be found, so need to reference
      ** through the pointer.
      */
      for (i = 0, p_sym = syms; i < number; i++, p_sym++)
      {
         SymTabAdd( p_sym->symtab$b_st_other ?
                              (PTR) (*(PTR *)p_sym->symtab$pq_st_value) : (PTR) p_sym->symtab$pq_st_value,
                    strtab + p_sym->symtab$l_st_name,
                    STlength( strtab + p_sym->symtab$l_st_name));
      }

      status = OK;
   }
   while (FALSE);

   if (syms) MEfree( syms );

   if (strtab) MEfree( strtab );

   return status;
}

/*{
**  Name: i64_load - Load symbols for IA64 .STB file
**
**  Description:
**      Verify that the file being processed is a valid .STB ELF
**      file. Then search the Section header records for
**      SYM references.
**
**  Inputs:
**	None.
**
**  Side effects:
**
**      None
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
static i4
i64_load()
{
   ELF64_EHDR elf_header;
   ELF64_SHDR *elf_section = NULL;
   i4         section;
   STATUS     status = FAIL;
   u_i4       recsize;

   do
   {
      /* Get the ELF header record. This should have the magic values
      ** defined.
      **
      ** THe header indicates the number and location of Section header
      ** records.
      */
      recsize = sizeof( ELF64_EHDR );

      if ( SYM_TABget_record( (PTR)&elf_header, &recsize)       ||
           (elf_header.ehdr$b_ei_mag0 != EHDR$K_ELFMAG0) ||
           (elf_header.ehdr$b_ei_mag1 != EHDR$K_ELFMAG1) ||
           (elf_header.ehdr$b_ei_mag2 != EHDR$K_ELFMAG2) ||
           (elf_header.ehdr$b_ei_mag3 != EHDR$K_ELFMAG3) )
      {
         TRdisplay( "Get ELF header record failed\n");

         break;
      }

      recsize = sizeof( ELF64_SHDR ) * elf_header.ehdr$w_e_shnum;

      elf_section = (ELF64_SHDR *) MEreqmem( 0, recsize, FALSE, &status );

      if (!elf_section)
      {
         TRdisplay( "ELF file. Failed to allocate memeory for %d elf sections\n",
                    elf_header.ehdr$w_e_shnum);

         break;
      }

      if (SYM_TABposition( elf_header.ehdr$q_e_shoff ))
      {
         TRdisplay( "ELF file position to %d failed\n", elf_header.ehdr$q_e_shoff);

         break;
      }

      if ( SYM_TABget_record( (PTR)elf_section, &recsize) )
      {
         TRdisplay( "ELF file. Failed to read elf sections\n");

         break;
      }

      /* Scan the Section header looking for references to SYMTABs. */

      for( section = 0; section < elf_header.ehdr$w_e_shnum; section++)
      {
         if ( (elf_section [section].shdr$l_sh_type == SHDR$K_SHT_SYMTAB) &&
              ( (status = elf_symbols( elf_section,  section)) ) )
         {
            TRdisplay( "Get elf_symbols failed\n");

            break;
         }
      }
   }

   while(FALSE);

   if (elf_section)
   {
      MEfree( (PTR) elf_section );
   }

   return status;
}
#endif

/*{
**  Name: DIAG_VMS_dump_stacks - produce formatted thread stacks
**
**  Description:
**      This routine is the entry point for producing
**	formatted stack dumps for the threads in
**	the DBMS server.
**
**  Inputs:
**	VOID	outfcn		output function
**	PTR	symfile		symbol table file name
**	PTR	stkfile		Binary stack dump file
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial VMS version.
**
*/
VOID
DIAG_VMS_dump_stacks(outfcn,symfile,stkfile)
VOID	outfcn();
PTR	symfile;
PTR	stkfile;
{
	MEadvise(ME_USER_ALLOC);

        if (!SYM_TABload(symfile))
	{
	    ProcessStacks(stkfile,outfcn);
	}
	else
	{
		/*	Open Failed 	*/
		outfcn("Can`t open Symbol Table file : %s\n",symfile,0,0,0,0,0,0);
		outfcn("Producing NON - symbolic stack traces\n\n",0,0,0,0,0,0,0);
		ProcessNSStacks(stkfile,outfcn);
		return;
	}

	SymTabFree();
	return;
}

/*{
**  Name: SYM_TABload - Populate the memory Symbol Table
**
**  Description:
**        Read the Symbol table records and sort.
**
**  Inputs:
**        PTR     symfile         symbol table file name
**
**  Side effects:
**
**        Symbol Table populated
**
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created.
*/

i4
SYM_TABload(char *symfile)
{
   i4      status = FAIL;

   if (!SYM_TABopen(symfile))
   {
#if defined(axm_vms)

      status = alpha_load();

#elif defined(i64_vms)

      status = i64_load();

#endif

      SYM_TABclose();

      if ( (status == OK) &&
           Sym_Table.Next != &Sym_Table)
      {
         SymTabSort();
      }
   }

   return status;
}

/*{
**  Name: FindSymName - returns symbol namefor address
**
**  Description:
**      Looks up an address in the symbol table
**	and retruns the corresponding symbol name string
**	If address is out of reange, it returns the
**	address as a string.
**
**  Inputs:
**	PTR     	LookupValue	address to find
**	PTR      	SymbolName	string in which result is returned
**      SIZE_TYPE	*offset         Offset from symbol address
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/
static i4
FindSymName(LookupValue, SymbolName, Offset)
PTR		LookupValue;
PTR		SymbolName;
SIZE_TYPE       *Offset;
{
	SYM_ENT	*SymEntPtr = Sym_Table.Next;

	if(LookupValue < SymEntPtr->SymValue)
        {
		return FAIL;	/*	Below range of our symbols	*/
        }

	while(SymEntPtr != &Sym_Table)
	{
		if (SymEntPtr->Next == &Sym_Table)
                {
                     return FAIL;
                }

		if (SymEntPtr->Next->SymValue > LookupValue)
		{
			STcopy(SymEntPtr->SymName,SymbolName);
                        *Offset = (SIZE_TYPE) (LookupValue - SymEntPtr->SymValue);
			return OK;
		}
		SymEntPtr = SymEntPtr->Next;
	}

	return FAIL;
}




/*{
**  Name: ProcessStacks - outputs symbolic stack dump
**
**  Description:
**	Reads stack dump information from the binary file
**	specified, maps pc address to the symbol name
**	and outputs the trace.
**
**  Inputs:
**	PTR	stk_file	name of binary stack dump file
**	VOID	outfcn		procedure to do output
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/
static
VOID
ProcessStacks(stk_file,outfcn)
PTR	stk_file;
VOID	outfcn();
{
	char		tempstr[100] = "abcdefghijklmnb";
	char		RetSymName[100];
	LOCATION	loc;
	FILE		*Fdesc;
	EV_STACK_ENTRY	StackRec;
	i4		count;
        SIZE_TYPE       Offset;



	if(LOfroms(PATH &FILENAME,stk_file, &loc) != OK)
	{
		outfcn("Can't Locate Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);
        	return;
	}

	if(SIfopen(&loc,"r",SI_RACC,(i4) sizeof(StackRec),&Fdesc)!= OK)
	{
		outfcn("Can't Open Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);
		return;
	}

	while(SIread(Fdesc,sizeof(StackRec),&count,&StackRec) == OK)
	{
		if(STcompare(StackRec.Vstring,tempstr))
		{
			STcopy(StackRec.Vstring,tempstr);
			outfcn("%s",tempstr);
		}
		if (FindSymName(StackRec.pc,RetSymName, &Offset))
                {
                   STprintf( RetSymName, "%p", StackRec.pc);
                }
		outfcn("%08x:%-16s %08x,%08x,%08x,%08x,%08x,%08x\n"
			,StackRec.sp,RetSymName,StackRec.args[0]
			,StackRec.args[1],StackRec.args[2],StackRec.args[3]
			,StackRec.args[4],StackRec.args[5]);
	}

	SIclose(Fdesc);

}


/*{
**  Name: ProcessNStacks - outputs NON symbolic stack dump
**
**  Description:
**	Reads stack dump information from the binary file
**	specified and outputs it as a formatted
**	NON symbolic stack trace.
**
**  Inputs:
**	PTR	stk_file	name of binary stack dump file
**	VOID	outfcn		procedure to do output
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/
static
VOID
ProcessNSStacks(stk_file,outfcn)
PTR	stk_file;
VOID	outfcn();
{
	char		tempstr[100] = "abcdefghijklmnb";
	LOCATION	loc;
	FILE		*Fdesc;
	EV_STACK_ENTRY	StackRec;
	i4		count;





	if(LOfroms(PATH &FILENAME,stk_file, &loc) != OK)
	{
		outfcn("Can't Locate Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);
        	return;
	}

	if(SIfopen(&loc,"r",SI_RACC,(i4) sizeof(StackRec),&Fdesc)!= OK)
	{
		outfcn("Can't Open Stack Dump file : %s\n",stk_file,0,0,0,0,0,0);
		return;
	}

	while(SIread(Fdesc,sizeof(StackRec),&count,&StackRec) == OK)
	{
		if(STcompare(StackRec.Vstring,tempstr))
		{
			STcopy(StackRec.Vstring,tempstr);
			outfcn("%s",tempstr);
		}
		outfcn("%08x:%08x (%08x,%08x,%08x,%08x,%08x,%08x)\n"
			,StackRec.sp,StackRec.pc,StackRec.args[0]
			,StackRec.args[1],StackRec.args[2],StackRec.args[3]
			,StackRec.args[4],StackRec.args[5]);
	}

	SIclose(Fdesc);

}


/*{
**  Name: SymTabAdd - Adds a symbol to the symbol table
**
**  Description:
**	Adds a new symbol entry into the linked list
**	of symbols.
**
**  Inputs:
**
**  Side effects:
**	u_i4	SymVal		Symbol Address.
**	PTR	SymName		Symbol Name.
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/
VOID
SymTabAdd(SymVal,SymName,SymLen)
PTR		SymVal;
PTR		SymName;
i4              SymLen;
{
	SYM_ENT		*NewPtr;

	/*	Allocate new Symbol entry	*/

	NewPtr = (SYM_ENT *) MEreqmem(0, (u_i4) sizeof(SYM_ENT), FALSE, NULL);

	/*	Fill in details		*/

	NewPtr->SymValue = SymVal;
	STlcopy(SymName, &NewPtr->SymName, SymLen);
	NewPtr->Next = &Sym_Table;
	NewPtr->Prev = Sym_Table.Prev;

	/*	Add to end of table	*/

	Sym_Table.Prev->Next = NewPtr;
        Sym_Table.Prev = NewPtr;
}

/*{
**  Name: SymTabSort - sorts the symbol table
**
**  Description:
**	Sorts the symbol table by ascending symbol address.
**
**      The list starts out as a unsort bi-directional linked list of N
**      entries (entries numbered 1-N).
**
**         Start at entry a   (a = 1..N-1)
**            Find lowest entry L (checking a..N)
**            if a != L Insert L before a
**              else set a to next entry
**
**      Inserting L before a simply involves manipulating the bi-directional
**      links of the entries.
**
**
**  Inputs:
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**      05-Oct-2011 (horda03) b125821
**              Original was very inefficient, as it constantly
**              swopped entries, rather than just rearrange the
**              pointers. In testing o;d algorithm took 3 seconds (real time)
**              to sort 5661 entries, new algorithm less within a second.
*/
static
VOID
SymTabSort()
{
	SYM_ENT	*HeadPtr;
	SYM_ENT	*CurEntry;
        SYM_ENT *lowest;

	HeadPtr = &Sym_Table;

	while( HeadPtr->Next != &Sym_Table)
	{
		lowest = HeadPtr->Next;

                CurEntry = lowest->Next;

		while(CurEntry != &Sym_Table)
		{
			/*	Compare with previous record	*/

                        if(CurEntry->SymValue < lowest->SymValue)
                        {
				lowest = CurEntry;
                        }


			/*	Move to next level;	*/

			CurEntry = CurEntry->Next;
		}
		/*	Move to next level	*/

                if (lowest != HeadPtr->Next)
                {
                   /* Lowest isn't in the correct position,
                   ** So remove it from its current position.
                   ** and insert it after HeadPtr.
                   */

                   lowest->Next->Prev = lowest->Prev;
                   lowest->Prev->Next = lowest->Next;

                   lowest->Next = HeadPtr->Next;
                   lowest->Prev = HeadPtr;
                   HeadPtr->Next->Prev = lowest;
                   HeadPtr->Next       = lowest;
                }

		HeadPtr = lowest;
	}
}




/*{
**  Name: SYM_TABopen - opens the symboltable file
**
**  Description:
**	Opens the symbol table file specified by
**	the supplied file name.
**
**  Inputs:
**	PTR	Sym_Tab_Name	Symbol table file name
**
**  Returns:
**	0	= OK
**	1	= FAIL
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/
static
i4
SYM_TABopen(Sym_Tab_Name)
PTR	Sym_Tab_Name;
{
   i4      status;
   char    efn [2];


   if (stb_file)
   {
      printf( "ERROR - only 1 STB file can be opened\n");

      return FAIL;
   }

   stb_file = &local_stb_file;

   stb_file->fab = cc$rms_fab;
   stb_file->fab.fab$l_fna = Sym_Tab_Name;
   stb_file->fab.fab$b_fns = STlength(Sym_Tab_Name);
   stb_file->fab.fab$l_dna = "";
   stb_file->fab.fab$b_dns = 0;
   stb_file->fab.fab$b_fac = FAB$M_BIO | FAB$M_GET;
   stb_file->fab.fab$b_shr = FAB$M_SHRGET;
   stb_file->fab.fab$l_nam = &stb_file->nam;

   stb_file->nam = cc$rms_nam;
   stb_file->nam.nam$l_rsa = stb_file->nam.nam$l_esa = efn;
   stb_file->nam.nam$b_rss = stb_file->nam.nam$b_ess = 0;

   status = sys$open(&stb_file->fab,0,0);
   if (status & 1)
   {
      stb_file->rab = cc$rms_rab;
      stb_file->rab.rab$l_fab = &stb_file->fab;
      stb_file->rab.rab$l_ubf = stb_file->block;
      stb_file->rab.rab$w_usz = sizeof(stb_file->block);

      status = sys$connect(&stb_file->rab,0,0);
      if (status & 1)
      {
         stb_file->eod    = 0;
         stb_file->ptr    = stb_file->block;

         return OK;
      }

      sys$close( &stb_file->fab, 0, 0);
   }

   /* Open of STB file failed. */

   stb_file = NULL;

   return FAIL;
}

/*{
**  Name: SYM_TABposition - MOve to file position
**
**  Description:
**      Set the VMS file pointer to the specified offset.
**
**  Inputs:
**	pos - Offset into file
**
**  Side effects:
**
**      File placed at desired position.
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
i4
SYM_TABposition( SIZE_TYPE pos )
{
   i4 status;

   if (pos < STB_File_pos)
   {
      /* Need to move backwards, so return to the
      ** start of the file.
      */
      STB_File_pos = 0;

      sys$rewind( &stb_file->rab,0,0);
   }

   stb_file->rab.rab$w_rsz = 0;

   /* Read through the file until we get the the
   ** required block.
   */

   while (STB_File_pos + stb_file->rab.rab$w_rsz < pos)
   {
      STB_File_pos += stb_file->rab.rab$w_rsz;

      stb_file->rab.rab$l_bkt = 0;

      status = sys$read(&stb_file->rab,0,0);

      if ( (status == RMS$_EOF) ||
           !(status & 1) )
      {
         return status;
      }
   }

   stb_file->ptr = stb_file->block + (pos - STB_File_pos);
   stb_file->eod = stb_file->block + stb_file->rab.rab$w_rsz - 1;

   STB_File_pos += stb_file->rab.rab$w_rsz;

   return OK;
}

/*{
**  Name: SYM_TABread - read a record from a symbol table file.
**
**      Read an object language record from the symbol table file
**
**  Inputs:
**      PTR     buf             Area in which to retrun the record read
**      u_i2    size            Number of bytes to read.
**      u_i2    *read            Parameter in which number of bytes read is returned
**
**  Returns:
**      OK
**      FAIL
**  Side effects:
**
**  History:
**      05-Oct-2011 (horda03) b125821
**         Created.
*/
static i4
SYM_TABread( char *buf, u_i4 size, u_i4 *read )
{
   u_i4 r_size, cp_bytes;
   i4   status;
   char *req_end;

   if (!stb_file)
   {
      /* STB file not open. */

      return FAIL;
   }

   for (r_size = size, *read = 0; r_size; r_size -= cp_bytes)
   {
      if (stb_file->ptr > stb_file->eod)
      {
         /* Read in the next block of data */

         stb_file->rab.rab$l_bkt = 0;

         status = sys$read(&stb_file->rab,0,0);

         if (status == RMS$_EOF) break;

         if ( !(status &1) ) return status;

         stb_file->ptr = stb_file->block;
         stb_file->eod = stb_file->ptr + stb_file->rab.rab$w_rsz - 1;

         STB_File_pos += stb_file->rab.rab$w_rsz;
      }

      req_end = stb_file->ptr + r_size -1;

      if (req_end > stb_file->eod)
      {
         /* Don't have all the required bytyes in the read buffer, so
         ** copy what we have, and then get the rest.
         */
         req_end = stb_file->eod;
      }

      cp_bytes = req_end - stb_file->ptr + 1;

      MEcopy( stb_file->ptr, cp_bytes, buf + *read);

      *read += cp_bytes;
      stb_file->ptr += cp_bytes;
   }

   return OK;
}


/*{
**  Name: SYM_TABget_record - get next  record from an AXP symbol table file.
**
**      Read a object language record from the symbol table file
**
**  Inputs:
**      PTR     buf             Area in which to retrun the record read
**      u_i4    *recsize        Size of record
**
**  Returns:
**      OK
**      FAIL
**  Side effects:
**
**  History:
**      05-Oct-2011 (horda03) b125821
**         Created.
*/
i4
SYM_TABget_record( char *buf, u_i4 *recsize )
{
   u_i4 read;

#ifdef axm_vms
   /* On Alpha, the length of the next record is contained in the
   ** next 2 bytes of the file.
   */
   *recsize = 0;

   if (SYM_TABread((char *) recsize, sizeof(u_i2), &read) != OK)
   {
      return FAIL;
   }

   *recsize += *recsize%2;
#endif

   if (SYM_TABread(buf, *recsize, &read) != OK)
   {
      return FAIL;
   }

   if (read != *recsize)
   {
      return FAIL;
   }

   return OK;
}

/*{
**  Name: SYM_TABclose - Closes the symbol table file.
**
**  Description:
**	Closes the connection to the open symbol table file.
**
**  Inputs:
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**      05-Oct-2011 (horda03) b125821
**              Reworked for Alpha and IA64.
**
*/
static
VOID
SYM_TABclose()
{
   if (stb_file)
   {
      sys$close(&stb_file->fab,0,0);

      stb_file = NULL;
   }
}

/*{
**  Name: SymTabFree - frees memory used by symbol table
**
**  Description:
**	Steps through the linked list of symbol definitions
**	freeing each.
**
**  Inputs:
**
**  Side effects:
**
**  History:
**	20-8-1995 (mckba01)
**		Initial version.
**
*/
static
VOID
SymTabFree()
{
	SYM_ENT	*SymEntPtr;
	SYM_ENT	*hold;

	SymEntPtr = Sym_Table.Next;


	while(SymEntPtr != &Sym_Table)
	{
		hold = SymEntPtr;
		SymEntPtr = SymEntPtr->Next;
		MEfree((PTR) hold);
	}
}


/*{
**  Name: EX_symname - Fimd symbol name for an address
**
**  Description:
**      Given an address, determine the related symbol and
**      the offset from the symbol location.
**
**  Inputs:
**      PTR     address - Address to de-reference
**	PTR	buf     - Pointer to buffer to contain result
**
**  Side effects:
**
**      None.
**
**  History:
**	05-Oct-2011 (horda03) b125821
**         Created
**
*/
char *
EX_symname( PTR address, char *buf )
{
   static i4 load_sym_tbl = 1;
   char   name [MAX_LOC+1];
   SIZE_TYPE offset;

   if (load_sym_tbl)
   {
      /* Symbol table has not been loaded, so do it now. */

      load_sym_tbl = 0;

      if (Sym_Table.Next == &Sym_Table)
      {
         $DESCRIPTOR (line, "$LINE");
         $DESCRIPTOR (commline, "");
         u_i2    commline_len;
         char    commline_buf[256];
         char    *p, *cmd;
         i4      coln_found = 0;
         char loc_part [MAX_LOC+1];
         char fname [MAX_LOC+1];
         LOCATION loc;

         /* Get the name of the EXE being executed, so that the .STB file
         ** can be loaded.
         */
         commline.dsc$a_pointer = commline_buf;
         commline.dsc$w_length = sizeof (commline_buf);
         cli$get_value (&line, &commline, &commline_len);
         commline_buf[commline_len] = '\0';

         for (p = cmd = commline_buf; *p; p++)
         {
            if ( (!coln_found) && (*p == ' ') )
            {
               cmd = p + 1;
            }
            else if (*p == ':')
            {
               coln_found = 1;
            }
            else if (coln_found && (*p == ' '))
            {
               *p = '\0';
               break;
            }
         }

         LOfroms( PATH & FILENAME, cmd, &loc );

         LOdetail( &loc, loc_part, loc_part, fname, loc_part, loc_part);

         STprintf( loc_part, "II_SYSTEM:[INGRES.DEBUG]%s.STB", fname);

         LOfroms( PATH & FILENAME, loc_part, &loc );

         if (!LOexist( &loc ))
         {
            /* Found the .STB file, so load it */

            SYM_TABload( loc_part );
         }
      }
   }

   if ( (Sym_Table.Next == &Sym_Table) ||
         FindSymName( address, name, &offset ) )
   {
      *buf = '\0';
   }
   else
   {
      STprintf( buf, offset ? "%s+%p" : "%s", name, offset);
   }

   return buf;
}
