#ifndef ALINEMARKERFILLDIALOG_H
#define ALINEMARKERFILLDIALOG_H

#include <QDialog>

namespace Ui {
class ALineMarkerFillDialog;
}

class TObject;
class TAttLine;
class TAttMarker;
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
    void on_pbClose_clicked();
    void on_pbUpdateObject_clicked();

    void on_pbMarkerColor_clicked();

private:
    Ui::ALineMarkerFillDialog *ui;
    TObject * tobj = nullptr;

    TAttLine   * lineAtt   = nullptr;
    TAttMarker * markerAtt = nullptr;

    int LineColor = 1;
    int MarkerColor = 1;

    TAttLine   * CopyLineAtt   = nullptr;
    TAttMarker * CopyMarkerAtt = nullptr;

private:
    void updateLineGui();
    void updateMarkerGui();
    void previewColor(int & color, QFrame *frame);
    void updateObject();

};

#endif // ALINEMARKERFILLDIALOG_H
