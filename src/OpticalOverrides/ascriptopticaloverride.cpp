#include "ascriptopticaloverride.h"
#include "amaterialparticlecolection.h"
#include "atracerstateful.h"
#include "aopticaloverridescriptinterface.h"
#include "amathscriptinterface.h"

#ifdef GUI
#include "ascriptwindow.h"
#include "ajavascriptmanager.h"
#include "globalsettingsclass.h"
#include <guiutils.h>
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
    Resources.overrideInterface->configure(Photon, NormalVector, MatFrom, MatTo);
    Resources.evaluateScript(Script);
    return Resources.overrideInterface->getResult(Status);
}

const QString AScriptOpticalOverride::getReportLine() const
{
    return QString();
}

void AScriptOpticalOverride::writeToJson(QJsonObject &json) const
{
    AOpticalOverride::writeToJson(json);

    json["Script"] = Script;
}

bool AScriptOpticalOverride::readFromJson(const QJsonObject &json)
{
    QString type = json["Model"].toString();
    if (type != getType()) return false; //file for wrong model!

    Script = json["Script"].toString();
    return true;
}

#ifdef GUI
#include <QPlainTextEdit>
QWidget *AScriptOpticalOverride::getEditWidget(QWidget *caller, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QVBoxLayout* l = new QVBoxLayout(f);
        QPlainTextEdit* pte = new QPlainTextEdit();
        pte->appendPlainText(Script);
        pte->moveCursor(QTextCursor::Start);
        pte->ensureCursorVisible();
        pte->setReadOnly(true);
        pte->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(pte, &QPlainTextEdit::customContextMenuRequested, [caller, pte, this]
        {openScriptWindow(caller); pte->clear(); pte->appendPlainText(Script);
            pte->moveCursor(QTextCursor::Start); pte->ensureCursorVisible();});
    l->addWidget(pte);
        QPushButton* pb = new QPushButton("Load / Edit script");
        QObject::connect(pb, &QPushButton::clicked, [caller, pte, this] {openScriptWindow(caller); pte->clear(); pte->appendPlainText(Script);
            pte->moveCursor(QTextCursor::Start); pte->ensureCursorVisible();});
    l->addWidget(pb);

    return f;
}
#endif

#include <QScriptEngine>
const QString AScriptOpticalOverride::checkOverrideData()
{
    if (Script.isEmpty()) return "Script not defined!";

    QScriptSyntaxCheckResult check = QScriptEngine::checkSyntax(Script);
    if (check.state() != QScriptSyntaxCheckResult::Valid)
    {
        int lineNumber = check.errorLineNumber();
        return QString("Syntax error at line %1").arg(lineNumber);
    }

    return "";
}

#ifdef GUI
void AScriptOpticalOverride::openScriptWindow(QWidget *caller)
{
    QString example = "photon.Absorbed()";

    TRandom2* RandGen = new TRandom2();
    AJavaScriptManager* sm = new AJavaScriptManager(RandGen);
    AScriptWindow* sw = new AScriptWindow(sm, new GlobalSettingsClass(0), true, caller);

    double v[3];
    v[0] = 0;
    v[1] = 0;
    v[2] = 1.0;
    double r[3];
    r[0] = 0;
    r[1] = 0;
    r[2] = 0;
    APhoton phot(r, v, -1, 0);
    double normal[3];
    normal[0] = 0;
    normal[1] = 0;
    normal[2] = 1.0;

    AOpticalOverrideScriptInterface* interfaceObject = new AOpticalOverrideScriptInterface(MatCollection, RandGen);
    interfaceObject->configure(&phot, normal, MatFrom, MatTo);
    interfaceObject->setObjectName("photon");
    sw->SetInterfaceObject(interfaceObject, "photon"); //takes ownership
    AMathScriptInterface* math = new AMathScriptInterface(RandGen);
    math->setObjectName("math");
    sw->SetInterfaceObject(math, "math"); //takes ownership

    sw->ConfigureForLightMode(&Script, "Optical override: custom script", example);
    sw->setWindowModality(Qt::ApplicationModal);
    sw->show();
    GuiUtils::AssureWidgetIsWithinVisibleArea(sw);

    while (sw->isVisible())
    {
        QCoreApplication::processEvents();
        QThread::usleep(200);
    }

    delete sw; //also deletes script manager
    delete RandGen;
}
#endif
