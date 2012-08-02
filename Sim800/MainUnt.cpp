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
    qDebug("%d pressed", ev->key());
}

void TMainFrm::keyReleaseEvent( QKeyEvent * ev )
{
    fPressedKeys -= (Qt::Key)ev->key();
    qDebug("%d released", ev->key());
}

bool TMainFrm::eventFilter( QObject*, QEvent* ev )
{
    if (ev->type() != QEvent::KeyPress) {
        return false;
    }
    keyPressEvent((QKeyEvent*)ev);
    return true;
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
