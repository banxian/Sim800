#include "65c02.h"

#define  BREAKPOINTS     5
#define  COMMANDLINES    5
#define  COMMANDS        39
#define  MAXARGS         40
#define  SOURCELINES     19
#define  STACKLINES      9
#define  WATCHES         5

#define  SCREENSPLIT1    356
#define  SCREENSPLIT2    456

#define  INVALID1        1
#define  INVALID2        2
#define  INVALID3        3
#define  ADDR_IMM        4
#define  ADDR_ABS        5
#define  ADDR_ZPG        6
#define  ADDR_ABSX       7
#define  ADDR_ABSY       8
#define  ADDR_ZPGX       9
#define  ADDR_ZPGY       10
#define  ADDR_REL        11
#define  ADDR_BBREL      12
#define  ADDR_INDX       13
#define  ADDR_ABSIINDX   14
#define  ADDR_INDY       15
#define  ADDR_IZPG       16
#define  ADDR_IABS       17

#define  COLOR_INSTBKG   0
#define  COLOR_INSTTEXT  1
#define  COLOR_INSTBP    2
#define  COLOR_DATABKG   3
#define  COLOR_DATATEXT  4
#define  COLOR_STATIC    5
#define  COLOR_BPDATA    6
#define  COLOR_COMMAND   7
#define  COLORS          8

typedef BOOL (*cmdfunction)(int);

BOOL CmdBreakpointAdd (int args);
BOOL CmdBreakpointClear (int args);
BOOL CmdBreakpointDisable (int args);
BOOL CmdBreakpointEnable (int args);
BOOL CmdCodeDump (int args);
BOOL CmdFeedKey (int args);
BOOL CmdFlagSet (int args);
BOOL CmdGo (int args);
BOOL CmdInput (int args);
BOOL CmdInternalMemoryDump (int args);
BOOL CmdLineDown (int args);
BOOL CmdLineUp (int args);
BOOL CmdMemoryDump (int args);
BOOL CmdMemoryEnter (int args);
BOOL CmdMemoryFill (int args);
BOOL CmdOutput (int args);
BOOL CmdPageDown (int args);
BOOL CmdPageUp (int args);
BOOL CmdProfile (int args);
BOOL CmdRegisterSet (int args);
BOOL CmdTrace (int args);
BOOL CmdTermColor (int args);
BOOL CmdTraceFile (int args);
BOOL CmdViewOutput (int args);
BOOL CmdWatchAdd (int args);
BOOL CmdWatchClear (int args);
BOOL CmdZap (int args);

typedef struct _addrrec {
    TCHAR format[12];
    int   bytes;
} addrrec, *addrptr;

typedef struct _argrec {
    TCHAR str[12];
    WORD  val1;
    WORD  val2;
} argrec, *argptr;

typedef struct _bprec {
    WORD address;
    WORD length;
    BOOL enabled;
} bprec, *bpptr;

typedef struct _cmdrec {
    TCHAR       name[12];
    cmdfunction function;
} cmdrec, *cmdptr;

typedef struct _instrec {
    TCHAR mnemonic[5];
    int   addrmode;
} instrec, *instptr;

typedef struct _symbolrec {
    WORD  value;
    TCHAR name[14];
} symbolrec, *symbolptr;

addrrec addressmode[19]  = { {TEXT("")         ,1},        // (implied)
                             {TEXT("")         ,1},        // INVALID1
                             {TEXT("")         ,2},        // INVALID2
                             {TEXT("")         ,3},        // INVALID3
                             {TEXT("#$%02X")   ,2},        // ADDR_IMM
                             {TEXT("%s")       ,3},        // ADDR_ABS
                             {TEXT("%s")       ,2},        // ADDR_ZPG
                             {TEXT("%s,X")     ,3},        // ADDR_ABSX
                             {TEXT("%s,Y")     ,3},        // ADDR_ABSY
                             {TEXT("%s,X")     ,2},        // ADDR_ZPGX
                             {TEXT("%s,Y")     ,2},        // ADDR_ZPGY
                             {TEXT("%s")       ,2},        // ADDR_REL
                             {TEXT("%s %s")    ,3},        // ADDR_BBREL
                             {TEXT("(%s,X)")   ,2},        // ADDR_INDX
                             {TEXT("(%s,X)")   ,3},        // ADDR_ABSIINDX
                             {TEXT("(%s),Y")   ,2},        // ADDR_INDY
                             {TEXT("(%s)")     ,2},        // ADDR_IZPG
                             {TEXT("(%s)")     ,3},        // ADDR_IABS
                             {TEXT("IO PAGE")  ,1} };      // ADDR_IO PAGE

cmdrec command[COMMANDS] = { {TEXT("B")       ,CmdBreakpointAdd},
							 {TEXT("BC")      ,CmdBreakpointClear},
                             {TEXT("BD")      ,CmdBreakpointDisable},
                             {TEXT("BE")      ,CmdBreakpointEnable},
                             {TEXT("D")       ,CmdMemoryDump},
							 {TEXT("DF")      ,CmdInternalMemoryDump},
                             {TEXT("EM")      ,CmdMemoryEnter},
                             {TEXT("FM")      ,CmdMemoryFill},
                             {TEXT("G")       ,CmdGo},
                             {TEXT("I")       ,CmdInput},
							 {TEXT("K")       ,CmdFeedKey},
							 {TEXT("L")       ,CmdCodeDump},
                             {TEXT("MS")      ,NULL}, // CmdMemorySearch
                             {TEXT("O")       ,CmdOutput},
                             {TEXT("PRO")     ,CmdProfile},
                             {TEXT("R")       ,CmdRegisterSet},
                             {TEXT("RB")      ,CmdFlagSet},
                             {TEXT("RC")      ,CmdFlagSet},
                             {TEXT("RD")      ,CmdFlagSet},
                             {TEXT("RI")      ,CmdFlagSet},
                             {TEXT("RN")      ,CmdFlagSet},
                             {TEXT("RR")      ,CmdFlagSet},
                             {TEXT("RV")      ,CmdFlagSet},
                             {TEXT("RZ")      ,CmdFlagSet},
                             {TEXT("SB")      ,CmdFlagSet},
                             {TEXT("SC")      ,CmdFlagSet},
                             {TEXT("SD")      ,CmdFlagSet},
                             {TEXT("SI")      ,CmdFlagSet},
                             {TEXT("SN")      ,CmdFlagSet},
                             {TEXT("SR")      ,CmdFlagSet},
                             {TEXT("SV")      ,CmdFlagSet},
                             {TEXT("SZ")      ,CmdFlagSet},
                             {TEXT("T")       ,CmdTrace},
							 {TEXT("TC")	  ,CmdTermColor}, //Invert color of terminal disp
                             {TEXT("TF")      ,CmdTraceFile},
							 {TEXT("VO")      ,CmdViewOutput},
                             {TEXT("W")       ,CmdWatchAdd},
                             {TEXT("WC")      ,CmdWatchClear},
							 {TEXT("ZAP")     ,CmdZap} };

instrec instruction[257] = { {TEXT("BRK"),0},              // 00h
                             {TEXT("ORA"),ADDR_INDX},      // 01h
                             {TEXT("NOP"),INVALID2},       // 02h
                             {TEXT("NOP"),INVALID1},       // 03h
                             {TEXT("TSB"),ADDR_ZPG},       // 04h
                             {TEXT("ORA"),ADDR_ZPG},       // 05h
                             {TEXT("ASL"),ADDR_ZPG},       // 06h
							 {TEXT("RMB0"),ADDR_ZPG},      // 07h
                             {TEXT("PHP"),0},              // 08h
                             {TEXT("ORA"),ADDR_IMM},       // 09h
                             {TEXT("ASL"),0},              // 0Ah
                             {TEXT("NOP"),INVALID1},       // 0Bh
                             {TEXT("TSB"),ADDR_ABS},       // 0Ch
                             {TEXT("ORA"),ADDR_ABS},       // 0Dh
                             {TEXT("ASL"),ADDR_ABS},       // 0Eh
                             {TEXT("BBR0"),ADDR_BBREL},        // 0Fh
                             {TEXT("BPL"),ADDR_REL},       // 10h
                             {TEXT("ORA"),ADDR_INDY},      // 11h
                             {TEXT("ORA"),ADDR_IZPG},      // 12h
                             {TEXT("NOP"),INVALID1},       // 13h
                             {TEXT("TRB"),ADDR_ZPG},       // 14h
                             {TEXT("ORA"),ADDR_ZPGX},      // 15h
                             {TEXT("ASL"),ADDR_ZPGX},      // 16h
                             {TEXT("RMB1"),ADDR_ZPG},       // 17h
                             {TEXT("CLC"),0},              // 18h
                             {TEXT("ORA"),ADDR_ABSY},      // 19h
                             {TEXT("INA"),0},              // 1Ah
                             {TEXT("NOP"),INVALID1},       // 1Bh
                             {TEXT("TRB"),ADDR_ABS},       // 1Ch
                             {TEXT("ORA"),ADDR_ABSX},      // 1Dh
                             {TEXT("ASL"),ADDR_ABSX},      // 1Eh
                             {TEXT("BBR1"),ADDR_BBREL},       // 1Fh
                             {TEXT("JSR"),ADDR_ABS},       // 20h
                             {TEXT("AND"),ADDR_INDX},      // 21h
                             {TEXT("NOP"),INVALID2},       // 22h
                             {TEXT("NOP"),INVALID1},       // 23h
                             {TEXT("BIT"),ADDR_ZPG},       // 24h
                             {TEXT("AND"),ADDR_ZPG},       // 25h
                             {TEXT("ROL"),ADDR_ZPG},       // 26h
                             {TEXT("RMB2"),ADDR_ZPG},       // 27h
                             {TEXT("PLP"),0},              // 28h
                             {TEXT("AND"),ADDR_IMM},       // 29h
                             {TEXT("ROL"),0},              // 2Ah
                             {TEXT("NOP"),INVALID1},       // 2Bh
                             {TEXT("BIT"),ADDR_ABS},       // 2Ch
                             {TEXT("AND"),ADDR_ABS},       // 2Dh
                             {TEXT("ROL"),ADDR_ABS},       // 2Eh
                             {TEXT("BBR2"),ADDR_BBREL},       // 2Fh
                             {TEXT("BMI"),ADDR_REL},       // 30h
                             {TEXT("AND"),ADDR_INDY},      // 31h
                             {TEXT("AND"),ADDR_IZPG},      // 32h
                             {TEXT("NOP"),INVALID1},       // 33h
                             {TEXT("BIT"),ADDR_ZPGX},      // 34h
                             {TEXT("AND"),ADDR_ZPGX},      // 35h
                             {TEXT("ROL"),ADDR_ZPGX},      // 36h
                             {TEXT("RMB3"),ADDR_ZPG},       // 37h
                             {TEXT("SEC"),0},              // 38h
                             {TEXT("AND"),ADDR_ABSY},      // 39h
                             {TEXT("DEA"),0},              // 3Ah
                             {TEXT("NOP"),INVALID1},       // 3Bh
                             {TEXT("BIT"),ADDR_ABSX},      // 3Ch
                             {TEXT("AND"),ADDR_ABSX},      // 3Dh
                             {TEXT("ROL"),ADDR_ABSX},      // 3Eh
                             {TEXT("BBR3"),ADDR_BBREL},       // 3Fh
                             {TEXT("RTI"),0},              // 40h
                             {TEXT("EOR"),ADDR_INDX},      // 41h
                             {TEXT("NOP"),INVALID2},       // 42h
                             {TEXT("NOP"),INVALID1},       // 43h
                             {TEXT("NOP"),INVALID2},       // 44h
                             {TEXT("EOR"),ADDR_ZPG},       // 45h
                             {TEXT("LSR"),ADDR_ZPG},       // 46h
                             {TEXT("RMB4"),ADDR_ZPG},       // 47h
                             {TEXT("PHA"),0},              // 48h
                             {TEXT("EOR"),ADDR_IMM},       // 49h
                             {TEXT("LSR"),0},              // 4Ah
                             {TEXT("NOP"),INVALID1},       // 4Bh
                             {TEXT("JMP"),ADDR_ABS},       // 4Ch
                             {TEXT("EOR"),ADDR_ABS},       // 4Dh
                             {TEXT("LSR"),ADDR_ABS},       // 4Eh
                             {TEXT("BBR4"),ADDR_BBREL},       // 4Fh
                             {TEXT("BVC"),ADDR_REL},       // 50h
                             {TEXT("EOR"),ADDR_INDY},      // 51h
                             {TEXT("EOR"),ADDR_IZPG},      // 52h
                             {TEXT("NOP"),INVALID1},       // 53h
                             {TEXT("NOP"),INVALID2},       // 54h
                             {TEXT("EOR"),ADDR_ZPGX},      // 55h
                             {TEXT("LSR"),ADDR_ZPGX},      // 56h
                             {TEXT("RMB5"),ADDR_ZPG},       // 57h
                             {TEXT("CLI"),0},              // 58h
                             {TEXT("EOR"),ADDR_ABSY},      // 59h
                             {TEXT("PHY"),0},              // 5Ah
                             {TEXT("NOP"),INVALID1},       // 5Bh
                             {TEXT("NOP"),INVALID3},       // 5Ch
                             {TEXT("EOR"),ADDR_ABSX},      // 5Dh
                             {TEXT("LSR"),ADDR_ABSX},      // 5Eh
                             {TEXT("BBR5"),ADDR_BBREL},       // 5Fh
                             {TEXT("RTS"),0},              // 60h
                             {TEXT("ADC"),ADDR_INDX},      // 61h
                             {TEXT("NOP"),INVALID2},       // 62h
                             {TEXT("NOP"),INVALID1},       // 63h
                             {TEXT("STZ"),ADDR_ZPG},       // 64h
                             {TEXT("ADC"),ADDR_ZPG},       // 65h
                             {TEXT("ROR"),ADDR_ZPG},       // 66h
                             {TEXT("RMB6"),ADDR_ZPG},       // 67h
                             {TEXT("PLA"),0},              // 68h
                             {TEXT("ADC"),ADDR_IMM},       // 69h
                             {TEXT("ROR"),0},              // 6Ah
                             {TEXT("NOP"),INVALID1},       // 6Bh
                             {TEXT("JMP"),ADDR_IABS},      // 6Ch
                             {TEXT("ADC"),ADDR_ABS},       // 6Dh
                             {TEXT("ROR"),ADDR_ABS},       // 6Eh
                             {TEXT("BBR6"),ADDR_BBREL},       // 6Fh
                             {TEXT("BVS"),ADDR_REL},       // 70h
                             {TEXT("ADC"),ADDR_INDY},      // 71h
                             {TEXT("ADC"),ADDR_IZPG},      // 72h
                             {TEXT("NOP"),INVALID1},       // 73h
                             {TEXT("STZ"),ADDR_ZPGX},      // 74h
                             {TEXT("ADC"),ADDR_ZPGX},      // 75h
                             {TEXT("ROR"),ADDR_ZPGX},      // 76h
                             {TEXT("RMB7"),ADDR_ZPG},       // 77h
                             {TEXT("SEI"),0},              // 78h
                             {TEXT("ADC"),ADDR_ABSY},      // 79h
                             {TEXT("PLY"),0},              // 7Ah
                             {TEXT("NOP"),INVALID1},       // 7Bh
                             {TEXT("JMP"),ADDR_ABSIINDX},  // 7Ch
                             {TEXT("ADC"),ADDR_ABSX},      // 7Dh
                             {TEXT("ROR"),ADDR_ABSX},      // 7Eh
                             {TEXT("BBR7"),ADDR_BBREL},       // 7Fh
                             {TEXT("BRA"),ADDR_REL},       // 80h
                             {TEXT("STA"),ADDR_INDX},      // 81h
                             {TEXT("NOP"),INVALID2},       // 82h
                             {TEXT("NOP"),INVALID1},       // 83h
                             {TEXT("STY"),ADDR_ZPG},       // 84h
                             {TEXT("STA"),ADDR_ZPG},       // 85h
                             {TEXT("STX"),ADDR_ZPG},       // 86h
                             {TEXT("SMB0"),ADDR_ZPG},       // 87h
                             {TEXT("DEY"),0},              // 88h
                             {TEXT("BIT"),ADDR_IMM},       // 89h
                             {TEXT("TXA"),0},              // 8Ah
                             {TEXT("NOP"),INVALID1},       // 8Bh
                             {TEXT("STY"),ADDR_ABS},       // 8Ch
                             {TEXT("STA"),ADDR_ABS},       // 8Dh
                             {TEXT("STX"),ADDR_ABS},       // 8Eh
                             {TEXT("BBS0"),ADDR_BBREL},       // 8Fh
                             {TEXT("BCC"),ADDR_REL},       // 90h
                             {TEXT("STA"),ADDR_INDY},      // 91h
                             {TEXT("STA"),ADDR_IZPG},      // 92h
                             {TEXT("NOP"),INVALID1},       // 93h
                             {TEXT("STY"),ADDR_ZPGX},      // 94h
                             {TEXT("STA"),ADDR_ZPGX},      // 95h
                             {TEXT("STX"),ADDR_ZPGY},      // 96h
                             {TEXT("SMB1"),ADDR_ZPG},       // 97h
                             {TEXT("TYA"),0},              // 98h
                             {TEXT("STA"),ADDR_ABSY},      // 99h
                             {TEXT("TXS"),0},              // 9Ah
                             {TEXT("NOP"),INVALID1},       // 9Bh
                             {TEXT("STZ"),ADDR_ABS},       // 9Ch
                             {TEXT("STA"),ADDR_ABSX},      // 9Dh
                             {TEXT("STZ"),ADDR_ABSX},      // 9Eh
                             {TEXT("BBS1"),ADDR_BBREL},       // 9Fh
                             {TEXT("LDY"),ADDR_IMM},       // A0h
                             {TEXT("LDA"),ADDR_INDX},      // A1h
                             {TEXT("LDX"),ADDR_IMM},       // A2h
                             {TEXT("NOP"),INVALID1},       // A3h
                             {TEXT("LDY"),ADDR_ZPG},       // A4h
                             {TEXT("LDA"),ADDR_ZPG},       // A5h
                             {TEXT("LDX"),ADDR_ZPG},       // A6h
                             {TEXT("SMB2"),ADDR_ZPG},       // A7h
                             {TEXT("TAY"),0},              // A8h
                             {TEXT("LDA"),ADDR_IMM},       // A9h
                             {TEXT("TAX"),0},              // AAh
                             {TEXT("NOP"),INVALID1},       // ABh
                             {TEXT("LDY"),ADDR_ABS},       // ACh
                             {TEXT("LDA"),ADDR_ABS},       // ADh
                             {TEXT("LDX"),ADDR_ABS},       // AEh
                             {TEXT("BBS2"),ADDR_BBREL},       // AFh
                             {TEXT("BCS"),ADDR_REL},       // B0h
                             {TEXT("LDA"),ADDR_INDY},      // B1h
                             {TEXT("LDA"),ADDR_IZPG},      // B2h
                             {TEXT("NOP"),INVALID1},       // B3h
                             {TEXT("LDY"),ADDR_ZPGX},      // B4h
                             {TEXT("LDA"),ADDR_ZPGX},      // B5h
                             {TEXT("LDX"),ADDR_ZPGY},      // B6h
                             {TEXT("SMB3"),ADDR_ZPG},       // B7h
                             {TEXT("CLV"),0},              // B8h
                             {TEXT("LDA"),ADDR_ABSY},      // B9h
                             {TEXT("TSX"),0},              // BAh
                             {TEXT("NOP"),INVALID1},       // BBh
                             {TEXT("LDY"),ADDR_ABSX},      // BCh
                             {TEXT("LDA"),ADDR_ABSX},      // BDh
                             {TEXT("LDX"),ADDR_ABSY},      // BEh
                             {TEXT("BBS3"),ADDR_BBREL},       // BFh
                             {TEXT("CPY"),ADDR_IMM},       // C0h
                             {TEXT("CMP"),ADDR_INDX},      // C1h
                             {TEXT("NOP"),INVALID2},       // C2h
                             {TEXT("NOP"),INVALID1},       // C3h
                             {TEXT("CPY"),ADDR_ZPG},       // C4h
                             {TEXT("CMP"),ADDR_ZPG},       // C5h
                             {TEXT("DEC"),ADDR_ZPG},       // C6h
                             {TEXT("SMB4"),ADDR_ZPG},       // C7h
                             {TEXT("INY"),0},              // C8h
                             {TEXT("CMP"),ADDR_IMM},       // C9h
                             {TEXT("DEX"),0},              // CAh
                             {TEXT("WAI"),INVALID1},       // CBh
                             {TEXT("CPY"),ADDR_ABS},       // CCh
                             {TEXT("CMP"),ADDR_ABS},       // CDh
                             {TEXT("DEC"),ADDR_ABS},       // CEh
                             {TEXT("BBS4"),ADDR_BBREL},       // CFh
                             {TEXT("BNE"),ADDR_REL},       // D0h
                             {TEXT("CMP"),ADDR_INDY},      // D1h
                             {TEXT("CMP"),ADDR_IZPG},      // D2h
                             {TEXT("NOP"),INVALID1},       // D3h
                             {TEXT("NOP"),INVALID2},       // D4h
                             {TEXT("CMP"),ADDR_ZPGX},      // D5h
                             {TEXT("DEC"),ADDR_ZPGX},      // D6h
                             {TEXT("SMB5"),ADDR_ZPG},       // D7h
                             {TEXT("CLD"),0},              // D8h
                             {TEXT("CMP"),ADDR_ABSY},      // D9h
                             {TEXT("PHX"),0},              // DAh
                             {TEXT("STP"),INVALID1},       // DBh
                             {TEXT("NOP"),INVALID3},       // DCh
                             {TEXT("CMP"),ADDR_ABSX},      // DDh
                             {TEXT("DEC"),ADDR_ABSX},      // DEh
                             {TEXT("BBS5"),ADDR_BBREL},       // DFh
                             {TEXT("CPX"),ADDR_IMM},       // E0h
                             {TEXT("SBC"),ADDR_INDX},      // E1h
                             {TEXT("NOP"),INVALID2},       // E2h
                             {TEXT("NOP"),INVALID1},       // E3h
                             {TEXT("CPX"),ADDR_ZPG},       // E4h
                             {TEXT("SBC"),ADDR_ZPG},       // E5h
                             {TEXT("INC"),ADDR_ZPG},       // E6h
                             {TEXT("SMB6"),ADDR_ZPG},       // E7h
                             {TEXT("INX"),0},              // E8h
                             {TEXT("SBC"),ADDR_IMM},       // E9h
                             {TEXT("NOP"),0},              // EAh
                             {TEXT("NOP"),INVALID1},       // EBh
                             {TEXT("CPX"),ADDR_ABS},       // ECh
                             {TEXT("SBC"),ADDR_ABS},       // EDh
                             {TEXT("INC"),ADDR_ABS},       // EEh
                             {TEXT("BBS6"),ADDR_BBREL},       // EFh
                             {TEXT("BEQ"),ADDR_REL},       // F0h
                             {TEXT("SBC"),ADDR_INDY},      // F1h
                             {TEXT("SBC"),ADDR_IZPG},      // F2h
                             {TEXT("NOP"),INVALID1},       // F3h
                             {TEXT("NOP"),INVALID2},       // F4h
                             {TEXT("SBC"),ADDR_ZPGX},      // F5h
                             {TEXT("INC"),ADDR_ZPGX},      // F6h
                             {TEXT("SMB7"),ADDR_ZPG},       // F7h
                             {TEXT("SED"),0},              // F8h
                             {TEXT("SBC"),ADDR_ABSY},      // F9h
                             {TEXT("PLX"),0},              // FAh
                             {TEXT("NOP"),INVALID1},       // FBh
                             {TEXT("NOP"),INVALID3},       // FCh
                             {TEXT("SBC"),ADDR_ABSX},      // FDh
                             {TEXT("INC"),ADDR_ABSX},      // FEh
                             {TEXT("BBS7"),ADDR_BBREL},    // FFh
							 {TEXT("   "),18} };            // 100h

COLORREF color[COLORS]		= {0x800000,0xC0C0C0,0x00FFFF,0x808000,
							   0x000080,0x800000,0x00FFFF,0xFFFFFF};

TCHAR commandstring[COMMANDLINES][80] = {TEXT(""),
                                         TEXT(" "),
										 VERSION,
										 TEXT(" 65C02 Simulator Debugger"),
                                         TEXT(" ")};

argrec arg[MAXARGS];
bprec  breakpoint[BREAKPOINTS];
DWORD  profiledata[256];
int    watch[WATCHES];

HFONT     debugfont     = (HFONT)0;
WORD      lastpc        = 0;
WORD      memorydump    = 0;
BOOL      profiling     = 0;
int       stepcount     = 0;
int       stepuntil     = -1;
symbolptr symboltable   = NULL;
int       symbolnum     = 0;
WORD      topoffset     = 0;
FILE     *tracefile     = NULL;
BOOL      usingbp       = 0;
BOOL      usingmemdump  = 0;
BOOL      usingwatches  = 0;
BOOL      viewingoutput = 0;
DWORD     ecycles       = 0;

void ComputeTopOffset (WORD centeroffset);
BOOL DisplayError (LPCTSTR errortext);
BOOL InternalSingleStep ();
WORD GetAddress (LPCTSTR symbol);
LPCTSTR GetSymbol (WORD address, int bytes);
void GetTargets (int *intermediate, int *final, BOOL *ind);


BOOL CheckBreakpoint (WORD address, BOOL memory) {
  int target[3];
  target[0] = address;
  target[1] = -1;
  target[2] = -1;
  BOOL ind		= 0;
  if (memory)
    GetTargets(&target[1],&target[2],&ind);
  int targetnum = memory ? 3 : 1;
  while (targetnum--)
    if (target[targetnum] >= 0) {
      WORD targetaddr = (WORD)(target[targetnum] & 0xFFFF);
      int  slot       = 0;
      while (slot < BREAKPOINTS) {
        if (breakpoint[slot].length && breakpoint[slot].enabled &&
            (breakpoint[slot].address <= targetaddr) &&
            (breakpoint[slot].address+breakpoint[slot].length > targetaddr))
          return 1;
        slot++;
      }
    }
  return 0;
}

BOOL CheckJump (WORD targetaddress) {
  WORD savedpc = regs.pc;
  InternalSingleStep();
  BOOL result = (regs.pc == targetaddress);
  regs.pc = savedpc;
  return result;
}

BOOL CmdBreakpointAdd (int args) {

  if (!args)
    arg[args = 1].val1 = regs.pc;
  BOOL addedone = 0;
  int  loop     = 0;
  while (loop++ < args)
    if (arg[loop].val1 || (arg[loop].str[0] == TEXT('0')) ||
        GetAddress(arg[loop].str)) {
      if (!arg[loop].val1)
        arg[loop].val1 = GetAddress(arg[loop].str);

      // FIND A FREE SLOT FOR THIS NEW BREAKPOINT
      int freeslot = 0;
      while ((freeslot < BREAKPOINTS) && breakpoint[freeslot].length)
        freeslot++;
      if ((freeslot >= BREAKPOINTS) && !addedone)
        return DisplayError(TEXT("All breakpoint slots are currently in use."));

      // ADD THE BREAKPOINT
      if (freeslot < BREAKPOINTS) {
        breakpoint[freeslot].address = arg[loop].val1;
        breakpoint[freeslot].length  = arg[loop].val2
                                         ? MIN(0x10000-arg[loop].val1,arg[loop].val2)
                                         : 1;
        breakpoint[freeslot].enabled = 1;
        addedone = 1;
        usingbp  = 1;
      }

    }
  return 1;
}

BOOL CmdBreakpointClear (int args) {

  // CHECK FOR ERRORS
  if (!args)
    return 1;
  if (!usingbp)
    return DisplayError(TEXT("There are no breakpoints defined."));

  // CLEAR EACH BREAKPOINT IN THE LIST
  while (args) {
    if (!_tcscmp(arg[args].str,TEXT("*"))) {
      int loop = BREAKPOINTS;
      while (loop--)
        breakpoint[loop].length = 0;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= BREAKPOINTS))
      breakpoint[arg[args].val1-1].length = 0;
    args--;
  }

  // IF THERE ARE NO MORE BREAKPOINTS DEFINED, DISABLE THE BREAKPOINT
  // FUNCTIONALITY AND ERASE THE BREAKPOINT DISPLAY FROM THE SCREEN
  int usedslot = 0;
  while ((usedslot < BREAKPOINTS) && !breakpoint[usedslot].length)
    usedslot++;
  if (usedslot >= BREAKPOINTS) {
    usingbp = 0;
    DebugDisplay(1);
    return 0;
  }
  else
    return 1;

}

BOOL CmdBreakpointDisable (int args) {

  // CHECK FOR ERRORS
  if (!args)
    return 1;
  if (!usingbp)
    return DisplayError(TEXT("There are no breakpoints defined."));

  // DISABLE EACH BREAKPOINT IN THE LIST
  while (args) {
    if (!_tcscmp(arg[args].str,TEXT("*"))) {
      int loop = BREAKPOINTS;
      while (loop--)
        breakpoint[loop].enabled = 0;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= BREAKPOINTS))
      breakpoint[arg[args].val1-1].enabled = 0;
    args--;
  }

  return 1;
}

BOOL CmdBreakpointEnable (int args) {

  // CHECK FOR ERRORS
  if (!args)
    return 1;
  if (!usingbp)
    return DisplayError(TEXT("There are no breakpoints defined."));

  // ENABLE EACH BREAKPOINT IN THE LIST
  while (args) {
    if (!_tcscmp(arg[args].str,TEXT("*"))) {
      int loop = BREAKPOINTS;
      while (loop--)
        breakpoint[loop].enabled = 1;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= BREAKPOINTS))
      breakpoint[arg[args].val1-1].enabled = 1;
    args--;
  }

  return 1;
}

BOOL CmdCodeDump (int args) {
  if ((!args) ||
      ((arg[1].str[0] != TEXT('0')) && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return 1;
  topoffset = arg[1].val1;
  if (!topoffset)
    topoffset = GetAddress(arg[1].str);
  return 1;
}

BOOL CmdFeedKey (int args) {
  KeybQueueKeypress(args ? arg[1].val1 ? arg[1].val1
                                       : arg[1].str[0]
                         : VK_SPACE,
                    0);
  return 0;
}

BOOL CmdFlagSet (int args) {
  static const TCHAR flagname[] = TEXT("CZIDBRVN");
  int loop = 0;
  while (loop < 8)
    if (flagname[loop] == arg[0].str[1])
      break;
    else
      loop++;
  if (loop < 8)
    if (arg[0].str[0] == TEXT('R'))
      regs.ps &= ~(1 << loop);
    else
      regs.ps |= (1 << loop);
  return 1;
}

BOOL CmdGo (int args) {
  stepcount = -1;
  stepuntil = args ? arg[1].val1 : -1;
  if (!stepuntil)
    stepuntil = GetAddress(arg[1].str);
  mode = MODE_STEPPING;
  FrameRefreshStatus ();
  return 0;
}

BOOL CmdInput (int args) {
  if ((!args) ||
      ((arg[1].str[0] != TEXT('0')) && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return 1;
  if (!arg[1].val1)
    arg[1].val1 = GetAddress(arg[1].str);
  ioread[arg[1].val1 & 0xFF](0,0);  // need to assign to value?
  return 1;
}

BOOL CmdInternalMemoryDump (int args) {
  TCHAR filename[MAX_PATH];
  _tcscpy(filename,progdir);
  _tcscat(filename,TEXT("Output.bin"));
  HANDLE file = CreateFile(filename,
                           GENERIC_WRITE,
                           0,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
  if (file != INVALID_HANDLE_VALUE) {
    DWORD byteswritten;
    WriteFile(file,
              mem,
              0x10000,
              &byteswritten,
              NULL);
    CloseHandle(file);
  }
  return 0;
}

BOOL CmdLineDown (int args) {
  topoffset += addressmode[instruction[*(mem+topoffset)].addrmode].bytes;
  return 1;
}

BOOL CmdLineUp (int args) {

  int  currnum = 1;
  do {
    int addrmode = instruction[*(mem+(WORD)(topoffset-currnum))].addrmode;
    if ((addrmode == 0) || (addrmode > 3))    // if not invalid opcode
	  if (addressmode[addrmode].bytes == currnum)  {
		topoffset -= currnum;
		break;
	}
  } while ((++currnum <= 3));
  if (currnum == 4) topoffset--;

  return 1;
}

BOOL CmdMemoryDump (int args) {
  if ((!args) ||
      ((arg[1].str[0] != TEXT('0')) && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return usingmemdump =0;
  memorydump   = arg[1].val1;
  if (!memorydump)
    memorydump = GetAddress(arg[1].str);
  usingmemdump = 1;
  return 1;
}

BOOL CmdMemoryEnter (int args) {
  if ((args < 2) ||
      ((arg[1].str[0] != TEXT('0')) && (!arg[1].val1) && (!GetAddress(arg[1].str))) ||
      ((arg[2].str[0] != TEXT('0')) && !arg[2].val1))
    return 1;
  WORD address = arg[1].val1;
  if (!address)
    address = GetAddress(arg[1].str);
  while (args >= 2) {
 //  *(mem+address+args-2)  = (BYTE)arg[args].val1;
	if (((address+args-2) & 0xFF00) == (iopage & 0xFF00)) {
			iowrite[(address+args-2) & 0xff]((BYTE)((address+args-2) & 0xff),(BYTE)(arg[args].val1));
		} else {
			*(mem+address+args-2)  = (BYTE)arg[args].val1;
		}
    args--;
  }
  return 1;
}

BOOL CmdMemoryFill (int args) {
  if ((!args) ||
      ((arg[1].str[0] != TEXT('0')) && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return 1;
  WORD address = arg[1].val1 ? arg[1].val1 : GetAddress(arg[1].str);
  WORD bytes   = MAX(1,arg[1].val2);
  while (bytes--) {
    if ((address & 0xFF00) != (iopage & 0XFF00))
      *(mem+address) = (BYTE)(arg[2].val1 & 0xFF);
    address++;
  }
  return 1;
}

BOOL CmdOutput (int args) {
  if ((!args) ||
      ((arg[1].str[0] != TEXT('0')) && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return 1;
  if (!arg[1].val1)
    arg[1].val1 = GetAddress(arg[1].str);
  iowrite[arg[1].val1 & 0xFF](1,arg[2].val1 & 0xFF);
  return 1;
}

BOOL CmdPageDown (int args) {
  int loop = 0;
  while (loop++ < SOURCELINES)
    CmdLineDown(args);
  return 1;
}

BOOL CmdPageUp (int args) {
  int loop = 0;
  while (loop++ < SOURCELINES)
    CmdLineUp(args);
  return 1;
}

BOOL CmdProfile (int args) {
  ZeroMemory(profiledata,256*sizeof(DWORD));
  profiling = 1;
  return 0;
}

BOOL CmdRegisterSet (int args) {
   if ((args == 2) &&
      (arg[1].str[0] == TEXT('P')) && (arg[2].str[0] == TEXT('L')))
    regs.pc = lastpc;
  else if ((args < 2) || ((arg[2].str[0] != TEXT('0')) && !arg[2].val1))
    return 1;
  else switch (arg[1].str[0]) {
    case TEXT('A'): regs.a  = (BYTE)(arg[2].val1 & 0xFF);    break;
    case TEXT('P'): regs.pc = arg[2].val1;                   break;
    case TEXT('S'): regs.sp = 0x100 | (arg[2].val1 & 0xFF);  break;
    case TEXT('X'): regs.x  = (BYTE)(arg[2].val1 & 0xFF);    break;
    case TEXT('Y'): regs.y  = (BYTE)(arg[2].val1 & 0xFF);    break;
    default:        return 1;
  }
  ComputeTopOffset(regs.pc);
  return 1;
}

BOOL CmdTermColor (int args) {
  TermColor = (TermColor == 0);
  return 1;
}

BOOL CmdTrace (int args) {
  stepcount = args ? arg[1].val1 : 1;
  stepuntil = -1;
  mode = MODE_STEPPING;
  DebugContinueStepping();
  return 0;
}

BOOL CmdTraceFile (int args) {
  if (tracefile)
    fclose(tracefile);
  TCHAR filename[MAX_PATH];
  _tcscpy(filename,progdir);
  _tcscat(filename,(args && arg[1].str[0]) ? arg[1].str : TEXT("Trace.txt"));
  tracefile = fopen(filename,TEXT("wt"));
  return 0;
}

BOOL CmdViewOutput (int args) {
  TerminalDisplay(1);
  viewingoutput = 1;
  return 0;
}

BOOL CmdWatchAdd (int args) {
  if (!args)
    return 1;
  BOOL addedone = 0;
  int  loop     = 0;
  while (loop++ < args)
    if (arg[loop].val1 || (arg[loop].str[0] == TEXT('0')) ||
        GetAddress(arg[loop].str)) {
      if (!arg[loop].val1)
        arg[loop].val1 = GetAddress(arg[loop].str);

      // FIND A FREE SLOT FOR THIS NEW WATCH
      int freeslot = 0;
      while ((freeslot < WATCHES) && (watch[freeslot] >= 0))
        freeslot++;
      if ((freeslot >= WATCHES) && !addedone)
        return DisplayError(TEXT("All watch slots are currently in use."));

      // VERIFY THAT THE WATCH IS NOT ON AN I/O LOCATION
//      if ((arg[loop].val1 & 0xFF00) == (iopage & 0xFF00))
//        return DisplayError(TEXT("You may not watch an I/O location."));

      // ADD THE WATCH
      if (freeslot < WATCHES) {
        watch[freeslot] = arg[loop].val1;
        addedone        = 1;
        usingwatches    = 1;
      }

    }
  return 1;
}

BOOL CmdWatchClear (int args) {

  // CHECK FOR ERRORS
  if (!args)
    return 1;
  if (!usingwatches)
    return DisplayError(TEXT("There are no watches defined."));

  // CLEAR EACH WATCH IN THE LIST
  while (args) {
    if (!_tcscmp(arg[args].str,TEXT("*"))) {
      int loop = WATCHES;
      while (loop--)
        watch[loop] = -1;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= WATCHES))
      watch[arg[args].val1-1] = -1;
    args--;
  }

  // IF THERE ARE NO MORE WATCHES DEFINED, ERASE THE WATCH DISPLAY
  int usedslot = 0;
  while ((usedslot < WATCHES) && (watch[usedslot] < 0))
    usedslot++;
  if (usedslot >= WATCHES) {
    usingwatches = 0;
    DebugDisplay(1);
    return 0;
  }
  else
    return 1;

}

BOOL CmdZap (int args) {
  int loop = addressmode[instruction[*(mem+regs.pc)].addrmode].bytes;
  while (loop--)
    *(mem+regs.pc+loop) = 0xEA;
  return 1;
}

void ComputeTopOffset (WORD centeroffset) {
  topoffset = centeroffset;
  int lincnt = 0;
  int  match[4] = {0,0,0,0};
  do {
    int  currnum = 3;
    do {
	  int addrmode = instruction[*(mem+(WORD)(topoffset-currnum))].addrmode;
//	  if ((addrmode == 0) || (addrmode > 3))    // if not invalid opcode
      if (addressmode[addrmode].bytes == currnum)
  	    if (!match[currnum]) match[currnum] = currnum;
	} while ((--currnum >= 1));
	if (match[1]>0) match[0] = match[1];
	if (match[2]>0) match[0] = match[2];
	if (match[3]>0) match[0] = match[3];
	if (match[0]) {
		lincnt++;
		topoffset -= match[0];
		match[0]=0; match[1]=0; match[2]=0; match[3]=0;
	} else {
        int nobytes = addressmode[instruction[*(mem+(WORD)(topoffset))].addrmode].bytes;
		topoffset += nobytes;
		match[0]=0; match[1]=0; match[2]=0; match[3]=0;
		match[nobytes] = -1;
		if (match[1] == -1) match[2] = -1;
		if (match[2] == -1) match[3] = -1;
		lincnt--;
		if (lincnt < 0) {
			lincnt = 19;
			topoffset = centeroffset - 40;
		}
	}
  } while ((lincnt < (SOURCELINES >> 1)));
}

BOOL DisplayError (LPCTSTR errortext) {
  int loop = COMMANDLINES-1;
  while (loop--)
      _tcscpy(commandstring[loop+2],commandstring[loop+1]);
  _tcscpy(commandstring[1],TEXT(" !* "));
  _tcscat(commandstring[1],errortext);
  return 0;
}

void DrawBreakpoints (HDC dc, int line) {
  RECT linerect;
  linerect.left   = SCREENSPLIT2;
  linerect.top    = 45+(line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  TCHAR fulltext[16] = TEXT("Breakpoints");
  SetTextColor(dc,color[COLOR_STATIC]);
  SetBkColor(dc,color[COLOR_DATABKG]);
  int loop = 0;
  do {
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               fulltext,_tcslen(fulltext),NULL);
    linerect.top    += 16;
    linerect.bottom += 16;
    if ((loop < BREAKPOINTS) && breakpoint[loop].length) {
      wsprintf(fulltext,
               TEXT("%d: %04X"),
               loop+1,
               (unsigned)breakpoint[loop].address);
      if (breakpoint[loop].length > 1)
        wsprintf(fulltext+_tcslen(fulltext),
                 TEXT("-%04X"),
                 (unsigned)(breakpoint[loop].address+breakpoint[loop].length-1));
      SetTextColor(dc,color[breakpoint[loop].enabled ? COLOR_BPDATA
                                                                  : COLOR_STATIC]);
    }
    else
      fulltext[0] = 0;
  } while (loop++ < BREAKPOINTS);
}

void DrawCommandLine (HDC dc, int line) {
  BOOL title = (commandstring[line][0] == TEXT(' '));
  SetTextColor(dc,color[COLOR_COMMAND]);
  SetBkColor(dc,0);
  RECT linerect;
  linerect.left   = 0;
  linerect.top    = 368+45-(line << 4);
  linerect.right  = 12;
  linerect.bottom = linerect.top+16;
  if (!title) {
    ExtTextOut(dc,1,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               TEXT(">"),1,NULL);
    linerect.left = 12;
  }
  linerect.right = 560;
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             commandstring[line]+title,_tcslen(commandstring[line]+title),NULL);
}

WORD DrawDisassembly (HDC dc, int line, WORD offset, LPTSTR text) {
  TCHAR addresstext[40] = TEXT("");
  TCHAR bytestext[10]   = TEXT("");
  TCHAR fulltext[50]    = TEXT("");
  WORD  inst            = *(mem+offset);

  if ((offset & 0xFF00) == (iopage & 0xFF00)) {
		inst = 0x100;

  }

  int   addrmode        = instruction[inst].addrmode;
  WORD  bytes           = addressmode[addrmode].bytes;


  // BUILD A STRING CONTAINING THE TARGET ADDRESS OR SYMBOL
  if (addressmode[addrmode].format[0]) {

      WORD address = *(LPWORD)(mem+((offset+1) & 0xffff));

      if (bytes == 2)
         address &= 0xFF;

      if (addrmode == ADDR_IMM) {
         WORD address = (*(LPWORD)(mem+((offset+1) & 0xffff)))& 0xFF;
         wsprintf(addresstext,
               addressmode[addrmode].format,
               (unsigned)address);
      }
	  if (addrmode == ADDR_REL) {
         address = (offset+2+(int)(signed char)address) & 0xffff;
         wsprintf(addresstext,
               addressmode[addrmode].format,
               (LPCTSTR)GetSymbol(address,bytes));
         if ((addrmode == ADDR_REL) && (offset == regs.pc) && CheckJump(address))
            if (address > offset)
	         _tcscat(addresstext,TEXT(" \x19"));
	      else
        	   _tcscat(addresstext,TEXT(" \x18"));
      }
	  if (addrmode == ADDR_BBREL) {
        address &= 0xFF;
		WORD zpaddr = address;
		address = *(LPWORD)(mem+((offset+2) & 0xffff));
        address &= 0xFF;
        address = (offset+3+(int)(signed char)address) & 0xffff;

		TCHAR zptxt[14];
		wsprintf(zptxt,(LPCTSTR)GetSymbol(zpaddr,2));
		wsprintf(addresstext,
               addressmode[addrmode].format,
               zptxt,
			   (LPCTSTR)GetSymbol(address,3));

         if ((addrmode == ADDR_BBREL) && (offset == regs.pc) && CheckJump(address))
            if (address > offset)
	         _tcscat(addresstext,TEXT(" \x19"));
	      else
        	   _tcscat(addresstext,TEXT(" \x18"));
      }

      if (addresstext[0] == 0) //else
   	   wsprintf(addresstext,
            addressmode[addrmode].format,
            (LPCTSTR)GetSymbol(address,bytes));
  }

  // BUILD A STRING CONTAINING THE ACTUAL BYTES THAT MAKE UP THIS
  // INSTRUCTION
  {
    int loop = 0;
    while (loop < bytes)
      wsprintf(bytestext+_tcslen(bytestext),
               TEXT("%02X"),
               (unsigned)*(mem+offset+(loop++)));
    while (_tcslen(bytestext) < 6)
      _tcscat(bytestext,TEXT(" "));
  }



  // PUT TOGETHER ALL OF THE DIFFERENT ELEMENTS THAT WILL MAKE UP THE LINE
  wsprintf(fulltext,
           TEXT("%04X  %s  %-9s %-4s %s"),
           (unsigned)offset,
           (LPCTSTR)bytestext,
           (LPCTSTR)GetSymbol(offset,0),
           (LPCTSTR)instruction[inst].mnemonic,
           (LPCTSTR)addresstext);
  if (text)
    _tcscpy(text,fulltext);

  // DRAW THE LINE
  if (dc) {
    RECT linerect;
    linerect.left   = 0;
    linerect.top    = 45+(line << 4);
    linerect.right  = SCREENSPLIT1-14;
    linerect.bottom = linerect.top+16;
    BOOL bp = usingbp && CheckBreakpoint(offset,offset == regs.pc);
    SetTextColor(dc,color[(offset == regs.pc) ? COLOR_INSTBKG
                                                           : bp ? COLOR_INSTBP
                                                                : COLOR_INSTTEXT]);
    SetBkColor(dc,color[(offset == regs.pc) ? bp ? COLOR_INSTBP
                                                              : COLOR_INSTTEXT
                                                         : COLOR_INSTBKG]);
    ExtTextOut(dc,12,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               fulltext,_tcslen(fulltext),NULL);
  }

  return bytes;
}

void DrawFlags (HDC dc, int line, WORD value, LPTSTR text) {
  TCHAR mnemonic[9] = TEXT("NVRBDIZC");
  TCHAR fulltext[2] = TEXT("?");
  RECT  linerect;
  if (dc) {
    linerect.left   = SCREENSPLIT1+63;
    linerect.top    = 45+(line << 4);
    linerect.right  = SCREENSPLIT1+72;
    linerect.bottom = linerect.top+16;
    SetBkColor(dc,color[COLOR_DATABKG]);
  }
  int loop = 8;
  while (loop--) {
    if (dc) {
      fulltext[0] = mnemonic[loop];
      SetTextColor(dc,color[(value & 1) ? COLOR_DATATEXT
                                                     : COLOR_STATIC]);
      ExtTextOut(dc,linerect.left,linerect.top,
                 ETO_CLIPPED | ETO_OPAQUE,&linerect,
                 fulltext,1,NULL);
      linerect.left  -= 9;
      linerect.right -= 9;
    }
    if (!(value & 1))
      mnemonic[loop] = TEXT('.');
    value >>= 1;
  }
  if (text)
    _tcscpy(text,mnemonic);
}

void DrawMemory (HDC dc, int line) {
  RECT linerect;
  linerect.left   = SCREENSPLIT2;
  linerect.top    = 45+(line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  TCHAR fulltext[16];
  wsprintf(fulltext,TEXT("Mem at %04X"),(unsigned)memorydump);
  SetTextColor(dc,color[COLOR_STATIC]);
  SetBkColor(dc,color[COLOR_DATABKG]);
  WORD curraddr = memorydump;
  int  loop     = 0;
  do {
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               fulltext,_tcslen(fulltext),NULL);
    linerect.top    += 16;
    linerect.bottom += 16;
    fulltext[0] = 0;
    if (loop < 4) {
      int loop2 = 0;
      while (loop2++ < 4) {
        if ((curraddr & 0xFF00) == (iopage & 0xFF00))
          _tcscpy(fulltext+_tcslen(fulltext),TEXT("IO "));
        else
          wsprintf(fulltext+_tcslen(fulltext),
                   TEXT("%02X "),
                   (unsigned)*(LPBYTE)(mem+curraddr));
        curraddr++;
      }
    }
    SetTextColor(dc,color[COLOR_DATATEXT]);
  } while (loop++ < 4);
}

void DrawRegister (HDC dc, int line, LPCTSTR name, int bytes, WORD value) {
  RECT linerect;
  linerect.left   = SCREENSPLIT1;
  linerect.top    = 45+(line << 4);
  linerect.right  = SCREENSPLIT1+40;
  linerect.bottom = linerect.top+16;
  SetTextColor(dc,color[COLOR_STATIC]);
  SetBkColor(dc,color[COLOR_DATABKG]);
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             name,_tcslen(name),NULL);
  TCHAR valuestr[8];
  if (bytes == 2)
    wsprintf(valuestr,TEXT("%04X"),(unsigned)value);
  else
    wsprintf(valuestr,TEXT("%02X"),(unsigned)value);
  linerect.left  = SCREENSPLIT1+40;
  linerect.right = SCREENSPLIT2;
  SetTextColor(dc,color[COLOR_DATATEXT]);
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             valuestr,_tcslen(valuestr),NULL);
}

void DrawStack (HDC dc, int line) {
  unsigned curraddr = regs.sp;
  int      loop     = 0;
  while (loop < STACKLINES) {
    curraddr++;
    RECT linerect;
    linerect.left   = SCREENSPLIT1;
    linerect.top    = 45+((loop+line) << 4);
    linerect.right  = SCREENSPLIT1+40;
    linerect.bottom = linerect.top+16;
    SetTextColor(dc,color[COLOR_STATIC]);
    SetBkColor(dc,color[COLOR_DATABKG]);
    TCHAR outtext[8] = TEXT("");
    if (curraddr <= 0x1FF)
      wsprintf(outtext,TEXT("%04X"),curraddr);
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outtext,_tcslen(outtext),NULL);
    linerect.left   = SCREENSPLIT1+40;
    linerect.right  = SCREENSPLIT2;
    SetTextColor(dc,color[COLOR_DATATEXT]);
    if (curraddr <= 0x1FF)
      wsprintf(outtext,TEXT("%02X"),(unsigned)*(LPBYTE)(mem+curraddr));
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outtext,_tcslen(outtext),NULL);
    loop++;
  }
}

void DrawTargets (HDC dc, int line) {
  int address[2];
  BOOL ind = 0;
  GetTargets(&address[0],&address[1],&ind);
  int loop = 2;
  while (loop--) {
    TCHAR addressstr[8] = TEXT("");
    TCHAR valuestr[8]   = TEXT("");
    if ((address[loop] & 0xFF00) == (iopage & 0xFF00)) {
      wsprintf(addressstr,TEXT("%04X"),address[loop]);
	  wsprintf(valuestr,TEXT("io page"));
	  address[loop] = -1;
	}
    if (address[loop] >= 0) {
      wsprintf(addressstr,TEXT("%04X"),address[loop]);
      if (loop)
        wsprintf(valuestr,TEXT("%02X"),*(LPBYTE)(mem+address[loop]));
      else
		if (ind == 99 && (address[loop] == 0xFF)) {
			wsprintf(valuestr,TEXT("%04X"), (WORD)(mem+(*(mem+0xFF) + (*(mem)*256))));
		}
		else
			wsprintf(valuestr,TEXT("%04X"),*(LPWORD)(mem+address[loop]));
    }
    if (!loop && !ind)
        _tcscpy(valuestr, TEXT(""));

	RECT linerect;
    linerect.left   = SCREENSPLIT1;
    linerect.top    = 45+((line+loop) << 4);
    linerect.right  = SCREENSPLIT1+40;
    linerect.bottom = linerect.top+16;
    SetTextColor(dc,color[COLOR_STATIC]);
    SetBkColor(dc,color[COLOR_DATABKG]);
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               addressstr,_tcslen(addressstr),NULL);
    linerect.left  = SCREENSPLIT1+40;
    linerect.right = SCREENSPLIT2;
    SetTextColor(dc,color[COLOR_DATATEXT]);
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               valuestr,_tcslen(valuestr),NULL);
  }
}

void DrawWatches (HDC dc, int line) {
  RECT linerect;
  linerect.left   = SCREENSPLIT2;
  linerect.top    = 45+(line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  TCHAR outstr[16] = TEXT("Watches");
  SetTextColor(dc,color[COLOR_STATIC]);
  SetBkColor(dc,color[COLOR_DATABKG]);
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             outstr,_tcslen(outstr),NULL);
  linerect.right = SCREENSPLIT2+64;
  int loop = 0;
  while (loop < WATCHES) {
    if (watch[loop] >= 0)
      wsprintf(outstr,TEXT("%d: %04X"),loop+1,watch[loop]);
    else
      outstr[0] = 0;
    linerect.top    += 16;
    linerect.bottom += 16;
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outstr,_tcslen(outstr),NULL);
    loop++;
  }
  linerect.left   = SCREENSPLIT2+64;
  linerect.top    = 45+(line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  SetTextColor(dc,color[COLOR_DATATEXT]);
  loop = 0;
  while (loop < WATCHES) {
    if (watch[loop] >= 0)

       if ((watch[loop] & 0xFF00) == (iopage & 0xFF00)) {
				wsprintf(outstr,TEXT("%02X"),ioread[watch[loop] & 0xFF]((BYTE)(watch[loop] & 0xff),0));
			} else {
				wsprintf(outstr,TEXT("%02X"),(unsigned)*(LPBYTE)(mem+watch[loop]));
			}

    else
      outstr[0] = 0;
    linerect.top    += 16;
    linerect.bottom += 16;
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outstr,_tcslen(outstr),NULL);
    loop++;
  }
}

BOOL ExecuteCommand (int args) {
  LPTSTR name = _tcstok(commandstring[0],TEXT(" ,-="));
  if (!name)
    name = commandstring[0];
  int         found    = 0;
  cmdfunction function = NULL;
  int         length   = _tcslen(name);
  int         loop     = 0;
  while ((loop < COMMANDS) && (name[0] >= command[loop].name[0])) {
    if (!_tcsncmp(name,command[loop].name,length)) {
      function = command[loop].function;
      if (!_tcscmp(name,command[loop].name)) {
        found = 1;
        loop  = COMMANDS;
      }
      else
        found++;
    }
    loop++;
  }
  if (found > 1)
    return DisplayError(TEXT("Ambiguous command"));
  else if (function)
    return function(args);
  else
    return DisplayError(TEXT("Illegal command"));
}

void FreeSymbolTable () {
  if (symboltable)
    VirtualFree(symboltable,0,MEM_RELEASE);
  symbolnum   = 0;
  symboltable = NULL;
}

WORD GetAddress (LPCTSTR symbol) {
  int loop = symbolnum;
  while (loop--)
    if (!_tcsicmp(symboltable[loop].name,symbol))
      return symboltable[loop].value;
  return 0;
}

LPCTSTR GetSymbol (WORD address, int bytes) {

  // PERFORM A BINARY SEARCH THROUGH THE SYMBOL TABLE LOOKING FOR A VALUE
  // MATCHING THIS ADDRESS

  if (symboltable) 	{

    int lowlimit  = -1;
    int highlimit = symbolnum;
    int curr      = symbolnum >> 1;
    do {
      int diff = ((int)address)-((int)symboltable[curr].value);
      if (diff < 0) {
        highlimit = curr;
        curr = lowlimit+((curr-lowlimit) >> 1);
      }
      else if (diff > 0) {
        lowlimit = curr;
        curr = curr+((highlimit+1-curr) >> 1);
      }
      else
        return symboltable[curr].name;
    } while ((curr > lowlimit) && (curr < highlimit));
  }

  // IF THERE IS NO SYMBOL FOR THIS ADDRESS, THEN JUST RETURN A STRING
  // CONTAINING THE ADDRESS NUMBER
  static TCHAR buffer[8];
  switch (bytes) {
    case 2:   wsprintf(buffer,TEXT("$%02X"),(unsigned)address);  break;
    case 3:   wsprintf(buffer,TEXT("$%04X"),(unsigned)address);  break;
    default:  buffer[0] = 0;                                     break;
  }
  return buffer;

}

void GetTargets (int *intermediate, int *final, BOOL *ind ) {
  *intermediate = -1;
  *final        = -1;
  *ind	    =  1;
  int  addrmode   = instruction[*(mem+regs.pc)].addrmode;
  BYTE argument8  = *(LPBYTE)(mem+regs.pc+1);
  WORD argument16 = *(LPWORD)(mem+regs.pc+1);
  switch (addrmode) {

    case ADDR_ABS:
      *final = argument16;
      break;

    case ADDR_ABSIINDX:
      argument16   += regs.x;
      *intermediate = argument16;
      *final        = *(LPWORD)(mem+*intermediate);
      break;

    case ADDR_ABSX:
      *intermediate = argument16;
	  *ind = 0;
	  argument16  += regs.x;
	  *final       = argument16;
      break;

    case ADDR_ABSY:
      *intermediate = argument16;
	  *ind = 0;
	  argument16  += regs.y;
      *final       = argument16;
      break;

    case ADDR_BBREL:
      *final = argument8;
      break;

    case ADDR_IABS:
      *intermediate = argument16;
      *final        = *(LPWORD)(mem+*intermediate);
      break;

    case ADDR_INDX:
      argument8    += regs.x;
      *intermediate = argument8;
      *final        = *(LPWORD)(mem+*intermediate);
      break;

    case ADDR_INDY:
      *intermediate = argument8;
      *final        = (*(LPWORD)(mem+*intermediate))+regs.y;
      break;

    case ADDR_IZPG:
      *intermediate = argument8;
      *final        = *(LPWORD)(mem+*intermediate);

      if (argument8 == 0xFF)  {
	  		*final = (WORD)(mem+(*(mem+0xFF) + (*(mem)*256)));
			*ind = 99;
		}
      break;

    case ADDR_ZPG:
      *final = argument8;
      break;

    case ADDR_ZPGX:
      *intermediate = argument8;
	  *ind = 0;
      *final = (argument8+regs.x) & 0xff;
      break;

    case ADDR_ZPGY:
	  *intermediate = argument8;
	  *ind = 0;
      *final = (argument8+regs.y) & 0xff;
      break;

  }
  if ((*final >= 0) &&
      ((!_tcscmp(instruction[*(mem+regs.pc)].mnemonic,TEXT("JMP"))) ||
       (!_tcscmp(instruction[*(mem+regs.pc)].mnemonic,TEXT("JSR")))))
    *final = -1;
}

BOOL InternalSingleStep () {
  BOOL result = 0;
//  try {
    ++profiledata[*(mem+regs.pc)];
    ecycles = CpuExecute();
    if (Displayflag && viewingoutput) TerminalDisplay(0);
	result = 1;
//  }
//  except (EXCEPTION_EXECUTE_HANDLER) {
//    result = 0;
//  }
  return result;
}

void OutputTraceLine () {
  TCHAR disassembly[50];
  TCHAR flags[9];

  DrawDisassembly((HDC)0,0,lastpc,disassembly);
  DrawFlags((HDC)0,0,regs.ps,flags);

  _ftprintf(tracefile,
            TEXT("%-40s a=%02x x=%02x y=%02x sp=%03x ps=%s clk=%02x "),
            (LPCTSTR)disassembly,
			(unsigned)regs.a,
            (unsigned)regs.x,
            (unsigned)regs.y,
            (unsigned)regs.sp,
            (LPCTSTR)flags,
			ecycles);
  int loop = 0;
  while (loop < WATCHES) {
    if (watch[loop] >= 0)
       _ftprintf(tracefile,
	  	    TEXT("%04X:%02X "),watch[loop],*(mem+watch[loop]));
	loop++;
  }
  _ftprintf(tracefile,TEXT("\n"));

}

int ParseCommandString () {
  int    args    = 0;
  LPTSTR currptr = commandstring[0];
  while (*currptr) {
    LPTSTR   endptr = NULL;
    unsigned length = _tcslen(currptr);
    _tcstok(currptr,TEXT(" ,-="));
    _tcsncpy(arg[args].str,currptr,11);
    arg[args].str[11] = 0;
    arg[args].val1    = (WORD)(_tcstoul(currptr,&endptr,16) & 0xFFFF);
    if (endptr)
      if (*endptr == TEXT('L')) {
        arg[args].val2 = (WORD)(_tcstoul(endptr+1,&endptr,16) & 0xFFFF);
        if (endptr && *endptr)
          arg[args].val2 = 0;
      }
      else {
        arg[args].val2 = 0;
        if (*endptr)
          arg[args].val1 = 0;
      }
    else
      arg[args].val2 = 0;
    BOOL more = ((*currptr) && (length > _tcslen(currptr)));
    args    += more;
    currptr += _tcslen(currptr)+more;
  }
  int loop = args;
  while (loop++ < MAXARGS-1) {
    arg[loop].str[0] = 0;
    arg[loop].val1   = 0;
    arg[loop].val2   = 0;
  }
  return args;
}

void WriteProfileData () {
  TCHAR filename[MAX_PATH];
  _tcscpy(filename,progdir);
  _tcscat(filename,TEXT("Profile.txt"));
  FILE *file = fopen(filename,TEXT("wt"));
  if (file) {
    DWORD maxvalue;
    do {
      maxvalue = 0;
      DWORD maxitem;
      DWORD loop;
      for (loop = 0; loop < 256; ++loop)
        if (profiledata[loop] > maxvalue) {
          maxvalue = profiledata[loop];
          maxitem  = loop;
        }
      if (maxvalue) {
        fprintf(file,
                TEXT("%9u  %02X  %s\n"),
                (unsigned)maxvalue,
                (unsigned)maxitem,
                (LPCTSTR)instruction[maxitem].mnemonic);
        profiledata[maxitem] = 0;
      }
    } while (maxvalue);
    fclose(file);
  }
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

void DebugBegin () {
  mode = MODE_DEBUG;
  addressmode[INVALID2].bytes = 2;
  addressmode[INVALID3].bytes = 3;
  ComputeTopOffset(regs.pc);
  DebugDisplay(1);
}

void DebugContinueStepping () {
  if (stepcount) {
    lastpc = regs.pc;
    InternalSingleStep();
	if (tracefile)
      OutputTraceLine();
	DrawStatusArea((HDC)0,0);
    if ((regs.pc == stepuntil) || CheckBreakpoint(regs.pc,1))
      stepcount = 0;
    else if (stepcount > 0)
      stepcount--;
  }
  if (stepcount) {
      if (!viewingoutput) CmdViewOutput(1);
  }
  else {
    mode = MODE_DEBUG;
    ComputeTopOffset(regs.pc);
    DebugDisplay(1);
    FrameRefreshStatus ();
  }
}

void DebugDestroy () {
  DebugEnd();
  DeleteObject(debugfont);
  FreeSymbolTable();
}

void DebugDisplay (BOOL drawbackground) {
  HDC dc = FrameGetDC();
  SelectObject(dc,debugfont);
  SetTextAlign(dc,TA_TOP | TA_LEFT);

  // DRAW THE BACKGROUND
  if (drawbackground) {
    RECT viewportrect;
    viewportrect.left   = 0;
    viewportrect.top    = 45;
    viewportrect.right  = SCREENSPLIT1-14;
    viewportrect.bottom = 304+45;
    SetBkColor(dc,color[COLOR_INSTBKG]);
    ExtTextOut(dc,0,0,ETO_OPAQUE,&viewportrect,TEXT(""),0,NULL);
    viewportrect.left   = SCREENSPLIT1-14;
    viewportrect.right  = 560;
    SetBkColor(dc,color[COLOR_DATABKG]);
    ExtTextOut(dc,0,0,ETO_OPAQUE,&viewportrect,TEXT(""),0,NULL);
  }

  // DRAW DISASSEMBLED LINES
  {
    int  line   = 0;
    WORD offset = topoffset;
    while (line < SOURCELINES) {
      offset += DrawDisassembly(dc,line,offset,NULL);
      line++;
    }
  }

  // DRAW THE DATA AREA
  DrawStack(dc,0);
  DrawTargets(dc,10);
  DrawRegister(dc,13,TEXT("A") ,1,regs.a);
  DrawRegister(dc,14,TEXT("X") ,1,regs.x);
  DrawRegister(dc,15,TEXT("Y") ,1,regs.y);
  DrawRegister(dc,16,TEXT("PC"),2,regs.pc);
  DrawRegister(dc,17,TEXT("SP"),2,regs.sp);
  DrawFlags(dc,18,regs.ps,NULL);
  if (usingbp)
    DrawBreakpoints(dc,0);
  if (usingwatches)
    DrawWatches(dc,7);
  if (usingmemdump)
    DrawMemory(dc,14);

  // DRAW THE COMMAND LINE
  {
    int line = COMMANDLINES;
    while (line--)
      DrawCommandLine(dc,line);
  }

  FrameReleaseDC(dc);
}

void DebugEnd () {
  if (profiling)
    WriteProfileData();
  if (tracefile) {
    fclose(tracefile);
    tracefile = NULL;
  }

}

void DebugInitialize () {

  // CLEAR THE BREAKPOINT AND WATCH TABLES
  ZeroMemory(breakpoint,BREAKPOINTS*sizeof(bprec));
  {
    int loop = 0;
    while (loop < WATCHES)
      watch[loop++] = -1;
  }

  // READ IN THE SYMBOL TABLE
  {
    TCHAR filename[MAX_PATH];
	TCHAR symfilename[MAX_PATH];
    _tcscpy(filename,progdir);
    _tcscpy(symfilename,ROMfile);
    int loop = _tcslen(symfilename);
    while (loop--)
      if (symfilename[loop] == TEXT('.')) {
        symfilename[loop] = 0;
        break;
	  }
	_tcscat(filename,symfilename);
	_tcscat(filename,TEXT(".sym"));

	int   symbolalloc = 0;
    FILE *infile      = fopen(filename,"rt");
    WORD  lastvalue   = 0;
    if (infile) {
      while (!feof(infile)) {

        // READ IN THE NEXT LINE, AND MAKE SURE IT IS SORTED CORRECTLY IN
        // VALUE ORDER
        DWORD value     = 0;
        char  name[14]  = "";
        char  line[256];
        fscanf(infile,"%x %13s",&value,name);
        fgets(line,255,infile);
        if (value)
          if (value < lastvalue) {
            MessageBox(GetDesktopWindow(),
                       TEXT("The symbol file is not sorted correctly.  ")
                       TEXT("Symbols will not be loaded."),
                       TITLE,
                       MB_ICONEXCLAMATION | MB_SETFOREGROUND);
            FreeSymbolTable();
            return;
          }
          else {

            // IF OUR CURRENT SYMBOL TABLE IS NOT BIG ENOUGH TO HOLD THIS
            // ADDITIONAL SYMBOL, THEN ALLOCATE A BIGGER TABLE AND COPY THE
            // CURRENT DATA ACROSS
            if ((!symboltable) || (symbolalloc <= symbolnum)) {
              symbolalloc += 8192/sizeof(symbolrec);
              symbolptr newtable = (symbolptr)VirtualAlloc(NULL,
                                                           symbolalloc*sizeof(symbolrec),
                                                           MEM_COMMIT,
                                                           PAGE_READWRITE);
              if (newtable) {
                if (symboltable) {
                  CopyMemory(newtable,symboltable,symbolnum*sizeof(symbolrec));
                  VirtualFree(symboltable,0,MEM_RELEASE);
                }
                symboltable = newtable;
              }
              else {
                MessageBox(GetDesktopWindow(),
                           TEXT("There is not enough memory available to load ")
                           TEXT("the symbol file."),
                           TITLE,
                           MB_ICONEXCLAMATION | MB_SETFOREGROUND);
                FreeSymbolTable();
              }
            }

            // SAVE THE NEW SYMBOL IN THE SYMBOL TABLE
            if (symboltable) {
              symboltable[symbolnum].value = (WORD)(value & 0xFFFF);
#ifdef UNICODE
              mbstowcs(symboltable[symbolnum].name,name,12);
#else
              strncpy(symboltable[symbolnum].name,name,12);
#endif
              symboltable[symbolnum].name[13] = 0;
              symbolnum++;
            }

            lastvalue = (WORD)value;
          }

      }
      fclose(infile);
    }
  }

  // CREATE A FONT FOR THE DEBUGGING SCREEN
  debugfont = CreateFont(15,0,0,0,FW_MEDIUM,0,0,0,OEM_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,FIXED_PITCH | 4 | FF_MODERN,
                         TEXT("Courier New"));

}

void DebugProcessChar (TCHAR ch) {
  if (mode != MODE_DEBUG)
    return;
  if ((ch == TEXT(' ')) && !commandstring[0][0])
    return;
  if ((ch >= 32) && (ch <= 126)) {
    ch = (TCHAR)CharUpper((LPTSTR)ch);
    int length = _tcslen(commandstring[0]);
    if (length < 68) {
      commandstring[0][length]   = ch;
      commandstring[0][length+1] = 0;
    }
    HDC dc = FrameGetDC();
    DrawCommandLine(dc,0);
    FrameReleaseDC(dc);
  }
}

void DebugProcessCommand (int keycode) {
  if ((mode == MODE_STEPPING) && (keycode == VK_ESCAPE))
    stepcount = 0;

  if ((mode == MODE_DEBUG) && (keycode == VK_ESCAPE))  {
    DebugEnd();
    mode = MODE_RUNNING;
	ClearCounters();
	TerminalDisplay(1);
	FrameRefreshStatus ();
  }


  if (mode != MODE_DEBUG)
    return;
  if (viewingoutput) {
    DebugDisplay(1);
    viewingoutput = 0;
    return;
  }
  BOOL needscmdrefresh  = 0;
  BOOL needsfullrefresh = 0;
  if ((keycode == VK_SPACE) && commandstring[0][0])
    return;
  if (keycode == VK_BACK) {
    int length = _tcslen(commandstring[0]);
    if (length)
      commandstring[0][length-1] = 0;
    needscmdrefresh = 1;
  }
  else if (keycode == VK_RETURN) {
    if ((!commandstring[0][0]) &&
        (commandstring[1][0] != TEXT(' ')))
      _tcscpy(commandstring[0],commandstring[1]);
    int loop = COMMANDLINES-1;
    while (loop--)
      _tcscpy(commandstring[loop+1],commandstring[loop]);
    needscmdrefresh  = COMMANDLINES;
    needsfullrefresh = ExecuteCommand(ParseCommandString());
    commandstring[0][0] = 0;
  }
  else switch (keycode) {
    case VK_SPACE:  needsfullrefresh = CmdTrace(0);     break;
    case VK_PRIOR:  needsfullrefresh = CmdPageUp(0);    break;
    case VK_NEXT:   needsfullrefresh = CmdPageDown(0);  break;
    case VK_UP:     needsfullrefresh = CmdLineUp(0);    break;
    case VK_DOWN:   needsfullrefresh = CmdLineDown(0);  break;
  }

  if (needsfullrefresh)
    DebugDisplay(0);
  else if (needscmdrefresh && (!viewingoutput) &&
           ((mode == MODE_DEBUG) || (mode == MODE_STEPPING))) {
    HDC dc = FrameGetDC();
    while (needscmdrefresh--)
		DrawCommandLine(dc,needscmdrefresh);
    FrameReleaseDC(dc);
  }
}

