#ifndef MAINUNT_H
#define MAINUNT_H

#include <QtGui/QMainWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QMimeData>
#include <QtCore/QDateTime>
#include <QtGui/QTreeWidgetItem>
#include "Common.h"
#include <QtGui/QLabel>
#include <QtGui/QProgressbar>
#include <QtGui/QImage>
#include <QtCore/QTranslator>
#include "KeypadUnt.h"


typedef struct tagLCDStripe {
    QImage bitmap;
    int left, top;
} TLCDStripe;


namespace Ui {
    class TMainFrm;
}


class TMainFrm : public QMainWindow {
    Q_OBJECT
public:
    explicit TMainFrm(QWidget *parent = 0);
    ~TMainFrm();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::TMainFrm *ui;
    QFileDialog OpenSaveFileDialog;
    QString fLastSavedProjectFilename;
    QLabel *action;
    QProgressBar *progress;
    QTime fPrepareBufferStartTime;
    bool fDiscardRenameCheck;
    QString fBookfilename;
    QTranslator* fTranslator;
    QString fLastLangCode;
    QSet<Qt::Key> fPressedKeys;
    TKeyItem* fKeyItems[8][8];
    TLCDStripe* fLCDStripes;
    QImage fLCDEmpty, fLCDPixel;
private:
    //void TryAcceptDrop( QFileInfo &info, int maxdepth, int &bypasscount, int &acceptcount );
    //void TryAcceptPages( QFileInfo &info, int maxdepth, int chapterindex, int &bypasscount, int &acceptcount );
    //void TryAcceptXmlFolder( QFileInfo &info, OpenBookWriter* writer, int baselen, int maxdepth, int &bypasscount, int &acceptcount );
    void initKeypad();
    void repaintKeypad();
    void updateKeypadMatrix();
    void initLcdStripe();
    void DrawShadowOrPixel( QByteArray* buffer, QPainter &painter, bool semishadow );

protected:
    virtual bool eventFilter(QObject*, QEvent*);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
public:
    void StoreTranslator( QTranslator* translator );
private slots:
    void onEmulationStartClicked();
    void onEmulationRestartClicked();
    void onEmulationTestClicked();
    void onBenchmarkClicked();
    void onLanguageEnglishClicked();
    void onLanguageChineseClicked();
    //void onToolsOptionsClicked();
    void onHelpContentsClicked();
    void onKeypadSizeChanged(int, int);
    void onMouseDown(int x, int y);
    void onMouseUp(int x, int y);
    
public slots:
    //void writeLog(QString content, TLogType logtype = ltMessage);
    void onStepFinished(quint16 pc);
    void onLCDBufferChanged(QByteArray* buffer);

};

QString LogTypeToString( TLogType logtype );
QString localLanguage();
#endif // MAINUNT_H
