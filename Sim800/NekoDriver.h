#ifndef _NEKO_DRIVER_UNIT_H
#define _NEKO_DRIVER_UNIT_H

#include "MainUnt.h"
#include <QtCore/QFile>
#include <QtGui/QTreeWidget>
#include <QtGui/QtGui>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtGui/QImage>
#include <vector>


class EmulatorThread;

class TNekoDriver: public QObject {
    Q_OBJECT
public:
    TNekoDriver();
    ~TNekoDriver();
private:
    QString fProjectViewNameFilter;
    EmulatorThread* fEmulatorThread;
    char* fNorBuffer; // for performance
    char* fBROMBuffer;
    QString fNorFilename;
    bool fFlashUpdated;
private:
    bool LoadBROM(const QString& filename);
    bool LoadFullNorFlash(const QString& filename);
    bool LoadDemoNor(const QString& filename);
    bool SaveFullNorFlash();

public:
    bool IsProjectEmpty();
    bool IsProjectModified();
    void SwitchNorBank(int bank);
    void Switch4000toBFFF(unsigned char bank); // used 0A/0D value
    void InitInternalAddrs();
    bool StartEmulation();
    bool RunDemoBin(const QString& filename);
    bool StopEmulation();
    bool PauseEmulation();
    bool ResumeEmulation();
    void CheckFlashProgramming(unsigned short addr, unsigned char data);

public slots:
    void onLCDBufferChanged(QByteArray* buffer);
    void onStepFinished(quint16 pc);
signals:
    void lcdBufferChanged(QByteArray* buffer);
    void stepFinished(quint16 pc);
};

class EmulatorThread : public QThread
{
    Q_OBJECT
public:
    explicit EmulatorThread(char* brom, char* nor);
    ~EmulatorThread();
protected:
    char* fBROMBuffer;
    char* fNorBuffer;
    bool fKeeping;
    void* fLCDBuffer;
private:
    unsigned int lastTicket;
    unsigned long long totalcycle;
    //const unsigned spdc1016freq = 3686400;
    bool measured;
    unsigned remeasure;
    unsigned batchlimiter;
    long batchcount;
    double sleepgap;
    long sleepcount;

protected:
    void run();

#ifdef AUTOTEST
private:
    bool enablelogging;
    void TryTest(unsigned line);
#endif

public:
    void StopKeeping();
signals:
    void lcdBufferChanged(QByteArray* buffer);
    void stepFinished(quint16 pc);
};

typedef TNekoDriver* PNekoDriver;
extern PNekoDriver theNekoDriver;

extern unsigned short lcdbuffaddr; // unused

extern unsigned keypadmatrix[8][8]; // char -> uint32

#endif