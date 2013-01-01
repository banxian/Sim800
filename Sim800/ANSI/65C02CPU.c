#ifndef HANDYPSP

#include "65c02.h"

#define  AF_SIGN       0x80
#define  AF_OVERFLOW   0x40
#define  AF_RESERVED   0x20
#define  AF_BREAK      0x10
#define  AF_DECIMAL    0x08
#define  AF_INTERRUPT  0x04
#define  AF_ZERO       0x02
#define  AF_CARRY      0x01

/****************************************************************************
*
*  GENERAL PURPOSE MACROS
*
***/

#define AF_TO_EF  flagc = (regs.ps & AF_CARRY);                             \
                  flagn = (regs.ps & AF_SIGN);                              \
                  flagv = (regs.ps & AF_OVERFLOW);                          \
                  flagz = (regs.ps & AF_ZERO);
#define EF_TO_AF  regs.ps = (regs.ps & ~(AF_CARRY | AF_SIGN |               \
                                         AF_OVERFLOW | AF_ZERO))            \
                              | (flagc ? AF_CARRY    : 0)                   \
                              | (flagn ? AF_SIGN     : 0)                   \
                              | (flagv ? AF_OVERFLOW : 0)                   \
                              | (flagz ? AF_ZERO     : 0);
#define CMOS      { }
#define CYC(a)   cycles += a;

// only address less than 1FFF can access to fixed ram

#define POP      (*(fixedram0000+((regs.sp >= 0x01FF) ? (regs.sp = 0x100) : ++regs.sp)))
#define PUSH(a)  *(fixedram0000+regs.sp--) = (a);                                    \
                 if (regs.sp < 0x100)                                       \
                   regs.sp = 0x1FF;
#define READ     ((addr < iorange)                                          \
                    ? ioread[addr & 0xFF]((BYTE)(addr & 0xff))            \
                    : *(pmemmap[addr >> 0xD] + (addr & 0x1FFF)))
#define SETNZ(a) {                                                          \
                   flagn = ((a) & 0x80);                                    \
                   flagz = !(a & 0xFF);                                     \
                 }
#define SETZ(a)  flagz = !(a & 0xFF);
#define TOBCD(a) (((((a)/10) % 10) << 4) | ((a) % 10))
#define TOBIN(a) (((a) >> 4)*10 + ((a) & 0x0F))
// Check flash
#define WRITE(a) { if ((addr >= iorange)) { \
                     if ((addr < 0x4000)) { \
                       *(pmemmap[addr >> 0xD] + (addr & 0x1FFF)) = (BYTE)(a); \
                     } else { \
                       checkflashprogram(addr, (BYTE)(a)); \
                     } \
                   }  else                                                     \
                     iowrite[addr & 0xFF]((BYTE)(addr & 0xff),(BYTE)(a));   \
                 }
// dangerous!! FFFE/FFFF may not in same stripe
// remove/add AF_BREAK to ps can not restore original AF_BREAK
/*regs.ps |= AF_BREAK;*/
//SEI not in stack (dangerous!)
#define IRQ  {  if (g_wai) {regs.pc++; g_wai = 0;}				    \
				if (!(regs.ps & AF_INTERRUPT)) { 				    \
			        PUSH(regs.pc >> 8)                                     \
                     PUSH(regs.pc & 0xFF)                                   \
			   EF_TO_AF      												\
                     regs.ps &= ~AF_BREAK;                                  \
                     PUSH(regs.ps)                                          \
  			   regs.pc = *(LPWORD)(pmemmap[7]+0x1FFE); CYC(7)               \
                    SEI                                                    \
                } 									    \
			 }
// TODO: avoid cross stripe using SHR |
#define NMI  {   if (g_wai) {regs.pc++; g_wai = 0; }                           \
			     PUSH(regs.pc >> 8)                                        \
                 PUSH(regs.pc & 0xFF)                                       \
				 SEI                                                \
				 EF_TO_AF                                           \
                 PUSH(regs.ps)                                              \
				 regs.pc = *(LPWORD)(pmemmap[7]+0x1FFA);                   \
				 g_nmi = 0; CYC(7) /* MERGE */						    \
			 }

/****************************************************************************
*
*  ADDRESSING MODE MACROS
*
***/


// BUG REPORT!!! IND ZP when set to $FF will get hi byte from $0100 vs. $0000 !!!!

#define ABS      addr = GetWord(regs.pc);  regs.pc += 2;
#define ABSIINDX addr = GetWord(GetWord(regs.pc)+(WORD)regs.x);  regs.pc += 2;
#define ABSX     addr = GetWord(regs.pc)+(WORD)regs.x;  regs.pc += 2;
#define ABSY     addr = GetWord(regs.pc)+(WORD)regs.y;  regs.pc += 2;
#define IABS     addr = GetWord(GetWord(regs.pc));  regs.pc += 2;
#define ACC      { }
#define IMM      addr = regs.pc++;
#define IMPLIED  { }
#define REL      addr = (signed char)GetByte(regs.pc++);
#define STACK    { }
// TODO: optimize Zeropage
#define ZPG      addr = GetByte(regs.pc++);
#define INDX     addr = GetWord((GetByte(regs.pc++)+regs.x) & 0xFF);
#define ZPGX     addr = (GetByte(regs.pc++)+regs.x) & 0xFF;
#define ZPGY     addr = (GetByte(regs.pc++)+regs.y) & 0xFF;
// #define IZPG     addr = *(LPWORD)(mem+*(mem+regs.pc++));
#define INDY     addr = GetWord(GetByte(regs.pc++))+(WORD)regs.y;

// optimized?
#define IZPG     if (GetByte(regs.pc) == 0xFF) {								\
					addr = *(fixedram0000+0xFF) + (*(fixedram0000)*256); 						\
					regs.pc++;												\
					}														\
				 else														\
	  				addr = GetWord(GetByte(regs.pc++));



/****************************************************************************
*
*  INSTRUCTION MACROS
*
***/

#define ADC      temp = READ;                                               \
                 if (regs.ps & AF_DECIMAL) {                                \
                   val    = TOBIN(regs.a)+TOBIN(temp)+(flagc != 0);         \
                   flagc  = (val > 99);                                     \
                   regs.a = TOBCD(val); CYC(1)                              \
                   SETNZ(regs.a);         /* check */                       \
                 }                                                          \
                 else {                                                     \
                   val    = regs.a+temp+(flagc != 0);                       \
                   flagc  = (val > 0xFF);                                   \
                   flagv  = (((regs.a & 0x80) == (temp & 0x80)) &&          \
                             ((regs.a & 0x80) != (val & 0x80)));            \
                   regs.a = val & 0xFF;                                     \
                   SETNZ(regs.a);                                           \
                 }
#define AND      regs.a &= READ;                                            \
                 SETNZ(regs.a)
#define ASL      val   = READ << 1;                                         \
                 flagc = (val > 0xFF);                                      \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define ASLA     val   = regs.a << 1;                                       \
                 flagc = (val > 0xFF);                                      \
                 SETNZ(val)                                                 \
                 regs.a = (BYTE)val;
#define BBR0     val = READ & 0x01; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR1     val = READ & 0x02; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR2     val = READ & 0x04; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR3     val = READ & 0x08; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR4     val = READ & 0x10; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR5     val = READ & 0x20; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR6     val = READ & 0x40; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBR7     val = READ & 0x80; REL                                     \
                 if (!val) { regs.pc += addr; CYC(1) }
#define BBS0     val = READ & 0x01; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS1     val = READ & 0x02; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS2     val = READ & 0x04; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS3     val = READ & 0x08; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS4     val = READ & 0x10; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS5     val = READ & 0x20; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS6     val = READ & 0x40; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BBS7     val = READ & 0x80; REL                                     \
                 if (val) { regs.pc += addr; CYC(1) }
#define BCC      if (!flagc) { regs.pc += addr; CYC(1) }
#define BCS      if ( flagc) { regs.pc += addr; CYC(1) }
#define BEQ      if ( flagz) { regs.pc += addr; CYC(1) }
#define BIT      val   = READ;                                              \
                 flagz = !(regs.a & val);                                   \
                 flagn = val & 0x80;                                        \
                 flagv = val & 0x40;
#define BITI     flagz = !(regs.a & READ);
#define BMI      if ( flagn) { regs.pc += addr; CYC(1) }
#define BNE      if (!flagz) { regs.pc += addr; CYC(1) }
#define BPL      if (!flagn) { regs.pc += addr; CYC(1) }
#define BRA      regs.pc += addr;
// assume 1FFE/1FFF in same stripe
// add AF_BREAK to real ps is wrong style
#define BRK      PUSH(++regs.pc >> 8)                                       \
                 PUSH(regs.pc & 0xFF)                                       \
                 EF_TO_AF                                                   \
                 regs.ps |= AF_BREAK;                                       \
                 PUSH(regs.ps)                                              \
                 regs.ps |= AF_INTERRUPT;                                   \
                 regs.pc = *(LPWORD)(pmemmap[7]+0x1FFE);
#define BVC      if (!flagv) { regs.pc += addr; CYC(1) }
#define BVS      if ( flagv) { regs.pc += addr; CYC(1) }
#define CLC      flagc = 0;
#define CLD      regs.ps &= ~AF_DECIMAL;
#define CLI      regs.ps &= ~AF_INTERRUPT;
#define CLV      flagv = 0;
#define CMP      val   = READ;                                              \
                 flagc = (regs.a >= val);                                   \
                 val   = regs.a-val;                                        \
                 SETNZ(val)
#define CPX      val   = READ;                                              \
                 flagc = (regs.x >= val);                                   \
                 val   = regs.x-val;                                        \
                 SETNZ(val)
#define CPY      val   = READ;                                              \
                 flagc = (regs.y >= val);                                   \
                 val   = regs.y-val;                                        \
                 SETNZ(val)
#define DEA      --regs.a;                                                  \
                 SETNZ(regs.a)
#define DEC      val = READ-1;                                              \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define DEX      --regs.x;                                                  \
                 SETNZ(regs.x)
#define DEY      --regs.y;                                                  \
                 SETNZ(regs.y)
#define EOR      regs.a ^= READ;                                            \
                 SETNZ(regs.a)
#define INA      ++regs.a;                                                  \
                 SETNZ(regs.a)
#define INC      val = READ+1;                                              \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define INX      ++regs.x;                                                  \
                 SETNZ(regs.x)
#define INY      ++regs.y;                                                  \
                 SETNZ(regs.y)
#define JMP      regs.pc = addr;
#define JSR      --regs.pc;                                                 \
                 PUSH(regs.pc >> 8)                                         \
                 PUSH(regs.pc & 0xFF)                                       \
                 regs.pc = addr;
#define LDA      regs.a = READ;                                             \
                 SETNZ(regs.a)
#define LDX      regs.x = READ;                                             \
                 SETNZ(regs.x)
#define LDY      regs.y = READ;                                             \
                 SETNZ(regs.y)
#define LSR      val   = READ;                                              \
                 flagc = (val & 1);                                         \
                 flagn = 0;                                                 \
                 val >>= 1;                                                 \
                 SETZ(val)                                                  \
                 WRITE(val)
#define LSRA     flagc = (regs.a & 1);                                      \
                 flagn = 0;                                                 \
                 regs.a >>= 1;                                              \
                 SETZ(regs.a)
#define NOP      { }
#define ORA      regs.a |= READ;                                            \
                 SETNZ(regs.a)
#define PHA      PUSH(regs.a)
#define PHP      EF_TO_AF                                                   \
                 regs.ps |= AF_RESERVED;                                    \
                 PUSH(regs.ps)
#define PHX      PUSH(regs.x)
#define PHY      PUSH(regs.y)
#define PLA      regs.a = POP;                                              \
                 SETNZ(regs.a)
// no necessary for PLP?
//regs.ps |= AF_BREAK;										
#define PLP      regs.ps = POP;                                             \
                 AF_TO_EF
#define PLX      regs.x = POP;                                              \
                 SETNZ(regs.x)
#define PLY      regs.y = POP;                                              \
                 SETNZ(regs.y)
#define RMB0     val = READ & 0xFE;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB1     val = READ & 0xFD;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB2     val = READ & 0xFB;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB3     val = READ & 0xF7;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB4     val = READ & 0xEF;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB5     val = READ & 0xDF;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB6     val = READ & 0xBF;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RMB7     val = READ & 0x7F;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define ROL      val   = (READ << 1) | (flagc != 0);                        \
                 flagc = (val > 0xFF);                                      \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define ROLA     val    = (((WORD)regs.a) << 1) | (flagc != 0);             \
                 flagc  = (val > 0xFF);                                     \
                 regs.a = val & 0xFF;                                       \
                 SETNZ(regs.a);
#define ROR      temp  = READ;                                              \
                 val   = (temp >> 1) | (flagc ? 0x80 : 0);                  \
                 flagc = temp & 1;                                          \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RORA     val    = (((WORD)regs.a) >> 1) | (flagc ? 0x80 : 0);       \
                 flagc  = regs.a & 1;                                       \
                 regs.a = val & 0xFF;                                       \
                 SETNZ(regs.a)
// AF_BREAK is not set on wqxsim
//regs.ps |= AF_BREAK;
#define RTI      regs.ps = POP;                                             \
	             CLI														\
				 g_irq = 0; /* MERGE */                                        \
				 AF_TO_EF                                                   \
                 regs.pc = POP;												\
				 regs.pc |= (((WORD)POP) << 8);
#define RTS      regs.pc = POP;                                             \
                 regs.pc |= (((WORD)POP) << 8);                             \
                 ++regs.pc;
#define SBC      temp = READ;                                               \
                 if (regs.ps & AF_DECIMAL) {                                \
                   val    = TOBIN(regs.a)-TOBIN(temp)-!flagc;               \
                   flagc  = (val < 0x8000);                                 \
                   regs.a = TOBCD(val);                                     \
				   SETNZ(regs.a); CYC(1) /* check*/                 \
                 }                                                          \
                 else {                                                     \
                   val    = regs.a-temp-!flagc;                             \
                   flagc  = (val < 0x8000);                                 \
                   flagv  = (((regs.a & 0x80) != (temp & 0x80)) &&          \
                             ((regs.a & 0x80) != (val & 0x80)));            \
                   regs.a = val & 0xFF;                                     \
                   SETNZ(regs.a);                                           \
                 }
#define SEC      flagc = 1;
#define SED      regs.ps |= AF_DECIMAL;
#define SEI      regs.ps |= AF_INTERRUPT;
#define SMB0     val = READ | 0x01;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB1     val = READ | 0x02;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB2     val = READ | 0x04;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB3     val = READ | 0x08;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB4     val = READ | 0x10;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB5     val = READ | 0x20;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB6     val = READ | 0x40;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define SMB7     val = READ | 0x80;                                         \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define STA      WRITE(regs.a)
#define STP      regs.pc--; g_stp=1;
#define STX      WRITE(regs.x)
#define STY      WRITE(regs.y)
#define STZ      WRITE(0)
#define TAX      regs.x = regs.a;                                           \
                 SETNZ(regs.x)
#define TAY      regs.y = regs.a;                                           \
                 SETNZ(regs.y)
#define TRB      val   = READ;                                              \
                 flagz = !(regs.a & val);                                   \
                 val  &= ~regs.a;                                           \
                 WRITE(val)
#define TSB      val   = READ;                                              \
                 flagz = !(regs.a & val);                                   \
                 val   |= regs.a;                                           \
                 WRITE(val)
#define TSX      regs.x = regs.sp & 0xFF;                                   \
                 SETNZ(regs.x)
#define TXA      regs.a = regs.x;                                           \
                 SETNZ(regs.a)
#define TXS      regs.sp = 0x100 | regs.x;
#define TYA      regs.a = regs.y;                                           \
                 SETNZ(regs.a)
#define WAI      regs.pc--; g_wai = 1;
#define INVALID1 { }
#define INVALID2 ++regs.pc;
#define INVALID3 regs.pc += 2;

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

DWORD CpuExecute () {
  WORD addr;
  BOOL flagc;
  BOOL flagn;
  BOOL flagv;
  BOOL flagz;
  WORD temp;
  WORD val;
  DWORD cycles = 0;
  AF_TO_EF					//get flags from regs.ps

/****************************************************************************
*
*  OPCODE TABLE
*
***/
//  do  {
	switch (GetByte(regs.pc++)) {
      case 0x00:       BRK           CYC(7)  break;
      case 0x01:       INDX ORA      CYC(6)  break;
      case 0x02:       INVALID2      CYC(2)  break;
      case 0x03:       INVALID1      CYC(1)  break;
      case 0x04: CMOS  ZPG TSB       CYC(5)  break;
      case 0x05:       ZPG ORA       CYC(3)  break;
      case 0x06:       ZPG ASL       CYC(5)  break;
      case 0x07: CMOS  ZPG RMB0      CYC(5)  break;
      case 0x08:       PHP           CYC(3)  break;
      case 0x09:       IMM ORA       CYC(2)  break;
      case 0x0A:       ASLA          CYC(2)  break;
      case 0x0B:       INVALID1      CYC(1)  break;
      case 0x0C: CMOS  ABS TSB       CYC(6)  break;
      case 0x0D:       ABS ORA       CYC(4)  break;
      case 0x0E:       ABS ASL       CYC(6)  break;
      case 0x0F: CMOS  ZPG BBR0      CYC(5)  break;
      case 0x10:       REL BPL       CYC(2)  break;
      case 0x11:       INDY ORA      CYC(5)  break;
      case 0x12: CMOS  IZPG ORA      CYC(5)  break;
      case 0x13:       INVALID1      CYC(1)  break;
      case 0x14: CMOS  ZPG TRB       CYC(5)  break;
      case 0x15:       ZPGX ORA      CYC(4)  break;
      case 0x16:       ZPGX ASL      CYC(6)  break;
      case 0x17: CMOS  ZPG RMB1      CYC(5)  break;
      case 0x18:       CLC           CYC(2)  break;
      case 0x19:       ABSY ORA      CYC(4)  break;
      case 0x1A: CMOS  INA           CYC(2)  break;
      case 0x1B:       INVALID1      CYC(1)  break;
      case 0x1C: CMOS  ABS TRB       CYC(6)  break;
      case 0x1D:       ABSX ORA      CYC(4)  break;
      case 0x1E:       ABSX ASL      CYC(6)  break;
      case 0x1F: CMOS  ZPG BBR1      CYC(5)  break;
      case 0x20:       ABS JSR       CYC(6)  break;
      case 0x21:       INDX AND      CYC(6)  break;
      case 0x22:       INVALID2      CYC(2)  break;
      case 0x23:       INVALID1      CYC(1)  break;
      case 0x24:       ZPG BIT       CYC(3)  break;
      case 0x25:       ZPG AND       CYC(3)  break;
      case 0x26:       ZPG ROL       CYC(5)  break;
      case 0x27: CMOS  ZPG RMB2      CYC(5)  break;
      case 0x28:       PLP           CYC(4)  break;
      case 0x29:       IMM AND       CYC(2)  break;
      case 0x2A:       ROLA          CYC(2)  break;
      case 0x2B:       INVALID1      CYC(1)  break;
      case 0x2C:       ABS BIT       CYC(4)  break;
      case 0x2D:       ABS AND       CYC(4)  break;
      case 0x2E:       ABS ROL       CYC(6)  break;
      case 0x2F: CMOS  ZPG BBR2      CYC(5)  break;
      case 0x30:       REL BMI       CYC(2)  break;
      case 0x31:       INDY AND      CYC(5)  break;
      case 0x32: CMOS  IZPG AND      CYC(5)  break;
      case 0x33:       INVALID1      CYC(1)  break;
      case 0x34: CMOS  ZPGX BIT      CYC(4)  break;
      case 0x35:       ZPGX AND      CYC(4)  break;
      case 0x36:       ZPGX ROL      CYC(6)  break;
      case 0x37: CMOS  ZPG RMB3      CYC(5)  break;
      case 0x38:       SEC           CYC(2)  break;
      case 0x39:       ABSY AND      CYC(4)  break;
      case 0x3A: CMOS  DEA           CYC(2)  break;
      case 0x3B:       INVALID1      CYC(1)  break;
      case 0x3C: CMOS  ABSX BIT      CYC(4)  break;
      case 0x3D:       ABSX AND      CYC(4)  break;
      case 0x3E:       ABSX ROL      CYC(6)  break;
      case 0x3F: CMOS  ZPG BBR3      CYC(5)  break;
      case 0x40:       RTI           CYC(6)  break;
      case 0x41:       INDX EOR      CYC(6)  break;
      case 0x42:       INVALID2      CYC(2)  break;
      case 0x43:       INVALID1      CYC(1)  break;
      case 0x44:       INVALID2      CYC(3)  break;
      case 0x45:       ZPG EOR       CYC(3)  break;
      case 0x46:       ZPG LSR       CYC(5)  break;
      case 0x47: CMOS  ZPG RMB4      CYC(5)  break;
      case 0x48:       PHA           CYC(3)  break;
      case 0x49:       IMM EOR       CYC(2)  break;
      case 0x4A:       LSRA          CYC(2)  break;
      case 0x4B:       INVALID1      CYC(1)  break;
      case 0x4C:       ABS JMP       CYC(3)  break;
      case 0x4D:       ABS EOR       CYC(4)  break;
      case 0x4E:       ABS LSR       CYC(6)  break;
      case 0x4F: CMOS  ZPG BBR4      CYC(5)  break;
      case 0x50:       REL BVC       CYC(2)  break;
      case 0x51:       INDY EOR      CYC(5)  break;
      case 0x52: CMOS  IZPG EOR      CYC(5)  break;
      case 0x53:       INVALID1      CYC(1)  break;
      case 0x54:       INVALID2      CYC(4)  break;
      case 0x55:       ZPGX EOR      CYC(4)  break;
      case 0x56:       ZPGX LSR      CYC(6)  break;
      case 0x57: CMOS  ZPG RMB5      CYC(5)  break;
      case 0x58:       CLI           CYC(2)  break;
      case 0x59:       ABSY EOR      CYC(4)  break;
      case 0x5A: CMOS  PHY           CYC(3)  break;
      case 0x5B:       INVALID1      CYC(1)  break;
      case 0x5C:       INVALID3      CYC(8)  break;
      case 0x5D:       ABSX EOR      CYC(4)  break;
      case 0x5E:       ABSX LSR      CYC(6)  break;
      case 0x5F: CMOS  ZPG BBR5      CYC(5)  break;
      case 0x60:       RTS           CYC(6)  break;
      case 0x61:       INDX ADC      CYC(6)  break;
      case 0x62:       INVALID2      CYC(2)  break;
      case 0x63:       INVALID1      CYC(1)  break;
      case 0x64: CMOS  ZPG STZ       CYC(3)  break;
      case 0x65:       ZPG ADC       CYC(3)  break;
      case 0x66:       ZPG ROR       CYC(5)  break;
      case 0x67: CMOS  ZPG RMB6      CYC(5)  break;
      case 0x68:       PLA           CYC(4)  break;
      case 0x69:       IMM ADC       CYC(2)  break;
      case 0x6A:       RORA          CYC(2)  break;
      case 0x6B:       INVALID1      CYC(1)  break;
      case 0x6C:       IABS JMP      CYC(6)  break;
      case 0x6D:       ABS ADC       CYC(4)  break;
      case 0x6E:       ABS ROR       CYC(6)  break;
      case 0x6F: CMOS  ZPG BBR6      CYC(5)  break;
      case 0x70:       REL BVS       CYC(2)  break;
      case 0x71:       INDY ADC      CYC(5)  break;
      case 0x72: CMOS  IZPG ADC      CYC(5)  break;
      case 0x73:       INVALID1      CYC(1)  break;
      case 0x74: CMOS  ZPGX STZ      CYC(4)  break;
      case 0x75:       ZPGX ADC      CYC(4)  break;
      case 0x76:       ZPGX ROR      CYC(6)  break;
      case 0x77: CMOS  ZPG RMB7      CYC(5)  break;
      case 0x78:       SEI           CYC(2)  break;
      case 0x79:       ABSY ADC      CYC(4)  break;
      case 0x7A: CMOS  PLY           CYC(4)  break;
      case 0x7B:       INVALID1      CYC(1)  break;
      case 0x7C: CMOS  ABSIINDX JMP  CYC(6)  break;
      case 0x7D:       ABSX ADC      CYC(4)  break;
      case 0x7E:       ABSX ROR      CYC(6)  break;
      case 0x7F: CMOS  ZPG BBR7      CYC(5)  break;
      case 0x80: CMOS  REL BRA       CYC(3)  break;
      case 0x81:       INDX STA      CYC(6)  break;
      case 0x82:       INVALID2      CYC(2)  break;
      case 0x83:       INVALID1      CYC(1)  break;
      case 0x84:       ZPG STY       CYC(3)  break;
      case 0x85:       ZPG
                           STA
                           CYC(3)
                           break;
      case 0x86:       ZPG STX       CYC(3)  break;
      case 0x87: CMOS  ZPG SMB0      CYC(5)  break;
      case 0x88:       DEY           CYC(2)  break;
      case 0x89: CMOS  IMM BITI      CYC(2)  break;
      case 0x8A:       TXA           CYC(2)  break;
      case 0x8B:       INVALID1      CYC(1)  break;
      case 0x8C:       ABS STY       CYC(4)  break;
      case 0x8D:       ABS STA       CYC(4)  break;
      case 0x8E:       ABS STX       CYC(4)  break;
      case 0x8F: CMOS  ZPG BBS0      CYC(5)  break;
      case 0x90:       REL BCC       CYC(2)  break;
      case 0x91:       INDY STA      CYC(6)  break;
      case 0x92: CMOS  IZPG STA      CYC(5)  break;
      case 0x93:       INVALID1      CYC(1)  break;
      case 0x94:       ZPGX STY      CYC(4)  break;
      case 0x95:       ZPGX STA      CYC(4)  break;
      case 0x96:       ZPGY STX      CYC(4)  break;
      case 0x97: CMOS  ZPG SMB1      CYC(5)  break;
      case 0x98:       TYA           CYC(2)  break;
      case 0x99:       ABSY STA      CYC(5)  break;
      case 0x9A:       TXS           CYC(2)  break;
      case 0x9B:       INVALID1      CYC(1)  break;
      case 0x9C: CMOS  ABS STZ       CYC(4)  break;
      case 0x9D:       ABSX STA      CYC(5)  break;
      case 0x9E: CMOS  ABSX STZ      CYC(5)  break;
      case 0x9F: CMOS  ZPG BBS1      CYC(5)  break;
      case 0xA0:       IMM LDY       CYC(2)  break;
      case 0xA1:       INDX LDA      CYC(6)  break;
      case 0xA2:       IMM LDX       CYC(2)  break;
      case 0xA3:       INVALID1      CYC(1)  break;
      case 0xA4:       ZPG LDY       CYC(3)  break;
      case 0xA5:       ZPG LDA       CYC(3)  break;
      case 0xA6:       ZPG LDX       CYC(3)  break;
      case 0xA7: CMOS  ZPG SMB2      CYC(5)  break;
      case 0xA8:       TAY           CYC(2)  break;
      case 0xA9:       IMM LDA       CYC(2)  break;
      case 0xAA:       TAX           CYC(2)  break;
      case 0xAB:       INVALID1      CYC(1)  break;
      case 0xAC:       ABS LDY       CYC(4)  break;
      case 0xAD:       ABS LDA       CYC(4)  break;
      case 0xAE:       ABS LDX       CYC(4)  break;
      case 0xAF: CMOS  ZPG BBS2      CYC(5)  break;
      case 0xB0:       REL BCS       CYC(2)  break;
      case 0xB1:       INDY LDA      CYC(5)  break;
      case 0xB2: CMOS  IZPG LDA      CYC(5)  break;
      case 0xB3:       INVALID1      CYC(1)  break;
      case 0xB4:       ZPGX LDY      CYC(4)  break;
      case 0xB5:       ZPGX LDA      CYC(4)  break;
      case 0xB6:       ZPGY LDX      CYC(4)  break;
      case 0xB7: CMOS  ZPG SMB3      CYC(5)  break;
      case 0xB8:       CLV           CYC(2)  break;
      case 0xB9:       ABSY LDA      CYC(4)  break;
      case 0xBA:       TSX           CYC(2)  break;
      case 0xBB:       INVALID1      CYC(1)  break;
      case 0xBC:       ABSX LDY      CYC(4)  break;
      case 0xBD:       ABSX LDA      CYC(4)  break;
      case 0xBE:       ABSY LDX      CYC(4)  break;
      case 0xBF: CMOS  ZPG BBS3      CYC(5)  break;
      case 0xC0:       IMM CPY       CYC(2)  break;
      case 0xC1:       INDX CMP      CYC(6)  break;
      case 0xC2:       INVALID2      CYC(2)  break;
      case 0xC3:       INVALID1      CYC(1)  break;
      case 0xC4:       ZPG CPY       CYC(3)  break;
      case 0xC5:       ZPG CMP       CYC(3)  break;
      case 0xC6:       ZPG DEC       CYC(5)  break;
      case 0xC7: CMOS  ZPG SMB4      CYC(5)  break;
      case 0xC8:       INY           CYC(2)  break;
      case 0xC9:       IMM CMP       CYC(2)  break;
      case 0xCA:       DEX           CYC(2)  break;
      case 0xCB: CMOS  WAI           CYC(3)  break;
      case 0xCC:       ABS CPY       CYC(4)  break;
      case 0xCD:       ABS CMP       CYC(4)  break;
      case 0xCE:       ABS DEC       CYC(6)  break;
      case 0xCF: CMOS  ZPG BBS4      CYC(5)  break;
      case 0xD0:       REL BNE       CYC(2)  break;
      case 0xD1:       INDY CMP      CYC(5)  break;
      case 0xD2: CMOS  IZPG CMP      CYC(5)  break;
      case 0xD3:       INVALID1      CYC(1)  break;
      case 0xD4:       INVALID2      CYC(4)  break;
      case 0xD5:       ZPGX CMP      CYC(4)  break;
      case 0xD6:       ZPGX DEC      CYC(6)  break;
      case 0xD7: CMOS  ZPG SMB5      CYC(5)  break;
      case 0xD8:       CLD           CYC(2)  break;
      case 0xD9:       ABSY CMP      CYC(4)  break;
      case 0xDA: CMOS  PHX           CYC(3)  break;
      case 0xDB: CMOS  STP           CYC(3)  break;
      case 0xDC:       INVALID3      CYC(4)  break;
      case 0xDD:       ABSX CMP      CYC(4)  break;
      case 0xDE:       ABSX DEC      CYC(6)  break;
      case 0xDF: CMOS  ZPG BBS5      CYC(5)  break;
      case 0xE0:       IMM CPX       CYC(2)  break;
      case 0xE1:       INDX SBC      CYC(6)  break;
      case 0xE2:       INVALID2      CYC(2)  break;
      case 0xE3:       INVALID1      CYC(1)  break;
      case 0xE4:       ZPG CPX       CYC(3)  break;
      case 0xE5:       ZPG SBC       CYC(3)  break;
      case 0xE6:       ZPG INC       CYC(5)  break;
      case 0xE7: CMOS  ZPG SMB6      CYC(5)  break;
      case 0xE8:       INX           CYC(2)  break;
      case 0xE9:       IMM SBC       CYC(2)  break;
      case 0xEA:       NOP           CYC(2)  break;
      case 0xEB:       INVALID1      CYC(1)  break;
      case 0xEC:       ABS CPX       CYC(4)  break;
      case 0xED:       ABS SBC       CYC(4)  break;
      case 0xEE:       ABS INC       CYC(6)  break;
      case 0xEF: CMOS  ZPG BBS6      CYC(5)  break;
      case 0xF0:       REL BEQ       CYC(2)  break;
      case 0xF1:       INDY SBC      CYC(5)  break;
      case 0xF2: CMOS  IZPG SBC      CYC(5)  break;
      case 0xF3:       INVALID1      CYC(1)  break;
      case 0xF4:       INVALID2      CYC(4)  break;
      case 0xF5:       ZPGX SBC      CYC(4)  break;
      case 0xF6:       ZPGX INC      CYC(6)  break;
      case 0xF7: CMOS  ZPG SMB7      CYC(5)  break;
      case 0xF8:       SED           CYC(2)  break;
      case 0xF9:       ABSY SBC      CYC(4)  break;
      case 0xFA: CMOS  PLX           CYC(4)  break;
      case 0xFB:       INVALID1      CYC(1)  break;
      case 0xFC:       INVALID3      CYC(4)  break;
      case 0xFD:       ABSX SBC      CYC(4)  break;
      case 0xFE:       ABSX INC      CYC(6)  break;
      case 0xFF: CMOS  ZPG BBS7      CYC(5)  break;
    }
      if (!g_stp ) {        // If STP, then no IRQ or NMI allowed
	   if (g_nmi) NMI;  // FIXME: NO MORE REVERSE
	   if (g_irq) IRQ;  // FIXME: NO MORE REVERSE
      }
//  } while (cycles < totalcycles);
  EF_TO_AF								// put flags back in regs.ps
  return cycles;
}

void CpuInitialize ()
{
    regs.a  = 0;
    regs.x  = 0;
    regs.y  = 0;
    regs.ps = 0x24;                       // set unused bit 5 to 1 and BRK=0 then irq disable =1
    // assume 1FFC/1FFD in same stripe
    regs.pc = *(LPWORD)(pmemmap[7]+0x1FFC);
    regs.sp = 0x01FF;
    g_irq	  = 0;  // FIXME: NO MORE REVERSE
    g_nmi	  = 0;  // FIXME: NO MORE REVERSE
    g_wai   = 0;
    g_stp   = 0;
}

#endif