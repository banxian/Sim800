//#include <commctrl.h>
#include <conio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <time.h>
#include <windows.h>

//#define  BUILDNUMBER       2     // Rename to 65c02
//#define  BUILDNUMBER       3     // remove registry, use ini file
//#define  BUILDNUMBER       4     // remove disk drives
//#define  BUILDNUMBER       5     // remove joysticks
//#define  BUILDNUMBER       6     // remove video system
//#define  BUILDNUMBER       7     // remove dll libraries
//#define  BUILDNUMBER       8     // debug bad JMP (0000,x)
//#define  BUILDNUMBER       9     // Remove shadowed mem
//#define  BUILDNUMBER       10     // Initial screen output to $C020
//#define  BUILDNUMBER       11     // Define 32K ram, IO page@0x8000, 32K ROM (- 1 page)
//#define  BUILDNUMBER       12     /* Clean out remaining code in all mods
//									remove speaker support, added autoboot choice
//									and ROM selection choices to config */
//#define  BUILDNUMBER       12     // Cleaned up bugs, added status lites, and IOpage to ini
//#define  BUILDNUMBER       13     // Cleaned up bugs, removed unused debugger cmds wrote help file
//#define  BUILDNUMBER       14      // converted buttons from right side to top of screen
//#define  BUILDNUMBER       15      // added irq and nmi inputs and load/save ram to disk
//#define  BUILDNUMBER       16      // corrected mem fill for io page below 0x8000
//#define  BUILDNUMBER       17      // corrected dialog boxes init cursor loc and tab order
//#define  BUILDNUMBER       18      // add ability to cause IRQ's & NMI's based on clock ticks
									// also replaced emulator with simulator throughout text
//#define  BUILDNUMBER       19      // add capture to file and fix ROM loading bug
//#define  BUILDNUMBER       20      // fix REL cmds cycle counting & debug tracefile output
//#define  BUILDNUMBER       21      // added all new W65C02S opcodes
//#define  BUILDNUMBER       22      // Convert source files to allow compiling under LCC
#define  BUILDNUMBER       23       // Fixed bug in debugger that caused program termination
									// when wrapping from $FFFF to $0000, added Tab support in terminal,
									// added instruction throttling to keep cycle timing more accurate
									// removed BUILDNUMBER from the ini config file structure


#define  SPEED_NORMAL      10
#define  SPEED_MAX         250


#define  MAX(a,b)          (((a) > (b)) ? (a) : (b))
#define  MIN(a,b)          (((a) < (b)) ? (a) : (b))

//#define HFINDFILE   HANDLE

typedef BYTE (_stdcall *iofunction1)(BYTE);
typedef void (_stdcall *iofunction2)(BYTE,BYTE);

typedef struct _IMAGE__ { int unused; } *HIMAGE;

typedef struct _regsrec {
  BYTE a;   // accumulator
  BYTE x;   // index X
  BYTE y;   // index Y
  BYTE ps;  // processor status
  WORD pc;  // program counter
  WORD sp;  // stack pointer
} regsrec, *regsptr;

extern DWORD      autoboot;
extern iofunction1 ioread[0x40];
extern iofunction2 iowrite[0x40];

// extern LPBYTE     mem;
extern unsigned char fixedram0000[0x10002]; // just like simulator
extern unsigned char* pmemmap[8]; // 0000~1FFF ... E000~FFFF
extern unsigned char* may4000ptr; // TODO: move into NekoDriver.h
extern unsigned char* norbankheader[0x10];
extern unsigned char* volume0array[0x100];
extern unsigned char* volume1array[0x100];
extern unsigned char* bbsbankheader[0x10];

extern WORD       iopage;
extern WORD       iorange;
extern TCHAR      progdir[MAX_PATH];
extern TCHAR      ROMfile[MAX_PATH];
extern TCHAR      RAMfile[MAX_PATH];
extern regsrec    regs;
extern BOOL       restart;
extern DWORD      speed;
extern DWORD	  totcycles;
extern DWORD      stmsecs;
extern DWORD      executed;
extern int        benchmark;
extern DWORD      throttle;
extern BOOL       irq;
extern BOOL       nmi;
extern BOOL       wai;
extern BOOL       stp;
extern DWORD      irqclk;       /*  used for auto-IRQ  */
extern DWORD      nmiclk;       /*  used for auto-NMI  */



DWORD   CpuExecute ();
void    CpuInitialize ();
void    MemDestroy ();
void    MemInitialize ();
void    MemReset ();
BOOL    LoadReg();
void    SaveReg();
void    TermInitialize ();
void    TerminalDisplay (BOOL drawbackground);
void	w65c22init(int);
void	w65c22upd(DWORD);
void    w65c22pins(WORD);



unsigned char GetByte( unsigned short address );
unsigned short GetWord( unsigned short address );
