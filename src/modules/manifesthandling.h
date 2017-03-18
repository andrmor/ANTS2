#ifndef MANIFESTHANDLING_H
#define MANIFESTHANDLING_H

#include <QString>
class TObject;

class ManifestItemBaseClass
{
public:
  QString FileName;
  double X, Y;
  int LimitEvents; //if != -1, only this number of events is loaded from file (unless preprocessing settings is set to even smaller number of events)

  int LineColor, LineWidth, LineStyle;

  bool extractBaseProperties(QString item);
  virtual bool extractProperties(QString item) = 0;
  virtual QString getType() const = 0;
  virtual TObject* makeTObject() = 0;

  ManifestItemBaseClass() : FileName(""), X(0), Y(0), LimitEvents(-1), LineColor(2), LineWidth(2), LineStyle(1) {}
  virtual ~ManifestItemBaseClass() {}
};

class ManifestItemHoleClass : public ManifestItemBaseClass
{
public:
  double Diameter;

  virtual bool extractProperties(QString item);
  virtual QString getType() const {return "hole";}
  virtual TObject *makeTObject();

  ManifestItemHoleClass(double Diameter) : Diameter(Diameter) {}
  ~ManifestItemHoleClass() {}
};

class ManifestItemSlitClass : public ManifestItemBaseClass
{
public:
  double Angle;
  double Length, Width;

  virtual bool extractProperties(QString item);
  virtual QString getType() const {return "slit";}
  virtual TObject *makeTObject();

  ManifestItemSlitClass(double Angle, double Length, double Width) : Angle(Angle), Length(Length), Width(Width) {}
  ~ManifestItemSlitClass() {}
};

class ManifestProcessorClass
{
public:
  ManifestProcessorClass(QString ManifestFile);

  ManifestItemBaseClass *makeManifestItem(QString FileName, QString &ErrorMessage);

private:
  QString ManifestFile;
  ManifestItemBaseClass* ManifestItem; //do not delete this object in destructor -> external ownership or cleaned up autromatically
  double X0, Y0;
  double dX, dY;
  QString Type;
  double Diameter;
  double Angle, Length, Width;

  void calculateCoordinates();
};

#endif // MANIFESTHANDLING_H
