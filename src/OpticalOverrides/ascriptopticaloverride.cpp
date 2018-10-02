#include "ascriptopticaloverride.h"
#include "amaterialparticlecolection.h"
#include "atracerstateful.h"
#include "aopticaloverridescriptinterface.h"
#include "amathscriptinterface.h"

#ifdef GUI
#include "ascriptwindow.h"
#include "ajavascriptmanager.h"
#include "globalsettingsclass.h"
#include "TRandom2.h"
#include "aphoton.h"
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCoreApplication>
#include <QThread>
#endif

#include <QDebug>
#include <QJsonObject>

AScriptOpticalOverride::AScriptOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}

AScriptOpticalOverride::~AScriptOpticalOverride() {}

AOpticalOverride::OpticalOverrideResultEnum AScriptOpticalOverride::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
        //qDebug() << "Configuring script interfaces";
    Resources.overrideInterface->configure(Resources.RandGen, Photon, NormalVector);
    Resources.mathInterface->setRandomGen(Resources.RandGen);
        //qDebug() << "Evaluating script";
    Resources.evaluateScript(Script);
        //qDebug() << "Photon result:"<<Resources.overrideInterface->getResult(Status)<<"Status"<<Status;;
    return Resources.overrideInterface->getResult(Status);
}

void AScriptOpticalOverride::printConfiguration(int)
{
    qDebug() << "-------Configuration:-------";
    qDebug() << "Script:";
    qDebug() << Script;
    qDebug() << "----------------------------";
}

QString AScriptOpticalOverride::getReportLine()
{
    return QString("to %1 -> Custom script").arg( (*MatCollection)[MatTo]->name );
}

void AScriptOpticalOverride::writeToJson(QJsonObject &json)
{
    AOpticalOverride::writeToJson(json);

    json["Script"] = Script;
}

bool AScriptOpticalOverride::readFromJson(QJsonObject &json)
{
    QString type = json["Model"].toString();
    if (type != getType()) return false; //file for wrong model!

    Script = json["Script"].toString();
    return true;
}

#ifdef GUI
QWidget *AScriptOpticalOverride::getEditWidget(QWidget *caller, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QVBoxLayout* l = new QVBoxLayout(f);
        QLabel* lab = new QLabel("");
    l->addWidget(lab);
        QPushButton* pb = new QPushButton("Load / Edit script");
        QObject::connect(pb, &QPushButton::clicked, [caller, this] {openScriptWindow(caller);});
    l->addWidget(pb);
        lab = new QLabel("");
    l->addWidget(lab);

    return f;
}
#endif

const QString AScriptOpticalOverride::checkOverrideData()
{
    if (Script.isEmpty()) return "Script not defined!";

    //TODO check syntax!

    return "";
}

#ifdef GUI
void AScriptOpticalOverride::openScriptWindow(QWidget *parent)
{
    QString example = "photon.Absorbed()";

    TRandom2* RandGen = new TRandom2(); //leak!
    AJavaScriptManager* sm = new AJavaScriptManager(RandGen); //leak!
    AScriptWindow* sw = new AScriptWindow(sm, new GlobalSettingsClass(0), true, parent); //leak!
    sw->ConfigureForLightMode(&Script, "Optical override: custom script", example);

    double vx[3];
    vx[0] = 0; vx[1] = 0; vx[3] = 1;
    double r[3];
    r[0] = 0; r[1] = 0; r[2] = 0;
    APhoton phot(vx, r, -1, 0);
    double normal[3];
    normal[0] = 0; normal[1] = 0; normal[2] = 1.0;

    AOpticalOverrideScriptInterface* interfaceObject = new AOpticalOverrideScriptInterface();
    interfaceObject->configure(RandGen, &phot, normal);
    interfaceObject->setObjectName("photon");
    sw->SetInterfaceObject(interfaceObject, "photon"); //steals ownership!
    AMathScriptInterface* math = new AMathScriptInterface(RandGen);
    math->setObjectName("math");
    sw->SetInterfaceObject(math, "math"); //steals ownership!
    sw->setWindowModality(Qt::ApplicationModal);
    sw->show();

    while (sw->isVisible())
    {
        QCoreApplication::processEvents();
        QThread::usleep(200);
    }
    //qDebug() << "Script reports:"<<Script;
}
#endif
