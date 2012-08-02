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
    explicit TKeyItem(int ID, QString& graphic, int matchedkeycode);
private:
    int row;
    int column;
    bool fPressed;
    bool fHold;
    QString& fGraphic;
    int fMatchedKeycode;
    QRect fRect;
public:
    void setRect(const QRect& rect);
    bool inRect(const QPoint& point);
    void paintSelf(QImage& image);
};

#endif // KEYPADUNT_H
