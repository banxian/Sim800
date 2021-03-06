#ifndef _DBCENTRE_H
#define _DBCENTRE_H

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QDataStream>


typedef struct tagPathRecItem {
    QString LastSourceFolder; // Add pages
    QString LastStockBookFolder; // Extract snb
    QString LastCustomBookFolder; // Save output snb
    QString LastExportFolder; // xml folder
    QString LastImportFolder; // xml folder
    QString LastProjectFolder; // mbk
    int LastSelectedItemIndex;
} TPathRecItem;

typedef struct tagStateRecItem {
    bool WindowMaxium;
    QByteArray MainFrmState;
    QByteArray KeypadLayoutState;
    QByteArray MessageLayoutState;
    bool RegEditorMaxium;
    QByteArray RegFrmState;
    QByteArray KeyLayoutState;
} TStateRecItem;

typedef struct tagGlobalRecItem {
    bool AutoExposure; // Gamma?
    bool RemoveJPEGArtifacts;
    bool AutoCrop;
    bool UseSurfaceBlur;
    int SPDC1016Frequency;
} TGlobalRecItem;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Project IO Helper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// enum
//QDataStream &operator<<(QDataStream &out, const MangaDoublePageMode &mode);
//QDataStream &operator>>(QDataStream &in, MangaDoublePageMode &mode);
//QDataStream &operator<<(QDataStream &out, const ScaleFilter &filter);
//QDataStream &operator>>(QDataStream &in, ScaleFilter &filter);
//QDataStream &operator<<(QDataStream &out, const TStreamCodec &codec);
//QDataStream &operator>>(QDataStream &in, TStreamCodec &codec);

// structural
//QDataStream &operator<<(QDataStream &out, const TBookBundleRec &book);
//QDataStream &operator<<(QDataStream &out, const TChapterBundleRec &chapter);
//QDataStream &operator<<(QDataStream &out, const TPageBundleRec &page);

//QDataStream &operator>>(QDataStream &in, TBookBundleRec &book);
//QDataStream &operator>>(QDataStream &in, TChapterBundleRec &chapter);
//QDataStream &operator>>(QDataStream &in, TPageBundleRec &page);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Used in NekoDriver etc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern TPathRecItem PathSetting;
extern TStateRecItem StateSetting;
extern TGlobalRecItem GlobalSetting;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LoadAppSettings( void );
void SaveAppSettings( void );

#endif
