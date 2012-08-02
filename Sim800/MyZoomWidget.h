#ifndef MYZOOMWIDGET_H
#define MYZOOMWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>


class MyZoomWidget : public QLabel
{
    Q_OBJECT
public:
    explicit MyZoomWidget(QWidget *parent = 0);

protected:
#ifndef QT_NO_GESTURES
    //bool event(QEvent *e);
    //void gestureEvent(QGestureEvent *e);
#endif
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void resizeEvent(QResizeEvent *);

signals:
    void mousePressed(int x, int y);
    void mouseReleased(int x, int y);
    void mouseDoubleClicked(int x, int y);
    void mouseMoved(int x, int y);
    void longPressStart(int x, int y);
    void longPressTimeout(int x, int y);
    void zoomIn(int x, int y);
    void zoomOut(int x, int y);
    void zoomEnd(int x, int y);
    void resized(int w, int h);

public slots:
    
};

#endif // MYZOOMWIDGET_H
