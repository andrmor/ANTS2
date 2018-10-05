#ifndef AOPTICALOVERRIDEDIALOG_H
#define AOPTICALOVERRIDEDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QSet>

namespace Ui {
class AOpticalOverrideDialog;
}

class AOpticalOverride;
class AMaterialParticleCollection;
class GraphWindowClass;
class GeometryWindowClass;
class AOpticalOverrideTester;

class AOpticalOverrideDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AOpticalOverrideDialog(AMaterialParticleCollection* MatCollection, int matFrom, int matTo,
                                    GraphWindowClass* GraphWindow, GeometryWindowClass* GeometryWindow, QWidget* parent);
    ~AOpticalOverrideDialog();

private slots:
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();
    void on_cobType_activated(int index);
    void on_pbTestOverride_clicked();

private:
    Ui::AOpticalOverrideDialog * ui;
    GraphWindowClass * GraphWindow;
    GeometryWindowClass* GeometryWindow;
    AOpticalOverride ** pOV;
    AOpticalOverride * ovLocal = 0;
    AMaterialParticleCollection* MatCollection;
    int matFrom;
    int matTo;
    QStringList matNames; //need?
    AOpticalOverrideTester* TesterWindow;

    int customWidgetPositionInLayout = 5;
    QWidget* customWidget = 0;

    QSet<AOpticalOverride*> openedOVs;

    void updateGui();
    AOpticalOverride* findInOpended(const QString& ovType);
    void clearOpenedExcept(AOpticalOverride* keepOV);
};

#endif // AOPTICALOVERRIDEDIALOG_H
