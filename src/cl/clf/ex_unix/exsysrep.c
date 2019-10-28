/*
** Copyright (c) 1985, 2004 Actian Corporation
**
*/

#ifdef __sparcv9
#define __sparcv9cpu
#endif

#include	<compat.h>
#include	<systypes.h>
#include	<clconfig.h>
#include	<clsigs.h>
#include	<gl.h>
#include        <iicommon.h>
#include	<lo.h>
#include	<nm.h>
#include	<pc.h>
#include	<st.h>
#include	<tr.h>
#include	<ex.h>
#include	<er.h>
#include	<cs.h>
#include	<evset.h>
#include	<exinternal.h>


GLOBALDEF void (*Ex_print_stack)(CS_SCB *, void *, PTR, TR_OUTPUT_FCN *, i4);
GLOBALDEF     i4 Ex_core_enabled = 0;      /* 1 = allow core          */

static STATUS ex_print_error(PTR arg1, i4 msg_length, char * msg_buffer);

# if defined(any_aix)
static VOID	dump_ris_trace(char *buffer, unsigned long sp);
# endif


/*{
** Name:    EXsys_report() -	Report System/Hardware Exceptions.
**
** Description:
**	Reports if an exception was generated by hardware or the operating
**	system.  When this is the case, an appropriate message is copied into
**	the buffer and TRUE is returned.  Otherwise, FALSE is returned.
**
** Inputs:
**	exarg	{EX_ARGS *}  The exception argument structure.
**	buffer	{char *}  The address of the buffer that can contain the
**			  message for the hardware/system execption.
**
** Returns:
**	{bool}	TRUE, if hardware or operating system exception.
**		FALSE otherwise.
**
** History:
**	xx/85 (fhc) -- Originally written by Fred as 'EXaccvio()'.
**	12/86 (jhw) -- Generalized for all hardware/system execptions.
**	07/87 (jhw) -- Added context support for BSDSIGS.
**	6/88 (daveb) -- put in sparc stuff, eliminated dependance on
**			obsolete BSD define, using BSDSIGS instead.
**			This is really messy machine dependant stuff,
**			and it will turn in a twisty maze of little
**			ifdefs very quickly.  When it gets bad, we need
**			to rethink how this is constructed.
**	07-mar-89 (russ)Changing ifdef BSDSIG to use the more general
**			xCL_011_USE_SIGVEC define.
**	06-jul-89 (mikem)
**	    Removed extraneous STcopy() which was causing SEGV's from 
**	    printing the relevant PC's.
**	28-Jul-89 (anton)
**	    Corrected previous STcopy fix for non SIGVEC machines
**	    Added stack dump to TR output on mc68000s
**	6-Dec-89 (anton)
**	    Added memory bounds check durring stack dumps.
**	16-Apr-90 (kimman)
**	    Added ds3_ulx specific code for signal printing.
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Add specials for pyr_u42, pyr_us5, ds3_ulx, ps2_us5, vax_ulx,
**		rtp_us5, sur_u42, dr5_us5, ib1_us5, hp3_us5, dg8_us5, hp8_us5;
**		Rearrange case statement;
**		Add more ifdef warnings to encourage people to do the right
**		thing;
**		Increase size of fmt variable to accommodate largest cp plus
**		REGFMT;
**		Changed include <signal.h> to <clsigs.h>.
**	04-oct-90 (jkb)
** 	    add code for sqs_ptx to report sp and pc on exceptions
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.  Removed sqs_us5 code duplicated in clsigs.h.
**	09-apr-91 (seng)
**	    Changes specific to the RS/6000 exception handler.
**	31-jan-92 (bonobo)
**	    Replaced double-question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	26-apr-93 (vijay)
**	    Moved <clsigs.h> before <ex.h> to avoid redefine of EXCONTINUE 
**		in the system header sys/context.h of AIX.
**          Removed duplicate code line after ifdef dg8_us5:
**              scp = (struct sigcontext *)exargs->exarg_array[2];
**	    Display stack trace info for ris. *(sp+1) gives the backptr
**	    and *(backptr+3) gives the saved lr.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      20-aug-93 (ed)
**          add missing include
**	25-aug-93 (swm)
**	    Added axp_osf machine-specific code.
**	31-dec-1993 (andys)
**	    If we get a SIGBUS or SIGSEGV, call Ex_print_stack to
**          call CS_dump_stack which will print a stack trace.
**          Note that Ex_print_stack is null unless initialised by a
**          dbms server in CSinitiate.
**	    Add routine ex_print_error.
**	31-jan-1994 (mikem)
**	    bug #57042
**	    Added support for su4_us5 to produce interesting
**	    PC and stack tracing in the case of AV's.
**      22-sep-93 (smc)
**	    Bug #56446
**          Changed initalisation of scp to use portable scp element
**          in EX_ARGS.
**	08-dec-93 (swm)
**	    Bug #56446
**	    Corrected register name displayed via STprintf for axp_osf.
**	10-feb-94 (ajc)
**	    Added hp8_bls specific entries based on hp8*
**	14-feb-94 (smc)
**	    Bug #60614
**	    Moved the define for REGFMT and register display code for
**	    axp_osf outside the SIGVEC/SIGACTION conditional, because
**	    it would not compile after a change to use SIGACTION rather
**	    than SIGVEC and there is actually no conditional dependency
**	    on them.
**	14-mar-94 (nrb)
**	    Bug #60684
**	    Corrected bug introduced by Bug fix #57042 for non-SIGVEC 
**	    platforms other than su4_us5 and sqs_ptx.
**	15-mar-94 (swm)
**	    Bug #60615
**	    The axp_osf register display in EXsys_report was setup to
**	    always display zeroes, and in any case did not include
**	    registers containing oprands likely to have caused a trap.
**	    Changed code to pick up sigcontext pointer from unix portion
**	    of EX_ARGS and display program counter, processor status and
**	    three operand/argument registers.
**	    Corrected comments on how signal, code and scp are picked up
**	    from EX_ARGS.
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**	14-may-1995 (sweeney)
**	    Change the error message associated with SIGPWR. The
**	    existing message is bordering on meaningless and generates
**	    unnecessary calls to Tech Support.
**      28-Nov-94 (nive/walro03)
**          For dg8_us5, Removed use of struct siginfo from if USE_SIGVEC
**          area since sigaction is used and not sigvec for dg8_us5
**	22-jun-95 (allmi01)
**	    Added dgi_us5 support for DG-UX on Intel base platforms following dg8_us5.
**	19-sep-95 (nick)
**	    Changes to support guard pages plus additional cleaning.
**	22-feb-1996 (prida01)
**	    Add support for diagnostic server
**	14-Mch-1996 (prida01)
**	    Add extra parameter to EXdump;
**	18-Mch-1996 (prida01)
**	    For access violation print symbol for function it
**	    occurred in for sun solaris.
**	14-nov-96 (kinpa04)
**		Added formatting stmt(STprintf) for axp_osf platform
**      20-Jun-97 (radve01)
**          axp_osf: register list extended to entire program context
**	05-Dec-1997 (allmi01)
**	    Added proper entries for sgi_us5 initial port.
**	12-jan-1998 (fucch01)
**	    Moved systypes.h, clconfig.h, and clsigs.h up in the includes
**	    list (just after compat.h), due to compile problems on sgi_us5.
**      18-feb-98 (toumi01)
**	    #ifdef SIGSYS (doesn't exist on Linux).
**  14-Oct-1998 (merja01)
**      Added axp_osf specific call to CS_axp_osf_dump_stack to produce
**      diagnostic stack trace.
**  05-Nov-1998 (merja01)
**      Changed axp stack dump to use Ex_print_stack, and print
**      exception name with formated registers.  Also, added
**      printing of vfp.
**	06-Nov-1998 (muhpa01)
**	    Added exception message & call for stack tracing for both
**	    hp8_us5 & hpb_us5.
**	18-jan-1999 (muhpa01)
**	    For hp8_us5 build on 10.20, remove code for access to old
**	    (10.10) version of ucontext_t.
**	10-may-1999 (walro03)
**	    Remove obsolete version string cvx_u42, dr5_us5, ib1_us5, ps2_us5,
**	    pyr_u42, rtp_us5, sqs_u42, sqs_us5, vax_ulx.
**	24-Sep-1999 (hanch04)
**	    Print out header infomation for stack dump.
**      29-Sep-1999 (hweho01)
**          Added support for AIX 64-bit (ris_u64) platform.
**	30-Nov-1999 (devjo01)
**	    Change core generation default to off.
**	08-aug-2000 (hayke02)
**	    Remove distinction between hp8_us5 and hpb_us5 in stack tracing
**	    code - assignments of ucontext.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-apr-1999 (hanch04)
**	    Added __sparcv9 for 64 bit Solaris 7
**      05-Sep-2000 (hweho01)
**          Put back the REGFMT definition for rs4_us5. 
**	24-jul-2001 (toumi01)
**	    Hack up some questionable code for i64_aix - FIXME !!!
**	11-dec-2001 (somsa01)
**	    Porting changes for hp2_us5.
**      23-Sep-2002 (hweho01)
**          Added changes for hybrid build on AIX platform.
**	06-May-2003 (bonro01)
**	    Don't support display of pc & sp in i64_hpu
**	    Don't display pc for LP64 platform, code is designed for
**	    32-bit pointers.
**	20-Oct-2004 (devjo01)
**	    Display registers, and allow stack dump for int_lnx
**	23-Aug-2005 (schka24)
**	    Sparcv9 on Solaris 10 has CCR instead of PSR at reg 0.
**	06-Oct-2005 (hanje04)
**	    Add register format and enable stack printing for x86_64
**	    Linux (a64_lnx)
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**      20-Nov-2006 (hanal04) Bug 117159
**          Get CS_dump_stack() working on hp2 in 64-bit mode.
**	04-Oct-2007 (bonro01)
**	    Add CS_sol_dump_stack() for su4_us5, su9_us5, a64_sol
**	    for both 32bit and 64bit stack tracing.
**      29-Jan-2008 (hanal04) Bug 119831
**          Grab the PC address on i64_hpu. Stack support will be added
**          but for now have to give support something to go on.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid and config symbols changed, fix here.
**	16-Nov-2010 (kschendel) SIR 124685
**	    Make Ex-print-stack call match the real CS-dump-stack.
**	    Delete a few more unused platform ifdefs (mc68000?!?).
**	23-Nov-2010 (kschendel)
**	    Drop obsolete DG ports.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/

bool
EXsys_report (exargs, buffer)
register EX_ARGS	*exargs;
char			*buffer;
{

    register EX		ex_n = exargs->exarg_num;
    register char	*cp;
    char                fmt[ 256 ];
    char                msg[ 80 ];
    char                *buffer_hdr;
    int                 sig;
    i4                  print_stack = FALSE;    /* shld we print stack trace? */
# if defined(any_aix) && defined(BUILD_ARCH64)
    unsigned long       stack_p = 0L;
# else
    u_i4		stack_p = 0;
# endif
    i4                  pid;
    STATUS              status;
    CL_ERR_DESC		error;

# ifdef sgi_us5
    struct sigcontext *scp;
# endif

# ifdef axp_osf
    struct sigcontext *scp;
    long pc = 0L;			/* program counter */
    long ps = 0L;			/* processor status */
    long pv = 0L;			/* procedure value */
    long gp = 0L;			/* global pointer */
	long sp = 0L;           /* stack pointer */ 
	long vfp = 0L;          /* Virtual frame pointer */
	long ra = 0L;           /* return(caller) address */
	long v0 = 0L;           /* last return code */
	long a0 = 0L;           /* argument registers */
	long a1 = 0L; 
	long a2 = 0L; 
	long a3 = 0L; 
	long a4 = 0L; 
	long a5 = 0L; 
	long t0 = 0L;           /* temp registers */
    long t1 = 0L;
    long t2 = 0L;
    long t3 = 0L;
    long t4 = 0L;
    long t5 = 0L;
    long t6 = 0L;
    long t7 = 0L;
    long t8 = 0L;
    long t9 = 0L;
    long t10= 0L;
    long t11= 0L;
    long s0 = 0L;           /* saved registers */
    long s1 = 0L;
    long s2 = 0L;
    long s3 = 0L;
    long s4 = 0L;
    long s5 = 0L;
#define REGFM0 "@PC=%p,PS=%p,PV=%p,GP=%p,SP=%p,VFP=%p,RA=%p,v0=%p\n"
#define REGFM1 "a0-5[%p,%p,%p,%p,%p,%p]\n"
#define REGFM2 "t0-11[%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p]\n"
#define REGFM3 "s0-5[%p,%p,%p,%p,%p,%p]\n"
#define REGFMT CONCAT(CONCAT(CONCAT(REGFM0, REGFM1), REGFM2), REGFM3)
# endif /* axp_osf */

# ifdef xCL_011_USE_SIGVEC

    register struct sigcontext	*scp;

# ifdef sparc
    int                 pc = 0;
    int                 npc = 0;
    int                 psr = 0;
    int                 g1 = 0;
    int                 o0 = 0;
# define        REGFMT "@ PC %x NPC %x PSR %x G1 %x o0 %x"
# endif /* sparc */

# if defined(sgi_us5)
    int                 pc = 0;
# define        REGFMT "@ PC = 0x%x"
# endif /* sgi_us5 */

# if defined(any_hpux)
#if defined(LP64)
      uint64_t  pc = 0;
      uint64_t  sp = 0;
# define        REGFMT "@ PC = 0x%lx, SP = 0x%lx"
#else
      int               pc = 0;
      int               sp = 0;
# define        REGFMT "@ PC = 0x%x, SP = 0x%x"
#endif
# define        sc_pc   sc_pcoq_head
# endif	/* any_hpux */

# if defined(rs4_us5)
	int pc = 0;
	unsigned long sp = 0;
# define REGFMT "@ PC = 0x%x ;"
# endif /* rs4_us5 */

# if defined(r64_us5)
        unsigned long pc = 0L;
        unsigned long sp = 0L;
# define REGFMT "@ PC = 0x%p ;"
# endif  /* r64_us5  */

# ifndef REGFMT
        # error "you need to customize exsysrep.c for your machine"
# endif

# else /* xCL_011_USE_SIGVEC */
#  ifdef xCL_068_USE_SIGACTION
#   if ( defined(xCL_072_SIGINFO_H_EXISTS) || defined(LNX) ) \
	 && defined(xCL_071_UCONTEXT_H_EXISTS)
    /*
    ** Linux implicitly includes siginfo.h from a non-standard location
    ** from within ucontext.h
    */
    register ucontext_t     *ucontext;

#    if defined(int_lnx) || defined (int_rpl)
#    define        REGFMT "@PC %08x\n" \
                          "ESP %08x  EBP %08x  ESI %08x  EDI %08x\n" \
		          "EAX %08x  EBX %08x  ECX %08x  EDX %08x\n" \
                          "DS  %08x  ES  %08x  SS  %08x\n"
#    elif defined(a64_lnx)
#    define        REGFMT "@PC %016lx\n" \
                          "RSP %016lx  RBP %016lx  RSI %016lx\n" \
			  "RDI %016lx  RAX %016lx  RBX %016lx\n" \
			  "RCX %016lx  RDX %016lx\n"
#    endif

#    if defined(sparc_sol) || defined(a64_sol)
    long                pc = 0;
    long                sp = 0;
    long                psr = 0;
    long                g1 = 0;
    long                o0 = 0;
#    if defined(sparc_sol)
#    define        REGFMT "@ PC %lx SP %lx PSR %lx G1 %lx o0 %lx"
#    else
#    define        REGFMT "@ PC %lx SP %lx"
#    endif
#    endif /* su4_us5 su9_us5 a64_sol */
#   endif /* xCL_072_SIGINFO_H_EXISTS && xCL_071_UCONTEXT_H_EXISTS */
#  endif /* xCL_011_USE_SIGACTION */
# if defined(sgi_us5)
    long long		pc = 0;
    long long		gp = 0;
    long long		sp = 0;
    long long		fp = 0;
# define        REGFMT "@ EPC = 0x%x GP = 0x%x SP 0x%x FP 0x%x "
# endif /* sgi_us5 */

# if defined(any_aix)
# if defined(LP64)
         unsigned long pc = 0L;
         unsigned long sp = 0L;
# define REGFMT "@ PC = 0x%p ;"
# else
         int pc = 0;
         unsigned long sp = 0;
# define REGFMT "@ PC = 0x%x ;"
# endif /* LP64 */
# endif /* aix */

# if defined(any_hpux)
#if defined(LP64)
    uint64_t            pc = 0;
    uint64_t            sp = 0;
    uint64_t            psw = 0;
# define        REGFMT  "@ PC = %lx, SP = %lx, PSW = %lx"
#else
    int                 pc = 0;
    int                 sp = 0;
    int                 psw = 0;
# define        REGFMT  "@ PC = %x, SP = %x, PSW = %x"
#endif
# endif /* hp-ux */ 

# endif /* xCL_011_USE_SIGVEC */
    /* in case caller just assumes we did something */
    if ( buffer )
    {
        buffer_hdr = buffer;
        buffer = buffer + STlength(buffer);
        STprintf(buffer, "Unknown exception %d (0x%x)", ex_n, ex_n );
    }

    /*
    ** This should report all hardware or system signals mapped to our
    ** internal Ingres exceptions.
    **
    ** For system delivered signals the argument count will be 3:
    ** The Unix-specific portion of EX_ARGS includes the elements:
    **    i4 signal	the signal number (or 0 if not raised by signal)
    **    i4 code	the sigcontext code or garbage.
    **    PTR scp	either a sigcontext structure or some other non-zero
    **			value.
    **
    ** For software raised exceptions (EXsignal), the argument count will
    ** be 1, and exarg_array[] will contain nothing.
    **
    */

    if ( exargs->exarg_count != 3 )
        return FALSE;           /* definately not a system raised exception */

    /*
    ** Decode the signal and point cp at the right message.  It's handy
    ** if these cases match the order in exsignal.c.  Non-signal raised
    ** exceptions that happen to have 3 args also go through here.
    */
    sig = exargs->signal;
    switch (ex_n)
    {
	/* 
	** Because of peculiar System V semantics, this is raised by a
	** handler other than i_EXcatch when it is interesting.  EX does
	** not catch it by default.
	*/
      case EXCHLDDIED:
# ifdef SIGCHLD
	cp = "Child died (SIGCHLD)";
# elif defined(SIGCLD)
	cp = "Child died (SIGCLD)";
# endif
	break;

      case EXHANGUP:
	cp = "Hangup (SIGHUP)";
	break;

      case EXINTR:
	cp = "Interrupt signal (SIGINT)";
	break;

      case EXQUIT:
	cp = "Quit signal (SIGQUIT)";
	break;

      case EXRESVOP:
	/* not used anymore, should never be called */
	STprintf(msg, "EXRESVOP driven by signal %d\n", sig );
	cp = msg;
	break;

      case EXBUSERR:
	/* not used anymore, should never be called */
	STprintf(msg, "EXBUSERR driven by signal %d\n", sig );
	cp = msg;
	break;

      case EXFLOAT:	/* undecoded SIGFPE */
	cp = "Floating Point Exception (SIGFPE)";
	break;
      case EXDECOVF:
	cp = "Floating Point Exception (SIGFPE):  Decimal overflow";
	break;
      case EXFLTDIV:
      case EXHFLTDIV:
	cp = "Floating Point Exception (SIGFPE):  Floating divide by zero";
	break;
      case EXFLTOVF:
      case EXHFLTOVF:
	cp = "Floating Point Exception (SIGFPE):  Floating overflow";
	break;
      case EXFLTUND:
      case EXHFLTUND:
	cp = "Floating Point Exception (SIGFPE):  Floating underflow";
	break;
      case EXINTDIV:
      case EXHINTDIV:
	cp = "Floating Point Exception (SIGFPE):  Integer divide by zero";
	break;
      case EXINTOVF:
      case EXHINTOVF:
	cp = "Floating Point Exception (SIGFPE):  Integer overflow";
	break;

      case EXTRACE:
	cp = "Trace trap (SIGTRAP)";
	break;

      case EXSEGVIO:
	switch( sig )
	{
# ifdef SIGXCPU
	case SIGXCPU:
	    cp = "CPU time limit exceeded (SIGXCPU), see setrlimit(2)";
	    break;
# endif
# ifdef SIGXFSZ
	case SIGXFSZ:
	    cp = "File size limit exceeded (SIGXFSZ), see setrlimit(2)";
	    break;
# endif
# ifdef SIGDANGER
	case SIGDANGER:
	    cp = "DANGER signal (SIGDANGER)";
	    break;
# endif
# ifdef SIGVTALRM
	case SIGVTALRM:
	    cp = "Virtual timer (SIGVTALRM)";
	    break;
# endif
# ifdef SIGPROF
	case SIGPROF:
	    cp = "Profile timer (SIGPROF)";
	    break;
# endif
# ifdef SIGLOST
	case SIGLOST:
	    cp = "Lost resource (SIGLOST)";
	    break;
# endif
# ifdef SIGUSR1
	case SIGUSR1:
	    cp = "User signal 1 (SIGUSR1)";
	    break;
# endif
# ifdef SIGUSR2
	case SIGUSR2:
	    cp = "User signal 2 (SIGUSR2)";
	    break;
# endif
# ifdef SIGEMT
	case SIGEMT:
	    cp = "EMT instruction (SIGEMT)";
	    break;
# endif
# ifdef SIGSYSV
	case SIGSYSV:
	    cp = "Bad argument to system call (SIGSYSV)";
	    break;
# endif
# ifdef SIGPWR
	case SIGPWR:
	    cp = "Power failure indication (SIGPWR)";
	    break;
# endif
# ifdef SIGPOLL
	case SIGPOLL:
	    cp = "(SIGPOLL)";
	    break;
# endif
	case SIGBUS:
	    cp = "Bus Error (SIGBUS)";
            print_stack = TRUE;
	    break;
	case SIGILL:
	    cp = "Illegal instruction (SIGILL)";
	    break;
# ifdef SIGSYS
	case SIGSYS:
	    cp = "Bad system call arguments (SIGSYS)";
	    break;
# endif
	case SIGSEGV:
	    cp = "Segmentation Violation (SIGSEGV)";
            print_stack = TRUE;
	    break;
	default:
	    STprintf(msg, "EXSEGVIO driven by signal %d\n", sig );
	    cp = msg;
	}
	break;

      case EXTIMEOUT:
	cp = "Alarm clock (SIGALRM)";
	break;

      case EXCOMMFAIL:
	cp = "Pipe signal (SIGPIPE)";
	break;

      case EXBREAKQUERY:
	cp = "Software termination signal (SIGTERM)";
	break;

      default:
	return FALSE;

    } /* end switch */


    /*
    ** If we can show register values, pick them up.
    */
# ifdef axp_osf
    scp = (struct sigcontext *)exargs->scp;

    if (scp != (struct sigcontext *)0)
    {
	pc = scp->sc_pc;
	ps = scp->sc_ps;
	pv = scp->sc_regs[27];
	gp = scp->sc_regs[29];
	sp = scp->sc_regs[30];
	vfp = exc_find_frame_ptr(NULL, scp, NULL);
	ra = scp->sc_regs[26];
	v0 = scp->sc_regs[0];
	a0 = scp->sc_traparg_a0;
	a1 = scp->sc_traparg_a1;
	a2 = scp->sc_traparg_a2;
	a3 = scp->sc_regs[19];
	a4 = scp->sc_regs[20];
	a5 = scp->sc_regs[21];
	t0 = scp->sc_regs[1];
	t1 = scp->sc_regs[2];
	t2 = scp->sc_regs[3];
	t3 = scp->sc_regs[4];
	t4 = scp->sc_regs[5];
	t5 = scp->sc_regs[6];
	t6 = scp->sc_regs[7];
	t7 = scp->sc_regs[8];
	s0 = scp->sc_regs[9];
	s1 = scp->sc_regs[10];
	s2 = scp->sc_regs[11];
	s3 = scp->sc_regs[12];
	s4 = scp->sc_regs[13];
	s5 = scp->sc_regs[14];
	t8 = scp->sc_regs[22];
	t9 = scp->sc_regs[23];
	t10= scp->sc_regs[24];
	t11= scp->sc_regs[25];
    }
# define gotregs
# endif /* axp_osf */

# ifdef xCL_011_USE_SIGVEC

    scp = (struct sigcontext *)exargs->scp;

# ifdef sparc
    pc = scp->sc_pc;
    npc = scp->sc_npc;
    psr = scp->sc_psr;
    g1 = scp->sc_g1;
    o0 = scp->sc_o0;
# define gotregs
# endif
# if defined(sgi_us5)
    pc = scp->sc_pc;
# define gotregs
# endif
# if defined(any_hpux)
    pc = scp->sc_pc;
    sp = scp->sc_sp;
    stack_p = sp;
# define gotregs
# endif
# if defined(any_aix)		/*see <mstsave.h>*/
	pc = scp->sc_jmpbuf.jmp_context.iar;
	sp = scp->sc_jmpbuf.jmp_context.gpr[1];
    	stack_p = sp;
# define gotregs
# endif /* aix */

#ifndef gotregs
    # error "exsysrep: need to get regs from scp here for your machine"
# endif

#else  /* xCL_011_USE_SIGVEC */

# ifdef xCL_068_USE_SIGACTION
#   if ( defined(xCL_072_SIGINFO_H_EXISTS) || defined(LNX) ) \
	 && defined(xCL_071_UCONTEXT_H_EXISTS)
    ucontext = (ucontext_t *)exargs->scp;

#   if defined(int_lnx) || defined(int_rpl) || defined(a64_lnx)
     /* We'll forego copying regs from ucontext to individual vars */
#    define gotregs
#   endif

#   if defined(sparc_sol) || defined(a64_sol)
    pc = ucontext->uc_mcontext.gregs[REG_PC];
    sp = ucontext->uc_mcontext.gregs[REG_SP];
#   if defined(sparc_sol)
#   if defined(REG_CCR)
    psr = ucontext->uc_mcontext.gregs[REG_CCR];
#   else
    psr = ucontext->uc_mcontext.gregs[REG_PSR];
#   endif
    g1 = ucontext->uc_mcontext.gregs[REG_G1];
    o0 = ucontext->uc_mcontext.gregs[REG_O0];
#   endif
    stack_p = sp;
#   define gotregs
#   endif /* su4_us5 su9_us5 a64_sol */

# if defined(any_hpux)
# if defined(i64_hpu)
    __uc_get_ip(ucontext, &pc);
# elif defined(hp2_us5)
    pc = ucontext->uc_mcontext.ss_wide.ss_64.ss_pcoq_head;
# else
    pc = ucontext->uc_mcontext.ss_narrow.ss_pcoq_head;
# endif  /* hp2_us5 */
    pc &= ~3;
# if defined(i64_hpu)
    sp = 0;
    psw = 0;
# elif defined(hp2_us5)
    sp = ucontext->uc_mcontext.ss_wide.ss_64.ss_sp;
    psw = ucontext->uc_mcontext.ss_wide.ss_64.ss_cr22;
# else
    sp = ucontext->uc_mcontext.ss_narrow.ss_sp;
    psw = ucontext->uc_mcontext.ss_narrow.ss_cr22;
# endif  /* hp2_us5 */
#   define gotregs
# endif /* hpux */

#  endif /* xCL_072_SIGINFO_H_EXISTS && xCL_071_UCONTEXT_H_EXISTS */
# endif /* xCL_011_USE_SIGACTION */

# define gotregs
# endif /* xCL_011_USE_SIGVEC */
# ifdef sgi_us5
# define gotregs
    scp = (struct sigcontext *)exargs->scp;

    pc = scp->sc_pc;
    gp = scp->sc_regs[28];
    sp = scp->sc_regs[29];
    fp = scp->sc_regs[30];
# endif /* sgi_us5 */


    /*
    ** Format the message as appropriate.
    */
    if( buffer )
    {
# ifdef axp_osf
	STprintf( fmt, "\n%s\n %s", cp, REGFMT );
	STprintf(buffer, fmt, pc, ps, pv, gp, sp, vfp, ra, v0,
					  a0, a1, a2, a3, a4, a5,
					  t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
					  s0, s1, s2, s3, s4, s5);
    TRdisplay(REGFMT, pc, ps, pv, gp, sp, vfp, ra, v0,
					  a0, a1, a2, a3, a4, a5,
					  t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
					  s0, s1, s2, s3, s4, s5);

# define gotfmt
# endif /* axp_osf */

# ifdef xCL_011_USE_SIGVEC
	/* figure out real format string */
	STprintf( fmt, "%s %s", cp, REGFMT );

	/* now print the message, formatting the registers */
# ifdef sparc
	STprintf( buffer, fmt, pc, npc, psr, g1, o0 );
# define gotfmt
# endif
# if defined(sgi_us5)
	STprintf(buffer, fmt, pc );
# define gotfmt
# endif
# if defined(any_hpux)
	STprintf(buffer, fmt, pc, sp );
# define gotfmt
# endif

# if defined(any_aix)
	STprintf(buffer, fmt, pc );
	dump_ris_trace(buffer+STlength(buffer), sp);
# define gotfmt
# endif

# ifndef gotfmt
	# error "exsysrep: need to STprintf your regs here."
# endif

# else /* xCL_011_USE_SIGVEC */

	/* figure out real format string */

#  ifdef xCL_068_USE_SIGACTION
#   if ( defined(xCL_072_SIGINFO_H_EXISTS) || defined(LNX) ) \
	 && defined(xCL_071_UCONTEXT_H_EXISTS)
#    if defined(any_hpux) && !defined(LP64)
	/* If we can get a symbol then print it out */
	{
	    PTR sym = NULL;
	    u_i4 addr = 0;
	    sym = (PTR)DIAGSymbolLookupAddr(pc,0);
	    if (sym)
	    {
		addr = DIAGSymbolLookupName(sym);
		STprintf( fmt, "%s %s(0x%x) %s", cp,sym,addr,REGFMT);
	    }
	    else
		STprintf( fmt, "%s %s", cp,REGFMT);
	    STprintf( buffer, fmt, pc, sp, psw );
	}
#    define gotfmt
#    endif /* hpux */

#    if defined(sparc_sol) || defined(a64_sol)
	/* If we can get a symbol then print it out */
	{
	    char sym[80];
	    long addr = 0, offset=0;
	    int  len;

	    sym[0] = '\0';
	    len = DIAGSymbolLookup(pc,&addr,&offset,sym,sizeof(sym));
	    if (len)
	    {
		STprintf( fmt, "%s %s(0x%lx+0x%lx) %s", 
			 cp,sym,addr,offset,REGFMT);
	    }
	    else
		STprintf( fmt, "%s %s", cp,REGFMT);
# if defined(sparc_sol)
	    STprintf( buffer, fmt, pc, sp, psr, g1, o0 );
# else
	    STprintf( buffer, fmt, pc, sp );
# endif
	}
#    define gotfmt
#    endif /* solaris */

#if  defined(hp2_us5)
	/* If we can get a symbol then print it out */
	{
            PTR sym = NULL;
            uint64_t addr = 0;
            sym = (PTR)DIAGSymbolLookupAddr(pc,0);
            if (sym)
            {
                STprintf( fmt, "%s %s %s", cp,sym,REGFMT);
            }
            else
                STprintf( fmt, "%s %s", cp,REGFMT);
            STprintf( buffer, fmt, pc, sp, psw );
        }
#    define gotfmt
#    endif /* hp2_us5 */

#    if defined(int_lnx) || defined(int_rpl)
     STprintf( fmt, "%s %s", cp, REGFMT );
     STprintf( buffer, fmt, ucontext->uc_mcontext.gregs[REG_EIP],
               ucontext->uc_mcontext.gregs[REG_ESP],
               ucontext->uc_mcontext.gregs[REG_EBP],
               ucontext->uc_mcontext.gregs[REG_ESI],
               ucontext->uc_mcontext.gregs[REG_EDI],
               ucontext->uc_mcontext.gregs[REG_EAX],
               ucontext->uc_mcontext.gregs[REG_EBX],
               ucontext->uc_mcontext.gregs[REG_ECX],
               ucontext->uc_mcontext.gregs[REG_EDX],
               ucontext->uc_mcontext.gregs[REG_DS],
               ucontext->uc_mcontext.gregs[REG_ES],
               ucontext->uc_mcontext.gregs[REG_SS] );
#    define gotfmt
#    endif /* int_lnx || int_rpl */

#    if defined(a64_lnx)
     STprintf( fmt, "%s %s", cp, REGFMT );
     STprintf( buffer, fmt, ucontext->uc_mcontext.gregs[REG_RIP],
               ucontext->uc_mcontext.gregs[REG_RSP],
               ucontext->uc_mcontext.gregs[REG_RBP],
               ucontext->uc_mcontext.gregs[REG_RSI],
               ucontext->uc_mcontext.gregs[REG_RDI],
               ucontext->uc_mcontext.gregs[REG_RAX],
               ucontext->uc_mcontext.gregs[REG_RBX],
               ucontext->uc_mcontext.gregs[REG_RCX],
               ucontext->uc_mcontext.gregs[REG_RDX] );
#    define gotfmt
#    endif /* a64_lnx */

#   endif /* xCL_072_SIGINFO_H_EXISTS && xCL_071_UCONTEXT_H_EXISTS */
#  endif /* xCL_011_USE_SIGACTION */

# ifndef gotfmt
	STcopy( cp, buffer );
# endif

# endif /* xCL_011_USE_SIGVEC */

	ERsend(ER_ERROR_MSG, buffer_hdr, STlength(buffer_hdr), &error);
    }

    if (print_stack && Ex_print_stack)
    {
	/* dump to evidence sets */
	EXdump(EV_SIGVIO_DUMP,&stack_p);
#   ifdef axp_osf
        Ex_print_stack(NULL, scp, NULL, ex_print_error, TRUE);
# elif defined(any_hpux) || defined(int_lnx) || defined(int_rpl) || \
       defined(a64_lnx) || defined(sparc_sol) || defined(a64_sol)
        Ex_print_stack( NULL, ucontext, NULL, ex_print_error, TRUE );
# else
        Ex_print_stack(NULL, stack_p, NULL, ex_print_error, TRUE);
#   endif
    }

    if (Ex_core_enabled)
    {
        Ex_core_enabled = 0;
        switch (pid = PCfork(&status))
        {
        case -1:
            PCexit(status);
        case 0:
            signal(SIGABRT, SIG_DFL);
            abort();
        default:
            break;
        }
    }
    return TRUE;
}

# if defined(any_aix)

GLOBALREF	char	*MEbase, *MElimit;

static  void
dump_ris_trace(char *buffer, unsigned long sp)
{
    int *fp;
    int stack_depth = 0;

    STprintf(buffer, "... Call Trace: ");
    buffer = buffer + STlength(buffer);
    for (fp = (int *)sp; fp && (stack_depth < 6); fp = (int *)*fp,stack_depth++)
    {
	if (((char *)fp < MElimit && (char *)fp > MEbase) 
	|| ( abs ((int)((int)&fp - *fp)) < 0x10000 ) )   /* non threaded */
	{
	    STprintf(buffer, "0x%p; ",  *(fp+2) );
	    buffer = buffer + STlength(buffer);
	}
	else
	    break;
    }

}

# endif /* aix */


/*
**
** Name:    ex_print_error() -  print ex message to errlog.log
**
** Description:
**      Function passed to CS_dump_stack to print message to errlog.log.
**      Called through TRformat hence first argument is a dummy for us.
**
** Inputs:
**      arg1                    ignored.
**      msg_length              length of message.
**      msg_buffer              bufer for message.
**
** Returns:
**      void
**
** History:
**      13-dec-1993 (andys)
**              Created.
**      31-dec-1993 (andys)
**              Add ER_ERROR_MSG parameter to ERsend.
*/
static STATUS
ex_print_error(PTR arg1, i4 msg_length, char * msg_buffer)
{
    CL_ERR_DESC err_code;
    ERsend(ER_ERROR_MSG, msg_buffer, msg_length, &err_code);
    return (OK);
}
