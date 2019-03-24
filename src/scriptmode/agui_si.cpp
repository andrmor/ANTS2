#include "agui_si.h"
#include "ajavascriptmanager.h"
#include "amessage.h"
#include "alineedit.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QDebug>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QCompleter>

AGui_SI::AGui_SI(AJavaScriptManager* ScriptManager) :
    AScriptInterface(), ScriptManager(ScriptManager)
{
    init();
}

void AGui_SI::init()
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

AGui_SI::~AGui_SI()
{
    delete Wid; Wid = 0;
}

bool AGui_SI::InitOnRun()
{
    init();
    return true;
}

void AGui_SI::ForceStop()
{
    delete Wid; Wid = 0;
}

void AGui_SI::buttonNew(const QString name, const QString addTo, const QString text)
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
    b->setContextMenuPolicy(Qt::CustomContextMenu);
    Widgets.insert(name, b);
    lay->addWidget(b);
}

void AGui_SI::buttonSetText(const QString name, const QString text, bool bold)
{
    QWidget* w = Widgets.value(name, 0);
    QPushButton* b = dynamic_cast<QPushButton*>(w);
    if (!b) abort("Button " + name + " does not exist");
    else
    {
        b->setText(text);
        QFont fo = b->font();
        fo.setBold(bold);
        b->setFont(fo);
    }
}

void AGui_SI::buttonOnClick(const QString name, const QVariant scriptFunction)
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
        abort("buttonOnClick() function requires function or its name as the second argument!");
        return;
    }

    QScriptValue func = ScriptManager->getProperty(functionName);
    if (!func.isValid() || !func.isFunction())
    {
        abort("buttonOnClick() function requires function or its name as the second argument!");
        return;
    }

    connect(b, &QPushButton::clicked,
            [=]() { ScriptManager->getProperty(functionName).call(); ScriptManager->ifError_AbortAndReport(); } );
}

void AGui_SI::buttonOnRightClick(const QString name, const QVariant scriptFunction)
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
        abort("buttonOnRightClick() function requires function or its name as the second argument!");
        return;
    }

    QScriptValue func = ScriptManager->getProperty(functionName);
    if (!func.isValid() || !func.isFunction())
    {
        abort("buttonOnRightClick() function requires function or its name as the second argument!");
        return;
    }

    connect(b, &QPushButton::customContextMenuRequested,
            [=]() { ScriptManager->getProperty(functionName).call(); ScriptManager->ifError_AbortAndReport(); } );
}

void AGui_SI::labelNew(const QString name, const QString addTo, const QString text)
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

void AGui_SI::labelSetText(const QString name, const QString labelText)
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

void AGui_SI::editNew(const QString name, const QString addTo, const QString text)
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
    ALineEdit* e = new ALineEdit(text);
    Widgets.insert(name, e);
    lay->addWidget(e);
}

void AGui_SI::editSetText(const QString name, const QString text)
{
    QWidget* w = Widgets.value(name, 0);
    ALineEdit* e = dynamic_cast<ALineEdit*>(w);
    if (!e) abort("Edit box " + name + " does not exist");
    else e->setText(text);
}

const QString AGui_SI::editGetText(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    ALineEdit* e = dynamic_cast<ALineEdit*>(w);
    if (!e)
    {
        abort("Edit box " + name + " does not exist");
        return "";
    }
    return e->text();
}

void AGui_SI::editSetIntValidator(const QString name, int min, int max)
{
    QWidget* w = Widgets.value(name, 0);
    ALineEdit* e = dynamic_cast<ALineEdit*>(w);
    if (!e) abort("Edit box " + name + " does not exist");
    else
    {
        const QValidator* old = e->validator();
        if (old) delete old;

        QValidator* validator = new QIntValidator(min, max, e);
        e->setValidator(validator);
    }
}

void AGui_SI::editSetDoubleValidator(const QString name, double min, double max, int decimals)
{
    QWidget* w = Widgets.value(name, 0);
    ALineEdit* e = dynamic_cast<ALineEdit*>(w);
    if (!e) abort("Edit box " + name + " does not exist");
    else
    {
        const QValidator* old = e->validator();
        if (old) delete old;

        QValidator* validator = new QDoubleValidator(min, max, decimals, e);
        e->setValidator(validator);
    }
}

void AGui_SI::editSetCompleter(const QString name, const QVariant arrayOfStrings)
{
    QWidget* w = Widgets.value(name, 0);
    ALineEdit* e = dynamic_cast<ALineEdit*>(w);
    if (!e) abort("Edit box " + name + " does not exist");
    else
    {
        const QCompleter* old = e->completer();
        if (old) delete old;

        QVariantList vl = arrayOfStrings.toList();
        QStringList sl;
        for (int i=0; i<vl.size(); i++) sl << vl.at(i).toString();

        //qDebug() << "possible:" << sl;

        QCompleter* completer = new QCompleter(sl, e);
        completer->setCaseSensitivity(Qt::CaseInsensitive); //Qt::CaseSensitive
        completer->setCompletionMode(QCompleter::PopupCompletion);
        //completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        completer->setFilterMode(Qt::MatchContains);
        completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
        completer->setWrapAround(false);
        e->setCompleter(completer);
    }
}

void AGui_SI::editOnTextChanged(const QString name, const QVariant scriptFunction)
{
    QWidget* w = Widgets.value(name, 0);
    ALineEdit* e = dynamic_cast<ALineEdit*>(w);
    if (!e)
    {
        abort("Edit box " + name + " does not exist");
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
        abort("editOnTextChanged() function requires function or its name as the second argument!");
        return;
    }

    QScriptValue func = ScriptManager->getProperty(functionName);
    if (!func.isValid() || !func.isFunction())
    {
        abort("editOnTextChanged() function requires function or its name as the second argument!");
        return;
    }

    connect(e, &ALineEdit::textChanged,
            [=]() { ScriptManager->getProperty(functionName).call(); ScriptManager->ifError_AbortAndReport(); } );
}

void AGui_SI::comboboxNew(const QString name, const QString addTo, bool Editable)
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

void AGui_SI::comboboxAppend(const QString name, const QVariant entries)
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

void AGui_SI::comboboxClear(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QComboBox* e = dynamic_cast<QComboBox*>(w);
    if (!e) abort("Combobox " + name + " does not exist");
    else e->clear();
}

const QString AGui_SI::comboboxGetSelected(const QString name)
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

void AGui_SI::comboboxOnTextChanged(const QString name, const QVariant scriptFunction)
{
    QWidget* w = Widgets.value(name, 0);
    QComboBox* b = dynamic_cast<QComboBox*>(w);
    if (!b)
    {
        abort("Combobox " + name + " does not exist");
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
        abort("comboboxOnTextChanged() function requires function or its name as the second argument!");
        return;
    }

    QScriptValue func = ScriptManager->getProperty(functionName);
    if (!func.isValid() || !func.isFunction())
    {
        abort("comboboxOnTextChanged() function requires function or its name as the second argument!");
        return;
    }

    connect(b, &QComboBox::currentTextChanged,
            [=]() { ScriptManager->getProperty(functionName).call(); ScriptManager->ifError_AbortAndReport(); } );
}

void AGui_SI::textNew(const QString name, const QString addTo, const QString text)
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

void AGui_SI::textClear(const QString name)
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

void AGui_SI::textAppendPlainText(const QString name, const QString text)
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

void AGui_SI::textAppendHtml(const QString name, const QString text)
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

const QString AGui_SI::textGet(const QString name)
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

void AGui_SI::checkboxNew(const QString name, const QString addTo, const QString text, bool checked)
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
    QCheckBox* cb = new QCheckBox(text);
    cb->setChecked(checked);
    Widgets.insert(name, cb);
    lay->addWidget(cb);
}

void AGui_SI::checkboxSetText(const QString name, const QString text)
{
    QWidget* w = Widgets.value(name, 0);
    QCheckBox* cb = dynamic_cast<QCheckBox*>(w);
    if (!cb) abort("Checkbox " + name + " does not exist");
    else cb->setText(text);
}

void AGui_SI::checkboxSetChecked(const QString name, bool checked)
{
    QWidget* w = Widgets.value(name, 0);
    QCheckBox* cb = dynamic_cast<QCheckBox*>(w);
    if (!cb) abort("Checkbox " + name + " does not exist");
    else cb->setChecked(checked);
}

bool AGui_SI::checkboxIsChecked(const QString name)
{
    QWidget* w = Widgets.value(name, 0);
    QCheckBox* cb = dynamic_cast<QCheckBox*>(w);
    if (!cb)
    {
        abort("Checkbox " + name + " does not exist");
        return false;
    }
    return cb->isChecked();
}

void AGui_SI::checkboxOnClick(const QString name, const QVariant scriptFunction)
{
    QWidget* w = Widgets.value(name, 0);
    QCheckBox* cb = dynamic_cast<QCheckBox*>(w);
    if (!cb)
    {
        abort("Checkbox " + name + " does not exist");
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
        abort("checkboxOnClick() function requires function or its name as the second argument!");
        return;
    }

    QScriptValue func = ScriptManager->getProperty(functionName);
    if (!func.isValid() || !func.isFunction())
    {
        abort("checkboxOnClick() function requires function or its name as the second argument!");
        return;
    }

    connect(cb, &QCheckBox::clicked,
            [=]() { ScriptManager->getProperty(functionName).call(); ScriptManager->ifError_AbortAndReport(); } );
}

void AGui_SI::setMinimumWidth(const QString name, int min)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setMinimumWidth(min);
}

void AGui_SI::setMinimumHeight(const QString name, int min)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setMinimumHeight(min);
}

void AGui_SI::setMaximumWidth(const QString name, int max)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setMaximumWidth(max);
}

void AGui_SI::setMaximumHeight(const QString name, int max)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setMaximumHeight(max);
}

void AGui_SI::setToolTip(const QString name, const QString text)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setToolTip(text);
}

void AGui_SI::addStretch(const QString addTo)
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

void AGui_SI::addHoizontalLine(const QString addTo)
{
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QFrame* e = new QFrame;
    e->setFrameShape(QFrame::HLine);
    e->setFrameShadow(QFrame::Raised);
    lay->addWidget(e);
}

void AGui_SI::addVerticalLine(const QString addTo)
{
    QLayout* lay = Layouts.value(addTo, 0);
    if (!lay)
    {
        abort("Layout " + addTo + " does not exist");
        return;
    }
    QFrame* e = new QFrame;
    e->setFrameShape(QFrame::VLine);
    e->setFrameShadow(QFrame::Raised);
    lay->addWidget(e);
}

void AGui_SI::verticalLayout(const QString name, const QString addTo)
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

void AGui_SI::horizontalLayout(const QString &name, const QString &addTo)
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

void AGui_SI::messageBox(const QString text)
{
    message(text, Wid);
}

void AGui_SI::show()
{
    Wid->show();
}

void AGui_SI::hide()
{
    Wid->hide();
}

void AGui_SI::setWidgetTitle(const QString title)
{
    Wid->setWindowTitle(title);
}

void AGui_SI::resize(int width, int height)
{
    Wid->resize(width, height);
}

void AGui_SI::setEnabled(const QString name, bool flag)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setEnabled(flag);
}

void AGui_SI::setVisible(const QString name, bool flag)
{
    QWidget* w = Widgets.value(name, 0);
    if (!w) abort("Widget " + name + " does not exist");
    else w->setVisible(flag);
}
