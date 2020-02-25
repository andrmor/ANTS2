#ifndef AROOTCOLORSELECTORDIALOG_H
#define AROOTCOLORSELECTORDIALOG_H

#include <QDialog>
#include <QVector>

namespace Ui {
class ARootColorSelectorDialog;
}

class QPainter;

class ARootColorSelectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ARootColorSelectorDialog(int & color, QWidget *parent = 0);
    ~ARootColorSelectorDialog();

protected:
  void paintEvent(QPaintEvent *e);
  void mousePressEvent(QMouseEvent *e);

private slots:
    void on_pbClose_clicked();
    void on_sbColor_editingFinished();

private:
    Ui::ARootColorSelectorDialog *ui;
    int & Color;
    int ColorOnStart = 1;

    int SquareSize = 30;
    QVector<int> BaseColors;

private:
    bool validateColor();
    void showColor();
    void PaintColorRow(QPainter *p, int row, int colorBase);
};

#endif // AROOTCOLORSELECTORDIALOG_H
