#ifndef KEYPADUNT_H
#define KEYPADUNT_H

#include <QtGui/QDialog>
#include <QtCore/QSet>

namespace Ui {
class TKeypadFrm;
}

class TKeypadFrm : public QDialog
{
    Q_OBJECT
    
public:
    explicit TKeypadFrm(QWidget *parent = 0);
    ~TKeypadFrm();
    
private:
    Ui::TKeypadFrm *ui;
    QSet<Qt::Key> fPressedKeys;
protected:
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);

};

class TKeyItem {
public:
    explicit TKeyItem(int ID, const QString& graphic, int matchedkeycode);
private:
    int fRow;
    int fColumn;
    bool fPressed;
    bool fHold;
    QString fGraphic;
    QSet<int> fMatchedKeycodes;
    QRect fRect;
public:
    void addKeycode(int matchedkeycode);
    void setRect(const QRect& rect);
    bool inRect(const QPoint& point);
    void paintSelf(QImage& image);
    bool press(int keycode);
    bool release(int keycode);
    void hold();
    void press();
    void release();
    bool pressed();
    int row();
    int column();
};

#endif // KEYPADUNT_H
