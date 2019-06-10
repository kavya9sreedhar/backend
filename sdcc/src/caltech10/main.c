/*-------------------------------------------------------------------------
  main.h - hc08 specific general function

  Copyright (C) 2003, Erik Petrich

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
-------------------------------------------------------------------------*/
/*
    Note that mlh prepended _hc08_ on the static functions.  Makes
    it easier to set a breakpoint using the debugger.
*/
#include "common.h"
#include "hc08.h"
#include "main.h"
#include "ralloc.h"
#include "gen.h"
#include "dbuf_string.h"

extern char * iComments2;
extern DEBUGFILE dwarf2DebugFile;
extern int dwarf2FinalizeFile(FILE *);

static char _hc08_defaultRules[] =
{
#include "peeph.rul"
};

static char _s08_defaultRules[] =
{
#include "peeph.rul"
};

HC08_OPTS hc08_opts;

typedef struct
{
  /** Unique id for this target */
  const int id;
  /** Target name used for -m */
  const char *const target;

  /** Target name string, used for --help */
  const char *const target_name;

  /** Specific processor for the given target family, specified by -p */
  char *processor;

  struct
  {
    /** Pointer to glue function */
    void (*do_glue) (void);
    /** TRUE if all types of glue functions should be inserted into the file that
        also defines main.
        We don't want this in cases like the z80 where the startup code is provided
        by a seperate moule.
    */

    bool glue_up_main;
    /* OR of MODEL_* */
    int supported_models;
    int default_model;
    /** return the model string, used as library destination;
        port->target is used as model string if get_model is NULL */
    const char *(*get_model) (void);
  }
  general;

  /* assembler related information */
  struct
  {
    /** Command to run and arguments (eg as-z80) */
    const char **cmd;
    /** alternate macro based form */
    const char *mcmd;
    /** arguments for debug mode */
    const char *debug_opts;
    /** arguments for normal assembly mode */
    const char *plain_opts;
    /* print externs as global */
    int externGlobal;
    /* assembler file extension */
    const char *file_ext;
    /** if non-null will be used to execute the assembler */
    void (*do_assemble) (set *);
  }
  assembler;


/* linked related info */
struct
{
  /** command to run (eg link-z80) */
  const char **cmd;
  /** alternate macro based form */
  const char *mcmd;
  /** if non-null will be used to execute the link */
  void (*do_link) (void);
  /** extension for object files (.rel, .obj, ...) */
  const char *rel_ext;
  /** 1 if port needs the .lnk file, 0 otherwise */
  const int needLinkerScript;
  const char *const *crt;
  const char *const *libs;
}
linker;

/** default peephole rules */
struct
{
  char *default_rules;
  int (*getSize) (lineNode * line);
  bitVect *(*getRegsRead) (lineNode * line);
  bitVect *(*getRegsWritten) (lineNode * line);
  bool (*deadMove) (const char *reg, lineNode * currpl, lineNode * head);
  bool (*notUsed) (const char *reg, lineNode * currPl, lineNode * head);
  bool (*canAssign) (const char *op1, const char *op2, const char *op3);
  bool (*notUsedFrom) (const char *reg, const char *label, lineNode *head);
  bool (*symmParmStack) (void);
}
peep;

/** Basic type sizes */
struct
{
  int char_size;
  int short_size;
  int int_size;
  int long_size;
  int longlong_size;
  int near_ptr_size;
  int far_ptr_size;
  int ptr_size;
  int funcptr_size;
  int banked_funcptr_size;
  int bit_size;
  int float_size;
}
s;

// tags for far, near, xstack, code generic pointers
struct
{
    int tag_far;
    int tag_near;
    int tag_xstack;
    int tag_code;
}
gp_tags;

// memory regions related stuff
struct
{
    const char *const xstack_name;
    const char *const istack_name;
    const char *code_name;
    const char *data_name;
    /* i think the following should work instead? unclear
    const char *const code_name;
    const cahr *const data_name; */
    const char *const idata_name;
    const char *const pdata_name;
    const char *const xdata_name;
    const char *const bit_name;
    const char *const reg_name;
    const char *const static_name;
    const char *const overlay_name;
    const char *const post_static_name;
    const char *const home_name;
    const char *const xidata_name;
    const char *const xinit_name;
    const char *const const_name;
    const char *const cabs_name;
    const char *const xabs_name;
    const char *const iabs_name;
    const char *const initialized_name;
    const char *const initializer_name;

    struct memmap *default_local_map;
    struct memmap *default_globl_map;
    int code_ro;
    unsigned int maxextalign;
}
mem;

struct
{
    void (*genExtraAreaDeclaration) (FILE *, bool);
    void (*genExtraAreaLinkOptions) (FILE *);
}
extraAreas;

/* stack related information */
struct
{
    /** -1 for grows down (z80), +1 for grows up (mcs51) */
    int direction;
    /** Extra overhead when calling between banks */
    int bank_overhead;
    /** Extra overhead when the function is an ISR */
    int isr_overhead;
    /** Standard overhead for a function call */
    int call_overhead;
    /** Re-enterant space */
    int reent_overhead;
    /** 'banked' call overhead.
    Mild overlap with bank_overhead */
    int banked_overhead;
    /** 0 if sp points to last item pushed, 1 if sp points to next location to use*/
    int offset;
}
stack;

struct
{
    /** Size of the biggest shift the port can handle. -1 if port can handle
    shifts of arbitrary size. */
    signed int shift;
    /* Has support routines for int x int -> long multiplication and unsigned int
    x unsigned int -> unsigned long multiplication */
    bool has_mulint2long;
}
support;

struct
{
    void (*emitDebuggerSymbol) (const char *);
    struct
    {
        int (*regNum) (const struct reg_info *);
        bitVect *cfiSame;
        bitVect *cfiUndef;
        int addressSize;
        int regNumRet;
        int regNumSP;
        int regNumBP;
        int offsetSP;
    }
    dwarf;
}
debugger;

struct
 {
     int maxCount;
     int sizeofElement;
     int sizeofMatchJump[3];
     int sizeofRangeCompare[3];
     int sizeofSubtract;
     int sizeofDispatch;
 }
 jumptableCost;

 /** Prefix to add to a C function (eg "_") */
 const char *fun_prefix;

 /** Called once the processor target has been selected.
 First chance to initalise and set any port specific variables.
 'port' is set before calling this. May be NULL.
 */
 void (*init) (void);

 /** Parses one option + its arguments */
 bool (*parseOption) (int *pargc, char **argv, int *i);

 /** Optional list of automatically parsed options. Should be
 implemented to at least show the help text correctly. */
 OPTION *poptions;

 /** Initialise port spectific paths */
 void (*initPaths) (void);

 /** Called after all the options have been parsed. */
 void (*finaliseOptions) (void);

 /** Called after the port has been selected but before any
 options are parsed. */
 void (*setDefaultOptions) (void);

 /** Does the dirty work. */
 void (*assignRegisters) (struct ebbIndex *);

 /** Returns the register name of a symbol.
 Used so that 'reg_info' can be an incomplete type. */
 const char *(*getRegName) (const struct reg_info *reg);

 int (*getRegByName) (const char *name);

 /** Try to keep track of register contents. */
 bool (*rtrackUpdate)(const char* line);

 /* list of keywords that are used by this
 target (used by lexer) */
 char **keywords;

 /* Write any port specific assembler output. */
 void (*genAssemblerPreamble) (FILE * of);

 /* invoked at end assembler file */
 void (*genAssemblerEnd) (FILE * of);

 /* Write the port specific IVT. If genIVT is NULL or if
 * it returns zero, default (8051) IVT generation code
 * will be used.
 */
 int (*genIVT) (struct dbuf_s * oBuf, symbol ** intTable, int intCount);
 void (*genXINIT) (FILE * of);

 /* Write port specific startup code */
 void (*genInitStartup) (FILE * of);

 /* parameter passing in register related functions */
 void (*reset_regparms) (struct sym_link *); /* reset the register count*/

 int (*reg_parm) (struct sym_link *, bool reentrant); /* will return 1 if can be
passed in register */

 /** Process the pragma string 'sz'. Returns 0 if recognised and
 processed, 1 otherwise. May be NULL.
 */
 int (*process_pragma) (const char *sz);

 /** Mangles a support function name to reflect the calling model.
 */
 const char *(*getMangledFunctionName) (const char *szOrginial);

 /** Returns true if the port can multiply the two types nativly
 without using support functions.
 */
 bool (*hasNativeMulFor) (iCode *ic, sym_link *left, sym_link *right);

 /** Returns true if the port has implemented certain bit
 manipulation iCodes (RRC, RLC, SWAP, GETHBIT, GETABIT, GETBYTE, GETWORD)
 */
 bool (*hasExtBitOp) (int op, int size);

 /** Returns the relative expense of accessing a particular output
 storage class. Larger values indicate higher expense.
 */
 int (*oclsExpense) (struct memmap * oclass);

 /** If TRUE, then tprintf and !dw will be used for some initalisers
 */
 bool use_dw_for_init;

 /** TRUE for targets with little endian byte ordering, FALSE for
 targets with big endian byte ordering.
 */
 bool little_endian;

 /* condition transformations */
 bool lt_nge; /* transform (a < b) to !(a >= b) */
 bool gt_nle; /* transform (a > b) to !(a <= b) */
 bool le_ngt; /* transform (a <= b) to !(a > b) */
 bool ge_nlt; /* transform (a >= b) to !(a < b) */
 bool ne_neq; /* transform a != b --> ! (a == b) */
 bool eq_nne; /* transform a == b --> ! (a != b) */
 bool arrayInitializerSuppported;
 bool (*cseOk) (iCode * ic, iCode * pdic);
 builtins *builtintable; /* table of builtin functions */
 int unqualified_pointer; /* unqualified pointers type is */
 int reset_labelKey; /* reset Label no 1 at the start of a function */
 int globals_allowed; /* global & static locals not allowed ? 0 ONLY TININative */
 int num_regs; /* Number of registers handled in the treedecomposition-based register allocator in SDCCralloc.hpp */
 #define PORT_MAGIC 0xAC32
  /** Used at runtime to detect if this structure has been completely filled in.*/
  int magic;
 }
 PORT_;


/* list of key words used by msc51 */
static char *_hc08_keywords[] =
{
  "at",
  //"bit",
  "code",
  "critical",
  "data",
  "far",
  //"idata",
  "interrupt",
  "near",
  //"pdata",
  "reentrant",
  //"sfr",
  //"sbit",
  //"using",
  "xdata",
  "_data",
  "_code",
  "_generic",
  "_near",
  "_xdata",
  //"_pdata",
  //"_idata",
  "_naked",
  "_overlay",
  NULL
};

void hc08_assignRegisters (ebbIndex *);

static int regParmFlg = 0;      /* determine if we can register a parameter */

static void
_hc08_init (void)
{
  hc08_opts.sub = SUB_HC08;
  asm_addTree (&asm_asxxxx_mapping);
}

static void
_s08_init (void)
{
  hc08_opts.sub = SUB_S08;
  asm_addTree (&asm_asxxxx_mapping);
}

static void
_hc08_reset_regparm (struct sym_link *funcType)
{
  regParmFlg = 0;
}

static int
_hc08_regparm (sym_link * l, bool reentrant)
{
  int size = getSize(l);

  /* If they fit completely, the first two bytes of parameters can go */
  /* into A and X, otherwise, they go on the stack. Examples:         */
  /*   foo(char p1)                    A <- p1                        */
  /*   foo(char p1, char p2)           A <- p1, X <- p2               */
  /*   foo(char p1, char p2, char p3)  A <- p1, X <- p2, stack <- p3  */
  /*   foo(int p1)                     XA <- p1                       */
  /*   foo(long p1)                    stack <- p1                    */
  /*   foo(char p1, int p2)            A <- p1, stack <- p2           */
  /*   foo(int p1, char p2)            XA <- p1, stack <- p2          */

  if (regParmFlg>=2)
    return 0;

  if ((regParmFlg+size)>2)
    {
      regParmFlg = 2;
      return 0;
    }

  regParmFlg += size;
  return 1+regParmFlg-size;
}

static bool
_hc08_parseOptions (int *pargc, char **argv, int *i)
{
  if (!strcmp (argv[*i], "--out-fmt-elf"))
    {
      options.out_fmt = 'E';
      debugFile = &dwarf2DebugFile;
      return TRUE;
    }

  if (!strcmp (argv[*i], "--oldralloc"))
    {
      options.oldralloc = TRUE;
      return TRUE;
    }

  return FALSE;
}

#define OPTION_SMALL_MODEL          "--model-small"
#define OPTION_LARGE_MODEL          "--model-large"

static OPTION _hc08_options[] =
  {
    {0, OPTION_SMALL_MODEL, NULL, "8-bit address space for data"},
    {0, OPTION_LARGE_MODEL, NULL, "16-bit address space for data (default)"},
    {0, "--out-fmt-elf", NULL, "Output executable in ELF format" },
    {0, "--oldralloc", NULL, "Use old register allocator"},
    {0, NULL }
  };

static void
_hc08_finaliseOptions (void)
{
  if (options.noXinitOpt)
    port->genXINIT = 0;

  if (options.model == MODEL_LARGE) {
      port->mem.default_local_map = xdata;
      port->mem.default_globl_map = xdata;
    }
  else
    {
      port->mem.default_local_map = data;
      port->mem.default_globl_map = data;
    }

  istack->ptrType = FPOINTER;
}

static void
_hc08_setDefaultOptions (void)
{
  options.code_loc = 0x8000;
  options.data_loc = 0x80;
  options.xdata_loc = 0;        /* 0 means immediately following data */
  options.stack_loc = 0x7fff;
  options.out_fmt = 's';        /* use motorola S19 output */

  options.omitFramePtr = 1;     /* no frame pointer (we use SP */
                                /* offsets instead)            */
}

static const char *
_hc08_getRegName (const struct reg_info *reg)
{
  if (reg)
    return reg->name;
  return "err";
}

static void
_hc08_genAssemblerPreamble (FILE * of)
{
  int i;
  int needOrg = 1;
  symbol *mainExists=newSymbol("main", 0);
  mainExists->block=0;

  fprintf (of, "\t.area %s\n",HOME_NAME);
  fprintf (of, "\t.area GSINIT0 (CODE)\n");
  fprintf (of, "\t.area %s\n",port->mem.static_name);
  fprintf (of, "\t.area %s\n",port->mem.post_static_name);
  fprintf (of, "\t.area %s\n",CODE_NAME);
  fprintf (of, "\t.area %s\n",port->mem.xinit_name);
  fprintf (of, "\t.area %s\n",port->mem.const_name);
  fprintf (of, "\t.area %s\n",port->mem.data_name);
  fprintf (of, "\t.area %s\n",port->mem.overlay_name);
  fprintf (of, "\t.area %s\n",port->mem.xdata_name);
  fprintf (of, "\t.area %s\n",port->mem.xidata_name);

  if ((mainExists=findSymWithLevel(SymbolTab, mainExists)))
    {
      // generate interrupt vector table
      fprintf (of, "\t.area\tCODEIVT (ABS)\n");

      for (i=maxInterrupts;i>0;i--)
        {
          if (interrupts[i])
            {
              if (needOrg)
                {
                  fprintf (of, "\t.org\t0x%04x\n", (0xfffe - (i * 2)));
                  needOrg = 0;
                }
              fprintf (of, "\t.dw\t%s\n", interrupts[i]->rname);
            }
          else
            needOrg = 1;
        }
      if (needOrg)
        fprintf (of, "\t.org\t0xfffe\n");
      fprintf (of, "\t.dw\t%s", "__sdcc_gs_init_startup\n\n");

      fprintf (of, "\t.area GSINIT0\n");
      fprintf (of, "__sdcc_gs_init_startup:\n");
      if (options.stack_loc)
        {
          fprintf (of, "\tldhx\t#0x%04x\n", options.stack_loc+1);
          fprintf (of, "\ttxs\n");
        }
      else
        fprintf (of, "\trsp\n");
      fprintf (of, "\tjsr\t__sdcc_external_startup\n");
      fprintf (of, "\tbeq\t__sdcc_init_data\n");
      fprintf (of, "\tjmp\t__sdcc_program_startup\n");
      fprintf (of, "__sdcc_init_data:\n");

      fprintf (of, "; _hc08_genXINIT() start\n");
      fprintf (of, "        ldhx #0\n");
      fprintf (of, "00001$:\n");
      fprintf (of, "        cphx #l_XINIT\n");
      fprintf (of, "        beq  00002$\n");
      fprintf (of, "        lda  s_XINIT,x\n");
      fprintf (of, "        sta  s_XISEG,x\n");
      fprintf (of, "        aix  #1\n");
      fprintf (of, "        bra  00001$\n");
      fprintf (of, "00002$:\n");
      fprintf (of, "; _hc08_genXINIT() end\n");

      fprintf (of, "\t.area GSFINAL\n");
      fprintf (of, "\tjmp\t__sdcc_program_startup\n\n");

      fprintf (of, "\t.area CSEG\n");
      fprintf (of, "__sdcc_program_startup:\n");
      fprintf (of, "\tjsr\t_main\n");
      fprintf (of, "\tbra\t.\n");

    }
}

static void
_hc08_genAssemblerEnd (FILE * of)
{
  if (options.out_fmt == 'E' && options.debug)
    {
      dwarf2FinalizeFile (of);
    }
}

static void
_hc08_genExtraAreas (FILE * asmFile, bool mainExists)
{
    fprintf (asmFile, "%s", iComments2);
    fprintf (asmFile, "; extended address mode data\n");
    fprintf (asmFile, "%s", iComments2);
    dbuf_write_and_destroy (&xdata->oBuf, asmFile);
}

/* Generate interrupt vector table. */
static int
_hc08_genIVT (struct dbuf_s * oBuf, symbol ** interrupts, int maxInterrupts)
{
  int i;

  dbuf_printf (oBuf, "\t.area\tCODEIVT (ABS)\n");
  dbuf_printf (oBuf, "\t.org\t0x%04x\n",
    (0xfffe - (maxInterrupts * 2)));

  for (i=maxInterrupts;i>0;i--)
    {
      if (interrupts[i])
        dbuf_printf (oBuf, "\t.dw\t%s\n", interrupts[i]->rname);
      else
        dbuf_printf (oBuf, "\t.dw\t0xffff\n");
    }
  dbuf_printf (oBuf, "\t.dw\t%s", "__sdcc_gs_init_startup\n");

  return TRUE;
}

/* Generate code to copy XINIT to XISEG */
static void _hc08_genXINIT (FILE * of) {
  fprintf (of, ";       _hc08_genXINIT() start\n");
  fprintf (of, ";       _hc08_genXINIT() end\n");
}


/* Do CSE estimation */
static bool cseCostEstimation (iCode *ic, iCode *pdic)
{
    operand *result = IC_RESULT(ic);
    sym_link *result_type = operandType(result);

    /* if it is a pointer then return ok for now */
    if (IC_RESULT(ic) && IS_PTR(result_type)) return 1;

    if (ic->op == ADDRESS_OF)
      return 0;

    /* if bitwise | add & subtract then no since hc08 is pretty good at it
       so we will cse only if they are local (i.e. both ic & pdic belong to
       the same basic block */
    if (IS_BITWISE_OP(ic) || ic->op == '+' || ic->op == '-') {
        /* then if they are the same Basic block then ok */
        if (ic->eBBlockNum == pdic->eBBlockNum) return 1;
        else return 0;
    }

    /* for others it is cheaper to do the cse */
    return 1;
}

/* Indicate which extended bit operations this port supports */
static bool
hasExtBitOp (int op, int size)
{
  if (op == RRC
      || op == RLC
      || (op == SWAP && size <= 2)
      || op == GETABIT
      || op == GETBYTE
      || op == GETWORD
     )
    return TRUE;
  else
    return FALSE;
}

/* Indicate the expense of an access to an output storage class */
static int
oclsExpense (struct memmap *oclass)
{
  /* The hc08's addressing modes allow access to all storage classes */
  /* inexpensively (<=0) */

  if (IN_DIRSPACE (oclass))     /* direct addressing mode is fastest */
    return -2;
  if (IN_FARSPACE (oclass))     /* extended addressing mode is almost at fast */
    return -1;
  if (oclass == istack)         /* stack is the slowest, but still faster than */
    return 0;                   /* trying to copy to a temp location elsewhere */

  return 0; /* anything we missed */
}

/*----------------------------------------------------------------------*/
/* hc08_dwarfRegNum - return the DWARF register number for a register.  */
/*   These are defined for the HC08 in "Motorola 8- and 16-bit Embedded */
/*   Application Binary Interface (M8/16EABI)"                          */
/*----------------------------------------------------------------------*/
static int
hc08_dwarfRegNum (const struct reg_info *reg)
{
  switch (reg->rIdx)
    {
    case A_IDX: return 0;
    case H_IDX: return 1;
    case X_IDX: return 2;
    case CND_IDX: return 17;
    case SP_IDX: return 15;
    }
  return -1;
}

static bool
_hasNativeMulFor (iCode *ic, sym_link *left, sym_link *right)
{
  return getSize (left) == 1 && getSize (right) == 1;
}

typedef struct asmLineNode
  {
    int size;
  }
asmLineNode;

static asmLineNode *
newAsmLineNode (void)
{
  asmLineNode *aln;

  aln = Safe_alloc ( sizeof (asmLineNode));
  aln->size = 0;

  return aln;
}

typedef struct hc08opcodedata
  {
    char name[6];
    char adrmode;
    /* info for registers used and/or modified by an instruction will be added here */
  }
hc08opcodedata;

#define HC08OP_STD 1
#define HC08OP_RMW 2
#define HC08OP_INH 3
#define HC08OP_IM1 4
#define HC08OP_BR 5
#define HC08OP_BTB 6
#define HC08OP_BSC 7
#define HC08OP_MOV 8
#define HC08OP_CBEQ 9
#define HC08OP_CPHX 10
#define HC08OP_LDHX 11
#define HC08OP_STHX 12
#define HC08OP_DBNZ 13

/* These must be kept sorted by opcode name */
static hc08opcodedata hc08opcodeDataTable[] =
  {
    {".db",   HC08OP_INH}, /* used by the code generator only in the jump table */
      // do we need this?
    {"adc",   HC08OP_STD},
    {"adci", },
    {"add",   HC08OP_STD},
    {"addi"},
    {"and"},
    {"andi"},
    {"asr",   HC08OP_RMW},
    {"cmp",   HC08OP_STD},
    {"cmpi"},
    {"dec",   HC08OP_RMW},
    {"inc",   HC08OP_RMW},
    {"lsl",   HC08OP_RMW},
    {"lsr",   HC08OP_RMW},
    {"neg",   HC08OP_RMW},
    {"not"},
    {"or"},
    {"ori"},
    {"rlc"},
    {"rol"},
    {"ror"},
    {"rrc"},
    {"sbb"},
    {"sbbi"},
    {"sub"},
    {"subi"},
    {"tst"},
    {"tsti"},
    {"xor"},
    {"xori"},
    {"sti"},
    {"cli"},
    {"stu"},
    {"clu"},
    {"stc"},
    {"clc"},
    {"tax"},
    {"txa"},
    {"inx"},
    {"dex"},
    {"tas"},
    {"tsa"},
    {"ins"},
    {"des"},
    {"ldi"},
    {"ldd"},
    {"ld"},
    {"std"},
    {"st"},
    {"jmp"},
    {"ja"},
    {"jae"},
    {"jb"},
    {"jbe"},
    {"je"},
    {"jg"},
    {"jge"},
    {"jl"},
    {"jle"},
    {"jne"},
    {"jns"},
    {"jnu"},
    {"jnv"},
    {"js"},
    {"ju"},
    {"jv"},
    {"call"},
    {"rts"},
    {"popf"},
    {"pushf"},
    {"in"},
    {"out"},
    {"nop"}
  };

static int
hc08_opcodeCompare (const void *key, const void *member)
{
  return strcmp((const char *)key, ((hc08opcodedata *)member)->name);
}

/*--------------------------------------------------------------------*/
/* Given an instruction and its first two operands, compute the       */
/* instruction size. There are a few cases where it's too complicated */
/* to distinguish between an 8-bit offset and 16-bit offset; in these */
/* cases we conservatively assume the 16-bit offset size.             */
/*--------------------------------------------------------------------*/
static int
hc08_instructionSize(const char *inst, const char *op1, const char *op2)
{
  hc08opcodedata *opcode;
  int size;
  long offset;
  char * endnum = NULL;

  opcode = bsearch (inst, hc08opcodeDataTable,
                    sizeof(hc08opcodeDataTable)/sizeof(hc08opcodedata),
                    sizeof(hc08opcodedata), hc08_opcodeCompare);

  if (!opcode)
    return 999;
  switch (opcode->adrmode)
    {
      case HC08OP_INH: /* Inherent addressing mode */
        return 1;

      case HC08OP_BSC: /* Bit set/clear direct addressing mode */
      case HC08OP_BR:  /* Branch (1 byte signed offset) */
      case HC08OP_IM1: /* 1 byte immediate addressing mode */
        return 2;

      case HC08OP_BTB:  /* Bit test direct addressing mode and branch */
        return 3;

      case HC08OP_RMW: /* read/modify/write instructions */
        if (!op2[0]) /* if not ,x or ,sp must be direct addressing mode */
          return 2;
        if (!op1[0])  /* if ,x with no offset */
          return 1;
        if (op2[0] == 'x')  /* if ,x with offset */
          return 2;
        return 3;  /* Otherwise, must be ,sp with offset */

      case HC08OP_STD: /* standard instruction */
        if (!op2[0])
          {
            if (op1[0] == '#') /* Immediate addressing mode */
              return 2;
            if (op1[0] == '*') /* Direct addressing mode */
              return 2;
            return 3; /* Otherwise, must be extended addressing mode */
          }
        else
          {
            if (!op1[0]) /* if ,x with no offset */
              return 1;
            size = 2;
            if (op2[0] == 's')
              size++;
            offset = strtol (op1, &endnum, 0) & 0xffff;
            if (endnum && *endnum)
              size++;
            else if (offset > 0xff)
              size++;
            return size;
          }
      case HC08OP_MOV:
        if (op2[0] == 'x')
          return 2;
        return 3;
      case HC08OP_CBEQ:
        if (op2[0] == 'x' && !op1[0])
          return 2;  /* cbeq ,x+,rel */
        if (op2[0] == 's')
          return 4;  /* cbeq oprx8,sp,rel */
        return 3;
      case HC08OP_CPHX:
        if (op1[0] == '*')
          return 2;
        return 3;
      case HC08OP_DBNZ:
        if (!op2[0])
          return 2;
        if (!op1[0] && op2[0] == 'x')
          return 2;
        if (op2[0] == 's')
          return 4;
        return 3;
      case HC08OP_LDHX:
      case HC08OP_STHX:
        if (op1[0] == '*')
          return 2;
        if (!op1[0] && op2[0] == 'x')
          return 2;
        if (op2[0] == 's' || op1[0] == '#' || !op2[0])
          return 3;
        size = 3;
        offset = strtol (op1, &endnum, 0) & 0xffff;
        if (endnum && *endnum)
          size++;
        else if (offset > 0xff)
          size++;
        return size;
      default:
        return 4;
    }
}


static asmLineNode *
hc08_asmLineNodeFromLineNode (lineNode *ln)
{
  asmLineNode *aln = newAsmLineNode();
  char *op, op1[256], op2[256];
  int opsize;
  const char *p;
  char inst[8];

  p = ln->line;

  while (*p && isspace(*p)) p++;
  for (op = inst, opsize=1; *p; p++)
    {
      if (isspace(*p) || *p == ';' || *p == ':' || *p == '=')
        break;
      else
        if (opsize < sizeof(inst))
          *op++ = tolower(*p), opsize++;
    }
  *op = '\0';

  if (*p == ';' || *p == ':' || *p == '=')
    return aln;

  while (*p && isspace(*p)) p++;
  if (*p == '=')
    return aln;

  for (op = op1, opsize=1; *p && *p != ','; p++)
    {
      if (!isspace(*p) && opsize < sizeof(op1))
        *op++ = tolower(*p), opsize++;
    }
  *op = '\0';

  if (*p == ',') p++;
  for (op = op2, opsize=1; *p && *p != ','; p++)
    {
      if (!isspace(*p) && opsize < sizeof(op2))
        *op++ = tolower(*p), opsize++;
    }
  *op = '\0';

  aln->size = hc08_instructionSize(inst, op1, op2);

  return aln;
}

static int
hc08_getInstructionSize (lineNode *line)
{
  if (!line->aln)
    line->aln = (asmLineNodeBase *) hc08_asmLineNodeFromLineNode (line);

  return line->aln->size;
}

/** $1 is always the basename.
    $2 is always the output file.
    $3 varies
    $l is the list of extra options that should be there somewhere...
    MUST be terminated with a NULL.
*/
static const char *_linkCmd[] =
{
  "sdld6808", "-nf", "$1", NULL
};

/* $3 is replaced by assembler.debug_opts resp. port->assembler.plain_opts */
static const char *_asmCmd[] =
{
  "sdas6808", "$l", "$3", "$2", "$1.asm", NULL
};

static const char * const _libs_hc08[] = { "hc08", NULL, };
static const char * const _libs_s08[] = { "s08", NULL, };

/* Globals */
PORT caltech10_port_ =
{
  TARGET_ID_CALTECH10,
  "CALTECH10",
  "CALTECH10",
  NULL,                         /* Processor name */
  {
    glue,
    FALSE,                      /* Emit glue around main */
    MODEL_SMALL | MODEL_LARGE,
    MODEL_SMALL,
    NULL,                       /* model == target */
  },
  {
    _asmCmd,
    NULL,
    "-plosgffwy",               /* Options with debug */
    "-plosgffw",                /* Options without debug */
    0,
    ".asm",
    NULL                        /* no do_assemble function */
  },
  {                             /* Linker */
    _linkCmd,
    NULL,
    NULL,
    ".rel",
    1,
    NULL,                       /* crt */
    _libs_hc08,                 /* libs */
  },
  {                             /* Peephole optimizer */
    _hc08_defaultRules,
    hc08_getInstructionSize,
  },
  {
    /* Sizes: char, short, int, long, long long, near ptr, far ptr, gptr, func ptr, banked func ptr, bit, float */
    1, 2, 2, 4, 8, 2, 2, 2, 2, 0, 1, 4
  },
  /* tags for generic pointers */
  { 0x00, 0x00, 0x00, 0x00 },           /* far, near, xstack, code */
  {
    "XSEG",
    "STACK",
    "CSEG    (CODE)",
    "DSEG    (PAG)",
    NULL, /* "ISEG" */
    NULL, /* "PSEG" */
    "XSEG",
    NULL, /* "BSEG" */
    "RSEG    (ABS)",
    "GSINIT  (CODE)",
    "OSEG    (PAG, OVR)",
    "GSFINAL (CODE)",
    "HOME    (CODE)",
    "XISEG",              // initialized xdata
    "XINIT   (CODE)",     // a code copy of xiseg
    "CONST   (CODE)",     // const_name - const data (code or not)
    "CABS    (ABS,CODE)", // cabs_name - const absolute data (code or not)
    "XABS    (ABS)",      // xabs_name - absolute xdata
    "IABS    (ABS)",      // iabs_name - absolute data
    NULL,                 // name of segment for initialized variables
    NULL,                 // name of segment for copies of initialized variables in code space
    NULL,
    NULL,
    1,
    1                     // No fancy alignments supported.
  },
  { _hc08_genExtraAreas,
    NULL },
  {
    -1,         /* direction (-1 = stack grows down) */
    0,          /* bank_overhead (switch between register banks) */
    4,          /* isr_overhead */
    2,          /* call_overhead */
    0,          /* reent_overhead */
    0,          /* banked_overhead (switch between code banks) */
    1           /* sp is offset by 1 from last item pushed */
  },
  {
    5, FALSE
  },
  {
    hc08_emitDebuggerSymbol,
    {
      hc08_dwarfRegNum,
      NULL,
      NULL,
      4,                        /* addressSize */
      14,                       /* regNumRet */
      15,                       /* regNumSP */
      -1,                       /* regNumBP */
      1,                        /* offsetSP */
    },
  },
  {
    256,        /* maxCount */
    2,          /* sizeofElement */
    {8,16,32},  /* sizeofMatchJump[] */
    {8,16,32},  /* sizeofRangeCompare[] */
    5,          /* sizeofSubtract */
    10,         /* sizeofDispatch */
  },
  "_",
  _hc08_init,
  _hc08_parseOptions,
  _hc08_options,
  NULL,
  _hc08_finaliseOptions,
  _hc08_setDefaultOptions,
  hc08_assignRegisters,
  _hc08_getRegName,
  0,
  NULL,
  _hc08_keywords,
  _hc08_genAssemblerPreamble,
  _hc08_genAssemblerEnd,        /* no genAssemblerEnd */
  _hc08_genIVT,
  _hc08_genXINIT,
  NULL,                         /* genInitStartup */
  _hc08_reset_regparm,
  _hc08_regparm,
  NULL,                         /* process_pragma */
  NULL,                         /* getMangledFunctionName */
  _hasNativeMulFor,             /* hasNativeMulFor */
  hasExtBitOp,                  /* hasExtBitOp */
  oclsExpense,                  /* oclsExpense */
  TRUE,                         /* use_dw_for_init */
  FALSE,                        /* little_endian */
  0,                            /* leave lt */
  0,                            /* leave gt */
  1,                            /* transform <= to ! > */
  1,                            /* transform >= to ! < */
  1,                            /* transform != to !(a == b) */
  0,                            /* leave == */
  FALSE,                        /* No array initializer support. */
  cseCostEstimation,
  NULL,                         /* no builtin functions */
  GPOINTER,                     /* treat unqualified pointers as "generic" pointers */
  1,                            /* reset labelKey to 1 */
  1,                            /* globals & local statics allowed */
  3,                            /* Number of registers handled in the tree-decomposition-based register allocator in SDCCralloc.hpp */
  PORT_MAGIC
};

PORT s08_port =
{
  TARGET_ID_S08,
  "s08",
  "S08",                        /* Target name */
  NULL,                         /* Processor name */
  {
    glue,
    FALSE,                      /* Emit glue around main */
    MODEL_SMALL | MODEL_LARGE,
    MODEL_LARGE,
    NULL,                       /* model == target */
  },
  {
    _asmCmd,
    NULL,
    "-plosgffwy",               /* Options with debug */
    "-plosgffw",                /* Options without debug */
    0,
    ".asm",
    NULL                        /* no do_assemble function */
  },
  {                             /* Linker */
    _linkCmd,
    NULL,
    NULL,
    ".rel",
    1,
    NULL,                       /* crt */
    _libs_s08,                  /* libs */
  },
  {                             /* Peephole optimizer */
    _s08_defaultRules,
    hc08_getInstructionSize,
  },
  {
    /* Sizes: char, short, int, long, long long, near ptr, far ptr, gptr, func ptr, banked func ptr, bit, float */
    1, 2, 2, 4, 8, 2, 2, 2, 2, 0, 1, 4
  },
  /* tags for generic pointers */
  { 0x00, 0x00, 0x00, 0x00 },           /* far, near, xstack, code */
  {
    "XSEG",
    "STACK",
    "CSEG    (CODE)",
    "DSEG    (PAG)",
    NULL, /* "ISEG" */
    NULL, /* "PSEG" */
    "XSEG",
    NULL, /* "BSEG" */
    "RSEG    (ABS)",
    "GSINIT  (CODE)",
    "OSEG    (PAG, OVR)",
    "GSFINAL (CODE)",
    "HOME    (CODE)",
    "XISEG",              // initialized xdata
    "XINIT   (CODE)",     // a code copy of xiseg
    "CONST   (CODE)",     // const_name - const data (code or not)
    "CABS    (ABS,CODE)", // cabs_name - const absolute data (code or not)
    "XABS    (ABS)",      // xabs_name - absolute xdata
    "IABS    (ABS)",      // iabs_name - absolute data
    NULL,                 // name of segment for initialized variables
    NULL,                 // name of segment for copies of initialized variables in code space
    NULL,
    NULL,
    1,
    1                     // No fancy alignments supported.
  },
  { _hc08_genExtraAreas,
    NULL },
  {
    -1,         /* direction (-1 = stack grows down) */
    0,          /* bank_overhead (switch between register banks) */
    4,          /* isr_overhead */
    2,          /* call_overhead */
    0,          /* reent_overhead */
    0,          /* banked_overhead (switch between code banks) */
    1           /* sp is offset by 1 from last item pushed */
  },
  {
    5, FALSE
  },
  {
    hc08_emitDebuggerSymbol,
    {
      hc08_dwarfRegNum,
      NULL,
      NULL,
      4,                        /* addressSize */
      14,                       /* regNumRet */
      15,                       /* regNumSP */
      -1,                       /* regNumBP */
      1,                        /* offsetSP */
    },
  },
  {
    256,        /* maxCount */
    2,          /* sizeofElement */
    {8,16,32},  /* sizeofMatchJump[] */
    {8,16,32},  /* sizeofRangeCompare[] */
    5,          /* sizeofSubtract */
    10,         /* sizeofDispatch */
  },
  "_",
  _s08_init,
  _hc08_parseOptions,
  _hc08_options,
  NULL,
  _hc08_finaliseOptions,
  _hc08_setDefaultOptions,
  hc08_assignRegisters,
  _hc08_getRegName,
  0,
  NULL,
  _hc08_keywords,
  _hc08_genAssemblerPreamble,
  _hc08_genAssemblerEnd,        /* no genAssemblerEnd */
  _hc08_genIVT,
  _hc08_genXINIT,
  NULL,                         /* genInitStartup */
  _hc08_reset_regparm,
  _hc08_regparm,
  NULL,                         /* process_pragma */
  NULL,                         /* getMangledFunctionName */
  _hasNativeMulFor,             /* hasNativeMulFor */
  hasExtBitOp,                  /* hasExtBitOp */
  oclsExpense,                  /* oclsExpense */
  TRUE,                         /* use_dw_for_init */
  FALSE,                        /* little_endian */
  0,                            /* leave lt */
  0,                            /* leave gt */
  1,                            /* transform <= to ! > */
  1,                            /* transform >= to ! < */
  1,                            /* transform != to !(a == b) */
  0,                            /* leave == */
  FALSE,                        /* No array initializer support. */
  cseCostEstimation,
  NULL,                         /* no builtin functions */
  GPOINTER,                     /* treat unqualified pointers as "generic" pointers */
  1,                            /* reset labelKey to 1 */
  1,                            /* globals & local statics allowed */
  3,                            /* Number of registers handled in the tree-decomposition-based register allocator in SDCCralloc.hpp */
  PORT_MAGIC
};
