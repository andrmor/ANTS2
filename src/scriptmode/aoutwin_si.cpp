#include "aoutwin_si.h"
#include "mainwindow.h"
#include "outputwindow.h"

#include <QApplication>

AOutWin_SI::AOutWin_SI(MainWindow *MW) :
    MW(MW)
{
    Description = "Access to the Output window of GUI";
}

void AOutWin_SI::ShowOutputWindow(bool flag, int tab)
{
    if (flag)
      {
        MW->Owindow->showNormal();
        MW->Owindow->raise();
      }
    else MW->Owindow->hide();

    if (tab>-1 && tab<5) MW->Owindow->SetTab(tab);
    qApp->processEvents();
}

void AOutWin_SI::Show()
{
    MW->Owindow->show();
}

void AOutWin_SI::Hide()
{
    MW->Owindow->hide();
}

void AOutWin_SI::SetWindowGeometry(QVariant xywh)
{
    if (xywh.type() != QVariant::List)
    {
        abort("Array [X Y Width Height] is expected");
        return;
    }
    QVariantList vl = xywh.toList();
    if (vl.size() != 4)
    {
        abort("Array [X Y Width Height] is expected");
        return;
    }

    int x = vl[0].toInt();
    int y = vl[1].toInt();
    int w = vl[2].toInt();
    int h = vl[3].toInt();
    MW->Owindow->move(x, y);
    MW->Owindow->resize(w, h);
}
