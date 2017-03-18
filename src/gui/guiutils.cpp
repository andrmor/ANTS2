#include "guiutils.h"

#include <QFont>
#include <QMainWindow>
#include <QLineEdit>
#include <QWheelEvent>
#include <QGraphicsScene>

void SetWindowFont(QMainWindow *w, int ptsize)
{
    QFont font = w->font();
    font.setPointSize(ptsize);
    w->setFont(font);
}

QIcon createColorCircleIcon(QSize size, Qt::GlobalColor color)
{
  QPixmap pm(size.width()-2, size.height()-2);
  pm.fill(Qt::transparent);
  QPainter b(&pm);
  b.setBrush(QBrush(color));
  b.drawEllipse(0, 0, size.width()-3, size.width()-3);
  return QIcon(pm);
}

TableDoubleDelegateClass::TableDoubleDelegateClass(QObject *parent)  : QItemDelegate(parent)
{
}

QWidget *TableDoubleDelegateClass::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
  QLineEdit *editor = new QLineEdit(parent);
  editor->setValidator(new QDoubleValidator(editor));
  editor->setEnabled(true);
  return editor;
}


myQGraphicsView::myQGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
  CursorMode = 0;
}

myQGraphicsView::myQGraphicsView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene,parent)
{
  CursorMode = 0;
}

void myQGraphicsView::wheelEvent(QWheelEvent *event)
{
  if(event->delta() > 0)
    {
      scale(1.1, 1.1);
    }
  else
    {
      scale(1.0/1.1, 1.0/1.1);
    }
}

void myQGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
  MousePosition = this->mapToScene(event->pos());
  //MousePosition = event->pos();
  //  qDebug() << MousePosition << this->mapToScene(event->pos());
  emit MouseMovedSignal(&MousePosition);
  QGraphicsView::mouseMoveEvent(event);
}

void myQGraphicsView::enterEvent(QEvent *event)
{
  QGraphicsView::enterEvent(event);
  if (CursorMode == 1)
    viewport()->setCursor(Qt::CrossCursor);
}

void myQGraphicsView::mousePressEvent(QMouseEvent *event)
{
  QGraphicsView::mousePressEvent(event);
  if (CursorMode == 1)
    viewport()->setCursor(Qt::CrossCursor);
}

void myQGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
  QGraphicsView::mouseReleaseEvent(event);
  if (CursorMode == 1)
    viewport()->setCursor(Qt::CrossCursor);
}
