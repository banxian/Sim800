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
private:
    QString fProjectViewNameFilter;
    EmulatorThread* fEmulatorThread;
    QByteArray fNorBuffer;
private:
    bool LoadDemoNor(const QString& filename);
    //static bool NicetoScaleOnFly( QSize &orgsize, bool ispng, bool isjpeg );
    //static bool NiceToRotate( int width, int height, int destwidth, int destheight );
    //static bool PrepareScaledQPicture( QImage& qimage, int destwidth, int destheight, ScaleFilter filter, int& scaletime );
    //static void OptimizeMangaImage( QImage& qimage );
    //static void SaveBundleRec( TPageBundleRec& page ); // Encode to file and drop image
    //bool PreparePageImagesBuffer();
    //void CalulatePages(int& allpagecount, int &readypagecount);
public:
    bool IsProjectEmpty();
    bool IsProjectModified();
    bool SwitchNorBank(int bank);
    bool StartEmulation();
    bool RunDemoBin(const QString& filename);
    bool StopEmulation();

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
protected:
    char* fBROMBuffer;
    char* fNorBuffer;
    bool fKeeping;
    QByteArray fLCDBuffer;
protected:
    void run();
public:
    void StopKeeping();
signals:
    void lcdBufferChanged(QByteArray* buffer);
    void stepFinished(quint16 pc);
};

typedef TNekoDriver* PNekoDriver;
extern PNekoDriver theNekoDriver;

extern unsigned short lcdbufaddr;

#endif