#include "ainterfacetoguiscript.h"
#include "ajavascriptmanager.h"
#include "amessage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>

AInterfaceToGuiScript::AInterfaceToGuiScript(AJavaScriptManager* ScriptManager) :
    AScriptInterface(), ScriptManager(ScriptManager)
{
    init();
}

void AInterfaceToGuiScript::init()
{
    delete Wid;
    Wid = new QWidget();
    Wid->resize(400, 200);
    Wid->setWindowFlags(Qt::WindowStaysOnTopHint);

    Widgets.clear(); //items owned by Wid!
    Layouts.clear(); //items owned by Wid!

    QVBoxLayout* lay = new QVBoxLayout(Wid);
    Layouts.insert("", lay);
}

AInterfaceToGuiScript::~AInterfaceToGuiScript()
{
    delete Wid;
}

bool AInterfaceToGuiScript::InitOnRun()
{
    init();
    return true;
}

void AInterfaceToGuiScript::buttonNew(const QString name, const QString addTo, const QString text)
{
    if (Widgets.contains(name))
    {
        abort("Widget " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QPushButton* b = new QPushButton(text);
    Widgets.insert(name, b);
    lay->addWidget(b);
}

void AInterfaceToGuiScript::buttonOnClick(const QString name, const QVariant scriptFunction)
{
    QWidget* w = Widgets.value(name, 0);
    QPushButton* b = dynamic_cast<QPushButton*>(w);
    if (!b)
    {
        abort("Button " + name + " does not exist");
        return;
    }

    QString functionName;
    QString typeArr = scriptFunction.typeName();
    if (typeArr == "QString") functionName = scriptFunction.toString();
    else if (typeArr == "QVariantMap")
    {
        QVariantMap vm = scriptFunction.toMap();
        functionName = vm["name"].toString();
    }
    if (functionName.isEmpty())
    {
        abort("ButtonOnClick() function requires function or its name as the second argument!");
        return;
    }

    QScriptValue func = ScriptManager->getProperty(functionName);
    if (!func.isValid() || !func.isFunction())
    {
        abort("ButtonOnClick() function requires function or its name as the second argument!");
        return;
    }

    connect(b, &QPushButton::clicked, [=](){ScriptManager->getProperty(functionName).call();});
    //QScriptValue res = func.call(); //QScriptValue(), args);
    //***!!! add error report!
}

void AInterfaceToGuiScript::labelNew(const QString name, const QString addTo, const QString text)
{
    if (Widgets.contains(name))
    {
        abort("Widget " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QLabel* l = new QLabel(text);
    Widgets.insert(name, l);
    lay->addWidget(l);
}

void AInterfaceToGuiScript::labelSetText(const QString name, const QString labelText)
{
    QWidget* w = Widgets.value(name, 0);
    QLabel* l = dynamic_cast<QLabel*>(w);
    if (!l)
    {
        abort("Label " + name + " does not exist");
        return;
    }
    l->setText(labelText);
}

void AInterfaceToGuiScript::editNew(const QString name, const QString addTo, const QString text)
{
    if (Widgets.contains(name))
    {
        abort("Widget " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QLineEdit* e = new QLineEdit(text);
    Widgets.insert(name, e);
    lay->addWidget(e);
}

void AInterfaceToGuiScript::editSetText(const QString name, const QString text)
{
    QWidget* w = Widgets.value(name, 0);
    QLineEdit* e = dynamic_cast<QLineEdit*>(w);
    if (!e) abort("Edit box " + name + " does not exist");
    else e->setText(text);
}

const QString AInterfaceToGuiScript::editGetText(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QLineEdit* e = dynamic_cast<QLineEdit*>(w);
    if (!e)
    {
        abort("Edit box " + name + " does not exist");
        return "";
    }
    return e->text();
}

void AInterfaceToGuiScript::comboboxNew(const QString name, const QString addTo, bool Editable)
{
    if (Widgets.contains(name))
    {
        abort("Widget " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QComboBox* e = new QComboBox();
    e->setEditable(Editable);
    Widgets.insert(name, e);
    lay->addWidget(e);
}

void AInterfaceToGuiScript::comboboxAppend(const QString name, const QVariant entries)
{
    QWidget* w = Widgets.value(name, 0);
    QComboBox* e = dynamic_cast<QComboBox*>(w);
    if (!e) abort("Combobox " + name + " does not exist");
    else
    {
        QVariantList vl = entries.toList();
        QStringList list;
        for (QVariant& v : vl) list << v.toString();
        e->addItems(list);
    }
}

void AInterfaceToGuiScript::comboboxClear(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QComboBox* e = dynamic_cast<QComboBox*>(w);
    if (!e) abort("Combobox " + name + " does not exist");
    else e->clear();
}

const QString AInterfaceToGuiScript::comboboxGetSelected(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QComboBox* e = dynamic_cast<QComboBox*>(w);
    if (!e)
    {
        abort("Combobox " + name + " does not exist");
        return "";
    }
    return e->currentText();
}

void AInterfaceToGuiScript::textNew(const QString name, const QString addTo, const QString text)
{
    if (Widgets.contains(name))
    {
        abort("Widget " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QPlainTextEdit* t = new QPlainTextEdit(text);
    Widgets.insert(name, t);
    lay->addWidget(t);
}

void AInterfaceToGuiScript::textClear(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QPlainTextEdit* t = dynamic_cast<QPlainTextEdit*>(w);
    if (!t)
    {
        abort("Text box " + name + " does not exist");
        return;
    }
    t->clear();
}

void AInterfaceToGuiScript::textAppendPlainText(const QString name, const QString text)
{
    QWidget* w = Widgets.value(name, 0);
    QPlainTextEdit* t = dynamic_cast<QPlainTextEdit*>(w);
    if (!t)
    {
        abort("Text box " + name + " does not exist");
        return;
    }
    t->appendPlainText(text);
}

void AInterfaceToGuiScript::textAppendHtml(const QString name, const QString text)
{
    QWidget* w = Widgets.value(name, 0);
    QPlainTextEdit* t = dynamic_cast<QPlainTextEdit*>(w);
    if (!t)
    {
        abort("Text box " + name + " does not exist");
        return;
    }
    t->appendHtml(text);
}

const QString AInterfaceToGuiScript::textGet(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QPlainTextEdit* t = dynamic_cast<QPlainTextEdit*>(w);
    if (!t)
    {
        abort("Text box " + name + " does not exist");
        return "";
    }
    return t->document()->toPlainText();
}

void AInterfaceToGuiScript::addStretch(const QString addTo)
{
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QVBoxLayout* v = dynamic_cast<QVBoxLayout*>(lay);
    if (v) v->addStretch();
    else
    {
        QHBoxLayout* h = dynamic_cast<QHBoxLayout*>(lay);
        if (h) h->addStretch();
    }
}

void AInterfaceToGuiScript::verticalLayout(const QString name, const QString addTo)
{
    if (Layouts.contains(name))
    {
        abort("Layout " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QVBoxLayout* l = new QVBoxLayout();
    Layouts.insert(name, l);

    QVBoxLayout* v = dynamic_cast<QVBoxLayout*>(lay);
    if (v) v->addLayout(l);
    else
    {
        QHBoxLayout* h = dynamic_cast<QHBoxLayout*>(lay);
        if (h) h->addLayout(l);
    }
}

void AInterfaceToGuiScript::horizontalLayout(const QString &name, const QString &addTo)
{
    if (Layouts.contains(name))
    {
        abort("Layout " + name + " already exists");
        return;
    }
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QHBoxLayout* l = new QHBoxLayout();
    Layouts.insert(name, l);

    QVBoxLayout* v = dynamic_cast<QVBoxLayout*>(lay);
    if (v) v->addLayout(l);
    else
    {
        QHBoxLayout* h = dynamic_cast<QHBoxLayout*>(lay);
        if (h) h->addLayout(l);
    }
}

void AInterfaceToGuiScript::messageBox(const QString text)
{
    message(text, Wid);
}

void AInterfaceToGuiScript::show()
{
    Wid->show();
}

void AInterfaceToGuiScript::hide()
{
    Wid->hide();
}

void AInterfaceToGuiScript::setWidgetTitle(const QString title)
{
    Wid->setWindowTitle(title);
}

void AInterfaceToGuiScript::setEnabled(const QString name, bool flag)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setEnabled(flag);
}

void AInterfaceToGuiScript::setVisible(const QString name, bool flag)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setVisible(flag);
}
