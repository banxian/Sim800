#   include "NekoDriver.h"
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
{

}

EmulatorThread::~EmulatorThread()
{
    free(fLCDBuffer);
}


extern WORD LogDisassembly (WORD offset, LPTSTR text);

extern bool timer0started;
extern bool timer1started;


void EmulatorThread::run()
{
    // Load PC from Reset Vector
    CpuInitialize();
    while(fKeeping) {
        //ContinueExecution();
        DWORD processtime		= totcycles;// needed for comm routines
        DWORD loop				= 4096; //speed * 300;// watchdog timer
        DWORD elapsed			= 0;
        DWORD CpuTicks          = 0;
        DWORD j = 0;

        //elapsed = GetTickCount()-stmsecs;

        while (loop/* && (totcycles < (elapsed * 100 * speed))*/) {
            //qDebug("PC:0x%04x, opcode: 0x%06x", regs.pc, (*(LPDWORD)(mem+regs.pc)) & 0xFFFFFF);
            //LogDisassembly(regs.pc, NULL);
            CpuTicks = CpuExecute();
            totcycles += CpuTicks;
            executed++;
            // add checks for reset, IRQ, NMI, and other pin signals
            //elapsed = GetTickCount()-stmsecs;
            loop--;
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
            //usleep(10);
            Sleep(0);
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
    }
    //this->deleteLater();
}

void EmulatorThread::StopKeeping()
{
    fKeeping = false;
}
