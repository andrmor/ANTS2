#ifndef AELEMENTANDISOTOPEDELEGATES_H
#define AELEMENTANDISOTOPEDELEGATES_H

#include <QObject>
#include <QWidget>

class QLineEdit;
class QPushButton;

class AChemicalElement;
class ISotope;

class AChemicalElementDelegate : public QWidget
{
    Q_OBJECT
public:
    AChemicalElementDelegate(AChemicalElement* element, bool* bClearInProgress);
    AChemicalElement* getElement() const {return element;}

private:
    AChemicalElement *element;
    bool* bClearInProgress;

    QPushButton* pbAddIsotope;

public slots:
    void onShowIsotopes(bool flag);

private slots:
    void onAddIsotopeClicked();

signals:
    void AddIsotopeClicked(AChemicalElement *element);

};

/*
class AElasticIsotopeDelegate : public QWidget
{
    Q_OBJECT
public:
    AElasticIsotopeDelegate(AElasticScatterElement* element, bool* bClearInProgress);

    AElasticScatterElement* getElement() const {return element;}

private:
    AElasticScatterElement *element;
    bool* bClearInProgress;

    QLineEdit* leiMass;
    QLineEdit* ledAbund;
    QPushButton* pbLoad;
    QPushButton* pbShow;

    void updateButtons();

private slots:
    void onShowClicked();
    void onLoadClicked();
    void onDelClicked();

    void onChanged();

signals:
    void ShowClicked(const AElasticScatterElement *element);
    void LoadClicked(AElasticScatterElement *element);
    void DelClicked(const AElasticScatterElement *element);
    void RequestActivateModifiedStatus();

};
*/

#endif // AELEMENTANDISOTOPEDELEGATES_H
