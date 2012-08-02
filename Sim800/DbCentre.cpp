#include <QtGui/QApplication>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtSql/QtSQL>
#include "DbCentre.h"

TPathRecItem PathSetting; // imp
TStateRecItem StateSetting; // imp
TGlobalRecItem GlobalSetting;

void LoadAppSettings( void )
{
    QSettings settings(QApplication::applicationDirPath() + "/ConfHonya.ini",
        QSettings::IniFormat);
    PathSetting.LastSourceFolder = settings.value("Path/LastSourceFolder", "").toString();
    PathSetting.LastStockBookFolder = settings.value("Path/LastStockBookFolder", "").toString();
    PathSetting.LastCustomBookFolder = settings.value("Path/LastCustomBookFolder", "").toString();
    PathSetting.LastExportFolder = settings.value("Path/LastExportFolder", "").toString();
    PathSetting.LastImportFolder = settings.value("Path/LastImportFolder", "").toString();
    PathSetting.LastProjectFolder = settings.value("Path/LastProjectFolder", "").toString();
    PathSetting.LastSelectedItemIndex = settings.value("Path/LastSelectedItemIndex", 0).toInt();

    StateSetting.WindowMaxium = settings.value("State/WindowMaxium", true).toBool();
    StateSetting.MainFrmState = settings.value("State/MainFrmState", QByteArray()).toByteArray();
    StateSetting.RegisterLayoutState = settings.value("State/RegisterLayoutState", QByteArray()).toByteArray();
    StateSetting.MessageLayoutState = settings.value("State/MessageLayoutState", QByteArray()).toByteArray();

    StateSetting.RegEditorMaxium = settings.value("State/RegEditorMaxium", true).toBool();
    StateSetting.RegFrmState = settings.value("State/RegEditorMaxium", QByteArray()).toByteArray();
    StateSetting.KeyLayoutState = settings.value("State/KeyLayoutState", QByteArray()).toByteArray();

    GlobalSetting.AutoCrop = settings.value("Global/AutoCrop", true).toBool();
    GlobalSetting.AutoExposure = settings.value("Global/AutoCrop", true).toBool();
    //GlobalSetting.SplitPage = MangaDoublePageMode(settings.value("Global/SplitPage", mdpmAutoNippon).toInt());
    GlobalSetting.PageSorting = PageSortingMethod(settings.value("Global/PageSorting", psmAutomatic).toInt());
    GlobalSetting.EnhanceLevel = FilterMode(settings.value("Global/EnhanceLevel", fmHigh).toInt());
    GlobalSetting.OutputFormat = CodecMode(settings.value("Global/OutputFormat", cmJPEGVeryHigh).toInt());
    GlobalSetting.OutputResolution = ResizeMode(settings.value("Global/OutputResolution", rmFullScreen).toInt());
    GlobalSetting.UseSurfaceBlur = settings.value("Global/UseSurfaceBlur", false).toBool();
    GlobalSetting.RemoveJPEGArtifacts = settings.value("Global/RemoveJPEGArtifacts", false).toBool();
}

void SaveAppSettings( void )
{
    QSettings settings(QApplication::applicationDirPath() + "/ConfHonya.ini",
        QSettings::IniFormat);
    settings.beginGroup("Path");
    settings.setValue("LastSourceFolder", PathSetting.LastSourceFolder);
    settings.setValue("LastStockBookFolder", PathSetting.LastStockBookFolder);
    settings.setValue("LastCustomBookFolder", PathSetting.LastCustomBookFolder);
    settings.setValue("LastExportFolder", PathSetting.LastExportFolder);
    settings.setValue("LastImportFolder", PathSetting.LastImportFolder);
    settings.setValue("LastProjectFolder", PathSetting.LastProjectFolder);
    settings.setValue("LastSelectedItemIndex", PathSetting.LastSelectedItemIndex);
    settings.endGroup();

    settings.beginGroup("State");
    settings.setValue("WindowMaxium", StateSetting.WindowMaxium);
    settings.setValue("MainFrmState", StateSetting.MainFrmState);
    settings.setValue("RegisterLayoutState", StateSetting.RegisterLayoutState);
    settings.setValue("MessageLayoutState", StateSetting.MessageLayoutState);

    settings.setValue("RegEditorMaxium", StateSetting.RegEditorMaxium);
    settings.setValue("RegFrmState", StateSetting.RegFrmState);
    settings.setValue("KeyLayoutState", StateSetting.KeyLayoutState);
    settings.endGroup();

    settings.beginGroup("Global");
    settings.setValue("AutoCrop", GlobalSetting.AutoCrop);
    settings.setValue("AutoExposure", GlobalSetting.AutoExposure);
    //settings.setValue("SplitPage", GlobalSetting.SplitPage);
    settings.setValue("PageSorting", GlobalSetting.PageSorting);
    settings.setValue("EnhanceLevel", GlobalSetting.EnhanceLevel);
    settings.setValue("OutputFormat", GlobalSetting.OutputFormat);
    settings.setValue("OutputResolution", GlobalSetting.OutputResolution);
    settings.setValue("UseSurfaceBlur", GlobalSetting.UseSurfaceBlur);
    settings.setValue("RemoveJPEGArtifacts", GlobalSetting.RemoveJPEGArtifacts);
    settings.endGroup();
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Project IO Helper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QDataStream & operator<<( QDataStream &out, const TStreamCodec &codec )
{
    out << quint32(codec);
    return out;
}

QDataStream & operator>>( QDataStream &in, TStreamCodec &codec )
{
    quint32 dummy;
    in >> dummy;
    codec = TStreamCodec(dummy);
    return in;
}
