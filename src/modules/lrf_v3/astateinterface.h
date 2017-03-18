#ifndef ASTATEINTERFACE_H
#define ASTATEINTERFACE_H

class QJsonObject;

class AStateInterface {
public:
  virtual void saveState(QJsonObject &state) const = 0;
  virtual void loadState(const QJsonObject &state) = 0;
};

#endif // ASTATEINTERFACE_H
