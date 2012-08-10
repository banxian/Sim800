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

#define  TITLE             TEXT("65C02 Simulator")
#define  VERSION           TEXT(" v2.10 (Apr 20, 2005)")
#define  MODE_LOGO         0
#define  MODE_PAUSED       1
#define  MODE_RUNNING      2
#define  MODE_DEBUG        3
#define  MODE_STEPPING     4

#define  SPEED_NORMAL      10
#define  SPEED_MAX         250

#define  VIEWPORTX         5
#define  VIEWPORTY         5

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
// extern BYTE pra;
// extern BYTE prb;
// extern BYTE ira;
// extern BYTE irb;
// extern BYTE lira;
// extern BYTE lirb;
// extern BYTE ddra;
// extern BYTE ddrb;
// extern BYTE ca1;
// extern BYTE ca2;
// extern BYTE cb1;
// extern BYTE cb2;
// extern BYTE t1cl;
// extern BYTE t1ch;
// extern BYTE t1ll;
// extern BYTE t1lh;
// extern BYTE t2ll;
// extern BYTE t2cl;
// extern BYTE t2ch;
// extern BYTE sr;
// extern BYTE acr;
// extern BYTE pcr;
// extern BYTE ifr;
// extern BYTE ier;
// extern BYTE t1ctl;
// extern BYTE t2ctl;
// extern BYTE pb6;
//extern BYTE pb7;


void    CommDestroy ();
void    CommReset ();
void    CommSetSerialPort (HWND,DWORD);
void    CommUpdate (DWORD);
DWORD   CpuExecute ();
void    CpuInitialize ();
void    DebugBegin ();
void    DebugContinueStepping ();
void    DebugDestroy ();
void    DebugDisplay (BOOL);
void    DebugEnd ();
void    DebugInitialize ();
void    DebugProcessChar (TCHAR);
void    DebugProcessCommand (int);
void    FrameCreateWindow ();
HDC     FrameGetDC ();
void    FrameRefreshStatus ();
void    FrameRegisterClass ();
void    FrameReleaseDC (HDC);
void    DrawStatusArea(HDC,int);
void    DrawIOArea(HDC,int);
void    KeybQueueKeypress (int,BOOL);
void    MemDestroy ();
void    MemInitialize ();
void    MemReset ();
BOOL    LoadReg();
void    SaveReg();
void    TermInitialize ();
void    TerminalDisplay (BOOL drawbackground);
void    ClearCounters(void);
void	w65c22init(int);
void	w65c22upd(DWORD);
void    w65c22pins(WORD);


BYTE __stdcall CommCommandRD (BYTE,BYTE);
BYTE __stdcall CommCommandWR (BYTE,BYTE);
BYTE __stdcall CommControlRD (BYTE,BYTE);
BYTE __stdcall CommControlWR (BYTE,BYTE);
BYTE __stdcall CommReceive (BYTE,BYTE);
BYTE __stdcall CommStatus (BYTE,BYTE);
BYTE __stdcall CommTransmit (BYTE,BYTE);
BYTE __stdcall KeybReadData (BYTE,BYTE);
BYTE __stdcall KeybReadFlag (BYTE,BYTE);
BYTE __stdcall TerminalOutputWR (BYTE,BYTE);
BYTE __stdcall TerminalOutputRD (BYTE,BYTE);
BYTE __stdcall LTPDataPort (BYTE,BYTE);
BYTE __stdcall LTPStatusPort (BYTE,BYTE);
BYTE __stdcall LTPCommandPort (BYTE,BYTE);
BYTE __stdcall SpkrToggle (BYTE,BYTE);
BYTE __stdcall w65c22write (BYTE,BYTE);
BYTE __stdcall w65c22read (BYTE,BYTE);
BYTE __stdcall w65c22loop (BYTE,BYTE);

BYTE __stdcall CommCommandRD (BYTE,BYTE);

unsigned char GetByte( unsigned short address );
unsigned short GetWord( unsigned short address );
