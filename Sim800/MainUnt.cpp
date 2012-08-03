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

    // connect
    QObject::connect(ui->keypadView, SIGNAL(resized(int, int)), this, SLOT(onKeypadSizeChanged(int, int)));
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


void TMainFrm::onReadyBuildBook()
{

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
    ui->lcdView->setPixmap(QPixmap::fromImage(via));
    ui->lcdView->repaint();
    delete buffer;
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

void TMainFrm::updateKeypadRegisters()
{
    theNekoDriver->PauseEmulation();
    byte port1control = mem[0x15];
    byte controlbit = 1;
    for (int y = 0; y < 8; y++) {
        byte src, dest;
        if ((port1control & controlbit) != 0) {
            src = 9;
            dest = 8;
        } else {
            src = 8;
            dest = 9;
        }
        byte srcbit = 1;
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[y][x];
            if (item) {
                if (item->pressed() && (mem[src] & srcbit) != 0) {
                    mem[dest] |= controlbit; // pull up
                }
                if (item->pressed() == false && (mem[src] & srcbit) == 0) {
                    mem[dest] &= ~controlbit; // pull down
                }
            }
            srcbit = srcbit << 1;
        }
        controlbit = controlbit << 1;
    }
    theNekoDriver->ResumeEmulation();
}


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
}


void TMainFrm::onKeypadSizeChanged(int w, int h)
{
    // reset keypad size
    int itemwidth = (w - 2) / 8;
    int itemheight = (h - 2) / 8;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TKeyItem* item = fKeyItems[7 - y][x];
            if (item) {
                item->setRect(QRect(x * itemwidth + 1, y * itemheight + 1, itemwidth, itemheight));
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
        NULL,       // P02, P10
        NULL,       // P03, P10
        NULL,       // P04, P10
        NULL,       // P05, P10
        NULL,       // P06, P10
        NULL,       // P07, P10

        NULL,       // P00, P11
        NULL,       // P01, P11
        NULL,       // P02, P11
        NULL,       // P03, P11
        NULL,       // P04, P11
        NULL,       // P05, P11
        NULL,       // P06, P11
        NULL,       // P07, P11

        new TKeyItem(16, "Help", Qt::Key_Control),  // P00, P12
        new TKeyItem(17, "Shift", Qt::Key_Shift),   // P01, P12
        new TKeyItem(18, "Caps", Qt::Key_CapsLock), // P02, P12
        new TKeyItem(19, "AC", Qt::Key_Escape),     // P03, P12
        new TKeyItem(20, "0", Qt::Key_0),           // P04, P12
        new TKeyItem(21, ".", Qt::Key_Period),      // P05, P12
        new TKeyItem(22, "=", Qt::Key_Equal),       // P06, P12
        new TKeyItem(23, "Left", Qt::Key_Left),     // P07, P12

        new TKeyItem(24, "Z", Qt::Key_Z),           // P00, P13
        new TKeyItem(25, "X", Qt::Key_X),           // P01, P13
        new TKeyItem(26, "C", Qt::Key_C),           // P02, P13
        new TKeyItem(27, "V", Qt::Key_V),           // P03, P13
        new TKeyItem(28, "B", Qt::Key_B),           // P04, P13
        new TKeyItem(29, "N", Qt::Key_N),           // P05, P13
        new TKeyItem(30, "M", Qt::Key_M),           // P06, P13
        new TKeyItem(31, "PgUp", Qt::Key_PageUp),   // P07, P13

        new TKeyItem(32, "A", Qt::Key_A),       // P00, P14
        new TKeyItem(33, "S", Qt::Key_S),       // P01, P14
        new TKeyItem(34, "D", Qt::Key_D),       // P02, P14
        new TKeyItem(35, "F", Qt::Key_F),       // P03, P14
        new TKeyItem(36, "G", Qt::Key_G),       // P04, P14
        new TKeyItem(37, "H", Qt::Key_H),       // P05, P14
        new TKeyItem(38, "J", Qt::Key_J),       // P06, P14
        new TKeyItem(39, "K", Qt::Key_K),       // P07, P14

        new TKeyItem(40, "Q", Qt::Key_Q),       // P00, P15
        new TKeyItem(41, "W", Qt::Key_W),       // P01, P15
        new TKeyItem(42, "E", Qt::Key_E),       // P02, P15
        new TKeyItem(43, "R", Qt::Key_R),       // P03, P15
        new TKeyItem(44, "T", Qt::Key_T),       // P04, P15
        new TKeyItem(45, "Y", Qt::Key_Y),       // P05, P15
        new TKeyItem(46, "U", Qt::Key_U),       // P06, P15
        new TKeyItem(47, "I", Qt::Key_I),       // P07, P15

        new TKeyItem(48, "O", Qt::Key_O),           // P00, P16
        new TKeyItem(49, "L", Qt::Key_L),           // P01, P16
        new TKeyItem(50, "Up", Qt::Key_Up),         // P02, P16
        new TKeyItem(51, "Down", Qt::Key_Down),     // P03, P16
        new TKeyItem(52, "P", Qt::Key_P),           // P04, P16
        new TKeyItem(53, "Enter", Qt::Key_Enter),   // P05, P16
        new TKeyItem(54, "PgDn", Qt::Key_PageDown), // P06, P16
        new TKeyItem(55, "Right", Qt::Key_Right),   // P07, P16

        NULL,       // P00, P17
        NULL,       // P01, P17
        new TKeyItem(58, "F1", Qt::Key_F1),       // P02, P17
        new TKeyItem(59, "F2", Qt::Key_F2),       // P03, P17
        new TKeyItem(60, "F3", Qt::Key_F3),       // P04, P17
        new TKeyItem(61, "F4", Qt::Key_F4),       // P05, P17
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
