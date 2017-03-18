#ifndef AROOTMARKERCONFIGURATOR_H
#define AROOTMARKERCONFIGURATOR_H

#include <QDialog>

class QPainter;
class QSpinBox;
class QComboBox;

class ARootMarkerConfigurator : public QDialog
{
  Q_OBJECT
public:
  ARootMarkerConfigurator(int* Color, int* Size, int* Style, QWidget* parent = 0);

protected:
  void paintEvent(QPaintEvent *e);
  void mousePressEvent(QMouseEvent *e);

private:
  int SquareSize;
  QList<int> BaseColors;
  int *ReturnColor, *ReturnWidth, *ReturnStyle;

  QSpinBox* sbSize;
  QComboBox* comStyle;
  QSpinBox* sbColor;

  void PaintColorRow(QPainter *p, int row, int colorBase);

private slots:
  void onAccept();
};

#endif // AROOTMARKERCONFIGURATOR_H
