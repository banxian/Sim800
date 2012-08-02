extern "C" {
#include "65c02.h"
}
#include <QtCore/QDebug>

DWORD     autoboot          = 0;
HINSTANCE instance          = (HINSTANCE)0;
int       mode              = MODE_LOGO;
TCHAR     progdir[MAX_PATH] = TEXT("");
BOOL      restart           = 0;
DWORD     speed             = 10;
WORD      iopage            = 0x8000;
WORD      iorange           = 0x0040;

DWORD     totcycles			= 0;
DWORD     stmsecs           = 0;
DWORD     executed			= 0;
int       benchmark         = 0;
BOOL      irq			= 1;
BOOL	  nmi				= 1;
BOOL	  stp				= 0;
BOOL	  wai				= 0;
DWORD     irqclk            = 0;
DWORD     irqcnt            = 0;
DWORD     nmiclk            = 0;
DWORD     nmicnt            = 0;
int 	  Capture			= 0;
TCHAR     Capfile[MAX_PATH] = TEXT("");
FILE     *Cfile				= NULL;
DWORD     throttle          = 0;



void ClearCounters() {
    stmsecs = GetTickCount();
    totcycles = 0;
    executed = 0;
}

void DisplayBenchmark (DWORD elapsed) {
// 
//     TCHAR buffer[255];
//     mode = MODE_PAUSED;
//     FrameRefreshStatus ();
//     wsprintf(buffer,
//         TEXT("This benchmark took %u.%02u seconds.\n"
//         "%u clock cycles were processed.\n"
//         "%u opcodes were completed.\n Throttle is set to %u"),
//         (unsigned)(elapsed / 1000),
//         (unsigned)((elapsed / 10) % 100),
//         (unsigned)(totcycles),
//         (unsigned)(executed),
//         (unsigned)(throttle));
//     MessageBox(framewindow,
//         buffer,
//         TEXT("Benchmark Results"),
//         MB_ICONINFORMATION);
//     mode = MODE_RUNNING;
//     FrameRefreshStatus ();
}

extern WORD LogDisassembly (WORD offset, LPTSTR text);

extern bool timer0started;
extern bool timer1started;

void ContinueExecution () {

    DWORD processtime		= totcycles;// needed for comm routines
    DWORD loop				= speed * 300;// watchdog timer
    DWORD elapsed			= 0;
    DWORD CpuTicks          = 0;
    DWORD j = 0;

    if (benchmark == 2) {
        ClearCounters();
        benchmark = 1;
    }

    elapsed = GetTickCount()-stmsecs;

    while (loop && (totcycles < (elapsed * 100 * speed))) {
        //qDebug("PC:0x%04x, opcode: 0x%06x", regs.pc, (*(LPDWORD)(mem+regs.pc)) & 0xFFFFFF);
        //LogDisassembly(regs.pc, NULL);
        CpuTicks = CpuExecute();
        totcycles += CpuTicks;
        executed++;
        //*** add checks for reset, IRQ, NMI, and other pin signals
        elapsed = GetTickCount()-stmsecs;
        loop--;
        /* added for irq timing */
        if (irqclk) {
            irqcnt += CpuTicks;
            if (irqcnt >= irqclk) {
                irq=0;
                irqcnt = irqcnt - irqclk;
            }
        }
        if (nmiclk) {
            nmicnt += CpuTicks;
            if (nmicnt >= nmiclk) {
                nmi=0;
                nmicnt = nmicnt - nmiclk;
            }
        }
        if (timer0started) {
            mem[02] = mem[02] + 1;
        }
        if (timer1started) {
            mem[03] = mem[03] + 1;
        }

        /* Throttling routine (simple delay loop)  */
        if (throttle) for (j=throttle*CpuTicks;j>1;j--);

    }
    /* Throttling update routine   */
    if (throttle) {
        if (totcycles < (elapsed * 100 * speed)) {
            throttle--;
            if (throttle < 1) throttle = 1;
        }
        else {
            throttle++;
        }
    }


//     if (benchmark && (elapsed >= 5000)) {
//         DisplayBenchmark(elapsed);
//         benchmark = 0;
//         ClearCounters();
//     }

//     DrawStatusArea((HDC)0,0);
//     if (Displayflag) TerminalDisplay(0);
//     CommUpdate(totcycles - processtime);

}

void EnterMessageLoop () {
//     MSG message;
//     while (GetMessage(&message,0,0,0)) {
//         TranslateMessage(&message);
//         DispatchMessage(&message);
//         while ((mode == MODE_RUNNING) || (mode == MODE_STEPPING))
//             if (PeekMessage(&message,0,0,0,PM_REMOVE)) {
//                 if (message.message == WM_QUIT)
//                     return;
//                 TranslateMessage(&message);
//                 DispatchMessage(&message);
//             }
//             else if (mode == MODE_STEPPING)
//                 DebugContinueStepping();
//             else {
//                 ContinueExecution();
//             }
//     }
//     while (PeekMessage(&message,0,0,0,PM_REMOVE)) ;
}

void GetProgramDirectory () {
    GetModuleFileName((HINSTANCE)0,progdir,MAX_PATH);
    progdir[MAX_PATH-1] = 0;
    int loop = _tcslen(progdir);
    while (loop--) {
        if ((progdir[loop] == TEXT('\\')) ||
            (progdir[loop] == TEXT(':'))) {
                progdir[loop+1] = 0;
                break;
        }
    }
}

// int APIENTRY WinMain (HINSTANCE passinstance, HINSTANCE null1, LPSTR null2, int null3) {
// 
//     // DO ONE-TIME INITIALIZATION
//      instance = passinstance;
//      GetProgramDirectory();
// //      FrameRegisterClass();
// 
//     do {
// 
//         // DO INITIALIZATION THAT MUST BE REPEATED FOR A RESTART
//         restart = 0;
//         mode    = MODE_LOGO;
// 
//         if (!LoadReg()) SaveReg();
// //         DebugInitialize();
//         MemInitialize();
// //         TermInitialize();
// //         FrameCreateWindow();
// 
//         // ENTER THE MAIN MESSAGE LOOP
// //         EnterMessageLoop();
// 
// 
//     } while (restart);
// 
// 
//     return 0;
// }


