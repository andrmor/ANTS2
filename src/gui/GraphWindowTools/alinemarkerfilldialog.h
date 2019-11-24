#ifndef ALINEMARKERFILLDIALOG_H
#define ALINEMARKERFILLDIALOG_H

#include <QDialog>

namespace Ui {
class ALineMarkerFillDialog;
}

class TObject;
class TAttLine;
class TAttMarker;
class TAttFill;
class QFrame;

class ALineMarkerFillDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ALineMarkerFillDialog(TObject * tobj, QWidget *parent = 0);
    ~ALineMarkerFillDialog();

private slots:
    void onReject();

    void on_pbLineColor_clicked();
    void on_pbMarkerColor_clicked();
    void on_pbFillColor_clicked();

    void on_pbClose_clicked();
    void on_pbUpdateObject_clicked();

    void on_cobFillStyle_currentIndexChanged(int index);
    void on_cbMarkerColorAsLine_clicked(bool checked);
    void on_cbFillColorAsLine_clicked(bool checked);
    void on_pbPreview_clicked();

private:
    Ui::ALineMarkerFillDialog *ui;
    TObject * tobj = nullptr;

    TAttLine   * lineAtt   = nullptr;
    TAttMarker * markerAtt = nullptr;
    TAttFill   * fillAtt   = nullptr;

    int LineColor   = 1;
    int MarkerColor = 1;
    int FillColor   = 1;

    TAttLine   * CopyLineAtt   = nullptr;
    TAttMarker * CopyMarkerAtt = nullptr;
    TAttFill   * CopyFillAtt   = nullptr;

private:
    void updateObject();
    void updateGui();

    void updateLineGui();
    void updateMarkerGui();
    void updateFillGui();
    void previewColor(int & color, QFrame *frame);

signals:
    void requestRedraw();

};

#endif // ALINEMARKERFILLDIALOG_H
