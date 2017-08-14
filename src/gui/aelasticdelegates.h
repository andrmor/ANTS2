#ifndef AELASTICDELEGATES_H
#define AELASTICDELEGATES_H

#include <QWidget>
#include <QObject>
#include <QString>

class AElasticScatterElement;
class QLineEdit;
class QPushButton;

class AElasticElementDelegate : public QWidget
{
    Q_OBJECT
public:
    AElasticElementDelegate(AElasticScatterElement* element);

private:
    AElasticScatterElement *element;

    QLineEdit* leName;
    QLineEdit* ledFraction;

private slots:
    void onAutoClicked();
    void onDelClicked();

    void onChanged();

signals:
    void AutoClicked(AElasticScatterElement *element);
    void DelClicked(AElasticScatterElement *element);
    void RequestUpdateIsotopes(const AElasticScatterElement* element, const QString newName, double newFraction);

};

class AElasticIsotopeDelegate : public QWidget
{
    Q_OBJECT
public:
    AElasticIsotopeDelegate(AElasticScatterElement* element);

private:
    AElasticScatterElement *element;

    QLineEdit* leiMass;
    QLineEdit* ledAbund;
    QPushButton* pbLoad;
    QPushButton* pbShow;

    void updateButtons();
    void updateToolTip();

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


#endif // AELASTICDELEGATES_H
