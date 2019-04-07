#include "aguiwindow.h"
#include "windownavigatorclass.h"
#include "ajsontools.h"

#include <QEvent>
#include <QResizeEvent>
#include <QDebug>

AGuiWindow::AGuiWindow(QWidget * parent) :
    QMainWindow(parent){}

void AGuiWindow::connectToNavigator(WindowNavigatorClass * wNav, const QString & idStr)
{
    WNav = wNav;
    IdStr = idStr;
}

void AGuiWindow::writeToJson(const QString & winName, QJsonObject & json) const
{
    QJsonObject js;
    js["X"] = WinPos_X;
    js["Y"] = WinPos_Y;
    js["W"] = WinSize_W;
    js["H"] = WinSize_H;
    js["Vis"] = bWinVisible;

    json[winName] = js;
}

void AGuiWindow::readFromJson(const QString & winName, const QJsonObject & json)
{
    if (!json.contains(winName)) return;

    QJsonObject js;
    parseJson(json, winName, js);

    parseJson(js, "X", WinPos_X);
    parseJson(js, "Y", WinPos_Y);
    parseJson(js, "W", WinSize_W);
    parseJson(js, "H", WinSize_H);
    parseJson(js, "Vis", bWinVisible);

    resize(WinSize_W, WinSize_H);
    move(WinPos_X, WinPos_Y);

    if (bWinVisible) show();
    else hide();
}

//#include <QApplication>
bool AGuiWindow::event(QEvent *event)
{
    if (WNav)
    {
        if (event->type() == QEvent::Hide)
        {
            bWinVisible = false;
            bWinGeomUpdateAllowed = false;
            WNav->HideWindowTriggered(IdStr);
            //return true;
        }
        else if (event->type() == QEvent::Show)
        {
            //qApp->processEvents();
            WNav->ShowWindowTriggered(IdStr);
            bWinGeomUpdateAllowed = true;
            bWinVisible = true;
            //return true;
        }
    }
    return QMainWindow::event(event);
}

void AGuiWindow::resizeEvent(QResizeEvent * event)
{
    //if (event && bWinGeomUpdateAllowed)
    if (bWinGeomUpdateAllowed)
    {
        //WinSize_W = event->size().width();
        WinSize_W = width();
        //WinSize_H = event->size().height();
        WinSize_H = height();
        //qDebug() << "Resize to " << WinSize_W << WinSize_H;
    }
    QMainWindow::resizeEvent(event);
}

void AGuiWindow::moveEvent(QMoveEvent * event)
{
    if (bWinGeomUpdateAllowed)
    {
        //WinPos_X = event->pos().x();
        WinPos_X = x();
        //WinPos_Y = event->pos().y();
        WinPos_Y = y();
        //qDebug() << "Move to" << WinPos_X << WinPos_Y;
    }
    QMainWindow::moveEvent(event);
}
