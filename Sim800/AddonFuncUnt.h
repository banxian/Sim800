#ifndef _ADDON_FUNC_UNIT
#define _ADDON_FUNC_UNIT

#include <string>
#include <QtGui/QImage>
#include <QtCore/QFile>
#include <Windows.h>
//#include "WincePlaceHolder.h"

using std::string;
using std::wstring;


enum TImageType {
    itUnknown = -1, itTarga, itBitmap, itPNG, itJPEG, itGif, itHDPhoto, itPKArchive, itRarArchive
};

QImage NewQImageFromRCDATA(LPCTSTR lpName);
QImage NewQImageFromResource(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName);
bool LoadQImageFromResource(QImage& qimage, HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName);
void Normalize8bitQImage( QImage& qimage ); // or Indexed8
void QuantizeToGrayscaleQImage( QImage& qimage, bool dither = false ); // diffusion?
void ReplaceAlphaWithChecker( QImage& qimage );
void UncropImage( QImage& qimage, int destwidth, int destheight );
void AutocropImage( QImage& qimage, bool fromdualpage, bool isleftpage );
bool IsGrayscaleImage(const QImage& qimage);
void AutoExposureImage( QImage& qimage, bool heavy = false );
int GetFreePhysMemory();
quint16 readuint16(QIODevice& qfile, bool bigendian = false, bool peek = false);
quint32 readuint32(QIODevice& qfile, bool bigendian = false, bool peek = false);
QImage NewQImageFromEXIFThumbnail(QFile& qfile);
TImageType GuessMIMEType(QIODevice* file);
TImageType GuessMIMEType(const QString& filename);
TImageType ExtToMIMEType(const QString& extname);
QString MIMETypeToExt(TImageType type);

wstring StringToWideString(const string& Source, unsigned int Codepage = CP_ACP);
QByteArray qUncompressEx(const uchar* data, int nbytes, int outputbytes);
QByteArray qUncompressEx1(const uchar* data, int nbytes, int outputbytes);
QByteArray qCompressEx(const uchar* data, int nbytes, int compressionLevel);

bool SaveByteArrayToFile( const QString& filename, const QByteArray& content );

int greedydiv(int n, int m);
int getCpuCount();
unsigned int getTickCount();
#endif