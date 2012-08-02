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

TKeyItem::TKeyItem( int ID, QString& graphic, int matchedkeycode )
    : row(ID / 8)
    , column(ID % 8)
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
    QPainter painter(&image);
    QBrush framebrush;
    painter.setPen(QPen(QColor(0x14C906), 2, Qt::SolidLine));
    QLinearGradient linearGradient(0, 0, 474, 36);
    linearGradient.setColorAt(0.0, Qt::white);
    linearGradient.setColorAt(0.2, Qt::lightGray);
    linearGradient.setColorAt(0.6, Qt::lightGray);
    linearGradient.setColorAt(1.0, Qt::darkGray);
    painter.setBrush(linearGradient);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setOpacity(0.5);
    painter.drawRoundedRect(fRect, 4, 4, Qt::AbsoluteSize);
    painter.drawText(fRect, Qt::AlignCenter | Qt::AlignHCenter | Qt::TextWrapAnywhere, fGraphic);
}
