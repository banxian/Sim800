#include "KeypadUnt.h"
#include "ui_KeypadFrm.h"
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>


TKeypadFrm::TKeypadFrm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TKeypadFrm)
{
    ui->setupUi(this);
}

TKeypadFrm::~TKeypadFrm()
{
    delete ui;
}

void TKeypadFrm::keyPressEvent( QKeyEvent* ev )
{
    fPressedKeys += (Qt::Key)ev->key();
    qDebug("%d pressed", ev->key());
}

void TKeypadFrm::keyReleaseEvent( QKeyEvent* ev )
{
    fPressedKeys -= (Qt::Key)ev->key();
    qDebug("%d released", ev->key());
}

TKeyItem::TKeyItem( int ID, const QString& graphic, int matchedkeycode )
    : fRow(ID / 10)
    , fColumn(ID % 10)
    , fPressed(false)
    , fHold(false)
    , fGraphic(graphic)
    //, fMatchedKeycode(matchedkeycode)
{
    fMatchedKeycodes.insert(matchedkeycode);
}

void TKeyItem::addKeycode( int matchedkeycode )
{
    fMatchedKeycodes.insert(matchedkeycode);
}

void TKeyItem::setRect( const QRect& rect )
{
    fRect = rect;
    // TODO: change font size

}

void TKeyItem::setSubscript( const QString& subscript )
{
    fSubscript = subscript;
}

bool TKeyItem::inRect( const QPoint& point )
{
    return fRect.contains(point);
}

void TKeyItem::paintSelf( QImage& image )
{
    if (fRect.isEmpty()) {
        return;
    }
    QPainter painter(&image);
    QBrush framebrush;
    QColor bgcolor;
    if (fHold) {
        painter.setPen(QPen(QColor(0x14C906), 2, Qt::SolidLine));
        bgcolor = QColor(0x9AC986);
    } else if (fPressed) {
        painter.setPen(QPen(QColor(0x404906), 2, Qt::SolidLine));
        bgcolor = Qt::lightGray;
    } else {
        painter.setPen(QPen(QColor(0x80C946), 2, Qt::SolidLine));
        bgcolor = QColor(0xD9E9CD);
    }
    //QLinearGradient linearGradient(0, 0, 474, 36);
    //linearGradient.setColorAt(0.0, Qt::white);
    //linearGradient.setColorAt(0.2, Qt::lightGray);
    //linearGradient.setColorAt(0.6, Qt::lightGray);
    //linearGradient.setColorAt(1.0, Qt::darkGray);
    //painter.setBrush(linearGradient);
    //painter.setBrush(framebrush);
    framebrush.setStyle(Qt::SolidPattern);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setOpacity(0.5);
    QBrush oldbrush = painter.brush();
    //framebrush.setStyle(Qt::NoBrush);
    framebrush.setColor(bgcolor);
    painter.setBrush(framebrush);
    painter.drawRoundedRect(fRect, 4, 4, Qt::AbsoluteSize);
    if (fSubscript.isEmpty() == false) {
        QPolygon subbg;
        subbg.append(QPoint(fRect.x() + fRect.width() * 2 / 3 + 2, fRect.y() + fRect.height() * 2 / 3));
        subbg.append(QPoint(fRect.x() + fRect.width(), fRect.y() + fRect.height() * 2 / 3));
        subbg.append(QPoint(fRect.x() + fRect.width(), fRect.y() + fRect.height() - 2));
        subbg.append(QPoint(fRect.x() + fRect.width() - 2, fRect.y() + fRect.height()));
        subbg.append(QPoint(fRect.x() + fRect.width() * 2 / 3 - 2, fRect.y() + fRect.height()));
        framebrush.setColor(painter.pen().color());
        // same color as frame
        painter.setBrush(framebrush);
        painter.drawPolygon(subbg);
    }
    painter.drawText(fRect, Qt::AlignCenter | Qt::AlignHCenter | Qt::TextWrapAnywhere, fGraphic);
    if (fSubscript.isEmpty() == false) {
        painter.setPen(QPen(QColor(0xFFFDE8), 2, Qt::SolidLine));
        painter.drawText(fRect.adjusted(2,2,-2,-2), Qt::AlignRight | Qt::AlignBottom | Qt::TextWrapAnywhere, fSubscript);
    }
    painter.setBrush(oldbrush);
}

bool TKeyItem::press( int keycode )
{
    if (fMatchedKeycodes.contains(keycode)) {
        fPressed = true;
        return true;
    }
    return false;
}

void TKeyItem::press()
{
    fPressed = true;
}

bool TKeyItem::release( int keycode )
{
    if (fMatchedKeycodes.contains(keycode)) {
        fPressed = false;
        return true;
    }
    return false;
}

void TKeyItem::release()
{
    fPressed = false;
}

bool TKeyItem::pressed()
{
    return fPressed;
}

void TKeyItem::hold()
{
    fPressed = true;
    fHold = true;
}

int TKeyItem::row()
{
    return fRow;
}

int TKeyItem::column()
{
    return fColumn;
}
