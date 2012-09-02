#include "NekoDriver.h"
#include "DBCentre.h"
extern "C" {
#include "ANSI/65c02.h"
}
#include "CC800IOName.h"


// Storage
PNekoDriver theNekoDriver;

TNekoDriver::TNekoDriver()
    : fEmulatorThread(NULL)
    , fNorBuffer(NULL)
    , fBROMBuffer(NULL)
{
    // Do initialization that must be repeated for a restart
    restart = 0;

    fBROMBuffer = (char*)malloc(512 * 0x8000); // 32K * 256 * 2
    for (int i = 0; i < 256; i++) {
        volume0array[i] = (unsigned char*)fBROMBuffer + i * 0x8000;
        volume1array[i] = volume0array[i] + 256 * 0x8000;
    }

    fNorBuffer = (char*)malloc(16 * 0x8000); // 32K * 16
    for (int i = 0; i < 16; i++) {
        norbankheader[i] = (unsigned char*)fNorBuffer + i * 0x8000;
    }

    //         DebugInitialize();
    MemInitialize();
    //         TermInitialize();
    //         FrameCreateWindow();

    // enter the main message loop
    //         EnterMessageLoop();

//     fEmulatorThread->start();
}


TNekoDriver::~TNekoDriver()
{
    if (fNorBuffer) {
        free(fNorBuffer);
        fNorBuffer = NULL;
    }
    if (fBROMBuffer) {
        free(fBROMBuffer);
        fBROMBuffer = NULL;
    }
}


void TNekoDriver::onLCDBufferChanged( QByteArray* buffer )
{
    emit lcdBufferChanged(buffer);
}

void TNekoDriver::onStepFinished( quint16 pc )
{
    emit stepFinished(pc);
}

bool TNekoDriver::StartEmulation()
{
    fEmulatorThread = new EmulatorThread(NULL, NULL);
    connect(fEmulatorThread, SIGNAL(lcdBufferChanged(QByteArray*)), 
        this, SLOT(onLCDBufferChanged(QByteArray*)));
    connect(fEmulatorThread, SIGNAL(stepFinished(quint16)), 
        this, SLOT(onStepFinished(quint16)));
    fEmulatorThread->start(QThread::InheritPriority);
    return true;
}

bool TNekoDriver::StopEmulation()
{
    if (fEmulatorThread) {
        fEmulatorThread->StopKeeping();
        fEmulatorThread->wait(4000);
        fEmulatorThread->deleteLater();
        fEmulatorThread = NULL;
    }
    return true;
}

bool TNekoDriver::RunDemoBin( const QString& filename )
{
    if (filename.isEmpty()) {
        //LoadDemoNor(QApplication::applicationDirPath() + "/mario.bin");
        LoadBROM(QApplication::applicationDirPath() + "/obj.bin");
        LoadFullNorFlash(QApplication::applicationDirPath() + "/cc800.fls");
        //fixedram0000[io00_bank_switch] = 2;
        //SwitchNorBank(2);
        //*(unsigned short*)&(pmemmap[mapE000][0x1FFC]) = 0x4018; // mario.bin
    } else {
        LoadDemoNor(filename);
        fixedram0000[io00_bank_switch] = 1;
        SwitchNorBank(1);
        *(unsigned short*)&(pmemmap[mapE000][0x1FFC]) = 0x4018; // mario.bin
    }
    //fEmulatorThread->start(QThread::InheritPriority);
    StopEmulation();
    StartEmulation();
    return true;
}

bool TNekoDriver::PauseEmulation()
{
    return false;
}

bool TNekoDriver::ResumeEmulation()
{
    return false;
}



EmulatorThread::EmulatorThread( char* brom, char* nor )
    : fBROMBuffer(brom)
    , fNorBuffer(nor)
    , fKeeping(true)
    , fLCDBuffer(malloc(160*80/8))
    , lastTicket(0)
    , totalcycle(0)
    , checked(false)
    , batchlimiter(0)
    , batchcount(UINT_MAX)
    , sleepgap(10)
    , sleepcount(0)
{

}

EmulatorThread::~EmulatorThread()
{
    free(fLCDBuffer);
}


extern WORD LogDisassembly (WORD offset, LPTSTR text);
extern void AppendLog(const char* text);

extern bool timer0started;
extern bool timer1started;
extern unsigned short gThreadFlags;

//DWORD lastTicket = 0;
//unsigned long long totalcycle = 0;
//const unsigned spdc1016freq = 3686400;
//bool checked = false;
//unsigned batchlimiter = 0;
//long batchcount = LONG_MAX;
//double sleepgap = 10;
//long sleepcount = 0;

// WQXSIM
extern bool timer0waveoutstart;
extern int prevtimer0value;
int gDeadlockCounter = 0;
extern bool lcdoffshift0flag;
bool matrixupdated = false;

void Turnoff2HzNMIMaskAddIRQFlag();
void CheckTimebaseAndEnableIRQnEXIE1();
void EnableWatchDogFlag();
void CheckLCDOffShift0AndEnableWatchDog();

void EmulatorThread::run()
{
    // Load PC from Reset Vector
    CpuInitialize();
    //stp = 1; // test
    unsigned int nmistart = GetTickCount();
    gThreadFlags &= 0xFFFEu; // Remove 0x01 from gThreadFlags (stack related)
    while(fKeeping) {
        //ContinueExecution();
        //DWORD processtime		= totcycles;// needed for comm routines
        //DWORD loop				= 4096; //speed * 300;// watchdog timer
        //DWORD elapsed			= 0;
        //DWORD CpuTicks          = 0;
        //DWORD j = 0;

        //elapsed = GetTickCount()-stmsecs;
        const unsigned spdc1016freq = GlobalSetting.SPDC1016Frequency;

        while (batchcount >= 0 && fKeeping) {
            //qDebug("PC:0x%04x, opcode: 0x%06x", regs.pc, (*(LPDWORD)(mem+regs.pc)) & 0xFFFFFF);
            LogDisassembly(regs.pc, NULL);
            if (matrixupdated) {
                matrixupdated = false;
                AppendLog("keypadmatrix updated.");
            }

            unsigned int dummynow = GetTickCount();
            if (dummynow - nmistart >= 500){
                // 2Hz NMI
                // TODO: use batchcount as NMI
#ifdef _DEBUG
                //if (dummynow - nmistart >= 1000) {
                //    // Debugger
                //    nmistart = dummynow - 3000; // delay 3s
                //} else {
                //    nmistart = dummynow; // prevent multi NMI
                //}
                nmistart = dummynow;
#else
                nmistart += 500;
#endif
                //nmi = 0; // next CpuExecute will execute two instructions
                gThreadFlags |= 0x08; // Add NMIFlag
            }

            if ((gThreadFlags & 0x08) != 0) {
                gThreadFlags &= 0xFFF7u; // remove 0x08 NMI Flag
                nmi = 0; // next CpuExecute will execute two instructions
                qDebug("ggv wanna NMI.");
            }
            if (((regs.ps & 0x4) == 0) && ((gThreadFlags & 0x10) != 0)) {
                gThreadFlags &= 0xFFEFu; // remove 0x10 IRQ Flag
                irq = 0; // B flag will remove in CpuExecute (AF_BREAK)
                qDebug("ggv wanna IRQ.");
            }

            DWORD CpuTicks = CpuExecute();
            totcycles += CpuTicks;
            totalcycle += CpuTicks;
            executed++;
            // add checks for reset, IRQ, NMI, and other pin signals
            //elapsed = GetTickCount()-stmsecs;
            if (lastTicket == 0) {
                lastTicket = GetTickCount();
            }

            gDeadlockCounter++;
            if (gDeadlockCounter == 6000) {
                // overflowed
                gDeadlockCounter = 0;
                if ((gThreadFlags & 0x80) == 0) {
                    // CheckTimerbaseAndEnableIRQnEXIE1
                    CheckTimebaseAndEnableIRQnEXIE1();
                    if (timer0started) {
                        // mayDestAddr == 5 in ReadRegister
                        // mayBaseTable have 0x100 elements?
                        //increment = mayBasetable[mayDestAddr];  // mayBaseTable[5] == 3
                        //t0overflow = (unsigned int)(increment + mayPreviousTimer0Value) < 0xFF;
                        //mayPreviousTimer0Value += increment;
                        //if ( !t0overflow )
                        //{
                        //    mayPreviousTimer0Value = 0;
                        //    Turnoff2HzNMIMaskAddIRQFlag();
                        //}
                        int increment = 3;
                        prevtimer0value += increment;
                        if (prevtimer0value >= 0xFFu) {
                            prevtimer0value = 0;
                            Turnoff2HzNMIMaskAddIRQFlag();
                        }
                    }
                } else {
                    // RESET
                    fixedram0000[io01_int_enable] |= 0x1; // TIMER A INTERRUPT ENABLE
                    fixedram0000[io02_timer0_val] |= 0x1; // [io01+1] Timer0 bit1 = 1
                    gThreadFlags &= 0xFF7F;      // remove 0x80 | 0x10
                    regs.pc = *(unsigned short*)&pmemmap[mapE000][0x1FFC];
                }
            } else {
                if (timer0started) {
                    // mayDestAddr == 5 in ReadRegister
                    // mayBaseTable have 0x100 elements?
                    int increment = 3;
                    prevtimer0value += increment;
                    if (prevtimer0value >= 0xFFu) {
                        prevtimer0value = 0;
                        Turnoff2HzNMIMaskAddIRQFlag();
                    }
                }
            }
            
            //loop--;
            //// added for irq timing/
            //if (irqclk) {
            //    irqcnt += CpuTicks;
            //    if (irqcnt >= irqclk) {
            //        irq=0;
            //        irqcnt = irqcnt - irqclk;
            //    }
            //}
            //if (nmiclk) {
            //    nmicnt += CpuTicks;
            //    if (nmicnt >= nmiclk) {
            //        nmi=0;
            //        nmicnt = nmicnt - nmiclk;
            //    }
            //}
            //if (timer0started) {
            //    fixedram0000[io02_timer0_val] = fixedram0000[io02_timer0_val] + 1;
            //}
            //if (timer1started) {
            //    fixedram0000[io03_timer1_val] = fixedram0000[io03_timer1_val] + 1;
            //}

            /* Throttling routine (simple delay loop)  */
            //if (throttle) for (j=throttle*CpuTicks;j>1;j--);
            if (totalcycle % spdc1016freq < 10 && totalcycle > spdc1016freq) {
                if (checked == false) {
                    checked = true;
                    if (totalcycle < spdc1016freq * 2) {
                        // first loop check!
                        // spdc1016 executed one second in fullspeed virtual timeline
                        unsigned long long realworldtime = GetTickCount() - lastTicket; // should less than 1000ms
                        lastTicket = GetTickCount();
                        //double virtual100ms = realworldtime / 100.0;
                        if (realworldtime > 1000) {
                            batchlimiter = spdc1016freq * 2;
                        } else if (batchlimiter == 0) {
                            // 1000 - realworldtime = overflow time, overflow time / 10 = sleepcount, freq / sleepcount = batchcount
                            //batchlimiter = spdc1016freq / ((1000 - realworldtime) / 10);
                            sleepcount = (1000 - realworldtime) / sleepgap;
                            batchlimiter = spdc1016freq * sleepgap / (1000 - realworldtime);
                        } else {
                            // sleep(0) is less than 10ms, but we'd never go here
                        }
                        batchcount = batchlimiter;
                    } else {
                        //// check once more
                        //unsigned long long realworldtime = GetTickCount() - lastTicket; // should less than 1000ms
                        //lastTicket = GetTickCount();
                        //if (realworldtime < 1000 && batchlimiter > 0) {
                        //    // sleep(0) is less than 10ms
                        //    // TODO: calculate real sleep gap
                        //    double lastexrtatime = sleepgap * sleepcount; // for eg400ms, sleep 20*10ms
                        //    long newextratime = (1000 - realworldtime); // for eg100ms, real sleepgap = (400 - 100) / 20
                        //    sleepgap = (lastexrtatime - newextratime) / sleepcount;
                        //    sleepcount = lastexrtatime / sleepgap;
                        //    batchlimiter = spdc1016freq * 10 / lastexrtatime;
                        //}
                    }
                }
            }
            if (totalcycle % spdc1016freq > 10 && totalcycle > spdc1016freq) {
                checked = false;
            }

            if (batchlimiter != 0) {
                batchcount -= CpuTicks;
                //if (batchcount < 0) {
                //    batchcount = batchlimiter;
                //    Sleep(10);
                //}
            }

            //usleep(10);
            //Sleep(0);
        }
        /* Throttling update routine   */
        //if (throttle) {
        //    if (totcycles < (elapsed * 100 * speed)) {
        //        throttle--;
        //        if (throttle < 1) throttle = 1;
        //    }
        //    else {
        //        throttle++;
        //    }
        //}
        if (memcmp(&fixedram0000[0x9C0], fLCDBuffer, 160*80/8) != 0) {
            memcpy(fLCDBuffer, &fixedram0000[0x9C0], 160*80/8);
            emit lcdBufferChanged(new QByteArray((const char*)fLCDBuffer, 160*80/8));
        }
        //emit stepFinished(regs.pc);
        //qDebug("PC:0x%04x", regs.pc);
        Sleep(10);
        if (batchlimiter > 0) {
            batchcount = batchlimiter;
        } else {
            batchcount = spdc1016freq * 2; // dirty fix
        }
    }
    //this->deleteLater();
}

void EmulatorThread::StopKeeping()
{
    fKeeping = false;
}

void CheckLCDOffShift0AndEnableWatchDog()
{
    //// may check hotkey press
    //if ( gLcdoffShift0Flag )
    //{
    //    if ( keypadmatrix1[6] || keypadmatrix1[7] )
    //    {
    //        // Line6 Dict Card
    //        // Line7 on/off
    //        EnableWatchDogFlag();               // 0x80
    //        gLcdoffShift0Flag = 0;
    //    }
    //}
    //else
    //{
    //    // Set lcdoffshift0flag only when keypadmatrix3 == 0xFB
    //    if ( keypadmatrix1[7] == 0xFBu )
    //        gLcdoffShift0Flag = 1;
    //}
    matrixupdated = true;
    if (lcdoffshift0flag) {
        // we don't have invert bit for row6,7
        //bool row67down = false;
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 8; x++) {
                if (keypadmatrix[y][x] == 1) {
                    //row6,7down = true;
                    EnableWatchDogFlag();
                    lcdoffshift0flag = false;
                    return;
                }
            }
        }
    } else {
        if (keypadmatrix[0][2] == 1) {
            lcdoffshift0flag = true; // this flag is used for UI?
        }
    }
}

void CheckTimebaseAndEnableIRQnEXIE1()
{
    //if ( gFixedRAM0_04_general_ctrl & 0xF )
    //{
    //    // TimeBase Clock Select bit0~3
    //    LOBYTE(gThreadFlags) = gThreadFlags | 0x10; // add 0x10 to gThreadFlags
    //    gFixedRAM0[(unsigned __int8)io01_int_ctrl] |= 8u;// EXTERNAL INTERRUPT SELECT1
    //}
    if (fixedram0000[io04_general_ctrl] & 0x0F) {
        gThreadFlags |= 0x10; // Add IRQ flag
        //irq = 0; // TODO: move to NMI check
        fixedram0000[io01_int_enable] |= 0x8; // EXTERNAL INTERRUPT SELECT1
    }
}

void Turnoff2HzNMIMaskAddIRQFlag()
{
    if ( fixedram0000[io04_general_ctrl] & 0xF )
    {
        gThreadFlags |= 0x10u; // Add 0x10 Flag to gThreadFlag
        //irq = 0; // TODO: move to 
        fixedram0000[io01_int_enable] |= 0x10u; // 2Hz NMI Mask off
    }
}

void EnableWatchDogFlag()
{
    gThreadFlags |= 0x80;
}
