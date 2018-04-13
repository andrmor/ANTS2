#ifndef AROOTTREERECORD_H
#define AROOTTREERECORD_H

#include "arootobjbase.h"

#include <QObject>
#include <QVector>
#include <QPair>
#include <QPair>
#include <QString>
#include <QMutex>

#include "TString.h"

#include <vector>

class TTree;

class ABranchBuffer
{
public:
    void createBranch(TTree* t);
    void fillBranch(const QVariant& val);

    QString name;
    QString type;

    TString C;
    int     I;
    float   F;
    double  D;
    bool    O;
    std::vector<TString> AC;
    std::vector<int>     AI;
    std::vector<float>   AF;
    std::vector<double>  AD;
    std::vector<bool>    AO;

private:
    // fast access selectors
    char cType = '-';
    bool bVector = false;

public:

    static QVector<QString> getAllTypes() {QVector<QString> s; s<<"C"<<"I"<<"F"<<"D"<<"O"<<"AC"<<"AI"<<"AF"<<"AD"<<"AO";return s;}
};

class ARootTreeRecord : public ARootObjBase
{
public:
    ARootTreeRecord(TObject* tree, const QString& name);

    // Protected by Mutex
    bool createTree(const QString& name, const QVector<QPair<QString, QString>>& branches);
    int countBranches() const;
    bool fillSingle(const QVariantList& vl);

private:
    QVector<ABranchBuffer> Branches;
};

#endif // AROOTTREERECORD_H
