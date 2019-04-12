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
    js["x"] = WinPos_X;
    js["y"] = WinPos_Y;
    js["w"] = WinSize_W;
    js["h"] = WinSize_H;
    js["vis"] = bWinVisible;

    json[winName] = js;
}

void AGuiWindow::readFromJson(const QString & winName, const QJsonObject & json)
{
    if (!json.contains(winName)) return;

    QJsonObject js;
    parseJson(json, winName, js);

    parseJson(js, "x", WinPos_X);
    parseJson(js, "y", WinPos_Y);
    parseJson(js, "w", WinSize_W);
    parseJson(js, "h", WinSize_H);
    parseJson(js, "vis", bWinVisible);

    resize(WinSize_W, WinSize_H);
    move(WinPos_X, WinPos_Y);

    if (bWinVisible) show();
    else hide();
}

//#include <QApplication>
bool AGuiWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if( windowState() == Qt::WindowMinimized )
        {
            //qDebug() << IdStr<<"Minimized!";
            event->accept();
            return true;
        }
        else if( windowState() == Qt::WindowNoState )
        {
            //qDebug() << IdStr<<"Restored!";
        }
        //qDebug() << windowState();
    }

    if (WNav)
    {
        if (event->type() == QEvent::Hide)
        {
            //qDebug() << IdStr<<"----Hide event"<<isVisible();
            bWinVisible = false;
            bWinGeomUpdateAllowed = false;
            WNav->HideWindowTriggered(IdStr);
            return true;
        }
        else if (event->type() == QEvent::Show)
        {
            //qDebug() << IdStr<<"----Show event";
            WNav->ShowWindowTriggered(IdStr);
            bWinGeomUpdateAllowed = true;
            bWinVisible = true;
            return true;
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
