#ifndef MAINUNT_H
#define MAINUNT_H

#include <QtGui/QMainWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QMimeData>
#include <QtCore/QDateTime>
#include <QtGui/QTreeWidgetItem>
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
    QTranslator* fTranslator;
    QString fLastLangCode;
    QSet<Qt::Key> fPressedKeys;
    TKeyItem* fKeyItems[8][8];
    TLCDStripe* fLCDStripes;
    QImage fLCDEmpty, fLCDPixel;
private:
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
    void onLCDBufferChanged(QByteArray* buffer);

};

QString localLanguage();
#endif // MAINUNT_H
