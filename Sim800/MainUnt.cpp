#include "MainUnt.h"
#include "ui_MainFrm.h"
#include <QtCore/QFile>
#include "DbCentre.h"
#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#include <Windows.h>
#include "NekoDriver.h"
#include "AddonFuncUnt.h"
#include <QtCore/QRegExp>
#include <QtGui/QInputDialog>
#include <QtGUI/QDesktopServices>
#include <QtCore/QSettings>
#include <QtXml/QDomDocument>
extern "C" {
#include "ANSI/65c02.h"
}


TMainFrm::TMainFrm(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TMainFrm)
    , fDiscardRenameCheck(false)
    , fLCDStripes(NULL)
{
    ui->setupUi(this);
    LoadAppSettings();

    // Additionally Initialize
    if (StateSetting.WindowMaxium) {
        setWindowState(Qt::WindowMaximized);
    }
    if (StateSetting.MainFrmState.isEmpty() == false) {
        restoreState(StateSetting.MainFrmState);
    }
    if (StateSetting.RegisterLayoutState.isEmpty() == false) {
        ui->splitterforregister->restoreState(StateSetting.RegisterLayoutState);
    }
    if (StateSetting.MessageLayoutState.isEmpty() == false) {
        ui->splitterforoutput->restoreState(StateSetting.MessageLayoutState);
    }
    ui->logView->horizontalHeader()->resizeSection(0, 60);
    ui->logView->horizontalHeader()->resizeSection(1, 80);

    // chibimaho do koneko
    // TODO: move initialize to main
    //theNekoDriver = new TNekoDriver(this);
    //theNekoDriver->SetDoublePageMode(MangaDoublePageMode(GlobalSetting.SplitPage));
    //fBookInfo.EnhanceLevel = GlobalSetting.EnhanceLevel;
    //fBookInfo.InternalCodec = GlobalSetting.OutputFormat;
    //fBookInfo.PageSize = GlobalSetting.OutputResolution;

    // QtDesigner const bug
    //QObject::connect(ui.projectView, SIGNAL(dropChanged(const QMimeData*)),
    //    this, SLOT(onProjectDropfiles(const QMimeData*)));
    QApplication::setApplicationName(tr("MangaBook"));

#ifndef _DEBUG
    // Nuke debug functions
    //ui->actionBookToFolder->setVisible(false);
    //ui->actionFolderToBook->setVisible(false);
#endif
    theNekoDriver = new TNekoDriver();
    connect(theNekoDriver, SIGNAL(lcdBufferChanged(QByteArray*)), 
            this, SLOT(onLCDBufferChanged(QByteArray*)));
    connect(theNekoDriver, SIGNAL(stepFinished(quint16)), 
        this, SLOT(onStepFinished(quint16)));

    FILTERKEYS oldfilterkeys;
    oldfilterkeys.cbSize = sizeof FILTERKEYS;
    bool result = SystemParametersInfo(SPI_GETFILTERKEYS, 
        sizeof FILTERKEYS, &oldfilterkeys, 0); 
    FILTERKEYS newfilterkeys = oldfilterkeys; 
    newfilterkeys.dwFlags |= FKF_FILTERKEYSON; 
    newfilterkeys.iWaitMSec = 0; 
    newfilterkeys.iBounceMSec = 0; 
    newfilterkeys.iDelayMSec = 0; 
    newfilterkeys.iDelayMSec = 0; 
    // turn on the FilterKeys feature to disable keyboard repeat 
    result = SystemParametersInfo(SPI_SETFILTERKEYS, 
        sizeof FILTERKEYS, &newfilterkeys, 0);

    // Get arrow key working
    QList<QWidget*> widgets = qFindChildren<QWidget*>(this);
    foreach(QWidget *widget, widgets)
        widget->installEventFilter(this);

    initKeypad();
    initLcdStripe();
    QByteArray* dummybuf = new QByteArray(160*80/8, 0xFFu);
    onLCDBufferChanged(dummybuf);

    // connect
    QObject::connect(ui->keypadView, SIGNAL(resized(int, int)), this, SLOT(onKeypadSizeChanged(int, int)));
    QObject::connect(ui->keypadView, SIGNAL(mousePressed(int, int)), this, SLOT(onMouseDown(int, int)));
    QObject::connect(ui->keypadView, SIGNAL(mouseReleased(int, int)), this, SLOT(onMouseUp(int, int)));
}

TMainFrm::~TMainFrm()
{
    StateSetting.WindowMaxium = windowState() == Qt::WindowMaximized;
    StateSetting.MainFrmState = saveState();
    StateSetting.RegisterLayoutState = ui->splitterforregister->saveState();
    StateSetting.MessageLayoutState = ui->splitterforoutput->saveState();

    FILTERKEYS oldfilterkeys;
    oldfilterkeys.cbSize = sizeof FILTERKEYS;
    bool result = SystemParametersInfo(SPI_GETFILTERKEYS, 
        sizeof FILTERKEYS, &oldfilterkeys, 0); 
    FILTERKEYS newfilterkeys = oldfilterkeys; 
    newfilterkeys.dwFlags &= ~FKF_FILTERKEYSON; 
    newfilterkeys.iWaitMSec = 0; 
    newfilterkeys.iBounceMSec = 0; 
    newfilterkeys.iDelayMSec = 0; 
    newfilterkeys.iDelayMSec = 0; 
    // turn on the FilterKeys feature to disable keyboard repeat 
    result = SystemParametersInfo(SPI_SETFILTERKEYS, 
        sizeof FILTERKEYS, &newfilterkeys, 0); 

    SaveAppSettings();
    theNekoDriver->StopEmulation();
    delete[] fLCDStripes;
    delete theNekoDriver;
    delete ui;
}

void TMainFrm::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TMainFrm::StoreTranslator( QTranslator* translator )
{
    fTranslator = translator;
}


void TMainFrm::onLanguageEnglishClicked()
{
    QApplication::removeTranslator(fTranslator);
    QFont f = QApplication::font();
    if (f.pointSize() == 9) {
        f.setPointSize(8);
        QApplication::setFont(f);
    }
    ui->retranslateUi(this);
    fLastLangCode = "enu";
}

void TMainFrm::onLanguageChineseClicked()
{
    QApplication::removeTranslator(fTranslator);
    fTranslator->load(QString("MangaBook_chs"), QApplication::applicationDirPath() + "/Language");
    QApplication::installTranslator(fTranslator);
    QFont f = QApplication::font();
    if (f.pointSize() == 8) {
        f.setPointSize(9); // TODO: Simsun 9 in Tahoma 8 fix
        QApplication::setFont(f);
    }
    ui->retranslateUi(this);
    fLastLangCode = "chs";
}

void TMainFrm::onHelpContentsClicked()
{
    QString smartchmname;
    if (fLastLangCode.isEmpty()) {
        smartchmname = QApplication::applicationDirPath() + "/MangaBookManual-" + localLanguage().toUpper() + ".chm";
    } else {
        smartchmname = QApplication::applicationDirPath() + "/MangaBookManual-" + fLastLangCode.toUpper() + ".chm";
    }
    if (QFileInfo(smartchmname).exists() == false) {
        smartchmname = QApplication::applicationDirPath() + "/MangaBookManual-ENU.chm";
    }
    QDesktopServices::openUrl(smartchmname);
}

void TMainFrm::writeLog( QString content, TLogType logtype /*= ltMessage*/ )
{
    if (content.isEmpty()) {
        content = "NULL";
    }
    QStringList list;
    list = content.split("\n");
    for (QStringList::iterator it = list.begin(); it != list.end(); it++) {
        if ((*it).isEmpty() && it + 1 == list.end()) {
            break;
        }
        int prevrowcount = ui->logView->rowCount();
        ui->logView->setRowCount(prevrowcount + 1);
        QTableWidgetItem *item;
        item = new QTableWidgetItem(QDateTime::currentDateTime().toString("hh:mm:ss"));
        ui->logView->setItem(prevrowcount, 0, item);
        item = new QTableWidgetItem(LogTypeToString(logtype));
        ui->logView->setItem(prevrowcount, 1, item);
        item = new QTableWidgetItem(*it);
        ui->logView->setItem(prevrowcount, 2, item);

        ui->logView->scrollToItem(item);
    }
    if (ui->logView->rowCount() < 80) {
        //ui->logView->resizeRowsToContents();
        //ui->logView->resizeColumnsToContents();
    }
}

void TMainFrm::onBenchmarkClicked()
{

}

void TMainFrm::onEmulationStartClicked()
{
    //theNekoDriver->StartEmulation();
    //theNekoDriver->StopEmulation();
    theNekoDriver->RunDemoBin("");
    //theNekoDriver->LoadFullNorFlash();
}

void TMainFrm::onEmulationRestartClicked()
{
    theNekoDriver->StopEmulation();
    theNekoDriver->StartEmulation();
}

void TMainFrm::onEmulationTestClicked()
{
    QString binname = OpenSaveFileDialog.getOpenFileName(this, tr("Load test binary"), PathSetting.LastSourceFolder, "PC1000 Binary Files (*.bin);;All Files (*.*)", 0, 0);
    if (binname.isEmpty()) {
        return;
    }
    PathSetting.LastSourceFolder = QFileInfo(binname).path();
    theNekoDriver->RunDemoBin(binname);
}

void TMainFrm::onStepFinished( quint16 pc )
{
    if (action == NULL) {
        action = new QLabel(tr("PC:0000"));
        action->setMinimumSize(120, 14);
        action->setMaximumSize(200, 18);
        action->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        ui->statusBar->addWidget(action);
    }
    //action->setText(QString(tr("PC:%1")).arg(ushort(pc), 4, 16, QLatin1Char('0')));
}


void TMainFrm::onLCDBufferChanged( QByteArray* buffer )
{
    QImage via(160, 80, QImage::Format_Mono);
    memcpy(via.bits(), buffer->data(), 160 * 80 / 8);
    QVector < QRgb > limelcdtable;
    limelcdtable.append(0xFFFFFDE8);
    limelcdtable.append(0xFF32284A);

    via.convertToFormat(QImage::Format_Indexed8);
    via.setColorTable(limelcdtable);
    via = via.scaled(320, 160);
    // via is still mono
    via = via.copy(-46, -2, 386, 164);
    QPainter painter(&via);
    painter.fillRect(46, 0, 2, 164, Qt::color0);
    DrawStripe(65, buffer, painter);
    for (int i = 79; i >= 0; i--)
    {
        if (i != 65) {
            DrawStripe(i, buffer, painter);
        }
    }
    painter.end();
    ui->lcdView->setPixmap(QPixmap::fromImage(via));
    ui->lcdView->repaint();
    delete buffer;
}

void TMainFrm::DrawStripe( int i, QByteArray* buffer, QPainter &painter )
{
    TLCDStripe* item = &fLCDStripes[i];
    char pixel = buffer->at((160/8) * i);
    if (pixel < 0) {
        painter.drawImage(item->left, item->top, item->bitmap);
    }
}


void TMainFrm::keyPressEvent( QKeyEvent * ev )
{
    fPressedKeys += (Qt::Key)ev->key();
    qDebug("0x%x pressed", ev->key());
    bool hitted = false;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item) {
                if (item->press(ev->key())) {
                    hitted = true;
                }
            }
        }
    }
    // pull up
    if (hitted) {
        repaintKeypad();
        //updateKeypadRegisters();
        updateKeypadMatrix();
    }
}

void TMainFrm::keyReleaseEvent( QKeyEvent * ev )
{
    fPressedKeys -= (Qt::Key)ev->key();
    qDebug("0x%x released", ev->key());
    bool hitted = false;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item) {
                if (item->release(ev->key())) {
                    hitted = true;

                }
            }
        }
    }
    // pull down
    if (hitted) {
        repaintKeypad();
        //updateKeypadRegisters();
        updateKeypadMatrix();
    }
}

bool TMainFrm::eventFilter( QObject*, QEvent* ev )
{
    if (ev->type() != QEvent::KeyPress) {
        return false;
    }
    keyPressEvent((QKeyEvent*)ev);
    return true;
}

void TMainFrm::repaintKeypad()
{
    QImage image(ui->keypadView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item && item->pressed() == false) {
                item->paintSelf(image);
            }
        }
    }
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item && item->pressed()) {
                item->paintSelf(image);
            }
        }
    }
    ui->keypadView->setPixmap(QPixmap::fromImage(image));
}

extern void CheckLCDOffShift0AndEnableWatchDog();
extern void AppendLog(const char* text);

void TMainFrm::updateKeypadMatrix()
{
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item) {
                keypadmatrix[y][x] = item->pressed();
            }
        }
    }
    // TODO: Check
    CheckLCDOffShift0AndEnableWatchDog();
    //AppendLog("keypadmatrix updated.");
}

void TMainFrm::onKeypadSizeChanged(int w, int h)
{
    // reset keypad size
    int itemwidth = (w - 2) / 10;
    int itemheight = (h - 2) / 6;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item) {
                //item->setRect(QRect(x * itemwidth + 1, y * itemheight + 1, itemwidth, itemheight));
                item->setRect(QRect(item->column() * itemwidth + 1, item->row() * itemheight + 1, itemwidth, itemheight));
            }
        }
    }
    repaintKeypad();
}

void TMainFrm::initKeypad()
{
    TKeyItem* item[8][8] = {
        NULL,       // P00, P10
        NULL,       // P01, P10
        new TKeyItem(18, "ON", Qt::Key_F12),        // P02, P10
        NULL,       // P03, P10
        NULL,       // P04, P10
        NULL,       // P05, P10
        NULL,       // P06, P10
        NULL,       // P07, P10

        new TKeyItem(0, "F5", Qt::Key_F5),          // P00, P11
        new TKeyItem(1, "F6", Qt::Key_F6),          // P01, P11
        new TKeyItem(2, "F7", Qt::Key_F7),          // P02, P11
        new TKeyItem(3, "F8", Qt::Key_F8),          // P03, P11
        new TKeyItem(4, "F9", Qt::Key_F9),          // P04, P11
        new TKeyItem(5, "F10", Qt::Key_F10),        // P05, P11
        new TKeyItem(6, "F11", Qt::Key_F11),        // P06, P11
        NULL,       // P07, P11

        new TKeyItem(50, "Help", Qt::Key_Control),  // P00, P12
        new TKeyItem(51, "Shift", Qt::Key_Shift),   // P01, P12
        new TKeyItem(52, "Caps", Qt::Key_CapsLock), // P02, P12
        new TKeyItem(53, "AC", Qt::Key_Escape),     // P03, P12
        new TKeyItem(54, "0", Qt::Key_0),           // P04, P12
        new TKeyItem(55, ".", Qt::Key_Period),      // P05, P12
        new TKeyItem(56, "=", Qt::Key_Equal),       // P06, P12
        new TKeyItem(57, "Left", Qt::Key_Left),     // P07, P12

        new TKeyItem(40, "Z", Qt::Key_Z),           // P00, P13
        new TKeyItem(41, "X", Qt::Key_X),           // P01, P13
        new TKeyItem(42, "C", Qt::Key_C),           // P02, P13
        new TKeyItem(43, "V", Qt::Key_V),           // P03, P13
        new TKeyItem(44, "B", Qt::Key_B),           // P04, P13
        new TKeyItem(45, "N", Qt::Key_N),           // P05, P13
        new TKeyItem(46, "M", Qt::Key_M),           // P06, P13
        new TKeyItem(47, "PgUp", Qt::Key_PageUp),   // P07, P13

        new TKeyItem(30, "A", Qt::Key_A),       // P00, P14
        new TKeyItem(31, "S", Qt::Key_S),       // P01, P14
        new TKeyItem(32, "D", Qt::Key_D),       // P02, P14
        new TKeyItem(33, "F", Qt::Key_F),       // P03, P14
        new TKeyItem(34, "G", Qt::Key_G),       // P04, P14
        new TKeyItem(35, "H", Qt::Key_H),       // P05, P14
        new TKeyItem(36, "J", Qt::Key_J),       // P06, P14
        new TKeyItem(37, "K", Qt::Key_K),       // P07, P14

        new TKeyItem(20, "Q", Qt::Key_Q),       // P00, P15
        new TKeyItem(21, "W", Qt::Key_W),       // P01, P15
        new TKeyItem(22, "E", Qt::Key_E),       // P02, P15
        new TKeyItem(23, "R", Qt::Key_R),       // P03, P15
        new TKeyItem(24, "T", Qt::Key_T),       // P04, P15
        new TKeyItem(25, "Y", Qt::Key_Y),       // P05, P15
        new TKeyItem(26, "U", Qt::Key_U),       // P06, P15
        new TKeyItem(27, "I", Qt::Key_I),       // P07, P15

        new TKeyItem(28, "O", Qt::Key_O),           // P00, P16
        new TKeyItem(38, "L", Qt::Key_L),           // P01, P16
        new TKeyItem(48, "Up", Qt::Key_Up),         // P02, P16
        new TKeyItem(58, "Down", Qt::Key_Down),     // P03, P16
        new TKeyItem(29, "P", Qt::Key_P),           // P04, P16
        new TKeyItem(39, "Enter", Qt::Key_Enter),   // P05, P16
        new TKeyItem(49, "PgDn", Qt::Key_PageDown), // P06, P16
        new TKeyItem(59, "Right", Qt::Key_Right),   // P07, P16

        NULL,       // P00, P17
        NULL,       // P01, P17
        new TKeyItem(12, "F1", Qt::Key_F1),       // P02, P17
        new TKeyItem(13, "F2", Qt::Key_F2),       // P03, P17
        new TKeyItem(14, "F3", Qt::Key_F3),       // P04, P17
        new TKeyItem(15, "F4", Qt::Key_F4),       // P05, P17
        NULL,       // P06, P17
        NULL,       // P07, P17
    };
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            fKeyItems[y][x] = item[y][x];
            if (item[y][x] == NULL) {
                keypadmatrix[y][x] = 2;
            }
        }
    }
    onKeypadSizeChanged(ui->keypadView->width(), ui->keypadView->height());
}

void TMainFrm::onMouseDown( int x1, int y1 )
{
    bool hitted = false;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item && item->inRect(QPoint(x1, y1)) && item->pressed() == false) {
                item->press();
                hitted = true;
            }
        }
    }
    if (hitted) {
        repaintKeypad();
        updateKeypadMatrix();
    }
}

void TMainFrm::onMouseUp( int x1, int y1 )
{
    bool hitted = false;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item && item->inRect(QPoint(x1, y1)) && item->pressed()) {
                item->release();
                hitted = true;
            }
        }
    }
    if (hitted) {
        repaintKeypad();
        updateKeypadMatrix();
    }
}

void TMainFrm::initLcdStripe()
{
    QImage PageUp = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_PGUP.png");
    QImage Star = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_STAR.png");
    QImage Num = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_NUM.png");
    QImage Eng = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_ENG.png");
    QImage Caps = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_CAPS.png");
    QImage Shift = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SHIFT.png");
    QImage Sound = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SOUND.png");
    QImage Flash = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_FLASH.png");
    QImage SharpBell = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SHARPBELL.png");
    QImage KeyClick = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_KEYCLICK.png");
    QImage Alaarm = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_ALARM.png");
    QImage Speaker = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SPEAKER.png");
    QImage Tape = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_TAPE.png");
    QImage Minus = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_MINUS.png");
    QImage Battery = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_BATTERY.png");
    QImage Secret = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SECRET.png");
    QImage PageLeft = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_PGLEFT.png");
    QImage PageRight = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_PGRIGHT.png");
    QImage PageDown = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_PGDOWN.png");
    QImage Left = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_LEFT.png");
    QImage HonzFrame = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_HFRAME.png");
    QImage Microphone = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_MICROPHONE.png");
    QImage HonzBar = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_HBAR.png");
    QImage Right = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_RIGHT.png");
    QImage SevenVert1 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_V.png");
    QImage SevenVert2 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_V.png");
    QImage SevenVert3 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_V.png");
    QImage SevenVert4 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_V.png");
    QImage SevenHonz1 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_H.png");
    QImage SevenHonz2 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_H.png");
    QImage SevenHonz3 = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEVEN_H.png");
    QImage VertFrame = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_VFRAME.png");
    QImage Up = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_UP.png");
    QImage Down = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_DOWN.png");
    QImage Line = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_LINE.png");
    QImage VertBar = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_VBAR.png");
    QImage SemiColon = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_SEMICOLON.png");
    QImage Point = QImage(":/LCDStripe/PpsDES/lcdstripe/WQX_POINT.png");

    fLCDStripes = new TLCDStripe[80];

    fLCDStripes[0].bitmap = SevenVert3;
    fLCDStripes[0].left = 4;
    fLCDStripes[0].top = 3;
    fLCDStripes[1].bitmap = SevenHonz1;
    fLCDStripes[1].left = 4;
    fLCDStripes[1].top = 3;
    fLCDStripes[2].bitmap = SevenVert1;
    fLCDStripes[2].left = 4;
    fLCDStripes[2].top = 3;
    fLCDStripes[3].bitmap = SevenHonz2;
    fLCDStripes[3].left = 4;
    fLCDStripes[3].top = 3;
    fLCDStripes[4].bitmap = Line;
    fLCDStripes[4].left = 369;
    fLCDStripes[4].top = 9;
    fLCDStripes[5].bitmap = SevenVert3;
    fLCDStripes[5].left = 14;
    fLCDStripes[5].top = 3;
    fLCDStripes[6].bitmap = SevenHonz1;
    fLCDStripes[6].left = 14;
    fLCDStripes[6].top = 3;
    fLCDStripes[7].bitmap = SevenVert1;
    fLCDStripes[7].left = 14;
    fLCDStripes[7].top = 3;
    fLCDStripes[8].bitmap = SevenHonz2;
    fLCDStripes[8].left = 14;
    fLCDStripes[8].top = 3;
    fLCDStripes[9].bitmap = SemiColon;
    fLCDStripes[9].left = 23;
    fLCDStripes[9].top = 5;
    fLCDStripes[10].bitmap = SevenVert3;
    fLCDStripes[10].left = 27;
    fLCDStripes[10].top = 3;
    fLCDStripes[11].bitmap = SevenHonz1;
    fLCDStripes[11].left = 27;
    fLCDStripes[11].top = 3;
    fLCDStripes[12].bitmap = Line;
    fLCDStripes[12].left = 369;
    fLCDStripes[12].top = 23;
    fLCDStripes[13].left = 27;
    fLCDStripes[13].bitmap = SevenVert1;
    fLCDStripes[14].bitmap = SevenHonz2;
    fLCDStripes[15].bitmap = SevenVert3;
    fLCDStripes[16].bitmap = SevenHonz1;
    fLCDStripes[17].bitmap = SevenVert1;
    fLCDStripes[18].bitmap = SevenHonz2;
    fLCDStripes[19].bitmap = SevenVert2;
    fLCDStripes[21].bitmap = SevenHonz3;
    fLCDStripes[22].bitmap = SevenVert4;
    fLCDStripes[23].bitmap = Point;
    fLCDStripes[24].bitmap = SevenVert2;
    fLCDStripes[25].bitmap = SevenHonz3;
    fLCDStripes[13].top = 3;
    fLCDStripes[14].left = 27;
    fLCDStripes[14].top = 3;
    fLCDStripes[15].left = 37;
    fLCDStripes[15].top = 3;
    fLCDStripes[16].left = 37;
    fLCDStripes[16].top = 3;
    fLCDStripes[17].left = 37;
    fLCDStripes[17].top = 3;
    fLCDStripes[18].left = 37;
    fLCDStripes[18].top = 3;
    fLCDStripes[19].left = 37;
    fLCDStripes[19].top = 3;
    fLCDStripes[20].bitmap = Line;
    fLCDStripes[20].left = 369;
    fLCDStripes[20].top = 41;
    fLCDStripes[21].left = 37;
    fLCDStripes[21].top = 3;
    fLCDStripes[22].left = 37;
    fLCDStripes[22].top = 3;
    fLCDStripes[23].left = 35;
    fLCDStripes[23].top = 19;
    fLCDStripes[24].left = 27;
    fLCDStripes[24].top = 3;
    fLCDStripes[25].left = 27;
    fLCDStripes[25].top = 3;
    fLCDStripes[26].bitmap = SevenVert4;
    fLCDStripes[26].left = 27;
    fLCDStripes[26].top = 3;
    fLCDStripes[27].bitmap = Point;
    fLCDStripes[29].bitmap = SevenVert2;
    fLCDStripes[30].bitmap = SevenHonz3;
    fLCDStripes[29].top = 3;
    fLCDStripes[30].top = 3;
    fLCDStripes[31].bitmap = SevenVert4;
    fLCDStripes[31].top = 3;
    fLCDStripes[33].top = 3;
    fLCDStripes[34].top = 3;
    fLCDStripes[35].top = 3;
    fLCDStripes[32].bitmap = Point;
    fLCDStripes[37].bitmap = Star;
    fLCDStripes[33].bitmap = SevenVert2;
    fLCDStripes[37].top = 23;
    fLCDStripes[38].top = 23;
    fLCDStripes[34].bitmap = SevenHonz3;
    fLCDStripes[38].bitmap = PageUp;
    fLCDStripes[39].bitmap = Num;
    fLCDStripes[27].left = 23;
    fLCDStripes[27].top = 19;
    fLCDStripes[28].bitmap = Line;
    fLCDStripes[28].left = 369;
    fLCDStripes[28].top = 55;
    fLCDStripes[29].left = 14;
    fLCDStripes[30].left = 14;
    fLCDStripes[31].left = 14;
    fLCDStripes[32].left = 12;
    fLCDStripes[32].top = 19;
    fLCDStripes[33].left = 4;
    fLCDStripes[34].left = 4;
    fLCDStripes[35].bitmap = SevenVert4;
    fLCDStripes[35].left = 4;
    fLCDStripes[36].bitmap = Line;
    fLCDStripes[36].left = 369;
    fLCDStripes[36].top = 73;
    fLCDStripes[37].left = 29;
    fLCDStripes[38].left = 17;
    fLCDStripes[39].left = 15;
    fLCDStripes[39].top = 34;
    fLCDStripes[40].bitmap = Eng;
    fLCDStripes[41].bitmap = Caps;
    fLCDStripes[43].top = 32;
    fLCDStripes[46].bitmap = Flash;
    fLCDStripes[47].bitmap = Sound;
    fLCDStripes[48].bitmap = KeyClick;
    fLCDStripes[49].bitmap = Alaarm;
    fLCDStripes[40].left = 15;
    fLCDStripes[50].bitmap = Speaker;
    fLCDStripes[51].bitmap = SharpBell;
    fLCDStripes[41].left = 15;
    fLCDStripes[42].bitmap = Shift;
    fLCDStripes[42].left = 15;
    fLCDStripes[51].left = 15;
    fLCDStripes[40].top = 43;
    fLCDStripes[41].top = 52;
    fLCDStripes[42].top = 62;
    fLCDStripes[43].bitmap = VertBar;
    fLCDStripes[43].left = 6;
    fLCDStripes[44].bitmap = Line;
    fLCDStripes[44].left = 369;
    fLCDStripes[44].top = 87;
    fLCDStripes[45].bitmap = VertBar;
    fLCDStripes[45].left = 6;
    fLCDStripes[45].top = 44;
    fLCDStripes[46].left = 30;
    fLCDStripes[46].top = 72;
    fLCDStripes[47].left = 15;
    fLCDStripes[47].top = 72;
    fLCDStripes[48].left = 32;
    fLCDStripes[48].top = 85;
    fLCDStripes[49].left = 15;
    fLCDStripes[49].top = 96;
    fLCDStripes[50].left = 30;
    fLCDStripes[50].top = 96;
    fLCDStripes[51].top = 85;
    fLCDStripes[52].bitmap = Line;
    fLCDStripes[52].left = 369;
    fLCDStripes[52].top = 105;
    fLCDStripes[53].bitmap = Microphone;
    fLCDStripes[53].left = 30;
    fLCDStripes[53].top = 107;
    fLCDStripes[54].bitmap = Tape;
    fLCDStripes[55].bitmap = Minus;
    fLCDStripes[58].bitmap = Battery;
    fLCDStripes[59].bitmap = Secret;
    fLCDStripes[61].bitmap = PageLeft;
    fLCDStripes[62].bitmap = PageRight;
    fLCDStripes[63].bitmap = Left;
    fLCDStripes[64].bitmap = PageDown;
    fLCDStripes[65].bitmap = VertFrame;
    fLCDStripes[66].bitmap = Down;
    fLCDStripes[54].left = 15;
    fLCDStripes[54].top = 107;
    fLCDStripes[55].left = 15;
    fLCDStripes[55].top = 116;
    fLCDStripes[56].bitmap = VertBar;
    fLCDStripes[56].left = 6;
    fLCDStripes[56].top = 56;
    fLCDStripes[57].bitmap = VertBar;
    fLCDStripes[57].left = 6;
    fLCDStripes[57].top = 92;
    fLCDStripes[58].left = 15;
    fLCDStripes[58].top = 121;
    fLCDStripes[59].left = 30;
    fLCDStripes[59].top = 121;
    fLCDStripes[60].bitmap = Line;
    fLCDStripes[60].left = 369;
    fLCDStripes[60].top = 119;
    fLCDStripes[61].left = 15;
    fLCDStripes[61].top = 130;
    fLCDStripes[62].left = 32;
    fLCDStripes[62].top = 130;
    fLCDStripes[63].left = 35;
    fLCDStripes[63].top = 142;
    fLCDStripes[64].left = 17;
    fLCDStripes[64].top = 140;
    fLCDStripes[65].left = 4;
    fLCDStripes[65].top = 23;
    fLCDStripes[66].left = 5;
    fLCDStripes[66].top = 141;
    fLCDStripes[67].bitmap = HonzBar;
    fLCDStripes[67].left = 13;
    fLCDStripes[69].bitmap = HonzBar;
    fLCDStripes[70].bitmap = Right;
    fLCDStripes[71].bitmap = HonzBar;
    fLCDStripes[73].bitmap = VertBar;
    fLCDStripes[75].bitmap = VertBar;
    fLCDStripes[76].bitmap = VertBar;
    fLCDStripes[77].bitmap = VertBar;
    fLCDStripes[78].bitmap = VertBar;
    fLCDStripes[72].bitmap = HonzFrame;
    fLCDStripes[79].bitmap = Up;
    fLCDStripes[67].top = 152;
    fLCDStripes[68].bitmap = Line;
    fLCDStripes[68].left = 369;
    fLCDStripes[68].top = 137;
    fLCDStripes[69].left = 23;
    fLCDStripes[69].top = 152;
    fLCDStripes[70].left = 369;
    fLCDStripes[70].top = 142;
    fLCDStripes[71].left = 33;
    fLCDStripes[71].top = 152;
    fLCDStripes[72].left = 5;
    fLCDStripes[72].top = 151;
    fLCDStripes[73].left = 6;
    fLCDStripes[73].top = 128;
    fLCDStripes[74].bitmap = Line;
    fLCDStripes[74].left = 369;
    fLCDStripes[74].top = 151;
    fLCDStripes[75].left = 6;
    fLCDStripes[75].top = 116;
    fLCDStripes[76].left = 6;
    fLCDStripes[76].top = 104;
    fLCDStripes[77].left = 6;
    fLCDStripes[77].top = 80;
    fLCDStripes[78].left = 6;
    fLCDStripes[78].top = 68;
    fLCDStripes[79].left = 5;
    fLCDStripes[79].top = 24;

    for (int i = 0; i < 80; i++)
    {
//        bitmap = (void *)*(pleft - 1);
//        if ( bitmap == sevenvert1 )
//        {
//            *pleft += 6;
//            goto LABEL_16;
//        }
//        if ( bitmap == g_hSevenVert2 )
//        {
//            *pleft += 6;
//LABEL_14:
//            top = pleft[1] + 9;
//LABEL_15:
//            pleft[1] = top;
//            goto LABEL_16;
//        }
//        if ( bitmap == g_hSevenHonz3 )
//        {
//            top = pleft[1] + 18;
//            goto LABEL_15;
//        }
//        if ( bitmap == g_hSevenVert4 || bitmap == g_hSevenHonz2 )
//            goto LABEL_14;
//LABEL_16:
//        left = *pleft;
//        pleft += 5;
//        *(pleft - 5) = left + 21;               // prev left
//        --loopcount;
//        *(pleft - 4) += 25;                     // prev top
        TLCDStripe* item = &fLCDStripes[i];
        //QImage* bitmap = &item->bitmap;
        if ( i == 2 || i == 7 || i == 13 || i == 17 ) {
            item->left += 6;
        } else if ( i == 19 || i == 24 || i == 29 || i == 33 ) {
            item->left += 6;
            item->top += 9;
        } else if ( i == 21 || i == 25 || i == 30 || i == 34 ) {
            item->top += 18;
        } else if ( i == 22 || i == 26 || i == 31 || i == 35 || i == 3 || i == 8 || i == 14 || i == 18 ) {
            item->top += 9;
        }
    }
}

QString LogTypeToString( TLogType logtype )
{
    switch (logtype)
    {
    case ltHint:
        return "Hint";
        break;
    case ltDebug:
        return "Debug";
        break;
    case ltMessage:
        return "Message";
        break;
    case ltError:
        return "Error";
        break;
    }
    return "Unknown";
}

QString localLanguage() {
    QLocale locale = QLocale::system();

    // This switch will translate to the correct locale
    switch (locale.language()) {
        case QLocale::German:
            return "deu";
        case QLocale::French:
            return "fra";
        case QLocale::Chinese:
            return "chs";
        case QLocale::HongKong:
        case QLocale::Macau:
        case QLocale::Taiwan:
            return "cht";
        case QLocale::Spanish:
            return "spa";
        case QLocale::Japanese:
            return "ja";
        case QLocale::Korean:
            return "kor";
        case QLocale::Belgium:
        case QLocale::Netherlands:
        case QLocale::NetherlandsAntilles:
            return "dut";
        default:
            return "enu";
    }
}
