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
    , fMatchedKeycode(matchedkeycode)
{

}

void TKeyItem::setRect( const QRect& rect )
{
    fRect = rect;
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
    if (fHold) {
        painter.setPen(QPen(QColor(0x14C906), 2, Qt::SolidLine));
        framebrush.setColor(QColor(0x9AC986));
    } else if (fPressed) {
        painter.setPen(QPen(QColor(0x404906), 2, Qt::SolidLine));
        framebrush.setColor(Qt::lightGray);
    } else {
        painter.setPen(QPen(QColor(0x80C946), 2, Qt::SolidLine));
        framebrush.setColor(Qt::darkGray);
    }
    //QLinearGradient linearGradient(0, 0, 474, 36);
    //linearGradient.setColorAt(0.0, Qt::white);
    //linearGradient.setColorAt(0.2, Qt::lightGray);
    //linearGradient.setColorAt(0.6, Qt::lightGray);
    //linearGradient.setColorAt(1.0, Qt::darkGray);
    //painter.setBrush(linearGradient);
    painter.setBrush(framebrush);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setOpacity(0.5);
    painter.drawRoundedRect(fRect, 4, 4, Qt::AbsoluteSize);
    painter.drawText(fRect, Qt::AlignCenter | Qt::AlignHCenter | Qt::TextWrapAnywhere, fGraphic);
}

bool TKeyItem::press( int keycode )
{
    if (keycode == fMatchedKeycode) {
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
    if (keycode == fMatchedKeycode) {
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
