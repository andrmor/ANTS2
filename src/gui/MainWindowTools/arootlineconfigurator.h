#ifndef AROOTLINECONFIGURATOR_H
#define AROOTLINECONFIGURATOR_H

#include <QDialog>

class QPainter;
class QSpinBox;
class QComboBox;
class QFrame;

class ARootLineConfigurator : public QDialog
{
  Q_OBJECT
public:
  ARootLineConfigurator(int* color, int* width, int* style, QWidget* parent = 0);

protected:
  void paintEvent(QPaintEvent *e);
  void mousePressEvent(QMouseEvent *e);

private:
  int SquareSize; 
  QList<int> BaseColors;
  int *ReturnColor, *ReturnWidth, *ReturnStyle;

  QSpinBox* sbWidth;
  QComboBox* comStyle;
  QSpinBox* sbColor;
  QFrame* frCol;

  void PaintColorRow(QPainter *p, int row, int colorBase);

private slots:
  void updateColorFrame();
  void previewColor();
  void onAccept();
};

#endif // AROOTLINECONFIGURATOR_H
