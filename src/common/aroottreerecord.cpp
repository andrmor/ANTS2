#include "aroottreerecord.h"

#include "TObject.h"
#include "TTree.h"

#include <QVariant>
#include <QDebug>

void ABranchBuffer::createBranch(TTree *t)
{
    TString n = name.toLatin1().data();
    TString f = n + "/" + type.toLatin1().data();

    if (type.length() == 1)
    {
        bVector = false;
        cType   = type.at(0).toLatin1();

        switch (cType)
        {
            case 'C' : t->Branch(n, &C, f); break;
            case 'I' : t->Branch(n, &I, f); break;
            case 'F' : t->Branch(n, &F, f); break;
            case 'D' : t->Branch(n, &D, f); break;
            case 'O' : t->Branch(n, &O, f); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
    else
    {
        bVector = true;
        cType   = type.at(1).toLatin1();

        switch (cType)
        {
            case 'C' : t->Branch(n, &AC); break;
            case 'I' : t->Branch(n, &AI); break;
            case 'F' : t->Branch(n, &AF); break;
            case 'D' : t->Branch(n, &AD); break;
            case 'O' : t->Branch(n, &AO); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
}

void ABranchBuffer::fillBranch(const QVariant &val)
{
    if (!bVector)
    {
        switch (cType)
        {
            case 'C' : C = val.toString().toLatin1().data(); break;
            case 'I' : I = val.toInt();    break;
            case 'F' : F = val.toFloat();  break;
            case 'D' : D = val.toDouble(); break;
            case 'O' : O = val.toBool();   break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
    else
    {
        const QVariantList vl = val.toList();
        switch (cType)
        {
            case 'C' : AC.resize(vl.size()); for (int i=0; i<vl.size(); i++) AC[i] = vl.at(i).toString().toLatin1().data(); break;
            case 'I' : AI.resize(vl.size()); for (int i=0; i<vl.size(); i++) AI[i] = vl.at(i).toInt();    break;
            case 'F' : AF.resize(vl.size()); for (int i=0; i<vl.size(); i++) AF[i] = vl.at(i).toFloat();  break;
            case 'D' : AD.resize(vl.size()); for (int i=0; i<vl.size(); i++) AD[i] = vl.at(i).toDouble(); break;
            case 'O' : AO.resize(vl.size()); for (int i=0; i<vl.size(); i++) AO[i] = vl.at(i).toBool();   break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
}

ARootTreeRecord::ARootTreeRecord(TObject *tree, const QString &name) :
    ARootObjBase(tree, name, "") {}

bool ARootTreeRecord::createTree(const QString &name, const QVector<QPair<QString, QString> > &branches)
{
    Branches.resize(branches.size());

    for (int ib = 0; ib < branches.size(); ib++)
    {
        ABranchBuffer& b = Branches[ib];
        b.name = branches.at(ib).first;
        b.type = branches.at(ib).second;
        if (!ABranchBuffer::getAllTypes().contains(b.type))
            return false;
    }

    TTree* t = new TTree(name.toLatin1().data(), "");

    if (Object) delete Object;
    Object = t;

    for (int ib = 0; ib < branches.size(); ib++)
    {
        ABranchBuffer& b = Branches[ib];
        b.createBranch(t);
    }

    return true;
}

bool ARootTreeRecord::fillSingle(const QVariantList &vl)
{
    if (vl.size() != Branches.size()) return false;

    for (int ib = 0; ib < Branches.size(); ib++)
    {
        ABranchBuffer& b = Branches[ib];
        b.fillBranch(vl.at(ib));
    }
    static_cast<TTree*>(Object)->Fill();

    return true;
}


