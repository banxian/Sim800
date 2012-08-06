#include "NekoDriver.h"
#include "DBCentre.h"
extern "C" {
#include "ANSI/65c02.h"
}


// Storage
PNekoDriver theNekoDriver;

extern void GetProgramDirectory();
extern void ContinueExecution();

TNekoDriver::TNekoDriver()
    : fEmulatorThread(NULL)
{
    GetProgramDirectory();

    // Do initialization that must be repeated for a restart
    restart = 0;
    mode    = MODE_LOGO;

    if (!LoadReg()){
        SaveReg();
    }
    //         DebugInitialize();
    MemInitialize();
    //         TermInitialize();
    //         FrameCreateWindow();

    // enter the main message loop
    //         EnterMessageLoop();

//     fEmulatorThread->start();
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
        LoadDemoNor(QApplication::applicationDirPath() + "/mario.bin");
    } else {
        LoadDemoNor(filename);
    }
    mem[0] = 1;
    SwitchNorBank(1);
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

extern bool timer0started;
extern bool timer1started;

//DWORD lastTicket = 0;
//unsigned long long totalcycle = 0;
//const unsigned spdc1016freq = 3686400;
//bool checked = false;
//unsigned batchlimiter = 0;
//long batchcount = LONG_MAX;
//double sleepgap = 10;
//long sleepcount = 0;

void EmulatorThread::run()
{
    // Load PC from Reset Vector
    CpuInitialize();
    while(fKeeping) {
        //ContinueExecution();
        //DWORD processtime		= totcycles;// needed for comm routines
        //DWORD loop				= 4096; //speed * 300;// watchdog timer
        //DWORD elapsed			= 0;
        //DWORD CpuTicks          = 0;
        //DWORD j = 0;

        //elapsed = GetTickCount()-stmsecs;
        const unsigned spdc1016freq = GlobalSetting.SPDC1016Frequency;

        while (batchcount >= 0) {
            //qDebug("PC:0x%04x, opcode: 0x%06x", regs.pc, (*(LPDWORD)(mem+regs.pc)) & 0xFFFFFF);
            //LogDisassembly(regs.pc, NULL);
            DWORD CpuTicks = CpuExecute();
            totcycles += CpuTicks;
            totalcycle += CpuTicks;
            executed++;
            // add checks for reset, IRQ, NMI, and other pin signals
            //elapsed = GetTickCount()-stmsecs;
            if (lastTicket == 0) {
                lastTicket = GetTickCount();
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
            if (timer0started) {
                mem[02] = mem[02] + 1;
            }
            if (timer1started) {
                mem[03] = mem[03] + 1;
            }

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
        //if (fLCDBuffer.isEmpty()) {
        //    fLCDBuffer = QByteArray((const char *)&mem[0x9C0], 160*80/8);
        //    emit lcdBufferChanged(new QByteArray(fLCDBuffer));
        //} else if (memcmp(&mem[0x9C0], fLCDBuffer.data(), 160*80/8) != 0) {
        //    memcpy(fLCDBuffer.data(), &mem[0x9C0], 160*80/8);
        //    emit lcdBufferChanged(new QByteArray(fLCDBuffer));
        //}
        if (memcmp(&mem[0x9C0], fLCDBuffer, 160*80/8) != 0) {
            memcpy(fLCDBuffer, &mem[0x9C0], 160*80/8);
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
