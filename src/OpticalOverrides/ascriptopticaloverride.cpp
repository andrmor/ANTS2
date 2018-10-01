#include "ascriptopticaloverride.h"
#include "amaterialparticlecolection.h"

#include "aopticaloverridescriptinterface.h"

#include <QDebug>
#include <QJsonObject>
#include <QScriptEngine> //?


#include "coreinterfaces.h"
AScriptOpticalOverride::AScriptOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo)
{
    //to transfer away
    ScriptEngine = new QScriptEngine(); //mem leak
    qDebug() << "Registering script interface";
    interfaceObject = new AOpticalOverrideScriptInterface(); //mem leak
    QScriptValue obj = ScriptEngine->newQObject(interfaceObject, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty("photon", obj);
        //coreObj = new AInterfaceToCore(this);
        //QScriptValue coreVal = engine->newQObject(coreObj, QScriptEngine::QtOwnership);
        //QString coreName = "core";
        //coreObj->setObjectName(coreName);
        //engine->globalObject().setProperty(coreName, coreVal);
        //interfaces.append(coreObj);
        //registering math module
        AInterfaceToMath* mathObj = new AInterfaceToMath(0);
        QScriptValue mathVal = ScriptEngine->newQObject(mathObj, QScriptEngine::QtOwnership);
//        mathObj->setObjectName(mathName);
        ScriptEngine->globalObject().setProperty("math", mathVal);
        //interfaces.append(mathObj);  //SERVICE OBJECT IS FIRST in interfaces!
}

void AScriptOpticalOverride::initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes)
{
    //external script engine will have interface already registered
}

AOpticalOverride::OpticalOverrideResultEnum AScriptOpticalOverride::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
{
    //qDebug() << "Configuring script interface";
    interfaceObject->configure(RandGen, Photon, NormalVector);
    //qDebug() << "Evaluating script";
    //QScriptValue res =
    ScriptEngine->evaluate(Script);
    //qDebug() << "eval result:" << res.toString()<<"Photon status:"<<interfaceObject->getResult();
    return interfaceObject->getResult();
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
#include <QFrame>
//#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
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
#include "ascriptwindow.h"
#include "ajavascriptmanager.h"
#include "globalsettingsclass.h"
#include "TRandom2.h"
void AScriptOpticalOverride::openScriptWindow(QWidget *parent)
{
    QString example = "photon.Absorbed()";

    TRandom2* RandGen = new TRandom2(); //leak!
    AJavaScriptManager* sm = new AJavaScriptManager(RandGen); //leak!
    AScriptWindow* sw = new AScriptWindow(sm, new GlobalSettingsClass(0), true, parent); //leak!
    sw->ConfigureForLightMode(&Script, "Optical override: custom script", example);

    //sw->SetInterfaceObject(0);
    sw->SetInterfaceObject(interfaceObject, "photon"); //steals ownership!
    sw->show();
}
#endif
