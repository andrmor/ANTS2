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
class TBranch;

class ABranchBuffer
{
public:
    void createBranch(TTree* t);
    void reconnectAddresses(TTree* t);

    void write(const QVariant& val);
    const QVariant read();

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

    // fast access selectors
    char cType = '-';
    bool bVector = false;
    TBranch* branchPtr = 0;

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
    bool isBranchExist(const QString& branchName) const;
    const QVariantList getBranch(const QString& branchName);
    const QVariant     getBranch(const QString& branchName, int entry);
    void save(const QString &FileName);

    //not protected by Mutex
    void reconnectBranchAddresses();

private:
    QVector<ABranchBuffer> Branches;

private:
    ABranchBuffer* getBranchBuffer(const QString& name);
};

#endif // AROOTTREERECORD_H
