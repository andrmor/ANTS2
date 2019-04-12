#ifndef AGUIWINDOW_H
#define AGUIWINDOW_H

#include <QMainWindow>
#include <QObject>

class WindowNavigatorClass;

class AGuiWindow : public QMainWindow
{
    Q_OBJECT
public:
    AGuiWindow(QWidget * parent);

    void connectToNavigator(WindowNavigatorClass * wNav, const QString & idStr);

    void writeToJson(const QString & winName, QJsonObject & json) const;
    void readFromJson(const QString & winName, const QJsonObject & json);

protected:
    bool event(QEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    void moveEvent(QMoveEvent * event) override;

    bool bWinGeomUpdateAllowed = true;

    int WinPos_X = 40;
    int WinPos_Y = 40;
    int WinSize_W = 800;
    int WinSize_H = 600;
    bool bWinVisible = true;
    bool bWinMaximized = false;

private:
    WindowNavigatorClass * WNav = nullptr;
    QString IdStr;
};

#endif // AGUIWINDOW_H
