#ifndef SCRIPTVALUECOPIER_H
#define SCRIPTVALUECOPIER_H

#include <QMap>
#include <QScriptValue>

class AScriptValueCopier
{
  QScriptEngine &m_toEngine;
  QMap<quint64, QScriptValue> copiedObjs;

  void copyProperties(QScriptValue &dest, QScriptValue obj);
public:
  AScriptValueCopier(QScriptEngine &toEngine) : m_toEngine(toEngine) {}

  QScriptValue copy(const QScriptValue &obj);
};

#endif // SCRIPTVALUECOPIER_H
