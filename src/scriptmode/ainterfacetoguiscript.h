#ifndef AINTERFACETOGUISCRIPT_H
#define AINTERFACETOGUISCRIPT_H

#include "ascriptinterface.h"

#include <QString>
#include <QVariant>
#include <QMap>

class AJavaScriptManager;
class QWidget;
class QLayout;

class AInterfaceToGuiScript : public AScriptInterface
{
    Q_OBJECT
public:
    AInterfaceToGuiScript(AJavaScriptManager* ScriptManager);
    ~AInterfaceToGuiScript();

    bool           InitOnRun() override;

public slots:
    void buttonNew(const QString name, const QString addTo, const QString text = "");
    void buttonOnClick(const QString name, const QVariant scriptFunction);

    void labelNew(const QString name, const QString addTo, const QString text);
    void labelSetText(const QString name, const QString labelText);

    void editNew(const QString name, const QString addTo, const QString text = "");
    void editSetText(const QString name, const QString text);
    const QString editGetText(const QString name);

    void comboboxNew(const QString name, const QString addTo, bool Editable = false);
    void comboboxAppend(const QString name, const QVariant entries);
    void comboboxClear(const QString name);
    const QString comboboxGetSelected(const QString name);

    void textNew(const QString name, const QString addTo, const QString text = "");
    void textClear(const QString name);
    void textAppendPlainText(const QString name, const QString text);
    void textAppendHtml(const QString name, const QString text);
    const QString textGet(const QString name);

    void addStretch(const QString addTo);

    void verticalLayout(const QString name, const QString addTo);
    void horizontalLayout(const QString& name, const QString& addTo);

    void messageBox(const QString text);

    void show();
    void hide();
    void setWidgetTitle(const QString title = "");

    void setEnabled(const QString name, bool flag);
    void setVisible(const QString name, bool flag);

private:
    AJavaScriptManager *ScriptManager;
    QWidget* Wid = 0;
    QMap<QString, QWidget*> Widgets;
    QMap<QString, QLayout*> Layouts;

    void init();
};

#endif // AINTERFACETOGUISCRIPT_H
