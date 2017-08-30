#ifndef AELEMENTANDISOTOPEDELEGATES_H
#define AELEMENTANDISOTOPEDELEGATES_H

#include <QObject>
#include <QWidget>
#include <QPoint>

class QLineEdit;
class QPushButton;

class AChemicalElement;
class ISotope;

class AChemicalElementDelegate : public QWidget
{
    Q_OBJECT
public:
    AChemicalElementDelegate(AChemicalElement* element, bool* bClearInProgress,  bool IsotopesShown);
    AChemicalElement* getElement() const {return element;}

private:
    AChemicalElement *element;
    bool* bClearInProgress;
    bool bIsotopesShown;

private slots:
    void onMenuRequested(const QPoint &pos);

signals:
    void AddIsotopeActivated(AChemicalElement *element);

};

class AIsotopeDelegate : public QWidget
{
    Q_OBJECT
public:
    AIsotopeDelegate(AChemicalElement* element, int isotopeIndexInElement, bool* bClearInProgress);

private:
    AChemicalElement *element;
    bool* bClearInProgress;
    int isotopeIndexInElement;

    QLineEdit* leiMass;
    QLineEdit* ledAbund;

private slots:
    void onChanged();
    void onMenuRequested(const QPoint &pos);

signals:    
    void RemoveIsotope(AChemicalElement* element, int isotopeIndexInElement);
    void IsotopePropertiesChanged(const AChemicalElement* element, int isotopeIndexInElement);

};

#endif // AELEMENTANDISOTOPEDELEGATES_H
