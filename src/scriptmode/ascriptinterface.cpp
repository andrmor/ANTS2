#include "ascriptinterface.h"

AScriptInterface::AScriptInterface() :
    QObject() {}

AScriptInterface::AScriptInterface(const AScriptInterface &other) :
    QObject(), H(other.H), Description(other.Description), bGuiThread(false) {}

const QString AScriptInterface::getDescription() const
{
    return Description + (IsMultithreadCapable() ? "\nMultithread-capable" : "");
}

const QHash<QString, QString> &AScriptInterface::getDeprecatedOrRemovedMethods() const
{
    return DepRem;
}

/*
const QStringList AScriptInterface::getDeprecatedOrRemovedMethods() const
{
    QStringList sl;

    QHashIterator<const QString, QString> iter(DepRem);
    while (iter.hasNext())
    {
        iter.next();
        sl << iter.key();
    }

    return sl;
}
*/

const QString AScriptInterface::help(QString method) const  //automatically requested to obtain help strings
{
    if (method.endsWith("()")) method.remove("()");
    if (method.endsWith("(")) method.remove("(");
    if (!H.contains(method)) return "";
    return H[method];
}

void AScriptInterface::abort(QString message) {emit AbortScriptEvaluation(message);}
