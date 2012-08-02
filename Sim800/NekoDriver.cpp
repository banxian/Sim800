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

EmulatorThread::EmulatorThread( char* brom, char* nor )
    : fBROMBuffer(brom)
    , fNorBuffer(nor)
    , fKeeping(true)
{

}

void EmulatorThread::run()
{
    // Load PC from Reset Vector
    CpuInitialize();
    while(fKeeping) {
        ContinueExecution();
        if (fLCDBuffer.isEmpty()) {
            fLCDBuffer = QByteArray((const char *)&mem[0x9C0], 160*80/8);
            emit lcdBufferChanged(new QByteArray(fLCDBuffer));
        } else if (memcmp(&mem[0x9C0], fLCDBuffer.data(), 160*80/8) != 0) {
            memcpy(fLCDBuffer.data(), &mem[0x9C0], 160*80/8);
            emit lcdBufferChanged(new QByteArray(fLCDBuffer));
        }
        emit stepFinished(regs.pc);
        //qDebug("PC:0x%04x", regs.pc);
    }
    //this->deleteLater();
}

void EmulatorThread::StopKeeping()
{
    fKeeping = false;
}
