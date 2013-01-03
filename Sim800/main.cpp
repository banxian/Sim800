////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UTF-8 without BOM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <QtGUI/QApplication>
#include <QtCore/QTranslator>
#include <QtCore/QSysInfo>
#include <QtCore/QLibrary>
#include <QtCore/QtPlugin>
#include "MainUnt.h"

#ifdef QT_NODLL
Q_IMPORT_PLUGIN(qjpegmod)
Q_IMPORT_PLUGIN(qpngmod)
//Q_IMPORT_PLUGIN(qgif)
//Q_IMPORT_PLUGIN(qmng)
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QTranslator translator;
    translator.load(QString("Sim800_") + localLanguage(), app.applicationDirPath() + "/Language");
    //translator.load("MPack_chs", app.applicationDirPath() + "/Language");
#ifdef __GNUC__
    QStringList matchlist = QString::fromUtf16((ushort*)L"Tahoma;宋体;微软雅黑;ＭＳ ゴシック").split(";", QString::KeepEmptyParts, Qt::CaseInsensitive);
#else
    // workaround for GCC utf8 BOM behavior
    static const ushort faces[] = {
        L'T', L'a', L'h', L'o', L'm', L'a', L';',
        0x5B8B, 0x4F53, L';',
        0x5FAE, 0x8F6F, 0x96C5, 0x9ED1, L';',
        0xFF2D, 0xFF33, L' ', 0x30B4, 0x30B7, 0x30C3, 0x30AF,
        0x0
    };
    QStringList matchlist = QString::fromUtf16(&faces[0]).split(";", QString::KeepEmptyParts, Qt::CaseInsensitive);
#endif
    QFont::insertSubstitutions("Tahoma", matchlist);
    QFont::insertSubstitutions("MS Shell Dlg2", matchlist);
    QFont f(QLatin1String("Tahoma"), 8);
    if (translator.isEmpty() == false) {
        app.installTranslator(&translator);
        QStringList matchlist = QObject::tr("Tahoma;Arial Unicode MS").split(";", QString::KeepEmptyParts, Qt::CaseInsensitive);
        QFont::removeSubstitution("Tahoma");
        QFont::insertSubstitutions("Tahoma", matchlist);
        f.setStyleStrategy(QFont::PreferAntialias);
        f.setPointSize(QObject::tr("9").toInt());
    }
    QApplication::setFont(f);

    if (QSysInfo::WindowsVersion < QSysInfo::WV_5_1) {
        QApplication::setStyle("CleanLooks");
    } else {
#ifdef WIN32
        // API is damn
        // IsAppThemed
        /*
        typedef int (__stdcall *FARPROC)(long);
        QLibrary uxthemelib("uxtheme");
        bool themed = false;
        FARPROC pEnableTheming = FARPROC(uxthemelib.resolve("EnableTheming"));
        if (pEnableTheming) {
            if (pEnableTheming(TRUE) != 0) {
                typedef int (__stdcall *FARPROC)();
                FARPROC pIsAppThemed = FARPROC(uxthemelib.resolve("IsAppThemed"));
                if (pIsAppThemed && pIsAppThemed() == TRUE) {
                    themed = true;
                }
            } else {
                // == S_OK
                themed = true;
            }
        }
        if (themed == false) {
            QApplication::setStyle("CleanLooks");
        }
        */
        // Simpler one
        typedef int (__stdcall *FARPROC)();
        QLibrary uxthemelib("uxtheme");
        FARPROC pIsAppThemed = FARPROC(uxthemelib.resolve("IsAppThemed"));
        if (pIsAppThemed && pIsAppThemed() == FALSE) {
            QApplication::setStyle("CleanLooks");
        }
#endif
    }
    
    TMainFrm mainfrm;
    mainfrm.StoreTranslator(&translator);
    //mainfrm.setAttribute( Qt::WA_DeleteOnClose, true );
#if defined(Q_WS_S60) || defined(Q_WS_MAEMO_5)
    mainfrm.showMaximized();
#else
    mainfrm.show();
#endif
    return app.exec();
}
