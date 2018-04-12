#include "aroottreerecord.h"

#include "TObject.h"
#include "TTree.h"

#include <QVariant>

void ATreeWriterBuffer::createBranch(TTree *t)
{
    TString n = name.toLatin1().data();
    TString f = n + "/" + type.toLatin1().data();
    if      (type == "C") t->Branch(n, &C, f);
    else if (type == "I") t->Branch(n, &I, f);
    else if (type == "F") t->Branch(n, &F, f);
    else if (type == "D") t->Branch(n, &D, f);
}

void ATreeWriterBuffer::fill(const QVariant &val)
{
    if      (type == "C") C = val.toString().toLatin1().data();
    else if (type == "I") I = val.toInt();
    else if (type == "F") F = val.toFloat();
    else if (type == "D") D = val.toDouble();
}

ARootTreeRecord::ARootTreeRecord(TObject *tree, const QString &name) :
    ARootObjBase(tree, name, "") {}

bool ARootTreeRecord::createTree(const QString &name, const QVector<QPair<QString, QString> > &branches)
{
    Branches.resize(branches.size());

    for (int ib = 0; ib < branches.size(); ib++)
    {
        ATreeWriterBuffer& b = Branches[ib];
        b.name = branches.at(ib).first;
        b.type = branches.at(ib).second;
        if (!ATreeWriterBuffer::getAllTypes().contains(b.type))
            return false;
    }

    TTree* t = new TTree(name.toLatin1().data(), "");

    if (Object) delete Object;
    Object = t;

    for (int ib = 0; ib < branches.size(); ib++)
    {
        ATreeWriterBuffer& b = Branches[ib];
        b.createBranch(t);
    }

    return true;
}

bool ARootTreeRecord::fillSingle(const QVariantList &vl)
{
    if (vl.size() != Branches.size()) return false;

    for (int ib = 0; ib < Branches.size(); ib++)
    {
        ATreeWriterBuffer& b = Branches[ib];
        b.fill(vl.at(ib));
    }
    static_cast<TTree*>(Object)->Fill();
}


