#ifndef AROOTTEXTCONFIGURATOR_H
#define AROOTTEXTCONFIGURATOR_H

#include <QDialog>
#include <QVector>

class QFrame;

namespace Ui {
class ARootTextConfigurator;
}

class ARootTextConfigurator : public QDialog
{
    Q_OBJECT

public:
    explicit ARootTextConfigurator(int & color, int & align, int & font, float & size, QWidget *parent = 0);
    ~ARootTextConfigurator();

protected:
  void paintEvent(QPaintEvent *e);
  void mousePressEvent(QMouseEvent *e);

private:
    Ui::ARootTextConfigurator *ui;
    int SquareSize = 30;
    QVector<int> BaseColors;

    int   & color;
    int   & align;
    int   & font;
    float & size;

private slots:
    void PaintColorRow(QPainter *p, int row, int colorBase);

  private slots:
    void updateColorFrame();
    void previewColor();
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();
};

#endif // AROOTTEXTCONFIGURATOR_H
