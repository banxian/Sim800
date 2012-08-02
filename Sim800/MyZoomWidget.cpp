#include "MyZoomWidget.h"

MyZoomWidget::MyZoomWidget(QWidget *parent)
    :QLabel(parent)
{

}

#ifndef QT_NO_GESTURES
//bool MyZoomWidget::event( QEvent * e )
//{
//    if (e->type() == QEvent::Gesture)
//        return gestureEvent(static_cast<QGestureEvent*>(e));
//    return QWidget::event(e);
//}

//void MyZoomWidget::gestureEvent( QGestureEvent * e )
//{

//}
#endif

void MyZoomWidget::mousePressEvent( QMouseEvent * e )
{
    emit mousePressed(e->x(), e->y());
    e->accept();
}

void MyZoomWidget::mouseReleaseEvent( QMouseEvent * e )
{
    emit mouseReleased(e->x(), e->y());
    e->accept();
}

void MyZoomWidget::mouseDoubleClickEvent( QMouseEvent * e )
{
    emit mouseDoubleClicked(e->x(), e->y());
    e->accept();
}

void MyZoomWidget::mouseMoveEvent( QMouseEvent * e )
{
    emit mouseMoved(e->x(), e->y());
    e->accept();
}

void MyZoomWidget::resizeEvent( QResizeEvent * e )
{
    emit resized(e->size().width(), e->size().height());
    e->accept();
}
