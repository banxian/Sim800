#include "MainUnt.h"
#include "ui_MainFrm.h"
#include <QtCore/QFile>
#include "DbCentre.h"
#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#ifdef _WIN32
#include <Windows.h>
#endif
#include "NekoDriver.h"
#include "AddonFuncUnt.h"
#include <QtCore/QRegExp>
#include <QtGui/QInputDialog>
#include <QtGUI/QDesktopServices>
#include <QtCore/QSettings>
#include <QtXml/QDomDocument>
extern "C" {
#include "ANSI/w65c02.h"
}


TMainFrm::TMainFrm(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TMainFrm)
    //, fDiscardRenameCheck(false)
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
    if (StateSetting.KeypadLayoutState.isEmpty() == false) {
        ui->splitterforkeypad->restoreState(StateSetting.KeypadLayoutState);
    }

    // chibimaho do koneko
    QApplication::setApplicationName(tr("SIM800"));

#ifndef _DEBUG
    // Nuke debug functions
    //ui->actionBookToFolder->setVisible(false);
    //ui->actionFolderToBook->setVisible(false);
#endif
    theNekoDriver = new TNekoDriver();
    connect(theNekoDriver, SIGNAL(lcdBufferChanged(QByteArray*)), 
            this, SLOT(onLCDBufferChanged(QByteArray*)));
    //connect(theNekoDriver, SIGNAL(stepFinished(quint16)), 
    //    this, SLOT(onStepFinished(quint16)));

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
    //QByteArray* dummybuf = new QByteArray(160*80/8, 0xFFu);
    //onLCDBufferChanged(dummybuf);

    // connect
    QObject::connect(ui->keypadView, SIGNAL(resized(int, int)), this, SLOT(onKeypadSizeChanged(int, int)));
    QObject::connect(ui->keypadView, SIGNAL(mousePressed(int, int)), this, SLOT(onMouseDown(int, int)));
    QObject::connect(ui->keypadView, SIGNAL(mouseReleased(int, int)), this, SLOT(onMouseUp(int, int)));
}

TMainFrm::~TMainFrm()
{
    StateSetting.WindowMaxium = windowState() == Qt::WindowMaximized;
    StateSetting.MainFrmState = saveState();
    StateSetting.KeypadLayoutState = ui->splitterforkeypad->saveState();
    //StateSetting.MessageLayoutState = ui->splitterforoutput->saveState();

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

extern bool lcdoffshift0flag;

void TMainFrm::onLCDBufferChanged( QByteArray* buffer )
{
    // TODO: Anti alias
    QImage via(fLCDEmpty);

    if (via.depth() < 24) {
        via = via.convertToFormat(QImage::Format_RGB32);
    }
    QPainter painter(&via);

    if (lcdoffshift0flag) {
        painter.setOpacity(0.8);
        painter.fillRect(via.rect(), QColor(0xFFFFFDE8));
    } else {
        DrawShadowOrPixel(buffer, painter, true);
        DrawShadowOrPixel(buffer, painter, false);
    }

    painter.end();
    ui->lcdView->setPixmap(QPixmap::fromImage(via));
    ui->lcdView->repaint();
    delete buffer;
}

void TMainFrm::DrawShadowOrPixel( QByteArray* buffer, QPainter &painter, bool semishadow )
{
    if (semishadow) {
        painter.setOpacity(0.2);
        QTransform shadow;
        shadow.translate(2, 2);
        painter.setTransform(shadow, true);
    } else {
        painter.resetTransform();
        painter.setOpacity(1);
    }
    for (int y = 79; y >= 0; y--)
    {
        char pixel = buffer->at((160/8) * y);
        if (pixel < 0) {
            TLCDStripe* item = &fLCDStripes[y];
            //painter.drawImage(QRect(item->left * 2 - 2, item->top * 2 - 2, item->bitmap.width(), item->bitmap.height()), item->bitmap);
            painter.drawImage(QRect(item->left, item->top, item->bitmap.width(), item->bitmap.height()), item->bitmap);
        }
    }

    int index = 0;
    int yp = 2;
    for (int y = 0; y < 80; y++) {
        int xp = 94 - 4 - 4;
        for (int i = 0; i < 160 / 8; i++) {
            const unsigned char pixel = static_cast<const unsigned char>(buffer->at(index));
            // try to shrink jump cost
            if (pixel) {
                if (i && (pixel & 0x80u)) {
                    painter.drawImage(xp, yp, fLCDPixel);
                }
                if (pixel & 0x40) {
                    painter.drawImage(xp + 4, yp, fLCDPixel);
                }
                if (pixel & 0x20) {
                    painter.drawImage(xp + 8, yp, fLCDPixel);
                }
                if (pixel & 0x10) {
                    painter.drawImage(xp + 12, yp, fLCDPixel);
                }
                if (pixel & 0x08) {
                    painter.drawImage(xp + 16, yp, fLCDPixel);
                }
                if (pixel & 0x04) {
                    painter.drawImage(xp + 20, yp, fLCDPixel);
                }
                if (pixel & 0x02) {
                    painter.drawImage(xp + 24, yp, fLCDPixel);
                }
                if (pixel & 0x01) {
                    painter.drawImage(xp + 28, yp, fLCDPixel);
                }
            }
            xp += 32;
            index++;
        }
        yp += 4;
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
        new TKeyItem(18, "ON/OFF", Qt::Key_F12),        // P02, P10
        NULL,       // P03, P10
        NULL,       // P04, P10
        NULL,       // P05, P10
        NULL,       // P06, P10
        NULL,       // P07, P10

        new TKeyItem(0, "Dictionary", Qt::Key_F5),          // P00, P11
        new TKeyItem(1, "Card", Qt::Key_F6),          // P01, P11
        new TKeyItem(2, "Calculator", Qt::Key_F7),          // P02, P11
        new TKeyItem(3, "Reminder", Qt::Key_F8),          // P03, P11
        new TKeyItem(4, "Data", Qt::Key_F9),          // P04, P11
        new TKeyItem(5, "Time", Qt::Key_F10),        // P05, P11
        new TKeyItem(6, "Network", Qt::Key_F11),        // P06, P11
        NULL,       // P07, P11

        new TKeyItem(50, "Help", Qt::Key_Control),  // P00, P12
        new TKeyItem(51, "Shift", Qt::Key_Shift),   // P01, P12
        new TKeyItem(52, "Caps", Qt::Key_CapsLock), // P02, P12
        new TKeyItem(53, "Esc", Qt::Key_Escape),     // P03, P12
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
        new TKeyItem(39, "Enter", Qt::Key_Return),   // P05, P16
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
    // number key
    item[3][4]->addKeycode(Qt::Key_1);
    item[3][5]->addKeycode(Qt::Key_2);
    item[3][6]->addKeycode(Qt::Key_3);
    item[4][4]->addKeycode(Qt::Key_4);
    item[4][5]->addKeycode(Qt::Key_5);
    item[4][6]->addKeycode(Qt::Key_6);
    item[5][4]->addKeycode(Qt::Key_7);
    item[5][5]->addKeycode(Qt::Key_8);
    item[5][6]->addKeycode(Qt::Key_9);

    item[3][4]->setSubscript("1");
    item[3][5]->setSubscript("2");
    item[3][6]->setSubscript("3");
    item[4][4]->setSubscript("4");
    item[4][5]->setSubscript("5");
    item[4][6]->setSubscript("6");
    item[5][4]->setSubscript("7");
    item[5][5]->setSubscript("8");
    item[5][6]->setSubscript("9");

    item[1][0]->setSubscript("C2E");
    item[1][1]->setSubscript("Note");
    item[1][2]->setSubscript("Conv");
    item[1][3]->setSubscript("Test");
    item[1][4]->setSubscript("Game");
    item[1][5]->setSubscript("Etc");

    item[2][1]->setSubscript("IME");
    item[2][3]->setSubscript("AC");

    item[6][0]->addKeycode(Qt::Key_Slash); // O /
    item[6][1]->addKeycode(Qt::Key_Asterisk); // L *
    item[6][2]->addKeycode(Qt::Key_Minus); // Up -
    item[6][3]->addKeycode(Qt::Key_Plus); // Down +

    item[2][6]->addKeycode(Qt::Key_Space); // =
    item[7][3]->addKeycode(Qt::Key_Backspace); // F2
    item[6][5]->addKeycode(Qt::Key_Enter); // keyboard vs keypad

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
    QImage PageUp = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_pgup.png");
    QImage Star = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_star.png");
    QImage Num = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_num.png");
    QImage Eng = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_eng.png");
    QImage Caps = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_caps.png");
    QImage Shift = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_shift.png");
    QImage Sound = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_sound.png");
    QImage Flash = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_flash.png");
    QImage SharpBell = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_sharpbell.png");
    QImage KeyClick = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_keyclick.png");
    QImage Alarm = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_alarm.png");
    QImage Speaker = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_speaker.png");
    QImage Tape = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_tape.png");
    QImage Minus = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_minus.png");
    QImage Battery = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_battery.png");
    QImage Secret = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_secret.png");
    QImage PageLeft = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_pgleft.png");
    QImage PageRight = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_pgright.png");
    QImage PageDown = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_pgdown.png");
    QImage Left = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_left.png");
    QImage HonzFrame = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_hframe.png");
    QImage Microphone = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_microphone.png");
    QImage HonzBar = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_hbar.png");
    QImage Right = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_right.png");
    QImage SevenVert1 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_vert1.png");
    QImage SevenVert2 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_vert2.png");
    QImage SevenVert3 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_vert3.png");
    QImage SevenVert4 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_vert4.png");
    QImage SevenHonz1 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_honz1.png");
    QImage SevenHonz2 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_honz2.png");
    QImage SevenHonz3 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_seven_honz3.png");
    QImage VertFrame = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_vframe.png");
    QImage Up = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_up.png");
    QImage Down = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_down.png");
    QImage Line = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_line.png");
    QImage Line5 = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_line5.png");
    QImage VertBar = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_vbar.png");
    QImage SemiColon = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_semicolon.png");
    QImage Point = QImage(":/LCDStripe/PpsDES/lcdstripe/lcd_point.png");
    fLCDPixel = QImage(":/LCDStripe/PpsDES/lcdstripe/lcdpixel.png");
    fLCDEmpty = QImage(":/LCDStripe/PpsDES/lcdstripe/lcdshadow.png");

    fLCDStripes = new TLCDStripe[80];

    // 7 segment display
    fLCDStripes[2].bitmap = SevenVert1;
    fLCDStripes[19].bitmap = SevenVert2;
    fLCDStripes[0].bitmap = SevenVert3;
    fLCDStripes[22].bitmap = SevenVert4;
    fLCDStripes[1].bitmap = SevenHonz1;
    fLCDStripes[3].bitmap = SevenHonz2;
    fLCDStripes[21].bitmap = SevenHonz3;
    int xdelta = 16;
    fLCDStripes[2].left  = 14;
    fLCDStripes[2].top   = 2;
    fLCDStripes[33].left = 14;
    fLCDStripes[33].top  = 12;
    fLCDStripes[0].left  = 5;
    fLCDStripes[0].top   = 2;
    fLCDStripes[35].left = 5;
    fLCDStripes[35].top  = 12;
    fLCDStripes[1].left  = 5;
    fLCDStripes[1].top   = 2;
    fLCDStripes[3].left  = 5;
    fLCDStripes[3].top   = 10;
    fLCDStripes[34].left = 5;
    fLCDStripes[34].top  = 19;

    fLCDStripes[7].bitmap = SevenVert1;
    fLCDStripes[24].bitmap = SevenVert2;
    fLCDStripes[5].bitmap = SevenVert3;
    fLCDStripes[26].bitmap = SevenVert4;
    fLCDStripes[6].bitmap = SevenHonz1;
    fLCDStripes[8].bitmap = SevenHonz2;
    fLCDStripes[25].bitmap = SevenHonz3;
    fLCDStripes[7].left  = fLCDStripes[2].left + xdelta;
    fLCDStripes[7].top   = fLCDStripes[2].top;
    fLCDStripes[29].left = fLCDStripes[33].left + xdelta;
    fLCDStripes[29].top  = fLCDStripes[33].top;
    fLCDStripes[5].left  = fLCDStripes[0].left + xdelta;
    fLCDStripes[5].top   = fLCDStripes[0].top;
    fLCDStripes[31].left = fLCDStripes[35].left + xdelta;
    fLCDStripes[31].top  = fLCDStripes[35].top;
    fLCDStripes[6].left  = fLCDStripes[1].left + xdelta;
    fLCDStripes[6].top   = fLCDStripes[1].top;
    fLCDStripes[8].left  = fLCDStripes[3].left + xdelta;
    fLCDStripes[8].top   = fLCDStripes[3].top;
    fLCDStripes[30].left = fLCDStripes[34].left + xdelta;
    fLCDStripes[30].top  = fLCDStripes[34].top;

    fLCDStripes[13].bitmap = SevenVert1;
    fLCDStripes[29].bitmap = SevenVert2;
    fLCDStripes[10].bitmap = SevenVert3;
    fLCDStripes[31].bitmap = SevenVert4;
    fLCDStripes[11].bitmap = SevenHonz1;
    fLCDStripes[14].bitmap = SevenHonz2;
    fLCDStripes[30].bitmap = SevenHonz3;
    xdelta += 2;
    fLCDStripes[13].left = fLCDStripes[7].left  + xdelta;
    fLCDStripes[13].top  = fLCDStripes[7].top;
    fLCDStripes[24].left = fLCDStripes[29].left + xdelta;
    fLCDStripes[24].top  = fLCDStripes[29].top;
    fLCDStripes[10].left = fLCDStripes[5].left  + xdelta;
    fLCDStripes[10].top  = fLCDStripes[5].top;
    fLCDStripes[26].left = fLCDStripes[31].left + xdelta;
    fLCDStripes[26].top  = fLCDStripes[31].top;
    fLCDStripes[11].left = fLCDStripes[6].left  + xdelta;
    fLCDStripes[11].top  = fLCDStripes[6].top;
    fLCDStripes[14].left = fLCDStripes[8].left  + xdelta;
    fLCDStripes[14].top  = fLCDStripes[8].top;
    fLCDStripes[25].left = fLCDStripes[30].left + xdelta;
    fLCDStripes[25].top  = fLCDStripes[30].top;
    xdelta -= 2;

    fLCDStripes[17].bitmap = SevenVert1;
    fLCDStripes[33].bitmap = SevenVert2;
    fLCDStripes[15].bitmap = SevenVert3;
    fLCDStripes[35].bitmap = SevenVert4;
    fLCDStripes[16].bitmap = SevenHonz1;
    fLCDStripes[18].bitmap = SevenHonz2;
    fLCDStripes[34].bitmap = SevenHonz3;
    fLCDStripes[17].left = fLCDStripes[13].left + xdelta;
    fLCDStripes[17].top  = fLCDStripes[13].top;
    fLCDStripes[19].left = fLCDStripes[24].left + xdelta;
    fLCDStripes[19].top  = fLCDStripes[24].top;
    fLCDStripes[15].left = fLCDStripes[10].left + xdelta;
    fLCDStripes[15].top  = fLCDStripes[10].top;
    fLCDStripes[22].left = fLCDStripes[26].left + xdelta;
    fLCDStripes[22].top  = fLCDStripes[26].top;
    fLCDStripes[16].left = fLCDStripes[11].left + xdelta;
    fLCDStripes[16].top  = fLCDStripes[11].top;
    fLCDStripes[18].left = fLCDStripes[14].left + xdelta;
    fLCDStripes[18].top  = fLCDStripes[14].top;
    fLCDStripes[21].left = fLCDStripes[25].left + xdelta;
    fLCDStripes[21].top  = fLCDStripes[25].top;

    fLCDStripes[32].bitmap = Point;
    fLCDStripes[32].left = 18;
    fLCDStripes[32].top = 24;
    fLCDStripes[9].bitmap = SemiColon;
    fLCDStripes[9].left = 35;
    fLCDStripes[9].top = 7;
    fLCDStripes[27].bitmap = Point;
    fLCDStripes[27].left = 35;
    fLCDStripes[27].top = 24;
    fLCDStripes[23].bitmap = Point;
    fLCDStripes[23].left = 53;
    fLCDStripes[23].top = 24;

    // right lines
    fLCDStripes[4].bitmap = Line;
    fLCDStripes[12].bitmap = Line5;
    fLCDStripes[20].bitmap = Line;
    fLCDStripes[28].bitmap = Line5;
    fLCDStripes[36].bitmap = Line;
    fLCDStripes[44].bitmap = Line5;
    fLCDStripes[52].bitmap = Line;
    fLCDStripes[60].bitmap = Line5;
    fLCDStripes[68].bitmap = Line;
    fLCDStripes[70].bitmap = Right;
    fLCDStripes[74].bitmap = Line;
    fLCDStripes[4].left = 743;
    fLCDStripes[4].top = 2;
    fLCDStripes[12].left = 743;
    fLCDStripes[12].top = 34;
    fLCDStripes[20].left = 743;
    fLCDStripes[20].top = 67;
    fLCDStripes[28].left = 743;
    fLCDStripes[28].top = 99;
    fLCDStripes[36].left = 743;
    fLCDStripes[36].top = 132;
    fLCDStripes[44].left = 743;
    fLCDStripes[44].top = 164;
    fLCDStripes[52].left = 743;
    fLCDStripes[52].top = 197;
    fLCDStripes[60].left = 743;
    fLCDStripes[60].top = 229;
    fLCDStripes[68].left = 743;
    fLCDStripes[68].top = 261;
    fLCDStripes[70].left = 743;
    fLCDStripes[70].top = 280;
    fLCDStripes[74].left = 743;
    fLCDStripes[74].top = 306;


    fLCDStripes[38].bitmap = PageUp;
    fLCDStripes[38].left = 29;
    fLCDStripes[38].top = 36;
    fLCDStripes[37].bitmap = Star;
    fLCDStripes[37].left = 47;
    fLCDStripes[37].top = 42;
    fLCDStripes[39].bitmap = Num;
    fLCDStripes[39].left = 25;
    fLCDStripes[39].top = 56;
    fLCDStripes[40].bitmap = Eng;
    fLCDStripes[40].left = 25;
    fLCDStripes[40].top = 77;
    fLCDStripes[41].bitmap = Caps;
    fLCDStripes[41].left = 25;
    fLCDStripes[41].top = 99;
    fLCDStripes[42].bitmap = Shift;
    fLCDStripes[42].left = 25;
    fLCDStripes[42].top = 120;
    fLCDStripes[46].bitmap = Flash;
    fLCDStripes[46].left = 48;
    fLCDStripes[46].top = 145;
    fLCDStripes[47].bitmap = Sound;
    fLCDStripes[47].left = 25;
    fLCDStripes[47].top = 147;
    fLCDStripes[48].bitmap = KeyClick;
    fLCDStripes[48].left = 52;
    fLCDStripes[48].top = 164;
    fLCDStripes[51].bitmap = SharpBell;
    fLCDStripes[51].left = 25;
    fLCDStripes[51].top = 166;
    fLCDStripes[50].bitmap = Speaker;
    fLCDStripes[50].left = 49;
    fLCDStripes[50].top = 185;
    fLCDStripes[49].bitmap = Alarm;
    fLCDStripes[49].left = 25;
    fLCDStripes[49].top = 186;
    fLCDStripes[53].bitmap = Microphone;
    fLCDStripes[53].left = 47;
    fLCDStripes[53].top = 203;
    fLCDStripes[54].bitmap = Tape;
    fLCDStripes[54].left = 25;
    fLCDStripes[54].top = 207;
    fLCDStripes[55].bitmap = Minus;
    fLCDStripes[55].left = 49;
    fLCDStripes[55].top = 226;
    fLCDStripes[58].bitmap = Battery;
    fLCDStripes[58].left = 26;
    fLCDStripes[58].top = 236;
    fLCDStripes[59].bitmap = Secret;
    fLCDStripes[59].left = 45;
    fLCDStripes[59].top = 240;
    fLCDStripes[61].bitmap = PageLeft;
    fLCDStripes[61].left = 25;
    fLCDStripes[61].top = 261;
    fLCDStripes[62].bitmap = PageRight;
    fLCDStripes[62].left = 47;
    fLCDStripes[62].top = 261;
    fLCDStripes[63].bitmap = Left;
    fLCDStripes[63].left = 48;
    fLCDStripes[63].top = 280;
    fLCDStripes[64].bitmap = PageDown;
    fLCDStripes[64].left = 29;
    fLCDStripes[64].top = 283;

    // vertframe
    fLCDStripes[65].bitmap = VertFrame;
    fLCDStripes[65].left = 5;
    fLCDStripes[65].top = 37;
    fLCDStripes[79].bitmap = Up;
    fLCDStripes[79].left = 10;
    fLCDStripes[79].top = 41;
    fLCDStripes[43].bitmap = VertBar;
    fLCDStripes[43].left = 10;
    fLCDStripes[43].top = 62;
    fLCDStripes[45].bitmap = VertBar;
    fLCDStripes[45].left = 10;
    fLCDStripes[45].top = 86;
    fLCDStripes[56].bitmap = VertBar;
    fLCDStripes[56].left = 10;
    fLCDStripes[56].top = 110;
    fLCDStripes[78].bitmap = VertBar;
    fLCDStripes[78].left = 10;
    fLCDStripes[78].top = 134;
    fLCDStripes[77].bitmap = VertBar;
    fLCDStripes[77].left = 10;
    fLCDStripes[77].top = 158;
    fLCDStripes[57].bitmap = VertBar;
    fLCDStripes[57].left = 10;
    fLCDStripes[57].top = 182;
    fLCDStripes[76].bitmap = VertBar;
    fLCDStripes[76].left = 10;
    fLCDStripes[76].top = 206;
    fLCDStripes[75].bitmap = VertBar;
    fLCDStripes[75].left = 10;
    fLCDStripes[75].top = 230;
    fLCDStripes[73].bitmap = VertBar;
    fLCDStripes[73].left = 10;
    fLCDStripes[73].top = 254;
    fLCDStripes[66].bitmap = Down;
    fLCDStripes[66].left = 10;
    fLCDStripes[66].top = 284;
    fLCDStripes[72].bitmap = HonzFrame;
    fLCDStripes[72].left = 5;
    fLCDStripes[72].top = 306;
    fLCDStripes[67].bitmap = HonzBar;
    fLCDStripes[67].left = 19;
    fLCDStripes[67].top = 310;
    fLCDStripes[69].bitmap = HonzBar;
    fLCDStripes[69].left = 30;
    fLCDStripes[69].top = 310;
    fLCDStripes[71].bitmap = HonzBar;
    fLCDStripes[71].left = 41;
    fLCDStripes[71].top = 310;
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
