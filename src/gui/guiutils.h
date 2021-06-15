#ifndef GUIUTILS_H
#define GUIUTILS_H

#include <QGraphicsView>
#include <QItemDelegate>

class QMainWindow;
class QWheelEvent;
class QGraphicsScene;

namespace GuiUtils
{
    void SetWindowFont(QMainWindow *w, int ptsize);
    QIcon createColorCircleIcon(QSize size, Qt::GlobalColor color);
    bool AssureWidgetIsWithinVisibleArea(QWidget* w);
}

class myQGraphicsView : public QGraphicsView
{
   Q_OBJECT
public:
  myQGraphicsView( QWidget * parent = 0 );
  myQGraphicsView( QGraphicsScene * scene, QWidget * parent = 0 );
  void setCursorMode(int mode) {CursorMode = mode;}
signals:
  void MouseMovedSignal(QPointF *Pos);
protected:
  virtual void wheelEvent ( QWheelEvent * event );
  void mouseMoveEvent(QMouseEvent *event);
  void enterEvent(QEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);

private:
  QPointF MousePosition;
  int CursorMode;
};

class TableDoubleDelegateClass : public QItemDelegate
{
  public:
    TableDoubleDelegateClass(QObject *parent = 0);

    QWidget* createEditor(QWidget *parent,
                          const QStyleOptionViewItem &/* option */,
                          const QModelIndex &/* index */) const;
};

#endif // GUIUTILS_H
