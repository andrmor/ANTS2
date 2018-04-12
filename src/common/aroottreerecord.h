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

class ATreeWriterBuffer
{
public:
    void createBranch(TTree* t);
    void fill(const QVariant& val);

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

    QString name;
    QString type;

    static QVector<QString> getAllTypes() {QVector<QString> s; s<<"C"<<"I"<<"F"<<"D"<<"O"<<"AC"<<"AI"<<"AF"<<"AD"<<"AO";return s;}
};

class ARootTreeRecord : public ARootObjBase
{
public:
    ARootTreeRecord(TObject* tree, const QString& name);

    int countBranches() const {return Branches.size();}

    //TObject* GetObject() override;  // unasave for multithread (draw on queued signal), only GUI thread can trigger draw

    // Protected by Mutex
    bool createTree(const QString& name, const QVector<QPair<QString, QString>>& branches);
    bool fillSingle(const QVariantList& vl);

private:
    QVector<ATreeWriterBuffer> Branches;
};

#endif // AROOTTREERECORD_H
