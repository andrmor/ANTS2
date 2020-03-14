#ifndef AGUIWINDOW_H
#define AGUIWINDOW_H

#include <QMainWindow>
#include <QObject>

class WindowNavigatorClass;

class AGuiWindow : public QMainWindow
{
    Q_OBJECT
public:
    AGuiWindow(const QString & idStr, QWidget * parent);

    void connectWinNavigator(WindowNavigatorClass * wNav);

    void writeGeomToJson(QJsonObject & json) const;
    void readGeomFromJson(const QJsonObject & json);

protected:
    bool event(QEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    void moveEvent(QMoveEvent * event) override;

    bool bWinGeomUpdateAllowed = true;

    int  WinPos_X = 40;
    int  WinPos_Y = 40;
    int  WinSize_W = 800;
    int  WinSize_H = 600;
    bool bWinVisible = false;

private:
    WindowNavigatorClass * WNav = nullptr;
    QString IdStr;
};

#endif // AGUIWINDOW_H
