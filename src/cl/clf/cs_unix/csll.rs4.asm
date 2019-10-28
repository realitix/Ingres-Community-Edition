##Copyright (c) 2004 Actian Corporation
#
# csll.rs4.asm
#
# CS for IBM RS/6000
#
# History:
#       (long ago) (vijay)
#               Created.
#	10 Oct 92 (vijay)
#		Save condition code register. Else, on the new compiler, csphil
#		would hang if the source file is compiled with optimization on.
#	20-jun-93 (vijay)
#		Save floating point registers.
#	27-dec-95 (thoro01)
#		changed _CS_swuser to CS_swuser ...  compile errors.
#	01-nov-1999 (mosjo01)
#		created from csll.ris.asm for rs4_us5 use.
#		expand/replace SAVEPFR and RESTOREFPR macros from AIX 4.2, 
#		no longer on AIX 4.3.
#       22-Apr-2003 (hweho01)
#               Added special NOP statement to avoid the warning msg 
#               from linker when libcompat shared library is being built.
#       26-Jun-2009 (kschendel) SIR 122138
#           Include r64_us5 code too.  Jam will now cpp csll.s (which
#           will include this file, and select the proper variant)
#           before it m4's and as's the result.
#       30-Jul-2009 (hweho01) BUG 125456
#           Remove change dated 26-Jun-2009, the 32-bit assembly and  
#           the 64-bit one need to be seperate files, because cpp is 
#           unable to process the file which contains # for in-line   
#           comments, the assembly file is processed by m4 on AIX.
#	12-Nov-2010 (kschendel) SIR 124685
#	    Moved inkernel and current to front of CS_SYSTEM, fix here.
#

#ifdef rs4_us5
# byte offsets into a CS_SCB structure

	.set CS__EX_SP,32
	.set CS__SP,40
	
# offsets into a CS_SYSTEM structure */

	.set CS__INKERNEL,204
	.set CS__CURRENT,184

# Parameters of the stack frame used by CS */

	.set CS_FRAME_SIZE,18*8+19*4+6*4
# 8*nfpr(18*8)+4*ngpr(=19*4) + linkarea(=6*4) + localstkarea(=0)+argarea(=0)
	.set CS_SAVEFPR, 18
	.set CS_SAVEGPR, 19

# if 0 
# /* This code was massaged into the assembler that follows */
# VOID
# CS_swuser(void)
# {
# 	extern CS_SYSTEM Cs_srv_block;
# 	extern CS_SCB Cs_idle_scb;
# 	extern CS_SCB *CS_xchng_thread();
# 	extern PTR ex_sptr;
# 	register CS_SCB *scb;
# 	register nat fakefp;
# 
# 	if( Cs_srv_block.cs_inkernel )
# 		return;
# 
# 	scb = Cs_srv_block.cs_current;
# 	if( scb != NULL )
# 	{
# 	    /* Save existing exception stack */
# 	    scb->cs_exsp = ex_sptr;
# 
# 	    /* Save the FP, LINK, and non-volatile regs for this thread */
# 	    saveregs( scb );
# 	}
# 
# 	/* use idle thread's exception stack while selecting */
# 	ex_sptr = Cs_idle_scb.cs_exsp;
# 
# 	/* run on idle thread's stack too, if possible */
# 	fakefp =  Cs_idle_scb.cs_registers[ CS_SP ];
# 
# 	scb = CS_xchng_thread( scb );
# 
# 	/*
# 	** causes FP and LINK and saved regs to be reloaded,
# 	** returning to another thread
# 	*/
# 	restore_regs( scb );
# }
# endif /* 0 */

	.align	1

CS_swuser:

# We save only nonvolatile registers on the stack.

	S_PROLOG(CS_swuser)

	mfspr	r0,lr			# get link register for return address

._sfpr..14:   stfd  14, -144(1)
._sfpr..15:   stfd  15, -136(1)
._sfpr..16:   stfd  16, -128(1)
._sfpr..17:   stfd  17, -120(1)
._sfpr..18:   stfd  18, -112(1)
._sfpr..19:   stfd  19, -104(1)
._sfpr..20:   stfd  20, -96(1)
._sfpr..21:   stfd  21, -88(1)
._sfpr..22:   stfd  22, -80(1)
._sfpr..23:   stfd  23, -72(1)
._sfpr..24:   stfd  24, -64(1)
._sfpr..25:   stfd  25, -56(1)
._sfpr..26:   stfd  26, -48(1)
._sfpr..27:   stfd  27, -40(1)
._sfpr..28:   stfd  28, -32(1)
._sfpr..29:   stfd  29, -24(1)
._sfpr..30:   stfd  30, -16(1)
._sfpr..31:   stfd  31, -8(1)
					# save floating point registers
	stm	13, -8*(CS_SAVEFPR)-4*(CS_SAVEGPR)(r1)
					# save integer registers
	st	r0,8(r1)		# save return addr from lr
	mfcr	r12			# save condition code reg
	st	r12,4(r1)		# 
	stu	r1, -CS_FRAME_SIZE(r1)	# push a frame and save back ptr

# Don't do anything else if in kernel mode

	LTOC(r4,Cs_srv_block,data)
	l	r3,CS__INKERNEL(r4)	# pickup cs_inkernel
	cmpi	0,r3,0			# test it
	bne	sw_ret			# if in kernel, just return

# Don't save if there is no current thread.

	l	r3,CS__CURRENT(r4)	# r3 = current scb, will be param later
	cmpi	0,r3,0			# skip save on first thread.
	beq	sw_no_save

# Save exception stack and frame pointer and the registers
	LTOC(r5,ex_sptr,data)		# loads address from TOC entry
	l	r6,0(r5)		# get current global exception stk ptr
	st	r6,CS__EX_SP(r3)	# save it in scb
	st	r1,CS__SP(r3)		# save fp: actually aren't using this
					# registers already stored in stk

sw_no_save:

# Use idle thread's exception stack and frame.

	LTOC(r5,ex_sptr,data)		# loads address from TOC entry
	LTOC(r6,Cs_idle_scb,data)	
	l	r7,CS__EX_SP(r6)	# get idle ex stack ptr 
	st	r7,0(r5)		# and set global ex stack to it...
	l	r1,CS__SP(r6)		# pick up idle thread FP.

# do we need idle thread's toc? or xchng thread's toc?
# xchng_thread returns the new scb in r3, given the current one in r3.
# Use new fp and exception stack off scb returned...

	bl	.CS_xchng_thread
	cror	31, 31, 31              # special NOP 
	cror	0, 0, 0
	l	r1,CS__SP(r3)
	l	r0,CS__EX_SP(r3)
	LTOC(r5,ex_sptr,data)		# loads address from TOC entry
	st	r0,0(r5)		# set global ex stack

#/* Get back to caller's frame and return */
sw_ret:

	ai	r1,r1,CS_FRAME_SIZE	# pop frame
	l	r0,8(r1)		# read saved lr from stk
	mtlr	r0  			# restore return addr
	lm	r13,-8*(CS_SAVEFPR)-4*(CS_SAVEGPR)(r1)

._rfpr..14:   lfd  14, -144(1)
._rfpr..15:   lfd  15, -136(1)
._rfpr..16:   lfd  16, -128(1)
._rfpr..17:   lfd  17, -120(1)
._rfpr..18:   lfd  18, -112(1)
._rfpr..19:   lfd  19, -104(1)
._rfpr..20:   lfd  20, -96(1)
._rfpr..21:   lfd  21, -88(1)
._rfpr..22:   lfd  22, -80(1)
._rfpr..23:   lfd  23, -72(1)
._rfpr..24:   lfd  24, -64(1)
._rfpr..25:   lfd  25, -56(1)
._rfpr..26:   lfd  26, -48(1)
._rfpr..27:   lfd  27, -40(1)
._rfpr..28:   lfd  28, -32(1)
._rfpr..29:   lfd  29, -24(1)
._rfpr..30:   lfd  30, -16(1)
._rfpr..31:   lfd  31, -8(1)
	         			# restore floating regs from stack
	l       r12,4(r1)               # 
	mtcr    r12                     # restore crs(only need cr2,cr3,cr4,cr5)
	S_EPILOG
	FCNDES(CS_swuser)

#/*** Test-and-set etc are in c for rios */
	.toc
	TOCE(Cs_srv_block,data)
	TOCE(ex_sptr,data)
	TOCE(Cs_idle_scb,data)
	TOCE(CS_xchng_thread,entry)
	TOCE(CS_eradicate,entry)
#endif
