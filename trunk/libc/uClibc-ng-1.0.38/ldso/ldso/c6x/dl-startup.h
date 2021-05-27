/* Copyright (C) 2010 Texas Instruments Incorporated
 * Contributed by Mark Salter <msalter@redhat.com>
 *
 * Borrowed heavily from frv arch:
 * Copyright (C) 2003 Red Hat, Inc.
 * 
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */
#undef DL_START
#define DL_START(X)   \
static void * __attribute_used__  \
_dl_start (unsigned placeholder, \
	   struct elf32_dsbt_loadmap *dl_boot_progmap, \
	   struct elf32_dsbt_loadmap *dl_boot_ldsomap, \
	   Elf32_Dyn *dl_boot_ldso_dyn_pointer,	       \
	   X)

/*
 * On entry, the kernel has set up the stack thusly:
 *
 *	0(sp)			pad0
 *	4(sp)			pad1
 *	8(sp)			argc
 *	12(sp)			argv[0]
 *	...
 *	(4*(argc+3))(sp)	NULL
 *	(4*(argc+4))(sp)	envp[0]
 *	...
 *				NULL
 *
 * Register values are unspecified, except:
 *
 *	B4  --> executable loadmap address
 *	A6  --> interpreter loadmap address
 *	B6  --> dynamic section address
 *
 * NB: DSBT index is always 0 for the executable
 *     and 1 for the interpreter
 */

__asm__("	.text\n"
	".globl _start\n"
	".hidden _start\n"
	"_start:\n"
	/* Find interpreter DSBT base in dynamic section */
	"	   MV .S2		B6,B2\n"
	" ||	   ADD .D1X		B6,4,A2\n"
	"          LDW .D2T2		*B2++[2],B0\n"
	" ||	   LDW .D1T1		*A2++[2],A0\n"
	"          MVKL .S2		" __stringify(DT_C6000_DSBT_BASE) ",B7\n"
	"          MVKH .S2		" __stringify(DT_C6000_DSBT_BASE) ",B7\n"
	"          NOP\n"
	"          NOP\n"
	/*
	 * B0 now holds dynamic tag and A0 holds tag value.
	 * Loop through looking for DSBT base tag
	 */
	"0:\n"
	" [B0]     CMPEQ .L2		B0,B7,B1\n"
	" || [!B0] MVK .S2		1,B1\n"
	" [!B1]	   BNOP .S1		0b,5\n"
	" ||[!B1]  LDW .D2T2		*B2++[2],B0\n"
	" ||[!B1]  LDW .D1T1		*A2++[2],A0\n"
	/*
	 * DSBT base in A0 needs to be relocated.
	 * Search through our loadmap to find where it got loaded.
	 *
	 * struct elf32_dsbt_loadmap {
	 *     Elf32_Half version;
	 *     Elf32_Half nsegs;
	 *     struct {
	 *         Elf32_Addr addr;
	 *         Elf32_Addr p_vaddr;
	 *         Elf32_Word p_memsz;
	 *     } segments[];
	 * }
	 *
	 */
	"          MV .S1		A6,A1\n"
	" [!A1]	   MV .S1X		B4,A1\n"
	"          ADD .D1		A1,2,A3\n"
	"          LDHU .D1T2		*A3++[1],B0\n"  /* nsegs */
	"          LDW .D1T1		*A3++[1],A10\n" /* addr */
	"          LDW .D1T1		*A3++[1],A11\n" /* p_vaddr */
	"          LDW .D1T1		*A3++[1],A12\n" /* p_memsz */
	"	   NOP\n"
	"	   NOP\n"
	/*
	 * Here we have:
	 *     B0  -> number of segments to search.
	 *     A3  -> pointer to next segment to check
	 *     A10 -> segment load address
	 *     A11 -> ELF segment virt address
	 *     A12 -> ELF segment size
	 */
	"0:\n"
	" [!B0]    B .S2                0f\n"
	" 	   SUB .D2              B0,1,B0\n"
	"	   CMPLTU .L1           A0,A11,A13\n"
	" ||	   SUB .S1              A12,1,A12\n"
	"	   ADD .D1              A11,A12,A12\n"
	"	   CMPGTU .L1           A0,A12,A14\n"
	"	   OR .L1               A13,A14,A2\n"
	" [A2]     B .S2                0b\n"
	" || [!A2] SUB .L1              A0,A11,A0\n"
	" [B0]     LDW .D1T1		*A3++[1],A10\n" /* addr */
	" || [!A2] ADD .L1              A0,A10,A0\n"
	" [B0]     LDW .D1T1		*A3++[1],A11\n" /* p_vaddr */
	" [B0]     LDW .D1T1		*A3++[1],A12\n" /* p_memsz */
	"          MV  .S2X		A0,B14\n"
	"	   NOP\n"
	"0:\n"
	"          B .S2		_dl_start\n"
	"          STW .D2T2		B14, *+B14[1]\n"
	"          ADD .D1X		B15,8,A8\n"
	"          ADDKPC .S2		ret_from_dl,B3,2\n"
	"ret_from_dl:\n"
	"	   B .S2X		A4\n"
	" ||       LDW .D2T2		*+B14[0],B14\n"
	" 	   ADDKPC .S2		__dl_fini,B0,0\n"
	"          MV .S1X		B0,A4\n"
	"	   NOP\n"
	"	   NOP\n"
	"	   NOP\n"
	"__dl_fini:\n"
	"          LDW .D2T2		*+B14[1],B14\n"
	"	   NOP		4\n"
	" 	   LDW .D2T1		*+B14($GOT(_dl_fini)), A0\n"
	"          NOP		4\n"
	"	   BNOP .S2X		A0, 5\n");

__asm__("	.text\n"
	"__c6x_cache_sync:\n"
	"	   MVK .S2		330,B0\n"
	"	   SWE\n"
	"	   NOP\n"
	"	   BNOP .S2		B3,5\n"
	"	   NOP\n"
	"	   NOP\n"
	"	   NOP\n"
	"	   NOP\n"
	"\n"
);

/*
 * Get a pointer to the argv array.  On many platforms this can be just
 * the address of the first argument, on other platforms we need to
 * do something a little more subtle here.
 */
#define GET_ARGV(ARGVP, ARGS) ARGVP = (((unsigned long*) ARGS) + 1)

struct elf32_dsbt_loadmap;

/*
 * Here is a macro to perform a relocation.  This is only used when
 * bootstrapping the dynamic loader.  RELP is the relocation that we
 * are performing, REL is the pointer to the address we are relocating.
 * SYMBOL is the symbol involved in the relocation, and LOAD is the
 * load address.
 */
#define PERFORM_BOOTSTRAP_RELOC(RELP,REL,SYMBOL,LOAD,SYMTAB) \
	switch(ELF_R_TYPE((RELP)->r_info)){				\
	case R_C6000_ABS_L16:						\
	    {								\
		    unsigned int opcode = *(REL);			\
		    unsigned int v = (SYMBOL) + (RELP)->r_addend;	\
		    opcode &= ~0x7fff80;				\
		    opcode |= ((v & 0xffff) << 7);			\
		    *(REL) = opcode;					\
	    }								\
	    break;							\
	case R_C6000_ABS_H16:						\
	    {								\
		    unsigned int opcode = *(REL);			\
		    unsigned int v = (SYMBOL) + (RELP)->r_addend;	\
		    opcode &= ~0x7fff80;				\
		    opcode |= ((v >> 9) & 0x7fff80);			\
		    *(REL) = opcode;					\
	    }								\
	    break;							\
	case R_C6000_ABS32:						\
	    *(REL) = (SYMBOL) + (RELP)->r_addend;			\
	    break;							\
	default:							\
	  _dl_exit(1);							\
	}

extern void __c6x_cache_sync(unsigned long start, unsigned long end)
  attribute_hidden;
